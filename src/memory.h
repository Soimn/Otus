u8
AlignOffset(void* ptr, u8 alignment)
{
    umm overshot     = (umm)ptr + (alignment - 1);
    umm rounded_down = overshot & ~(alignment - 1);
    
    return (u8)((u8*)rounded_down - (u8*)ptr);
}

void*
Align(void* ptr, u8 alignment)
{
    return (u8*)ptr + AlignOffset(ptr, alignment);
}

void
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

void
Zero(void* ptr, umm size)
{
    u8* bptr = (u8*)ptr;
    
    for (umm i = 0; i < size; ++i)
    {
        bptr[i] = 0;
    }
}

#define ZeroStruct(s) Zero(s, sizeof((s)[0]))

typedef struct Memory_Block
{
    struct Memory_Block* next;
    u32 offset;
    u32 space;
} Memory_Block;

typedef struct Memory_Free_Entry
{
    struct Memory_Free_Entry* next;
    u32 offset;
    u32 size;
} Memory_Free_Entry;

#define MEMORY_BLOCK_CURSOR(block) ((u8*)(block) + (block)->offset)
#define MEMORY_ARENA_DEFAULT_BLOCK_SIZE KILOBYTES(4) - sizeof(Memory_Block)

typedef struct Memory_Arena
{
    Memory_Block* first;
    Memory_Block* current;
    Memory_Free_Entry* free_list;
} Memory_Arena;

#define ARENA_INIT(a) {.allocator = a}

void
Arena_FreeAll(Memory_Arena* arena)
{
    for (Memory_Block* block = arena->first; block; )
    {
        Memory_Block* next = block->next;
        
        System_FreeMemory(block);
        
        block = next;
    }
    
    arena->first   = 0;
    arena->current = 0;
}

void
Arena_FreeSize(Memory_Arena* arena, void* ptr, umm size)
{
    Memory_Block* scan = arena->first;
    for (; scan != 0; scan = scan->next)
    {
        if ((u8*)ptr >= (u8*)(scan + 1) &&
            (u8*)ptr + size <= (u8*)MEMORY_BLOCK_CURSOR(scan))
        {
            break;
        }
    }
    
    ASSERT(scan != 0);
    
    umm offset = AlignOffset(ptr, ALIGNOF(Memory_Free_Entry));
    
    if (size >= sizeof(Memory_Free_Entry) + offset)
    {
        Memory_Free_Entry* entry = (Memory_Free_Entry*)((u8*)ptr + offset);
        entry->next   = arena->free_list;
        entry->offset = offset;
        entry->size   = size - offset;
        
        arena->free_list = entry;
    }
}

void
Arena_ClearAll(Memory_Arena* arena)
{
    for (Memory_Block* block = arena->first; block != 0; block = block->next)
    {
        if (block->offset == sizeof(Memory_Block)) break;
        
        block->space += arena->current->offset - sizeof(Memory_Block);
        block->offset = sizeof(Memory_Block);
    }
    
    arena->current = arena->first;
}

bool
Arena_IsCleared(Memory_Arena* arena)
{
    return (arena->current == arena->first &&
            (arena->first == 0 || arena->first->offset == sizeof(Memory_Block)));
}

void*
Arena_Allocate(Memory_Arena* arena, umm size, u8 alignment)
{
    ASSERT(size > 0 && size <= U32_MAX);
    ASSERT(alignment != 0 && (alignment & (~alignment + 1)) == alignment);
    
    void* result = 0;
    
    Memory_Free_Entry** prev_entry = &arena->free_list;
    Memory_Free_Entry*  entry      =  arena->free_list;
    while (entry != 0)
    {
        u8* ptr    = (u8*)entry - entry->offset;
        umm offset = AlignOffset(ptr, alignment);
        if (offset + size <= entry->offset + entry->size)
        {
            result = ptr + offset;
            *prev_entry = entry->next;
            
            Arena_FreeSize(arena, result + size, (entry->offset + entry->size) - (offset + size));
            
            break;
        }
        
        prev_entry = &entry->next;
        entry      = entry->next;
    }
    
    if (result == 0)
    {
        if (arena->current == 0 ||
            arena->current->space < size + AlignOffset(MEMORY_BLOCK_CURSOR(arena->current), alignment))
        {
            
            Memory_Block* prev_block = arena->current;
            Memory_Block* block      = arena->current->next;
            
            while (block != 0)
            {
                ASSERT(block->offset == sizeof(Memory_Block));
                if (block->space >= size)
                {
                    prev_block->next = block->next;
                    break;
                }
                
                else
                {
                    prev_block = block;
                    block      = block->next;
                }
            }
            
            if (arena->first == 0 || block == 0)
            {
                umm block_size = MAX(size, MEMORY_ARENA_DEFAULT_BLOCK_SIZE);
                
                block         = System_AllocateMemory(block_size);
                block->next   = 0;
                block->offset = sizeof(Memory_Block);
                block->space  = block_size;
                
                if (arena->current) arena->current->next = block;
                else                arena->first         = block;
                
                arena->current = block;
            }
            
            else
            {
                block->next          = arena->current->next;
                arena->current->next = block;
                
                Arena_FreeSize(arena, MEMORY_BLOCK_CURSOR(arena->current), arena->current->space);
                arena->current->offset += arena->current->space;
                arena->current->space   = 0;
                
                arena->current = block;
            }
        }
        
        result = Align(MEMORY_BLOCK_CURSOR(arena->current), alignment);
        
        umm advancement = AlignOffset((u8*)arena->current + arena->current->offset, alignment) + size;
        arena->current->offset += advancement;
        arena->current->space  -= advancement;
    }
    
    return result;
}

Memory_Arena
Arena_BeginTempMemory(Memory_Arena* source_arena)
{
    Memory_Block* new_block = source_arena->first;
    
    while (new_block != 0)
    {
        if (new_block->offset == sizeof(Memory_Block)) break;
        else new_block = new_block->next;
    }
    
    if (new_block == 0)
    {
        new_block         = System_AllocateMemory(MEMORY_ARENA_DEFAULT_BLOCK_SIZE);
        new_block->next   = 0;
        new_block->offset = sizeof(Memory_Block);
        new_block->space  = MEMORY_ARENA_DEFAULT_BLOCK_SIZE;
        
        if (source_arena->current) source_arena->current->next = new_block;
        else
        {
            source_arena->first   = new_block;
            source_arena->current = new_block;
        }
    }
    
    Memory_Arena temp_arena = {
        .first     = new_block,
        .current   = new_block,
        .free_list = 0,
    };
    
    return temp_arena;
}

void
Arena_EndTempMemory(Memory_Arena* source_arena, Memory_Arena* temp_arena)
{
    Arena_ClearAll(temp_arena);
    ZeroStruct(temp_arena);
}

//////////////////////////////////////////

typedef struct Bucket_Array
{
    Memory_Arena* arena;
    void* first;
    void* current;
    umm current_bucket_size;
    umm current_bucket_count;
    umm bucket_count;
    umm bucket_size;
    umm element_size;
} Bucket_Array;

#define Bucket_Array(T) Bucket_Array
#define BUCKET_ARRAY_INIT(A, T, S) (Bucket_Array){.arena = (A), .element_size = sizeof(T), .bucket_size = (S)}

void
BucketArray_Clear(Bucket_Array* array)
{
    array->current              = array->first;
    array->current_bucket_size  = 0;
    array->current_bucket_count = 0;
}

void*
BucketArray_AppendElement(Bucket_Array* array)
{
    if (array->bucket_count == 0 || array->current_bucket_size == array->bucket_size)
    {
        if (array->bucket_count != 0 && *(void**)array->current != 0)
        {
            array->current               = *(void**)array->current;
            array->current_bucket_size   = 0;
            array->current_bucket_count += 0;
        }
        
        else
        {
            umm bucket_byte_size = sizeof(u64) + array->element_size * array->bucket_size;
            
            void* bucket = Arena_Allocate(array->arena, bucket_byte_size, ALIGNOF(u64));
            Zero(bucket, bucket_byte_size);
            
            if (array->first != 0) *(void**)array->current = bucket;
            else                   array->first            = bucket;
            
            array->current               = bucket;
            array->bucket_count         += 1;
            array->current_bucket_size   = 0;
            array->current_bucket_count += 0;
        }
    }
    
    void* result = (u8*)array->current + sizeof(u64) + array->current_bucket_size * array->element_size;
    
    array->current_bucket_size += 1;
    
    return result;
}

umm
BucketArray_ElementCount(Bucket_Array* array)
{
    return (array->bucket_count - 1) * array->bucket_size + array->current_bucket_size;
}

void*
BucketArray_ElementAt(Bucket_Array* array, umm index)
{
    umm bucket_index  = index / array->bucket_size;
    umm bucket_offset = index % array->bucket_size;
    
    void* result = 0;
    
    void* bucket = array->first;
    for (umm i = 0; i < bucket_index && bucket != 0; i += 1, bucket = *(void**)bucket);
    
    if (bucket != 0) result = (u8*)bucket + sizeof(u64) + bucket_offset * array->element_size;
    
    return result;
}

Slice
BucketArray_FlattenContent(Memory_Arena* arena, Bucket_Array* array)
{
    Slice result = {0};
    
    umm size = BucketArray_ElementCount(array) * array->element_size;
    
    if (size != 0)
    {
        result = (Slice){
            .data = Arena_Allocate(arena, size, ALIGNOF(u64)),
            .size = size
        };
        
        umm cursor = 0;
        void* scan = array->first;
        for (umm i = 0; i < array->current_bucket_count; ++i)
        {
            umm bucket_byte_size = (*(void**)scan == 0 ? array->current_bucket_size : array->bucket_size) * array->element_size;
            
            Copy((u8*)scan + sizeof(u64), (u8*)result.data + cursor, bucket_byte_size);
            
            cursor += bucket_byte_size;
            scan    = *(void**)scan;
        }
    }
    
    return result;
}

typedef struct Bucket_Array_Iterator
{
    void* current_bucket;
    void* current;
    umm index;
} Bucket_Array_Iterator;

Bucket_Array_Iterator
BucketArray_CreateIterator(Bucket_Array* array)
{
    Bucket_Array_Iterator it = {
        .current_bucket = array->first,
        .current        = (u8*)array->first + sizeof(u64),
        .index          = 0
    };
    
    return it;
}

void
BucketArray_AdvanceIterator(Bucket_Array* array, Bucket_Array_Iterator* it)
{
    ASSERT(it->current != 0);
    
    it->index += 1;
    umm offset = it->index % array->bucket_size;
    
    if (*(void**)it->current_bucket != 0 || offset < array->current_bucket_size)
    {
        if (offset == 0) it->current_bucket = *(void**)it->current_bucket;
        
        it->current = (u8*)it->current_bucket + sizeof(u64) + offset * array->element_size;
    }
}

//////////////////////////////////////////

#define DynamicArray_Reserve(arena, array, T, capacity) _DynamicArray_Reserve((arena), (array), sizeof(T), (capacity))
void
_DynamicArray_Reserve(Memory_Arena* arena, Dynamic_Array* array, umm element_size, umm capacity)
{
    if (array->capacity < capacity)
    {
        void* memory = Arena_Allocate(arena, capacity * element_size, ALIGNOF(u64));
        
        Copy(array->data, memory, array->size * element_size);
        Zero((u8*)memory + array->size * element_size, (capacity - array->size) * element_size);
        
        Arena_FreeSize(arena, array->data, array->capacity * element_size);
        array->data     = memory;
        array->capacity = capacity;
    }
}

#define DynamicArray_Append(arena, array, T) _DynamicArray_Append((arena), (array), sizeof(T))
void*
_DynamicArray_Append(Memory_Arena* arena, Dynamic_Array* array, umm element_size)
{
    if (array->size == array->capacity)
    {
        _DynamicArray_Reserve(arena, array, element_size, MAX(10, array->size * 1.5));
    }
    
    void* result = (u8*)array->data + array->size * element_size;
    Zero(result, element_size);
    
    array->size += 1;
    
    return result;
}

#define DynamicArray_FreeAll(arena, array, T) _DynamicArray_FreeAll((arena), (array), sizeof(T))
void
_DynamicArray_FreeAll(Memory_Arena* arena, Dynamic_Array* array, umm element_size)
{
    Arena_FreeSize(arena, array->data, array->capacity * element_size);
    
    array->data     = 0;
    array->size     = 0;
    array->capacity = 0;
}