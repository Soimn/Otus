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