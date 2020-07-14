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
PrintCString(const char* cstring)
{
    String string = {0};
    string.data = (U8*)cstring;
    
    for (char* charp = (char*)cstring; *charp; ++charp) ++string.size;
    
    PrintString(string);
}

void
PrintString(String string)
{
    ASSERT(string.size <= U32_MAX);
    
    HANDLE stdout_handle = GetStdHandle((DWORD)-11);
    WriteConsole(stdout_handle, string.data, (U32)string.size, 0, 0);
}

void
PrintToBuffer(Bucket_Array(char)* buffer, const char* message, ...)
{
    Arg_List arg_list;
    ARG_LIST_START(arg_list, message);
    
    for (char* scan = (char*)message; *scan; ++scan)
    {
        if (*scan == '%')
        {
            ++scan;
            
            if (*scan == '%')
            {
                char* c = BucketArray_Append(buffer);
                *c = '%';
            }
            
            else if (*scan == 's')
            {
                char* cstring = ARG_LIST_GET_ARG(arg_list, char*);
                
                for (; *cstring; ++cstring)
                {
                    char* c = BucketArray_Append(buffer);
                    *c = *cstring;
                }
            }
            
            else if (*scan == 'S')
            {
                String string = ARG_LIST_GET_ARG(arg_list, String);
                
                for (UMM i = 0; i < string.size; ++i)
                {
                    char* c = BucketArray_Append(buffer);
                    *c = string.data[i];
                }
            }
            
            else if (*scan == 'c')
            {
                char* c = BucketArray_Append(buffer);
                *c = ARG_LIST_GET_ARG(arg_list, char);
                
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
                        
                        char* c = BucketArray_Append(buffer);
                        *c = '-';
                    }
                    
                    number = (U64)i32;
                }
                
                else
                {
                    I64 i64 = ARG_LIST_GET_ARG(arg_list, I64);
                    if (i64 < 0)
                    {
                        i64 = -i64;
                        
                        char* c = BucketArray_Append(buffer);
                        *c = '-';
                    }
                    
                    number = (U64)i64;
                }
                
                U64 largest_place = 1;
                for (U64 num_cpy = number / 10; num_cpy != 0; num_cpy /= 10) largest_place *= 10;
                
                for (;;)
                {
                    char* c = BucketArray_Append(buffer);
                    *c= '0' + (U8)((number / largest_place) % 10);
                    
                    largest_place /= 10;
                    
                    if (number == 0) break;
                }
            }
            
            else if (*scan == 'f')
            {
                NOT_IMPLEMENTED;
            }
            
            else INVALID_CODE_PATH;
        }
        
        else
        {
            char* c = BucketArray_Append(buffer);
            *c = *scan;
        }
    }
}

bool
ReadEntireFile(Memory_Allocator* allocator, String path, String* content)
{
    bool encountered_errors = false;
    
    // TODO(soimn): Load, translate and null terminate file contents
    NOT_IMPLEMENTED;
    
    return encountered_errors;
}

Mutex
Mutex_Init()
{
    Mutex mutex = {0};
    
    NOT_IMPLEMENTED;
    
    return mutex;
}

void
Mutex_Lock(Mutex mutex)
{
    NOT_IMPLEMENTED;
}

void
Mutex_Unlock(Mutex mutex)
{
    NOT_IMPLEMENTED;
}