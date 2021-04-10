enum TOKEN_KIND
{
    Token_Invalid = 0,
    
    Token_Add,
    Token_Sub,
    Token_Mul,
    Token_Div,
    Token_Percent,
    Token_And,
    Token_Or,
    Token_Not,
    Token_BitAnd,
    Token_BitOr,
    Token_BitNot,
    Token_BitShiftLeft,
    Token_BitShiftRight,
    Token_Equal,
    Token_Less,
    Token_Greater,
    Token_Hat,
    Token_Arrow,
    Token_ThickArrow,
    Token_Hash,
    Token_At,
    Token_Cash,
    Token_Comma,
    Token_Semicolon,
    Token_Period,
    Token_Elipsis,
    Token_ElipsisLess,
    Token_Colon,
    Token_ColonColon,
    Token_Underscore,
    Token_QuestionMark,
    Token_Backtick,
    
    Token_AddEqual,
    Token_SubEqual,
    Token_MulEqual,
    Token_DivEqual,
    Token_PercentEqual,
    Token_AndEqual,
    Token_OrEqual,
    Token_NotEqual,
    Token_BitAndEqual,
    Token_BitOrEqual,
    Token_BitNotEqual,
    Token_BitShiftLeftEqual,
    Token_BitShiftRightEqual,
    Token_EqualEqual,
    Token_LessEqual,
    Token_GreaterEqual,
    Token_HatEqual,
    Token_ColonEqual,
    
    Token_OpenParen,
    Token_CloseParen,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenBrace,
    Token_CloseBrace,
    
    Token_Identifier,
    Token_String,
    Token_Character,
    Token_Number,
    
    Token_EndOfStream,
};

// NOTE(soimn): Fuck MSVC for not being a capable compiler and requiring this stupid hack
#define LIST_KEYWORDS()              \
X(Keyword_If,       "if"),       \
X(Keyword_Else,     "else"),     \
X(Keyword_When,     "when"),     \
X(Keyword_For,      "for"),      \
X(Keyword_While,    "while"),    \
X(Keyword_Continue, "continue"), \
X(Keyword_Break,    "break"),    \
X(Keyword_Return,   "return"),   \
X(Keyword_Defer,    "defer"),    \
X(Keyword_Using,    "using"),    \
X(Keyword_Proc,     "proc"),     \
X(Keyword_Macro,    "macro"),    \
X(Keyword_Where,    "where"),    \
X(Keyword_Struct,   "struct"),   \
X(Keyword_Union,    "union"),    \
X(Keyword_Enum,     "enum"),     \
X(Keyword_True,     "true"),     \
X(Keyword_False,    "false"),    \
X(Keyword_Import,   "import"),   \
X(Keyword_In,       "in"),       \
X(Keyword_NotIn,    "not_in"),   \
X(Keyword_Do,       "do"),

enum KEYWORD_KIND
{
    Keyword_Invalid = 0,
    
#define X(A, B) A
    LIST_KEYWORDS()
#undef X
    
    KEYWORD_KIND_COUNT
};

typedef struct Token
{
    Enum8(TOKEN_KIND) kind;
    bool ends_with_space;
    u32 line;
    u32 column;
    u32 size;
    
    union
    {
        struct
        {
            String string;
            Enum8(KEYWORD_KIND) keyword;
        };
        
        u32 character;
        
        Number number;
    };
} Token;

void
LexerError(File_ID file, u32 line, u32 column, u32 text_length, Enum32(ERROR_CODE) code, const char* message, ...)
{
    NOT_IMPLEMENTED;
}

Bucket_Array(Token)
LexFile(String file_contents, Memory_Arena* token_arena, Memory_Arena* string_arena)
{
    ASSERT(file_contents.data[file_contents.size - 1] == 0);
    
    Bucket_Array tokens = BUCKET_ARRAY_INIT(token_arena, Token, 256);
    
    u8* current = file_contents.data;
    u32 line    = 0;
    u32 column  = 0;
    
    bool encountered_errors = false;
    for (;;)
    {
        bool is_comment        = false;
        u32 comment_nest_level = 0;
        for (;; current += 1)
        {
            if (*current == '\n')
            {
                line  += 1;
                column = 0;
                
                if (is_comment && comment_nest_level == 0) is_comment = false;
            }
            
            else if (IsWhitespace(*current) || *current == '\r')
            {
                column += 1;
            }
            
            else if (current[0] == '/' && current[1] == '/')
            {
                current += 1;
                
                is_comment = true;
            }
            
            else if (current[0] == '/' && current[1] == '*')
            {
                if (!is_comment || comment_nest_level != 0)
                {
                    current += 1;
                    
                    is_comment          = true;
                    comment_nest_level += 1;
                }
            }
            
            else if (current[0] == '*' && current[1] == '/')
            {
                if (comment_nest_level != 0)
                {
                    current += 1;
                    
                    comment_nest_level -= 1;
                    if (comment_nest_level == 0) is_comment = false;
                }
            }
            
            else if (!is_comment) break;
        }
        
        Token* token = BucketArray_AppendElement(&tokens);
        *token = (Token){
            .kind   = Token_Invalid,
            .line   = line,
            .column = column,
            .size   = 0,
        };
        
        u8* start = current;
        char c    = *current;
        current  += (c != 0);
        
        switch (c)
        {
            case '\0': token->kind = Token_EndOfStream; break;
            
            case '#': token->kind = Token_Hash;         break;
            case '@': token->kind = Token_At;           break;
            case '$': token->kind = Token_Cash;         break;
            case ',': token->kind = Token_Comma;        break;
            case ';': token->kind = Token_Semicolon;    break;
            case '`': token->kind = Token_Backtick;     break;
            case '?': token->kind = Token_QuestionMark; break;
            case '(': token->kind = Token_OpenParen;    break;
            case ')': token->kind = Token_CloseParen;   break;
            case '[': token->kind = Token_OpenBracket;  break;
            case ']': token->kind = Token_CloseBracket; break;
            case '{': token->kind = Token_OpenBrace;    break;
            case '}': token->kind = Token_CloseBrace;   break;
            
#define SINGLE_DOUBLE_EQUAL_OR_DOUBLE_EQUAL(c, single_kind, double_kind, single_equal_kind, double_equal_kind) \
case c:                                                                                                    \
{                                                                                                          \
if (*current == '=')                                                                                   \
{                                                                                                      \
current   += 1;                                                                                    \
token->kind = single_equal_kind;                                                                   \
}                                                                                                      \
else if (*current == c)                                                                                \
{                                                                                                      \
current += 1;                                                                                      \
if (*current == '=')                                                                               \
{                                                                                                  \
current   += 1;                                                                                \
token->kind = double_equal_kind;                                                               \
}                                                                                                  \
else token->kind = double_kind;                                                                    \
}                                                                                                      \
else token->kind = single_kind;                                                                        \
} break                                                                                                    \
            
            SINGLE_DOUBLE_EQUAL_OR_DOUBLE_EQUAL('&', Token_BitAnd, Token_And, Token_BitAndEqual, Token_AndEqual);
            SINGLE_DOUBLE_EQUAL_OR_DOUBLE_EQUAL('|', Token_BitOr,  Token_Or,  Token_BitOrEqual,  Token_OrEqual);
            
            SINGLE_DOUBLE_EQUAL_OR_DOUBLE_EQUAL('<', Token_Less,    Token_BitShiftLeft,  Token_LessEqual,
                                                Token_BitShiftLeftEqual);
            SINGLE_DOUBLE_EQUAL_OR_DOUBLE_EQUAL('>', Token_Greater, Token_BitShiftRight, Token_GreaterEqual,
                                                Token_BitShiftRightEqual);
            
#undef SINGLE_DOUBLE_EQUAL_OR_DOUBLE_EQUAL
            
#define SINGLE_OR_EQUAL(c, single_kind, equal_kind) \
case c:                                         \
{                                               \
if (*current == '=')                        \
{                                           \
current    += 1;                        \
token->kind = equal_kind;               \
}                                           \
else token->kind = single_kind;             \
}break
            
            SINGLE_OR_EQUAL('+', Token_Add,     Token_AddEqual);
            SINGLE_OR_EQUAL('*', Token_Mul,     Token_MulEqual);
            SINGLE_OR_EQUAL('/', Token_Div,     Token_DivEqual);
            SINGLE_OR_EQUAL('%', Token_Percent, Token_PercentEqual);
            SINGLE_OR_EQUAL('!', Token_Not,     Token_NotEqual);
            SINGLE_OR_EQUAL('~', Token_BitNot,  Token_BitNotEqual);
            SINGLE_OR_EQUAL('^', Token_Hat,     Token_HatEqual);
#undef SINGLE_OR_EQUAL
            
            case ':':
            {
                if (*current == ':')
                {
                    current    += 1;
                    token->kind = Token_ColonColon;
                }
                
                else if (*current == '=')
                {
                    current    += 1;
                    token->kind = Token_ColonEqual;
                }
                
                else token->kind = Token_Colon;
            } break;
            
            case '-':
            {
                if (*current == '=')
                {
                    current    += 1;
                    token->kind = Token_SubEqual;
                }
                
                else if (*current == '>')
                {
                    current    += 1;
                    token->kind = Token_Arrow;
                }
                
                else token->kind = Token_Sub;
            } break;
            
            case '=':
            {
                if (*current == '=')
                {
                    current    += 1;
                    token->kind = Token_EqualEqual;
                }
                
                else if (*current == '>')
                {
                    current    += 1;
                    token->kind = Token_ThickArrow;
                }
                
                else token->kind = Token_Equal;
            } break;
            
            default:
            {
                if (IsAlpha(c) || c == '_')
                {
                    if (!IsAlpha(*current) && !IsDigit(*current) && *current != '_')
                    {
                        token->kind = Token_Underscore;
                    }
                    
                    else
                    {
                        token->kind = Token_Identifier;
                        
                        while (IsAlpha(*current) || IsDigit(*current) || *current == '_')
                        {
                            current += 1;
                        }
                        
                        token->string.data = start;
                        token->string.size = (umm)(current - start);
                        
                        String KeywordStrings[KEYWORD_KIND_COUNT] = {
#define X(A, B) [A] = CONST_STRING(B)
                            LIST_KEYWORDS()
#undef X
                        };
                        
                        ASSERT(KEYWORD_KIND_COUNT < U8_MAX);
                        for (u8 i = 0; i < KEYWORD_KIND_COUNT; ++i)
                        {
                            if (StringCompare(token->string, KeywordStrings[i]))
                            {
                                token->keyword = i;
                            }
                        }
                    }
                }
                
                else if (IsDigit(c) || c == '.')
                {
                    if (c == '.' && !IsDigit(*current))
                    {
                        if (*current == '.')
                        {
                            current += 1;
                            
                            if (*current == '<')
                            {
                                current += 1;
                                token->kind = Token_ElipsisLess;
                            }
                            
                            
                            else token->kind = Token_Elipsis;
                        }
                        
                        else token->kind = Token_Period;
                    }
                    
                    else
                    {
                        token->kind = Token_Number;
                        
                        bool is_hex    = false;
                        bool is_binary = false;
                        bool is_float  = (c == '.');
                        
                        if (c == '0')
                        {
                            if      (*current == 'x') is_hex = true;
                            else if (*current == 'h') is_hex = true, is_float = true;
                            else if (*current == 'b') is_binary = true;
                            
                            if (is_hex || is_float || is_binary) current += 1;
                        }
                        
                        u8 initial   = (IsDigit(c) && !(is_hex || is_float || is_binary) ? c - '0' : 0);
                        u64 integer  = initial;
                        f64 floating = initial;
                        
                        umm digit_count        = 0;
                        f64 floating_mod       = 1;
                        bool integer_overflow  = false;
                        bool truncated_to_zero = false;
                        for (;; current += 1)
                        {
                            u8 digit = 0;
                            
                            if (IsDigit(*current) && (!is_binary || *current <= '1'))                digit = *current - '0';
                            else if (is_hex && ToUpper(*current) >= 'A' && ToUpper(*current) <= 'F') digit = (ToUpper(*current) - 'A') + 10;
                            
                            else if (!is_float && *current == '.')
                            {
                                is_float = true;
                            }
                            
                            else if (*current != '_') break;
                            
                            if (*current != '.' && *current != '_')
                            {
                                if (is_float) floating_mod /= 10;
                                
                                u64 new_integer = integer * 10 + digit;
                                integer_overflow = (integer_overflow || new_integer < integer);
                                integer          = new_integer;
                                
                                f64 new_floating = floating * (is_float ? 1 : 10) + digit * floating_mod;
                                truncated_to_zero = (truncated_to_zero || floating != 0 && new_floating == 0);
                                floating          = new_floating;
                                
                                digit_count += 1;
                            }
                        }
                        
                        if (is_float)
                        {
                            if (is_hex)
                            {
                                ASSERT(!integer_overflow);
                                
                                if (digit_count == 8)
                                {
                                    u32 i = (u32)integer;
                                    f32 f = 0;
                                    Copy(&i, &f, sizeof(u32));
                                    
                                    token->number.is_float    = true;
                                    token->number.is_negative = false;
                                    token->number.floating    = f;
                                }
                                
                                else if (digit_count == 16)
                                {
                                    u64 i = integer;
                                    f64 f = 0;
                                    Copy(&i, &f, sizeof(u64));
                                    
                                    token->number.is_float    = true;
                                    token->number.is_negative = false;
                                    token->number.floating    = f;
                                }
                                
                                else
                                {
                                    //// ERROR: Invalid digit count for hexadecimal floating point literal
                                    encountered_errors = true;
                                }
                            }
                            
                            else
                            {
                                if (truncated_to_zero)
                                {
                                    //// ERROR: Floating point literal truncated to zero
                                    encountered_errors = true;
                                }
                                
                                else
                                {
                                    token->number.is_float    = true;
                                    token->number.is_negative = false;
                                    token->number.floating    = floating;
                                }
                            }
                        }
                        
                        else
                        {
                            if (integer_overflow)
                            {
                                //// ERROR: Integer literal too large
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                token->number.is_float    = false;
                                token->number.is_negative = false;
                                token->number.integer     = integer;
                            }
                        }
                    }
                }
                
                else if (c == '"' || c == '\'')
                {
                    if (c == '"') token->kind = Token_String;
                    else          token->kind = Token_Character;
                    
                    while (*current != 0 && *current != (token->kind == Token_String ? '"' : '\''))
                    {
                        if (current[0] == '\\' && current[1] != 0) current += 2;
                        else                                       current += 1;
                    }
                    
                    if (*current == 0)
                    {
                        //// ERROR: Unterminated string
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        String raw_string = {
                            .data = start + 1,
                            .size = (start - current) - 2
                        };
                        
                        String resolved_string = {0};
                        resolved_string.data   = Arena_Allocate(string_arena, raw_string.size, ALIGNOF(u8));
                        
                        for (umm i = 0; i < raw_string.size; )
                        {
                            if (raw_string.data[i] == '\\')
                            {
                                c  = raw_string.data[i + 1];
                                i += 2;
                                
                                ASSERT(raw_string.size > i);
                                
                                // TODO(soimn): Should ansi escape codes be supported?
                                if      (c == 'a') resolved_string.data[resolved_string.size++] = '\a';
                                else if (c == 'b') resolved_string.data[resolved_string.size++] = '\b';
                                else if (c == 'f') resolved_string.data[resolved_string.size++] = '\f';
                                else if (c == 'n') resolved_string.data[resolved_string.size++] = '\n';
                                else if (c == 'r') resolved_string.data[resolved_string.size++] = '\r';
                                else if (c == 't') resolved_string.data[resolved_string.size++] = '\t';
                                else if (c == 'v') resolved_string.data[resolved_string.size++] = '\v';
                                else if (c == 'u' || c == 'x')
                                {
                                    u32 codepoint = 0;
                                    for (umm j = i; !encountered_errors && i < j + (c == 'u' ? 6 : 2); ++i)
                                    {
                                        if (i < raw_string.size && IsDigit(raw_string.data[i]))
                                        {
                                            codepoint = codepoint * 10 + (raw_string.data[i] - '0');
                                        }
                                        
                                        else if (i < raw_string.size &&
                                                 ToUpper(raw_string.data[i]) >= 'A' &&
                                                 ToUpper(raw_string.data[i]) <= 'F')
                                        {
                                            codepoint = codepoint * 10 + (ToUpper(raw_string.data[i]) - 'A') + 10;
                                        }
                                        
                                        else
                                        {
                                            //// ERROR: Missing digits in codepoint/byte escape sequence
                                            encountered_errors = true;
                                        }
                                    }
                                    
                                    if (!encountered_errors)
                                    {
                                        if (codepoint <= U8_MAX)
                                        {
                                            resolved_string.data[resolved_string.size++] = (u8)codepoint;
                                        }
                                        
                                        else if (codepoint <= 0x7FF)
                                        {
                                            resolved_string.data[resolved_string.size++] = (u8)(0xC0 | (codepoint & 0x7C0) >> 6);
                                            resolved_string.data[resolved_string.size++] = (u8)(0x80 | (codepoint & 0x03F) >> 0);
                                        }
                                        
                                        else if (codepoint <= 0xFFFF)
                                        {
                                            resolved_string.data[resolved_string.size++] = (u8)(0xE0 | (codepoint & 0xF000) >> 12);
                                            resolved_string.data[resolved_string.size++] = (u8)(0x80 | (codepoint & 0x0FC0) >> 6);
                                            resolved_string.data[resolved_string.size++] = (u8)(0x80 | (codepoint & 0x003F) >> 0);
                                        }
                                        
                                        else if (codepoint <= 0x10FFFF)
                                        {
                                            resolved_string.data[resolved_string.size++] = (u8)(0xF0 | (codepoint & 0x1C0000) >> 18);
                                            resolved_string.data[resolved_string.size++] = (u8)(0x80 | (codepoint & 0x03F000) >> 12);
                                            resolved_string.data[resolved_string.size++] = (u8)(0x80 | (codepoint & 0x000FC0) >> 6);
                                            resolved_string.data[resolved_string.size++] = (u8)(0x80 | (codepoint & 0x00003F) >> 0);
                                        }
                                        
                                        else
                                        {
                                            //// ERROR: Codepoint is out of UTF-8 range
                                            encountered_errors = true;
                                        }
                                    }
                                }
                                
                                else
                                {
                                    resolved_string.data[resolved_string.size++] = c;
                                }
                            }
                            
                            else
                            {
                                resolved_string.data[resolved_string.size++] = raw_string.data[i];
                                i += 1;
                            }
                        }
                        
                        if (!encountered_errors)
                        {
                            if (token->kind == Token_String) token->string = resolved_string;
                            else
                            {
                                if (resolved_string.size == 1 && (*resolved_string.data & 0x80) == 0    ||
                                    resolved_string.size == 2 && (*resolved_string.data & 0xE0) == 0xA0 ||
                                    resolved_string.size == 3 && (*resolved_string.data & 0xF0) == 0xE0 ||
                                    resolved_string.size == 4 && (*resolved_string.data & 0xF8) == 0xF0)
                                {
                                    token->character = 0;
                                    Copy(resolved_string.data, (u8*)&token->character + (4 - resolved_string.size), resolved_string.size);
                                }
                                
                                else
                                {
                                    //// ERROR: Illegal character size
                                    encountered_errors = true;
                                }
                            }
                        }
                    }
                }
                
                else encountered_errors = true;
                
            } break;
        }
        
        token->size = (u32)(current - start);
        
        token.ends_with_space = (IsWhitespace(*current) || *current == '\n');
        
        if (encountered_errors) token->kind = Token_Invalid;
        if (token->kind == Token_Invalid || token->kind == Token_EndOfStream) break;
        else                                                                  continue;
    }
    
    return tokens;
}