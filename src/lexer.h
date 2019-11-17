#pragma once

#include "common.h"

// ASSUMPTION(soimn): It is assumed that all whitespace and comments should be ignored
// TODO(soimn): Find out how to track the current module

struct Lexer
{
    String input;
    
    U32 module_id;
    U32 line;
    U32 column;
    
    char peek[2];
};

enum TOKEN_TYPE
{
    Token_Unknown = 0,
    Token_Error,
    Token_EndOfStream,
    
    Token_Identifier,
    Token_String,
    
    /// Arithmetic
    Token_Plus,
    Token_PlusEquals,
    Token_Increment,
    Token_Minus,
    Token_MinusEquals,
    Token_Decrement,
    Token_Asterisk, // multiply or dereference
    Token_MultiplyEquals,
    Token_Divide,
    Token_DivideEquals,
    Token_Modulo,
    Token_ModuloEquals,
    
    /// Bitwise operators
    Token_Ampersand, // and or reference
    Token_AndEquals,
    Token_Or,
    Token_OrEquals,
    Token_XOR,
    Token_XOREquals,
    Token_Not,
    Token_NotEquals,
    
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
    Token_Colon,
    
    /// Bitwise shift
    Token_RightShift,
    Token_RightShiftEquals,
    Token_LeftShift,
    Token_LeftShiftEquals,
    
    Token_Equals,
    Token_Dot,
    Token_Elipsis,
    Token_Comma,
    
    Token_OpenParen,
    Token_CloseParen,
    Token_OpenBrace,
    Token_CloseBrace,
    Token_OpenBracket,
    Token_CloseBracket,
    
    TOKEN_TYPE_COUNT
};

struct Token
{
    Enum32(TOKEN_TYPE) type;
    
    U32 module_id;
    U32 line;
    U32 column;
    
    String string;
};

inline String
GetNameOfTokenType(Enum32(TOKEN_TYPE) type)
{
    String result = {};
    
    switch (type)
    {
        case Token_Unknown:            result = CONST_STRING("Token_Unknown");            break;
        case Token_EndOfStream:        result = CONST_STRING("Token_EndOfStream");        break;
        case Token_Identifier:         result = CONST_STRING("Token_Identifier");         break;
        case Token_String:             result = CONST_STRING("Token_String");             break;
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
        case Token_NotEquals:          result = CONST_STRING("Token_NotEquals");          break;
        case Token_LogicalAnd:         result = CONST_STRING("Token_LogicalAnd");         break;
        case Token_LogicalOr:          result = CONST_STRING("Token_LogicalOr");          break;
        case Token_LogicalNot:         result = CONST_STRING("Token_LogicalNot");         break;
        case Token_IsEqual:            result = CONST_STRING("Token_IsEqual");            break;
        case Token_IsNotEqual:         result = CONST_STRING("Token_IsNotEqual");         break;
        case Token_GreaterThan:        result = CONST_STRING("Token_GreaterThan");        break;
        case Token_GreaterThanOrEqual: result = CONST_STRING("Token_GreaterThanOrEqual"); break;
        case Token_LessThan:           result = CONST_STRING("Token_LessThan");           break;
        case Token_LessThanOrEqual:    result = CONST_STRING("Token_LessThanOrEqual");    break;
        case Token_Questionmark:       result = CONST_STRING("Token_Questionmark");       break;
        case Token_Colon:              result = CONST_STRING("Token_Colon");              break;
        case Token_RightShift:         result = CONST_STRING("Token_RightShift");         break;
        case Token_RightShiftEquals:   result = CONST_STRING("Token_RightShiftEquals");   break;
        case Token_LeftShift:          result = CONST_STRING("Token_LeftShift");          break;
        case Token_LeftShiftEquals:    result = CONST_STRING("Token_LeftShiftEquals");    break;
        case Token_Equals:             result = CONST_STRING("Token_Equals");             break;
        case Token_Dot:                result = CONST_STRING("Token_Dot");                break;
        case Token_Elipsis:            result = CONST_STRING("Token_Elipsis");            break;
        case Token_Comma:              result = CONST_STRING("Token_Comma");              break;
        case Token_OpenParen:          result = CONST_STRING("Token_OpenParen");          break;
        case Token_CloseParen:         result = CONST_STRING("Token_CloseParen");         break;
        case Token_OpenBrace:          result = CONST_STRING("Token_OpenBrace");          break;
        case Token_CloseBrace:         result = CONST_STRING("Token_CloseBrace");         break;
        case Token_OpenBracket:        result = CONST_STRING("Token_OpenBracket");        break;
        case Token_CloseBracket:       result = CONST_STRING("Token_CloseBracket");       break;
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
    lexer->input.size -= amount;
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

inline void
EatAllCommentsAndWhitespace(String* string)
{
    for (;;)
    {
        while (string->size && IsSpacing(*string->data))
        {
            ++string->data;
            --string->size;
        }
        
        if (string->size > 1)
        {
            if (string->data[0] == '/' && string->data[1] == '/')
            {
                while (string->size && !IsEndOfLine(*string->data))
                {
                    ++string->data;
                    --string->size;
                }
                
                if (string->size)
                {
                    ++string->data;
                    --string->size;
                }
            }
            
            else if (string->data[0] == '/' && string->data[1] == '*')
            {
                while (string->size > 1 && !(string->data[0] == '*' && string->data[1] == '/'))
                {
                    ++string->data;
                    --string->size;
                }
                
                if (string->size > 1)
                {
                    string->data += 2;
                    string->size -= 2;
                }
                
                else
                {
                    string->size = 0;
                }
            }
            
            else break;
        }
        
        else break;
    }
}

inline Token
GetToken(Lexer* lexer)
{
    Token token = {};
    token.type  = Token_Unknown;
    
    EatAllCommentsAndWhitespace(&lexer->input);
    Refill(lexer);
    
    token.string      = {};
    token.string.data = lexer->input.data;
    
    char c = lexer->peek[0];
    Advance(lexer, 1);
    
    switch (c)
    {
        case 0: token.type = Token_EndOfStream; break;
        
        case '?': token.type = Token_Questionmark; break;
        case ':': token.type = Token_Colon;        break;
        case ',': token.type = Token_Dot;          break;
        
        case '(': token.type = Token_OpenParen;    break;
        case ')': token.type = Token_CloseParen;   break;
        case '{': token.type = Token_OpenBrace;    break;
        case '}': token.type = Token_CloseBrace;   break;
        case '[': token.type = Token_OpenBracket;  break;
        case ']': token.type = Token_CloseBracket; break;
        
        case '.':
        {
            if (lexer->peek[0] == '.' && lexer->peek[1] == '.')
            {
                token.type = Token_Elipsis;
                Advance(lexer, 2);
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
        
        case '+':
        case '-':
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
        case '~':
        case '=':
        case '!':
        {
            if (lexer->peek[0] == '=')
            {
                if (c == '*')      token.type = Token_MultiplyEquals;
                else if (c == '/') token.type = Token_DivideEquals;
                else if (c == '%') token.type = Token_ModuloEquals;
                else if (c == '^') token.type = Token_XOREquals;
                else if (c == '~') token.type = Token_NotEquals;
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
                else if (c == '~') token.type = Token_Not;
                else if (c == '=') token.type = Token_Equals;
                else               token.type = Token_LogicalNot;
            }
        } break;
        
        default:
        {
            if (IsAlpha(c) || c == '_')
            {
                token.type = Token_Identifier;
                
                while (IsAlpha(lexer->peek[0]) || IsNumeric(lexer->peek[0]) || c == '_')
                {
                    Advance(lexer, 1);
                }
                
                token.string.size = lexer->input.data - token.string.data;
            }
            
            else if (IsNumeric(c))
            {
                // TODO(soimn): Find out how numbers should be handled
                NOT_IMPLEMENTED;
            }
            
            else if (c == '"')
            {
                token.type = Token_String;
                
                while (lexer->peek[0] != 0 && lexer->peek[0] != '"')
                {
                    Advance(lexer, 1);
                }
                
                if (lexer->peek[0] != 0)
                {
                    token.string.size = lexer->input.data - token.string.data;
                    Advance(lexer, 1);
                }
                
                else
                {
                    //// ERROR: End of stream reached before encountering a terminating "  character in string
                    token.type = Token_Error;
                }
                
            }
            
            else
            {
                token.type = Token_Unknown;
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
PeekToken(Lexer* lexer)
{
    Lexer temp_lexer = *lexer;
    return GetToken(&temp_lexer);
}