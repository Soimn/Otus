inline char
ToUpper(char c)
{
    if (c >= 'a' && c <= 'z')
    {
        c = (c - 'a') + 'A';
    }
    
    return c;
}

inline char
ToLower(char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        c = (c - 'A') + 'a';
    }
    
    return c;
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
Print(Bucket_Array* array, const char* format, ...)
{
    Arg_List arg_list;
    ARG_LIST_START(arg_list, format);
    
    for (char* scan = (char*)format; *scan; )
    {
        if (*scan == '%')
        {
            ++scan;
            
            if (*scan == 0 || *scan == '%')
            {
                *BucketArray_Append(array) = '%';
                if (*scan == '%') ++scan;
            }
            
            else if (*scan == 's')
            {
                const char* cstring = ARG_LIST_GET_ARG(arg_list, const char*);
                
                for (char* c = (char*)cstring; *c; ++c)
                {
                    *BucketArray_Append(array) = *c;
                }
            }
            
            else if (*scan == 'S')
            {
                String string = ARG_LIST_GET_ARG(arg_list, String);
                
                for (umm i = 0; i < string.size; ++i)
                {
                    *BucketArray_Append(array) = string.data[i];
                }
            }
            
            else INVALID_CODE_PATH;
        }
        
        else
        {
            *BucketArray_Append(array) = *scan;
            ++scan;
        }
    }
}