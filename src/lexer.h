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

enum KEYWORD_KIND
{
    Keyword_Foreign,
    Keyword_Import,
    Keyword_Load,
    Keyword_As,
    
    Keyword_If,
    Keyword_Else,
    Keyword_For,
    Keyword_Break,
    Keyword_Continue,
    
    Keyword_Using,
    Keyword_Return,
    Keyword_Defer,
    
    Keyword_Proc,
    Keyword_Struct,
    Keyword_Enum,
    
    Keyword_True,
    Keyword_False,
    
    KEYWORD_KIND_COUNT
};

String KeywordStrings[KEYWORD_KIND_COUNT] = {
    [Keyword_Foreign]  = CONST_STRING("Foreign"),
    [Keyword_Import]   = CONST_STRING("Import"),
    [Keyword_Load]     = CONST_STRING("Load"),
    [Keyword_As]       = CONST_STRING("As"),
    [Keyword_If]       = CONST_STRING("If"),
    [Keyword_Else]     = CONST_STRING("Else"),
    [Keyword_For]      = CONST_STRING("For"),
    [Keyword_Break]    = CONST_STRING("Break"),
    [Keyword_Continue] = CONST_STRING("Continue"),
    [Keyword_Using]    = CONST_STRING("Using"),
    [Keyword_Return]   = CONST_STRING("Return"),
    [Keyword_Defer]    = CONST_STRING("Defer"),
    [Keyword_Proc]     = CONST_STRING("Proc"),
    [Keyword_Struct]   = CONST_STRING("Struct"),
    [Keyword_Enum]     = CONST_STRING("Enum"),
    [Keyword_True]     = CONST_STRING("True"),
    [Keyword_False]    = CONST_STRING("False"),
};

typedef struct Number
{
    union { f64 floating; u64 integer; };
    
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
    u8 kind;
    
    union
    {
        Number number;
        
        struct
        {
            String string;
            u8 keyword;
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
TextInterval_Merge(Text_Interval i0, Text_Interval i1)
{
    ASSERT(i0.position.file == i1.position.file);
    
    Text_Interval result = {0};
    
    imm delta = (i1.position.column + i1.position.offset_to_line) - (i0.position.column + i0.position.offset_to_line);
    
    if (delta > 0)
    {
        result.position = i0.position;
        result.size     = MAX(i0.size, (u32)delta + i1.size);
    }
    
    else
    {
        result.position = i1.position;
        result.size     = MAX(i1.size, (u32)-delta + i0.size);
    }
    
    return result;
}

typedef struct Lexer
{
    Package* package;
    File_ID file;
    u8* start;
    Token token;
    bool token_valid;
} Lexer;

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
        base_pos.column += lexer->token.text.size;
        
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
                    ++nest_level;
                    
                    current   += 2;
                    is_comment = true;
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
            
#define SINGLE_OR_EQ(character, t0, t1) else if (c == character) token.kind = (current[0] == '=' ? t0 : t1)
            SINGLE_OR_EQ('+', Token_PlusEQ,    Token_Plus);
            SINGLE_OR_EQ('-', Token_MinusEQ,   Token_Minus);
            SINGLE_OR_EQ('*', Token_MulEQ,     Token_Asterisk);
            SINGLE_OR_EQ('/', Token_DivEQ,     Token_Slash);
            SINGLE_OR_EQ('%', Token_ModEQ,     Token_Mod);
            SINGLE_OR_EQ('=', Token_IsEqual,   Token_Equals);
#undef SINGLE_OR_EQ
            
            else if (c == '&')
            {
                if (current[0] == '&')
                {
                    if (current[1] == '=') token.kind = Token_AndEQ;
                    else                   token.kind = Token_And;
                    
                    current += (current[1] == '=' ? 2 : 1);
                }
                
                else
                {
                    if (current[0] == '=') token.kind = Token_BitAndEQ;
                    else                   token.kind = Token_BitAnd;
                    
                    current += (current[0] == '=' ? 1 : 0);
                }
            }
            
            else if (c == '|')
            {
                if (current[0] == '|')
                {
                    if (current[1] == '=') token.kind = Token_OrEQ;
                    else                   token.kind = Token_Or;
                    
                    current += (current[1] == '=' ? 2 : 1);
                }
                
                else
                {
                    if (current[0] == '=') token.kind = Token_BitOrEQ;
                    else                   token.kind = Token_BitOr;
                    
                    current += (current[0] == '=' ? 1 : 0);
                }
                
            }
            
            else if (c == '<')
            {
                if (current[0] == '<')
                {
                    if (current[1] == '=') token.kind = Token_BitShiftLEQ;
                    else                   token.kind = Token_BitShiftL;
                    
                    current += (current[1] == '=' ? 2 : 1);
                }
                
                else
                {
                    if (current[0] == '=') token.kind == Token_LessEQ;
                    else                   token.kind == Token_Less;
                    
                    current += (current[0] == '=' ? 1 : 0);
                }
            }
            
            else if (c == '>')
            {
                if (current[0] == '>')
                {
                    if (current[1] == '=') token.kind = Token_BitShiftREQ;
                    else                   token.kind = Token_BitShiftR;
                    
                    current += (current[1] == '=' ? 2 : 1);
                }
                
                else
                {
                    if (current[0] == '=') token.kind == Token_GreaterEQ;
                    else                   token.kind == Token_Greater;
                    
                    current += (current[0] == '=' ? 1 : 0);
                }
            }
            
            else if (c == '"' || c == '\'')
            {
                char terminator = c;
                String string   = { .data = current };
                
                while (*current != 0 && *current != terminator) ++current;
                
                if (*current == 0)
                {
                    //// ERROR: Missing matching "/'
                    token.kind = Token_Invalid;
                }
                
                else
                {
                    string.size = current - token.string.data;
                    ++current;
                    
                    NOT_IMPLEMENTED;
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
                        //// Only underscores, must have at least one alphanumerical symbol
                        
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
                    uint digit_count        = (integer != 0);
                    while (!encountered_errors)
                    {
                        if (is_float) digit_mod /= 10;
                        
                        u8 digit = 0;
                        
                        if (IsDigit(*current))
                        {
                            digit = *current - '0';
                            
                            if (is_binary && digit >= 2) encountered_errors = true;
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
                    
                    if (!encountered_errors)
                    {
                        if (is_float)
                        {
                            i8 exponent_sign = 1;
                            u64 exponent     = 0;
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
                                    }
                                    
                                    else if (*current == '_')
                                    {
                                        if (current[-1] == '-' || current[-1] == '+' ||
                                            ToUpper(current[-1]) == 'E')
                                        {
                                            encountered_errors = true;
                                        }
                                        
                                        else; // NOTE(soimn): Do nothing
                                    }
                                    
                                    else break;
                                    
                                    ++current;
                                }
                            }
                            
                            if (!encountered_errors)
                            {
                                // TODO(soimn): find a better way to deal with the exponent
                                if (exponent_sign == 1) for (u64 i = 0; i < exponent; ++i) floating *= 10;
                                else                    for (u64 i = 0; i < exponent; ++i) floating /= 10;
                                
                                if (is_hex)
                                {
                                    if (digit_count == 8)
                                    {
                                        // TODO(soimn): Convert from base 16 integer to binary32 without relying on type punning
                                        token.number.is_float = true;
                                        token.number.floating = (f32)*(f32*)&integer;
                                        token.number.min_fit_bits = 32;
                                    }
                                    
                                    else if (digit_count == 16)
                                    {
                                        // TODO(soimn): Convert from base 16 integer to binary32 without relying on type punning
                                        token.number.is_float = true;
                                        token.number.floating = (f64)*(f64*)&integer;
                                        token.number.min_fit_bits = 64;
                                    }
                                    
                                    else encountered_errors = true;
                                }
                                
                                else
                                {
                                    token.number.is_float = true;
                                    token.number.floating = floating;
                                    
                                    // TODO(soimn): find a better way of judging min fit bits
                                    if (floating > FLT_MAX || floating < FLT_MIN) token.number.min_fit_bits = 64;
                                    else                                          token.number.min_fit_bits = 32;
                                }
                            }
                        }
                        
                        else
                        {
                            token.number.is_float = false;
                            token.number.integer  = integer;
                        }
                    }
                    
                    if (encountered_errors) token.kind = Token_Invalid;
                }
            }
            
            else; // NOTE(soimn): Invalid token
        }
        
        token.text.position = base_pos;
        token.text.size     = (u32)(current - lexer->start) - (base_pos.offset_to_line + base_pos.column);
        
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
DEBUG_TestLexer()
{
    const char* lex_string = "x : int = 0; x *= 1.0e10;";
    
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
            case Token_Char:         name = "Token_Char"  ;       break;
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