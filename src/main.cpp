#include "common.h"

#include "lexer.h"
#include "parser.h"
#include "ast.h"

struct DEBUG_Node_Format
{
    U32 tag_color;
    U32 text_index;
    U32 first_child;
    U32 num_children;
    B32 is_small;
};

struct DEBUG_Node_String_Format
{
    U32 size;
    U32 offset;
};

inline void
DEBUGDumpNodeToArray(AST_Node* node, DEBUG_Node_Format** format, U32 first_child, Bucket_Array* string_storage, Bucket_Array* string_array)
{
    ZeroStruct(format[0]);
    if (format[1]) ZeroStruct(format[1]);
    if (format[2]) ZeroStruct(format[1]);
    
    auto PushString = [&string_storage, &string_array](String string) -> U32
    {
        U32 index = (U32)ElementCount(string_array);
        
        DEBUG_Node_String_Format* format = (DEBUG_Node_String_Format*)PushElement(string_array);
        
        format[0].size = 0;
        format[0].offset = (U32)ElementCount(string_storage);
        
        for (U32 i = 0; i < string.size; ++i, ++format[0].size)
        {
            *(char*)PushElement(string_storage) = string.data[i];
        }
        
        return index;
    };
    
    U32 if_string      = 0;
    U32 while_string   = 1;
    U32 defer_string   = 2;
    U32 return_string  = 3;
    U32 scope_string   = 4;
    U32 args_string    = 5;
    U32 lambda_string  = 6;
    U32 ternary_string = 7;
    U32 cast_string    = 8;
    U32 call_string    = 9;
    
    U32 operator_expression_color   = 0x00AF1212;
    U32 literal_expression_color    = 0x001212AF;
    U32 identifier_expression_color = 0x001212AF;
    U32 lambda_expression_color     = 0x0012AF12;
    U32 conditional_statement_color = 0x00AFAF12;
    U32 declaration_statement_color = 0x00AF12AF;
    U32 arguments_statement_color   = 0x00FFFFFF;
    U32 misc_statement_color        = 0x00222222;
    
    format[0]->first_child = first_child;
    
    if (node->node_type == ASTNode_Scope)
    {
        format[0]->tag_color    = misc_statement_color;
        format[0]->text_index   = scope_string;
        
        for (AST_Node* scan = node->scope; scan; )
        {
            ++format[0]->num_children;
            scan = scan->next;
        }
    }
    
    else if (node->node_type == ASTNode_FuncDecl || (node->node_type == ASTNode_Expression && node->expr_type == ASTExpr_LambdaDecl))
    {
        format[0]->tag_color     = declaration_statement_color;
        format[0]->num_children  = 2;
        
        if (node->name.string.size)
        {
            format[0]->text_index = PushString(node->name.string);
        }
        
        else
        {
            format[0]->text_index = lambda_string;
        }
        
        format[1]->tag_color    = arguments_statement_color;
        format[1]->text_index   = args_string;
        format[1]->first_child  = format[0]->first_child + 2;
        format[1]->num_children = 0;
        
        U32 offset = 0;
        
        for (AST_Node* scan = node->arguments; scan; )
        {
            ++offset;
            
            if (scan->node_type == ASTNode_FuncDecl || (scan->node_type == ASTNode_Expression && scan->expr_type == ASTExpr_LambdaDecl))
            {
                offset += 2;
            }
            
            else if (scan->node_type == ASTNode_Expression && scan->expr_type == ASTExpr_TypeCast)
            {
                ++offset;
            }
            
            ++format[1]->num_children;
            scan = scan->next;
        }
        
        format[2]->tag_color    = misc_statement_color;
        format[2]->text_index   = scope_string;
        format[2]->first_child  = format[1]->first_child + offset;
        format[2]->num_children = 0;
        
        for (AST_Node* scan = node->body; scan; )
        {
            ++format[0]->num_children;
            scan = scan->next;
        }
    }
    
    else
    {
        if (node->node_type == ASTNode_If)
        {
            format[0]->tag_color    = conditional_statement_color;
            format[0]->text_index   = if_string;
            format[0]->num_children = 3;
        }
        
        else if (node->node_type == ASTNode_While)
        {
            format[0]->tag_color    = conditional_statement_color;
            format[0]->text_index   = while_string;
            format[0]->num_children = 2;
        }
        
        else if (node->node_type == ASTNode_Defer)
        {
            format[0]->tag_color    = misc_statement_color;
            format[0]->text_index   = defer_string;
            format[0]->num_children = 1;
        }
        
        else if (node->node_type == ASTNode_Return)
        {
            format[0]->tag_color    = misc_statement_color;
            format[0]->text_index   = return_string;
            format[0]->num_children = 1;
        }
        
        else if (node->node_type == ASTNode_VarDecl)
        {
            format[0]->tag_color     = declaration_statement_color;
            format[0]->text_index    = PushString(node->name.string);
            format[0]->num_children  = (node->value != 0 ? 1 : 0);
        }
        
        else if (node->node_type == ASTNode_StructDecl || node->node_type == ASTNode_EnumDecl)
        {
            format[0]->tag_color  = declaration_statement_color;
            format[0]->text_index = PushString(node->name.string);
            
            for (AST_Node* scan = node->members; scan; )
            {
                ++format[0]->num_children;
                scan = scan->next;
            }
        }
        
        else if (node->node_type == ASTNode_ConstDecl)
        {
            format[0]->tag_color     = declaration_statement_color;
            format[0]->text_index    = PushString(node->name.string);
            format[0]->num_children += (node->value != 0 ? 1 : 0);
        }
        
        else if (node->node_type == ASTNode_Expression)
        {
            switch(node->expr_type)
            {
                case ASTExpr_NumericLiteral:
                {
                    format[0]->tag_color   = literal_expression_color;
                    format[0]->is_small    = true;
                    format[0]->first_child = 0;
                    
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
                    
                    format[0]->text_index = PushString(string);
                } break;
                
                case ASTExpr_Ternary:
                {
                    format[0]->tag_color    = conditional_statement_color;
                    format[0]->text_index   = ternary_string;
                    format[0]->num_children = 3;
                } break;
                
                case ASTExpr_StringLiteral:
                {
                    format[0]->tag_color  = literal_expression_color;
                    format[0]->text_index = PushString(node->string_literal);
                } break;
                
                case ASTExpr_Identifier:
                {
                    format[0]->tag_color  = identifier_expression_color;
                    format[0]->text_index = PushString(node->identifier.string);
                } break;
                
                case ASTExpr_FunctionCall:
                {
                    // TODO(soimn): Write proper function calls
                    format[0]->tag_color    = lambda_expression_color;
                    format[0]->text_index   = call_string;
                } break;
                
                case ASTExpr_TypeCast:
                {
                    format[0]->tag_color    = lambda_expression_color;
                    format[0]->text_index   = cast_string;
                    format[0]->num_children = 2;
                    
                    format[1]->tag_color  = conditional_statement_color;
                    format[1]->text_index = PushString(CONST_STRING("Type"));
                };
                
                default:
                {
                    format[0]->tag_color    = operator_expression_color;
                    format[0]->num_children = (node->right ? 2 : 1);
                    format[0]->text_index   = node->expr_type;
                    format[0]->is_small     = true;
                } break;
            }
        }
    }
    
    ++format[0]->text_index;
    if (format[1]) ++format[1]->text_index;
    if (format[2]) ++format[2]->text_index;
}

inline void
DEBUGFormatASTRecursive(Bucket_Array* array, AST_Node* node, DEBUG_Node_Format** format, Bucket_Array* string_storage, Bucket_Array* string_array)
{
    DEBUGDumpNodeToArray(node, format, (U32)ElementCount(array), string_storage, string_array);
    
    UMM index = ElementCount(array);
    
    auto PushFormat = [&array] (AST_Node* node)
    {
        U32 num_formats = 1;
        
        if (node->node_type == ASTNode_FuncDecl || (node->node_type == ASTNode_Expression && node->expr_type == ASTExpr_LambdaDecl))
        {
            num_formats = 3;
        }
        
        else if (node->node_type == ASTNode_Expression && node->expr_type == ASTExpr_TypeCast)
        {
            num_formats = 2;
        }
        
        for (U32 j = 0; j < num_formats; ++j)
        {
            (DEBUG_Node_Format*)PushElement(array);
        }
    };
    
    auto GetFormat = [&array, &index] (AST_Node* node, DEBUG_Node_Format** new_format)
    {
        new_format[0] = (DEBUG_Node_Format*)ElementAt(array, index++);
        
        if (node->node_type == ASTNode_FuncDecl || (node->node_type == ASTNode_Expression && node->expr_type == ASTExpr_LambdaDecl))
        {
            new_format[1] = (DEBUG_Node_Format*)ElementAt(array, index++);
            new_format[2] = (DEBUG_Node_Format*)ElementAt(array, index++);
        }
        
        else if (node->node_type == ASTNode_Expression && node->expr_type == ASTExpr_TypeCast)
        {
            new_format[1] = (DEBUG_Node_Format*)ElementAt(array, index++);
        }
    };
    
    if (node->node_type == ASTNode_Scope)
    {
        for (AST_Node* scan = node->scope; scan; )
        {
            PushFormat(scan);
            scan = scan->next;
        }
        
        for (AST_Node* scan = node->scope; scan; )
        {
            DEBUG_Node_Format* new_format[3] = {};
            GetFormat(scan, new_format);
            
            DEBUGFormatASTRecursive(array, scan, new_format, string_storage, string_array);
            
            scan = scan->next;
        }
    }
    
    else if (node->node_type == ASTNode_FuncDecl || (node->node_type == ASTNode_Expression && node->expr_type == ASTExpr_LambdaDecl))
    {
        for (AST_Node* scan = node->arguments; scan; )
        {
            PushFormat(scan);
            scan = scan->next;
        }
        
        for (AST_Node* scan = node->arguments; scan; )
        {
            DEBUG_Node_Format* new_format[3] = {};
            GetFormat(scan, new_format);
            
            DEBUGFormatASTRecursive(array, scan, new_format, string_storage, string_array);
            
            scan = scan->next;
        }
        
        for (AST_Node* scan = node->body; scan; )
        {
            PushFormat(scan);
            scan = scan->next;
        }
        
        for (AST_Node* scan = node->body; scan; )
        {
            DEBUG_Node_Format* new_format[3] = {};
            GetFormat(scan, new_format);
            
            DEBUGFormatASTRecursive(array, scan, new_format, string_storage, string_array);
            
            scan = scan->next;
        }
    }
    
    else
    {
        for (U32 i = 0; i < ARRAY_COUNT(node->children); ++i)
        {
            if (node->children[i])
            {
                PushFormat(node->children[i]);
            }
        }
        
        for (U32 i = 0; i < ARRAY_COUNT(node->children); ++i)
        {
            if (node->children[i])
            {
                DEBUG_Node_Format* new_format[3] = {};
                GetFormat(node->children[i], new_format);
                
                DEBUGFormatASTRecursive(array, node->children[i], new_format, string_storage, string_array);
            }
        }
    }
}

inline void
DEBUGDumpASTToFile(AST* ast, const char* file_name)
{
#pragma warning(push)
#pragma warning(disable:4996;disable:4189) 
    
    HARD_ASSERT(!ast->is_refined);
    
    U32 magic_value = 0x00FF00DD;
    U32 node_count  = (U32)ElementCount(&ast->container);
    
    Bucket_Array string_storage = BUCKET_ARRAY(char, 4000);
    Bucket_Array string_array   = BUCKET_ARRAY(DEBUG_Node_String_Format, 60);
    
    auto PushString = [&string_storage, &string_array](String string) -> U32
    {
        U32 index = (U32)ElementCount(&string_array);
        
        DEBUG_Node_String_Format* format = (DEBUG_Node_String_Format*)PushElement(&string_array);
        
        format->size = 0;
        format->offset = (U32)ElementCount(&string_storage);
        
        for (U32 i = 0; i < string.size; ++i, ++format->size)
        {
            *(char*)PushElement(&string_storage) = string.data[i];
        }
        
        return index;
    };
    
    U32 strings[10 + AST_EXPRESSION_TYPE_COUNT] = {};
    
    strings[0] = PushString(CONST_STRING("If"));
    strings[1] = PushString(CONST_STRING("While"));
    strings[2] = PushString(CONST_STRING("Defer"));
    strings[3] = PushString(CONST_STRING("Return"));
    strings[4] = PushString(CONST_STRING("{...}"));
    strings[5] = PushString(CONST_STRING("Args"));
    strings[6] = PushString(CONST_STRING("Lambda"));
    strings[7] = PushString(CONST_STRING("Ternary"));
    strings[8] = PushString(CONST_STRING("Cast"));
    strings[9] = PushString(CONST_STRING("Call"));
    
    U32 expression_strings[AST_EXPRESSION_TYPE_COUNT] = {};
    
    for (U32 i = 10; i < AST_EXPRESSION_TYPE_COUNT; ++i)
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
    
    Bucket_Array format_storage = BUCKET_ARRAY(DEBUG_Node_Format, 256);
    {
        DEBUG_Node_Format* format[3] = {};
        format[0] = (DEBUG_Node_Format*)PushElement(&format_storage);
        
        DEBUGFormatASTRecursive(&format_storage, ast->root, format, &string_storage, &string_array);
    }
    
    U32 string_count = (U32)ElementCount(&string_array);
    
    FILE* file = fopen("tree.bin", "wb");
    
    fwrite(&magic_value,  1, 4, file);
    fwrite(&node_count,   1, 4, file);
    fwrite(&string_count, 1, 4, file);
    
    for (Bucket_Array_Iterator it = Iterate(&format_storage);
         it.current;
         Advance(&it))
    {
        DEBUG_Node_Format* format = (DEBUG_Node_Format*)it.current;
        
        fwrite((U8*)&format->tag_color,    1, 4, file);
        fwrite((U8*)&format->text_index,   1, 4, file);
        fwrite((U8*)&format->first_child,  1, 4, file);
        fwrite((U8*)&format->num_children, 1, 4, file);
        fwrite((U8*)&format->is_small,     1, 4, file);
    }
    
    for (Bucket_Array_Iterator it = Iterate(&string_array);
         it.current;
         Advance(&it))
    {
        fwrite(&((DEBUG_Node_String_Format*)it.current)->size,   1, 4, file);
        fwrite(&((DEBUG_Node_String_Format*)it.current)->offset, 1, 4, file);
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

int
main(int argc, const char** argv)
{
    AST result = ParseFile(CONST_STRING("Hell :: struct {n:float = 4; I:int;}i := 0; f : float = 12344;"));
    
    Print("%s\n", (result.is_valid ? "succeeded" : "failed"));
    
    if (result.is_valid)
    {
        DEBUGDumpASTToFile(&result, "tree.bin");
    }
    
    return 0;
}