#pragma once

enum AST_EXPRESSION_TYPE
{
    ASTExp_Addition,
    ASTExp_Subtraction,
    ASTExp_Multiplication,
    ASTExp_Division,
    ASTExp_Modulo,
    
};

struct AST_Expression
{
    Enum64(AST_EXPRESSION_TYPE) type;
    
    AST_Expression* middle; // NOTE(soimn): This is used when dealing with trinary operators
    AST_Expression* left;
    AST_Expression* right;
};