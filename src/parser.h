typedef struct Parser_State
{
    Compiler_State* compiler_state;
    
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
SkipToPeek(Parser_State* state)
{
    state->token_it = state->peek_it;
    GetToken(state);
}

AST_Node*
AddExpression(Parser_State* state, Enum8(AST_EXPR_KIND) expr_kind)
{
    AST_Node* result = 0;
    
    NOT_IMPLEMENTED;
    
    return result;
}

AST_Node*
AddStatement(Parser_State* state, Enum8(AST_NODE_KIND) kind)
{
    AST_Node* result = 0;
    
    NOT_IMPLEMENTED;
    
    return result;
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
ParseScope(Parser_State* state, AST_Node* scope_node)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
ParseStatement(Parser_State* state)
{
    bool encountered_errors = false;
    
    Token token              = GetToken(state);
    Text_Pos statement_start = state->text_pos;
    
    Bucket_Array(Attribute) attributes = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Attribute, 3);
    
    if (token.kind == Token_At)
    {
        if (!ParseAttributes(state, &attributes))
        {
            encountered_errors = true;
        }
    }
    
    if (!encountered_errors)
    {
        token           = GetToken(state);
        Token peek      = PeekNextToken(state);
        Token peek_next = PeekNextToken(state);
        
        AST_Node* statement             = 0;
        bool do_not_check_for_semicolon = false;
        
        if (token.kind == Token_Semicolon)
        {
            // NOTE(soimn): Semicolon is eaten due to 'do_not_check_for_semicolon' being false
            
            if (BucketArray_ElementCount(&attributes) == 0)
            {
                //// ERROR: Stray semicolon
                encountered_errors = true;
            }
            
            else
            {
                statement = AddStatement(state, AST_Empty);
            }
        }
        
        else if (token.kind == Token_OpenBrace)
        {
            statement = AddStatement(state, AST_Scope);
            do_not_check_for_semicolon = true;
            
            if (!ParseScope(state, statement))
            {
                encountered_errors = true;
            }
        }
        
        else if (token.kind == Token_Identifier)
        {
            if (token.keyword == Keyword_If)
            {
                statement = AddStatement(state, AST_If);
                do_not_check_for_semicolon = true;
                
                SkipToNextToken(state);
                token = GetToken(state);
                peek  = PeekNextToken(state);
                
                // NOTE(soimn): If init statement. Allows only assignment and declaration
                if (peek.kind == Token_Comma || peek.kind == Token_Colon || peek.kind == Token_Equal)
                {
                    NOT_IMPLEMENTED;
                }
                
                if (!encountered_errors)
                {
                    if (!ParseExpression(state, &statement->if_statement.condition)) encountered_errors = true;
                    else
                    {
                        NOT_IMPLEMENTED;
                    }
                }
            }
            
            else if (token.keyword == Keyword_Else)
            {
                //// ERROR: Illegal else without matching if
                encountered_errors = true;
            }
            
            else if (token.keyword == Keyword_When)
            {
                statement = AddStatement(state, AST_When);
                do_not_check_for_semicolon = true;
                
                NOT_IMPLEMENTED;
            }
            
            else if (token.keyword == Keyword_While ||
                     (peek.kind == Token_Colon &&
                      peek_next.kind == Token_Identifier && peek.keyword == Keyword_While))
            {
                statement = AddStatement(state, AST_While);
                do_not_check_for_semicolon = true;
                
                String label = {};
                if (peek.kind == Token_Colon)
                {
                    label = token.string;
                    
                    SkipToNextToken(state); // Skip label name
                    SkipToNextToken(state); // Skip colon
                }
                
                SkipToNextToken(state);
                
                NOT_IMPLEMENTED;
            }
            
            else if (token.keyword == Keyword_For ||
                     (peek.kind == Token_Colon &&
                      peek_next.kind == Token_Identifier && peek.keyword == Keyword_For))
            {
                statement = AddStatement(state, AST_For);
                do_not_check_for_semicolon = true;
                
                String label = {};
                if (peek.kind == Token_Colon)
                {
                    label = token.string;
                    SkipToPeek(state);
                }
                
                else SkipToNextToken(state);
                
                NOT_IMPLEMENTED;
            }
            
            else if (token.keyword == Keyword_Break)
            {
                NOT_IMPLEMENTED;
            }
            
            else if (token.keyword == Keyword_Continue)
            {
                NOT_IMPLEMENTED;
            }
            
            else if (token.keyword == Keyword_Return)
            {
                NOT_IMPLEMENTED;
            }
            
            else if (token.keyword == Keyword_Using)
            {
                NOT_IMPLEMENTED;
            }
            
            else if (token.keyword == Keyword_Defer)
            {
                NOT_IMPLEMENTED;
            }
            
            else if (peek.kind == Token_Comma || peek.kind == Token_Colon)
            {
                do_not_check_for_semicolon = true;
                
                // NOTE(soimn): Assignment, variable declaration or constant declaration
                NOT_IMPLEMENTED;
            }
        }
        
        if (!encountered_errors && statement == 0)
        {
            statement = AddStatement(state, AST_Expression);
            
            if (!ParseExpression(state, &statement->expression))
            {
                encountered_errors = true;
            }
        }
        
        if (!encountered_errors)
        {
            if (BucketArray_ElementCount(&attributes) != 0)
            {
                Slice attribute_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &attributes);
                statement->attributes = SLICE_TO_DYNAMIC_ARRAY(attribute_slice);
            }
            
            statement->text = TextInterval_FromEndpoints(statement_start, state->text_pos);
            
            if (!do_not_check_for_semicolon)
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
                    bool is_using = (token.keyword == Keyword_Using);
                    
                    Text_Pos import_decl_start = state.text_pos;
                    
                    SkipToPeek(&state);
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
                            Package_ID package_id = INVALID_PACKAGE_ID;
                            
                            for (umm i = 0; i < state.compiler_state->workspace->packages.size; ++i)
                            {
                                Package* package = Slice_ElementAt(&state.compiler_state->workspace->packages, Package, i);
                                
                                if (StringCompare(resolved_path, package->path))
                                {
                                    package_id = i;
                                    break;
                                }
                            }
                            
                            if (package_id == INVALID_PACKAGE_ID)
                            {
                                // TODO(soimn): Load package
                                NOT_IMPLEMENTED;
                            }
                            
                            String alias = {};
                            
                            token = GetToken(&state);
                            if (token.kind == Token_Identifier && token.keyword == Keyword_As)
                            {
                                if (!is_using)
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
                                        alias = token.string;
                                        SkipToNextToken(&state);
                                    }
                                }
                            }
                            
                            AST_Node* import_node = AddStatement(&state, AST_ImportDecl);
                            import_node->text = TextInterval_FromEndpoints(import_decl_start, state.text_pos);
                            
                            import_node->import.alias      = alias;
                            import_node->import.package_id = package_id;
                            import_node->import.is_using   = is_using;
                            
                            token = GetToken(&state);
                            if (token.kind == Token_At)
                            {
                                Bucket_Array attributes;
                                if (!ParseAttributes(&state, &attributes))
                                {
                                    encountered_errors = true;
                                }
                                
                                Slice attribute_slice = BucketArray_FlattenContent(&state.compiler_state->persistent_memory, &attributes);
                                
                                import_node->attributes = SLICE_TO_DYNAMIC_ARRAY(attribute_slice);
                            }
                            
                            token = GetToken(&state);
                            if (token.kind == Token_Semicolon) SkipToNextToken(&state);
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
                    if (ParseStatement(&state))
                    {
                        encountered_errors = true;
                    }
                }
            }
        }
    }
    
    return !encountered_errors;
}