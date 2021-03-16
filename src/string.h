bool
IsWhitespace(u8 c)
{
    return (c == ' ' || c == '\t' || c == '\v' || c == '\f');
}

bool
IsAlpha(u8 c)
{
    return (c >= 'a' && c <= 'z' ||
            c >= 'A' && c <= 'Z');
}

bool
IsDigit(u8 c)
{
    return (c >= '0' && c <= '9');
}

u8
ToUpper(u8 c)
{
    return (c >= 'a' && c <= 'z' ? (c - 'a') + 'A': c);
}

u8
ToLower(u8 c)
{
    return (c >= 'A' && c <= 'Z' ? (c - 'A') + 'a': c);
}

bool
StringCompare(String s0, String s1)
{
    if (s0.size == s1.size)
    {
        while (s0.size && *s0.data == *s1.data)
        {
            s0.data += 1;
            s0.size -= 1;
            
            s1.data += 1;
            s1.size -= 1;
        }
    }
    
    return (s0.size == s1.size);
}