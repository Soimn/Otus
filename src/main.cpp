#include "common.h"
#include "string.h"
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "symbols.h"
#include "sema.h"

#ifdef GREMLIN_PLATFORM_WINDOWS
#include "win32_layer.h"
#endif

inline File_ID
LoadAndParseFile(Module* module, String path)
{
    File_ID file_id = LoadFileForCompilation(module, path);
    
    if (file_id != INVALID_ID)
    {
        File* file = (File*)ElementAt(&module->files, file_id);
        if (ParseFile(module, file))
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
    module.files          = BUCKET_ARRAY(&module.main_arena, File, 8);
    module.string_storage = BUCKET_ARRAY(&module.main_arena, String, 256);
    module.symbol_tables  = BUCKET_ARRAY(&module.main_arena, Symbol_Table, 64);
    
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
            /// Check usage of context specific statements and expressions
            /// Check if all referenced symbols are defined and used in a valid context, and build type table
            /// Compute size of types
            /// Infer types and check for type compatibility, also check function call argument validity
            /// Check compile time assertions
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