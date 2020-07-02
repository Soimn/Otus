#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

#undef near
#undef far

void*
System_AllocateMemory(UMM size)
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
Print(const char* message, ...)
{
    HANDLE std_handle = GetStdHandle((DWORD)-11);
    
    CONSOLE_SCREEN_BUFFER_INFO console_info;
    GetConsoleScreenBufferInfo(std_handle, &console_info);
    
    Arg_List arg_list;
    ARG_LIST_START(arg_list, message);
    
    for (char* scan = (char*)message; *scan; ++scan)
    {
        if (*scan == '%')
        {
            ++scan;
            
            if (*scan == '%') WriteConsole(std_handle, scan, 1, 0, 0);
            
            else if (*scan == 's')
            {
                char* cstring = ARG_LIST_GET_ARG(arg_list, char*);
                
                U32 length = 0;
                for (; *cstring; ++cstring) ++length;
                
                WriteConsole(std_handle, cstring, length, 0, 0);
            }
            
            else if (*scan == 'S')
            {
                String string = ARG_LIST_GET_ARG(arg_list, String);
                
                ASSERT(string.size <= U32_MAX);
                WriteConsole(std_handle, string.data, (U32)string.size, 0, 0);
            }
            
            else if (*scan == 'c')
            {
                char c = ARG_LIST_GET_ARG(arg_list, char);
                
                WriteConsole(std_handle, &c, 1, 0, 0);
            }
            
            else if (*scan == 'u' || *scan == 'U' ||
                     *scan == 'i' || *scan == 'I')
            {
                U64 number = 0;
                
                if (*scan == 'u') number = ARG_LIST_GET_ARG(arg_list, U32);
                else if (*scan == 'U') number = ARG_LIST_GET_ARG(arg_list, U64);
                
                else if (*scan == 'i')
                {
                    I32 i32 = ARG_LIST_GET_ARG(arg_list, I32);
                    if (i32 < 0)
                    {
                        i32 = -i32;
                        
                        char minus = '-';
                        WriteConsole(std_handle, &minus, 1, 0, 0);
                    }
                    
                    number = (U64)i32;
                }
                
                else
                {
                    I64 i64 = ARG_LIST_GET_ARG(arg_list, I64);
                    if (i64 < 0)
                    {
                        i64 = -i64;
                        
                        char minus = '-';
                        WriteConsole(std_handle, &minus, 1, 0, 0);
                    }
                    
                    number = (U64)i64;
                }
                
                U64 largest_place = 1;
                for (U64 num_cpy = number / 10; num_cpy != 0; num_cpy /= 10) largest_place *= 10;
                
                for (;;)
                {
                    char digit = '0' + (U8)((number / largest_place) % 10);
                    WriteConsole(std_handle, &digit, 1, 0, 0);
                    
                    largest_place /= 10;
                    
                    if (number == 0) break;
                }
            }
            
            else if (*scan == 'f')
            {
                NOT_IMPLEMENTED;
            }
            
            else if (*scan == 'C')
            {
                NOT_IMPLEMENTED;
            }
            
            else INVALID_CODE_PATH;
        }
        
        else
        {
            WriteConsole(std_handle, scan, 1, 0, 0);
        }
    }
    
    SetConsoleTextAttribute(std_handle, console_info.wAttributes);
}