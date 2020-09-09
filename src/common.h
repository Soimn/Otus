/// Compilation switches
//////////////////////////////////////////

// OTUS_DEBUG_MODE     - enable additional validation checking
// OTUS_DISABLE_ASSERT - disable runtime assertions

/// Foregin headers
//////////////////////////////////////////

#include <stdint.h>
#include <stdarg.h>
#include <float.h>

/// Types
//////////////////////////////////////////

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#if INTPTR_MAX > 0xFFFFFFFF
typedef u64 umm;
typedef i64 imm;
#else
typedef u32 umm;
typedef i32 imm;
#endif

typedef u32 uint;

typedef float  f32;
typedef double f64;

typedef u8  b8;
typedef u16 b16;
typedef u32 b32;
typedef u64 b64;

typedef b8 bool;

#define false 0
#define true 1

typedef struct Buffer
{
    u8* data;
    umm size;
} Buffer;

typedef Buffer String;

#define CONST_STRING(str) {.data = (u8*)(str), .size = sizeof(str) - 1}

#define U8_MAX  ((u8)0xFF)
#define U16_MAX ((u16)0xFFFF)
#define U32_MAX ((u32)0xFFFFFFFF)
#define U64_MAX ((U64)0xFFFFFFFFFFFFFFFF)

#define I8_MAX  ((i8)(U8_MAX >> 1))
#define I16_MAX ((i16)(U16_MAX >> 1))
#define I32_MAX ((i32)(U32_MAX >> 1))
#define I64_MAX ((i64)(U64_MAX >> 1))

#define I8_MIN  ((i8) 1 << 7)
#define I16_MIN ((i16)1 << 15)
#define I32_MIN ((i32)1 << 31)
#define I64_MIN ((i64)1 << 63)

#define F32_MIN FLT_MIN
#define F64_MIN DBL_MIN

#define F32_MAX FLT_MAX
#define F64_MAX DBL_MAX

#define Enum8(NAME)  u8
#define Enum16(NAME) u16
#define Enum32(NAME) u32
#define Enum64(NAME) u64

#define Flag8(NAME)  u8
#define Flag16(NAME) u16
#define Flag32(NAME) u32
#define Flag64(NAME) u64

/// IDs and indeces
//////////////////////////////////////////

typedef u32 File_ID;

/// Utility macros
//////////////////////////////////////////

#define global static

#define CONCAT_(x, y) x##y
#define CONCAT(x, y) CONCAT_(x, y)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define U64_LOW(x) (u32)(x & 0xFFFFFFFF)
#define U64_HIGH(x) (u32)(x >> 32)

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

/// Foreign
//////////////////////////////////////////

void* System_AllocateMemory(umm size);
void System_FreeMemory(void* memory);

#define Arg_List va_list
#define ARG_LIST_START va_start
#define ARG_LIST_GET_ARG va_arg

void PrintCString(const char* cstring);
void PrintString(String string);

typedef u64 Mutex;
Mutex Mutex_Init();
void Mutex_Lock(Mutex m);
void Mutex_Unlock(Mutex m);

#ifdef _WIN32

#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)

#else
#error "unsupported platform"
#endif
