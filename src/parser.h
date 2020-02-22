
// TODO(soimn): Improve this to include certain directories and "libraries" as well
inline bool
EnsureFileIsIncludedInCompilation(Module* module, String current_dir, String relative_path, File_ID* file_id)
{
    bool encountered_errors = false;
    
    Memory_Arena scratch = {};
    String absolute_path = {};
    
    if (TryResolveFilePath(&scratch, current_dir, relative_path, &absolute_path))
    {
        bool file_found = false;
        for (Bucket_Array_Iterator it = Iterate(&module->files);
             it.current;
             Advance(&it))
        {
            File* current_file = (File*)it.current;
            
            if (StringCompare(absolute_path, current_file->absolute_file_path))
            {
                *file_id   = (ID)it.current_index;
                file_found = true;
            }
        }
        
        if (!file_found)
        {
            *file_id = (ID)ElementCount(&module->files);
            
            File* file = (File*)PushElement(&module->files);
            file->stage              = CompStage_Init;
            file->is_currently_used  = false;
            file->absolute_file_path = absolute_path;
            
            if (!TryLoadFileContents(&module->file_arena, absolute_path,  &file->file_contents))
            {
                //// ERROR: Failed to load file contents
                encountered_errors = true;
            }
            
            file->ast.nodes = BUCKET_ARRAY(&module->main_arena, AST_Node, AST_NODE_BUCKET_SIZE);
            file->ast.root  = (AST_Node*)PushElement(&file->ast.nodes);
            
            AST_Node* root = file->ast.root;
            root->node_type                = ASTNode_Root;
            root->root_scope.table         = (ID)ElementCount(&module->symbol_tables);
            root->root_scope.num_decls     = 0;
            root->root_scope.using_imports = BUCKET_ARRAY(&module->main_arena, Using_Import, 10);
            root->imported_files           = BUCKET_ARRAY(&module->main_arena, File_ID, 10);
            
            Symbol_Table* table = (Symbol_Table*)PushElement(&module->symbol_tables);
            *table = BUCKET_ARRAY(&module->main_arena, Symbol, SYMBOL_TABLE_BUCKET_SIZE);
        }
    }
    
    else
    {
        //// ERROR: Failed to resolve file path in import directive
        encountered_errors = true;
    }
    
    FreeArena(&scratch);
    
    return !encountered_errors;
}

inline AST_Node*
PushNode(Parser_State* state, Enum32(AST_NODE_TYPE) node_type, Enum32(AST_EXPR_TYPE) expr_type = ASTExpr_Invalid)
{
    ASSERT(node_type != ASTNode_Invalid);
    ASSERT(expr_type == ASTExpr_Invalid || expr_type != ASTExpr_Invalid && node_type == ASTNode_Expression);
    
    AST_Node* node = (AST_Node*)PushElement(&state->ast->nodes);
    node->node_type = node_type;
    node->expr_type = expr_type;
    
    return node;
}

inline String_ID
RegisterString(Parser_State* state, String string)
{
    String_ID result = (ID)ElementCount(&state->module->string_table);
    
    String* entry = (String*)PushElement(&state->module->string_table);
    entry->size = string.size;
    entry->data = string.data;
    
    return result;
}

inline Symbol_Table_ID
PushSymbolTable(Parser_State* state)
{
    Symbol_Table_ID table_id = (ID)ElementCount(&state->module->symbol_tables);
    
    Symbol_Table* table = (Symbol_Table*)PushElement(&state->module->symbol_tables);
    *table = BUCKET_ARRAY(&state->module->main_arena, Symbol, SYMBOL_TABLE_BUCKET_SIZE);
    
    return table_id;
}

inline Symbol*
PushSymbol(Parser_State* state, Symbol_Table_ID table_id, Symbol_ID* out_id)
{
    ASSERT(table_id > 0 && table_id < ElementCount(&state->module->symbol_tables));
    
    Symbol_Table* table = (Symbol_Table*)ElementAt(&state->module->symbol_tables, table_id);
    if (out_id) *out_id = ElementCount(&state->module->symbol_tables);
    
    return (Symbol*)PushElement(table);
}

inline bool
ParseType(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    *result = PushNode(state, ASTNode_Expression, ASTExpr_Type);
    
    AST_Node** current_mod = &(*result)->type_modifiers;
    while (!encountered_errors)
    {
        Token token = GetToken(state);
        
        if (token.type == Token_And)
        {
            *current_mod = PushNode(state, ASTNode_Expression, ASTExpr_ReferenceType);
        }
        
        else if (token.type == Token_OpenBracket)
        {
            SkipPastToken(state, token);
            
            if (token.type == Token_CloseBracket)
            {
                *result = PushNode(state, ASTNode_Expression, ASTExpr_SliceType);
            }
            
            else if (token.type == Token_PeriodPeriod)
            {
                *result = PushNode(state, ASTNode_Expression, ASTExpr_DynamicArrayType);
            }
            
            else
            {
                *result = PushNode(state, ASTNode_Expression, ASTExpr_StaticArrayType);
                
                bool ParseExpression(Parser_State* state, AST_Node** result, bool at_statement_level);
                
                if (!ParseExpression(state, &(*result)->static_array_size, false))
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
        }
        
        else if (token.type == Token_Identifier)
        {
            if (StringCompare(token.string, CONST_STRING("proc")))
            {
                (*result)->type_base = PushNode(state, ASTNode_Expression, ASTExpr_ProcType);
                
                SkipPastToken(state, token);
                token = GetToken(state);
                
                if (token.type == Token_OpenParen)
                {
                    SkipPastToken(state, token);
                    
                    AST_Node** current_arg = &(*result)->type_base->function_arg_types;
                    while (!encountered_errors)
                    {
                        token = GetToken(state);
                        
                        if (token.type == Token_CloseParen)
                        {
                            SkipPastToken(state, token);
                            break;
                        }
                        
                        else
                        {
                            Token peek = PeekNextToken(state, token);
                            if (token.type == Token_Identifier && peek.type == Token_Colon)
                            {
                                SkipPastToken(state, peek);
                            }
                            
                            if (ParseType(state, current_arg))
                            {
                                token = GetToken(state);
                                
                                if (token.type == Token_Comma || token.type == Token_CloseParen)
                                {
                                    if (token.type == Token_Comma)
                                    {
                                        SkipPastToken(state, token);
                                    }
                                    
                                }
                                
                                else
                                {
                                    //// ERROR: Missing closing parenthesis after arguments in function type
                                    encountered_errors = true;
                                }
                            }
                            
                            else
                            {
                                //// ERROR
                                encountered_errors = true;
                            }
                            
                            current_arg = &(*current_arg)->next;
                        }
                    }
                }
                
                token = GetToken(state);
                if (!encountered_errors && token.type == Token_Arrow)
                {
                    SkipPastToken(state, token);
                    token = GetToken(state);
                    
                    if (token.type == Token_OpenParen)
                    {
                        SkipPastToken(state, token);
                        
                        AST_Node** current_value = &(*result)->type_base->function_return_types;
                        while (!encountered_errors)
                        {
                            token = GetToken(state);
                            
                            if (token.type == Token_CloseParen)
                            {
                                SkipPastToken(state, token);
                                break;
                            }
                            
                            else
                            {
                                Token peek = PeekNextToken(state, token);
                                if (token.type == Token_Identifier && peek.type == Token_Colon)
                                {
                                    SkipPastToken(state, peek);
                                }
                                
                                if (ParseType(state, current_value))
                                {
                                    token = GetToken(state);
                                    
                                    if (token.type == Token_Comma || token.type == Token_CloseParen)
                                    {
                                        if (token.type == Token_Comma)
                                        {
                                            SkipPastToken(state, token);
                                        }
                                        
                                    }
                                    
                                    else
                                    {
                                        //// ERROR: Missing closing parenthesis after return types in function type
                                        encountered_errors = true;
                                    }
                                }
                                
                                else
                                {
                                    //// ERROR
                                    encountered_errors = true;
                                }
                                
                                current_value = &(*current_value)->next;
                            }
                        }
                    }
                    
                    else
                    {
                        if (!ParseType(state, &(*result)->type_base->function_return_types))
                        {
                            //// ERROR
                            encountered_errors = true;
                        }
                    }
                }
            }
            
            else
            {
                (*result)->type_base = PushNode(state, ASTNode_Expression, ASTExpr_Identifier);
                (*result)->identifier = RegisterString(state, token.string);
            }
        }
        
        else
        {
            //// ERROR: Invalid identifier in type
            encountered_errors = true;
        }
        
        current_mod = &(*current_mod)->next;
    }
    
    return !encountered_errors;
}

// NOTE(soimn): Operator precedence
// 0. postfix
// 1. unary
// 2. infix functions
// 3. mult level (mult, div, mod, bitand, bitshift)
// 4. add level (add, sub, bitor)
// 5. comparative
// 6. logical and
// 7. logical or
// 8. Sequencing (comma)
// 9. statement level (assignment, increment)

// NOTE(soimn): All operators on the same level of precedence are left associative, except unary operators, which 
//              are right-associative

inline bool
ParsePostfixOrPrimaryExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    while (!encountered_errors)
    {
        Token token = GetToken(state);
        
        if (token.type == Token_Identifier)
        {
            if (*result == 0)
            {
                *result = PushNode(state, ASTNode_Expression, ASTExpr_Identifier);
                (*result)->identifier = RegisterString(state, token.string);
                
                SkipPastToken(state, token);
            }
            
            else if ((*result)->expr_type == ASTExpr_Member)
            {
                if ((*result)->right == 0)
                {
                    (*result)->right = PushNode(state, ASTNode_Expression, ASTExpr_Identifier);
                    (*result)->right->identifier = RegisterString(state, token.string);
                    
                    SkipPastToken(state, token);
                }
                
                else
                {
                    //// ERROR: Identifiers cannot be used as postfix operators
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Identifiers cannot be used as postfix operators
                encountered_errors = true;
            }
        }
        
        else if (token.type == Token_Number)
        {
            if (*result == 0)
            {
                *result = PushNode(state, ASTNode_Expression, ASTExpr_Number);
                (*result)->number = token.number;
                
                SkipPastToken(state, token);
            }
            
            else
            {
                //// ERROR: Numbers cannot be used as postfix operators
                encountered_errors = true;
            }
        }
        
        else if (token.type == Token_String)
        {
            if (*result == 0)
            {
                *result = PushNode(state, ASTNode_Expression, ASTExpr_String);
                (*result)->string = token.string;
                
                SkipPastToken(state, token);
            }
            
            else
            {
                //// ERROR: Strings cannot be used as postfix operators
                encountered_errors = true;
            }
        }
        
        else if (token.type == Token_OpenParen)
        {
            if (*result == 0 || (*result)->expr_type == ASTExpr_Member)
            {
                //// ERROR: Missing function pointer before open parenthesis in function call expression
                encountered_errors = true;
            }
            
            else
            {
                AST_Node* call_pointer = *result;
                
                *result = PushNode(state, ASTNode_Expression, ASTExpr_Call);
                (*result)->call_pointer = call_pointer;
                
                SkipPastToken(state, token);
                token = GetToken(state);
                
                if (token.type != Token_CloseParen)
                {
                    bool ParseSequenceExpression(Parser_State* state, AST_Node** result);
                    if (!ParseSequenceExpression(state, &(*result)->call_arguments))
                    {
                        //// ERROR
                        encountered_errors = true;
                    }
                }
                
                if (!encountered_errors && !MetRequiredToken(state, Token_CloseParen))
                {
                    //// ERROR: Missing closing parenthesis after arguments in function call expression
                    encountered_errors = true;
                }
            }
        }
        
        else if (token.type == Token_OpenBracket)
        {
            if (*result == 0 || (*result)->expr_type == ASTExpr_Member)
            {
                //// ERROR: Missing expression before subscript operator
                encountered_errors = true;
            }
            
            else
            {
                SkipPastToken(state, token);
                token = GetToken(state);
                
                AST_Node* pointer = *result;
                AST_Node* start   = 0;
                AST_Node* length  = 0;
                
                bool ParseExpression(Parser_State* state, AST_Node** result, bool at_statement_level);
                
                if (token.type != Token_Colon)
                {
                    if (!ParseExpression(state, &start, false))
                    {
                        //// ERROR
                        encountered_errors = true;
                    }
                    
                    token = GetToken(state);
                }
                
                if (!encountered_errors)
                {
                    if (token.type == Token_Colon)
                    {
                        SkipPastToken(state, token);
                        token = GetToken(state);
                        
                        if (token.type != Token_CloseBracket)
                        {
                            if (!ParseExpression(state, &length, false))
                            {
                                //// ERROR
                                encountered_errors = true;
                            }
                        }
                        
                        if (!encountered_errors)
                        {
                            *result = PushNode(state, ASTNode_Expression, ASTExpr_Slice);
                            (*result)->slice_pointer = pointer;
                            (*result)->slice_start   = start;
                            (*result)->slice_length  = length;
                        }
                    }
                    
                    else
                    {
                        *result = PushNode(state, ASTNode_Expression, ASTExpr_Subscript);
                        (*result)->left  = pointer;
                        (*result)->right = start;
                    }
                    
                    if (!encountered_errors && !MetRequiredToken(state, Token_CloseBracket))
                    {
                        //// ERROR: Missing closing bracket after slice/subscript
                        encountered_errors = true;
                    }
                }
            }
        }
        
        else if (token.type == Token_Period)
        {
            if (*result == 0)
            {
                //// ERROR: Missing expression before member operator
                encountered_errors = true;
            }
            
            else
            {
                SkipPastToken(state, token);
                
                AST_Node* left = *result;
                
                *result = PushNode(state, ASTNode_Expression, ASTExpr_Member);
                (*result)->left = left;
            }
        }
        
        else
        {
            if (*result == 0)
            {
                //// ERROR: Missing primary expression
                encountered_errors = true;
            }
            
            else if ((*result)->expr_type == ASTExpr_Member && (*result)->right == 0)
            {
                //// ERROR: Missing right hand side of member operator
                encountered_errors = true;
            }
            
            break;
        }
    }
    
    return !encountered_errors;
}

inline bool
ParseUnaryExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    AST_Node** current_expr = result;
    while (!encountered_errors)
    {
        Token token = GetToken(state);
        Token peek  = PeekNextToken(state, token);
        
        if (token.type == Token_Plus)
        {
            SkipPastToken(state, token);
        }
        
        else if (token.type == Token_Minus)
        {
            SkipPastToken(state, token);
            
            *current_expr = PushNode(state, ASTNode_Expression, ASTExpr_Negate);
            current_expr = &(*current_expr)->operand;
        }
        
        else if (token.type == Token_Asterisk)
        {
            SkipPastToken(state, token);
            
            *current_expr = PushNode(state, ASTNode_Expression, ASTExpr_Dereference);
            current_expr = &(*current_expr)->operand;
        }
        
        else if (token.type == Token_And)
        {
            SkipPastToken(state, token);
            
            *current_expr = PushNode(state, ASTNode_Expression, ASTExpr_Reference);
            current_expr = &(*current_expr)->operand;
        }
        
        else if (token.type == Token_Identifier &&
                 (StringCompare(token.string, CONST_STRING("cast")) ||
                  StringCompare(token.string, CONST_STRING("transmute"))))
        {
            bool is_cast = StringCompare(token.string, CONST_STRING("cast"));
            SkipPastToken(state, token);
            
            *current_expr = PushNode(state, ASTNode_Expression, (is_cast ? ASTExpr_TypeCast : ASTExpr_TypeTransmute));
            
            if (MetRequiredToken(state, Token_OpenParen))
            {
                if (ParseType(state, &(*current_expr)->cast_transmute_type))
                {
                    if (MetRequiredToken(state, Token_CloseParen))
                    {
                        current_expr = &(*current_expr)->operand;
                    }
                    
                    else
                    {
                        //// ERROR: Missing closing parenthesis after target type in cast/trasnmute expression
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Missing target type after cast/transmute keyword in cast/transmute expression
                encountered_errors = true;
            }
        }
        
        else if (token.type == Token_Identifier && peek.type == Token_Quote)
        {
            String_ID function_name = RegisterString(state, token.string);
            SkipPastToken(state, peek);
            
            *current_expr = PushNode(state, ASTNode_Expression, ASTExpr_Call);
            (*current_expr)->function_name = function_name;
            
            if (ParseUnaryExpression(state, &(*current_expr)->call_arguments))
            {
                break;
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        else
        {
            if (!ParsePostfixOrPrimaryExpression(state, current_expr))
            {
                //// ERROR
                encountered_errors = true;
            }
            
            break;
        }
    }
    
    return !encountered_errors;
}

inline bool
ParseInfixFunctionExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseUnaryExpression(state, result))
    {
        Token token = GetToken(state);
        
        if (token.type == Token_Quote || token.type == Token_QuoteQuote)
        {
            bool is_unary = (token.type == Token_Quote);
            
            AST_Node* first_arg = *result;
            SkipPastToken(state, token);
            token = GetToken(state);
            
            if (token.type == Token_Identifier)
            {
                String_ID function_name = RegisterString(state, token.string);
                SkipPastToken(state, token);
                
                *result = PushNode(state, ASTNode_Expression, ASTExpr_Call);
                (*result)->function_name  = function_name;
                (*result)->call_arguments = first_arg;
                
                if (!is_unary)
                {
                    if (!ParseUnaryExpression(state, &(*result)->call_arguments->next))
                    {
                        //// ERROR
                        encountered_errors = true;
                    }
                }
            }
            
            else
            {
                //// ERROR: Missing function name after quotation marks in infix function call expression
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseMultLevelExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseInfixFunctionExpression(state, result))
    {
        while (!encountered_errors)
        {
            Token token = GetToken(state);
            
            Enum32(AST_EXPR_TYPE) type = ASTExpr_Invalid;
            switch (token.type)
            {
                case Token_Asterisk:       type = ASTExpr_Mul;           break;
                case Token_ForwardSlash:   type = ASTExpr_Div;           break;
                case Token_Percentage:     type = ASTExpr_Mod;           break;
                case Token_And:            type = ASTExpr_BitAnd;        break;
                case Token_GreaterGreater: type = ASTExpr_BitShiftRight; break;
                case Token_LessLess:       type = ASTExpr_BitShiftLeft;  break;
            }
            
            if (type != ASTExpr_Invalid)
            {
                AST_Node* left = *result;
                SkipPastToken(state, token);
                
                *result = PushNode(state, ASTNode_Expression, type);
                (*result)->left = left;
                
                if (!ParseInfixFunctionExpression(state, &(*result)->right))
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
            
            else break;
        }
    }
    
    else
    {
        //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseAddLevelExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseMultLevelExpression(state, result))
    {
        while (!encountered_errors)
        {
            Token token = GetToken(state);
            
            Enum32(AST_EXPR_TYPE) type = ASTExpr_Invalid;
            switch (token.type)
            {
                case Token_Plus:  type = ASTExpr_Add;   break;
                case Token_Minus: type = ASTExpr_Sub;   break;
                case Token_Pipe:  type = ASTExpr_BitOr; break;
            }
            
            if (type != ASTExpr_Invalid)
            {
                AST_Node* left = *result;
                SkipPastToken(state, token);
                
                *result = PushNode(state, ASTNode_Expression, type);
                (*result)->left = left;
                
                if (!ParseMultLevelExpression(state, &(*result)->right))
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
            
            else break;
        }
    }
    
    else
    {
        //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseComparativeExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseAddLevelExpression(state, result))
    {
        Token token = GetToken(state);
        
        Enum32(AST_EXPR_TYPE) type = ASTExpr_Invalid;
        switch (token.type)
        {
            case Token_EqualsEquals:  type = ASTExpr_CmpEqual;          break;
            case Token_ExclamationEQ: type = ASTExpr_CmpNotEqual;       break;
            case Token_Less:          type = ASTExpr_CmpLess;           break;
            case Token_LessEQ:        type = ASTExpr_CmpLessOrEqual;    break;
            case Token_Greater:       type = ASTExpr_CmpGreater;        break;
            case Token_GreaterEQ:     type = ASTExpr_CmpGreaterOrEqual; break;
        }
        
        if (type != ASTExpr_Invalid)
        {
            AST_Node* left = *result;
            SkipPastToken(state, token);
            
            *result = PushNode(state, ASTNode_Expression, type);
            (*result)->left = left;
            
            if (ParseAddLevelExpression(state, &(*result)->right))
            {
                token = GetToken(state);
                
                switch (token.type)
                {
                    case Token_EqualsEquals:
                    case Token_ExclamationEQ:
                    case Token_Less:
                    case Token_LessEQ:
                    case Token_Greater:
                    case Token_GreaterEQ:
                    {
                        //// ERROR: Chaining of comparative operators is not allowed
                        encountered_errors = true;
                    } break;
                }
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseLogicalAndExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseComparativeExpression(state, result))
    {
        Token token = GetToken(state);
        
        if (token.type == Token_AndAnd)
        {
            AST_Node* left = *result;
            SkipPastToken(state, token);
            
            *result = PushNode(state, ASTNode_Expression, ASTExpr_LogicalAnd);
            (*result)->left = left;
            
            if (!ParseLogicalAndExpression(state, &(*result)->right))
            {
                //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseLogicalOrExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseLogicalAndExpression(state, result))
    {
        Token token = GetToken(state);
        
        if (token.type == Token_PipePipe)
        {
            AST_Node* left = *result;
            SkipPastToken(state, token);
            
            *result = PushNode(state, ASTNode_Expression, ASTExpr_LogicalOr);
            (*result)->left = left;
            
            if (!ParseLogicalOrExpression(state, &(*result)->right))
            {
                //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

bool
ParseSequenceExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    AST_Node** current_expr = result;
    
    while (!encountered_errors)
    {
        if (ParseLogicalOrExpression(state, current_expr))
        {
            Token token = GetToken(state);
            
            if (token.type == Token_Comma) SkipPastToken(state, token);
            else break;
        }
        
        else
        {
            //// ERROR
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

bool
ParseExpression(Parser_State* state, AST_Node** result, bool at_statement_level)
{
    bool encountered_errors = false;
    
    if (ParseSequenceExpression(state, result))
    {
        Token token = GetToken(state);
        
        bool is_assignment         = false;
        Enum32(AST_EXPR_TYPE) type = ASTExpr_Invalid;
        
        switch (token.type)
        {
            
            case Token_Equals:           type = ASTExpr_Assignment;      is_assignment = true; break;
            case Token_PlusEQ:           type = ASTExpr_AddEQ;           is_assignment = true; break;
            case Token_MinusEQ:          type = ASTExpr_SubEQ;           is_assignment = true; break;
            case Token_AsteriskEQ:       type = ASTExpr_MulEQ;           is_assignment = true; break;
            case Token_ForwardSlashEQ:   type = ASTExpr_DivEQ;           is_assignment = true; break;
            case Token_PercentageEQ:     type = ASTExpr_ModEQ;           is_assignment = true; break;
            case Token_AndEQ:            type = ASTExpr_BitAndEQ;        is_assignment = true; break;
            case Token_PipeEQ:           type = ASTExpr_BitOrEQ;         is_assignment = true; break;
            case Token_AndAndEQ:         type = ASTExpr_LogicalAndEQ;    is_assignment = true; break;
            case Token_PipePipeEQ:       type = ASTExpr_LogicalOrEQ;     is_assignment = true; break;
            case Token_GreaterGreaterEQ: type = ASTExpr_BitShiftRightEQ; is_assignment = true; break;
            case Token_LessLessEQ:       type = ASTExpr_BitShiftLeftEQ;  is_assignment = true; break;
            
            case Token_PlusPlus:   type = ASTExpr_Increment; break;
            case Token_MinusMinus: type = ASTExpr_Decrement; break;
        }
        
        if (type != ASTExpr_Invalid)
        {
            SkipPastToken(state, token);
            
            if (is_assignment)
            {
                if (at_statement_level)
                {
                    AST_Node* left = *result;
                    
                    *result = PushNode(state, ASTNode_Expression, type);
                    (*result)->left = left;
                    
                    if (!ParseSequenceExpression(state, &(*result)->right))
                    {
                        //// ERROR
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Assignment is not allowed in this context. Assignment is only allowed at the 
                    ////        statement level
                    encountered_errors = true;
                }
            }
            
            else
            {
                if (at_statement_level)
                {
                    AST_Node* operand = *result;
                    
                    *result = PushNode(state, ASTNode_Expression, type);
                    (*result)->operand = operand;
                }
                
                else
                {
                    //// ERROR: Increment/Decrement is not allowed in this context. Increment/Decrement is only 
                    ////        allowed at the statement level
                    encountered_errors = true;
                }
            }
        }
    }
    
    else
    {
        //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

bool
ParseScope(Parser_State* state, Parser_Context context,  AST_Node** result, bool require_braces)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state);
    
    bool is_braced = false;
    if (token.type == Token_OpenBrace)
    {
        is_braced = true;
        
        SkipPastToken(state, token);
        token = GetToken(state);
    }
    
    if (require_braces && !is_braced)
    {
        //// ERROR: Braces are required for scope delimitation in the current context
        encountered_errors = true;
    }
    
    *result = PushNode(state, ASTNode_Scope);
    (*result)->scope.table         = PushSymbolTable(state);
    (*result)->scope.num_decls     = 0;
    (*result)->scope.using_imports = BUCKET_ARRAY(&state->module->main_arena, Using_Import, 10);
    
    bool is_done = false;
    AST_Node** current_statement = &(*result)->first_in_scope;
    while (!encountered_errors && !is_done)
    {
        token = GetToken(state);
        
        if (token.type == Token_CloseBrace)
        {
            if (is_braced)
            {
                SkipPastToken(state, token);
                is_done = true;
            }
            
            else
            {
                //// ERROR: Unexpected closing brace in scope that is not brace delimited
                encountered_errors = true;
            }
        }
        
        else if (token.type == Token_EndOfStream)
        {
            //// ERROR: Unexpected EOF in scope
            encountered_errors = true;
        }
        
        else
        {
            bool ParseStatement(Parser_State* state, Parser_Context context, AST_Node** result);
            
            Parser_Context stmnt_context = context;
            stmnt_context.scope = &(*result)->scope;
            
            if (ParseStatement(state, stmnt_context, current_statement))
            {
                if (*current_statement != 0) current_statement = &(*current_statement)->next;
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        if (!is_braced) is_done = true;
    }
    
    return !encountered_errors;
}

bool
ParseStatement(Parser_State* state, Parser_Context context, AST_Node** result)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state);
    
    if (token.type == Token_EndOfStream || token.type == Token_Error)
    {
        if (token.type == Token_EndOfStream)
        {
            //// ERROR: Unexpected EOF in statement
        }
        
        encountered_errors = true;
    }
    
    else if (token.type == Token_OpenBrace)
    {
        if (context.allow_subscopes)
        {
            Parser_Context subscope_context = context;
            subscope_context.allow_loose_expressions = true;
            subscope_context.allow_using             = true;
            subscope_context.allow_defer             = true;
            subscope_context.allow_ctrl_structs      = true;
            subscope_context.allow_var_decls         = true;
            subscope_context.allow_proc_decls        = true;
            subscope_context.allow_struct_decls      = true;
            subscope_context.allow_enum_decls        = true;
            subscope_context.allow_const_decls       = true;
            
            if (!ParseScope(state, subscope_context, result, true))
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Subscopes are illegal in the current context
            encountered_errors = true;
        }
    }
    
    else if (token.type == Token_Semicolon)
    {
        SkipPastToken(state, token);
    }
    
    else if (token.type == Token_Identifier)
    {
        if (StringCompare(token.string, CONST_STRING("if")))
        {
            if (context.allow_ctrl_structs)
            {
                SkipPastToken(state, token);
                
                *result = PushNode(state, ASTNode_IfElse);
                
                if (MetRequiredToken(state, Token_OpenParen))
                {
                    if (ParseExpression(state, &(*result)->if_condition, false))
                    {
                        Parser_Context if_context = context;
                        if_context.allow_loose_expressions = true;
                        if_context.allow_subscopes         = true;
                        if_context.allow_using             = true;
                        if_context.allow_defer             = true;
                        if_context.allow_var_decls         = true;
                        if_context.allow_proc_decls        = true;
                        if_context.allow_struct_decls      = true;
                        if_context.allow_enum_decls        = true;
                        if_context.allow_const_decls       = true;
                        
                        if (MetRequiredToken(state, Token_CloseParen))
                        {
                            if (ParseScope(state, if_context, &(*result)->true_body, false))
                            {
                                token = GetToken(state);
                                
                                if (token.type == Token_Identifier && StringCompare(token.string, CONST_STRING("else")))
                                {
                                    SkipPastToken(state, token);
                                    
                                    if (!ParseScope(state, if_context, &(*result)->false_body, false))
                                    {
                                        //// ERROR
                                        encountered_errors = true;
                                    }
                                }
                            }
                            
                            else
                            {
                                //// ERROR
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            //// ERROR: Missing closing parenthesis after if statement condition
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Missing condition after if keyword in if statement
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Control structures are illegal in the current context
                encountered_errors = true;
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("else")))
        {
            //// ERROR: Illegal else without matching if
            encountered_errors = true;
        }
        
        else if (StringCompare(token.string, CONST_STRING("while")))
        {
            if (context.allow_ctrl_structs)
            {
                SkipPastToken(state, token);
                
                *result = PushNode(state, ASTNode_While);
                
                if (MetRequiredToken(state, Token_OpenParen))
                {
                    if (ParseExpression(state, &(*result)->while_condition, false))
                    {
                        if (MetRequiredToken(state, Token_CloseParen))
                        {
                            Parser_Context while_context = context;
                            while_context.allow_loose_expressions = true;
                            while_context.allow_subscopes         = true;
                            while_context.allow_using             = true;
                            while_context.allow_defer             = true;
                            while_context.allow_var_decls         = true;
                            while_context.allow_proc_decls        = true;
                            while_context.allow_struct_decls      = true;
                            while_context.allow_enum_decls        = true;
                            while_context.allow_const_decls       = true;
                            
                            if (!ParseScope(state, while_context, &(*result)->while_body, false))
                            {
                                //// ERROR
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            //// ERROR: Missing closing parenthesis after condition in while statement
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Missing condition after while keyword in while statement
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Control structures are illegal in the current context
                encountered_errors = true;
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("for")))
        {
            if (context.allow_ctrl_structs)
            {
                NOT_IMPLEMENTED;
            }
            
            else
            {
                //// ERROR: Control structures are illegal in the current context
                encountered_errors = true;
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("return")))
        {
            if (context.allow_return)
            {
                SkipPastToken(state, token);
                
                *result = PushNode(state, ASTNode_Return);
                
                if (ParseExpression(state, &(*result)->return_value, false))
                {
                    if (!MetRequiredToken(state, Token_Semicolon))
                    {
                        //// ERROR: Missing terminating semicolon after return statement
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Return statements are illegal in the current context
                encountered_errors = true;
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("using")))
        {
            if (context.allow_using)
            {
                ASSERT(context.allow_loose_expressions || context.allow_var_decls ||
                       context.allow_enum_decls);
                
                SkipPastToken(state, token);
                
                *result = PushNode(state, ASTNode_Using);
                
                Parser_Context using_context = {};
                using_context.scope = context.scope;
                
                using_context.allow_subscopes    = false;
                using_context.allow_using        = false;
                using_context.allow_defer        = false;
                using_context.allow_return       = false;
                using_context.allow_loop_jmp     = false;
                using_context.allow_ctrl_structs = false;
                using_context.allow_proc_decls   = false;
                using_context.allow_struct_decls = false;
                using_context.allow_const_decls  = false;
                
                using_context.allow_loose_expressions = context.allow_loose_expressions;
                using_context.allow_var_decls         = context.allow_var_decls;
                using_context.allow_enum_decls        = context.allow_enum_decls;
                
                if (!ParseStatement(state, using_context, &(*result)->using_statement))
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Using declarations are illegal in the current context
                encountered_errors = true;
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("defer")))
        {
            if (context.allow_defer)
            {
                SkipPastToken(state, token);
                
                *result = PushNode(state, ASTNode_Defer);
                
                Parser_Context defer_context = context;
                defer_context.allow_subscopes         = true;
                defer_context.allow_loose_expressions = true;
                defer_context.allow_using             = true;
                defer_context.allow_ctrl_structs      = true;
                defer_context.allow_var_decls         = true;
                defer_context.allow_proc_decls        = true;
                defer_context.allow_struct_decls      = true;
                defer_context.allow_enum_decls        = true;
                defer_context.allow_const_decls       = true;
                
                if (!ParseScope(state, defer_context, &(*result)->defer_statement, false))
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Defer statements are illegal in the current context
                encountered_errors = true;
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("break")) || 
                 StringCompare(token.string, CONST_STRING("continue")))
        {
            if (context.allow_loop_jmp)
            {
                bool is_break = StringCompare(token.string, CONST_STRING("break"));
                SkipPastToken(state, token);
                
                *result = PushNode(state, (is_break ? ASTNode_Break : ASTNode_Continue));
                
                if (!MetRequiredToken(state, Token_Semicolon))
                {
                    //// ERROR: Missing semicolon after loop jump statement
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Loop flow control statements are illegal in the current context
                encountered_errors = true;
            }
        }
        
        else
        {
            Token peek = PeekNextToken(state, token);
            
            if (peek.type == Token_Colon)
            {
                if (context.allow_var_decls)
                {
                    String_ID name = RegisterString(state, token.string);
                    
                    SkipPastToken(state, peek);
                    token = GetToken(state);
                    
                    *result = PushNode(state, ASTNode_VarDecl);
                    ++context.scope->num_decls;
                    
                    Symbol* symbol = PushSymbol(state, context.scope->table, &(*result)->symbol);
                    symbol->symbol_type = Symbol_Var;
                    symbol->name        = name;
                    symbol->type        = INVALID_ID;
                    
                    if (token.type != Token_Equals)
                    {
                        if (!ParseType(state, &(*result)->var_type))
                        {
                            //// ERROR
                            encountered_errors = true;
                        }
                        
                        token = GetToken(state);
                    }
                    
                    if (!encountered_errors && token.type == Token_Equals)
                    {
                        SkipPastToken(state, token);
                        
                        if (!ParseExpression(state, &(*result)->var_value, false))
                        {
                            //// ERROR
                            encountered_errors = true;
                        }
                    }
                    
                    if (!encountered_errors && !MetRequiredToken(state, Token_Semicolon))
                    {
                        //// ERROR: Missing semicolon after variable declaration
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Variable declarations are illegal in the current context
                    encountered_errors = true;
                }
            }
            
            else if (peek.type == Token_ColonColon)
            {
                peek = PeekNextToken(state, peek);
                
                if (peek.type == Token_Identifier)
                {
                    if (StringCompare(peek.string, CONST_STRING("proc")))
                    {
                        if (context.allow_proc_decls)
                        {
                            String_ID name = RegisterString(state, token.string);
                            SkipPastToken(state, peek);
                            
                            *result = PushNode(state, ASTNode_FuncDecl);
                            ++context.scope->num_decls;
                            
                            Symbol* symbol = PushSymbol(state, context.scope->table, &(*result)->symbol);
                            symbol->symbol_type = Symbol_Func;
                            symbol->name        = name;
                            symbol->type        = INVALID_ID;
                            
                            // TODO(soimn): How should function arguments, and default values be stored?
                            NOT_IMPLEMENTED;
                            
                            Parser_Context function_context = {};
                            function_context.scope = context.scope;
                            
                            function_context.allow_loop_jmp = false;
                            
                            function_context.allow_loose_expressions = true;
                            function_context.allow_subscopes         = true;
                            function_context.allow_using             = true;
                            function_context.allow_defer             = true;
                            function_context.allow_return            = true;
                            function_context.allow_ctrl_structs      = true;
                            function_context.allow_var_decls         = true;
                            function_context.allow_proc_decls        = true;
                            function_context.allow_struct_decls      = true;
                            function_context.allow_enum_decls        = true;
                            function_context.allow_const_decls       = true;
                            
                            if (!ParseScope(state, function_context, &(*result)->function_body, true))
                            {
                                //// ERROR
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            //// ERROR: Procedure declarations are illegal in the current context
                            encountered_errors = true;
                        }
                    }
                    
                    else if (StringCompare(peek.string, CONST_STRING("struct")))
                    {
                        if (context.allow_struct_decls)
                        {
                            String_ID name = RegisterString(state, token.string);
                            
                            SkipPastToken(state, peek);
                            token = GetToken(state);
                            
                            *result = PushNode(state, ASTNode_StructDecl);
                            ++context.scope->num_decls;
                            
                            Symbol* symbol = PushSymbol(state, context.scope->table, &(*result)->symbol);
                            symbol->symbol_type = Symbol_TypeName;
                            symbol->name        = name;
                            symbol->type        = INVALID_ID;
                            
                            Parser_Context struct_context = {};
                            struct_context.scope                   = context.scope;
                            struct_context.allow_loose_expressions = false;
                            struct_context.allow_subscopes         = false;
                            struct_context.allow_defer             = false;
                            struct_context.allow_return            = false;
                            struct_context.allow_loop_jmp          = false;
                            struct_context.allow_ctrl_structs      = false;
                            struct_context.allow_proc_decls        = false;
                            struct_context.allow_struct_decls      = false;
                            struct_context.allow_enum_decls        = false;
                            struct_context.allow_const_decls       = false;
                            
                            struct_context.allow_using             = true;
                            struct_context.allow_var_decls         = true;
                            
                            if (!ParseScope(state, struct_context, &(*result)->struct_members, true))
                            {
                                //// ERROR
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            //// ERROR: Struct declarations are illegal in the current context
                            encountered_errors = true;
                        }
                    }
                    
                    else if (StringCompare(peek.string, CONST_STRING("enum")))
                    {
                        if (context.allow_enum_decls)
                        {
                            String_ID name = RegisterString(state, token.string);
                            
                            SkipPastToken(state, peek);
                            token = GetToken(state);
                            
                            *result = PushNode(state, ASTNode_EnumDecl);
                            ++context.scope->num_decls;
                            
                            Symbol* symbol = PushSymbol(state, context.scope->table, &(*result)->symbol);
                            symbol->symbol_type = Symbol_TypeName;
                            symbol->name        = name;
                            symbol->type        = INVALID_ID;
                            
                            if (token.type != Token_OpenBrace)
                            {
                                if (!ParseType(state, &(*result)->enum_type))
                                {
                                    //// ERROR
                                    encountered_errors = true;
                                }
                            }
                            
                            Parser_Context enum_context = {};
                            enum_context.scope                   = context.scope;
                            enum_context.allow_subscopes         = false;
                            enum_context.allow_using             = false;
                            enum_context.allow_defer             = false;
                            enum_context.allow_return            = false;
                            enum_context.allow_loop_jmp          = false;
                            enum_context.allow_ctrl_structs      = false;
                            enum_context.allow_var_decls         = false;
                            enum_context.allow_proc_decls        = false;
                            enum_context.allow_struct_decls      = false;
                            enum_context.allow_enum_decls        = false;
                            enum_context.allow_const_decls       = false;
                            
                            enum_context.allow_loose_expressions = true;
                            
                            if (!encountered_errors)
                            {
                                // HACK(soimn): To produce better error messages the enum body is first parsed as a 
                                //              regular scope which only allows loose expressions, then the expressions 
                                //              are iterated, validated and changed from assignments to enum members.
                                
                                if (ParseScope(state, enum_context, &(*result)->enum_members, true))
                                {
                                    for (AST_Node* scan = (*result)->enum_members; scan; scan = scan->next)
                                    {
                                        if (scan->expr_type != ASTExpr_Assignment &&
                                            scan->left->expr_type == ASTExpr_Identifier)
                                        {
                                            String_ID member_name  = scan->left->identifier;
                                            AST_Node* member_value = scan->right;
                                            
                                            *scan = {};
                                            scan->node_type = ASTNode_EnumMemberDecl;
                                            scan->enum_member_value = member_value;
                                            
                                            Symbol* member_symbol = PushSymbol(state, (*result)->enum_members->scope.table, &scan->symbol);
                                            member_symbol->symbol_type = Symbol_EnumValue;
                                            member_symbol->name        = member_name;
                                            member_symbol->type        = INVALID_ID;
                                        }
                                        
                                        else
                                        {
                                            //// ERROR: Illegal declaration of enum value
                                            encountered_errors = true;
                                        }
                                    }
                                }
                                
                                else
                                {
                                    //// ERROR
                                    encountered_errors = true;
                                }
                            }
                        }
                        
                        else
                        {
                            //// ERROR: Enum declarations are illegal in the current context
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        if (context.allow_const_decls)
                        {
                            String_ID name = RegisterString(state, token.string);
                            
                            SkipPastToken(state, token);
                            token = GetToken(state);
                            SkipPastToken(state, token);
                            
                            *result = PushNode(state, ASTNode_ConstDecl);
                            
                            Symbol* symbol = PushSymbol(state, context.scope->table, &(*result)->symbol);
                            symbol->symbol_type = Symbol_Constant;
                            symbol->name        = name;
                            symbol->type        = INVALID_ID;
                            
                            if (ParseExpression(state, &(*result)->const_value, false))
                            {
                                if (!MetRequiredToken(state, Token_Semicolon))
                                {
                                    //// ERROR: Missing semicolon after loose expression
                                    encountered_errors = true;
                                }
                            }
                            
                            else
                            {
                                //// ERROR
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            //// ERROR: Constant declarations are illegal in the current context
                            encountered_errors = true;
                        }
                    }
                }
                
                else
                {
                    if (context.allow_loose_expressions)
                    {
                        if (ParseExpression(state, result, true))
                        {
                            if (!MetRequiredToken(state, Token_Semicolon))
                            {
                                //// ERROR: Missing semicolon after loose expression
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            //// ERROR
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Loose expressions are illegal in the current context
                        encountered_errors = true;
                    }
                }
            }
        }
    }
    
    else
    {
        if (context.allow_loose_expressions)
        {
            if (ParseExpression(state, result, true))
            {
                if (!MetRequiredToken(state, Token_Semicolon))
                {
                    //// ERROR: Missing semicolon after loose expression
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Loose expressions are illegal in the current context
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

bool
ParseFile(Module* module, File_ID file_id)
{
    bool encountered_errors = true;
    
    File* file               = (File*)ElementAt(&module->files, file_id);
    Export_Info* export_info = (Export_Info*)ElementAt(&module->export_table, file_id);
    
    Parser_State state = {};
    state.lexer_state.current    = file->file_contents.data;
    state.lexer_state.line_start = file->file_contents.data;
    state.lexer_state.column     = 0;
    state.lexer_state.line       = 0;
    state.ast    = &file->ast;
    state.module = module;
    
    AST_Node* root = state.ast->root;
    
    bool should_export = true;
    AST_Node** current_statement = &root->first_in_scope;
    while (!encountered_errors)
    {
        Token token = GetToken(&state);
        
        if (token.type == Token_Hash)
        {
            SkipPastToken(&state, token);
            token = GetToken(&state);
            
            if (token.type == Token_Identifier)
            {
                if (StringCompare(token.string, CONST_STRING("import")) ||
                    StringCompare(token.string, CONST_STRING("import_local")))
                {
                    bool is_exported = (StringCompare(token.string, CONST_STRING("import")) && should_export);
                    
                    SkipPastToken(&state, token);
                    token = GetToken(&state);
                    
                    if (token.type == Token_String)
                    {
                        String path        = token.string;
                        String current_dir = GetDirFromFilePath(file->absolute_file_path);
                        SkipPastToken(&state, token);
                        
                        Memory_Arena scratch = {};
                        
                        File_ID imported_file_id = INVALID_ID;
                        if (EnsureFileIsIncludedInCompilation(module, current_dir, path, &imported_file_id))
                        {
                            bool found = false;
                            for (Bucket_Array_Iterator it = Iterate(&root->imported_files);
                                 it.current;
                                 Advance(&it))
                            {
                                if (imported_file_id == *(File_ID*)it.current)
                                {
                                    found = true;
                                    break;
                                }
                            }
                            
                            if (!found)
                            {
                                *(File_ID*)PushElement(&root->imported_files) = imported_file_id;
                            }
                            
                            if (is_exported)
                            {
                                found = false;
                                for (Bucket_Array_Iterator it = Iterate(&export_info->exported_files);
                                     it.current;
                                     Advance(&it))
                                {
                                    if (imported_file_id == *(File_ID*)it.current)
                                    {
                                        found = true;
                                        break;
                                    }
                                }
                                
                                if (!found)
                                {
                                    *(File_ID*)PushElement(&export_info->exported_files) = imported_file_id;
                                }
                            }
                        }
                        
                        else
                        {
                            //// ERROR
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Missing path to file in import directive
                        encountered_errors = true;
                    }
                }
                
                
                else if (StringCompare(token.string, CONST_STRING("scope_export")) ||
                         StringCompare(token.string, CONST_STRING("scope_local")))
                {
                    should_export = StringCompare(token.string, CONST_STRING("scope_export"));
                    
                    SkipPastToken(&state, token);
                }
                
                else
                {
                    //// ERROR: Unknown compiler directive
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Missing name of compiler directive after pound sign
                encountered_errors = true;
            }
        }
        
        else
        {
            Parser_Context stmnt_context = {};
            stmnt_context.scope = &root->root_scope;
            
            stmnt_context.allow_loose_expressions = false;
            stmnt_context.allow_subscopes         = false;
            stmnt_context.allow_defer             = false;
            stmnt_context.allow_return            = false;
            stmnt_context.allow_loop_jmp          = false;
            stmnt_context.allow_ctrl_structs      = false;
            
            stmnt_context.allow_using        = true;
            stmnt_context.allow_var_decls    = true;
            stmnt_context.allow_proc_decls   = true;
            stmnt_context.allow_struct_decls = true;
            stmnt_context.allow_enum_decls   = true;
            stmnt_context.allow_const_decls  = true;
            
            if (ParseStatement(&state, stmnt_context, current_statement))
            {
                AST_Node* statement = *current_statement;
                
                if (statement != 0)
                {
                    if (should_export && (statement->node_type == ASTNode_VarDecl    ||
                                          statement->node_type == ASTNode_FuncDecl   ||
                                          statement->node_type == ASTNode_ConstDecl  ||
                                          statement->node_type == ASTNode_StructDecl ||
                                          statement->node_type == ASTNode_EnumDecl))
                    {
                        *(Symbol_ID*)PushElement(&export_info->exported_symbols) = statement->symbol;
                    }
                    
                    else if (should_export && statement->node_type == ASTNode_Using)
                    {
                        UMM newest_index = ElementCount(&root->root_scope.using_imports) - 1;
                        Using_Import* newest_entry = (Using_Import*)ElementAt(&root->root_scope.using_imports,
                                                                              newest_index);
                        
                        Using_Import* import = (Using_Import*)PushElement(&export_info->exported_using_imports);
                        import->access_chain           = newest_entry->access_chain;
                        import->table_id               = newest_entry->table_id;
                        import->num_decls_before_valid = newest_entry->num_decls_before_valid;
                    }
                    
                    current_statement = &(*current_statement)->next;
                }
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    return !encountered_errors;
}