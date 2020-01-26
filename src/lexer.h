enum LEXER_TOKEN_TYPE
{
    Token_Error,
    
    Token_Plus,
    Token_Minus,
    Token_And,
    Token_Equals,
    Token_Colon,
    Token_Pipe,
    
    Token_PlusPlus,
    Token_MinusMinus,
    Token_AndAnd,
    Token_EQEQ,
    Token_ColonColon,
    Token_PipePipe,
    
    Token_PlusEQ,
    Token_MinusEQ,
    Token_AndEQ,
    Token_ColonEQ,
    Token_PipeEQ,
    
    Token_Hat,
    Token_Asterisk,
    Token_Slash,
    Token_Percentage,
    Token_Exclamation,
    
    Token_HatEQ,
    Token_AsteriskEQ,
    Token_SlashEQ,
    Token_PercentageEQ,
    Token_ExclamationEQ,
    
    Token_GreaterThan,
    Token_LessThan,
    Token_GTGT,
    Token_LTLT,
    Token_GTEQ,
    Token_LTEQ,
    Token_GTGTEQ,
    Token_LTLTEQ,
    
    Token_Period,
    Token_PeriodPeriod,
    
    Token_Tick,
    Token_Tilde,
    Token_Question,
    Token_Hash,
    Token_At,
    Token_Backtick,
    Token_Underscore,
    Token_Comma,
    Token_Semicolon,
    Token_Backslash,
    Token_RightArrow,
    
    Token_OpenParen,
    Token_CloseParen,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenBrace,
    Token_CloseBrace,
    
    Token_Number,
    Token_String,
    Token_Identifier,
    
    Token_EndOfStream,
};

enum LEXER_ERROR_CODE
{
    LexerError_InvalidToken,
    LexerError_MissingTerminatingQuote,
    LexerError_IntegerLiteralTooLarge,
    LexerError_FloatLiteralTooLarge,
    LexerError_FloatLiteralTooSmall,
    LexerError_InvalidHexadecimalLiteral,
};

struct Token
{
    Enum32(LEXER_TOKEN_TYPE) type;
    U32 line;
    U32 column;
    U8* line_start;
    String token_string;
    
    union
    {
        String string;
        Number number;
        Enum32(LEXER_ERROR_CODE) error_code;
    };
};

struct Lexer
{
    U8* current;
    U8* end;
    U8* line_start;
    U32 line;
};

inline Lexer
LexFile(File* file)
{
    Lexer lexer = {};
    lexer.current    = file->file_contents.data;
    lexer.end        = file->file_contents.data + file->file_contents.size;
    lexer.line_start = lexer.current;
    
    return lexer;
}

// IMPORTANT NOTE(soimn): GetToken expects a null terminated string
// TODO(soimn): Find out whether the lexer should handle error reporting itself or "outsource" it by returning an 
//              error token
inline Token
GetToken(Lexer* lexer)
{
    Token token = {};
    token.type  = Token_Error;
    
    U8* current_char       = lexer->current;
    U32 current_line       = lexer->line;
    U8* current_line_start = lexer->line_start;
    
    // NOTE(soimn): Helper function to improve readability
    auto Advance = [](U8** current_char)
    {
        *current_char += 1;
    };
    
    for (;;)
    {
        char c = *current_char;
        if (c == ' ' || c == '\t' || c == '\v' || c == '\r')
        {
            Advance(&current_char);
        }
        
        else if (c == '\n')
        {
            Advance(&current_char);
            ++current_line;
            current_line_start = current_char;
        }
        
        else break;
    }
    
    token.line              = current_line;
    token.column            = (U32)(current_char - current_line_start);
    token.line_start        = current_line_start;
    token.token_string.data = current_char;
    
    char c = *current_char;
    
    // NOTE(soimn): this is to protect the lexer from jumping over an EOF and reading garbage
    if (c != 0) Advance(&current_char);
    
    switch (c)
    {
        case 0:    token.type = Token_EndOfStream;  break;
        
        case '\'': token.type = Token_Tick;         break;
        case '\\': token.type = Token_Backslash;    break;
        
        case '~':  token.type = Token_Tilde;        break;
        case '?':  token.type = Token_Question;     break;
        case '#':  token.type = Token_Hash;         break;
        case '@':  token.type = Token_At;           break;
        case '`':  token.type = Token_Backtick;     break;
        case ',':  token.type = Token_Comma;        break;
        case ';':  token.type = Token_Semicolon;    break;
        
        case '(':  token.type = Token_OpenParen;    break;
        case ')':  token.type = Token_CloseParen;   break;
        case '[':  token.type = Token_OpenBracket;  break;
        case ']':  token.type = Token_CloseBracket; break;
        case '{':  token.type = Token_OpenBrace;    break;
        case '}':  token.type = Token_CloseBrace;   break;
        
        
#define SINGLE_OR_EQUALS(character, single_token, equals_token) \
        case character:                                                 \
        {                                                               \
            if (*current_char == '=')                                   \
            {                                                           \
                token.type = equals_token;                              \
                Advance(&current_char);                                 \
            }                                                           \
            else                                                        \
            {                                                           \
                token.type = single_token;                              \
            }                                                           \
        } break
        
        SINGLE_OR_EQUALS('^', Token_Hat, Token_HatEQ);
        SINGLE_OR_EQUALS('*', Token_Asterisk, Token_AsteriskEQ);
        SINGLE_OR_EQUALS('/', Token_Slash, Token_SlashEQ);
        SINGLE_OR_EQUALS('%', Token_Percentage, Token_PercentageEQ);
        SINGLE_OR_EQUALS('!', Token_Exclamation, Token_ExclamationEQ);
        
#undef SINGLE_OR_EQUALS
        
#define SINGLE_DOUBLE_OR_EQUALS(character, single_token, double_token, equals_token) \
        case character:                                                                      \
        {                                                                                    \
            if (*current_char == character)                                                  \
            {                                                                                \
                token.type = double_token;                                                   \
                Advance(&current_char);                                                      \
            }                                                                                \
            else if (*current_char == '=')                                                   \
            {                                                                                \
                token.type = equals_token;                                                   \
                Advance(&current_char);                                                      \
            }                                                                                \
            else                                                                             \
            {                                                                                \
                token.type = single_token;                                                   \
            }                                                                                \
        } break
        
        SINGLE_DOUBLE_OR_EQUALS('+', Token_Plus,   Token_PlusPlus,   Token_PlusEQ);
        SINGLE_DOUBLE_OR_EQUALS('&', Token_And,    Token_AndAnd,     Token_AndEQ);
        SINGLE_DOUBLE_OR_EQUALS('|', Token_Pipe,   Token_PipePipe,   Token_PipeEQ);
        SINGLE_DOUBLE_OR_EQUALS(':', Token_Colon,  Token_ColonColon, Token_ColonEQ);
        SINGLE_DOUBLE_OR_EQUALS('=', Token_Equals, Token_EQEQ,       Token_EQEQ);
        
#undef SINGLE_DOUBLE_OR_EQUALS
        
#define SINGLE_DOUBLE_EQUALS_OR_DOUBLE_AND_EQUALS(character, single_token, double_token, equals_token, double_and_equals_token) \
        case character:                                                    \
        {                                                                  \
            if (*current_char == character)                                \
            {                                                              \
                Advance(&current_char);                                    \
                if (*current_char == '=')                                  \
                {                                                          \
                    token.type = double_and_equals_token;                  \
                    Advance(&current_char);                                \
                }                                                          \
                else                                                       \
                {                                                          \
                    token.type = double_token;                             \
                }                                                          \
            }                                                              \
            else if (*current_char == '=')                                 \
            {                                                              \
                token.type = equals_token;                                 \
                Advance(&current_char);                                    \
            }                                                              \
            else                                                           \
            {                                                              \
                token.type = single_token;                                 \
            }                                                              \
        } break
        
        SINGLE_DOUBLE_EQUALS_OR_DOUBLE_AND_EQUALS('>', Token_GreaterThan, Token_GTGT, Token_GTEQ, Token_GTGTEQ);
        SINGLE_DOUBLE_EQUALS_OR_DOUBLE_AND_EQUALS('<', Token_LessThan, Token_LTLT, Token_LTEQ, Token_LTLTEQ);
        
#undef SINGLE_DOUBLE_EQUALS_OR_DOUBLE_AND_EQUALS
        
        case '-':
        {
            if (*current_char == '-')
            {
                token.type = Token_MinusMinus;
                Advance(&current_char);
            }
            
            else if (*current_char == '=')
            {
                token.type = Token_MinusEQ;
                Advance(&current_char);
            }
            
            else if (*current_char == '>')
            {
                token.type = Token_RightArrow;
                Advance(&current_char);
            }
            
            else
            {
                token.type = Token_Minus;
            }
        } break;
        
        case '.':
        {
            if (*current_char == '.')
            {
                token.type = Token_PeriodPeriod;
                Advance(&current_char);
            }
            
            else
            {
                token.type = Token_Period;
            }
        } break;
        
        default:
        {
            if (c == '"')
            {
                token.type = Token_String;
                token.string.data = current_char;
                
                while (*current_char != 0 && *current_char != '"')
                {
                    if (*current_char == '\\' && *(current_char + 1) == '"')
                    {
                        Advance(&current_char);
                    }
                    
                    Advance(&current_char);
                }
                
                if (*current_char == '"')
                {
                    token.string.size = current_char - token.string.data;
                    Advance(&current_char);
                }
                
                else
                {
                    //// ERROR
                    token.type       = Token_Error;
                    token.error_code = LexerError_MissingTerminatingQuote;
                }
            }
            
            else if (IsDigit(c))
            {
                token.type = Token_Number;
                
                // NOTE(soimn): This is decremented to include the c character aswell
                U8* start = current_char - 1;
                
                bool is_hex = false;
                if (c == '0' && ToLower(*current_char) == 'x')
                {
                    is_hex = true;
                    start += 2;
                    Advance(&current_char);
                }
                
                bool has_passed_point = false;
                for (;;)
                {
                    if (IsDigit(*current_char))
                    {
                        Advance(&current_char);
                    }
                    
                    else if (*current_char == '.' && IsDigit(*(current_char + 1)))
                    {
                        if (!has_passed_point && !is_hex)
                        {
                            has_passed_point = true;
                            Advance(&current_char);
                        }
                        
                        else break;
                    }
                    
                    else if (is_hex && ToLower(*current_char) >= 'a' && ToLower(*current_char) <= 'f')
                    {
                        Advance(&current_char);
                    }
                    
                    else break;
                }
                
                U8* end = current_char;
                
                if (is_hex && start == end)
                {
                    //// ERROR
                    token.type       = Token_Error;
                    token.error_code = LexerError_InvalidHexadecimalLiteral;
                }
                
                else
                {
                    if (!has_passed_point)
                    {
                        token.number.is_integer = true;
                        token.number.u64        = 0;
                        
                        U8 base = (is_hex ? 16 : 10);
                        for (U8* scan = start; scan < end; ++scan)
                        {
                            U8 digit = 0;
                            
                            if (IsDigit(*scan))
                            {
                                digit = *scan - '0';
                            }
                            
                            else
                            {
                                digit = (ToLower(*scan) - 'a') + 10;
                            }
                            
                            U64 new_number = token.number.u64;
                            new_number *= base;
                            new_number += digit;
                            
                            if (new_number > token.number.u64 || new_number == 0 && token.number.u64 == 0)
                            {
                                token.number.u64 = new_number;
                            }
                            
                            else
                            {
                                //// ERROR
                                token.type       = Token_Error;
                                token.error_code = LexerError_IntegerLiteralTooLarge;
                            }
                        }
                    }
                    
                    else
                    {
                        // IMPORTANT TODO(soimn): Implement a proper float parser
                        // Usefull resources:
                        //   - https://ampl.com/REFS/rounding.pdf
                        
                        // TODO(soimn): Detect under- and overflow
                        
                        token.number.is_integer = false;
                        token.number.f64        = 0;
                        
                        U8* scan = start;
                        for (; scan < end; ++scan)
                        {
                            if (*scan == '.') break;
                            else
                            {
                                token.number.f64 *= 10;
                                token.number.f64 += *scan - '0';
                            }
                        }
                        
                        ++scan;
                        
                        F64 decimal_place = 0.1;
                        
                        for (; scan < end; ++scan)
                        {
                            token.number.f64 += (*scan - '0') * decimal_place;
                            decimal_place    /= 10;
                        }
                    }
                }
            }
            
            else if (IsAlpha(c) || c == '_' || !IsASCII(c))
            {
                if (c == '_' && !IsAlpha(*current_char) && IsASCII(*current_char))
                {
                    token.type = Token_Underscore;
                }
                
                else
                {
                    token.type = Token_Identifier;
                    
                    // NOTE(soimn): This is decremented to include the c character aswell
                    token.string.data = current_char - 1;
                    
                    while (*current_char == '_' || IsAlpha(*current_char) || IsDigit(*current_char) || !IsASCII(*current_char))
                    {
                        Advance(&current_char);
                    }
                    
                    token.string.size = current_char - token.string.data;
                }
            }
        } break;
    }
    
    token.token_string.size = current_char - token.token_string.data;
    
    return token;
}

inline void
SkipPastToken(Lexer* lexer, Token token)
{
    U8* new_current = token.line_start + token.column + token.token_string.size;
    ASSERT(new_current < lexer->end);
    
    lexer->current    = new_current;
    lexer->line       = token.line;
    lexer->line_start = token.line_start;
}

inline Token
GetAndSkipToken(Lexer* lexer)
{
    Token token = GetToken(lexer);
    SkipPastToken(lexer, token);
    
    return token;
}

inline Token
PeekNextToken(Lexer* lexer, Token prev_token)
{
    Lexer tmp_lexer = *lexer;
    SkipPastToken(&tmp_lexer, prev_token);
    
    return GetToken(&tmp_lexer);
}

inline bool
RequireAndSkipToken(Lexer* lexer, Enum32(LEXER_TOKEN_TYPE) type)
{
    return (GetAndSkipToken(lexer).type == type);
}