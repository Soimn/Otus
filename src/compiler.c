#include "otus.h"

#ifdef OTUS_DEBUG
#define ASSERT(EX) ((EX) ? 1 : *(volatile int*)0)
#else
#define ASSERT(EX)
#endif

#define NOT_IMPLEMENTED ASSERT(!"NOT_IMPLEMENTED")
#define INVALID_DEFAULT_CASE ASSERT(!"INVALID_DEFAULT_CASE")

#define ARRAY_COUNT(A) (sizeof(A) / sizeof(0[A]))

#define U8_MAX  (u8)0xFF
#define U16_MAX (u16)0xFFFF
#define U32_MAX (u32)0xFFFFFFFF
#define U64_MAX (u64)0xFFFFFFFFFFFFFFFF

#define I8_MAX  (i8)U8_MAX   >> 1
#define I16_MAX (i16)U16_MAX >> 1
#define I32_MAX (i32)U32_MAX >> 1
#define I64_MAX (i64)U64_MAX >> 1

#define CONST_STRING(s) (String){.data = (u8*)(s), .size = sizeof(s) - 1}

#define OFFSETOF(e, T) (umm)&((T*)0)->e
#define ALIGNOF(T) OFFSETOF(t, struct { char c; T t; })

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))

#define IS_POW_OF_2(N) (((u64)(N) & ((u64)(N) - 1)) == 0)

typedef struct File
{
    String path;
    String contents;
} File;

typedef struct Compiler_State
{
    // IMPORTANT NOTE: Must be first field (Workspace* -> Compiler_State* casting)
    Workspace workspace;
} Compiler_State;

/// ////////////////////////////////////////////////////////////////////////


API_FUNC Workspace*
OpenWorkspace()
{
    static Compiler_State compiler_state_stub = {
        .workspace = { .has_errors = true }
    };
    
    NOT_IMPLEMENTED;
    
    return &compiler_state_stub.workspace;
}

API_FUNC void
CloseWorkspace(Workspace* workspace)
{
    Compiler_State* compiler_state = (Compiler_State*)workspace;
    (void)compiler_state;
    
    NOT_IMPLEMENTED;
}

/// ////////////////////////////////////////////////////////////////////////

API_FUNC void
AddFile(Workspace* workspace, String path)
{
    Compiler_State* compiler_state = (Compiler_State*)workspace;
    (void)compiler_state;
    
    NOT_IMPLEMENTED;
}

API_FUNC void
AddSourceCode(Workspace* workspace, String code)
{
    Compiler_State* compiler_state = (Compiler_State*)workspace;
    (void)compiler_state;
    
    NOT_IMPLEMENTED;
}

API_FUNC void
AddDeclaration(Workspace* workspace, ...)
{
    Compiler_State* compiler_state = (Compiler_State*)workspace;
    (void)compiler_state;
    
    NOT_IMPLEMENTED;
}

API_FUNC void
AddDeclarationToPackage(Workspace* workspace, Package_ID package, ...)
{
    Compiler_State* compiler_state = (Compiler_State*)workspace;
    (void)compiler_state;
    
    NOT_IMPLEMENTED;
}

/// ////////////////////////////////////////////////////////////////////////

API_FUNC Declaration*
InspectNextDeclaration(Workspace* workspace)
{
    Compiler_State* compiler_state = (Compiler_State*)workspace;
    (void)compiler_state;
    
    Declaration* declaration = 0;
    
    NOT_IMPLEMENTED;
    
    return declaration;
}

API_FUNC void
FinnishCompilation(Workspace* workspace)
{
    Compiler_State* compiler_state = (Compiler_State*)workspace;
    (void)compiler_state;
    
    NOT_IMPLEMENTED;
}

API_FUNC void
BuildBinary(Workspace* workspace, Binary_Options options)
{
    Compiler_State* compiler_state = (Compiler_State*)workspace;
    (void)compiler_state;
    
    NOT_IMPLEMENTED;
}

/// ////////////////////////////////////////////////////////////////////////

API_FUNC void
ModifyCurrentDeclaration(Workspace* workspace, Declaration declaration)
{
    NOT_IMPLEMENTED;
}

API_FUNC void
HandleCurrentDeclarationLater(Workspace* workspace)
{
    NOT_IMPLEMENTED;
}

/// ////////////////////////////////////////////////////////////////////////