inline AST_Node*
PushNode(Parser_State* state, Enum32(AST_NODE_TYPE) node_type, Enum32(AST_EXPR_TYPE) expr_type = ASTExpr_Invalid)
{
    AST_Node* result = (AST_Node*)PushElement(&state->ast->nodes);
    result->expr_type = expr_type;
    result->node_type = node_type;
    
    return result;
}

inline bool
ParseExpression(Parser_State* state, Parser_Context context, AST_Node** result)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

inline bool
ParseStatement(Parser_State* state, Parser_Context context, AST_Node** result)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

inline bool
PushFileForParsing(Module* module, String current_directory, String file_path,
                   Symbol_Table_ID* export_table_id = 0)
{
    bool encountered_errors = false;
    
    LockMutex(&module->file_array_mutex);
    
    Memory_Arena tmp_memory = {};
    String resolved_path    = {};
    File* file              = 0;
    
    if (TryResolvePath(&tmp_memory, current_directory, file_path, &resolved_path))
    {
        for (Bucket_Array_Iterator it = Iterate(&module->files); it.current; Advance(&it))
        {
            if (StringCompare(resolved_path, ((File*)it.current)->file_path))
            {
                file = (File*)it.current;
                break;
            }
        }
        
        if (file == 0)
        {
            file = (File*)PushElement(&module->files);
            
            file->file_path = PushString(&module->file_arena, resolved_path);
            
            if (TryLoadFile(&module->file_arena, file->file_path, &file->file_contents))
            {
                Parser_State* state = &file->parser_state;
                
                state->lexer_state.current    = file->file_contents.data;
                state->lexer_state.line_start = state->lexer_state.current;
                state->lexer_state.column     = 0;
                state->lexer_state.line       = 0;
                
                state->ast    = &file->ast;
                state->file   = file;
                state->module = module;
                
                state->ast->root    = PushNode(state, ASTNode_Root);
                file->file_table_id = PushSymbolTable(module);
                
                Scope_Info* scope = &state->ast->root->scope;
                scope->table     = file->file_table_id;
                scope->num_decls = 0;
                
                file->stage = CompStage_Init;
            }
            
            else
            {
                //// ERROR: Failed to load file
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR: Failed to resolve path
        encountered_errors = true;
    }
    
    FreeArena(&tmp_memory);
    UnlockMutex(&module->file_array_mutex);
    
    if (!encountered_errors)
    {
        if (export_table_id != 0)
        {
            *export_table_id = file->file_table_id;
        }
    }
    
    return !encountered_errors;
}

inline bool
ParseScope(Parser_State* state, Parser_Context context, AST_Node* scope_node)
{
    bool encountered_errors = false;
    
    Symbol_Table* table = (Symbol_Table*)ElementAt(&state->module->symbol_tables, scope_node->scope.table);
    
    AST_Node** current_statement = &scope_node->first_in_scope;
    
    while (!encountered_errors)
    {
        Token token = GetToken(state);
        
        bool was_handled = false;
        
        if (token.type == Token_Hash)
        {
            SkipPastToken(state, token);
            token = GetToken(state);
            
            if (token.type == Token_Identifier)
            {
                if (StringCompare(token.string, CONST_STRING("import")))
                {
                    if (context.allow_scope_directives)
                    {
                        SkipPastToken(state, token);
                        token = GetToken(state);
                        
                        if (token.type == Token_String)
                        {
                            String path = token.string;
                            
                            SkipPastToken(state, token);
                            
                            Symbol_Table_ID imported_table_id = INVALID_ID;
                            if (PushFileForParsing(state->module, state->file->file_dir, path, &imported_table_id))
                            {
                                Imported_Symbol_Table* imported_table = 0;
                                for (Bucket_Array_Iterator it = Iterate(&table->imported_tables);
                                     it.current;
                                     Advance(&it))
                                {
                                    Imported_Symbol_Table* scan = (Imported_Symbol_Table*)it.current;
                                    
                                    if (scan->table_id == imported_table_id)
                                    {
                                        imported_table = scan;
                                    }
                                }
                                
                                if (imported_table == 0)
                                {
                                    imported_table = (Imported_Symbol_Table*)PushElement(&table->imported_tables);
                                    imported_table->restriction_id         = INVALID_ID;
                                    imported_table->table_id               = imported_table_id;
                                    imported_table->num_decls_before_valid = 0;
                                }
                                
                                else
                                {
                                    imported_table->restriction_id = INVALID_ID;
                                }
                                
                                was_handled = true;
                            }
                            
                            else
                            {
                                //// ERROR: Failed to import file
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            //// ERROR: Missing path after import compiler directive
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Scope directives are illegal in the current context
                        encountered_errors = true;
                    }
                }
                
                else if (StringCompare(token.string, CONST_STRING("import_local")))
                {
                    if (context.allow_scope_directives)
                    {
                        SkipPastToken(state, token);
                        token = GetToken(state);
                        
                        if (token.type == Token_String)
                        {
                            String path = token.string;
                            
                            SkipPastToken(state, token);
                            
                            Symbol_Table_ID imported_table_id = INVALID_ID;
                            if (PushFileForParsing(state->module, state->file->file_dir, path, &imported_table_id))
                            {
                                Imported_Symbol_Table* imported_table = 0;
                                for (Bucket_Array_Iterator it = Iterate(&table->imported_tables);
                                     it.current;
                                     Advance(&it))
                                {
                                    Imported_Symbol_Table* scan = (Imported_Symbol_Table*)it.current;
                                    
                                    if (scan->table_id == imported_table_id)
                                    {
                                        imported_table = scan;
                                    }
                                }
                                
                                if (imported_table == 0)
                                {
                                    imported_table = (Imported_Symbol_Table*)PushElement(&table->imported_tables);
                                    imported_table->restriction_id         = state->file->file_table_id;
                                    imported_table->table_id               = imported_table_id;
                                    imported_table->num_decls_before_valid = 0;
                                }
                                
                                was_handled = true;
                            }
                            
                            else
                            {
                                //// ERROR: Failed to import file
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            //// ERROR: Missing path after import_local compiler directive
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Scope directives are illegal in the current context
                        encountered_errors = true;
                    }
                }
                
                else if (StringCompare(token.string, CONST_STRING("scope_global")))
                {
                    if (context.allow_scope_directives)
                    {
                        context.global_by_default = true;
                        SkipPastToken(state, token);
                        
                        was_handled = true;
                    }
                    
                    else
                    {
                        //// ERROR: Scope directives are illegal in the current context
                        encountered_errors = true;
                    }
                }
                
                else if (StringCompare(token.string, CONST_STRING("scope_local")))
                {
                    if (context.allow_scope_directives)
                    {
                        context.global_by_default = false;
                        SkipPastToken(state, token);
                        
                        was_handled = true;
                    }
                    
                    else
                    {
                        //// ERROR: Scope directives are illegal in the current context
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    // NOTE(soimn): Do nothing, let ParseStatement handle it
                }
            }
            
            else
            {
                //// ERROR: Expected identifier after hash in compiler directive
                encountered_errors = true;
            }
        }
        
        if (!was_handled && !encountered_errors)
        {
            Parser_Context new_context = {};
            new_context.scope = &scope_node->scope;
            new_context.global_by_default  = context.global_by_default;
            new_context.allow_subscopes    = false;
            new_context.allow_using        = false;
            new_context.allow_defer        = false;
            new_context.allow_return       = false;
            new_context.allow_loop_jmp     = false;
            new_context.allow_ctrl_structs = false;
            
            if (ParseStatement(state, new_context, current_statement))
            {
                current_statement = &(*current_statement)->next;
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

bool
DoParsingWork(Parser_State* state)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}