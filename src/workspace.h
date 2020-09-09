typedef struct Path_Prefix
{
    String prefix;
    String path;
} Path_Prefix;


enum OPTIMIZATION_LEVEL
{
    OptLevel_None,      // No optimizations
    OptLevel_Debug,     // Add debug info
    OptLevel_FastDebug, // Optimize for speed and add debug info
    OptLevel_Fast,      // Optimize for speed, fast compile time
    OptLevel_Fastest,   // Optimize for speed, slow compile time
    OptLevel_Small,     // Optimize for size
};

typedef struct Workspace_Options
{
    Bucket_Array(Path_Prefix) path_prefixes;
    u8 opt_level;
} Workspace_Options;

typedef struct Package
{
    Memory_Arena arena;
    Bucket_Array(char) error_buffer;
} Package;

typedef struct Workspace
{
    Memory_Allocator allocator;
    Workspace_Options options;
    
    Bucket_Array(Package) packages;
} Workspace;