// TODO(soimn):
// - Store the location of the directive which modified each bit in the current_flag

/// Helper macros
#define PARSER_SET_FLAG(flag, is_set) state.current_flags = ((is_set) ? state.current_flags | (U32)(flag) : state.current_flags & ~(U32)flag)

typedef struct Parser_State
{
    Worker_Context* context;
    Lexer* lexer;
    
    Scope* current_scope;
    bool allow_scope_directives;
    Flag32(EXPRESSION_FLAG) current_flags;
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
    Expression* expression = Array_Append(&state.current_scope->block.expressions);
    expression->kind  = kind;
    expression->flags = state.current_flags;
    
    Memory_Arena* arena  = &state.context->arena;
    U32 default_capacity = 4; // TODO(soimn): Find a reasonable value for this
    
    switch (kind)
    {
        case ExprKind_TypeLiteral:
        case ExprKind_Call:
        expression->call_expr.args          = ARRAY_INIT(arena, Named_Expression, default_capacity);
        break;
        
        case ExprKind_Proc:
        expression->proc_expr.args          = ARRAY_INIT(arena, Named_Expression, default_capacity);
        expression->proc_expr.return_values = ARRAY_INIT(arena, Named_Expression, default_capacity);
        break;
        
        case ExprKind_Struct:
        expression->struct_expr.args        = ARRAY_INIT(arena, Named_Expression, default_capacity);
        expression->struct_expr.members     = ARRAY_INIT(arena, Named_Expression, default_capacity);
        break;
        
        case ExprKind_Enum:
        expression->enum_expr.args          = ARRAY_INIT(arena, Named_Expression, default_capacity);
        expression->enum_expr.members       = ARRAY_INIT(arena, Named_Expression, default_capacity);
        break;
    }
    
    return expression;
}

Identifier
PushIdentifier(Parser_State state, String string)
{
    Identifier identifier = NULL_IDENTIFIER;
    
    Workspace* workspace = state.context->workspace;
    
    LockMutex(workspace->string_mutex);
    
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
    
    UnlockMutex(workspace->string_mutex);
    
    return identifier;
}

bool
ParseHexValue(Parser_State state, Text_Pos pos, String string, U32* value)
{
    bool encountered_errors = false;
    
    ASSERT(string.size <= 8);
    
    for (UMM i = 0; i < string.size; ++i)
    {
        char c = ToUpper(string.data[i]);
        
        Text_Interval char_interval;
        char_interval.position        = pos;
        char_interval.position.column = i;
        char_interval.size            = 1;
        
        if (IsDigit(c))
        {
            *value *= 16;
            *value += c - '0';
        }
        
        else if (c >= 'A' && c <= 'F')
        {
            *value *= 16;
            *value += c - 'A';
        }
        
        else
        {
            ParserReport(state, ReportSeverity_Error, char_interval,
                         "Invalid character in hexadecimal number");
            
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

bool
PushStringLiteral(Parser_State state, Token token, String_Literal* literal)
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
                            if (token.string.size >= (i + 1) + 2)
                            {
                                String hex_value_string = {
                                    .data = token.string.data + (i + 1),
                                    .size = 2
                                };
                                
                                Text_Pos hex_value_pos = token.text.position;
                                hex_value_pos.column  += i + 1;
                                
                                U32 value;
                                if (ParseHexValue(state, hex_value_pos, hex_value_string, &value))
                                {
                                    processed_string.data[processed_string.size] = (U8)value;
                                    processed_string.size                       += 1;
                                }
                                
                                else encountered_errors = true;
                            }
                            
                            else
                            {
                                Text_Interval report_interval = token.text;
                                report_interval.position.column += i - 1;
                                report_interval.size             = 2 + 2;
                                
                                ParserReport(state, ReportSeverity_Error, report_interval,
                                             "Invalid number of digits in hexadecimal number after 'x' in byte escape sequence");
                                
                                encountered_errors = true;
                            }
                        }
                        
                        else if (ToUpper(token.string.data[i]) == 'U')
                        {
                            U32 value = 0;
                            
                            if (token.string.data[i] == 'u')
                            {
                                if (token.string.size >= (i + 1) + 4)
                                {
                                    String hex_value_string = {
                                        .data = token.string.data + (i + 1),
                                        .size = 4
                                    };
                                    
                                    Text_Pos hex_value_pos = token.text.position;
                                    hex_value_pos.column  += i + 1;
                                    
                                    if (!ParseHexValue(state, hex_value_pos, hex_value_string, &value))
                                    {
                                        encountered_errors = true;
                                    }
                                }
                                
                                else
                                {
                                    Text_Interval report_interval = token.text;
                                    report_interval.position.column += i - 1;
                                    report_interval.size             = 2 + 4;
                                    
                                    ParserReport(state, ReportSeverity_Error, report_interval,
                                                 "Invalid number of digits in hexadecimal number after 'u' in codepoint escape sequence");
                                    
                                    encountered_errors = true;
                                }
                            }
                            
                            else
                            {
                                if (token.string.size >= (i + 1) + 6)
                                {
                                    String hex_value_string = {
                                        .data = token.string.data + (i + 1),
                                        .size = 6
                                    };
                                    
                                    Text_Pos hex_value_pos = token.text.position;
                                    hex_value_pos.column  += i + 1;
                                    
                                    if (!ParseHexValue(state, hex_value_pos, hex_value_string, &value))
                                    {
                                        encountered_errors = true;
                                    }
                                }
                                
                                else
                                {
                                    Text_Interval report_interval = token.text;
                                    report_interval.position.column += i - 1;
                                    report_interval.size             = 2 + 6;
                                    
                                    ParserReport(state, ReportSeverity_Error, report_interval,
                                                 "Invalid number of digits in hexadecimal number after 'U' in codepoint escape sequence");
                                    
                                    encountered_errors = true;
                                }
                            }
                            
                            if (!encountered_errors)
                            {
                                if (value <= 0x7F)
                                {
                                    processed_string.data[processed_string.size] = (U8)value;
                                    processed_string.size                       += 1;
                                }
                                
                                else if (value <= 0x7FF)
                                {
                                    U8* output             = &processed_string.data[processed_string.size];
                                    processed_string.size += 2;
                                    
                                    output[0] = (U8)(0xC0 | ((value & 0x7C0) >> 6));
                                    output[1] = (U8)(0x80 | value & 0x03F);
                                }
                                
                                else if (value <= 0xFFFF)
                                {
                                    U8* output             = &processed_string.data[processed_string.size];
                                    processed_string.size += 3;
                                    
                                    output[0] = (U8)(0xE0 | ((value & 0xF000) >> 12));
                                    output[1] = (U8)(0x80 | ((value & 0x0FC0) >> 6));
                                    output[2] = (U8)(0x80 | value & 0x3F);
                                }
                                
                                else if (value <= 0x10FFFF)
                                {
                                    U8* output             = &processed_string.data[processed_string.size];
                                    processed_string.size += 4;
                                    
                                    output[0] = (U8)(0xF0 | ((value & 0x1C0000) >> 18));
                                    output[1] = (U8)(0x80 | ((value & 0x03F000) >> 12));
                                    output[2] = (U8)(0x80 | ((value & 0x000FC0) >> 6));
                                    output[3] = (U8)(0x80 | value & 0x3F);
                                }
                                
                                else
                                {
                                    // NOTE(soimn): This can _only_ happen with the 6 digit hex value
                                    
                                    Text_Interval report_interval = token.text;
                                    report_interval.position.column += i - 1;
                                    report_interval.size             = 2 + 6;
                                    
                                    ParserReport(state, ReportSeverity_Error, report_interval,
                                                 "Invalid codepoint. Codepoint escape sequence encodes a codepoint outside the bounds of UTF8");
                                    
                                    encountered_errors = true;
                                }
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
    
    LockMutex(workspace->string_mutex);
    
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
    
    UnlockMutex(workspace->string_mutex);
    
    MemoryArena_FreeSize(&state.context->arena, buffer.data, (U32)buffer.size);
    
    return !encountered_errors;
}

inline bool
ParseCharacter(Parser_State state, Token token, Character* character)
{
    ASSERT(token.kind == Token_String);
    
    bool encountered_errors = false;
    
    // TODO(soimn): Better error reports with the text interval encompassing the #char directive and the string literal
    
    if (token.string.size == 0)
    {
        ParserReport(state, ReportSeverity_Error, token.text,
                     "Cannot convert a string literal of zero length to a character literal");
        
        encountered_errors = true;
    }
    
    else if (token.string.size == 1)
    {
        *character = token.string.data[0];
    }
    
    else
    {
        NOT_IMPLEMENTED;
    }
    
    return !encountered_errors;
}

bool
ParserImportSourceFile(Parser_State state, Token token, File_ID* file_id)
{
    bool encountered_errors = false;
    
    String_Literal literal = 0;
    
    if (PushStringLiteral(state, token, &literal))
    {
        String string = *(String*)BucketArray_ElementAt(&state.context->workspace->string_storage, literal);
        
        I64 colon_pos = StringFind(string, ':', FORWARDS);
        
        String mounting_point_name = string;
        String file_path           = string;
        
        bool has_mounting_point = false;
        bool has_file_ext       = false;
        
        Text_Pos file_path_position = token.text.position;
        file_path_position.column += 1;
        
        Text_Interval mounting_point_name_interval = TextInterval(token.text.position, 0);
        
        UMM path_start_pos = 0;
        if (colon_pos != -1)
        {
            has_mounting_point       = true;
            mounting_point_name.size = (UMM)colon_pos;
            
            file_path.data += (UMM)colon_pos;
            file_path.size -= (UMM)colon_pos;
            
            path_start_pos = (UMM)colon_pos + 1;
            
            for (U32 i = 0, j = 1; i < mounting_point_name.size + 1; )
            {
                char c = token.string.data[j];
                
                if (c > I8_MAX)
                {
                    U8 bytes_to_skip = 2;
                    
                    if      ((c & 0xF0) == 0xF0) bytes_to_skip = 4;
                    else if ((c & 0xF0) == 0x14) bytes_to_skip = 3;
                    
                    j += bytes_to_skip;
                }
                
                else if (c == '\\')
                {
                    if (token.string.data[j + 1] == '\\')     j += 2;
                    else if (token.string.data[j + 1] == 'u') j += 6;
                    else if (token.string.data[j + 1] == 'U') j += 10;
                    else                                      j += 1;
                }
                
                else j += 1;
                
                if (i == mounting_point_name.size)
                {
                    mounting_point_name_interval.size = j - 1;
                }
            }
        }
        
        if (string.size - path_start_pos != 0)
        {
            I64 period_pos = StringFind(file_path, '.', BACKWARDS);
            
            UMM file_path_size_wo_ext = file_path.size;
            if (period_pos != -1)
            {
                has_file_ext          = true;
                file_path_size_wo_ext = (UMM)period_pos;
                
                if (!StringConstStringCompare(AdvanceString(file_path, period_pos), CONST_STRING(GREMLIN_FILE_EXT)))
                {
                    ParserReport(state, ReportSeverity_Error, token.text,
                                 "Cannot import a file with the '.%S' extension. Only '.gr' files can be imported as source files", AdvanceString(file_path, period_pos));
                    
                    encountered_errors = true;
                }
            }
            
            if (!encountered_errors)
            {
                if (file_path_size_wo_ext != 0)
                {
                    UMM one_past_last_slash = 0;
                    for (UMM i = 0, j = 1; !encountered_errors && i < file_path_size_wo_ext; ++i)
                    {
                        char c = (char)file_path.data[i];
                        
                        if (IsAlpha(c) || IsDigit(c))
                        {
                            ++j;
                        }
                        
                        else if (c > I8_MAX)
                        {
                            char first_char = token.string.data[j];
                            
                            if (first_char == '\\')
                            {
                                if (token.string.data[j + 1] == 'u') j += 6;
                                else
                                {
                                    ASSERT(token.string.data[j + 1] == 'U');
                                    j += 10;
                                }
                            }
                            
                            U8 bytes_to_skip = 2;
                            
                            if      ((c & 0xF0) == 0xF0) bytes_to_skip = 4;
                            else if ((c & 0xF0) == 0x14) bytes_to_skip = 3;
                            
                            if (first_char != '\\') j += bytes_to_skip;
                            i += bytes_to_skip - 1;
                        }
                        
                        else if (c == '/')
                        {
                            if (i - one_past_last_slash != 0)
                            {
                                ++j;
                                one_past_last_slash = i + 1;
                            }
                            
                            else
                            {
                                Text_Pos shifted_pos = file_path_position;
                                shifted_pos.column  += j;
                                Text_Interval report_interval = TextInterval(shifted_pos, 1);
                                
                                ParserReport(state, ReportSeverity_Error, report_interval,
                                             "Missing directory name between before path separator in import path string");
                                
                                encountered_errors = true;
                            }
                        }
                        
                        else if (c == '\\')
                        {
                            Text_Pos shifted_pos = file_path_position;
                            shifted_pos.column  += j;
                            Text_Interval report_interval = TextInterval(shifted_pos, 1);
                            
                            ParserReport(state, ReportSeverity_Error, report_interval,
                                         "Illegal use of backslash in import path string. Forward slash is the only legal path separator");
                            
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            Text_Pos shifted_pos = file_path_position;
                            shifted_pos.column  += j;
                            Text_Interval report_interval = TextInterval(shifted_pos, 1);
                            
                            ParserReport(state, ReportSeverity_Error, report_interval,
                                         "Illegal character in import path string");
                            
                            encountered_errors = true;
                        }
                    }
                    
                    if (file_path.data[file_path.size - 1] == '/') file_path.size -= 1;
                    
                    String prefix_path = state.context->workspace->build_options.default_mounting_point_path;
                    if (!encountered_errors && has_mounting_point)
                    {
                        Array(Mounting_Point)* mounting_points;
                        mounting_points = &state.context->workspace->build_options.mounting_points;
                        
                        bool found_mpoint = false;
                        for (U32 i = 0; i < mounting_points->size; ++i)
                        {
                            Mounting_Point* mpoint = Array_ElementAt(mounting_points, i);
                            
                            if (StringCompare(mounting_point_name, mpoint->name))
                            {
                                prefix_path  = mpoint->path;
                                found_mpoint = true;
                                break;
                            }
                        }
                        
                        if (!found_mpoint)
                        {
                            ParserReport(state, ReportSeverity_Error, mounting_point_name_interval,
                                         "No registered mounting point bears the name '%S'", mounting_point_name);
                            
                            encountered_errors = true;
                        }
                    }
                    
                    if (!encountered_errors)
                    {
                        // NOTE(soimn): It may be beneficial to split the context arena into a persistent and 
                        //              temporary part
                        String full_path;
                        full_path.size = prefix_path.size + file_path.size;
                        full_path.data = MemoryArena_AllocSize(&state.context->arena, full_path.size,
                                                               ALIGNOF(U8));
                        
                        Copy(prefix_path.data, full_path.data, prefix_path.size);
                        Copy(file_path.data, full_path.data + prefix_path.size, file_path.size);
                        
                        if (has_file_ext)
                        {
                            File_Handle handle;
                            if (OpenFileForReading(full_path, &handle))
                            {
                                AddSourceFile(state.context->workspace, full_path);
                                CloseFileHandle(&handle);
                            }
                            
                            else
                            {
                                ParserReport(state, ReportSeverity_Error, token.text,
                                             "Failed to open the file '%S'", full_path);
                                
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            if (DoesDirectoryExist(full_path))
                            {
                                AddAllSourceFilesInDirectory(state.context->workspace, full_path);
                            }
                            
                            else
                            {
                                ParserReport(state, ReportSeverity_Error, token.text,
                                             "Failed to import directory '%S', as it does not exist", full_path);
                                
                                encountered_errors = true;
                            }
                        }
                        
                        MemoryArena_FreeSize(&state.context->arena, full_path.data, (U32)full_path.size);
                    }
                }
                
                else
                {
                    ParserReport(state, ReportSeverity_Error, token.text,
                                 "Missing path before file extension in import path string");
                    
                    encountered_errors = true;
                }
            }
        }
        
        else
        {
            ParserReport(state, ReportSeverity_Error, token.text, "Missing file path in import path string");
            
            encountered_errors = true;
        }
    }
    
    else encountered_errors = true;
    
    return !encountered_errors;
}

void
ParserInitScope(Parser_State state, Scope* scope)
{
    scope->symbols           = ARRAY_INIT(&state.context->arena, Symbol,     4);
    scope->block.statements  = ARRAY_INIT(&state.context->arena, Statement,  4);
    scope->block.expressions = ARRAY_INIT(&state.context->arena, Expression, 8);
    
    if (state.current_scope != 0)
    {
        scope->block.export_by_default = state.current_scope->block.export_by_default;
    }
    else scope->block.export_by_default = state.context->workspace->build_options.export_by_default;
    
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
bool ParseStatement(Parser_State state, Statement* statement);
bool ParseBlock(Parser_State state, Block* block, bool allow_single_statement, bool is_transparent);

enum COMPILER_DIRECTIVE_KIND
{
    Directive_BoundsCheck = 0x0001,
    Directive_Run         = 0x0002,
    Directive_Type        = 0x0004,
    Directive_Distinct    = 0x0008,
    Directive_Strict      = 0x0010,
    Directive_Packed      = 0x0020,
    Directive_Align       = 0x0040,
    Directive_Inline      = 0x0080,
    Directive_Foreign     = 0x0100,
    Directive_NoDiscard   = 0x0200,
    Directive_Deprecated  = 0x0400,
    Directive_Char        = 0x0800,
    Directive_If          = 0x1000,
    Directive_ScopeExport = 0x2000,
    
    COMPILER_DIRECTIVE_KIND_COUNT
};

typedef struct Compiler_Directive_Info
{
    Enum32(COMPILER_DIRECTIVE_KIND) kind;
    
    union
    {
        // BoundsCheck, Strict, Packed, Inline, ScopeExport
        bool is_set;
        
        Expression* align_expression;
        Expression* foreign_library;
        Expression* deprecation_notice;
        
        Character character;
        
        struct
        {
            Expression* condition;
            Scope true_scope;
            Scope false_scope;
        } const_if;
    };
} Compiler_Directive_Info;

bool
ParseCompilerDirective(Parser_State state, Compiler_Directive_Info* info, bool consume_expression_directives)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    ASSERT(token.kind == Token_Hash);
    
    Text_Pos start_of_directive = token.text.position;
    
    Token peek = PeekNextToken(state.lexer, token);
    
    if (peek.kind == Token_Identifier)
    {
        if (StringConstStringCompare(peek.string, CONST_STRING("bounds_check")))
        {
            info->kind   = Directive_BoundsCheck;
            info->is_set = true;
            
            SkipPastToken(state.lexer, peek);
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("no_bounds_check")))
        {
            info->kind   = Directive_BoundsCheck;
            info->is_set = false;
            
            SkipPastToken(state.lexer, peek);
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("run")))
        {
            info->kind = Directive_Run;
            
            SkipPastToken(state.lexer, peek);
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("type")))
        {
            info->kind = Directive_Type;
            
            SkipPastToken(state.lexer, peek);
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("distinct")))
        {
            info->kind = Directive_Distinct;
            
            SkipPastToken(state.lexer, peek);
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("strict")))
        {
            info->kind   = Directive_Strict;
            info->is_set = true;
            
            SkipPastToken(state.lexer, peek);
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("loose")))
        {
            info->kind   = Directive_Strict;
            info->is_set = false;
            
            SkipPastToken(state.lexer, peek);
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("packed")))
        {
            info->kind   = Directive_Packed;
            info->is_set = true;
            
            SkipPastToken(state.lexer, peek);
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("padded")))
        {
            info->kind   = Directive_Packed;
            info->is_set = false;
            
            SkipPastToken(state.lexer, peek);
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("align")))
        {
            info->kind = Directive_Align;
            
            SkipPastToken(state.lexer, peek);
            
            if (!ParseExpression(state, &info->align_expression, false))
            {
                encountered_errors = true;
            }
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("inline")))
        {
            info->kind   = Directive_Inline;
            info->is_set = true;
            
            SkipPastToken(state.lexer, peek);
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("no_inline")))
        {
            info->kind   = Directive_Inline;
            info->is_set = false;
            
            SkipPastToken(state.lexer, peek);
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("foreign")))
        {
            info->kind = Directive_Foreign;
            
            SkipPastToken(state.lexer, peek);
            
            if (!ParseExpression(state, &info->foreign_library, false))
            {
                encountered_errors = true;
            }
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("no_discard")))
        {
            info->kind = Directive_NoDiscard;
            
            SkipPastToken(state.lexer, peek);
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("deprecated")))
        {
            info->kind = Directive_Deprecated;
            
            SkipPastToken(state.lexer, peek);
            
            if (!ParseExpression(state, &info->deprecation_notice, false))
            {
                encountered_errors = true;
            }
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("char")))
        {
            info->kind = Directive_Char;
            
            if (consume_expression_directives)
            {
                SkipPastToken(state.lexer, peek);
                token = GetToken(state.lexer);
                
                if (peek.kind == Token_String)
                {
                    if (ParseCharacter(state, token, &info->character))
                    {
                        SkipPastToken(state.lexer, token);
                    }
                    
                    else encountered_errors = true;
                }
                
                else
                {
                    Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive,
                                                                              token.text.position);
                    
                    ParserReport(state, ReportSeverity_Error, report_interval,
                                 "Missing string literal after 'char' directive");
                    
                    encountered_errors = true;
                }
            }
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("scope_export")))
        {
            info->kind   = Directive_ScopeExport;
            info->is_set = true;
            
            SkipPastToken(state.lexer, peek);
        }
        
        else if (StringConstStringCompare(peek.string, CONST_STRING("scope_file")))
        {
            info->kind   = Directive_ScopeExport;
            info->is_set = false;
            
            SkipPastToken(state.lexer, peek);
        }
        
        else
        {
            Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive, 
                                                                      peek.text.position);
            
            ParserReport(state, ReportSeverity_Error, report_interval,
                         "Missing name of compiler directive after '#' token");
            
            encountered_errors = true;
        }
    }
    
    else if (peek.kind == Token_Keyword && peek.keyword == Keyword_If)
    {
        info->kind = Directive_If;
        
        SkipPastToken(state.lexer, peek);
        
        if (ParseExpression(state, &info->const_if.condition, false))
        {
            ParserInitScope(state, &info->const_if.true_scope);
            // TODO(soimn): Is there a reason for not allowing single statement const if statements?
            if (ParseBlock(state, &info->const_if.true_scope.block, true, true))
            {
                token = GetToken(state.lexer);
                
                if (token.kind == Token_Keyword && token.keyword == Keyword_Else)
                {
                    SkipPastToken(state.lexer, token);
                    
                    ParserInitScope(state, &info->const_if.false_scope);
                    if (!ParseBlock(state, &info->const_if.false_scope.block, true, true))
                    {
                        encountered_errors = true;
                    }
                }
            }
            
            else encountered_errors = true;
        }
        
        else encountered_errors = true;
    }
    
    else
    {
        Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive, 
                                                                  peek.text.position);
        
        ParserReport(state, ReportSeverity_Error, report_interval,
                     "Missing name of compiler directive after '#' token");
        
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

bool
ParseCodeNotes(Parser_State state, Array(Code_Note)* note_array)
{
    bool encountered_errors = false;
    
    for (;;)
    {
        
        Token token = GetToken(state.lexer);
        ASSERT(token.kind == Token_At);
        
        Text_Interval at_text = token.text;
        
        SkipPastToken(state.lexer, token);
        token = GetToken(state.lexer);
        
        if (token.kind == Token_Identifier)
        {
            Code_Note* note = Array_Append(note_array);
            
            note->name = token.string;
            note->text = TextIntervalMerge(at_text, token.text);
            
            SkipPastToken(state.lexer, token);
        }
        
        else
        {
            Text_Interval report_interval = TextIntervalMerge(at_text, token.text);
            
            ParserReport(state, ReportSeverity_Error, report_interval,
                         "Missing name of code note");
            
            encountered_errors = true;
        }
        
        token = GetToken(state.lexer);
        if (token.kind == Token_At) continue;
        else                        break;
    }
    
    return !encountered_errors;
}

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
            expr->is_using = true;
            
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
                    Text_Interval report_interval = TextIntervalFromEndpoints(slice_pointer->text.position, 
                                                                              token.text.position);
                    
                    ParserReport(state, ReportSeverity_Error, report_interval, 
                                 "Missing matching closing bracket before end of statement");
                    
                    encountered_errors = true;
                }
            }
        }
        
        else if (token.kind == Token_OpenBrace)
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
    Text_Pos start_of_expression = token.text.position;
    
    Flag32(COMPILER_DIRECTIVE_KIND) encountered_directives = 0;
    
    while (!encountered_errors && token.kind == Token_Hash && *result == 0)
    {
        Compiler_Directive_Info info = {0};
        
        Text_Pos  start_of_directive = token.text.position;
        Token name_token = PeekNextToken(state.lexer, token);
        
        if (ParseCompilerDirective(state, &info, true))
        {
            if ((info.kind & encountered_directives) == 0)
            {
                encountered_directives |= info.kind;
                
                if (info.kind == Directive_BoundsCheck)   PARSER_SET_FLAG(ExprFlag_BoundsCheck, info.is_set);
                else if (info.kind == Directive_Type)     PARSER_SET_FLAG(ExprFlag_TypeEvalContext, info.is_set);
                else if (info.kind == Directive_Distinct) PARSER_SET_FLAG(ExprFlag_DistinctType, info.is_set);
                else if (info.kind == Directive_Inline)
                {
                    Expression* expr = 0;
                    if (ParseInfixAndPostfixCallExpression(state, &expr))
                    {
                        Expression* call_expr = expr;
                        
                        if (call_expr->kind != ExprKind_Call)
                        {
                            while (call_expr->kind == ExprKind_Not              ||
                                   call_expr->kind == ExprKind_BitNot           ||
                                   call_expr->kind == ExprKind_Neg              ||
                                   call_expr->kind == ExprKind_Reference        ||
                                   call_expr->kind == ExprKind_Dereference      ||
                                   call_expr->kind == ExprKind_SliceType        ||
                                   call_expr->kind == ExprKind_ArrayType        ||
                                   call_expr->kind == ExprKind_DynamicArrayType ||
                                   call_expr->kind == ExprKind_Polymorphic)
                            {
                                call_expr = call_expr->unary_expr.operand;
                            }
                            
                            while (call_expr->kind == ExprKind_Member)
                            {
                                call_expr = call_expr->binary_expr.left;
                            }
                        }
                        
                        if (call_expr->kind == ExprKind_Call)
                        {
                            if (info.is_set) call_expr->call_expr.forced_inline = TRI_TRUE;
                            else             call_expr->call_expr.forced_inline = TRI_FALSE;
                            
                            *result = expr;
                        }
                        
                        else
                        {
                            ParserReport(state, ReportSeverity_Error, call_expr->text,
                                         "Cannot apply the 'inline' directive on an expression that is not a procedure call");
                            
                            encountered_errors = true;
                        }
                    }
                    
                    else encountered_errors = true;
                }
                
                else if (info.kind == Directive_Run)
                {
                    *result = PushExpression(state, ExprKind_Run);
                    
                    if (!ParsePrimaryExpression(state, &(*result)->run_expr))
                    {
                        encountered_errors = true;
                    }
                }
                
                else if (info.kind == Directive_Char)
                {
                    *result = PushExpression(state, ExprKind_Character);
                    (*result)->character = info.character;
                }
                
                else
                {
                    token = GetToken(state.lexer);
                    
                    Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive,
                                                                              token.text.position);
                    
                    ParserReport(state, ReportSeverity_Error, report_interval, "Invalid use of compiler directive. The '%S' directive cannot be used in an expression context", name_token.string);
                    
                    encountered_errors = true;
                }
            }
            
            else
            {
                token = GetToken(state.lexer);
                
                Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive,
                                                                          token.text.position);
                
                ParserReport(state, ReportSeverity_Error, report_interval,
                             "Duplicate use of the '%S' directive", name_token.string);
                
                encountered_errors = true;
            }
        }
        
        else encountered_errors = true;
    }
    
    if (!encountered_errors && *result == 0)
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
                
                Flag32(COMPILER_DIRECTIVE_KIND) encountered_proc_directives = 0;
                
                while (!encountered_errors)
                {
                    token = GetToken(state.lexer);
                    
                    if (token.kind == Token_Hash)
                    {
                        Compiler_Directive_Info info = {0};
                        
                        Text_Pos start_of_directive = token.text.position;
                        Token name_token = PeekNextToken(state.lexer, token);
                        
                        if (ParseCompilerDirective(state, &info, true))
                        {
                            if ((info.kind & encountered_proc_directives) == 0)
                            {
                                encountered_proc_directives |= info.kind;
                                
                                if (info.kind == Directive_BoundsCheck) PARSER_SET_FLAG(ExprFlag_BoundsCheck,
                                                                                        info.is_set);
                                
                                else if (info.kind == Directive_Deprecated)
                                {
                                    (*result)->proc_expr.is_deprecated      = true;
                                    (*result)->proc_expr.deprecation_notice = info.deprecation_notice;
                                }
                                
                                else if (info.kind == Directive_NoDiscard)
                                {
                                    (*result)->proc_expr.no_discard = true;
                                }
                                
                                else if (info.kind == Directive_Foreign)
                                {
                                    (*result)->proc_expr.is_foreign       = true;
                                    (*result)->proc_expr.foreign_library = info.foreign_library;
                                }
                                
                                else if (info.kind == Directive_Inline)
                                {
                                    (*result)->proc_expr.is_inlined = info.is_set;
                                }
                                
                                else
                                {
                                    token = GetToken(state.lexer);
                                    
                                    Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive, 
                                                                                              token.text.position);
                                    
                                    ParserReport(state, ReportSeverity_Error, report_interval,
                                                 "Invalid use of compiler directive. The '%S' directive cannot be used to tag a procedure", name_token.string);
                                    
                                    encountered_errors = true;
                                }
                            }
                            
                            else
                            {
                                token = GetToken(state.lexer);
                                
                                Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive,
                                                                                          token.text.position);
                                
                                ParserReport(state, ReportSeverity_Error, report_interval,
                                             "Duplicate use of the '%S' directive", name_token.string);
                                
                                encountered_errors = true;
                            }
                        }
                        
                        else encountered_errors = true;
                    }
                }
                
                token = GetToken(state.lexer);
                (*result)->text = TextIntervalFromEndpoints(start_of_procedure_header, token.text.position);
                
                if (!encountered_errors)
                {
                    token = GetToken(state.lexer);
                    
                    if (token.kind == Token_OpenBrace)
                    {
                        (*result)->proc_expr.has_body = true;
                        ParserInitScope(state, &(*result)->proc_expr.scope);
                        
                        if (!ParseBlock(state, &(*result)->proc_expr.scope.block, false, false))
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
                
                Flag32(COMPILER_DIRECTIVE_KIND) encountered_struct_directives = 0;
                
                while (!encountered_errors)
                {
                    token = GetToken(state.lexer);
                    
                    if (token.kind == Token_Hash)
                    {
                        Compiler_Directive_Info info = {0};
                        
                        Text_Pos start_of_directive = token.text.position;
                        Token name_token = PeekNextToken(state.lexer, token);
                        
                        if (ParseCompilerDirective(state, &info, true))
                        {
                            if ((info.kind & encountered_struct_directives) == 0)
                            {
                                encountered_struct_directives |= info.kind;
                                
                                if (info.kind == Directive_Align)
                                {
                                    (*result)->struct_expr.alignment = info.align_expression;
                                }
                                
                                else if (info.kind == Directive_Packed)
                                {
                                    (*result)->struct_expr.is_packed = info.is_set;
                                }
                                
                                else if (info.kind == Directive_Strict)
                                {
                                    (*result)->struct_expr.is_strict = info.is_set;
                                }
                                
                                else
                                {
                                    token = GetToken(state.lexer);
                                    
                                    Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive, 
                                                                                              token.text.position);
                                    
                                    ParserReport(state, ReportSeverity_Error, report_interval,
                                                 "Invalid use of compiler directive. The '%S' directive cannot be used to tag a structure", name_token.string);
                                    
                                    encountered_errors = true;
                                }
                            }
                            
                            else
                            {
                                token = GetToken(state.lexer);
                                
                                Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive,
                                                                                          token.text.position);
                                
                                ParserReport(state, ReportSeverity_Error, report_interval,
                                             "Duplicate use of the '%S' directive", name_token.string);
                                
                                encountered_errors = true;
                            }
                        }
                        
                        else encountered_errors = true;
                    }
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
                
                Flag32(COMPILER_DIRECTIVE_KIND) encountered_enum_directives = 0;
                
                while (!encountered_errors)
                {
                    token = GetToken(state.lexer);
                    
                    if (token.kind == Token_Hash)
                    {
                        Compiler_Directive_Info info = {0};
                        
                        Text_Pos start_of_directive = token.text.position;
                        Token name_token = PeekNextToken(state.lexer, token);
                        
                        if (ParseCompilerDirective(state, &info, true))
                        {
                            if ((info.kind & encountered_enum_directives) == 0)
                            {
                                encountered_enum_directives |= info.kind;
                                
                                if (info.kind == Directive_Strict)
                                {
                                    (*result)->enum_expr.is_strict = info.is_set;
                                }
                                
                                else
                                {
                                    token = GetToken(state.lexer);
                                    
                                    Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive, 
                                                                                              token.text.position);
                                    
                                    ParserReport(state, ReportSeverity_Error, report_interval,
                                                 "Invalid use of compiler directive. The '%S' directive cannot be used to tag an enumeration", name_token.string);
                                    
                                    encountered_errors = true;
                                }
                            }
                            
                            else
                            {
                                token = GetToken(state.lexer);
                                
                                Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive,
                                                                                          token.text.position);
                                
                                ParserReport(state, ReportSeverity_Error, report_interval,
                                             "Duplicate use of the '%S' directive", name_token.string);
                                
                                encountered_errors = true;
                            }
                        }
                        
                        else encountered_errors = true;
                    }
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
            
            else if (token.keyword == Keyword_True ||
                     token.keyword == Keyword_False ||
                     token.keyword == Keyword_Nil)
            
            {
                *result = PushExpression(state, ExprKind_Trilean);
                (*result)->text = token.text;
                
                if      (token.keyword == Keyword_True)  (*result)->trilean = TRI_TRUE;
                else if (token.keyword == Keyword_False) (*result)->trilean = TRI_FALSE;
                else                                     (*result)->trilean = TRI_NIL;
                
                SkipPastToken(state.lexer, token);
            }
            
            else
            {
                ParserReport(state, ReportSeverity_Error, token.text,
                             "Invalid use of keyword in expression");
                
                encountered_errors = true;
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
            *result = PushExpression(state, ExprKind_String);
            
            if (PushStringLiteral(state, token, &(*result)->string))
            {
                (*result)->text = token.text;
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
ParseStatement(Parser_State state, Statement* statement)
{
    bool encountered_errors = false;
    bool emitted_statement  = false;
    
    Token token = GetToken(state.lexer);
    
    Flag32(COMPILER_DIRECTIVE_KIND) encountered_directives = 0;
    
    while (!encountered_errors && token.kind == Token_Hash)
    {
        Text_Pos start_of_directive = token.text.position;
        
        Token name_token = PeekNextToken(state.lexer, token);
        
        Compiler_Directive_Info info = {0};
        
        if (ParseCompilerDirective(state, &info, false))
        {
            if ((info.kind & encountered_directives) == 0)
            {
                if (info.kind != Directive_ScopeExport &&
                    info.kind != Directive_If)
                {
                    encountered_directives |= info.kind;
                }
                
                if (info.kind == Directive_BoundsCheck)   PARSER_SET_FLAG(ExprFlag_BoundsCheck, info.is_set);
                else if (info.kind == Directive_Type)     PARSER_SET_FLAG(ExprFlag_TypeEvalContext, info.is_set);
                else if (info.kind == Directive_Distinct) PARSER_SET_FLAG(ExprFlag_DistinctType, info.is_set);
                else if (info.kind == Directive_Char)
                {
                    break;
                }
                
                else if (info.kind == Directive_ScopeExport)
                {
                    if (state.allow_scope_directives) state.current_scope->block.export_by_default = info.is_set;
                    else
                    {
                        token = GetToken(state.lexer);
                        
                        Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive,
                                                                                  token.text.position);
                        
                        ParserReport(state, ReportSeverity_Error, report_interval,
                                     "Scope altering compiler directives are not allowed in this context");
                        
                        encountered_errors = true;
                    }
                }
                
                else if (info.kind == Directive_If)
                {
                    statement->kind = StatementKind_If;
                    
                    statement->if_stmnt.condition   = info.const_if.condition;
                    statement->if_stmnt.true_scope  = info.const_if.true_scope;
                    statement->if_stmnt.false_scope = info.const_if.false_scope;
                    
                    statement->if_stmnt.is_const = true;
                    
                    token = GetToken(state.lexer);
                    statement->text = TextIntervalFromEndpoints(start_of_directive, 
                                                                token.text.position);
                    
                    emitted_statement = true;
                }
                
                else
                {
                    token = GetToken(state.lexer);
                    
                    Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive,
                                                                              token.text.position);
                    
                    ParserReport(state, ReportSeverity_Error, report_interval, "Invalid use of compiler directive. The '%S' directive cannot be used in a statement context", name_token.string);
                    
                    encountered_errors = true;
                }
            }
            
            else
            {
                token = GetToken(state.lexer);
                
                Text_Interval report_interval = TextIntervalFromEndpoints(start_of_directive,
                                                                          token.text.position);
                
                ParserReport(state, ReportSeverity_Error, report_interval,
                             "Duplicate use of the '%S' directive", name_token.string);
                
                encountered_errors = true;
            }
        }
        
        else encountered_errors = true;
        
        token = GetToken(state.lexer);
    }
    
    if (!encountered_errors && !emitted_statement)
    {
        token = GetToken(state.lexer);
        
        bool encountered_using       = false;
        bool handled_using           = false;
        Text_Pos start_of_using_decl = {0};
        if (token.kind == Token_Keyword && token.keyword == Keyword_Using)
        {
            encountered_using   = true;
            start_of_using_decl = token.text.position;
            
            SkipPastToken(state.lexer, token);
        }
        
        if (token.kind == Token_Error)
        {
            encountered_errors = true;
        }
        
        else if (token.kind == Token_EndOfStream)
        {
            ParserReport(state, ReportSeverity_Error, token.text,
                         "Encountered end of file before mathing closing brace in block");
            ParserReport(state, ReportSeverity_Info, TextInterval(state.current_scope->block.text.position, 1),
                         "Location of the open brace which began the erroneous block");
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
            statement->kind = StatementKind_Scope;
            
            ParserInitScope(state, &statement->scope);
            
            if (!ParseBlock(state, &statement->scope.block, false, false))
            {
                encountered_errors = true;
            }
        }
        
        else
        {
            bool was_not_handled = false;
            
            if (token.kind == Token_Keyword)
            {
                if (token.keyword == Keyword_Import)
                {
                    Text_Pos start_of_import = token.text.position;
                    
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    statement->kind = StatementKind_ImportDecl;
                    
                    if (token.kind == Token_String)
                    {
                        if (ParserImportSourceFile(state, token, &statement->import_decl.file_id))
                        {
                            SkipPastToken(state.lexer, token);
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_Identifier)
                            {
                                statement->import_decl.alias = PushIdentifier(state, token.string);
                                
                                SkipPastToken(state.lexer, token);
                                token = GetToken(state.lexer);
                            }
                            
                            else statement->import_decl.alias = NULL_IDENTIFIER;
                            
                            if (token.kind == Token_Semicolon)
                            {
                                statement->text = TextIntervalFromEndpoints(start_of_import,
                                                                            token.text.position);
                                
                                SkipPastToken(state.lexer, token);
                            }
                            
                            else
                            {
                                Text_Interval report_interval = TextIntervalFromEndpoints(start_of_import,
                                                                                          token.text.position);
                                
                                ParserReport(state, ReportSeverity_Error, report_interval, "Missing semicolon after import declaration");
                                
                                encountered_errors = true;
                            }
                        }
                        
                        else encountered_errors = true;
                    }
                    
                    else
                    {
                        Text_Interval report_interval = TextIntervalFromEndpoints(start_of_import, 
                                                                                  token.text.position);
                        ParserReport(state, ReportSeverity_Error, report_interval,
                                     "Missing import path after 'import' keyword in import declaration");
                        
                        encountered_errors = true;
                    }
                }
                
                else if (token.keyword == Keyword_If)
                {
                    Text_Pos start_of_if = token.text.position;
                    
                    SkipPastToken(state.lexer, token);
                    
                    statement->kind = StatementKind_If;
                    
                    if (ParseExpression(state, &statement->if_stmnt.condition, false))
                    {
                        token = GetToken(state.lexer);
                        statement->text = TextIntervalFromEndpoints(start_of_if, token.text.position);
                        
                        ParserInitScope(state, &statement->if_stmnt.true_scope);
                        if (ParseBlock(state, &statement->if_stmnt.true_scope.block, true, false))
                        {
                            token = GetToken(state.lexer);
                            
                            if (token.kind == Token_Keyword && token.keyword == Keyword_Else)
                            {
                                SkipPastToken(state.lexer, token);
                                
                                ParserInitScope(state, &statement->if_stmnt.false_scope);
                                if (!ParseBlock(state, &statement->if_stmnt.false_scope.block, true, false))
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
                    
                    statement->kind = StatementKind_While;
                    
                    if (ParseExpression(state, &statement->while_stmnt.condition, false))
                    {
                        token = GetToken(state.lexer);
                        statement->text = TextIntervalFromEndpoints(start_of_while, token.text.position);
                        
                        ParserInitScope(state, &statement->while_stmnt.scope);
                        if (!ParseBlock(state, &statement->while_stmnt.scope.block, true, false))
                        {
                            encountered_errors = true;
                        }
                    }
                    
                    else encountered_errors = true;
                }
                
                else if (token.keyword == Keyword_Break || token.keyword == Keyword_Continue)
                {
                    statement->kind = (token.keyword == Keyword_Break 
                                       ? StatementKind_Break 
                                       : StatementKind_Continue);
                    
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
                    
                    statement->kind = StatementKind_Return;
                    
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
                    
                    statement->kind = StatementKind_Defer;
                    
                    ParserInitScope(state, &statement->scope);
                    if (!ParseBlock(state, &statement->scope.block, true, false))
                    {
                        encountered_errors = true;
                    }
                    
                    token = GetToken(state.lexer);
                    statement->text = TextIntervalFromEndpoints(start_of_defer, token.text.position);
                }
                
                else was_not_handled = true;
            }
            
            if (was_not_handled)
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
                                
                                if (token.kind == Token_Colon)
                                {
                                    statement->kind = StatementKind_ConstDecl;
                                    
                                    statement->const_decl.types       = types;
                                    statement->const_decl.is_exported = state.current_scope->block.export_by_default;
                                    
                                    statement->const_decl.values = ARRAY_INIT(&state.context->arena, Expression*, 4);
                                    
                                    statement->const_decl.names = ARRAY_INIT(&state.context->arena, Identifier, 4);
                                    for (U32 i = 0; i < expression_list.size; ++i)
                                    {
                                        Expression* expr = Array_ElementAt(&expression_list, i);
                                        
                                        if (expr->kind == ExprKind_Identifier)
                                        {
                                            Identifier* ident = Array_Append(&statement->const_decl.names);
                                            *ident = expr->identifier;
                                        }
                                        
                                        else if (expr->kind == ExprKind_Polymorphic)
                                        {
                                            ParserReport(state, ReportSeverity_Error, expr->text, "Constant names cannot be polymorphic");
                                            
                                            encountered_errors = true;
                                        }
                                        
                                        else
                                        {
                                            ParserReport(state, ReportSeverity_Error, expr->text, "Invalid expression for constant name");
                                            
                                            encountered_errors = true;
                                        }
                                    }
                                }
                                
                                else
                                {
                                    statement->kind = StatementKind_VarDecl;
                                    
                                    statement->var_decl.types       = types;
                                    statement->var_decl.is_using    = encountered_using;
                                    statement->var_decl.is_exported = state.current_scope->block.export_by_default;
                                    
                                    handled_using = true;
                                    
                                    statement->var_decl.values = ARRAY_INIT(&state.context->arena, Expression*, 4);
                                    
                                    statement->var_decl.names = ARRAY_INIT(&state.context->arena, Identifier, 4);
                                    for (U32 i = 0; i < expression_list.size; ++i)
                                    {
                                        Expression* expr = Array_ElementAt(&expression_list, i);
                                        
                                        if (expr->kind == ExprKind_Identifier)
                                        {
                                            Identifier* ident = Array_Append(&statement->var_decl.names);
                                            *ident = expr->identifier;
                                        }
                                        
                                        else if (expr->kind == ExprKind_Polymorphic)
                                        {
                                            ParserReport(state, ReportSeverity_Error, expr->text, "Variable names cannot be polymorphic");
                                            
                                            encountered_errors = true;
                                        }
                                        
                                        else
                                        {
                                            ParserReport(state, ReportSeverity_Error, expr->text, "Invalid expression for variable name");
                                            
                                            encountered_errors = true;
                                        }
                                    }
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
                                        if (statement->kind == StatementKind_VarDecl)
                                        {
                                            statement->var_decl.is_uninitialized = true;
                                            
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
                                        if (statement->kind == StatementKind_ConstDecl)
                                        {
                                            values = &statement->const_decl.values;
                                        }
                                        
                                        else values = &statement->var_decl.values;
                                        
                                        
                                        if (!ParseExpressionList(state, values))
                                        {
                                            encountered_errors = true;
                                        }
                                    }
                                }
                                
                                if (!encountered_errors)
                                {
                                    token = GetToken(state.lexer);
                                    
                                    if (encountered_using)
                                    {
                                        statement->text = TextIntervalFromEndpoints(start_of_using_decl, 
                                                                                    token.text.position);
                                    }
                                    
                                    else
                                    {
                                        statement->text = TextIntervalFromEndpoints(start_of_expression, 
                                                                                    token.text.position);
                                    }
                                    
                                    if (token.kind == Token_At)
                                    {
                                        Array(Code_Note)* code_notes = 0;
                                        if (statement->kind == StatementKind_ConstDecl)
                                        {
                                            code_notes = &statement->const_decl.code_note;
                                        }
                                        
                                        else code_notes = &statement->var_decl.code_note;
                                        
                                        *code_notes = ARRAY_INIT(&state.context->arena, Code_Note, 4);
                                        
                                        if (!ParseCodeNotes(state, code_notes))
                                        {
                                            encountered_errors = true;
                                        }
                                    }
                                    
                                    if (token.kind == Token_Semicolon)
                                    {
                                        SkipPastToken(state.lexer, token);
                                    }
                                    
                                    else
                                    {
                                        bool should_ignore_missing_semicolon = false;
                                        
                                        if (statement->kind == StatementKind_ConstDecl &&
                                            statement->const_decl.values.size == 1)
                                        {
                                            Expression* expr = Array_ElementAt(&statement->const_decl.values, 0);
                                            
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
                        
                        // NOTE(soimn): A lot of these checks are redundant, given that ParseExpression handles 
                        //              all cases where an assignment compatible operator is not followed by an 
                        //              equals sign. However, checking for assignment operators without infering 
                        //              the equals sign from context is a lot clearer.
                        
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
                        
                        if (is_assignment)
                        {
                            statement->kind = StatementKind_Assignment;
                            
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
                            if (encountered_using)
                            {
                                statement->kind = StatementKind_UsingDecl;
                                
                                statement->using_decl.expressions = expression_list;
                                
                                statement->text = TextIntervalFromEndpoints(start_of_using_decl,
                                                                            token.text.position);
                                
                                handled_using = true;
                            }
                            
                            else
                            {
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
                                    statement->kind = StatementKind_Expression;
                                    
                                    // NOTE(soimn): expression_list stores pointers to the expressions
                                    statement->expression_stmnt = Array_ElementAt(&expression_list, 0);
                                    Array_RemoveAllAndFreeHeader(&expression_list);
                                }
                            }
                            
                            if (!encountered_errors)
                            {
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
        
        if (!encountered_errors && encountered_errors && !handled_using)
        {
            token = GetToken(state.lexer);
            Text_Interval report_interval = TextIntervalFromEndpoints(start_of_using_decl,
                                                                      token.text.position);
            
            ParserReport(state, ReportSeverity_Error, report_interval,
                         "'using' cannot be used in this context");
            ParserReport(state, ReportSeverity_Info, report_interval,
                         "Valid use of 'using' declarations are exclusive to variable declarations, named expressions in procedures and structures, and as a prefix to member expressions");
            
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

bool
ParseBlock(Parser_State state, Block* block, bool allow_single_statement, bool is_transparent)
{
    bool encountered_errors = false;
    
    state.allow_scope_directives = (is_transparent ? state.allow_scope_directives : false);
    
    Token token = GetToken(state.lexer);
    
    Text_Pos start_of_block = token.text.position;
    
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
            Statement statement = {0};
            statement.kind = StatementKind_Invalid;
            
            if (ParseStatement(state, &statement))
            {
                if (statement.kind != StatementKind_Invalid)
                {
                    Statement* new_entry = Array_Append(&state.current_scope->block.statements);
                    *new_entry = statement;
                }
            }
            
            else encountered_errors = true;
        }
        
        if (is_single_statement) break;
        else continue;
    }
    
    token = GetToken(state.lexer);
    
    block->text = TextIntervalFromEndpoints(start_of_block, token.text.position);
    
    return !encountered_errors;
}

void
ParseFile(Worker_Context* context, File_ID file_id)
{
    Lexer lexer_state = {0};
    
    Parser_State state = {0};
    state.context                = context;
    state.lexer                  = &lexer_state;
    state.allow_scope_directives = true;
    state.current_flags          = context->workspace->build_options.default_expression_flags;
    
    
    
    Source_File* file = BucketArray_ElementAt(&context->workspace->source_files, file_id);
    
    File_Handle file_handle;
    bool did_open = OpenFileForReading(file->path, &file_handle);
    ASSERT(did_open);
    
    LockMutex(state.context->workspace->file_mutex);
    
    file->contents.size = file_handle.file_size + 1;
    file->contents.data = MemoryArena_AllocSize(&state.context->workspace->file_arena,
                                                file_handle.file_size + 1, ALIGNOF(U8));
    
    LoadFileContents(&file_handle, &file->contents);
    CloseFileHandle(&file_handle);
    
    lexer_state = LexFile(context, file_id, file->contents);
    
    Scope* scope = BucketArray_Append(&state.context->workspace->file_scopes);
    
    UnlockMutex(state.context->workspace->file_mutex);
    
    
    
    bool encountered_errors = false;
    
    while (!encountered_errors)
    {
        Token token = GetToken(state.lexer);
        Token peek  = PeekNextToken(state.lexer, token);
        
        if (token.kind == Token_EndOfStream) break;
        
        else
        {
            Statement statement = {0};
            statement.kind = StatementKind_Invalid;
            
            if (ParseStatement(state, &statement))
            {
                if (statement.kind != StatementKind_Invalid)
                {
                    Statement* new_entry = Array_Append(&state.current_scope->block.statements);
                    *new_entry = statement;
                }
            }
            
            else encountered_errors = true;
        }
    }
    
    if (!encountered_errors)
    {
        scope->block.text = TextIntervalMerge(scope->block.text, GetToken(state.lexer).text);
        
        NOT_IMPLEMENTED;
    }
}