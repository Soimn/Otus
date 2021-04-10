#pragma once

/// General types
/////////////////////////////////////////////////////
#ifdef _WIN32

#ifdef OTUS_DLL_EXPORTS
# define API_FUNC __declspec(dllexport)
#else
# define API_FUNC __declspec(dllimport)
#endif

typedef signed __int8  i8;
typedef signed __int16 i16;
typedef signed __int32 i32;
typedef signed __int64 i64;

typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

#elif __linux__

#ifdef OTUS_DLL_EXPORTS
# define API_FUNC __attribute__ ((visibility("default")))
#else
# define API_FUNC __attribute__ ((visibility("default")))
#endif

typedef __INT8_TYPE__  i8;
typedef __INT16_TYPE__ i16;
typedef __INT32_TYPE__ i32;
typedef __INT64_TYPE__ i64;

typedef __UINT8_TYPE__  u8;
typedef __UINT16_TYPE__ u16;
typedef __UINT32_TYPE__ u32;
typedef __UINT64_TYPE__ u64;

#else

#error "unsupported platform"

#endif

typedef u64 umm;
typedef i64 imm;

typedef float  f32;
typedef double f64;

typedef u8  b8;
typedef u16 b16;
typedef u32 b32;
typedef u64 b64;

typedef b8 bool;

#define false 0
#define true 1

#define Enum8(NAME)  u8
#define Enum16(NAME) u16
#define Enum32(NAME) u32
#define Enum64(NAME) u64

typedef struct String
{
    u8* data;
    umm size;
} String;

typedef struct Dynamic_Array
{
    void* allocator;
    void* data;
    u64 size;
    u64 capacity;
} Dynamic_Array;

#define Dynamic_Array(T) Dynamic_Array

typedef struct Slice
{
    void* data;
    u64 size;
} Slice;

#define Slice(T) Slice

/// IDs and handles
/////////////////////////////////////////////////////

#define INVALID_ID -1

typedef i64 File_ID;
typedef i32 Package_ID;
typedef i32 Declaration_ID;

/// AST
/////////////////////////////////////////////////////

/// Declarations
/////////////////////////////////////////////////////

enum DECLARATION_KIND
{
    Declaration_Invalid = 0,
    Declaration_Import,
    Declaration_Variable,
    Declaration_Constant,
};

typedef struct Declaration
{
    Enum8(DECLARATION_KIND) kind;
    Package_ID package;
    // ast
    // symbols
} Declaration;

#define DECLARATION_BLOCK_SIZE 64
typedef struct Declaration_Block
{
    struct Declaration_Block* next;
    u64 committed_map;
    Declaration declarations[DECLARATION_BLOCK_SIZE];
} Declaration_Block;

typedef struct Declaration_Array
{
    Declaration_Block* first;
    Declaration_Block* current;
    u32 block_count;
    u32 current_block_size;
} Declaration_Array;

typedef struct Declaration_Iterator
{
    Declaration_Block* first_block;
    Declaration_Block* current_block;
    Declaration* current;
    Declaration_ID current_id;
} Declaration_Iterator;

/// Workspace bookkeeping
/////////////////////////////////////////////////////

// NOTE(soimn): Including the same file on disk in two
//              different packages will result in two
//              distinct File structs, each with a set
//              of declarations parsed from the same
//              file content. This is the case, because
//              each package needs its own copy of the
//              declarations in a file.
typedef struct File
{
    String name;
    String path;
    String contents;
    Slice(Declaration_ID) declarations;
} File;

typedef struct Package
{
    String name;
    Dynamic_Array(File) files;
    // symbols
} Package;

typedef struct Error_Report
{
} Error_Report;

typedef struct Workspace
{
    Declaration_Array declarations;
    Dynamic_Array(Package) packages;
    
    bool has_errors;
    Dynamic_Array(Error_Report) error_reports;
} Workspace;

/// API
/////////////////////////////////////////////////////

typedef struct Workspace_Options
{
    
} Workspace_Options;

API_FUNC Workspace* OpenWorkspace(Workspace_Options options);
API_FUNC void CloseWorkspace(Workspace* workspace);

API_FUNC File_ID ParseFile(Workspace* workspace, Package_ID package_id, String name, String path, String contents);
API_FUNC File_ID LoadAndParseFile(Workspace* workspace, Package_ID package_id, String path);

API_FUNC void CommitDeclarationByID(Workspace* workspace, Declaration_ID id);

/// Helper functions
/////////////////////////////////////////////////////

void
CommitDeclaration(Workspace* workspace, Declaration_Iterator it)
{
    CommitDeclarationByID(workspace, it.current_id);
}

Declaration_Iterator
IterateDeclarations(Workspace* workspace)
{
    Declaration_Iterator result = {0};
    
    // TODO(soimn): NOT_IMPLEMENTED
    
    return result;
}

void
AdvanceIterator(Declaration_Iterator* it)
{
    // TODO(soimn): NOT_IMPLEMENTED
}

Package_ID
AddNewPackage(Workspace* workspace, String name)
{
    Package_ID result = INVALID_ID;
    
    // TODO(soimn): NOT_IMPLEMENTED
    
    return result;
}

Package_ID
PackageID(Workspace* workspace, String name)
{
    Package_ID result = INVALID_ID;
    
    // TODO(soimn): NOT_IMPLEMENTED
    
    return result;
}

bool
PackageExists(Workspace* workspace, String name)
{
    return (PackageID(workspace, name) != INVALID_ID);
}

/*
Declaration_ID
AddNewDeclaration(Workspace* workspace, Package_ID package, ...)
{
}*/

/// Memory management utilities
/////////////////////////////////////////////////////

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
