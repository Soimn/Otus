inline UMM
StringLength(const char* cstring)
{
    UMM result = 0;
    while (*cstring++) ++result;
    
    return result;
}

inline String
WrapCString(const char* cstring)
{
    String result;
    result.data = (U8*)cstring;
    result.size = StringLength(cstring);
    
    return result;
}

inline bool
StringCompare(String s0, String s1)
{
    bool result = false;
    
    if (s0.data == s1.data && s0.size == s1.size) result = true;
    else if (s0.size == s1.size)
    {
        while (s0.size && s1.size && *s0.data == *s1.data)
        {
            ++s0.data;
            --s0.size;
            
            ++s1.data;
            --s1.size;
        }
        
        result = (s0.size == 0 && s0.size == s1.size);
    }
    
    return result;
}

inline bool
DoesStringStartWith(String string, String comparand)
{
    while (string.size && comparand.size && *string.data == *comparand.data)
    {
        ++string.data;
        --string.size;
        
        ++comparand.data;
        --comparand.size;
    }
    
    return (comparand.size == 0);
}

inline bool
StringCStringCompare(String string, const char* cstring)
{
    while (string.size != 0 && *cstring != 0)
    {
        string.data += 1;
        string.size -= 1;
        cstring      = (char*)cstring + 1;
    }
    
    return (string.size == 0 && *cstring == 0);
}

inline bool
StringConstStringCompare(String string, const char* cstring, UMM cstring_length)
{
    bool result = false;
    
    if (string.data == (U8*)cstring && string.size == cstring_length) result = true;
    else if (string.size == cstring_length)
    {
        while (string.size != 0 && *cstring != 0)
        {
            string.data += 1;
            string.size -= 1;
            cstring      = (char*)cstring + 1;
        }
        
        result = (string.size == 0 && *cstring == 0);
    }
    
    return result;
}

inline I64
StringFind(String string, char c, Enum32(FORWARDS_OR_BACKWARDS) direction)
{
    I64 resulting_index = -1;
    
    // NOTE(soimn): Strings should never exceed I64_MAX
    ASSERT(string.size <= I64_MAX);
    
    if (direction == FORWARDS)
    {
        for (UMM i = 0; i < string.size; ++i)
        {
            if (string.data[0] == c)
            {
                resulting_index = i;
                break;
            }
        }
    }
    
    else
    {
        for (IMM i = string.size - 1; i >= 0; --i)
        {
            if (string.data[0] == c)
            {
                resulting_index = i;
                break;
            }
        }
    }
    
    return resulting_index;
}

inline String
SubString(String s, UMM offset, IMM length)
{
    String result = s;
    result.data  += offset;
    result.size   = length;
    
    return result;
}

inline String
AdvanceString(String s, UMM amount)
{
    String result = {0};
    
    if (amount < s.size)
    {
        result.data = s.data + amount;
        result.size = s.size - amount;
    }
    
    return result;
}

bool
IsAlpha(char c)
{
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

bool
IsDigit(char c)
{
    return (c >= '0' && c <= '9');
}

char
ToUpper(char c)
{
    return (c >= 'a' && c <= 'z' ? (c - 'a') + 'A' : c);
}

char
ToLower(char c)
{
    return (c >= 'A' && c <= 'Z' ? (c - 'A') + 'a' : c);
}

UMM
FormatInteger(Buffer memory, U64 value)
{
    String result = {.data = memory.data, .size = 0};
    
    U64 value_copy      = value;
    U64 largest_place   = 1;
    UMM required_memory = 0;
    
    do {
        value_copy      /= 10;
        largest_place   *= 10;
        required_memory += 1;
    } while (value_copy != 0);
    
    do
    {
        U8 digit = (value / largest_place) % 10;
        
        if (result.size < memory.size)
        {
            result.data[result.size - 1] = digit + '0';
            ++result.size;
        }
        
        largest_place /= 10;
        value         /= 10;
    } while (value != 0);
    
    return required_memory;
}

UMM
FormatString(Buffer memory, String format_string, Arg_List arg_list)
{
    UMM required_memory = 0;
    
    while (format_string.size != 0)
    {
        if (format_string.data[0] == '%')
        {
            ++format_string.data;
            --format_string.size;
            
            ASSERT(format_string.size != 0);
            
            char c = format_string.data[0];
            ++format_string.data;
            --format_string.size;
            
            switch (c)
            {
                case '%':
                {
                    if (memory.size)
                    {
                        memory.data[0] = c;
                        ++memory.data;
                        --memory.size;
                    }
                    
                    ++required_memory;
                } break;
                
                case 'f':
                case 'F':
                {
                    F64 value;
                    
                    if (c == 'f') value = ARG_LIST_GET_ARG(arg_list, F32);
                    else value          = ARG_LIST_GET_ARG(arg_list, F64);
                    
                    U64 int_value = (U64)value;
                    
                    if (value - int_value < 1)
                    {
                        value -= int_value;
                        
                        UMM integer_length = FormatInteger(memory, int_value);
                        required_memory += integer_length;
                        memory.size     -= (memory.size < integer_length ? 0 : integer_length);
                    }
                    
                    else
                    {
                        NOT_IMPLEMENTED;
                    }
                    
                    if (memory.size)
                    {
                        memory.data[0] = '.';
                        ++memory.data;
                        --memory.size;
                    }
                    
                    ++required_memory;
                    
                    for (U32 i = 0; i < 7; ++i)
                    {
                        value *= 10;
                        
                        if (memory.size)
                        {
                            memory.data[0] = (U8)value + '0';
                            ++memory.data;
                            --memory.size;
                        }
                        
                        ++required_memory;
                        
                        if (value == 0) break;
                    }
                } break;
                
                case 'u':
                case 'U':
                case 'i':
                case 'I':
                {
                    U64 value   = 0;
                    I64 i_value = 0;
                    
                    if (c == 'u') value        = ARG_LIST_GET_ARG(arg_list, U32);
                    else if (c == 'U') value   = ARG_LIST_GET_ARG(arg_list, U64);
                    else if (c == 'i') i_value = ARG_LIST_GET_ARG(arg_list, I32);
                    else i_value               = ARG_LIST_GET_ARG(arg_list, I64);
                    
                    if (i_value < 0)
                    {
                        i_value = -i_value;
                        
                        if (memory.size)
                        {
                            memory.data[0] = '-';
                            ++memory.data;
                            --memory.size;
                        }
                        
                        ++required_memory;
                    }
                    
                    value = i_value;
                    
                    UMM integer_length = FormatInteger(memory, value);
                    required_memory += integer_length;
                    memory.size     -= (memory.size < integer_length ? 0 : integer_length);
                } break;
                
                case 's':
                {
                    const char* cstring = ARG_LIST_GET_ARG(arg_list, const char*);
                    
                    while (*cstring)
                    {
                        if (memory.size)
                        {
                            memory.data[0] = *cstring;
                            ++memory.data;
                            --memory.size;
                        }
                        
                        ++required_memory;
                    }
                } break;
                
                case 'S':
                {
                    String string = ARG_LIST_GET_ARG(arg_list, String);
                    
                    for (UMM i = 0; i < string.size; ++i)
                    {
                        if (memory.size)
                        {
                            memory.data[0] = string.data[i];
                            ++memory.data;
                            --memory.size;
                        }
                        
                        ++required_memory;
                    }
                } break;
                
                INVALID_DEFAULT_CASE;
            }
        }
        
        else
        {
            if (memory.size)
            {
                memory.data[0] = format_string.data[0];
                ++memory.data;
                --memory.size;
            }
            
            ++format_string.data;
            --format_string.size;
            ++required_memory;
        }
    }
    
    return required_memory;
}

UMM
FormatCString(Buffer memory, const char* format_string, ...)
{
    Arg_List arg_list;
    ARG_LIST_START(arg_list, format_string);
    return FormatString(memory, WrapCString(format_string), arg_list);
}