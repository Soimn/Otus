#pragma once

/// Standard library dependencies
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

// NOTE(soimn): This is defined to avoid stupid implicit casts when using varargs with overloaded functions
struct Arg_List
{
    va_list list;
};

/// Base type decalarations
///////////////////////////////////////////////////////////////////////////////

#ifdef GREMLIN_PLATFORM_WINDOWS
typedef unsigned __int8  U8;
typedef unsigned __int16 U16;
typedef unsigned __int32 U32;
typedef unsigned __int64 U64;

typedef signed __int8  I8;
typedef signed __int16 I16;
typedef signed __int32 I32;
typedef signed __int64 I64;

#if _WIN64
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

typedef float  F32;
typedef double F64;
#else
#error "Support for platforms other than windows is not yet implemented"
#endif

struct Buffer
{
    U8* data;
    UMM size;
};

typedef Buffer String;

///////////////////////////////////////////////////////////////////////////////

/// Common macro declarations
///////////////////////////////////////////////////////////////////////////////

/// Utility macros

#define U8_MIN  0
#define U16_MIN 0
#define U32_MIN 0
#define U64_MIN 0

#define U8_MAX  0xFF
#define U16_MAX 0xFFFF
#define U32_MAX 0xFFFFFFFF
#define U64_MAX 0xFFFFFFFFFFFFFFFF

#define I8_MIN  -(U8_MAX  + 1) / 2
#define I16_MIN -(U16_MAX + 1) / 2
#define I32_MIN -(U32_MAX + 1) / 2
#define I64_MIN -(U64_MAX + 1) / 2

#define I8_MAX  (U8_MAX >> 1)
#define I16_MAX (U16_MAX >> 1)
#define I32_MAX (U32_MAX >> 1)
#define I64_MAX (U64_MAX >> 1)

#define Enum8(NAME)  U8
#define Enum16(NAME) U16
#define Enum32(NAME) U32
#define Enum64(NAME) U64

#define Flag8(NAME)  U8
#define Flag16(NAME) U16
#define Flag32(NAME) U32
#define Flag64(NAME) U64

#define ARRAY_COUNT(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

#define CONST_STRING(string) {(U8*)string, sizeof(string) - 1}

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

/// Debug mode macros

#ifdef GREMLIN_DEBUG

#ifndef GREMLIN_DISABLE_ASSERTIONS
#define GREMLIN_ENABLE_ASSERTIONS
#endif

#endif

#ifdef GREMLIN_ENABLE_ASSERTIONS
// TODO(soimn): Replace this with a report and then a forced crash
#define ASSERT(EX, ...) if (EX); else *(volatile int*)0 = 0
#else
#define ASSERT(EX, ...)
#endif

#define INVALID_DEFAULT_CASE default: ASSERT(!"INVALID DEFAULT CASE"); break
#define NOT_IMPLEMENTED ASSERT(!"NOT IMPLEMENTED")

///////////////////////////////////////////////////////////////////////////////

#include "memory.h"

#define INVALID_ID (ID)-1
typedef U32 ID;

typedef ID File_ID;
typedef ID Scope_ID;
typedef ID String_ID;
typedef ID Symbol_Table_ID;

struct Symbol_ID
{
    Symbol_Table_ID symbol_table;
    ID symbol;
};

struct File_And_Line
{
    File_ID file_id;
    U32 line;
    U32 column;
};

struct Parse_Tree
{
    Bucket_Array(Parse_Node) nodes;
    struct Parse_Node* root;
};

struct AST
{
    Bucket_Array(AST_Node) nodes;
    struct AST_Node* root;
};

struct File
{
    String file_path;
    String file_contents;
    
    Bucket_Array(File_ID) imported_files;
    
    Parse_Tree parse_tree;
    AST ast;
};

struct Module
{
    Memory_Arena main_arena;
    Memory_Arena parser_arena;
    Memory_Arena string_arena;
    
    Memory_Arena file_content_arena;
    Bucket_Array(File) files;
    
    Bucket_Array(String) string_storage;
    Bucket_Array(Symbol_Table) symbol_tables;
    
    U32 total_scope_count;
    
    bool failed_to_parse_all_files;
};

// TODO(soimn): Implement BigNum for parsing
struct Number
{
    bool is_integer;
    
    union
    {
        U64 u64;
        F64 f64;
    };
};