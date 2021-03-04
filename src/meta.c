#include "compiler_api.h"

#include <stdio.h>

#define STRING(x) (String){.data=(u8*)(x), .size=sizeof(x) - 1}
#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

int
main()
{
    Path_Prefix path_prefixes[] = {
        {.name = STRING("core"), .path = STRING("../core/")}
    };
    
    Path_Prefix* temp = path_prefixes;
    
    Workspace_Options workspace_options = {
        .path_prefixes = (Slice){.data = temp, .size = ARRAY_COUNT(path_prefixes)},
        .prep_option   = WorkspacePrep_TypeChecked
    };
    
    Workspace* workspace = Workspace_Open(workspace_options, STRING("../examples/demo.os"));
    
    for (Declaration declaration; Workspace_PopDeclaration(workspace, &declaration); )
    {
        Workspace_CommitDeclaration(workspace, declaration);
    }
    
    Workspace_Close(workspace);
}