#include "common.h"

#include "lexer.h"
#include "parser.h"
#include "ast.h"

int
main(int argc, const char** argv)
{
    AST result = ParseFile(CONST_STRING("X::0 + 0 * (0 % 0);"));
    
    return 0;
}