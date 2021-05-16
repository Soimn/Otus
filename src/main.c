#include "otus.h"

#include <stdio.h>
#include <string.h>

#define ASSERT(EX) ((EX) ? 1 : *(volatile int*)0)
#define NOT_IMPLEMENTED ASSERT(!"NOT_IMPLEMENTED")
#define CONST_STRING(STR) (String){ .data = (u8*)(STR), .size = sizeof(STR) - 1 }

int
main(int argc, const char** argv)
{
    Workspace_Options workspace_options = {
        .working_directory = CONST_STRING("")
    };
    
    Workspace* workspace = OpenWorkspace(workspace_options);
    AddFile(workspace, CONST_STRING("main.os"));
    
    for (Declaration* declaration = InspectNextDeclaration(workspace);
         declaration != 0;
         declaration = InspectNextDeclaration(workspace))
    {
    }
    
    FinnishCompilation(workspace);
    
    Binary_Options debug_binary_options   = {0};
    Binary_Options release_binary_options = {0};
    
    BuildBinary(workspace, debug_binary_options);
    BuildBinary(workspace, release_binary_options);
    
    CloseWorkspace(workspace);
}