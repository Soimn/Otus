#pragma once

/// Types
//////////////////////////////////////////

#ifdef _WIN32
typedef signed __int8  I8;
typedef signed __int16 I16;
typedef signed __int32 I32;
typedef signed __int64 I64;

typedef unsigned __int8  U8;
typedef unsigned __int16 U16;
typedef unsigned __int32 U32;
typedef unsigned __int64 U64;

#ifdef _WIN64
typedef U64 UMM;
typedef I64 IMM;
#else
typedef U32 UMM;
typedef I32 IMM;
#endif

#else

#include <stdint.h>
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

#endif

typedef float  F32;
typedef double F64;

typedef U8  B8;
typedef U16 B16;
typedef U32 B32;
typedef U64 B64;

/// Utility macros
//////////////////////////////////////////

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#ifndef GREMLIN_DISABLE_ASSERT
#define ASSERT(EX) if (EX); else *(volatile int*)0 = 0
#else
#define ASSERT(EX)
#endif

#define INVALID_DEFAULT_CASE default: ASSERT(!"INVALID_DEFAULT_CASE")
#define NOT_IMPLEMENTED ASSERT(!"NOT_IMPLEMENTED")

#define CONST_STRING(str) String{(U8*)(str), sizeof((str)[0]) - 1}

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

#define INVALID_ID (ID)-1
#define INDIRECT_ID_ASSEMBLE(first, last) ((Indirect_ID)(first) | ((Indirect_ID)(last) << 32))
#define INVALID_INDIRECT_ID INDIRECT_ID_ASSEMBLE(INVALID_ID, INVALID_ID)
#define INDIRECT_ID_FIRST_PART(indirect_id) (ID)(indirect_id & 0xFFFFFFFF)
#define INDIRECT_ID_LAST_PART(indirect_id) (ID)((indirect_id >> 32) & 0xFFFFFFFF)

/// Common types
//////////////////////////////////////////

struct Buffer
{
    U8* data;
    UMM size;
};

typedef Buffer String;
typedef I32 ID;
typedef I64 Indirect_ID;

typedef ID File_ID;
typedef ID String_ID;
typedef ID Symbol_Table_ID;
typedef ID Type_ID;
typedef Indirect_ID Symbol_ID;

struct Number
{
    union
    {
        U64 u64;
        F32 f32;
    };
    
    bool is_u64;
    bool is_f32;
};

struct Mutex
{
    // TODO(soimn): Implement this
};

/// Platform layer functions
//////////////////////////////////////////

inline void* AllocateMemory(UMM size);
inline void FreeMemory(void* ptr);
inline void PrintChar(char c);
inline void PrintCString(const char* cstring);

inline String GetDirFromFilePath(String path);
inline bool TryResolveFilePath(struct Memory_Arena* arena, String current_dir, String file_path, String* out_path);
inline bool TryLoadFileContents(struct Memory_Arena* arena, String path, String* out_contents);

inline void LockMutex(Mutex* mutex);
inline void UnlockMutex(Mutex* mutex);

/// Common functions
//////////////////////////////////////////

inline UMM
StringLength(const char* cstring)
{
    UMM result = 0;
    
    U8* b_cstring = (U8*)cstring;
    while (*b_cstring)
    {
        ++result;
        ++b_cstring;
    }
    
    return result;
}

inline bool
StringCompare(String s0, String s1)
{
    bool result = false;
    
    if (s0.data == s1.data && s0.size == s1.size) result = true;
    else
    {
        while (s0.size && s1.size && *s0.data == *s1.data)
        {
            ++s0.data;
            --s0.size;
            
            ++s1.data;
            --s1.size;
        }
        
        result = (s0.size == 0 && s0.size == s1.size);
    }
    
    return result;
}