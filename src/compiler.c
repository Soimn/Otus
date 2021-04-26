#include "otus.h"

#ifdef OTUS_DEBUG
#define ASSERT(EX) ((EX) ? 1 : *(volatile int*)0)
#else
#define ASSERT(EX)
#endif

#define NOT_IMPLEMENTED ASSERT(!"NOT_IMPLEMENTED")
#define INVALID_DEFAULT_CASE ASSERT(!"INVALID_DEFAULT_CASE")

#define ARRAY_COUNT(A) (sizeof(A) / sizeof(0[A]))

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

API_FUNC void
BeginCompilation(Workspace* workspace)
{
    Compiler_State* compiler_state = (Compiler_State*)workspace;
    (void)compiler_state;
    
    NOT_IMPLEMENTED;
}

API_FUNC void
FinishCompilation(Workspace* workspace)
{
    Compiler_State* compiler_state = (Compiler_State*)workspace;
    (void)compiler_state;
    
    NOT_IMPLEMENTED;
}

API_FUNC Compilation_Message
WaitForNextMessage(Workspace* workspace)
{
    Compiler_State* compiler_state = (Compiler_State*)workspace;
    (void)compiler_state;
    
    Compilation_Message message = {0};
    
    if (workspace->has_errors) message.kind = CompilationMessage_Done;
    else
    {
        NOT_IMPLEMENTED;
    }
    
    return message;
}