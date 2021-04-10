#include "otus.h"

#ifdef OTUS_DEBUG
#define ASSERT(EX) ((EX) ? 1 : *(volatile int*)0)
#else
#define ASSERT(EX)
#endif

#define NOT_IMPLEMENTED ASSERT(!"NOT_IMPLEMENTED")
#define INVALID_DEFAULT_CASE ASSERT(!"INVALID_DEFAULT_CASE")

#define ARRAY_COUNT(A) (sizeof(A) / sizeof(0[A]))

static Workspace WorkspaceStub = { .has_errors = true };

API_FUNC Workspace*
OpenWorkspace(Workspace_Options options)
{
    Workspace* workspace = &WorkspaceStub;
    
    // TODO(soimn): Try to allocate workspace resources
    
    return workspace;
}

API_FUNC void
CloseWorkspace(Workspace* workspace)
{
    NOT_IMPLEMENTED;
}

API_FUNC File_ID
ParseFile(Workspace* workspace, Package_ID package_id, String name, String path, String contents)
{
    File_ID file_id = INVALID_ID;
    
    NOT_IMPLEMENTED;
    
    return file_id;
}

API_FUNC File_ID
LoadAndParseFile(Workspace* workspace, Package_ID package_id, String path)
{
    File_ID file_id = INVALID_ID;
    
    NOT_IMPLEMENTED;
    
    return file_id;
}

API_FUNC void
CommitDeclarationByID(Workspace* workspace, Declaration_ID id)
{
    NOT_IMPLEMENTED;
}