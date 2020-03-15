enum LEXER_TOKEN_TYPE
{
    Token_Error,
    Token_EndOfStream,   // EOF
    
    Token_Plus,          // +
    Token_Minus,         // -
    Token_Asterisk,      // *
    Token_ForwardSlash,  // /
    Token_BackwardSlash, // \\ 
    Token_Percentage,    // %
    Token_And,           // &
    Token_Pipe,          // |
    Token_Tilde,         // ~
    Token_Greater,       // >
    Token_Less,          // <
    Token_Exclamation,   // !
    Token_QuestionMark,  // ?
    Token_Period,        // .
    Token_Comma,         // ,
    Token_Colon,         // :
    Token_Semicolon,     // ;
    Token_Equals,        // =
    Token_Hash,          // #
    Token_Hat,           // ^
    Token_Cash,          // $
    Token_At,            // @
    Token_Tick,          // '
    Token_Underscore,    // _
    
    Token_OpenParen,     // (
    Token_CloseParen,    // )
    Token_OpenBracket,   // [
    Token_CloseBracket,  // ]
    Token_OpenBrace,     // {
    Token_CloseBrace,    // }
    
    Token_Identifier,    // [_]?[_A-Za-z0-9]+
    Token_String,        // ["]((\\")|[^"])*["]
    Token_Number,        // 0((x|h)[A-Fa-f0-9]+)|[0-9]+(\.[0-9]+)?
};

struct Text_Pos
{
    UMM line;
    UMM offset_to_line;
    UMM offset;
};

struct Token
{
    Enum32(LEXER_TOKEN_TYPE) type;
    
    union
    {
        String string;
        Number number;
    };
    
    Text_Pos position;
    UMM size;
};

struct Lexer
{
    Comp_State* comp_state;
    U8* start;
    Text_Pos pos;
    
    struct
    {
        bool is_valid;
        Token token;
    } cache;
};

inline Token
GetToken(Lexer* lexer)
{
    Token token = {Token_Error};
    
    if (lexer->cache.is_valid) token = lexer->cache.token;
    else
    {
        // NOTE(soimn): Helper functions to improve readability
        auto CharAt = [](Lexer* lexer, U32 pos) -> U8
        {
            return *(lexer->start + lexer->pos.offset + pos);
        };
        
        auto Advance = [](Lexer* lexer, U32 amount)
        {
            lexer->pos.offset += amount;
        };
        
        auto IsAlpha = [](U8 c) -> bool
        {
            return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
        };
        
        auto IsDigit = [](U8 c) -> bool
        {
            return (c >= '0' && c <= '9');
        };
        
        char c;
        while (c = CharAt(lexer, 0), c != 0)
        {
            if (c == ' ' || c == '\t' || c == '\v' || c == '\r') Advance(lexer, 1);
            else if (c == '\n')
            {
                Advance(lexer, 1);
                lexer->pos.line          += 1;
                lexer->pos.offset_to_line = lexer->pos.offset;
            }
            
            else if (c == '/' && CharAt(lexer, 1) == '/')
            {
                while (c = CharAt(lexer, 0), c != 0 && c != '\n')
                {
                    Advance(lexer, 1);
                }
            }
            
            else if (c == '/' && CharAt(lexer, 1) == '*')
            {
                Advance(lexer, 2);
                
                U32 indentation_level = 0;
                while (c = CharAt(lexer, 0), c != 0)
                {
                    if (c == '/' && CharAt(lexer, 1) == '*')
                    {
                        Advance(lexer, 2);
                        ++indentation_level;
                    }
                    
                    else if (c == '*' && CharAt(lexer, 1) == '/')
                    {
                        Advance(lexer, 2);
                        
                        if (indentation_level != 0) --indentation_level;
                        else break;
                    }
                    
                    else
                    {
                        Advance(lexer, 1);
                        
                        if (c == '\n')
                        {
                            Advance(lexer, 1);
                            lexer->pos.line          += 1;
                            lexer->pos.offset_to_line = lexer->pos.offset;
                        }
                    }
                }
            }
            
            else break;
        }
        
        Text_Pos start_of_token = lexer->pos;
        c                       = CharAt(lexer, 0);
        switch (c)
        {
            case '\0': token.type = Token_EndOfStream; break;
            case '+':  token.type  = Token_Plus;         Advance(lexer, 1); break;
            case '-':  token.type = Token_Minus;         Advance(lexer, 1); break;
            case '*':  token.type = Token_Asterisk;      Advance(lexer, 1); break;
            case '/':  token.type = Token_ForwardSlash;  Advance(lexer, 1); break;
            case '\\': token.type = Token_BackwardSlash; Advance(lexer, 1); break;
            case '%':  token.type = Token_Percentage;    Advance(lexer, 1); break;
            case '&':  token.type = Token_And;           Advance(lexer, 1); break;
            case '|':  token.type = Token_Pipe;          Advance(lexer, 1); break;
            case '~':  token.type = Token_Tilde;         Advance(lexer, 1); break;
            case '>':  token.type = Token_Greater;       Advance(lexer, 1); break;
            case '<':  token.type = Token_Less;          Advance(lexer, 1); break;
            case '!':  token.type = Token_Exclamation;   Advance(lexer, 1); break;
            case '?':  token.type = Token_QuestionMark;  Advance(lexer, 1); break;
            case '.':  token.type = Token_Period;        Advance(lexer, 1); break;
            case ',':  token.type = Token_Comma;         Advance(lexer, 1); break;
            case ':':  token.type = Token_Colon;         Advance(lexer, 1); break;
            case ';':  token.type = Token_Semicolon;     Advance(lexer, 1); break;
            case '=':  token.type = Token_Equals;        Advance(lexer, 1); break;
            case '#':  token.type = Token_Hash;          Advance(lexer, 1); break;
            case '^':  token.type = Token_Hat;           Advance(lexer, 1); break;
            case '$':  token.type = Token_Cash;          Advance(lexer, 1); break;
            case '@':  token.type = Token_At;            Advance(lexer, 1); break;
            case '\'': token.type = Token_Tick;          Advance(lexer, 1); break;
            case '(':  token.type = Token_OpenParen;     Advance(lexer, 1); break;
            case ')':  token.type = Token_CloseParen;    Advance(lexer, 1); break;
            case '[':  token.type = Token_OpenBracket;   Advance(lexer, 1); break;
            case ']':  token.type = Token_CloseBracket;  Advance(lexer, 1); break;
            case '{':  token.type = Token_OpenBrace;     Advance(lexer, 1); break;
            case '}':  token.type = Token_CloseBrace;    Advance(lexer, 1); break;
            
            default:
            {
                if (c == '_' || IsAlpha(c))
                {
                    if (!IsAlpha(CharAt(lexer, 1)) && !IsDigit(CharAt(lexer, 1)))
                    {
                        token.type = Token_Underscore;
                        Advance(lexer, 1);
                    }
                    
                    else
                    {
                        token.type = Token_Identifier;
                        token.string.data = lexer->start + lexer->pos.offset;
                        
                        while (c = CharAt(lexer, 0), IsAlpha(c) || IsDigit(c) || c == '_')
                        {
                            Advance(lexer, 1);
                            ++token.string.size;
                        }
                    }
                }
                
                else if (c == '"')
                {
                    Advance(lexer, 1);
                    
                    token.type = Token_String;
                    token.string.data = lexer->start + lexer->pos.offset;
                    
                    while (c = CharAt(lexer, 0), c != '"')
                    {
                        if (c == 0)
                        {
                            //// ERROR: Unexpected end of file in string
                            token.type = Token_Error;
                            break;
                        }
                        
                        else
                        {
                            if (c == '\\' && CharAt(lexer, 1) != 0)
                            {
                                Advance(lexer, 1);
                                ++token.string.size;
                            }
                            
                            Advance(lexer, 1);
                            ++token.string.size;
                            
                            if (c == '"') break;
                        }
                    }
                }
                
                else if (IsDigit(c))
                {
                    token.type = Token_Number;
                    
                    bool is_hex   = false;
                    bool is_float = false;
                    if (c == '0')
                    {
                        U8 next = CharAt(lexer, 1);
                        
                        is_hex   = (next == 'x' || next == 'h');
                        is_float = (next == 'h');
                        
                        if (is_hex) Advance(lexer, 2);
                    }
                    
                    if (is_hex && CharAt(lexer, 0) == 0)
                    {
                        //// ERROR: Missing digits of hexadecimal literal
                        token.type = Token_Error;
                    }
                    
                    else
                    {
                        U8 base             = (is_hex ? 16 : 10);
                        F64 multiplier      = base;
                        F64 multiplier_step = 1.0;
                        
                        F64 floating = 0;
                        U64 integer  = 0;
                        
                        bool floating_overflow = false;
                        bool integer_overflow  = false;
                        
                        U32 num_digits = 0;
                        
                        while (c = CharAt(lexer, 0), c != 0)
                        {
                            if (IsDigit(c))
                            {
                                Advance(lexer, 1);
                                
                                F64 new_floating = floating * multiplier;
                                U64 new_integer  = integer * base;
                                new_floating += (c - '0');
                                new_integer  += (c - '0');
                                
                                floating_overflow = (floating_overflow || new_floating < floating);
                                integer_overflow  = (integer_overflow || new_integer < integer);
                                
                                floating = new_floating;
                                integer  = new_integer;
                                
                                multiplier *= multiplier_step;
                                
                                ++num_digits;
                            }
                            
                            else if (is_hex && ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')))
                            {
                                Advance(lexer, 1);
                                
                                U64 new_integer = integer * base;
                                new_integer += (c >= 'a' ? c - 'a' : c - 'A') + 10;
                                
                                integer_overflow  = (integer_overflow || new_integer < integer);
                                
                                ++num_digits;
                            }
                            
                            else if (!is_float && c == '.' && IsDigit(CharAt(lexer, 1)))
                            {
                                Advance(lexer, 1);
                                
                                is_float = true;
                                multiplier_step = 0.1;
                            }
                            
                            else if (c == '_') Advance(lexer, 1);
                            else break;
                            
                            if (floating_overflow || integer_overflow) break;
                        }
                        
                        if (is_float && floating_overflow)
                        {
                            //// ERROR: Floating point literal exceeds range of float
                            token.type = Token_Error;
                        }
                        
                        else if (!is_float && integer_overflow)
                        {
                            //// ERROR: Integer literal exceeds range of int
                            token.type = Token_Error;
                        }
                        
                        else
                        {
                            if (is_hex && is_float)
                            {
                                // TODO(soimn): Find a better way of converting the hex values to floating point
                                
                                if (num_digits == 8)
                                {
                                    token.number.is_int = false;
                                    token.number.is_f64 = false;
                                    
                                    U32 hex_value = (U32)integer;
                                    token.number.f32 = *(F32*)&hex_value;
                                }
                                
                                else if (num_digits == 16)
                                {
                                    token.number.is_int = false;
                                    token.number.is_f64 = true;
                                    
                                    U64 hex_value = (U64)integer;
                                    token.number.f64 = *(F64*)&hex_value;
                                }
                                
                                else
                                {
                                    //// ERROR: Hexadecimal floating point literals must have a length 8 or 16 digits ////        corresponding to float32 and float64 respectively
                                    token.type = Token_Error;
                                }
                            }
                            
                            else if (is_float)
                            {
                                if (CharAt(lexer, 0) == 'e' || CharAt(lexer, 0) == 'E')
                                {
                                    Advance(lexer, 1);
                                    
                                    bool is_signed = false;
                                    if (CharAt(lexer, 0) == '+') Advance(lexer, 1);
                                    else if (CharAt(lexer, 0) == '-')
                                    {
                                        is_signed = true;
                                        Advance(lexer, 1);
                                    }
                                    
                                    if (IsDigit(CharAt(lexer, 0)))
                                    {
                                        U64 exponent           = 0;
                                        bool exponent_overflow = false;
                                        while (c = CharAt(lexer, 0), c != 0)
                                        {
                                            if (IsDigit(c))
                                            {
                                                U64 new_exponent = exponent * 10;
                                                new_exponent    += (c - '0');
                                                
                                                exponent_overflow = exponent_overflow || new_exponent < exponent;
                                            }
                                            
                                            else if (c == '_') Advance(lexer, 1);
                                            else break;
                                        }
                                        
                                        if (exponent_overflow)
                                        {
                                            //// ERROR: Floating point literal exponent is out of range
                                            token.type = Token_Error;
                                        }
                                        
                                        else
                                        {
                                            F64 exponent_mult = (is_signed ? 0.1 : 10);
                                            
                                            // TODO(soimn): Find a better way of dealing with exponents
                                            while (exponent-- > 0)
                                            {
                                                floating *= exponent_mult;
                                                
                                                if (floating < F32_MIN)
                                                {
                                                    //// ERROR: Floating point literal is smaller than F32_MIN
                                                    token.type = Token_Error;
                                                }
                                                
                                                else if (floating > F32_MAX)
                                                {
                                                    //// ERROR: Floating point literal is larger than F32_MAX
                                                    token.type = Token_Error;
                                                }
                                            }
                                        }
                                    }
                                    
                                    else
                                    {
                                        //// ERROR: Missing exponent digits after exponent specifier in floating point 
                                        ////        literal
                                        token.type = Token_Error;
                                    }
                                }
                                
                                if (token.type != Token_Error)
                                {
                                    token.number.is_int = false;
                                    token.number.is_f64 = false;
                                    token.number.f32    = (F32)floating;
                                }
                            }
                            
                            else
                            {
                                token.number.is_int = true;
                                token.number.u64    = integer;
                            }
                        }
                    }
                }
                
                else
                {
                    //// ERROR: Encountered an unknown token
                    token.type = Token_Error;
                }
            } break;
        }
        
        token.position = start_of_token;
        token.size = lexer->pos.offset - start_of_token.offset;
        lexer->cache.token    = token;
        lexer->cache.is_valid = true;
    }
    
    return token;
}

inline void
SkipPastToken(Lexer* lexer, Token token)
{
    ASSERT(lexer->cache.is_valid);
    
    lexer->pos         = token.position;
    lexer->pos.offset += token.size;
    
    lexer->cache.is_valid = false;
}

inline Token
PeekNextToken(Lexer* lexer, Token token)
{
    Lexer tmp_lexer = *lexer;
    SkipPastToken(&tmp_lexer, token);
    return GetToken(&tmp_lexer);
}