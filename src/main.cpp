#include "common.h"

#include "lexer.h"
#include "parser.h"

int
main(int argc, const char** argv)
{
    //Lexer lexer = LexString(CONST_STRING("( + ) aa.... ->->--> * & / / /*/*/**//*/**/*/*/*/ && & << < 3300000000000000000"));
    /*
    Token token = {};
    while (token.type != Token_EndOfStream)
    {
        token = GetToken(&lexer);
        Print("%S", GetNameOfTokenType(token.type));
        
        if (token.type == Token_Number)
        {
            if (token.number.type == Token_Number_U64)
            {
                Print(" : U64 : %U", token.number.u64);
            }
            
            else if (token.number.type == Token_Number_F32)
            {
                printf(" : F32 : %f", token.number.f32);
            }
            
            else if (token.number.type == Token_Number_F64)
            {
                printf(" : F64 : %f", token.number.f64);
            }
        }
        
        Print("\n");
    }
    */
    
    return ParseModule(CONST_STRING("x :: \"hello\"; y :: (f: float, n: int) {func := (l: float) -> int {i++[];};}"));
}