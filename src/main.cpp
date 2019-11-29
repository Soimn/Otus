#include "common.h"

#include "lexer.h"
#include "parser.h"
#include "ast.h"

int
main(int argc, const char** argv)
{
    //Lexer lexer = LexString(CONST_STRING("( + ) aa.... ->->--> * & / / /*/*/**//*/**/*/*/*/ && & << < 33000000000000000000000000000"));
    /*
    Token token = {};
    while (token.type != Token_EndOfStream)
    {
        token = GetToken(&lexer);
        Print("%S", GetNameOfTokenType(token.type));
    }
    */
    
    AST_Module module = ParseModule(CONST_STRING("x :: \"hello\"; y :: (f: float, n: int) {func := (l: float) -> int {i++[1000000000000000];};}"));
    
    return module.is_valid;
}