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
    return (U8)((U8*)Align(ptr, alignment) - (U8*)ptr);
}

inline UMM
RoundSize(UMM size, U8 alignment)
{
    UMM overshot     = size + (alignment - 1);
    UMM rounded_down = overshot & ~(alignment - 1);
    return rounded_down;
}

inline void
Copy(void* src, void* dst, UMM size)
{
    U8* bsrc = (U8*)src;
    U8* bdst = (U8*)dst;
    
    if (bsrc > bdst)
    {
        for (UMM i = 0; i < size; ++i)
        {
            bdst[i] = bsrc[i];
        }
    }
    
    else if (bsrc < bdst)
    {
        for (UMM i = 0; i < size; ++i)
        {
            bdst[(size - 1) - i] = bsrc[(size - 1) - i];
        }
    }
}

inline void
Zero(void* ptr, UMM size)
{
    U8* bptr = (U8*)ptr;
    
    for (UMM i = 0; i < size; ++i)
    {
        bptr[i] = 0;
    }
}

#define ZeroStruct(ptr) Zero((ptr), sizeof((ptr)[0]))
#define ZeroArray(ptr, count) Zero((ptr), RoundSize(sizeof((ptr)[0]), alignof((ptr)[0])) * (count))

/////////////////////////////////////////

struct Memory_Block
{
    Memory_Block* next;
    UMM offset;
    UMM size;
};

struct Free_List_Entry
{
    Free_List_Entry* next;
    UMM size;
    U8 offset;
};

#define MEMORY_ARENA_DEFAULT_BLOCK_SIZE (4096 - sizeof(Memory_Block))
struct Memory_Arena
{
    Memory_Block* first_block;
    Memory_Block* current_block;
    UMM default_block_size;
    Free_List_Entry* first_free;
    Free_List_Entry* last_free;
    UMM largest_free;
};

inline void*
PushSize(Memory_Arena* arena, UMM size, U8 alignment)
{
    Memory_Block* block = arena->current_block;
    U8* push_ptr        = (U8*)block + (block ? block->offset : 0);
    
    if (!block || block->offset + AlignOffset(push_ptr, alignment) + size > block->size)
    {
        if (arena->default_block_size == 0)
        {
            arena->default_block_size = MEMORY_ARENA_DEFAULT_BLOCK_SIZE;
        }
        
        UMM block_size = sizeof(Memory_Block) + MAX(arena->default_block_size, size);
        Memory_Block* new_block = (Memory_Block*)AllocateMemory(block_size);
        new_block->next   = 0;
        new_block->offset = sizeof(Memory_Block);
        new_block->size   = block_size;
        
        if (arena->current_block) arena->current_block->next = new_block;
        else arena->first_block = new_block; 
        
        arena->current_block = new_block;
        
        block    = new_block;
        push_ptr = (U8*)block + block->offset;
    }
    
    void* result   = Align(push_ptr, alignment);
    block->offset += AlignOffset(push_ptr, alignment) + size;
    
    return result;
}

#define PushStruct(arena, T) (T*)PushSize(arena, sizeof(T), alignof(T))
#define PushArray(arena, T, count) (T*)PushSize(arena, RoundSize(sizeof(T), alignof(T)) * (count), alignof(T))

inline String
PushString(Memory_Arena* arena, String string_to_copy)
{
    String result = {};
    result.data = (U8*)PushSize(arena, string_to_copy.size, alignof(U8));
    result.size = string_to_copy.size;
    
    Copy(string_to_copy.data, result.data, string_to_copy.size);
    
    return result;
}

inline void
FreeArena(Memory_Arena* arena)
{
    Memory_Block* block = arena->first_block;
    
    while (block)
    {
        Memory_Block* next_block = block->next;
        
        FreeMemory(block);
        
        block = next_block;
    }
}

/////////////////////////////////////////

struct Bucket_Array
{
    Memory_Arena* arena;
    void* first_bucket;
    void* current_bucket;
    U32 current_bucket_size;
    U32 bucket_capacity;
    U32 element_size;
    U32 bucket_count;
};

inline Bucket_Array
BucketArray_(Memory_Arena* arena, I64 element_size, I64 bucket_capacity)
{
    ASSERT(element_size > 0 && element_size < U32_MAX);
    ASSERT(bucket_capacity > 0 && bucket_capacity < U32_MAX);
    
    Bucket_Array array = {};
    array.arena           = arena;
    array.bucket_capacity = (U32)bucket_capacity;
    array.element_size    = (U32)element_size;
    
    return array;
}

#define BUCKET_ARRAY(arena, T, bucket_capacity) BucketArray_((arena), RoundSize(sizeof(T), alignof(T)), (bucket_capacity))
#define Bucket_Array(T) Bucket_Array

inline void*
PushElement(Bucket_Array* array)
{
    if (!array->bucket_count || array->current_bucket_size == array->bucket_capacity)
    {
        UMM bucket_size = sizeof(U64) + array->element_size * array->bucket_capacity;
        void* new_bucket = PushSize(array->arena, bucket_size, alignof(U64));
        Zero(new_bucket, bucket_size);
        
        if (array->bucket_count) *(void**)array->current_bucket = new_bucket;
        else array->first_bucket = new_bucket;
        
        array->current_bucket      = new_bucket;
        array->current_bucket_size = 0;
        ++array->bucket_count;
    }
    
    void* result = (U8*)array->current_bucket + sizeof(U64) + array->current_bucket_size * array->element_size;
    ++array->current_bucket_size;
    
    return result;
}

inline UMM
ElementCount(Bucket_Array* array)
{
    return array->bucket_capacity * array->bucket_count + array->current_bucket_size;
}

inline void*
ElementAt(Bucket_Array* array, UMM index)
{
    void* result = 0;
    
    UMM bucket_index  = index / array->bucket_capacity;
    U32 bucket_offset = (U32)(index % array->bucket_capacity);
    
    if (bucket_index * array->bucket_capacity + bucket_offset < ElementCount(array))
    {
        void* bucket = array->first_bucket;
        
        for (U32 i = 0; i < (U32)bucket_index; ++i)
        {
            bucket = *(void**)bucket;
        }
        
        result = (U8*)bucket + sizeof(U64) + bucket_offset * array->element_size;
    }
    
    return result;
}

struct Bucket_Array_Iterator
{
    UMM current_index;
    void* current;
    
    void* current_bucket;
    U32 last_bucket_size;
    U32 bucket_capacity;
    U32 element_size;
};

inline Bucket_Array_Iterator
Iterate(Bucket_Array* array)
{
    Bucket_Array_Iterator it = {};
    it.current_bucket   = array->first_bucket;
    it.last_bucket_size = array->current_bucket_size;
    it.bucket_capacity  = array->bucket_capacity;
    it.element_size     = array->element_size;
    
    it.current = (U8*)it.current_bucket + sizeof(U64);
    
    return it;
}

inline void
Advance(Bucket_Array_Iterator* it)
{
    if (it->current)
    {
        ++it->current_index;
        
        U32 offset = (U32)(it->current_index % it->bucket_capacity);
        
        if (offset == 0)
        {
            it->current_bucket = *(void**)it->current_bucket;
        }
        
        if (it->current_bucket == 0 || *(void**)it->current_bucket == 0 && offset >= it->last_bucket_size)
        {
            *it = {};
        }
        
        else
        {
            it->current = (U8*)it->current_bucket + sizeof(U64) + offset * it->element_size;
        }
    }
}