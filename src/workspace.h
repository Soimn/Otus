typedef struct Package
{
    String name;
    
} Package;

typedef struct Workspace
{
    Memory_Allocator allocator;
} Workspace;

Workspace :: struct
{
    allocator: Memory_Allocator,
    packages: [..]Packages,
}