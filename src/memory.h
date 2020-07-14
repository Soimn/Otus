inline void*
Align(void* ptr, U8 alignment)
{
    UMM overshot     = (UMM)ptr + (alignment - 1);
    UMM rounded_down = overshot & ~(alignment - 1);
    return (void*)rounded_down;
}

inline U8
AlignOffset(void* ptr, U8 alignment)
{
    UMM overshot     = (UMM)ptr + (alignment - 1);
    UMM rounded_down = overshot & ~(alignment - 1);
    
    return (U8)(overshot - rounded_down);
}

inline void
Zero(void* ptr, UMM size)
{
    for (UMM i = 0; i < size; ++i)
    {
        *((U8*)ptr + i) = 0;
    }
}

#define ZeroStruct(ptr) Zero(ptr, sizeof(ptr[0]))

inline void
Copy(void* src, void* dst, UMM size)
{
    U8* bsrc = src;
    U8* bdst = dst;
    
    if (bsrc < bdst)
    {
        for (UMM i = 0; i < size; ++i)
        {
            bdst[size - i] = bsrc[size - i];
        }
    }
    
    else
    {
        for (UMM i = 0; i < size; ++i)
        {
            bdst[i] = bsrc[i];
        }
    }
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

typedef struct Memory_Allocator
{
    void* first_page;
    void* current_page;
    Mutex mutex;
} Memory_Allocator;

void*
Allocator_AllocatePage(Memory_Allocator* allocator, UMM page_size)
{
    Mutex_Lock(allocator->mutex);
    
    void* page = System_AllocateMemory(page_size + sizeof(U64));
    ASSERT(page != 0); // TODO(soimn): Handle Out of Memory errors more graciously than an assert
    
    *(void**)page = 0;
    
    if (allocator->first_page) *(void**)allocator->current_page = page;
    else allocator->first_page                                  = page;
    
    allocator->current_page = page;
    
    Mutex_Unlock(allocator->mutex);
    
    return (U8*)page + sizeof(U64);
}

void
Allocator_FreePage(Memory_Allocator* allocator, void* memory)
{
    void* page = (U8*)memory - sizeof(U64);
    
    Mutex_Lock(allocator->mutex);
    
    void* prev_page = allocator->first_page;
    while (prev_page && prev_page != page && *(void**)prev_page != page)
    {
        prev_page = *(void**)prev_page;
    }
    
    ASSERT(prev_page != 0);
    
    *(void**)prev_page = *(void**)page;
    
    if (page == allocator->first_page) allocator->first_page     = *(void**)page;
    if (page == allocator->current_page) allocator->current_page = prev_page;
    
    System_FreeMemory(page);
    
    Mutex_Unlock(allocator->mutex);
}

void
Allocator_FreeAll(Memory_Allocator* allocator)
{
    Mutex_Lock(allocator->mutex);
    
    void* current_page = allocator->first_page;
    while (current_page != 0)
    {
        void* next_page = *(void**)current_page;
        
        System_FreeMemory(current_page);
        current_page = next_page;
    }
    
    Mutex_Unlock(allocator->mutex);
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

#define OTUS_MEMORY_BLOCK_SIZE KILOBYTES(4) - (sizeof(U64) + sizeof(Memory_Block))

typedef struct Memory_Block
{
    struct Memory_Block* next;
    U16 offset;
    U16 space;
} Memory_Block;

typedef struct Memory_Arena_Free_Entry
{
    struct Memory_Arena_Free_Entry* next;
    U16 size;
    U8 offset;
} Memory_Arena_Free_Entry;

typedef struct Memory_Arena
{
    Memory_Allocator* allocator;
    Memory_Arena_Free_Entry* first_free;
    Memory_Block* first_block;
    Memory_Block* current_block;
} Memory_Arena;

#define MEMORY_ARENA_INIT(A) Arena_Init(A)
Memory_Arena
Arena_Init(Memory_Allocator* allocator)
{
    Memory_Arena arena = { .allocator = (allocator) };
    return arena;
}

void
Arena_FreeSize(Memory_Arena* arena, void* ptr, UMM size)
{
    ASSERT(size != 0);
    ASSERT(((UMM)ptr - ((UMM)ptr & ~(ALIGNOF(Memory_Block) - 1))) + size <= OTUS_MEMORY_BLOCK_SIZE);
    
    if (size >= sizeof(Memory_Arena_Free_Entry) + (ALIGNOF(Memory_Arena_Free_Entry) - 1))
    {
        // NOTE(soimn): Make sure the memory is owned by the allocator
#ifdef OTUS_DEBUG_MODE
        Memory_Block* scan = arena->first_block;
        while (scan != 0)
        {
            if (ptr >= (U8*)scan + sizeof(Memory_Block) && ptr < (U8*)scan + scan->offset)
            {
                break;
            }
            
            else scan = *(void**)scan;
        }
        
        ASSERT(scan != 0);
#endif
        
        Memory_Arena_Free_Entry* new_entry = Align(ptr, ALIGNOF(Memory_Arena_Free_Entry));
        new_entry->next   = 0;
        new_entry->offset = AlignOffset(ptr, ALIGNOF(Memory_Arena_Free_Entry));
        new_entry->size   = (U16)size - new_entry->offset;
    }
}

void*
Arena_AllocateSize(Memory_Arena* arena, UMM size, U8 alignment)
{
    ASSERT(alignment == 1 || alignment == 2 || alignment == 4 || alignment == 8);
    ASSERT(size != 0 && size + (alignment - 1) < OTUS_MEMORY_BLOCK_SIZE);
    
    void* result = 0;
    
    if (arena->first_free)
    {
        Memory_Arena_Free_Entry* scan = arena->first_free;
        while (scan != 0)
        {
            if (scan->offset + scan->size >= size + (alignment - 1))
            {
                result = Align((U8*)scan - scan->offset, alignment);
                
                Arena_FreeSize(arena, (U8*)result + size, (U16)(scan->size - (size + ((U8*)result - (U8*)scan))));
            }
        }
    }
    
    if (result == 0)
    {
        if (arena->first_block == 0 || arena->current_block->space < (U16)(size + (alignment - 1)))
        {
            if (arena->first_block)
            {
                Arena_FreeSize(arena, (U8*)arena->current_block + arena->current_block->offset,
                               arena->current_block->space);
            }
            
            Memory_Block* new_block = Allocator_AllocatePage(arena->allocator, OTUS_MEMORY_BLOCK_SIZE);
            new_block->next   = 0;
            new_block->offset = sizeof(Memory_Block);
            new_block->space  = OTUS_MEMORY_BLOCK_SIZE;
            
            if (arena->first_block == 0) arena->first_block = new_block;
            else arena->current_block->next                 = new_block;
            
            arena->current_block = new_block;
        }
        
        result = Align((U8*)arena->current_block + arena->current_block->offset, alignment);
        
        arena->current_block->offset += AlignOffset(result, alignment);
        arena->current_block->space  -= AlignOffset(result, alignment);
    }
    
    return result;
}

void
Arena_FreeAll(Memory_Arena* arena)
{
    Memory_Block* block = arena->first_block;
    while (block)
    {
        Memory_Block* next_block = block->next;
        
        Allocator_FreePage(arena->allocator, block);
        
        block = next_block;
    }
    
    arena->first_free    = 0;
    arena->first_block   = 0;
    arena->current_block = 0;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

typedef struct Bucket_Array
{
    Memory_Arena* arena;
    void* first_free;
    void* first_bucket;
    void* current_bucket;
    U16 current_bucket_size;
    U16 bucket_size;
    U16 element_size;
    U16 bucket_count;
} Bucket_Array;

#define Bucket_Array(E) Bucket_Array
#define BUCKET_ARRAY_INIT(arena, T, bucket_size) BucketArrayInit((arena), sizeof(((T*)0)[0]), (bucket_size))

Bucket_Array
BucketArrayInit(Memory_Arena* arena, UMM element_size, UMM bucket_size)
{
    ASSERT(arena != 0);
    ASSERT(element_size != 0 && (F32)element_size * (F32)bucket_size <= OTUS_MEMORY_BLOCK_SIZE);
    
    Bucket_Array array = {
        .bucket_size  = (U16)bucket_size,
        .element_size = (U16)MAX(sizeof(U64), element_size)
    };
    
    return array;
}

void*
BucketArray_Append(Bucket_Array* array)
{
    void* result = 0;
    
    if (array->first_free)
    {
        result = array->first_free;
        
        array->first_free = *(void**)array->first_free;
    }
    
    else
    {
        if (array->bucket_count == 0 || array->current_bucket_size == array->bucket_size)
        {
            void* new_bucket = Arena_AllocateSize(array->arena,
                                                  sizeof(U64) + array->element_size * array->bucket_size,
                                                  ALIGNOF(U64));
            
            if (array->bucket_count == 0) array->first_bucket = new_bucket;
            else *(void**)array->current_bucket               = new_bucket;
            
            array->current_bucket      = new_bucket;
            array->current_bucket_size = 0;
            array->bucket_count       += 1;
        }
        
        result = (U8*)array->current_bucket + sizeof(U64) + array->element_size * array->current_bucket_size;
        
        array->bucket_size += 1;
    }
    
    return result;
}

void
BucketArray_Free(Bucket_Array* array, void* ptr)
{
#ifdef OTUS_DEBUG_MODE
    void* scan = array->first_bucket;
    while (scan != 0)
    {
        if (ptr >= (U8*)scan + sizeof(U64) &&
            ptr < (U8*)scan + sizeof(U64) array->element_size * array->bucket_size)
        {
            break;
        }
        
        else scan = *(void**)scan;
    }
    
    ASSERT(scan != 0);
#endif
    
    *(void**)ptr      = array->first_free;
    array->first_free = ptr;
}

void
BucketArray_FreeAll(Bucket_Array* array)
{
    void* bucket = array->first_bucket;
    while (bucket != 0)
    {
        void* next_bucket = *(void**)bucket;
        
        Arena_FreeSize(array->arena, bucket, sizeof(U64) + array->element_size * array->bucket_size);
        bucket = next_bucket;
    }
    
    array->first_bucket        = 0;
    array->current_bucket      = 0;
    array->first_free          = 0;
    array->bucket_count        = 0;
    array->current_bucket_size = 0;
}

U32
BucketArray_ElementCount(Bucket_Array* array)
{
    return array->bucket_size * (array->bucket_count - 1) + array->current_bucket_size;
}

void*
BucketArray_ElementAt(Bucket_Array* array, U32 index)
{
    U16 bucket_index  = (U16)(index % array->bucket_size);
    U16 bucket_offset = (U16)(index / array->bucket_size); 
    
    void* bucket = array->first_bucket;
    for (U16 i = 0; i < bucket_index; ++i) bucket = *(void**)bucket;
    
    return (U8*)bucket + sizeof(U64) + array->element_size * bucket_offset;
}

typedef struct Bucket_Array_Iterator
{
    void* current;
    U32 current_index;
    
    void* current_bucket;
    U16 last_bucket_size;
    U16 element_size;
    U16 bucket_size;
} Bucket_Array_Iterator;

Bucket_Array_Iterator
BucketArray_Iterate(Bucket_Array* array)
{
    Bucket_Array_Iterator iterator = {0};
    
    if (array->first_bucket != 0)
    {
        iterator.current_bucket   = array->first_bucket;
        iterator.current          = (U8*)array->first_bucket + sizeof(U64);
        iterator.element_size     = array->element_size;
        iterator.bucket_size      = array->bucket_size;
        iterator.last_bucket_size = array->current_bucket_size;
    }
    
    return iterator;
}

void
BucketArrayIterator_Advance(Bucket_Array_Iterator* iterator)
{
    iterator->current        = 0;
    iterator->current_index += 1;
    
    U16 bucket_index = (U16)(iterator->current_index % iterator->bucket_size);
    
    if (*(void**)iterator->current_bucket == 0 &&
        bucket_index < iterator->last_bucket_size)
    {
        if (bucket_index == 0)
        {
            iterator->current_bucket = *(void**)iterator->current_bucket;
        }
        
        iterator->current = (U8*)iterator->current_bucket + sizeof(U64) + iterator->element_size * bucket_index;
    }
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

typedef struct Array
{
    Memory_Arena* arena;
    void* data;
    UMM size;
    UMM capacity;
} Array;

#define Array(T) Array
#define ARRAY_INIT(arena, T, initial_capacity) Array_Init((arena), sizeof(((T*)0)[0]), (initial_capacity))

Array
Array_Init(Memory_Arena* arena, UMM element_size, UMM initial_capacity)
{
    void* memory = Arena_AllocateSize(arena, sizeof(U64) + element_size * initial_capacity, ALIGNOF(U64));
    *(U64*)memory = element_size;
    
    Array array = {
        .arena = arena,
        .data  = (U64*)memory + 1,
        .capacity = initial_capacity
    };
    
    return array;
}

void
Array_Reserve(Array* array, UMM size)
{
    array->capacity = size;
    
    UMM element_size = *((U64*)array->data - 1);
    
    UMM size_in_bytes      = sizeof(U64) + element_size * array->size;
    UMM new_size_in_bytes  = sizeof(U64) + element_size * array->capacity;
    
    void* new_memory = Arena_AllocateSize(array->arena, new_size_in_bytes, ALIGNOF(U64));
    
    Copy((U64*)array->data - 1, new_memory, size_in_bytes);
    
    Zero((U8*)new_memory + size_in_bytes, (array->capacity - array->size) * element_size);
    
    Arena_FreeSize(array->arena, (U64*)array - 1, size_in_bytes);
}

void*
Array_Append(Array* array)
{
    UMM element_size = *((U64*)array->data - 1);
    
    if (array->size == array->capacity)
    {
        Array_Reserve(array, array->capacity + MAX(1, array->capacity / 2));
    }
    
    void* result = (U8*)array->data + element_size * array->size;
    array->size += 1;
    
    return result;
}

void
Array_Destroy(Array* array)
{
    UMM element_size = *((U64*)array->data - 1);
    Arena_FreeSize(array->arena, (U64*)array->data - 1, sizeof(U64) + array->capacity * element_size);
}

void*
Array_ElementAt(Array* array, UMM index)
{
    ASSERT(array->size > index);
    
    UMM element_size = *((U64*)array->data - 1);
    return (U8*)array->data + index * element_size;
}