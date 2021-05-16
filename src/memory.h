umm
RoundToAlignment(umm n, u8 alignment)
{
    u64 overshot     = n + (alignment - 1);
    u64 rounded_down = overshot & ~(alignment - 1);
    
    return rounded_down;
}


void*
Align(void* ptr, u8 alignment)
{
    return (void*)RoundToAlignment((umm)ptr, alignment);
}

u8
AlignOffset(void* ptr, u8 alignemnt)
{
    return (u8)((u64)ptr - RoundToAlignment((umm)ptr, alignment));
}

typedef struct Memory_Block
{
    struct Memory_Block* next;
    u32 offset;
    u32 space;
} Memory_Block;

typedef struct Memory_Arena_Free_Entry
{
    struct Memory_Arena_Free_Entry* next;
    u32 size;
    u8 offset;
} Memory_Arena_Free_Entry;

#define MEMORY_ARENA_DEFAULT_BLOCK_SIZE 4096
typedef struct Memory_Arena
{
    Memory_Block* first;
    Memory_Block* current;
    Memory_Arena_Free_Entry* first_free;
    u64 wasted_space;
} Memory_Arena;

void*
Arena_PushSize(Memory_Arena* arena, umm size)
{
    ASSERT(sizeof(Memory_Block) % ALIGNOF(u64) == 0);
    ASSERT(size < U32_MAX - sizeof(Memory_Block));
    
    u32 rounded_size = (u32)RoundToAlignment(size, ALIGNOF(u64));
    
    if (!arena->current || arena->current->space < size)
    {
        if (arena->current->next != 0 && arena->current->next->offset - sizeof(Memory_Block) >= size)
        {
            arena->current = arena->current->next;
            
            arena->current->space += arena->current->offset - sizeof(Memory_Block);
            arena->current->offset = 0;
        }
        
        else
        {
            umm block_size = MAX(sizeof(Memory_Block) + size, MEMORY_ARENA_DEFAULT_BLOCK_SIZE);
            
            Memory_Block* new_block = System_AllocateMemory(block_size);
            new_block->next   = (arena->current != 0 ? arena->current->next : 0);
            new_block->offset = sizeof(Memory_Block);
            new_block->space  = block_size - sizeof(Memory_Block);
            
            if (!arena->current) arena->first         = new_block;
            else                 arena->current->next = new_block;
            
            arena->current = new_block;
        }
    }
    
    void* result = (u8*)arena->current + arena->current->offset + align_offset;
    
    arena->current->offset += align_offset + size;
    align->current->space  -= align_offset + size;
    
    return result;
}

void
Arena_FreeSize(Memory_Arena* arena, void* ptr, umm size)
{
}

void*
Arena_AllocateSize(Memory_Arena* arena, umm size, u8 alignment)
{
    ASSERT(IS_POW_OF_2(alignment) && alignment <= 8);
    
    void* result = 0;
    
    for (Memory_Arena_Free_Entry* scan = arena->first_free;
         scan != 0;
         scan = scan->next)
    {
        if ((scan->offset - AlignOffset((u8*)scan - scan->offset, aligment)) + scan->size >= size)
        {
        }
    }
    
    return result;
}

void
Arena_ClearAll(Memory_Arena* arena)
{
}

void
Arena_FreeAll(Memory_Arena* arena)
{
}

typedef struct Bucket_Array
{
    Memory_Arena* arena;
    void* first;
    void* current;
    u16 block_count;
    u16 block_size;
    u16 current_block_size;
    u16 element_size;
} Bucket_Array;

#define Bucket_Array(T) Bucket_Array
#define BUCKET_ARRAY_INIT(A, T, C) (Bucket_Array){.arena = (A), .element_size = sizeof(T), .block_size = (C)}

void*
BucketArray_AppendElement(Bucket_Array* array)
{
}

void*
BucketArray_ElementCount(Bucket_Array* array)
{
}

void*
BucketArray_ElementAt(Bucket_Array* array, umm index)
{
}

void
BucketArray_Flatten(Bucket_Array* array, Memory_Arena* arena)
{
}