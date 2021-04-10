#include "compiler_api.h"

#ifndef OTUS_DISABLE_ASSERTIONS
# define ASSERT(EX) ((EX) ? 1 : *(volatile int*)0)
#else
# define ASSERT(EX)
#endif

#define NOT_IMPLEMENTED ASSERT(!"NOT_IMPLEMENTED")
#define INVALID_CODE_PATH ASSERT(!"INVALID_CODE_PATH")
#define INVALID_DEFAULT_CASE default: ASSERT(!"INVALID_DEFAULT_CASE"); break

#define U8_MAX  (u8)0xFF
#define U16_MAX (u16)0xFFFF
#define U32_MAX (u32)0xFFFFFFFF
#define U64_MAX (u64)0xFFFFFFFFFFFFFFFF

#define S8_MAX  (s8)U8_MAX   >> 1
#define S16_MAX (s16)U16_MAX >> 1
#define S32_MAX (s32)U32_MAX >> 1
#define S64_MAX (s64)U64_MAX >> 1

#define CONST_STRING(s) (String){.data = (u8*)(s), .size = sizeof(s) - 1}

#define OFFSETOF(e, T) (umm)&((T*)0)->e
#define ALIGNOF(T) OFFSETOF(t, struct { char c; T t; })

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))

void*
System_AllocateMemory(umm size)
{
    void* result = 0;
    
    NOT_IMPLEMENTED;
    
    return result;
}

void
System_FreeMemory(void* ptr)
{
    NOT_IMPLEMENTED;
}

#include "memory.h"
#include "string.h"
#include "lexer.h"
#include "parser.h"