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
    void* result = SystemAllocateMemory(size + 15);
    
    // TODO(soimn): Memory allocation failure recovery
    HARD_ASSERT(result != 0);
    
    U8* aligned = (U8*)Align(result, alignof(U64));
    
    if (aligned - (U8*)result == 0)
    {
        aligned = (U8*)Align((U8*)result + alignof(U64) - 1, alignof(U64));
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


struct Bucket_Array
{
    void** bucket_list;
    void* current_bucket;
    U32 current_bucket_offset;
    U32 bucket_size;
    U32 bucket_count;
    U32 element_size;
};

#define BUCKET_ARRAY(type, bucket_size) {0, 0, 0, (bucket_size), 0, (U32)RoundSize(sizeof(type), alignof(type))}

inline void*
PushElement(Bucket_Array* array)
{
    HARD_ASSERT(array->element_size != 0);
    HARD_ASSERT(array->bucket_size  != 0);
    
    void* result = 0;
    
    if (!array->bucket_count || array->current_bucket_offset == sizeof(U64) + array->bucket_size * array->element_size)
    {
        HARD_ASSERT(array->bucket_count != U32_MAX);
        
        void* new_bucket = AllocateMemory(sizeof(U64) + array->bucket_size * array->element_size);
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

inline void
ClearArray(Bucket_Array* array)
{
    HARD_ASSERT(array->element_size != 0);
    HARD_ASSERT(array->bucket_size  != 0);
    
    void** bucket = array->bucket_list;
    
    while (bucket != 0)
    {
        void** next = (void**)*bucket;
        
        FreeMemory(bucket);
        
        bucket = next;
    }
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
    UMM size;
    UMM capacity;
    U32 element_size;
    F32 growth_factor;
};

#define DYNAMIC_ARRAY(type, growth_factor) {0, 0, 0, (U32)RoundSize(sizeof(type), alignof(type)), growth_factor}

inline void*
PushElement(Dynamic_Array* array, UMM count = 1)
{
    HARD_ASSERT(array->element_size != 0 && count > 0);
    
    if (array->size + count >= array->capacity)
    {
        if (array->memory)
        {
            UMM new_capacity = (UMM)(array->capacity * array->growth_factor + 0.5f) + 1;
            
            new_capacity = MAX(new_capacity, array->size + count);
            
            void* new_memory = AllocateMemory(new_capacity * array->element_size);
            
            Copy(array->memory, new_memory, array->size * array->element_size);
            FreeMemory(array->memory);
            
            array->memory   = new_memory;
            array->capacity = new_capacity;
        }
        
        else
        {
            array->memory   = AllocateMemory(array->element_size);
            array->capacity = 1;
        }
    }
    
    void* result = (U8*)array->memory + array->element_size * array->size;
    array->size += count;
    
    return result;
}

inline void
Reserve(Dynamic_Array* array, UMM size)
{
    HARD_ASSERT(array->element_size != 0);
    
    if (size)
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
ResetArray(Dynamic_Array* array, IMM new_capacity = -1)
{
    HARD_ASSERT(array->element_size != 0);
    
    array->size = 0;
    
    if (new_capacity != -1)
    {
        HARD_ASSERT(new_capacity >= 0);
        
        ClearArray(array);
        Reserve(array, new_capacity);
    }
}

inline void*
ElementAt(Dynamic_Array* array, UMM index)
{
    HARD_ASSERT(array->element_size != 0);
    
    void* result = 0;
    
    if (index < array->size)
    {
        result = (U8*)array->memory + index * array->element_size;
    }
    
    return result;
}

inline UMM
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