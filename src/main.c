#include "common.h"
#include "string.h"
#include "memory.h"
#include "ast.h"
#include "workspace.h"
#include "lexer.h"
#include "sema.h"
#include "parser.h"

#ifdef GREMLIN_PLATFORM_WINDOWS

#include "windows_layer.h"

#else
#error "Platform not supported"
#endif

int
main()
{
}
