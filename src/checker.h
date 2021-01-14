typedef struct Sema_State
{
    Compiler_State* compiler_state;
    
    bool in_proc;
    bool in_loop;
    bool in_struct;
    bool in_enum;
    bool in_params;
    bool in_return_values;
} Sema_State;

bool
SemaAnalyzeExpression(Sema_State state, Expression* expression)
{
    bool encountered_errors = false;
    
    if (expression->kind == Expr_Empty)
    {
        //// ERROR: Missing primary expression
        encountered_errors = true;
    }
    
    else if (expression->kind == Expr_Ternary)
    {
        encountered_errors = (!SemaAnalyzeExpression(state, expression->ternary_expression.condition) ||
                              !SemaAnalyzeExpression(state, expression->ternary_expression.true_expr) ||
                              !SemaAnalyzeExpression(state, expression->ternary_expression.false_expr));
    }
    
    else if (expression->kind == Expr_Add           ||
             expression->kind == Expr_Sub           ||
             expression->kind == Expr_Mul           ||
             expression->kind == Expr_Div           ||
             expression->kind == Expr_Mod           ||
             expression->kind == Expr_And           ||
             expression->kind == Expr_Or            ||
             expression->kind == Expr_BitAnd        ||
             expression->kind == Expr_BitOr         ||
             expression->kind == Expr_BitXor        ||
             expression->kind == Expr_IsEqual       ||
             expression->kind == Expr_IsNotEqual    ||
             expression->kind == Expr_IsLess        ||
             expression->kind == Expr_IsGreater     ||
             expression->kind == Expr_IsLessOrEqual ||
             expression->kind == Expr_IsGreaterOrEqual)
    {
        encountered_errors = (!SemaAnalyzeExpression(state, expression->binary_expression.left) ||
                              !SemaAnalyzeExpression(state, expression->binary_expression.right));
    }
    
    else if (expression->kind == Expr_Neg       ||
             expression->kind == Expr_Not       ||
             expression->kind == Expr_BitNot    ||
             expression->kind == Expr_Reference ||
             expression->kind == Expr_Dereference)
    {
        encountered_errors = (!SemaAnalyzeExpression(state, expression->unary_expression.operand));
    }
    
    else if (expression->kind == Expr_Member)
    {
        if (expression->binary_expression.right->kind != Expr_Identifier)
        {
            NOT_IMPLEMENTED;
        }
    }
    // Postfix
    Expr_Member,
    Expr_Call,
    Expr_Subscript,
    
    // Types
    Expr_Slice,
    Expr_Array,
    Expr_DynamicArray,
    Expr_Pointer,
    Expr_Struct,
    Expr_Union,
    Expr_Enum,
    Expr_Proc,
    Expr_Polymorphic,
    
    // Literals
    Expr_Number,
    Expr_String,
    Expr_Identifier,
    Expr_StructLiteral,
    Expr_ArrayLiteral,
    Expr_Interval,
    
    // Special
    Expr_NamedArgument,
    return !encountered_errors;
}

bool
SemaAnalyzeStatement(Sema_State state, Statement* statement)
{
    bool encountered_errors = false;
    
    if (state.in_params || state.in_return_values)
    {
        if (statement->kind == Statement_Parameter)
        {
            if (state.in_return_values)
            {
                //// ERROR:
                encountered_errors = true;
                NOT_IMPLEMENTED;
            }
            
            else
            {
                NOT_IMPLEMENTED;
            }
        }
        
        else if (statement->kind == Statement_ReturnValue)
        {
            if (state.in_params)
            {
                //// ERROR:
                encountered_errors = true;
                NOT_IMPLEMENTED;
            }
            
            else
            {
                NOT_IMPLEMENTED;
            }
        }
        
        else
        {
            //// ERROR:
            encountered_errors = true;
            NOT_IMPLEMENTED;
        }
    }
    
    else
    {
        if (statement->kind == Statement_Parameter)
        {
            //// ERROR:
            encountered_errors = true;
            NOT_IMPLEMENTED;
        }
        
        else if (statement->kind == Statement_ReturnValue)
        {
            //// ERROR:
            encountered_errors = true;
            NOT_IMPLEMENTED;
        }
        
        else if (statement->kind == Statement_Block)
        {
            for (umm i = 0; i < statement->block.statements.size; ++i)
            {
                encountered_errors = encountered_errors || !SemaAnalyzeStatement(compiler_state, statement);
            }
        }
        
        else if (statement->kind == Statement_Expression)
        {
            encountered_errors = !SemaAnalyzeExpression(compiler_state, statement->expression);
        }
        
        else if (statement->kind == Statement_ImportDecl)
        {
            NOT_IMPLEMENTED;
        }
        
        else if (statement->kind == Statement_VariableDecl)
        {
            NOT_IMPLEMENTED;
        }
        
        else if (statement->kind == Statement_ConstantDecl)
        {
            NOT_IMPLEMENTED;
        }
        
        else if (statement->kind == Statement_If)
        {
            NOT_IMPLEMENTED;
        }
        
        else if (statement->kind == Statement_When)
        {
            NOT_IMPLEMENTED;
        }
        
        else if (statement->kind == Statement_While)
        {
            NOT_IMPLEMENTED;
        }
        
        else if (statement->kind == Statement_Break || statement->kind == Statement_Continue)
        {
            NOT_IMPLEMENTED;
        }
        
        else if (statement->kind == Statement_Return)
        {
            NOT_IMPLEMENTED;
        }
        
        else if (statement->kind == Statement_Using)
        {
            NOT_IMPLEMENTED;
        }
        
        else if (statement->kind == Statement_Defer)
        {
            NOT_IMPLEMENTED;
        }
        
        else if (statement->kind == Statement_Assignment)
        {
            NOT_IMPLEMENTED;
        }
        
        else
        {
            //// ERROR:
            encountered_errors = true;
            NOT_IMPLEMENTED;
        }
    }
    
    return !encountered_errors;
}

bool
SemaAnalyzeDeclaration(Compiler_State* compiler_state, Declaration declaration)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
GenerateSymbolDataForDeclaration(Compiler_State* compiler_state, Declaration declaration)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
TypeCheckDeclaration(Compiler_State* compiler_state, Declaration declaration)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}