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
    
    Workspace* workspace = OpenWorkspace(workspace_options); // init resources and state
    AddFile(workspace, CONST_STRING("main.os"));             // add main file
    
    BeginCompilation(workspace); // begin compilation and message loop
    
    for (Compilation_Message message = WaitForNextMessage(workspace);
         message.kind != CompilationMessage_Done;                             // This will accumulate errors
         //message.kind != CompilationMessage_Done && !workspace->has_errors; // This will exit on first error
         message = WaitForNextMessage(workspace))
    {
        if (message.kind == CompilationMessage_CheckedDeclaration)
        {
            Declaration declaration = message.declaration;
            
            // dont do anything if the declaration has no notes
            if (declaration.notes.size == 0) continue;
            
            // handle declaration later if it has over 1 note
            else if (declaration.notes.size > 1) HandleCurrentDeclarationLater(workspace);
            else
            {
                declaration.notes.size = 0;
                
                ModifyCurrentDeclaration(workspace, declaration);
            }
        }
    }
    
    FinishCompilation(workspace); // end intercept of messages, finish compilation and report errors
    
    CloseWorkspace(workspace); // free resources
}