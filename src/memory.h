#pragma once

inline U8*
Align(void* ptr, U8 alignment)
{
	return ((U8*) ptr + (U8)(((~((UMM) ptr)) + 1) & (U8)(alignment - 1)));
}

inline U8
AlignOffset(void* ptr, U8 alignment)
{
	return (U8)((U8*) Align(ptr, alignment) - (U8*) ptr);
}

inline UMM
RoundSize(UMM size, U8 alignment)
{
    return size + AlignOffset((U8*)size, alignment);
}

inline void
Copy(void* source, void* dest, UMM size)
{
    HARD_ASSERT(size);
    
    U8* bsource = (U8*) source;
    U8* bdest   = (U8*) dest;
    
    while (bsource < (U8*) source + size)
    {
        *(bdest++) = *(bsource++);
    }
}

#define CopyStruct(source, dest) Copy((void*) (source), (void*) (dest), sizeof(*(source)))
#define CopyArray(source, dest, count) Copy((void*) (source), (void*) (dest), sizeof(*(source)) * (count))

inline void
ZeroSize(void* ptr, UMM size)
{
	U8* bptr = (U8*) ptr;
    
	while (bptr < (U8*) ptr + size) *(bptr++) = 0;
}

#define ZeroStruct(type) (*(type) = {})
#define ZeroArray(type, count) ZeroSize(type, sizeof((type)[0]) * (count))

inline void*
AllocateMemory(UMM size)
{
    void* result = SystemAllocateMemory(size + alignof(U64));
    
    // TODO(soimn): Memory allocation failure recovery
    HARD_ASSERT(result != 0);
    
    U8* aligned = (U8*)Align(result, alignof(U64));
    
    if (aligned - (U8*)result == 0)
    {
        aligned += alignof(U64);
    }
    
    *(aligned - 1) = (U8)(aligned - (U8*)result);
    
    return aligned;
}

inline void
FreeMemory(void* ptr)
{
    U8* memory = (U8*)ptr - *((U8*)ptr - 1);
    SystemFreeMemory(memory);
}

struct Memory_Arena_Block
{
    Memory_Arena_Block* next;
    U32 offset;
    U32 size;
};

#define MEMORY_ARENA_DEFAULT_MINIMUM_BLOCK_SIZE 1024
struct Memory_Arena
{
    Memory_Arena_Block* first_block;
    Memory_Arena_Block* current_block;
    U32 minimum_block_size;
    U32 block_count;
};

#define MEMORY_ARENA(minimum_block_size) {0, 0, minimum_block_size, 0}

inline void*
PushSize(Memory_Arena* arena, U32 size, U8 alignment)
{
    HARD_ASSERT(size != 0 && alignment != 0);
    HARD_ASSERT(U32_MAX - alignment > size);
    
    U32 aligned_size = 0;
    U8* push_ptr     = 0;
    
    if (arena->block_count)
    {
        push_ptr = (U8*)(arena->current_block + 1) + arena->current_block->offset;
        aligned_size = AlignOffset(push_ptr, alignment) + size;
    }
    
    if (!arena->block_count || arena->current_block->offset + aligned_size >= arena->current_block->size)
    {
        U32 block_size = MAX(arena->minimum_block_size, aligned_size);
        
        if (block_size == 0)
        {
            // TODO(soimn): Maybe warn about an uninitialized memory arena
            block_size = MEMORY_ARENA_DEFAULT_MINIMUM_BLOCK_SIZE;
            arena->minimum_block_size = block_size;
        }
        
        Memory_Arena_Block* new_block = (Memory_Arena_Block*)AllocateMemory(sizeof(Memory_Arena_Block) + block_size);
        ZeroStruct(new_block);
        
        if (arena->block_count)
        {
            arena->current_block->next = new_block;
        }
        
        else
        {
            arena->first_block = new_block;
        }
        
        push_ptr = (U8*)(new_block + 1);
        
        arena->current_block = new_block;
        ++arena->block_count;
    }
    
    void* result = Align(push_ptr, alignment);
    arena->current_block->offset += aligned_size;
    
    return result;
}

inline void
ClearArena(Memory_Arena* arena)
{
    Memory_Arena_Block* scan = arena->first_block;
    
    while (scan)
    {
        Memory_Arena_Block* next = scan->next;
        FreeMemory(scan);
        scan = next;
    }
}

struct Bucket_Array
{
    Memory_Arena* arena;
    void** bucket_list;
    void* current_bucket;
    U32 current_bucket_offset;
    U32 bucket_size;
    U32 bucket_count;
    U32 element_size;
};

#define BUCKET_ARRAY(arena, type, bucket_size) {arena, 0, 0, 0, (bucket_size), 0, (U32)RoundSize(sizeof(type), alignof(type))}

inline void*
PushElement(Bucket_Array* array)
{
    HARD_ASSERT(array->element_size != 0);
    HARD_ASSERT(array->bucket_size  != 0);
    
    void* result = 0;
    
    if (!array->bucket_count || array->current_bucket_offset == sizeof(U64) + array->bucket_size * array->element_size)
    {
        HARD_ASSERT(array->bucket_count != U32_MAX);
        
        void* new_bucket = PushSize(array->arena, sizeof(U64) + array->bucket_size * array->element_size, alignof(U64));
        ZeroSize(new_bucket, sizeof(U64));
        
        if (!array->bucket_count)
        {
            array->bucket_list = (void**)new_bucket;
        }
        
        else
        {
            *(void**)array->current_bucket = new_bucket;
        }
        
        array->current_bucket        = new_bucket;
        array->current_bucket_offset = sizeof(U64);
        ++array->bucket_count;
    }
    
    result = (U8*)array->current_bucket + array->current_bucket_offset;
    array->current_bucket_offset += array->element_size;
    
    return result;
}

inline void*
ElementAt(Bucket_Array* array, UMM index)
{
    HARD_ASSERT(array->element_size != 0);
    HARD_ASSERT(array->bucket_size  != 0);
    
    void* result = 0;
    
    U32 bucket_index = (U32)(index / array->bucket_size);
    U32 offset = sizeof(U64) + (index % array->bucket_size) * array->element_size;
    
    if (bucket_index == array->bucket_count - 1)
    {
        if (offset < array->current_bucket_offset)
        {
            result = (U8*)array->current_bucket + offset;
        }
    }
    
    else if (bucket_index < array->bucket_count - 1)
    {
        void** bucket = array->bucket_list;
        
        for (U32 i = 0; i < index; ++i)
        {
            bucket = (void**)*bucket;
        }
        
        result = (U8*)bucket + offset;
    }
    
    return result;
}

inline UMM
ElementCount(Bucket_Array* array)
{
    HARD_ASSERT(array->element_size != 0);
    HARD_ASSERT(array->bucket_size  != 0);
    
    UMM count = 0;
    
    if (array->bucket_count)
    {
        U32 elements_in_last_bucket = (array->current_bucket_offset - sizeof(U64)) / array->element_size;
        count = (array->bucket_count - 1) * array->bucket_size + elements_in_last_bucket;
    }
    
    return count;
}

struct Bucket_Array_Iterator
{
    void* current_bucket;
    UMM current_index;
    U32 last_bucket_offset;
    U32 bucket_count;
    U32 bucket_size;
    U32 element_size;
    void* current;
};

inline Bucket_Array_Iterator
Iterate(Bucket_Array* array)
{
    HARD_ASSERT(array->element_size != 0);
    HARD_ASSERT(array->bucket_size  != 0);
    
    Bucket_Array_Iterator iterator = {};
    
    if (array->bucket_count)
    {
        iterator.current_bucket     = (void*)array->bucket_list;
        iterator.current_index      = 0;
        iterator.last_bucket_offset = array->current_bucket_offset;
        iterator.bucket_count       = array->bucket_count;
        iterator.bucket_size        = array->bucket_size;
        iterator.element_size       = array->element_size;
        
        iterator.current = (U8*)iterator.current_bucket + sizeof(U64);
    }
    
    return iterator;
}

inline void
Advance(Bucket_Array_Iterator* iterator)
{
    iterator->current = 0;
    
    ++iterator->current_index;
    
    U32 bucket_index  = (U32)(iterator->current_index / iterator->bucket_size);
    U32 bucket_offset = (U32)(iterator->current_index % iterator->bucket_size);
    
    U32 offset = sizeof(U64) + bucket_offset * iterator->element_size;
    
    if (bucket_index < iterator->bucket_count - 1 || (bucket_index == iterator->bucket_count - 1 && offset < iterator->last_bucket_offset))
    {
        if (bucket_offset == 0)
        {
            iterator->current_bucket = *(void**)iterator->current_bucket;
        }
        
        iterator->current = (U8*)iterator->current_bucket + offset;
    }
}

struct Dynamic_Array
{
    void* memory;
    U32 size;
    U32 capacity;
    U32 element_size;
    F32 growth_factor;
};

#define DYNAMIC_ARRAY(type, growth_factor) {0, 0, 0, (U32)RoundSize(sizeof(type), alignof(type)), growth_factor}

inline void*
PushElement(Dynamic_Array* array)
{
    HARD_ASSERT(array->element_size != 0);
    
    if (array->size >= array->capacity)
    {
        if (array->memory)
        {
            UMM new_capacity = (UMM)(array->capacity * array->growth_factor + 0.5f) + 1;
            HARD_ASSERT(new_capacity < U32_MAX);
            
            void* new_memory = AllocateMemory(new_capacity * array->element_size);
            
            Copy(array->memory, new_memory, array->size * array->element_size);
            FreeMemory(array->memory);
            
            array->memory   = new_memory;
            array->capacity = (U32)new_capacity;
        }
        
        else
        {
            array->memory   = AllocateMemory(array->element_size);
            array->capacity = 1;
        }
    }
    
    void* result = (U8*)array->memory + array->element_size * array->size;
    ++array->size;
    
    return result;
}

inline void
Reserve(Dynamic_Array* array, U32 size)
{
    HARD_ASSERT(array->element_size != 0);
    HARD_ASSERT(size != 0);
    
    if (array->capacity != size)
    {
        if (array->memory)
        {
            void* new_memory = AllocateMemory(size * array->element_size);
            
            Copy(array->memory, new_memory, array->size * array->element_size);
            FreeMemory(array->memory);
            
            array->memory   = new_memory;
            array->capacity = size;
        }
        
        else
        {
            array->memory   = AllocateMemory(size * array->element_size);
            array->capacity = size;
        }
    }
}

inline void
ClearArray(Dynamic_Array* array)
{
    HARD_ASSERT(array->element_size != 0);
    
    FreeMemory(array->memory);
    array->size     = 0;
    array->capacity = 0;
}

inline void
ResetArray(Dynamic_Array* array, U32 new_capacity = 0)
{
    HARD_ASSERT(array->element_size != 0);
    
    array->size = 0;
    
    if (new_capacity != 0 && new_capacity != array->capacity)
    {
        ClearArray(array);
        Reserve(array, new_capacity);
    }
}

inline void*
ElementAt(Dynamic_Array* array, U32 index)
{
    HARD_ASSERT(array->element_size != 0);
    
    void* result = 0;
    
    if (index < array->size)
    {
        result = (U8*)array->memory + index * array->element_size;
    }
    
    return result;
}

inline U32
ElementCount(Dynamic_Array* array)
{
    HARD_ASSERT(array->element_size != 0);
    return array->size;
}

struct Dynamic_Array_Iterator
{
    void* memory;
    UMM size;
    UMM current_index;
    U32 element_size;
    
    void* current;
};

inline Dynamic_Array_Iterator
Iterate(Dynamic_Array* array)
{
    HARD_ASSERT(array->element_size != 0);
    
    Dynamic_Array_Iterator iterator = {};
    
    iterator.memory        = array->memory;
    iterator.size          = array->size;
    iterator.current_index = 0;
    iterator.element_size  = array->element_size;
    
    iterator.current = iterator.memory;
    
    return iterator;
}

inline void
Advance(Dynamic_Array_Iterator* iterator)
{
    HARD_ASSERT(iterator->element_size != 0);
    
    ++iterator->current_index;
    iterator->current = (U8*)iterator->current + iterator->element_size;
    
    if (iterator->current_index >= iterator->size)
    {
        iterator->current = 0;
    }
}

struct Static_Array
{
    Memory_Arena* arena;
    void* memory;
    U32 element_size;
    U32 size;
    U32 capacity;
};

inline Static_Array
StaticArray(Memory_Arena* arena, U32 size, U8 alignment, U32 capacity)
{
    HARD_ASSERT(size != 0 && alignment != 0 && capacity != 0);
    
    U32 element_size = (U32)RoundSize(size, alignment);
    
    Static_Array result = {};
    result.arena        = arena;
    result.element_size = element_size;
    result.capacity     = capacity;
    
    result.memory = PushSize(arena, element_size * capacity, alignment);
    
    return result;
}

#define STATIC_ARRAY(arena, type, capacity) StaticArray(arena, sizeof(type), alignof(type), capacity)

inline void*
PushElement(Static_Array* array)
{
    HARD_ASSERT(array->element_size != 0 && array->capacity != 0 && array->memory != 0);
    
    void* result = 0;
    
    if (array->size < array->capacity)
    {
        result = (U8*)array->memory + array->element_size * array->size;
        ++array->size;
    }
    
    return result;
}

inline void
ResetArray(Static_Array* array)
{
    HARD_ASSERT(array->element_size != 0 && array->capacity != 0 && array->memory != 0);
    array->size = 0;
}

inline void*
ElementAt(Static_Array* array, U32 index)
{
    HARD_ASSERT(array->element_size != 0 && array->capacity != 0 && array->memory != 0);
    
    void* result = 0;
    
    if (index < array->size)
    {
        result = (U8*)array->memory + array->element_size * index;
    }
    
    return result;
}

inline U32
ElementCount(Static_Array* array)
{
    HARD_ASSERT(array->element_size != 0 && array->capacity != 0 && array->memory != 0);
    return array->size;
}

struct Static_Array_Iterator
{
    void* memory;
    U32 current_index;
    U32 size;
    U32 element_size;
    
    void* current;
};

inline Static_Array_Iterator
Iterate(Static_Array* array)
{
    HARD_ASSERT(array->element_size != 0 && array->capacity != 0 && array->memory != 0);
    
    Static_Array_Iterator iterator = {};
    iterator.memory       = array->memory;
    iterator.size         = array->size;
    iterator.element_size = array->element_size;
    
    iterator.current = array->memory;
    
    return iterator;
}

inline void
Advance(Static_Array_Iterator* iterator)
{
    iterator->current = 0;
    ++iterator->current_index;
    
    if (iterator->current_index < iterator->size)
    {
        iterator->current = (U8*)iterator->memory + iterator->element_size * iterator->current_index;
    }
}