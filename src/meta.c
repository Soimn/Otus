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
    
#if 0
    for (Declaration_Iterator it = DeclarationPool_CreateIterator(pool);
         it.current != 0;
         DeclarationPool_AdvanceIterator(pool, &it, false))
    {
        // ...
    }
    
    for (Declaration_Iterator it = DeclarationPool_CreateIterator(pool);
         DeclarationPool_AdvanceIterator(pool, &it, false);)
    {
        // ...
    }
#endif
    
    for (Declaration declaration; Workspace_PopDeclaration(workspace, &declaration); )
    {
        Workspace_CommitDeclaration(workspace, declaration);
    }
    
    Workspace_Close(workspace);
}