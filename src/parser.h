typedef struct Scope_Builder
{
    Bucket_Array(Expression*) expressions;
    Bucket_Array(Statement*) statements;
} Scope_Builder;

typedef struct Parser_State
{
    Compiler_State* compiler_state;
    
    Scope_Builder scope_builder;
    Scope_Builder top_scope_builder;
    
    Memory_Arena* temp_memory;
    Memory_Arena* decl_memory;
    
    Bucket_Array(Token) tokens;
    Bucket_Array_Iterator token_it;
    Bucket_Array_Iterator peek_it;
    Text_Pos text_pos;
    umm peek_index;
} Parser_State;

Token
GetToken(Parser_State* state)
{
    Token* token = (Token*)state->token_it.current;
    
    state->text_pos   = token->text.pos;
    state->peek_index = 0;
    
    return *token;
}

Token
PeekNextToken(Parser_State* state)
{
    if (state->peek_index == 0) state->peek_it = state->token_it;
    
    BucketArray_AdvanceIterator(&state->tokens, &state->peek_it);
    state->peek_index += 1;
    
    return *(Token*)state->peek_it.current;
}

void
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

Statement*
AddStatement(Parser_State* state)
{
    Statement* result = Arena_Allocate(state->decl_memory, sizeof(Statement), ALIGNOF(Statement));
    ZeroStruct(result);
    
    return result;
}

Expression*
AddExpression(Parser_State* state, Enum8(EXPR_KIND) kind)
{
    Expression* result = Arena_Allocate(state->decl_memory, sizeof(Expression), ALIGNOF(Expression));
    
    ZeroStruct(result);
    result->kind = kind;
    
    *(Expression**)BucketArray_AppendElement(&state->scope_builder.expressions)     = result;
    
    return result;
}

Scope_Builder
PushNewScopeBuilder(Parser_State* state)
{
    Scope_Builder prev = state->scope_builder;
    
    state->scope_builder = (Scope_Builder){
        .expressions = BUCKET_ARRAY_INIT(state->temp_memory, Expression*, 10),
        .statements  = BUCKET_ARRAY_INIT(state->temp_memory, Statement*, 10),
    };
    
    return prev;
}

Statement*
AddStatementToCurrentScope(Parser_State* state, Enum8(STATEMENT_KIND) kind)
{
    Statement* statement = AddStatement(state);
    *(Statement**)BucketArray_AppendElement(&state->scope_builder.statements) = statement;
    
    statement->kind = kind;
    
    return statement;
}

//////////////////////////////////////////

bool ParsePrimaryExpression(Parser_State* state, Expression** result, bool tolerate_missing_primary);
bool ParseExpression(Parser_State* state, Expression** result);
bool ParseStatement(Parser_State* state, Statement* result, bool check_for_semicolon);
bool ParseScope(Parser_State* state, Statement** block);

bool
ParseAttributes(Parser_State* state, Slice(Attribute)* attribute_slice)
{
    bool encountered_errors = false;
    
    Bucket_Array attributes = BUCKET_ARRAY_INIT(state->temp_memory, Attribute, 10);
    
    Token token = GetToken(state);
    
    if (token.kind == Token_At)
    {
        while (!encountered_errors)
        {
            SkipToNextToken(state);
            token = GetToken(state);
            
            Attribute* attribute = BucketArray_AppendElement(&attributes);
            
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
                    
                    Bucket_Array arguments = BUCKET_ARRAY_INIT(state->temp_memory, Expression*, 10);
                    
                    if (token.kind != Token_CloseParen)
                    {
                        while (!encountered_errors)
                        {
                            Expression** argument = BucketArray_AppendElement(&arguments);
                            
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
                            attribute->arguments = BucketArray_FlattenContent(state->decl_memory, &arguments);
                        }
                    }
                }
            }
            
            token = GetToken(state);
            
            if (token.kind == Token_At) continue;
            else break;
        }
    }
    
    *attribute_slice = BucketArray_FlattenContent(state->decl_memory, &attributes);
    
    return !encountered_errors;
}

bool
ParseParameterList(Parser_State* state, Statement** parameters)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state);
    
    if (token.kind == Token_OpenParen)
    {
        SkipToNextToken(state);
        token = GetToken(state);
        
        if (token.kind != Token_CloseParen)
        {
            *parameters = AddStatement(state);
            (*parameters)->kind = Statement_Block;
            
            Scope_Builder prev_builder = PushNewScopeBuilder(state);
            
            while (!encountered_errors)
            {
                Statement* parameter = AddStatementToCurrentScope(state, Statement_Parameter);
                
                token = GetToken(state);
                
                if (!ParseAttributes(state, &parameter->attributes)) encountered_errors = true;
                else
                {
                    token = GetToken(state);
                    if (token.kind == Token_Identifier && token.keyword == Keyword_Using)
                    {
                        parameter->parameter.is_using = true;
                    }
                    
                    if (!encountered_errors)
                    {
                        Expression* expression;
                        if (!ParseExpression(state, &expression)) encountered_errors = true;
                        else
                        {
                            token = GetToken(state);
                            
                            if (token.kind == Token_Colon || token.kind == Token_ColonEqual)
                            {
                                parameter->parameter.name = expression;
                                
                                bool is_decl_assign = (token.kind == Token_ColonEqual);
                                
                                if (token.kind == Token_Colon)
                                {
                                    SkipToNextToken(state);
                                    if (!ParseExpression(state, &parameter->parameter.type))
                                    {
                                        encountered_errors = true;
                                    }
                                }
                                
                                if (!encountered_errors)
                                {
                                    token = GetToken(state);
                                    if (is_decl_assign || token.kind == Token_Equal)
                                    {
                                        SkipToNextToken(state);
                                        if (!ParseExpression(state, &parameter->parameter.default_value))
                                        {
                                            encountered_errors = true;
                                        }
                                    }
                                }
                            }
                            
                            else
                            {
                                parameter->parameter.type = expression;
                                
                                token = GetToken(state);
                                if (token.kind == Token_Equal)
                                {
                                    //// ERROR: Illegal use of default value on parameter without name
                                    encountered_errors = true;
                                }
                                
                                else if (token.kind == Token_ColonColon)
                                {
                                    //// ERROR: Illegal parameter format. Parameters that only accept constants are defined by marking
                                    ////        their name with a prefixing $
                                    encountered_errors = true;
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
            }
            
            state->scope_builder = prev_builder;
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
    }
    
    return !encountered_errors;
}

bool
ParseNamedArgumentList(Parser_State* state, Slice(Expression*)* argument_slice)
{
    bool encountered_errors = false;
    
    Bucket_Array arguments = BUCKET_ARRAY_INIT(state->temp_memory, Expression*, 10);
    
    while (!encountered_errors)
    {
        Expression** argument = BucketArray_AppendElement(&arguments);
        *argument = AddExpression(state, Expr_NamedArgument);
        
        Expression* expression;
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
                    (*argument)->named_argument.name = expression;
                    
                    SkipToNextToken(state);
                    
                    if (!ParseExpression(state, &(*argument)->named_argument.value))
                    {
                        encountered_errors = true;
                    }
                }
            }
            
            else
            {
                (*argument)->named_argument.value= expression;
            }
            
            token = GetToken(state);
            if (token.kind == Token_Comma) SkipToNextToken(state);
            else break;
        }
    }
    
    *argument_slice = BucketArray_FlattenContent(state->decl_memory, &arguments);
    
    return !encountered_errors;
}

bool
ParsePostfixExpression(Parser_State* state, Expression** result)
{
    bool encountered_errors = false;
    
    Expression** member_expression_parent = result;
    
    while (!encountered_errors)
    {
        Token token = GetToken(state);
        
        if (token.kind == Token_Period)
        {
            SkipToNextToken(state);
            
            Expression* left = *result;
            
            *result = AddExpression(state, Expr_Member);
            (*result)->binary_expression.left = left;
            
            if (!ParsePrimaryExpression(state, &(*result)->binary_expression.right, false))
            {
                encountered_errors = true;
            }
            
            else
            {
                result = &(*result)->binary_expression.right;
            }
        }
        
        else
        {
            result = member_expression_parent;
            // NOTE(soimn): defer member_expression_parent = result;
            
            if (token.kind == Token_OpenParen)
            {
                Expression* pointer = *result;
                *result = AddExpression(state, Expr_Call);
                (*result)->call_expression.pointer = pointer;
                
                SkipToNextToken(state);
                token = GetToken(state);
                
                if (token.kind != Token_CloseParen)
                {
                    if (!ParseNamedArgumentList(state, &(*result)->call_expression.arguments))
                    {
                        encountered_errors = true;
                    }
                }
                
                if (!encountered_errors)
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
            
            else if (token.kind == Token_OpenBrace)
            {
                Expression* type = *result;
                
                *result = AddExpression(state, Expr_StructLiteral);
                (*result)->struct_literal.type = type;
                
                SkipToNextToken(state);
                
                if (!ParseNamedArgumentList(state, &(*result)->struct_literal.arguments)) encountered_errors = true;
                else
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
            
            if (token.kind == Token_OpenBracket)
            {
                SkipToNextToken(state);
                token = GetToken(state);
                
                Expression* index         = 0;
                Expression* length        = 0;
                Expression* step          = 0;
                Slice(Expression*) element_slice = {0};
                bool is_array_literal            = false;
                
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
                    
                    if (token.kind == Token_Comma || index == 0 && token.kind == Token_CloseBracket)
                    {
                        Bucket_Array elements = BUCKET_ARRAY_INIT(state->temp_memory, Expression*, 10);
                        *(Expression**)BucketArray_AppendElement(&elements) = index;
                        
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
                        
                        element_slice = BucketArray_FlattenContent(state->decl_memory, &elements);
                    }
                    
                    else if (token.kind == Token_Colon)
                    {
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
                        
                        if (is_array_literal)
                        {
                            Expression* type = *result;
                            
                            *result = AddExpression(state, Expr_ArrayLiteral);
                            (*result)->array_literal.type     = type;
                            (*result)->array_literal.elements = element_slice;
                        }
                        
                        else
                        {
                            Expression* pointer = *result;
                            
                            *result = AddExpression(state,  Expr_Subscript);
                            (*result)->subscript_expression.pointer = pointer;
                            (*result)->subscript_expression.index   = index;
                            (*result)->subscript_expression.length  = length;
                            (*result)->subscript_expression.step    = step;
                        }
                    }
                }
            }
            
            else if (token.kind == Token_OpenBracket)
            {
                SkipToNextToken(state);
                token = GetToken(state);
                
                Expression* index  = 0;
                Expression* length = 0;
                Expression* step   = 0;
                
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
                        
                        Expression* pointer = *result;
                        
                        *result = AddExpression(state, Expr_Subscript);
                        (*result)->subscript_expression.pointer = pointer;
                        (*result)->subscript_expression.index   = index;
                        (*result)->subscript_expression.length  = length;
                        (*result)->subscript_expression.step    = step;
                    }
                }
                
                member_expression_parent = result;
            }
            
            else break;
        }
    }
    
    return !encountered_errors;
}

bool
ParsePrimaryExpression(Parser_State* state, Expression** result, bool tolerate_missing_primary)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state);
    
    bool missing_primary = false;
    
    if (token.kind == Token_Number)
    {
        *result = AddExpression(state, Expr_Number);
        (*result)->number = token.number;
        
        SkipToNextToken(state);
    }
    
    else if (token.kind == Token_String)
    {
        *result = AddExpression(state, Expr_String);
        (*result)->string = token.string;
        
        SkipToNextToken(state);
    }
    
    else if (token.kind == Token_Identifier && token.keyword == Keyword_Invalid ||
             token.kind == Token_Underscore)
    {
        *result = AddExpression(state, Expr_Identifier);
        (*result)->string = (token.kind == Token_Identifier ? token.string : (String){0});
        
        SkipToNextToken(state);
    }
    
    else if (token.kind == Token_OpenParen)
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
    
    else if (token.kind == Token_Identifier && token.keyword == Keyword_Proc)
    {
        *result = AddExpression(state, Expr_Proc);
        
        SkipToNextToken(state);
        token = GetToken(state);
        
        if (token.kind == Token_OpenParen)
        {
            if (!ParseParameterList(state, &(*result)->proc.parameters))
            {
                encountered_errors = true;
            }
        }
        
        if (!encountered_errors)
        {
            token = GetToken(state);
            
            if (token.kind == Token_MinusGreater)
            {
                Scope_Builder prev_builder = PushNewScopeBuilder(state);
                
                bool is_single = true;
                
                token = GetToken(state);
                if (token.kind == Token_OpenParen)
                {
                    SkipToNextToken(state);
                    is_single = false;
                }
                
                token = GetToken(state);
                if (token.kind == Token_CloseParen)
                {
                    //// ERROR: Missing return values
                    encountered_errors = true;
                }
                
                else
                {
                    while (!encountered_errors)
                    {
                        Statement* return_value = AddStatementToCurrentScope(state, Statement_ReturnValue);
                        
                        if (!ParseAttributes(state, &return_value->attributes)) encountered_errors = true;
                        else
                        {
                            Expression* expression;
                            if (!ParseExpression(state, &expression)) encountered_errors = true;
                            else
                            {
                                token = GetToken(state);
                                if (token.kind == Token_Colon)
                                {
                                    return_value->return_value.name = expression;
                                    
                                    SkipToNextToken(state);
                                    
                                    if (!ParseExpression(state, &return_value->return_value.type))
                                    {
                                        encountered_errors = true;
                                    }
                                }
                                
                                else
                                {
                                    return_value->return_value.type = expression;
                                }
                            }
                        }
                        
                        if (is_single) break;
                        else
                            if (!encountered_errors)
                        {
                            token = GetToken(state);
                            
                            if (token.kind == Token_Comma) SkipToNextToken(state);
                            else break;
                        }
                    }
                }
                
                if (!encountered_errors && !is_single)
                {
                    token = GetToken(state);
                    if (token.kind == Token_CloseParen) SkipToNextToken(state);
                    else
                    {
                        //// ERROR: Missing matching closing parenthesis
                        encountered_errors = true;
                    }
                }
                
                Slice expression_slice = BucketArray_FlattenContent(state->decl_memory,
                                                                    &state->scope_builder.expressions);
                Slice statement_slice  = BucketArray_FlattenContent(state->decl_memory,
                                                                    &state->scope_builder.statements);
                
                (*result)->proc.return_values = AddStatement(state);
                (*result)->proc.return_values->kind              = Statement_Block;
                (*result)->proc.return_values->block.expressions = expression_slice;
                (*result)->proc.return_values->block.statements  = statement_slice;
                
                state->scope_builder = prev_builder;
            }
            
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
                
                if (!encountered_errors)
                {
                    token = GetToken(state);
                    
                    if (token.kind != Token_OpenBrace && token.kind != Token_At &&
                        (token.kind != Token_Identifier || token.keyword != Keyword_Do))
                    {
                        //// ERROR: Missing body of procedure
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        if (!ParseScope(state, &(*result)->proc.body))
                        {
                            encountered_errors = true;
                        }
                    }
                }
            }
        }
    }
    
    else if (token.kind == Token_Identifier && token.keyword == Keyword_Struct ||
             token.kind == Token_Identifier && token.keyword == Keyword_Union)
    {
        *result = AddExpression(state, Expr_Struct);
        
        (*result)->structure.is_union = (token.keyword == Keyword_Union);
        
        SkipToNextToken(state);
        token = GetToken(state);
        
        if (!ParseParameterList(state, &(*result)->structure.parameters)) encountered_errors = true;
        else
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
            
            if (!encountered_errors)
            {
                
                Slice attribute_slice;
                if (!ParseAttributes(state, &attribute_slice)) encountered_errors = true;
                else
                {
                    token = GetToken(state);
                    
                    if (token.kind == Token_Identifier && token.keyword == Keyword_Do)
                    {
                        //// ERROR: Invalid use of do keyword after struct/union type header
                        encountered_errors = true;
                    }
                    
                    else if (token.kind != Token_OpenBrace)
                    {
                        //// ERROR: Missing struct/union body
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        (*result)->structure.members = AddStatement(state);
                        
                        if (!ParseScope(state, &(*result)->structure.members)) encountered_errors = true;
                        else
                        {
                            (*result)->structure.members->attributes = attribute_slice;
                        }
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
        *result = AddExpression(state, Expr_Enum);
        
        SkipToNextToken(state);
        token = GetToken(state);
        
        if (token.kind != Token_OpenBrace && token.kind != Token_At)
        {
            if (!ParseExpression(state, &(*result)->enumeration.type))
            {
                encountered_errors = true;
            }
        }
        
        if (!encountered_errors)
        {
            token = GetToken(state);
            
            if (token.kind != Token_OpenBrace && token.kind != Token_At)
            {
                //// ERROR: Missing body of enum
                encountered_errors = true;
            }
            
            else
            {
                Slice attribute_slice;
                if (!ParseAttributes(state, &attribute_slice)) encountered_errors = true;
                else
                {
                    SkipToNextToken(state);
                    token = GetToken(state);
                    
                    if (token.kind == Token_Identifier && token.keyword == Keyword_Do)
                    {
                        //// ERROR: Invalid use of do keyword after enum type header
                        encountered_errors = true;
                    }
                    
                    else if (token.kind != Token_OpenBrace)
                    {
                        //// ERROR: Missing enum body
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        (*result)->enumeration.members = AddStatement(state);
                        
                        if (!ParseScope(state, &(*result)->enumeration.members)) encountered_errors = true;
                        else
                        {
                            (*result)->enumeration.members->attributes = attribute_slice;
                        }
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
    
    else
    {
        if (token.kind == Token_Identifier)
        {
            //// ERROR: Invalid use of keyword in expression
            encountered_errors = true;
        }
        
        else missing_primary = true;
    }
    
    return !encountered_errors;
}

bool
ParsePrefixExpression(Parser_State* state, Expression** result)
{
    bool encountered_errors = false;
    
    Bucket_Array directives = BUCKET_ARRAY_INIT(state->temp_memory, Directive, 10);
    
    Expression** parent = 0;
    
    while (!encountered_errors)
    {
        Slice directive_slice = BucketArray_FlattenContent(state->decl_memory, &directives);
        BucketArray_Clear(&directives);
        
        Token token = GetToken(state);
        
        Enum8(EXPR_KIND) kind = Expr_Invalid;
        
        switch (token.kind)
        {
            case Token_Plus:         kind = Expr_Add;         break;
            case Token_Minus:        kind = Expr_Neg;         break;
            case Token_Tilde:        kind = Expr_BitNot;      break;
            case Token_Exclamation:  kind = Expr_Not;         break;
            case Token_Ampersand:    kind = Expr_Reference;   break;
            case Token_Asterisk:     kind = Expr_Dereference; break;
        }
        
        if (kind != Expr_Invalid)
        {
            SkipToNextToken(state);
            
            if (kind != Expr_Add) // NOTE(soimn): Eat prefix + but dont emit an AST node
            {
                *result = AddExpression(state, kind);
                (*result)->directives = directive_slice;
                
                result = &(*result)->unary_expression.operand;
            }
        }
        
        else
        {
            if (token.kind == Token_Cash)
            {
                SkipToNextToken(state);
                
                *result = AddExpression(state, Expr_Polymorphic);
                (*result)->directives = directive_slice;
                
                if (parent == 0) parent = result;
                result  = &(*result)->unary_expression.operand;
            }
            
            else if (token.kind == Token_Hat)
            {
                SkipToNextToken(state);
                
                *result = AddExpression(state, Expr_Pointer);
                (*result)->directives = directive_slice;
                
                if (parent == 0) parent = result;
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
                    
                    *result = AddExpression(state, Expr_Slice);
                    (*result)->directives = directive_slice;
                    
                    if (parent == 0) parent = result;
                    result = &(*result)->unary_expression.operand;
                }
                
                else if (token.kind == Token_PeriodPeriod && peek.kind == Token_CloseBracket)
                {
                    SkipToNextToken(state);
                    SkipToNextToken(state);
                    
                    *result = AddExpression(state, Expr_DynamicArray);
                    (*result)->directives = directive_slice;
                    
                    if (parent == 0) parent = result;
                    result = &(*result)->unary_expression.operand;
                }
                
                else
                {
                    *result = AddExpression(state, Expr_Array);
                    (*result)->directives = directive_slice;
                    
                    if (!ParseExpression(state, &(*result)->array_type.size)) encountered_errors = true;
                    else
                    {
                        token = GetToken(state);
                        
                        if (token.kind == Token_CloseBracket)
                        {
                            SkipToNextToken(state);
                            
                            if (parent == 0) parent = result;
                            result = &(*result)->array_type.type;
                        }
                        
                        else
                        {
                            //// ERROR: Missing matching closing bracket
                            encountered_errors = true;
                        }
                    }
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
                        bool ended_with_space = token.ends_with_space;
                        
                        Directive* directive = BucketArray_AppendElement(&directives);
                        
                        SkipToNextToken(state);
                        token = GetToken(state);
                        
                        if (ended_with_space && token.kind == Token_OpenParen)
                        {
                            Bucket_Array arguments = BUCKET_ARRAY_INIT(state->temp_memory,
                                                                       Expression*, 10);
                            
                            while(!encountered_errors)
                            {
                                if (!ParseExpression(state, BucketArray_AppendElement(&arguments))) encountered_errors = true;
                                else
                                {
                                    token = GetToken(state);
                                    if (token.kind == Token_Comma) SkipToNextToken(state);
                                    else break;
                                }
                            }
                            
                            if (!encountered_errors)
                            {
                                directive->arguments = BucketArray_FlattenContent(state->decl_memory,
                                                                                  &arguments);
                                
                                token = GetToken(state);
                                if (token.kind == Token_CloseParen) SkipToNextToken(state);
                                else
                                {
                                    //// ERROR: Missing matching closing parenthesis
                                    encountered_errors = true;
                                }
                            }
                        }
                        
                        if (!encountered_errors)
                        {
                            token = GetToken(state);
                            if (token.kind == Token_Hash) continue;
                            else break;
                        }
                    }
                }
            }
            
            else
            {
                token = GetToken(state);
                if (!ParsePrimaryExpression(state, result, (directive_slice.size == 1))) encountered_errors = true;
                else
                {
                    (*result)->directives = directive_slice;
                    
                    if ((*result)->kind != Expr_Empty)
                    {
                        // HACK(soimn): The passing the "parent" pointer is done to help with precedence fixup later
                        if (!ParsePostfixExpression(state, (parent != 0 && token.kind != Token_OpenParen ? parent : result)))
                        {
                            encountered_errors = true;
                        }
                    }
                    
                    break;
                }
            }
        }
    }
    
    return !encountered_errors;
}

bool
ParseIntervalExpression(Parser_State* state, Expression** result)
{
    bool encountered_errors = false;
    
    if (!ParsePrefixExpression(state, result)) encountered_errors = true;
    else
    {
        while (!encountered_errors)
        {
            Token token = GetToken(state);
            
            if (token.kind == Token_PeriodPeriod || token.kind == Token_PeriodPeriodLess)
            {
                
                Expression* min = *result;
                
                *result = AddExpression(state, Expr_Interval);
                (*result)->interval_expression.min     = min;
                (*result)->interval_expression.is_open = (token.kind == Token_PeriodPeriodLess);
                
                SkipToNextToken(state);
                
                if (!ParsePrefixExpression(state, &(*result)->interval_expression.max))
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
ParseMulLevelExpression(Parser_State* state, Expression** result)
{
    bool encountered_errors = false;
    
    if (!ParseIntervalExpression(state, result)) encountered_errors = true;
    else
    {
        while (!encountered_errors)
        {
            Token token = GetToken(state);
            
            Enum8(EXPR_KIND) kind = Expr_Invalid;
            
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
                
                Expression* left = *result;
                
                *result = AddExpression(state, kind);
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
ParseAddLevelExpression(Parser_State* state, Expression** result)
{
    bool encountered_errors = false;
    
    if (!ParseMulLevelExpression(state, result)) encountered_errors = true;
    else
    {
        while (!encountered_errors)
        {
            Token token = GetToken(state);
            
            Enum8(EXPR_KIND) kind = Expr_Invalid;
            
            switch (token.kind)
            {
                case Token_Plus:  kind = Expr_Add;   break;
                case Token_Minus: kind = Expr_Sub;   break;
                case Token_Pipe:  kind = Expr_BitOr; break;
                case Token_Hat:   kind = Expr_BitXor; break;
            }
            
            if (kind != Expr_Invalid)
            {
                SkipToNextToken(state);
                
                Expression* left = *result;
                
                *result = AddExpression(state, kind);
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
ParseComparisonExpression(Parser_State* state, Expression** result)
{
    bool encountered_errors = false;
    
    if (!ParseAddLevelExpression(state, result)) encountered_errors = true;
    else
    {
        while (!encountered_errors)
        {
            Token token = GetToken(state);
            
            Enum8(EXPR_KIND) kind = Expr_Invalid;
            
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
                
                Expression* left = *result;
                
                *result = AddExpression(state, kind);
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
ParseLogicalAndExpression(Parser_State* state, Expression** result)
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
                
                Expression* left = *result;
                
                *result = AddExpression(state, Expr_Or);
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
ParseLogicalOrExpression(Parser_State* state, Expression** result)
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
                
                Expression* left = *result;
                
                *result = AddExpression(state, Expr_Or);
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
ParseTernaryExpression(Parser_State* state, Expression** result)
{
    bool encountered_errors = false;
    
    if (!ParseLogicalOrExpression(state, result)) encountered_errors = true;
    else
    {
        Token token = GetToken(state);
        
        if (token.kind == Token_QuestionMark)
        {
            Expression* condition = *result;
            
            *result = AddExpression(state, Expr_Ternary);
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
ParseExpression(Parser_State* state, Expression** result)
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
ParseBlock(Parser_State* state, Statement* block, bool is_single_line)
{
    bool encountered_errors = false;
    
    Scope_Builder prev_builder = state->scope_builder;
    
    state->scope_builder = (Scope_Builder){
        .expressions = BUCKET_ARRAY_INIT(state->temp_memory, Expression*, 10),
        .statements  = BUCKET_ARRAY_INIT(state->temp_memory, Statement*, 10),
    };
    
    do
    {
        Token token = GetToken(state);
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
            Statement* statement = AddStatement(state);
            if (!ParseStatement(state, statement, true)) encountered_errors = true;
            else
            {
                *(Statement**)BucketArray_AppendElement(&state->scope_builder.statements) = statement;
            }
        }
    } while (!encountered_errors && !is_single_line);
    
    
    Slice expression_slice = BucketArray_FlattenContent(state->decl_memory, &state->scope_builder.expressions);
    Slice statement_slice  = BucketArray_FlattenContent(state->decl_memory, &state->scope_builder.statements);
    
    block->kind              = Statement_Block;
    block->block.expressions = expression_slice;
    block->block.statements  = statement_slice;
    
    state->scope_builder = prev_builder;
    
    return !encountered_errors;
}

bool
ParseScope(Parser_State* state, Statement** block)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state);
    
    *block = AddStatement(state);
    (*block)->kind = Statement_Block;
    
    bool is_single_line = false;
    
    if (token.kind == Token_Identifier && token.keyword == Keyword_Do)
    {
        is_single_line = true;
        SkipToNextToken(state);
    }
    
    else if (token.kind == Token_OpenBrace || token.kind == Token_At)
    {
        is_single_line = false;
        
        if (token.kind == Token_At)
        {
            if (!ParseAttributes(state, &(*block)->attributes))
            {
                encountered_errors = true;
            }
        }
        
        if (!encountered_errors)
        {
            token = GetToken(state);
            
            if (token.kind == Token_OpenBrace) SkipToNextToken(state);
            else
            {
                //// ERROR: Single line scopes must be prefixed with the do keyword
                encountered_errors = true;
            }
        }
    }
    
    else INVALID_CODE_PATH;
    
    if (!encountered_errors)
    {
        if (!ParseBlock(state, *block, is_single_line))
        {
            encountered_errors = false;
        }
    }
    
    return !encountered_errors;
}

bool
ParseStatement(Parser_State* state, Statement* result, bool check_for_semicolon)
{
    bool encountered_errors = false;
    
    *result = (Statement){.kind = Statement_Invalid};
    
    Token token = GetToken(state);
    
    if (!ParseAttributes(state, &result->attributes)) encountered_errors = true;
    else
    {
        token           = GetToken(state);
        Token peek      = PeekNextToken(state);
        Token peek_next = PeekNextToken(state);
        
        if (token.kind == Token_Semicolon)
        {
            if (result->attributes.size == 0)
            {
                //// ERROR: Stray semicolon
                encountered_errors = true;
            }
            
            else
            {
                result->kind = Statement_Empty;
                
                SkipToNextToken(state);
            }
        }
        
        else if (token.kind == Token_OpenBrace)
        {
            result->kind = Statement_Block;
            
            if (!ParseBlock(state, result, false))
            {
                encountered_errors = true;
            }
        }
        
        else if (token.kind == Token_Identifier && token.keyword == Keyword_Import  ||
                 token.kind == Token_Identifier && token.keyword == Keyword_Foreign ||
                 (token.kind == Token_Identifier && token.keyword == Keyword_Using &&
                  peek.kind == Token_Identifier && (peek.keyword == Keyword_Import || token.keyword == Keyword_Foreign)))
        {
            result->kind = Statement_ImportDecl;
            
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
                    String path = {0};
                    {
                        String raw_path = token.string;
                        
                        String prefix_name = {0};
                        String suffix      = raw_path;
                        for (umm i = 0; !encountered_errors && i < raw_path.size; ++i)
                        {
                            if (raw_path.data[i] == ':')
                            {
                                if (suffix.data == raw_path.data)
                                {
                                    prefix_name.data = raw_path.data;
                                    prefix_name.size = i;
                                }
                                
                                else
                                {
                                    //// ERROR: Illegal use of colon in import raw_path. Forward slash is the only legal path separator
                                    encountered_errors = true;
                                }
                            }
                            
                            else if (raw_path.data[i] == '\\')
                            {
                                //// ERROR: Illegal use of backslash in import path. Forward slash is the only legal path separator
                                encountered_errors = true;
                            }
                            
                            else if (raw_path.data[i] == '/')
                            {
                                suffix.data = raw_path.data + (i + 1);
                                suffix.size = raw_path.size - (i + 1);
                            }
                        }
                        
                        if (!encountered_errors)
                        {
                            String prefix_string;
                            if (prefix_name.data != 0)
                            {
                                if (prefix_name.size == 0)
                                {
                                    //// ERROR: Missing name of path prefix
                                    encountered_errors = true;
                                }
                                
                                else
                                {
                                    bool found_prefix = false;
                                    for (umm i = 0; i < state->compiler_state->workspace.path_prefixes.size; ++i)
                                    {
                                        Path_Prefix* path_prefix = Slice_ElementAt(&state->compiler_state->workspace.path_prefixes, Path_Prefix, i);
                                        
                                        if (StringCompare(prefix_name, path_prefix->name))
                                        {
                                            prefix_string = path_prefix->path;
                                            found_prefix  = true;
                                            break;
                                        }
                                    }
                                    
                                    if (!found_prefix)
                                    {
                                        //// ERROR: Unknown path prefix
                                        encountered_errors = true;
                                    }
                                    
                                    else
                                    {
                                        raw_path.data += prefix_name.size + 1;
                                        raw_path.size -= prefix_name.size + 1;
                                    }
                                }
                            }
                            
                            if (!encountered_errors)
                            {
                                if (is_foreign)
                                {
                                    if (suffix.size == 0)
                                    {
                                        //// ERROR: Missing file name in foreign import path
                                        encountered_errors = true;
                                    }
                                }
                                
                                else
                                {
                                    if (suffix.size == 0)
                                    {
                                        //// ERROR: Missing name of directory in import path
                                        encountered_errors = true;
                                    }
                                    
                                    else if (suffix.data != prefix_name.data)
                                    {
                                        if (suffix.size > 1 && !StringCompare(suffix, CONST_STRING("..")))
                                        {
                                            for (umm i = 0; i < suffix.size; ++i)
                                            {
                                                if (suffix.data[i] == '.')
                                                {
                                                    //// ERROR: Cannot import files
                                                    encountered_errors = true;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                                
                                if (!encountered_errors)
                                {
                                    void* memory = Arena_Allocate(state->temp_memory, prefix_string.size + raw_path.size, ALIGNOF(u8));
                                    Copy(prefix_string.data, memory, prefix_string.size);
                                    Copy(raw_path.data, (u8*)memory + prefix_string.size, raw_path.size);
                                    
                                    path.data = memory;
                                    path.size = prefix_string.size + raw_path.size;
                                }
                            }
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        SkipToNextToken(state);
                        
                        if (is_foreign)
                        {
                            NOT_IMPLEMENTED;
                        }
                        
                        else
                        {
                            NOT_IMPLEMENTED;
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
        
        else if (token.kind == Token_Identifier && token.keyword != Keyword_Invalid &&
                 token.keyword != Keyword_True  && token.keyword != Keyword_False &&
                 !(peek.kind == Token_Identifier && (peek_next.kind == Token_Colon      || peek_next.kind == Token_ColonEqual ||
                                                     peek_next.kind == Token_ColonColon || peek_next.kind == Token_Comma)))
        {
            if (token.keyword == Keyword_If)
            {
                result->kind = Statement_If;
                
                SkipToNextToken(state);
                
                Statement statement;
                if (!ParseStatement(state, &statement, false)) encountered_errors = true;
                else
                {
                    token = GetToken(state);
                    
                    if (token.kind != Token_Semicolon)
                    {
                        if (statement.kind != Statement_Expression)
                        {
                            //// ERROR: Missing condition in if statement
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            if (statement.attributes.size != 0)
                            {
                                //// ERROR: Attributes can only be applied to statements
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                result->if_statement.condition = statement.expression;
                            }
                        }
                    }
                    
                    else
                    {
                        SkipToNextToken(state);
                        
                        result->if_statement.init  = AddStatement(state);
                        *result->if_statement.init = statement;
                        
                        if (!ParseExpression(state, &result->if_statement.condition))
                        {
                            encountered_errors = true;
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(state);
                        
                        if (token.kind != Token_OpenBrace && token.kind != Token_At &&
                            (token.kind != Token_Identifier || token.keyword != Keyword_Do))
                        {
                            //// ERROR: Missing body of if statement
                        }
                        
                        else
                        {
                            if (!ParseScope(state, &result->if_statement.true_body)) encountered_errors = true;
                            else
                            {
                                token = GetToken(state);
                                
                                if (token.kind == Token_Identifier && token.keyword == Keyword_Else)
                                {
                                    SkipToNextToken(state);
                                    token = GetToken(state);
                                    
                                    if (token.kind == Token_Identifier && token.keyword == Keyword_If)
                                    {
                                        result->if_statement.false_body = AddStatement(state);
                                        if (!ParseStatement(state, result->if_statement.false_body, true))
                                        {
                                            encountered_errors = true;
                                        }
                                    }
                                    
                                    else
                                    {
                                        if (token.kind != Token_OpenBrace && token.kind != Token_At &&
                                            (token.kind != Token_Identifier || token.keyword != Keyword_Do))
                                        {
                                            //// ERROR: Missing body of else clause in if statement
                                            encountered_errors = true;
                                        }
                                        
                                        else
                                        {
                                            if (!ParseScope(state, &result->if_statement.false_body))
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
                result->kind = Statement_When;
                
                SkipToNextToken(state);
                
                Statement statement;
                if (!ParseStatement(state, &statement, false)) encountered_errors = true;
                else
                {
                    if (statement.kind != Statement_Expression)
                    {
                        //// ERROR: Missing condition in when statements. When statements do not support init
                        ////        statements
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        if (statement.attributes.size != 0)
                        {
                            //// ERROR: Attributes can only be applied to statements
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            result->when_statement.condition = statement.expression;
                            
                            token = GetToken(state);
                            
                            if (token.kind != Token_OpenBrace && token.kind != Token_At &&
                                (token.kind != Token_Identifier || token.keyword != Keyword_Do))
                            {
                                //// ERROR: Missing body of when statement
                            }
                            
                            else
                            {
                                if (!ParseScope(state, &result->when_statement.true_body)) encountered_errors = true;
                                else
                                {
                                    token = GetToken(state);
                                    
                                    if (token.kind == Token_Identifier && token.keyword == Keyword_Else)
                                    {
                                        SkipToNextToken(state);
                                        token = GetToken(state);
                                        
                                        if (token.kind == Token_Identifier && token.keyword == Keyword_When)
                                        {
                                            result->when_statement.false_body = AddStatement(state);
                                            if (!ParseStatement(state, result->when_statement.false_body, true))
                                            {
                                                encountered_errors = true;
                                            }
                                        }
                                        
                                        else
                                        {
                                            if (token.kind != Token_OpenBrace && token.kind != Token_At &&
                                                (token.kind != Token_Identifier || token.keyword != Keyword_Do))
                                            {
                                                //// ERROR: Missing body of else clause in when statement
                                                encountered_errors = true;
                                            }
                                            
                                            else
                                            {
                                                if (!ParseScope(state, &result->when_statement.false_body))
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
            
            else if (token.keyword == Keyword_While)
            {
                result->kind = Statement_While;
                
                SkipToNextToken(state);
                
                Statement statement;
                if (!ParseStatement(state, &statement, false)) encountered_errors = true;
                else
                {
                    if (statement.kind == Statement_Expression)
                    {
                        if (statement.attributes.size != 0)
                        {
                            //// ERROR: Attributes can only be applied to statements
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            result->while_statement.condition = statement.expression;
                        }
                    }
                    
                    else
                    {
                        result->while_statement.init  = AddStatement(state);
                        *result->while_statement.init = statement;
                        
                        token = GetToken(state);
                        if (token.kind != Token_Semicolon)
                        {
                            //// ERROR: Missing condition in while statement
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            if (!ParseExpression(state, &result->while_statement.condition))
                            {
                                encountered_errors = true;
                            }
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(state);
                        if (token.kind == Token_Semicolon)
                        {
                            SkipToNextToken(state);
                            
                            result->while_statement.step = AddStatement(state);
                            if (!ParseStatement(state, result->while_statement.step, false))
                            {
                                encountered_errors = true;
                            }
                        }
                        
                        token = GetToken(state);
                        if (token.kind != Token_OpenBrace && token.kind != Token_At &&
                            (token.keyword != Token_Identifier || token.keyword != Keyword_Do))
                        {
                            //// ERROR: Missing body of while statement
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            if (!ParseScope(state, &result->while_statement.body))
                            {
                                encountered_errors = true;
                            }
                        }
                    }
                }
            }
            
            else if (token.keyword == Keyword_Break || token.keyword == Keyword_Continue)
            {
                Enum8(STATEMENT_KIND) kind = (token.keyword == Keyword_Break ? Statement_Break : Statement_Continue);
                
                result->kind = kind;
                
                SkipToNextToken(state);
                token = GetToken(state);
                
                if (token.kind != Token_Semicolon)
                {
                    Expression* expression;
                    if (!ParseExpression(state, &expression)) encountered_errors = true;
                    else
                    {
                        if (kind == Statement_Break) result->break_statement.label    = expression;
                        else                         result->continue_statement.label = expression;
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
                result->kind = Statement_Defer;
                
                SkipToNextToken(state);
                token = GetToken(state);
                
                if (token.kind == Token_Identifier && token.keyword == Keyword_Do)
                {
                    //// ERROR: Invalid use of do keyword. Defer statements are directly followed by a statement, no do is used.
                    encountered_errors = true;
                }
                
                else
                {
                    result->defer_statement.statement = AddStatement(state);
                    if (!ParseStatement(state, result->defer_statement.statement, true))
                    {
                        encountered_errors = true;
                    }
                }
            }
            
            else if (token.keyword == Keyword_Using)
            {
                result->kind = Statement_Using;
                
                SkipToNextToken(state);
                
                if (!ParseExpression(state, &result->using_statement.symbol_path)) encountered_errors = true;
                else
                {
                    token = GetToken(state);
                    
                    if (token.kind == Token_Identifier && token.keyword == Keyword_As)
                    {
                        SkipToNextToken(state);
                        token = GetToken(state);
                        
                        if (!ParseExpression(state, &result->using_statement.alias))
                        {
                            encountered_errors = true;
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
                result->kind = Statement_Return;
                
                token = GetToken(state);
                if (token.kind != Token_Semicolon)
                {
                    if (!ParseNamedArgumentList(state, &result->return_statement.arguments))
                    {
                        encountered_errors = true;
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
            
            else
            {
                //// ERROR: Invalid use of keyword
                encountered_errors = true;
            }
        }
        
        else
        {
            bool is_using = false;
            if (token.kind == Token_Identifier && token.keyword == Keyword_Using)
            {
                is_using = true;
                
                SkipToNextToken(state);
            }
            
            Expression* expression;
            if (!ParseExpression(state, &expression)) encountered_errors = true;
            else
            {
                token = GetToken(state);
                peek  = PeekNextToken(state);
                
                if (token.kind == Token_Colon &&
                    (peek.kind == Token_OpenBrace ||
                     peek.kind == Token_Identifier && (peek.keyword == Keyword_If    ||
                                                       peek.keyword == Keyword_When  ||
                                                       peek.keyword == Keyword_Else  ||
                                                       peek.keyword == Keyword_For   ||
                                                       peek.keyword == Keyword_While)))
                {
                    if (peek.kind == Token_Identifier && peek.keyword == Keyword_Else)
                    {
                        //// ERROR: Illegal use of label on else statement
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        SkipToNextToken(state);
                        
                        Slice attributes = result->attributes;
                        if (!ParseStatement(state, result, true)) encountered_errors = true;
                        else
                        {
                            result->attributes = attributes;
                            
                            switch(result->kind)
                            {
                                case Statement_Block: result->block.label           = expression; break;
                                case Statement_If:    result->if_statement.label    = expression; break;
                                case Statement_When:  result->when_statement.label  = expression; break;
                                case Statement_While: result->while_statement.label = expression; break;
                                INVALID_DEFAULT_CASE;
                            }
                        }
                    }
                }
                
                else if (token.kind == Token_Comma || token.kind == Token_Colon ||
                         token.kind == Token_ColonEqual || token.kind == Token_ColonColon ||
                         IsAssignment(token.kind))
                {
                    Bucket_Array lhs = BUCKET_ARRAY_INIT(state->temp_memory, Expression*, 3);
                    *(Expression**)BucketArray_AppendElement(&lhs) = expression;
                    
                    Token token = GetToken(state);
                    
                    if (token.kind == Token_Comma)
                    {
                        SkipToNextToken(state);
                        
                        while (!encountered_errors)
                        {
                            if (!ParseExpression(state, BucketArray_AppendElement(&lhs))) encountered_errors = true;
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
                        
                        Bucket_Array types  = BUCKET_ARRAY_INIT(state->temp_memory, Expression*, 3);
                        Bucket_Array values = BUCKET_ARRAY_INIT(state->temp_memory, Expression*, 3);
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
                                        Expression* rhs = BucketArray_ElementAt(&values, 0);
                                        
                                        if (rhs->kind == Expr_Proc  || rhs->kind == Expr_Struct ||
                                            rhs->kind == Expr_Union || rhs->kind == Expr_Enum)
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
                                if (BucketArray_ElementCount(&lhs)    > 1 ||
                                    BucketArray_ElementCount(&types)  > 1 ||
                                    BucketArray_ElementCount(&values) > 1)
                                {
                                    Slice name_slice  = BucketArray_FlattenContent(state->decl_memory, &lhs);
                                    Slice type_slice  = BucketArray_FlattenContent(state->decl_memory, &types);
                                    Slice value_slice = BucketArray_FlattenContent(state->decl_memory, &values);
                                    
                                    if (is_constant)
                                    {
                                        result->kind = Statement_ConstantDecl;
                                        
                                        result->constant_declaration.names  = name_slice;
                                        result->constant_declaration.types  = type_slice;
                                        result->constant_declaration.values = value_slice;
                                        result->constant_declaration.is_multi = true;
                                        result->constant_declaration.is_using = is_using;
                                        
                                    }
                                    
                                    else
                                    {
                                        result->kind = Statement_VariableDecl;
                                        
                                        result->variable_declaration.names  = name_slice;
                                        result->variable_declaration.types  = type_slice;
                                        result->variable_declaration.values = value_slice;
                                        result->variable_declaration.is_multi         = true;
                                        result->variable_declaration.is_using         = is_using;
                                        result->variable_declaration.is_uninitialized = is_uninitialized;
                                    }
                                }
                                
                                else
                                {
                                    if (is_constant)
                                    {
                                        result->kind = Statement_ConstantDecl;
                                        
                                        result->constant_declaration.name  = BucketArray_ElementAt(&lhs, 0);
                                        result->constant_declaration.type  = BucketArray_ElementAt(&types, 0);
                                        result->constant_declaration.value = BucketArray_ElementAt(&values, 0);
                                        result->constant_declaration.is_multi = false;
                                        
                                    }
                                    
                                    else
                                    {
                                        result->kind = Statement_VariableDecl;
                                        
                                        result->variable_declaration.name  = BucketArray_ElementAt(&lhs, 0);
                                        result->variable_declaration.type  = BucketArray_ElementAt(&types, 0);
                                        result->variable_declaration.value = BucketArray_ElementAt(&values, 0);
                                        result->variable_declaration.is_multi         = false;
                                        result->variable_declaration.is_using         = is_using;
                                        result->variable_declaration.is_uninitialized = is_uninitialized;
                                    }
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
                                result->kind = Statement_Assignment;
                                
                                Bucket_Array rhs = BUCKET_ARRAY_INIT(state->temp_memory, Expression*, 3);
                                
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
                                            SkipToNextToken(state);
                                            
                                            if (BucketArray_ElementCount(&lhs) > 1 ||
                                                BucketArray_ElementCount(&rhs) > 1)
                                            {
                                                Slice lhs_slice = BucketArray_FlattenContent(state->decl_memory, &lhs);
                                                Slice rhs_slice = BucketArray_FlattenContent(state->decl_memory, &rhs);
                                                
                                                result->assignment_statement.left_exprs  = lhs_slice;
                                                result->assignment_statement.right_exprs = rhs_slice;
                                                result->assignment_statement.is_multi    = true;
                                            }
                                            
                                            else
                                            {
                                                result->assignment_statement.left_expr  = BucketArray_ElementAt(&lhs, 0);
                                                result->assignment_statement.right_expr = BucketArray_ElementAt(&rhs, 0);
                                                result->assignment_statement.is_multi   = false;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                else
                {
                    result->kind       = Statement_Expression;
                    result->expression = expression;
                    
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
    
    return !encountered_errors;
}

bool
ParseFile(Compiler_State* compiler_state, Package_ID package_id, File_ID file_id)
{
    bool encountered_errors = false;
    
    Arena_ClearAll(&compiler_state->temp_memory);
    
    Parser_State state = {};
    state.compiler_state = compiler_state;
    state.tokens         = BUCKET_ARRAY_INIT(&compiler_state->temp_memory, Token, 256);
    
    state.scope_builder = (Scope_Builder){
        .expressions = BUCKET_ARRAY_INIT(&compiler_state->temp_memory, Expression*, 10),
        .statements  = BUCKET_ARRAY_INIT(&compiler_state->temp_memory, Statement*, 10),
    };
    
    if (!LexFile(compiler_state, file_id, &state.tokens)) encountered_errors = true;
    else
    {
        state.token_it = BucketArray_CreateIterator(&state.tokens);
        state.peek_it  = BucketArray_CreateIterator(&state.tokens);
        
        while (!encountered_errors)
        {
            Memory_Arena temp_memory = Arena_BeginTempMemory(&compiler_state->temp_memory);
            state.temp_memory = &temp_memory;
            state.decl_memory = &compiler_state->persistent_memory;
            
            Token token = GetToken(&state);
            
            if (token.kind == Token_EndOfStream) break;
            else
            {
                Statement* statement = AddStatement(&state);
                
                if (!ParseStatement(&state, statement, true))
                {
                    encountered_errors = true;
                }
            }
            
            Arena_EndTempMemory(&compiler_state->temp_memory, &temp_memory);
        }
    }
    
    if (!encountered_errors)
    {
        for (Bucket_Array_Iterator it = BucketArray_CreateIterator(&state.scope_builder.statements);
             it.current != 0;
             BucketArray_AdvanceIterator(&state.scope_builder.statements, &it))
        {
            Statement* statement = it.current;
            Workspace_PushDeclaration(&compiler_state->workspace, (Declaration){.package = package_id, .ast = statement}, DeclarationStage_Parsed);
        }
    }
    
    return !encountered_errors;
}