enum LEXER_TOKEN_TYPE
{
    Token_Unknown,
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
    
    Token_Tick,
    Token_Tilde,
    Token_Question,
    Token_Hash,
    Token_At,
    Token_Backtick,
    Token_Underscore,
    Token_Period,
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


struct Number
{
    bool is_integer;
    
    union
    {
        U64 u64;
        F64 f64;
    };
};

struct Token
{
    Enum32(LEXER_TOKEN_TYPE) type;
    U32 line;
    U32 column;
    String preceeding_string;
    String token_string;
    
    union
    {
        String string;
        Number number;
    };
};

struct Lexer
{
    String file_path;
    U8* current;
    U8* end;
    U8* line_start;
    U32 line;
    U32 column;
};

inline Lexer
LexFile(File* file)
{
    Lexer lexer = {};
    lexer.file_path  = file->file_path;
    lexer.current    = file->file_contents.data;
    lexer.end        = file->file_contents.data + file->file_contents.size;
    lexer.line_start = lexer.current;
    
    return lexer;
}

inline Token
GetToken(Lexer* lexer)
{
    Token token = {};
    token.type  = Token_Unknown;
    
    auto Advance = [](Lexer* lexer, U32 amount = 1)
    {
        for (U32 i = 0; i < amount && lexer->current < lexer->end; ++i)
        {
            ++lexer->column;
            ++lexer->current;
        }
    };
    
    auto ReportLexerError = [](Lexer* lexer, const char* message)
    {
        Print("%S(%u:%u): %s\n", lexer->file_path, lexer->line, lexer->column, message);
    };
    
    for (;;)
    {
        if (*lexer->current == ' ' || *lexer->current == '\t' || *lexer->current == '\v' || *lexer->current == '\r')
        {
            Advance(lexer, 1);
        }
        
        else if (*lexer->current == '\n')
        {
            Advance(lexer, 1);
            lexer->line_start = lexer->current;
            lexer->column     = 0;
            ++lexer->line;
        }
        
        else break;
    }
    
    token.line   = lexer->line;
    token.column = lexer->column;
    token.preceeding_string.data = lexer->line_start;
    token.preceeding_string.size = lexer->current - lexer->line_start;
    token.token_string.data = lexer->current;
    
    char c = *lexer->current;
    Advance(lexer, 1);
    
    switch (c)
    {
        case 0: token.type = Token_EndOfStream; break;
        
        case '\'': token.type = Token_Tick;      break;
        case '\\': token.type = Token_Backslash; break;
        
        case '~': token.type = Token_Tilde;      break;
        case '?': token.type = Token_Question;   break;
        case '#': token.type = Token_Hash;       break;
        case '@': token.type = Token_At;         break;
        case '`': token.type = Token_Backtick;   break;
        case '.': token.type = Token_Period;     break;
        case ',': token.type = Token_Comma;      break;
        case ';': token.type = Token_Semicolon;  break;
        
        case '(': token.type = Token_OpenParen;    break;
        case ')': token.type = Token_CloseParen;   break;
        case '[': token.type = Token_OpenBracket;  break;
        case ']': token.type = Token_CloseBracket; break;
        case '{': token.type = Token_OpenBrace;    break;
        case '}': token.type = Token_CloseBrace;   break;
        
        
#define SINGLE_OR_EQUALS(character, single_token, equals_token) \
        case character:                                                 \
        {                                                               \
            if (*lexer->current == '=')                                 \
            {                                                           \
                token.type = equals_token;                              \
                Advance(lexer, 1);                                      \
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
            if (*lexer->current == character)                                                \
            {                                                                                \
                token.type = double_token;                                                   \
                Advance(lexer, 1);                                                           \
            }                                                                                \
            else if (*lexer->current == '=')                                                 \
            {                                                                                \
                token.type = equals_token;                                                   \
                Advance(lexer, 1);                                                           \
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
            if (*lexer->current == character)                              \
            {                                                              \
                Advance(lexer, 1);                                         \
                if (*lexer->current == '=')                                \
                {                                                          \
                    token.type = double_and_equals_token;                  \
                    Advance(lexer, 1);                                     \
                }                                                          \
                else                                                       \
                {                                                          \
                    token.type = double_token;                             \
                }                                                          \
            }                                                              \
            else if (*lexer->current == '=')                               \
            {                                                              \
                token.type = equals_token;                                 \
                Advance(lexer, 1);                                         \
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
            if (*lexer->current == '-')
            {
                token.type = Token_MinusMinus;
                Advance(lexer, 1);
            }
            
            else if (*lexer->current == '=')
            {
                token.type = Token_MinusEQ;
                Advance(lexer, 1);
            }
            
            else if (*lexer->current == '>')
            {
                token.type = Token_RightArrow;
                Advance(lexer, 1);
            }
            
            else
            {
                token.type = Token_Minus;
            }
        } break;
        
        default:
        {
            if (c == '"')
            {
                token.type = Token_String;
                
                token.string.data = lexer->current;
                
                while (*lexer->current != 0 && *lexer->current != '"')
                {
                    if (*lexer->current == '\\' && *(lexer->current + 1) == '"')
                    {
                        Advance(lexer, 1);
                    }
                    
                    Advance(lexer, 1);
                }
                
                if (*lexer->current == '"')
                {
                    token.string.size = lexer->current - token.string.data;
                    Advance(lexer, 1);
                }
                
                else
                {
                    //// ERROR
                    ReportLexerError(lexer,"Did not encounter a terminating '\"' character in string literal before end of file");
                    token.type = Token_Error;
                }
            }
            
            else if (IsDigit(c))
            {
                // IMPORTANT NOTE(soimn): The precision of F64 is not great enough to cover the entire range of an
                //                        unsigned 64 bit integer without a loss of precision
                // HACK(soimn): Replace this
                
                F64 acc = c - '0';
                bool is_integer = true;
                
                while (IsDigit(*lexer->current))
                {
                    acc *= 10;
                    acc += *lexer->current - '0';
                    Advance(lexer, 1);
                }
                
                if (*lexer->current == '.' && IsDigit(*(lexer->current + 1)))
                {
                    is_integer = false;
                    Advance(lexer, 1);
                    
                    F64 current_place = 0.1f;
                    
                    while (IsDigit(*lexer->current))
                    {
                        acc += (*lexer->current - '0') * current_place;
                        current_place /= 10;
                        Advance(lexer, 1);
                    }
                }
                
                token.type = Token_Number;
                token.number.is_integer = is_integer;
                
                if (is_integer)
                {
                    if (acc <= U64_MAX)
                    {
                        token.number.u64 = (U64)acc;
                    }
                    
                    else
                    {
                        //// ERROR
                        ReportLexerError(lexer, "Integer constant is too large to be represented by any integer type.\n");
                        token.type = Token_Error;
                    }
                }
                
                else
                {
                    token.number.f64 = acc;
                }
            }
            
            else if (IsAlpha(c) || c == '_' || !IsASCII(c))
            {
                if (c == '_' && !IsAlpha(*lexer->current) && IsASCII(*lexer->current))
                {
                    token.type = Token_Underscore;
                }
                
                else
                {
                    token.type = Token_Identifier;
                    
                    token.string.data = lexer->current - 1;
                    
                    while (*lexer->current == '_' || IsAlpha(*lexer->current) || IsDigit(*lexer->current) || !IsASCII(*lexer->current))
                    {
                        Advance(lexer, 1);
                    }
                    
                    token.string.size = lexer->current - token.string.data;
                }
            }
        } break;
    }
    
    token.token_string.size = lexer->current - token.token_string.data;
    
    return token;
}

inline Token
PeekToken(Lexer* lexer, U32 amount = 1)
{
    Token result = {};
    
    Lexer tmp_lexer = *lexer;
    while (amount--)
    {
        result = GetToken(&tmp_lexer);
    }
    
    return result;
}

inline void
SkipToken(Lexer* lexer, U32 amount = 1)
{
    while (amount--)
    {
        GetToken(lexer);
    }
}

inline bool
RequiredToken(Lexer* lexer, Enum32(LEXER_TOKEN_TYPE) type)
{
    return (GetToken(lexer).type == type);
}