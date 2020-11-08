inline u8
AlignOffset(void* ptr, u8 alignment)
{
    umm overshot     = (umm)ptr + (alignment - 1);
    umm rounded_down = overshot & ~(alignment - 1);
    
    return (u8)((u8*)rounded_down - (u8*)ptr);
}

inline void*
Align(void* ptr, u8 alignment)
{
    return (u8*)ptr + AlignOffset(ptr, alignment);
}

inline void
Copy(void* src, void* dst, umm size)
{
    u8* bsrc = (u8*)src;
    u8* bdst = (u8*)dst;
    
    if (bsrc < bdst && bsrc + size > bdst)
    {
        for (umm i = 0; i < size; ++i)
        {
            bdst[size - i] = bsrc[size - i];
        }
    }
    
    else
    {
        for (umm i = 0; i < size; ++i)
        {
            bdst[i] = bsrc[i];
        }
    }
}

inline void
Zero(void* ptr, umm size)
{
    u8* bptr = (u8*)ptr;
    
    for (umm i = 0; i < size; ++i)
    {
        bptr[i] = 0;
    }
}

#define ZeroStruct(s) Zero(s, sizeof((s)[0]))

typedef struct Memory_Allocator
{
    void* first_block;
} Memory_Allocator;

void*
Allocator_Allocate(Memory_Allocator* allocator, umm size)
{
    void* memory = System_AllocateMemory(size + sizeof(u64));
    Zero(memory, size + sizeof(u64));
    
    *(void**)memory = allocator->first_block;
    allocator->first_block = memory;
    
    return (u8*)memory + sizeof(u64);
}

void
Allocator_Free(Memory_Allocator* allocator, void* ptr)
{
    void* prev = 0;
    void* curr = 0;
    for (void* block = allocator->first_block; block; )
    {
        void* next = *(void**)block;
        
        if (ptr == (u8*)block + sizeof(u64))
        {
            curr = block;
            break;
        }
        
        prev  = block;
        block = next;
    }
    
    ASSERT(curr != 0);
    
    if (prev) *(void**)prev     = *(void**)curr;
    else allocator->first_block = *(void**)curr;
    
    System_FreeMemory(curr);
}

void
Allocator_FreeAll(Memory_Allocator* allocator)
{
    for (void* scan = allocator->first_block; scan; )
    {
        void* next = *(void**)scan;
        
        System_FreeMemory(scan);
        
        scan = next;
    }
}


typedef struct Memory_Block
{
    struct Memory_Block* next;
    u32 offset;
    u32 space;
} Memory_Block;

typedef struct Memory_Free_Entry
{
    struct Memory_Free_Entry* next;
    u32 space;
    u8 offset;
} Memory_Free_Entry;

#define MEMORY_ARENA_DEFAULT_BLOCK_SIZE KILOBYTES(4) - sizeof(Memory_Block)
#define MEMORY_FREE_ENTRY_MIN_SIZE sizeof(Memory_Free_Entry) + (ALIGNOF(Memory_Free_Entry) - 1)

typedef struct Memory_Arena
{
    Memory_Allocator* allocator;
    Memory_Block* first;
    Memory_Block* current;
    Memory_Free_Entry* first_free;
} Memory_Arena;

#define ARENA_INIT(a) {.allocator = a}

void
Arena_FreeSize(Memory_Arena* arena, void* ptr, umm size)
{
    ASSERT(size <= U32_MAX);
    
#ifdef OTUS_DEBUG_MODE
    
    bool was_found = false;
    for (Memory_Block* block = arena->first; block; block = block->next)
    {
        if (ptr >= block + 1 && (u8*)ptr + size < (u8*)(block + 1) + block->offset)
        {
            was_found = true;
            break;
        }
    }
    
    ASSERT(was_found);
    
#endif
    
    if (size >= MEMORY_FREE_ENTRY_MIN_SIZE)
    {
        Memory_Free_Entry* entry = Align(ptr, ALIGNOF(u64));
        entry->next   = arena->first_free;
        entry->offset = AlignOffset(ptr, ALIGNOF(u64));
        entry->space  = (u32)size - entry->offset;
        
        arena->first_free = entry;
    }
}

void
Arena_FreeAll(Memory_Arena* arena)
{
    for (Memory_Block* block = arena->first; block; )
    {
        Memory_Block* next = block->next;
        
        Allocator_Free(arena->allocator, block);
        
        block = next;
    }
    
    arena->first      = 0;
    arena->current    = 0;
    arena->first_free = 0;
}

void
Arena_ClearAll(Memory_Arena* arena)
{
    arena->current    = arena->first;
    arena->first_free = 0;
    
    if (arena->current)
    {
        arena->current->space += arena->current->offset - sizeof(Memory_Block);
        arena->current->offset = sizeof(Memory_Block);
    }
}

void*
Arena_Allocate(Memory_Arena* arena, umm size, u8 alignment)
{
    ASSERT(size > 0 && size <= U32_MAX);
    ASSERT(alignment && (aligment & (~alignment + 1)) == alignment);
    
    void* result = 0;
    
    if (arena->first_free != 0)
    {
        Memory_Free_Entry* prev = 0;
        for (Memory_Free_Entry* scan = arena->first_free; scan; )
        {
            if (scan->offset + scan->space >= size + (alignment - 1))
            {
                u8 offset = AlignOffset((u8*)scan - scan->offset, alignment);
                
                result = ((u8*)scan - scan->offset) + offset;
                
                if (prev) prev->next   = scan->next;
                else arena->first_free = scan->next;
                
                Arena_FreeSize(arena, (u8*)result + size, (scan->offset + scan->space) - (offset + size));
                
                break;
            }
            
            prev = scan;
            scan = scan->next;
        }
    }
    
    if (result == 0)
    {
        if (arena->current == 0 || arena->current->space == 0)
        {
            if (arena->current != 0 && arena->current->next != 0 &&
                size + (alignment - 1) <= (arena->current->next->space + arena->current->next->offset - sizeof(Memory_Block)))
            {
                arena->current = arena->current->next;
                Zero(arena->current + 1, arena->current->space);
            }
            
            else
            {
                uint block_size = MAX((uint)size, MEMORY_ARENA_DEFAULT_BLOCK_SIZE);
                
                Memory_Block* block = Allocator_Allocate(arena->allocator, sizeof(Memory_Block) + block_size);
                block->next   = (arena->current != 0 ? arena->current->next : 0);
                block->offset = sizeof(Memory_Block);
                block->space  = block_size;
                
                if (arena->current) arena->current->next = block;
                else                arena->first         = block;
                
                arena->current = block;
            }
        }
        
        result = Align((u8*)arena->current + arena->current->offset, alignment);
        
        uint advancement = AlignOffset((u8*)arena->current + arena->current->offset, alignment) + (uint)size;
        arena->current->offset += advancement;
        arena->current->space  -= advancement;
    }
    
    return result;
}

typedef struct Bucket_Array
{
    Memory_Arena* arena;
    void* first_free;
    void* first;
    void* current;
    u16 element_size;
    u16 current_bucket_size;
    u16 bucket_size;
    u16 bucket_count;
} Bucket_Array;

#define Bucket_Array(T) Bucket_Array

#define BUCKET_ARRAY_INIT(a, T, c) {.arena = (a), .element_size = sizeof(((T*)0)[0]), .bucket_size = (c)}

void
BucketArray_Free(Bucket_Array* array, void* ptr)
{
#ifdef OTUS_DEBUG_MODE
    bool was_found = false;
    for (void* bucket = array->first; bucket; bucket = *(void**)bucket)
    {
        if (ptr >= (u8*)bucket + sizeof(u64) &&
            (u8*)ptr + array->element_size <= (u8*)bucket + sizeof(u64) + array->element_size * array->bucket_size)
        {
            umm offset = (u8*)ptr - ((u8*)bucket + sizeof(u64));
            
            if (ptr == array->current &&
                offset + array->element_size > array->element_size * array->current_bucket_size)
            {
                continue;
            }
            
            else
            {
                ASSERT(offset % array->element_size == 0);
                was_found = true;
            }
        }
    }
    
    ASSERT(was_found);
#endif
    
    if (array->element_size >= sizeof(u64))
    {
        *(void**)ptr      = array->first_free;
        array->first_free = ptr;
    }
}

void
BucketArray_FreeAll(Bucket_Array* array)
{
    for (void* bucket = array->first; bucket; )
    {
        void* next = *(void**)bucket;
        
        Arena_FreeSize(array->arena, bucket, sizeof(u64) + array->element_size * array->bucket_size);
        
        bucket = next;
    }
    
    array->first_free          = 0;
    array->first               = 0;
    array->current             = 0;
    array->current_bucket_size = 0;
    array->bucket_count        = 0;
}

void
BucketArray_ClearAll(Bucket_Array* array)
{
    array->first_free          = 0;
    array->current             = array->first;
    array->current_bucket_size = 0;
    array->bucket_count        = (array->current != 0 ? 1 : 0);
}

void*
BucketArray_Append(Bucket_Array* array)
{
    ASSERT(array->element_size != 0 && array->bucket_size != 0);
    
    void* result = 0;
    
    if (array->first_free)
    {
        result = array->first_free;
        
        array->first_free = *(void**)array->first_free;
    }
    
    else
    {
        if (array->current == 0 || array->current_bucket_size == array->bucket_size)
        {
            if (array->current != 0 && *(void**)array->current != 0)
            {
                array->current = *(void**)array->current;
                
                Zero((u8*)array->current + sizeof(u64), array->element_size * array->bucket_size);
                ++array->bucket_count;
            }
            
            else
            {
                void* new_bucket = Arena_Allocate(array->arena, sizeof(u64) + array->element_size * array->bucket_size,
                                                  ALIGNOF(u64));
                
                if (array->current) *(void**)array->current = new_bucket;
                else                *(void**)array->first   = new_bucket;
                
                array->current              = new_bucket;
                array->current_bucket_size = 0;
                ++array->bucket_count;
            }
        }
        
        result = (u8*)array->current + sizeof(u64) + array->element_size * array->current_bucket_size;
        
        ++array->current_bucket_size;
    }
    
    return result;
}

void*
BucketArray_ElementAt(Bucket_Array* array, u32 index)
{
    u32 bucket_index  = index / array->bucket_size;
    u32 bucket_offset = index % array->bucket_size;
    
    ASSERT(bucket_index < array->bucket_count);
    ASSERT(bucket_index != array->bucket_count - 1 ||
           bucket_offset < array->current_bucket_size));
    
    void* bucket = array->first;
    for (u32 i = 0; i < bucket_index; ++i) bucket = *(void**)bucket;
    
    return (u8*)bucket + sizeof(u64) + bucket_offset * array->element_size;
}

u32
BucketArray_ElementCount(Bucket_Array* array)
{
    return array->bucket_count * MAX((i32)array->bucket_count - 1, 0) + array->current_bucket_size;
}

typedef struct Bucket_Array_Iterator
{
    void* current_bucket;
    void* current;
    u32 index;
    u16 bucket_size;
    u16 last_bucket_size;
    u16 element_size;
} Bucket_Array_Iterator;

Bucket_Array_Iterator
BucketArray_Iterate(Bucket_Array* array)
{
    Bucket_Array_Iterator result = {
        .current_bucket   = array->first,
        .bucket_size      = array->bucket_size,
        .last_bucket_size = array->current_bucket_size,
        .element_size     = array->element_size
    };
    
    if (array->current != 0)
    {
        result.current = (u8*)result.current_bucket + sizeof(u64);
    }
    
    return result;
}

void
BucketArrayIterator_Advance(Bucket_Array_Iterator* it)
{
    ASSERT(it->current != 0);
    
    ++it->index;
    it->current = 0;
    
    if (it->index % it->bucket_size == 0)
    {
        it->current = *(void**)it->current;
    }
    
    if (it->current_bucket != 0 &&
        (*(void**)it->current_bucket != 0 ||
         it->index % it->bucket_size < it->last_bucket_size))
    {
        it->current = (u8*)it->current_bucket + sizeof(u64) + (it->index % it->bucket_size) * it->element_size;
    }
}