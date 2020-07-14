typedef struct Parser_State
{
    Package* package;
    Lexer* lexer;
    Scope* current_scope;
} Parser_State;

bool ParseExpression(Parser_State state, Expression** expr);
bool ParseDeclaration(Parser_State state);
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

Procedure
InitProc(Parser_State state)
{
    Procedure proc = {
        .parameters    = ARRAY_INIT(&state.package->arena, Procedure_Parameter, 3),
        .return_values = ARRAY_INIT(&state.package->arena, Named_Value, 1),
        .body         = InitScope(state)
    };
    
    return proc;
}

Structure
InitStruct(Parser_State state)
{
    Structure structure = {
        .parameters = ARRAY_INIT(&state.package->arena, Named_Value, 1),
        .members    = ARRAY_INIT(&state.package->arena, Named_Value, 3)
    };
    
    return structure;
}

Enumeration
InitEnum(Parser_State state)
{
    Enumeration enumeration = {
        .members = ARRAY_INIT(&state.package->arena, Named_Value, 3)
    };
    
    return enumeration;
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
        expression.procedure = InitProc(state);
        break;
        
        case Expression_Struct:
        expression.structure = InitStruct(state);
        break;
        
        case Expression_Enum:
        expression.enumeration = InitEnum(state);
        break;
        
        INVALID_DEFAULT_CASE;
    }
    
    return expression;
}

Statement
InitDeclaration(Parser_State state, Enum32(DECLARATION_KIND) kind)
{
    Statement statement = {0};
    statement.kind = Statement_Declaration;
    
    Declaration* declaration = &statement.declaration;
    declaration->kind = kind;
    
    declaration->attributes = ARRAY_INIT(&state.package->arena, Attribute, 0);
    declaration->names      = ARRAY_INIT(&state.package->arena, String, 1);
    declaration->types      = ARRAY_INIT(&state.package->arena, Expression, 1);
    
    switch (declaration->kind)
    {
        case Declaration_Var:
        declaration->var.values = ARRAY_INIT(&state.package->arena, Expression, 1);
        break;
        
        case Declaration_Const:
        declaration->constant.values = ARRAY_INIT(&state.package->arena, Expression, 1);
        break;
        
        case Declaration_Proc:
        declaration->procedure = InitProc(state);
        break;
        
        case Declaration_Struct:
        declaration->structure = InitStruct(state);
        break;
        
        case Declaration_Enum:
        declaration->enumeration = InitEnum(state);
        break;
        
        INVALID_DEFAULT_CASE;
    }
    
    return statement;
}

Statement
InitStatement(Parser_State state, Enum32(STATEMENT_KIND) kind)
{
    Statement statement = {0};
    statement.kind = kind;
    
    switch (statement.kind)
    {
        case Statement_ConstIf:
        statement.const_if.body = ARRAY_INIT(&state.package->arena, Statement, 10);
        break;
        
        case Statement_Scope:
        statement.scope = InitScope(state);
        break;
        
        case Statement_If:
        statement.if_statement.true_body  = InitScope(state);
        statement.if_statement.false_body = InitScope(state);
        break;
        
        case Statement_Return:
        statement.return_statement.values = ARRAY_INIT(&state.package->arena, Named_Value, 2);
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
AppendDeclaration(Parser_State state, Enum32(DECLARATION_KIND) kind)
{
    Statement* statement = Array_Append(&state.current_scope->statements);
    *statement = InitDeclaration(state, kind);
    
    return statement;
}

Statement*
AppendStatement(Parser_State state, Enum32(STATEMENT_KIND) kind)
{
    Statement* statement = Array_Append(&state.current_scope->statements);
    *statement = InitStatement(state, kind);
    
    return statement;
}

bool
ParseProc(Parser_State state, Procedure* procedure)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
ParseStruct(Parser_State state, Structure* structure)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
ParseEnum(Parser_State state, Enumeration* enumeration)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
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
                    
                    if (!ParseProc(state, &(*current)->procedure))
                    {
                        encountered_errors = true;
                    }
                }
                
                else if (token.keyword == Keyword_Struct || token.keyword == Keyword_Union)
                {
                    *current = AppendExpression(state, Expression_Struct);
                    
                    if (!ParseStruct(state, &(*current)->structure))
                    {
                        encountered_errors = true;
                    }
                }
                
                else if (token.keyword == Keyword_Enum)
                {
                    *current = AppendExpression(state, Expression_Enum);
                    
                    if (!ParseEnum(state, &(*current)->enumeration))
                    {
                        encountered_errors = true;
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
ParseDeclaration(Parser_State state)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
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
                if (!ParseDeclaration(state))
                {
                    encountered_errors = true;
                }
            }
            
            else if (token.kind == Token_Identifier)
            {
                Enum32(STATEMENT_KIND) kind = Statement_Expression;
                
                Token peek = GetToken(state.lexer);
                
                for (;;)
                {
                    if (peek.kind == Token_Identifier)
                    {
                        peek = PeekNextToken(state.lexer, peek);
                        
                        if (peek.kind == Token_Comma)
                        {
                            peek = PeekNextToken(state.lexer, peek);
                            continue;
                        }
                    }
                    
                    if (peek.kind == Token_Colon)
                    {
                        kind = Statement_Declaration;
                    }
                    
                    else
                    {
                        Token peek_1 = PeekNextToken(state.lexer, peek);
                        Token peek_2 = PeekNextToken(state.lexer, peek_1);
                        
                        if (peek.kind == Token_Equals && peek_1.kind != Token_Equals)
                        {
                            // NOTE(soimn): regular assignment
                            kind = Statement_Assignment;
                        }
                        
                        else if (peek_1.kind == Token_Equals && (peek.kind == Token_Plus       ||
                                                                 peek.kind == Token_Minus      ||
                                                                 peek.kind == Token_Asterisk   ||
                                                                 peek.kind == Token_Slash      ||
                                                                 peek.kind == Token_Percentage ||
                                                                 peek.kind == Token_Ampersand  ||
                                                                 peek.kind == Token_Pipe       ||
                                                                 peek.kind == Token_Hat))
                        {
                            // NOTE(soimn): single token binary operator
                            kind = Statement_Assignment;
                        }
                        
                        else if (peek_2.kind == Token_Equals && peek_1.kind == peek_2.kind &&
                                 (peek_1.kind == Token_Ampersand ||
                                  peek_1.kind == Token_Pipe      ||
                                  peek_1.kind == Token_Less      ||
                                  peek_1.kind == Token_Greater))
                        {
                            // NOTE(soimn): double token binary operator
                            kind = Statement_Assignment;
                        }
                    }
                    
                    break;
                }
                
                if (kind == Statement_Declaration)
                {
                    if (!ParseDeclaration(state))
                    {
                        encountered_errors = true;
                    }
                }
                
                else if (kind == Statement_Assignment)
                {
                    Statement* statement = AppendStatement(state, Statement_Assignment);
                    
                    for (;;)
                    {
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_Identifier)
                        {
                            String* string = Array_Append(&statement->assignment.left);
                            *string = token.string;
                            
                            SkipPastToken(state.lexer, token);
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_Comma)
                            {
                                SkipPastToken(state.lexer, token);
                                continue;
                            }
                        }
                        
                        else
                        {
                            //// ERROR: Missing identifier after comma in multivariable assignment statement
                            encountered_errors = true;
                            break;
                        }
                        
                        peek         = PeekNextToken(state.lexer, token);
                        Token peek_1 = PeekNextToken(state.lexer, peek);
                        
                        if (token.kind == Token_Equals && peek.kind != Token_Equals)
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
                        
                        else if (peek_1.kind == Token_Equals && token.kind == peek.kind)
                        {
                            switch (token.kind)
                            {
                                case Token_Ampersand: statement->assignment.operator = Expression_LogicalAnd; break;
                                case Token_Pipe:      statement->assignment.operator = Expression_LogicalOr;  break;
                                case Token_Less:      statement->assignment.operator = Expression_BitRShift;  break;
                                case Token_Greater:   statement->assignment.operator = Expression_BitLShift;  break;
                                
                                INVALID_DEFAULT_CASE;
                            }
                            
                            SkipPastToken(state.lexer, peek_1);
                        }
                        
                        break;
                    }
                    
                    if (!encountered_errors)
                    {
                        while (!encountered_errors)
                        {
                            Expression* expr = Array_Append(&statement->assignment.right);
                            if (!ParseExpression(state, &expr)) encountered_errors = true;
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
                if (!ParseDeclaration(state)) encountered_errors = true;
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