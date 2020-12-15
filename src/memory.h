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

typedef struct Memory_Block
{
    struct Memory_Block* next;
    u32 offset;
    u32 space;
} Memory_Block;

#define MEMORY_BLOCK_CURSOR(block) ((u8*)(block) + (block)->offset)
#define MEMORY_ARENA_DEFAULT_BLOCK_SIZE KILOBYTES(4) - sizeof(Memory_Block)

typedef struct Memory_Arena
{
    Memory_Block* first;
    Memory_Block* current;
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

inline bool
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
            // NOTE(soimn): Deciding whether to discard the free space in the current or new block.
            //              Since the arena does not possess a free list, and the works like a
            //              stack, only the current block can keep track of free space.
            if (block->space > arena->current->space)
            {
                block->next          = arena->current->next;
                arena->current->next = block;
                
                arena->current = block;
            }
            
            else
            {
                prev_block = arena->first;
                while (prev_block->next != arena->current) prev_block = prev_block->next;
                
                prev_block->next = block;
                block->next      = arena->current;
            }
        }
    }
    
    void* result = Align(MEMORY_BLOCK_CURSOR(arena->current), alignment);
    
    umm advancement = AlignOffset((u8*)arena->current + arena->current->offset, alignment) + size;
    arena->current->offset += advancement;
    arena->current->space  -= advancement;
    
    return result;
}

inline umm
Arena_TotalAllocatedMemory(Memory_Arena* arena)
{
    umm total = 0;
    
    for (Memory_Block* scan = arena->first; scan != 0; scan = scan->next)
    {
        total += scan->offset - sizeof(Memory_Block);
    }
    
    return total;
}

inline void
Arena_FlattenContent(Memory_Arena* arena, void* dst, umm dst_size)
{
    ASSERT(dst_size == Arena_TotalAllocatedMemory(arena));
    
    umm cursor = 0;
    for (Memory_Block* scan = arena->first; scan != 0; scan = scan->next)
    {
        umm block_size = scan->offset - sizeof(Memory_Block);
        
        Copy((u8*)(scan + 1), (u8*)dst + cursor, block_size);
        
        cursor += block_size;
    }
}