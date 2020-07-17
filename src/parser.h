typedef struct Parser_State
{
    Package* package;
    Lexer* lexer;
    Scope* current_scope;
} Parser_State;

bool ParseExpression(Parser_State state, Expression** expr);
bool ParseDeclaration(Parser_State state, Declaration* declaration);
bool ParseStatement(Parser_State state);
bool ParseScope(Parser_State state, Scope* scope, bool allow_single_statement);

Scope
InitScope(Parser_State state)
{
    Scope scope = {
        .statements      = ARRAY_INIT(&state.package->arena, Statement, 10),
        .expressions     = ARRAY_INIT(&state.package->arena, Expression, 10),
        .symbol_table_id = 0
    };
    
    return scope;
}

Expression
InitExpression(Parser_State state,  Enum32(EXPRESSION_KIND) kind)
{
    Expression expression = {0};
    expression.kind = kind;
    
    switch (expression.kind)
    {
        case Expression_Call:
        case Expression_StructLiteral:
        expression.call.parameters = ARRAY_INIT(&state.package->arena, Named_Value, 3);
        break;
        
        case Expression_Proc:
        expression.procedure.parameters    = ARRAY_INIT(&state.package->arena, Procedure_Parameter, 3);
        expression.procedure.return_values = ARRAY_INIT(&state.package->arena, Named_Value, 1);
        expression.procedure.body          = InitScope(state);
        break;
        
        case Expression_Struct:
        expression.structure.parameters = ARRAY_INIT(&state.package->arena, Named_Value, 1);
        expression.structure.members    = ARRAY_INIT(&state.package->arena, Named_Value, 3);
        break;
        
        case Expression_Enum:
        expression.enumeration.members = ARRAY_INIT(&state.package->arena, Named_Value, 3);
        break;
        
        INVALID_DEFAULT_CASE;
    }
    
    return expression;
}

Statement
InitStatement(Parser_State state, Enum32(STATEMENT_KIND) kind)
{
    Statement statement = {0};
    statement.kind = kind;
    
    switch (statement.kind)
    {
        case Statement_Declaration:
        statement.declaration.attributes = ARRAY_INIT(&state.package->arena, Attribute, 0);
        statement.declaration.names      = ARRAY_INIT(&state.package->arena, String, 1);
        statement.declaration.types      = ARRAY_INIT(&state.package->arena, Expression*, 1);
        break;
        
        case Statement_Scope:
        statement.scope = InitScope(state);
        break;
        
        case Statement_ConstIf:
        statement.const_if.body = ARRAY_INIT(&state.package->arena, Statement, 10);
        break;
        
        case Statement_If:
        statement.if_statement.true_body  = InitScope(state);
        statement.if_statement.false_body = InitScope(state);
        break;
        
        case Statement_Return:
        statement.return_statement.values = ARRAY_INIT(&state.package->arena, Named_Value, 2);
        break;
        
        case Statement_Assignment:
        statement.assignment.left  = ARRAY_INIT(&state.package->arena, String, 2);
        statement.assignment.right = ARRAY_INIT(&state.package->arena, Expression*, 2);
        break;
        
        INVALID_DEFAULT_CASE;
    }
    
    return statement;
}

Expression*
AppendExpression(Parser_State state, Enum32(EXPRESSION_KIND) kind)
{
    Expression* expression = Array_Append(&state.current_scope->expressions);
    *expression = InitExpression(state, kind);
    
    return expression;
}

Statement*
AppendStatement(Parser_State state, Enum32(STATEMENT_KIND) kind)
{
    Statement* statement = Array_Append(&state.current_scope->statements);
    *statement = InitStatement(state, kind);
    
    return statement;
}

bool
ParsePrimaryExpression(Parser_State state, Expression** expr)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    
    if (token.kind == Token_Hash)
    {
        NOT_IMPLEMENTED;
    }
    
    Expression** current         = expr;
    Expression* top_node_if_type = 0;
    
    while (!encountered_errors)
    {
        token = GetToken(state.lexer);
        
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
                current = &(*current)->operand;
            }
            
            else
            {
                SkipPastToken(state.lexer, token);
                
                //// WARNING: There are no increment or decrement operators in this language
            }
        }
        
        else if (token.kind == Token_Ampersand)
        {
            SkipPastToken(state.lexer, token);
            
            *current = AppendExpression(state, Expression_Reference);
            
            if (top_node_if_type == 0) top_node_if_type = *current;
            current = &(*current)->operand;
            
        }
        
        else if (token.kind == Token_Asterisk)
        {
            SkipPastToken(state.lexer, token);
            
            *current = AppendExpression(state, Expression_Dereference);
            current = &(*current)->operand;
        }
        
        else if (token.kind == Token_Tilde)
        {
            SkipPastToken(state.lexer, token);
            
            *current = AppendExpression(state, Expression_BitNot);
            current = &(*current)->operand;
        }
        
        else if (token.kind == Token_Exclamation)
        {
            SkipPastToken(state.lexer, token);
            
            *current = AppendExpression(state, Expression_LogicalNot);
            current = &(*current)->operand;
        }
        
        else if (token.kind == Token_OpenBracket)
        {
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            if (token.kind == Token_Period)
            {
                SkipPastToken(state.lexer, token);
                token      = GetToken(state.lexer);
                Token peek = PeekNextToken(state.lexer, token);
                
                if (token.kind != Token_Period || peek.kind != Token_CloseBracket)
                {
                    //// ERROR: Invalid use of period token after open bracket
                    encountered_errors = true;
                }
                
                else
                {
                    SkipPastToken(state.lexer, peek);
                    
                    *current = AppendExpression(state, Expression_DynamicArrayType);
                }
            }
            
            else if (token.kind == Token_CloseBracket)
            {
                SkipPastToken(state.lexer, token);
                
                *current = AppendExpression(state, Expression_SpanType);
            }
            
            else
            {
                *current = AppendExpression(state, Expression_ArrayType);
                
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
            
            if (top_node_if_type == 0) top_node_if_type = *current;
            current = &(*current)->array_type.elem_type;
        }
        
        else
        {
            Token peek = PeekNextToken(state.lexer, token);
            
            if (token.kind == Token_OpenParen)
            {
                SkipPastToken(state.lexer, token);
                
                if (!ParseExpression(state, current)) encountered_errors = true;
                else
                {
                    token = GetToken(state.lexer);
                    
                    if (token.kind == Token_CloseParen) SkipPastToken(state.lexer, token);
                    else
                    {
                        //// ERROR: Missing matching closing parenthesis
                        encountered_errors = true;
                    }
                }
            }
            
            else if (token.kind == Token_OpenBrace)
            {
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                *current = AppendExpression(state, Expression_StructLiteral);
                
                if (token.kind == Token_Colon)
                {
                    SkipPastToken(state.lexer, token);
                    
                    if (!ParseExpression(state, &(*current)->struct_literal.type)) encountered_errors = true;
                    else
                    {
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_Colon) SkipPastToken(state.lexer, token);
                        else
                        {
                            //// ERROR: Missing matching colon after type in struct literal
                            encountered_errors = true;
                        }
                    }
                }
                
                token = GetToken(state.lexer);
                
                if (token.kind != Token_CloseBrace)
                {
                    while (!encountered_errors)
                    {
                        token = GetToken(state.lexer);
                        
                        Named_Value* element = Array_Append(&(*current)->struct_literal.args);
                        
                        peek = PeekNextToken(state.lexer, token);
                        
                        if (token.kind == Token_Identifier && peek.kind == Token_Equals &&
                            PeekNextToken(state.lexer, peek).kind != Token_Equals)
                        {
                            element->name = token.string;
                            
                            SkipPastToken(state.lexer, peek);
                        }
                        
                        if (!ParseExpression(state, &element->value)) encountered_errors = true;
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
                        //// ERROR: Missing terminating closing brace after argument in struct literal
                        encountered_errors = true;
                    }
                }
            }
            
            else if (token.kind == Token_Cash)
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
                    *current = AppendExpression(state, Expression_PolymorphicName);
                    (*current)->string = token.string;
                    
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (token.kind == Token_Slash)
                    {
                        Expression** rightmost_alias = current;
                        
                        while (!encountered_errors)
                        {
                            SkipPastToken(state.lexer, token);
                            
                            Expression* left = *rightmost_alias;
                            
                            *rightmost_alias = AppendExpression(state, Expression_PolymorphicAlias);
                            (*rightmost_alias)->left = left;
                            rightmost_alias = &(*current)->right;
                            
                            if (!ParsePrimaryExpression(state, rightmost_alias)) encountered_errors = true;
                            else
                            {
                                token = GetToken(state.lexer);
                                
                                if (token.kind != Token_Slash) break;
                                else continue;
                            }
                        }
                    }
                }
            }
            
            else if (token.kind == Token_Keyword)
            {
                if (token.keyword == Keyword_Proc)
                {
                    *current = AppendExpression(state, Expression_Proc);
                    
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (token.kind == Token_OpenParen)
                    {
                        SkipPastToken(state.lexer, token);
                        
                        while (!encountered_errors)
                        {
                            Procedure_Parameter* parameter = Array_Append(&(*current)->procedure.parameters);
                            
                            token = GetToken(state.lexer);
                            peek  = PeekNextToken(state.lexer, token);
                            
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
                                //// ERROR: Missing name of procedure parameter
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                parameter->name = token.string;
                                
                                SkipPastToken(state.lexer, peek);
                                token = GetToken(state.lexer);
                                
                                if (token.kind != Token_Equals)
                                {
                                    if (!ParseExpression(state, &parameter->type))
                                    {
                                        encountered_errors = true;
                                    }
                                }
                                
                                if (!encountered_errors)
                                {
                                    token = GetToken(state.lexer);
                                    
                                    if (token.kind == Token_Equals)
                                    {
                                        if (!ParseExpression(state, &parameter->value))
                                        {
                                            encountered_errors = true;
                                        }
                                    }
                                    
                                    if (!encountered_errors && parameter->type == 0 && parameter->value == 0)
                                    {
                                        //// ERROR: Missing type and/or value of procedure parameter
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
                        token = GetToken(state.lexer);
                        peek  = PeekNextToken(state.lexer, token);
                        
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
                                Named_Value* return_value = Array_Append(&(*current)->procedure.return_values);
                                
                                token = GetToken(state.lexer);
                                peek  = PeekNextToken(state.lexer, token);
                                
                                if (token.kind == Token_Identifier && peek.kind == Token_Comma)
                                {
                                    return_value->name = token.string;
                                    
                                    SkipPastToken(state.lexer, peek);
                                }
                                
                                if (!ParseExpression(state, &return_value->value)) encountered_errors = true;
                                else
                                {
                                    token = GetToken(state.lexer);
                                    
                                    if (token.kind == Token_Comma)
                                    {
                                        if (is_array) SkipPastToken(state.lexer, token);
                                        else
                                        {
                                            //// ERROR: Illegal use of comma after return value. Multiple return values
                                            ////        must be enclosed in parentheses
                                            encountered_errors = true;
                                        }
                                    }
                                    
                                    else break;
                                }
                            }
                            
                            if (is_array && !encountered_errors)
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
                            (*current)->kind = Expression_ProcPointer;
                        }
                        
                        else
                        {
                            if (!ParseScope(state, &(*current)->procedure.body, false))
                            {
                                encountered_errors = true;
                            }
                        }
                    }
                }
                
                else if (token.keyword == Keyword_Struct || token.keyword == Keyword_Union)
                {
                    *current = AppendExpression(state, Expression_Struct);
                    
                    (*current)->structure.is_union = (token.keyword == Keyword_Union ? true : false);
                    
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
                                Named_Value* parameter = Array_Append(&(*current)->structure.parameters);
                                
                                token = GetToken(state.lexer);
                                peek  = PeekNextToken(state.lexer, token);
                                
                                if (token.kind != Token_Identifier || peek.kind != Token_Colon)
                                {
                                    //// ERROR: Missing name of parameter
                                    encountered_errors = true;
                                }
                                
                                else
                                {
                                    parameter->name = token.string;
                                    
                                    SkipPastToken(state.lexer, peek);
                                    
                                    if (!ParseExpression(state, &parameter->value)) encountered_errors = true;
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
                                    Named_Value* member = Array_Append(&(*current)->structure.members);
                                    
                                    token = GetToken(state.lexer);
                                    peek  = PeekNextToken(state.lexer, token);
                                    
                                    if (token.kind != Token_Identifier || peek.kind != Token_Colon)
                                    {
                                        //// ERROR: Missing name of struct member
                                        encountered_errors = true;
                                    }
                                    
                                    else
                                    {
                                        member->name = token.string;
                                        
                                        SkipPastToken(state.lexer, peek);
                                        
                                        if (!ParseExpression(state, &member->value)) encountered_errors = true;
                                        else
                                        {
                                            token = GetToken(state.lexer);
                                            
                                            if (token.kind == Token_Comma) SkipPastToken(state.lexer, token);
                                            
                                            else if (token.kind == Token_Semicolon)
                                            {
                                                //// ERROR: Invalid use of semicolon in struct body. Struct members are
                                                ////        delimited by commas
                                                encountered_errors = true;
                                            }
                                            
                                            else if (token.kind == Token_Equals)
                                            {
                                                //// ERROR: Invalid use of equals sign in struct body. Struct members
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
                                    //// ERROR: Missing matching closing brace after struct body
                                    encountered_errors = true;
                                }
                            }
                        }
                    }
                }
                
                else if (token.keyword == Keyword_Enum)
                {
                    *current = AppendExpression(state, Expression_Enum);
                    
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (token.kind != Token_OpenBrace)
                    {
                        if (!ParseExpression(state, &(*current)->enumeration.member_type))
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
                                Named_Value* member = Array_Append(&(*current)->enumeration.members);
                                
                                token = GetToken(state.lexer);
                                
                                if (token.kind != Token_Identifier)
                                {
                                    //// ERROR: Missing name of enum member
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
                                //// ERROR: Missing matching closing brace after enum body
                                encountered_errors = true;
                            }
                        }
                    }
                }
                
                else
                {
                    //// ERROR: Invalid use of keyword in expression
                    encountered_errors = true;
                }
            }
            
            else if (token.kind == Token_Identifier)
            {
                *current = AppendExpression(state, Expression_Identifier);
                (*current)->string = token.string;
            }
            
            else if (token.kind == Token_Number)
            {
                *current = AppendExpression(state, Expression_Number);
                (*current)->number = token.number;
            }
            
            else if (token.kind == Token_String)
            {
                *current = AppendExpression(state, Expression_String);
                (*current)->string = token.string;
            }
            
            else if (token.kind == Token_Char)
            {
                NOT_IMPLEMENTED;
            }
            
            else if (token.kind == Token_Keyword &&
                     (token.keyword == Keyword_True || token.keyword == Keyword_False))
            {
                *current = AppendExpression(state, Expression_Boolean);
                (*current)->boolean = (token.keyword == Keyword_True ? true : false);
            }
            
            else
            {
                //// ERROR: Missing primary expression
                encountered_errors = true;
            }
            
            if (!encountered_errors)
            {
                while (!encountered_errors)
                {
                    Expression** rightmost_member = 0;
                    
                    token = GetToken(state.lexer);
                    
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
                                Named_Value* element = Array_Append(&(*current)->call.parameters);
                                
                                peek = PeekNextToken(state.lexer, token);
                                
                                if (token.kind == Token_Identifier && peek.kind == Token_Equals &&
                                    PeekNextToken(state.lexer, peek).kind != Token_Equals)
                                {
                                    element->name = token.string;
                                    
                                    SkipPastToken(state.lexer, peek);
                                }
                                
                                if (!ParseExpression(state, &element->value)) encountered_errors = true;
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
                            if (token.kind == Token_CloseBracket)
                            {
                                SkipPastToken(state.lexer, token);
                                
                                if (is_span || start != 0 || length != 0)
                                {
                                    Expression* array = *current;
                                    
                                    *current = AppendExpression(state, (is_span ? Expression_Span : Expression_Subscript));
                                    
                                    if (is_span)
                                    {
                                        (*current)->span.array  = array;
                                        (*current)->span.start  = start;
                                        (*current)->span.length = length;
                                    }
                                    
                                    else
                                    {
                                        (*current)->subscript.array = array;
                                        (*current)->subscript.index = start;
                                    }
                                }
                                
                                else
                                {
                                    //// ERROR: Invalid use of subscript operator
                                    encountered_errors = true;
                                }
                            }
                            
                            else
                            {
                                //// ERROR: Missing matching closing bracket
                                encountered_errors = true;
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
                            Expression* left    = 0;
                            Expression** parent = 0;
                            
                            // NOTE(soimn): The rightmost_member is only valid when the previous operator was
                            //              a member operator
                            if ((*current)->kind == Expression_Member)
                            {
                                left   = *rightmost_member;
                                parent = rightmost_member;
                            }
                            
                            else
                            {
                                left   = *current;
                                parent = current;
                            }
                            
                            *parent = AppendExpression(state, Expression_Member);
                            (*parent)->left          = left;
                            (*parent)->right         = AppendExpression(state, Expression_Identifier);
                            (*parent)->right->string = token.string;
                            
                            rightmost_member = &(*parent)->right;
                            
                            SkipPastToken(state.lexer, token);
                        }
                    }
                    
                    else break;
                }
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
    
    if (!ParsePrimaryExpression(state, expr)) encountered_errors = true;
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

// TODO(soimn): Check precedence
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
            attribute->parameters = ARRAY_INIT(declaration->attributes.arena, Attribute_Parameter, 3);
            
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
                    
                    while (!encountered_errors)
                    {
                        Attribute_Parameter* parameter = Array_Append(&attribute->parameters);
                        
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_Number)
                        {
                            parameter->is_number = true;
                            parameter->is_string = false;
                            parameter->number    = token.number;
                            
                            SkipPastToken(state.lexer, token);
                        }
                        
                        else if (token.kind == Token_String)
                        {
                            parameter->is_number = false;
                            parameter->is_string = true;
                            parameter->string    = token.string;
                            
                            SkipPastToken(state.lexer, token);
                        }
                        
                        else if (token.kind == Token_Keyword && (token.keyword == Keyword_True ||
                                                                 token.keyword == Keyword_False))
                        {
                            parameter->is_number = false;
                            parameter->is_string = false;
                            parameter->boolean   = (token.keyword == Keyword_True ? true : false);
                            
                            SkipPastToken(state.lexer, token);
                        }
                        
                        else
                        {
                            //// ERROR: Missing attribute parameter after comma
                            encountered_errors = true;
                        }
                        
                        if (!encountered_errors)
                        {
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_Comma) SkipPastToken(state.lexer, token);
                            else break;
                        }
                    }
                    
                    token = GetToken(state.lexer);
                    
                    if (token.kind == Token_CloseParen) SkipPastToken(state.lexer, token);
                    else
                    {
                        //// ERROR: Missing matching closing parenthesis after attribute arguments
                        
                        encountered_errors = true;
                    }
                }
                
                token = GetToken(state.lexer);
                
                if (token.kind != Token_Comma) break;
                else
                {
                    if (is_array) SkipPastToken(state.lexer, token);
                    else
                    {
                        //// ERROR: Expected declaration after single attribute but encountered comma,
                        ////        use [] for multiple attributes
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
                    
                    Expression* tmp_expr;
                    if (!ParseExpression(state, &tmp_expr)) encountered_errors = true;
                    else
                    {
                        token = GetToken(state.lexer);
                        
                        // NOTE(soimn): Proc, struct and enums that are not part of a list are handled separately
                        if (token.kind != Token_Comma && (tmp_expr->kind == Expression_Proc   ||
                                                          tmp_expr->kind == Expression_Struct ||
                                                          tmp_expr->kind == Expression_Enum))
                        {
                            if (tmp_expr->kind == Expression_Proc)
                            {
                                declaration->kind      = Declaration_Proc;
                                declaration->procedure = tmp_expr->procedure;
                            }
                            
                            else if (tmp_expr->kind == Expression_Struct)
                            {
                                declaration->kind      = Declaration_Struct;
                                declaration->structure = tmp_expr->structure;
                            }
                            
                            else
                            {
                                declaration->kind        = Declaration_Enum;
                                declaration->enumeration = tmp_expr->enumeration;
                            }
                            
                            // HACK(soimn):
                            Array_OrderedRemove(&state.current_scope->expressions,
                                                state.current_scope->expressions.size - 1);
                            
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
                                    //// ERROR: Missing declaration after constant declaration
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
ParseConstantAssert(Parser_State state, Statement* statement)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    ASSERT(token.kind == Token_Hash);
    
    SkipPastToken(state.lexer, token);
    token = GetToken(state.lexer);
    ASSERT(token.kind == Token_Identifier && StringCStringCompare(token.string, "assert"));
    
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
        
        if (ParseExpression(state, &statement->const_assert.condition))
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
                    statement->const_assert.message = token.string;
                    
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
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
    
    return !encountered_errors;
}

bool
ParseStatement(Parser_State state)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    
    if (token.kind == Token_Hash)
    {
        // assert, const if
        NOT_IMPLEMENTED;
    }
    
    if (token.kind != Token_Error)
    {
        if (token.kind == Token_Unknown)
        {
            //// ERROR: Encountered an unknown token. Expected a statement
            encountered_errors = true;
        }
        
        else if (token.kind == Token_Keyword && token.keyword == Keyword_Import)
        {
            //// ERROR: Invalid use of import statement. Import statements are only valid in global scope
            encountered_errors = true;
        }
        
        else if (token.kind == Token_Keyword && token.keyword == Keyword_Load)
        {
            //// ERROR: Invalid use of load statement. Load statements are only valid in global scope
            encountered_errors = true;
        }
        
        else if (token.kind == Token_OpenBrace)
        {
            Statement* statement = AppendStatement(state, Statement_Scope);
            
            if (!ParseScope(state, &statement->scope, false))
            {
                encountered_errors = true;
            }
        }
        
        else if (token.kind == Token_Keyword && token.keyword == Keyword_If)
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
                
                if (!ParseExpression(state, &statement->if_statement.condition)) encountered_errors = true;
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
                        
                        if (!ParseScope(state, &statement->if_statement.true_body, true)) encountered_errors = true;
                        else
                        {
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_Keyword && token.keyword == Keyword_Else)
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
        
        else if (token.kind == Token_Keyword && token.keyword == Keyword_Else)
        {
            //// ERROR: Illegal else without matching if
            encountered_errors = true;
        }
        
        else if (token.kind == Token_Keyword && token.keyword == Keyword_Defer)
        {
            Statement* statement = AppendStatement(state, Statement_Defer);
            
            SkipPastToken(state.lexer, token);
            
            if (!ParseScope(state, &statement->defer_statement, true))
            {
                encountered_errors = true;
            }
        }
        
        else if (token.kind == Token_Keyword && token.keyword == Keyword_Using)
        {
            Statement* statement = AppendStatement(state, Statement_Using);
            
            SkipPastToken(state.lexer, token);
            
            if (ParseExpression(state, &statement->using_expression))
            {
                encountered_errors = true;
            }
        }
        
        else if (token.kind == Token_Keyword && token.keyword == Keyword_Return)
        {
            Statement* statement = AppendStatement(state, Statement_Return);
            
            SkipPastToken(state.lexer, token);
            
            while (!encountered_errors)
            {
                Named_Value* named_value = Array_Append(&statement->return_statement.values);
                
                token = GetToken(state.lexer);
                Token peek  = PeekNextToken(state.lexer, token);
                
                if (token.kind == Token_Identifier && peek.kind == Token_Equals &&
                    PeekNextToken(state.lexer, peek).kind != Token_Equals)
                {
                    named_value->name = token.string;
                    
                    SkipPastToken(state.lexer, peek);
                }
                
                if (!ParseExpression(state, &named_value->value)) encountered_errors = true;
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
        
        else
        {
            if (token.kind == Token_At)
            {
                Statement* statement = AppendStatement(state, Statement_Declaration);
                
                if (!ParseDeclaration(state, &statement->declaration))
                {
                    encountered_errors = true;
                }
            }
            
            else if (token.kind == Token_Identifier)
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
                    
                    if (is_equals || peek_1.kind == Token_Equals && is_single ||
                        peek_2.kind == Token_Equals && is_double)
                    {
                        Statement* statement = AppendStatement(state, Statement_Assignment);
                        
                        for (;;)
                        {
                            token = GetToken(state.lexer);
                            
                            String* identifier = Array_Append(&statement->assignment.left);
                            *identifier = token.string;
                            
                            SkipPastToken(state.lexer, token);
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_Comma) SkipPastToken(state.lexer, token);
                            else break;
                        }
                        
                        if (!encountered_errors)
                        {
                            token = GetToken(state.lexer);
                            peek  = PeekNextToken(state.lexer, token);
                            
                            if (token.kind == Token_Equals)
                            {
                                statement->assignment.operator = Expression_Invalid;
                                
                                SkipPastToken(state.lexer, peek);
                            }
                            
                            else if (peek.kind == Token_Equals)
                            {
                                switch (token.kind)
                                {
                                    case Token_Plus:       statement->assignment.operator = Expression_Add;    break;
                                    case Token_Minus:      statement->assignment.operator = Expression_Sub;    break;
                                    case Token_Asterisk:   statement->assignment.operator = Expression_Mul;    break;
                                    case Token_Slash:      statement->assignment.operator = Expression_Div;    break;
                                    case Token_Percentage: statement->assignment.operator = Expression_Mod;    break;
                                    case Token_Ampersand:  statement->assignment.operator = Expression_BitAnd; break;
                                    case Token_Pipe:       statement->assignment.operator = Expression_BitOr;  break;
                                    case Token_Hat:        statement->assignment.operator = Expression_BitXor; break;
                                    
                                    INVALID_DEFAULT_CASE;
                                }
                                
                                SkipPastToken(state.lexer, peek);
                            }
                            
                            else
                            {
                                switch (token.kind)
                                {
                                    case Token_Ampersand: statement->assignment.operator = Expression_LogicalAnd; break;
                                    case Token_Pipe:      statement->assignment.operator = Expression_LogicalOr;  break;
                                    case Token_Less:      statement->assignment.operator = Expression_BitRShift;  break;
                                    case Token_Greater:   statement->assignment.operator = Expression_BitLShift;  break;
                                    
                                    INVALID_DEFAULT_CASE;
                                }
                                
                                peek = PeekNextToken(state.lexer, peek);
                                SkipPastToken(state.lexer, peek);
                            }
                            
                            while (!encountered_errors)
                            {
                                Expression** expr = Array_Append(&statement->assignment.right);
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
ResolvePathString(Parser_State state, String relative_path, String* path)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
ParseFile(Workspace* workspace, Package* package, String path,
          File_ID* file_id, bool is_package_file)
{
    bool encountered_errors = false;
    
    *file_id = BucketArray_ElementCount(&package->loaded_files);
    
    Source_File* file = BucketArray_Append(&package->loaded_files);
    file->path      = path;
    file->is_loaded = false;
    
    // NOTE(soimn): ReadEntireFile ensures the content is null terminated and valid unicode
    if (ReadEntireFile(package->allocator, file->path, &file->content))
    {
        file->is_loaded = true;
        Lexer lexer = {0};
        lexer.position.file_id = *file_id;
        lexer.file_start       = file->content.data;
        
        Parser_State state = {0};
        state.package = package;
        state.lexer   = &lexer;
        
        file->scope = InitScope(state);
        
        
        /// Handle package declaration
        
        Token token = GetToken(state.lexer);
        if (token.kind == Token_Keyword && token.keyword == Keyword_Package)
        {
            if (is_package_file)
            {
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                if (token.kind == Token_Identifier)
                {
                    // NOTE(soimn): The main package is named main by default, instead of PACKAGE_TMP_NAME
                    ASSERT(StringCStringCompare(package->name, PACKAGE_TMP_NAME) ||
                           StringCStringCompare(package->name, "main"));
                    
                    package->name = token.string;
                    
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    Mutex_Lock(workspace->package_mutex);
                    
                    for (Bucket_Array_Iterator it = BucketArray_Iterate(&workspace->packages);
                         it.current != 0;
                         BucketArrayIterator_Advance(&it))
                    {
                        Package* current = it.current;
                        if (current != package && StringCompare(package->name, current->name))
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
                            //// ERROR: Missing semicolon after package declaration
                            encountered_errors = true;
                        }
                    }
                }
                
                else
                {
                    //// ERROR: Missing package name in package declaration
                    encountered_errors = true;
                }
            }
            
            else
            {
                
                //// ERROR: Encountered package declaration in loaded file
                encountered_errors = true;
            }
        }
        
        else if (is_package_file)
        {
            // NOTE(soimn): Allow the main package to not explicitly state the package name
            if (!StringCStringCompare(package->name, "main"))
            {
                //// ERROR: Missing package declaration in imported file
                encountered_errors = true;
            }
        }
        
        
        
        /// Parse file contents
        
        // NOTE(soimn): This loop handles parsing of import-, load statements and scope directives.
        //              This is done to not complicate the rest of the parser, as these are only valid
        //              in global scope. Constant ifs in global scope may contain these statements and
        //              dirtectives and are therefore also handled by this loop.
        //
        //              Every other kind of statement, directive, expression or declaration is delegated to
        //              the ParseDeclaration function. Except for constant assertions, which are handled
        //              both in this loop and by the ParseStatement function.
        //
        //              The "Global_Context" is just a way of handling parsing of nested constant ifs inline
        
        
        
        typedef struct Global_Context
        {
            bool export_by_default;
            Array(Statement)* statements;
        } Global_Context;
        
        Bucket_Array(Global_Context) context_stack = BUCKET_ARRAY_INIT(&state.package->arena,
                                                                       Global_Context, 3);
        
        Global_Context* current_context = BucketArray_Append(&context_stack);
        current_context->export_by_default = workspace->export_by_default;
        current_context->statements        = &file->scope.statements;
        
        while (!encountered_errors)
        {
            token      = GetToken(state.lexer);
            Token peek = PeekNextToken(state.lexer, token);
            
            if (token.kind == Token_EndOfStream) break;
            
            else if (token.kind == Token_Error) encountered_errors = true;
            
            else if (token.kind == Token_Unknown)
            {
                //// ERROR: Encountered an unknown token in global scope
                encountered_errors = true;
            }
            
            else if (token.kind == Token_Keyword && token.keyword == Keyword_Import)
            {
                Statement* statement = Array_Append(current_context->statements);
                *statement = InitStatement(state, Statement_Import);
                
                statement->import.is_exported = current_context->export_by_default;
                
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                if (token.kind != Token_String)
                {
                    //// ERROR: Missing import path name after import keyword in import statement
                    encountered_errors = true;
                }
                
                else
                {
                    String import_path_string;
                    if (ResolvePathString(state, token.string, &import_path_string))
                    {
                        statement->import.package_id = Workspace_AppendPackage(workspace, import_path_string);
                        
                        if (!encountered_errors)
                        {
                            SkipPastToken(state.lexer, token);
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_Keyword && token.keyword == Keyword_As)
                            {
                                SkipPastToken(state.lexer, token);
                                token = GetToken(state.lexer);
                                
                                if (token.kind != Token_Identifier && token.kind != Token_Underscore)
                                {
                                    //// ERROR: Missing alias after as keyword in import statement
                                    encountered_errors = true;
                                }
                                
                                else
                                {
                                    String underscore;
                                    underscore.data = (U8*)"_";
                                    underscore.size = 1;
                                    
                                    if (token.kind == Token_Keyword) statement->import.alias = token.string;
                                    else statement->import.alias = underscore;
                                    
                                    SkipPastToken(state.lexer, token);
                                }
                            }
                            
                            token = GetToken(state.lexer);
                            
                            if (!encountered_errors)
                            {
                                if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                                else
                                {
                                    //// ERROR: Missing terminating semicolon after import statement
                                    encountered_errors = true;
                                }
                            }
                        }
                    }
                    
                    else encountered_errors;
                }
            }
            
            else if (token.kind == Token_Keyword && token.keyword == Keyword_Load)
            {
                Statement* statement = Array_Append(current_context->statements);
                *statement = InitStatement(state, Statement_Load);
                
                statement->load.is_exported = current_context->export_by_default;
                
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                if (token.kind != Token_String)
                {
                    //// ERROR: Missing load path after load leyword in load statement
                    encountered_errors = true;
                }
                
                else
                {
                    String load_path;
                    if (ResolvePathString(state, token.string, &load_path))
                    {
                        SkipPastToken(state.lexer, token);
                        token = GetToken(state.lexer);
                        
                        if (ParseFile(workspace, package, load_path, &statement->load.file_id, false))
                        {
                            if (token.kind == Token_Keyword && token.keyword == Keyword_As)
                            {
                                //// ERROR: Invalid us of as keyword. Only import statements can be aliased
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                                else
                                {
                                    //// ERROR: Missing terminating semicolon after load statement
                                    encountered_errors = true;
                                }
                            }
                        }
                        
                        else encountered_errors = true;
                    }
                    
                    else encountered_errors = true;
                }
            }
            
            else if (token.kind == Token_Hash && peek.kind == Token_Identifier &&
                     StringCStringCompare(peek.string, "scope_export"))
            {
                SkipPastToken(state.lexer, peek);
                current_context->export_by_default = true;
            }
            
            else if (token.kind == Token_Hash && peek.kind == Token_Identifier &&
                     StringCStringCompare(peek.string, "scope_file"))
            {
                SkipPastToken(state.lexer, peek);
                current_context->export_by_default = false;
            }
            
            else if (token.kind == Token_Hash && peek.kind == Token_Identifier &&
                     StringCStringCompare(peek.string, "assert"))
            {
                Statement* statement = Array_Append(current_context->statements);
                *statement = InitStatement(state, Statement_ConstAssert);
                
                if (!ParseConstantAssert(state, statement))
                {
                    encountered_errors = true;
                }
            }
            
            else if (token.kind == Token_Hash && peek.kind == Token_Keyword &&
                     peek.keyword == Keyword_If)
            {
                Statement* statement = Array_Append(current_context->statements);
                *statement = InitStatement(state, Statement_ConstIf);
                
                SkipPastToken(state.lexer, peek);
                
                if (!ParseExpression(state, &statement->const_if.condition)) encountered_errors = true;
                else
                {
                    token = GetToken(state.lexer);
                    
                    if (token.kind != Token_OpenBrace)
                    {
                        //// ERROR: Missing block after condition in constant if
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        SkipPastToken(state.lexer, token);
                        
                        Global_Context* if_context = BucketArray_Append(&context_stack);
                        
                        if_context->export_by_default = current_context->export_by_default;
                        if_context->statements        = &statement->const_if.body;
                    }
                }
            }
            
            else if (token.kind == Token_CloseBrace)
            {
                U32 stack_size = BucketArray_ElementCount(&context_stack);
                
                if (stack_size == 1)
                {
                    //// ERROR: Encountered illegal token in global scope
                    encountered_errors = true;
                }
                
                else
                {
                    BucketArray_Free(&context_stack, current_context);
                    stack_size = BucketArray_ElementCount(&context_stack);
                    
                    current_context = BucketArray_ElementAt(&context_stack, stack_size - 1);
                }
            }
            
            else
            {
                Statement* statement = Array_Append(current_context->statements);
                *statement = InitStatement(state, Statement_Declaration);
                
                if (!ParseDeclaration(state, &statement->declaration)) encountered_errors = true;
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
    
    File_ID main_file_id;
    if (!ParseFile(workspace, package, package->path, &main_file_id, true)) encountered_errors = true;
    else
    {
        // Run Sema
        NOT_IMPLEMENTED;
    }
    
    return !encountered_errors;
}