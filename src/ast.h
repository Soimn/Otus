#pragma once

#include "common.h"

// TODO(soimn): At the moment only cast(TYPE) is allowed. Should something like cast(typeof(var)) be supported?

enum AST_EXPRESSION_TYPE
{
    ASTExp_Invalid,
    
    /// Arithmetic
    ASTExp_Plus,
    ASTExp_UnaryPlus,
    ASTExp_PreIncrement,
    ASTExp_PostIncrement,
    ASTExp_Minus,
    ASTExp_UnaryMinus,
    ASTExp_PreDecrement,
    ASTExp_PostDecrement,
    ASTExp_Multiply,
    ASTExp_Divide,
    ASTExp_Modulus,
    
    /// Assignment - arithmetic
    ASTExp_PlusEquals,
    ASTExp_MinusEquals,
    ASTExp_MultiplyEquals,
    ASTExp_DivideEquals,
    ASTExp_ModulusEquals,
    
    /// Logical
    ASTExp_LogicalNot,
    ASTExp_LogicalAnd,
    ASTExp_LogicalOr,
    
    /// Comparative
    ASTExp_IsEqual,
    ASTExp_IsNotEqual,
    ASTExp_IsGreaterThan,
    ASTExp_IsGreaterThanOrEqual,
    ASTExp_IsLessThan,
    ASTExp_IsLessThanOrEqual,
    
    /// Bitwise
    ASTExp_Not,
    ASTExp_And,
    ASTExp_Or,
    ASTExp_XOR,
    ASTExp_LeftShift,
    ASTExp_RightShift,
    
    /// Assignment - bitwise
    ASTExp_AndEquals,
    ASTExp_OrEquals,
    ASTExp_XOREquals,
    ASTExp_LeftShiftEquals,
    ASTExp_RightShiftEquals,
    
    /// Assignment
    ASTExp_Equals,
    
    /// Literals
    ASTExp_NumericLiteral,
    ASTExp_StringLiteral,
    
    /// Special
    ASTExp_Lambda, // TODO(soimn): Where should lamdas be defined, and how should duplicates be handled?
    ASTExp_FunctionCall,
    ASTExp_Subscript,
    ASTExp_Member,
    ASTExp_Ternary,
    ASTExp_Dereference,
    ASTExp_Reference,
    ASTExp_TypeCast,
    ASTExp_Identifier,
};

struct AST_Type
{
    bool is_function_type;
    
    AST_Type* next;
    
    union
    {
        /// is_function_type : false
        struct
        {
            // TODO(soimn): Some sort of abstraction of an ordered sequence of '&' and '[' EXPRESSION ']'
            String type_name;
        };
        
        /// is_function_type : true
        struct
        {
            AST_Type* first_argument;
            AST_Type* return_type;
        };
    };
};

struct AST_Expression
{
    Enum32(AST_EXPRESSION_TYPE) type;
    
    union
    {
        /// All binary operations
        struct
        {
            AST_Expression* left;
            AST_Expression* right;
        };
        
        /// All unary operations
        AST_Expression* operand;
        
        /// Numeric literal
        Number number;
        
        /// String literal
        String string;
        
        /// Lambda
        struct
        {
            AST_Expression* lambda_arguments;
            AST_Type lambda_return_type;
            AST_Expression* lambda_capture_list;
            struct AST_Scope* lambda_body;
        };
        
        /// Function call
        struct
        {
            AST_Expression* function_pointer;
            AST_Expression* function_arguments;
        };
        
        /// Ternary
        struct
        {
            AST_Expression* condition;
            AST_Expression* true_expr;
            AST_Expression* false_expr;
        };
        
        /// Type cast
        struct
        {
            // TODO(soimn): This prevents cast(typeof(var)) and should maybe be altered once the parser is more
            //              complete / defined
            AST_Type target_type;
            
            AST_Expression* to_cast;
        };
        
        /// Identifier
        String identifier;
    };
};

typedef U64 AST_Scope_ID;
#define AST_INVALID_SCOPE_ID 0
#define AST_GLOBAL_SCOPE_ID  1

struct AST_Scope
{
    AST_Scope_ID id;
    struct AST_Statement* statements;
};

enum AST_STATEMENT_TYPE
{
    ASTStmt_Invalid,
    
    ASTStmt_Expression,
    ASTStmt_If,
    ASTStmt_While,
    ASTStmt_Defer,
    ASTStmt_Return,
    
    ASTStmt_VarDecl,
    ASTStmt_FuncDecl,
    ASTStmt_ConstDecl,
    ASTStmt_StructDecl,
    
    ASTStmt_Scope,
};

struct AST_Statement
{
    Enum32(AST_STATEMENT_TYPE) type;
    
    AST_Statement* next;
    
    union
    {
        /// Expression
        AST_Expression* expression;
        
        /// If and while
        struct
        {
            AST_Expression* condition;
            AST_Scope true_body;
            AST_Statement* false_statement;
        };
        
        /// Defer
        AST_Scope defer_scope;
        
        /// Return
        AST_Expression* return_value;
        
        /// Variable declaration
        struct
        {
            String var_name;
            AST_Type var_type;
            AST_Expression* var_value;
            bool var_is_uninitialized;
        };
        
        /// Function declaration
        struct
        {
            String function_name;
            AST_Expression** function_arguments;
            AST_Type function_return_type;
            AST_Scope function_scope;
        };
        
        /// Constant declaration
        struct
        {
            String constant_name;
            AST_Expression* constant_value;
        };
        
        /// Struct declaration
        struct
        {
            String struct_name;
            AST_Statement* struct_members;
        };
        
        /// Scope
        AST_Scope scope;
    };
};

struct AST_Node
{
    union
    {
        AST_Expression expression;
        AST_Statement statement;
    };
    
    bool is_expression;
};

struct AST
{
    AST_Scope global_scope;
    AST_Scope local_scope;
    
    Bucket_Array container;
    
    bool is_valid;
};

inline AST_Expression*
PushExpression(AST* ast)
{
    AST_Node* node = (AST_Node*)PushElement(&ast->container);
    node->is_expression = true;
    
    return &node->expression;
}

inline AST_Statement*
PushStatement(AST* ast)
{
    AST_Node* node = (AST_Node*)PushElement(&ast->container);
    node->is_expression = false;
    
    return &node->statement;
}

inline AST_Scope_ID
GetNewScopeID()
{
    static AST_Scope_ID current_id = AST_GLOBAL_SCOPE_ID;
    
    return ++current_id;
}

inline bool
IsPostfixExpression(AST_Expression* expression)
{
    Enum32(AST_EXPRESSION_TYPE) type = expression->type;
    
    return (type == ASTExp_StringLiteral || type == ASTExp_Identifier ||
            type == ASTExp_FunctionCall  || type == ASTExp_Lambda     ||
            type == ASTExp_Subscript     || type == ASTExp_Member);
}

inline bool
IsUnaryExpression(AST_Expression* expression)
{
    Enum32(AST_EXPRESSION_TYPE) type = expression->type;
    
    return (type == ASTExp_PreIncrement || type == ASTExp_PreDecrement ||
            type == ASTExp_Reference    || type == ASTExp_Dereference  ||
            type == ASTExp_UnaryPlus    || type == ASTExp_UnaryMinus   ||
            type == ASTExp_Not          || type == ASTExp_LogicalNot);
}


/// IMPORTANT TODO(soimn): Err on all postfix expressions after lambda other than function call