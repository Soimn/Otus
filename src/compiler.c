#include "common.h"
#include "memory.h"
#include "string.h"
#include "lexer.h"

#ifdef _WIN32
#include "windows_layer.h"
#else
#error "unsupported platform"
#endif

EXPORT Workspace OpenWorkspace(Workspace_Options options, String file);
EXPORT void CloseWorkspace(Workspace* workspace);

EXPORT void ParseAllFiles(Workspace* workspace, ---);

EXPORT Declaration* FetchDeclaration(Workspace* workspace);
EXPORT


ParseEverything -> ParsedStack

while decl_pile != empty
{
    FetchDecl;
    TypeCheckDecl;
    
    ResubmitDecl;
    SubmitNewDecl;
    
    ModifyDecl;
    
    GenerateBytecodeForDecl;
    
    CommitDecl;
}

BytecodeStack

Runnable

GenerateBinary