enum TOKEN_KIND
{
    Token_Invalid = 0,
    
    Token_Plus,
    Token_Minus,
    Token_Asterisk,
    Token_Slash,
    Token_Mod,
    Token_BitAnd,
    Token_BitOr,
    Token_BitNot,
    Token_BitShiftL,
    Token_BitShiftR,
    Token_And,
    Token_Or,
    Token_Not,
    Token_Less,
    Token_Greater,
    Token_Equals,
    
    Token_PlusEQ,
    Token_MinusEQ,
    Token_MulEQ,
    Token_DivEQ,
    Token_ModEQ,
    Token_BitAndEQ,
    Token_BitOrEQ,
    Token_BitShiftLEQ,
    Token_BitShiftREQ,
    Token_AndEQ,
    Token_OrEQ,
    Token_LessEQ,
    Token_GreaterEQ,
    Token_IsEqual,
    
    Token_Hat,
    Token_Cash,
    Token_At,
    Token_Hash,
    Token_Period,
    Token_Colon,
    Token_Comma,
    Token_Semicolon,
    Token_Underscore,
    
    Token_LArrow,      // <-
    Token_RArrow,      // ->
    Token_ThickRArrow, // =>
    Token_Inc,
    Token_Dec,
    
    Token_OpenParen,
    Token_CloseParen,
    Token_OpenBrace,
    Token_CloseBrace,
    Token_OpenBracket,
    Token_CloseBracket,
    
    Token_Char,
    Token_Number,
    Token_String,
    Token_Identifier,
    
    Token_EOF,
};

#define KEYWORD_LIST()               \
X(Keyword_Foreign , "foreign"),  \
X(Keyword_Import  , "import"),   \
X(Keyword_As      , "as"),       \
X(Keyword_If      , "if"),       \
X(Keyword_Else    , "else"),     \
X(Keyword_Break   , "break"),    \
X(Keyword_Continue, "continue"), \
X(Keyword_Using   , "using"),    \
X(Keyword_Return  , "return"),   \
X(Keyword_Defer   , "defer"),    \
X(Keyword_Proc    , "proc"),     \
X(Keyword_Struct  , "struct"),   \
X(Keyword_Union   , "union"),    \
X(Keyword_Enum    , "enum"),     \
X(Keyword_True    , "true"),     \
X(Keyword_False   , "false")

enum KEYWORD_KIND
{
    Keyword_Invalid = 0,
    
#define X(Key, String) Key
    KEYWORD_LIST()
#undef X
    
    KEYWORD_KIND_COUNT
};

String KeywordStrings[KEYWORD_KIND_COUNT] = {
    
#define X(Key, String) [Key] = CONST_STRING(String)
    KEYWORD_LIST()
#undef X
    
};

typedef struct Number
{
    union
    {
        f64 floating;
        u64 integer;
    };
    
    bool is_float;
    u8 min_fit_bits;
} Number;

typedef struct Text_Pos
{
    File_ID file;
    uint offset_to_line;
    uint line;
    uint column;
} Text_Pos;

typedef struct Text_Interval
{
    Text_Pos position;
    uint size;
} Text_Interval;

typedef struct Token
{
    Text_Interval text;
    Enum8(TOKEN_KIND) kind;
    
    union
    {
        Number number;
        
        struct
        {
            String string;
            Enum8(KEYWORD_KIND) keyword;
        };
    };
} Token;

Text_Interval
TextInterval(Text_Pos pos, uint size)
{
    Text_Interval result = {
        .position = pos,
        .size     = size
    };
    
    return result;
}

Text_Interval
TextIntervalFromEndpoints(Text_Pos start, Text_Pos end)
{
    ASSERT(start.file == end.file);
    
    imm delta = (end.offset_to_line + end.column) + (start.offset_to_line + start.column);
    ASSERT(delta >= 0);
    
    Text_Interval result = {
        .position = start,
        .size     = (uint)delta;
    };
    
    return result;
}

Text_Interval
TextInterval_Merge(Text_Interval i0, Text_Interval i1)
{
    ASSERT(i0.position.file == i1.position.file);
    
    Text_Interval result = {0};
    
    imm delta = (i1.position.column + i1.position.offset_to_line) - (i0.position.column + i0.position.offset_to_line);
    ASSERT(delta >= 0);
    
    result.position = i0.position;
    result.size     = MAX(i0.size, (uint)delta + i1.size);
    
    return result;
}

typedef struct Lexer
{
    File_ID file;
    u8* start;
    
    Token token;
    bool token_valid;
} Lexer;

void
LexerReportError(Text_Interval interval, const char* message, ...)
{
    Arg_List arg_list;
    ARG_LIST_START(arg_list, format);
    
    Print("%s(%d:%d): ", FileNameFromID(interval.position.file_id), interval.position.line, interval.position.column);
    PrintArgList(format, arg_list);
}

Token
GetToken(Lexer* lexer)
{
    // NOTE(soimn): Guard against GetToken being called after encountering an erroneous token
    ASSERT(!lexer->token_valid || lexer->token.kind != Token_Invalid);
    
    Token token = {.kind = Token_Invalid};
    
    if (lexer->token_valid) token = lexer->token;
    else
    {
        Text_Pos base_pos = lexer->token.text.position;
        base_pos.column  += lexer->token.text.size;
        
        u8* current = lexer->start + base_pos.offset_to_line + base_pos.column;
        
        /// --- Skip whitespace and comments
        {
            uint nest_level = 0;
            bool is_comment = false;
            while (*current != 0)
            {
                if (*current == ' '  ||
                    *current == '\t' ||
                    *current == '\v' ||
                    *current == '\r')
                {
                    ++current;
                }
                
                else if (*current == '\n')
                {
                    ++current;
                    ++base_pos.line;
                    base_pos.offset_to_line = (u32)(current - lexer->start);
                    
                    if (nest_level == 0) is_comment = false;
                }
                
                else if (current[0] == '/' && current[1] == '/')
                {
                    current   += 2;
                    is_comment = true;
                }
                
                else if (current[0] == '/' && current[1] == '*')
                {
                    current += 2;
                    
                    is_comment = true;
                    ++nest_level;
                }
                
                else if (current[0] == '*' && current[1] == '/')
                {
                    if (nest_level == 0) break;
                    else
                    {
                        --nest_level;
                        
                        current   += 2;
                        is_comment = (nest_level != 0);
                    }
                }
                
                else
                {
                    if (is_comment) ++current;
                    else break;
                }
            }
            
            base_pos.column = (u32)(current - lexer->start) - base_pos.offset_to_line;
        }
        /// ---
        
        if (*current == 0) token.kind = Token_EOF;
        else
        {
            char c = *current;
            ++current;
            
            if      (c == '^') token.kind = Token_Hat;
            else if (c == '$') token.kind = Token_Cash;
            else if (c == ':') token.kind = Token_Colon;
            else if (c == ',') token.kind = Token_Comma;
            else if (c == ';') token.kind = Token_Semicolon;
            
            else if (c == '*')
            {
                if (current[0] == '=') token.kind = Token_MulEQ,    current += 1;
                else                   token.kind = Token_Asterisk, current += 0;
            }
            
            else if (c == '/')
            {
                if (current[0] == '=') token.kind = Token_DivEQ, current += 1;
                else                   token.kind = Token_Slash, current += 0;
            }
            
            else if (c == '%')
            {
                if (current[0] == '=') token.kind = Token_ModEQ, current += 1;
                else                   token.kind = Token_Mod,   current += 0;
            }
            
            else if (c == '=')
            {
                if      (current[0] == '=') token.kind = Token_IsEqual,     current += 1;
                else if (current[0] == '>') token.kind = Token_ThickRArrow, current += 1;
                else                        token.kind = Token_Equals,      current += 0;
            }
            
            else if (c == '+')
            {
                if      (current[0] == '=') token.kind = Token_PlusEQ, current += 1;
                else if (current[0] == '+') token.kind = Token_Inc,    current += 1;
                else                        token.kind = Token_Plus,   current += 0;
            }
            
            else if (c == '-')
            {
                if      (current[0] == '>') token.kind = Token_RArrow,  current += 1;
                else if (current[0] == '=') token.kind = Token_MinusEQ, current += 1;
                else if (current[0] == '-') token.kind = Token_Dec,     current += 1;
                else                        token.kind = Token_Minus,   current += 0;
            }
            
            else if (c == '&')
            {
                if (current[0] == '&')
                {
                    if (current[1] == '=') token.kind = Token_AndEQ, current += 2;
                    else                   token.kind = Token_And,   current += 1;
                }
                
                else
                {
                    if (current[0] == '=') token.kind = Token_BitAndEQ, current += 1;
                    else                   token.kind = Token_BitAnd,   current += 0;
                }
            }
            
            else if (c == '|')
            {
                if (current[0] == '|')
                {
                    if (current[1] == '=') token.kind = Token_OrEQ, current += 2;
                    else                   token.kind = Token_Or,   current += 1;
                }
                
                else
                {
                    if (current[0] == '=') token.kind = Token_BitOrEQ, current += 1;
                    else                   token.kind = Token_BitOr,   current += 0;
                }
                
            }
            
            else if (c == '<')
            {
                if (current[0] == '<')
                {
                    if (current[1] == '=') token.kind = Token_BitShiftLEQ, current += 2;
                    else                   token.kind = Token_BitShiftL,   current += 1;
                }
                
                else
                {
                    if (current[0] == '=') token.kind == Token_LessEQ, current += 1;
                    if (current[0] == '-') token.kind == Token_LArrow, current += 1;
                    else                   token.kind == Token_Less,   current += 0;
                }
            }
            
            else if (c == '>')
            {
                if (current[0] == '>')
                {
                    if (current[1] == '=') token.kind = Token_BitShiftREQ, current += 2;
                    else                   token.kind = Token_BitShiftR,   current += 1;
                }
                
                else
                {
                    if (current[0] == '=') token.kind == Token_GreaterEQ, current += 1;
                    else                   token.kind == Token_Greater,   current += 0;
                }
            }
            
            else if (c == '"' || c == '\'')
            {
                token.kind = (c == '"' ? Token_String : Token_Char);
                
                // NOTE(soimn): 'c' is one ahead of 'current'
                char terminator = c;
                String string   = { .data = current };
                
                while (*current != 0 && *current != terminator)
                {
                    if (current[0] == '\\' && (current[1] == terminator || current[1] == '\\')) current += 2;
                    else                                                                        current += 1;
                }
                
                if (*current == 0)
                {
                    //// ERROR
                    
                    Text_Pos start = base_pos;
                    Text_Pos end   = base_pos;
                    end.column += (uint)(current - lexer->start) - (base_pos.offset_to_line + base_pos.column);
                    
                    LexerReportError(TextIntervalFromEndpoints(start, end),
                                     "Missing terminating '%s' character in %s literal",
                                     (token.kind == Token_Char ? "\\'" : "\""),
                                     (token.kind == Token_Char ? "character" : "string"));
                    
                    token.kind = Token_Invalid;
                }
                
                else
                {
                    string.size = current - token.string.data;
                    ++current;
                    
                    String resolved_string = {
                        .data = (u8*)Arena_Allocate(lexer->arena, string.size, ALIGNOF(char)),
                        .size = 0
                    };
                    
                    bool encountered_errors = false;
                    
                    for (umm i = 0; !encountered_errors && i < string.size; ++i)
                    {
                        if (string.data[i] == '\\')
                        {
                            Text_Pos escape_sequence_start = base_pos;
                            escape_sequence_start.column  += i + 1;
                            
                            ++i;
                            
                            // NOTE(soimn): Since a backslash would escape the terminator, there should never be a case
                            //              where a backslash is at the end of the string (without being escaped by a
                            //              preceeding backslash)
                            ASSERT(string.size > i);
                            
                            c = string.data[i];
                            ++i;
                            
                            if      (c == 'a')  resolved_string.data[resolved_string.size++] = '\a';
                            else if (c == 'b')  resolved_string.data[resolved_string.size++] = '\b';
                            else if (c == 'e')  resolved_string.data[resolved_string.size++] = '\e';
                            else if (c == 'f')  resolved_string.data[resolved_string.size++] = '\f';
                            else if (c == 'n')  resolved_string.data[resolved_string.size++] = '\n';
                            else if (c == 'r')  resolved_string.data[resolved_string.size++] = '\r';
                            else if (c == 't')  resolved_string.data[resolved_string.size++] = '\t';
                            else if (c == 'v')  resolved_string.data[resolved_string.size++] = '\v';
                            else if (c == '\\') resolved_string.data[resolved_string.size++] = '\\';
                            else if (c == '\'') resolved_string.data[resolved_string.size++] = '\'';
                            else if (c == '"')  resolved_string.data[resolved_string.size++] = '\"';
                            
                            else if (c == 'u' ||
                                     c == 'x')
                            {
                                bool is_byte = (c == 'x');
                                
                                uint digit_count = 0;
                                u32 codepoint    = 0;
                                
                                while (i < string.size && digit_count < (is_byte ? 2 : 6))
                                {
                                    u8 digit = 0;
                                    
                                    if (IsDigit(string.data[i])) digit = string.data[i] - '0';
                                    else if (ToUpper(string.data[i]) >= 'A' && ToUpper(string.data[i]) <= 'F')
                                    {
                                        digit = (string.data[i] - 'A') + 10;
                                    }
                                    
                                    else break;
                                    
                                    codepoint = codepoint * 16 + digit;
                                    
                                    ++i;
                                    ++digit_count;
                                }
                                
                                if      (digit_count == 2) resolved_string.data[resolved_string.size++] = (u8)codepoint;
                                else if (digit_count == 6)
                                {
                                    if (codepoint <= 0x007F)
                                    {
                                        resolved_string.data[resolved_string.size] = (u8)codepoint;
                                        resolved_string.size                      += 1;
                                    }
                                    
                                    else if (codepoint <= 0x07FF)
                                    {
                                        u8* bytes = resolved_string.data;
                                        resolved_string.size += 2;
                                        
                                        bytes[0] = (u8)(0xC0 | (codepoint & 0x07C0) >> 6);
                                        bytes[1] = (u8)(0x80 | (codepoint & 0x003F) >> 0);
                                    }
                                    
                                    else if (codepoint <= 0xFFFF)
                                    {
                                        u8* bytes = resolved_string.data;
                                        resolved_string.size += 3;
                                        
                                        bytes[0] = (u8)(0xE0 | (codepoint & 0xF000) >> 12);
                                        bytes[1] = (u8)(0x80 | (codepoint & 0x0FC0) >> 6);
                                        bytes[2] = (u8)(0x80 | (codepoint & 0x003F) >> 0);
                                    }
                                    
                                    else if (codepoint <= 0x10FFFF)
                                    {
                                        u8* bytes = resolved_string.data;
                                        resolved_string.size += 4;
                                        
                                        bytes[0] = (u8)(0xF0 | (codepoint & 0x1C0000) >> 18);
                                        bytes[1] = (u8)(0x80 | (codepoint & 0x03F000) >> 12);
                                        bytes[2] = (u8)(0x80 | (codepoint & 0x000FC0) >> 6);
                                        bytes[3] = (u8)(0x80 | (codepoint & 0x00003F) >> 0);
                                    }
                                    
                                    else
                                    {
                                        //// ERROR
                                        
                                        LexerReportError(TextInterval(escape_sequence_start, digit_count + (sizeof("\\u") - 1)),
                                                         "Codepoint out of range. The maximum valid codepoint is U+10FFFF");
                                        
                                        encountered_errors = true;
                                    }
                                }
                                
                                else
                                {
                                    //// ERROR
                                    
                                    LexerReportError(TextInterval(escape_sequence_start, digit_count + (sizeof("\\u") - 1)),
                                                     "Missing remainder of digits in codepoint escape sequence. Expected %u, got %u",
                                                     (is_byte ? 2 : 6), digit_count);
                                    
                                    encountered_errors = true;
                                }
                            }
                        }
                        
                        else
                        {
                            resolved_string.data[resolved_string.size] = string.data[i];
                            resolved_string.size                      += 1;
                        }
                    }
                    
                    if (encountered_errors) token.kind = Token_Invalid;
                    else
                    {
                        if (token.kind == Token_String) token.string = resolved_string;
                        else
                        {
                            if (resolved_string.size == 0)
                            {
                                //// ERROR
                                
                                LexerReportError(TextInterval(escape_sequence_start, digit_count + (sizeof("\\u") - 1)),
                                                 "Encoutered empty character literal. Character literals must contain a character");
                                
                                encountered_errors = true;
                            }
                            
                            else if (resolved_string.size > 4                                             ||
                                     (resolved_string.data[0] & 0xE0) == 0xE0 && resolved_string.size > 3 ||
                                     (resolved_string.data[0] & 0xC0) == 0xC0 && resolved_string.size > 2 ||
                                     (resolved_string.data[0] & 0x80) == 0    && resolved_string.size > 1)
                            {
                                //// ERROR
                                
                                LexerReportError(TextInterval(escape_sequence_start, digit_count + (sizeof("\\u") - 1)),
                                                 "Invalid number of characters in character literal. Character literals may only contain one character");
                                
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                Copy(resolved_string.data + (4 - resolved_string.size), &token.character, resolved_string.size);
                                Zero(&token.character, 4 - resolved_string.size);
                            }
                        }
                    }
                }
            }
            
            else if (IsAlpha(c) || c == '_')
            {
                token.kind = Token_Identifier;
                
                token.string.data = current - 1;
                token.string.size = 0;
                
                bool is_only_underscores = (c == '_');
                while (IsAlpha(*current) || IsDigit(*current) || *current == '_')
                {
                    is_only_underscores = (is_only_underscores && *current == '_');
                    
                    ++current;
                }
                
                token.string.size = current - token.string.data;
                
                if (is_only_underscores)
                {
                    if (token.string.size == 1) token.kind = Token_Underscore;
                    else
                    {
                        //// ERROR
                        
                        LexerReportError(TextInterval(base_pos, token.string.size),
                                         "Invalid identifier. Expected an alphanumeric character after a sequence of underscores");
                        
                        token.kind = Token_Invalid;
                    }
                }
                
                else
                {
                    token.keyword = 0;
                    
                    for (uint i = 0; i < KEYWORD_KIND_COUNT; ++i)
                    {
                        if (StringCompare(token.string, KeywordStrings[i]))
                        {
                            token.keyword = (u8)i;
                            break;
                        }
                    }
                }
            }
            
            else if (IsDigit(c) || c == '.')
            {
                if (c == '.' && !IsDigit(*current)) token.kind = Token_Period;
                else
                {
                    token.kind = Token_Number;
                    
                    u8* start_of_number = current;
                    
                    bool is_float  = false;
                    bool is_hex    = false;
                    bool is_binary = false;
                    
                    f64 floating = 0;
                    u64 integer  = 0;
                    
                    if (c == '0')
                    {
                        if      (*current == 'x') is_hex = true;
                        else if (*current == 'h') is_hex = true, is_float = true;
                        else if (*current == 'b') is_binary = true;
                        
                        if (is_hex || is_float || is_binary) current += 2;
                    }
                    
                    else if (c == '.')
                    {
                        is_float = true;
                    }
                    
                    else
                    {
                        integer  = c - '0';
                        floating = c - '0';
                    }
                    
                    bool encountered_errors = false;
                    f64 digit_mod           = 1;
                    u8 base                 = (is_hex ? 16 : (is_binary ? 2 : 10));
                    uint digit_count        = (IsDigit(c) && !(is_hex || is_binary) ? 1 : 0);
                    
                    while (!encountered_errors)
                    {
                        if (is_float) digit_mod /= 10;
                        
                        u8 digit = 0;
                        
                        if (IsDigit(*current))
                        {
                            digit = *current - '0';
                            
                            if (is_binary && digit >= 2) break;
                        }
                        
                        else if (is_hex && ToUpper(*current) >= 'A' && ToUpper(*current) <= 'F')
                        {
                            digit = (ToUpper(*current) - 'A') + 10;
                        }
                        
                        else if (*current == '.' && !(is_float || is_hex || is_binary))
                        {
                            ++current;
                            is_float = true;
                            continue;
                        }
                        
                        else if (*current == '_')
                        {
                            if (is_float  && digit_mod == 0.1   ||
                                is_hex    && current[-1] == 'h' ||
                                is_hex    && current[-1] == 'x' ||
                                is_binary && current[-1] == 'b')
                            {
                                //// ERROR
                                
                                LexerReportError(TextInterval(base_pos, (current + 1) - start_of_number),
                                                 "Invalid use of digit separator with no preceeding digit");
                                
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                ++current;
                                continue;
                            }
                        }
                        
                        else break;
                        
                        ++current;
                        
                        // TODO(soimn): Handle overflow and precision loss
                        integer  = integer * base + digit;
                        floating = floating * (is_float ? 1 : base) + digit * digit_mod;
                        
                        ++digit_count;
                    }
                    
                    if (!encountered_errors && current[-1] == '_')
                    {
                        //// ERROR
                        
                        LexerReportError(TextInterval(base_pos, current - start_of_number),
                                         "Invalid use of digit separator with no succeeding digit");
                        
                        encountered_errors = true;
                    }
                    
                    if (!encountered_errors)
                    {
                        if (is_float)
                        {
                            i8 exponent_sign = 1;
                            uint exponent    = 0;
                            if (ToUpper(*current) == 'E')
                            {
                                ++current;
                                
                                if (*current == '+' || *current == '-')
                                {
                                    exponent_sign = (*current == '-' ? -1 : 1);
                                    ++current;
                                }
                                
                                while (!encountered_errors)
                                {
                                    if (IsDigit(*current))
                                    {
                                        exponent = exponent * 10 + (*current - '0');
                                        ++current;
                                    }
                                    
                                    else if (*current == '_')
                                    {
                                        if (current[-1] == '-' || current[-1] == '+' ||
                                            ToUpper(current[-1]) == 'E')
                                        {
                                            //// ERROR
                                            
                                            LexerReportError(TextInterval(base_pos, (current + 1) - start_of_number),
                                                             "Invalid use of digit separator with no preceeding digit");
                                        }
                                        
                                        else ++current;
                                    }
                                    
                                    else break;
                                }
                            }
                            
                            if (!encountered_errors)
                            {
                                // TODO(soimn): Find a better way to deal with the exponent
                                // TODO(soimn): Error on too large or small exponent
                                if (exponent_sign == 1) for (uint i = 0; i < exponent; ++i) floating *= 10;
                                else                    for (uint i = 0; i < exponent; ++i) floating /= 10;
                                
                                if (is_hex)
                                {
                                    if (digit_count == 8)
                                    {
                                        // TODO(soimn): Convert from integer to binary32 without relying on type punning
                                        token.number.is_float     = true;
                                        token.number.floating     = (f32)*(f32*)&integer;
                                        token.number.min_fit_bits = 32;
                                    }
                                    
                                    else if (digit_count == 16)
                                    {
                                        // TODO(soimn): Convert from integer to binary32 without relying on type punning
                                        token.number.is_float     = true;
                                        token.number.floating     = (f64)*(f64*)&integer;
                                        token.number.min_fit_bits = 64;
                                    }
                                    
                                    else
                                    {
                                        //// ERROR
                                        
                                        LexerReportError(TextInterval(base_pos, current - start_of_number),
                                                         "Invalid number of digits in hexadecimal floating point literal. Expected 8 or 16, got %u",
                                                         digit_count);
                                        
                                        encountered_errors = true;
                                    }
                                }
                                
                                else
                                {
                                    token.number.is_float = true;
                                    token.number.floating = floating;
                                    
                                    // TODO(soimn): Find a better way of judging min fit bits
                                    if (floating > FLT_MAX || floating < FLT_MIN) token.number.min_fit_bits = 64;
                                    else                                          token.number.min_fit_bits = 32;
                                }
                            }
                        }
                        
                        else
                        {
                            token.number.is_float = false;
                            token.number.integer  = integer;
                            
                            if      (integer <= U8_MAX)  token.number.min_fit_bits = 8;
                            else if (integer <= U16_MAX) token.number.min_fit_bits = 16;
                            else if (integer <= U32_MAX) token.number.min_fit_bits = 32;
                            else                         token.number.min_fit_bits = 64;
                        }
                    }
                    
                    
                    // NOTE(soimn): Handle trailing characters
                    if (!encountered_errors)
                    {
                        if (*current == '.' || IsAlpha(*current) || IsDigit(*curent))
                        {
                            //// ERROR
                            
                            // TODO(soimn): Improve this error message
                            LexerReportError(TextInterval(base_pos, (current + 1) - start_of_number),
                                             "Trailing characters in numeric literal");
                            
                            encountered_errors = true;
                        }
                        
                        else if (current[-1] == '_')
                        {
                            //// ERROR
                            
                            LexerReportError(TextInterval(base_pos, current - start_of_number),
                                             "Invalid use of digit separator with no succeeding digit");
                            
                            encountered_errors = true;
                        }
                    }
                    
                    
                    if (encountered_errors) token.kind = Token_Invalid;
                }
            }
            
            else token.kind = Token_Invalid;
        }
        
        token.text.position = base_pos;
        token.text.size     = (uint)(current - lexer->start) - (base_pos.offset_to_line + base_pos.column);
        
        lexer->token       = token;
        lexer->token_valid = true;
    }
    
    return token;
}

inline void
SkipPastToken(Lexer* lexer, Token token)
{
    lexer->token       = token;
    lexer->token_valid = false;
}

inline Token
PeekNextToken(Lexer* lexer, Token token)
{
    Lexer peek_lexer = *lexer;
    
    SkipPastToken(&peek_lexer, token);
    return GetToken(&peek_lexer);
}

void
DEBUG_TestLexer(const char* lex_string)
{
    Lexer lexer = {
        .start = (u8*)lex_string,
    };
    
    for (;;)
    {
        Token token = GetToken(&lexer);
        
        const char* name = 0;
        
        switch (token.kind)
        {
            case Token_Invalid:      name = "Token_Invalid";      break;
            case Token_Plus:         name = "Token_Plus";         break;
            case Token_Minus:        name = "Token_Minus";        break;
            case Token_Asterisk:     name = "Token_Asterisk";     break;
            case Token_Slash:        name = "Token_Slash";        break;
            case Token_Mod:          name = "Token_Mod";          break;
            case Token_BitAnd:       name = "Token_BitAnd";       break;
            case Token_BitOr:        name = "Token_BitOr";        break;
            case Token_BitNot:       name = "Token_BitNot";       break;
            case Token_And:          name = "Token_And";          break;
            case Token_Or:           name = "Token_Or";           break;
            case Token_Not:          name = "Token_Not";          break;
            case Token_Less:         name = "Token_Less";         break;
            case Token_Greater:      name = "Token_Greater";      break;
            case Token_Equals:       name = "Token_Equals";       break;
            case Token_PlusEQ:       name = "Token_PlusEQ";       break;
            case Token_MinusEQ:      name = "Token_MinusEQ";      break;
            case Token_MulEQ:        name = "Token_MulEQ";        break;
            case Token_DivEQ:        name = "Token_DivEQ";        break;
            case Token_ModEQ:        name = "Token_ModEQ";        break;
            case Token_BitAndEQ:     name = "Token_BitAndEQ";     break;
            case Token_BitOrEQ:      name = "Token_BitOrEQ";      break;
            case Token_AndEQ:        name = "Token_AndEQ";        break;
            case Token_OrEQ:         name = "Token_OrEQ";         break;
            case Token_LessEQ:       name = "Token_LessEQ";       break;
            case Token_GreaterEQ:    name = "Token_GreaterEQ";    break;
            case Token_IsEqual:      name = "Token_IsEqual";      break;
            case Token_Hat:          name = "Token_Hat";          break;
            case Token_Cash:         name = "Token_Cash";         break;
            case Token_At:           name = "Token_At";           break;
            case Token_Hash:         name = "Token_Hash";         break;
            case Token_Period:       name = "Token_Period";       break;
            case Token_Colon:        name = "Token_Colon";        break;
            case Token_Comma:        name = "Token_Comma";        break;
            case Token_Semicolon:    name = "Token_Semicolon";    break;
            case Token_Underscore:   name = "Token_Underscore";   break;
            case Token_OpenParen:    name = "Token_OpenParen";    break;
            case Token_CloseParen:   name = "Token_CloseParen";   break;
            case Token_OpenBrace:    name = "Token_OpenBrace";    break;
            case Token_CloseBrace:   name = "Token_CloseBrace";   break;
            case Token_OpenBracket:  name = "Token_OpenBracket";  break;
            case Token_CloseBracket: name = "Token_CloseBracket"; break;
            case Token_Char:         name = "Token_Char";         break;
            case Token_Number:       name = "Token_Number";       break;
            case Token_String:       name = "Token_String";       break;
            case Token_Identifier:   name = "Token_Identifier";   break;
            case Token_EOF:          name = "Token_EOF";          break;
            
        }
        
        PrintCString(name);
        
        PrintCString("\n");
        
        if (token.kind == Token_Invalid || token.kind == Token_EOF) break;
        else SkipPastToken(&lexer, token);
    }
}