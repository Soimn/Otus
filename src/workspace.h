typedef struct Source_File
{
    String path;
    String content;
    Scope scope;
    
    bool is_loaded;
} Source_File;

enum PACKAGE_STATUS
{
    PackageStatus_Unavailable,
    PackageStatus_ReadyToParse,
    PackageStatus_Parsed,
};

typedef struct Package
{
    Enum32(PACKAGE_STATUS) status;
    Memory_Allocator* allocator;
    Memory_Arena arena;
    
    String name;
    String path;
    Bucket_Array(Source_File) loaded_files;
    
    Bucket_Array(Statement) statements;
    Bucket_Array(Expression) expressions;
    
    Bucket_Array(Symbol_Table) symbol_tables;
    Scope* scope;
    
    Bucket_Array(char) error_buffer;
} Package;

typedef struct Mounting_Point
{
    String name;
    String path;
} Mounting_Point;

typedef struct Workspace
{
    Memory_Allocator allocator;
    Memory_Arena arena;
    
    Mutex package_mutex;
    Bucket_Array(Package) packages;
    
    // NOTE(soimn): Element 0 is the default
    Bucket_Array(Mounting_Point) mounting_points;
    bool export_by_default;
    
    U8 num_threads_working;
    bool encountered_errors;
} Workspace;

#define PACKAGE_TMP_NAME "**TMP_NAME**"
Package_ID
Workspace_AppendPackage(Workspace* workspace, String path)
{
    Mutex_Lock(workspace->package_mutex);
    
    Package_ID package_id = BucketArray_ElementCount(&workspace->packages);
    
    Package* package = BucketArray_Append(&workspace->packages);
    
    package->status    = PackageStatus_ReadyToParse;
    package->allocator = &workspace->allocator;
    package->name.data = (U8*)PACKAGE_TMP_NAME;
    package->name.size = sizeof(PACKAGE_TMP_NAME) - 1;
    package->path      = path;
    
    package->arena         = MEMORY_ARENA_INIT(&workspace->allocator);
    package->loaded_files  = BUCKET_ARRAY_INIT(&package->arena, Source_File, 10);
    package->symbol_tables = BUCKET_ARRAY_INIT(&package->arena, Symbol_Table, 10);
    package->error_buffer  = BUCKET_ARRAY_INIT(&package->arena, char, 1024);
    
    // NOTE(soimn): Appending a dummy table to make the 0 ID invalid
    BucketArray_Append(&package->symbol_tables);
    
    Mutex_Unlock(workspace->package_mutex);
    
    return package_id;
}