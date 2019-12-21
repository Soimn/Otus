#pragma once

#include "common.h"

#define AST_NODE_BUCKET_SIZE 64
#define AST_GLOBAL_SCOPE_ID 0
#define AST_FIRST_VALID_NONSPECIAL_SCOPE_ID 1

enum AST_NODE_TYPE
{
    ASTNode_Invalid,
    
    ASTNode_Expression,
    
    ASTNode_Scope,
    
    ASTNode_If,
    ASTNode_While,
    ASTNode_Defer,
    ASTNode_Return,
    
    ASTNode_VarDecl,
    ASTNode_StructDecl,
    ASTNode_EnumDecl,
    ASTNode_ConstDecl,
    ASTNode_FuncDecl,
    
    AST_NODE_TYPE_COUNT
};

enum AST_EXPRESSION_TYPE
{
    ASTExpr_Invalid,
    
    // Unary
    ASTExpr_UnaryPlus,
    ASTExpr_UnaryMinus,
    ASTExpr_PreIncrement,
    ASTExpr_PreDecrement,
    ASTExpr_PostIncrement,
    ASTExpr_PostDecrement,
    
    ASTExpr_BitwiseNot,
    
    ASTExpr_LogicalNot,
    
    ASTExpr_Reference,
    ASTExpr_Dereference,
    
    // Binary
    ASTExpr_Addition,
    ASTExpr_Subtraction,
    ASTExpr_Multiplication,
    ASTExpr_Division,
    ASTExpr_Modulus,
    
    ASTExpr_BitwiseAnd,
    ASTExpr_BitwiseOr,
    ASTExpr_BitwiseXOR,
    ASTExpr_BitwiseLeftShift,
    ASTExpr_BitwiseRightShift,
    
    ASTExpr_LogicalAnd,
    ASTExpr_LogicalOr,
    
    ASTExpr_AddEquals,
    ASTExpr_SubEquals,
    ASTExpr_MultEquals,
    ASTExpr_DivEquals,
    ASTExpr_ModEquals,
    ASTExpr_BitwiseAndEquals,
    ASTExpr_BitwiseOrEquals,
    ASTExpr_BitwiseXOREquals,
    ASTExpr_BitwiseLeftShiftEquals,
    ASTExpr_BitwiseRightShiftEquals,
    
    ASTExpr_Equals,
    
    ASTExpr_IsEqual,
    ASTExpr_IsNotEqual,
    ASTExpr_IsGreaterThan,
    ASTExpr_IsLessThan,
    ASTExpr_IsGreaterThanOrEqual,
    ASTExpr_IsLessThanOrEqual,
    
    ASTExpr_Subscript,
    ASTExpr_Member,
    
    // Special
    ASTExpr_Ternary,
    ASTExpr_Identifier,
    ASTExpr_NumericLiteral,
    ASTExpr_StringLiteral,
    ASTExpr_LambdaDecl,
    ASTExpr_FunctionCall,
    ASTExpr_TypeCast,
    
    AST_EXPRESSION_TYPE_COUNT
};

union AST_String
{
    U64 id;
    String string;
};

typedef AST_String AST_Type;

typedef U64 AST_Scope_ID;

struct AST_Node
{
    Enum32(AST_NODE_TYPE) node_type;
    Enum32(AST_EXPRESSION_TYPE) expr_type;
    
    AST_Node* next;
    AST_String name;
    
    union
    {
        AST_Node* children[3];
        
        /// Binary operators
        struct
        {
            AST_Node* left;
            AST_Node* right;
        };
        
        /// Unary operators
        AST_Node* operand;
        
        /// Function and lambda declaration
        struct
        {
            AST_Node* arguments;
            AST_Node* body;
        };
        
        /// Struct and enums
        struct
        {
            AST_Node* members;
        };
        
        /// Function call
        struct
        {
            AST_Node* call_arguments;
            AST_Node* call_function;
        };
        
        /// Variable and constant declaration, return statement, enum member, struct member, function argument
        struct
        {
            AST_Node* value;
        };
        
        /// If, ternary and while
        struct
        {
            AST_Node* condition;
            AST_Node* true_body;
            AST_Node* false_body;
        };
        
        /// Cast
        struct
        {
            AST_Node* to_cast;
        };
        
        /// Defer
        struct
        {
            AST_Node* statement;
        };
        
        AST_Node* scope;
    };
    
    union
    {
        /// Function and lambda declaration
        struct
        {
            AST_Type return_type;
            // modifiers
        };
        
        /// Enums, cast and variable declaration
        AST_Type type;
        
        AST_String identifier;
        String string_literal;
        Number numeric_literal;
        
        /// Scope
        AST_Scope_ID scope_id;
    };
};

struct AST
{
    AST_Node* root;
    Bucket_Array container;
    bool is_refined;
    bool is_valid;
};

inline AST_Node*
PushNode(AST* ast)
{
    AST_Node* node = (AST_Node*)PushElement(&ast->container);
    
    ZeroStruct(node);
    
    return node;
}

inline AST_Scope_ID
GetNewScopeID()
{
    static AST_Scope_ID current_id = AST_FIRST_VALID_NONSPECIAL_SCOPE_ID;
    return current_id++;
}

/*

Function declaration syntax
// FunctionName :: proc(parameter_name_0: parameter_type_0 ... ) -> return_type {}

Struct declaration
// Struct_Name :: struct {}

Constant declaration
// ConstantName :: VALUE


Lambdas
// proc(f: float) -> float

//
//  func := proc(f: float) -> float
//  {
//       return f * f;
//  };

Operators

// the usual: + += - -= * *= / /= [] () ! > < <= >= == != & && | || << >> %
// reference: &
// dereference: *
// member: . (automatically dereferences)

// bitwise not, xor, nand, nor, etc?

*/

/*
/// Pointers are memory pointers that can store any address (also addresses that are illegal to access)
/// References are memory pointers that can only store valid addresses


// 1
pointer_to_value:   & AST_Node;
reference_to_value: * AST_Node;

// 2
pointer_to_value:   * AST_Node;
reference_to_value: & AST_Node;

/// Usage
value := 2;

pointer_to_value = &value;
assert(value == *pointer_to_value);

reference_to_value = &value;
assert(value == *reference_to_value);

// Usage with syntax nr. 1
Function :: (reference: * AST_Node) // asserts that any pointer passed is non null
{
    if (reference); // allways true
    else;           // allways false
}

Function(0); // compile time error
Function(1); // valid but requires explicit conversion from int to pointer

Function(pointer_to_value); // runtime check if enabled
Function(reference_to_value); // No check
*/