#pragma once

#include "common.h"
#include "string.h"
#include "lexer.h"
#include "ast.h"
#include "module.h"

struct Parser_State
{
    AST* ast;
    Lexer* lexer;
};

// NOTE(soimn): types consists of an optional sequence of "[..]" or '[' NUM ']' followed by zero or more '&', '*' 
//              and "[]" tokens and end in an identifier
inline bool
ParseType(Parser_State state, AST_Type* result, bool is_declaration = false)
{
    bool encountered_errors = false;
    
    String string = {};
    string.data = state.lexer->input.data;
    
    bool has_passed_storage_declaration = false;
    while (!encountered_errors)
    {
        Token token = PeekToken(state.lexer);
        
        if (token.type == Token_Identifier)
        {
            SkipToken(state.lexer);
            break;
        }
        
        else if (token.type == Token_Keyword)
        {
            if (token.keyword == Keyword_Proc)
            {
                SkipToken(state.lexer);
                
                if (MetRequiredToken(state.lexer, Token_OpenParen))
                {
                    while (!encountered_errors)
                    {
                        token = PeekToken(state.lexer);
                        Token peek =  PeekToken(state.lexer, 2);
                        
                        if (token.type == Token_CloseParen)
                        {
                            SkipToken(state.lexer);
                            break;
                        }
                        
                        else if (token.type == Token_Identifier && peek.type == Token_Colon)
                        {
                            SkipToken(state.lexer);
                            SkipToken(state.lexer);
                            
                            token = PeekToken(state.lexer);
                            
                            if (ParseType(state, 0))
                            {
                                token = PeekToken(state.lexer);
                            }
                            
                            else
                            {
                                //// ERROR: Failed to parse type of argument in function type
                                encountered_errors = true;
                            }
                            
                        }
                        
                        else
                        {
                            if (ParseType(state, 0))
                            {
                                // Succeeded
                            }
                            
                            else
                            {
                                //// ERROR: Invalid token in agruments of function type
                                encountered_errors = true;
                            }
                        }
                        
                        if (!encountered_errors && token.type != Token_CloseParen)
                        {
                            if (MetRequiredToken(state.lexer, Token_Comma))
                            {
                                // Succeeded
                            }
                            
                            else
                            {
                                //// ERROR: Missing separating comma between arguments in function type
                                encountered_errors = true;
                            }
                        }
                        
                    }
                    
                    if (!encountered_errors)
                    {
                        token = PeekToken(state.lexer);
                        
                        if (token.type == Token_Arrow)
                        {
                            SkipToken(state.lexer);
                            
                            if (ParseType(state, 0))
                            {
                                token = PeekToken(state.lexer);
                            }
                            
                            else
                            {
                                //// ERROR: Failed to parse return type in function type
                                encountered_errors = true;
                            }
                        }
                    }
                }
                
                else
                {
                    //// ERROR: Missing open paren before arguments in function type
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Invalid use of keyword in type
                encountered_errors = true;
            }
            
            break;
        }
        
        else if (token.type == Token_Ampersand || token.type == Token_Asterisk)
        {
            SkipToken(state.lexer);
        }
        
        else if (token.type == Token_OpenBracket)
        {
            SkipToken(state.lexer);
            
            for (;;)
            {
                token = PeekToken(state.lexer);
                
                if (token.type == Token_CloseBracket)
                {
                    SkipToken(state.lexer);
                }
                
                else if (token.type == Token_Number || token.type == Token_Elipsis)
                {
                    if (!has_passed_storage_declaration && is_declaration)
                    {
                        SkipToken(state.lexer);
                        
                        if (MetRequiredToken(state.lexer, Token_CloseBracket))
                        {
                            if (MetRequiredToken(state.lexer, Token_OpenBracket))
                            {
                                continue;
                            }
                            
                            else
                            {
                                // Succeeded
                            }
                        }
                        
                        else
                        {
                            //// Missing bracket after number / elipsis in array storage specifier of type
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        if (!is_declaration)
                        {
                            //// ERROR: Encountered array storage specifier in non-declaration type
                        }
                        
                        else
                        {
                            //// ERROR: Encountered array storage specifier after first token in type
                        }
                        
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Encountered illegal token after open bracket in type
                    encountered_errors = true;
                }
                
                has_passed_storage_declaration = true;
                break;
            }
        }
        
        else
        {
            //// ERROR: Encountered illegal token in type
            encountered_errors = true;
        }
        
        has_passed_storage_declaration = true;
    }
    
    string.size = state.lexer->input.data - string.data;
    
    if (result != 0)
    {
        result->string = string;
    }
    
    return !encountered_errors;
}

inline bool
ParseAssignmentExpression(Parser_State state, AST_Node** result);

inline bool
ParseScope(Parser_State state, AST_Node** result);

inline bool
ParseFunctionDeclaration(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(state.lexer);
    
    *result = PushNode(state.ast);
    (*result)->node_type = ASTNode_Expression; // NOTE(soimn): Assume this is a lambda
    (*result)->expr_type = ASTExpr_LambdaDecl;
    
    if (token.type == Token_Identifier)
    {
        (*result)->node_type   = ASTNode_FuncDecl;
        (*result)->expr_type   = ASTExpr_Invalid;
        (*result)->name.string = token.string;
        
        SkipToken(state.lexer);
        
        if (MetRequiredToken(state.lexer, Token_DoubleColon))
        {
            // Succeeded
        }
        
        else
        {
            //// ERROR: Missing double colon after function name in function declaration
            encountered_errors = true;
        }
    }
    
    if (!encountered_errors)
    {
        token = PeekToken(state.lexer);
        
        if (token.type == Token_Keyword && token.keyword == Keyword_Proc)
        {
            SkipToken(state.lexer);
            
            if (MetRequiredToken(state.lexer, Token_OpenParen))
            {
                AST_Node** current_argument = &(*result)->arguments;
                
                while (!encountered_errors)
                {
                    token = PeekToken(state.lexer);
                    
                    if (token.type == Token_CloseParen)
                    {
                        SkipToken(state.lexer);
                        break;
                    }
                    
                    else if (token.type == Token_Identifier)
                    {
                        *current_argument = PushNode(state.ast);
                        (*current_argument)->node_type   = ASTNode_VarDecl;
                        (*current_argument)->name.string = token.string;
                        
                        SkipToken(state.lexer);
                        
                        if (MetRequiredToken(state.lexer, Token_Colon))
                        {
                            token = PeekToken(state.lexer);
                            
                            if (token.type != Token_Equals)
                            {
                                if (ParseType(state, &(*current_argument)->type))
                                {
                                    token = PeekToken(state.lexer);
                                }
                                
                                else
                                {
                                    //// ERROR: Failed to parse type of argument in function / lambda declaration
                                    encountered_errors = true;
                                }
                            }
                            
                            if (token.type == Token_Equals)
                            {
                                SkipToken(state.lexer);
                                
                                if (ParseAssignmentExpression(state, &(*current_argument)->value))
                                {
                                    token = PeekToken(state.lexer);
                                }
                                
                                else
                                {
                                    //// ERROR: Failed to parse right hand side of assignment expression in function / 
                                    ////        lambda declaration
                                    encountered_errors = true;
                                }
                            }
                            
                            if (token.type != Token_CloseParen)
                            {
                                if (MetRequiredToken(state.lexer, Token_Comma))
                                {
                                    // Succeeded
                                    current_argument = &(*current_argument)->next;
                                }
                                
                                else
                                {
                                    //// ERROR: Missing separating comma between arguments in function / lambda 
                                    ////        declaration
                                    encountered_errors = true;
                                }
                            }
                        }
                        
                        else
                        {
                            //// ERROR: Missing colon after agrument name in arguments of function / lambda 
                            ////        declaration
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Invalid token in agruments of function / lambda declaration
                        encountered_errors = true;
                    }
                    
                }
                
                if (!encountered_errors)
                {
                    token = PeekToken(state.lexer);
                    
                    if (token.type == Token_Arrow)
                    {
                        SkipToken(state.lexer);
                        
                        if (ParseType(state, &(*result)->return_type))
                        {
                            token = PeekToken(state.lexer);
                        }
                        
                        else
                        {
                            //// ERROR: Failed to parse return type in function / lambda declaration
                            encountered_errors = true;
                        }
                    }
                    
                    if (!encountered_errors && token.type == Token_OpenBrace)
                    {
                        if (ParseScope(state, &(*result)->body))
                        {
                            // Succeeded
                        }
                        
                        else
                        {
                            //// ERROR: Failed to parse body of function / lambda declaration
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Missing body after function header in function / lambda declaration
                        encountered_errors = true;
                    }
                }
            }
            
            else
            {
                //// ERROR: Missing open paren before arguments in function / lambda declaration
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Missing proc keyword in function / lambda declaration
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

inline bool
ParsePostfixExpression(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(state.lexer);
    
    if (token.type == Token_Error || token.type == Token_EndOfStream)
    {
        //// ERROR: Invalid token in expression
        encountered_errors = true;
    }
    
    else if (token.type == Token_Number)
    {
        if (*result == 0)
        {
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Expression;
            (*result)->expr_type = ASTExpr_NumericLiteral;
            (*result)->numeric_literal = token.number;
            
            SkipToken(state.lexer);
        }
        
        else
        {
            //// ERROR: Invalid use of numeric constant as postix operator
            encountered_errors = true;
        }
    }
    
    else if (token.type == Token_String)
    {
        if (*result == 0)
        {
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Expression;
            (*result)->expr_type = ASTExpr_StringLiteral;
            (*result)->string_literal = token.string;
            
            SkipToken(state.lexer);
            
            if (ParsePostfixExpression(state, result))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse postix expression after string literal
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Invalid use of numeric constant as postix operator
            encountered_errors = true;
        }
    }
    
    else if (token.type == Token_Identifier)
    {
        if (*result != 0)
        {
            if ((*result)->expr_type == ASTExpr_Member)
            {
                if ((*result)->right == 0)
                {
                    (*result)->right = PushNode(state.ast);
                    (*result)->right->node_type = ASTNode_Expression;
                    (*result)->right->expr_type = ASTExpr_Identifier;
                    (*result)->right->identifier.string = token.string;
                    
                    SkipToken(state.lexer);
                    
                    if (ParsePostfixExpression(state, result))
                    {
                        // Succeeded
                    }
                    
                    else
                    {
                        //// ERROR: Failed to parse postix expression after identifier
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Invalid use of identifier as a postfix expression
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Invalid use of identifier as a postfix expression
                encountered_errors = true;
            }
        }
        
        else
        {
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Expression;
            (*result)->expr_type = ASTExpr_Identifier;
            (*result)->identifier.string = token.string;
            
            SkipToken(state.lexer);
            
            if (ParsePostfixExpression(state, result))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse postix expression after identifier
                encountered_errors = true;
            }
        }
    }
    
    else if (token.type == Token_Dot)
    {
        if (*result != 0)
        {
            SkipToken(state.lexer);
            
            AST_Node* left = *result;
            
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Expression;
            (*result)->expr_type = ASTExpr_Member;
            (*result)->left      = left;
            
            if (ParsePostfixExpression(state, result))
            {
                if ((*result)->right == 0)
                {
                    //// ERROR: Missing right operand of member operator
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Failed to parse right hand side of member operator
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Invalid use of member operator with no left operand
            encountered_errors = true;
        }
    }
    
    else if (token.type == Token_OpenBracket)
    {
        if (*result != 0)
        {
            if ((*result)->expr_type == ASTExpr_Member && (*result)->right == 0)
            {
                //// ERROR: Invalid use of subscript operator with member operator as left operand
                encountered_errors = true;
            }
            
            else
            {
                AST_Node* left = *result;
                
                *result = PushNode(state.ast);
                (*result)->node_type = ASTNode_Expression;
                (*result)->expr_type = ASTExpr_Subscript;
                (*result)->left      = left;
                
                SkipToken(state.lexer);
                
                if (ParseAssignmentExpression(state, &(*result)->right))
                {
                    if (MetRequiredToken(state.lexer, Token_CloseBracket))
                    {
                        if (ParsePostfixExpression(state, result))
                        {
                            // Succeeded
                        }
                        
                        else
                        {
                            //// ERROR: Failed to parse postfix expression after subscript
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Missing closing bracket after index in subscript postfix expression
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Failed to parse index in a subscript postfix expression
                    encountered_errors = true;
                }
            }
        }
        
        else
        {
            //// ERROR: Invalid use of subscript operator with no left operand
            encountered_errors = true;
        }
    }
    
    else if (token.type == Token_Keyword && token.keyword == Keyword_Proc)
    {
        if (*result == 0)
        {
            if (ParseFunctionDeclaration(state, result))
            {
                if (ParsePostfixExpression(state, result))
                {
                    // Succeeded
                }
                
                else
                {
                    //// ERROR: Failed to parse postfix expression after lambda declaration
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Failed to parse lambda declaration
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Invalid use of lambda declaration as a postfix expression
            encountered_errors = true;
        }
    }
    
    else if (token.type == Token_Increment || token.type == Token_Decrement)
    {
        if (*result != 0)
        {
            if ((*result)->expr_type == ASTExpr_Member && (*result)->right == 0)
            {
                //// ERROR: Invalid use of post increment / decrement with member operator as operand
                encountered_errors = true;
            }
            
            else
            {
                AST_Node* operand = *result;
                
                *result = PushNode(state.ast);
                (*result)->node_type = ASTNode_Expression;
                (*result)->expr_type = (token.type == Token_Increment ? ASTExpr_PostIncrement : ASTExpr_PostDecrement);
                (*result)->operand = operand;
                
                SkipToken(state.lexer);
                
                if (ParsePostfixExpression(state, result))
                {
                    // Succeeded
                }
                
                else
                {
                    //// ERROR: Failed to parse postix expression after post increment / decrement
                    encountered_errors = true;
                }
            }
        }
        
        else
        {
            //// ERROR: Invalid use of post increment / decrement with no operand
            encountered_errors = true;
        }
    }
    
    else if (token.type == Token_OpenParen)
    {
        if (*result != 0)
        {
            if ((*result)->expr_type != ASTExpr_Member && (*result)->expr_type != ASTExpr_StringLiteral)
            {
                SkipToken(state.lexer);
                
                AST_Node* call_function = *result;
                
                *result = PushNode(state.ast);
                (*result)->node_type = ASTNode_Expression;
                (*result)->expr_type = ASTExpr_FunctionCall;
                (*result)->call_function = call_function;
                
                AST_Node** current_argument = &(*result)->call_arguments;
                
                while (!encountered_errors)
                {
                    token = PeekToken(state.lexer);
                    
                    if (token.type == Token_CloseParen)
                    {
                        SkipToken(state.lexer);
                        break;
                    }
                    
                    else
                    {
                        if (ParseAssignmentExpression(state, current_argument))
                        {
                            current_argument = &(*current_argument)->next;
                        }
                        
                        else
                        {
                            //// ERROR: Failed to parse argument in function call expression
                            encountered_errors = true;
                        }
                    }
                    
                }
                
                if (!encountered_errors)
                {
                    if (ParsePostfixExpression(state, result))
                    {
                        // Succeeded
                    }
                    
                    else
                    {
                        //// ERROR: Failed to parse postfix expression after function call
                        encountered_errors = true;
                    }
                }
            }
            
            else
            {
                //// ERROR: Invalid use of function call operator on non-callable expression
                encountered_errors = true;
            }
        }
        
        else
        {
            SkipToken(state.lexer);
            
            if (ParseAssignmentExpression(state, result))
            {
                if (MetRequiredToken(state.lexer, Token_CloseParen))
                {
                    if (ParsePostfixExpression(state, result))
                    {
                        // Succeeded
                    }
                    
                    else
                    {
                        //// ERROR: Failed to parse postix expression after parenthesized expression
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Missing closing parenthesis at end of parenthesized expression
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Failed to parse contents of parenthesized expression
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        // NOTE(soimn): Do nothing
    }
    
    return !encountered_errors;
}

inline bool
ParseUnaryExpression(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(state.lexer);
    
    Enum32(AST_EXPRESSION_TYPE) type = ASTExpr_Invalid;
    
    switch (token.type)
    {
        case Token_Increment:  type = ASTExpr_PreIncrement; break;
        case Token_Decrement:  type = ASTExpr_PreDecrement; break;
        case Token_Ampersand:  type = ASTExpr_Dereference;  break;
        case Token_Asterisk:   type = ASTExpr_Reference;    break;
        case Token_Plus:       type = ASTExpr_UnaryPlus;    break;
        case Token_Minus:      type = ASTExpr_UnaryMinus;   break;
        case Token_Not:        type = ASTExpr_BitwiseNot;   break;
        case Token_LogicalNot: type = ASTExpr_LogicalNot;   break;
    }
    
    if (type != ASTExpr_Invalid)
    {
        *result = PushNode(state.ast);
        (*result)->node_type = ASTNode_Expression;
        (*result)->expr_type = type;
        
        SkipToken(state.lexer);
        
        if (ParseUnaryExpression(state, &(*result)->operand))
        {
            
        }
        
        else
        {
            //// ERROR: Failed to parse operand of unary expression
            encountered_errors = true;
        }
    }
    
    else
    {
        if (ParsePostfixExpression(state, result))
        {
            // NOTE(soimn): If this is true the expression is "empty" which is invalid
            if (*result == 0)
            {
                //// ERROR: Failed to parse expression as the expression is empty
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Failed to parse postfix expression
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

inline bool
ParseCastExpression(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(state.lexer);
    
    if (token.type == Token_Keyword && token.keyword == Keyword_Cast)
    {
        SkipToken(state.lexer);
        
        *result = PushNode(state.ast);
        (*result)->node_type = ASTNode_Expression;
        (*result)->expr_type = ASTExpr_TypeCast;
        
        if (MetRequiredToken(state.lexer, Token_OpenParen))
        {
            if (ParseType(state, &(*result)->type))
            {
                if (MetRequiredToken(state.lexer, Token_CloseParen))
                {
                    if (ParseCastExpression(state, &(*result)->to_cast))
                    {
                        // Succeeded
                    }
                    
                    else
                    {
                        //// ERROR: Failed to parse expression to be cast in cast expression
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Missing close parenthesis after target type in cast expression
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Failed to parse target type in cast expression
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Missing open parenthesis before target type in cast expression
            encountered_errors = true;
        }
    }
    
    else
    {
        if (ParseUnaryExpression(state, result))
        {
            // Succeeded
        }
        
        else
        {
            //// ERROR: Failed to parse unary expression
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

inline bool
ParseMultiplicativeExpression(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseCastExpression(state, result))
    {
        Token token = PeekToken(state.lexer);
        
        Enum32(AST_EXPRESSION_TYPE) type = ASTExpr_Invalid;
        
        switch(token.type)
        {
            case Token_Asterisk: type = ASTExpr_Multiplication; break;
            case Token_Divide:   type = ASTExpr_Division;       break;
            case Token_Modulo:   type = ASTExpr_Modulus;        break;
        }
        
        
        if (type != ASTExpr_Invalid)
        {
            AST_Node* left = *result;
            
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Expression;
            (*result)->expr_type = type;
            (*result)->left      = left;
            
            SkipToken(state.lexer);
            
            if (ParseMultiplicativeExpression(state, &(*result)->right))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse right hand side of multiplicative expression
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR: Failed to parse cast expression
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseAdditiveExpression(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseMultiplicativeExpression(state, result))
    {
        Token token = PeekToken(state.lexer);
        if (token.type == Token_Plus || token.type == Token_Minus)
        {
            AST_Node* left = *result;
            
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Expression;
            (*result)->expr_type = (token.type == Token_Plus ? ASTExpr_Addition : ASTExpr_Subtraction);
            (*result)->left      = left;
            
            SkipToken(state.lexer);
            
            if (ParseAdditiveExpression(state, &(*result)->right))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse right hand side of additive expression
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR: Failed to parse multiplicative expression
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseBitwiseShiftExpression(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseAdditiveExpression(state, result))
    {
        Token token = PeekToken(state.lexer);
        if (token.type == Token_LeftShift || token.type == Token_RightShift)
        {
            AST_Node* left = *result;
            
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Expression;
            (*result)->expr_type = (token.type == Token_LeftShift ? ASTExpr_BitwiseLeftShift : ASTExpr_BitwiseRightShift);
            (*result)->left      = left;
            
            SkipToken(state.lexer);
            
            if (ParseBitwiseShiftExpression(state, &(*result)->right))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse right hand side of bitwise shift expression
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR: Failed to parse additive expression
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseRelationalExpression(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseBitwiseShiftExpression(state, result))
    {
        Token token = PeekToken(state.lexer);
        
        Enum32(AST_EXPRESSION_TYPE) type = ASTExpr_Invalid;
        
        switch (token.type)
        {
            case Token_LessThan:           type = ASTExpr_IsLessThan;           break;
            case Token_LessThanOrEqual:    type = ASTExpr_IsLessThanOrEqual;    break;
            case Token_GreaterThan:        type = ASTExpr_IsGreaterThan;        break;
            case Token_GreaterThanOrEqual: type = ASTExpr_IsGreaterThanOrEqual; break;
        }
        
        if (type != ASTExpr_Invalid)
        {
            AST_Node* left = *result;
            
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Expression;
            (*result)->expr_type = type;
            (*result)->left      = left;
            
            SkipToken(state.lexer);
            
            if (ParseRelationalExpression(state, &(*result)->right))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse right hand side of relational expression
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR: Failed to parse shift expression
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseEqualityExpression(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseRelationalExpression(state, result))
    {
        Token token = PeekToken(state.lexer);
        if (token.type == Token_IsEqual || token.type == Token_IsNotEqual)
        {
            AST_Node* left = *result;
            
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Expression;
            (*result)->expr_type = (token.type == Token_IsEqual ? ASTExpr_IsEqual : ASTExpr_IsNotEqual);
            (*result)->left      = left;
            
            SkipToken(state.lexer);
            
            if (ParseEqualityExpression(state, &(*result)->right))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse right hand side of equality expression
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR: Failed to parse relational expression
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseBitwiseAndExpression(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseEqualityExpression(state, result))
    {
        if (MetRequiredToken(state.lexer, Token_Ampersand))
        {
            AST_Node* left = *result;
            
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Expression;
            (*result)->expr_type = ASTExpr_BitwiseAnd;
            (*result)->left      = left;
            
            if (ParseBitwiseAndExpression(state, &(*result)->right))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse right hand side of biwise and expression
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR: Failed to parse equality expression
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseExclusiveOrExpression(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseBitwiseAndExpression(state, result))
    {
        if (MetRequiredToken(state.lexer, Token_XOR))
        {
            AST_Node* left = *result;
            
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Expression;
            (*result)->expr_type = ASTExpr_BitwiseXOR;
            (*result)->left      = left;
            
            if (ParseExclusiveOrExpression(state, &(*result)->right))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse right hand side of exclusive or expression
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR: Failed to parse bitwise and expression
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseInclusiveOrExpression(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseExclusiveOrExpression(state, result))
    {
        if (MetRequiredToken(state.lexer, Token_Or))
        {
            AST_Node* left = *result;
            
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Expression;
            (*result)->expr_type = ASTExpr_BitwiseOr;
            (*result)->left      = left;
            
            if (ParseInclusiveOrExpression(state, &(*result)->right))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse right hand side of inclusive or expression
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR: Failed to parse exlusive or expression
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseLogicalAndExpression(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseInclusiveOrExpression(state, result))
    {
        if (MetRequiredToken(state.lexer, Token_LogicalAnd))
        {
            AST_Node* left = *result;
            
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Expression;
            (*result)->expr_type = ASTExpr_LogicalAnd;
            (*result)->left      = left;
            
            if (ParseLogicalAndExpression(state, &(*result)->right))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse right hand side of logical and expression
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR: Failed to parse inclusive or expression
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseLogicalOrExpression(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseLogicalAndExpression(state, result))
    {
        if (MetRequiredToken(state.lexer, Token_LogicalOr))
        {
            AST_Node* left = *result;
            
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Expression;
            (*result)->expr_type = ASTExpr_LogicalOr;
            (*result)->left      = left;
            
            if (ParseLogicalOrExpression(state, &(*result)->right))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse right hand side of logical or expression
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR: Failed to parse logical and expression
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseConditionalExpression(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseLogicalOrExpression(state, result))
    {
        if (MetRequiredToken(state.lexer, Token_Questionmark))
        {
            AST_Node* condition = *result;
            
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Expression;
            (*result)->expr_type = ASTExpr_Ternary;
            (*result)->condition = condition;
            
            if (ParseConditionalExpression(state, &(*result)->true_body))
            {
                if (MetRequiredToken(state.lexer, Token_Colon))
                {
                    if (ParseConditionalExpression(state, &(*result)->false_body))
                    {
                        // Succeeded
                    }
                    
                    else
                    {
                        //// ERROR: Failed to parse false expression in ternary
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Missing false expression in ternary
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Failed to parse true expression in ternary
                encountered_errors = true;
            }
        }
        
        else
        {
            // NOTE(soimn): When there expression is not a ternary, do nothing
        }
    }
    
    else
    {
        //// ERROR: Failed to parse logical or expression
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseAssignmentExpression(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseConditionalExpression(state, result))
    {
        Token token = PeekToken(state.lexer);
        
        Enum32(AST_EXPRESSION_TYPE) type = ASTExpr_Invalid;
        
        switch (token.type)
        {
            case Token_Equals:           type = ASTExpr_Equals;                  break;
            case Token_PlusEquals:       type = ASTExpr_AddEquals;               break;
            case Token_MinusEquals:      type = ASTExpr_SubEquals;               break;
            case Token_MultiplyEquals:   type = ASTExpr_MultEquals;              break;
            case Token_DivideEquals:     type = ASTExpr_DivEquals;               break;
            case Token_ModuloEquals:     type = ASTExpr_ModEquals;               break;
            case Token_AndEquals:        type = ASTExpr_BitwiseAndEquals;        break;
            case Token_OrEquals:         type = ASTExpr_BitwiseOrEquals;         break;
            case Token_XOREquals:        type = ASTExpr_BitwiseXOREquals;        break;
            case Token_LeftShiftEquals:  type = ASTExpr_BitwiseLeftShiftEquals;  break;
            case Token_RightShiftEquals: type = ASTExpr_BitwiseRightShiftEquals; break;
        }
        
        if (type != ASTExpr_Invalid)
        {
            AST_Node* left = *result;
            
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Expression;
            (*result)->expr_type = type;
            (*result)->left      = left;
            
            SkipToken(state.lexer);
            
            if (ParseAssignmentExpression(state, &(*result)->right))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse right hand side of assignment expression
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR: Failed to parse conditional expression
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseDeclaration(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(state.lexer);
    
    if (token.type == Token_Identifier)
    {
        Token peek = PeekToken(state.lexer, 2);
        
        if (peek.type == Token_Colon)
        {
            *result = PushNode(state.ast);
            (*result)->name.string = token.string;
            (*result)->node_type   = ASTNode_VarDecl;
            
            SkipToken(state.lexer);
            SkipToken(state.lexer);
            token = PeekToken(state.lexer);
            
            if (token.type != Token_Equals)
            {
                if (ParseType(state, &(*result)->type, true))
                {
                    token = PeekToken(state.lexer);
                }
                
                else
                {
                    //// ERROR: Failed to parse the stated type of variable in declaration
                    encountered_errors = true;
                }
            }
            
            if (token.type == Token_Equals)
            {
                SkipToken(state.lexer);
                
                if (ParseAssignmentExpression(state, &(*result)->value))
                {
                    // Succeeded
                }
                
                else
                {
                    //// ERROR: Failed to parse right hand side of variable declaration with assignment
                    encountered_errors = true;
                }
            }
            
            if (!MetRequiredToken(state.lexer, Token_Semicolon))
            {
                //// ERROR: Missing semicolon after variable declaration
                encountered_errors = true;
            }
        }
        
        else if (peek.type == Token_DoubleColon)
        {
            peek = PeekToken(state.lexer, 3);
            
            if (peek.type == Token_Keyword)
            {
                if (peek.keyword == Keyword_Struct)
                {
                    *result = PushNode(state.ast);
                    (*result)->name.string = token.string;
                    (*result)->node_type   = ASTNode_StructDecl;
                    
                    SkipToken(state.lexer);
                    SkipToken(state.lexer);
                    SkipToken(state.lexer);
                    
                    if (MetRequiredToken(state.lexer, Token_OpenBrace))
                    {
                        AST_Node** current_member = &(*result)->members;
                        
                        while (!encountered_errors)
                        {
                            token = PeekToken(state.lexer);
                            
                            if (token.type == Token_CloseBrace)
                            {
                                SkipToken(state.lexer);
                                break;
                            }
                            
                            else if (token.type == Token_Identifier)
                            {
                                *current_member = PushNode(state.ast);
                                (*current_member)->node_type   = ASTNode_VarDecl;
                                (*current_member)->name.string = token.string;
                                
                                SkipToken(state.lexer);
                                
                                if (MetRequiredToken(state.lexer, Token_Colon))
                                {
                                    token = PeekToken(state.lexer);
                                    
                                    if (token.type != Token_Equals)
                                    {
                                        if (ParseType(state, &(*current_member)->type))
                                        {
                                            token = PeekToken(state.lexer);
                                        }
                                        
                                        else
                                        {
                                            //// ERROR: Failed to parse type of member in struct declaration
                                            encountered_errors = true;
                                        }
                                    }
                                    
                                    if (token.type == Token_Equals)
                                    {
                                        SkipToken(state.lexer);
                                        
                                        if (ParseAssignmentExpression(state, &(*current_member)->value))
                                        {
                                            token = PeekToken(state.lexer);
                                        }
                                        
                                        else
                                        {
                                            //// ERROR: Failed to parse right hand side of assignment expression struct 
                                            ////        declaration
                                            encountered_errors = true;
                                        }
                                    }
                                    
                                    if (token.type != Token_CloseParen)
                                    {
                                        if (MetRequiredToken(state.lexer, Token_Semicolon))
                                        {
                                            // Succeeded
                                            current_member = &(*current_member)->next;
                                        }
                                        
                                        else
                                        {
                                            //// ERROR: Missing separating semicolon between members struct declaration
                                            encountered_errors = true;
                                        }
                                    }
                                }
                                
                                else
                                {
                                    //// ERROR: Missing colon after agrument name in members of function / lambda 
                                    ////        declaration
                                    encountered_errors = true;
                                }
                            }
                            
                            else
                            {
                                //// ERROR: Invalid token in struct declaration
                                encountered_errors = true;
                            }
                            
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Missing struct body
                        encountered_errors = true;
                    }
                }
                
                else if (peek.keyword == Keyword_Enum)
                {
                    *result = PushNode(state.ast);
                    (*result)->name.string = token.string;
                    (*result)->node_type   = ASTNode_EnumDecl;
                    
                    SkipToken(state.lexer);
                    SkipToken(state.lexer);
                    SkipToken(state.lexer);
                    
                    token = PeekToken(state.lexer);
                    
                    if (token.type != Token_OpenBrace)
                    {
                        if (ParseType(state, &(*result)->type))
                        {
                            // Succeeded
                        }
                        
                        else
                        {
                            //// ERROR: Failed to parse type specifier in enum declaration
                            encountered_errors = true;
                        }
                    }
                    
                    AST_Node** current_member = &(*result)->members;
                    
                    if (MetRequiredToken(state.lexer, Token_OpenBrace))
                    {
                        while (!encountered_errors)
                        {
                            token = PeekToken(state.lexer);
                            
                            if (token.type == Token_CloseBrace)
                            {
                                SkipToken(state.lexer);
                                break;
                            }
                            
                            else if (token.type == Token_Identifier)
                            {
                                *current_member = PushNode(state.ast);
                                (*current_member)->node_type   = ASTNode_VarDecl;
                                (*current_member)->name.string = token.string;
                                
                                SkipToken(state.lexer);
                                token = PeekToken(state.lexer);
                                
                                if (token.type == Token_Equals)
                                {
                                    SkipToken(state.lexer);
                                    
                                    if (ParseAssignmentExpression(state, &(*current_member)->value))
                                    {
                                        token = PeekToken(state.lexer);
                                    }
                                    
                                    else
                                    {
                                        //// ERROR: Failed to parse right hand side of assignment expression in enum body
                                        encountered_errors = true;
                                    }
                                }
                                
                                if (!encountered_errors && token.type != Token_CloseBrace)
                                {
                                    if (MetRequiredToken(state.lexer, Token_Comma))
                                    {
                                        // Succeeded
                                        current_member = &(*current_member)->next;
                                    }
                                    
                                    else
                                    {
                                        //// ERROR: Missing separating comma between elements in enum body
                                        encountered_errors = true;
                                    }
                                }
                            }
                            
                            else
                            {
                                //// ERROR: Invalid token in enum body
                                encountered_errors = true;
                            }
                            
                        }
                        
                    }
                    
                    else
                    {
                        //// ERROR: Missing body of enum declaration
                        encountered_errors = true;
                    }
                }
                
                else if (peek.keyword == Keyword_Proc)
                {
                    if (ParseFunctionDeclaration(state, result))
                    {
                        // Succeeded
                    }
                    
                    else
                    {
                        //// ERROR: Failed to pasre function declaration
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Invalid use of keyword in constant declaration
                    encountered_errors = true;
                }
            }
            
            else
            {
                *result = PushNode(state.ast);
                (*result)->name.string = token.string;
                (*result)->node_type   = ASTNode_ConstDecl;
                
                SkipToken(state.lexer);
                SkipToken(state.lexer);
                
                if (ParseAssignmentExpression(state, &(*result)->value))
                {
                    if (MetRequiredToken(state.lexer, Token_Semicolon))
                    {
                        // Succeeded
                    }
                    
                    else
                    {
                        //// ERROR: Missing semicolon after constant declaration
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Failed to parse the assigned expression in constant declaration
                    encountered_errors = true;
                }
            }
        }
        
        else
        {
            //// ILLEGAL USE OF PARSE DECLARATION FUNCTION. Missing colon or double colon
            INVALID_CODE_PATH;
        }
    }
    
    else
    {
        //// ILLEGAL USE OF PARSE DECLARATION FUNCTION. Missing identifier
        INVALID_CODE_PATH;
    }
    
    return !encountered_errors;
}

inline bool
ParseStatement(Parser_State state, AST_Node** result);

inline bool
ParseScope(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    bool is_brace_delimited = false;
    
    Token token = PeekToken(state.lexer);
    
    if (token.type == Token_OpenBrace)
    {
        is_brace_delimited = true;
        
        SkipToken(state.lexer);
    }
    
    *result = PushNode(state.ast);
    (*result)->node_type = ASTNode_Scope;
    (*result)->scope_id  = GetNewScopeID();
    
    AST_Node** current_statement = &(*result)->scope;
    
    do
    {
        token = PeekToken(state.lexer);
        
        if (token.type == Token_CloseBrace)
        {
            if (is_brace_delimited)
            {
                SkipToken(state.lexer);
                break;
            }
            
            else
            {
                //// ERROR: Encountered closing brace before end of statement
                encountered_errors = true;
            }
        }
        
        else
        {
            if (ParseStatement(state, current_statement))
            {
                current_statement = &(*current_statement)->next;
            }
            
            else
            {
                //// ERROR: Failed to parse statement in scope
                encountered_errors = true;
            }
        }
        
    } while (!encountered_errors && is_brace_delimited);
    
    return !encountered_errors;
}

inline bool
ParseStatement(Parser_State state, AST_Node** result)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(state.lexer);
    
    if (token.type == Token_Error || token.type == Token_EndOfStream)
    {
        //// ERROR: Encountered invalid token / unexpected end of stream while trying to parse a statement
        encountered_errors = true;
    }
    
    else if (token.type == Token_Semicolon)
    {
        SkipToken(state.lexer);
    }
    
    else if (token.type == Token_OpenBrace)
    {
        SkipToken(state.lexer);
        
        *result = PushNode(state.ast);
        (*result)->node_type = ASTNode_Scope;
        (*result)->scope_id = GetNewScopeID();
        
        AST_Node** current_statement = &(*result)->scope;
        
        token = PeekToken(state.lexer);
        
        while (!encountered_errors && token.type != Token_CloseBrace)
        {
            if (ParseStatement(state, current_statement))
            {
                current_statement = &(*current_statement)->next;
            }
            
            else
            {
                //// ERROR: Failed to parse statement in scope
                encountered_errors = true;
            }
        }
        
        if (!encountered_errors)
        {
            SkipToken(state.lexer);
        }
    }
    
    else if (token.type == Token_Keyword)
    {
        if (token.keyword == Keyword_If || token.keyword == Keyword_While)
        {
            bool is_if = (token.keyword == Keyword_If);
            
            SkipToken(state.lexer);
            
            *result = PushNode(state.ast);
            (*result)->node_type = (is_if ? ASTNode_If : ASTNode_While);
            
            if (MetRequiredToken(state.lexer, Token_OpenParen))
            {
                if (ParseAssignmentExpression(state, &(*result)->condition))
                {
                    if (MetRequiredToken(state.lexer, Token_CloseParen))
                    {
                        if (ParseScope(state, &(*result)->true_body))
                        {
                            token = PeekToken(state.lexer);
                            if (is_if && token.type == Token_Keyword && token.keyword == Keyword_Else)
                            {
                                token = PeekToken(state.lexer);
                                
                                if (token.type == Token_Keyword && token.keyword == Keyword_If)
                                {
                                    if (ParseStatement(state, &(*result)->false_body))
                                    {
                                        // Succeeded
                                    }
                                    
                                    else
                                    {
                                        //// ERROR: Failed to parse else if statement
                                        encountered_errors = true;
                                    }
                                }
                                
                                else
                                {
                                    if (ParseScope(state, &(*result)->false_body))
                                    {
                                        // Succeeded
                                    }
                                    
                                    else
                                    {
                                        //// ERROR: Failed to parse body of else statement
                                    }
                                }
                            }
                        }
                        
                        else
                        {
                            //// ERROR: Failed to parse statement after condition in if / while statement
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Missing close parenthesis after condition in if / while statement
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Failed to parse expression in if / while condition
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Missing open parenthesis before condition in if / while statement
                encountered_errors = true;
            }
        }
        
        else if (token.keyword == Keyword_Else)
        {
            //// ERROR: Invalid use of else without matching if
            encountered_errors = true;
        }
        
        else if (token.keyword == Keyword_Defer)
        {
            SkipToken(state.lexer);
            
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Defer;
            
            if (ParseScope(state, &(*result)->statement))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse statement to be defered
                encountered_errors = true;
            }
        }
        
        else if (token.keyword == Keyword_Return)
        {
            SkipToken(state.lexer);
            
            *result = PushNode(state.ast);
            (*result)->node_type = ASTNode_Return;
            
            if (ParseAssignmentExpression(state, &(*result)->value))
            {
                if (MetRequiredToken(state.lexer, Token_Semicolon))
                {
                    // Succeeded
                }
                
                else
                {
                    //// ERROR: Missing semicolon after return statement
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Failed to parse expression in return statement
                encountered_errors = true;
            }
        }
        
        else if (token.keyword == Keyword_Cast)
        {
            if (ParseAssignmentExpression(state, result))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse cast expression
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Invalid use of keyword in statement
            encountered_errors = true;
        }
    }
    
    else
    {
        Token peek = PeekToken(state.lexer, 2);
        
        if (peek.type == Token_Colon || peek.type == Token_DoubleColon)
        {
            if (ParseDeclaration(state, result))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse declaration
                encountered_errors = true;
            }
        }
        
        else
        {
            if (ParseAssignmentExpression(state, result))
            {
                if (MetRequiredToken(state.lexer, Token_Semicolon))
                {
                    // Succeeded
                }
                
                else
                {
                    //// ERROR: Missing semicolon after expression statement
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Failed to parse expression
                encountered_errors = true;
            }
        }
    }
    
    return !encountered_errors;
}

inline bool
ParseFile(Module* module, File* file, String file_contents)
{
    Lexer lexer = LexString(file_contents);
    
    file->ast.container = BUCKET_ARRAY(AST_Node, AST_NODE_BUCKET_SIZE);
    
    Parser_State state = {};
    state.ast   = &file->ast;
    state.lexer = &lexer;
    
    state.ast->root = PushNode(state.ast);
    state.ast->root->node_type = ASTNode_Scope;
    state.ast->root->scope_id  = GetNewScopeID();
    
    bool encountered_errors = false;
    
    Token token = PeekToken(state.lexer, 1);
    Token peek  = PeekToken(state.lexer, 2);
    
    AST_Node** current_statement = &state.ast->root->scope;
    
    while (!encountered_errors && token.type != Token_EndOfStream)
    {
        if (token.type == Token_Identifier && (peek.type == Token_Colon || peek.type == Token_DoubleColon))
        {
            if (ParseDeclaration(state, current_statement))
            {
                current_statement = &(*current_statement)->next;
            }
            
            else
            {
                //// ERROR: Failed to parse declaration in global scope
                encountered_errors = true;
            }
        }
        
        else if (token.type == Token_Hash)
        {
            if (peek.type == Token_Identifier)
            {
                if (StringCompare(peek.string, CONST_STRING("load")))
                {
                    SkipToken(state.lexer);
                    SkipToken(state.lexer);
                    
                    token = PeekToken(state.lexer);
                    
                    if (token.type == Token_String)
                    {
                        String file_path = token.string;
                        
                        SkipToken(state.lexer);
                        
                        File_ID* loaded_file = (File_ID*)PushElement(&file->loaded_files);
                        
                        if (LoadFileForCompilation(module, file_path, loaded_file))
                        {
                            // Successfully loaded file for compilation
                        }
                        
                        else
                        {
                            //// ERROR: Failed to load file referenced in *file*
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Missing path after load directive
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Unknown compiler directive
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Encountered invalid token in global scope
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Encountered invalid token in global scope
            encountered_errors = true;
        }
        
        token = PeekToken(state.lexer);
    }
    
    return !encountered_errors;
}