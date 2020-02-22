enum LEXER_TOKEN_TYPE
{
    Token_Error,
    Token_EndOfStream,
    
    Token_Plus,
    Token_Minus,
    Token_Asterisk,
    Token_ForwardSlash,
    Token_BackwardSlash,
    Token_Percentage,
    Token_And,
    Token_Pipe,
    Token_Greater,
    Token_Less,
    Token_Exclamation,
    Token_Period,
    Token_Comma,
    Token_Colon,
    Token_Semicolon,
    Token_Equals,
    
    Token_PlusEQ,
    Token_MinusEQ,
    Token_AsteriskEQ,
    Token_ForwardSlashEQ,
    Token_PercentageEQ,
    Token_AndEQ,
    Token_PipeEQ,
    Token_GreaterEQ,
    Token_LessEQ,
    Token_ExclamationEQ,
    Token_AndAndEQ,
    Token_PipePipeEQ,
    Token_GreaterGreaterEQ,
    Token_LessLessEQ,
    
    Token_PlusPlus,
    Token_MinusMinus,
    Token_AndAnd,
    Token_PipePipe,
    Token_GreaterGreater,
    Token_LessLess,
    Token_PeriodPeriod,
    Token_ColonColon,
    Token_EqualsEquals,
    
    Token_OpenParen,
    Token_CloseParen,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenBrace,
    Token_CloseBrace,
    
    Token_Arrow,      // ->
    Token_ThickArrow, // =>
    Token_Hash,
    Token_At,
    Token_Quote,
    Token_QuoteQuote,
    
    Token_Identifier,
    Token_String,
    Token_Number,
};

struct Token
{
    Enum32(LEXER_TOKEN_TYPE) type;
    
    String string;
    Number number;
    
    U8* line_start;
    UMM column;
    UMM line;
    UMM token_size;
};

inline Token
GetToken(Parser_State* state)
{
    Token token = {};
    
    Lexer_State lexer = state->lexer_state;
    
    auto Advance = [](Lexer_State* lexer, U32 amount)
    {
        if (*lexer->current != 0)
        {
            lexer->current += amount;
            lexer->column  += amount;
        }
    };
    
    auto IsAlpha = [](char c) -> bool
    {
        return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
    };
    
    auto IsDigit = [](char c) -> bool
    {
        return (c >= '0' && c <= '9');
    };
    
    while (*lexer.current != 0)
    {
        char c = *lexer.current;
        
        if (c == ' ' || c == '\t' || c == '\v' || c == '\r')
        {
            Advance(&lexer, 1);
        }
        
        else if (c == '\n')
        {
            Advance(&lexer, 1);
            lexer.line_start = lexer.current;
            lexer.column     = 0;
            ++lexer.line;
        }
        
        else if (c == '/' && lexer.current[1] == '/')
        {
            Advance(&lexer, 2);
            
            while (*lexer.current != 0 && *lexer.current != '\n') Advance(&lexer, 1);
            Advance(&lexer, 1);
        }
        
        else if (c == '/' && lexer.current[1] == '*')
        {
            Advance(&lexer, 2);
            
            U32 nesting_level = 0;
            while (*lexer.current != 0)
            {
                if (lexer.current[0] == '/' && lexer.current[1] == '*')
                {
                    Advance(&lexer, 2);
                    ++nesting_level;
                }
                
                else if (lexer.current[0] == '*' && lexer.current[1] == '/')
                {
                    Advance(&lexer, 2);
                    
                    if (nesting_level == 0) break;
                    else --nesting_level;
                }
                
                else
                {
                    if (*lexer.current == '\n')
                    {
                        Advance(&lexer, 1);
                        lexer.line_start = lexer.current;
                        lexer.column     = 0;
                        ++lexer.line;
                    }
                    
                    else Advance(&lexer, 1);
                }
            }
        }
        
        else break;
    }
    
    token.line_start = lexer.line_start;
    token.column     = lexer.column;
    token.line       = lexer.line;
    
    char c = *lexer.current;
    Advance(&lexer, 1);
    
    switch(c)
    {
        case 0: token.type = Token_EndOfStream; break;
        
        case '\\': token.type = Token_BackwardSlash; break;
        case ',': token.type  =  Token_Comma;        break;
        case ';': token.type  =  Token_Semicolon;    break;
        case '#': token.type  =  Token_Hash;         break;
        case '@': token.type  =  Token_At;           break;
        case '(': token.type  =  Token_OpenParen;    break;
        case ')': token.type  =  Token_CloseParen;   break;
        case '[': token.type  =  Token_OpenBracket;  break;
        case ']': token.type  =  Token_CloseBracket; break;
        case '{': token.type  =  Token_OpenBrace;    break;
        case '}': token.type  =  Token_CloseBrace;   break;
        
#define SINGLE_DOUBLE_DOUBLE_EQ_OR_EQ(character, single_token, double_token, double_eq_token, eq_token) \
        case character:                                                                                         \
        {                                                                                                       \
            if (*lexer.current == character)                                                                    \
            {                                                                                                   \
                Advance(&lexer, 1);                                                                             \
                if (*lexer.current == '=')                                                                      \
                {                                                                                               \
                    token.type = double_eq_token;                                                               \
                    Advance(&lexer, 1);                                                                         \
                }                                                                                               \
                else token.type = double_token;                                                                 \
            }                                                                                                   \
            else if (*lexer.current == '=')                                                                     \
            {                                                                                                   \
                token.type = eq_token;                                                                          \
                Advance(&lexer, 1);                                                                             \
            }                                                                                                   \
            else token.type = single_token;                                                                     \
        } break;
        
        SINGLE_DOUBLE_DOUBLE_EQ_OR_EQ('&', Token_And,     Token_AndAnd,         Token_AndAndEQ,   Token_AndEQ);
        SINGLE_DOUBLE_DOUBLE_EQ_OR_EQ('|', Token_Pipe,    Token_PipePipe,       Token_PipePipeEQ, Token_PipeEQ);
        SINGLE_DOUBLE_DOUBLE_EQ_OR_EQ('<', Token_Less,    Token_LessLess,       Token_LessLessEQ, Token_LessEQ);
        SINGLE_DOUBLE_DOUBLE_EQ_OR_EQ('>', Token_Greater, Token_GreaterGreater, Token_GreaterGreaterEQ, Token_GreaterEQ);
        
#undef SINGLE_DOUBLE_DOUBLE_EQ_OR_EQ
        
#define SINGLE_DOUBLE_OR_EQ(character, single_token, double_token, eq_token) \
        case character:                                                              \
        {                                                                            \
            if (*lexer.current == character)                                         \
            {                                                                        \
                token.type = double_token;                                           \
                Advance(&lexer, 1);                                                  \
            }                                                                        \
            else if (*lexer.current == '=')                                          \
            {                                                                        \
                token.type = eq_token;                                               \
                Advance(&lexer, 1);                                                  \
            }                                                                        \
            else token.type = single_token;                                          \
        } break;
        
        SINGLE_DOUBLE_OR_EQ('+', Token_Plus, Token_PlusPlus, Token_PlusEQ);
        
#undef SINGLE_DOUBLE_OR_EQ
        
#define SINGLE_OR_EQ(character, single_token, eq_token) \
        case character:                                         \
        {                                                       \
            if (*lexer.current == '=')                          \
            {                                                   \
                token.type = eq_token;                          \
                Advance(&lexer, 1);                             \
            }                                                   \
            else token.type = single_token;                     \
        } break;
        
        SINGLE_OR_EQ('!', Token_Exclamation,  Token_ExclamationEQ);
        SINGLE_OR_EQ('*', Token_Asterisk,     Token_AsteriskEQ);
        SINGLE_OR_EQ('/', Token_ForwardSlash, Token_ForwardSlashEQ);
        SINGLE_OR_EQ('%', Token_Percentage,   Token_PercentageEQ);
        
#undef SINGLE_OR_EQ
        
#define SINGLE_OR_DOUBLE(character, single_token, double_token) \
        case character:                                                 \
        {                                                               \
            if (*lexer.current == character)                            \
            {                                                           \
                token.type = double_token;                              \
                Advance(&lexer, 1);                                     \
            }                                                           \
            else token.type = single_token;                             \
        } break;
        
        SINGLE_OR_DOUBLE('.',  Token_Period, Token_PeriodPeriod);
        SINGLE_OR_DOUBLE(':',  Token_Colon,  Token_ColonColon);
        SINGLE_OR_DOUBLE('\'', Token_Quote,  Token_QuoteQuote);
        
#undef SINGLE_OR_DOUBLE
        
        case '-':
        {
            if (*lexer.current == '=')
            {
                token.type = Token_MinusEQ;
                Advance(&lexer, 1);
            }
            
            else if (*lexer.current == '-')
            {
                token.type = Token_MinusMinus;
                Advance(&lexer, 1);
            }
            
            else if (*lexer.current == '>')
            {
                token.type = Token_Arrow;
                Advance(&lexer, 1);
            }
            
            else token.type = Token_Minus;
        } break;
        
        case '=':
        {
            if (*lexer.current == '=')
            {
                token.type = Token_EqualsEquals;
                Advance(&lexer, 1);
            }
            
            else if (*lexer.current == '>')
            {
                token.type = Token_ThickArrow;
                Advance(&lexer, 1);
            }
            
            else token.type = Token_Equals;
        } break;
        
        default:
        {
            if (IsAlpha(c) || c == '_')
            {
                token.type = Token_Identifier;
                
                token.string.data = lexer.current - 1;
                
                while (IsAlpha(*lexer.current) || IsDigit(*lexer.current) || *lexer.current == '_')
                {
                    Advance(&lexer, 1);
                }
                
                token.string.size = lexer.current - token.string.data;
            }
            
            else if (IsDigit(c))
            {
                // IMPORTANT TODO(soimn): Implement a proper parser which handles invalid literals gracefully
                
                token.type = Token_Number;
                
                U8* start = lexer.current - 1;
                
                bool is_hex       = false;
                bool is_hex_float = false;
                bool has_point    = false;
                bool has_exponent = false;
                
                I8 exponent_sign = 1;
                U64 exponent     = 0;
                
                if (c == 'x' || c == 'X')      is_hex       = true;
                else if (c == 'f' || c == 'F') is_hex_float = true;
                
                if (is_hex || is_hex_float)
                {
                    start += 2;
                    Advance(&lexer, 1);
                }
                
                while (*lexer.current != 0)
                {
                    c = *lexer.current;
                    
                    if (c == '_'                                           ||
                        IsDigit(c)                                         ||
                        (is_hex || is_hex_float) && (c >= 'A' && c <= 'F') ||
                        (is_hex || is_hex_float) && (c >= 'a' && c <= 'f'))
                    {
                        Advance(&lexer, 1);
                    }
                    
                    else if (!has_point && c == '.')
                    {
                        has_point = true;
                        Advance(&lexer, 1);
                    }
                    
                    else if (c == 'e' || c == 'E')
                    {
                        has_exponent = true;
                        Advance(&lexer, 1);
                        
                        if (is_hex || is_hex_float)
                        {
                            if (*lexer.current == '-' || *lexer.current == '+')
                            {
                                if (*lexer.current == '-') exponent_sign = -1;
                                Advance(&lexer, 1);
                            }
                            
                            if (IsDigit(*lexer.current))
                            {
                                while (IsDigit(*lexer.current))
                                {
                                    exponent *= 10;
                                    exponent += *lexer.current - '0';
                                    
                                    Advance(&lexer, 1);
                                }
                            }
                            
                            else
                            {
                                //// ERROR: Missing exponent after exponent specifier in numeric literal
                                token.type = Token_Error;
                            }
                        }
                        
                        else
                        {
                            //// ERROR: Exponents are only allowed on decimal literals
                            token.type = Token_Error;
                        }
                    }
                    
                    else break;
                }
                
                U8* end = lexer.current;
                
                if (end != start && *(end - 1) == '_')
                {
                    //// ERROR: Underscores in numeric literals are only allowed to be used as separators between 
                    ////        digits, not as the last character in the literal
                    token.type = Token_Error;
                }
                
                else if ((is_hex || is_hex_float) && end == start)
                {
                    //// ERROR: Missing numeric after base specifier in numeric literal
                    token.type = Token_Error;
                }
                
                else if (has_point && !IsDigit(*(end - 1)))
                {
                    //// ERROR: Invalid decimal part following point in numeric literal
                    token.type = Token_Error;
                }
                
                else if (exponent_sign == 1 && exponent > 127 || exponent_sign == -1 && exponent < -126)
                {
                    //// ERROR: Exponent out of range
                    token.type = Token_Error;
                }
                
                else
                {
                    if (has_point || has_exponent)
                    {
                        token.number.is_f32 = true;
                        
                        U8* scan = start;
                        for (; scan < end; ++scan)
                        {
                            if (*scan == '.' || *scan == 'e' || *scan == 'E') break;
                            else
                            {
                                F32 next = token.number.f32 * 10 + (*scan - '0');
                                
                                if (token.number.f32 > next)
                                {
                                    //// ERROR: Floating point literal too large
                                    token.type = Token_Error;
                                }
                                
                                else token.number.f32 = next;
                            }
                        }
                        
                        if (has_point)
                        {
                            ++scan;
                            for (; scan < end; ++scan)
                            {
                                F32 place = 0.1f;
                                
                                if (*scan == 'e' || *scan == 'E') break;
                                else
                                {
                                    F32 next = token.number.f32 + place * (*scan - '0');
                                    place /= 10;
                                    
                                    if (token.number.f32 > next)
                                    {
                                        //// ERROR: Floating point literal too large
                                        token.type = Token_Error;
                                    }
                                    
                                    else token.number.f32 = next;
                                }
                            }
                        }
                        
                        if (has_exponent)
                        {
                            for (; exponent != 0; exponent += -exponent_sign)
                            {
                                token.number.f32 *= (exponent_sign == 1 ? 10 : 0.1f);
                            }
                        }
                    }
                    
                    else if (is_hex_float)
                    {
                        token.number.is_f32 = true;
                        
                        U64 num = 0;
                        for (U8* scan = start; scan < end; ++scan)
                        {
                            U8 digit = 0;
                            
                            if (IsDigit(*scan))                    digit = *scan - '0';
                            else if (*scan >= 'A' && *scan <= 'F') digit = (*scan - 'A') + 10;
                            else                                   digit = (*scan - 'a') + 10;
                            
                            U64 next_num = num * 16 + digit;
                            
                            if (num > next_num)
                            {
                                //// ERROR: Hexadecimal floating point literal too large
                                token.type = Token_Error;
                            }
                            
                            else num = next_num;
                        }
                        
                        token.number.f32 = *(F32*)&num;
                    }
                    
                    else
                    {
                        token.number.is_u64 = true;
                        
                        U8 base = (is_hex ? 16 : 10);
                        for (U8* scan = start; scan < end; ++scan)
                        {
                            U8 digit = 0;
                            
                            if (IsDigit(*scan))                    digit = *scan - '0';
                            else if (*scan >= 'A' && *scan <= 'F') digit = (*scan - 'A') + 10;
                            else                                   digit = (*scan - 'a') + 10;
                            
                            U64 next = token.number.u64 * base + digit;
                            
                            if (token.number.u64 > next)
                            {
                                //// ERROR: Integer literal too large
                                token.type = Token_Error;
                            }
                            
                            else token.number.u64 = next;
                        }
                    }
                }
            }
            
            else if (c == '"')
            {
                token.type = Token_String;
                
                token.string.data = lexer.current;
                
                while (*lexer.current != 0 && *lexer.current != '"')
                {
                    Advance(&lexer, 1);
                }
                
                if (*lexer.current == '"')
                {
                    token.string.size = lexer.current - token.string.data;
                    Advance(&lexer, 1);
                }
                
                else
                {
                    //// ERROR: Missing terminator for string literal
                    token.type = Token_Error;
                }
            }
            
            else
            {
                token.type = Token_Error;
            }
        } break;
    }
    
    token.token_size = lexer.column - token.column;
    
    return token;
}

inline void
SkipPastToken(Parser_State* state, Token token)
{
    state->lexer_state.line_start = token.line_start;
    state->lexer_state.column     = token.column + token.token_size;
    state->lexer_state.line       = token.line;
    state->lexer_state.current    = state->lexer_state.line_start + state->lexer_state.column;
}

inline Token
PeekNextToken(Parser_State* state, Token token)
{
    Parser_State tmp_state = *state;
    SkipPastToken(&tmp_state, token);
    return GetToken(&tmp_state);
}

inline bool
MetRequiredToken(Parser_State* state, Enum32(LEXER_TOKEN_TYPE) type)
{
    bool result = false;
    
    Token token = GetToken(state);
    
    if (token.type == type)
    {
        result = true;
        SkipPastToken(state, token);
    }
    
    return result;
}