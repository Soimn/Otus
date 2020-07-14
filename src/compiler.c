#include "common.h"
#include "string.h"
#include "memory.h"
#include "lexer.h"
#include "ast.h"
#include "workspace.h"
#include "parser.h"

#ifdef _WIN32
#include "windows_layer.h"
#else
#error "unsupported platform"
#endif

global Workspace* GlobalWorkspace = 0;

void
Workspace_WorkerParsingLoop(Workspace* workspace)
{
    bool is_exhausted = false;
    for (;;)
    {
        // TODO(soimn): thread safe cmp
        if (workspace->encountered_errors || workspace->num_threads_working == 0) break;
        
        Mutex_Lock(workspace->package_mutex);
        
        Package* package_to_parse = 0;
        
        for (Bucket_Array_Iterator it = BucketArray_Iterate(&workspace->packages);
             it.current != 0;
             BucketArrayIterator_Advance(&it))
        {
            Package* current = it.current;
            
            if (current->status == PackageStatus_ReadyToParse)
            {
                package_to_parse = current;
                break;
            }
        }
        
        Mutex_Unlock(workspace->package_mutex);
        
        if (package_to_parse != 0)
        {
            // TODO(soimn): thread safe inc
            if (is_exhausted) workspace->num_threads_working += 1;
            is_exhausted = false;
            
            bool parsed_successfully = Workspace_ParsePackage(workspace, package_to_parse);
            if (!parsed_successfully)
            {
                // TODO(soimn): thread safe store
                workspace->encountered_errors = true;
            }
        }
        
        else if (!is_exhausted)
        {
            // TODO(soimn): thread safe dec
            is_exhausted                      = true;
            workspace->num_threads_working   -= 1;
        }
        
    }
}

EXPORT bool
Workspace_Open(Workspace_ID* workspace_id, Workspace_Options options, String file_path)
{
    bool encountered_errors = false;
    
    if (GlobalWorkspace != 0)
    {
        PrintCString("ERROR: Cannot open workspace before previous workspace is closed");
        encountered_errors = true;
    }
    
    else
    {
        Workspace* workspace = System_AllocateMemory(sizeof(GlobalWorkspace));
        
        GlobalWorkspace = workspace;
        *workspace_id   = (Workspace_ID)workspace;
        
        NOT_IMPLEMENTED;
        
        workspace->arena    = MEMORY_ARENA_INIT(&workspace->allocator);
        workspace->packages = BUCKET_ARRAY_INIT(&workspace->arena, Package, 10);
        
        Package* main_pkg = BucketArray_Append(&workspace->packages);
        main_pkg->name.data = (U8*)"main";
        main_pkg->name.size = sizeof("main") - 1;
        main_pkg->path      = file_path;
        
        // TODO(soimn): Run Workspace_WorkerParsingLoop on several threads
        // TODO(soimn): Set workspace->num_threads_working to the total number of threads
        workspace->num_threads_working = 1;
        
        Workspace_WorkerParsingLoop(workspace);
    }
    
    return !encountered_errors;
}

EXPORT void
Workspace_Close(Workspace_ID workspace_id)
{
    Workspace* workspace = (Workspace*)workspace_id;
    if (workspace == GlobalWorkspace)
    {
        Allocator_FreeAll(&workspace->allocator);
        System_FreeMemory(workspace);
        GlobalWorkspace = 0;
    }
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