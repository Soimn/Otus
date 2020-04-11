/// Foregin headers
//////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <float.h>
#include <math.h>

#ifdef _WIN32

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

#undef near
#undef far

#else
#endif

/// Types
//////////////////////////////////////////

typedef int8_t  I8;
typedef int16_t I16;
typedef int32_t I32;
typedef int64_t I64;

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

#if INTPTR_MAX > 0xFFFFFFFF
typedef U64 UMM;
typedef I64 IMM;
#else
typedef U32 UMM;
typedef I32 IMM;
#endif

typedef U32 uint;

typedef float  F32;
typedef double F64;

typedef U8  B8;
typedef U16 B16;
typedef U32 B32;
typedef U64 B64;

typedef B8 bool;

#define false 0
#define true 1

/// Utility macros
//////////////////////////////////////////

#define CONCAT_(x, y) x##y
#define CONCAT(x, y) CONCAT_(x, y)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define U64_LOW(x) (U32)(x & 0xFFFFFFFF)
#define U64_HIGH(x) (U32)(x >> 32)

#define BYTES(X)     (UMM)(X)
#define KILOBYTES(X) (BYTES(X)     * 1024ULL)
#define MEGABYTES(X) (KILOBYTES(X) * 1024ULL)
#define GIGABYTES(X) (MEGABYTES(X) * 1024ULL)

#define global static
#define internal static
#define local_persist static

#ifndef GREMLIN_DISABLE_ASSERT
#define ASSERT(EX) if (EX); else *(volatile int*)0 = 0
#define STATIC_ASSERT(EX) static_assert((EX), "Assertion failed: " #EX)
#else
#define ASSERT(EX)
#define STATIC_ASSERT(EX)
#endif

#define INVALID_CODE_PATH ASSERT(!"INVALID_CODE_PATH")
#define INVALID_DEFAULT_CASE default: ASSERT(!"INVALID_DEFAULT_CASE")
#define NOT_IMPLEMENTED ASSERT(!"NOT_IMPLEMENTED")

#define OFFSETOF(T, E) (UMM)&((T*) 0)->E
#define ALIGNOF(T) (U8)OFFSETOF(struct { char c; T type; }, type)

#define ARRAY_COUNT(EX) (sizeof(EX) / sizeof((EX)[0]))

#define CONST_STRING(str) (str), sizeof((str)[0]) - 1

#define U8_MAX  ((U8)0xFF)
#define U16_MAX ((U16)0xFFFF)
#define U32_MAX ((U32)0xFFFFFFFF)
#define U64_MAX ((U64)0xFFFFFFFFFFFFFFFF)

#define I8_MAX  ((I8)(U8_MAX >> 1))
#define I16_MAX ((I16)(U16_MAX >> 1))
#define I32_MAX ((I32)(U32_MAX >> 1))
#define I64_MAX ((I64)(U64_MAX >> 1))

#define I8_MIN  ((I8) 1 << 7)
#define I16_MIN ((I16)1 << 15)
#define I32_MIN ((I32)1 << 31)
#define I64_MIN ((I64)1 << 63)

#define Enum8(NAME)  U8
#define Enum16(NAME) U16
#define Enum32(NAME) U32
#define Enum64(NAME) U64

#define Flag8(NAME)  U8
#define Flag16(NAME) U16
#define Flag32(NAME) U32
#define Flag64(NAME) U64

#define NULL_IDENTIFIER 0
#define BLANK_IDENTIFIER 1

/// Common types
//////////////////////////////////////////

typedef struct Buffer
{
    U8* data;
    UMM size;
} Buffer;

typedef Buffer String;

typedef UMM Identifier;
typedef UMM String_Literal;
typedef U32 Character;

typedef struct Number
{
    union
    {
        U64 u64;
        F64 f64;
    };
    
    U8 min_fit_bits;
    bool is_f64;
} Number;

typedef U32 File_ID;
typedef U32 Scope_ID;
typedef U64 Scope_Index;

typedef struct Text_Pos
{
    File_ID file_id;
    UMM offset_to_line;
    UMM line;
    UMM column;
} Text_Pos;

typedef struct Text_Interval
{
    Text_Pos position;
    UMM size;
} Text_Interval;

enum FORWARDS_OR_BACKWARDS
{
    BACKWARDS = 0,
    FORWARDS  = 1,
};

/// Foreign
//////////////////////////////////////////

#define F32_MIN FLT_MIN // 1.4012984643e-45
#define F32_MAX FLT_MAX // 3.4028234664e+38

#define F64_MIN DBL_MIN
#define F64_MAX DBL_MAX

#define F32_EPSILON FLT_EPSILON
#define F64_EPSILON DBL_EPSILON

#define IS_INF(n) isinf(n)

#define Arg_List va_list
#define ARG_LIST_START va_start
#define ARG_LIST_GET_ARG va_arg

typedef struct Mutex
{
    U32 _;
} Mutex;

void
TryLockMutex(Mutex mutex)
{
    NOT_IMPLEMENTED;
}

void
UnlockMutex(Mutex mutex)
{
    NOT_IMPLEMENTED;
}

void*
Malloc(UMM size)
{
    return malloc(size);
}

void
Free(void* ptr)
{
    free(ptr);
}

void
PrintCString(const char* cstring)
{
    puts(cstring);
}

void
PrintString(String string)
{
    for (U32 i = 0; i < string.size; ++i)
    {
        putchar(string.data[i]);
    }
}

void
PrintChar(char c)
{
    putchar(c);
}

/// Common utility functions
//////////////////////////////////////////

bool
IsPowerOf2(U64 n)
{
    return (n == ((~n + 1) & n));
}