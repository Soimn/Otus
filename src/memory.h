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

struct Bucket_Array
{
    void** buckets;
    void* current_bucket;
    U32 current_bucket_offset;
    U32 bucket_size;
    U32 bucket_count;
    U32 element_size;
};

#define BUCKET_ARRAY(type, bucket_size) {0, 0, 0, (bucket_size), 0, RoundSize(sizeof(type), alignof(type))}

inline void*
PushElement(Bucket_Array* array)
{
    if (!array->current_bucket || array->bucket_size - array->current_bucket_offset == 0)
    {
        UMM header_size = (alignof(U64) - 1) + sizeof(U64);
        UMM bucket_size = array->element_size * array->bucket_size;
        
        void* new_bucket = AllocateMemory(header_size + bucket_size);
        
        if (array->current_bucket)
        {
            *(void**)Align(array->current_bucket, alignof(U64)) = new_bucket;
        }
        
        else
        {
            array->buckets = (void**)new_bucket;
        }
        
        array->current_bucket        = new_bucket;
        array->current_bucket_offset = 0;
    }
    
    U8* start_of_bucket = Align(array->current_bucket, alignof(U64)) + sizeof(U64);
    
    void* result = start_of_bucket + array->current_bucket_offset * array->element_size;
    
    ++array->current_bucket_offset;
    
    return result;
}

inline void
ClearArray(Bucket_Array* array)
{
    void** bucket = array->buckets;
    
    while (bucket != 0)
    {
        void** temp = (void**)*bucket;
        
        FreeMemory(bucket);
        
        bucket = temp;
    }
}

inline void*
ElementAt(Bucket_Array* array, UMM index)
{
    void* result = 0;
    
    UMM bucket_index  = index / array->bucket_size;
    U32 bucket_offset = index % array->bucket_size;
    
    if (bucket_index < array->bucket_count && (bucket_index != array->bucket_count - 1 || bucket_offset < array->current_bucket_offset))
    {
        void** scan = array->buckets;
        
        for (U32 i = 0; i < bucket_index; ++i)
        {
            scan = (void**)*scan;
        }
        
        result = Align(scan, alignof(U64)) + array->element_size * bucket_offset;
    }
    
    return result;
}

struct Bucket_Array_Iterator
{
    void** current_bucket;
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
    Bucket_Array_Iterator iterator = {};
    
    iterator.current_bucket     = array->buckets;
    iterator.current_index      = 0;
    iterator.last_bucket_offset = array->current_bucket_offset;
    iterator.bucket_count       = array->bucket_count;
    iterator.bucket_size        = array->bucket_size;
    iterator.element_size       = array->element_size;
    
    return iterator;
}

inline void
Advance(Bucket_Array_Iterator* iterator)
{
    iterator->current = 0;
    
    ++iterator->current_index;
    
    UMM bucket_index  = iterator->current_index / iterator->bucket_size;
    U32 bucket_offset = iterator->current_index % iterator->bucket_size;
    
    if (bucket_index < iterator->bucket_count && (bucket_index != iterator->bucket_count - 1 || bucket_offset < iterator->last_bucket_offset))
    {
        if (iterator->current_index % iterator->bucket_size == 0)
        {
            iterator->current_bucket = (void**)*iterator->current_bucket;
        }
        
        iterator->current = Align(iterator->current_bucket, alignof(U64)) + iterator->element_size * bucket_offset;
    }
}