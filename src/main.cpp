#include "common.h"

#include "memory.h"
#include "ast.h"
#include "module.h"
#include "lexer.h"
#include "parser.h"

#ifdef _WIN32
#include "windows_layer.h"
#else
#error "Support for the current platform is not yet available"
#endif

int
main()
{
    String input_string = CONST_STRING("X :: 7 + 5 * 4;");
    
    return 0;
}