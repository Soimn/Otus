typedef struct Symbol
{
    Text_Interval text;
    Identifier name;
    bool is_constant;
    
    bool is_exported;
    
    // Type info
    // value
} Symbol;

void
SemaReport(Workspace* workspace, Enum32(REPORT_SEVERITY) severity,
           Text_Interval text, const char* format_string, ...)
{
    Arg_List arg_list;
    ARG_LIST_START(arg_list, format_string);
    
    Report(workspace, severity, CompilationStage_SemanticAnalysis, WrapCString(format_string), arg_list, &text);
}

bool SemaProcessScope(Workspace* workspace, Scope* scope, bool in_proc, bool in_loop);

bool
SemaEnsureExpressionEvaluatesToAValue(Workspace* workspace, Expression* expr)
{
    bool is_value = !(expr->kind == ExprKind_SliceType        ||
                      expr->kind == ExprKind_DynamicArrayType ||
                      expr->kind == ExprKind_Polymorphic      ||
                      expr->kind == ExprKind_Proc             ||
                      expr->kind == ExprKind_Enum             ||
                      expr->kind == ExprKind_Struct);
    
    if (!is_value)
    {
        if (expr->kind == ExprKind_Polymorphic)
        {
            SemaReport(workspace, ReportSeverity_Error, expr->text,
                       "Polymorphic name declarations are not allowed in an expression context");
        }
        
        else if (expr->kind == ExprKind_SliceType || expr->kind == ExprKind_DynamicArrayType)
        {
            SemaReport(workspace, ReportSeverity_Error, expr->text,
                       "Types are not allowed in this context");
        }
        
        else
        {
            SemaReport(workspace, ReportSeverity_Error, expr->text,
                       "Expression does not evaluate to a value");
        }
    }
    
    return is_value;
}

bool
SemaProcessExpression(Workspace* workspace, Expression* expr, bool is_type)
{
    bool encountered_errors = false;
    
    switch (expr->kind)
    {
        case ExprKind_Identifier:
        case ExprKind_Number:
        case ExprKind_String:
        case ExprKind_Character:
        case ExprKind_Trilean:
        break;
        
        case ExprKind_Add:
        case ExprKind_Sub:
        case ExprKind_Mul:
        case ExprKind_Div:
        case ExprKind_Mod:
        case ExprKind_And:
        case ExprKind_Or:
        case ExprKind_BitAnd:
        case ExprKind_BitOr:
        case ExprKind_BitShiftLeft:
        case ExprKind_BitShiftRight:
        case ExprKind_CmpEqual:
        case ExprKind_CmpNotEqual:
        case ExprKind_CmpGreater:
        case ExprKind_CmpLess:
        case ExprKind_CmpGreaterOrEqual:
        case ExprKind_CmpLessOrEqual:
        case ExprKind_Not:
        case ExprKind_BitNot:
        case ExprKind_Neg:
        case ExprKind_Dereference:
        {
            if (SemaEnsureExpressionEvaluatesToAValue(workspace, expr->binary_expr.left))
            {
                if (SemaProcessExpression(workspace, expr->binary_expr.left, false))
                {
                    if (SemaEnsureExpressionEvaluatesToAValue(workspace, expr->binary_expr.right))
                    {
                        if (!SemaProcessExpression(workspace, expr->binary_expr.right, false))
                        {
                            encountered_errors = true;
                        }
                    }
                    
                    else encountered_errors = true;
                }
                
                else encountered_errors = true;
            }
            
            else encountered_errors = true;
            
        } break;
        
        case ExprKind_Member:
        {
            if (SemaEnsureExpressionEvaluatesToAValue(workspace, expr->binary_expr.left))
            {
                if (expr->binary_expr.right->kind == ExprKind_Member)
                {
                    if (expr->binary_expr.right->binary_expr.left->kind != ExprKind_Identifier)
                    {
                        SemaReport(workspace, ReportSeverity_Error, expr->binary_expr.right->binary_expr.left->text,
                                   "Invalid right hand side of member operator. Expression must be an identifier signifying the member to access");
                        
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        if (!SemaProcessExpression(workspace, expr->binary_expr.right, false))
                        {
                            encountered_errors = true;
                        }
                    }
                }
                
                else if (expr->binary_expr.right->kind != ExprKind_Identifier)
                {
                    SemaReport(workspace, ReportSeverity_Error, expr->binary_expr.right->text,
                               "Invalid right hand side of member operator. Expression must be an identifier signifying the member to access");
                    
                    encountered_errors = true;
                }
            }
            
            else encountered_errors = true;
            
        } break;
        
        case ExprKind_Subscript:
        case ExprKind_Slice:
        {
            if (SemaEnsureExpressionEvaluatesToAValue(workspace, expr->array_expr.pointer))
            {
                if (!SemaProcessExpression(workspace, expr->array_expr.pointer, false))
                {
                    encountered_errors = true;
                }
            }
            
            else encountered_errors = true;
            
            if (!encountered_errors && expr->array_expr.start != 0)
            {
                if (SemaEnsureExpressionEvaluatesToAValue(workspace, expr->array_expr.start))
                {
                    if (!SemaProcessExpression(workspace, expr->array_expr.start, false))
                    {
                        encountered_errors = true;
                    }
                }
                
                else encountered_errors = true;
            }
            
            if (!encountered_errors && expr->array_expr.length != 0)
            {
                if (SemaEnsureExpressionEvaluatesToAValue(workspace, expr->array_expr.length))
                {
                    if (!SemaProcessExpression(workspace, expr->array_expr.length, false))
                    {
                        encountered_errors = true;
                    }
                }
                
                else encountered_errors = true;
            }
            
        } break;
        
        case ExprKind_Run:
        {
            if (SemaEnsureExpressionEvaluatesToAValue(workspace, expr->run_expr))
            {
                if (!SemaProcessExpression(workspace, expr->run_expr, false))
                {
                    encountered_errors = true;
                }
            }
            
            else encountered_errors = true;
        } break;
        
        case ExprKind_Reference:
        {
            if (is_type)
            {
                // NOTE(soimn): Fallthrough
            }
            
            else
            {
                if (SemaEnsureExpressionEvaluatesToAValue(workspace, expr->unary_expr.operand))
                {
                    if (!SemaProcessExpression(workspace, expr->unary_expr.operand, false))
                    {
                        encountered_errors = true;
                    }
                }
                
                else encountered_errors = true;
                
                break;
            }
        }
        
        // NOTE(soimn): ExprKind_Reference must be located directly above this case, as it may fallthrough
        case ExprKind_SliceType:
        case ExprKind_DynamicArrayType:
        {
            if (expr->unary_expr.operand->kind == ExprKind_Call)
            {
                NOT_IMPLEMENTED;
            }
            
            else if (expr->unary_expr.operand->kind == ExprKind_SliceType        ||
                     expr->unary_expr.operand->kind == ExprKind_DynamicArrayType ||
                     expr->unary_expr.operand->kind == ExprKind_Reference        ||
                     expr->unary_expr.operand->kind == ExprKind_Struct           ||
                     expr->unary_expr.operand->kind == ExprKind_Enum             ||
                     (expr->unary_expr.operand->kind == ExprKind_Proc &&
                      !expr->unary_expr.operand->proc_expr.has_body))
            {
                if (!SemaProcessExpression(workspace, expr->unary_expr.operand, true))
                {
                    encountered_errors = true;
                }
            }
            
            else if (expr->unary_expr.operand->kind != ExprKind_Identifier)
            {
                SemaReport(workspace, ReportSeverity_Error, expr->unary_expr.operand->text,
                           "Invalid use of expression as type name");
                
                encountered_errors = true;
            }
        } break;
        
        case ExprKind_Polymorphic:
        {
            SemaReport(workspace, ReportSeverity_Error, expr->text,
                       "Polymorphic name declarations are not allowed in an expression context");
            
            encountered_errors = true;
        } break;
        
        case ExprKind_TypeLiteral:
        {
            if (expr->call_expr.forced_inline != TRI_NIL)
            {
                SemaReport(workspace, ReportSeverity_Error, expr->text,
                           "Type literals cannot be specified as inline");
                
                encountered_errors = true;
                break;
            }
            
            if (SemaProcessExpression(workspace, expr->call_expr.pointer, true))
            {
                for (U32 i = 0; !encountered_errors && i < expr->call_expr.args.size; ++i)
                {
                    Named_Expression* arg = Array_ElementAt(&expr->call_expr.args, i);
                    
                    if (arg->is_const || arg->is_baked || arg->is_using)
                    {
                        const char* kind = 0;
                        if      (arg->is_const) kind = "const";
                        else if (arg->is_baked) kind = "baked";
                        else                    kind = "using";
                        
                        SemaReport(workspace, ReportSeverity_Error, arg->value->text,
                                   "Cannot apply the '%s' modifier to a type literal argument", kind);
                        
                        encountered_errors = true;
                    }
                    
                    else if (arg->type != 0)
                    {
                        SemaReport(workspace, ReportSeverity_Error, arg->type->text,
                                   "Cannot apply type specifier to named expression in an expression context");
                        
                        encountered_errors = true;
                    }
                    
                    else if (SemaEnsureExpressionEvaluatesToAValue(workspace, arg->value))
                    {
                        if (!SemaProcessExpression(workspace, arg->value, false))
                        {
                            encountered_errors = true;
                        }
                    }
                    
                    else encountered_errors = true;
                }
            }
            
            else encountered_errors = true;
        } break;
        
        case ExprKind_Call:
        {
            if (SemaEnsureExpressionEvaluatesToAValue(workspace, expr->call_expr.pointer) &&
                SemaProcessExpression(workspace, expr->call_expr.pointer, false))
            {
                for (U32 i = 0; !encountered_errors && i < expr->call_expr.args.size; ++i)
                {
                    Named_Expression* arg = Array_ElementAt(&expr->call_expr.args, i);
                    
                    if (arg->is_const || arg->is_baked || arg->is_using)
                    {
                        const char* kind = 0;
                        if      (arg->is_const) kind = "const";
                        else if (arg->is_baked) kind = "baked";
                        else                    kind = "using";
                        
                        SemaReport(workspace, ReportSeverity_Error, arg->value->text,
                                   "Cannot apply the '%s' modifier to a procedure call argument", kind);
                        
                        encountered_errors = true;
                    }
                    
                    else if (arg->type != 0)
                    {
                        SemaReport(workspace, ReportSeverity_Error, arg->type->text,
                                   "Cannot apply type specifier to named expression in an expression context");
                        
                        encountered_errors = true;
                    }
                    
                    else if (SemaEnsureExpressionEvaluatesToAValue(workspace, arg->value))
                    {
                        if (!SemaProcessExpression(workspace, arg->value, false))
                        {
                            encountered_errors = true;
                        }
                    }
                    
                    else encountered_errors = true;
                }
            }
            
            else encountered_errors = true;
        } break;
        
        case ExprKind_Proc:
        {
            NOT_IMPLEMENTED;
        } break;
        
        case ExprKind_Enum:
        {
            NOT_IMPLEMENTED;
        } break;
        
        case ExprKind_Struct:
        {
            NOT_IMPLEMENTED;
        } break;
        
        INVALID_DEFAULT_CASE;
    }
    
    return !encountered_errors;
}

bool
SemaProcessConstantDecl(Workspace* workspace, Statement* statement)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
SemaProcessVariableDecl(Workspace* workspace, Statement* statement)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
SemaProcessScope(Workspace* workspace, Scope* scope, bool in_global_scope, bool in_loop)
{
    bool encountered_errors = false;
    
    ASSERT(!in_global_scope || !in_loop);
    
    scope->scope_id = ATOMIC_INC64(&workspace->scope_count) - 1;
    
    for (U32 i = 0; i < scope->block.statements.size; ++i)
    {
        if (encountered_errors) break;
        
        Statement* statement = Array_ElementAt(&scope->block.statements, i);
        
        if (statement->kind == StatementKind_ConstDecl)
        {
            if (!SemaProcessConstantDecl(workspace, statement))
            {
                encountered_errors = true;
            }
        }
        
        else if (statement->kind == StatementKind_VarDecl)
        {
            if (!SemaProcessVariableDecl(workspace, statement))
            {
                encountered_errors = true;
            }
        }
        
        else if (!in_global_scope)
        {
            switch (statement->kind)
            {
                case StatementKind_UsingDecl:
                {
                    for (U32 j = 0; j < statement->using_decl.expressions.size; ++j)
                    {
                        Expression* expr = Array_ElementAt(&statement->using_decl.expressions, j);
                        
                        if (expr->kind != ExprKind_Identifier &&
                            !(expr->kind == ExprKind_Member &&
                              expr->binary_expr.left->kind == ExprKind_Identifier &&
                              SemaProcessExpression(workspace, expr, false)))
                        {
                            encountered_errors = true;
                            break;
                        }
                    }
                } break;
                
                case StatementKind_Scope:
                {
                    if (!SemaProcessScope(workspace, &statement->scope, false, in_loop))
                    {
                        encountered_errors = true;
                    }
                } break;
                
                case StatementKind_If:
                {
                    if (!SemaProcessExpression(workspace, statement->if_stmnt.condition, false)       ||
                        !SemaProcessScope(workspace, &statement->if_stmnt.true_scope, false, in_loop) ||
                        !SemaProcessScope(workspace, &statement->if_stmnt.false_scope, false, in_loop))
                    {
                        encountered_errors = true;
                    }
                } break;
                
                case StatementKind_While:
                {
                    if (!SemaProcessExpression(workspace, statement->while_stmnt.condition, false) ||
                        !SemaProcessScope(workspace, &statement->while_stmnt.scope, false, true))
                    {
                        encountered_errors = true;
                    }
                } break;
                
                case StatementKind_Break:
                case StatementKind_Continue:
                {
                    if (!in_loop)
                    {
                        SemaReport(workspace, ReportSeverity_Error, statement->text,
                                   "%s statements are not allowed outside of loops", (statement->kind == StatementKind_Break ? "Break" : "Continue"));
                        
                        encountered_errors = true;
                    }
                } break;
                
                case StatementKind_Return:
                {
                    for (U32 j = 0; j < statement->return_stmnt.values.size; ++j)
                    {
                        Expression* expr = Array_ElementAt(&statement->return_stmnt.values, j);
                        
                        if (!SemaProcessExpression(workspace, expr, false))
                        {
                            encountered_errors = true;
                            break;
                        }
                    }
                } break;
                
                case StatementKind_Defer:
                {
                    if (!SemaProcessScope(workspace, &statement->scope, false, false))
                    {
                        encountered_errors = true;
                    }
                } break;
                
                case StatementKind_Assignment:
                {
                    for (U32 j = 0; j < statement->assignment_stmnt.left.size; ++j)
                    {
                        Expression* expr = Array_ElementAt(&statement->assignment_stmnt.left, j);
                        
                        if (!SemaProcessExpression(workspace, expr, false))
                        {
                            encountered_errors = true;
                            break;
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        for (U32 j = 0; j < statement->assignment_stmnt.right.size; ++j)
                        {
                            Expression* expr = Array_ElementAt(&statement->assignment_stmnt.right, j);
                            
                            if (!SemaProcessExpression(workspace, expr, false))
                            {
                                encountered_errors = true;
                                break;
                            }
                        }
                    }
                } break;
                
                case StatementKind_Expression:
                {
                    if (!SemaProcessExpression(workspace, statement->expression_stmnt, false))
                    {
                        encountered_errors = true;
                    }
                } break;
                
                INVALID_DEFAULT_CASE;
            }
        }
        
        else if (statement->kind != StatementKind_ImportDecl)
        {
            SemaReport(workspace, ReportSeverity_Error, statement->text,
                       "Invalid use of statement in global scope. Only import-, constant and variable declarations are legal in global scope");
            
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}