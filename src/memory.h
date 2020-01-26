inline void*
AllocateMemory(UMM size)
{
    void* result = malloc(size);
    ASSERT(result != 0);
    return result;
}

inline void
FreeMemory(void* ptr)
{
    free(ptr);
}

///////////////////////////////////////////////////////////////////////////////////

inline void*
Align(void* ptr, U8 alignment)
{
    // NOTE(soimn): increment past alignment boundary and round down by masking off the 'offset' digits
    UMM past_boundary = (UMM)ptr + (alignment - 1);
    UMM mask           = ~(alignment - 1);
    
    return (void*)(past_boundary & mask);
}

inline U8
AlignOffset(void* ptr, U8 alignment)
{
    return (U8)((U8*)Align(ptr, alignment) - (U8*)ptr);
}

inline UMM
RoundSize(UMM size, U8 alignment)
{
    return size + AlignOffset((U8*)0 + size, alignment);
}

inline void
ZeroSize(void* ptr, UMM size)
{
    for (UMM i = 0; i < size; ++i)
    {
        ((U8*)ptr)[i] = 0;
    }
}

inline void
Copy(void* src, void* dst, UMM size)
{
    for (UMM i = 0; i < size; ++i)
    {
        ((U8*)dst)[i] = ((U8*)src)[i];
    }
}

///////////////////////////////////////////////////////////////////////////////////

struct Memory_Arena_Block
{
    void* memory;
    Memory_Arena_Block* next;
    U32 offset;
    U32 size;
};

#define MEMORY_ARENA_DEFAULT_BLOCK_SIZE 4096 - (sizeof(Memory_Arena_Block) + (alignof(Memory_Arena_Block) + 1))

struct Memory_Arena
{
    Memory_Arena_Block* first_block;
    Memory_Arena_Block* current_block;
};

inline void*
PushSize(Memory_Arena* arena, U32 size, U8 alignment)
{
    U64 maximum_required_size = (U64)size + (alignment - 1);
    
    ASSERT(maximum_required_size < U32_MAX);
    
    if (!arena->current_block || arena->current_block->size - arena->current_block->offset < maximum_required_size)
    {
        U32 header_size = sizeof(Memory_Arena_Block) + (alignof(Memory_Arena_Block) - 1);
        U64 block_size  = MAX(MEMORY_ARENA_DEFAULT_BLOCK_SIZE, maximum_required_size + header_size);
        
        ASSERT(block_size <= U32_MAX);
        
        void* new_memory = (Memory_Arena_Block*)AllocateMemory(block_size);
        
        Memory_Arena_Block* new_block = (Memory_Arena_Block*)Align(new_memory, alignof(Memory_Arena_Block));
        new_block->memory = new_memory;
        new_block->next   = 0;
        new_block->size   = (U32)block_size;
        new_block->offset = (U32)((U8*)(new_block + 1) - (U8*)new_memory);
        
        if (arena->first_block)
        {
            arena->current_block->next = new_block;
        }
        
        else
        {
            arena->first_block = new_block;
        }
        
        arena->current_block = new_block;
    }
    
    U8* push_ptr = (U8*)arena->current_block->memory + arena->current_block->offset;
    
    void* result = push_ptr;
    arena->current_block->offset += AlignOffset(push_ptr, alignment) + size;
    
    return result;
}

#define PushStruct(arena, type) PushSize(arena, (U32)sizeof(type), (U8)alignof(type))

inline void
ClearArena(Memory_Arena* arena)
{
    for (Memory_Arena_Block* scan = arena->first_block; scan; )
    {
        Memory_Arena_Block* next = scan->next;
        FreeMemory(scan->memory);
        scan = next;
    }
}

///////////////////////////////////////////////////////////////////////////////////

#define BUCKET_ARRAY_HEADER_SIZE (U32)RoundSize(sizeof(void*), alignof(U64))
struct Bucket_Array
{
    Memory_Arena* arena;
    void* first_bucket;
    void* current_bucket;
    U32 current_offset;
    U32 element_size;
    U32 bucket_capacity;
};

#define Bucket_Array(type) Bucket_Array

inline Bucket_Array
BucketArray(Memory_Arena* arena, U64 element_size, U64 bucket_capacity)
{
    ASSERT(element_size <= U32_MAX);
    ASSERT(bucket_capacity / element_size <= U32_MAX);
    
    Bucket_Array bucket_array = {};
    bucket_array.arena           = arena;
    bucket_array.element_size    = (U32)element_size;
    bucket_array.bucket_capacity = (U32)bucket_capacity;
    
    return bucket_array;
}

#define BUCKET_ARRAY(arena, type, bucket_capacity) BucketArray(arena, RoundSize(sizeof(type), alignof(type)), bucket_capacity)

inline void*
PushElement(Bucket_Array* array)
{
    if (!array->current_bucket || array->current_offset == array->bucket_capacity)
    {
        U32 bucket_size = BUCKET_ARRAY_HEADER_SIZE + array->element_size * array->bucket_capacity;
        
        void* new_bucket = PushSize(array->arena, bucket_size, alignof(void*));
        ZeroSize(new_bucket, bucket_size);
        
        if (array->first_bucket)
        {
            *(void**)array->current_bucket = new_bucket;
        }
        
        else
        {
            array->first_bucket = new_bucket;
        }
        
        array->current_bucket = new_bucket;
        array->current_offset = 0;
    }
    
    U8* block_start = (U8*)array->current_bucket + BUCKET_ARRAY_HEADER_SIZE;
    void* result    = block_start + array->current_offset * array->element_size;
    ++array->current_offset;
    
    return result;
}

inline UMM
ElementCount(Bucket_Array* array)
{
    UMM result = 0;
    
    void* scan = array->first_bucket;
    while (scan && *(void**)scan != 0)
    {
        result += array->bucket_capacity;
        scan = *(void**)scan;
    }
    
    result += array->current_offset;
    
    return result;
}

inline void*
ElementAt(Bucket_Array* array, UMM index)
{
    UMM bucket_index  = index / array->bucket_capacity;
    U32 bucket_offset = index % array->bucket_capacity;
    
    void* bucket = array->first_bucket;
    
    for (UMM i = 0; i < bucket_index; ++i)
    {
        bucket = *(void**)bucket;
    }
    
    U8* block_start = (U8*)bucket + BUCKET_ARRAY_HEADER_SIZE;
    void* result    = block_start + bucket_offset * array->element_size;
    
    if (*(void**)bucket == 0 && bucket_offset >= array->current_offset)
    {
        result = 0;
    }
    
    return result;
}

struct Bucket_Array_Iterator
{
    void* current_bucket;
    U32 element_size;
    U32 bucket_capacity;
    U32 last_bucket_offset;
    
    UMM current_index;
    void* current;
};

inline Bucket_Array_Iterator
Iterate(Bucket_Array* array)
{
    Bucket_Array_Iterator iterator = {};
    iterator.current_bucket     = array->first_bucket;
    iterator.element_size       = array->element_size;
    iterator.bucket_capacity    = array->bucket_capacity;
    iterator.last_bucket_offset = array->current_offset;
    iterator.current_index      = 0;
    
    iterator.current = 0;
    
    if (iterator.current_bucket)
    {
        iterator.current = (U8*)iterator.current_bucket + BUCKET_ARRAY_HEADER_SIZE;
    }
    
    return iterator;
}

inline void
Advance(Bucket_Array_Iterator* iterator)
{
    iterator->current = 0;
    ++iterator->current_index;
    
    if (iterator->current_index % iterator->bucket_capacity == 0)
    {
        iterator->current_bucket = *(void**)iterator->current_bucket;
    }
    
    else if (*(void**)iterator->current_bucket == 0 && iterator->current_index >= iterator->last_bucket_offset)
    {
        iterator->current_bucket = 0;
    }
    
    if (iterator->current_bucket)
    {
        U32 bucket_offset  = iterator->current_index % iterator->bucket_capacity;
        U8* block_start    = (U8*)iterator->current_bucket + BUCKET_ARRAY_HEADER_SIZE;
        
        iterator->current = block_start + bucket_offset * iterator->element_size;
    }
}