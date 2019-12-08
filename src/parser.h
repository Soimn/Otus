#pragma once

#include "common.h"
#include "string.h"
#include "lexer.h"
#include "ast.h"

struct Parser_State
{
    Lexer* lexer;
    AST* ast;
};

inline bool
ParseType(Parser_State state, AST_Type* resulting_type)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(state.lexer);
    
    if (token.type == Token_OpenParen)
    {
        /// This is a function type
        NOT_IMPLEMENTED;
    }
    
    else
    {
        /// This is a normal type
        NOT_IMPLEMENTED;
    }
    
    return !encountered_errors;
}

inline bool
ParseConditionalExpression(Parser_State state, AST_Expression** expression);

inline bool
ParseAssignmentExpression(Parser_State state, AST_Expression** expression);

inline bool
ParsePostfixExpression(Parser_State state, AST_Expression** expression)
{
    bool encountered_errors = false;
    
    auto ParseLambdaDeclaration = [](Parser_State state, AST_Expression** expression) -> bool
    {
        NOT_IMPLEMENTED;
    };
    
    Token token = PeekToken(state.lexer);
    
    if (*expression != 0)
    {
        Enum32(AST_EXPRESSION_TYPE) expression_type = (*expression)->type;
        bool is_postfix_expression = IsPostfixExpression(*expression);
        
        if (token.type == Token_Number)
        {
            if (!is_postfix_expression)
            {
                HARD_ASSERT(IsUnaryExpression(*expression));
                
                (*expression)->operand = PushExpression(state.ast);
                (*expression)->operand->type   = ASTExp_NumericLiteral;
                (*expression)->operand->number = token.number;
                
                SkipToken(state.lexer);
            }
            
            else
            {
                //// ERROR: Invalid use of numeric literal after postfix expression
                encountered_errors = true;
            }
        }
        
        else if (token.type == Token_String)
        {
            if (!is_postfix_expression)
            {
                HARD_ASSERT(IsUnaryExpression(*expression));
                
                (*expression)->operand = PushExpression(state.ast);
                (*expression)->operand->type   = ASTExp_StringLiteral;
                (*expression)->operand->string = token.string;
                
                SkipToken(state.lexer);
                
                if (ParsePostfixExpression(state, &(*expression)->operand))
                {
                    // Succeeded
                }
                
                else
                {
                    //// ERROR: Failed to parse postfix expression after string literal
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Invalid use of string literal after postfix expression
                encountered_errors = true;
            }
        }
        
        else if (token.type == Token_Identifier)
        {
            if (!is_postfix_expression)
            {
                HARD_ASSERT(IsUnaryExpression(*expression));
                
                (*expression)->operand = PushExpression(state.ast);
                (*expression)->operand->type       = ASTExp_Identifier;
                (*expression)->operand->identifier = token.string;
                
                SkipToken(state.lexer);
                
                if (ParsePostfixExpression(state, &(*expression)->operand))
                {
                    // Succeeded
                }
                
                else
                {
                    //// ERROR: Failed to parse postfix expression after identifier
                    encountered_errors = true;
                }
            }
            
            else
            {
                if (expression_type == ASTExp_Member)
                {
                    (*expression)->right = PushExpression(state.ast);
                    (*expression)->right->type       = ASTExp_Identifier;
                    (*expression)->right->identifier = token.string;
                    
                    SkipToken(state.lexer);
                    
                    if (ParsePostfixExpression(state, &(*expression)->right))
                    {
                        // Succeeded
                    }
                    
                    else
                    {
                        //// ERROR: Failed to parse postfix expression after identifier
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Illegal use of identifier after postfix expression
                    encountered_errors = true;
                }
            }
        }
        
        else if (token.type == Token_Increment || token.type == Token_Decrement)
        {
            if (is_postfix_expression)
            {
                if (expression_type != ASTExp_Lambda && expression_type != ASTExp_Member)
                {
                    bool is_inc = (token.type == Token_Increment);
                    SkipToken(state.lexer);
                    
                    AST_Expression* operand = *expression;
                    
                    *expression = PushExpression(state.ast);
                    (*expression)->type    = (is_inc ? ASTExp_PostIncrement : ASTExp_PostDecrement);
                    (*expression)->operand = operand;
                    
                    if (ParsePostfixExpression(state, expression))
                    {
                        // Succeeded
                    }
                    
                    else
                    {
                        //// ERROR: Failed to parse postfix expression after post increment / decrement
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Illegal use of increment / decrement as postfix operator after lambda / member 
                    ////        operator
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Missing expression before post increment / decrement
                encountered_errors = true;
            }
        }
        
        else if (token.type == Token_Dot)
        {
            if (is_postfix_expression)
            {
                if (expression_type != ASTExp_Member)
                {
                    if (expression_type != ASTExp_Lambda)
                    {
                        AST_Expression* left = *expression;
                        
                        *expression = PushExpression(state.ast);
                        (*expression)->type = ASTExp_Member;
                        (*expression)->left = left;
                        
                        SkipToken(state.lexer);
                        
                        if (ParsePostfixExpression(state, expression))
                        {
                            // Succeeded
                        }
                        
                        else
                        {
                            //// ERROR: Failed to parse the right hand side of member operator
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Illegal use of dot operator with lambda as left operand
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Invalid use of member operator with member operator as left operand
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Inavalid left operand of member operator
                encountered_errors = true;
            }
        }
        
        else if (token.type == Token_OpenBracket)
        {
            if (is_postfix_expression)
            {
                if (expression_type != ASTExp_Member && expression_type != ASTExp_Lambda)
                {
                    AST_Expression* left = *expression;
                    
                    *expression = PushExpression(state.ast);
                    (*expression)->type = ASTExp_Subscript;
                    (*expression)->left = left;
                    
                    SkipToken(state.lexer);
                    
                    if (ParseConditionalExpression(state, &(*expression)->right))
                    {
                        if (RequireToken(state.lexer, Token_CloseBracket))
                        {
                            if (ParsePostfixExpression(state, expression))
                            {
                                // Succeeded
                            }
                            
                            else
                            {
                                //// ERROR: Failed to parse postfix expression after subscript expression
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            //// ERROR: Missing closing bracket after expression in subscript operation
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Failed to parse subscript expression in subsript operation
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Illegal use of subscript operator with lambda / member operator as left operand
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Invalid operand for subscript expression
                encountered_errors = true;
            }
        }
        
        else if (token.type == Token_OpenParen)
        {
            Enum32(AST_EXPRESSION_TYPE) type = ASTExp_Invalid;
            
            Token peek[2] = {};
            peek[0] = PeekToken(state.lexer, 2);
            peek[1] = PeekToken(state.lexer, 3);
            
            if (peek[0].type == Token_Identifier &&
                peek[1].type == Token_Colon)
            {
                type = ASTExp_Lambda;
            }
            
            else if (peek[0].type == Token_CloseParen)
            {
                if (peek[1].type == Token_Arrow || peek[1].type == Token_OpenBrace)
                {
                    type = ASTExp_Lambda;
                }
                
                else
                {
                    type = ASTExp_FunctionCall;
                }
            }
            
            else if (is_postfix_expression)
            {
                type = ASTExp_FunctionCall;
            }
            
            if (type == ASTExp_Lambda)
            {
                if (!is_postfix_expression)
                {
                    if (ParseLambdaDeclaration(state, &(*expression)->operand))
                    {
                        if (ParsePostfixExpression(state, &(*expression)->operand))
                        {
                            // NOTE(soimn): If the type of the resulting expression is a lambda no postfix operators 
                            //              were parsed after the declaration
                            if ((*expression)->operand->type == ASTExp_Lambda)
                            {
                                // TODO(soimn): Should the parser catch this error, or should it be passed on and 
                                //              caught in type checking?
                                
                                //// ERROR: Invalid use of unary operator with lambda declaration as operand
                                encountered_errors = true;
                            }
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
                    //// ERROR: Illegal use of lambda declaration after postfix expression
                    encountered_errors = true;
                }
            }
            
            else if (type == ASTExp_FunctionCall)
            {
                if (is_postfix_expression)
                {
                    NOT_IMPLEMENTED;
                }
                
                else
                {
                    //// ERROR: Missing function pointer or identifier in function call expression
                    encountered_errors = true;
                }
            }
            
            else
            {
                SkipToken(state.lexer);
                
                if (!is_postfix_expression)
                {
                    if (ParseAssignmentExpression(state, &(*expression)->right))
                    {
                        if (RequireToken(state.lexer, Token_CloseParen))
                        {
                            if (ParsePostfixExpression(state, &(*expression)->right))
                            {
                                // Succeeded
                            }
                            
                            else
                            {
                                //// ERROR: Failed to parse postfix expression after expression enclosed in 
                                ////        parentheses
                            }
                        }
                        
                        else
                        {
                            //// ERROR: Missing closing parenthesis after expression
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Failed to parse expression enclosed in parentheses
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Invalid use of contained expression after postfix expression
                    encountered_errors = true;
                }
            }
        }
        
        else
        {
            if (is_postfix_expression)
            {
                if (expression_type == ASTExp_Member)
                {
                    //// ERROR: Missing operand on right hand side of member operator
                    encountered_errors = true;
                }
                
                else
                {
                    // NOTE(soimn): Do nothing
                }
            }
            
            else
            {
                HARD_ASSERT(IsUnaryExpression(*expression));
                
                //// ERROR: Missing right operand of unary operator
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        if (token.type == Token_Number)
        {
            *expression = PushExpression(state.ast);
            (*expression)->type   = ASTExp_NumericLiteral;
            (*expression)->number = token.number;
            
            SkipToken(state.lexer);
        }
        
        else if (token.type == Token_String)
        {
            *expression = PushExpression(state.ast);
            (*expression)->type   = ASTExp_StringLiteral;
            (*expression)->string = token.string;
            
            SkipToken(state.lexer);
            
            if (ParsePostfixExpression(state, expression))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse postfix expression
                encountered_errors = true;
            }
        }
        
        else if (token.type == Token_Identifier)
        {
            *expression = PushExpression(state.ast);
            (*expression)->type       = ASTExp_Identifier;
            (*expression)->identifier = token.string;
            
            SkipToken(state.lexer);
            
            if (ParsePostfixExpression(state, expression))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse postfix expression
                encountered_errors = true;
            }
        }
        
        else if (token.type == Token_OpenParen)
        {
            Token peek[2] = {};
            peek[0] = PeekToken(state.lexer, 2);
            peek[1] = PeekToken(state.lexer, 3);
            
            if ((peek[0].type == Token_Identifier && peek[1].type == Token_Colon) ||
                peek[0].type == Token_CloseParen)
            {
                if (ParseLambdaDeclaration(state, expression))
                {
                    if (ParsePostfixExpression(state, expression))
                    {
                        // NOTE(soimn): If the type of the resulting expression is a lambda no postfix operators 
                        //              were parsed after the declaration
                        if ((*expression)->operand->type == ASTExp_Lambda)
                        {
                            // TODO(soimn): Should the parser catch this error, or should it be passed on and 
                            //              caught in type checking?
                            
                            //// ERROR: Invalid use of unary operator with lambda declaration as operand
                            encountered_errors = true;
                        }
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
                SkipToken(state.lexer);
                
                if (ParseAssignmentExpression(state, expression))
                {
                    if (RequireToken(state.lexer, Token_CloseParen))
                    {
                        if (ParsePostfixExpression(state, expression))
                        {
                            // Succeeded
                        }
                        
                        else
                        {
                            //// ERROR: Failed to parse postfix expression after expression enclosed in 
                            ////        parentheses
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Missing closing parenthesis after expression
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Failed to parse expression enclosed in parentheses
                    encountered_errors = true;
                }
            }
        }
        
        else
        {
            //// ERROR: Encountered invalid token in expression
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

inline bool
ParseCastExpression(Parser_State state, AST_Expression** expression);

inline bool
ParseUnaryExpression(Parser_State state, AST_Expression** expression)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(state.lexer);
    
    Enum32(AST_EXPRESSION_TYPE) type = ASTExp_Invalid;
    
    switch(token.type)
    {
        case Token_Increment:  type = ASTExp_PreIncrement; break;
        case Token_Decrement:  type = ASTExp_PreDecrement; break;
        case Token_Ampersand:  type = ASTExp_Reference;    break;
        case Token_Asterisk:   type = ASTExp_Dereference;  break;
        case Token_Plus:       type = ASTExp_UnaryPlus;    break;
        case Token_Minus:      type = ASTExp_UnaryMinus;   break;
        case Token_Not:        type = ASTExp_Not;          break;
        case Token_LogicalNot: type = ASTExp_LogicalNot;   break;
    }
    
    if (type != ASTExp_Invalid)
    {
        *expression = PushExpression(state.ast);
        (*expression)->type = type;
        
        // NOTE(soimn): The reason for this assertion is to ensure the IsUnaryExpression function is up to date
        HARD_ASSERT(IsUnaryExpression(*expression));
        
        SkipToken(state.lexer);
        
        if (ParseCastExpression(state, expression))
        {
            // Succeeded
        }
        
        else
        {
            //// ERROR: Failed to parse operand of unary expression
            encountered_errors = true;
        }
    }
    
    else
    {
        // NOTE(soimn): The reason for this assertion is to ensure the IsUnaryExpression function is up to date
        HARD_ASSERT(!IsUnaryExpression(*expression));
        
        if (ParsePostfixExpression(state, expression))
        {
            // Succeeded
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
ParseCastExpression(Parser_State state, AST_Expression** expression)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(state.lexer);
    
    if (token.type == Token_Keyword && token.keyword == Keyword_Cast)
    {
        *expression = PushExpression(state.ast);
        (*expression)->type = ASTExp_TypeCast;
        
        SkipToken(state.lexer);
        
        if (RequireToken(state.lexer, Token_OpenParen))
        {
            if (ParseType(state, &(*expression)->target_type))
            {
                if (RequireToken(state.lexer, Token_CloseParen))
                {
                    if (ParseCastExpression(state, &(*expression)->to_cast))
                    {
                        // Succeeded
                    }
                    
                    else
                    {
                        //// ERROR: Failed to parse target expression in cast expression
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Missing matching closing parenthesis after target type in cast expression
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
            //// ERROR: Missing open parenthesis after cast keyword in cast expression
            encountered_errors = true;
        }
    }
    
    else
    {
        if (ParseUnaryExpression(state, expression))
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
ParseMultiplicativeExpression(Parser_State state, AST_Expression** expression)
{
    bool encountered_errors = false;
    
    if (ParseCastExpression(state, expression))
    {
        Token token = PeekToken(state.lexer);
        
        Enum32(AST_EXPRESSION_TYPE) type = ASTExp_Invalid;
        
        switch(token.type)
        {
            case Token_Asterisk: type = ASTExp_Multiply; break;
            case Token_Divide:   type = ASTExp_Divide;   break;
            case Token_Modulo:   type = ASTExp_Modulus;  break;
        }
        
        if (type != ASTExp_Invalid)
        {
            AST_Expression* left = *expression;
            
            *expression = PushExpression(state.ast);
            (*expression)->type = type;
            (*expression)->left = left;
            
            if (ParseMultiplicativeExpression(state, &(*expression)->right))
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
ParseAdditiveExpression(Parser_State state, AST_Expression** expression)
{
    bool encountered_errors = false;
    
    if (ParseMultiplicativeExpression(state, expression))
    {
        Token token = PeekToken(state.lexer);
        
        if (token.type == Token_Plus || token.type == Token_Minus)
        {
            AST_Expression* left = *expression;
            
            *expression = PushExpression(state.ast);
            (*expression)->type = (token.type == Token_Plus ? ASTExp_Plus : ASTExp_Minus);
            (*expression)->left = left;
            
            if (ParseAdditiveExpression(state, &(*expression)->right))
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
    
    return encountered_errors;
}

inline bool
ParseShiftExpression(Parser_State state, AST_Expression** expression)
{
    bool encountered_errors = false;
    
    if (ParseAdditiveExpression(state, expression))
    {
        Token token = PeekToken(state.lexer);
        
        if (token.type == Token_LeftShift || token.type == Token_RightShift)
        {
            AST_Expression* left = *expression;
            
            *expression = PushExpression(state.ast);
            (*expression)->type = (token.type == Token_LeftShift ? ASTExp_LeftShift : ASTExp_RightShift);
            (*expression)->left = left;
            
            if (ParseShiftExpression(state, &(*expression)->right))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse right hand side of shift expression
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
ParseRelationalExpression(Parser_State state, AST_Expression** expression)
{
    bool encountered_errors = false;
    
    if (ParseShiftExpression(state, expression))
    {
        Token token = PeekToken(state.lexer);
        
        Enum32(AST_EXPRESSION_TYPE) type = ASTExp_Invalid;
        
        switch(token.type)
        {
            case Token_LessThan:           type = ASTExp_IsLessThan;           break;
            case Token_LessThanOrEqual:    type = ASTExp_IsLessThanOrEqual;    break;
            case Token_GreaterThan:        type = ASTExp_IsGreaterThan;        break;
            case Token_GreaterThanOrEqual: type = ASTExp_IsGreaterThanOrEqual; break;
        }
        
        if (type != ASTExp_Invalid)
        {
            AST_Expression* left = *expression;
            
            *expression = PushExpression(state.ast);
            (*expression)->type = type;
            (*expression)->left = left;
            
            if (ParseRelationalExpression(state, &(*expression)->right))
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
        //// ERROR: Failed to parse equality expression
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseEqualityExpression(Parser_State state, AST_Expression** expression)
{
    bool encountered_errors = false;
    
    if (ParseRelationalExpression(state, expression))
    {
        Token token = PeekToken(state.lexer);
        
        if (token.type == Token_IsEqual || token.type == Token_IsNotEqual)
        {
            AST_Expression* left = *expression;
            
            *expression = PushExpression(state.ast);
            (*expression)->type = (token.type == Token_IsEqual ? ASTExp_IsEqual : ASTExp_IsNotEqual);
            (*expression)->left = left;
            
            SkipToken(state.lexer);
            
            if (ParseEqualityExpression(state, &(*expression)->right))
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
ParseAndExpression(Parser_State state, AST_Expression** expression)
{
    bool encountered_errors = false;
    
    if (ParseEqualityExpression(state, expression))
    {
        if (RequireToken(state.lexer, Token_Ampersand))
        {
            AST_Expression* left = *expression;
            
            *expression = PushExpression(state.ast);
            (*expression)->type = ASTExp_And;
            (*expression)->left = left;
            
            if (ParseAndExpression(state, &(*expression)->right))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse right hand side of and expression
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
ParseExclusiveOrExpression(Parser_State state, AST_Expression** expression)
{
    bool encountered_errors = false;
    
    if (ParseAndExpression(state, expression))
    {
        if (RequireToken(state.lexer, Token_XOR))
        {
            AST_Expression* left = *expression;
            
            *expression = PushExpression(state.ast);
            (*expression)->type = ASTExp_XOR;
            (*expression)->left = left;
            
            if (ParseExclusiveOrExpression(state, &(*expression)->right))
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
    
    return !encountered_errors;
}

inline bool
ParseInclusiveOrExpression(Parser_State state, AST_Expression** expression)
{
    bool encountered_errors = false;
    
    if (ParseExclusiveOrExpression(state, expression))
    {
        if (RequireToken(state.lexer, Token_Or))
        {
            AST_Expression* left = *expression;
            
            *expression = PushExpression(state.ast);
            (*expression)->type = ASTExp_Or;
            (*expression)->left = left;
            
            if (ParseInclusiveOrExpression(state, expression))
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
        //// ERROR: Failed to parse exclusive or expression
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseLogicalAndExpression(Parser_State state, AST_Expression** expression)
{
    bool encountered_errors = false;
    
    if (ParseInclusiveOrExpression(state, expression))
    {
        if (RequireToken(state.lexer, Token_LogicalAnd))
        {
            AST_Expression* left = *expression;
            
            *expression = PushExpression(state.ast);
            (*expression)->type = ASTExp_LogicalAnd;
            (*expression)->left = left;
            
            if (ParseLogicalAndExpression(state, &(*expression)->right))
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
ParseLogicalOrExpression(Parser_State state, AST_Expression** expression)
{
    bool encountered_errors = false;
    
    if (ParseLogicalAndExpression(state, expression))
    {
        if (RequireToken(state.lexer, Token_LogicalOr))
        {
            AST_Expression* left = *expression;
            
            *expression = PushExpression(state.ast);
            (*expression)->type = ASTExp_LogicalOr;
            (*expression)->left = left;
            
            if (ParseLogicalOrExpression(state, &(*expression)->right))
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
ParseConditionalExpression(Parser_State state, AST_Expression** expression)
{
    bool encountered_errors = false;
    
    if (ParseLogicalOrExpression(state, expression))
    {
        if (RequireToken(state.lexer, Token_Questionmark))
        {
            AST_Expression* condition = *expression;
            
            *expression = PushExpression(state.ast);
            (*expression)->type = ASTExp_Ternary;
            (*expression)->condition = condition;
            
            if (ParseConditionalExpression(state, &(*expression)->true_expr))
            {
                if (RequireToken(state.lexer, Token_Colon))
                {
                    if (ParseConditionalExpression(state, &(*expression)->false_expr))
                    {
                        // Succeeded
                    }
                    
                    else
                    {
                        //// ERROR: Failed to parse false clause in ternary
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Missing else clause in ternary
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Failed to parse true clause in ternary
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
ParseAssignmentExpression(Parser_State state, AST_Expression** resulting_expression)
{
    bool encountered_errors = false;
    
    if (ParseConditionalExpression(state, resulting_expression))
    {
        Token token = PeekToken(state.lexer);
        
        Enum32(AST_EXPRESSION_TYPE) type = ASTExp_Invalid;
        
        switch (token.type)
        {
            case Token_Equals:           type = ASTExp_Equals;           break;
            case Token_PlusEquals:       type = ASTExp_PlusEquals;       break;
            case Token_MinusEquals:      type = ASTExp_MinusEquals;      break;
            case Token_MultiplyEquals:   type = ASTExp_MultiplyEquals;   break;
            case Token_DivideEquals:     type = ASTExp_DivideEquals;     break;
            case Token_ModuloEquals:     type = ASTExp_ModulusEquals;    break;
            case Token_AndEquals:        type = ASTExp_AndEquals;        break;
            case Token_OrEquals:         type = ASTExp_OrEquals;         break;
            case Token_XOREquals:        type = ASTExp_XOREquals;        break;
            case Token_LeftShiftEquals:  type = ASTExp_LeftShiftEquals;  break;
            case Token_RightShiftEquals: type = ASTExp_RightShiftEquals; break;
        }
        
        if (type != ASTExp_Invalid)
        {
            SkipToken(state.lexer);
            
            AST_Expression* left = *resulting_expression;
            
            *resulting_expression = PushExpression(state.ast);
            (*resulting_expression)->type = type;
            (*resulting_expression)->left = left;
            
            if (ParseAssignmentExpression(state, &(*resulting_expression)->right))
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
ParseConstantOrFunctionDeclaration(Parser_State state, AST_Statement** resulting_statement)
{
    bool encountered_errors = false;
    
    *resulting_statement = PushStatement(state.ast);
    
    String name = GetToken(state.lexer).string;
    SkipToken(state.lexer);
    
    if (PeekToken(state.lexer).type == Token_OpenParen)
    {
        Token peek = PeekToken(state.lexer, 2);
        
        if (peek.type == Token_Identifier && PeekToken(state.lexer, 3).type == Token_Colon || peek.type == Token_CloseParen)
        {
            /// This is a function declaration
            NOT_IMPLEMENTED;
        }
    }
    
    if (!encountered_errors && *resulting_statement == 0)
    {
        /// This is a constant declaration
        NOT_IMPLEMENTED;
    }
    
    return !encountered_errors;
}

// NOTE(soimn): This is only called when the pattern IDENT ':' is found
inline bool
ParseVariableDeclaration(Parser_State state, AST_Statement** resulting_statement)
{
    bool encountered_errors = false;
    
    String name = GetToken(state.lexer).string;
    SkipToken(state.lexer);
    
    *resulting_statement = PushStatement(state.ast);
    (*resulting_statement)->type     = ASTStmt_VarDecl;
    (*resulting_statement)->var_name = name;
    
    Token token = PeekToken(state.lexer);
    
    if (token.type != Token_Equals)
    {
        if (ParseType(state, &(*resulting_statement)->var_type))
        {
            // Succeeded
        }
        
        else
        {
            //// ERROR: Failed to parse type in variable declaration
            encountered_errors = true;
        }
        
        token = PeekToken(state.lexer);
    }
    
    if (!encountered_errors && token.type == Token_Equals)
    {
        SkipToken(state.lexer);
        
        if (RequireToken(state.lexer, Token_TripleDash))
        {
            (*resulting_statement)->var_is_uninitialized = true;
        }
        
        else
        {
            if (ParseAssignmentExpression(state, &(*resulting_statement)->var_value))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse assignment expression in variable declaration
                encountered_errors = true;
            }
        }
    }
    
    if (!RequireToken(state.lexer, Token_Semicolon))
    {
        //// ERROR: Missing semiclon after variable declaration
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseScope(Parser_State state, AST_Scope* scope);

inline bool
ParseStatement(Parser_State state, AST_Statement** resulting_statement)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(state.lexer);
    
    if (token.type == Token_Semicolon)
    {
        SkipToken(state.lexer);
    }
    
    else if (token.type == Token_OpenBrace)
    {
        *resulting_statement = PushStatement(state.ast);
        (*resulting_statement)->type = ASTStmt_Scope;
        
        if (ParseScope(state, &(*resulting_statement)->scope))
        {
            // Succeeded
        }
        
        else
        {
            //// ERROR: Failed to parse sub scope
            encountered_errors = true;
        }
    }
    
    else if (token.type == Token_Keyword)
    {
        if (token.keyword == Keyword_If)
        {
            *resulting_statement = PushStatement(state.ast);
            (*resulting_statement)->type = ASTStmt_If;
            
            if (ParseConditionalExpression(state, &(*resulting_statement)->condition))
            {
                if ((*resulting_statement)->condition == 0)
                {
                    //// ERROR: Missing condition in if statement
                    encountered_errors = true;
                }
                
                else
                {
                    if (ParseScope(state, &(*resulting_statement)->true_body))
                    {
                        token = PeekToken(state.lexer);
                        
                        if (token.type == Token_Keyword && StringCompare(token.string, CONST_STRING("else")))
                        {
                            SkipToken(state.lexer);
                            
                            if (ParseStatement(state, &(*resulting_statement)->false_statement))
                            {
                                AST_Statement* false_statement = (*resulting_statement)->false_statement;
                                if (false_statement->type != ASTStmt_If && false_statement->type != ASTStmt_Scope)
                                {
                                    // NOTE(soimn): This is an else statement without an explicit scope delimited by 
                                    //              braces. Insert scope node to ensure correct lexical scoping and defer 
                                    //              behaviour.
                                    
                                    AST_Statement* false_statement_value = PushStatement(state.ast);
                                    *false_statement_value = *false_statement;
                                    
                                    false_statement->type             = ASTStmt_Scope;
                                    false_statement->scope.id         = GetNewScopeID();
                                    false_statement->scope.statements = false_statement_value;
                                }
                            }
                            
                            else
                            {
                                //// ERROR: Failed to parse else body of if statement
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            // The if statement is not followed by an else statement, continue
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Failed to parse body of if statement
                        encountered_errors = true;
                    }
                }
            }
            
            else
            {
                //// ERROR: Failed to parse condition of if statement
                encountered_errors = true;
            }
        }
        
        else if (token.keyword == Keyword_While)
        {
            *resulting_statement = PushStatement(state.ast);
            (*resulting_statement)->type = ASTStmt_While;
            
            if (ParseConditionalExpression(state, &(*resulting_statement)->condition))
            {
                if ((*resulting_statement)->condition == 0)
                {
                    //// ERROR: Missing condition in while statement
                    encountered_errors = true;
                }
                
                else
                {
                    if (ParseScope(state, &(*resulting_statement)->true_body))
                    {
                        // Succeeded
                    }
                    
                    else
                    {
                        //// ERROR: Failed to parse while statement body
                        encountered_errors = true;
                    }
                }
            }
            
            else
            {
                //// ERROR: Failed to parse condition of while statement
            }
        }
        
        else if (token.keyword == Keyword_Defer)
        {
            *resulting_statement = PushStatement(state.ast);
            (*resulting_statement)->type = ASTStmt_Defer;
            
            if (ParseScope(state, &(*resulting_statement)->defer_scope))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse body of defer statement
                encountered_errors = true;
            }
        }
        
        else if (token.keyword == Keyword_Return)
        {
            *resulting_statement = PushStatement(state.ast);
            (*resulting_statement)->type = ASTStmt_Return;
            
            if (ParseConditionalExpression(state, &(*resulting_statement)->return_value))
            {
                if (!RequireToken(state.lexer, Token_Semicolon))
                {
                    //// ERROR: Missing semicolon after return statement
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Failed to parse retutn value
                encountered_errors = true;
            }
        }
        
        else if (token.keyword == Keyword_Local || token.keyword == Keyword_Global)
        {
            //// ERROR: Invalid use of local/global keyword
        }
        
        else
        {
            //// ERROR: Invalid keyword in statement
            encountered_errors = true;
        }
    }
    
    else if (token.type == Token_Identifier)
    {
        token = PeekToken(state.lexer, 2);
        
        if (token.type == Token_DoubleColon)
        {
            if (ParseConstantOrFunctionDeclaration(state, resulting_statement))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse constant or function declaration
                encountered_errors = true;
            }
        }
        
        else if (token.type == Token_Colon)
        {
            if (ParseVariableDeclaration(state, resulting_statement))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse variabe declaration in scope
                encountered_errors = true;
            }
        }
        
        else
        {
            *resulting_statement = PushStatement(state.ast);
            (*resulting_statement)->type = ASTStmt_Expression;
            
            if (ParseAssignmentExpression(state, &(*resulting_statement)->expression))
            {
                if (!RequireToken(state.lexer, Token_Semicolon))
                {
                    //// ERROR: Missing semicolon after expression in statement
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Failed to parse expression in statement
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        *resulting_statement = PushStatement(state.ast);
        (*resulting_statement)->type = ASTStmt_Expression;
        
        if (ParseAssignmentExpression(state, &(*resulting_statement)->expression))
        {
            if (!RequireToken(state.lexer, Token_Semicolon))
            {
                //// ERROR: Missing semicolon after expression in statement
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Failed to parse expression in statement
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

inline bool
ParseScope(Parser_State state, AST_Scope* scope)
{
    bool encountered_errors = false;
    
    if (scope->id == AST_INVALID_SCOPE_ID)
    {
        scope->id = GetNewScopeID();
    }
    
    bool is_brace_delimited = false;
    
    if (RequireToken(state.lexer, Token_OpenBrace))
    {
        is_brace_delimited = true;
    }
    
    Token token = PeekToken(state.lexer);
    
    AST_Statement** current_statement = &scope->statements;
    
    while (!encountered_errors)
    {
        if (token.type == Token_CloseBrace)
        {
            if (is_brace_delimited)
            {
                SkipToken(state.lexer);
                break;
            }
            
            else
            {
                //// ERROR: Expected statement before closing brace
                encountered_errors = true;
            }
        }
        
        else if (token.type == Token_EndOfStream)
        {
            //// ERROR: Encountered end of file before scope end
            encountered_errors = true;
        }
        
        else if (token.type == Token_Semicolon)
        {
            SkipToken(state.lexer);
            token = PeekToken(state.lexer);
            
            if (is_brace_delimited) continue;
            else break;
        }
        
        else
        {
            if (ParseStatement(state, current_statement))
            {
                if (!is_brace_delimited)
                {
                    // NOTE(soimn): Done parsing the first statement. When the scope is not brace delimited, the 
                    //              parsing of the scope is done.
                    break;
                }
            }
            
            else
            {
                //// ERROR: Failed to parse statement in scope
                encountered_errors = true;
            }
        }
        
        token = PeekToken(state.lexer);
        current_statement = &(*current_statement)->next;
    }
    
    return !encountered_errors;
}

inline bool
ParseDeclarationScope(Parser_State state, AST_Statement** current_global_statement, AST_Statement** current_local_statement, AST_Statement** current_default_statement)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(state.lexer);
    
    auto ParseVariableConstantOrFunctionDeclaration = [](Parser_State state, AST_Statement** resulting_statement) -> bool
    {
        bool encountered_errors = false;
        
        Token token = PeekToken(state.lexer, 2);
        
        if (token.type == Token_DoubleColon)
        {
            if (ParseConstantOrFunctionDeclaration(state, resulting_statement))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse constant of function declaration in scope
                encountered_errors = true;
            }
        }
        
        else if (token.type == Token_Colon)
        {
            if (ParseVariableDeclaration(state, resulting_statement))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Failed to parse variable declaration
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Encountered an invalid statement in scope
            encountered_errors = true;
        }
        
        return !encountered_errors;
    };
    
    while(!encountered_errors)
    {
        if (token.type == Token_EndOfStream) break;
        else if (token.type == Token_CloseBrace) break;
        else
        {
            if (token.type == Token_Identifier || (token.type == Token_Keyword && (token.keyword == Keyword_Inline ||
                                                                                   token.keyword == Keyword_NoInline)))
            {
                if (ParseVariableConstantOrFunctionDeclaration(state, current_default_statement))
                {
                    current_default_statement = &(*current_default_statement)->next;
                }
                
                else
                {
                    //// ERROR: Failed to parse declaration
                    encountered_errors = true;
                }
            }
            
            else if (token.type == Token_Keyword)
            {
                if (token.keyword == Keyword_Local)
                {
                    SkipToken(state.lexer);
                    
                    if (RequireToken(state.lexer, Token_OpenBrace))
                    {
                        if (ParseDeclarationScope(state, current_global_statement, current_local_statement, current_local_statement))
                        {
                            if (RequireToken(state.lexer, Token_CloseBrace))
                            {
                                // Succeeded
                            }
                            
                            else
                            {
                                //// ERROR: Missing matching closing brace in local declaration scope
                            }
                        }
                    }
                    
                    else
                    {
                        
                        if (ParseVariableConstantOrFunctionDeclaration(state, current_local_statement))
                        {
                            current_local_statement = &(*current_local_statement)->next;
                        }
                        
                        else
                        {
                            //// ERROR: Failed to parse local declaration
                            encountered_errors = true;
                        }
                    }
                }
                
                else if (token.keyword == Keyword_Global)
                {
                    SkipToken(state.lexer);
                    
                    if (RequireToken(state.lexer, Token_OpenBrace))
                    {
                        if (ParseDeclarationScope(state, current_global_statement, current_local_statement, current_global_statement))
                        {
                            if (RequireToken(state.lexer, Token_CloseBrace))
                            {
                                // Succeeded
                            }
                            
                            else
                            {
                                //// ERROR: Missing matching closing brace in global declaration scope
                            }
                        }
                    }
                    
                    else
                    {
                        if (ParseVariableConstantOrFunctionDeclaration(state, current_local_statement))
                        {
                            current_global_statement = &(*current_global_statement)->next;
                        }
                        
                        else
                        {
                            //// ERROR: Failed to parse declaration
                            encountered_errors = true;
                        }
                    }
                }
                
                else
                {
                    //// ERROR: Invalid use of token in x scope
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Invalid use of token in x scope
                encountered_errors = true;
            }
        }
    }
    
    return !encountered_errors;
}

inline AST
ParseFile(String input)
{
    AST   ast   = {};
    Lexer lexer = LexString(input);
    
    if (input.size)
    {
        Parser_State state = {};
        
        state.lexer = &lexer;
        state.ast   = &ast;
        
        state.ast->global_scope.id = AST_GLOBAL_SCOPE_ID;
        state.ast->local_scope.id  = GetNewScopeID();
        
        AST_Statement** current_global_statement = &state.ast->global_scope.statements;
        AST_Statement** current_local_statement  = &state.ast->local_scope.statements;
        
        if (ParseDeclarationScope(state, current_global_statement, current_local_statement, current_global_statement))
        {
            if (state.lexer->input.size == 0)
            {
                ast.is_valid = true;
            }
            
            else
            {
                //// ERROR: Encountered an unmatched closing brace in parsing of global declaration scope
            }
        }
    }
    
    else
    {
        ast.is_valid = true;
    }
    
    return ast;
}