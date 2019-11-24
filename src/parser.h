#pragma once

#include "common.h"
#include "memory.h"
#include "lexer.h"

inline bool
ParsePostfixExpression(Lexer* lexer)
{
    NOT_IMPLEMENTED;
}

inline bool
ParseCastExpression(Lexer* lexer);

inline bool
ParseUnaryExpression(Lexer* lexer)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(lexer);
    
    if (token.type == Token_Increment)
    {
        SkipToken(lexer);
        
        encountered_errors = ParseUnaryExpression(lexer);
    }
    
    else if (token.type == Token_Decrement)
    {
        SkipToken(lexer);
        
        encountered_errors = ParseUnaryExpression(lexer);
    }
    
    else if (token.type == Token_Ampersand)
    {
        SkipToken(lexer);
        
        encountered_errors = ParseCastExpression(lexer);
    }
    
    else if (token.type == Token_Asterisk)
    {
        SkipToken(lexer);
        
        encountered_errors = ParseCastExpression(lexer);
    }
    
    else if (token.type == Token_Plus)
    {
        SkipToken(lexer);
        
        encountered_errors = ParseCastExpression(lexer);
    }
    
    else if (token.type == Token_Minus)
    {
        SkipToken(lexer);
        
        encountered_errors = ParseCastExpression(lexer);
    }
    
    else if (token.type == Token_Not)
    {
        SkipToken(lexer);
        
        encountered_errors = ParseCastExpression(lexer);
    }
    
    else if (token.type == Token_LogicalNot)
    {
        SkipToken(lexer);
        
        encountered_errors = ParseCastExpression(lexer);
    }
    
    return encountered_errors;
}

inline bool
ParseCastExpression(Lexer* lexer)
{
    NOT_IMPLEMENTED;
}

inline bool
ParseMultiplicativeExpression(Lexer* lexer)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseCastExpression(lexer);
    
    if (!encountered_errors)
    {
        Token token = PeekToken(lexer);
        
        if (token.type == Token_Asterisk)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseMultiplicativeExpression(lexer);
        }
        
        else if (token.type == Token_Divide)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseMultiplicativeExpression(lexer);
        }
        
        else if (token.type == Token_Modulo)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseMultiplicativeExpression(lexer);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseAdditiveExpression(Lexer* lexer)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseMultiplicativeExpression(lexer);
    
    if (!encountered_errors)
    {
        Token token = PeekToken(lexer);
        
        if (token.type == Token_Plus)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseAdditiveExpression(lexer);
        }
        
        else if (token.type == Token_Minus)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseAdditiveExpression(lexer);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseShiftExpression(Lexer* lexer)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseAdditiveExpression(lexer);
    
    if (!encountered_errors)
    {
        Token token = PeekToken(lexer);
        
        if (token.type == Token_LeftShift)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseShiftExpression(lexer);
        }
        
        else if (token.type == Token_RightShift)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseShiftExpression(lexer);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseRelationalExpression(Lexer* lexer)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseShiftExpression(lexer);
    
    if (!encountered_errors)
    {
        Token token = PeekToken(lexer);
        
        if (token.type == Token_LessThan)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseRelationalExpression(lexer);
        }
        
        else if (token.type == Token_GreaterThan)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseRelationalExpression(lexer);
        }
        
        else if (token.type == Token_LessThanOrEqual)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseRelationalExpression(lexer);
        }
        
        else if (token.type == Token_GreaterThanOrEqual)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseRelationalExpression(lexer);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseEqualityExpression(Lexer* lexer)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseRelationalExpression(lexer);
    
    if (!encountered_errors)
    {
        Token token = PeekToken(lexer);
        
        if (token.type == Token_IsEqual)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseEqualityExpression(lexer);
        }
        
        else if (token.type == Token_IsNotEqual)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseEqualityExpression(lexer);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseAndExpression(Lexer* lexer)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseEqualityExpression(lexer);
    
    if (!encountered_errors)
    {
        if (PeekToken(lexer).type == Token_Ampersand)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseAndExpression(lexer);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseExclusiveOrExpression(Lexer* lexer)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseAndExpression(lexer);
    
    if (!encountered_errors)
    {
        if (PeekToken(lexer).type == Token_XOR)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseExclusiveOrExpression(lexer);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseInclusiveOrExpression(Lexer* lexer)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseExclusiveOrExpression(lexer);
    
    if (!encountered_errors)
    {
        if (PeekToken(lexer).type == Token_Or)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseInclusiveOrExpression(lexer);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseLogicalAndExpression(Lexer* lexer)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseInclusiveOrExpression(lexer);
    
    if (!encountered_errors)
    {
        if (PeekToken(lexer).type == Token_LogicalAnd)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseLogicalAndExpression(lexer);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseLogicalOrExpression(Lexer* lexer)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseLogicalAndExpression(lexer);
    
    if (!encountered_errors)
    {
        if (PeekToken(lexer).type == Token_LogicalOr)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseLogicalOrExpression(lexer);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseExpression(Lexer* lexer);

inline bool
ParseConditionalExpression(Lexer* lexer)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseLogicalOrExpression(lexer);
    
    if (!encountered_errors)
    {
        if (PeekToken(lexer).type == Token_Questionmark)
        {
            SkipToken(lexer);
            
            encountered_errors = ParseExpression(lexer);
            
            if (!encountered_errors)
            {
                if (RequireToken(lexer, Token_Colon))
                {
                    encountered_errors = ParseConditionalExpression(lexer);
                }
                
                else
                {
                    //// ERROR: Missing else clause in ternary
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Invalid expression in ternary
            }
        }
    }
    
    else
    {
        //// ERROR: Invalid conditional expression
    }
    
    return encountered_errors;
}

inline bool
ParseAssignmentExpression(Lexer* lexer)
{
    NOT_IMPLEMENTED;
}

inline bool
ParseExpression(Lexer* lexer)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(lexer);
    
    if (token.type != Token_Comma && token.type != Token_Semicolon)
    {
        encountered_errors = ParseAssignmentExpression(lexer);
        
        if (!encountered_errors)
        {
            if (RequireToken(lexer, Token_Comma))
            {
                encountered_errors = ParseExpression(lexer);
            }
        }
    }
    
    else
    {
        //// ERROR: Expected expression
        encountered_errors = true;
    }
    
    return encountered_errors;
}

inline bool
ParseFunction(Lexer* lexer);

inline bool
ParseVariableDeclaration(Lexer* lexer);

inline bool
ParseConstantDeclaration(Lexer* lexer);

// NOTE(soimn): This parses eveything that is legal in a function body
inline bool
ParseStatement(Lexer* lexer)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(lexer);
    
    if (token.type == Token_Identifier)
    {
        Token peek = PeekToken(lexer, 2);
        
        if (peek.type == Token_Colon)
        {
            encountered_errors = ParseVariableDeclaration(lexer);
        }
        
        else if (peek.type == Token_DoubleColon)
        {
            String name = token.string;
            SkipToken(lexer);
            SkipToken(lexer);
            
            if (PeekToken(lexer, 3).type == Token_OpenParen)
            {
                Lexer tmp_lexer = *lexer;
                encountered_errors = ParseFunction(&tmp_lexer);
                
                if (!encountered_errors)
                {
                    *lexer = tmp_lexer;
                }
                
                else
                {
                    encountered_errors = ParseConstantDeclaration(lexer);
                    
                    if (encountered_errors)
                    {
                        //// ERROR: Did not math a valid function or constant declaration
                    }
                }
            }
            
            else
            {
                encountered_errors = ParseConstantDeclaration(lexer);
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("if")))
        {
            if (RequireToken(lexer, Token_OpenParen))
            {
                encountered_errors = ParseExpression(lexer) || !RequireToken(lexer, Token_CloseParen);
                
                if (!encountered_errors)
                {
                    encountered_errors = ParseStatement(lexer);
                    
                    peek = PeekToken(lexer);
                    if (!encountered_errors && peek.type == Token_Identifier && StringCompare(peek.string, CONST_STRING("else")))
                    {
                        SkipToken(lexer);
                        
                        encountered_errors = ParseStatement(lexer);
                    }
                }
            }
            
            else
                //// ERROR: Missing open paren after keyword "if"
            {
                encountered_errors = true;
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("while")))
        {
            if (RequireToken(lexer, Token_OpenParen))
            {
                encountered_errors = ParseExpression(lexer) || !RequireToken(lexer, Token_CloseParen);
                
                if (!encountered_errors)
                {
                    encountered_errors = ParseStatement(lexer);
                }
            }
            
            else
            {
                //// ERROR: Missing open paren after keyword "while"
                encountered_errors = true;
            }
        }
        
        // TODO(soimn): For, do, do while, return, defer, ...
        
        else
        {
            encountered_errors = ParseExpression(lexer) || !RequireToken(lexer, Token_Semicolon);
        }
    }
    
    else if (token.type == Token_Semicolon)
    {
        SkipToken(lexer);
    }
    
    else
    {
        encountered_errors = ParseExpression(lexer) || !RequireToken(lexer, Token_Semicolon);
    }
    
    return encountered_errors;
}

// NOTE(soimn): This parses something of the form '{' LIST_OF_STATEMENTS '}'
inline bool
ParseCompoundStatement(Lexer* lexer)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(lexer);
    while (!encountered_errors && token.type != Token_CloseBrace)
    {
        if (token.type == Token_OpenBrace)
        {
            encountered_errors = ParseCompoundStatement(lexer);
        }
        
        else
        {
            encountered_errors = ParseStatement(lexer);
        }
        
        token = PeekToken(lexer);
    }
    
    if (!encountered_errors)
    {
        SkipToken(lexer);
    }
    
    return encountered_errors;
}

inline bool
ParseFunctionHeader(Lexer* lexer, bool require_return_type, bool allow_default_values);

inline bool
ParseType(Lexer* lexer)
{
    bool encountered_errors = false;
    
    Token token = GetToken(lexer);
    
    if (token.type != Token_OpenParen)
    {
        for (;;)
        {
            if (token.type == Token_Ampersand)
            {
                // ...
            }
            
            else if (token.type == Token_OpenBracket)
            {
                encountered_errors = ParseExpression(lexer) || !RequireToken(lexer, Token_CloseBracket);
                // ...
            }
            
            else if (token.type == Token_Identifier)
            {
                break;
            }
            
            else
            {
                //// ERROR: Weird shit in type
                encountered_errors = true;
            }
            
            token = GetToken(lexer);
        }
    }
    
    else
    {
        encountered_errors = ParseFunctionHeader(lexer, true, false);
    }
    
    return encountered_errors;
}

// NOTE(soimn): Called on: IDENT "::" "struct"
inline bool
ParseStructDeclaration(Lexer* lexer)
{
    bool encountered_errors = false;
    
    Token name = GetToken(lexer);
    SkipToken(lexer);
    SkipToken(lexer);
    encountered_errors = !RequireToken(lexer, Token_OpenBrace);
    
    if (!encountered_errors)
    {
        while (!encountered_errors && PeekToken(lexer).type != Token_CloseBrace)
        {
            Token token = GetToken(lexer);
            
            if (token.type == Token_Identifier)
            {
                String field_name = token.string;
                
                encountered_errors = !RequireToken(lexer, Token_Colon);
                token = GetToken(lexer);
                
                if (!encountered_errors)
                {
                    if (token.type != Token_Equals)
                    {
                        encountered_errors = ParseType(lexer);
                    }
                    
                    if (token.type == Token_Equals)
                    {
                        encountered_errors = ParseExpression(lexer);
                    }
                    
                    encountered_errors = encountered_errors || !RequireToken(lexer, Token_Semicolon);
                }
            }
            
            else
            {
                //// ERROR: Invalid token in struct body
                encountered_errors = true;
            }
        }
    }
    
    return encountered_errors || !RequireToken(lexer, Token_CloseBrace);
}

// NOTE(soimn): Parses (arg_list) -> return_type
inline bool
ParseFunctionHeader(Lexer* lexer, bool require_return_type, bool allow_default_values)
{
    bool encountered_errors = false;
    
    Token token = GetToken(lexer);
    if (token.type == Token_OpenParen)
    {
        token = PeekToken(lexer);
        while (!encountered_errors && token.type != Token_CloseParen)
        {
            if (token.type == Token_Identifier)
            {
                String name = token.string;
                SkipToken(lexer);
                
                if (RequireToken(lexer, Token_Colon))
                {
                    encountered_errors = ParseType(lexer);
                    
                    if (!encountered_errors)
                    {
                        if (PeekToken(lexer).type == Token_Equals)
                        {
                            if (allow_default_values)
                            {
                                SkipToken(lexer);
                                
                                encountered_errors = ParseExpression(lexer);
                            }
                            
                            else
                            {
                                //// ERROR: Default argument values are illegal in this context
                                encountered_errors = true;
                            }
                        }
                    }
                }
                
                else
                {
                    //// ERROR: Expected colon after argument name in function argument list
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Invalid expression in function argument list
                encountered_errors = true;
            }
        }
        
        if (!encountered_errors)
        {
            SkipToken(lexer);
            token = PeekToken(lexer);
            
            if (token.type == Token_Arrow)
            {
                SkipToken(lexer);
                
                encountered_errors = ParseType(lexer);
            }
            
            else if (require_return_type)
            {
                //// ERROR: Missing return type in function header
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR: Missing open paren
        encountered_errors = true;
    }
    
    return encountered_errors;
}

// NOTE(soimn): Parses (arg_list) -> return_type {...}
inline bool
ParseFunction(Lexer* lexer)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseFunctionHeader(lexer, false, true);
    
    if (!encountered_errors)
    {
        if (PeekToken(lexer).type == Token_OpenBrace)
        {
            encountered_errors = ParseCompoundStatement(lexer);
        }
        
        else
        {
            //// ERROR: Missing function body
            encountered_errors = true;
        }
    }
    
    return encountered_errors;
}

// NOTE(soimn): Called on: IDENT ':'
inline bool
ParseVariableDeclaration(Lexer* lexer)
{
    bool encountered_errors = false;
    
    Token name = GetToken(lexer);
    SkipToken(lexer); // TODO(soimn): Should this be an assert instead?
    
    Token token = GetToken(lexer);
    
    if (token.type != Token_Equals)
    {
        encountered_errors = ParseType(lexer);
    }
    
    if (!encountered_errors && token.type == Token_Equals)
    {
        encountered_errors = ParseExpression(lexer);
    }
    
    return encountered_errors || !RequireToken(lexer, Token_Semicolon);
}

// NOTE(soimn): Called on: IDENT "::"
inline bool
ParseConstantDeclaration(Lexer* lexer)
{
    return ParseExpression(lexer) || !RequireToken(lexer, Token_Semicolon);
}

inline bool
ParseExternalDeclaration(Lexer* lexer)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(lexer);
    
    while (!encountered_errors && token.type != Token_EndOfStream)
    {
        if (token.type == Token_Alpha)
        {
            // User level note
            NOT_IMPLEMENTED;
        }
        
        else if (token.type == Token_Hash)
        {
            // Compiler directive
            NOT_IMPLEMENTED;
        }
        
        else if (token.type == Token_Identifier)
        {
            Token peek = PeekToken(lexer, 2);
            if (peek.type == Token_Colon)
            {
                encountered_errors = ParseVariableDeclaration(lexer);
            }
            
            else if (peek.type == Token_DoubleColon)
            {
                peek = PeekToken(lexer, 3);
                if (peek.type == Token_OpenParen)
                {
                    String name = token.string;
                    SkipToken(lexer);
                    SkipToken(lexer);
                    
                    Lexer tmp_lexer = *lexer;
                    encountered_errors = ParseFunction(&tmp_lexer);
                    
                    if (!encountered_errors)
                    {
                        *lexer = tmp_lexer;
                    }
                    
                    else
                    {
                        encountered_errors = ParseConstantDeclaration(lexer);
                        
                        if (encountered_errors)
                        {
                            //// ERROR: Did not match a valid function or constant declaration
                        }
                    }
                }
                
                else if (peek.type == Token_Identifier && StringCompare(peek.string, CONST_STRING("struct")))
                {
                    encountered_errors = ParseStructDeclaration(lexer);
                    
                }
                
                else
                {
                    String name = token.string;
                    SkipToken(lexer);
                    SkipToken(lexer);
                    
                    encountered_errors = ParseConstantDeclaration(lexer);
                }
            }
            
            else
            {
                //// ERROR: Loose identifier, or something
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Invalid token in external declaration
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}