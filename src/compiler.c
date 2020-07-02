#include "common.h"
#include "string.h"
#include "memory.h"
#include "lexer.h"
#include "ast.h"
#include "workspace.h"
#include "parser.h"

global Workspace* GlobalWorkspace = 0;

EXPORT bool
Workspace_Open(Workspace_ID* workspace_id, Workspace_Options options, String file_path)
{
    bool encountered_errors = false;
    
    if (GlobalWorkspace != 0)
    {
        Print("ERROR: Cannot open workspace before previous workspace is closed");
        encountered_errors = true;
    }
    
    else
    {
        GlobalWorkspace = System_AllocateMemory(sizeof(GlobalWorkspace));
        *workspace_id   = (Workspace_ID)GlobalWorkspace;
        
        NOT_IMPLEMENTED;
    }
    
    return !encountered_errors;
}

EXPORT void
Workspace_Close(Workspace_ID workspace_id)
{
    Workspace* workspace = (Workspace*)workspace_id;
    if (workspace == GlobalWorkspace)
    {
        Allocator_FreeAll(workspace->allocator);
        System_FreeMemory(workspace);
        GlobalWorkspace = 0;
    }
}

EXPORT bool 
Workspace_BeginBuild(Workspace_ID workspace_id, bool intercept)
{
    bool encountered_errors = false;
    
    Workspace* workspace = (Workspace*)workspace_id;
    if (workspace != GlobalWorkspace || workspace->encountered_errors)
    {
        encountered_errors = true;
    }
    
    else
    {
        NOT_IMPLEMENTED;
    }
    
    return !encountered_errors;
}

EXPORT bool
Workspace_InspectNextDeclaration(Workspace_ID workspace_id, Declaration* declaration)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

EXPORT bool
Workspace_ModifyCurrentDeclaration(Workspace_ID workspace_id, Declaration declaration)
{
    bool encountered_errors = false;
    
    Workspace* workspace = (Workspace*)workspace_id;
    if (workspace != GlobalWorkspace || workspace->encountered_errors)
    {
        encountered_errors = true;
    }
    
    else
    {
        NOT_IMPLEMENTED;
    }
    
    return !encountered_errors;
}

EXPORT bool
Workspace_InjectDeclaration(Workspace_ID workspace_id, Package_ID package, Declaration declaration)
{
    bool encountered_errors = false;
    
    Workspace* workspace = (Workspace*)workspace_id;
    if (workspace != GlobalWorkspace || workspace->encountered_errors)
    {
        encountered_errors = true;
    }
    
    else
    {
        NOT_IMPLEMENTED;
    }
    
    return !encountered_errors;
}

EXPORT bool
Workspace_InjectCode(Workspace_ID workspace_id, Package_ID package, String code)
{
    bool encountered_errors = false;
    
    Workspace* workspace = (Workspace*)workspace_id;
    if (workspace != GlobalWorkspace || workspace->encountered_errors)
    {
        encountered_errors = true;
    }
    
    else
    {
        NOT_IMPLEMENTED;
    }
    
    return !encountered_errors;
}

EXPORT bool
Workspace_GenerateBinary(Workspace_ID workspace_id, Binary_Options options)
{
    bool encountered_errors = false;
    
    Workspace* workspace = (Workspace*)workspace_id;
    if (workspace != GlobalWorkspace || workspace->encountered_errors)
    {
        encountered_errors = true;
    }
    
    else
    {
        NOT_IMPLEMENTED;
    }
    
    return !encountered_errors;
}