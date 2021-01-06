#include "compiler_api.h"

// TODO(soimn): Remove these
#include <stdlib.h> // malloc & free
#include <stdio.h>  // printf and putc
#include <stdarg.h> // va_*

#define CONST_STRING(str) (String){.data = (u8*)(str), .size = sizeof(str) - 1}

#define U8_MAX  ((u8)0xFF)
#define U16_MAX ((u16)0xFFFF)
#define U32_MAX ((u32)0xFFFFFFFF)
#define U64_MAX ((u64)0xFFFFFFFFFFFFFFFF)

#define I8_MAX  ((i8)(U8_MAX >> 1))
#define I16_MAX ((i16)(U16_MAX >> 1))
#define I32_MAX ((i32)(U32_MAX >> 1))
#define I64_MAX ((i64)(U64_MAX >> 1))

#define I8_MIN  ((i8) 1 << 7)
#define I16_MIN ((i16)1 << 15)
#define I32_MIN ((i32)1 << 31)
#define I64_MIN ((i64)1 << 63)

#define CONCAT_(x, y) x##y
#define CONCAT(x, y) CONCAT_(x, y)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define BYTES(X)     (umm)(X)
#define KILOBYTES(X) (BYTES(X)     * 1024ULL)
#define MEGABYTES(X) (KILOBYTES(X) * 1024ULL)
#define GIGABYTES(X) (MEGABYTES(X) * 1024ULL)

#ifndef OTUS_DISABLE_ASSERT
#define ASSERT(EX) ((EX) ? 0 : *(volatile int*)0)
#else
#define ASSERT(EX)
#endif

#define INVALID_CODE_PATH ASSERT(!"INVALID_CODE_PATH")
#define INVALID_DEFAULT_CASE default: ASSERT(!"INVALID_DEFAULT_CASE")
#define NOT_IMPLEMENTED ASSERT(!"NOT_IMPLEMENTED")

#define OFFSETOF(T, E) (umm)&((T*)0)->E
#define ALIGNOF(T) (u8)OFFSETOF(struct { char c; T type; }, type)

#define ARRAY_COUNT(EX) (sizeof(EX) / sizeof((EX)[0]))

#define Slice_ElementAt(array, type, index) (type*)((u8*)(array)->data + sizeof(type) * (index))
#define DynamicArray_ElementAt(array, type, index) (type*)((u8*)(array)->data + sizeof(type) * (index))
#define SLICE_TO_DYNAMIC_ARRAY(slice) (Dynamic_Array){.data = (slice).data, .size = (slice).size}

//////////////////////////////////////////

void*
System_AllocateMemory(umm size)
{
    void* result = malloc(size);
    ASSERT(result != 0 && (((umm)result + 7) & ~7) == (umm)result);
    
    for (u8* scan = result; scan < (u8*)result + size; ++scan)
    {
        *scan = 0;
    }
    
    return result;
}

void
System_FreeMemory(void* memory)
{
    free(memory);
}

#define Arg_List va_list
#define ARG_LIST_START va_start
#define ARG_LIST_GET_ARG va_arg

void
System_PrintChar(char c)
{
    putchar(c);
}

void
System_PrintString(String string)
{
    printf("%.*s", (int)string.size, string.data);
}

struct Memory_Arena;
bool System_ReadEntireFile(String path, struct Memory_Arena* arena, String* content);

//////////////////////////////////////////

inline Text_Interval
TextInterval(Text_Pos pos, u32 size)
{
    Text_Interval interval = {
        .pos  = pos,
        .size = size
    };
    
    return interval;
}

inline Text_Interval
TextInterval_Merge(Text_Interval i0, Text_Interval i1)
{
    ASSERT(i0.pos.file == i1.pos.file);
    ASSERT(i0.pos.offset_to_line + i0.pos.column + i0.size <= i1.pos.offset_to_line + i1.pos.column + i1.size);
    
    Text_Interval interval = {
        .pos  = i0.pos,
        .size = (i1.size + i1.pos.column + i1.pos.offset_to_line) - (i0.pos.column + i0.pos.offset_to_line)
    };
    
    return interval;
}

inline Text_Interval
TextInterval_FromEndpoints(Text_Pos p0, Text_Pos p1)
{
    ASSERT(p0.file == p1.file);
    ASSERT(p0.offset_to_line + p0.column <= p1.offset_to_line + p1.column);
    
    u32 size = (p1.column + p1.offset_to_line) - (p0.column + p0.offset_to_line);
    
    return TextInterval(p0, size);
}

//////////////////////////////////////////

#include "memory.h"
#include "string.h"

typedef struct Compiler_State
{
    Workspace* workspace;
    Memory_Arena persistent_memory;  // NOTE(soimn): is never cleared
    Memory_Arena transient_memory;   // NOTE(soimn): is cleared between every file
    Memory_Arena temp_memory;        // NOTE(soimn): is often cleared
} Compiler_State;

#include "lexer.h"
#include "parser.h"

#if 0
OLD

ParseEverything -> ParsedStack

while decl_pile != empty
{
    FetchDecl;
    TypeCheckDecl;
    
    ResubmitDecl;
    SubmitNewDecl;
    
    ModifyDecl;
    
    GenerateBytecodeForDecl;
    
    CommitDecl;
}

BytecodeStack

Runnable

GenerateBinary
#endif

#if 0
NEW

OpenWorkspace

ParseFile

while statement_pile != empty
{
    FetchStatement;
    TypeCheckStatement;
    
    ResubmitStatement;
    SubmitNewStatement;
    
    CommitDeclaration;
}

GenerateBinary

CloseWorkspace

#endif