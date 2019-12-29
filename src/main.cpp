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
    File* file = (File*)PushElement(&module.files);
    ZeroStruct(file);
    
    bool succeeded = ParseFile(&module, file, CONST_STRING("X :: ++Hello[*p & 1]++;hello: [6][6][..]int = 5;HellloFunc :: proc(n:float, f:guard_float)->int{return 2 * 2 * (5 & 4 << 1);}"));
    
    if (succeeded)
    {
        //SemaRegisterAllSymbolDeclarations(&module, file);
        PrintASTNodeAndChildrenAsSourceCode(file->parser_ast.root, true);
    }
    
    //PrintASTNodeAndChildren(file->ast.root, 0);
    Print("\n%s\n", (succeeded ? "succeeded" : "failed"));
    
    return 0;
}