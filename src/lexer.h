#pragma once

#include "common.h"

struct Lexer
{
    String input;
    
    File_Loc location;
    
    char peek[2];
};

enum TOKEN_TYPE
{
    Token_Error = 0,
    Token_EndOfStream,
    
    Token_Identifier,
    Token_String,
    Token_Number,
    
    /// Arithmetic
    Token_Plus,
    Token_PlusEquals,
    Token_Increment,
    Token_Minus,
    Token_MinusEquals,
    Token_Decrement,
    Token_Asterisk,
    Token_MultiplyEquals,
    Token_Divide,
    Token_DivideEquals,
    Token_Modulo,
    Token_ModuloEquals,
    
    /// Bitwise operators
    Token_Ampersand,
    Token_AndEquals,
    Token_Or,
    Token_OrEquals,
    Token_XOR,
    Token_XOREquals,
    Token_Not,
    
    Token_Equals,
    
    /// Logical operations
    Token_LogicalAnd,
    Token_LogicalOr,
    Token_LogicalNot,
    
    /// Comparison
    Token_IsEqual,
    Token_IsNotEqual,
    Token_GreaterThan,
    Token_GreaterThanOrEqual,
    Token_LessThan,
    Token_LessThanOrEqual,
    
    Token_Questionmark,
    
    /// Bitwise shift
    Token_RightShift,
    Token_RightShiftEquals,
    Token_LeftShift,
    Token_LeftShiftEquals,
    
    
    Token_OpenParen,
    Token_CloseParen,
    Token_OpenBrace,
    Token_CloseBrace,
    Token_OpenBracket,
    Token_CloseBracket,
    
    /// Special
    Token_Arrow,
    Token_Dot,
    Token_Elipsis,
    Token_Comma,
    Token_Colon,
    Token_DoubleColon,
    Token_Semicolon,
    Token_TripleDash,
    Token_Alpha,
    Token_Hash,
    Token_Keyword,
    
    TOKEN_TYPE_COUNT,
};

enum LEXER_KEYWORD_TYPE
{
    Keyword_If,
    Keyword_Else,
    Keyword_While,
    Keyword_Defer,
    Keyword_Return,
    Keyword_Cast,
    Keyword_Struct,
    Keyword_Enum,
    Keyword_Proc,
};

struct Token
{
    Enum32(TOKEN_TYPE) type;
    
    File_Loc location;
    
    String string;
    Number number;
    Enum32(LEXER_KEYWORD_TYPE) keyword;
};

inline String
GetNameOfTokenType(Enum32(TOKEN_TYPE) type)
{
    String result = {};
    
    switch (type)
    {
        case Token_Error:              result = CONST_STRING("Token_Error");              break;
        case Token_EndOfStream:        result = CONST_STRING("Token_EndOfStream");        break;
        case Token_Identifier:         result = CONST_STRING("Token_Identifier");         break;
        case Token_String:             result = CONST_STRING("Token_String");             break;
        case Token_Number:             result = CONST_STRING("Token_Number");             break;
        case Token_Plus:               result = CONST_STRING("Token_Plus");               break;
        case Token_PlusEquals:         result = CONST_STRING("Token_PlusEquals");         break;
        case Token_Increment:          result = CONST_STRING("Token_Increment");          break;
        case Token_Minus:              result = CONST_STRING("Token_Minus");              break;
        case Token_MinusEquals:        result = CONST_STRING("Token_MinusEquals");        break;
        case Token_Decrement:          result = CONST_STRING("Token_Decrement");          break;
        case Token_Asterisk:           result = CONST_STRING("Token_Asterisk");           break;
        case Token_MultiplyEquals:     result = CONST_STRING("Token_MultiplyEquals");     break;
        case Token_Divide:             result = CONST_STRING("Token_Divide");             break;
        case Token_DivideEquals:       result = CONST_STRING("Token_DivideEquals");       break;
        case Token_Modulo:             result = CONST_STRING("Token_Modulo");             break;
        case Token_ModuloEquals:       result = CONST_STRING("Token_ModuloEquals");       break;
        case Token_Ampersand:          result = CONST_STRING("Token_Ampersand");          break;
        case Token_AndEquals:          result = CONST_STRING("Token_AndEquals");          break;
        case Token_Or:                 result = CONST_STRING("Token_Or");                 break;
        case Token_OrEquals:           result = CONST_STRING("Token_OrEquals");           break;
        case Token_XOR:                result = CONST_STRING("Token_XOR");                break;
        case Token_XOREquals:          result = CONST_STRING("Token_XOREquals");          break;
        case Token_Not:                result = CONST_STRING("Token_Not");                break;
        case Token_Equals:             result = CONST_STRING("Token_Equals");             break;
        case Token_LogicalAnd:         result = CONST_STRING("Token_LogicalAnd");         break;
        case Token_LogicalOr:          result = CONST_STRING("Token_LogicalOr");          break;
        case Token_LogicalNot:         result = CONST_STRING("Token_LogicalNot");         break;
        case Token_IsEqual:            result = CONST_STRING("Token_IsEqual");            break;
        case Token_IsNotEqual:         result = CONST_STRING("Token_IsNotEqual");         break;
        case Token_GreaterThan:        result = CONST_STRING("Token_GreaterThan");        break;
        case Token_GreaterThanOrEqual: result = CONST_STRING("Token_GreaterThanOrEqual"); break;
        case Token_LessThan:           result = CONST_STRING("Token_LessThan");           break;
        case Token_LessThanOrEqual:    result = CONST_STRING("Token_LessThanOrEqual");    break;
        case Token_RightShift:         result = CONST_STRING("Token_RightShift");         break;
        case Token_RightShiftEquals:   result = CONST_STRING("Token_RightShiftEquals");   break;
        case Token_LeftShift:          result = CONST_STRING("Token_LeftShift");          break;
        case Token_LeftShiftEquals:    result = CONST_STRING("Token_LeftShiftEquals");    break;
        case Token_OpenParen:          result = CONST_STRING("Token_OpenParen");          break;
        case Token_CloseParen:         result = CONST_STRING("Token_CloseParen");         break;
        case Token_OpenBrace:          result = CONST_STRING("Token_OpenBrace");          break;
        case Token_CloseBrace:         result = CONST_STRING("Token_CloseBrace");         break;
        case Token_OpenBracket:        result = CONST_STRING("Token_OpenBracket");        break;
        case Token_CloseBracket:       result = CONST_STRING("Token_CloseBracket");       break;
        case Token_Arrow:              result = CONST_STRING("Token_Arrow");              break;
        case Token_Dot:                result = CONST_STRING("Token_Dot");                break;
        case Token_Elipsis:            result = CONST_STRING("Token_Elipsis");            break;
        case Token_Comma:              result = CONST_STRING("Token_Comma");              break;
        case Token_Questionmark:       result = CONST_STRING("Token_Questionmark");       break;
        case Token_Colon:              result = CONST_STRING("Token_Colon");              break;
        case Token_DoubleColon:        result = CONST_STRING("Token_DoubleColon");        break;
        case Token_Semicolon:          result = CONST_STRING("Token_Semicolon");          break;
        case Token_TripleDash:         result = CONST_STRING("Token_TripleDash");         break;
        case Token_Alpha:              result = CONST_STRING("Token_Alpha");              break;
        case Token_Hash:               result = CONST_STRING("Token_Hash");               break;
        case Token_Keyword:            result = CONST_STRING("Token_Keyword");            break;
        INVALID_DEFAULT_CASE;
        
    }
    
    return result;
}

inline void
Refill(Lexer* lexer)
{
    lexer->peek[0] = (lexer->input.size != 0 ? lexer->input.data[0] : 0);
    lexer->peek[1] = (lexer->input.size >= 2 ? lexer->input.data[1] : 0);
}

inline void
Advance(Lexer* lexer, U32 amount)
{
    lexer->input.data += amount;
    lexer->input.size -= MIN(amount, lexer->input.size);
    
    Refill(lexer);
}

inline Lexer
LexString(String input)
{
    Lexer lexer = {};
    
    lexer.input = input;
    Refill(&lexer);
    
    return lexer;
}

inline Token
GetToken(Lexer* lexer)
{
    Token token = {};
    token.type  = Token_Error;
    
    for (;;)
    {
        while (IsSpacing(lexer->peek[0]))
        {
            Advance(lexer, 1);
        }
        
        if (lexer->input.size > 1)
        {
            if (lexer->peek[0] == '/' && lexer->peek[1] == '/')
            {
                while (lexer->input.size && !IsEndOfLine(lexer->peek[0]))
                {
                    Advance(lexer, 1);
                }
                
                if (lexer->input.size)
                {
                    Advance(lexer, 1);
                }
            }
            
            else if (lexer->peek[0] == '/' && lexer->peek[1] == '*')
            {
                Advance(lexer, 2);
                
                for (U32 nesting_level = 1; lexer->input.size && nesting_level != 0; )
                {
                    if (lexer->peek[0] == '*' && lexer->peek[1] == '/')
                    {
                        --nesting_level;
                        Advance(lexer, 2);
                        continue;
                    }
                    
                    else if (lexer->peek[0] == '/' && lexer->peek[1] == '*')
                    {
                        ++nesting_level;
                        Advance(lexer, 2);
                        continue;
                    }
                    
                    Advance(lexer, 1);
                }
            }
            
            else break;
        }
        
        else break;
    }
    
    token.string      = {};
    token.string.data = lexer->input.data;
    
    char c = lexer->peek[0];
    Advance(lexer, 1);
    
    switch (c)
    {
        case 0: token.type = Token_EndOfStream; break;
        
        case '?': token.type = Token_Questionmark; break;
        case ',': token.type = Token_Comma;        break;
        
        case '(': token.type = Token_OpenParen;    break;
        case ')': token.type = Token_CloseParen;   break;
        case '{': token.type = Token_OpenBrace;    break;
        case '}': token.type = Token_CloseBrace;   break;
        case '[': token.type = Token_OpenBracket;  break;
        case ']': token.type = Token_CloseBracket; break;
        
        case '@': token.type = Token_Alpha;        break;
        case '#': token.type = Token_Hash;         break;
        
        case ';': token.type = Token_Semicolon;    break;
        
        case '~': token.type = Token_Not;          break;
        
        case ':':
        {
            if (lexer->peek[0] == ':')
            {
                token.type = Token_DoubleColon;
                Advance(lexer, 1);
            }
            
            else
            {
                token.type = Token_Colon;
            }
        } break;
        
        case '.':
        {
            if (lexer->peek[0] == '.')
            {
                token.type = Token_Elipsis;
                Advance(lexer, 1);
            }
            
            else
            {
                token.type = Token_Dot;
            }
        } break;
        
        case '>':
        case '<':
        {
            if (lexer->peek[0] == c)
            {
                if (lexer->peek[1] == '=')
                {
                    if (lexer->peek[0] == '>') token.type = Token_RightShiftEquals;
                    else                       token.type = Token_LeftShiftEquals;
                    
                    Advance(lexer, 2);
                }
                
                else
                {
                    if (lexer->peek[0] == '>') token.type = Token_RightShift;
                    else                       token.type = Token_LeftShift;
                    
                    Advance(lexer, 1);
                }
            }
            
            else
            {
                if (lexer->peek[0] == '=')
                {
                    if (lexer->peek[0] == '>') token.type = Token_GreaterThanOrEqual;
                    else                       token.type = Token_LessThanOrEqual;
                    
                    Advance(lexer, 1);
                }
                
                else
                {
                    if (lexer->peek[0] == '>') token.type = Token_GreaterThan;
                    else                       token.type = Token_LessThan;
                }
            }
        } break;
        
        case '-':
        {
            if (lexer->peek[0] == '>')
            {
                token.type = Token_Arrow;
                Advance(lexer, 1);
                break;
            }
            
            else if (lexer->peek[0] == '-' && lexer->peek[1] == '-')
            {
                token.type = Token_TripleDash;
                Advance(lexer, 2);
                break;
            }
            
            else
            {
                // Fallthrough
            }
        }
        
        case '+':
        case '&':
        case '|':
        {
            if (lexer->peek[0] == '=')
            {
                if (c == '+')      token.type = Token_PlusEquals;
                else if (c == '-') token.type = Token_MinusEquals;
                else if (c == '&') token.type = Token_AndEquals;
                else               token.type = Token_OrEquals;
                
                Advance(lexer, 1);
            }
            
            else if (lexer->peek[0] == c)
            {
                if (c == '+')      token.type = Token_Increment;
                else if (c == '-') token.type = Token_Decrement;
                else if (c == '&') token.type = Token_LogicalAnd;
                else               token.type = Token_LogicalOr;
                
                Advance(lexer, 1);
            }
            
            else
            {
                if (c == '+')      token.type = Token_Plus;
                else if (c == '-') token.type = Token_Minus;
                else if (c == '&') token.type = Token_Ampersand;
                else               token.type = Token_Or;
            }
        } break;
        
        case '*':
        case '/':
        case '%':
        case '^':
        case '=':
        case '!':
        {
            if (lexer->peek[0] == '=')
            {
                if (c == '*')      token.type = Token_MultiplyEquals;
                else if (c == '/') token.type = Token_DivideEquals;
                else if (c == '%') token.type = Token_ModuloEquals;
                else if (c == '^') token.type = Token_XOREquals;
                else if (c == '=') token.type = Token_IsEqual;
                else               token.type = Token_IsNotEqual;
                
                Advance(lexer, 1);
            }
            
            else
            {
                if (c == '*')      token.type = Token_Asterisk;
                else if (c == '/') token.type = Token_Divide;
                else if (c == '%') token.type = Token_Modulo;
                else if (c == '^') token.type = Token_XOR;
                else if (c == '=') token.type = Token_Equals;
                else               token.type = Token_LogicalNot;
            }
        } break;
        
        default:
        {
            if (IsAlpha(c) || c == '_')
            {
                token.type = Token_Identifier;
                
                while (IsAlpha(lexer->peek[0]) || IsNumeric(lexer->peek[0]) || lexer->peek[0] == '_')
                {
                    Advance(lexer, 1);
                }
                
                token.string.size = lexer->input.data - token.string.data;
                
                if (StringCompare(token.string, CONST_STRING("if")))
                {
                    token.type = Token_Keyword;
                    token.keyword = Keyword_If;
                }
                
                else if (StringCompare(token.string, CONST_STRING("else")))
                {
                    token.type = Token_Keyword;
                    token.keyword = Keyword_Else;
                }
                
                else if (StringCompare(token.string, CONST_STRING("while")))
                {
                    token.type = Token_Keyword;
                    token.keyword = Keyword_While;
                }
                
                else if (StringCompare(token.string, CONST_STRING("defer")))
                {
                    token.type = Token_Keyword;
                    token.keyword = Keyword_Defer;
                }
                
                else if (StringCompare(token.string, CONST_STRING("return")))
                {
                    token.type = Token_Keyword;
                    token.keyword = Keyword_Return;
                }
                
                else if (StringCompare(token.string, CONST_STRING("cast")))
                {
                    token.type = Token_Keyword;
                    token.keyword = Keyword_Cast;
                }
                
                else if (StringCompare(token.string, CONST_STRING("struct")))
                {
                    token.type = Token_Keyword;
                    token.keyword = Keyword_Struct;
                }
                
                else if (StringCompare(token.string, CONST_STRING("enum")))
                {
                    token.type = Token_Keyword;
                    token.keyword = Keyword_Enum;
                }
                
                else if (StringCompare(token.string, CONST_STRING("proc")))
                {
                    token.type = Token_Keyword;
                    token.keyword = Keyword_Proc;
                }
            }
            
            else if (IsNumeric(c))
            {
                // IMPORTANT TODO(soimn): This routine is not robust and does not handle overflow, invalid exponent or invalid number length
                
                token.type = Token_Number;
                
                F64 acc = c - '0';
                I32 exp = 0;
                
                bool has_decimal_point = false;
                
                while (IsNumeric(lexer->peek[0]))
                {
                    acc *= 10;
                    acc += lexer->peek[0] - '0';
                    Advance(lexer, 1);
                }
                
                if (lexer->peek[0] == '.')
                {
                    has_decimal_point = true;
                    
                    Advance(lexer, 1);
                    
                    while (IsNumeric(lexer->peek[0]))
                    {
                        acc *= 10;
                        acc += lexer->peek[0] - '0';
                        exp -= 1;
                        Advance(lexer, 1);
                    }
                }
                
                if (ToLower(lexer->peek[0]) == 'e')
                {
                    Advance(lexer, 1);
                    
                    I32 post_exp     = 0;
                    I8 post_exp_sign = 1;
                    
                    if (lexer->peek[0] == '-' || lexer->peek[0] == '+')
                    {
                        post_exp_sign = (lexer->peek[0] == '-' ? -1 : 1);
                        Advance(lexer, 1);
                    }
                    
                    while (IsNumeric(lexer->peek[0]))
                    {
                        post_exp *= 10;
                        post_exp += lexer->peek[0] - '0';
                        Advance(lexer, 1);
                    }
                    
                    exp += post_exp * post_exp_sign;
                }
                
                if (exp < 0)
                {
                    exp = -exp;
                    for (I32 i = 0; i < exp; ++i)
                    {
                        acc /= 10;
                    }
                }
                
                else
                {
                    for (I32 i = 0; i < exp; ++i)
                    {
                        acc *= 10;
                    }
                }
                
                if (lexer->peek[0] == 'f')
                {
                    Advance(lexer, 1);
                    
                    token.number.is_single_precision = true;
                    token.number.f32 = (F32)acc;
                }
                
                else if (has_decimal_point)
                {
                    token.number.f64 = acc;
                }
                
                else
                {
                    if (acc <= U64_MAX)
                    {
                        token.number.is_integer = true;
                        token.number.u64 = (U64)acc;
                    }
                    
                    else
                    {
                        //// ERROR: Integer constant too out of range
                        token.type = Token_Error;
                    }
                }
            }
            
            else if (c == '"')
            {
                token.type = Token_String;
                
                while (lexer->peek[0] != 0 && lexer->peek[0] != '"')
                {
                    bool escaped_char = (lexer->peek[0] == '\\' && lexer->peek[1] == '"');
                    Advance(lexer, (escaped_char ? 2 : 1));
                }
                
                if (lexer->peek[0] != 0)
                {
                    token.string.size = lexer->input.data - token.string.data;
                    Advance(lexer, 1);
                }
                
                else
                {
                    //// ERROR: End of stream reached before encountering a terminating '"'  character in string
                    token.type = Token_Error;
                }
                
            }
            
            else
            {
                token.type = Token_Error;
            }
        } break;
    }
    
    if (token.string.size == 0)
    {
        token.string.size = lexer->input.data - token.string.data;
    }
    
    return token;
}

inline Token
PeekToken(Lexer* lexer, U32 step = 1)
{
    Lexer temp_lexer = *lexer;
    Token token = {};
    
    for (U32 i = 0; i < step; ++i)
        token = GetToken(&temp_lexer);
    
    return token;
}

inline void
SkipToken(Lexer* lexer)
{
    GetToken(lexer);
}

inline bool
MetRequiredToken(Lexer* lexer, Enum32(TOKEN_TYPE) type)
{
    Token token = PeekToken(lexer);
    
    bool result = (token.type == type);
    
    if (result)
    {
        GetToken(lexer);
    }
    
    return result;
}