#pragma once

#include <windows.h>
#include <stdio.h>

inline void*
AllocateMemory(UMM size)
{
    void* memory = VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    ASSERT(memory != 0 && AlignOffset(memory, alignof(U64)) == 0);
    
    return memory;
}

inline void
FreeMemory(void* ptr)
{
    U32 result = VirtualFree(ptr, 0, MEM_RELEASE);
    ASSERT(result != 0);
}

inline void
PrintChar(char c)
{
    putchar(c);
}

inline void
PrintCString(const char* cstring)
{
    puts(cstring);
}

inline String
GetDirFromFilePath(String path)
{
    String result = {};
    
    NOT_IMPLEMENTED;
    
    return result;
}

inline bool
TryResolveFilePath(struct Memory_Arena* arena, String current_dir, String path, String* abs_path)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

inline bool
TryLoadFileContents(struct Memory_Arena* arena, String path, String* contents)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

inline void
LockMutex(Mutex* mutex)
{
    NOT_IMPLEMENTED;
}

inline void
UnlockMutex(Mutex* mutex)
{
    NOT_IMPLEMENTED;
}