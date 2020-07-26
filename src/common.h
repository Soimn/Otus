/// Compilation switches
//////////////////////////////////////////

// OTUS_DEBUG_MODE     - enable additional validation checking
// OTUS_DISABLE_ASSERT - disable runtime assertions

/// Foregin headers
//////////////////////////////////////////

#include <stdint.h>
#include <stdarg.h>

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

typedef struct Buffer
{
    U8* data;
    UMM size;
} Buffer;

typedef Buffer String;

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

/// IDs and indeces
//////////////////////////////////////////
typedef U32 Package_ID;
typedef U32 File_ID;
typedef U32 Symbol_Table_ID;
typedef U64 Workspace_ID;
typedef U32 Type_ID;
typedef U32 String_ID;

typedef I32 Scope_Index;

/// Compiler Options
//////////////////////////////////////////

enum OPTIMIZATION_LEVEL
{
    OptLevel_None,      // No optimizations
    OptLevel_Debug,     // Add debug info
    OptLevel_FastDebug, // Optimize for speed and add debug info
    OptLevel_Fast,      // Optimize for speed, fast compile time
    OptLevel_Fastest,   // Optimize for speed, slow compile time
    OptLevel_Small,     // Optimize for size
};

typedef struct Workspace_Options
{
    Enum32(OPTIMIZATION_LEVEL) opt_level;
} Workspace_Options;

typedef struct Binary_Options
{
    bool build_dll;
} Binary_Options;

/// Utility macros
//////////////////////////////////////////

#define global static

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

#ifndef OTUS_DISABLE_ASSERT
#define ASSERT(EX) if (EX); else *(volatile int*)0 = 0
#else
#define ASSERT(EX)
#endif

#define INVALID_CODE_PATH ASSERT(!"INVALID_CODE_PATH")
#define INVALID_DEFAULT_CASE default: ASSERT(!"INVALID_DEFAULT_CASE")
#define NOT_IMPLEMENTED ASSERT(!"NOT_IMPLEMENTED")

#define OFFSETOF(T, E) (UMM)&((T*)0)->E
#define ALIGNOF(T) (U8)OFFSETOF(struct { char c; T type; }, type)

#define ARRAY_COUNT(EX) (sizeof(EX) / sizeof((EX)[0]))

#define HIDDEN(EX) EX

#define CONST_STRING(string) {(U8*)string, sizeof(string) - 1}

/// Foreign
//////////////////////////////////////////

void* System_AllocateMemory(UMM size);
void System_FreeMemory(void* memory);

#define Arg_List va_list
#define ARG_LIST_START va_start
#define ARG_LIST_GET_ARG va_arg

void PrintCString(const char* cstring);
void PrintString(String string);

struct Bucket_Array;
void PrintToBuffer(struct Bucket_Array* buffer, const char* message, ...);

struct Memory_Allocator;
bool ReadEntireFile(struct Memory_Allocator* allocator, String path, String* content);

typedef U64 Mutex;
Mutex Mutex_Init();
void Mutex_Lock(Mutex mutex);
void Mutex_Unlock(Mutex mutex);

#ifdef _WIN32

#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)

#else
#error "unsupported platform"
#endif
