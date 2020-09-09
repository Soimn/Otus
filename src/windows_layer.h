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
PrintCString(const char* cstring)
{
    String string = {0};
    string.data = (u8*)cstring;
    
    for (char* scan = (char*)cstring; *scan; ++scan) ++string.size;
    
    PrintString(string);
}

void
PrintString(String string)
{
    ASSERT(string.size <= U32_MAX);
    
    HANDLE stdout_handle = GetStdHandle((DWORD)-11);
    WriteConsole(stdout_handle, string.data, (u32)string.size, 0, 0);
}

Mutex
Mutex_Init()
{
    Mutex result = {0};
    
    NOT_IMPLEMENTED;
    
    return result;
}

void
Mutex_Lock(Mutex m)
{
    NOT_IMPLEMENTED;
}

void
Mutex_Unlock(Mutex m)
{
    NOT_IMPLEMENTED;
}