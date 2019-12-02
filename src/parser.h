#pragma once

// IMPORTANT TODO(soimn): Refactor all while token != loops and take Token_EndOfStream into account

#include "common.h"
#include "memory.h"
#include "lexer.h"
#include "ast.h"

inline bool
ParseCastExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result);

inline bool
ParseAssignmentExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result);

inline bool
ParseExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result);

inline bool
ParseCompoundStatement(Lexer* lexer, AST_Unit* unit, AST_Statement** result);

inline bool
ParseType(Lexer* lexer, String* type_string);


ParseFunctionHeader(Lexer* lexer, bool require_return_type, bool allow_default_values, AST_Unit* unit, AST_Statement** arguments, String* return_type_string);

inline bool
ParseVariableDeclaration(Lexer* lexer, AST_Unit* unit, AST_Statement** result, bool allow_assignment = true);

inline bool
ParseFunctionCallArguments(Lexer* lexer, AST_Unit* unit, AST_Expression** result)
{
    bool encountered_errors = false;
    
    encountered_errors = !RequireToken(lexer, Token_OpenParen);
    
    Token token = PeekToken(lexer);
    while (!encountered_errors)
    {
        if (token.type == Token_CloseParen)
        {
            SkipToken(lexer);
            break;
        }
        
        else
        {
            encountered_errors = ParseAssignmentExpression(lexer);
            
            token = PeekToken(lexer);
            if (!encountered_errors && token.type != Token_CloseParen)
            {
                encountered_errors = !RequireToken(lexer, Token_Comma);
            }
        }
    }
    
    return encountered_errors;
}

inline bool
ParsePostfixExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(lexer);
    
    if (token.type == Token_Number)
    {
        // NOTE(soimn): numbers dont support postfix operators
        Token number = GetToken(lexer);
    }
    
    else if (token.type == Token_String)
    {
        // NOTE(soimn): strings dont support postfix operators
        Token string = GetToken(lexer);
    }
    
    else
    {
        while (!encountered_errors)
        {
            if (token.type == Token_Identifier)
            {
                SkipToken(lexer);
            }
            
            else if (token.type == Token_OpenParen)
            {
                encountered_errors = ParseFunctionCallArguments(lexer);
            }
            
            else if (token.type == Token_OpenBracket)
            {
                SkipToken(lexer);
                
                encountered_errors = ParseExpression(lexer) || !RequireToken(lexer, Token_CloseBracket);
            }
            
            else if (token.type == Token_Dot)
            {
                SkipToken(lexer);
            }
            
            else if (token.type == Token_Increment)
            {
                SkipToken(lexer);
            }
            
            else if (token.type == Token_Decrement)
            {
                SkipToken(lexer);
            }
            
            else if (token.type == Token_Error)
            {
                encountered_errors = true;
            }
            
            else break;
            
            token = PeekToken(lexer);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseUnaryExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result)
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
    
    else
    {
        encountered_errors = ParsePostfixExpression(lexer);
    }
    
    return encountered_errors;
}

inline bool
ParseCastExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result)
{
    bool encountered_errors = false;
    
    if (PeekToken(lexer).type == Token_OpenParen)
    {
        Lexer init_lexer_state = *lexer;
        
        encountered_errors = ParseFunctionHeader(lexer, unit, result);
        
        if (!encountered_errors)
        {
            if (PeekToken(lexer).type == Token_OpenBrace)
            {
                encountered_errors = ParseCompoundStatement(lexer, unit, );
            }
            
            else
            {
                //// ERROR: Missing function body
                encountered_errors = true;
            }
        }
        
        else
        {
            *lexer = init_lexer_state;
            DeleteExpression(*result);
            *result = 0;
            
            SkipToken(lexer);
            
            encountered_errors = ParseType(lexer);
            
            if (!encountered_errors && RequireToken(lexer, Token_CloseParen))
            {
                if (PeekToken(&init_lexer_state, 1).type == Token_Identifier && PeekToken(&init_lexer_state, 2).type == Token_CloseParen)
                {
                    // NOTE(soimn): This is either a cast expression or an identifier inside parens
                }
                
                else
                {
                    // NOTE(soimn): This is a cast expression
                }
                
                encountered_errors = ParseCastExpression(lexer);
                
            }
            
            else
            {
                // NOTE(soimn): This is either a unary expression or an invalid cast expression
                
                *lexer = init_lexer_state;
                encountered_errors = ParseUnaryExpression(lexer);
            }
        }
    }
    
    else
    {
        encountered_errors = ParseUnaryExpression(lexer, unit, result);
    }
    
    return encountered_errors;
}

inline bool
ParseMultiplicativeExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseCastExpression(lexer, unit, result);
    
    if (!encountered_errors)
    {
        Token token = PeekToken(lexer);
        
        if (token.type == Token_Multiply || token.type == Token_Divide || token.type == Token_Modulo)
        {
            AST_Expression* left = *result;
            
            Enum32(AST_EXPRESSION_TYPE) type = ASTExp_Modulus;
            
            if (token.type == Token_Multiply)
            {
                type = ASTExp_Multiply;
            }
            
            else if (token.type == Token_Divide)
            {
                type = ASTExp_Divide;
            }
            
            *result = PushExpression(unit);
            (*result)->type = type;
            (*result)->left = left;
            
            SkipToken(lexer);
            
            encountered_errors = ParseMultiplicativeExpression(lexer, unit, &(*result)->right);
        }
        
    }
    
    return encountered_errors;
}

inline bool
ParseAdditiveExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseMultiplicativeExpression(lexer, unit, result);
    
    if (!encountered_errors)
    {
        Token token = PeekToken(lexer);
        
        if (token.type == Token_Plus || token.type == Token_Minus)
        {
            AST_Expression* left = *result;
            
            *result = PushExpression(unit);
            (*result)->type = (token.type == Token_Plus ? ASTExp_Plus : ASTExp_Minus);
            (*result)->left = left;
            
            SkipToken(lexer);
            
            encountered_errors = ParseAdditiveExpression(lexer, unit, &(*result)->right);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseShiftExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseAdditiveExpression(lexer);
    
    if (!encountered_errors)
    {
        Token token = PeekToken(lexer);
        
        if (token.type == Token_LeftShift || token.type == Token_RightShift)
        {
            AST_Expression* left = *result;
            
            *result = PushExpression(unit);
            (*result)->type = (token.type == Token_LeftShift ? ASTExp_LeftShift : ASTExp_RightShift);
            (*result)->left = left;
            
            SkipToken(lexer);
            
            encountered_errors = ParseShiftExpression(lexer, unit, &(*result)->right);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseRelationalExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseShiftExpression(lexer, unit, result);
    
    if (!encountered_errors)
    {
        Token token = PeekToken(lexer);
        
        if (token.type == Token_LessThan || token.type == Token_GreaterThan || token.type == Token_LessThanOrEqual || token.type == Token_GreaterThanOrEqual)
        {
            AST_Expression* left = *result;
            
            Enum32(AST_EXPRESSION_TYPE) type = ASTExp_IsGreaterThanOrEqual;
            
            if (token.type == Token_LessThan)
            {
                type = ASTExp_IsLessThan;
            }
            
            else if (token.type == Token_GreaterThan)
            {
                type = ASTExp_IsGreaterThan;
            }
            
            else if (token.type == Token_LessThanOrEqual)
            {
                type = ASTExp_IsLessThanOrEqual;
            }
            
            *result = PushExpression(unit);
            (*result)->type = type;
            (*result)->left = left;
            
            SkipToken(lexer);
            
            encountered_errors = ParseRelationalExpression(lexer, unit, &(*result)->right);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseEqualityExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseRelationalExpression(lexer, unit, result);
    
    if (!encountered_errors)
    {
        Token token = PeekToken(lexer);
        
        if (token.type == Token_IsEqual || token.type == Token_IsNotEqual)
        {
            AST_Expression* left = *result;
            
            *result = PushExpression(unit);
            (*result)->type = (token.type == Token_IsEqual ? ASTExp_IsEqual : ASTExp_IsNotEqual);
            (*result)->left = left;
            
            SkipToken(lexer);
            
            encountered_errors = ParseEqualityExpression(lexer, unit, &(*result)->right);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseAndExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseEqualityExpression(lexer, unit, result);
    
    if (!encountered_errors)
    {
        if (PeekToken(lexer).type == Token_Ampersand)
        {
            AST_Expression* left = *result;
            
            *result = PushExpression(unit);
            (*result)->type = ASTExp_And;
            (*result)->left = left;
            
            SkipToken(lexer);
            
            encountered_errors = ParseAndExpression(lexer, unit, &(*result)->right);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseExclusiveOrExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseAndExpression(lexer, unit, result);
    
    if (!encountered_errors)
    {
        if (PeekToken(lexer).type == Token_XOR)
        {
            AST_Expression* left = *result;
            
            *result = PushExpression(unit);
            (*result)->type = ASTExp_XOR;
            (*result)->left = left;
            
            SkipToken(lexer);
            
            encountered_errors = ParseExclusiveOrExpression(lexer, unit, &(*result)->type);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseInclusiveOrExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseExclusiveOrExpression(lexer, unit, result);
    
    if (!encountered_errors)
    {
        if (PeekToken(lexer).type == Token_Or)
        {
            AST_Expression* left = *result;
            
            *result = PushExpression(unit);
            (*result)->type = ASTExp_Or;
            (*result)->left = left;
            
            SkipToken(lexer);
            
            encountered_errors = ParseInclusiveOrExpression(lexer, unit, &(*result)->right);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseLogicalAndExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result, bool* is_conditional_expression)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseInclusiveOrExpression(lexer, unit, result);
    
    if (!encountered_errors)
    {
        if (PeekToken(lexer).type == Token_LogicalAnd)
        {
            if (is_conditional_expression)
            {
                *is_conditional_expression = true;
            }
            
            AST_Expression* left = *result;
            
            *result = PushExpression(unit);
            (*result)->type = ASTExp_LogicalAnd;
            (*result)->left = left;
            
            SkipToken(lexer);
            
            encountered_errors = ParseLogicalAndExpression(lexer, unit, &(*result)->right, 0);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseLogicalOrExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result, bool* is_conditional_expression)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseLogicalAndExpression(lexer, unit, result, is_conditional_expression);
    
    if (!encountered_errors)
    {
        if (PeekToken(lexer).type == Token_LogicalOr)
        {
            if (is_conditional_expression)
            {
                *is_conditional_expression = true;
            }
            
            AST_Expression* left = *result;
            
            *result = PushExpression(unit);
            (*result)->type = ASTExp_LogicalOr;
            (*result)->left = left;
            
            SkipToken(lexer);
            
            encountered_errors = ParseLogicalOrExpression(lexer, unit, &(*result)->right, 0);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseConditionalExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result, bool* is_conditional_expression)
{
    bool encountered_errors = false;
    
    encountered_errors = ParseLogicalOrExpression(lexer, unit, result, is_conditional_expression);
    
    if (!encountered_errors)
    {
        if (PeekToken(lexer).type == Token_Questionmark)
        {
            if (is_conditional_expression)
            {
                *is_conditional_expression = true;
            }
            
            AST_Expression* condition = *result;
            
            *result = PushExpression(unit);
            (*result)->type = ASTExp_Ternary;
            (*result)->condition = condition;
            
            SkipToken(lexer);
            
            encountered_errors = ParseExpression(lexer, unit, &(*result)->true_exp);
            
            if (!encountered_errors)
            {
                if (RequireToken(lexer, Token_Colon))
                {
                    encountered_errors = ParseConditionalExpression(lexer, unit, &(*result)->false_exp, 0);
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
ParseAssignmentExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result)
{
    bool encountered_errors = false;
    
    bool is_conditional_expression = false;
    encountered_errors = ParseConditionalExpression(lexer, unit, result, &is_conditional_expression);
    
    if (!encountered_errors)
    {
        if (!is_conditional_expression)
        {
            // NOTE(soimn): If both succeed and stop at the same point, the expression must be an isolated unary 
            //              expression
            
            // NOTE(soimn): Use result of ParseUnaryExpression
            
            Token token = PeekToken(lexer);
            
            // NOTE(soimn): Parse assignment if the next token is an assignment operator, else return
            
            if (token.type == Token_Equals)
            {
                SkipToken(lexer);
                
                encountered_errors = ParseAssignmentExpression(lexer);
            }
            
            else if (token.type == Token_PlusEquals)
            {
                SkipToken(lexer);
                
                encountered_errors = ParseAssignmentExpression(lexer);
            }
            
            else if (token.type == Token_MinusEquals)
            {
                SkipToken(lexer);
                
                encountered_errors = ParseAssignmentExpression(lexer);
            }
            
            else if (token.type == Token_MultiplyEquals)
            {
                SkipToken(lexer);
                
                encountered_errors = ParseAssignmentExpression(lexer);
            }
            
            else if (token.type == Token_DivideEquals)
            {
                SkipToken(lexer);
                
                encountered_errors = ParseAssignmentExpression(lexer);
            }
            
            else if (token.type == Token_ModuloEquals)
            {
                SkipToken(lexer);
                
                encountered_errors = ParseAssignmentExpression(lexer);
            }
            
            else if (token.type == Token_LeftShiftEquals)
            {
                SkipToken(lexer);
                
                encountered_errors = ParseAssignmentExpression(lexer);
            }
            
            else if (token.type == Token_RightShiftEquals)
            {
                SkipToken(lexer);
                
                encountered_errors = ParseAssignmentExpression(lexer);
            }
            
            else if (token.type == Token_AndEquals)
            {
                SkipToken(lexer);
                
                encountered_errors = ParseAssignmentExpression(lexer);
            }
            
            else if (token.type == Token_OrEquals)
            {
                SkipToken(lexer);
                
                encountered_errors = ParseAssignmentExpression(lexer);
            }
            
            else if (token.type == Token_XOREquals)
            {
                SkipToken(lexer);
                
                encountered_errors = ParseAssignmentExpression(lexer);
            }
        }
    }
    
    return encountered_errors;
}

inline bool
ParseExpression(Lexer* lexer, AST_Unit* unit, AST_Expression** result)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(lexer);
    
    if (token.type != Token_Comma && token.type != Token_Semicolon)
    {
        encountered_errors = ParseAssignmentExpression(lexer, unit, result);
        
        if (!encountered_errors)
        {
            if (RequireToken(lexer, Token_Comma))
            {
                encountered_errors = ParseExpression(lexer, unit, &(*result)->next);
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

// NOTE(soimn): This parses eveything that is legal in a function body
inline bool
ParseStatement(Lexer* lexer, AST_Unit* unit, AST_Statement** result)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(lexer);
    
    if (token.type == Token_Semicolon)
    {
        SkipToken(lexer);
    }
    
    else
    {
        if (token.type == Token_Identifier)
        {
            Token peek = PeekToken(lexer, 2);
            
            if (peek.type == Token_Colon)
            {
                encountered_errors = ParseVariableDeclaration(lexer, unit, result);
            }
            
            else if (peek.type == Token_DoubleColon)
            {
                String name = token.string;
                SkipToken(lexer);
                SkipToken(lexer);
                
                // IMPORTANT TODO(soimn): Parsing of local functions
                if (PeekToken(lexer, 3).type == Token_OpenParen)
                {
                    *result = PushStatement(unit);
                    (*result)->type = ASTStmt_ConstDecl;
                    (*result)->name = name;
                    
                    encountered_errors = ParseExpression(lexer, unit, &(*result)->const_value);
                }
            }
            
            else if (StringCompare(token.string, CONST_STRING("if")))
            {
                *result = PushStatement(unit);
                (*result)->type = ASTStmt_If;
                
                if (RequireToken(lexer, Token_OpenParen))
                {
                    encountered_errors = ParseExpression(lexer, unit, &(*result)->condition) || !RequireToken(lexer, Token_CloseParen);
                    
                    if (!encountered_errors)
                    {
                        if (PeekToken(lexer).type == Token_OpenBrace)
                        {
                            encountered_errors = ParseCompoundStatement(lexer, unit, &(*result)->true_body);
                        }
                        
                        else
                        {
                            (*result)->true_body = PushStatement(unit);
                            (*result)->true_body->type = ASTStmt_Compound;
                            
                            encountered_errors = ParseStatement(lexer, unit, &(*result)->true_body->body);
                        }
                        
                        peek = PeekToken(lexer);
                        if (!encountered_errors && peek.type == Token_Identifier && StringCompare(peek.string, CONST_STRING("else")))
                        {
                            SkipToken(lexer);
                            
                            token = PeekToken(lexer);
                            
                            if (token.type == Token_OpenBrace)
                            {
                                encountered_errors = ParseCompoundStatement(lexer, unit, &(*result)->false_body);
                            }
                            
                            else if (token.type == Token_Identifier && StringCompare(token.string, CONST_STRING("if")))
                            {
                                encountered_errors = ParseStatement(lexer, unit, &(*result)->false_body);
                            }
                            
                            else
                            {
                                (*result)->false_body = PushStatement(unit);
                                (*result)->false_body->type = ASTStmt_Compound;
                                
                                encountered_errors = ParseStatement(lexer, unit, &(*result)->false_body->body);
                            }
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
                *result = PushStatement(unit);
                (*result)->type = ASTStmt_While;
                
                if (RequireToken(lexer, Token_OpenParen))
                {
                    encountered_errors = ParseExpression(lexer, unit, &(*result)->true_body) || !RequireToken(lexer, Token_CloseParen);
                    
                    if (PeekToken(lexer).type == Token_OpenBrace)
                    {
                        encountered_errors = ParseCompoundStatement(lexer, unit, &(*result)->true_body);
                    }
                    
                    else
                    {
                        (*result)->true_body = PushStatement(unit);
                        (*result)->true_body->type = ASTStmt_Compound;
                        
                        encountered_errors = ParseStatement(lexer, unit, &(*result)->true_body->body);
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
        
        else
        {
            encountered_errors = ParseExpression(lexer) || !RequireToken(lexer, Token_Semicolon);
        }
    }
    
    return encountered_errors;
}

// NOTE(soimn): This parses something of the form '{' LIST_OF_STATEMENTS '}'
inline bool
ParseCompoundStatement(Lexer* lexer, AST_Unit* unit, AST_Statement** result)
{
    bool encountered_errors = false;
    
    encountered_errors = !RequireToken(lexer, Token_OpenBrace);
    
    if (!encountered_errors)
    {
        *result = PushStatement(unit);
        (*result)->type = ASTStmt_Compound;
        
        AST_Statement** current_statement = &(*result)->body;
        
        Token token = PeekToken(lexer);
        while (!encountered_errors && token.type != Token_CloseBrace)
        {
            if (token.type == Token_OpenBrace)
            {
                encountered_errors = ParseCompoundStatement(lexer, unit, current_statement);
                current_statement = &(*current_statement)->next;
            }
            
            else
            {
                encountered_errors = ParseStatement(lexer, unit, current_statement);
                current_statement = &(*current_statement)->next;
            }
            
            token = PeekToken(lexer);
        }
        
        if (!encountered_errors)
        {
            SkipToken(lexer);
        }
    }
    
    return encountered_errors;
}

inline bool
ParseType(Lexer* lexer, String* type_string)
{
    bool encountered_errors = false;
    
    type_string->data = lexer->input.data;
    
    Token token = GetToken(lexer);
    
    for (;;)
    {
        if (token.type == Token_Asterisk)
        {
        }
        
        else if (token.type == Token_OpenBracket)
        {
            encountered_errors = ParseExpression(lexer) || !RequireToken(lexer, Token_CloseBracket);
        }
        
        else if (token.type == Token_Identifier)
        {
            break;
        }
        
        else if (token.type == Token_OpenParen)
        {
            token = PeekToken(lexer);
            
            while (!encountered_errors && token.type != Token_CloseParen)
            {
                if (token.type == Token_Identifier)
                {
                    SkipToken(lexer);
                    
                    if (RequireToken(...))
                }
                
                else
                {
                    //// Expected variable name or type name in argument of function type
                    encountered_errors = true;
                }
            }
            
            encountered_errors = encountered_errors || !RequireToken(lexer, Token_CloseParen);
            
            break;
        }
        
        else
        {
            //// ERROR: Weird shit in type
            encountered_errors = true;
            break;
        }
        
        token = GetToken(lexer);
    }
    
    type_string->size = (UMM)(lexer->input.data - type_string->data);
    
    return encountered_errors;
}

// NOTE(soimn): Parses (arg_list) -> return_type
inline bool
ParseFunctionHeader(Lexer* lexer, AST_Unit* unit, AST_Statement** arguments, String* return_type_string)
{
    bool encountered_errors = false;
    
    Token token = GetToken(lexer);
    if (token.type == Token_OpenParen)
    {
        AST_Statement** current_arg = arguments;
        
        token = PeekToken(lexer);
        while (!encountered_errors && token.type != Token_CloseParen)
        {
            encountered_errors = ParseVariableDeclaration(lexer, unit, current_arg);
            current_arg = &(*current_arg)->next;
            
            token = PeekToken(lexer);
            
            if (!encountered_errors && token.type != Token_CloseParen)
            {
                ecnountered_errors = !RequireToken(lexer, Token_Comma);
                token = PeekToken(lexer);
            }
        }
        
        if (!encountered_errors)
        {
            SkipToken(lexer);
            token = PeekToken(lexer);
            
            if (token.type == Token_Arrow)
            {
                SkipToken(lexer);
                
                encountered_errors = ParseType(lexer, return_type_string);
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

// NOTE(soimn): Called on: IDENT ':'
inline bool
ParseVariableDeclaration(Lexer* lexer, AST_Unit* unit, AST_Statement** result, bool allow_assignment)
{
    bool encountered_errors = false;
    
    Token name = GetToken(lexer);
    
    if (name.type == Token_Identifier && RequireToken(lexer, Token_Colon))
    {
        *result = PushStatement(unit);
        (*result)->type = ASTStmt_VarDecl;
        (*result)->name = name.string;
        
        Token token = PeekToken(lexer);
        
        if (token.type != Token_Equals)
        {
            encountered_errors = ParseType(lexer, &(*result)->type_string);
            token = PeekToken(lexer);
        }
        
        if (!encountered_errors && token.type == Token_Equals)
        {
            if (allow_assignment)
            {
                SkipToken(lexer);
                encountered_errors = ParseExpression(lexer, unit, &(*result)->value);
            }
            
            else
            {
                //// ERROR: Assignment not allowed in this context
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR: Expected colon after name in variable declaration
        encountered_errors = true;
    }
    
    return encountered_errors || !RequireToken(lexer, Token_Semicolon);
}

inline AST_Unit
ParseUnit(String string)
{
    Lexer lexer = LexString(string);
    
    AST_Unit unit = {};
    AST_Statement** current_statement = &unit.first_statement;
    
    bool encountered_errors = false;
    
    Token token = PeekToken(&lexer);
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
            Token peek = PeekToken(&lexer, 2);
            if (peek.type == Token_Colon)
            {
                encountered_errors = ParseVariableDeclaration(&lexer, current_statement);
                current_statement  = &(*current_statement)->next;
            }
            
            else if (peek.type == Token_DoubleColon)
            {
                peek = PeekToken(&lexer, 3);
                if (peek.type == Token_OpenParen)
                {
                    String name = token.string;
                    SkipToken(&lexer);
                    SkipToken(&lexer);
                    
                    AST_Statement* decl = PushStatement(&unit);
                    decl->name = name;
                    
                    *current_statement = decl;
                    current_statement  = &decl->next;
                    
                    Lexer tmp_lexer = lexer;
                    
                    encountered_errors = ParseFunctionHeader(&tmp_lexer, false, true, &decl->func_body, &decl->func_arguments, &decl->return_type_string);
                    
                    if (!encountered_errors)
                    {
                        decl->type = ASTStmt_FuncDecl;
                        
                        lexer = tmp_lexer;
                        
                        if (PeekToken(&lexer).type == Token_OpenBrace)
                        {
                            encountered_errors = ParseCompoundStatement(&lexer, unit, &decl->func_body);
                        }
                        
                        else
                        {
                            //// ERROR: Missing function body
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        decl->type = ASTStmt_ConstDecl;
                        
                        encountered_errors = ParseExpression(&lexer, &unit, &decl->const_value) || !RequireToken(&lexer, Token_Semicolon);
                        
                        if (encountered_errors)
                        {
                            //// ERROR: Did not match a valid function or constant declaration
                        }
                    }
                }
                
                else if (peek.type == Token_Identifier && StringCompare(peek.string, CONST_STRING("struct")))
                {
                    String name = token.string;
                    SkipToken(&lexer);
                    SkipToken(&lexer);
                    
                    AST_Statement* struct_decl = PushStatement(&unit);
                    struct_decl->type = ASTStmt_StructDecl;
                    struct_decl->name = name;
                    
                    *current_statement = struct_decl;
                    current_statement  = &struct_decl->next;
                    
                    AST_Statement** current_field = &struct_decl->struct_body;
                    
                    if (RequireToken(&lexer, Token_OpenBrace))
                    {
                        token = GetToken(&lexer);
                        while (!encountered_errors && token.type != Token_CloseBrace)
                        {
                            if (token.type == Token_Identifier)
                            {
                                String variable_name = token.string;
                                
                                AST_Statement* variable_decl = PushStatement(&unit);
                                variable_decl->type = ASTStmt_VarDecl;
                                variable_decl->name = variable_name;
                                
                                token = PeekToken(&lexer);
                                
                                while (!encountered_errors && token.type != Token_CloseBrace)
                                {
                                    encountered_errors = ParseVariableDeclaration(&lexer, current_field);
                                    token = PeekToken(&lexer);
                                    current_field = &(*current_field)->next;
                                }
                                
                                if (!encountered_errors)
                                {
                                    SkipToken(&lexer);
                                }
                            }
                            
                            else
                            {
                                //// ERROR: Invalid token in struct body
                                encountered_errors = true;
                            }
                            
                            if (!encountered_errors)
                            {
                                token = GetToken(&lexer);
                            }
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Missing open brace after struct keyword in struct declaration
                        encountered_errors = true;
                    }
                    
                    encountered_errors = (encountered_errors || !RequireToken(&lexer, Token_CloseBrace));
                }
                
                else
                {
                    String name = token.string;
                    SkipToken(&lexer);
                    SkipToken(&lexer);
                    
                    AST_Statement* declaration = PushStatement(&unit);
                    
                    declaration->type = ASTStmt_ConstDecl;
                    declaration->name = name;
                    
                    encountered_errors = ParseExpression(&lexer, &declaration->value);
                    
                    *current_statement = declaration;
                    current_statement  = &declaration->next;
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
        
        token = PeekToken(&lexer);
    }
    
    unit.is_valid = !encountered_errors;
    
    return unit;
}