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
    
    Workspace* workspace = Workspace_Open((Slice){.data = path_prefixes, .size = ARRAY_COUNT(path_prefixes)},
                                          STRING("../examples/demo.os"));
    
    for (Declaration_Iterator it = Workspace_IterateDeclarations(workspace);
         it.current != 0;
         Workspace_AdvanceIterator(workspace, &it, false))
    {
        // ...
    }
    
    Workspace_Close(workspace);
}