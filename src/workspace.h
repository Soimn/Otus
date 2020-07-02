typedef struct Source_File
{
    String name;
    String path;
    String content;
} Source_File;

typedef struct Package
{
    String name;
    String path;
    Bucket_Array(Symbol_Table) symbol_tables;
    Scope top_scope;
    
    Bucket_Array(Source_File) loaded_files;
} Package;

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
    Enum32(OPTIMIZATION_LEVEL) opt_level;
} Workspace_Options;

typedef struct Workspace
{
    Memory_Allocator allocator;
    
    Bucket_Array(Package) packages;
    
    bool encountered_errors;
} Workspace;

typedef struct Binary_Options
{
    bool build_dll;
} Binary_Options;