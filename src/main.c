#include "otus.h"

#include <stdio.h>
#include <string.h>

#define ASSERT(EX) ((EX) ? 1 : *(volatile int*)0)
#define NOT_IMPLEMENTED ASSERT(!"NOT_IMPLEMENTED")
#define CONST_STRING(STR) (String){ .data = (u8*)(STR), .size = sizeof(STR) - 1 }

int
main(int argc, const char** argv)
{
    if (argc != 2) fprintf(stderr, "Wrong number of arguments\n");
    else
    {
        String main_file = {
            .data = (u8*)argv[1],
            .size = strlen(argv[1])
        };
        
        Workspace* workspace    = OpenWorkspace((Workspace_Options){});
        Package_ID main_package = AddNewPackage(workspace, CONST_STRING("main"));
        LoadAndParseFile(workspace, main_package, main_file);
        
        Memory_Arena scratch_arena = {0};
        
        for (Declaration_Iterator it = IterateDeclarations(workspace);
             it.current != 0 && !workspace->has_errors;
             AdvanceIterator(&it))
        {
            if (it.current->kind == Declaration_Import) CommitDeclaration(workspace, it);
            else
            {
                NOT_IMPLEMENTED;
            }
        }
        
        if (workspace->has_errors)
        {
            fprintf(stderr, "There were errors. Exiting...\n");
        }
        
        CloseWorkspace(workspace);
    }
}