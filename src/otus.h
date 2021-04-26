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

//////////////////////////////////////////////////////

typedef u32 Package_ID;
typedef u32 File_ID;

//////////////////////////////////////////////////////

enum DECLARATION_KIND
{
    Declaration_Invalid = 0,
    Declaration_Proc,
    Declaration_Macro,
    Declaration_Struct,
    Declaration_Union,
    Declaration_Enum,
    Declaration_Import,
    Declaration_Include,
};

typedef struct Locator
{
    Package_ID package;
    File_ID file;
    u32 line;
    u32 column;
} Locator;

typedef struct Code_Note
{
    
} Code_Note;

typedef struct Declaration
{
    Enum8(DECLARATION_KIND) kind;
    Locator locator;
    // notes
    //symbol_id
    //AST* ast;
} Declaration;

typedef struct Package
{
    bool _;
} Package;

//////////////////////////////////////////////////////

typedef struct Workspace {
    bool has_errors;
} Workspace;

typedef struct Workspace_Options
{
    String working_directory;
} Workspace_Options;

enum COMPILATION_MESSAGE_KIND
{
    CompilationMessage_Done = 0,
    CompilationMessage_LoadFile,
    CompilationMessage_ParsedDeclaration,
    CompilationMessage_CheckedDeclaration
};

typedef struct Compilation_Message
{
    Enum8(COMPILATION_MESSAGE_KIND) kind;
} Compilation_Message;

API_FUNC Workspace* OpenWorkspace();
API_FUNC void CloseWorkspace(Workspace* workspace);

API_FUNC void AddFile(Workspace* workspace, String path);
API_FUNC void AddSourceCode(Workspace* workspace, String code);
API_FUNC void AddDeclaration(Workspace* workspace, ...);

API_FUNC void AddDeclarationToPackage(Workspace* workspace, Package_ID package, Declaration declaration);

API_FUNC void BeginCompilation(Workspace* workspace);
API_FUNC void FinishCompilation(Workspace* workspace);
API_FUNC Compilation_Message WaitForNextMessage(Workspace* workspace);

// TODO(soimn): Allow the user to provide more info (like: file, line, package)
API_FUNC void ReportError(Workspace* workspace, String message);
API_FUNC void ReportWarning(Workspace* workspace, String message);

API_FUNC void ModifyCurrentDeclaration(Workspace* workspace, Declaration declaration);
API_FUNC void HandleCurrentDeclarationLater(Workspace* workspace);

// TODO(soimn): User level preprocessor and parse tree modification support
// API_FUNC void UseThisStringInsteadOfLoadingTheFile(Workspace* workspace, String text);
// API_FUNC void UseThisASTInstead(Workspace* workspace, AST* ast);