STATIC_ASSERT(ALIGNOF(U64) == 8);
STATIC_ASSERT(ALIGNOF(U32) == 4);
STATIC_ASSERT(ALIGNOF(U16) == 2);
STATIC_ASSERT(ALIGNOF(U8)  == 1);

inline void*
Align(void* ptr, U8 alignment)
{
    STATIC_ASSERT(sizeof(UMM) == sizeof(void*));
    
    UMM overshot     = (UMM)ptr + (alignment - 1);
    UMM rounded_down = overshot & ~(alignment - 1);
    return (void*)rounded_down;
}

inline U8
AlignOffset(void* ptr, U8 alignment)
{
    STATIC_ASSERT(sizeof(UMM) == sizeof(void*));
    
    UMM overshot = (UMM)ptr + (alignment - 1);
    UMM aligned  = overshot & ~(alignment - 1);
    
    return (U8)(aligned - (UMM)ptr);
}

inline void
Copy(void* src, void* dst, UMM size)
{
    U8* b_src = (U8*)src;
    U8* b_dst = (U8*)dst;
    
    if (b_src < b_dst)
    {
        for (UMM i = size; i > 0; --i)
        {
            b_dst[i - 1] = b_src[i - 1];
        }
    }
    
    else if (b_src > b_dst)
    {
        for (UMM i = 0; i < size; ++i)
        {
            b_dst[i] = b_src[i];
        }
    }
}

inline void
Zero(void* ptr, UMM size)
{
    U8* b_ptr = (U8*)ptr;
    
    for (UMM i = 0; i < size; ++i)
    {
        b_ptr[i] = 0;
    }
}

#define ZeroStruct(S) Zero((S), sizeof((S)[0]))

//////////////////////////////////////////

typedef struct Memory_Block_Free_Entry
{
    struct Memory_Block_Free_Entry* next;
    U8 offset;
    U32 size;
} Memory_Block_Free_Entry;

#define MEMORY_ARENA_MIN_ALLOCATION_SIZE sizeof(Memory_Block_Free_Entry) + (ALIGNOF(Memory_Block_Free_Entry) - 1)
STATIC_ASSERT(MEMORY_ARENA_MIN_ALLOCATION_SIZE >= BYTES(16));

typedef struct Memory_Block
{
    struct Memory_Block* next;
    U8* memory;
    U32 offset;
    U32 space;
} Memory_Block;

typedef struct Memory_Arena
{
    Memory_Block_Free_Entry* free_list;
    Memory_Block* first_block;
    Memory_Block* current_block;
    struct Memory_Allocator* allocator;
} Memory_Arena;

typedef struct Memory_Allocator
{
    Memory_Block* first_block;
    Memory_Block* current_block;
    U32 block_count;
    U32 page_size;
    U32 block_size; // NOTE(soimn): page_size - sizeof(Memory_Block)
    Mutex mutex;
} Memory_Allocator;

//////////////////////////////////////////

Memory_Block*
MemoryAllocator_AllocateBlock(Memory_Allocator* allocator)
{
    TryLockMutex(allocator->mutex);
    
    if (allocator->page_size == 0)
    {
        allocator->block_size = allocator->page_size - sizeof(Memory_Block);
    }
    
    // TODO(soimn): replace this with a page fetch
    void* memory = Malloc(allocator->page_size + ALIGNOF(Memory_Block));
    
    Memory_Block* new_block = Align(memory, ALIGNOF(Memory_Block));
    new_block->next   = 0;
    new_block->memory = memory;
    new_block->offset = AlignOffset(memory, ALIGNOF(Memory_Block)) + sizeof(Memory_Block);
    new_block->space  = allocator->block_size;
    
    if (allocator->current_block) allocator->current_block->next = new_block;
    else                          allocator->first_block         = new_block;
    
    allocator->current_block = new_block;
    allocator->block_count  += 1;
    
    UnlockMutex(allocator->mutex);
    
    return new_block;
}

void
MemoryAllocator_FreeBlock(Memory_Allocator* allocator, Memory_Block* block)
{
    TryLockMutex(allocator->mutex);
    
    Memory_Block* prev_scan = 0;
    Memory_Block* scan      = allocator->first_block;
    while (scan)
    {
        if (scan == block) break;
        else
        {
            prev_scan = scan;
            scan      = scan->next;
        }
    }
    
    ASSERT(scan != 0);
    
    if (prev_scan) prev_scan->next = scan->next;
    else allocator->first_block    = scan->next;
    
    Free(block->memory);
    
    UnlockMutex(allocator->mutex);
}

//////////////////////////////////////////

void
MemoryArena_FreeSize(Memory_Arena* arena, void* ptr, U32 size)
{
    if (size >= MEMORY_ARENA_MIN_ALLOCATION_SIZE)
    {
        Memory_Block_Free_Entry* free_entry = Align(ptr, ALIGNOF(Memory_Block_Free_Entry));
        free_entry->offset = AlignOffset(ptr, ALIGNOF(Memory_Block_Free_Entry));
        free_entry->size   = size - free_entry->offset;
    }
}

void*
MemoryArena_AllocSize(Memory_Arena* arena, UMM umm_size, U8 alignment)
{
    ASSERT(umm_size + alignment <= arena->allocator->block_size);
    ASSERT(IsPowerOf2(alignment) && alignment <= ALIGNOF(U64));
    
    void* result = 0;
    
    U32 size = (U32)MAX(umm_size, MEMORY_ARENA_MIN_ALLOCATION_SIZE - (alignment - 1));
    
    Memory_Block_Free_Entry* prev_scan = 0;
    Memory_Block_Free_Entry* scan      = arena->free_list;
    
    while (scan)
    {
        if (scan->offset + scan->size >= size + alignment)
        {
            if (prev_scan != 0) prev_scan->next = scan->next;
            else             arena->free_list   = scan->next;
            
            void* base_ptr = (U8*)scan - scan->offset;
            result = Align(base_ptr, alignment);
            
            MemoryArena_FreeSize(arena, (U8*)result + size,
                                 (scan->offset + scan->size) - AlignOffset(base_ptr, alignment));
            break;
        }
        
        prev_scan = scan;
        scan      = scan->next;
    }
    
    // NOTE(soimn): If no free entries of an appropriate size were found, allocate a new one
    if (!result)
    {
        if (!arena->current_block || arena->current_block->space < size + alignment)
        {
            Memory_Block* new_block = MemoryAllocator_AllocateBlock(arena->allocator);
            
            if (arena->current_block) arena->current_block->next = new_block;
            else                      arena->first_block         = new_block;
            
            arena->current_block = new_block;
        }
        
        void* base_ptr = arena->current_block->memory + arena->current_block->offset;
        result                        = Align(base_ptr, alignment);
        arena->current_block->offset += AlignOffset(base_ptr, alignment) + size;
    }
    
    return result;
}

void
MemoryArena_FreeAll(Memory_Arena* arena)
{
    Memory_Block* scan = arena->first_block;
    while (scan)
    {
        Memory_Block* scan_next = scan->next;
        MemoryAllocator_FreeBlock(arena->allocator, scan);
        scan = scan_next;
    }
    
    arena->first_block   = 0;
    arena->current_block = 0;
    arena->free_list     = 0;
}

//////////////////////////////////////////


/// Storage data structures

// NOTE(soimn): This is segmented into an array and an array header to maintain memory compatibility with native 
//              arrays in Gremlin
typedef struct Array
{
    void* data;
    U32 size;
    U32 capacity;
} Array;

#define Array(T) Array
#define ARRAY_INIT(arena, T, cap) Array_Init((arena), sizeof(T), (cap))

typedef struct Array_Header
{
    Memory_Arena* arena;
    U32 element_size;
} Array_Header;

inline Array
Array_Init(Memory_Arena* arena, UMM element_size, UMM capacity)
{
    ASSERT(element_size < U32_MAX);
    ASSERT(capacity     < U32_MAX);
    
    STATIC_ASSERT(sizeof(Array_Header) % 8 == 0);
    void* memory = MemoryArena_AllocSize(arena, sizeof(Array_Header) + element_size * capacity, ALIGNOF(U64));
    
    Array_Header* header = memory;
    header->arena        = arena;
    header->element_size = (U32)element_size;
    
    Array array = {
        .data     = (U8*)memory + sizeof(Array_Header),
        .size     = 0,
        .capacity = (U32)capacity
    };
    
    return array;
}

void
Array_Reserve(Array* array, UMM capacity)
{
    ASSERT(capacity < U32_MAX);
    
    if (array->capacity < capacity)
    {
        Array_Header* header = (Array_Header*)((U8*)array->data - sizeof(Array_Header));
        
        void* new_memory = MemoryArena_AllocSize(header->arena,
                                                 sizeof(Array_Header) + header->element_size * capacity,
                                                 ALIGNOF(U64));
        
        Array_Header* new_header = new_memory;
        new_header->arena        = header->arena;
        new_header->element_size = header->element_size;
        
        Copy(array->data, (U8*)new_memory + sizeof(Array_Header), array->size * header->element_size);
        
        array->data     = (U8*)new_memory + sizeof(Array_Header);
        array->capacity = (U32)capacity;
    }
}

void*
Array_Append(Array* array)
{
    if (array->size == array->capacity)
    {
        Array_Reserve(array, array->capacity * 2 + 1);
    }
    
    Array_Header* header = (Array_Header*)((U8*)array->data - sizeof(Array_Header));
    
    void* result = (U8*)array->data + array->size * header->element_size;
    array->size += 1;
    
    return result;
}

void*
Array_ElementAt(Array* array, UMM index)
{
    ASSERT(index < array->size);
    
    Array_Header* header = (Array_Header*)((U8*)array->data - sizeof(Array_Header));
    
    void* result = (U8*)array->data + index * header->element_size;
    
    return result;
}

void
Array_UnorderedRemove(Array* array, UMM index)
{
    Array_Header* header = (Array_Header*)((U8*)array->data - sizeof(Array_Header));
    
    Copy(Array_ElementAt(array, array->size - 1), Array_ElementAt(array, index), header->element_size);
    
    array->size -= 1;
}

void
Array_OrderedRemove(Array* array, UMM index)
{
    Array_Header* header = (Array_Header*)((U8*)array->data - sizeof(Array_Header));
    
    Copy(Array_ElementAt(array, index + 1), Array_ElementAt(array, index),
         (array->size - (index + 1)) * header->element_size);
    
    array->size -= 1;
}

void
Array_RemoveAll(Array* array)
{
    Array_Header* header = (Array_Header*)((U8*)array->data - sizeof(Array_Header));
    
    MemoryArena_FreeSize(header->arena, array->data, array->capacity * header->element_size);
}

void
Array_RemoveAllAndFreeHeader(Array* array)
{
    Array_RemoveAll(array);
    
    Array_Header* header = (Array_Header*)((U8*)array->data - sizeof(Array_Header));
    MemoryArena_FreeSize(header->arena, header, sizeof(Array_Header));
    
    array->data     = 0;
    array->size     = 0;
    array->capacity = 0;
}

//////////////////////////////////////////

typedef struct Bucket_Array
{
    Memory_Arena* arena;
    void* first_bucket;
    void* current_bucket;
    U32 current_bucket_size;
    U32 element_size;
    U32 bucket_size;
    U32 bucket_count;
} Bucket_Array;

#define Bucket_Array(T) Bucket_Array
#define BUCKET_ARRAY_INIT(arena, T, bucket_size) BucketArray_Init((arena), sizeof(T), (bucket_size))

inline Bucket_Array
BucketArray_Init(Memory_Arena* arena, UMM element_size, UMM bucket_size)
{
    ASSERT(element_size < U32_MAX);
    ASSERT(bucket_size  < U32_MAX);
    
    Bucket_Array array = {
        .arena          = arena,
        .first_bucket   = 0,
        .current_bucket = 0,
        .element_size   = (U32)element_size,
        .bucket_size    = (U32)bucket_size,
        .bucket_count   = 0
    };
    
    return array;
}

void*
BucketArray_Append(Bucket_Array* array)
{
    if (array->current_bucket_size == array->bucket_size)
    {
        UMM memory_size = sizeof(U64) + array->bucket_size * array->element_size;
        void* memory    = MemoryArena_AllocSize(array->arena, memory_size, ALIGNOF(U64));
        Zero(memory, memory_size);
        
        if (array->current_bucket) *(void**)array->current_bucket = memory;
        else                                  array->first_bucket = memory;
        
        array->current_bucket = memory;
        array->bucket_count  += 1;
    }
    
    void* result = (U8*)array->current_bucket + sizeof(U64) + array->current_bucket_size * array->element_size;
    array->current_bucket_size += 1;
    
    return result;
}

U32
BucketArray_ElementCount(Bucket_Array* array)
{
    return array->bucket_count * array->bucket_size + array->current_bucket_size;
}

void*
BucketArray_ElementAt(Bucket_Array* array, UMM index)
{
    ASSERT(index < BucketArray_ElementCount(array));
    
    U32 bucket_index  = (U32)index / array->bucket_size;
    U32 bucket_offset = (U32)index % array->bucket_size;
    
    void* bucket = array->first_bucket;
    for (U32 i = 0; i < bucket_index; ++i) bucket = *(void**)bucket;
    
    void* result = (U8*)bucket + sizeof(U64) + bucket_offset * array->element_size;
    
    return result;
}

void
BucketArray_UnorderedRemove(Bucket_Array* array, UMM index)
{
    ASSERT(index < BucketArray_ElementCount(array));
    
    Copy(BucketArray_ElementAt(array, BucketArray_ElementCount(array) - 1), BucketArray_ElementAt(array, index),
         array->element_size);
    
    array->current_bucket_size -= 1;
    
    if (array->current_bucket_size == 0)
    {
        void* prev_scan = 0;
        void* scan      = array->first_bucket;
        while (scan != array->current_bucket)
        {
            prev_scan = scan;
            scan      = *(void**)scan;
        }
        
        if (prev_scan != 0)
        {
            *(void**)prev_scan = 0;
            array->current_bucket = prev_scan;
        }
        
        else
        {
            array->first_bucket   = 0;
            array->current_bucket = 0;
        }
        
        array->bucket_count -= 1;
        array->current_bucket_size = array->bucket_size;
        
        MemoryArena_FreeSize(array->arena, scan, sizeof(U64) + array->bucket_size * array->element_size);
    }
}

void
BucketArray_FreeAll(Bucket_Array* array)
{
    void* scan = array->first_bucket;
    while (scan)
    {
        void* scan_next = *(void**)scan;
        
        MemoryArena_FreeSize(array->arena, scan, sizeof(U64) + array->bucket_size * array->element_size);
        
        scan = scan_next;
    }
    
    array->first_bucket        = 0;
    array->current_bucket      = 0;
    array->current_bucket_size = 0;
    array->bucket_count        = 0;
}

typedef struct Bucket_Array_Iterator
{
    void* current_bucket;
    
    UMM current_index;
    void* current;
    
    U32 last_bucket_size;
    U32 element_size;
    U32 bucket_size;
    U32 remaining_bucket_count;
} Bucket_Array_Iterator;

inline Bucket_Array_Iterator
BucketArray_Iterate(Bucket_Array* array)
{
    Bucket_Array_Iterator iterator = {
        .current_bucket         = array->first_bucket,
        .last_bucket_size       = array->current_bucket_size,
        .element_size           = array->element_size,
        .bucket_size            = array->bucket_size,
        .remaining_bucket_count = array->bucket_count
    };
    
    iterator.current_index = 0;
    iterator.current       = 0;
    
    if (array->first_bucket)
    {
        iterator.current = BucketArray_ElementAt(array, 0);
    }
    
    return iterator;
}

inline void
BucketArrayIterator_Advance(Bucket_Array_Iterator* iterator)
{
    // NOTE(soimn): If iterator->current is 0 Advance was called with in invalid iterator
    ASSERT(iterator->current != 0);
    
    iterator->current = 0;
    
    ++iterator->current_index;
    
    UMM bucket_offset = iterator->current_index % iterator->bucket_size;
    
    if (bucket_offset == 0)
    {
        iterator->remaining_bucket_count = iterator->remaining_bucket_count - 1;
        iterator->current_bucket = *(void**)iterator->current_bucket;
    }
    
    if (iterator->remaining_bucket_count == 0 ||
        iterator->remaining_bucket_count == 1 && bucket_offset >= iterator->last_bucket_size)
    {
        iterator->current = 0;
    }
    
    else
    {
        iterator->current = (U8*)iterator->current_bucket + sizeof(U64) + bucket_offset * iterator->element_size;
    }
}