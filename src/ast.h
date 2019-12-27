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

union AST_Type
{
    U64 id;
    String string;
};

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

inline void
PrintASTNodeAndChildren(AST_Node* node, U32 indentation_level = 0)
{
    for (U32 i = 0; i < indentation_level; ++i)
    {
        PrintChar('\t');
    }
    
    if (node->node_type != ASTNode_Expression)
    {
        String string_to_print = {};
        
        switch (node->node_type)
        {
            case ASTNode_Invalid:    string_to_print = CONST_STRING("ASTNode_Invalid");    break;
            case ASTNode_Scope:      string_to_print = CONST_STRING("ASTNode_Scope");      break;
            case ASTNode_If:         string_to_print = CONST_STRING("ASTNode_If");         break;
            case ASTNode_While:      string_to_print = CONST_STRING("ASTNode_While");      break;
            case ASTNode_Defer:      string_to_print = CONST_STRING("ASTNode_Defer");      break;
            case ASTNode_Return:     string_to_print = CONST_STRING("ASTNode_Return");     break;
            case ASTNode_VarDecl:    string_to_print = CONST_STRING("ASTNode_VarDecl");    break;
            case ASTNode_StructDecl: string_to_print = CONST_STRING("ASTNode_StructDecl"); break;
            case ASTNode_EnumDecl:   string_to_print = CONST_STRING("ASTNode_EnumDecl");   break;
            case ASTNode_ConstDecl:  string_to_print = CONST_STRING("ASTNode_ConstDecl");  break;
            case ASTNode_FuncDecl:   string_to_print = CONST_STRING("ASTNode_FuncDecl");   break;
            INVALID_DEFAULT_CASE;
        }
        
        
        Print("%S", string_to_print);
    }
    
    else
    {
        String string_to_print = {};
        
        switch (node->expr_type)
        {
            case ASTExpr_Invalid:         string_to_print = CONST_STRING("ASTExpr_Invalid");         break;
            case ASTExpr_UnaryPlus:       string_to_print = CONST_STRING("ASTExpr_UnaryPlus");       break;
            case ASTExpr_UnaryMinus:      string_to_print = CONST_STRING("ASTExpr_UnaryMinus");      break;
            case ASTExpr_PreIncrement:    string_to_print = CONST_STRING("ASTExpr_PreIncrement");    break;
            case ASTExpr_PreDecrement:    string_to_print = CONST_STRING("ASTExpr_PreDecrement");    break;
            case ASTExpr_PostIncrement:   string_to_print = CONST_STRING("ASTExpr_PostIncrement");   break;
            case ASTExpr_PostDecrement:   string_to_print = CONST_STRING("ASTExpr_PostDecrement");   break;
            case ASTExpr_BitwiseNot:      string_to_print = CONST_STRING("ASTExpr_BitwiseNot");      break;
            case ASTExpr_LogicalNot:      string_to_print = CONST_STRING("ASTExpr_LogicalNot");      break;
            case ASTExpr_Reference:       string_to_print = CONST_STRING("ASTExpr_Reference");       break;
            case ASTExpr_Dereference:     string_to_print = CONST_STRING("ASTExpr_Dereference");     break;
            case ASTExpr_Addition:        string_to_print = CONST_STRING("ASTExpr_Addition");        break;
            case ASTExpr_Subtraction:     string_to_print = CONST_STRING("ASTExpr_Subtraction");     break;
            case ASTExpr_Multiplication:  string_to_print = CONST_STRING("ASTExpr_Multiplication");  break;
            case ASTExpr_Division:        string_to_print = CONST_STRING("ASTExpr_Division");        break;
            case ASTExpr_Modulus:         string_to_print = CONST_STRING("ASTExpr_Modulus");         break;
            case ASTExpr_BitwiseAnd:      string_to_print = CONST_STRING("ASTExpr_BitwiseAnd");      break;
            case ASTExpr_BitwiseOr:       string_to_print = CONST_STRING("ASTExpr_BitwiseOr");       break;
            case ASTExpr_BitwiseXOR:      string_to_print = CONST_STRING("ASTExpr_BitwiseXOR");      break;
            case ASTExpr_LogicalAnd:      string_to_print = CONST_STRING("ASTExpr_LogicalAnd");      break;
            case ASTExpr_LogicalOr:       string_to_print = CONST_STRING("ASTExpr_LogicalOr");       break;
            case ASTExpr_AddEquals:       string_to_print = CONST_STRING("ASTExpr_AddEquals");       break;
            case ASTExpr_SubEquals:       string_to_print = CONST_STRING("ASTExpr_SubEquals");       break;
            case ASTExpr_MultEquals:      string_to_print = CONST_STRING("ASTExpr_MultEquals");      break;
            case ASTExpr_DivEquals:       string_to_print = CONST_STRING("ASTExpr_DivEquals");       break;
            case ASTExpr_ModEquals:       string_to_print = CONST_STRING("ASTExpr_ModEquals");       break;
            case ASTExpr_BitwiseOrEquals: string_to_print = CONST_STRING("ASTExpr_BitwiseOrEquals"); break;
            case ASTExpr_Equals:          string_to_print = CONST_STRING("ASTExpr_Equals");          break;
            case ASTExpr_IsGreaterThan:   string_to_print = CONST_STRING("ASTExpr_IsGreaterThan");   break;
            case ASTExpr_IsEqual:         string_to_print = CONST_STRING("ASTExpr_IsEqual");         break;
            case ASTExpr_IsNotEqual:      string_to_print = CONST_STRING("ASTExpr_IsNotEqual");      break;
            case ASTExpr_IsLessThan:      string_to_print = CONST_STRING("ASTExpr_IsLessThan");      break;
            case ASTExpr_Subscript:       string_to_print = CONST_STRING("ASTExpr_Subscript");       break;
            case ASTExpr_Member:          string_to_print = CONST_STRING("ASTExpr_Member");          break;
            case ASTExpr_Ternary:         string_to_print = CONST_STRING("ASTExpr_Ternary");         break;
            case ASTExpr_NumericLiteral:  string_to_print = CONST_STRING("ASTExpr_NumericLiteral");  break;
            case ASTExpr_Identifier:      string_to_print = CONST_STRING("ASTExpr_Identifier");      break;
            case ASTExpr_StringLiteral:   string_to_print = CONST_STRING("ASTExpr_StringLiteral");   break;
            case ASTExpr_LambdaDecl:      string_to_print = CONST_STRING("ASTExpr_LambdaDecl");      break;
            case ASTExpr_FunctionCall:    string_to_print = CONST_STRING("ASTExpr_FunctionCall");    break;
            case ASTExpr_TypeCast:        string_to_print = CONST_STRING("ASTExpr_TypeCast");        break;
            
            case ASTExpr_BitwiseAndEquals:  string_to_print = CONST_STRING("ASTExpr_BitwiseAndEquals");  break;
            case ASTExpr_BitwiseXOREquals:  string_to_print = CONST_STRING("ASTExpr_BitwiseXOREquals");  break;
            case ASTExpr_BitwiseLeftShift:  string_to_print = CONST_STRING("ASTExpr_BitwiseLeftShift");  break;
            case ASTExpr_BitwiseRightShift: string_to_print = CONST_STRING("ASTExpr_BitwiseRightShift"); break;
            case ASTExpr_IsLessThanOrEqual: string_to_print = CONST_STRING("ASTExpr_IsLessThanOrEqual"); break;
            
            case ASTExpr_IsGreaterThanOrEqual:
            string_to_print = CONST_STRING("ASTExpr_IsGreaterThanOrEqual");
            break;
            
            case ASTExpr_BitwiseLeftShiftEquals:
            string_to_print = CONST_STRING("ASTExpr_BitwiseLeftShiftEquals");
            break;
            
            case ASTExpr_BitwiseRightShiftEquals:
            string_to_print = CONST_STRING("ASTExpr_BitwiseRightShiftEquals"); 
            break;
            
        }
        
        Print("%S", string_to_print);
    }
    
    PrintChar('\n');
    
    if (node->node_type == ASTNode_Scope)
    {
        AST_Node* scan = node->scope;
        
        while (scan)
        {
            PrintASTNodeAndChildren(scan, indentation_level + 1);
            scan = scan->next;
        }
    }
    
    else
    {
        for (U32 i = 0; i < ARRAY_COUNT(node->children); ++i)
        {
            if (node->children[i] != 0)
            {
                PrintASTNodeAndChildren(node->children[i], indentation_level + 1);
            }
        }
    }
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