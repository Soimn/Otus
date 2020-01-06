inline void
PrintChar(char c)
{
    putchar(c);
}

///////////////////////////////////////////////////////////////////////////////////

inline char
ToLower(char c)
{
    char result = c;
    
    if (c >= 'A' && c <= 'Z')
    {
        result -= 'A';
        result += 'a';
    }
    
    return result;
}

inline char
ToUpper(char c)
{
    char result = c;
    
    if (c >= 'a' && c <= 'z')
    {
        result -= 'a';
        result += 'A';
    }
    
    return result;
}

inline bool
IsAlpha(char c)
{
    return (c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z');
}

inline bool
IsDigit(char c)
{
    return (c >= '0' && c <= '9');
}

inline bool
IsASCII(char c)
{
    return ((U8)c <= I8_MAX);
}

///////////////////////////////////////////////////////////////////////////////////

inline UMM
StringLength(const char* cstring)
{
    UMM length = 0;
    
    for (U8* scan = (U8*)cstring; *scan; ++scan)
    {
        length += 1;
    }
    
    return length;
}

inline bool
StringCompare(String s0, String s1)
{
    while (s0.size && s1.size && *s0.data == *s1.data)
    {
        ++s0.data;
        --s0.size;
        
        ++s1.data;
        --s1.size;
    }
    
    return (s0.size == s1.size);
}

///////////////////////////////////////////////////////////////////////////////////

inline void
Print(const char* format, Arg_List arg_list)
{
    for (char* scan = (char*)format; *scan; ++scan)
    {
        if (*scan == '%')
        {
            ++scan;
            ASSERT(*scan != 0);
            
            if (*scan == '%')
            {
                PrintChar('%');
            }
            
            else if (*scan == 's')
            {
                const char* cstring = va_arg(arg_list.list, const char*);
                
                for (char* cstring_scan = (char*)cstring; *cstring_scan; ++cstring_scan)
                {
                    PrintChar(*cstring_scan);
                }
            }
            
            else if (*scan == 'S')
            {
                String string = va_arg(arg_list.list, String);
                
                for (U32 i = 0; i < string.size; ++i)
                {
                    PrintChar((char)string.data[i]);
                }
            }
            
            else if (ToLower(*scan) == 'u' || ToLower(*scan) == 'i')
            {
                U64 number = 0;
                
                if (*scan == 'u')
                {
                    number = va_arg(arg_list.list, U32);
                }
                
                else if (*scan == 'U')
                {
                    number = va_arg(arg_list.list, U64);
                }
                
                else if (*scan == 'i')
                {
                    I32 number_i32 = va_arg(arg_list.list, I32);
                    
                    if (number_i32 < 0)
                    {
                        PrintChar('-');
                        number_i32 = -number_i32;
                    }
                    
                    number = (U64)number_i32;
                }
                
                else if (*scan == 'I')
                {
                    I64 number_i64 = va_arg(arg_list.list, I64);
                    
                    if (number_i64 < 0)
                    {
                        PrintChar('-');
                        number_i64 = -number_i64;
                    }
                    
                    number = (U64)number_i64;
                }
                
                U64 largest_place = 1;
                for (U64 num_copy = number / 10; num_copy != 0; num_copy /= 10)
                {
                    largest_place *= 10;
                }
                
                while (largest_place != 0)
                {
                    U8 digit = (number / largest_place) % 10;
                    
                    PrintChar(digit + '0');
                    
                    largest_place /= 10;
                }
            }
        }
        
        else
        {
            PrintChar(*scan);
        }
    }
}

inline void
Print(const char* format, ...)
{
    Arg_List arg_list;
    va_start(arg_list.list, format);
    
    Print(format, arg_list);
    
    va_end(arg_list.list);
}