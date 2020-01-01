#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

typedef int8_t   I8;
typedef int16_t  I16;
typedef int32_t  I32;
typedef int64_t  I64;

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef float    F32;
typedef double   F64;

#ifndef GREMLIN_32BIT
typedef U64 UMM;
typedef I64 IMM;
#else
typedef U32 UMM;
typedef I32 IMM;
#endif

typedef U8  B8;
typedef U16 B16;
typedef U32 B32;
typedef U64 B64;

struct Buffer
{
    U8* data;
    UMM size;
};

typedef Buffer String;

struct Number
{
    bool is_integer;
    bool is_single_precision;
    
    union
    {
        U64 u64;
        F32 f32;
        F64 f64;
    };
};

typedef U32 File_ID;

struct File_Loc
{
    File_ID file;
    U32 line;
    U32 column;
};

#define CONST_STRING(cstring) {(U8*)cstring, sizeof(cstring) - 1}

#define U8_MAX  (U8)  0xFF
#define U16_MAX (U16) 0xFFFF
#define U32_MAX (U32) 0xFFFFFFFF
#define U64_MAX (U64) 0xFFFFFFFFFFFFFFFF

#define U8_MIN  (U8)  0
#define U16_MIN (U16) 0
#define U32_MIN (U32) 0
#define U64_MIN (U64) 0

#define I8_MAX  (I8)  (U8_MAX  >> 1)
#define I16_MAX (I16) (U16_MAX >> 1)
#define I32_MAX (I32) (U32_MAX >> 1)
#define I64_MAX (I64) (U64_MAX >> 1)

#define I8_MIN  (I8)  (~I8_MAX)
#define I16_MIN (I16) (~I16_MAX)
#define I32_MIN (I32) (~I32_MAX)
#define I64_MIN (I64) (~I64_MAX)

#define Flag8(type)  U8
#define Flag16(type) U16
#define Flag32(type) U32
#define Flag64(type) U64

#define Enum8(type)  U8
#define Enum16(type) U16
#define Enum32(type) U32
#define Enum64(type) U64

#define ARRAY_COUNT(X) (sizeof(X) / sizeof((X)[0]))

#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

#ifdef GREMLIN_DEBUG
#define HARD_ASSERT(EX) if (EX); else *(volatile int*)0 = 0
#define NOT_IMPLEMENTED HARD_ASSERT(!"NOT IMPLEMENTED")
#define INVALID_CODE_PATH HARD_ASSERT(!"INVALID CODE PATH")
#define INVALID_DEFAULT_CASE default: HARD_ASSERT(!"INVALID DEFAULT CASE"); break
#else
#define HARD_ASSERT(EX)
#define NOT_IMPLEMENTED
#define INVALID_DEFAULT_CASE
#endif

inline void*
SystemAllocateMemory(UMM size)
{
    return malloc(size);
}

inline void
SystemFreeMemory(void* ptr)
{
    free(ptr);
}

inline void
PrintChar(char c)
{
    putchar(c);
}

inline void
PrintCString(const char* cstring)
{
    printf("%s", cstring);
}

#include "memory.h"
#include "string.h"


typedef U64 AST_Scope_ID;
typedef U32 Identifier_ID;
typedef U32 Symbol_Table_ID;
typedef U64 Symbol_ID;
typedef U32 Type_ID;
typedef U32 File_ID;

struct Module
{
    Memory_Arena universal_arena;
    Memory_Arena parser_arena;
    U32 parser_ast_bucket_size;
    
    AST_Scope_ID total_scope_count;
    
    Dynamic_Array identifier_table;
    Bucket_Array files;
    
    Dynamic_Array symbol_table_array;
    Bucket_Array type_table;
    
    Dynamic_Array file_loading_buffer;
};