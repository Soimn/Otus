// TODO(soimn): Consider implementing an "array builder" that uses temp storage to build arrays before commiting 
//              the final array to persistent memory

typedef struct Parser_State
{
    Workspace* workspace;
    Package* package;
    Lexer* lexer;
    
    Scope* current_scope;
    bool export_by_default;
    bool is_global_scope;
    bool is_proc_parameter;
    bool should_bounds_check;
    
    //Buffer array_builder_buffer;
} Parser_State;

bool ParseExpression(Parser_State state, Expression** expr);
bool ParseDeclaration(Parser_State state, Declaration* declaration);
bool ParseStatement(Parser_State state);
bool ParseScope(Parser_State state, Scope* scope, bool allow_single_statement);

Expression*
AppendExpression(Parser_State state, Enum32(EXPRESSION_KIND) kind)
{
    Expression* expression = BucketArray_Append(&state.package->expressions);
    expression->kind = kind;
    
    switch (expression->kind)
    {
        case Expression_Call:
        expression->call.arguments = ARRAY_INIT(&state.package->arena, Named_Argument, 3);
        break;
        
        case Expression_DataLiteral:
        expression->data_literal.arguments = ARRAY_INIT(&state.package->arena, Named_Argument, 3);
        break;
        
        case Expression_Proc:
        expression->procedure.parameters    = ARRAY_INIT(&state.package->arena, Procedure_Parameter, 3);
        expression->procedure.return_values = ARRAY_INIT(&state.package->arena, Procedure_Return_Value, 1);
        break;
        
        case Expression_Struct:
        expression->structure.parameters = ARRAY_INIT(&state.package->arena, Structure_Parameter, 1);
        expression->structure.members    = ARRAY_INIT(&state.package->arena, Structure_Member, 3);
        break;
        
        case Expression_Enum:
        expression->enumeration.members = ARRAY_INIT(&state.package->arena, Enumeration_Member, 3);
        break;
        
        INVALID_DEFAULT_CASE;
    }
    
    return expression;
}

Statement*
AppendStatement(Parser_State state, Enum32(STATEMENT_KIND) kind)
{
    Statement* statement = Array_Append(&state.current_scope->statements);
    statement->kind = kind;
    
    switch (statement->kind)
    {
        case Statement_Declaration:
        statement->declaration.attributes = ARRAY_INIT(&state.package->arena, Attribute, 0);
        statement->declaration.names      = ARRAY_INIT(&state.package->arena, String, 1);
        statement->declaration.types      = ARRAY_INIT(&state.package->arena, Expression*, 1);
        break;
        
        case Statement_Return:
        statement->return_statement.arguments = ARRAY_INIT(&state.package->arena, Named_Argument, 2);
        break;
        
        case Statement_Assignment:
        statement->assignment_statement.left  = ARRAY_INIT(&state.package->arena, Expression*, 2);
        statement->assignment_statement.right = ARRAY_INIT(&state.package->arena, Expression*, 2);
        break;
        
        INVALID_DEFAULT_CASE;
    }
    
    return statement;
}

enum DIRECTIVE_KIND
{
    Directive_ScopeExport,
    Directive_ScopeFile,
    Directive_If,
    Directive_BoundsCheck,
    Directive_NoBoundsCheck,
    Directive_Assert,
    Directive_Distinct,
    Directive_Run,
    Directive_Location,
    Directive_CallerLocation,
    Directive_File,
    Directive_Line,
    Directive_NoAlias,
    Directive_Required,
    
    DIRECTIVE_COUNT
};

global String DirectiveStrings[DIRECTIVE_COUNT] = {
    [Directive_ScopeExport]     = CONST_STRING("scope_export"),
    [Directive_ScopeFile]       = CONST_STRING("scope_file"),
    [Directive_If]              = CONST_STRING("if"),
    [Directive_BoundsCheck]     = CONST_STRING("bounds_check"),
    [Directive_NoBoundsCheck]   = CONST_STRING("no_bounds_check"),
    [Directive_Assert]          = CONST_STRING("assert"),
    [Directive_Distinct]        = CONST_STRING("distinct"),
    [Directive_Run]             = CONST_STRING("run"),
    [Directive_Location]        = CONST_STRING("location"),
    [Directive_CallerLocation]  = CONST_STRING("caller_location"),
    [Directive_File]            = CONST_STRING("file"),
    [Directive_Line]            = CONST_STRING("line"),
    [Directive_NoAlias]         = CONST_STRING("no_alias"),
    [Directive_Required]        = CONST_STRING("required")
};

bool
ParsePostfixUnaryExpression(Parser_State state, Expression** expr)
{
    bool encountered_errors = false;
    
    Expression** current = expr;
    
    while (!encountered_errors)
    {
        Token token = GetToken(state.lexer);
        
        if (token.kind == Token_OpenParen)
        {
            SkipPastToken(state.lexer, token);
            
            Expression* pointer = *current;
            
            *current = AppendExpression(state, Expression_Call);
            (*current)->call.pointer = pointer;
            
            token = GetToken(state.lexer);
            
            if (token.kind != Token_CloseParen)
            {
                while (!encountered_errors)
                {
                    Named_Argument* argument = Array_Append(&(*current)->call.arguments);
                    
                    Token peek = PeekNextToken(state.lexer, token);
                    
                    if (token.kind == Token_Identifier && peek.kind == Token_Equals &&
                        PeekNextToken(state.lexer, peek).kind != Token_Equals)
                    {
                        argument->name = token.string;
                        
                        SkipPastToken(state.lexer, peek);
                    }
                    
                    if (!ParseExpression(state, &argument->value)) encountered_errors = true;
                    else
                    {
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_Comma) SkipPastToken(state.lexer, token);
                        else break;
                    }
                }
            }
            
            if (!encountered_errors)
            {
                token = GetToken(state.lexer);
                
                if (token.kind == Token_CloseParen) SkipPastToken(state.lexer, token);
                else
                {
                    //// ERROR: Missing matching closing parenthesis after arguments in procedure call
                    encountered_errors = true;
                }
            }
        }
        
        else if (token.kind == Token_OpenBracket)
        {
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            Expression* start  = 0;
            Expression* length = 0;
            bool is_span       = false;
            
            if (token.kind != Token_Colon)
            {
                if (!ParseExpression(state, &start))
                {
                    encountered_errors = true;
                }
            }
            
            token = GetToken(state.lexer);
            if (!encountered_errors && token.kind == Token_Colon)
            {
                is_span = true;
                
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                if (token.kind != Token_CloseBracket)
                {
                    if (!ParseExpression(state, &length))
                    {
                        encountered_errors = true;
                    }
                }
            }
            
            if (!encountered_errors)
            {
                token = GetToken(state.lexer);
                if (token.kind != Token_CloseBracket)
                {
                    //// ERROR: Missing matching closing bracket
                    encountered_errors = true;
                }
                
                else
                {
                    SkipPastToken(state.lexer, token);
                    
                    Expression* array = *current;
                    
                    *current = AppendExpression(state, Expression_Subscript);
                    
                    (*current)->subscript.array               = array;
                    (*current)->subscript.start               = start;
                    (*current)->subscript.length              = length;
                    (*current)->subscript.is_span             = is_span;
                    (*current)->subscript.should_bounds_check = state.should_bounds_check;
                }
            }
        }
        
        else if (token.kind == Token_Period)
        {
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            if (token.kind != Token_Identifier)
            {
                //// ERROR: Missing name of member on right side of member access operator
                encountered_errors = true;
            }
            
            else
            {
                Expression* left = *current;
                
                *current = AppendExpression(state, Expression_Member);
                (*current)->left         = left;
                (*current)->right         = AppendExpression(state, Expression_Identifier);
                (*current)->right->string = token.string;
                
                SkipPastToken(state.lexer, token);
            }
        }
        
        else break;
    }
    
    return !encountered_errors;
}

bool
ParsePrimaryExpression(Parser_State state, Expression** expr)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    
    if (token.kind == Token_Hash)
    {
        SkipPastToken(state.lexer, token);
        token = GetToken(state.lexer);
        
        if (token.kind == Token_Identifier)
        {
            if (StringCompare(token.string, DirectiveStrings[Directive_Run]))
            {
                SkipPastToken(state.lexer, token);
                
                *expr = AppendExpression(state, Expression_Run);
                
                if (!ParseExpression(state, &(*expr)->operand))
                {
                    encountered_errors = true;
                }
            }
            
            else if (StringCompare(token.string, DirectiveStrings[Directive_Location]) ||
                     StringCompare(token.string, DirectiveStrings[Directive_File])     ||
                     StringCompare(token.string, DirectiveStrings[Directive_Line]))
            {
                *expr = AppendExpression(state, Expression_Location);
                
                if (StringCompare(token.string, DirectiveStrings[Directive_File]))
                {
                    (*expr)->location.is_file = true;
                }
                
                else if (StringCompare(token.string, DirectiveStrings[Directive_Line]))
                {
                    (*expr)->location.is_line = true;
                }
                
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                if (token.kind == Token_OpenParen)
                {
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (token.kind != Token_Identifier)
                    {
                        //// ERROR: Missing name of declaration after parenthesis in * directive
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        (*expr)->location.name = token.string;
                        
                        SkipPastToken(state.lexer, token);
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_CloseParen) SkipPastToken(state.lexer, token);
                        else
                        {
                            //// ERROR: Missing matching closing parenthesis after declaration name in * directive
                            encountered_errors = true;
                        }
                    }
                }
            }
            
            else if (StringCompare(token.string, DirectiveStrings[Directive_CallerLocation]))
            {
                if (state.is_proc_parameter) *expr = AppendExpression(state, Expression_CallerLocation);
                else
                {
                    //// ERROR: Invalid use of caller location directive outside procedure parameter list
                    encountered_errors = true;
                }
            }
            
            else if (StringCompare(token.string, DirectiveStrings[Directive_BoundsCheck]) ||
                     StringCompare(token.string, DirectiveStrings[Directive_NoBoundsCheck]))
            {
                state.should_bounds_check = StringCompare(token.string, DirectiveStrings[Directive_BoundsCheck]);
                
                SkipPastToken(state.lexer, token);
                
                if (!ParseExpression(state, expr))
                {
                    encountered_errors = true;
                }
            }
            
            else
            {
                uint i = 0;
                for (; i < DIRECTIVE_COUNT; ++i)
                {
                    if (StringCompare(token.string, DirectiveStrings[i])) break;
                }
                
                if (i < DIRECTIVE_COUNT)
                {
                    //// ERROR: Invalid use of diretive at expression level
                    encountered_errors = true;
                }
                
                else
                {
                    //// ERROR: Missing name of directive
                    encountered_errors = true;
                }
            }
        }
        
        else
        {
            //// ERROR: Missing name of directive
            encountered_errors = true;
        }
    }
    
    else if (token.kind == Token_OpenParen)
    {
        SkipPastToken(state.lexer, token);
        
        if (!ParseExpression(state, expr)) encountered_errors = true;
        else
        {
            token = GetToken(state.lexer);
            
            if (token.kind == Token_CloseParen) SkipPastToken(state.lexer, token);
            else
            {
                //// ERROR: Missing matching closing parenthesis after expression
                encountered_errors = true;
            }
        }
    }
    
    else if (token.kind == Token_OpenBrace)
    {
        SkipPastToken(state.lexer, token);
        token = GetToken(state.lexer);
        
        *expr = AppendExpression(state, Expression_DataLiteral);
        
        if (token.kind == Token_Colon)
        {
            SkipPastToken(state.lexer, token);
            
            if (!ParseExpression(state, &(*expr)->data_literal.type)) encountered_errors = true;
            else
            {
                token = GetToken(state.lexer);
                
                if (token.kind == Token_Colon) SkipPastToken(state.lexer, token);
                else
                {
                    //// ERROR: Missing matching colon after type in data literal
                    encountered_errors = true;
                }
            }
        }
        
        token = GetToken(state.lexer);
        
        if (token.kind != Token_CloseBrace)
        {
            while (!encountered_errors)
            {
                token      = GetToken(state.lexer);
                Token peek = PeekNextToken(state.lexer, token);
                
                Named_Argument* argument = Array_Append(&(*expr)->data_literal.arguments);
                
                if (token.kind == Token_Identifier && peek.kind == Token_Equals &&
                    PeekNextToken(state.lexer, peek).kind != Token_Equals)
                {
                    argument->name = token.string;
                    
                    SkipPastToken(state.lexer, peek);
                }
                
                if (!ParseExpression(state, &argument->value)) encountered_errors = true;
                else
                {
                    token = GetToken(state.lexer);
                    
                    if (token.kind == Token_Comma) SkipPastToken(state.lexer, token);
                    else break;
                }
            }
        }
        
        if (!encountered_errors)
        {
            if (token.kind == Token_CloseBrace) SkipPastToken(state.lexer, token);
            else
            {
                //// ERROR: Missing terminating closing brace after argument in data literal
                encountered_errors = true;
            }
        }
    }
    
    else if (token.kind == Token_Cash)
    {
        if (!state.is_proc_parameter)
        {
            //// ERROR: Use of polymorphic name definition in this context is illegal. Polymorphic names can only
            ////        be used in procedure arguments
            encountered_errors = true;
        }
        
        else
        {
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            if (token.kind != Token_Identifier)
            {
                //// ERROR: Missing name of polymorphic type after cash token
                encountered_errors = true;
            }
            
            else
            {
                *expr = AppendExpression(state, Expression_PolymorphicName);
                (*expr)->string = token.string;
                
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                if (token.kind == Token_Slash)
                {
                    SkipPastToken(state.lexer, token);
                    
                    Expression* left = *expr;
                    
                    *expr = AppendExpression(state, Expression_PolymorphicAlias);
                    (*expr)->left = left;
                    
                    if (!ParsePrimaryExpression(state, &(*expr)->right))
                    {
                        encountered_errors = true;
                    }
                }
            }
        }
    }
    
    else if (token.kind == Token_Identifier)
    {
        if (StringCompare(token.string, KeywordStrings[Keyword_Proc]))
        {
            *expr = AppendExpression(state, Expression_Proc);
            
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            if (token.kind == Token_OpenParen)
            {
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                if (token.kind != Token_CloseParen)
                {
                    while (!encountered_errors)
                    {
                        Procedure_Parameter* parameter = Array_Append(&(*expr)->procedure.parameters);
                        
                        token      = GetToken(state.lexer);
                        Token peek = PeekNextToken(state.lexer, token);
                        
                        if (token.kind == Token_Hash && peek.kind == Token_Identifier &&
                            StringCompare(peek.string, DirectiveStrings[Directive_NoAlias]))
                        {
                            SkipPastToken(state.lexer, peek);
                            
                            parameter->no_alias = true;
                            
                            token = GetToken(state.lexer);
                            peek  = PeekNextToken(state.lexer, token);
                        }
                        
                        if (token.kind == Token_Identifier &&
                            StringCompare(token.string, KeywordStrings[Keyword_Using]))
                        {
                            SkipPastToken(state.lexer, token);
                            
                            parameter->is_using = true;
                            
                            token = GetToken(state.lexer);
                            peek  = PeekNextToken(state.lexer, token);
                        }
                        
                        if (token.kind == Token_Cash)
                        {
                            if (peek.kind == Token_Cash)
                            {
                                SkipPastToken(state.lexer, peek);
                                parameter->is_baked = true;
                            }
                            
                            else
                            {
                                SkipPastToken(state.lexer, token);
                                parameter->is_const = true;
                            }
                            
                            token = GetToken(state.lexer);
                            peek  = PeekNextToken(state.lexer, token);
                        }
                        
                        if (token.kind != Token_Identifier || peek.kind != Token_Colon)
                        {
                            if (peek.kind != Token_Colon)
                            {
                                //// ERROR: Missing colon after name of procedure parameter
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                //// ERROR: Missing name of procedure parameter
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            parameter->name = token.string;
                            
                            SkipPastToken(state.lexer, peek);
                            token = GetToken(state.lexer);
                            
                            if (token.kind != Token_Equals)
                            {
                                // NOTE(soimn): This is the only place polymorphic names and aliases are allowed
                                state.is_proc_parameter = true;
                                
                                if (!ParseExpression(state, &parameter->type))
                                {
                                    encountered_errors = true;
                                }
                                
                                state.is_proc_parameter = false;
                            }
                            
                            if (!encountered_errors)
                            {
                                token = GetToken(state.lexer);
                                
                                if (token.kind == Token_Equals)
                                {
                                    if (!ParseExpression(state, &parameter->value)) encountered_errors = true;
                                    {
                                        token = GetToken(state.lexer);
                                        
                                        if (token.kind == Token_Comma) SkipPastToken(state.lexer, token);
                                        else break;
                                    }
                                }
                            }
                        }
                    }
                }
                
                if (!encountered_errors)
                {
                    token = GetToken(state.lexer);
                    
                    if (token.kind == Token_CloseParen) SkipPastToken(state.lexer, token);
                    else
                    {
                        //// ERROR: Missing matching closing parnethesis after procedure parameters
                        encountered_errors = true;
                    }
                }
            }
            
            if (!encountered_errors)
            {
                token      = GetToken(state.lexer);
                Token peek = PeekNextToken(state.lexer, token);
                
                if (token.kind == Token_Minus && peek.kind == Token_Greater)
                {
                    SkipPastToken(state.lexer, peek);
                    token = GetToken(state.lexer);
                    
                    bool is_array = false;
                    if (token.kind == Token_OpenParen)
                    {
                        is_array = true;
                        
                        SkipPastToken(state.lexer, token);
                    }
                    
                    while (!encountered_errors)
                    {
                        Procedure_Return_Value* return_value = Array_Append(&(*expr)->procedure.return_values);
                        
                        token = GetToken(state.lexer);
                        peek  = PeekNextToken(state.lexer, token);
                        
                        if (token.kind == Token_Hash && peek.kind == Token_Identifier &&
                            StringCompare(peek.string, DirectiveStrings[Directive_Required]))
                        {
                            SkipPastToken(state.lexer, peek);
                            
                            return_value->is_required = true;
                            
                            token = GetToken(state.lexer);
                            peek  = PeekNextToken(state.lexer, token);
                        }
                        
                        if (token.kind == Token_Identifier && peek.kind == Token_Equals)
                        {
                            return_value->name = token.string;
                            
                            SkipPastToken(state.lexer, peek);
                        }
                        
                        if (!ParseExpression(state, &return_value->type)) encountered_errors = true;
                        else
                        {
                            token = GetToken(state.lexer);
                            
                            if (is_array && token.kind == Token_Comma) SkipPastToken(state.lexer, token);
                            else break;
                        }
                    }
                    
                    if (!encountered_errors && is_array)
                    {
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_CloseParen) SkipPastToken(state.lexer, token);
                        else
                        {
                            //// ERROR: Missing matching closing parenthesis after procedure return values list
                            encountered_errors = true;
                        }
                    }
                }
            }
            
            if (!encountered_errors)
            {
                token = GetToken(state.lexer);
                
                if (token.kind != Token_OpenBrace)
                {
                    Token peek   = PeekNextToken(state.lexer, token);
                    Token peek_1 = PeekNextToken(state.lexer, peek);
                    
                    if (token.kind == Token_Minus && peek.kind == Token_Minus &&
                        peek_1.kind == Token_Minus)
                    {
                        SkipPastToken(state.lexer, peek_1);
                        (*expr)->procedure.has_body = false;
                    }
                    
                    else
                    {
                        (*expr)->kind = Expression_ProcPointer;
                    }
                }
                
                else
                {
                    (*expr)->procedure.has_body = true;
                    
                    if (!ParseScope(state, &(*expr)->procedure.body, false))
                    {
                        encountered_errors = true;
                    }
                }
            }
        }
        
        else if (StringCompare(token.string, KeywordStrings[Keyword_Struct]) ||
                 StringCompare(token.string, KeywordStrings[Keyword_Union]))
        {
            *expr = AppendExpression(state, Expression_Struct);
            
            (*expr)->structure.is_union = StringCompare(token.string, KeywordStrings[Keyword_Union]);
            
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            if (token.kind == Token_OpenParen)
            {
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                if (token.kind != Token_CloseParen)
                {
                    while (!encountered_errors)
                    {
                        Structure_Parameter* parameter = Array_Append(&(*expr)->structure.parameters);
                        
                        token      = GetToken(state.lexer);
                        Token peek = PeekNextToken(state.lexer, token);
                        
                        if (token.kind == Token_Identifier &&
                            StringCompare(token.string, KeywordStrings[Keyword_Using]))
                        {
                            SkipPastToken(state.lexer, token);
                            
                            parameter->is_using = true;
                            
                            token = GetToken(state.lexer);
                            peek  = PeekNextToken(state.lexer, token);
                        }
                        
                        if (token.kind != Token_Identifier || peek.kind != Token_Colon)
                        {
                            if (peek.kind != Token_Colon)
                            {
                                //// ERROR: Missing colon after name of structure parameter
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                //// ERROR: Missing name of structure parameter
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            parameter->name = token.string;
                            
                            SkipPastToken(state.lexer, peek);
                            
                            if (!ParseExpression(state, &parameter->type)) encountered_errors = true;
                            else
                            {
                                token = GetToken(state.lexer);
                                
                                if (token.kind == Token_Equals)
                                {
                                    if (!ParseExpression(state, &parameter->value))
                                    {
                                        encountered_errors = true;
                                    }
                                }
                                
                                token = GetToken(state.lexer);
                                if (!encountered_errors)
                                {
                                    if (token.kind == Token_Comma) SkipPastToken(state.lexer, token);
                                    else break;
                                }
                            }
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_CloseParen) SkipPastToken(state.lexer, token);
                        else
                        {
                            //// ERROR: Missing matching closing parenthesis after structure parameters
                            encountered_errors = true;
                        }
                    }
                }
            }
            
            if (!encountered_errors)
            {
                token = GetToken(state.lexer);
                
                if (token.kind != Token_OpenBrace)
                {
                    //// ERROR: Missing struct body
                    encountered_errors = true;
                }
                
                else
                {
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (token.kind != Token_CloseBrace)
                    {
                        while (!encountered_errors)
                        {
                            Structure_Member* member = Array_Append(&(*expr)->structure.members);
                            
                            token      = GetToken(state.lexer);
                            Token peek = PeekNextToken(state.lexer, token);
                            
                            if (token.kind == Token_Identifier &&
                                StringCompare(token.string, KeywordStrings[Keyword_Using]))
                            {
                                SkipPastToken(state.lexer, token);
                                
                                member->is_using = true;
                                
                                token = GetToken(state.lexer);
                                peek  = PeekNextToken(state.lexer, token);
                            }
                            
                            if (token.kind != Token_Identifier || peek.kind != Token_Colon)
                            {
                                if (peek.kind != Token_Colon)
                                {
                                    //// ERROR: Missing colon after name of structure member
                                    encountered_errors = true;
                                }
                                
                                else
                                {
                                    //// ERROR: Missing name of structure member
                                    encountered_errors = true;
                                }
                            }
                            
                            else
                            {
                                member->name = token.string;
                                
                                SkipPastToken(state.lexer, peek);
                                
                                if (!ParseExpression(state, &member->type)) encountered_errors = true;
                                else
                                {
                                    token = GetToken(state.lexer);
                                    
                                    if (token.kind == Token_Comma) SkipPastToken(state.lexer, token);
                                    
                                    else if (token.kind == Token_Semicolon)
                                    {
                                        //// ERROR: Invalid use of semicolon in structure body. Structure members are
                                        ////        delimited by commas
                                        encountered_errors = true;
                                    }
                                    
                                    else if (token.kind == Token_Equals)
                                    {
                                        //// ERROR: Invalid use of equals sign in struct bodyure. Structure members
                                        ////        cannot be assigned a default value
                                        encountered_errors = true;
                                    }
                                    
                                    else break;
                                }
                            }
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_CloseBrace) SkipPastToken(state.lexer, token);
                        else
                        {
                            //// ERROR: Missing matching closing brace after structure body
                            encountered_errors = true;
                        }
                    }
                }
            }
        }
        
        else if (StringCompare(token.string, KeywordStrings[Keyword_Enum]))
        {
            *expr = AppendExpression(state, Expression_Enum);
            
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            if (token.kind != Token_OpenBrace)
            {
                if (!ParseExpression(state, &(*expr)->enumeration.type))
                {
                    encountered_errors = true;
                }
            }
            
            if (!encountered_errors)
            {
                token = GetToken(state.lexer);
                
                if (token.kind == Token_OpenBrace) SkipPastToken(state.lexer, token);
                else
                {
                    //// ERROR: Missing body of enum
                    encountered_errors = true;
                }
                
                token = GetToken(state.lexer);
                if (!encountered_errors && token.kind != Token_CloseBrace)
                {
                    while (!encountered_errors)
                    {
                        Enumeration_Member* member = Array_Append(&(*expr)->enumeration.members);
                        
                        token = GetToken(state.lexer);
                        
                        if (token.kind != Token_Identifier)
                        {
                            //// ERROR: Missing name of enumeration member
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            member->name = token.string;
                            
                            SkipPastToken(state.lexer, token);
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_Equals)
                            {
                                SkipPastToken(state.lexer, token);
                                
                                if (!ParseExpression(state, &member->value))
                                {
                                    encountered_errors = true;
                                }
                            }
                            
                            if (!encountered_errors)
                            {
                                token = GetToken(state.lexer);
                                
                                if (token.kind == Token_Comma) SkipPastToken(state.lexer, token);
                                else break;
                            }
                        }
                    }
                }
                
                if (!encountered_errors)
                {
                    token = GetToken(state.lexer);
                    
                    if (token.kind == Token_CloseBrace) SkipPastToken(state.lexer, token);
                    else
                    {
                        //// ERROR: Missing matching closing brace after enumeration body
                        encountered_errors = true;
                    }
                }
            }
        }
        
        else if (StringCompare(token.string, KeywordStrings[Keyword_True]) ||
                 StringCompare(token.string, KeywordStrings[Keyword_False]))
        {
            *expr = AppendExpression(state, Expression_Boolean);
            (*expr)->boolean = StringCompare(token.string, KeywordStrings[Keyword_True]);
            
            SkipPastToken(state.lexer, token);
        }
        
        else
        {
            *expr = AppendExpression(state, Expression_Identifier);
            (*expr)->string = token.string;
            
            SkipPastToken(state.lexer, token);
        }
    }
    
    else if (token.kind == Token_Number)
    {
        *expr = AppendExpression(state, Expression_Number);
        (*expr)->number = token.number;
        
        SkipPastToken(state.lexer, token);
    }
    
    else if (token.kind == Token_String)
    {
        *expr = AppendExpression(state, Expression_String);
        (*expr)->string = token.string;
        
        SkipPastToken(state.lexer, token);
    }
    
    else if (token.kind == Token_Char)
    {
        *expr = AppendExpression(state, Expression_Character);
        (*expr)->character = token.character;
        
        SkipPastToken(state.lexer, token);
    }
    
    else
    {
        //// ERROR: Missing primary expression
        encountered_errors = true;
    }
    
    if (!encountered_errors)
    {
        if (!ParsePostfixUnaryExpression(state, expr))
        {
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

bool
ParsePrefixUnaryExpression(Parser_State state, Expression** expr)
{
    bool encountered_errors = false;
    
    Expression** current = expr;
    
    while (!encountered_errors)
    {
        Token token = GetToken(state.lexer);
        
        if (token.kind == Token_Plus)
        {
            // NOTE(soimn): Ignore unary plus
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            if (token.kind == Token_Plus)
            {
                SkipPastToken(state.lexer, token);
                
                //// WARNING: There are no increment or decrement operators in this language
            }
        }
        
        else if (token.kind == Token_Minus)
        {
            SkipPastToken(state.lexer, token);
            
            if (token.kind != Token_Minus)
            {
                *current = AppendExpression(state, Expression_Neg);
                current  = &(*current)->operand;
            }
            
            else
            {
                SkipPastToken(state.lexer, token);
                
                //// WARNING: There are no increment or decrement operators in this language
            }
        }
        
        else if (token.kind == Token_Period)
        {
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            if (token.kind != Token_Period)
            {
                //// ERROR: Missing left side of member access expression
                encountered_errors = true;
            }
            
            else
            {
                SkipPastToken(state.lexer, token);
                
                *current = AppendExpression(state, Expression_ArrayExpand);
                current  = &(*current)->operand;
            }
        }
        
        else if (token.kind == Token_Ampersand)
        {
            SkipPastToken(state.lexer, token);
            
            *current = AppendExpression(state, Expression_Reference);
            current  = &(*current)->operand;
        }
        
        else if (token.kind == Token_Asterisk)
        {
            SkipPastToken(state.lexer, token);
            
            *current = AppendExpression(state, Expression_Dereference);
            current  = &(*current)->operand;
        }
        
        else if (token.kind == Token_Tilde)
        {
            SkipPastToken(state.lexer, token);
            
            *current = AppendExpression(state, Expression_BitNot);
            current  = &(*current)->operand;
        }
        
        else if (token.kind == Token_Exclamation)
        {
            SkipPastToken(state.lexer, token);
            
            *current = AppendExpression(state, Expression_LogicalNot);
            current  = &(*current)->operand;
        }
        
        else if (token.kind == Token_Hat)
        {
            SkipPastToken(state.lexer, token);
            
            *current = AppendExpression(state, Expression_PointerType);
            current  = &(*current)->operand;
        }
        
        else if (token.kind == Token_OpenBracket)
        {
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            *current = AppendExpression(state, Expression_ArrayType);
            
            if (token.kind == Token_Period)
            {
                SkipPastToken(state.lexer, token);
                token      = GetToken(state.lexer);
                Token peek = PeekNextToken(state.lexer, token);
                
                if (token.kind != Token_Period || peek.kind != Token_CloseBracket)
                {
                    if (peek.kind != Token_Period)
                    {
                        //// ERROR: Missing left side of memeber access expression
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        //// ERROR: Missing matching closing bracket in dynamic array type
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    SkipPastToken(state.lexer, peek);
                    
                    (*current)->array_type.is_dynamic = true;
                }
            }
            
            else
            {
                if (token.kind == Token_CloseBracket) SkipPastToken(state.lexer, token);
                else
                {
                    if (!ParseExpression(state, &(*current)->array_type.size)) encountered_errors = true;
                    else
                    {
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_CloseBracket) SkipPastToken(state.lexer, token);
                        else
                        {
                            //// ERROR: Missing matching closing bracket after size in array type decorator
                            encountered_errors = true;
                        }
                    }
                }
            }
            
            current = &(*current)->array_type.elem_type;
        }
        
        else
        {
            if (!ParsePrimaryExpression(state, current))
            {
                encountered_errors = true;
            }
            
            break;
        }
    }
    
    return !encountered_errors;
}

bool
ParseMulLevelExpression(Parser_State state, Expression** expr)
{
    bool encountered_errors = false;
    
    if (!ParsePrefixUnaryExpression(state, expr)) encountered_errors = true;
    else
    {
        Token token = GetToken(state.lexer);
        Token peek  = PeekNextToken(state.lexer, token);
        
        Enum32(EXPRESSION_KIND) kind = Expression_Invalid;
        
        if (peek.kind != Token_Equals)
        {
            if      (token.kind == Token_Asterisk)   kind = Expression_Mul;
            else if (token.kind == Token_Slash)      kind = Expression_Div;
            else if (token.kind == Token_Percentage) kind = Expression_Mod;
            else if (token.kind == Token_Ampersand)  kind = Expression_BitAnd;
            
            else if (token.kind == Token_Greater && peek.kind == Token_Greater &&
                     PeekNextToken(state.lexer, peek).kind != Token_Equals)
            {
                kind = Expression_BitLShift;
            }
            
            else if (token.kind == Token_Less && peek.kind == Token_Less &&
                     PeekNextToken(state.lexer, peek).kind != Token_Equals)
            {
                kind = Expression_BitRShift;
            }
        }
        
        if (kind != Expression_Invalid)
        {
            if (kind == Expression_BitLShift || kind == Expression_BitRShift) SkipPastToken(state.lexer, peek);
            else                                                              SkipPastToken(state.lexer, token);
            
            Expression* left = *expr;
            
            *expr = AppendExpression(state, kind);
            (*expr)->left = left;
            
            if (!ParseMulLevelExpression(state, &(*expr)->right))
            {
                encountered_errors = true;
            }
        }
    }
    
    return !encountered_errors;
}

bool
ParseAddLevelExpression(Parser_State state, Expression** expr)
{
    bool encountered_errors = false;
    
    if (!ParseMulLevelExpression(state, expr)) encountered_errors = true;
    else
    {
        Token token = GetToken(state.lexer);
        Token peek  = PeekNextToken(state.lexer, token);
        
        Enum32(EXPRESSION_KIND) kind = Expression_Invalid;
        
        if (peek.kind != Token_Equals)
        {
            if      (token.kind == Token_Plus)  kind = Expression_Add;
            else if (token.kind == Token_Minus) kind = Expression_Sub;
            else if (token.kind == Token_Pipe)  kind = Expression_BitOr;
        }
        
        if (kind != Expression_Invalid)
        {
            SkipPastToken(state.lexer, token);
            
            Expression* left = *expr;
            
            *expr = AppendExpression(state, kind);
            (*expr)->left = left;
            
            if (!ParseAddLevelExpression(state, &(*expr)->right))
            {
                encountered_errors = true;
            }
        }
    }
    
    return !encountered_errors;
}

bool
ParseLogicalAndExpression(Parser_State state, Expression** expr)
{
    bool encountered_errors = false;
    
    if (!ParseAddLevelExpression(state, expr)) encountered_errors = true;
    else
    {
        Token token = GetToken(state.lexer);
        Token peek  = PeekNextToken(state.lexer, token);
        
        if (token.kind == Token_Ampersand && peek.kind == Token_Ampersand &&
            PeekNextToken(state.lexer, peek).kind != Token_Equals)
        {
            SkipPastToken(state.lexer, peek);
            
            Expression* left = *expr;
            
            *expr = AppendExpression(state, Expression_LogicalAnd);
            (*expr)->left = left;
            
            if (!ParseLogicalAndExpression(state, &(*expr)->right))
            {
                encountered_errors = true;
            }
        }
    }
    
    return !encountered_errors;
}

bool
ParseLogicalOrExpression(Parser_State state, Expression** expr)
{
    bool encountered_errors = false;
    
    if (!ParseLogicalAndExpression(state, expr)) encountered_errors = true;
    else
    {
        Token token = GetToken(state.lexer);
        Token peek  = PeekNextToken(state.lexer, token);
        
        if (token.kind == Token_Pipe && peek.kind == Token_Pipe &&
            PeekNextToken(state.lexer, peek).kind != Token_Equals)
        {
            SkipPastToken(state.lexer, peek);
            
            Expression* left = *expr;
            
            *expr = AppendExpression(state, Expression_LogicalOr);
            (*expr)->left = left;
            
            if (!ParseLogicalOrExpression(state, &(*expr)->right))
            {
                encountered_errors = true;
            }
        }
    }
    
    return !encountered_errors;
}

bool
ParseComparision(Parser_State state, Expression** expr)
{
    bool encountered_errors = false;
    
    if (!ParseLogicalOrExpression(state, expr)) encountered_errors = true;
    else
    {
        Token token = GetToken(state.lexer);
        Token peek  = PeekNextToken(state.lexer, token);
        
        Enum32(EXPRESSION_KIND) kind = Expression_Invalid;
        
        if (peek.kind == Token_Equals)
        {
            if      (token.kind == Token_Equals)      kind = Expression_CmpEqual;
            else if (token.kind == Token_Exclamation) kind = Expression_CmpNotEqual;
            else if (token.kind == Token_Greater)     kind = Expression_CmpGreaterOrEqual;
            else if (token.kind == Token_Less)        kind = Expression_CmpLessOrEqual;
        }
        
        else
        {
            if      (token.kind == Token_Greater) kind = Expression_CmpGreater;
            else if (token.kind == Token_Less)    kind = Expression_CmpLess;
        }
        
        if (kind != Expression_Invalid)
        {
            if (kind == Expression_CmpGreater || kind == Expression_CmpLess) SkipPastToken(state.lexer, token);
            else                                                             SkipPastToken(state.lexer, peek);
            
            Expression* left = *expr;
            
            *expr = AppendExpression(state, kind);
            (*expr)->left = left;
            
            if (!ParseLogicalOrExpression(state, &(*expr)->right))
            {
                encountered_errors = true;
            }
        }
    }
    
    return !encountered_errors;
}

bool
ParseTernary(Parser_State state, Expression** expr)
{
    bool encountered_errors = false;
    
    if (!ParseComparision(state, expr)) encountered_errors = true;
    else
    {
        Token token = GetToken(state.lexer);
        
        if (token.kind == Token_QuestionMark)
        {
            Expression* condition = *expr;
            
            *expr = AppendExpression(state, Expression_Ternary);
            (*expr)->ternary.condition = condition;
            
            SkipPastToken(state.lexer, token);
            
            if (!ParseExpression(state, &(*expr)->ternary.true_expr)) encountered_errors = true;
            else
            {
                token = GetToken(state.lexer);
                
                if (token.kind != Token_Colon)
                {
                    //// ERROR: Missing else clause in ternary
                    encountered_errors = true;
                }
                
                else
                {
                    if (!ParseExpression(state, &(*expr)->ternary.false_expr))
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
ParseExpression(Parser_State state, Expression** expr)
{
    return ParseTernary(state, expr);
}

bool
ParseDeclaration(Parser_State state, Declaration* declaration)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    
    state.is_global_scope = false;
    declaration->is_exported = state.export_by_default;
    
    if (token.kind == Token_At)
    {
        SkipPastToken(state.lexer, token);
        token = GetToken(state.lexer);
        
        bool is_array = false;
        if (token.kind == Token_OpenBracket)
        {
            is_array = true;
            
            SkipPastToken(state.lexer, token);
        }
        
        while (!encountered_errors)
        {
            Attribute* attribute = Array_Append(&declaration->attributes);
            attribute->arguments = ARRAY_INIT(declaration->attributes.arena, Attribute_Argument, 3);
            
            token = GetToken(state.lexer);
            
            if (token.kind != Token_Identifier)
            {
                //// ERROR: Missing name of attribute
                encountered_errors = true;
            }
            
            else
            {
                attribute->name = token.string;
                
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                if (token.kind == Token_OpenParen)
                {
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (token.kind != Token_CloseParen)
                    {
                        while (!encountered_errors)
                        {
                            Attribute_Argument* argument = Array_Append(&attribute->arguments);
                            
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_Number)
                            {
                                argument->is_number = true;
                                argument->is_string = false;
                                argument->number    = token.number;
                                
                                SkipPastToken(state.lexer, token);
                            }
                            
                            else if (token.kind == Token_String)
                            {
                                argument->is_number = false;
                                argument->is_string = true;
                                argument->string    = token.string;
                                
                                SkipPastToken(state.lexer, token);
                            }
                            
                            else if (token.kind == Token_Identifier && 
                                     (StringCompare(token.string, KeywordStrings[Keyword_True]) ||
                                      StringCompare(token.string, KeywordStrings[Keyword_False])))
                            {
                                argument->is_number = false;
                                argument->is_string = false;
                                argument->boolean   = StringCompare(token.string, KeywordStrings[Keyword_True]);
                                
                                SkipPastToken(state.lexer, token);
                            }
                            
                            else
                            {
                                //// ERROR: Invalid attribute argument. Only strings, numbers and 
                                ////        boolean literals are legal as attribute arguments
                                encountered_errors = true;
                            }
                            
                            if (!encountered_errors)
                            {
                                token = GetToken(state.lexer);
                                
                                if (token.kind == Token_Comma) SkipPastToken(state.lexer, token);
                                else break;
                            }
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_CloseParen) SkipPastToken(state.lexer, token);
                        else
                        {
                            //// ERROR: Missing matching closing parenthesis after attribute arguments
                            
                            encountered_errors = true;
                        }
                    }
                }
                
                token = GetToken(state.lexer);
                
                if (token.kind != Token_Comma) break;
                else
                {
                    if (is_array) SkipPastToken(state.lexer, token);
                    else
                    {
                        //// ERROR: Expected declaration after single attribute but encountered comma.
                        ////        Use bracketed lists for multiple attributes
                        encountered_errors = true;
                    }
                }
            }
        }
        
        token = GetToken(state.lexer);
        
        if (is_array)
        {
            if (token.kind == Token_CloseBracket) SkipPastToken(state.lexer, token);
            else
            {
                //// ERROR: Missing matching closing bracket after attribute list
                encountered_errors = true;
            }
        }
        
        else if (token.kind == Token_CloseBracket)
        {
            //// ERROR: Expected declaration after single attribute but encountered closing bracket,
            ////        did you forget an open bracket after the @ sign?
            encountered_errors = true;
        }
    }
    
    while (!encountered_errors)
    {
        token = GetToken(state.lexer);
        
        if (token.kind != Token_Identifier)
        {
            //// ERROR: Missing name of declaration
            encountered_errors = true;
        }
        
        else
        {
            String* name = Array_Append(&declaration->names);
            *name = token.string;
            
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            if (token.kind == Token_Comma) SkipPastToken(state.lexer, token);
            else break;
        }
    }
    
    if (!encountered_errors)
    {
        token = GetToken(state.lexer);
        
        if (token.kind != Token_Colon)
        {
            //// ERROR: Invalid use of attributes. Attributes can only be applied to declarations
            encountered_errors = true;
        }
        
        else
        {
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            if (token.kind != Token_Colon && token.kind != Token_Equals)
            {
                while (!encountered_errors)
                {
                    Expression** type = Array_Append(&declaration->types);
                    
                    if (!ParseExpression(state, type)) encountered_errors = true;
                    else
                    {
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_Comma) SkipPastToken(state.lexer, token);
                        else break;
                    }
                }
            }
            
            if (!encountered_errors)
            {
                token = GetToken(state.lexer);
                
                if (token.kind == Token_Colon)
                {
                    SkipPastToken(state.lexer, token);
                    token      = GetToken(state.lexer);
                    Token peek = PeekNextToken(state.lexer, token);
                    
                    bool is_distinct = false;
                    if (token.kind == Token_Hash && peek.kind == Token_Identifier &&
                        StringCompare(peek.string, DirectiveStrings[Directive_Distinct]))
                    {
                        SkipPastToken(state.lexer, peek);
                        is_distinct = true;
                    }
                    
                    Expression* tmp_expr;
                    if (!ParseExpression(state, &tmp_expr)) encountered_errors = true;
                    else
                    {
                        token = GetToken(state.lexer);
                        
                        // NOTE(soimn): Procs, and struct and enums that are not part of a list, are handled 
                        //              separately
                        if (token.kind != Token_Comma && (tmp_expr->kind == Expression_Proc   ||
                                                          tmp_expr->kind == Expression_Struct ||
                                                          tmp_expr->kind == Expression_Enum))
                        {
                            if (tmp_expr->kind == Expression_Proc)
                            {
                                if (is_distinct)
                                {
                                    //// ERROR: The distinct directive cannot be applied to procedures
                                    encountered_errors = true;
                                }
                                
                                else
                                {
                                    declaration->kind      = Declaration_Proc;
                                    declaration->procedure = tmp_expr->procedure;
                                }
                            }
                            
                            else if (tmp_expr->kind == Expression_Struct)
                            {
                                if (is_distinct)
                                {
                                    //// WARNING: Redundant use of the distinct directive, as structures are implicitly
                                    ////          distinct
                                }
                                
                                declaration->kind      = Declaration_Struct;
                                declaration->structure = tmp_expr->structure;
                            }
                            
                            else
                            {
                                if (is_distinct)
                                {
                                    //// WARNING: Redundant use of the distinct directive, as enumerations are 
                                    ////          implicitly distinct
                                }
                                
                                declaration->kind        = Declaration_Enum;
                                declaration->enumeration = tmp_expr->enumeration;
                            }
                            
                            // HACK(soimn):
                            BucketArray_Free(&state.package->expressions, tmp_expr);
                            
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                            else
                            {
                                // NOTE(soimn): Do nothing. Semicolons are not required after procs, structs and 
                                //              enums that are not part of a list
                            }
                        }
                        
                        else
                        {
                            declaration->kind            = Declaration_Const;
                            declaration->constant.values = ARRAY_INIT(declaration->types.arena, Expression*, 2);
                            
                            declaration->constant.is_distinct = is_distinct;
                            
                            *(Expression**)Array_Append(&declaration->constant.values) = tmp_expr;
                            
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_Comma)
                            {
                                SkipPastToken(state.lexer, token);
                                
                                while (!encountered_errors)
                                {
                                    Expression** expr = Array_Append(&declaration->constant.values);
                                    
                                    if (!ParseExpression(state, expr)) encountered_errors = true;
                                    else
                                    {
                                        token = GetToken(state.lexer);
                                        
                                        if (token.kind == Token_Comma) SkipPastToken(state.lexer, token);
                                        else break;
                                    }
                                }
                            }
                            
                            if (!encountered_errors)
                            {
                                token = GetToken(state.lexer);
                                
                                if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                                else
                                {
                                    //// ERROR: Missing semicolon after constant declaration
                                    encountered_errors = true;
                                }
                            }
                        }
                    }
                }
                
                else
                {
                    declaration->kind       = Declaration_Var;
                    declaration->var.values = ARRAY_INIT(declaration->types.arena, Expression*, 3);
                    
                    if (token.kind == Token_Equals)
                    {
                        SkipPastToken(state.lexer, token);
                        
                        token        = GetToken(state.lexer);
                        Token peek   = PeekNextToken(state.lexer, token);
                        Token peek_1 = PeekNextToken(state.lexer, peek);
                        
                        
                        if (token.kind == Token_Minus && peek.kind == Token_Minus &&
                            peek_1.kind == Token_Minus)
                        {
                            SkipPastToken(state.lexer, peek_1);
                            
                            declaration->var.is_uninitialized = true;
                        }
                        
                        else
                        {
                            while (!encountered_errors)
                            {
                                Expression** expr = Array_Append(&declaration->var.values);
                                
                                if (!ParseExpression(state, expr)) encountered_errors = true;
                                else
                                {
                                    token = GetToken(state.lexer);
                                    
                                    if (token.kind == Token_Comma) SkipPastToken(state.lexer, token);
                                    else break;
                                }
                            }
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                        else
                        {
                            //// ERROR: Missing semicolon after variable declaration
                            encountered_errors = true;
                        }
                    }
                }
            }
        }
    }
    
    return !encountered_errors;
}

bool
ResolvePathString(Parser_State state,  Token token, String* path)
{
    bool encountered_errors = false;
    
    String prefix        = {0};
    String relative_path = token.string;
    
    for (UMM i = 0; i < relative_path.size; ++i)
    {
        if (relative_path.data[i] == ':')
        {
            String mounting_point_name = {
                .data = relative_path.data,
                .size = i
            };
            
            relative_path.data += i + 1;
            relative_path.size -= i + 1;
            
            for (Bucket_Array_Iterator it = BucketArray_Iterate(&state.workspace->mounting_points);
                 it.current != 0;
                 BucketArrayIterator_Advance(&it))
            {
                Mounting_Point* mp = it.current;
                
                if (StringCompare(mounting_point_name, mp->name))
                {
                    prefix = mp->path;
                    break;
                }
            }
            
            if (prefix.data == 0)
            {
                //// ERROR: No mounting point found with the name ____
                encountered_errors = true;
            }
            
            break;
        }
    }
    
    if (!encountered_errors)
    {
        if (prefix.data == 0)
        {
            // NOTE(soimn): Set prefix to the default mounting point if no other was specified
            prefix = ((Mounting_Point*)BucketArray_ElementAt(&state.workspace->mounting_points, 0))->path;
        }
        
        path->size = prefix.size + relative_path.size;
        path->data = Arena_AllocateSize(&state.package->arena, path->size, ALIGNOF(U8));
        
        Copy(prefix.data, path->data, prefix.size);
        Copy(relative_path.data, path->data + prefix.size, relative_path.size);
    }
    
    return !encountered_errors;
}

bool
ParseStatement(Parser_State state)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    
    if (token.kind == Token_Hash)
    {
        SkipPastToken(state.lexer, token);
        token = GetToken(state.lexer);
        
        if (token.kind != Token_Identifier)
        {
            //// ERROR: Missing name of directive
            encountered_errors = true;
        }
        
        else
        {
            if (StringCompare(token.string, DirectiveStrings[Directive_If]))
            {
                Statement* prev_statement = 0;
                while (!encountered_errors)
                {
                    Statement* statement = AppendStatement(state, Statement_If);
                    statement->if_statement.is_const = true;
                    
                    if (prev_statement)
                    {
                        prev_statement->if_statement.false_body.first_statement = statement;
                        prev_statement->if_statement.false_body.last_statement  = statement;
                    }
                    
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (token.kind != Token_OpenParen)
                    {
                        //// ERROR: Missing open parenthesis before condition in if statement
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        SkipPastToken(state.lexer, token);
                        
                        if (!ParseExpression(state, &statement->if_statement.condition))
                        {
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            token = GetToken(state.lexer);
                            
                            if (token.kind != Token_CloseParen)
                            {
                                //// ERROR: Missing parenthesis after condition in if statement
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                if (!ParseScope(state, &statement->if_statement.true_body, true))
                                {
                                    encountered_errors = true;
                                }
                                
                                else
                                {
                                    token = GetToken(state.lexer);
                                    
                                    if (token.kind == Token_Identifier &&
                                        StringCompare(token.string, KeywordStrings[Keyword_Else]))
                                    {
                                        SkipPastToken(state.lexer, token);
                                        token = GetToken(state.lexer);
                                        
                                        if (token.kind == Token_Identifier &&
                                            StringCompare(token.string, KeywordStrings[Keyword_If]))
                                        {
                                            prev_statement = statement;
                                            continue;
                                        }
                                        
                                        else
                                        {
                                            if (!ParseScope(state, &statement->if_statement.false_body, true))
                                            {
                                                encountered_errors = true;
                                            }
                                            
                                            break;
                                        }
                                    }
                                    
                                    else break;
                                }
                            }
                        }
                    }
                }
            }
            
            else if (StringCompare(token.string, DirectiveStrings[Directive_ScopeExport]))
            {
                if (state.is_global_scope)
                {
                    SkipPastToken(state.lexer, token);
                    
                    state.export_by_default = true;
                }
                
                else
                {
                    //// ERROR: Illegal use of scope directive outside global scope
                    encountered_errors = true;
                }
            }
            
            else if (StringCompare(token.string, DirectiveStrings[Directive_ScopeFile]))
            {
                if (state.is_global_scope)
                {
                    SkipPastToken(state.lexer, token);
                    
                    state.export_by_default = false;
                }
                
                else
                {
                    //// ERROR: Illegal use of scope directive outside global scope
                    encountered_errors = true;
                }
            }
            
            else if (StringCompare(token.string, DirectiveStrings[Directive_BoundsCheck]) ||
                     StringCompare(token.string, DirectiveStrings[Directive_NoBoundsCheck]))
            {
                state.should_bounds_check = StringCompare(token.string, DirectiveStrings[Directive_BoundsCheck]);
                
                SkipPastToken(state.lexer, token);
                
                if (!ParseStatement(state))
                {
                    encountered_errors = true;
                }
            }
            
            else if (StringCompare(token.string, DirectiveStrings[Directive_Assert]))
            {
                Statement* statement = AppendStatement(state, Statement_ConstAssert);
                
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                if (token.kind != Token_OpenParen)
                {
                    //// ERROR: Missing open parenthesis before condition in constant assertion
                    encountered_errors = true;
                }
                
                else
                {
                    SkipPastToken(state.lexer, token);
                    
                    if (!ParseExpression(state, &statement->const_assert_statement.condition))
                    {
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_Comma)
                        {
                            SkipPastToken(state.lexer, token);
                            token = GetToken(state.lexer);
                            
                            if (token.kind != Token_String)
                            {
                                //// ERROR: Expected assertion message after condition in constant assertion
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                statement->const_assert_statement.message = token.string;
                                
                                SkipPastToken(state.lexer, token);
                                token = GetToken(state.lexer);
                            }
                        }
                        
                        token = GetToken(state.lexer);
                        if (token.kind != Token_CloseParen)
                        {
                            //// ERROR: Missing matching closing parenthesis in constant assertion
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                            else
                            {
                                //// ERROR: Missing terminating semicolon after constant assertion
                                encountered_errors = true;
                            }
                        }
                    }
                }
            }
            
            else if (StringCompare(token.string, DirectiveStrings[Directive_Run]))
            {
                SkipPastToken(state.lexer, token);
                
                Statement* statement = AppendStatement(state, Statement_Run);
                
                if (!ParseScope(state, &statement->run_statement.scope, true))
                {
                    encountered_errors = true;
                }
            }
            
            else
            {
                uint i = 0;
                for (; i < DIRECTIVE_COUNT; ++i)
                {
                    if (StringCompare(token.string, DirectiveStrings[i])) break;
                }
                
                if (i < DIRECTIVE_COUNT)
                {
                    //// ERROR: Invalid use of diretive at expression level
                    encountered_errors = true;
                }
                
                else
                {
                    //// ERROR: Encountered an unknown directive
                    encountered_errors = true;
                }
            }
        }
    }
    
    // NOTE(soimn): This is not a directive
    else
    {
        if (token.kind == Token_Error)
        {
            encountered_errors = true;
        }
        
        else if (token.kind == Token_Unknown)
        {
            //// ERROR: Encountered an unknown token. Expected a statement
            encountered_errors = true;
        }
        
        else if (token.kind == Token_OpenBrace)
        {
            if (state.is_global_scope)
            {
                //// ERROR: Illegal use of subscope in global scope
                encountered_errors = true;
            }
            
            else
            {
                Statement* statement = AppendStatement(state, Statement_Scope);
                
                if (!ParseScope(state, &statement->scope, false))
                {
                    encountered_errors = true;
                }
            }
        }
        
        else if (token.kind == Token_Semicolon)
        {
            if (!state.is_global_scope) SkipPastToken(state.lexer, token);
            else
            {
                //// ERROR: Illegal use of statement in global scope
                encountered_errors = true;
            }
        }
        
        if (token.kind == Token_Identifier)
        {
            if (StringCompare(token.string, KeywordStrings[Keyword_Import]))
            {
                if (!state.is_global_scope)
                {
                    //// ERROR: Invalid use of import statement. Import statements are only valid in global scope
                    encountered_errors = true;
                }
                
                else
                {
                    Statement* statement = AppendStatement(state, Statement_Import);
                    statement->import.is_exported = state.export_by_default;
                    
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (token.kind != Token_String)
                    {
                        //// ERROR: Missing import path string after import keyword in import statement
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        String package_path;
                        if (!ResolvePathString(state, token, &package_path)) encountered_errors = true;
                        else
                        {
                            statement->import.package_id = Workspace_AppendPackage(state.workspace, package_path);
                            
                            SkipPastToken(state.lexer, token);
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_Identifier && 
                                StringCompare(token.string, KeywordStrings[Keyword_As]))
                            {
                                SkipPastToken(state.lexer, token);
                                token = GetToken(state.lexer);
                                
                                if (token.kind != Token_Identifier)
                                {
                                    //// ERROR: Missing alias after as keyword in import statement
                                    encountered_errors = true;
                                }
                                
                                else
                                {
                                    statement->import.alias = token.string;
                                    
                                    SkipPastToken(state.lexer, token);
                                }
                            }
                            
                            if (!encountered_errors)
                            {
                                token = GetToken(state.lexer);
                                
                                if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                                else
                                {
                                    //// ERROR: Missing semicolon after import statement
                                    encountered_errors = true;
                                }
                            }
                        }
                    }
                }
            }
            
            else if (StringCompare(token.string, KeywordStrings[Keyword_Foreign]))
            {
                if (!state.is_global_scope)
                {
                    //// ERROR: Invalid use of foreign import  statement. Foreign import statements are only valid in
                    ////        global scope
                    encountered_errors = true;
                }
                
                else
                {
                    Statement* statement = AppendStatement(state, Statement_ForeignImport);
                    statement->import.is_exported = state.export_by_default;
                    
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (token.kind != Token_Identifier ||
                        !StringCompare(token.string, KeywordStrings[Keyword_Import]))
                    {
                        //// ERROR: Missing import keyword after foreign in foreign import statement
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        SkipPastToken(state.lexer, token);
                        token = GetToken(state.lexer);
                        
                        if (token.kind != Token_String)
                        {
                            //// ERROR: Missing import path in foreign import statement
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            if (!ResolvePathString(state, token, &statement->foreign_import.path))
                            {
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                token = GetToken(state.lexer);
                                
                                if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                                
                                else if (token.kind == Token_Identifier &&
                                         StringCompare(token.string, KeywordStrings[Keyword_As]))
                                {
                                    //// ERROR: Invalid use of as keyword. Only import statements are allowed to
                                    ////        have an alias
                                    encountered_errors = true;
                                }
                                
                                else
                                {
                                    //// ERROR: Missing semicolon after foreign import statement
                                    encountered_errors = true;
                                }
                            }
                        }
                    }
                }
            }
            
            else if (StringCompare(token.string, KeywordStrings[Keyword_Load]))
            {
                if (!state.is_global_scope)
                {
                    //// ERROR: Invalid use of load statement. Load statements are only valid in global scope
                    encountered_errors = true;
                }
                
                else
                {
                    Statement* statement = AppendStatement(state, Statement_Load);
                    statement->load.is_exported = state.export_by_default;
                    
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (token.kind != Token_String)
                    {
                        //// ERROR: Missing load path after load keyword in load statement
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        String path_string;
                        if (!ResolvePathString(state, token, &path_string)) encountered_errors = true;
                        else
                        {
                            SkipPastToken(state.lexer, token);
                            token = GetToken(state.lexer);
                            
                            statement->load.file_id = BucketArray_ElementCount(&state.package->loaded_files);
                            
                            Source_File* file = BucketArray_Append(&state.package->loaded_files);
                            file->path = path_string;
                            
                            if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                            
                            else if (token.kind == Token_Identifier &&
                                     StringCompare(token.string, KeywordStrings[Keyword_As]))
                            {
                                //// ERROR: Illegal use of as keyword in load statement. Only import statements can
                                ////        be aliased
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                //// ERROR: Missing semicolon after load statement
                                encountered_errors = true;
                            }
                        }
                    }
                }
            }
            
            else if (StringCompare(token.string, KeywordStrings[Keyword_If]))
            {
                if (state.is_global_scope)
                {
                    //// ERROR: Illegal use of if statement in global scope
                    encountered_errors = true;
                }
                
                else
                {
                    Statement* statement = AppendStatement(state, Statement_If);
                    
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (token.kind != Token_OpenParen)
                    {
                        //// ERROR: Missing open parenthesis before condition in if statement
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        SkipPastToken(state.lexer, token);
                        token = GetToken(state.lexer);
                        
                        if (!ParseExpression(state, &statement->if_statement.condition))
                        {
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            token = GetToken(state.lexer);
                            
                            if (token.kind != Token_CloseParen)
                            {
                                //// ERROR: Missing closing parenthesis after condition in if statement
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                SkipPastToken(state.lexer, token);
                                
                                if (!ParseScope(state, &statement->if_statement.true_body, true))
                                {
                                    encountered_errors = true;
                                }
                                
                                else
                                {
                                    token = GetToken(state.lexer);
                                    
                                    if (token.kind == Token_Identifier &&
                                        StringCompare(token.string, KeywordStrings[Keyword_Else]))
                                    {
                                        SkipPastToken(state.lexer, token);
                                        
                                        if (!ParseScope(state, &statement->if_statement.false_body, true))
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
            
            else if (StringCompare(token.string, KeywordStrings[Keyword_Else]))
            {
                //// ERROR: Illegal else without matching if
                encountered_errors = true;
            }
            
            else if (StringCompare(token.string, KeywordStrings[Keyword_Defer]))
            {
                if (state.is_global_scope)
                {
                    //// ERROR: Illegal use of defer statement in global scope
                    encountered_errors = true;
                }
                
                else
                {
                    Statement* statement = AppendStatement(state, Statement_Defer);
                    
                    SkipPastToken(state.lexer, token);
                    
                    if (!ParseScope(state, &statement->defer_statement.body, true))
                    {
                        encountered_errors = true;
                    }
                }
            }
            
            else if (StringCompare(token.string, KeywordStrings[Keyword_Using]))
            {
                if (state.is_global_scope)
                {
                    //// ERROR: Illegal use of using statement in global scope
                    encountered_errors = true;
                }
                
                else
                {
                    Statement* statement = AppendStatement(state, Statement_Using);
                    
                    SkipPastToken(state.lexer, token);
                    
                    if (ParseExpression(state, &statement->using_statement.path))
                    {
                        encountered_errors = true;
                    }
                }
            }
            
            else if (StringCompare(token.string, KeywordStrings[Keyword_Return]))
            {
                if (state.is_global_scope)
                {
                    //// ERROR: Illegal use of return statement in global scope
                    encountered_errors = true;
                }
                
                else
                {
                    Statement* statement = AppendStatement(state, Statement_Return);
                    
                    SkipPastToken(state.lexer, token);
                    
                    while (!encountered_errors)
                    {
                        Named_Argument* argument = Array_Append(&statement->return_statement.arguments);
                        
                        token      = GetToken(state.lexer);
                        Token peek = PeekNextToken(state.lexer, token);
                        
                        if (token.kind == Token_Identifier && peek.kind == Token_Equals &&
                            PeekNextToken(state.lexer, peek).kind != Token_Equals)
                        {
                            argument->name = token.string;
                            
                            SkipPastToken(state.lexer, peek);
                        }
                        
                        if (!ParseExpression(state, &argument->value)) encountered_errors = true;
                        else
                        {
                            token = GetToken(state.lexer);
                            
                            if (token.kind != Token_Comma) break;
                            else
                            {
                                SkipPastToken(state.lexer, token);
                                continue;
                            }
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                        else
                        {
                            //// ERROR: Missing terminating semicolon after return statement
                            encountered_errors = true;
                        }
                    }
                }
            }
            
            else
            {
                Token peek = PeekNextToken(state.lexer, token);
                
                if (peek.kind == Token_Comma)
                {
                    for (;;)
                    {
                        Token peek_1 = PeekNextToken(state.lexer, peek);
                        
                        if (peek_1.kind != Token_Identifier) break;
                        else
                        {
                            peek = PeekNextToken(state.lexer, peek_1);
                            
                            if (peek.kind == Token_Comma) continue;
                            else break;
                        }
                    }
                }
                
                if (peek.kind == Token_Colon)
                {
                    Statement* statement = AppendStatement(state, Statement_Declaration);
                    
                    if (!ParseDeclaration(state, &statement->declaration))
                    {
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    Token peek_1 = PeekNextToken(state.lexer, peek);
                    Token peek_2 = PeekNextToken(state.lexer, peek_1);
                    
                    bool is_equals = (peek.kind == Token_Equals && peek_1.kind != Token_Equals);
                    
                    bool is_single = (peek.kind == Token_Plus       ||
                                      peek.kind == Token_Minus      ||
                                      peek.kind == Token_Asterisk   ||
                                      peek.kind == Token_Slash      ||
                                      peek.kind == Token_Percentage ||
                                      peek.kind == Token_Ampersand  ||
                                      peek.kind == Token_Pipe       ||
                                      peek.kind == Token_Hat);
                    
                    bool is_double = (peek.kind == peek_1.kind && (peek.kind == Token_Ampersand ||
                                                                   peek.kind == Token_Pipe      ||
                                                                   peek.kind == Token_Less      ||
                                                                   peek.kind == Token_Greater));
                    
                    if (peek.kind == Token_Comma || is_equals    ||
                        peek_1.kind == Token_Equals && is_single ||
                        peek_2.kind == Token_Equals && is_double)
                    {
                        Statement* statement = AppendStatement(state, Statement_Assignment);
                        
                        while (!encountered_errors)
                        {
                            Expression** expr = Array_Append(&statement->assignment_statement.left);
                            
                            if (!ParseExpression(state, expr)) encountered_errors = true;
                            else
                            {
                                token = GetToken(state.lexer);
                                
                                if (token.kind == Token_Comma) SkipPastToken(state.lexer, token);
                                else break;
                            }
                        }
                        
                        if (!encountered_errors)
                        {
                            token = GetToken(state.lexer);
                            peek  = PeekNextToken(state.lexer, token);
                            
                            if (token.kind == Token_Equals)
                            {
                                statement->assignment_statement.operator = Expression_Invalid;
                                
                                SkipPastToken(state.lexer, peek);
                            }
                            
                            else if (peek.kind == Token_Equals)
                            {
                                switch (token.kind)
                                {
                                    case Token_Plus:
                                    statement->assignment_statement.operator = Expression_Add;
                                    break;
                                    
                                    case Token_Minus:
                                    statement->assignment_statement.operator = Expression_Sub;
                                    break;
                                    
                                    case Token_Asterisk:
                                    statement->assignment_statement.operator = Expression_Mul;    
                                    break;
                                    
                                    case Token_Slash:
                                    statement->assignment_statement.operator = Expression_Div;    
                                    break;
                                    
                                    case Token_Percentage:
                                    statement->assignment_statement.operator = Expression_Mod;    
                                    break;
                                    
                                    case Token_Ampersand:
                                    statement->assignment_statement.operator = Expression_BitAnd; 
                                    break;
                                    
                                    case Token_Pipe:
                                    statement->assignment_statement.operator = Expression_BitOr;  
                                    break;
                                    
                                    case Token_Hat:
                                    statement->assignment_statement.operator = Expression_BitXor; 
                                    break;
                                    
                                    INVALID_DEFAULT_CASE;
                                }
                                
                                SkipPastToken(state.lexer, peek);
                            }
                            
                            else
                            {
                                switch (token.kind)
                                {
                                    case Token_Ampersand: 
                                    statement->assignment_statement.operator = Expression_LogicalAnd; 
                                    break;
                                    
                                    case Token_Pipe:
                                    statement->assignment_statement.operator = Expression_LogicalOr;  
                                    break;
                                    
                                    case Token_Less:
                                    statement->assignment_statement.operator = Expression_BitLShift;  
                                    break;
                                    
                                    case Token_Greater:
                                    statement->assignment_statement.operator = Expression_BitRShift;  
                                    break;
                                    
                                    INVALID_DEFAULT_CASE;
                                }
                                
                                peek = PeekNextToken(state.lexer, peek);
                                SkipPastToken(state.lexer, peek);
                            }
                            
                            while (!encountered_errors)
                            {
                                Expression** expr = Array_Append(&statement->assignment_statement.right);
                                
                                if (!ParseExpression(state, expr)) encountered_errors = true;
                                else
                                {
                                    token = GetToken(state.lexer);
                                    
                                    if (token.kind == Token_Comma) SkipPastToken(state.lexer, token);
                                    else break;
                                }
                            }
                            
                            if (!encountered_errors)
                            {
                                token = GetToken(state.lexer);
                                
                                if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                                else
                                {
                                    //// ERROR: Missing terminating semicolon after assignment
                                    encountered_errors = true;
                                }
                            }
                        }
                    }
                    
                    else
                    {
                        Statement* statement = AppendStatement(state, Statement_Expression);
                        
                        if (!ParseExpression(state, &statement->expression)) encountered_errors = true;
                        else
                        {
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                            else
                            {
                                //// ERROR: Missing terminating semicolon after expression
                                encountered_errors = true;
                            }
                        }
                    }
                }
            }
        }
        
        else if (token.kind == Token_At)
        {
            Statement* statement = AppendStatement(state, Statement_Declaration);
            
            if (!ParseDeclaration(state, &statement->declaration))
            {
                encountered_errors = true;
            }
        }
        
        else
        {
            Statement* statement = AppendStatement(state, Statement_Expression);
            
            if (!ParseExpression(state, &statement->expression)) encountered_errors = true;
            else
            {
                token = GetToken(state.lexer);
                
                if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                else
                {
                    //// ERROR: Missing terminating semicolon after expression
                    encountered_errors = true;
                }
            }
        }
    }
    
    return !encountered_errors;
}

bool
ParseScope(Parser_State state, Scope* scope, bool allow_single_statement)
{
    bool encountered_errors = false;
    
    state.current_scope = scope;
    
    Token token = GetToken(state.lexer);
    
    bool is_single_statement = false;
    if (token.kind == Token_OpenBrace) SkipPastToken(state.lexer, token);
    else
    {
        if (allow_single_statement) is_single_statement = true;
        else
        {
            //// ERROR: Expected a block
            encountered_errors = true;
        }
    }
    
    while (!encountered_errors)
    {
        token = GetToken(state.lexer);
        
        if (!is_single_statement && token.kind == Token_CloseBrace)
        {
            SkipPastToken(state.lexer, token);
            break;
        }
        
        else
        {
            if (!ParseStatement(state))
            {
                encountered_errors = true;
            }
            
            if (is_single_statement) break;
        }
    }
    
    return !encountered_errors;
}

bool
ParseFile(Workspace* workspace, Package* package, File_ID file_id)
{
    bool encountered_errors = false;
    
    Source_File* file = BucketArray_ElementAt(&package->loaded_files, file_id);
    
    // IMPORTANT TODO(soimn): Check if file->path points to a file or a directory when file_id is 0
    //                        import "dir", should be import "dir/dir.os"
    
    // NOTE(soimn): ReadEntireFile ensures the content is null terminated and valid utf-8 unicode
    if (ReadEntireFile(package->allocator, file->path, &file->content))
    {
        file->is_loaded = true;
        
        Lexer lexer = {0};
        lexer.arena            = &package->arena;
        lexer.position.file_id = file_id;
        lexer.file_start       = file->content.data;
        
        Parser_State state = {0};
        state.workspace           = workspace;
        state.package             = package;
        state.lexer               = &lexer;
        state.export_by_default   = workspace->export_by_default;
        state.is_global_scope     = true;
        state.is_proc_parameter   = false;
        state.should_bounds_check = workspace->bounds_check_by_default;
        
        /// Handle package declaration
        
        Token token = GetToken(state.lexer);
        Token peek  = PeekNextToken(state.lexer, token);
        if (token.kind == Token_Hash && peek.kind == Token_Identifier &&
            StringCStringCompare(peek.string, "package"))
        {
            // NOTE(soimn): File_ID 0 is reserved for the package's main file
            if (file_id == 0)
            {
                //// ERROR: Encountered package directive in loaded file
                encountered_errors = true;
            }
            
            else
            {
                SkipPastToken(state.lexer, peek);
                token = GetToken(state.lexer);
                
                if (token.kind != Token_Identifier)
                {
                    //// ERROR: Missing package name in package directive
                    encountered_errors = true;
                }
                
                else
                {
                    // NOTE(soimn): The main package is named main by default, instead of PACKAGE_TMP_NAME
                    ASSERT(StringCStringCompare(package->name, PACKAGE_TMP_NAME) ||
                           StringCStringCompare(package->name, "main"));
                    
                    package->name = token.string;
                    
                    SkipPastToken(state.lexer, token);
                    
                    Mutex_Lock(workspace->package_mutex);
                    
                    for (Bucket_Array_Iterator it = BucketArray_Iterate(&workspace->packages);
                         it.current != 0;
                         BucketArrayIterator_Advance(&it))
                    {
                        Package* current = it.current;
                        if (package != current && StringCompare(package->name, current->name))
                        {
                            //// ERROR: Package name collision
                            encountered_errors = true;
                            
                            break;
                        }
                    }
                    
                    Mutex_Unlock(workspace->package_mutex);
                    
                    if (!encountered_errors)
                    {
                        if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                        else
                        {
                            //// ERROR: Missing semicolon after package directive
                            encountered_errors = true;
                        }
                    }
                }
            }
        }
        
        // NOTE(soimn): File_ID 0 is reserved for the package's main file
        else if (file_id == 0)
        {
            // NOTE(soimn): Allow the main package to elide the package name
            // NOTE(soimn): The main package is named main by default, instead of PACKAGE_TMP_NAME
            if (!StringCStringCompare(package->name, "main"))
            {
                //// ERROR: Missing package directive in imported file
                encountered_errors = true;
            }
        }
        
        while (!encountered_errors)
        {
            token = GetToken(state.lexer);
            
            if (token.kind == Token_EndOfStream) break;
            else
            {
                if (!ParseStatement(state))
                {
                    encountered_errors = true;
                }
            }
        }
    }
    
    else encountered_errors = true;
    
    return !encountered_errors;
}

bool
Workspace_ParsePackage(Workspace* workspace, Package* package)
{
    ASSERT(package->status == PackageStatus_ReadyToParse);
    
    bool encountered_errors = false;
    
    Source_File* main_file = BucketArray_Append(&package->loaded_files);
    main_file->path = package->path;
    
    File_ID cursor = 0;
    
    while (!encountered_errors && cursor < BucketArray_ElementCount(&package->loaded_files))
    {
        if (!ParseFile(workspace, package, cursor))
        {
            encountered_errors = true;
        }
    }
    
    if (!encountered_errors)
    {
        // Run sema
        NOT_IMPLEMENTED;
    }
    
    return !encountered_errors;
}