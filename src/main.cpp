#include "common.h"

#include "lexer.h"
#include "ast.h"
#include "module.h"
#include "parser.h"
#include "sema.h"

int
main(int argc, const char** argv)
{
    Module module = {};
    module.universal_arena    = MEMORY_ARENA(4000);
    module.parser_arena       = MEMORY_ARENA(4000);
    module.identifier_table   = DYNAMIC_ARRAY(Identifier, 1.5f);
    module.files              = BUCKET_ARRAY(&module.universal_arena, File, 8);
    module.symbol_table_array = DYNAMIC_ARRAY(Symbol_Table_Entry, 1.1f);
    module.type_table         = BUCKET_ARRAY(&module.universal_arena, Type_Table_Entry, 256);
    
    module.parser_ast_bucket_size = 256;
    
    File_ID first_file_id;
    bool succeeded = LoadFileForCompilation(&module, CONST_STRING("path"), &first_file_id);
    
    if (succeeded)
    {
    }
    
    else
    {
        //// ERROR: Failed to load file for compilation
    }
    
    return 0;
}