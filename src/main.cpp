#include "common.h"
#include "string.h"
#include "lexer.h"
#include "ast.h"
#include "parser.h"

#ifdef GREMLIN_PLATFORM_WINDOWS
#include "win32_layer.h"
#endif

inline File_ID
LoadAndParseFile(Module* module, String path)
{
    File_ID file_id = LoadFileForCompilation(module, path);
    
    if (file_id != INVALID_ID)
    {
        if (ParseFile(module, file_id))
        {
            // Succeeded
        }
        
        else
        {
            //// ERROR
            module->failed_to_parse_all_files = true;
        }
    }
    
    else
    {
        //// ERROR
        Print("Failed to load %S\n", path);
    }
    
    return file_id;
}

int
main(int argc, const char** argv)
{
    Module module = {};
    module.files = BUCKET_ARRAY(&module.main_arena, File, 8);
    
    bool encountered_errors = false;
    
    String target_file_path = {};
    
    if (argc == 2)
    {
        target_file_path.data = (U8*)argv[1];
        target_file_path.size = StringLength(argv[1]);
    }
    
    else
    {
        Print("Invalid number of arguments. Expected path to target source file.\n");
        encountered_errors = true;
    }
    
    if (!encountered_errors)
    {
        LoadAndParseFile(&module, target_file_path);
        
        if (!module.failed_to_parse_all_files)
        {
            /// Build list of identifiers, declared symbols, type names and create symbol tables
            /// Check code for correctness (declared before use, type compatibility, function calls, context 
            ///                             specific statements)
            /// Build final AST
            /// Generate code
        }
        
        else
        {
            encountered_errors = true;
        }
    }
    
    Print("\nCompilation %s.\n", (encountered_errors ? "failed" : "succeeded"));
    
    return 0;
}