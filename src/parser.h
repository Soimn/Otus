typedef struct Scope_Builder
{
    Bucket_Array(AST_Node*) expressions;
    Bucket_Array(AST_Node*) statements;
} Scope_Builder;

typedef struct Parser_State
{
    Compiler_State* compiler_state;
    
    Scope_Builder scope_builder;
    
    Bucket_Array(Token) tokens;
    Bucket_Array_Iterator token_it;
    Bucket_Array_Iterator peek_it;
    Text_Pos text_pos;
    umm peek_index;
} Parser_State;

inline Token
GetToken(Parser_State* state)
{
    Token* token = (Token*)state->token_it.current;
    
    state->text_pos   = token->text.pos;
    state->peek_index = 0;
    
    return *token;
}

inline Token
PeekNextToken(Parser_State* state)
{
    if (state->peek_index == 0) state->peek_it = state->token_it;
    
    BucketArray_AdvanceIterator(&state->tokens, &state->peek_it);
    state->peek_index += 1;
    
    return *(Token*)state->peek_it.current;
}

inline void
SkipToNextToken(Parser_State* state)
{
    BucketArray_AdvanceIterator(&state->tokens, &state->token_it);
    GetToken(state);
}

inline void
SkipPastPeek(Parser_State* state)
{
    state->token_it = state->peek_it;
    GetToken(state);
}

bool
IsAssignment(Enum8(TOKEN_KIND) kind)
{
    return (kind == Token_TildeEqual    || kind == Token_Equal           ||
            kind == Token_PlusEqual     || kind == Token_MinusEqual      ||
            kind == Token_AsteriskEqual || kind == Token_PercentageEqual ||
            kind == Token_PipeEqual     || kind == Token_AmpersandEqual  ||
            kind == Token_TildeEqual);
}

enum COMPILER_DIRECTIVE_KIND
{
    Directive_Invalid = 0,
    
    Directive_Include,
};

typedef struct Compiler_Directive
{
    Enum8(COMPILER_DIRECTIVE_KIND) kind;
    
    union
    {
        struct
        {
            String path;
        } include;
    };
} Compiler_Directive;

bool
ResolvePathString(Parser_State* state, String path_string, String* resolved_path, bool is_file_path)
{
    bool encountered_errors = false;
    
    String prefix_name = {
        .data = path_string.data,
        .size = 0
    };
    
    String extension = {0};
    
    for (umm i = 0; i < path_string.size; ++i)
    {
        if (path_string.data[i] == ':' && prefix_name.size == 0)
        {
            prefix_name.size = i;
        }
        
        else if (path_string.data[i] == '\\' ||
                 path_string.data[i] == ':')
        {
            //// ERROR: Wrong path separator
            encountered_errors = true;
            break;
        }
        
        else if (path_string.data[i] == '.')
        {
            extension = (String){
                .data = path_string.data + (i + 1),
                .size = path_string.size - (i + 1)
            };
            
            if (extension.size == 0)
            {
                //// ERROR: Missing extension after period
                encountered_errors = true;
            }
            
            break;
        }
    }
    
    if (!encountered_errors)
    {
        if (extension.size == 0)
        {
            if (is_file_path)
            {
                //// ERROR: Missing extension
                encountered_errors = true;
            }
        }
        
        else
        {
            if (!is_file_path)
            {
                //// ERROR: Path to directory should not have an extension
                encountered_errors = true;
            }
            
            else if (!StringCompare(extension, CONST_STRING("os")))
            {
                //// ERROR: Wrong extension
                encountered_errors = true;
            }
        }
    }
    
    if (!encountered_errors)
    {
        String prefix_path = {.size = 0};
        String file_path   = path_string;
        
        if (prefix_name.size != 0)
        {
            file_path.data += prefix_path.size + 1;
            file_path.size -= prefix_path.size + 1;
            
            Path_Prefix* found_prefix = 0;
            for (umm i = 0; i < state->compiler_state->workspace->path_prefixes.size; ++i)
            {
                Path_Prefix* prefix = (Path_Prefix*)DynamicArray_ElementAt(&state->compiler_state->workspace->path_prefixes, Path_Prefix, i);
                
                if (StringCompare(prefix_name, prefix->name))
                {
                    found_prefix = prefix;
                    break;
                }
            }
            
            if (found_prefix != 0) prefix_path = found_prefix->path;
            else
            {
                //// ERROR: No path prefix with the name ''
                encountered_errors = true;
            }
        }
        
        if (!encountered_errors)
        {
            resolved_path->data = Arena_Allocate(&state->compiler_state->temp_memory, prefix_path.size + file_path.size, ALIGNOF(u8));
            resolved_path->size = prefix_path.size + file_path.size;
            
            Copy(prefix_path.data, resolved_path->data, prefix_path.size);
            Copy(file_path.data, resolved_path->data + prefix_path.size, file_path.size);
        }
    }
    
    // TODO(soimn): Verify file or directory existence and construct absolute path
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

//////////////////////////////////////////

bool
ParseAttributes(Parser_State* state, Bucket_Array(AST_Node)* attributes)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state);
    ASSERT(token.kind == Token_At);
    SkipToNextToken(state);
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
ParseCompilerDirective(Parser_State* state, Compiler_Directive* directive)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
ParseExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
ParseStatement(Parser_State* state, AST_Node* result)
{
    bool encountered_errors = false;
    
    bool ParseScope(Parser_State* state, AST_Node* scope_node);
    
    Token token     = GetToken(state);
    Token peek      = PeekNextToken(state);
    Token peek_next = PeekNextToken(state);
    
    Bucket_Array attribute_array = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Attribute, 3);
    if (token.kind == Token_At)
    {
        if (!ParseAttributes(state, &attribute_array))
        {
            encountered_errors = true;
        }
    }
    
    Slice attribute_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &attribute_array);
    result->attributes = SLICE_TO_DYNAMIC_ARRAY(attribute_slice);
    
    token     = GetToken(state);
    peek      = PeekNextToken(state);
    peek_next = PeekNextToken(state);
    
    if (token.kind == Token_Semicolon)
    {
        if (result->attributes.size == 0)
        {
            //// ERROR: Stray semicolon
            encountered_errors = true;
        }
        
        else
        {
            result->kind = AST_Empty;
            
            SkipToNextToken(state);
        }
    }
    
    else if (token.kind == Token_OpenBrace)
    {
        result->kind = AST_Scope;
        
        if (!ParseScope(state, result))
        {
            encountered_errors = true;
        }
    }
    
    else if (token.kind == Token_Identifier && token.keyword == Keyword_For ||
             ((token.kind == Token_Identifier || token.kind == Token_Underscore) && peek.kind == Token_Colon &&
              peek_next.kind == Token_Identifier && peek_next.keyword == Keyword_For))
    {
        result->kind = AST_For;
        
        if (peek.kind == Token_Colon)
        {
            if (token.kind == Token_Underscore)
            {
                //// ERROR: Loop labels cannot be blank
                encountered_errors = true;
            }
            
            else
            {
                result->for_statement.label = token.string;
                
                SkipToNextToken(state); // NOTE(soimn): Skip label
                SkipToNextToken(state); // NOTE(soimn): Skip colon
            }
        }
        
        if (!encountered_errors)
        {
            SkipToNextToken(state);
            token = GetToken(state);
            
            NOT_IMPLEMENTED;
        }
    }
    
    else if (token.kind == Token_Identifier && token.keyword == Keyword_While ||
             ((token.kind == Token_Identifier || token.kind == Token_Underscore) && peek.kind == Token_Colon &&
              peek_next.kind == Token_Identifier && peek_next.keyword == Keyword_While))
    {
        result->kind = AST_While;
        
        if (peek.kind == Token_Colon)
        {
            if (token.kind == Token_Underscore)
            {
                //// ERROR: Loop labels cannot be blank
                encountered_errors = true;
            }
            
            else
            {
                result->while_statement.label = token.string;
                
                SkipToNextToken(state); // NOTE(soimn): Skip label
                SkipToNextToken(state); // NOTE(soimn): Skip colon
            }
        }
        
        if (!encountered_errors)
        {
            SkipToNextToken(state);
            token = GetToken(state);
            peek  = PeekNextToken(state);
            
            if ((token.kind == Token_Identifier || token.kind == Token_Underscore) &&
                (peek.kind == Token_Comma || peek.kind == Token_Colon || IsAssignment(peek.kind)))
            {
                result->while_statement = ;
                if (!ParseStatement(state, result->while_statement.init)) encountered_errors = true;
                else
                {
                    if (token.kind == Token_Semicolon) SkipToNextToken(state);
                    else
                    {
                        //// ERROR: Missing while statement condition after init statement
                        encountered_errors = true;
                    }
                }
            }
            
            else if (token.kind == Token_Semicolon)
            {
                // NOTE(soimn): Skip semicolon after empty init statement
                SkipToNextToken(state);
            }
            
            if (!encountered_errors)
            {
                token = GetToken(state);
                
                if (token.kind == Token_OpenBrace ||
                    token.kind == Token_Identifier && token.keyword == Keyword_Do)
                {
                    //// ERROR: Missing while statement condition before body
                    encountered_errors = true;
                }
                
                else
                {
                    if (token.kind != Token_Semicolon)
                    {
                        if (!ParseExpression(state, &result->if_statement.condition))
                        {
                            encountered_errors = true;
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(state);
                        
                        if (token.kind == Token_Semicolon)
                        {
                            SkipToNextToken(state);
                            token = GetToken(state);
                            
                            if (token.kind != Token_OpenBrace && (token.kind != Token_Identifier || token.keyword != Keyword_Do))
                            {
                                result->while_statement.step = ;
                                if (!ParseStatement(state, result->while_statement.step))
                                {
                                    encountered_errors = true;
                                }
                            }
                        }
                        
                        if (!encountered_errors)
                        {
                            if (token.kind != Token_OpenBrace && (token.kind != Token_Identifier || token.keyword != Keyword_Do))
                            {
                                //// ERROR: Missing body of while statement
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                if (token.kind == Token_Identifier) SkipToNextToken(state);
                                
                                result->while_statement.body = ;
                                if (!ParseScope(state, result->while_statement.body))
                                {
                                    encountered_errors = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    else if (token.kind == Token_Identifier && token.keyword != Keyword_Invalid)
    {
        if (token.keyword == Keyword_If)
        {
            result->kind = AST_If;
            
            SkipToNextToken(state);
            token = GetToken(state);
            peek  = PeekNextToken(state);
            
            if ((token.kind == Token_Identifier || token.kind == Token_Underscore) &&
                (peek.kind == Token_Comma || peek.kind == Token_Colon || IsAssignment(peek.kind)))
            {
                result->if_statement.init = ;
                if (!ParseStatement(state, result->if_statement.init)) encountered_errors = true;
                else
                {
                    if (token.kind == Token_Semicolon) SkipToNextToken(state);
                    else
                    {
                        //// ERROR: Missing if statement condition after init statement
                        encountered_errors = true;
                    }
                }
            }
            
            else if (token.kind == Token_Semicolon)
            {
                // NOTE(soimn): Skip semicolon after empty init statement
                SkipToNextToken(state);
            }
            
            if (!encountered_errors)
            {
                token = GetToken(state);
                
                if (token.kind == Token_Semicolon)
                {
                    //// ERROR: Step statements are not allowed in if statements
                    encountered_errors = true;
                }
                
                else if (token.kind == Token_OpenBrace ||
                         token.kind == Token_Identifier && token.keyword == Keyword_Do)
                {
                    //// ERROR: Missing if statement condition before body
                    encountered_errors = true;
                }
                
                else
                {
                    if (!ParseExpression(state, &result->if_statement.condition)) encountered_errors = true;
                    else
                    {
                        token = GetToken(state);
                        
                        if (token.kind == Token_Semicolon)
                        {
                            //// ERROR: Step statements are not allowed in if statements
                            encountered_errors = true;
                        }
                        
                        else if (token.kind != Token_OpenBrace && (token.kind != Token_Identifier || token.keyword != Keyword_Do))
                        {
                            //// ERROR: Missing body of if statement
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            if (token.kind == Token_Identifier) SkipToNextToken(state);
                            
                            result->if_statement.true_body = ;
                            if (!ParseScope(state, result->if_statement.true_body)) encountered_errors = true;
                            else
                            {
                                token = GetToken(state);
                                peek  = PeekNextToken(state);
                                
                                if (token.kind == Token_Identifier && token.keyword == Keyword_Else)
                                {
                                    SkipToNextToken(state);
                                    
                                    if (peek.kind == Token_Identifier && peek.keyword == Keyword_If)
                                    {
                                        result->if_statement.false_body = ;
                                        if (!ParseStatement(state, result->if_statement.false_body))
                                        {
                                            encountered_errors = true;
                                        }
                                    }
                                    
                                    else
                                    {
                                        result->if_statement.false_body = ;
                                        if (!ParseScope(state, result->if_statement.false_body))
                                        {
                                            encountered_errors = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        else if (token.keyword == Keyword_When)
        {
            result->kind = AST_When;
            
            SkipToNextToken(state);
            token = GetToken(state);
            peek  = PeekNextToken(state);
            
            if (token.kind == Token_Semicolon ||
                (token.kind == Token_Identifier || token.kind == Token_Underscore) &&
                (peek.kind == Token_Comma || peek.kind == Token_Colon || IsAssignment(peek.kind)))
            {
                //// ERROR: Init statements are disallowed in when statements
                encountered_errors = true;
            }
            
            else
            {
                if (token.kind == Token_Semicolon)
                {
                    //// ERROR: Step statements are not allowed in when statements
                    encountered_errors = true;
                }
                
                else if (token.kind == Token_OpenBrace ||
                         token.kind == Token_Identifier && token.keyword == Keyword_Do)
                {
                    //// ERROR: Missing when statement condition before body
                    encountered_errors = true;
                }
                
                else
                {
                    if (!ParseExpression(state, &result->when_statement.condition)) encountered_errors = true;
                    else
                    {
                        token = GetToken(state);
                        
                        if (token.kind == Token_Semicolon)
                        {
                            //// ERROR: Step statements are not allowed in when statements
                            encountered_errors = true;
                        }
                        
                        else if (token.kind != Token_OpenBrace && (token.kind != Token_Identifier || token.keyword != Keyword_Do))
                        {
                            //// ERROR: Missing body of when statement
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            if (token.kind == Token_Identifier) SkipToNextToken(state);
                            
                            result->when_statement.true_body = ;
                            if (!ParseScope(state, result->when_statement.true_body)) encountered_errors = true;
                            else
                            {
                                token = GetToken(state);
                                peek  = PeekNextToken(state);
                                
                                if (token.kind == Token_Identifier && token.keyword == Keyword_Else)
                                {
                                    SkipToNextToken(state);
                                    
                                    if (peek.kind == Token_Identifier && peek.keyword == Keyword_When)
                                    {
                                        result->when_statement.false_body = ;
                                        if (!ParseStatement(state, result->when_statement.false_body))
                                        {
                                            encountered_errors = true;
                                        }
                                    }
                                    
                                    else
                                    {
                                        result->when_statement.false_body = ;
                                        if (!ParseScope(state, result->when_statement.false_body))
                                        {
                                            encountered_errors = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        else if (token.keyword == Keyword_Else)
        {
            if (result->attributes.size != 0)
            {
                //// ERROR: Attributes cannot be applied to else statements
                encountered_errors = true;
            }
            
            else
            {
                //// ERROR: Illegal else without macthing if
                encountered_errors = true;
            }
        }
        
        else if (token.keyword == Keyword_Break || token.keyword == Keyword_Continue)
        {
            Enum8(AST_NODE_KIND) kind = (token.keyword == Keyword_Break ? AST_Break : AST_Continue);
            
            result->kind = kind;
            
            SkipToNextToken(state);
            token = GetToken(state);
            
            if (token.kind == Token_Identifier)
            {
                if (token.keyword != Keyword_Invalid)
                {
                    //// ERROR: Invalid use of keyword as label
                    encountered_errors = true;
                }
                
                else
                {
                    if (kind == AST_Break) result->break_statement.label    = token.string;
                    else                   result->continue_statement.label = token.string;
                    
                    SkipToNextToken(state);
                }
            }
            
            if (!encountered_errors)
            {
                token = GetToken(state);
                if (token.kind == Token_Semicolon) SkipToNextToken(state);
                else
                {
                    //// ERROR: Missing semicolon after statement
                    encountered_errors = true;
                }
            }
        }
        
        else if (token.keyword == Keyword_Defer)
        {
            result->kind = AST_Defer;
            
            SkipToNextToken(state);
            token = GetToken(state);
            
            if (token.kind == Token_Identifier && token.keyword == Keyword_Do)
            {
                //// ERROR: Invalid use of so keyword. Defer statements are directly followed by a statement, no do is used.
                encountered_errors = true;
            }
            
            else
            {
                result->defer_statement.scope = ;
                if (!ParseScope(state, result->defer_statement.scope))
                {
                    encountered_errors = true;
                }
            }
        }
        
        else if (token.keyword == Keyword_Using)
        {
            result->kind = AST_Using;
            
            SkipToNextToken(state);
            
            if (!ParseExpression(state, &result->using_statement.symbol_path)) encountered_errors = true;
            else
            {
                token = GetToken(state);
                
                if (token.kind == Token_Identifier)
                {
                    if (token.keyword == Keyword_Import)
                    {
                        //// ERROR: Invalid use of import declaration. Import declarations are only allowed in global scope
                        encountered_errors = true;
                    }
                    
                    else if (token.keyword == Keyword_As)
                    {
                        SkipToNextToken(state);
                        token = GetToken(state);
                        
                        if (token.kind != Token_Identifier)
                        {
                            if (token.kind == Token_Underscore)
                            {
                                //// ERROR: Using declaration alias cannot be blank
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                //// ERROR: Missing alias after as keyword in using declaration
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            result->using_statement.alias = token.string;
                            SkipToNextToken(state);
                        }
                    }
                }
                
                if (!encountered_errors)
                {
                    token = GetToken(state);
                    if (token.kind == Token_Semicolon) SkipToNextToken(state);
                    else
                    {
                        //// ERROR: Missing semicolon after statement
                        encountered_errors = true;
                    }
                }
            }
        }
        
        else if (token.keyword == Keyword_Return)
        {
            result->kind = AST_Return;
            
            Bucket_Array arguments = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Named_Argument, 3);
            
            token = GetToken(state);
            while (token.kind != Token_Semicolon)
            {
                token = GetToken(state);
                peek  = PeekNextToken(state);
                
                Named_Argument* argument = BucketArray_AppendElement(&arguments);
                
                if (peek.kind == Token_Equal)
                {
                    if (token.kind == Token_Identifier)
                    {
                        if (token.keyword != Keyword_Invalid)
                        {
                            //// ERROR: Invalid use of keyword as argument name
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            argument->name = token.string;
                            SkipPastPeek(state);
                        }
                    }
                    
                    else if (token.kind == Token_Underscore)
                    {
                        //// ERROR: Blank identifier cannot be used in as names in named argument lists
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        //// ERROR: Missing semicolon after statement
                        encountered_errors = true;
                    }
                }
                
                if (!encountered_errors)
                {
                    if (!ParseExpression(state, &argument->value)) encountered_errors = true;
                    else
                    {
                        token = GetToken(state);
                        
                        if (token.kind == Token_Comma) SkipToNextToken(state);
                        else break;
                    }
                }
            }
            
            Slice arguments_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &arguments);
            result->return_statement.arguments = SLICE_TO_DYNAMIC_ARRAY(arguments_slice);
            
            token = GetToken(state);
            if (token.kind == Token_Semicolon) SkipToNextToken(state);
            else
            {
                //// ERROR: Missing semicolon after statement
                encountered_errors = true;
            }
        }
        
        else
        {
            if (token.keyword == Keyword_Import)
            {
                //// ERROR: Invalid use of import declaration. Import declarations are only allowed in global scope
                encountered_errors = true;
            }
            
            else
            {
                // IMPORTANT TODO(soimn): This breaks inline and no_inline calls
                //// ERROR: Invalid use of keyword
                encountered_errors = true;
            }
        }
    }
    
    else if ((token.kind == Token_Identifier || token.kind == Token_Underscore) &&
             (peek.kind == Token_Comma || peek.kind == Token_Colon || IsAssignment(peek.kind)))
    {
        NOT_IMPLEMENTED;
    }
    
    else
    {
        result->kind = AST_Expression;
        
        if (!ParseExpression(state, &result)) encountered_errors = true;
        else
        {
            token = GetToken(state);
            if (token.kind == Token_Semicolon) SkipToNextToken(state);
            else
            {
                //// ERROR: Missing semicolon after statement
                encountered_errors = true;
            }
        }
    }
    
    return !encountered_errors;
}

bool
ParseScope(Parser_State* state, AST_Node* scope_node)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state);
    
    bool is_single_line = false;
    if (token.kind != Token_OpenBrace)
    {
        ASSERT(token.kind == Token_Identifier && token.keyword == Keyword_Do);
        is_single_line = true;
    }
    
    SkipToNextToken(state);
    
    Scope_Builder prev_builder = state->scope_builder;
    
    state->scope_builder = (Scope_Builder){
        .expressions = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, AST_Node, 10),
        .statements  = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, AST_Node, 10),
    };
    
    do
    {
        token = GetToken(state);
        if (token.kind == Token_CloseBrace)
        {
            if (is_single_line)
            {
                //// ERROR: Missing statement
                encountered_errors = true;
            }
            
            else
            {
                SkipToNextToken(state);
                break;
            }
        }
        
        else
        {
            AST_Node* statement = ;
            if (!ParseStatement(state, statement))
            {
                encountered_errors = true;
            }
        }
    } while (!encountered_errors && !is_single_line);
    
    state->scope_builder = prev_builder;
    
    return !encountered_errors;
}

// TODO(soimn): Add information about where this file was loaded from and check for existence/absence of package decl
bool
ParseFile(Compiler_State* compiler_state, File_ID file_id)
{
    bool encountered_errors = false;
    
    Parser_State state = {};
    state.compiler_state = compiler_state;
    state.tokens         = BUCKET_ARRAY_INIT(&compiler_state->transient_memory, Token, 256);
    
    // TODO(soimn): current scope
    // TODO(soimn): handle public and private package scope
    
    if (!LexFile(compiler_state, file_id, &state.tokens)) encountered_errors = true;
    else
    {
        state.token_it = BucketArray_CreateIterator(&state.tokens);
        state.peek_it  = BucketArray_CreateIterator(&state.tokens);
        
        Token token = GetToken(&state);
        while (!encountered_errors)
        {
            token      = GetToken(&state);
            Token peek = PeekNextToken(&state);
            
            if (token.kind == Token_EndOfStream) break;
            else
            {
                if (token.kind == Token_Identifier && token.keyword == Keyword_Import ||
                    (token.kind == Token_Identifier && token.keyword == Keyword_Using &&
                     peek.kind  == Token_Identifier && peek.keyword  == Keyword_Import))
                {
                    AST_Node* import_node = ;
                    import_node->kind            = AST_ImportDecl;
                    import_node->import.is_using = (token.keyword == Keyword_Using);
                    
                    Text_Pos import_decl_start = state.text_pos;
                    
                    SkipPastPeek(&state);
                    token = GetToken(&state);
                    
                    if (token.kind != Token_String)
                    {
                        //// ERROR: Missing path
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        String resolved_path;
                        if (!ResolvePathString(&state, token.string, &resolved_path, false)) encountered_errors = true;
                        else
                        {
                            import_node->import.package_id = INVALID_PACKAGE_ID;
                            
                            for (umm i = 0; i < state.compiler_state->workspace->packages.size; ++i)
                            {
                                Package* package = Slice_ElementAt(&state.compiler_state->workspace->packages, Package, i);
                                
                                if (StringCompare(resolved_path, package->path))
                                {
                                    import_node->import.package_id = i;
                                    break;
                                }
                            }
                            
                            if (import_node->import.package_id == INVALID_PACKAGE_ID)
                            {
                                // TODO(soimn): Load package
                                NOT_IMPLEMENTED;
                            }
                            
                            token = GetToken(&state);
                            if (token.kind == Token_Identifier && token.keyword == Keyword_As)
                            {
                                if (!import_node->import.is_using)
                                {
                                    //// ERROR: Illegal use of 'as' keyword outside of using declaration
                                    encountered_errors = true;
                                }
                                
                                else
                                {
                                    SkipToNextToken(&state);
                                    token = GetToken(&state);
                                    
                                    if (token.kind != Token_Identifier)
                                    {
                                        //// ERROR: Missing alias after 'as' keyword in using import declaration
                                        encountered_errors = true;
                                    }
                                    
                                    else
                                    {
                                        import_node->import.alias = token.string;
                                        SkipToNextToken(&state);
                                    }
                                }
                            }
                            
                            token = GetToken(&state);
                            if (token.kind == Token_At)
                            {
                                Bucket_Array attributes = BUCKET_ARRAY_INIT(&state.compiler_state->temp_memory, Attribute, 3);
                                if (!ParseAttributes(&state, &attributes)) encountered_errors = true;
                                else
                                {
                                    // IMPORTANT TODO(soimn): Flattening
                                    Slice attribute_slice = BucketArray_FlattenContent(&state.compiler_state->persistent_memory, &attributes);
                                    
                                    import_node->attributes = SLICE_TO_DYNAMIC_ARRAY(attribute_slice);
                                }
                            }
                            
                            token = GetToken(&state);
                            if (token.kind == Token_Semicolon)
                            {
                                import_node->text = TextInterval_FromEndpoints(import_decl_start, state.text_pos);
                                SkipToNextToken(&state);
                            }
                            else
                            {
                                //// ERROR: Missing terminating semicolon after import statement
                                encountered_errors = true;
                            }
                        }
                    }
                }
                
                else if (token.kind == Token_Hash &&
                         peek.kind == Token_Identifier && StringCompare(peek.string, CONST_STRING("include")))
                {
                    Compiler_Directive directive;
                    if (!ParseCompilerDirective(&state, &directive)) encountered_errors = true;
                    else
                    {
                        String resolved_path;
                        if (!ResolvePathString(&state, directive.include.path, &resolved_path, true)) encountered_errors = true;
                        else
                        {
                            // TODO(soimn): Load file
                            NOT_IMPLEMENTED;
                        }
                    }
                }
                
                else
                {
                    // TODO(soimn): Remember memory managemement, use parser state to store accumulated size, or calculate from cached offset?
                    AST_Node* statement = ;
                    if (!ParseStatement(&state, statement))
                    {
                        encountered_errors = true;
                    }
                }
            }
        }
    }
    
    return !encountered_errors;
}