#pragma once

inline bool
IsAlpha(char c)
{
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

inline bool
IsNumeric(char c)
{
    return (c >= '0' && c <= '9');
}

inline bool
IsWhitespace(char c)
{
    return (c == ' ' || c == '\t' || c == '\v');
}

inline bool
IsEndOfLine(char c)
{
    return (c == '\n');
}

inline bool
IsSpacing(char c)
{
    return IsWhitespace(c) || IsEndOfLine(c);
}

inline char
ToLower(char c)
{
    char result = c;
    
    if (result >= 'A' && result <= 'Z')
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
    
    if (result >= 'a' && result <= 'z')
    {
        result -= 'a';
        result += 'A';
    }
    
    return result;
}

inline bool
StringCompare(String s0, String s1)
{
    while (s0.size && s1.size && s0.data[0] == s1.data[0])
    {
        ++s0.data;
        --s0.size;
        
        ++s1.data;
        --s1.size;
    }
    
    return (s0.size == 0 && s0.size == s1.size);
}

inline void
Print(String format, va_list arg_list)
{
    while (format.size)
    {
        if (*format.data == '%')
        {
            ++format.data;
            --format.size;
            
            if (format.size)
            {
                char c = *format.data;
                
                ++format.data;
                --format.size;
                
                if (c == '%')
                {
                    PrintChar('%');
                }
                
                else if (c == 's')
                {
                    const char* cstring = va_arg(arg_list, const char*);
                    PrintCString(cstring);
                }
                
                else if (c == 'S')
                {
                    String string = va_arg(arg_list, String);
                    
                    while (string.size)
                    {
                        PrintChar((char)*string.data);
                        ++string.data;
                        --string.size;
                    }
                }
                
                else if (ToLower(c) == 'u' || ToLower(c) == 'i')
                {
                    I64 number          = 0;
                    U64 unsigned_number = 0;
                    
                    if (c == 'u')
                    {
                        unsigned_number = va_arg(arg_list, U32);
                    }
                    
                    else if (c == 'i')
                    {
                        number = va_arg(arg_list, I32);
                    }
                    
                    else if (c == 'U')
                    {
                        unsigned_number = va_arg(arg_list, U64);
                    }
                    
                    else if (c == 'I')
                    {
                        number = va_arg(arg_list, I64);
                    }
                    
                    if (number < 0)
                    {
                        PrintChar('-');
                        number = -number;
                    }
                    
                    unsigned_number = number;
                    
                    U64 largest_place = 1;
                    
                    for (U64 num = unsigned_number; num != 0; num /= 10)
                    {
                        largest_place *= 10;
                    }
                    
                    while (largest_place != 0)
                    {
                        char digit = (char)(unsigned_number / largest_place);
                        
                        PrintChar(digit + '0');
                        
                        unsigned_number -= digit * largest_place;
                        largest_place /= 10;
                    }
                }
                
                else
                {
                    PrintChar('%');
                    PrintChar(c);
                }
            }
            
            else
            {
                PrintChar('%');
            }
        }
        
        else
        {
            PrintChar((char)*format.data);
            ++format.data;
            --format.size;
        }
    }
}

inline void
Print(String format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);
    
    Print(format, arg_list);
    
    va_end(arg_list);
}

inline void
Print(const char* format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);
    
    String format_string = {(U8*)format, 0};
    for (char* scan = (char*)format; *scan; ++scan, ++format_string.size);
    
    Print(format_string, arg_list);
    
    va_end(arg_list);
}