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
            AST_Type* return_type;
            // modifiers
        };
        
        /// Enums, cast and variable declaration
        AST_Type* type;
        
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
#pragma warning(push)
#pragma warning(disable:4996;disable:4189) 
    
    
    U32 magic_value = 0x00FF00DD;
    U32 node_count  = (U32)ElementCount(&ast->container);
    
    struct Node_Format
    {
        U32 tag_color;
        U32 text_index;
        U32 first_child;
        U32 num_children;
        B32 is_small;
    };
    
    struct String_Format
    {
        U32 size;
        U32 offset;
    };
    
    UMM scratch_memory_size = 4 * sizeof(U32) + 2 * node_count * sizeof(Node_Format);
    void* scratch_node_memory = AllocateMemory(scratch_memory_size);
    UMM scratch_node_memory_offset = 0;
    ZeroSize(scratch_node_memory, scratch_memory_size);
    
    auto Write = [&scratch_node_memory, &scratch_node_memory_offset, scratch_memory_size] (U8* value, UMM size)
    {
        Copy(value, (U8*)scratch_node_memory + scratch_node_memory_offset, size);
        scratch_node_memory_offset += size;
        HARD_ASSERT(scratch_node_memory_offset < scratch_memory_size);
    };
    
    Bucket_Array string_storage = BUCKET_ARRAY(char, 4000);
    Bucket_Array string_array   = BUCKET_ARRAY(String_Format, 60);
    
    auto PushString = [&string_storage, &string_array](String string) -> U32
    {
        U32 index = (U32)ElementCount(&string_array);
        
        String_Format* format = (String_Format*)PushElement(&string_array);
        
        format->size = 0;
        format->offset = (U32)ElementCount(&string_storage);
        
        for (U32 i = 0; i < string.size; ++i, ++format->size)
        {
            *(char*)PushElement(&string_storage) = string.data[i];
        }
        
        return index;
    };
    
    U32 if_string        = PushString(CONST_STRING("If"));
    U32 while_string     = PushString(CONST_STRING("While"));
    U32 defer_string     = PushString(CONST_STRING("Defer"));
    U32 return_string    = PushString(CONST_STRING("Return"));
    U32 scope_string     = PushString(CONST_STRING("{...}"));
    U32 arguments_string = PushString(CONST_STRING("Args"));
    U32 lambda_string    = PushString(CONST_STRING("Lambda"));
    U32 ternary_string   = PushString(CONST_STRING("Ternary"));
    U32 cast_string      = PushString(CONST_STRING("Cast"));
    U32 call_string      = PushString(CONST_STRING("Call"));
    
    U32 expression_strings[AST_EXPRESSION_TYPE_COUNT] = {};
    
    for (U32 i = 0; i < AST_EXPRESSION_TYPE_COUNT; ++i)
    {
        switch(i)
        {
            case ASTExpr_UnaryPlus:               expression_strings[i] = PushString(CONST_STRING("Unary +"));   break;
            case ASTExpr_UnaryMinus:              expression_strings[i] = PushString(CONST_STRING("Unary -"));   break;
            case ASTExpr_PreIncrement:            expression_strings[i] = PushString(CONST_STRING("Pre  ++"));   break;
            case ASTExpr_PreDecrement:            expression_strings[i] = PushString(CONST_STRING("Pre  --"));   break;
            case ASTExpr_PostIncrement:           expression_strings[i] = PushString(CONST_STRING("Post ++"));   break;
            case ASTExpr_PostDecrement:           expression_strings[i] = PushString(CONST_STRING("Post --"));   break;
            case ASTExpr_BitwiseNot:              expression_strings[i] = PushString(CONST_STRING("~"));         break;
            case ASTExpr_LogicalNot:              expression_strings[i] = PushString(CONST_STRING("!"));         break;
            case ASTExpr_Reference:               expression_strings[i] = PushString(CONST_STRING("Ref"));       break;
            case ASTExpr_Dereference:             expression_strings[i] = PushString(CONST_STRING("Deref"));     break;
            case ASTExpr_Addition:                expression_strings[i] = PushString(CONST_STRING("+"));         break;
            case ASTExpr_Subtraction:             expression_strings[i] = PushString(CONST_STRING("-"));         break;
            case ASTExpr_Multiplication:          expression_strings[i] = PushString(CONST_STRING("*"));         break;
            case ASTExpr_Division:                expression_strings[i] = PushString(CONST_STRING("/"));         break;
            case ASTExpr_Modulus:                 expression_strings[i] = PushString(CONST_STRING("%"));         break;
            case ASTExpr_BitwiseAnd:              expression_strings[i] = PushString(CONST_STRING("&"));         break;
            case ASTExpr_BitwiseOr:               expression_strings[i] = PushString(CONST_STRING("|"));         break;
            case ASTExpr_BitwiseXOR:              expression_strings[i] = PushString(CONST_STRING("^"));         break;
            case ASTExpr_BitwiseLeftShift:        expression_strings[i] = PushString(CONST_STRING("<<"));        break;
            case ASTExpr_BitwiseRightShift:       expression_strings[i] = PushString(CONST_STRING(">>"));        break;
            case ASTExpr_LogicalAnd:              expression_strings[i] = PushString(CONST_STRING("&&"));        break;
            case ASTExpr_LogicalOr:               expression_strings[i] = PushString(CONST_STRING("||"));        break;
            case ASTExpr_AddEquals:               expression_strings[i] = PushString(CONST_STRING("+="));        break;
            case ASTExpr_SubEquals:               expression_strings[i] = PushString(CONST_STRING("-="));        break;
            case ASTExpr_MultEquals:              expression_strings[i] = PushString(CONST_STRING("*="));        break;
            case ASTExpr_DivEquals:               expression_strings[i] = PushString(CONST_STRING("/="));        break;
            case ASTExpr_ModEquals:               expression_strings[i] = PushString(CONST_STRING("%="));        break;
            case ASTExpr_BitwiseAndEquals:        expression_strings[i] = PushString(CONST_STRING("&="));        break;
            case ASTExpr_BitwiseOrEquals:         expression_strings[i] = PushString(CONST_STRING("|="));        break;
            case ASTExpr_BitwiseXOREquals:        expression_strings[i] = PushString(CONST_STRING("^="));        break;
            case ASTExpr_BitwiseLeftShiftEquals:  expression_strings[i] = PushString(CONST_STRING("<<="));       break;
            case ASTExpr_BitwiseRightShiftEquals: expression_strings[i] = PushString(CONST_STRING(">>="));       break;
            case ASTExpr_Equals:                  expression_strings[i] = PushString(CONST_STRING("="));         break;
            case ASTExpr_IsEqual:                 expression_strings[i] = PushString(CONST_STRING("=="));        break;
            case ASTExpr_IsNotEqual:              expression_strings[i] = PushString(CONST_STRING("!="));        break;
            case ASTExpr_IsGreaterThan:           expression_strings[i] = PushString(CONST_STRING(">"));         break;
            case ASTExpr_IsLessThan:              expression_strings[i] = PushString(CONST_STRING("<"));         break;
            case ASTExpr_IsGreaterThanOrEqual:    expression_strings[i] = PushString(CONST_STRING(">="));        break;
            case ASTExpr_IsLessThanOrEqual:       expression_strings[i] = PushString(CONST_STRING("<="));        break;
            case ASTExpr_Subscript:               expression_strings[i] = PushString(CONST_STRING("Subscript")); break;
            case ASTExpr_Member:                  expression_strings[i] = PushString(CONST_STRING("Member"));    break;
        }
    }
    
    U32 operator_expression_color   = 0x00AF1212;
    U32 literal_expression_color    = 0x001212AF;
    U32 identifier_expression_color = 0x001212AF;
    U32 lambda_expression_color     = 0x0012AF12;
    U32 conditional_statement_color = 0x00AFAF12;
    U32 declaration_statement_color = 0x00AF12AF;
    U32 arguments_statement_color   = 0x00FFFFFF;
    U32 misc_statement_color        = 0x00222222;
    
    B32 last_was_function            = false;
    U32 function_argument_count      = 0;
    U32 remaining_function_arguments = 0;
    
    // IMPORTANT TODO(soimn): Child offset
    for (Bucket_Array_Iterator it = Iterate(&ast->container);
         it.current;
         Advance(&it))
    {
        Node_Format format          = {};
        Node_Format argument_format = {};
        Node_Format scope_format    = {};
        
        AST_Node* node = (AST_Node*)it.current;
        
        format.first_child = (U32)it.current_index + 1;
        
        if (last_was_function)
        {
            if (remaining_function_arguments == 0)
            {
                // NOTE(soimn): Skip the scope node, since it has been 
                //              inserted by the function
                last_was_function = false;
                continue;
            }
            
            else
            {
                --remaining_function_arguments;
            }
        }
        
        if (node->node_type == ASTNode_Scope)
        {
            format.tag_color    = misc_statement_color;
            format.text_index   = scope_string;
            
            for (AST_Node* scan = node->scope; scan; )
            {
                ++format.num_children;
                scan = scan->next;
            }
        }
        
        else if (node->node_type == ASTNode_If)
        {
            format.tag_color    = conditional_statement_color;
            format.text_index   = if_string;
            format.num_children = 3;
        }
        
        else if (node->node_type == ASTNode_While)
        {
            format.tag_color    = conditional_statement_color;
            format.text_index   = while_string;
            format.num_children = 2;
        }
        
        else if (node->node_type == ASTNode_Defer)
        {
            format.tag_color    = misc_statement_color;
            format.text_index   = defer_string;
            format.num_children = 1;
        }
        
        else if (node->node_type == ASTNode_Return)
        {
            format.tag_color    = misc_statement_color;
            format.text_index   = return_string;
            format.num_children = 1;
        }
        
        else if (node->node_type == ASTNode_VarDecl)
        {
            format.tag_color     = declaration_statement_color;
            format.text_index    = PushString(node->name);
            format.num_children += (node->value != 0 ? 1 : 0);
            format.num_children += (node->type  != 0 ? 1 : 0);
        }
        
        else if (node->node_type == ASTNode_StructDecl || node->node_type == ASTNode_EnumDecl)
        {
            format.tag_color  = declaration_statement_color;
            format.text_index = PushString(node->name);
            
            for (AST_Node* scan = node->members; scan; )
            {
                ++format.num_children;
                scan = scan->next;
            }
        }
        
        else if (node->node_type == ASTNode_ConstDecl)
        {
            format.tag_color     = declaration_statement_color;
            format.text_index    = PushString(node->name);
            format.num_children += (node->value != 0 ? 1 : 0);
        }
        
        else if (node->node_type == ASTNode_FuncDecl)
        {
            format.tag_color     = declaration_statement_color;
            format.text_index    = PushString(node->name);
            format.num_children  = 2;
            
            argument_format.tag_color   = arguments_statement_color;
            argument_format.text_index  = arguments_string;
            argument_format.first_child = format.first_child + 2;
            
            for (AST_Node* scan = node->arguments; scan; )
            {
                ++argument_format.num_children;
                scan = scan->next;
            }
            
            last_was_function       = true;
            function_argument_count = argument_format.num_children;
            remaining_function_arguments = function_argument_count;
            
            scope_format.tag_color    = misc_statement_color;
            scope_format.text_index   = scope_string;
            scope_format.first_child  = argument_format.first_child + argument_format.num_children;
            
            for (AST_Node* scan = node->body; scan; )
            {
                ++scope_format.num_children;
                scan = scan->next;
            }
        }
        
        else if (node->node_type == ASTNode_Expression)
        {
            switch(node->expr_type)
            {
                case ASTExpr_NumericLiteral:
                {
                    format.tag_color   = literal_expression_color;
                    format.is_small    = true;
                    format.first_child = 0;
                    
                    Number num = node->numeric_literal;
                    
                    char buffer[20] = {};
                    U32 length      = 0;
                    
                    if (num.is_integer)
                    {
                        length = snprintf(buffer, 0, "%llu", num.u64);
                        snprintf(buffer, 20, "%llu", num.u64);
                    }
                    
                    else if (num.is_single_precision)
                    {
                        length = snprintf(buffer, 0, "%g", num.f32);
                        snprintf(buffer, 20, "%g", num.f32);
                    }
                    
                    else
                    {
                        length = snprintf(buffer, 0, "%g", num.f64);
                        snprintf(buffer, 20, "%g", num.f64);
                    }
                    
                    String string = {};
                    string.data = (U8*)buffer;
                    string.size = length;
                    
                    format.text_index = PushString(string);
                } break;
                
                case ASTExpr_Ternary:
                {
                    format.tag_color    = conditional_statement_color;
                    format.text_index   = ternary_string;
                    format.num_children = 3;
                } break;
                
                case ASTExpr_StringLiteral:
                {
                    format.tag_color  = literal_expression_color;
                    format.text_index = PushString(node->string_literal);
                } break;
                
                case ASTExpr_Identifier:
                {
                    format.tag_color  = identifier_expression_color;
                    format.text_index = PushString(node->identifier);
                } break;
                
                case ASTExpr_FunctionCall:
                {
                    // TODO(soimn): Write proper function calls
                    format.tag_color    = lambda_expression_color;
                    format.text_index   = call_string;
                } break;
                
                case ASTExpr_LambdaDecl:
                {
                    format.tag_color     = lambda_expression_color;
                    format.text_index    = lambda_string;
                    format.num_children  = 2;
                    
                    argument_format.tag_color   = arguments_statement_color;
                    argument_format.text_index  = arguments_string;
                    argument_format.first_child = format.first_child + 2;
                    
                    for (AST_Node* scan = node->arguments; scan; )
                    {
                        ++argument_format.num_children;
                        scan = scan->next;
                    }
                    
                    last_was_function       = true;
                    function_argument_count = argument_format.num_children;
                    remaining_function_arguments = function_argument_count;
                    
                    scope_format.tag_color    = misc_statement_color;
                    scope_format.text_index   = scope_string;
                    scope_format.first_child  = argument_format.first_child + argument_format.num_children;
                    
                    for (AST_Node* scan = node->body; scan; )
                    {
                        ++scope_format.num_children;
                        scan = scan->next;
                    }
                } break;
                
                case ASTExpr_TypeCast:
                {
                    format.tag_color    = lambda_expression_color;
                    format.text_index   = cast_string;
                    format.num_children = 2;
                    
                    argument_format.tag_color = conditional_statement_color;
                    argument_format.text_index = PushString(CONST_STRING("Type"));
                };
                
                default:
                {
                    format.tag_color = operator_expression_color;
                    format.num_children = (node->right ? 2 : 1);
                    format.text_index = expression_strings[node->expr_type];
                    format.is_small = true;
                } break;
            }
        }
        
        format.text_index          += 1;
        argument_format.text_index += 1;
        scope_format.text_index    += 1;
        
        Write((U8*)&format.tag_color,    4);
        Write((U8*)&format.text_index,   4);
        Write((U8*)&format.first_child,  4);
        Write((U8*)&format.num_children, 4);
        Write((U8*)&format.is_small,     4);
        
        if (argument_format.tag_color != 0)
        {
            Write((U8*)&argument_format.tag_color,    4);
            Write((U8*)&argument_format.text_index,   4);
            Write((U8*)&argument_format.first_child,  4);
            Write((U8*)&argument_format.num_children, 4);
            Write((U8*)&argument_format.is_small,     4);
        }
        
        if (scope_format.tag_color != 0)
        {
            Write((U8*)&scope_format.tag_color,    4);
            Write((U8*)&scope_format.text_index,   4);
            Write((U8*)&scope_format.first_child,  4);
            Write((U8*)&scope_format.num_children, 4);
            Write((U8*)&scope_format.is_small,     4);
        }
    }
    
    U32 string_count = (U32)ElementCount(&string_array);
    
    FILE* file = fopen("tree.bin", "wb");
    
    fwrite(&magic_value,  1, 4, file);
    fwrite(&node_count,   1, 4, file);
    fwrite(&string_count, 1, 4, file);
    fwrite(scratch_node_memory, 1, scratch_node_memory_offset, file);
    
    for (Bucket_Array_Iterator it = Iterate(&string_array);
         it.current;
         Advance(&it))
    {
        fwrite(&((String_Format*)it.current)->size,   1, 4, file);
        fwrite(&((String_Format*)it.current)->offset, 1, 4, file);
    }
    
    for (Bucket_Array_Iterator it = Iterate(&string_storage);
         it.current;
         Advance(&it))
    {
        fwrite((char*)it.current, 1, 1, file);
    }
    
    fclose(file);
    
#pragma warning(pop)
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