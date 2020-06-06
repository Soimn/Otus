

// IMPORTANT TODO(soimn): Remember to handle NULL and BLANK identifiers
// IMPORTANT TODO(soimn): Remember to handle illegal use of baked arguments and polymorphic names
// IMPORTANT TODO(soimn): Ensure polymorphic types are handled correctly
// TODO(soimn): Construct the symbol table


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

bool SemaProcessScope(Workspace* workspace, Scope* scope, bool in_global_scope, bool in_loop);

bool
SemaEnsureExpressionEvaluatesToAValue(Workspace* workspace, Expression* expr)
{
    bool is_value = !(expr->kind == ExprKind_SliceType        ||
                      expr->kind == ExprKind_DynamicArrayType ||
                      expr->kind == ExprKind_ArrayType        ||
                      expr->kind == ExprKind_Polymorphic      ||
                      expr->kind == ExprKind_Proc             ||
                      expr->kind == ExprKind_ProcGroup        ||
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
        
        else if (expr->kind == ExprKind_ProcGroup)
        {
            SemaReport(workspace, ReportSeverity_Error, expr->text,
                       "Procedure groups cannot be used in expressions directly. They must first be bound to a constant");
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
SemaProcessExpression(Workspace* workspace, Expression* expr, bool is_type, bool allow_poly)
{
    bool encountered_errors = false;
    
    if (is_type && !(expr->kind == ExprKind_Identifier  ||
                     expr->kind == ExprKind_Polymorphic ||
                     expr->kind == ExprKind_Call        ||
                     expr->kind == ExprKind_Proc        ||
                     expr->kind == ExprKind_Struct      ||
                     expr->kind == ExprKind_Enum        ||
                     expr->kind == ExprKind_Reference   ||
                     expr->kind == ExprKind_SliceType   ||
                     expr->kind == ExprKind_ArrayType   ||
                     expr->kind == ExprKind_DynamicArrayType))
    {
        SemaReport(workspace, ReportSeverity_Error, expr->text,
                   "Invalid use of expression in type context");
        
        encountered_errors = true;
    }
    
    else if (!is_type && (expr->kind == ExprKind_Polymorphic ||
                          expr->kind == ExprKind_ArrayType   ||
                          expr->kind == ExprKind_SliceType   ||
                          expr->kind == ExprKind_DynamicArrayType))
    {
        SemaReport(workspace, ReportSeverity_Error, expr->text,
                   "Invalid use of expression outside of type context");
        
        encountered_errors = true;
    }
    
    else
    {
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
                    if (SemaProcessExpression(workspace, expr->binary_expr.left, false, false))
                    {
                        if (SemaEnsureExpressionEvaluatesToAValue(workspace, expr->binary_expr.right))
                        {
                            if (!SemaProcessExpression(workspace, expr->binary_expr.right, false, false))
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
                            if (!SemaProcessExpression(workspace, expr->binary_expr.right, false, false))
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
                    if (!SemaProcessExpression(workspace, expr->array_expr.pointer, false, false))
                    {
                        encountered_errors = true;
                    }
                }
                
                else encountered_errors = true;
                
                if (!encountered_errors && expr->array_expr.start != 0)
                {
                    if (SemaEnsureExpressionEvaluatesToAValue(workspace, expr->array_expr.start))
                    {
                        if (!SemaProcessExpression(workspace, expr->array_expr.start, false, false))
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
                        if (!SemaProcessExpression(workspace, expr->array_expr.length, false, false))
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
                    if (!SemaProcessExpression(workspace, expr->run_expr, false, false))
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
                        if (!SemaProcessExpression(workspace, expr->unary_expr.operand, false, false))
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
            case ExprKind_ArrayType:
            case ExprKind_DynamicArrayType:
            {
                if (expr->unary_expr.operand->kind == ExprKind_Call)
                {
                    // NOTE(soimn): Valid calls in types are typeof and type parameterization 
                    // NOTE(soimn): is_type is set to false to allow for ordinary expressions
                    //              in the call arguments
                    if (!SemaProcessExpression(workspace, expr->unary_expr.operand, false, false))
                    {
                        encountered_errors = true;
                    }
                }
                
                else if (expr->unary_expr.operand->kind == ExprKind_Polymorphic      ||
                         expr->unary_expr.operand->kind == ExprKind_SliceType        ||
                         expr->unary_expr.operand->kind == ExprKind_ArrayType        ||
                         expr->unary_expr.operand->kind == ExprKind_DynamicArrayType ||
                         expr->unary_expr.operand->kind == ExprKind_Reference        ||
                         expr->unary_expr.operand->kind == ExprKind_Struct           ||
                         expr->unary_expr.operand->kind == ExprKind_Enum             ||
                         (expr->unary_expr.operand->kind == ExprKind_Proc &&
                          !expr->unary_expr.operand->proc_expr.has_body))
                {
                    if (!SemaProcessExpression(workspace, expr->unary_expr.operand, true, allow_poly))
                    {
                        encountered_errors = true;
                    }
                }
                
                else if (expr->unary_expr.operand->kind != ExprKind_Identifier)
                {
                    SemaReport(workspace, ReportSeverity_Error, expr->unary_expr.operand->text,
                               "Invalid use of expression in type");
                    
                    encountered_errors = true;
                }
            } break;
            
            case ExprKind_Polymorphic:
            {
                if (!allow_poly)
                {
                    SemaReport(workspace, ReportSeverity_Error, expr->text,
                               "Polymorphic names are not allowed in this context");
                    
                    SemaReport(workspace, ReportSeverity_Info, expr->text,
                               "Polymorphic names are only valid in the type of a procedure argument, where the procedure is either bound to a constant or invoked immediately");
                    
                    encountered_errors = true;
                }
            } break;
            
            case ExprKind_TypeLiteral:
            {
                if (SemaProcessExpression(workspace, expr->type_lit_expr.type, true, false))
                {
                    for (U32 i = 0; !encountered_errors && i < expr->type_lit_expr.args.size; ++i)
                    {
                        Named_Expression* arg = Array_ElementAt(&expr->type_lit_expr.args, i);
                        
                        if (arg->is_const || arg->is_baked || arg->is_using)
                        {
                            const char* kind = 0;
                            if      (arg->is_const) kind = "const";
                            else if (arg->is_baked) kind = "baked";
                            else                    kind = "using";
                            
                            SemaReport(workspace, ReportSeverity_Error, arg->value->text,
                                       "Cannot apply the '%s' modifier to member name of type literal argument",
                                       kind);
                            
                            encountered_errors = true;
                        }
                        
                        else if (arg->type != 0)
                        {
                            SemaReport(workspace, ReportSeverity_Error, arg->type->text,
                                       "Cannot apply type specifier to member name of type literal argument");
                            
                            encountered_errors = true;
                        }
                        
                        else if (SemaEnsureExpressionEvaluatesToAValue(workspace, arg->value))
                        {
                            if (!SemaProcessExpression(workspace, arg->value, false, false))
                            {
                                encountered_errors = true;
                            }
                            // IMPORTANT TODO(soimn): Handle the use of types as arguments, and proc groups
                            NOT_IMPLEMENTED;
                        }
                        
                        else encountered_errors = true;
                    }
                }
                
                else encountered_errors = true;
            } break;
            
            case ExprKind_Call:
            {
                if (SemaProcessExpression(workspace, expr->call_expr.pointer, false, true) &&
                    (expr->call_expr.pointer->kind == ExprKind_Proc ||
                     SemaEnsureExpressionEvaluatesToAValue(workspace, expr->call_expr.pointer)))
                {
                    if (expr->call_expr.pointer->kind == ExprKind_Proc &&
                        !expr->call_expr.pointer->proc_expr.has_body)
                    {
                        SemaReport(workspace, ReportSeverity_Error, expr->call_expr.pointer->text,
                                   "Cannot invoke a procedure without a body");
                        
                        encountered_errors = true;
                    }
                    
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
                        
                        else
                        {
                            if (SemaEnsureExpressionEvaluatesToAValue(workspace, arg->value))
                            {
                                // NOTE(soimn): This is a regualar value
                                if (!SemaProcessExpression(workspace, arg->value, false, false))
                                {
                                    encountered_errors = true;
                                }
                            }
                            
                            else
                            {
                                // NOTE(soimn): This might be a type
                                NOT_IMPLEMENTED;
                            }
                        }
                    }
                }
                
                else encountered_errors = true;
            } break;
            
            case ExprKind_Proc:
            {
                if (is_type && expr->proc_expr.has_body)
                {
                    SemaReport(workspace, ReportSeverity_Error, expr->text,
                               "Invalid use of procedure, with body, as type");
                    
                    encountered_errors = true;
                }
                
                else
                {
                    for (U32 i = 0; !encountered_errors && i < expr->proc_expr.args.size; ++i)
                    {
                        Named_Expression* named_expr = Array_ElementAt(&expr->proc_expr.args, i);
                        
                        if (named_expr->name == NULL_IDENTIFIER)
                        {
                            SemaReport(workspace, ReportSeverity_Error, named_expr->text,
                                       "Missing name of procedure argument");
                            
                            encountered_errors = true;
                        }
                        
                        else if (named_expr->name == BLANK_IDENTIFIER)
                        {
                            SemaReport(workspace, ReportSeverity_Error, named_expr->text,
                                       "Procedure argument name cannot be blank");
                            
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            if (named_expr->type == 0)
                            {
                                SemaReport(workspace, ReportSeverity_Error, named_expr->text,
                                           "Missing type specifier for procedure argument");
                                
                                encountered_errors = true;
                            }
                            
                            else if (named_expr->value != 0 && !expr->proc_expr.has_body)
                            {
                                SemaReport(workspace, ReportSeverity_Error, named_expr->value->text,
                                           "Cannot assign a value to procedure argument in procedure without body");
                                
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                if (named_expr->is_baked && !allow_poly)
                                {
                                    SemaReport(workspace, ReportSeverity_Error, named_expr->text,
                                               "Procedure argument cannot be baked in this context");
                                    SemaReport(workspace, ReportSeverity_Info, named_expr->text,
                                               "Procedure arguments can only be baked when the procedure is bound to a constant or immediately invoked");
                                    
                                    encountered_errors = true;
                                }
                                
                                else if (named_expr->value != 0)
                                {
                                    if (!SemaEnsureExpressionEvaluatesToAValue(workspace, named_expr->value) ||
                                        !SemaProcessExpression(workspace, named_expr->value, false, false))
                                    {
                                        encountered_errors = true;
                                    }
                                }
                                
                                if (!encountered_errors)
                                {
                                    if (named_expr->type != named_expr->value)
                                    {
                                        if (!SemaProcessExpression(workspace, named_expr->type, true,
                                                                   !is_type && allow_poly))
                                        {
                                            encountered_errors = true;
                                        }
                                    }
                                    
                                    else named_expr->type = 0;
                                }
                            }
                        }
                    }
                    
                    for (U32 i = 0; !encountered_errors && i < expr->proc_expr.return_values.size; ++i)
                    {
                        Named_Expression* named_expr = Array_ElementAt(&expr->proc_expr.return_values, i);
                        
                        if (named_expr->name == NULL_IDENTIFIER)
                        {
                            SemaReport(workspace, ReportSeverity_Error, named_expr->text,
                                       "Missing name or type of procedure return value");
                            
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            if (named_expr->name == BLANK_IDENTIFIER) named_expr->name = NULL_IDENTIFIER;
                            
                            if (named_expr->is_const || named_expr->is_baked || named_expr->is_using)
                            {
                                const char* kind = 0;
                                if      (named_expr->is_const) kind = "const";
                                else if (named_expr->is_baked) kind = "baked";
                                else                           kind = "using";
                                
                                SemaReport(workspace, ReportSeverity_Error, named_expr->text,
                                           "Illegal use of %s modifier on procedure return value", kind);
                                
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                if (named_expr->type != 0 && named_expr->value != 0 ||
                                    named_expr->value != 0 && named_expr->name != NULL_IDENTIFIER)
                                {
                                    SemaReport(workspace, ReportSeverity_Error, named_expr->text,
                                               "Cannot assign a value to a procedure return value");
                                    
                                    encountered_errors = true;
                                }
                                
                                else
                                {
                                    if (named_expr->value != 0) named_expr->type = named_expr->value;
                                    
                                    if (!SemaProcessExpression(workspace, named_expr->type, true, false))
                                    {
                                        encountered_errors = true;
                                    }
                                }
                            }
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        if (expr->proc_expr.foreign_library != 0 && 
                            (!SemaEnsureExpressionEvaluatesToAValue(workspace, expr->proc_expr.foreign_library) ||
                             !SemaProcessExpression(workspace, expr->proc_expr.foreign_library, false, false)))
                        {
                            encountered_errors = true;
                        }
                        
                        else if (expr->proc_expr.deprecation_notice != 0 &&
                                 (!SemaEnsureExpressionEvaluatesToAValue(workspace, 
                                                                         expr->proc_expr.deprecation_notice) ||
                                  !SemaProcessExpression(workspace, expr->proc_expr.deprecation_notice, false, false)))
                        {
                            encountered_errors = true;
                        }
                        
                        else if (!SemaProcessScope(workspace, &expr->proc_expr.scope, false, false))
                        {
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            if (expr->proc_expr.is_inlined == TRI_NIL) 
                                expr->proc_expr.is_inlined = workspace->build_options.inline_by_default;
                            
                            if (expr->proc_expr.no_discard == false)
                                expr->proc_expr.no_discard = workspace->build_options.no_discard_by_default;
                        }
                    }
                }
            } break;
            
            case ExprKind_ProcGroup:
            {
                for (U32 i = 0; !encountered_errors && i < expr->proc_group_expr.procedures.size; ++i)
                {
                    Expression* expr_i = Array_ElementAt(&expr->proc_group_expr.procedures, i);
                    
                    if (expr_i->kind == ExprKind_Identifier)
                    {
                        if (expr_i->identifier == NULL_IDENTIFIER)
                        {
                            SemaReport(workspace, ReportSeverity_Error, expr_i->text,
                                       "Missing name of procedure in procedure group expression list");
                            
                            encountered_errors = true;
                        }
                        
                        else if (expr_i->identifier == BLANK_IDENTIFIER)
                        {
                            SemaReport(workspace, ReportSeverity_Error, expr_i->text,
                                       "Name of procedure in procedure group expression list cannot be blank");
                            
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        SemaReport(workspace, ReportSeverity_Error, expr_i->text,
                                   "Invalid use of expression as procedure name in procedure group expresion list");
                        
                        encountered_errors = true;
                    }
                }
            } break;
            
            case ExprKind_Enum:
            {
                for (U32 i = 0; !encountered_errors && i < expr->enum_expr.members.size; ++i)
                {
                    Named_Expression* named_expr = Array_ElementAt(&expr->enum_expr.members, i);
                    
                    if (named_expr->name == NULL_IDENTIFIER)
                    {
                        SemaReport(workspace, ReportSeverity_Error, named_expr->text,
                                   "Missing name of enum member");
                        
                        encountered_errors = true;
                    }
                    
                    else if (named_expr->name == BLANK_IDENTIFIER)
                    {
                        SemaReport(workspace, ReportSeverity_Error, named_expr->text,
                                   "Enum member name cannot be blank");
                        
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        if (named_expr->is_const || named_expr->is_baked || named_expr->is_using)
                        {
                            const char* kind = 0;
                            if      (named_expr->is_const) kind = "const";
                            else if (named_expr->is_baked) kind = "baked";
                            else                           kind = "using";
                            
                            SemaReport(workspace, ReportSeverity_Error, named_expr->value->text,
                                       "Cannot apply the '%s' modifier to a enum member", kind);
                            
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            if (named_expr->type != 0)
                            {
                                SemaReport(workspace, ReportSeverity_Error, named_expr->type->text,
                                           "Cannot apply type specifier to enum member");
                                
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                if (named_expr->value != 0)
                                {
                                    if (!SemaEnsureExpressionEvaluatesToAValue(workspace, named_expr->value) ||
                                        !SemaProcessExpression(workspace, named_expr->value, false, false))
                                    {
                                        encountered_errors = true;
                                    }
                                }
                            }
                        }
                    }
                }
                
                if (!encountered_errors && expr->enum_expr.size != 0)
                {
                    if (!SemaEnsureExpressionEvaluatesToAValue(workspace, expr->enum_expr.size) ||
                        !SemaProcessExpression(workspace, expr->enum_expr.size, false, false))
                    {
                        encountered_errors = true;
                    }
                }
            } break;
            
            case ExprKind_Struct:
            {
                for (U32 i = 0; !encountered_errors && i < expr->struct_expr.args.size; ++i)
                {
                    Named_Expression* named_expr = Array_ElementAt(&expr->struct_expr.args, i);
                    
                    if (named_expr->name == NULL_IDENTIFIER)
                    {
                        SemaReport(workspace, ReportSeverity_Error, named_expr->text,
                                   "Missing name of struct argument");
                        
                        encountered_errors = true;
                    }
                    
                    else if (named_expr->name == BLANK_IDENTIFIER)
                    {
                        SemaReport(workspace, ReportSeverity_Error, named_expr->text,
                                   "Struct argument name cannot be blank");
                        
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        if (named_expr->type == 0)
                        {
                            SemaReport(workspace, ReportSeverity_Error, named_expr->text,
                                       "Missing type specifier for struct argument");
                            
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            if (named_expr->is_const || named_expr->is_baked)
                            {
                                const char* mod = (named_expr->is_baked ? "Baked" : "Const");
                                
                                SemaReport(workspace, ReportSeverity_Warning, named_expr->text,
                                           "%s specifier redundant. All struct arguments are baked by default", mod);
                                
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                // NOTE(soimn): When the ':' token is found, but a type is not present, the
                                //              type is marked for inferral by setting the type pointer
                                //              equal to the value. This information is only usefull for
                                //              correctness checking in the sema stage and should be zeroed
                                //              before the type checking stage
                                if (named_expr->type == named_expr->value)
                                {
                                    named_expr->type = 0;
                                }
                                
                                else
                                {
                                    if (!SemaProcessExpression(workspace, named_expr->type, false, false))
                                    {
                                        encountered_errors = true;
                                    }
                                }
                                
                                if (!encountered_errors &&
                                    !SemaProcessExpression(workspace, named_expr->value, false, false))
                                {
                                    encountered_errors = true;
                                }
                            }
                        }
                    }
                }
                
                for (U32 i = 0; !encountered_errors && i < expr->struct_expr.args.size; ++i)
                {
                    Named_Expression* named_expr = Array_ElementAt(&expr->struct_expr.args, i);
                    
                    if (named_expr->name == NULL_IDENTIFIER)
                    {
                        SemaReport(workspace, ReportSeverity_Error, named_expr->text,
                                   "Missing name of struct member");
                        
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        if (named_expr->is_const || named_expr->is_baked)
                        {
                            const char* mod = (named_expr->is_baked ? "Baked" : "Const");
                            
                            SemaReport(workspace, ReportSeverity_Warning, named_expr->text,
                                       "%s specifier cannot be used on a struct member", mod);
                            
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            if (named_expr->value != 0)
                            {
                                SemaReport(workspace, ReportSeverity_Error, named_expr->text,
                                           "Assignment is not allowed within struct body");
                                
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                if (named_expr->type == 0)
                                {
                                    SemaReport(workspace, ReportSeverity_Error, named_expr->text,
                                               "Missing type specifier for struct member");
                                    
                                    encountered_errors = true;
                                }
                                
                                else
                                {
                                    if (!SemaProcessExpression(workspace, named_expr->type, true, false))
                                    {
                                        encountered_errors = true;
                                    }
                                }
                            }
                        }
                    }
                }
                
                if (!encountered_errors && expr->struct_expr.alignment != 0)
                {
                    if (!SemaEnsureExpressionEvaluatesToAValue(workspace, expr->struct_expr.alignment) ||
                        !SemaProcessExpression(workspace, expr->struct_expr.alignment, false, false))
                    {
                        encountered_errors = true;
                    }
                }
                
                if (!encountered_errors)
                {
                    if (expr->struct_expr.is_union && 
                        (expr->struct_expr.is_strict || expr->struct_expr.is_packed))
                    {
                        SemaReport(workspace, ReportSeverity_Error, expr->text,
                                   "A struct with the union directive applied cannot be strict or packed");
                        
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        if (expr->struct_expr.is_strict == TRI_NIL) 
                            expr->struct_expr.is_strict = workspace->build_options.strict_by_default;
                        
                        if (expr->struct_expr.is_packed == TRI_NIL)
                            expr->struct_expr.is_packed = workspace->build_options.packed_by_default;
                    }
                }
                
            } break;
            
            INVALID_DEFAULT_CASE;
        }
    }
    
    return !encountered_errors;
}

bool
SemaProcessConstantDecl(Workspace* workspace, Statement* statement)
{
    bool encountered_errors = false;
    
    Constant_Declaration* const_decl = &statement->const_decl;
    
    if (const_decl->names.size == 0)
    {
        SemaReport(workspace, ReportSeverity_Error, statement->text,
                   "Constant declaration must contain at least one name");
        
        encountered_errors = true;
    }
    
    else if (const_decl->values.size == 0)
    {
        SemaReport(workspace, ReportSeverity_Error, statement->text,
                   "Constant declaration must define at least one value");
        
        encountered_errors = true;
    }
    
    else if (const_decl->values.size != const_decl->names.size && const_decl->values.size != 1)
    {
        SemaReport(workspace, ReportSeverity_Error, statement->text,
                   "Mismatch between number of names and values in constant declarations");
        
        encountered_errors = true;
    }
    
    else if (const_decl->types.size != const_decl->types.size && const_decl->types.size != 1)
    {
        SemaReport(workspace, ReportSeverity_Error, statement->text,
                   "Mismatch between number of names and types in constant declarations");
        
        encountered_errors = true;
    }
    
    else
    {
        for (U32 i = 0; !encountered_errors && i < const_decl->types.size; ++i)
        {
            if (!SemaProcessExpression(workspace, Array_ElementAt(&const_decl->types, i), true, false))
            {
                encountered_errors = true;
            }
        }
        
        for (U32 i = 0; !encountered_errors && i < const_decl->types.size; ++i)
        {
            Expression* expr = Array_ElementAt(&const_decl->types, i);
            
            if (!SemaEnsureExpressionEvaluatesToAValue(workspace, expr) ||
                !SemaProcessExpression(workspace, expr, false, false))
            {
                encountered_errors = true;
            }
        }
    }
    
    return !encountered_errors;
}

bool
SemaProcessVariableDecl(Workspace* workspace, Statement* statement)
{
    bool encountered_errors = false;
    
    Variable_Declaration* var_decl = &statement->var_decl;
    
    if (var_decl->names.size == 0)
    {
        SemaReport(workspace, ReportSeverity_Error, statement->text,
                   "Variable declaration must contain at least one name");
        
        encountered_errors = true;
    }
    
    else if (var_decl->values.size > 1 && var_decl->names.size != var_decl->values.size)
    {
        SemaReport(workspace, ReportSeverity_Error, statement->text,
                   "Mismatch between number of names and values in variable declaration");
        
        encountered_errors = true;
    }
    
    else if (var_decl->values.size != 0 && var_decl->is_uninitialized)
    {
        SemaReport(workspace, ReportSeverity_Error, statement->text,
                   "Cannot assign values to an uninitialized variable");
        
        encountered_errors = true;
    }
    
    else if (var_decl->types.size > 1 && var_decl->types.size != var_decl->names.size)
    {
        SemaReport(workspace, ReportSeverity_Error, statement->text,
                   "Mismatch between number of names and types in variable declaration");
        
        encountered_errors = true;
    }
    
    else if (var_decl->types.size == 0 && var_decl->values.size == 0)
    {
        SemaReport(workspace, ReportSeverity_Error, statement->text,
                   "Cannot infer type of an uninitialized variable");
        
        encountered_errors = true;
    }
    
    else
    {
        for (U32 i = 0; !encountered_errors && i < var_decl->types.size; ++i)
        {
            if (!SemaProcessExpression(workspace, Array_ElementAt(&var_decl->types, i), true, false))
            {
                encountered_errors = true;
            }
        }
        
        for (U32 i = 0; !encountered_errors && i < var_decl->values.size; ++i)
        {
            Expression* expr = Array_ElementAt(&var_decl->values, i);
            
            if (!SemaEnsureExpressionEvaluatesToAValue(workspace, expr) ||
                !SemaProcessExpression(workspace, expr, false, false))
            {
                encountered_errors = true;
            }
        }
    }
    
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
                              SemaProcessExpression(workspace, expr, false, false)))
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
                    if (!SemaProcessExpression(workspace, statement->if_stmnt.condition, false, false) ||
                        !SemaProcessScope(workspace, &statement->if_stmnt.true_scope, false, in_loop)  ||
                        !SemaProcessScope(workspace, &statement->if_stmnt.false_scope, false, in_loop))
                    {
                        encountered_errors = true;
                    }
                } break;
                
                case StatementKind_While:
                {
                    if (!SemaProcessExpression(workspace, statement->while_stmnt.condition, false, false) ||
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
                        
                        if (!SemaProcessExpression(workspace, expr, false, false))
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
                        
                        if (!SemaProcessExpression(workspace, expr, false, false))
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
                            
                            if (!SemaProcessExpression(workspace, expr, false, false))
                            {
                                encountered_errors = true;
                                break;
                            }
                        }
                    }
                } break;
                
                case StatementKind_Expression:
                {
                    if (!SemaProcessExpression(workspace, statement->expression_stmnt, false, false))
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