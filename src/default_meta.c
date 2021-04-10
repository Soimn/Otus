#include "compiler_api.h"

int
main(const int argc, const char** argv)
{
    Workspace_Options workspace_options = {
        .path_prefixes = {}
    };
    
    // NOTE(soimn): Initialize compiler resources
    Workspace* workspace = OpenWorkspace(workspace_options);
    
    if (ParseFileAndAllDependencies(workspace, "main.os"))
    {
        // NOTE(soimn): Run sema and register declaration headers
        // NOTE(soimn): Unconditionally iterate over all declarations
        for (Declaration_Iterator it = IterateDeclarations(workspace);
             it.current != 0 && !workspace->has_errors;
             AdvanceIterator(&it))
        {
            if (!CheckSemanticsAndGenerateSymbolInfo(workspace, it.current))
            {
                //// ERROR: Semantically incorrect declaration
            }
            
            else
            {
                if (it.current is a proc/struct/enum declaration)
                {
                    // Default values for parameters
                    CommitDeclarationHeader(it.current);
                }
                
                else continue;
            }
        }
        
        // NOTE(soimn): Unconditionally iterate over all declarations
        for (Declaration_Iterator it = IterateDeclarations(workspace);
             it.current != 0 && !workspace->has_errors;
             AdvanceIterator(&it))
        {
            
        }
    }
    
    if (workspace->has_errors)
    {
        DumpErrorsToStdOut();
    }
    
    CloseWorkspace(workspace);
    
    return 0;
}

// A declaration bundles an AST, symbol table and some flags, along with
// information about which package the declaration belongs to, and whic
// file it came from

// Array that stores all declarations, both new, modified and comitted
// The status of the declaration is indicated by a set of flags

// New declarations can be added to the array

// Comitting a declaration will set the comitted flag for the declaration
// Only declarations stored in the declaration array can be committed
// Before a declaration is marked as committed it is checked against the
// rules of the language and the previously committed declarations
// A declaration cannot be "uncommitted"

// The type and symbol tables are updated by committing declarations

// NOTE(soimn): If the metaprogram commits a erroneous declaration, the whole
//              workspace is marked as erroneous, since the metaprogram has
//              broken the promise with the compiler.