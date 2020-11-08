#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

#undef near
#undef far

void*
System_AllocateMemory(umm size)
{
    void* memory = VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    ASSERT(memory);
    
    return memory;
}

void
System_FreeMemory(void* memory)
{
    VirtualFree(memory, 0, MEM_RELEASE);
}

void
PrintChar(char c)
{
    HANDLE stdout_handle = GetStdHandle((DWORD)-11);
    WriteConsoleA(stdout_handle, &c, 1, 0, 0);
}

void
PrintString(String string)
{
    ASSERT(string.size <= U32_MAX);
    
    HANDLE stdout_handle = GetStdHandle((DWORD)-11);
    WriteConsoleA(stdout_handle, string.data, (u32)string.size, 0, 0);
}