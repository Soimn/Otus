#pragma once

#include "common.h"

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

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

struct Parser_AST_Node
{
    Enum32(AST_NODE_TYPE) node_type;
    Enum32(AST_EXPRESSION_TYPE) expr_type;
    
    Parser_AST_Node* next;
    String name;
    
    union
    {
        Parser_AST_Node* children[3];
        
        /// Binary operators
        struct
        {
            Parser_AST_Node* left;
            Parser_AST_Node* right;
        };
        
        /// Unary operators
        Parser_AST_Node* operand;
        
        /// Function and lambda declaration
        struct
        {
            Parser_AST_Node* arguments;
            Parser_AST_Node* body;
        };
        
        /// Struct and enums
        struct
        {
            Parser_AST_Node* members;
        };
        
        /// Function call
        struct
        {
            Parser_AST_Node* call_arguments;
            Parser_AST_Node* call_function;
        };
        
        /// Variable and constant declaration, return statement, enum member, struct member, function argument
        struct
        {
            Parser_AST_Node* value;
        };
        
        /// If, ternary and while
        struct
        {
            Parser_AST_Node* condition;
            Parser_AST_Node* true_body;
            Parser_AST_Node* false_body;
        };
        
        /// Defer
        Parser_AST_Node* statement;
        
        Parser_AST_Node* scope;
    };
    
    union
    {
        /// Function and lambda declaration
        struct
        {
            String return_type_string;
            // modifiers
        };
        
        /// Enums, cast and variable declaration
        String type_string;
        
        String identifier;
        String string_literal;
        Number numeric_literal;
        
        /// Scope
        AST_Scope_ID scope_id;
    };
};

struct Parser_AST
{
    Parser_AST_Node* root;
    Bucket_Array container;
};

inline Parser_AST_Node*
PushNode(Parser_AST* ast)
{
    Parser_AST_Node* node = (Parser_AST_Node*)PushElement(&ast->container);
    
    ZeroStruct(node);
    
    return node;
}

inline AST_Scope_ID
GetNewScopeID(Module* module)
{
    return module->total_scope_count++;
}

inline void
PrintASTNodeAndChildren(Parser_AST_Node* node, U32 indentation_level = 0)
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
        Parser_AST_Node* scan = node->scope;
        
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

inline void
PrintASTExpressionAsSourceCode(Parser_AST_Node* expression)
{
    void PrintASTNodeAndChildrenAsSourceCode(Parser_AST_Node* node, bool is_root = false, String separation_string = CONST_STRING("\n"), U32 indentation_level = 0, bool should_indent = true);
    
    switch (expression->expr_type)
    {
        case ASTExpr_UnaryPlus:    PrintChar('+'); break;
        case ASTExpr_UnaryMinus:   PrintChar('-'); break;
        case ASTExpr_PreIncrement: Print("++");    break;
        case ASTExpr_PreDecrement: Print("--");    break;
        case ASTExpr_BitwiseNot:   PrintChar('~'); break;
        case ASTExpr_LogicalNot:   PrintChar('!'); break;
        case ASTExpr_Reference:    PrintChar('&'); break;
        case ASTExpr_Dereference:  PrintChar('*'); break;
        
        case ASTExpr_PostIncrement:
        case ASTExpr_PostDecrement:
        {
            PrintASTExpressionAsSourceCode(expression->operand);
            Print("%s", (expression->expr_type == ASTExpr_PostIncrement ? "++" : "--"));
        } break;
        
        case ASTExpr_Addition:
        case ASTExpr_Subtraction:
        case ASTExpr_Multiplication:
        case ASTExpr_Division:
        case ASTExpr_Modulus:
        case ASTExpr_BitwiseAnd:
        case ASTExpr_BitwiseOr:
        case ASTExpr_BitwiseXOR:
        case ASTExpr_BitwiseLeftShift:
        case ASTExpr_BitwiseRightShift:
        case ASTExpr_LogicalAnd:
        case ASTExpr_LogicalOr:
        case ASTExpr_AddEquals:
        case ASTExpr_SubEquals:
        case ASTExpr_MultEquals:
        case ASTExpr_DivEquals:
        case ASTExpr_ModEquals:
        case ASTExpr_BitwiseAndEquals:
        case ASTExpr_BitwiseOrEquals:
        case ASTExpr_BitwiseXOREquals:
        case ASTExpr_BitwiseLeftShiftEquals:
        case ASTExpr_BitwiseRightShiftEquals:
        case ASTExpr_Equals:
        case ASTExpr_IsEqual:
        case ASTExpr_IsNotEqual:
        case ASTExpr_IsGreaterThan:
        case ASTExpr_IsLessThan:
        case ASTExpr_IsGreaterThanOrEqual:
        case ASTExpr_IsLessThanOrEqual:
        {
            if (expression->left->expr_type != ASTExpr_NumericLiteral) PrintChar('(');
            PrintASTExpressionAsSourceCode(expression->left);
            if (expression->left->expr_type != ASTExpr_NumericLiteral) PrintChar(')');
            PrintChar(' ');
            
            switch (expression->expr_type)
            {
                case ASTExpr_Addition:                PrintChar('+'); break;
                case ASTExpr_Subtraction:             PrintChar('-'); break;
                case ASTExpr_Multiplication:          PrintChar('*'); break;
                case ASTExpr_Division:                PrintChar('/'); break;
                case ASTExpr_Modulus:                 PrintChar('%'); break;
                case ASTExpr_BitwiseAnd:              PrintChar('&'); break;
                case ASTExpr_BitwiseOr:               PrintChar('|'); break;
                case ASTExpr_BitwiseXOR:              PrintChar('^'); break;
                case ASTExpr_BitwiseLeftShift:        Print("<<");    break;
                case ASTExpr_BitwiseRightShift:       Print(">>");    break;
                case ASTExpr_LogicalAnd:              Print("&&");    break;
                case ASTExpr_LogicalOr:               Print("||");    break;
                case ASTExpr_AddEquals:               Print("+=");    break;
                case ASTExpr_SubEquals:               Print("-=");    break;
                case ASTExpr_MultEquals:              Print("*=");    break;
                case ASTExpr_DivEquals:               Print("/=");    break;
                case ASTExpr_ModEquals:               Print("%=");    break;
                case ASTExpr_BitwiseAndEquals:        Print("&=");    break;
                case ASTExpr_BitwiseOrEquals:         Print("|=");    break;
                case ASTExpr_BitwiseXOREquals:        Print("^=");    break;
                case ASTExpr_BitwiseLeftShiftEquals:  Print("<<=");   break;
                case ASTExpr_BitwiseRightShiftEquals: Print(">>=");   break;
                case ASTExpr_Equals:                  PrintChar('='); break;
                case ASTExpr_IsEqual:                 Print("==");    break;
                case ASTExpr_IsNotEqual:              Print("!=");    break;
                case ASTExpr_IsGreaterThan:           PrintChar('>'); break;
                case ASTExpr_IsLessThan:              PrintChar('<'); break;
                case ASTExpr_IsGreaterThanOrEqual:    Print(">=");    break;
                case ASTExpr_IsLessThanOrEqual:       Print("<=");    break;
            }
            
            PrintChar(' ');
            if (expression->right->expr_type != ASTExpr_NumericLiteral) PrintChar('(');
            PrintASTExpressionAsSourceCode(expression->right);
            if (expression->right->expr_type != ASTExpr_NumericLiteral) PrintChar(')');
        } break;
        
        case ASTExpr_Subscript:
        {
            PrintASTExpressionAsSourceCode(expression->left);
            PrintChar('[');
            PrintASTExpressionAsSourceCode(expression->right);
            PrintChar(']');
        } break;
        
        case ASTExpr_Member:
        {
            PrintASTExpressionAsSourceCode(expression->left);
            PrintChar('.');
            PrintASTExpressionAsSourceCode(expression->right);
        } break;
        
        case ASTExpr_Ternary:
        {
            PrintChar('(');
            PrintASTExpressionAsSourceCode(expression->condition);
            Print(" ? ");
            PrintASTExpressionAsSourceCode(expression->true_body);
            Print(" : ");
            PrintASTExpressionAsSourceCode(expression->false_body);
            PrintChar(')');
        } break;
        
        case ASTExpr_Identifier:
        {
            Print("%S", expression->identifier);
        } break;
        
        case ASTExpr_NumericLiteral:
        {
            if (expression->numeric_literal.is_integer)
            {
                Print("%U", expression->numeric_literal.u64);
            }
            
            else
            {
                Print("FLOAT");
            }
        } break;
        
        case ASTExpr_StringLiteral:
        {
            Print("\"%S\"", expression->string_literal);
        } break;
        
        case ASTExpr_LambdaDecl:
        {
            Print("proc(", expression->name);
            
            Parser_AST_Node* arg_scan = expression->arguments;
            
            while (arg_scan)
            {
                Print("%S", expression->name);
                
                if (expression->type_string.size != 0)
                {
                    Print(": %S", expression->type_string);
                }
                
                else
                {
                    Print(" :");
                }
                
                if (expression->value)
                {
                    Print("%s= ", (expression->node_type ? " " : ""));
                    PrintASTExpressionAsSourceCode(expression->value);
                }
                
                if (arg_scan->next)
                {
                    Print(", ");
                }
                
                arg_scan = arg_scan->next;
            }
            
            PrintChar(')');
            
            if (expression->return_type_string.size != 0)
            {
                Print(" -> %S", expression->return_type_string);
            }
            
            PrintChar('\n');
            
            PrintASTNodeAndChildrenAsSourceCode(expression->body, false, CONST_STRING(" "), 0, false);
        } break;
        
        case ASTExpr_FunctionCall:
        {
            PrintASTExpressionAsSourceCode(expression->call_function);
            PrintChar('(');
            
            Parser_AST_Node* arg_scan = expression->call_arguments;
            
            while (arg_scan)
            {
                PrintASTExpressionAsSourceCode(arg_scan);
                
                if (arg_scan->next)
                {
                    Print(", ");
                }
                
                arg_scan = arg_scan->next;
            }
            
            PrintChar(')');
            
        } break;
        
        case ASTExpr_TypeCast:
        {
            Print("cast(%S) ", expression->type_string);
            PrintASTExpressionAsSourceCode(expression->operand);
        } break;
    }
    
    switch (expression->expr_type)
    {
        case ASTExpr_UnaryPlus:
        case ASTExpr_UnaryMinus:
        case ASTExpr_PreIncrement:
        case ASTExpr_PreDecrement:
        case ASTExpr_BitwiseNot:
        case ASTExpr_LogicalNot:
        case ASTExpr_Reference:
        case ASTExpr_Dereference:
        {
            PrintASTExpressionAsSourceCode(expression->operand);
        } break;
    }
}

inline void
PrintASTNodeAndChildrenAsSourceCode(Parser_AST_Node* node, bool is_root = false, String separation_string = CONST_STRING("\n"), U32 indentation_level = 0, bool should_indent = true)
{
    if (should_indent)
    {
        for (U32 i = 0; i < indentation_level; ++i)
        {
            PrintChar('\t');
        }
    }
    
    if (node->node_type == ASTNode_Expression)
    {
        PrintASTExpressionAsSourceCode(node);
        PrintChar(';');
    }
    
    else if (node->node_type == ASTNode_Scope)
    {
        if (!is_root)
        {
            Print("{\n");
        }
        
        Parser_AST_Node* scan = node->scope;
        
        while (scan)
        {
            PrintASTNodeAndChildrenAsSourceCode(scan, false, separation_string, indentation_level + !is_root);
            scan = scan->next;
        }
        
        if (!is_root)
        {
            for (U32 i = 0; i < indentation_level; ++i)
            {
                PrintChar('\t');
            }
            
            PrintChar('}');
        }
    }
    
    else if (node->node_type == ASTNode_If)
    {
        Print("if (");
        PrintASTExpressionAsSourceCode(node->condition);
        Print(")\n");
        PrintASTNodeAndChildrenAsSourceCode(node->true_body, false, separation_string, indentation_level);
        
        if (node->false_body)
        {
            Print("else");
            
            if (node->false_body->node_type == ASTNode_Scope)
            {
                PrintChar('\n');
                PrintASTNodeAndChildrenAsSourceCode(node->false_body, false, separation_string, indentation_level);
            }
            
            else
            {
                PrintChar(' ');
                PrintASTNodeAndChildrenAsSourceCode(node->false_body, false, separation_string, indentation_level, false);
            }
        }
    }
    
    else if (node->node_type == ASTNode_While)
    {
        Print("while (");
        PrintASTExpressionAsSourceCode(node->condition);
        Print(")\n");
        PrintASTNodeAndChildrenAsSourceCode(node->true_body, false, separation_string, indentation_level);
    }
    
    else if (node->node_type == ASTNode_Defer)
    {
        Print("defer");
        
        if (node->statement->node_type == ASTNode_Scope)
        {
            PrintChar('\n');
            PrintASTNodeAndChildrenAsSourceCode(node->statement, false, separation_string, indentation_level);
        }
        
        else
        {
            PrintChar(' ');
            PrintASTNodeAndChildrenAsSourceCode(node->statement, false, separation_string, indentation_level, false);
        }
    }
    
    else if (node->node_type == ASTNode_Return)
    {
        Print("return ");
        PrintASTExpressionAsSourceCode(node->value);
        PrintChar(';');
    }
    
    else if (node->node_type == ASTNode_VarDecl)
    {
        Print("%S", node->name);
        
        if (node->type_string.size != 0)
        {
            Print(": %S", node->type_string);
        }
        
        else
        {
            Print(" :");
        }
        
        if (node->value)
        {
            Print("%s= ", (node->node_type ? " " : ""));
            PrintASTExpressionAsSourceCode(node->value);
        }
        
        PrintChar(';');
    }
    
    else if (node->node_type == ASTNode_StructDecl)
    {
        Print("%S :: struct\n", node->name);
        
        for (U32 i = 0; i < indentation_level; ++i)
        {
            PrintChar('\t');
        }
        
        PrintChar('{');
        
        Parser_AST_Node* scan = node->members;
        
        while (scan)
        {
            for (U32 i = 0; i < indentation_level + 1; ++i)
            {
                PrintChar('\t');
            }
            
            Print("%S", node->name);
            
            if (node->type_string.size != 0)
            {
                Print(": %S", node->type_string);
            }
            
            else
            {
                Print(" :");
            }
            
            if (node->value)
            {
                Print("%s= ", (node->node_type ? " " : ""));
                PrintASTExpressionAsSourceCode(node->value);
            }
            
            Print(";\n");
            
            scan = scan->next;
        }
        
        for (U32 i = 0; i < indentation_level; ++i)
        {
            PrintChar('\t');
        }
        
        PrintChar('}');
    }
    
    else if (node->node_type == ASTNode_EnumDecl)
    {
        Print("%S :: enum");
        
        if (node->type_string.size != 0)
        {
            Print(" %S ", node->type_string);
        }
        
        Print("\n");
        
        for (U32 i = 0; i < indentation_level; ++i)
        {
            PrintChar('\t');
        }
        
        PrintChar('{');
        
        Parser_AST_Node* scan = node->members;
        
        while (scan)
        {
            for (U32 i = 0; i < indentation_level + 1; ++i)
            {
                PrintChar('\t');
            }
            
            Print("%S", scan->name);
            
            if (node->value)
            {
                Print(" = ");
                PrintASTExpressionAsSourceCode(node->value);
            }
            
            if (scan->next)
            {
                PrintChar(',');
            }
            
            PrintChar('\n');
            
            scan = scan->next;
        }
        
        for (U32 i = 0; i < indentation_level; ++i)
        {
            PrintChar('\t');
        }
        
        PrintChar('}');
    }
    
    else if (node->node_type == ASTNode_ConstDecl)
    {
        Print("%S :: ", node->name);
        PrintASTExpressionAsSourceCode(node->value);
        PrintChar(';');
    }
    
    else if (node->node_type == ASTNode_FuncDecl)
    {
        Print("%S :: proc(", node->name);
        
        Parser_AST_Node* arg_scan = node->arguments;
        
        while (arg_scan)
        {
            Print("%S", arg_scan->name);
            
            if (arg_scan->type_string.size != 0)
            {
                Print(": %S", arg_scan->type_string);
            }
            
            else
            {
                Print(" :");
            }
            
            if (arg_scan->value)
            {
                Print("%s= ", (arg_scan->node_type ? " " : ""));
                PrintASTExpressionAsSourceCode(arg_scan->value);
            }
            
            if (arg_scan->next)
            {
                Print(", ");
            }
            
            arg_scan = arg_scan->next;
        }
        
        PrintChar(')');
        
        if (node->return_type_string.size != 0)
        {
            Print(" -> %S", node->return_type_string);
        }
        
        PrintChar('\n');
        
        PrintASTNodeAndChildrenAsSourceCode(node->body, false, separation_string, indentation_level);
    }
    
    Print("%S", separation_string);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

#define SEMA_AST_LOCAL_SYMBOL_TABLE_STORAGE_SIZE 4

struct Sema_AST_Node
{
    Sema_AST_Node* next;
    
    Enum32(AST_NODE_TYPE) node_type;
    Enum32(AST_EXPRESSION_TYPE) expr_type;
    
    union
    {
        Sema_AST_Node* children[3];
        
        /// Binary operators
        struct
        {
            Sema_AST_Node* left;
            Sema_AST_Node* right;
        };
        
        /// Unary operators
        Sema_AST_Node* operand;
        
        /// Ternary, if and while
        struct
        {
            Sema_AST_Node* condition;
            Sema_AST_Node* true_body;
            Sema_AST_Node* false_body;
        };
        
        /// Function call
        struct
        {
            Sema_AST_Node* call_function;
            Sema_AST_Node* call_arguments;
        };
        
        /// Function and lambda declaration
        struct
        {
            Sema_AST_Node* function_arguments;
            Sema_AST_Node* function_body;
        };
        
        /// Variable and constant declaration, return statement, enum member, struct member, function argument
        Sema_AST_Node* value;
        
        /// Struct and enum declaration
        Sema_AST_Node* members;
        
        /// Defer
        Sema_AST_Node* statement;
        
        Sema_AST_Node* scope;
    };
    
    union
    {
        /// Function call
        Symbol_ID call_function_name;
        
        /// Function and lambda declaration
        struct
        {
            Symbol_ID function_name;
            Type_ID function_type;
            Type_ID function_return_type;
            U32 function_argument_count;
        };
        
        /// Variable and constant declaration
        struct
        {
            Symbol_ID var_name;
            Type_ID var_type;
        };
        
        /// Struct declaration
        struct
        {
            Symbol_ID struct_name;
            Type_ID struct_type;
        };
        
        struct
        {
            Symbol_ID enum_name;
            Type_ID enum_type;
            Type_ID enum_member_type;
        };
        
        /// Cast
        Type_ID target_type;
        
        Symbol_ID identifier;
        
        String string_literal;
        Number numeric_literal;
        
        /// Scope
        struct
        {
            AST_Scope_ID scope_id;
            Symbol_Table_ID first_few_symbol_tables[SEMA_AST_LOCAL_SYMBOL_TABLE_STORAGE_SIZE];
            Bucket_Array remaining_symbol_tables;
            U32 total_symbol_table_count;
            U32 statement_count;
        };
    };
};

struct Sema_AST
{
    Sema_AST_Node* root;
    Static_Array container;
};

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