String KeywordStrings[KEYWORD_KIND_COUNT] = {
    [Keyword_Package]  = CONST_STRING("package"),
    [Keyword_Import]   = CONST_STRING("import"),
    [Keyword_As]       = CONST_STRING("as"),
    [Keyword_Defer]    = CONST_STRING("defer"),
    [Keyword_Return]   = CONST_STRING("return"),
    [Keyword_Using]    = CONST_STRING("using"),
    [Keyword_Proc]     = CONST_STRING("proc"),
    [Keyword_Where]    = CONST_STRING("where"),
    [Keyword_Struct]   = CONST_STRING("struct"),
    [Keyword_Union]    = CONST_STRING("union"),
    [Keyword_Enum]     = CONST_STRING("enum"),
    [Keyword_If]       = CONST_STRING("if"),
    [Keyword_Else]     = CONST_STRING("else"),
    [Keyword_When]     = CONST_STRING("when"),
    [Keyword_While]    = CONST_STRING("while"),
    [Keyword_For]      = CONST_STRING("for"),
    [Keyword_Break]    = CONST_STRING("break"),
    [Keyword_Continue] = CONST_STRING("continue"),
};

void
ReportLexerError(Workspace* workspace, Text_Interval text, Text_Interval highlight, const char* message, ...)
{
    File* file = DynamicArray_ElementAt(&workspace->files, File, text.pos.file);
    
    Print("%S(%u:%u): ", file->path, highlight.pos.line, highlight.pos.column);
    
    Arg_List arg_list;
    ARG_LIST_START(arg_list, message);
    
    PrintArgList(message, arg_list);
}

bool
LexFile(Workspace* workspace, File_ID file_id)
{
    ASSERT(file_id != INVALID_FILE_ID);
    ASSERT(Arena_IsCleared(TempArena(workspace)));
    
    File* file = DynamicArray_ElementAt(&workspace->files, File, file_id);
    
    // NOTE(soimn): The lexer requires null terminated input
    ASSERT(file->content.data[file->content.size] == 0);
    
    u8* file_content   = file->content.data;
    u32 offset         = 0;
    u32 offset_to_line = 0;
    u32 line           = 0;
    
#define CURRENT_TOKEN_SIZE(token, offset)\
((offset) - ((token)->text.pos.column + (token)->text.pos.offset_to_line))
    
    bool encountered_errors = false;
    while (!encountered_errors)
    {
        Token* token = Arena_Allocate(TempArena(workspace), sizeof(Token), ALIGNOF(Token));
        
        bool is_comment        = false;
        umm comment_nest_level = 0;
        while (file_content[offset] != 0)
        {
            if (file_content[offset] == '\n')
            {
                offset_to_line = offset;
                line          += 1;
                
                offset += 1;
            }
            
            else if (file_content[offset] == ' '  ||
                     file_content[offset] == '\r' ||
                     file_content[offset] == '\t' ||
                     file_content[offset] == '\v' ||
                     file_content[offset] == '\f')
            {
                offset += 1;
            }
            
            else if (!is_comment && (file_content[offset]     == '/' &&
                                     file_content[offset + 1] == '/'))
            {
                is_comment = true;
                
                offset += 2;
            }
            
            else if (file_content[offset] == '/' && file_content[offset + 1] == '*')
            {
                offset += 2;
                
                if (comment_nest_level != 0 || !is_comment)
                {
                    comment_nest_level += 1;
                }
            }
            
            else if (file_content[offset] == '*' && file_content[offset + 1] == '/')
            {
                offset += 2;
                
                if (comment_nest_level != 0)
                {
                    comment_nest_level -= 1;
                    
                    if (comment_nest_level == 0) break;
                }
            }
            
            else break;
        }
        
        token->text.pos.file           = file_id;
        token->text.pos.offset_to_line = offset_to_line;
        token->text.pos.line           = line;
        token->text.pos.column         = offset - offset_to_line;
        
        char c = file_content[offset];
        offset += (c != 0 ? 1 : 0);
        
        switch (c)
        {
            case 0: token->kind = Token_EndOfStream; break;
            
            case '"':  token->kind = Token_Plus;         break;
            case '-':  token->kind = Token_Minus;        break;
            case '*':  token->kind = Token_Asterisk;     break;
            case '/':  token->kind = Token_Slash;        break;
            case '%':  token->kind = Token_Percentage;   break;
            case '!':  token->kind = Token_Exclamation;  break;
            case '@':  token->kind = Token_At;           break;
            case '#':  token->kind = Token_Hash;         break;
            case '$':  token->kind = Token_Cash;         break;
            case '&':  token->kind = Token_Ampersand;    break;
            case '=':  token->kind = Token_Equals;       break;
            case '?':  token->kind = Token_QuestionMark; break;
            case '|':  token->kind = Token_Pipe;         break;
            case '~':  token->kind = Token_Tilde;        break;
            case '^':  token->kind = Token_Hat;          break;
            case '\'': token->kind = Token_Quote;        break;
            case ':':  token->kind = Token_Colon;        break;
            case ',':  token->kind = Token_Comma;        break;
            case ';':  token->kind = Token_Semicolon;    break;
            case '<':  token->kind = Token_Less;         break;
            case '>':  token->kind = Token_Greater;      break;
            case '(':  token->kind = Token_OpenParen;    break;
            case ')':  token->kind = Token_CloseParen;   break;
            case '[':  token->kind = Token_OpenBracket;  break;
            case ']':  token->kind = Token_CloseBracket; break;
            case '{':  token->kind = Token_OpenBrace;    break;
            case '}':  token->kind = Token_CloseBrace;   break;
            
            default:
            {
                if (IsAlpha(c) || c == '_')
                {
                    if (c == '_' && !(IsAlpha(file_content[offset]) ||
                                      IsDigit(file_content[offset]) ||
                                      file_content[offset] == '_'))
                    {
                        token->kind = Token_Underscore;
                    }
                    
                    else
                    {
                        token->kind = Token_Identifier;
                        
                        token->string.data = file_content + offset - 1;
                        
                        bool is_only_underscores = (c == '_');
                        while (file_content[offset] == '_'   ||
                               IsAlpha(file_content[offset]) ||
                               IsDigit(file_content[offset]))
                        {
                            is_only_underscores = (is_only_underscores && (file_content[offset] != '_'));
                            
                            token->string.size += 1;
                            offset     += 1;
                        }
                        
                        if (is_only_underscores)
                        {
                            //// ERROR
                            
                            Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                            Text_Interval report_highlight = report_text;
                            
                            ReportLexerError(workspace, report_text, report_highlight,
                                             "An identifier must contain at least one alphanumeric character");
                            
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            for (umm i = 0; i < KEYWORD_KIND_COUNT; ++i)
                            {
                                if (StringCompare(token->string, KeywordStrings[i]))
                                {
                                    token->keyword = i;
                                    break;
                                }
                            }
                        }
                    }
                }
                
                else if (IsDigit(c) || c == '.')
                {
                    if (c == '.' && !IsDigit(file_content[offset]))
                    {
                        token->kind = Token_Period;
                    }
                    
                    else
                    {
                        bool is_hex    = false;
                        bool is_binary = false;
                        bool is_float  = (c == '.');
                        
                        if (c == '0')
                        {
                            if      (file_content[offset] == 'x') is_hex    = true;
                            else if (file_content[offset] == 'b') is_binary = true;
                            else if (file_content[offset] == 'h') is_float = true, is_hex = true;
                            
                            if (is_hex || is_binary) offset += 1;
                        }
                        
                        u64 integer  = 0;
                        f64 floating = 0;
                        
                        u8 base = (is_hex ? 16  : (!is_binary ? 10 : 2));
                        
                        if (c != '.' && !(is_hex || is_binary))
                        {
                            integer  = c - '0';
                            floating = c - '0';
                        }
                        
                        bool integer_overflown        = false;
                        bool last_was_digit           = (IsDigit(c) && !(is_hex || is_binary));
                        bool contains_non_zero_digits = false;
                        umm hex_digit_count           = 0;
                        f64 floating_digit_adj        = 0.1;
                        while (!encountered_errors)
                        {
                            u8 digit = 0;
                            
                            if (IsDigit(file_content[offset]))
                            {
                                digit = file_content[offset] - '0';
                                
                                if (is_binary && digit > 1)
                                {
                                    //// ERROR
                                    
                                    Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                    Text_Interval report_highlight = report_text;
                                    
                                    report_highlight.pos.column += report_text.size - 1;
                                    report_highlight.size        = 1;
                                    
                                    ReportLexerError(workspace, report_text, report_highlight, "Invalid binary digit '%u'", digit);
                                    
                                    encountered_errors = true;
                                }
                            }
                            
                            else if (is_hex && (ToUpper(file_content[offset]) >= 'A' &&
                                                ToUpper(file_content[offset]) <= 'F'))
                            {
                                digit = (ToUpper(file_content[offset])) + 10;
                            }
                            
                            else if (file_content[offset] == '.' && !(is_float || is_hex || is_binary))
                            {
                                is_float = true;
                            }
                            
                            else if (file_content[offset] == '_')
                            {
                                if (last_was_digit); // NOTE(soimn): Ignore
                                else
                                {
                                    //// ERROR
                                    
                                    Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                    Text_Interval report_highlight = report_text;
                                    
                                    report_highlight.pos.column += report_text.size - 1;
                                    report_highlight.size        = 1;
                                    
                                    ReportLexerError(workspace, report_text, report_highlight,
                                                     "Encountered digit separator with no preceeding digit");
                                    
                                    encountered_errors = true;
                                }
                            }
                            
                            else break;
                            
                            if (!(file_content[offset] == '.' || file_content[offset] == '_'))
                            {
                                u64 new_integer = integer * base + digit;
                                integer_overflown = (integer_overflown || new_integer < integer);
                                
                                integer  = new_integer;
                                floating = floating * (is_float ? 1 : 10) + digit * (is_float ? floating_digit_adj : 1);
                                
                                hex_digit_count += 1;
                                last_was_digit   = true;
                                contains_non_zero_digits = (contains_non_zero_digits || digit != 0);
                                
                                if (is_float) floating_digit_adj /= 10;
                            }
                            
                            else last_was_digit = false;
                            
                            offset += 1;
                        }
                        
                        if (!encountered_errors)
                        {
                            if (hex_digit_count == 0)
                            {
                                // NOTE(soimn): Zero digits before and after decimal point has already been handled.
                                //              The only remaining possibility for zero digit literals are hex and
                                //              binary literals
                                ASSERT(is_hex || is_binary);
                                
                                //// ERROR
                                
                                Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                Text_Interval report_highlight = report_text;
                                
                                ReportLexerError(workspace, report_text, report_highlight,
                                                 "Missing digits after hex/binary literal prefix");
                                
                                encountered_errors = true;
                            }
                            
                            else if (file_content[offset - 1] == '_')
                            {
                                //// ERROR
                                
                                Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                Text_Interval report_highlight = report_text;
                                
                                report_highlight.pos.column += report_text.size - 1;
                                report_highlight.size        = 1;
                                
                                ReportLexerError(workspace, report_text, report_highlight, "Missing digits after digit separator");
                                
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                if (is_float)
                                {
                                    if (is_hex)
                                    {
                                        if (hex_digit_count == 8)
                                        {
                                            token->kind = Token_Number;
                                            token->number.is_float     = true;
                                            token->number.min_fit_bits = 32;
                                            
                                            // IMPORTANT NOTE(soimn): This assumes that integers and floats are
                                            //                        represented with the same endianess
                                            // HACK(soimn): Replace this with something more sane
                                            u32 i = integer;
                                            Copy(&i, &token->number.floating, sizeof(f32));
                                        }
                                        
                                        else if (hex_digit_count == 16)
                                        {
                                            token->kind = Token_Number;
                                            token->number.is_float     = true;
                                            token->number.min_fit_bits = 64;
                                            
                                            // IMPORTANT NOTE(soimn): This assumes that integers and floats are
                                            //                        represented with the same endianess
                                            // HACK(soimn): Replace this with something more sane
                                            Copy(&integer, &token->number.floating, sizeof(f32));
                                        }
                                        
                                        else
                                        {
                                            //// ERROR
                                            
                                            Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                            Text_Interval report_highlight = report_text;
                                            
                                            ReportLexerError(workspace, report_text, report_highlight,
                                                             "Invalid number of digits in hexadecimal floating point literal");
                                            
                                            encountered_errors = true;
                                        }
                                    }
                                    
                                    else
                                    {
                                        if (ToUpper(file_content[offset]) == 'E')
                                        {
                                            offset += 1;
                                            
                                            u64 exponent = 0;
                                            i8 sign      = 1;
                                            
                                            if (file_content[offset] == '-' ||
                                                file_content[offset] == '+')
                                            {
                                                sign = (file_content[offset] == '-' ? -1 : 1);
                                                
                                                offset += 1;
                                            }
                                            
                                            last_was_digit = false;
                                            while (!encountered_errors)
                                            {
                                                if (IsDigit(file_content[offset]))
                                                {
                                                    u64 new_exponent = exponent * 10 + (file_content[offset] - '0');
                                                    
                                                    if (new_exponent < exponent)
                                                    {
                                                        //// ERROR
                                                        
                                                        Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                                        Text_Interval report_highlight = report_text;
                                                        
                                                        ReportLexerError(workspace, report_text, report_highlight,
                                                                         "Exponent too large");
                                                        
                                                        encountered_errors = true;
                                                    }
                                                    
                                                    else exponent = new_exponent;
                                                    
                                                    last_was_digit = true;
                                                    
                                                    offset += 1;
                                                }
                                                
                                                else if (file_content[offset] == '_')
                                                {
                                                    if (last_was_digit) offset += 1;
                                                    else
                                                    {
                                                        //// ERROR
                                                        
                                                        Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                                        Text_Interval report_highlight = report_text;
                                                        
                                                        report_highlight.pos.column += report_text.size - 1;
                                                        report_highlight.size        = 1;
                                                        
                                                        ReportLexerError(workspace, report_text, report_highlight,
                                                                         "Encountered digit separator with no preceeding digit");
                                                        
                                                        encountered_errors = true;
                                                    }
                                                }
                                                
                                                else break;
                                            }
                                            
                                            if (!encountered_errors)
                                            {
                                                if (sign == 1) for (umm i = 0; i < exponent; ++i) floating *= exponent;
                                                else           for (umm i = 0; i < exponent; ++i) floating /= exponent;
                                            }
                                        }
                                        
                                        if (!encountered_errors)
                                        {
                                            u64 flt_bits = 0;
                                            Copy(&floating, &flt_bits, sizeof(f64));
                                            
                                            // NOTE(soimn): Ignore the sign bit
                                            if (contains_non_zero_digits && (flt_bits << 1) == 0)
                                            {
                                                //// ERROR
                                                
                                                Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                                Text_Interval report_highlight = report_text;
                                                
                                                ReportLexerError(workspace, report_text, report_highlight,
                                                                 "Floating point literal truncated to zero");
                                                
                                                encountered_errors = true;
                                            }
                                            
                                            // NOTE(soimn): Is infinity
                                            else if ((flt_bits & (U64_MAX >> 1)) == ((umm)0x7FF << (64 - 12)))
                                            {
                                                //// ERROR
                                                
                                                Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                                Text_Interval report_highlight = report_text;
                                                
                                                ReportLexerError(workspace, report_text, report_highlight,
                                                                 "Floating point literal too large");
                                                
                                                encountered_errors = true;
                                            }
                                            
                                            // NOTE(soimn): Is NaN
                                            else if ((flt_bits & (U64_MAX >> 1)) > ((umm)0x7FF << (64 - 12)))
                                            {
                                                INVALID_CODE_PATH;
                                            }
                                            
                                            else
                                            {
                                                token->kind = Token_Number;
                                                token->number.is_float = true;
                                                token->number.floating = floating;
                                                
                                                // HACK(soimn): Replace this with something more sane
                                                volatile f32 f = floating;
                                                if ((f64)f == floating) token->number.min_fit_bits = 32;
                                                else                    token->number.min_fit_bits = 64;
                                            }
                                        }
                                    }
                                }
                                
                                else
                                {
                                    if (integer_overflown)
                                    {
                                        //// ERROR
                                        
                                        Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                        Text_Interval report_highlight = report_text;
                                        
                                        ReportLexerError(workspace, report_text, report_highlight, "Integer literal too large");
                                        
                                        encountered_errors = true;
                                    }
                                    
                                    else
                                    {
                                        token->kind = Token_Number;
                                        
                                        token->number.integer  = integer;
                                        token->number.is_float = false;
                                        
                                        if      (integer <= U8_MAX)  token->number.min_fit_bits = 8;
                                        else if (integer <= U16_MAX) token->number.min_fit_bits = 16;
                                        else if (integer <= U32_MAX) token->number.min_fit_bits = 32;
                                        else if (integer <= U64_MAX) token->number.min_fit_bits = 64;
                                    }
                                }
                            }
                        }
                    }
                }
                
                else if (c == '"')
                {
                    bool encountered_errors = false;
                    
                    String raw_string = {
                        .data = file_content + offset,
                        .size = 0
                    };
                    
                    while (file_content[offset] != 0 &&
                           file_content[offset] != '"')
                    {
                        if (file_content[offset] == '\\' &&
                            file_content[offset] != 0)
                        {
                            offset += 2;
                        }
                        
                        else offset += 1;
                    }
                    
                    raw_string.size = (offset + file_content) - raw_string.data;
                    
                    if (file_content[offset] == 0)
                    {
                        //// ERROR
                        
                        Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                        Text_Interval report_highlight = report_text;
                        
                        ReportLexerError(workspace, report_text, report_highlight, "Unterminated string");
                        
                        encountered_errors = true;
                    }
                    
                    else
                    {
                        offset += 1;
                        
                        if (raw_string.size == 0) token->string = raw_string;
                        else
                        {
                            token->string.data = Arena_Allocate(PersistentArena(workspace), raw_string.size, ALIGNOF(u8));
                            token->string.size = 0;
                            
                            for (umm i = 0; i < raw_string.size; )
                            {
                                if (raw_string.data[i] == '\\')
                                {
                                    c  = raw_string.data[i + 1];
                                    i += 2;
                                    ASSERT(raw_string.size > i);
                                    
                                    if      (c == 'a') token->string.data[token->string.size++] = '\a';
                                    else if (c == 'b') token->string.data[token->string.size++] = '\b';
                                    else if (c == 'e') token->string.data[token->string.size++] = '\e';
                                    else if (c == 'f') token->string.data[token->string.size++] = '\f';
                                    else if (c == 'n') token->string.data[token->string.size++] = '\n';
                                    else if (c == 'r') token->string.data[token->string.size++] = '\r';
                                    else if (c == 't') token->string.data[token->string.size++] = '\t';
                                    else if (c == 'v') token->string.data[token->string.size++] = '\v';
                                    else if (c == 'u' || c == 'x')
                                    {
                                        u32 codepoint = 0;
                                        for (umm j = i; !encountered_errors && i < j + (c == 'u' ? 6 : 2); ++i)
                                        {
                                            if (i == raw_string.size)
                                            {
                                                //// ERROR
                                                
                                                Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                                Text_Interval report_highlight = report_text;
                                                
                                                ReportLexerError(workspace, report_text, report_highlight,
                                                                 "Missing digits in codepoint/byte escape sequence");
                                                
                                                encountered_errors = true;
                                            }
                                            
                                            else codepoint = codepoint * 10 + (raw_string.data[i] - '0');
                                        }
                                        
                                        if (!encountered_errors)
                                        {
                                            if (codepoint <= U8_MAX)
                                            {
                                                token->string.data[token->string.size++] = (u8)codepoint;
                                            }
                                            
                                            else if (codepoint <= 0x7FF)
                                            {
                                                token->string.data[token->string.size++] = 0xC0 | (codepoint & 0x7C0) >> 6;
                                                token->string.data[token->string.size++] = 0x80 | (codepoint & 0x03F) >> 0;
                                            }
                                            
                                            else if (codepoint <= 0xFFFF)
                                            {
                                                token->string.data[token->string.size++] = 0xE0 | (codepoint & 0xF000) >> 12;
                                                token->string.data[token->string.size++] = 0x80 | (codepoint & 0x0FC0) >> 6;
                                                token->string.data[token->string.size++] = 0x80 | (codepoint & 0x003F) >> 0;
                                            }
                                            
                                            else if (codepoint <= 0x10FFFF)
                                            {
                                                token->string.data[token->string.size++] = 0xF0 | (codepoint & 0x1C0000) >> 18;
                                                token->string.data[token->string.size++] = 0x80 | (codepoint & 0x03F000) >> 12;
                                                token->string.data[token->string.size++] = 0x80 | (codepoint & 0x000FC0) >> 6;
                                                token->string.data[token->string.size++] = 0x80 | (codepoint & 0x00003F) >> 0;
                                            }
                                            
                                            else
                                            {
                                                //// ERROR
                                                
                                                Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                                Text_Interval report_highlight = report_text;
                                                
                                                ReportLexerError(workspace, report_text, report_highlight,
                                                                 "Codepoint is out of UTF-8 range");
                                                
                                                encountered_errors = true;
                                            }
                                        }
                                    }
                                    
                                    else
                                    {
                                        token->string.data[token->string.size++] = raw_string.data[i];
                                    }
                                }
                                
                                else
                                {
                                    token->string.data[token->string.size++] = raw_string.data[i];
                                    i += 1;
                                }
                            }
                        }
                    }
                }
                
                else
                {
                    //// ERROR
                    
                    Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                    Text_Interval report_highlight = report_text;
                    
                    ReportLexerError(workspace, report_text, report_highlight, "Encountered an invalid token");
                    
                    encountered_errors = true;
                }
                
            } break;
        }
        
        token->text.size = offset - (token->text.pos.column + token->text.pos.offset_to_line);
        
        if (encountered_errors) token->kind = Token_Invalid;
        
        if (token->kind == Token_EndOfStream) break;
    }
    
#undef CURRENT_TOKEN_SIZE
    
    if (!encountered_errors)
    {
        umm total_memory_used = Arena_TotalAllocatedMemory(TempArena(workspace));
        ASSERT(total_memory_used % sizeof(Token) == 0);
        
        file->tokens.size = total_memory_used / sizeof(Token);
        file->tokens.data = Arena_Allocate(PersistentArena(workspace),
                                           total_memory_used, ALIGNOF(Token));
        
        Arena_FlattenContent(TempArena(workspace),
                             file->tokens.data, total_memory_used);
        Arena_ClearAll(TempArena(workspace));
    }
    
    return !encountered_errors;
}