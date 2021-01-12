char
ToUpper(char c)
{
    if (c >= 'a' && c <= 'z')
    {
        c = (c - 'a') + 'A';
    }
    
    return c;
}

char
ToLower(char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        c = (c - 'A') + 'a';
    }
    
    return c;
}

bool
IsAlpha(char c)
{
    return (c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z');
}

bool
IsDigit(char c)
{
    return (c >= '0' && c <= '9');
}

bool
StringCompare(String s0, String s1)
{
    while (s0.size && s1.size && *s0.data == *s1.data)
    {
        s0.data += 1;
        s0.size -= 1;
        
        s1.data += 1;
        s1.size -= 1;
    }
    
    return (s0.size == 0 && s0.size == s1.size);
}

void
PrintArgList(const char* format, Arg_List arg_list)
{
    for (char* scan = (char*)format; *scan; )
    {
        if (*scan == '%')
        {
            ++scan;
            
            if (*scan == 0 || *scan == '%')
            {
                System_PrintChar('%');
                if (*scan == '%') ++scan;
            }
            
            else if (*scan == 's')
            {
                const char* cstring = ARG_LIST_GET_ARG(arg_list, const char*);
                
                String string = { .data = (u8*)cstring };
                
                for (char* cstring_scan = (char*)cstring; *cstring_scan; ++cstring_scan)
                {
                    ++string.size;
                }
                
                System_PrintString(string);
            }
            
            else if (*scan == 'S')
            {
                String string = ARG_LIST_GET_ARG(arg_list, String);
                System_PrintString(string);
            }
            
            else if (*scan == 'U' ||
                     *scan == 'u' ||
                     *scan == 'I' ||
                     *scan == 'i')
            {
                i64 signed_number   = 0;
                u64 unsigned_number = 0;
                
                if      (*scan == 'I') signed_number   = ARG_LIST_GET_ARG(arg_list, i64);
                else if (*scan == 'i') signed_number   = ARG_LIST_GET_ARG(arg_list, i32);
                else if (*scan == 'U') unsigned_number = ARG_LIST_GET_ARG(arg_list, u64);
                else                   unsigned_number = ARG_LIST_GET_ARG(arg_list, u32);
                
                if (*scan == 'I' || *scan == 'i')
                {
                    if (signed_number < 0)
                    {
                        System_PrintChar('-');
                        signed_number *= -1;
                    }
                    
                    unsigned_number = (u64)signed_number;
                }
                
                u64 reversed_number = 0;
                while (unsigned_number != 0)
                {
                    reversed_number  = reversed_number * 10 + unsigned_number % 10;
                    unsigned_number /= 10;
                }
                
                do
                {
                    System_PrintChar(reversed_number % 10 + '0');
                    reversed_number /= 10;
                    
                } while (reversed_number != 0);
            }
            
            else INVALID_CODE_PATH;
        }
        
        else
        {
            System_PrintChar(*scan);
            ++scan;
        }
    }
}

void
Print(const char* format, ...)
{
    Arg_List arg_list;
    ARG_LIST_START(arg_list, format);
    
    PrintArgList(format, arg_list);
}