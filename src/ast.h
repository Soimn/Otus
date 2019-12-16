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

// TODO(soimn): How should types in the parsing stage be handled?
struct AST_Type
{
    U64 value;
};

typedef U64 AST_Scope_ID;

struct AST_Node
{
    Enum32(AST_NODE_TYPE) node_type;
    Enum32(AST_EXPRESSION_TYPE) expr_type;
    
    AST_Node* next;
    String name;
    
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
            AST_Node* call_function;
            AST_Node* call_arguments;
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
        
        String identifier;
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

// NOTE(soimn): This would be a great example of a function that would benefit of being locally scoped
inline void
FlattenAST_FlattenASTNode(Bucket_Array* array, AST_Node* node)
{
    if (node->node_type == ASTNode_Scope)
    {
        AST_Node* prev_scan = 0;
        AST_Node* scan = node->scope;
        
        while (scan != 0)
        {
            AST_Node* new_node = (AST_Node*)PushElement(array);
            *new_node = *scan;
            
            if (prev_scan)
            {
                prev_scan->next = scan;
            }
            
            prev_scan = scan;
            scan = scan->next;
        }
        
        scan = node->scope;
        
        while (scan != 0)
        {
            FlattenAST_FlattenASTNode(array, scan);
            
            scan = scan->next;
        }
    }
    
    else
    {
        for (U32 i = 0; i < ARRAY_COUNT(node->children); ++i)
        {
            if (node->children[i] != 0)
            {
                AST_Node* new_node = (AST_Node*)PushElement(array);
                *new_node = *node->children[i];
                
                node->children[i] = new_node;
            }
        }
        
        for (U32 i = 0; i < ARRAY_COUNT(node->children); ++i)
        {
            if (node->children[i] != 0)
            {
                FlattenAST_FlattenASTNode(array, node->children[i]);
            }
        }
    }
}

inline void
FlattenAST(AST* ast)
{
    U32 new_bucket_size = ast->container.bucket_size;
    Bucket_Array new_storage = BUCKET_ARRAY(AST_Node, new_bucket_size);
    
    AST_Node* new_root = (AST_Node*)PushElement(&new_storage);
    *new_root = *ast->root;
    
    FlattenAST_FlattenASTNode(&new_storage, new_root);
    
    Bucket_Array test = BUCKET_ARRAY(int, 8);
    for (U32 i = 0; i < 8; ++i)
    {
        *(int*)PushElement(&test) = U32_MAX;
    }
    
    ClearArray(&test);
    
    ast->container = new_storage;
}

inline void
DEBUGDumpASTToFile(AST* ast, const char* file_name)
{
    FILE* file = fopen(file_name, "wb");
    
    U32 magic_value = 0x00FF00DD;
    U32 node_count  = (U32)ElementCount(&ast->container);
    
    fwrite(&magic_value, 1, 4, file);
    fwrite(&node_count,  1, 4, file);
    fwrite(&node_count,  1, 4, file);
    
    struct Node_Format
    {
        U32 tag_color;
        U32 text_index;
        U32 first_child;
        U32 num_children;
        B32 is_small;
    };
    
    NOT_IMPLEMENTED;
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