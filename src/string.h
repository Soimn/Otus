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

inline bool
StringCStringCompare(String string, const char* const_cstring)
{
    char* cstring = (char*)const_cstring;
    
    while (string.size && *cstring && *string.data == *cstring)
    {
        string.data += 1;
        string.size -= 1;
        
        cstring += 1;
    }
    
    return (string.size == 0 && *cstring == 0);
}