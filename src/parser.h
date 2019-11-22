#pragma once

#include "common.h"
#include "memory.h"
#include "lexer.h"

inline bool
ParseExpression(Lexer* lexer)
{
    NOT_IMPLEMENTED;
}

inline bool
ParseFunctionHeader(Lexer* lexer, bool require_return_type, bool allow_default_values);

inline bool
ParseType(Lexer* lexer)
{
    bool encountered_errors = false;
    
    // TODO(soimn): Should pointers to arrays be legal, how should they be represented?
    // TODO(soimn): Parse array tags
    
    Token token = GetToken(lexer);
    
    if (token.type != Token_OpenParen)
    {
        for (;;)
        {
            if (token.type == Token_Asterisk)
            {
                continue;
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
    GetToken(lexer);
    GetToken(lexer);
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

inline bool
ParseFunctionHeader(Lexer* lexer, bool require_return_type, bool allow_default_values)
{
    NOT_IMPLEMENTED;
}

inline bool
ParseFunction(Lexer* lexer)
{
    NOT_IMPLEMENTED;
}

// NOTE(soimn): Called on: IDENT ':'
inline bool
ParseVariableDeclaration(Lexer* lexer)
{
    bool encountered_errors = false;
    
    Token name = GetToken(lexer);
    GetToken(lexer);
    
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
    Token name = GetToken(lexer);
    GetToken(lexer);
    
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
                    encountered_errors = ParseFunction(lexer);
                }
                
                else if (token.type != Token_Identifier)
                {
                    encountered_errors = ParseConstantDeclaration(lexer);
                }
                
                else if (StringCompare(peek.string, CONST_STRING("struct")))
                {
                    encountered_errors = ParseStructDeclaration(lexer);
                    
                }
                
                else
                {
                    //// ERROR: Invalid token in external declaration succeding ::
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