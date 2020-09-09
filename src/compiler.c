#include "common.h"
#include "memory.h"
#include "string.h"
#include "lexer.h"

#ifdef _WIN32
#include "windows_layer.h"
#else
#error "unsupported platform"
#endif

EXPORT int TEMP_FUNC() { return 32; };