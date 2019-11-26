#include "common.h"

#include "lexer.h"
#include "parser.h"

int
main(int argc, const char** argv)
{
    //Lexer lexer = LexString(CONST_STRING("( + ) aa.... ->->--> * & / / /**/ && & << <"));
    
    /*Token token = {};
    while (token.type != Token_EndOfStream)
    {
        token = GetToken(&lexer);
        Print("%S\n", GetNameOfTokenType(token.type));
    }*/
    
    return ParseModule(CONST_STRING("x :: \"hello\"; y :: (f: float, n: int) {func := (l: float) -> int {i++[];};}"));
}