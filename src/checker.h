typedef struct Sema_State
{
    Compiler_State* compiler_state;
    
    bool in_proc;
    bool in_loop;
    bool in_struct;
    bool in_enum;
    bool in_params;
    bool in_return_values;
    bool in_named_argument_list;
} Sema_State;

bool
SemaValidateExpression(Sema_State state, Expression* expression)
{
    bool is_valid = false;
    
    if (expression->kind == Expr_Number     ||
        expression->kind == Expr_String     ||
        expression->kind == Expr_Boolean    ||
        expression->kind == Expr_Identifier ||
        expression->kind == Expr_Empty)
    {
        is_valid = true;
    }
    
    else if (expression->kind == Expr_Ternary)
    {
        if (expression->ternary_expression.condition == 0)
        {
            //// ERROR: Missing condition in ternary expression
            is_valid = false;
        }
        
        else if (expression->ternary_expression.true_expr == 0)
        {
            //// ERROR: Missing true clause in ternary expression
            is_valid = false;
        }
        
        
        else if (expression->ternary_expression.false_expr == 0)
        {
            //// ERROR: Missing false clause in ternary expression
            is_valid = false;
        }
        
        else is_valid = (SemaValidateExpression(state, expression->ternary_expression.condition) &&
                         SemaValidateExpression(state, expression->ternary_expression.true_expr) &&
                         SemaValidateExpression(state, expression->ternary_expression.false_expr));
    }
    
    else if (expression->kind == Expr_Add              ||
             expression->kind == Expr_Sub              ||
             expression->kind == Expr_Mul              ||
             expression->kind == Expr_Div              ||
             expression->kind == Expr_Mod              ||
             expression->kind == Expr_And              ||
             expression->kind == Expr_Or               ||
             expression->kind == Expr_BitAnd           ||
             expression->kind == Expr_BitOr            ||
             expression->kind == Expr_BitXor           ||
             expression->kind == Expr_IsEqual          ||
             expression->kind == Expr_IsNotEqual       ||
             expression->kind == Expr_IsLess           ||
             expression->kind == Expr_IsGreater        ||
             expression->kind == Expr_IsLessOrEqual    ||
             expression->kind == Expr_IsGreaterOrEqual ||
             expression->kind == Expr_Chain            ||
             expression->kind == Expr_IsIn             ||
             expression->kind == Expr_IsNotIn          ||
             expression->kind == Expr_Interval)
    {
        if (expression->binary_expression.left == 0)
        {
            //// ERROR: Missing left hand side of binary expression
            is_valid = false;
        }
        
        else if (expression->binary_expression.right == 0)
        {
            //// ERROR: Missing right hand side of binary expression
            is_valid = false;
        }
        
        else is_valid = (SemaValidateExpression(state, expression->binary_expression.left) &&
                         SemaValidateExpression(state, expression->binary_expression.right));
    }
    
    else if (expression->kind == Expr_Neg          ||
             expression->kind == Expr_Not          ||
             expression->kind == Expr_BitNot       ||
             expression->kind == Expr_Reference    ||
             expression->kind == Expr_Dereference  ||
             expression->kind == Expr_Slice        ||
             expression->kind == Expr_DynamicArray ||
             expression->kind == Expr_Pointer)
    {
        if (expression->unary_expression.operand == 0)
        {
            //// ERROR: Missing operand in unary expression
            is_valid = false;
        }
        
        else is_valid = SemaValidateExpression(state, expression->unary_expression.operand);
    }
    
    else if (expression->kind == Expr_Member)
    {
        if (expression->binary_expression.left == 0)
        {
            //// ERROR: Missing left hand side of binary expression
            is_valid = false;
        }
        
        else if (expression->binary_expression.right == 0)
        {
            //// ERROR: Missing right hand side of binary expression
            is_valid = false;
        }
        
        else if (expression->binary_expression.right->kind != Expr_Identifier ||
                 expression->binary_expression.right->string.size == 0)
        {
            //// ERROR: Right hand side of member operator must be a non blank identifier
            is_valid = false;
        }
        
        else
        {
            is_valid = (SemaValidateExpression(state, expression->binary_expression.left) &&
                        SemaValidateExpression(state, expression->binary_expression.right));
        }
    }
    
    else if (expression->kind == Expr_Call          ||
             expression->kind == Expr_StructLiteral ||
             expression->kind == Expr_ArrayLiteral)
    {
        Expression* pointer = (expression->kind == Expr_Call
                               ? expression->call_expression.pointer
                               : (expression->kind == Expr_StructLiteral
                                  ? expression->struct_literal.type
                                  : expression->array_literal.type));
        
        Slice(Expression*) arguments = (expression->kind == Expr_Call
                                        ? expression->call_expression.arguments
                                        : (expression->kind == Expr_StructLiteral
                                           ? expression->struct_literal.arguments
                                           : expression->array_literal.elements));
        
        if (expression->kind == Expr_Call && pointer == 0)
        {
            //// ERROR: Missing procedure operand in call expression
            is_valid = false;
        }
        
        else
        {
            Sema_State sub_state = state;
            sub_state.in_named_argument_list = true;
            
            for (umm i = 0; is_valid && i < arguments.size; ++i)
            {
                Expression* expr = *Slice_ElementAt(&arguments, Expression*, i);
                
                if (expr->kind != Expr_NamedArgument)
                {
                    //// ERROR: Invalid use of expression in named argument list. All expressions in a named argument list must be of type named argument
                    is_valid = false;
                }
                
                else is_valid = (is_valid && SemaValidateExpression(sub_state, expr));
            }
        }
    }
    
    else if (expression->kind == Expr_Subscript)
    {
        if (expression->subscript_expression.pointer == 0)
        {
            //// ERROR: Missing operand for subscript expression
            is_valid = false;
        }
        
        else is_valid = (SemaValidateExpression(state, expression->subscript_expression.pointer) &&
                         (expression->subscript_expression.index  == 0 || SemaValidateExpression(state, expression->subscript_expression.index))  &&
                         (expression->subscript_expression.length == 0 || SemaValidateExpression(state, expression->subscript_expression.length)) &&
                         (expression->subscript_expression.step   == 0 || SemaValidateExpression(state, expression->subscript_expression.step)));
    }
    
    else if (expression->kind == Expr_Array)
    {
        if (expression->array_type.type == 0)
        {
            //// ERROR: Missing type of array type
            is_valid = false;
        }
        
        else if (expression->array_type.size == 0)
        {
            //// ERROR: Missing size of array type
            is_valid = false;
        }
        
        else is_valid = (SemaValidateExpression(state, expression->array_type.type) &&
                         SemaValidateExpression(state, expression->array_type.size));
    }
    
    else if (expression->kind == Expr_Polymorphic)
    {
        if (expression->unary_expression.operand == 0)
        {
            //// ERROR: Missing operand in unary expression
            is_valid = false;
        }
        
        else if (expression->unary_expression.operand->kind != Expr_Identifier ||
                 expression->unary_expression.operand->string.size == 0)
        {
            //// ERROR: Operand must be a non blank identifier
            is_valid = false;
        }
        
        else is_valid = SemaValidateExpression(state, expression->unary_expression.operand);
    }
    
    else if (expression->kind == Expr_Proc)
    {
        NOT_IMPLEMENTED;
    }
    
    else if (expression->kind == Expr_Struct)
    {
        NOT_IMPLEMENTED;
    }
    
    else if (expression->kind == Expr_Enum)
    {
        NOT_IMPLEMENTED;
    }
    
    else if (expression->kind == Expr_NamedArgument)
    {
        if (!state.in_named_argument_list)
        {
            //// ERROR: Named argument outside named argument list
            is_valid = false;
        }
        
        else if (expression->named_argument.value == 0)
        {
            //// ERROR: Missing value of named argument
            is_valid = false;
        }
        
        else
        {
            Sema_State sub_state = state;
            sub_state.in_named_argument_list = false;
            
            is_valid = (SemaValidateExpression(sub_state, expression->named_argument.value) &&
                        (expression->named_argument.name == 0 || SemaValidateExpression(sub_state, expression->named_argument.name)));
        }
    }
    
    else if (expression->kind == Expr_Compound)
    {
        if (expression->compound.expression == 0)
        {
            //// ERROR: Missing expression in compound expression
            is_valid = false;
        }
        
        else
        {
            // NOTE(soimn): Collapsing compound expression to not hinder pattern matching
            *expression = *expression->compound.expression;
            
            is_valid = SemaValidateExpression(state, expression);
        }
    }
    
    else
    {
        //// ERROR: Invalid expression kind
        is_valid = false;
    }
    
    return is_valid;
}

bool
SemaValidateStatementAndGenerateSymbolInfo(Sema_State state, Statement* statement)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
SemaValidateDeclarationAndGenerateSymbolInfo(Compiler_State* compiler_state, Declaration declaration)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
TypeCheckDeclaration(Compiler_State* compiler_state, Declaration declaration)
{
    bool encountered_errors = false;
    
    // TODO(soimn): Struct specialization fixup
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}