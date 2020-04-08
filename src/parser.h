// TODO(soimn):
// * Improve compiler directive parsing (cleanup and error on duplicates)

typedef struct Expression_State_Flag
{
    bool is_set;
    Text_Interval text;
} Expression_State_Flag;

typedef struct Parser_State
{
    Worker_Context* context;
    Lexer* lexer;
    
    Block* current_block;
    bool allow_scope_controlling_directives;
    bool should_export;
    Expression_State_Flag current_flags[EXPRESSION_FLAG_COUNT];
} Parser_State;

inline void
ParserReport(Parser_State state, Enum32(REPORT_SEVERITY) severity, Text_Interval text,
             const char* format_string, ...)
{
    Arg_List arg_list;
    ARG_LIST_START(arg_list, format_string);
    
    Report(state.context->workspace, severity, CompilationStage_Parsing, WrapCString(format_string), arg_list, &text);
}

Expression*
PushExpression(Parser_State state, Enum32(EXPRESSION_KIND) kind)
{
    Expression* expression = BucketArray_Append(&state.current_block->expressions);
    expression->kind  = kind;
    
    NOT_IMPLEMENTED; // flags
    
    Memory_Arena* arena  = &state.context->arena;
    U32 default_capacity = 4; // TODO(soimn): Find a reasonable value for this
    
    switch (kind)
    {
        case ExprKind_TypeLiteral:
        case ExprKind_Call:
        expression->call_expr.args = ARRAY_INIT(arena, Named_Expression, default_capacity);
        break;
        
        case ExprKind_Proc:
        expression->proc_expr.args          = ARRAY_INIT(arena, Named_Expression, default_capacity);
        expression->proc_expr.return_values = ARRAY_INIT(arena, Named_Expression, default_capacity);
        break;
        
        case ExprKind_Struct:
        expression->struct_expr.args    = ARRAY_INIT(arena, Named_Expression, default_capacity);
        expression->struct_expr.members = ARRAY_INIT(arena, Named_Expression, default_capacity);
        break;
        
        case ExprKind_Enum:
        expression->enum_expr.args    = ARRAY_INIT(arena, Named_Expression, default_capacity);
        expression->enum_expr.members = ARRAY_INIT(arena, Named_Expression, default_capacity);
        break;
    }
    
    return expression;
}

Declaration*
PushDeclaration(Parser_State state, Enum32(DECLARATION_KIND) kind)
{
    Declaration* declaration = Array_Append(&state.current_block->declarations);
    declaration->kind = kind;
    return declaration;
}

Statement*
PushStatement(Parser_State state, Enum32(STATEMENT_KIND) kind)
{
    Statement* statement = Array_Append(&state.current_block->statements);
    statement->kind = kind;
    return statement;
}

Identifier
PushIdentifier(Parser_State state, String string)
{
    Identifier identifier = NULL_IDENTIFIER;
    
    Workspace* workspace = state.context->workspace;
    
    TryLockMutex(workspace->identifier_storage_mutex);
    
    for (Bucket_Array_Iterator it = BucketArray_Iterate(&workspace->identifier_storage);
         it.current != 0;
         BucketArrayIterator_Advance(&it))
    {
        if (StringCompare(string, *(String*)it.current))
        {
            identifier = it.current_index;
            break;
        }
    }
    
    if (identifier == NULL_IDENTIFIER)
    {
        identifier = BucketArray_ElementCount(&workspace->identifier_storage);
        
        String* new_entry = BucketArray_Append(&workspace->identifier_storage);
        new_entry->data = MemoryArena_AllocSize(&workspace->string_arena, string.size, ALIGNOF(U8));
        new_entry->size = string.size;
        
        Copy(string.data, new_entry->data, new_entry->size);
    }
    
    UnlockMutex(workspace->identifier_storage_mutex);
    
    return identifier;
}

bool
PushStringLiteral(Parser_State state, Token token, String_Literal* literal, I64* out_character)
{
    bool encountered_errors = false;
    
    Workspace* workspace = state.context->workspace;
    
    Buffer buffer = {
        .data = MemoryArena_AllocSize(&state.context->arena, token.string.size, ALIGNOF(U8)),
        .size = token.string.size
    };
    
    String processed_string = {
        .data = buffer.data,
        .size = 0
    };
    
    for (UMM i = 0; i < token.string.size; ++i)
    {
        if (token.string.data[i] == '\\')
        {
            i += 1;
            
            if (token.string.size > i)
            {
                bool should_insert_character = false;
                U8 character                 = 0;
                
                switch (token.string.data[i])
                {
                    case 'a':  character = '\a'; should_insert_character = true; break;
                    case 'b':  character = '\b'; should_insert_character = true; break;
                    case 'f':  character = '\f'; should_insert_character = true; break;
                    case 'n':  character = '\n'; should_insert_character = true; break;
                    case 'r':  character = '\r'; should_insert_character = true; break;
                    case 't':  character = '\t'; should_insert_character = true; break;
                    case 'v':  character = '\v'; should_insert_character = true; break;
                    case '\\': character = '\\'; should_insert_character = true; break;
                    case '\'': character = '\''; should_insert_character = true; break;
                    case '\"': character = '\"'; should_insert_character = true; break;
                    
                    default:
                    {
                        if (token.string.data[i] == 'x')
                        {
                            i += 2;
                            
                            if (token.string.size > i)
                            {
                                U8 high_char = ToUpper(token.string.data[i - 1]);
                                U8 low_char  = ToUpper(token.string.data[i]);
                                
                                if ((IsDigit(high_char) || high_char >= 'A' && high_char <= 'F') &&
                                    (IsDigit(low_char)  || low_char >= 'A'  && low_char <= 'F'))
                                {
                                    U8 high_part = 0;
                                    U8 low_part  = 0;
                                    
                                    if (IsDigit(high_char)) high_part =  high_char - '0';
                                    else                    high_part = (high_char - 'A') + 10;
                                    
                                    if (IsDigit(low_char)) low_part =  low_char - '0';
                                    else                   low_part = (low_char - 'A') + 10;
                                    
                                    U8 byte_value = (high_part << 4) | low_part;
                                    
                                    processed_string.data[processed_string.size] = byte_value;
                                    processed_string.size                       += 1;
                                }
                                
                                else
                                {
                                    Text_Pos start_pos = token.text.position;
                                    start_pos.column += i;
                                    
                                    Text_Interval report_interval = TextInterval(start_pos, 2);
                                    
                                    ParserReport(state, ReportSeverity_Error, report_interval,
                                                 "Value after byte escape sequence is not a valid hex value");
                                    
                                    encountered_errors = true;
                                }
                            }
                            
                            else
                            {
                                Text_Pos start_pos = token.text.position;
                                start_pos.column += i - 2;
                                
                                Text_Interval report_interval = TextInterval(start_pos, 2);
                                
                                ParserReport(state, ReportSeverity_Error, report_interval,
                                             "Missing hex value after byte value escape sequence");
                                
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            Text_Pos start_pos = token.text.position;
                            start_pos.column += i;
                            
                            Text_Interval report_interval = TextInterval(start_pos, 2);
                            
                            ParserReport(state, ReportSeverity_Error, report_interval,
                                         "Encountered an unknown escape sequence");
                            
                            encountered_errors = true;
                        }
                    } break;
                }
                
                if (should_insert_character)
                {
                    processed_string.data[processed_string.size] = character;
                    processed_string.size                       += 1;
                }
            }
            
            else
            {
                
            }
        }
        
        else
        {
            processed_string.data[processed_string.size] = token.string.data[i];
            processed_string.size                       += 1;
        }
    }
    
    TryLockMutex(workspace->string_storage_mutex);
    
    if (!encountered_errors)
    {
        bool did_find = false;
        
        for (Bucket_Array_Iterator it = BucketArray_Iterate(&workspace->string_storage);
             it.current != 0;
             BucketArrayIterator_Advance(&it))
        {
            if (StringCompare(processed_string, *(String*)it.current))
            {
                *literal = it.current_index;
                did_find = true;
                break;
            }
        }
        
        if (!did_find)
        {
            *literal = BucketArray_ElementCount(&workspace->string_storage);
            
            String* new_entry = BucketArray_Append(&workspace->string_storage);
            new_entry->data = MemoryArena_AllocSize(&workspace->string_arena, processed_string.size, ALIGNOF(U8));
            new_entry->size = processed_string.size;
            
            Copy(processed_string.data, new_entry->data, new_entry->size);
        }
    }
    
    UnlockMutex(workspace->identifier_storage_mutex);
    
    if (processed_string.size != 0 && out_character != 0)
    {
        U32 first_char = 0;
        
        if (processed_string.data[0] < I8_MAX)
        {
            first_char = processed_string.data[0];
            processed_string.size -= 1;
        }
        
        else
        {
            // NOTE(soimn): The file loader ensures the file is encoded in valid UTF-8 characters
            
            U8 first_byte = processed_string.data[0];
            
            U8 bytes[4] = {0};
            
            if ((first_byte & 0xF0) == 0xF0)
            {
                ASSERT(processed_string.size >= 4);
                
                bytes[0] = processed_string.data[0];
                bytes[1] = processed_string.data[1];
                bytes[2] = processed_string.data[2];
                bytes[3] = processed_string.data[3];
                
                processed_string.size -= 4;
            }
            
            else if ((first_byte & (1 << 5)) != 0)
            {
                ASSERT(processed_string.size >= 3);
                
                bytes[0] = processed_string.data[0];
                bytes[1] = processed_string.data[1];
                bytes[2] = processed_string.data[2];
                
                processed_string.size -= 3;
            }
            
            else
            {
                ASSERT(processed_string.size >= 2);
                
                bytes[0] = processed_string.data[0];
                bytes[1] = processed_string.data[1];
                
                processed_string.size -= 2;
            }
            
            NOT_IMPLEMENTED;
        }
        
        if (processed_string.size == 0)
        {
            *out_character = first_char;
        }
    }
    
    MemoryArena_FreeSize(&state.context->arena, buffer.data, (U32)buffer.size);
    
    return !encountered_errors;
}

bool
IsNameOfCompilerDirective(String string)
{
    bool result = (StringCStringCompare(string, "import")          ||
                   StringCStringCompare(string, "if")              ||
                   StringCStringCompare(string, "scope_export")    ||
                   StringCStringCompare(string, "scope_file")      ||
                   StringCStringCompare(string, "foreign")         ||
                   StringCStringCompare(string, "strict")          ||
                   StringCStringCompare(string, "loose")           ||
                   StringCStringCompare(string, "packed")          ||
                   StringCStringCompare(string, "padded")          ||
                   StringCStringCompare(string, "no_discard")      ||
                   StringCStringCompare(string, "deprecated")      ||
                   StringCStringCompare(string, "align")           ||
                   StringCStringCompare(string, "bounds_check")    ||
                   StringCStringCompare(string, "no_bounds_check") ||
                   StringCStringCompare(string, "inline")          ||
                   StringCStringCompare(string, "no_inline")       ||
                   StringCStringCompare(string, "type")            ||
                   StringCStringCompare(string, "distinct")        ||
                   StringCStringCompare(string, "char")            ||
                   StringCStringCompare(string, "run"));
    
    return result;
}

bool ParseNamedExpressionList(Parser_State state, Array(Named_Expression)* result);

bool ParsePostfixExpression(Parser_State state, Expression** result);
bool ParsePrimaryExpression(Parser_State state, Expression** result);
bool ParseMemberExpression(Parser_State state, Expression** result);
bool ParsePrefixExpression(Parser_State state, Expression** result);
bool ParseInfixAndPostfixCallExpression(Parser_State state, Expression** result);
bool ParseMultExpression(Parser_State state, Expression** result);
bool ParseAddExpression(Parser_State state, Expression** result);
bool ParseComparativeExpression(Parser_State state, Expression** result);
bool ParseLogicalAndExpression(Parser_State state, Expression** result);
bool ParseLogicalOrExpression(Parser_State state, Expression** result);
bool ParseExpression(Parser_State state, Expression** result, bool allow_assignment);


bool ParseDeclaration(Parser_State state);
bool ParseStatement(Parser_State state);
bool ParseBlock(Parser_State state, Block* block, bool allow_single_statement);

bool
ParseNamedExpressionList(Parser_State state, Array(Named_Expression)* result)
{
    bool encountered_errors = false;
    
    do
    {
        Named_Expression* expr = Array_Append(result);
        
        Token token = GetToken(state.lexer);
        
        if (token.kind == Token_Keyword && token.keyword == Keyword_Using)
        {
            expr->is_used  = true;
            
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
        }
        
        if (token.kind == Token_Cash)
        {
            expr->is_const = true;
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            if (token.kind == Token_Cash)
            {
                expr->is_baked = true;
                
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
            }
        }
        
        if (token.kind == Token_Identifier)
        {
            expr->name = PushIdentifier(state, token.string);
            
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
        }
        
        else if (token.kind == Token_Blank)
        {
            expr->name = BLANK_IDENTIFIER;
            
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
        }
        
        else expr->name = NULL_IDENTIFIER;
        
        if (token.kind == Token_Colon)
        {
            SkipPastToken(state.lexer, token);
            
            if (!ParseLogicalOrExpression(state, &expr->type))
            {
                encountered_errors = true;
            }
            
            token = GetToken(state.lexer);
        }
        
        if (!encountered_errors && (token.kind == Token_Equals || expr->name == NULL_IDENTIFIER))
        {
            if (token.kind == Token_Equals) SkipPastToken(state.lexer, token);
            
            if (!ParseLogicalOrExpression(state, &expr->value))
            {
                encountered_errors = true;
            }
            
            token = GetToken(state.lexer);
        }
        
        if (token.kind == Token_Comma)
        {
            SkipPastToken(state.lexer, token);
            continue;
        }
        
        else break;
        
    } while (!encountered_errors);
    
    return !encountered_errors;
}

bool
ParseExpressionList(Parser_State state, Array(Expression*)* array)
{
    bool encountered_errors = false;
    
    do
    {
        Expression** expr = Array_Append(array);
        
        if (ParseExpression(state, expr, true))
        {
            Token token = GetToken(state.lexer);
            
            if (token.kind == Token_Comma)
            {
                SkipPastToken(state.lexer, token);
                continue;
            }
            
            else break;
        }
        
        else encountered_errors = true;
        
    } while (!encountered_errors);
    
    return !encountered_errors;
}

bool
ParsePostfixExpression(Parser_State state, Expression** result)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    
    do
    {
        if (token.kind == Token_OpenParen)
        {
            Expression* call_pointer = *result;
            
            *result = PushExpression(state, ExprKind_Call);
            (*result)->call_expr.pointer = call_pointer;
            
            Text_Interval start_of_arg_list = token.text;
            
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            if (token.kind != Token_CloseParen)
            {
                if (!ParseNamedExpressionList(state, &(*result)->call_expr.args))
                {
                    encountered_errors = true;
                }
            }
            
            if (!encountered_errors)
            {
                token = GetToken(state.lexer);
                if (token.kind == Token_CloseParen)
                {
                    Text_Interval call_interval = TextIntervalMerge(start_of_arg_list,
                                                                    token.text);
                    
                    (*result)->text = TextIntervalMerge(call_pointer->text, call_interval);
                }
                
                else
                {
                    Text_Interval report_interval = TextIntervalFromEndpoints(start_of_arg_list.position,
                                                                              token.text.position);
                    
                    ParserReport(state, ReportSeverity_Error, report_interval, 
                                 "Missing matching closing parenthesis after argument list");
                    encountered_errors = true;
                }
            }
        }
        
        else if (token.kind == Token_OpenBracket)
        {
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            Expression* slice_pointer = *result;
            Expression* start         = 0;
            
            if (token.kind != Token_Colon)
            {
                if (!ParseLogicalOrExpression(state, &start))
                {
                    encountered_errors = true;
                }
            }
            
            token = GetToken(state.lexer);
            if (token.kind == Token_Colon)
            {
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                *result = PushExpression(state, ExprKind_Slice);
                (*result)->array_expr.pointer = slice_pointer;
                (*result)->array_expr.start   = start;
                
                if (token.kind != Token_CloseBracket)
                {
                    if (!ParseLogicalOrExpression(state, &(*result)->array_expr.length))
                    {
                        encountered_errors = true;
                    }
                }
            }
            
            else
            {
                *result = PushExpression(state, ExprKind_Subscript);
                (*result)->array_expr.pointer = slice_pointer;
                (*result)->array_expr.start   = start;
            }
            
            if (!encountered_errors)
            {
                if (token.kind == Token_CloseBracket)
                {
                    SkipPastToken(state.lexer, token);
                    
                    (*result)->text = TextIntervalMerge(slice_pointer->text, token.text);
                }
                
                else
                {
                    Text_Interval report_interval = TextIntervalFromEndpoints(slice_pointer->text.position, token.text.position);
                    
                    ParserReport(state, ReportSeverity_Error, report_interval, 
                                 "Missing matching closing bracket before end of statement");
                    
                    encountered_errors = true;
                }
            }
        }
        
        else if (token.kind == Token_OpenBracket)
        {
            Expression* type = *result;
            
            *result = PushExpression(state, ExprKind_TypeLiteral);
            (*result)->call_expr.pointer = type;
            
            Text_Pos start_of_arg_list = token.text.position;
            
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            if (token.kind != Token_CloseBrace)
            {
                if (!ParseNamedExpressionList(state, &(*result)->call_expr.args))
                {
                    encountered_errors = true;
                }
            }
            
            if (!encountered_errors)
            {
                token = GetToken(state.lexer);
                if (token.kind == Token_CloseBracket)
                {
                    SkipPastToken(state.lexer, token);
                    
                    (*result)->text = TextIntervalMerge(type->text, token.text);
                }
                
                else
                {
                    Text_Interval report_interval = TextIntervalFromEndpoints(start_of_arg_list,
                                                                              token.text.position);
                    
                    ParserReport(state, ReportSeverity_Error, report_interval, 
                                 "Missing matching closing brace after argument list");
                    encountered_errors = true;
                }
            }
        }
        
        else if (token.kind == Token_Hat)
        {
            Expression* operand = *result;
            
            *result = PushExpression(state, ExprKind_Dereference);
            (*result)->unary_expr.operand = operand;
            (*result)->text = TextIntervalMerge(operand->text, token.text);
        }
        
        else break;
        
    } while (!encountered_errors);
    
    return !encountered_errors;
}

bool
ParsePrimaryExpression(Parser_State state, Expression** result)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    
    // NOTE(soimn): These convey information about the #char directive without the need to store it in the 
    //              Parser_State
    bool is_char            = false;
    Text_Interval char_text = {0};
    
    while (!encountered_errors && token.kind == Token_Hash)
    {
        Token peek = PeekNextToken(state.lexer, token);
        
        if (peek.kind == Token_Identifier)
        {
            if (StringCStringCompare(peek.string, "bounds_check"))
            {
                state.current_flags[ExprFlag_BoundsCheck].is_set = true;
                state.current_flags[ExprFlag_BoundsCheck].text   = TextIntervalMerge(token.text, peek.text);
                
                SkipPastToken(state.lexer, peek);
            }
            
            else if (StringCStringCompare(peek.string, "no_bounds_check"))
            {
                state.current_flags[ExprFlag_BoundsCheck].is_set = false;
                state.current_flags[ExprFlag_BoundsCheck].text   = TextIntervalMerge(token.text, peek.text);
                
                SkipPastToken(state.lexer, peek);
            }
            
            else if (StringCStringCompare(peek.string, "type"))
            {
                state.current_flags[ExprFlag_TypeEvalContext].is_set = true;
                state.current_flags[ExprFlag_TypeEvalContext].text   = TextIntervalMerge(token.text, peek.text);
                
                SkipPastToken(state.lexer, peek);
            }
            
            else if (StringCStringCompare(peek.string, "inline"))
            {
                state.current_flags[ExprFlag_InlineCall].is_set = true;
                state.current_flags[ExprFlag_InlineCall].text   = TextIntervalMerge(token.text, peek.text);
                
                SkipPastToken(state.lexer, peek);
            }
            
            else if (StringCStringCompare(peek.string, "no_inline"))
            {
                state.current_flags[ExprFlag_InlineCall].is_set = false;
                state.current_flags[ExprFlag_InlineCall].text   = TextIntervalMerge(token.text, peek.text);
                
                SkipPastToken(state.lexer, peek);
            }
            
            else if (StringCStringCompare(peek.string, "run"))
            {
                NOT_IMPLEMENTED;
            }
            
            else if (StringCStringCompare(peek.string, "distinct"))
            {
                state.current_flags[ExprFlag_DistinctType].is_set = true;
                state.current_flags[ExprFlag_DistinctType].text   = TextIntervalMerge(token.text, peek.text);
                
                SkipPastToken(state.lexer, peek);
            }
            
            else if (StringCStringCompare(peek.string, "char"))
            {
                is_char   = true;
                char_text = TextIntervalMerge(token.text, peek.text);
            }
            
            else if (IsNameOfCompilerDirective(peek.string))
            {
                Text_Interval report_interval = TextIntervalMerge(token.text, peek.text);
                
                ParserReport(state, ReportSeverity_Error, report_interval,
                             "The '%S' compiler directive cannot be used in an expression context", peek.string);
                
                encountered_errors = true;
            }
            
            
            else
            {
                Text_Interval report_interval = TextIntervalMerge(token.text, peek.text);
                
                ParserReport(state, ReportSeverity_Error, report_interval,
                             "unknown compiler directive");
                
                encountered_errors = true;
            }
        }
        
        else
        {
            ParserReport(state, ReportSeverity_Error, token.text,
                         "Missing name of compiler directive after '#' token");
            
            encountered_errors = true;
        }
    }
    
    if (!encountered_errors)
    {
        if (token.kind == Token_Keyword)
        {
            if (token.keyword == Keyword_Proc)
            {
                Text_Pos start_of_procedure_header = token.text.position;
                
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                *result = PushExpression(state, ExprKind_Proc);
                
                if (token.kind == Token_OpenParen)
                {
                    Text_Pos start_of_arguments = token.text.position;
                    
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (token.kind != Token_CloseParen)
                    {
                        if (!ParseNamedExpressionList(state, &(*result)->proc_expr.args))
                        {
                            encountered_errors = true;
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(state.lexer);
                        if (token.kind == Token_CloseParen) SkipPastToken(state.lexer, token);
                        else
                        {
                            Text_Interval report_interval = TextIntervalFromEndpoints(start_of_arguments, 
                                                                                      token.text.position);
                            
                            ParserReport(state, ReportSeverity_Error, report_interval,
                                         "Missing matching closing parenthesis after procedure argument list");
                            
                            encountered_errors = true;
                        }
                    }
                }
                
                token = GetToken(state.lexer);
                Token peek = PeekNextToken(state.lexer, token);
                
                if (!encountered_errors && token.kind == Token_Minus && peek.kind == Token_Greater)
                {
                    SkipPastToken(state.lexer, peek);
                    token = GetToken(state.lexer);
                    
                    Text_Pos start_of_return_values = token.text.position;
                    
                    bool is_contained = false;
                    if (token.kind == Token_OpenParen)
                    {
                        is_contained = true;
                        SkipPastToken(state.lexer, token);
                    }
                    
                    if (ParseNamedExpressionList(state, &(*result)->proc_expr.return_values))
                    {
                        token = GetToken(state.lexer);
                        
                        if (is_contained)
                        {
                            if (token.kind == Token_CloseParen) SkipPastToken(state.lexer, token);
                            else
                            {
                                Text_Interval report_interval = TextIntervalFromEndpoints(start_of_return_values, 
                                                                                          token.text.position);
                                
                                ParserReport(state, ReportSeverity_Error, report_interval,
                                             "Missing matching closing parenthesis after procedure return value list");
                                
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            if ((*result)->proc_expr.return_values.size != 1)
                            {
                                Text_Interval report_interval = TextIntervalFromEndpoints(start_of_return_values, 
                                                                                          token.text.position);
                                
                                ParserReport(state, ReportSeverity_Error, report_interval,
                                             "Multiple procedure return values have to be enclosed in parentheses");
                                
                                encountered_errors = true;
                            }
                        }
                    }
                    
                    else encountered_errors = true;
                }
                
                token = GetToken(state.lexer);
                while (!encountered_errors && token.kind == Token_Hash)
                {
                    if (peek.kind == Token_Identifier)
                    {
                        if (StringCStringCompare(peek.string, "bounds_check"))
                        {
                            state.current_flags[ExprFlag_BoundsCheck].is_set = true;
                            state.current_flags[ExprFlag_BoundsCheck].text   = TextIntervalMerge(token.text, peek.text);
                            
                            SkipPastToken(state.lexer, peek);
                        }
                        
                        else if (StringCStringCompare(peek.string, "no_bounds_check"))
                        {
                            state.current_flags[ExprFlag_BoundsCheck].is_set = false;
                            state.current_flags[ExprFlag_BoundsCheck].text   = TextIntervalMerge(token.text, peek.text);
                            
                            SkipPastToken(state.lexer, peek);
                        }
                        
                        else if (StringCStringCompare(peek.string, "inline"))
                        {
                            SkipPastToken(state.lexer, peek);
                            (*result)->proc_expr.is_inlined = true;
                        }
                        
                        else if (StringCStringCompare(peek.string, "no_inline"))
                        {
                            SkipPastToken(state.lexer, peek);
                            (*result)->proc_expr.is_inlined = false;
                        }
                        
                        else if (StringCStringCompare(peek.string, "deprecated"))
                        {
                            Text_Pos start_of_directive = token.text.position;
                            
                            SkipPastToken(state.lexer, peek);
                            
                            (*result)->proc_expr.is_deprecated = true;
                            
                            token = GetToken(state.lexer);
                            if (token.kind == Token_String)
                            {
                                SkipPastToken(state.lexer, token);
                                
                                if (!PushStringLiteral(state, token, &(*result)->proc_expr.deprecation_note, 0))
                                {
                                    encountered_errors = true;
                                }
                            }
                            
                            else
                            {
                                Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive, 
                                                                                          token.text.position);
                                
                                ParserReport(state, ReportSeverity_Error, report_interval,
                                             "Missing deprecation notice after 'deprecated' compiler directive");
                                
                                encountered_errors = true;
                            }
                        }
                        
                        else if (StringCStringCompare(peek.string, "no_discard"))
                        {
                            SkipPastToken(state.lexer, peek);
                            (*result)->proc_expr.no_discard = true;
                        }
                        
                        else if (StringCStringCompare(peek.string, "foreign"))
                        {
                            Text_Pos start_of_directive = token.text.position;
                            
                            SkipPastToken(state.lexer, peek);
                            
                            (*result)->proc_expr.is_foreign = true;
                            
                            token = GetToken(state.lexer);
                            if (token.kind == Token_String)
                            {
                                SkipPastToken(state.lexer, token);
                                
                                if (!PushStringLiteral(state, token, &(*result)->proc_expr.foreign_library, 0))
                                {
                                    encountered_errors = true;
                                }
                            }
                            
                            else
                            {
                                Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive, 
                                                                                          token.text.position);
                                
                                ParserReport(state, ReportSeverity_Error, report_interval,
                                             "Missing foreign library name after 'foreign' compiler directive");
                                
                                encountered_errors = true;
                            }
                        }
                        
                        else if (StringCStringCompare(peek.string, "import")       ||
                                 StringCStringCompare(peek.string, "scope_export") ||
                                 StringCStringCompare(peek.string, "scope_file"))
                        {
                            Text_Interval report_interval = TextIntervalMerge(token.text, peek.text);
                            
                            ParserReport(state, ReportSeverity_Error, report_interval,
                                         "Scope controlling directives are illegal in this context. Scope controlling directives can only be used in global scope");
                            
                            encountered_errors = true;
                        }
                        
                        else if (IsNameOfCompilerDirective(peek.string))
                        {
                            Text_Interval report_interval = TextIntervalMerge(token.text, peek.text);
                            
                            ParserReport(state, ReportSeverity_Error, report_interval,
                                         "The '%S' directive cannot be used in this context", peek.string);
                            
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            Text_Interval report_interval = TextIntervalMerge(token.text, peek.text);
                            
                            ParserReport(state, ReportSeverity_Error, report_interval,
                                         "unknown compiler directive");
                            
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        ParserReport(state, ReportSeverity_Error, token.text,
                                     "Missing name of compiler directive after '#' token");
                        
                        encountered_errors = true;
                    }
                    
                    token = GetToken(state.lexer);
                }
                
                token = GetToken(state.lexer);
                (*result)->text = TextIntervalFromEndpoints(start_of_procedure_header, token.text.position);
                
                if (!encountered_errors)
                {
                    token = GetToken(state.lexer);
                    
                    if (token.kind == Token_OpenBrace)
                    {
                        if (!ParseBlock(state, &(*result)->proc_expr.block, false))
                        {
                            encountered_errors = true;
                        }
                    }
                }
            }
            
            else if (token.keyword == Keyword_Struct)
            {
                Text_Interval start_of_struct = token.text;
                
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                if (token.kind == Token_OpenParen)
                {
                    Text_Pos start_of_arguments = token.text.position;
                    
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (token.kind != Token_CloseParen)
                    {
                        if (!ParseNamedExpressionList(state, &(*result)->struct_expr.args))
                        {
                            encountered_errors = true;
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_CloseParen) SkipPastToken(state.lexer, token);
                        else
                        {
                            Text_Interval report_interval = TextIntervalFromEndpoints(start_of_arguments, 
                                                                                      token.text.position);
                            ParserReport(state, ReportSeverity_Error, report_interval,
                                         "Missing closing parenthesis after struct type argument list");
                            
                            encountered_errors = true;
                        }
                    }
                }
                
                token = GetToken(state.lexer);
                
                while (!encountered_errors && token.kind == Token_Hash)
                {
                    Token peek = PeekNextToken(state.lexer, token);
                    
                    if (peek.kind == Token_Identifier)
                    {
                        if (StringCStringCompare(peek.string, "strict"))
                        {
                            SkipPastToken(state.lexer, peek);
                            (*result)->struct_expr.is_strict = true;
                        }
                        
                        else if (StringCStringCompare(peek.string, "loose"))
                        {
                            SkipPastToken(state.lexer, peek);
                            (*result)->struct_expr.is_strict = false;
                        }
                        
                        else if (StringCStringCompare(peek.string, "packed"))
                        {
                            SkipPastToken(state.lexer, peek);
                            (*result)->struct_expr.is_packed = true;
                        }
                        
                        else if (StringCStringCompare(peek.string, "padded"))
                        {
                            SkipPastToken(state.lexer, peek);
                            (*result)->struct_expr.is_packed = false;
                        }
                        
                        else if (IsNameOfCompilerDirective(peek.string))
                        {
                            Text_Interval report_interval = TextIntervalMerge(token.text, peek.text);
                            
                            ParserReport(state, ReportSeverity_Error, report_interval,
                                         "The '%S' directive cannot be used in this context", peek.string);
                            
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            Text_Interval report_interval = TextIntervalMerge(token.text, peek.text);
                            
                            ParserReport(state, ReportSeverity_Error, report_interval,
                                         "unknown compiler directive");
                            
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        ParserReport(state, ReportSeverity_Error, token.text,
                                     "Missing name of compiler directive after '#' token");
                        
                        encountered_errors = true;
                    }
                    
                    token = GetToken(state.lexer);
                }
                
                if (!encountered_errors)
                {
                    token = GetToken(state.lexer);
                    
                    if (token.kind != Token_OpenBrace)
                    {
                        Text_Interval report_interval = TextIntervalFromEndpoints(start_of_struct.position, 
                                                                                  token.text.position);
                        
                        ParserReport(state, ReportSeverity_Error, report_interval,
                                     "Missing block after struct header");
                        
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        Text_Pos start_of_members = token.text.position;
                        
                        SkipPastToken(state.lexer, token);
                        token = GetToken(state.lexer);
                        
                        if (token.kind != Token_CloseBrace)
                        {
                            if (!ParseNamedExpressionList(state, &(*result)->struct_expr.members))
                            {
                                encountered_errors = true;
                            }
                        }
                        
                        if (!encountered_errors)
                        {
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_CloseBrace)
                            {
                                (*result)->text = TextIntervalMerge(start_of_struct, token.text);
                                SkipPastToken(state.lexer, token);
                            }
                            
                            else
                            {
                                Text_Interval report_interval = TextIntervalFromEndpoints(start_of_members, 
                                                                                          token.text.position);
                                ParserReport(state, ReportSeverity_Error, report_interval,
                                             "Missing closing brace after struct member list");
                                
                                encountered_errors = true;
                            }
                        }
                    }
                }
            }
            
            else if (token.keyword == Keyword_Enum)
            {
                Text_Interval start_of_enum = token.text;
                
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                if (token.kind == Token_OpenParen)
                {
                    Text_Pos start_of_arguments = token.text.position;
                    
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (token.kind != Token_CloseParen)
                    {
                        if (!ParseNamedExpressionList(state, &(*result)->enum_expr.args))
                        {
                            encountered_errors = true;
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_CloseParen) SkipPastToken(state.lexer, token);
                        else
                        {
                            Text_Interval report_interval = TextIntervalFromEndpoints(start_of_arguments, 
                                                                                      token.text.position);
                            
                            ParserReport(state, ReportSeverity_Error, report_interval,
                                         "Missing closing parenthesis after enum type argument list");
                            
                            encountered_errors = true;
                        }
                    }
                }
                
                token = GetToken(state.lexer);
                
                while (!encountered_errors && token.kind == Token_Hash)
                {
                    Token peek = PeekNextToken(state.lexer, token);
                    
                    if (peek.kind == Token_Identifier)
                    {
                        if (StringCStringCompare(peek.string, "strict"))
                        {
                            SkipPastToken(state.lexer, peek);
                            (*result)->enum_expr.is_strict = true;
                        }
                        
                        else if (StringCStringCompare(peek.string, "loose"))
                        {
                            SkipPastToken(state.lexer, peek);
                            (*result)->enum_expr.is_strict = false;
                        }
                        
                        else if (IsNameOfCompilerDirective(peek.string))
                        {
                            Text_Interval report_interval = TextIntervalMerge(token.text, peek.text);
                            
                            ParserReport(state, ReportSeverity_Error, report_interval,
                                         "The '%S' directive cannot be used in this context", peek.string);
                            
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            Text_Interval report_interval = TextIntervalMerge(token.text, peek.text);
                            
                            ParserReport(state, ReportSeverity_Error, report_interval,
                                         "unknown compiler directive");
                            
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        ParserReport(state, ReportSeverity_Error, token.text,
                                     "Missing name of compiler directive after '#' token");
                        
                        encountered_errors = true;
                    }
                    
                    token = GetToken(state.lexer);
                }
                
                if (!encountered_errors)
                {
                    token = GetToken(state.lexer);
                    
                    if (token.kind != Token_OpenBrace)
                    {
                        Text_Interval report_interval = TextIntervalFromEndpoints(start_of_enum.position, 
                                                                                  token.text.position);
                        
                        ParserReport(state, ReportSeverity_Error, report_interval,
                                     "Missing block after enum header");
                        
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        Text_Pos start_of_members = token.text.position;
                        
                        SkipPastToken(state.lexer, token);
                        token = GetToken(state.lexer);
                        
                        if (token.kind != Token_CloseBrace)
                        {
                            if (!ParseNamedExpressionList(state, &(*result)->enum_expr.members))
                            {
                                encountered_errors = true;
                            }
                        }
                        
                        if (!encountered_errors)
                        {
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_CloseBrace)
                            {
                                (*result)->text = TextIntervalMerge(start_of_enum, token.text);
                                SkipPastToken(state.lexer, token);
                            }
                            
                            else
                            {
                                Text_Interval report_interval = TextIntervalFromEndpoints(start_of_members, 
                                                                                          token.text.position);
                                ParserReport(state, ReportSeverity_Error, report_interval,
                                             "Missing closing brace after enum member list");
                                
                                encountered_errors = true;
                            }
                        }
                    }
                }
            }
        }
        
        else if (token.kind == Token_Number)
        {
            *result = PushExpression(state, ExprKind_Number);
            (*result)->number = token.number;
            (*result)->text = token.text;
            SkipPastToken(state.lexer, token);
        }
        
        else if (token.kind == Token_String)
        {
            String_Literal literal;
            I64 character = -1;
            if (PushStringLiteral(state, token, &literal, &character))
            {
                if (is_char)
                {
                    *result = PushExpression(state, ExprKind_Character);
                    (*result)->character = (Character)character;
                    (*result)->text      = TextIntervalMerge(char_text, token.text);
                }
                
                else
                {
                    *result = PushExpression(state, ExprKind_String);
                    (*result)->string = literal;
                    (*result)->text   = token.text;
                }
                
                SkipPastToken(state.lexer, token);
            }
            
            else encountered_errors = true;
        }
        
        
        else if (token.kind == Token_Identifier)
        {
            *result = PushExpression(state, ExprKind_Identifier);
            (*result)->identifier = PushIdentifier(state, token.string);
            (*result)->text = token.text;
            SkipPastToken(state.lexer, token);
        }
        
        else if (token.kind == Token_OpenParen)
        {
            SkipPastToken(state.lexer, token);
            
            if (ParseExpression(state, result, false))
            {
                token = GetToken(state.lexer);
                
                if (token.kind == Token_CloseParen) SkipPastToken(state.lexer, token);
                else
                {
                    Text_Interval report_interval = TextIntervalFromEndpoints((*result)->text.position, 
                                                                              token.text.position);
                    
                    ParserReport(state, ReportSeverity_Error, report_interval,
                                 "Missing closing parnethesis after enclosed expression");
                    
                    encountered_errors = true;
                }
            }
            
            else encountered_errors = true;
        }
        
        if (*result == 0)
        {
            ParserReport(state, ReportSeverity_Error, TextInterval(token.text.position, 0),
                         "Missing primary expression");
            encountered_errors = true;
        }
        
        
        if (!encountered_errors)
        {
            if (!ParsePostfixExpression(state, result))
            {
                encountered_errors = true;
            }
            
            if (is_char && (*result)->kind != ExprKind_Character)
            {
                token = GetToken(state.lexer);
                Text_Interval report_interval = TextIntervalFromEndpoints(char_text.position, 
                                                                          token.text.position);
                
                ParserReport(state, ReportSeverity_Error, report_interval, 
                             "Expected a string literal after #char directive");
                
                encountered_errors = true;
            }
        }
    }
    
    return !encountered_errors;
}

bool
ParseMemberExpression(Parser_State state, Expression** result)
{
    bool encountered_errors = false;
    
    Text_Interval start_of_expression = GetToken(state.lexer).text;
    if (ParsePrimaryExpression(state, result))
    {
        do
        {
            Token token = GetToken(state.lexer);
            
            if (token.kind == Token_Period)
            {
                SkipPastToken(state.lexer, token);
                
                Expression* left = *result;
                
                *result = PushExpression(state, ExprKind_Member);
                (*result)->binary_expr.left = left;
                
                if (ParsePrimaryExpression(state, &(*result)->binary_expr.right))
                {
                    (*result)->text = TextIntervalMerge(start_of_expression, (*result)->binary_expr.right->text);
                }
                
                else encountered_errors = true;
            }
            else break;
            
        } while (!encountered_errors);
    }
    
    else
    {
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

bool
ParsePrefixExpression(Parser_State state, Expression** result)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    Token peek  = PeekNextToken(state.lexer, token);
    
    while (token.kind == Token_Plus)
    {
        // NOTE(soimn): Catch for unary plus, skip token but do nothing
        SkipPastToken(state.lexer, token);
        token = GetToken(state.lexer);
    }
    
    Text_Interval start_of_expression = token.text;
    
    Enum32(EXPRESSION_KIND) kind = ExprKind_Invalid;
    switch(token.kind)
    {
        case Token_Minus:       kind = ExprKind_Neg;       break;
        case Token_Tilde:       kind = ExprKind_BitNot;    break;
        case Token_Exclamation: kind = ExprKind_Not;       break;
        case Token_Hat:         kind = ExprKind_Reference; break;
    }
    
    if (kind != ExprKind_Invalid && peek.kind != Token_Equals)
    {
        SkipPastToken(state.lexer, token);
        
        *result = PushExpression(state, kind);
        
        if (ParsePrefixExpression(state, &(*result)->unary_expr.operand))
        {
            (*result)->text = TextIntervalMerge(start_of_expression, (*result)->unary_expr.operand->text);
        }
        
        else encountered_errors = true;
    }
    
    else if (token.kind == Token_Cash)
    {
        SkipPastToken(state.lexer, token);
        
        *result = PushExpression(state, ExprKind_Polymorphic);
        
        if (ParsePrefixExpression(state, &(*result)->unary_expr.operand))
        {
            (*result)->text = TextIntervalMerge(start_of_expression, (*result)->unary_expr.operand->text);
        }
        
        else encountered_errors = true;
    }
    
    else if (token.kind == Token_OpenBracket)
    {
        SkipPastToken(state.lexer, token);
        token = GetToken(state.lexer);
        
        Token peek_1 = PeekNextToken(state.lexer, peek);
        
        if (token.kind == Token_CloseBracket)
        {
            *result = PushExpression(state, ExprKind_SliceType);
            SkipPastToken(state.lexer, token);
            
            if (ParsePrefixExpression(state, &(*result)->unary_expr.operand))
            {
                (*result)->text = TextIntervalMerge(start_of_expression, (*result)->unary_expr.operand->text);
            }
            
            else encountered_errors = true;
        }
        
        else if (token.kind == Token_Period && peek.kind == Token_Period &&
                 peek_1.kind == Token_CloseBracket)
        {
            *result = PushExpression(state, ExprKind_DynamicArrayType);
            SkipPastToken(state.lexer, peek_1);
            
            if (ParsePrefixExpression(state, &(*result)->unary_expr.operand))
            {
                (*result)->text = TextIntervalMerge(start_of_expression, (*result)->unary_expr.operand->text);
            }
            
            else encountered_errors = true;
        }
        
        else
        {
            *result = PushExpression(state, ExprKind_ArrayType);
            
            if (!ParseLogicalOrExpression(state, &(*result)->array_expr.length))
            {
                encountered_errors = true;
            }
            
            token = GetToken(state.lexer);
            if (token.kind == Token_CloseBracket)
            {
                SkipPastToken(state.lexer, token);
                
                if (ParsePrefixExpression(state, &(*result)->array_expr.pointer))
                {
                    (*result)->text = TextIntervalMerge(start_of_expression, (*result)->array_expr.pointer->text);
                }
                
                else encountered_errors = true;
            }
            
            else
            {
                Text_Interval report_interval = TextIntervalFromEndpoints(start_of_expression.position,
                                                                          token.text.position);
                ParserReport(state, ReportSeverity_Error, report_interval, 
                             "Missing matching closing bracket in static array type expression");
                
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        if (token.kind == Token_Identifier && (peek.kind == Token_OpenParen || peek.kind == Token_Quote))
        {
            *result = PushExpression(state, ExprKind_Call);
            (*result)->call_expr.pointer = PushExpression(state, ExprKind_Identifier);
            (*result)->call_expr.pointer->identifier = PushIdentifier(state, token.string);
            
            Named_Expression* first_arg = Array_Append(&(*result)->call_expr.args);
            
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            if (token.kind == Token_OpenParen)
            {
                Text_Interval start_of_arg_list = token.text;
                
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                if (token.kind != Token_CloseParen)
                {
                    if (!ParseNamedExpressionList(state, &(*result)->call_expr.args))
                    {
                        encountered_errors = true;
                    }
                }
                
                if (!encountered_errors)
                {
                    token = GetToken(state.lexer);
                    
                    if (token.kind == Token_CloseParen) SkipPastToken(state.lexer, token);
                    else
                    {
                        Text_Interval report_interval = TextIntervalFromEndpoints(start_of_arg_list.position,
                                                                                  token.text.position);
                        
                        ParserReport(state, ReportSeverity_Error, report_interval, 
                                     "Missing matching closing parenthesis after argument list");
                        
                        encountered_errors = true;
                    }
                }
            }
            
            if (!encountered_errors)
            {
                token = GetToken(state.lexer);
                
                if (token.kind == Token_Quote)
                {
                    SkipPastToken(state.lexer, token);
                    
                    // NOTE(soimn): This is a prefix procedure call
                    
                    if (ParsePrefixExpression(state, &first_arg->value))
                    {
                        (*result)->text = TextIntervalMerge(start_of_expression, first_arg->value->text);
                    }
                    
                    else encountered_errors = true;
                }
                
                else
                {
                    // NOTE(soimn): This is a normal procedure call
                    
                    Array_OrderedRemove(&(*result)->call_expr.args, 0);
                    
                    (*result)->text = TextIntervalFromEndpoints(start_of_expression.position, 
                                                                token.text.position);
                    
                    if (!ParsePostfixExpression(state, result))
                    {
                        encountered_errors = true;
                    }
                }
            }
        }
        
        else
        {
            if (!ParseMemberExpression(state, result))
            {
                encountered_errors = true;
            }
        }
    }
    
    return !encountered_errors;
}

bool
ParseInfixAndPostfixCallExpression(Parser_State state, Expression** result)
{
    bool encountered_errors = false;
    
    Text_Interval start_of_expression = GetToken(state.lexer).text;
    if (ParsePrefixExpression(state, result))
    {
        do
        {
            Token token = GetToken(state.lexer);
            Token quote_token = token;
            
            if (token.kind != Token_Quote) break;
            else
            {
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                Expression* first_arg_expr = *result;
                
                *result = PushExpression(state, ExprKind_Call);
                
                if (token.kind != Token_Identifier)
                {
                    ParserReport(state, ReportSeverity_Error, quote_token.text,
                                 "Missing name of procedure after quote in call expression");
                    encountered_errors = true;
                }
                
                else
                {
                    (*result)->call_expr.pointer = PushExpression(state, ExprKind_Identifier);
                    (*result)->call_expr.pointer->identifier = PushIdentifier(state, token.string);
                    
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    Named_Expression* first_arg  = Array_Append(&(*result)->call_expr.args);
                    Named_Expression* second_arg = Array_Append(&(*result)->call_expr.args);
                    
                    first_arg->value = first_arg_expr;
                    
                    if (token.kind == Token_OpenParen)
                    {
                        Text_Interval start_of_arg_list = token.text;
                        
                        SkipPastToken(state.lexer, token);
                        token = GetToken(state.lexer);
                        
                        if (token.kind != Token_CloseParen)
                        {
                            if (!ParseNamedExpressionList(state, &(*result)->call_expr.args))
                            {
                                encountered_errors = true;
                            }
                        }
                        
                        if (!encountered_errors)
                        {
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_CloseParen) SkipPastToken(state.lexer, token);
                            else
                            {
                                Text_Interval report_interval = TextIntervalFromEndpoints(start_of_arg_list.position,
                                                                                          token.text.position);
                                
                                ParserReport(state, ReportSeverity_Error, report_interval, 
                                             "Missing matching closing parenthesis after argument list");
                                
                                encountered_errors = true;
                            }
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_Quote)
                        {
                            SkipPastToken(state.lexer, token);
                            
                            if (ParsePrefixExpression(state, &second_arg->value))
                            {
                                (*result)->text = TextIntervalMerge(start_of_expression, second_arg->value->text);
                            }
                            
                            else
                            {
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            Array_OrderedRemove(&(*result)->call_expr.args, 1);
                            (*result)->text = TextIntervalFromEndpoints(start_of_expression.position, 
                                                                        GetToken(state.lexer).text.position);
                        }
                    }
                }
            }
            
        } while (!encountered_errors);
    }
    
    else
    {
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

bool
ParseMultExpression(Parser_State state, Expression** result)
{
    bool encountered_errors = false;
    
    Text_Interval start_of_expression = GetToken(state.lexer).text;
    if (ParseInfixAndPostfixCallExpression(state, result))
    {
        do
        {
            Token token  = GetToken(state.lexer);
            Token peek   = PeekNextToken(state.lexer, token);
            Token peek_1 = PeekNextToken(state.lexer, token);
            
            Enum32(EXPRESSION_KIND) kind = ExprKind_Invalid;
            
            if (token.kind == Token_Star)                                       kind = ExprKind_Mul;
            else if (token.kind == Token_Slash)                                 kind = ExprKind_Div;
            else if (token.kind == Token_Percentage)                            kind = ExprKind_Mod;
            else if (token.kind == Token_Greater && peek.kind == Token_Greater) kind = ExprKind_BitShiftLeft;
            else if (token.kind == Token_Less && peek.kind == Token_Less)       kind = ExprKind_BitShiftRight;
            
            if (kind == ExprKind_Invalid || peek.kind == Token_Equals || peek_1.kind == Token_Equals) break;
            else
            {
                if (kind == ExprKind_BitShiftLeft || kind == ExprKind_BitShiftRight)
                {
                    SkipPastToken(state.lexer, peek);
                }
                
                else SkipPastToken(state.lexer, token);
                
                
                Expression* left = *result;
                Expression* right;
                
                if (ParseInfixAndPostfixCallExpression(state, &right))
                {
                    *result = PushExpression(state, kind);
                    (*result)->binary_expr.left  = left;
                    (*result)->binary_expr.right = right;
                    (*result)->text  = TextIntervalMerge(start_of_expression, right->text);
                }
                
                else
                {
                    encountered_errors = true;
                }
            }
            
        } while (!encountered_errors);
    }
    
    else
    {
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

bool
ParseAddExpression(Parser_State state, Expression** result)
{
    bool encountered_errors = false;
    
    Text_Interval start_of_expression = GetToken(state.lexer).text;
    if (ParseMultExpression(state, result))
    {
        do
        {
            Token token = GetToken(state.lexer);
            Token peek  = PeekNextToken(state.lexer, token);
            
            Enum32(EXPRESSION_KIND) kind = ExprKind_Invalid;
            switch (token.kind)
            {
                case Token_Plus:  kind = ExprKind_Add;   break;
                case Token_Minus: kind = ExprKind_Sub;   break;
                case Token_Pipe:  kind = ExprKind_BitOr; break;
            }
            
            if (kind == ExprKind_Invalid || peek.kind == Token_Equals) break;
            else
            {
                SkipPastToken(state.lexer, token);
                
                Expression* left = *result;
                Expression* right;
                
                if (ParseMultExpression(state, &right))
                {
                    *result = PushExpression(state, kind);
                    (*result)->binary_expr.left  = left;
                    (*result)->binary_expr.right = right;
                    (*result)->text  = TextIntervalMerge(start_of_expression, right->text);
                }
                
                else
                {
                    encountered_errors= true;
                }
            }
        } while (!encountered_errors);
    }
    
    else
    {
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

bool
ParseComparativeExpression(Parser_State state, Expression** result)
{
    bool encountered_errors = false;
    
    Text_Interval start_of_expression = GetToken(state.lexer).text;
    
    if (ParseAddExpression(state, result))
    {
        while (!encountered_errors)
        {
            Token token = GetToken(state.lexer);
            Token peek  = PeekNextToken(state.lexer, token);
            
            Enum32(EXPRESSION_KIND) kind = ExprKind_Invalid;
            
            if (token.kind == Token_Equals && peek.kind == Token_Equals)
            {
                kind = ExprKind_CmpEqual;
            }
            
            else if (token.kind == Token_Exclamation && peek.kind == Token_Equals)
            {
                kind = ExprKind_CmpNotEqual;
            }
            
            else if (token.kind == Token_Greater)
            {
                kind = (peek.kind == Token_Equals ? ExprKind_CmpGreaterOrEqual : ExprKind_CmpGreater);
            }
            
            else if (token.kind == Token_Less)
            {
                kind = (peek.kind == Token_Equals ? ExprKind_CmpLessOrEqual : ExprKind_CmpLess);
            }
            
            if (kind == ExprKind_Invalid) break;
            else
            {
                SkipPastToken(state.lexer, (peek.kind == Token_Equals ? peek : token));
                
                Expression* left = *result;
                Expression* right;
                
                if (ParseAddExpression(state, &right))
                {
                    *result = PushExpression(state, kind);
                    (*result)->binary_expr.left = left;
                    (*result)->text = TextIntervalMerge(start_of_expression, right->text);
                }
                
                else encountered_errors = true;
                
            }
        }
    }
    
    else
    {
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

bool
ParseLogicalAndExpression(Parser_State state, Expression** result)
{
    bool encountered_errors = false;
    
    Text_Interval start_of_expression = GetToken(state.lexer).text;
    if (ParseComparativeExpression(state, result))
    {
        do
        {
            Token token  = GetToken(state.lexer);
            Token peek   = PeekNextToken(state.lexer, token);
            Token peek_1 = PeekNextToken(state.lexer, peek);
            
            if (token.kind != Token_Ampersand || peek.kind != Token_Ampersand ||
                peek_1.kind == Token_Equals)
            {
                break;
            }
            
            else
            {
                SkipPastToken(state.lexer, peek);
                
                Expression* left = *result;
                Expression* right;
                
                if (ParseLogicalAndExpression(state, &right))
                {
                    *result = PushExpression(state, ExprKind_And);
                    (*result)->binary_expr.left = left;
                    (*result)->text = TextIntervalMerge(start_of_expression, right->text);
                }
                
                else
                {
                    encountered_errors= true;
                }
            }
        } while (!encountered_errors);
    }
    
    else
    {
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

bool
ParseLogicalOrExpression(Parser_State state, Expression** result)
{
    bool encountered_errors = false;
    
    Text_Interval start_of_expression = GetToken(state.lexer).text;
    if (ParseLogicalAndExpression(state, result))
    {
        do
        {
            Token token  = GetToken(state.lexer);
            Token peek   = PeekNextToken(state.lexer, token);
            Token peek_1 = PeekNextToken(state.lexer, peek);
            
            if (token.kind != Token_Pipe || peek.kind != Token_Pipe || peek_1.kind == Token_Equals) break;
            else
            {
                SkipPastToken(state.lexer, peek);
                
                Expression* left  = *result;
                Expression* right;
                
                if (ParseLogicalOrExpression(state, &right))
                {
                    *result = PushExpression(state, ExprKind_Or);
                    (*result)->binary_expr.left  = left;
                    (*result)->binary_expr.right = right;
                    (*result)->text  = TextIntervalMerge(start_of_expression, right->text);
                }
                
                else
                {
                    encountered_errors = true;
                }
            }
        } while (!encountered_errors);
    }
    
    else
    {
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

bool
ParseExpression(Parser_State state, Expression** result, bool allow_assignment)
{
    bool encountered_errors = false;
    
    if (ParseLogicalOrExpression(state, result))
    {
        Token token  = GetToken(state.lexer);
        Token peek   = PeekNextToken(state.lexer, token);
        Token peek_1 = PeekNextToken(state.lexer, peek);
        
        bool is_assignment            = false;
        Text_Interval report_interval = {0};
        
        if (token.kind == Token_Equals)
        {
            is_assignment = true;
            report_interval = token.text;
        }
        
        else if (peek.kind == Token_Equals && (token.kind == Token_Plus || token.kind == Token_Minus ||
                                               token.kind == Token_Star || token.kind == Token_Slash ||
                                               token.kind == Token_Percentage))
        {
            is_assignment = true;
            report_interval = TextIntervalMerge(token.text, peek.text);
        }
        
        else if (peek_1.kind == Token_Equals)
        {
            if ((token.kind == Token_Pipe      && peek.kind == Token_Pipe)      ||
                (token.kind == Token_Ampersand && peek.kind == Token_Ampersand) ||
                (token.kind == Token_Greater   && peek.kind == Token_Greater)   ||
                (token.kind == Token_Less      && peek.kind == Token_Less))
            {
                is_assignment = true;
                report_interval = TextIntervalMerge(token.text, peek_1.text);
            }
        }
        
        if (is_assignment && !allow_assignment)
        {
            ParserReport(state, ReportSeverity_Info, report_interval,
                         "Assignment operator in expression context. Assignment operators are only allowed at the statement level.");
            
            encountered_errors = true;
        }
    }
    
    else encountered_errors = true;
    
    return !encountered_errors;
}

bool
ParseStatement(Parser_State state)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    
    while (!encountered_errors && token.kind == Token_Hash)
    {
        Token peek = PeekNextToken(state.lexer, token);
        
        if (peek.kind == Token_Identifier)
        {
            if (StringCStringCompare(peek.string, "bounds_check"))
            {
                state.current_flags[ExprFlag_BoundsCheck].is_set = true;
                state.current_flags[ExprFlag_BoundsCheck].text   = TextIntervalMerge(token.text, peek.text);
                
                SkipPastToken(state.lexer, peek);
            }
            
            else if (StringCStringCompare(peek.string, "no_bounds_check"))
            {
                state.current_flags[ExprFlag_BoundsCheck].is_set = false;
                state.current_flags[ExprFlag_BoundsCheck].text   = TextIntervalMerge(token.text, peek.text);
                
                SkipPastToken(state.lexer, peek);
            }
            
            else if (StringCStringCompare(peek.string, "if"))
            {
                NOT_IMPLEMENTED;
            }
            
            else if (StringCStringCompare(peek.string, "import")       ||
                     StringCStringCompare(peek.string, "scope_export") ||
                     StringCStringCompare(peek.string, "scope_file"))
            {
                Text_Interval report_interval = TextIntervalMerge(token.text, peek.text);
                
                ParserReport(state, ReportSeverity_Error, report_interval,
                             "Scope controlling directives are illegal in this context. Scope controlling directives can only be used in global scope");
                
                encountered_errors = true;
            }
            
            else if (!IsNameOfCompilerDirective(peek.string))
            {
                Text_Interval report_interval = TextIntervalMerge(token.text, peek.text);
                
                ParserReport(state, ReportSeverity_Error, report_interval,
                             "unknown compiler directive");
                
                encountered_errors = true;
            }
            
            else break;
        }
        
        else
        {
            ParserReport(state, ReportSeverity_Error, token.text,
                         "Missing name of compiler directive after '#' token");
            
            encountered_errors = true;
        }
        
        token = GetToken(state.lexer);
    }
    
    if (!encountered_errors)
    {
        if (token.kind == Token_Error)
        {
            encountered_errors = true;
        }
        
        else if (token.kind == Token_EndOfStream)
        {
            ParserReport(state, ReportSeverity_Error, token.text,
                         "Encountered end of file before mathing closing brace in block");
            ParserReport(state, ReportSeverity_Info, TextInterval(state.current_block->text.position, 1),
                         "Location of the open brace which begins the erroneous block");
            encountered_errors = true;
        }
        
        else if (token.kind == Token_Semicolon)
        {
            // NOTE(soimn): Stray semicolons are ignored to allow more flexibility in valid code style
            while (token.kind == Token_Semicolon)
            {
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
            }
        }
        
        else if (token.kind == Token_OpenBrace)
        {
            Statement* statement = PushStatement(state, StatementKind_Block);
            
            if (!ParseBlock(state, &statement->block_stmnt, false))
            {
                encountered_errors = true;
            }
        }
        
        else if (token.kind == Token_Keyword)
        {
            if (token.keyword == Keyword_If)
            {
                Text_Pos start_of_if = token.text.position;
                
                SkipPastToken(state.lexer, token);
                
                Statement* statement = PushStatement(state, StatementKind_If);
                
                if (ParseExpression(state, &statement->if_stmnt.condition, false))
                {
                    token = GetToken(state.lexer);
                    statement->text = TextIntervalFromEndpoints(start_of_if, token.text.position);
                    
                    if (ParseBlock(state, &statement->if_stmnt.true_block, true))
                    {
                        token = GetToken(state.lexer);
                        
                        if (token.kind == Token_Keyword && token.keyword == Keyword_Else)
                        {
                            SkipPastToken(state.lexer, token);
                            
                            if (!ParseBlock(state, &statement->if_stmnt.false_block, true))
                            {
                                encountered_errors = true;
                            }
                        }
                    }
                    
                    else encountered_errors = true;
                }
                
                else encountered_errors = true;
            }
            
            else if (token.keyword == Keyword_Else)
            {
                ParserReport(state, ReportSeverity_Error, token.text, "Encountered else without a matching if statement");
                
                encountered_errors = true;
            }
            
            else if (token.keyword == Keyword_While)
            {
                Text_Pos start_of_while = token.text.position;
                
                SkipPastToken(state.lexer, token);
                
                Statement* statement = PushStatement(state, StatementKind_While);
                
                if (ParseExpression(state, &statement->while_stmnt.condition, false))
                {
                    token = GetToken(state.lexer);
                    statement->text = TextIntervalFromEndpoints(start_of_while, token.text.position);
                    
                    if (!ParseBlock(state, &statement->while_stmnt.block, true))
                    {
                        encountered_errors = true;
                    }
                }
                
                else encountered_errors = true;
            }
            
            else if (token.keyword == Keyword_Break || token.keyword == Keyword_Continue)
            {
                Statement* statement = PushStatement(state, (token.keyword == Keyword_Break 
                                                             ? StatementKind_Break 
                                                             : StatementKind_Continue));
                
                statement->text = token.text;
                
                SkipPastToken(state.lexer, token);
                token = GetToken(state.lexer);
                
                if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                else
                {
                    ParserReport(state, ReportSeverity_Error, statement->text,
                                 "Missing semicolon after end of statement");
                    
                    encountered_errors = true;
                }
            }
            
            else if (token.keyword == Keyword_Return)
            {
                Text_Pos start_of_return = token.text.position;
                
                SkipPastToken(state.lexer, token);
                
                Statement* statement = PushStatement(state, StatementKind_Return);
                
                if (ParseNamedExpressionList(state, &statement->return_stmnt.values))
                {
                    token = GetToken(state.lexer);
                    statement->text = TextIntervalFromEndpoints(start_of_return, token.text.position);
                    
                    token = GetToken(state.lexer);
                    
                    if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                    else
                    {
                        ParserReport(state, ReportSeverity_Error, statement->text,
                                     "Missing semicolon after end of statement");
                        
                        encountered_errors = true;
                    }
                }
                
                else encountered_errors = true;
            }
            
            else if (token.keyword == Keyword_Defer)
            {
                Text_Pos start_of_defer = token.text.position;
                
                SkipPastToken(state.lexer, token);
                
                Statement* statement = PushStatement(state, StatementKind_Defer);
                
                if (!ParseBlock(state, &statement->defer_stmnt.block, true))
                {
                    encountered_errors = true;
                }
            }
            
            else INVALID_CODE_PATH;
        }
        
        else
        {
            Array(Expression*) expression_list = ARRAY_INIT(&state.context->arena, Expression*, 4);
            
            Text_Pos start_of_expression = token.text.position;
            
            if (!ParseExpressionList(state, &expression_list))
            {
                encountered_errors = true;
            }
            
            if (!encountered_errors)
            {
                token        = GetToken(state.lexer);
                Token peek   = PeekNextToken(state.lexer, token);
                Token peek_1 = PeekNextToken(state.lexer, peek);
                
                if (token.kind == Token_Colon)
                {
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    Expression* type = 0;
                    if (token.kind != Token_Colon && token.kind != Token_Equals)
                    {
                        if (!ParseExpression(state, &type, true))
                        {
                            encountered_errors = true;
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(state.lexer);
                        
                        Array(Expression*) types = ARRAY_INIT(&state.context->arena, Expression*, 4);
                        
                        if (token.kind != Token_Colon && token.kind != Token_Equals)
                        {
                            if (!ParseExpressionList(state, &types))
                            {
                                encountered_errors = true;
                            }
                        }
                        
                        if (!encountered_errors)
                        {
                            token = GetToken(state.lexer);
                            
                            Declaration* declaration = 0;
                            
                            if (token.kind == Token_Colon)
                            {
                                // NOTE(soimn): This is considered a constant declaration
                                
                                declaration = PushDeclaration(state, DeclKind_ConstantDecl);
                                declaration->const_decl.names = expression_list;
                                declaration->const_decl.types = types;
                                
                                declaration->const_decl.values = ARRAY_INIT(&state.context->arena, Expression*, 4);
                            }
                            
                            else
                            {
                                // NOTE(soimn): This is considered a variable declaration
                                
                                declaration = PushDeclaration(state, DeclKind_VariableDecl);
                                declaration->var_decl.names = expression_list;
                                declaration->var_decl.types = types;
                                
                                declaration->var_decl.values = ARRAY_INIT(&state.context->arena, Expression*, 4);
                            }
                            
                            if (token.kind == Token_Colon || token.kind == Token_Equals)
                            {
                                Text_Interval start_of_assignment = token.text;
                                
                                SkipPastToken(state.lexer, token);
                                
                                token  = GetToken(state.lexer);
                                peek   = PeekNextToken(state.lexer, token);
                                peek_1 = PeekNextToken(state.lexer, peek);
                                
                                if (token.kind == Token_Minus && peek.kind == Token_Minus && peek_1.kind == Token_Minus)
                                {
                                    if (declaration->kind == DeclKind_VariableDecl)
                                    {
                                        declaration->var_decl.is_uninitialized = true;
                                        
                                        SkipPastToken(state.lexer, peek_1);
                                        
                                        token = GetToken(state.lexer);
                                        
                                        if (token.kind == Token_Comma)
                                        {
                                            Text_Interval report_interval = TextIntervalMerge(start_of_assignment,
                                                                                              peek_1.text);
                                            
                                            ParserReport(state, ReportSeverity_Error, report_interval,
                                                         "Explicit uninitialization used in expression list");
                                            
                                            ParserReport(state, ReportSeverity_Info, report_interval,
                                                         "The explicit uninitialization token sequence \"---\" is not an expression and is therefore not allowed to be used in an expression list");
                                            
                                            encountered_errors = true;
                                        }
                                    }
                                    
                                    else
                                    {
                                        Text_Interval report_interval = TextIntervalMerge(start_of_assignment, 
                                                                                          peek_1.text);
                                        
                                        ParserReport(state, ReportSeverity_Error, report_interval,
                                                     "Explicitly uninitializing a constant is illegal");
                                        
                                        encountered_errors = true;
                                    }
                                }
                                
                                else
                                {
                                    Array* values = 0;
                                    if (declaration->kind == DeclKind_ConstantDecl)
                                    {
                                        values = &declaration->const_decl.values;
                                    }
                                    
                                    else values = &declaration->var_decl.values;
                                    
                                    
                                    if (!ParseExpressionList(state, values))
                                    {
                                        encountered_errors = true;
                                    }
                                }
                            }
                            
                            if (!encountered_errors)
                            {
                                token = GetToken(state.lexer);
                                
                                if (token.kind == Token_Semicolon)
                                {
                                    SkipPastToken(state.lexer, token);
                                }
                                
                                else
                                {
                                    bool should_ignore_missing_semicolon = false;
                                    
                                    if (declaration->kind == DeclKind_ConstantDecl &&
                                        declaration->const_decl.values.size == 1)
                                    {
                                        Expression* expr = ((Expression**)declaration->const_decl.values.data)[0];
                                        
                                        if (expr->kind == ExprKind_Proc || expr->kind == ExprKind_Struct ||
                                            expr->kind == ExprKind_Enum)
                                        {
                                            should_ignore_missing_semicolon = true;
                                        }
                                    }
                                    
                                    if (!should_ignore_missing_semicolon)
                                    {
                                        Text_Interval report_interval = TextIntervalFromEndpoints(start_of_expression, 
                                                                                                  token.text.position);
                                        
                                        ParserReport(state, ReportSeverity_Error, report_interval,
                                                     "Missing semicolon after declaration");
                                        
                                        encountered_errors = true;
                                    }
                                }
                            }
                        }
                    }
                }
                
                else
                {
                    Enum32(EXPRESSION_KIND) kind = ExprKind_Invalid;
                    bool is_assignment = false;
                    
                    // NOTE(soimn): A lot of these checks are redundant, given that ParseExpression handles all 
                    //              cases where an assignment compatible operator is not followed by an equals 
                    //              sign. However, checking for assignment operators without infering the equals 
                    //              sign from context is a lot clearer.
                    
                    if (peek.kind == Token_Equals)
                    {
                        switch (token.kind)
                        {
                            case Token_Plus:       kind = ExprKind_Add;    break;
                            case Token_Minus:      kind = ExprKind_Sub;    break;
                            case Token_Star:       kind = ExprKind_Mul;    break;
                            case Token_Slash:      kind = ExprKind_Div;    break;
                            case Token_Percentage: kind = ExprKind_Mod;    break;
                            case Token_Pipe:       kind = ExprKind_BitOr;  break;
                            case Token_Ampersand:  kind = ExprKind_BitAnd; break;
                        }
                        
                        is_assignment = true;
                        
                        if (kind != ExprKind_Invalid) SkipPastToken(state.lexer, peek);
                        else                          SkipPastToken(state.lexer, token);
                    }
                    
                    if (!is_assignment)
                    {
                        if (token.kind == Token_Pipe && peek.kind == Token_Pipe &&
                            peek_1.kind == Token_Equals)
                        {
                            kind = ExprKind_Or;
                            is_assignment = true;
                        }
                        
                        else if (token.kind == Token_Ampersand && peek.kind == Token_Ampersand &&
                                 peek_1.kind == Token_Equals)
                        {
                            kind = ExprKind_And;
                            is_assignment = true;
                        }
                        
                        else if (token.kind == Token_Greater && peek.kind == Token_Greater && 
                                 peek_1.kind == Token_Equals)
                        {
                            kind = ExprKind_BitShiftRight;
                            
                            is_assignment = true;
                        }
                        
                        else if (token.kind == Token_Less && peek.kind == Token_Less && 
                                 peek_1.kind == Token_Equals)
                        {
                            kind = ExprKind_BitShiftLeft;
                            
                            is_assignment = true;
                        }
                        
                        if (is_assignment)
                        {
                            SkipPastToken(state.lexer, peek_1);
                        }
                    }
                    
                    Statement* statement = 0;
                    
                    if (is_assignment)
                    {
                        statement = PushStatement(state, StatementKind_Assignment);
                        statement->assignment_stmnt.kind = kind;
                        statement->assignment_stmnt.left = expression_list;
                        
                        do
                        {
                            Expression* expr = Array_Append(&statement->assignment_stmnt.right);
                            
                            if (ParseExpression(state, &expr, false))
                            {
                                token = GetToken(state.lexer);
                                
                                if (token.kind == Token_Comma)
                                {
                                    SkipPastToken(state.lexer, token);
                                    continue;
                                }
                                
                                else break;
                            }
                            
                            else encountered_errors = true;
                            
                        } while (!encountered_errors);
                        
                        token = GetToken(state.lexer);
                        statement->text = TextIntervalFromEndpoints(start_of_expression, token.text.position);
                        
                        if (statement->assignment_stmnt.right.size != 1 &&
                            statement->assignment_stmnt.right.size != statement->assignment_stmnt.left.size)
                        {
                            Text_Interval report_interval = TextIntervalFromEndpoints(start_of_expression,
                                                                                      token.text.position);
                            
                            ParserReport(state, ReportSeverity_Error, report_interval,
                                         "Mismatch between number of expressions on the left and right side of assignment operator");
                            
                            encountered_errors = true;
                        }
                        
                        if (!encountered_errors)
                        {
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_Semicolon) SkipPastToken(state.lexer, token);
                            else
                            {
                                ParserReport(state, ReportSeverity_Error, statement->text,
                                             "Missing semicolon after end of statement");
                                
                                encountered_errors = true;
                            }
                        }
                    }
                    
                    else
                    {
                        statement = PushStatement(state, StatementKind_Expression);
                        
                        if (expression_list.size != 1)
                        {
                            token = GetToken(state.lexer);
                            
                            Text_Interval report_interval = TextIntervalFromEndpoints(start_of_expression, 
                                                                                      token.text.position);
                            
                            ParserReport(state, ReportSeverity_Error, report_interval,
                                         "Expression lists are not allowed to be used as statements. Expression lists can only be used in argument lists and multiple declaration and assignment");
                            
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            // NOTE(soimn): expression_list stores pointers to the expression
                            statement->expression_stmnt = Array_ElementAt(&expression_list, 0);
                            Array_RemoveAllAndFreeHeader(&expression_list);
                            
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_Semicolon)
                            {
                                statement->text = TextIntervalFromEndpoints(start_of_expression, 
                                                                            token.text.position);
                                SkipPastToken(state.lexer, token);
                            }
                            
                            else
                            {
                                ParserReport(state, ReportSeverity_Error, statement->text,
                                             "Missing semicolon after end of statement");
                                
                                encountered_errors = true;
                            }
                        }
                        
                    }
                }
            }
        }
    }
    
    return !encountered_errors;
}

bool
ParseBlock(Parser_State state, Block* block, bool allow_single_statement)
{
    bool encountered_errors = false;
    
    block->imported_scopes = ARRAY_INIT(&state.context->arena, Imported_Scope, 4);
    block->declarations    = ARRAY_INIT(&state.context->arena, Declaration, 4);
    block->statements      = ARRAY_INIT(&state.context->arena, Statement, 4);
    block->expressions     = BUCKET_ARRAY_INIT(&state.context->arena, Expression, 8);
    
    Token token = GetToken(state.lexer);
    
    Text_Pos start_of_block = token.text.position;
    
    while (!encountered_errors && token.kind == Token_Hash)
    {
        Token peek = PeekNextToken(state.lexer, token);
        
        if (peek.kind == Token_Identifier)
        {
            if (StringCStringCompare(peek.string, "bounds_check"))
            {
                state.current_flags[ExprFlag_BoundsCheck].is_set = true;
                state.current_flags[ExprFlag_BoundsCheck].text   = TextIntervalMerge(token.text, peek.text);
                
                SkipPastToken(state.lexer, peek);
            }
            
            else if (StringCStringCompare(peek.string, "no_bounds_check"))
            {
                state.current_flags[ExprFlag_BoundsCheck].is_set = false;
                state.current_flags[ExprFlag_BoundsCheck].text   = TextIntervalMerge(token.text, peek.text);
                
                SkipPastToken(state.lexer, peek);
            }
            else if (StringCStringCompare(peek.string, "import")       ||
                     StringCStringCompare(peek.string, "scope_export") ||
                     StringCStringCompare(peek.string, "scope_file"))
            {
                Text_Interval report_interval = TextIntervalMerge(token.text, peek.text);
                
                ParserReport(state, ReportSeverity_Error, report_interval,
                             "Scope controlling directives are illegal in this context. Scope controlling directives can only be used in global scope");
                
                encountered_errors = true;
            }
            
            else if (IsNameOfCompilerDirective(peek.string))
            {
                Text_Interval report_interval = TextIntervalMerge(token.text, peek.text);
                
                ParserReport(state, ReportSeverity_Error, report_interval,
                             "The '%S' directive is not allowed to be used at statement level", peek.string);
                
                encountered_errors = true;
            }
            
            else
            {
                Text_Interval report_interval = TextIntervalMerge(token.text, peek.text);
                
                ParserReport(state, ReportSeverity_Error, report_interval,
                             "unknown compiler directive");
                
                encountered_errors = true;
            }
        }
        
        else
        {
            ParserReport(state, ReportSeverity_Error, token.text,
                         "Missing name of compiler directive after '#' token");
            
            encountered_errors = true;
        }
        
        token = GetToken(state.lexer);
    }
    
    if (!encountered_errors)
    {
        token = GetToken(state.lexer);
        
        bool is_single_statement = false;
        if (token.kind == Token_OpenBrace)
        {
            is_single_statement = false;
            SkipPastToken(state.lexer, token);
        }
        
        else
        {
            if (!allow_single_statement)
            {
                ParserReport(state, ReportSeverity_Error, token.text,
                             "Expected a block, but got a single statement");
                
                encountered_errors = true;
            }
        }
        
        while (!encountered_errors)
        {
            token = GetToken(state.lexer);
            
            if (token.kind == Token_CloseBrace)
            {
                if (is_single_statement)
                {
                    ParserReport(state, ReportSeverity_Error, token.text,
                                 "Encountered closing brace before a statement in single statement block");
                    
                    encountered_errors = true;
                }
                
                else
                {
                    SkipPastToken(state.lexer, token);
                    break;
                }
            }
            
            else
            {
                bool was_handled = false;
                
                if (token.kind == Token_Hash)
                {
                    Token peek = PeekNextToken(state.lexer, token);
                    
                    if (peek.kind == Token_Identifier)
                    {
                        if (StringCStringCompare(peek.string, "import"))
                        {
                            was_handled = true;
                            
                            if (state.allow_scope_controlling_directives)
                            {
                                Text_Pos start_of_directive = token.text.position;
                                
                                SkipPastToken(state.lexer, peek);
                                token = GetToken(state.lexer);
                                
                                if (token.kind == Token_String)
                                {
                                    String import_path = token.string;
                                    
                                    SkipPastToken(state.lexer, token);
                                    
                                    Scope_ID scope_id = AddSourceFile(state.context->workspace, import_path);
                                    
                                    Array(Imported_Scope)* imported_scopes = &state.current_block->imported_scopes;
                                    
                                    bool did_find = false;
                                    
                                    Imported_Scope* data = (Imported_Scope*)imported_scopes->data;
                                    for (U32 i = 0; i < imported_scopes->size; ++i)
                                    {
                                        if (data[i].scope_id == scope_id)
                                        {
                                            did_find = true;
                                        }
                                    }
                                    
                                    if (!did_find)
                                    {
                                        Imported_Scope* scope = Array_Append(imported_scopes);
                                        scope->scope_id = scope_id;
                                    }
                                }
                                
                                else
                                {
                                    Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive,
                                                                                              token.text.position);
                                    
                                    ParserReport(state, ReportSeverity_Error, report_interval,
                                                 "Missing import path in import directive");
                                    
                                    encountered_errors = true;
                                }
                            }
                        }
                        
                        else if (StringCStringCompare(peek.string, "scope_export"))
                        {
                            was_handled = true;
                            
                            if (state.allow_scope_controlling_directives)
                            {
                                SkipPastToken(state.lexer, peek);
                                state.should_export = true;
                            }
                        }
                        
                        else if (StringCStringCompare(peek.string, "scope_file"))
                        {
                            was_handled = true;
                            
                            if (state.allow_scope_controlling_directives)
                            {
                                SkipPastToken(state.lexer, peek);
                                state.should_export = false;
                            }
                        }
                        
                        if (!encountered_errors && !state.allow_scope_controlling_directives)
                        {
                            Text_Interval report_interval = TextIntervalMerge(token.text, peek.text);
                            
                            ParserReport(state, ReportSeverity_Error, report_interval,
                                         "Scope controlling directives are illegal in this context. Scope controlling directives can only be used in global scope");
                            
                            encountered_errors = true;
                        }
                    }
                }
                
                if (!was_handled && !ParseStatement(state))
                {
                    encountered_errors = true;
                }
            }
            
            if (is_single_statement) break;
            else continue;
        }
    }
    
    token = GetToken(state.lexer);
    
    block->text = TextIntervalFromEndpoints(start_of_block, token.text.position);
    
    return !encountered_errors;
}

void
ParseFile(Worker_Context* context, File_ID file_id)
{
    Lexer lexer_state = LexFile(context, file_id);
    
    Parser_State state = {0};
    state.context = context;
    state.lexer   = &lexer_state;
    
    NOT_IMPLEMENTED; // setup
    
    Block* block = 0;
    
    block->imported_scopes = ARRAY_INIT(&state.context->arena, Imported_Scope, 4);
    block->declarations    = ARRAY_INIT(&state.context->arena, Declaration, 4);
    block->statements      = ARRAY_INIT(&state.context->arena, Statement, 4);
    block->expressions     = BUCKET_ARRAY_INIT(&state.context->arena, Expression, 8);
    
    bool encountered_errors = false;
    
    do
    {
        Token token = GetToken(state.lexer);
        Token peek  = PeekNextToken(state.lexer, token);
        
        if (token.kind == Token_EndOfStream) break;
        
        else if (token.kind == Token_Hash && peek.kind == Token_Identifier &&
                 StringCStringCompare(peek.string, "import"))
        {
            Text_Pos start_of_directive = token.text.position;
            
            SkipPastToken(state.lexer, peek);
            token = GetToken(state.lexer);
            
            if (token.kind == Token_String)
            {
                String import_path = token.string;
                
                SkipPastToken(state.lexer, token);
                
                Scope_ID scope_id = AddSourceFile(state.context->workspace, import_path);
                
                Array(Imported_Scope)* imported_scopes = &state.current_block->imported_scopes;
                
                bool did_find = false;
                
                Imported_Scope* data = (Imported_Scope*)imported_scopes->data;
                for (U32 i = 0; i < imported_scopes->size; ++i)
                {
                    if (data[i].scope_id == scope_id)
                    {
                        did_find = true;
                    }
                }
                
                if (!did_find)
                {
                    Imported_Scope* scope = Array_Append(imported_scopes);
                    scope->scope_id = scope_id;
                }
            }
            
            else
            {
                Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive,
                                                                          token.text.position);
                
                ParserReport(state, ReportSeverity_Error, report_interval,
                             "Missing import path in import directive");
                
                encountered_errors = true;
            }
        }
        
        else if (token.kind == Token_Hash && peek.kind == Token_Identifier &&
                 StringCStringCompare(peek.string, "scope_export"))
        {
            SkipPastToken(state.lexer, peek);
            state.should_export = true;
        }
        
        else if (token.kind == Token_Hash && peek.kind == Token_Identifier &&
                 StringCStringCompare(peek.string, "scope_file"))
        {
            SkipPastToken(state.lexer, peek);
            state.should_export = false;
        }
        
        else
        {
            if (!ParseStatement(state))
            {
                encountered_errors = true;
            }
        }
    } while (!encountered_errors);
}