void*
Align(void* ptr, u8 alignment)
{
    umm overshoot    = (umm)ptr + (alignment - 1);
    umm rounded_down = overshoot & ~(alignment - 1);
    return (void*)rounded_down;
}

u8
AlignOffset(void* ptr, u8 alignment)
{
    umm overshoot    = (umm)ptr + (alignment - 1);
    umm rounded_down = overshoot & ~(alignment - 1);
    return (u8)(rounded_down - (umm)ptr);
}

void
Copy(void* src, void* dst, umm size)
{
    u8* bsrc = src;
    u8* bdst = dst;
    
    if (bsrc < bdst)
    {
        for (smm i = size - 1; i >= 0; --i)
        {
            bdst[i] = bsrc[i];
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

void
Zero(void* ptr, umm size)
{
    for (umm i = 0; i < size; ++i)
    {
        ((u8*)ptr)[i] = 0;
    }
}

#define MEMORY_BLOCK_DEFAULT_SIZE 4 * 1024
typedef struct Memory_Block
{
    struct Memory_Block* next;
    u32 offset;
    u32 space;
} Memory_Block;

typedef struct Memory_Arena
{
    Memory_Block* first;
    Memory_Block* current;
} Memory_Arena;

void*
Arena_Allocate(Memory_Arena* arena, umm size, u8 alignment)
{
    // NOTE(soimn): It is intentional that this wastes a few bytes at the end of a block
    if (!arena->current || arena->current->space < size + alignment)
    {
        if (arena->current && arena->current->next &&
            (arena->current->next->offset + arena->current->next->space) - sizeof(Memory_Block) >= size)
        {
            arena->current = arena->current->next;
            arena->current->space += arena->current->offset - sizeof(Memory_Block);
            arena->current->offset = 0;
        }
        
        else
        {
            umm block_size = sizeof(Memory_Block) + MAX(MEMORY_BLOCK_DEFAULT_SIZE, size);
            Memory_Block* new_block = System_AllocateMemory(block_size);
            new_block->next   = arena->current->next;
            new_block->offset = sizeof(Memory_Block);
            new_block->space  = (u32)(block_size - sizeof(Memory_Block));
            
            if (!arena->current || arena->current->next && arena->first == arena->current) arena->first         = new_block;
            else                                                                           arena->current->next = new_block;
            
            arena->current = new_block;
        }
    }
    
    u8* start = (u8*)arena->current + arena->current->offset;
    u8 offset = AlignOffset(start, alignment);
    
    arena->current->offset += offset;
    arena->current->space  -= offset;
    
    return start + offset;
}

void
Arena_ClearAll(Memory_Arena* arena)
{
    arena->current = arena->first;
    
    if (arena->current != 0)
    {
        arena->current->space += arena->current->offset - sizeof(Memory_Block);
        arena->current->offset = 0;
    }
}

void
Arena_FreeAll(Memory_Arena* arena)
{
    for (Memory_Block* block = arena->first; block != 0; )
    {
        Memory_Block* next_block = block->next;
        
        System_FreeMemory(block);
        
        block = next_block;
    }
}

typedef struct Bucket_Array
{
    Memory_Arena* arena;
    void* first;
    void* current;
    umm bucket_count;
    umm bucket_size;
    umm element_size;
    umm current_bucket_size;
} Bucket_Array;

#define Bucket_Array(T) Bucket_Array
#define BUCKET_ARRAY_INIT(A, T, B) (Bucket_Array){.arena = (A), .bucket_size = (B), .element_size = sizeof(T)}

void*
BucketArray_AppendElement(Bucket_Array* array)
{
    if (!array->current || array->current_bucket_size == array->bucket_size)
    {
        if (array->current && *(void**)array->current)
        {
            array->current             = *(void**)array->current;
            array->current_bucket_size = 0;
        }
        
        else
        {
            void* new_bucket = Arena_Allocate(array->arena, sizeof(u64) + array->bucket_size * array->element_size, ALIGNOF(u64));
            
            if (array->current) *(void**)array->current = new_bucket;
            else                 array->first           = new_bucket;
            
            array->current       = new_bucket;
            array->bucket_count += 1;
        }
        
        Zero(array->current, sizeof(u64) + array->bucket_size * array->element_size);
    }
    
    void* result                = (u8*)array->current + sizeof(u64) + array->current_bucket_size * array->element_size;
    array->current_bucket_size += 1;
    
    return result;
}

void
BucketArray_ClearAll(Bucket_Array* array)
{
    array->current             = array->first;
    array->current_bucket_size = 0;
}

umm
BucketArray_ElementCount(Bucket_Array* array)
{
    return (array->bucket_count != 0 ? array->bucket_count - 1 : 0) * array->bucket_size + array->current_bucket_size;
}

void*
BucketArray_ElementAt(Bucket_Array* array, umm index)
{
    umm bucket_index  = index / array->bucket_size;
    umm bucket_offset = index % array->bucket_size;
    
    void* bucket = array->first;
    
    for (umm i = 0; i < bucket_index; ++i)
    {
        bucket = *(void**)bucket;
    }
    
    return (u8*)bucket + sizeof(u64) + bucket_offset * array->element_size;
}