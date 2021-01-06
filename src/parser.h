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

bool
IsAssignment(Enum8(TOKEN_KIND) kind)
{
    return (kind == Token_PlusEqual       || kind == Token_MinusEqual ||
            kind == Token_AsteriskEqual   || kind == Token_SlashEqual ||
            kind == Token_PercentageEqual || kind == Token_TildeEqual ||
            kind == Token_AmpersandEqual  || kind == Token_PipeEqual  ||
            kind == Token_TildeEqual      || kind == Token_Equal);
}

AST_Node*
AddNode(Parser_State* state)
{
    AST_Node* result = Arena_Allocate(&state->compiler_state->persistent_memory, sizeof(AST_Node), ALIGNOF(AST_Node));
    Zero(result, sizeof(AST_Node));
    
    return result;
}

AST_Node*
AddExpressionNode(Parser_State* state, Enum8(AST_EXPR_KIND) expr_kind)
{
    AST_Node* result = AddNode(state);
    
    result->kind      = AST_Expression;
    result->expr_kind = expr_kind;
    
    *(AST_Node**)BucketArray_AppendElement(&state->scope_builder.expressions) = result;
    
    return result;
}

bool
ResolvePathString(Parser_State* state, String path_string, String* resolved_path)
{
    bool encountered_errors = false;
    
    String prefix_name = {
        .data = path_string.data,
        .size = 0
    };
    
    for (umm i = 0; !encountered_errors && i < path_string.size; ++i)
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
        }
        
        else if (path_string.data[i] == '.')
        {
            //// ERROR: Path to directory should not have an extension
            encountered_errors = true;
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
    
    // TODO(soimn): Verify directory existence and construct absolute path
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

//////////////////////////////////////////

bool ParseExpression(Parser_State* state, AST_Node** result);
bool ParseScope(Parser_State* state, AST_Node* scope_node);

bool
ParseAttributes(Parser_State* state, Bucket_Array(Attribute)* attributes)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state);
    ASSERT(token.kind == Token_At);
    
    while (!encountered_errors)
    {
        SkipToNextToken(state);
        token = GetToken(state);
        
        Attribute* attribute = BucketArray_AppendElement(attributes);
        
        if (token.kind != Token_Identifier)
        {
            //// ERROR: Missing name of attribute
            encountered_errors = true;
        }
        
        else
        {
            attribute->name = token.string;
            
            bool ended_with_space = token.ends_with_space;
            
            SkipToNextToken(state);
            token = GetToken(state);
            
            if (!ended_with_space && token.kind == Token_OpenParen)
            {
                SkipToNextToken(state);
                token = GetToken(state);
                
                Bucket_Array arguments = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, AST_Node*, 3);
                
                if (token.kind != Token_CloseParen)
                {
                    while (!encountered_errors)
                    {
                        AST_Node** argument = BucketArray_AppendElement(&arguments);
                        
                        if (!ParseExpression(state, argument)) encountered_errors = true;
                        else
                        {
                            token = GetToken(state);
                            
                            if (token.kind == Token_Comma) SkipToNextToken(state);
                            else break;
                        }
                    }
                }
                
                if (!encountered_errors)
                {
                    token = GetToken(state);
                    
                    if (token.kind != Token_CloseParen)
                    {
                        //// ERROR: Missing matching closing parenthesis after attribute arguments
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        Slice argument_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &arguments);
                        attribute->arguments = SLICE_TO_DYNAMIC_ARRAY(argument_slice);
                    }
                }
            }
        }
        
        token = GetToken(state);
        
        if (token.kind == Token_At) continue;
        else break;
    }
    
    return !encountered_errors;
}

bool
ParseParameterList(Parser_State* state, Bucket_Array(Parameter)* parameters)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state);
    ASSERT(token.kind == Token_OpenParen);
    
    SkipToNextToken(state);
    token = GetToken(state);
    
    if (token.kind != Token_CloseParen)
    {
        while (!encountered_errors)
        {
            Parameter* parameter = BucketArray_AppendElement(parameters);
            
            token = GetToken(state);
            if (token.kind == Token_At)
            {
                Bucket_Array attributes = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Attribute, 10);
                
                if (!ParseAttributes(state, &attributes)) encountered_errors = true;
                else
                {
                    Slice attribute_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &attributes);
                    parameter->attributes = SLICE_TO_DYNAMIC_ARRAY(attribute_slice);
                }
            }
            
            if (!encountered_errors)
            {
                token = GetToken(state);
                if (token.kind == Token_Identifier && token.keyword == Keyword_Using)
                {
                    SkipToNextToken(state);
                    parameter->is_using = true;
                }
                
                AST_Node* expression;
                if (!ParseExpression(state, &expression)) encountered_errors = true;
                else
                {
                    token = GetToken(state);
                    
                    if (token.kind == Token_Colon || token.kind == Token_ColonEqual)
                    {
                        parameter->name = expression;
                        
                        if (token.kind == Token_Colon)
                        {
                            SkipToNextToken(state);
                            
                            if (!ParseExpression(state, &parameter->type))
                            {
                                encountered_errors = true;
                            }
                        }
                        
                        token = GetToken(state);
                        if (token.kind == Token_ColonEqual || token.kind == Token_Equal)
                        {
                            if (parameter->type != 0 && token.kind == Token_ColonEqual)
                            {
                                //// ERROR: Invalid use if inferred type declaration operator
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                SkipToNextToken(state);
                                
                                if (!ParseExpression(state, &parameter->value))
                                {
                                    encountered_errors = true;
                                }
                            }
                        }
                    }
                    
                    else
                    {
                        parameter->type = expression;
                        
                        if (token.kind == Token_Equal)
                        {
                            //// ERROR: Cannot assign default value to parameter without name
                            encountered_errors = true;
                        }
                    }
                }
                
                if (!encountered_errors)
                {
                    token = GetToken(state);
                    
                    if (token.kind == Token_Comma) SkipToNextToken(state);
                    else break;
                }
            }
        }
    }
    
    if (!encountered_errors)
    {
        token = GetToken(state);
        
        if (token.kind != Token_CloseParen)
        {
            //// ERROR: Missing matching closing parenthesis
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

bool
ParseNamedArgumentList(Parser_State* state, Bucket_Array(AST_Node)* arguments)
{
    bool encountered_errors = false;
    
    while (!encountered_errors)
    {
        Named_Argument* argument = BucketArray_AppendElement(arguments);
        
        AST_Node* expression;
        if (!ParseExpression(state, &expression)) encountered_errors = true;
        else
        {
            Token token = GetToken(state);
            if (token.kind == Token_Equal)
            {
                if (expression->kind != Expr_Identifier && IsAssignment(token.kind))
                {
                    //// ERROR: Illegal use of assignment operator in expression context
                    encountered_errors = true;
                }
                
                else if (expression->string.size == 0)
                {
                    //// ERROR: Illegal use of blank identifier as argument name
                    encountered_errors = true;
                }
                
                else
                {
                    argument->name = expression;
                    
                    SkipToNextToken(state);
                    
                    if (!ParseExpression(state, &argument->value))
                    {
                        encountered_errors = true;
                    }
                }
            }
            
            else
            {
                argument->value = expression;
            }
            
            token = GetToken(state);
            if (token.kind == Token_Comma) SkipToNextToken(state);
            else break;
        }
    }
    
    return !encountered_errors;
}

bool
ParsePrimaryExpression(Parser_State* state, AST_Node** result, bool in_type_context, AST_Node** parent, bool tolerate_missing_primary)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state);
    
    bool missing_primary = false;
    
    if (token.kind == Token_Number)
    {
        if (in_type_context)
        {
            //// ERROR: Invalid use of numeric literal as type name
            encountered_errors = true;
        }
        
        else
        {
            *result = AddExpressionNode(state, Expr_Number);
            (*result)->number = token.number;
            
            SkipToNextToken(state);
        }
    }
    
    else if (token.kind == Token_String)
    {
        if (in_type_context)
        {
            //// ERROR: Invalid use of string as type name
            encountered_errors = true;
        }
        
        else
        {
            *result = AddExpressionNode(state, Expr_String);
            (*result)->string = token.string;
            
            SkipToNextToken(state);
        }
    }
    
    else if (token.kind == Token_Identifier && token.keyword == Keyword_Invalid ||
             token.kind == Token_Underscore)
    {
        if (in_type_context && token.kind == Token_Underscore)
        {
            //// ERROR: Invalid use of blank identifier as type name
            encountered_errors = true;
        }
        
        else
        {
            *result = AddExpressionNode(state, Expr_Identifier);
            (*result)->string = (token.kind == Token_Identifier ? token.string : (String){0});
            
            SkipToNextToken(state);
        }
    }
    
    else if (token.kind == Token_OpenParen)
    {
        if (in_type_context)
        {
            //// ERROR: Missing type name after type specific prefix operator
            encountered_errors = true;
        }
        
        else
        {
            SkipToNextToken(state);
            
            if (!ParseExpression(state, result)) encountered_errors = true;
            else
            {
                token = GetToken(state);
                
                if (token.kind == Token_CloseParen) SkipToNextToken(state);
                else
                {
                    //// ERROR: Missing matching closing parenthesis
                    encountered_errors = true;
                }
            }
        }
    }
    
    else if (token.kind == Token_Identifier && token.keyword == Keyword_Proc)
    {
        *result = AddExpressionNode(state, Expr_Proc);
        
        SkipToNextToken(state);
        token = GetToken(state);
        
        Bucket_Array parameters = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Parameter, 10);
        
        if (token.kind == Token_OpenParen)
        {
            if (!ParseParameterList(state, &parameters))
            {
                encountered_errors = true;
            }
        }
        
        Slice parameter_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &parameters);
        (*result)->proc.parameters = SLICE_TO_DYNAMIC_ARRAY(parameter_slice);
        
        
        if (!encountered_errors)
        {
            token = GetToken(state);
            
            Bucket_Array return_values = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Return_Value, 10);
            
            if (token.kind == Token_MinusGreater)
            {
                token = GetToken(state);
                
                bool multiple_return_values = false;
                if (token.kind == Token_OpenParen)
                {
                    multiple_return_values = true;
                    
                    SkipToNextToken(state);
                }
                
                while (!encountered_errors)
                {
                    Return_Value* return_value = BucketArray_AppendElement(&return_values);
                    
                    token = GetToken(state);
                    
                    Bucket_Array attributes = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Attribute, 10);
                    
                    if (token.kind == Token_At)
                    {
                        if (!ParseAttributes(state, &attributes))
                        {
                            encountered_errors = true;
                        }
                    }
                    
                    Slice attribute_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &attributes);
                    return_value->attributes = SLICE_TO_DYNAMIC_ARRAY(attribute_slice);
                    
                    AST_Node* expression;
                    if (!ParseExpression(state, &expression)) encountered_errors = true;
                    else
                    {
                        token = GetToken(state);
                        if (expression->kind == Expr_Identifier && token.kind == Token_Colon)
                        {
                            return_value->name = expression;
                            
                            SkipToNextToken(state);
                            
                            if (!ParseExpression(state, &return_value->type))
                            {
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            return_value->type = expression;
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(state);
                        if (token.kind != Token_Comma || !multiple_return_values) break;
                        else
                        {
                            SkipToNextToken(state);
                            continue;
                        }
                    }
                }
                
                if (multiple_return_values)
                {
                    token = GetToken(state);
                    
                    if (token.kind == Token_CloseParen) SkipToNextToken(state);
                    else
                    {
                        //// ERROR: Missing matching closing parenthesis
                        encountered_errors = true;
                    }
                }
            }
            
            Slice return_value_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &return_values);
            (*result)->proc.return_values = SLICE_TO_DYNAMIC_ARRAY(return_value_slice);
            
            if (!encountered_errors)
            {
                token = GetToken(state);
                
                if (token.kind == Token_Identifier && token.keyword == Keyword_Where)
                {
                    SkipToNextToken(state);
                    
                    if (!ParseExpression(state, &(*result)->proc.where_expression))
                    {
                        encountered_errors = true;
                    }
                }
                
                token = GetToken(state);
                
                if (token.kind == Token_Identifier && token.keyword == Keyword_Do)
                {
                    //// ERROR: Invalid use of do keyword after proc type. Only if, while and for statements support do
                    encountered_errors = true;
                }
                
                else
                {
                    token = GetToken(state);
                    
                    if (token.kind == Token_At || token.kind == Token_OpenBrace)
                    {
                        Bucket_Array attributes = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Attribute, 10);
                        
                        if (token.kind == Token_At)
                        {
                            if (!ParseAttributes(state, &attributes))
                            {
                                encountered_errors = true;
                            }
                        }
                        
                        Slice attribute_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &attributes);
                        (*result)->proc.body_attributes = SLICE_TO_DYNAMIC_ARRAY(attribute_slice);
                        
                        if (!encountered_errors)
                        {
                            token = GetToken(state);
                            
                            if (token.kind != Token_OpenBrace)
                            {
                                //// ERROR: Missing procedure body
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                if (!ParseScope(state, (*result)->proc.body))
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
    
    else if (token.kind == Token_Identifier && token.keyword == Keyword_Struct ||
             token.kind == Token_Identifier && token.keyword == Keyword_Union)
    {
        *result = AddExpressionNode(state, Expr_Struct);
        
        (*result)->structure.is_union = (token.keyword == Keyword_Union);
        
        SkipToNextToken(state);
        token = GetToken(state);
        
        Bucket_Array parameters = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Parameter, 10);
        
        if (token.kind == Token_OpenParen)
        {
            if (!ParseParameterList(state, &parameters))
            {
                encountered_errors = true;
            }
        }
        
        Slice parameter_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &parameters);
        (*result)->structure.parameters = SLICE_TO_DYNAMIC_ARRAY(parameter_slice);
        
        if (!encountered_errors)
        {
            token = GetToken(state);
            
            if (token.kind == Token_Identifier && token.keyword == Keyword_Where)
            {
                SkipToNextToken(state);
                
                if (!ParseExpression(state, &(*result)->structure.where_expression))
                {
                    encountered_errors = true;
                }
            }
            
            token = GetToken(state);
            
            if (token.kind == Token_Identifier && token.keyword == Keyword_Do)
            {
                //// ERROR: Invalid use of do keyword after struct/union type header. Only if, while and for statements support do
                encountered_errors = true;
            }
            
            else
            {
                Bucket_Array body_attributes = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Attribute, 10);
                
                if (token.kind == Token_At)
                {
                    if (!ParseAttributes(state, &body_attributes))
                    {
                        encountered_errors = true;
                    }
                }
                
                Slice body_attribute_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &body_attributes);
                (*result)->structure.body_attributes = SLICE_TO_DYNAMIC_ARRAY(body_attribute_slice);
                
                if (!encountered_errors)
                {
                    token = GetToken(state);
                    
                    if (token.kind != Token_OpenBrace)
                    {
                        //// ERROR: Missing struct/union body
                        encountered_errors = true;
                    }
                    
                    else if (token.kind != Token_CloseBrace)
                    {
                        SkipToNextToken(state);
                        token = GetToken(state);
                        
                        Bucket_Array members = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Struct_Member, 10);
                        
                        while (!encountered_errors)
                        {
                            Struct_Member* member = BucketArray_AppendElement(&members);
                            
                            token = GetToken(state);
                            
                            if (token.kind == Token_Identifier && token.keyword == Keyword_Using)
                            {
                                member->is_using = true;
                                
                                SkipToNextToken(state);
                            }
                            
                            
                            token = GetToken(state);
                            if (token.kind == Token_At)
                            {
                                //// ERROR: Invalid placement of attribute. Attributes must come after the type field for a struct member
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                if (!ParseExpression(state, &member->name))
                                {
                                    encountered_errors = true;
                                }
                            }
                            
                            if (!encountered_errors)
                            {
                                token = GetToken(state);
                                
                                if (token.kind != Token_Colon)
                                {
                                    //// ERROR: Missing type of struct member
                                    encountered_errors = true;
                                }
                                
                                else
                                {
                                    SkipToNextToken(state);
                                    
                                    if (!ParseExpression(state, &member->type)) encountered_errors = true;
                                    else
                                    {
                                        token = GetToken(state);
                                        
                                        Bucket_Array attributes = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Attribute, 10);
                                        
                                        if (token.kind == Token_At)
                                        {
                                            if (!ParseAttributes(state, &attributes))
                                            {
                                                encountered_errors = true;
                                            }
                                        }
                                        
                                        Slice attribute_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory,
                                                                                           &attributes);
                                        member->attributes = SLICE_TO_DYNAMIC_ARRAY(attribute_slice);
                                        
                                        if (!encountered_errors)
                                        {
                                            token = GetToken(state);
                                            
                                            if (token.kind == Token_Comma) SkipToNextToken(state);
                                            else break;
                                        }
                                    }
                                }
                            }
                        }
                        
                        Slice member_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &members);
                        (*result)->structure.members = SLICE_TO_DYNAMIC_ARRAY(member_slice);
                    }
                }
                
                if (!encountered_errors)
                {
                    token = GetToken(state);
                    
                    if (token.kind == Token_CloseBrace) SkipToNextToken(state);
                    else
                    {
                        //// ERROR: Missing matching closing brace
                        encountered_errors = true;
                    }
                }
            }
        }
    }
    
    else if (token.kind == Token_Identifier && token.keyword == Keyword_Enum)
    {
        *result = AddExpressionNode(state, Expr_Enum);
        
        SkipToNextToken(state);
        token = GetToken(state);
        
        if (token.kind != Token_OpenBrace)
        {
            if (!ParseExpression(state, &(*result)->enumeration.type))
            {
                encountered_errors = true;
            }
        }
        
        if (!encountered_errors)
        {
            token = GetToken(state);
            
            if (token.kind != Token_OpenBrace)
            {
                //// ERROR: Missing body of enum
                encountered_errors = true;
            }
            
            else
            {
                SkipToNextToken(state);
                
                Bucket_Array members = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Enum_Member, 10);
                
                token = GetToken(state);
                
                if (token.kind != Token_CloseBrace)
                {
                    while (!encountered_errors)
                    {
                        Enum_Member* member = BucketArray_AppendElement(&members);
                        
                        token = GetToken(state);
                        
                        if (token.kind == Token_At)
                        {
                            //// ERROR: Invalid placement of attribute. Attributes must come after the value field
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            if (!ParseExpression(state, &member->name))
                            {
                                encountered_errors = true;
                            }
                        }
                        
                        if (!encountered_errors)
                        {
                            token = GetToken(state);
                            
                            if (token.kind == Token_Equal)
                            {
                                SkipToNextToken(state);
                                
                                if (!ParseExpression(state, &member->value))
                                {
                                    encountered_errors = true;
                                }
                            }
                            
                            if (!encountered_errors)
                            {
                                token = GetToken(state);
                                
                                Bucket_Array attributes = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Attribute, 10);
                                
                                if (token.kind == Token_At)
                                {
                                    if (!ParseAttributes(state, &attributes))
                                    {
                                        encountered_errors = true;
                                    }
                                }
                                
                                Slice attribute_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &attributes);
                                member->attributes = SLICE_TO_DYNAMIC_ARRAY(attribute_slice);
                                
                                if (!encountered_errors)
                                {
                                    token = GetToken(state);
                                    
                                    if (token.kind == Token_Comma) SkipToNextToken(state);
                                    else break;
                                }
                            }
                        }
                    }
                }
                
                if (!encountered_errors)
                {
                    token = GetToken(state);
                    
                    if (token.kind != Token_CloseBrace)
                    {
                        //// ERROR: Missing matching closing brace
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        SkipToNextToken(state);
                        
                        Slice member_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &members);
                        (*result)->enumeration.members = SLICE_TO_DYNAMIC_ARRAY(member_slice);
                    }
                }
            }
        }
    }
    
    else
    {
        if (in_type_context)
        {
            //// ERROR: Missing type name
            encountered_errors = true;
        }
        
        else if (token.kind == Token_Identifier)
        {
            //// ERROR: Invalid use of keyword in expression
            encountered_errors = true;
        }
        
        else missing_primary = true;
    }
    
    // NOTE(soimn): Handles type specific primary and postfix
    if (!encountered_errors)
    {
        // NOTE(soimn): This handles the Struct(i, j, k){a, b, c} and Struct(i, j, k)[a, b, c] cases
        token = GetToken(state);
        if (!missing_primary && token.kind == Token_OpenParen)
        {
            SkipToNextToken(state);
            
            AST_Node* pointer = *parent;
            
            *parent = AddExpressionNode(state, Expr_CallButMaybeCastOrSpecialization);
            (*parent)->call_expression.pointer = pointer;
            
            Bucket_Array arguments = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Named_Argument, 10);
            
            token = GetToken(state);
            if (token.kind != Token_CloseParen)
            {
                if (!ParseNamedArgumentList(state, &arguments))
                {
                    encountered_errors = true;
                }
            }
            
            token = GetToken(state);
            if (token.kind != Token_CloseParen)
            {
                //// ERROR: Missing matching closing parenthesis
                encountered_errors = true;
            }
            
            else
            {
                SkipToNextToken(state);
                
                Slice argument_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &arguments);
                (*parent)->call_expression.arguments = SLICE_TO_DYNAMIC_ARRAY(argument_slice);
            }
        }
        
        if (!encountered_errors)
        {
            token = GetToken(state);
            if (token.kind == Token_OpenBracket)
            {
                SkipToNextToken(state);
                token = GetToken(state);
                
                AST_Node* index         = 0;
                AST_Node* length        = 0;
                AST_Node* step          = 0;
                Slice element_slice     = {0};
                bool is_slice_subscript = false;
                
                token = GetToken(state);
                if (token.kind != Token_Colon)
                {
                    if (!ParseExpression(state, &index))
                    {
                        encountered_errors = true;
                    }
                }
                
                if (!encountered_errors)
                {
                    token = GetToken(state);
                    
                    if (token.kind == Token_Comma || token.kind == Token_CloseBracket)
                    {
                        is_slice_subscript = false;
                        
                        Bucket_Array elements = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, AST_Node*, 10);
                        *(AST_Node**)BucketArray_AppendElement(&elements) = index;
                        
                        if (token.kind == Token_Comma)
                        {
                            SkipToNextToken(state);
                            
                            while (!encountered_errors)
                            {
                                if (!ParseExpression(state, BucketArray_AppendElement(&elements))) encountered_errors = true;
                                else
                                {
                                    token = GetToken(state);
                                    
                                    if (token.kind == Token_Comma) SkipToNextToken(state);
                                    else break;
                                }
                            }
                        }
                        
                        element_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &elements);
                    }
                    
                    else if (token.kind == Token_Colon)
                    {
                        is_slice_subscript = true;
                        
                        SkipToNextToken(state);
                        token = GetToken(state);
                        
                        if (token.kind != Token_CloseBracket)
                        {
                            if (token.kind == Token_Colon) SkipToNextToken(state);
                            else
                            {
                                if (!ParseExpression(state, &length))
                                {
                                    encountered_errors = true;
                                }
                            }
                            
                            token = GetToken(state);
                            if (token.kind != Token_CloseBracket)
                            {
                                if (!ParseExpression(state, &step))
                                {
                                    encountered_errors = true;
                                }
                            }
                        }
                    }
                }
                
                if (!encountered_errors)
                {
                    token = GetToken(state);
                    
                    if (token.kind != Token_CloseBracket)
                    {
                        //// ERROR: Missing matching closing bracket
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        SkipToNextToken(state);
                        
                        AST_Node* primary_expression = *parent;
                        
                        if (is_slice_subscript)
                        {
                            if (in_type_context)
                            {
                                //// ERROR: Illegal use of subscript operator on type
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                *parent = AddExpressionNode(state, Expr_Subscript);
                                (*parent)->subscript_expression.operand = primary_expression;
                                (*parent)->subscript_expression.index   = index;
                                (*parent)->subscript_expression.length  = length;
                                (*parent)->subscript_expression.step    = step;
                            }
                        }
                        
                        else
                        {
                            if (in_type_context || missing_primary || element_slice.size != 1)
                            {
                                *parent = AddExpressionNode(state, Expr_ArrayLiteral);
                                (*parent)->array_literal.type     = primary_expression;
                                (*parent)->array_literal.elements = SLICE_TO_DYNAMIC_ARRAY(element_slice);
                            }
                            
                            else
                            {
                                *parent = AddExpressionNode(state, Expr_SubscriptButMaybeLiteral);
                                (*parent)->subscript_expression.operand = primary_expression;
                                (*parent)->subscript_expression.index   = index;
                                (*parent)->subscript_expression.length  = 0;
                                (*parent)->subscript_expression.step    = 0;
                            }
                        }
                    }
                }
            }
            
            else if (in_type_context && token.kind == Token_OpenBrace)
            {
                SkipToNextToken(state);
                
                Bucket_Array arguments = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Named_Argument, 10);
                
                token = GetToken(state);
                if (token.kind != Token_CloseBrace)
                {
                    if (!ParseNamedArgumentList(state, &arguments))
                    {
                        encountered_errors = true;
                    }
                }
                
                Slice argument_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &arguments);
                
                if (!encountered_errors)
                {
                    token = GetToken(state);
                    if (token.kind != Token_CloseBrace)
                    {
                        //// ERROR: Missing matching closing brace
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        SkipToNextToken(state);
                        
                        AST_Node* type = *parent;
                        
                        *parent = AddExpressionNode(state, Expr_StructLiteral);
                        (*parent)->struct_literal.type      = type;
                        (*parent)->struct_literal.arguments = SLICE_TO_DYNAMIC_ARRAY(argument_slice);
                        
                        missing_primary = false;
                    }
                }
            }
            
            if (missing_primary)
            {
                if (tolerate_missing_primary)
                {
                    *result = AddExpressionNode(state, Expr_Empty);
                }
                
                else
                {
                    //// ERROR: Missing primary expression
                    encountered_errors = true;
                }
            }
        }
    }
    
    // NOTE(soimn): Postfix
    while (!missing_primary && !encountered_errors)
    {
        token = GetToken(state);
        
        if (token.kind == Token_Period)
        {
            SkipToNextToken(state);
            token = GetToken(state);
            
            if (token.kind != Token_Identifier || token.keyword != Keyword_Invalid)
            {
                if (token.kind == Token_Identifier)
                {
                    //// ERROR: Invalid use of keyword in expression
                    encountered_errors = true;
                }
                
                else if (token.kind == Token_Underscore)
                {
                    //// ERROR: Illegal use of blank identifier on right hand side of member operator.
                    ////        Unnamed members cannot be accessed with the member operator.
                    encountered_errors = true;
                }
                
                else
                {
                    //// ERROR: Missing member name on right hand side of member operator
                    encountered_errors = true;
                }
            }
            
            else
            {
                AST_Node* left = *parent;
                
                *parent = AddExpressionNode(state, Expr_Member);
                (*parent)->binary_expression.left = left;
                (*parent)->binary_expression.right         = AddExpressionNode(state, Expr_Identifier);
                (*parent)->binary_expression.right->string = token.string;
                
                SkipToNextToken(state);
            }
        }
        
        else if (token.kind == Token_OpenParen)
        {
            Bucket_Array arguments = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, Named_Argument, 10);
            
            SkipToNextToken(state);
            token = GetToken(state);
            
            if (token.kind != Token_CloseParen)
            {
                if (!ParseNamedArgumentList(state, &arguments))
                {
                    encountered_errors = true;
                }
            }
            
            if (!encountered_errors)
            {
                token = GetToken(state);
                
                if (token.kind != Token_CloseParen)
                {
                    //// ERROR: Missing matching closing parenthesis
                    encountered_errors = true;
                }
                
                else
                {
                    SkipToNextToken(state);
                    
                    Slice argument_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &arguments);
                    
                    AST_Node* pointer = *parent;
                    
                    *parent = AddExpressionNode(state, Expr_Call);
                    (*parent)->call_expression.pointer   = pointer;
                    (*parent)->call_expression.arguments = SLICE_TO_DYNAMIC_ARRAY(argument_slice);
                }
            }
        }
        
        else if (token.kind == Token_OpenBracket)
        {
            SkipToNextToken(state);
            token = GetToken(state);
            
            AST_Node* index  = 0;
            AST_Node* length = 0;
            AST_Node* step   = 0;
            
            if (token.kind == Token_CloseBracket)
            {
                //// ERROR: Invalid use of slice type prefix operator as postfix operator
                encountered_errors = true;
            }
            
            else
            {
                if (token.kind == Token_Colon) SkipToNextToken(state);
                else
                {
                    if (!ParseExpression(state, &index))
                    {
                        encountered_errors = true;
                    }
                }
                
                if (!encountered_errors)
                {
                    if (token.kind == Token_Colon) SkipToNextToken(state);
                    else if (token.kind != Token_CloseBracket)
                    {
                        if (!ParseExpression(state, &length))
                        {
                            encountered_errors = true;
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(state);
                        if (token.kind != Token_CloseBracket)
                        {
                            if (!ParseExpression(state, &step))
                            {
                                encountered_errors = true;
                            }
                        }
                    }
                }
            }
            
            if (!encountered_errors)
            {
                token = GetToken(state);
                
                if (token.kind != Token_CloseBracket)
                {
                    //// ERROR: Missing matching closing bracket
                    encountered_errors = true;
                }
                
                else
                {
                    SkipToNextToken(state);
                    
                    AST_Node* operand = *parent;
                    
                    *parent = AddExpressionNode(state, Expr_Subscript);
                    (*parent)->subscript_expression.operand = operand;
                    (*parent)->subscript_expression.index   = index;
                    (*parent)->subscript_expression.length  = length;
                    (*parent)->subscript_expression.step    = step;
                }
            }
        }
        
        else break;
    }
    
    return !encountered_errors;
}

bool
ParseType(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    AST_Node** parent = result;
    while (!encountered_errors)
    {
        Token token = GetToken(state);
        
        if (token.kind == Token_Cash)
        {
            SkipToNextToken(state);
            
            *result = AddExpressionNode(state, Expr_Polymorphic);
            result  = &(*result)->unary_expression.operand;
        }
        
        else if (token.kind == Token_Hat)
        {
            SkipToNextToken(state);
            
            *result = AddExpressionNode(state, Expr_Pointer);
            result = &(*result)->unary_expression.operand;
        }
        
        else if (token.kind == Token_OpenBracket)
        {
            SkipToNextToken(state);
            token = GetToken(state);
            
            Token peek = PeekNextToken(state);
            
            if (token.kind == Token_CloseBracket)
            {
                SkipToNextToken(state);
                
                *result = AddExpressionNode(state, Expr_Slice);
                result = &(*result)->unary_expression.operand;
            }
            
            else if (token.kind == Token_PeriodPeriod && peek.kind == Token_CloseBracket)
            {
                SkipToNextToken(state);
                SkipToNextToken(state);
                
                *result = AddExpressionNode(state, Expr_DynamicArray);
                result = &(*result)->unary_expression.operand;
            }
            
            else
            {
                *result = AddExpressionNode(state, Expr_Array);
                
                if (!ParseExpression(state, &(*result)->array_type.size)) encountered_errors = true;
                else
                {
                    token = GetToken(state);
                    
                    if (token.kind == Token_CloseBracket)
                    {
                        SkipToNextToken(state);
                        
                        result = &(*result)->array_type.type;
                    }
                    
                    else
                    {
                        if (token.kind == Token_Comma)
                        {
                            //// ERROR: Missing type name before array literal
                            encountered_errors = true;
                        }
                        
                        else if (token.kind == Token_Colon)
                        {
                            //// ERROR: Missing expression to slice before slice expression
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            //// ERROR: Missing matching closing bracket
                            encountered_errors = true;
                        }
                    }
                }
            }
        }
        
        else
        {
            if (token.kind == Token_Plus        ||
                token.kind == Token_Minus       ||
                token.kind == Token_Tilde       ||
                token.kind == Token_Exclamation ||
                token.kind == Token_Ampersand   ||
                token.kind == Token_Asterisk    ||
                token.kind == Token_PeriodPeriod)
            {
                //// ERROR: Illegal use of non-type prefix operator on type name
                encountered_errors = true;
            }
            
            else
            {
                if (!ParsePrimaryExpression(state, result, true, parent, false)) encountered_errors = true;
                else
                {
                    token = GetToken(state);
                    
                    if (token.kind != Token_Slash) break;
                    else
                    {
                        SkipToNextToken(state);
                        
                        AST_Node* left = *parent;
                        
                        *parent = AddExpressionNode(state, Expr_PolymorphicAlias);
                        (*parent)->binary_expression.left = left;
                        
                        if (!ParseType(state, &(*parent)->binary_expression.right)) encountered_errors = true;
                        else break;
                    }
                }
            }
        }
    }
    
    return !encountered_errors;
}

bool
ParsePrefixExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    Bucket_Array directives = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, String, 10);
    
    while (!encountered_errors)
    {
        Slice directive_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &directives);
        BucketArray_Clear(&directives);
        
        Token token = GetToken(state);
        
        Enum8(AST_EXPR_KIND) kind = Expr_Invalid;
        
        switch (token.kind)
        {
            case Token_Plus:         kind = Expr_Add;         break;
            case Token_Minus:        kind = Expr_Neg;         break;
            case Token_Tilde:        kind = Expr_BitNot;      break;
            case Token_Exclamation:  kind = Expr_Not;         break;
            case Token_Ampersand:    kind = Expr_Reference;   break;
            case Token_Asterisk:     kind = Expr_Dereference; break;
            case Token_PeriodPeriod: kind = Expr_Spread;      break;
        }
        
        if (kind != Expr_Invalid)
        {
            SkipToNextToken(state);
            
            if (kind != Expr_Add) // NOTE(soimn): Eat prefix + but dont emit an AST node
            {
                *result = AddExpressionNode(state, kind);
                (*result)->directives = SLICE_TO_DYNAMIC_ARRAY(directive_slice);
                
                result = &(*result)->unary_expression.operand;
            }
        }
        
        else
        {
            Token peek      = PeekNextToken(state);
            Token peek_next = PeekNextToken(state);
            
            if (token.kind == Token_Cash || token.kind == Token_Hat ||
                token.kind == Token_OpenBracket && peek.kind == Token_CloseBracket ||
                token.kind == Token_OpenBracket && peek.kind == Token_PeriodPeriod && peek_next.kind == Token_CloseBracket)
            {
                if (!ParseType(state, result)) encountered_errors = true;
                else
                {
                    (*result)->directives = SLICE_TO_DYNAMIC_ARRAY(directive_slice);
                }
            }
            
            else if (token.kind == Token_Hash)
            {
                while (!encountered_errors)
                {
                    SkipToNextToken(state);
                    token = GetToken(state);
                    
                    if (token.kind != Token_Identifier)
                    {
                        //// ERROR: Missing name of directive
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        *(String*)BucketArray_AppendElement(&directives) = token.string;
                        
                        SkipToNextToken(state);
                        token = GetToken(state);
                        
                        if (token.kind == Token_Hash) continue;
                        else break;
                    }
                }
            }
            
            else
            {
                if (!ParsePrimaryExpression(state, result, false, result, (directive_slice.size == 1))) encountered_errors = true;
                else
                {
                    (*result)->directives = SLICE_TO_DYNAMIC_ARRAY(directive_slice);
                    break;
                }
            }
        }
    }
    
    return !encountered_errors;
}

bool
ParseMulLevelExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (!ParsePrefixExpression(state, result)) encountered_errors = true;
    else
    {
        while (!encountered_errors)
        {
            Token token = GetToken(state);
            
            Enum8(AST_EXPR_KIND) kind = Expr_Invalid;
            
            switch (token.kind)
            {
                case Token_Asterisk:   kind = Expr_Mul;    break;
                case Token_Slash:      kind = Expr_Div;    break;
                case Token_Percentage: kind = Expr_Mod;    break;
                case Token_Ampersand:  kind = Expr_BitAnd; break;
            }
            
            if (kind != Expr_Invalid)
            {
                SkipToNextToken(state);
                
                AST_Node* left = *result;
                
                *result = AddExpressionNode(state, kind);
                (*result)->binary_expression.left = left;
                
                if (!ParsePrefixExpression(state, &(*result)->binary_expression.right))
                {
                    encountered_errors = true;
                }
            }
            
            else break;
        }
    }
    
    return !encountered_errors;
}

bool
ParseAddLevelExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (!ParseMulLevelExpression(state, result)) encountered_errors = true;
    else
    {
        while (!encountered_errors)
        {
            Token token = GetToken(state);
            
            Enum8(AST_EXPR_KIND) kind = Expr_Invalid;
            
            switch (token.kind)
            {
                case Token_Plus:  kind = Expr_Add;   break;
                case Token_Minus: kind = Expr_Sub;   break;
                case Token_Pipe:  kind = Expr_BitOr; break;
            }
            
            if (kind != Expr_Invalid)
            {
                SkipToNextToken(state);
                
                AST_Node* left = *result;
                
                *result = AddExpressionNode(state, kind);
                (*result)->binary_expression.left = left;
                
                if (!ParseMulLevelExpression(state, &(*result)->binary_expression.right))
                {
                    encountered_errors = true;
                }
            }
            
            else break;
        }
    }
    
    return !encountered_errors;
}

bool
ParseComparisonExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (!ParseAddLevelExpression(state, result)) encountered_errors = true;
    else
    {
        while (!encountered_errors)
        {
            Token token = GetToken(state);
            
            Enum8(AST_EXPR_KIND) kind = Expr_Invalid;
            
            switch (token.kind)
            {
                case Token_EqualEqual:       kind = Expr_IsEqual;          break;
                case Token_ExclamationEqual: kind = Expr_IsNotEqual;       break;
                case Token_Less:             kind = Expr_IsLess;           break;
                case Token_Greater:          kind = Expr_IsGreater;        break;
                case Token_LessEqual:        kind = Expr_IsLessOrEqual;    break;
                case Token_GreaterEqual:     kind = Expr_IsGreaterOrEqual; break;
            }
            
            if (kind != Expr_Invalid)
            {
                SkipToNextToken(state);
                
                AST_Node* left = *result;
                
                *result = AddExpressionNode(state, kind);
                (*result)->binary_expression.left = left;
                
                if (!ParseAddLevelExpression(state, &(*result)->binary_expression.right))
                {
                    encountered_errors = true;
                }
            }
            
            else break;
        }
    }
    
    return !encountered_errors;
}

bool
ParseLogicalAndExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (!ParseComparisonExpression(state, result)) encountered_errors = true;
    else
    {
        while (!encountered_errors)
        {
            Token token = GetToken(state);
            
            if (token.kind == Token_AmpersandAmpersand)
            {
                SkipToNextToken(state);
                
                AST_Node* left = *result;
                
                *result = AddExpressionNode(state, Expr_Or);
                (*result)->binary_expression.left = left;
                
                if (!ParseComparisonExpression(state, &(*result)->binary_expression.right))
                {
                    encountered_errors = true;
                }
            }
            
            else break;
        }
    }
    
    return !encountered_errors;
}

bool
ParseLogicalOrExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (!ParseLogicalAndExpression(state, result)) encountered_errors = true;
    else
    {
        while (!encountered_errors)
        {
            Token token = GetToken(state);
            
            if (token.kind == Token_PipePipe)
            {
                SkipToNextToken(state);
                
                AST_Node* left = *result;
                
                *result = AddExpressionNode(state, Expr_Or);
                (*result)->binary_expression.left = left;
                
                if (!ParseLogicalAndExpression(state, &(*result)->binary_expression.right))
                {
                    encountered_errors = true;
                }
            }
            
            else break;
        }
    }
    
    return !encountered_errors;
}

bool
ParseTernaryExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (!ParseLogicalOrExpression(state, result)) encountered_errors = true;
    else
    {
        Token token = GetToken(state);
        
        if (token.kind == Token_QuestionMark)
        {
            AST_Node* condition = *result;
            
            *result = AddExpressionNode(state, Expr_Ternary);
            (*result)->ternary_expression.condition = condition;
            
            SkipToNextToken(state);
            
            if (!ParseLogicalOrExpression(state, &(*result)->ternary_expression.true_expr)) encountered_errors = true;
            else
            {
                token = GetToken(state);
                
                if (token.kind != Token_Colon)
                {
                    //// ERROR: Missing else expression in ternary expression
                    encountered_errors = true;
                }
                
                else
                {
                    SkipToNextToken(state);
                    
                    if (!ParseLogicalOrExpression(state, &(*result)->ternary_expression.false_expr))
                    {
                        encountered_errors = true;
                    }
                }
            }
        }
    }
    
    return !encountered_errors;
}

bool
ParseExpression(Parser_State* state, AST_Node** result)
{
    bool encountered_errors = false;
    
    if (!ParseTernaryExpression(state, result)) encountered_errors = true;
    else
    {
        Token token = GetToken(state);
        
        if (IsAssignment(token.kind))
        {
            //// ERROR: Assignment is statement level, and is not allowed in expressions
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

bool
ParseStatement(Parser_State* state, AST_Node* result)
{
    bool encountered_errors = false;
    
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
    
    else if (token.kind == Token_Identifier && token.keyword == Keyword_Import  ||
             token.kind == Token_Identifier && token.keyword == Keyword_Foreign ||
             (token.kind == Token_Identifier && token.keyword == Keyword_Using &&
              peek.kind == Token_Identifier && (peek.keyword == Keyword_Import || token.keyword == Keyword_Foreign)))
    {
        result->kind = AST_ImportDecl;
        
        bool is_using   = false;
        bool is_foreign = false;
        
        if (token.keyword == Keyword_Using)
        {
            is_using = true;
            SkipToNextToken(state);
        }
        
        token = GetToken(state);
        if (token.keyword == Keyword_Foreign)
        {
            is_foreign = true;
            
            SkipToNextToken(state);
            token = GetToken(state);
            
            if (token.kind != Token_Identifier || token.keyword != Keyword_Import)
            {
                //// ERROR: Invalid use of foreign keyword outside of import declaration
                encountered_errors = true;
            }
        }
        
        if (!encountered_errors)
        {
            token = GetToken(state);
            ASSERT(token.kind == Token_Identifier && token.keyword == Keyword_Import);
            
            SkipToNextToken(state);
            token = GetToken(state);
            
            if (token.kind != Token_String)
            {
                //// ERROR: Missing import path in import declaration
                encountered_errors = true;
            }
            
            else
            {
                String resolved_path;
                if (!ResolvePathString(state, token.string, &resolved_path)) encountered_errors = true;
                else
                {
                    if (is_foreign)
                    {
                        NOT_IMPLEMENTED;
                    }
                    
                    else
                    {
                        result->import_declaration.package_id = INVALID_PACKAGE_ID;
                        
                        for (umm i = 0; i < state->compiler_state->workspace->packages.size; ++i)
                        {
                            Package* package = DynamicArray_ElementAt(&state->compiler_state->workspace->packages, Package, i);
                            
                            if (StringCompare(resolved_path, package->path))
                            {
                                result->import_declaration.package_id = i;
                            }
                        }
                        
                        if (result->import_declaration.package_id == INVALID_PACKAGE_ID)
                        {
                            NOT_IMPLEMENTED;
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(state);
                        if (token.kind == Token_Identifier && token.keyword == Keyword_As)
                        {
                            if (!is_using)
                            {
                                //// ERROR: Illegal use of as keyword outside of using statement
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                SkipToNextToken(state);
                                token = GetToken(state);
                                
                                if (token.kind == Token_Underscore)
                                {
                                    //// ERROR: Import alias cannot be blank
                                    encountered_errors = true;
                                }
                                
                                else if (token.kind != Token_Identifier)
                                {
                                    //// ERROR: Missing import alias after as keyword
                                    encountered_errors = true;
                                }
                                
                                else if (token.keyword != Keyword_Invalid)
                                {
                                    //// ERROR: Illegal use of keyword as import alias
                                    encountered_errors = true;
                                }
                                
                                else
                                {
                                    result->import_declaration.alias = token.string;
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
            }
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
                result->while_statement.init = AddNode(state);
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
                                result->while_statement.step = AddNode(state);
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
                                
                                result->while_statement.body = AddNode(state);
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
    
    else if (token.kind == Token_Identifier && token.keyword != Keyword_Invalid &&
             token.keyword != Keyword_True  && token.keyword != Keyword_False)
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
                result->if_statement.init = AddNode(state);
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
                            
                            result->if_statement.true_body = AddNode(state);
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
                                        result->if_statement.false_body = AddNode(state);
                                        if (!ParseStatement(state, result->if_statement.false_body))
                                        {
                                            encountered_errors = true;
                                        }
                                    }
                                    
                                    else
                                    {
                                        result->if_statement.false_body = AddNode(state);
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
                            
                            result->when_statement.true_body = AddNode(state);
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
                                        result->when_statement.false_body = AddNode(state);
                                        if (!ParseStatement(state, result->when_statement.false_body))
                                        {
                                            encountered_errors = true;
                                        }
                                    }
                                    
                                    else
                                    {
                                        result->when_statement.false_body = AddNode(state);
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
                //// ERROR: Invalid use of do keyword. Defer statements are directly followed by a statement, no do is used.
                encountered_errors = true;
            }
            
            else
            {
                result->defer_statement.scope = AddNode(state);
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
                
                if (token.kind == Token_Identifier && token.keyword == Keyword_As)
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
            if (token.kind != Token_Semicolon)
            {
                if (!ParseNamedArgumentList(state, &arguments))
                {
                    encountered_errors = true;
                }
            }
            
            if (!encountered_errors)
            {
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
        }
        
        else
        {
            //// ERROR: Invalid use of keyword
            encountered_errors = true;
        }
    }
    
    else if ((token.kind == Token_Identifier || token.kind == Token_Underscore) &&
             (peek.kind == Token_Comma      || peek.kind == Token_Colon      ||
              peek.kind == Token_ColonEqual || peek.kind == Token_ColonColon ||
              IsAssignment(peek.kind)))
    {
        Bucket_Array lhs = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, AST_Node*, 3);
        
        for (;;)
        {
            if (!ParseExpression(state, BucketArray_AppendElement(&lhs))) encountered_errors = true;
            else
            {
                token = GetToken(state);
                
                if (token.kind == Token_Comma) SkipToNextToken(state);
                else break;
            }
        }
        
        if (!encountered_errors)
        {
            token = GetToken(state);
            
            Bucket_Array types  = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, AST_Node*, 3);
            Bucket_Array values = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, AST_Node*, 3);
            bool is_uninitialized = false;
            bool is_constant      = false;
            
            if (token.kind == Token_Colon || token.kind == Token_ColonEqual)
            {
                if (token.kind == Token_Colon)
                {
                    SkipToNextToken(state);
                    
                    token = GetToken(state);
                    
                    if (token.kind == Token_Equal)
                    {
                        //// ERROR: Missing type list in varliable declaration. Note: ':=' and ': =' are not the same
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        while (!encountered_errors)
                        {
                            if (!ParseExpression(state, BucketArray_AppendElement(&types))) encountered_errors = true;
                            else
                            {
                                token = GetToken(state);
                                
                                if (token.kind == Token_Comma) SkipToNextToken(state);
                                else break;
                            }
                        }
                    }
                }
                
                token = GetToken(state);
                if (!encountered_errors &&
                    (token.kind == Token_Equal      || token.kind == Token_ColonEqual ||
                     token.kind == Token_ColonColon || token.kind == Token_Colon))
                {
                    is_constant = (token.kind == Token_Colon || token.kind == Token_ColonColon);
                    
                    SkipToNextToken(state);
                    token = GetToken(state);
                    
                    if (token.kind == Token_MinusMinusMinus)
                    {
                        if (!is_constant) is_uninitialized = true;
                        else
                        {
                            //// ERROR: Constants must be assigned a value and cannot be uninitialized
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        while (!encountered_errors)
                        {
                            token = GetToken(state);
                            
                            if (token.kind == Token_MinusMinusMinus)
                            {
                                //// ERROR: '---' is not an expression, and cannot be used in an comma separated expression list
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                if (!ParseExpression(state, BucketArray_AppendElement(&values))) encountered_errors = true;
                                else
                                {
                                    token = GetToken(state);
                                    
                                    if (token.kind == Token_Comma) SkipToNextToken(state);
                                    else break;
                                }
                            }
                        }
                    }
                }
                
                token = GetToken(state);
                
                if (!encountered_errors)
                {
                    if (token.kind == Token_Semicolon) SkipToNextToken(state);
                    else
                    {
                        bool does_not_need_semicolon = false;
                        if (is_constant && BucketArray_ElementCount(&values) == 1)
                        {
                            AST_Node* rhs = BucketArray_ElementAt(&values, 0);
                            
                            ASSERT(rhs->kind == AST_Expression);
                            if (rhs->expr_kind == Expr_Proc  || rhs->expr_kind == Expr_Struct ||
                                rhs->expr_kind == Expr_Union || rhs->expr_kind == Expr_Enum)
                            {
                                does_not_need_semicolon = true;
                            }
                        }
                        
                        if (!does_not_need_semicolon)
                        {
                            //// ERROR: Missing semicolon after statement
                            encountered_errors = true;
                        }
                    }
                }
                
                if (!encountered_errors)
                {
                    Slice name_slice;
                    Slice type_slice  = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &types);
                    Slice value_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &values);
                    
                    name_slice.size = BucketArray_ElementCount(&lhs);
                    name_slice.data = Arena_Allocate(&state->compiler_state->persistent_memory,
                                                     sizeof(String) * name_slice.size, ALIGNOF(String));
                    
                    // NOTE(soimn): This could copy whole buckets instead
                    for (Bucket_Array_Iterator it = BucketArray_CreateIterator(&lhs);
                         it.current != 0;
                         BucketArray_AdvanceIterator(&lhs, &it))
                    {
                        Copy(it.current, (String*)name_slice.data + it.index, sizeof(String));
                    }
                    
                    if (is_constant)
                    {
                        result->kind = AST_ConstantDecl;
                        
                        result->constant_declaration.names  = SLICE_TO_DYNAMIC_ARRAY(name_slice);
                        result->constant_declaration.types  = SLICE_TO_DYNAMIC_ARRAY(type_slice);
                        result->constant_declaration.values = SLICE_TO_DYNAMIC_ARRAY(value_slice);
                    }
                    
                    else
                    {
                        result->kind = AST_VariableDecl;
                        
                        result->variable_declaration.names  = SLICE_TO_DYNAMIC_ARRAY(name_slice);
                        result->variable_declaration.types  = SLICE_TO_DYNAMIC_ARRAY(type_slice);
                        result->variable_declaration.values = SLICE_TO_DYNAMIC_ARRAY(value_slice);
                        result->variable_declaration.is_uninitialized = is_uninitialized;
                    }
                }
            }
            
            else
            {
                if (!IsAssignment(token.kind))
                {
                    //// ERROR: Comma separated expressions are only allowed in multiple variable declaration and assignment
                    encountered_errors = true;
                }
                
                else
                {
                    result->kind = AST_Assignment;
                    
                    Bucket_Array rhs = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, AST_Node*, 3);
                    
                    switch (token.kind)
                    {
                        case Token_Equal:           result->assignment_statement.op = Expr_Invalid; break;
                        case Token_PlusEqual:       result->assignment_statement.op = Expr_Add;     break;
                        case Token_MinusEqual:      result->assignment_statement.op = Expr_Sub;     break;
                        case Token_AsteriskEqual:   result->assignment_statement.op = Expr_Mul;     break;
                        case Token_SlashEqual:      result->assignment_statement.op = Expr_Div;     break;
                        case Token_PercentageEqual: result->assignment_statement.op = Expr_Mod;     break;
                        case Token_TildeEqual:      result->assignment_statement.op = Expr_BitNot;  break;
                        case Token_AmpersandEqual:  result->assignment_statement.op = Expr_BitAnd;  break;
                        case Token_PipeEqual:       result->assignment_statement.op = Expr_BitOr;   break;
                        INVALID_DEFAULT_CASE;
                    }
                    
                    SkipToNextToken(state);
                    token = GetToken(state);
                    
                    if (token.kind == Token_Semicolon)
                    {
                        //// ERROR: Missing right hand side of assignment
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        while (!encountered_errors)
                        {
                            if (!ParseExpression(state, BucketArray_AppendElement(&rhs))) encountered_errors = true;
                            else
                            {
                                token = GetToken(state);
                                
                                if (token.kind == Token_Comma) SkipToNextToken(state);
                                else break;
                            }
                        }
                        
                        if (!encountered_errors)
                        {
                            token = GetToken(state);
                            
                            if (token.kind != Token_Semicolon)
                            {
                                //// ERROR: Missing semicolon after statement
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                Slice lhs_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &lhs);
                                Slice rhs_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &rhs);
                                
                                result->assignment_statement.left_side  = SLICE_TO_DYNAMIC_ARRAY(lhs_slice);
                                result->assignment_statement.right_side = SLICE_TO_DYNAMIC_ARRAY(rhs_slice);
                            }
                        }
                    }
                }
            }
        }
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
        .expressions = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, AST_Node*, 10),
        .statements  = BUCKET_ARRAY_INIT(&state->compiler_state->temp_memory, AST_Node*, 10),
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
            AST_Node* statement = AddNode(state);
            if (!ParseStatement(state, statement)) encountered_errors = true;
            else
            {
                *(AST_Node**)BucketArray_AppendElement(&state->scope_builder.statements) = statement;
            }
        }
    } while (!encountered_errors && !is_single_line);
    
    Slice expression_slice = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &state->scope_builder.expressions);
    Slice statement_slice  = BucketArray_FlattenContent(&state->compiler_state->persistent_memory, &state->scope_builder.statements);
    
    scope_node->scope.expressions = SLICE_TO_DYNAMIC_ARRAY(expression_slice);
    scope_node->scope.statements  = SLICE_TO_DYNAMIC_ARRAY(statement_slice);
    
    state->scope_builder = prev_builder;
    
    return !encountered_errors;
}

// TODO(soimn): Add information about where this file was loaded from and check for existence/absence of package decl
bool
ParseFile(Compiler_State* compiler_state, File_ID file_id)
{
    bool encountered_errors = false;
    
    Arena_ClearAll(&compiler_state->transient_memory);
    Arena_ClearAll(&compiler_state->temp_memory);
    
    Parser_State state = {};
    state.compiler_state = compiler_state;
    state.tokens         = BUCKET_ARRAY_INIT(&compiler_state->transient_memory, Token, 256);
    
    if (!LexFile(compiler_state, file_id, &state.tokens)) encountered_errors = true;
    else
    {
        state.token_it = BucketArray_CreateIterator(&state.tokens);
        state.peek_it  = BucketArray_CreateIterator(&state.tokens);
        
        Bucket_Array statements = BUCKET_ARRAY_INIT(&state.compiler_state->transient_memory, AST_Node*, 100);
        
        while (!encountered_errors)
        {
            Arena_ClearAll(&compiler_state->temp_memory);
            
            Token token = GetToken(&state);
            
            if (token.kind == Token_EndOfStream) break;
            else
            {
                if (!ParseStatement(&state, BucketArray_AppendElement(&statements)))
                {
                    encountered_errors = true;
                }
            }
        }
        
        if (!encountered_errors)
        {
            Slice statement_slice = BucketArray_FlattenContent(&state.compiler_state->persistent_memory, &statements);
            
            (void)statement_slice;
            NOT_IMPLEMENTED;
        }
    }
    
    return !encountered_errors;
}