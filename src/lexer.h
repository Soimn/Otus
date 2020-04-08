typedef enum LEXER_TOKEN_KIND
{
    Token_Error,
    Token_EndOfStream,
    
    Token_Plus,
    Token_Minus,
    Token_Star,
    Token_Slash,
    Token_Percentage,
    Token_Ampersand,
    Token_Tilde,
    Token_Pipe,
    Token_Less,
    Token_Greater,
    Token_Backslash,
    Token_Equals,
    Token_Exclamation,
    Token_Question,
    Token_At,
    Token_Hash,
    Token_Cash,
    Token_Hat,
    Token_Quote,
    Token_Period,
    Token_Comma,
    Token_Colon,
    Token_Semicolon,
    
    Token_OpenParen,
    Token_CloseParen,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenBrace,
    Token_CloseBrace,
    
    Token_Blank,
    Token_Identifier,
    Token_Keyword,
    Token_String,
    Token_Number,
} LEXER_TOKEN_KIND;

enum LEXER_KEYWORD_KIND
{
    Keyword_If,
    Keyword_Else,
    Keyword_While,
    Keyword_Break,
    Keyword_Continue,
    Keyword_Using,
    Keyword_Defer,
    Keyword_Return,
    Keyword_Enum,
    Keyword_Struct,
    Keyword_Proc,
};

typedef struct Token
{
    Enum32(LEXER_TOKEN_KIND) kind;
    
    union
    {
        String string;
        Number number;
        Enum32(LEXER_KEYWORD_KIND) keyword;
    };
    
    Text_Interval text;
} Token;

typedef struct Lexer
{
    Worker_Context* context;
    
    U8* start;
    Text_Pos current;
    
    struct
    {
        Token token;
        bool is_valid;
    } cache;
} Lexer;

Lexer
LexFile(Worker_Context* context, File_ID file_id)
{
    Source_File* file = BucketArray_ElementAt(&context->workspace->source_files, file_id);
    ASSERT(file->contents.data[file->contents.size - 1] == 0);
    
    Lexer lexer = {0};
    lexer.context         = context;
    lexer.start           = file->contents.data;
    lexer.current.file_id = file_id;
    
    return lexer;
}

inline void
LexerReport(Lexer* lexer, Enum32(REPORT_SEVERITY) severity, Text_Interval text, const char* format_string, ...)
{
    Arg_List arg_list;
    ARG_LIST_START(arg_list, format_string);
    
    Report(lexer->context->workspace, severity, CompilationStage_Lexing, WrapCString(format_string),
           arg_list, &text);
}

Token
GetToken(Lexer* lexer)
{
    Token result;
    
    if (lexer->cache.is_valid)
    {
        result = lexer->cache.token;
    }
    
    else
    {
        U8* current = lexer->start + lexer->current.offset_to_line + lexer->current.column;
        
        { /// Skip whitespace and comments
            char c;
            while (c = current[0], c != 0)
            {
                if (c == ' ' || c == '\t' || c == '\v' || c == '\r') ++current;
                else if (c == '\n')
                {
                    lexer->current.line          += 1;
                    lexer->current.offset_to_line = current - lexer->start;
                    ++current;
                }
                
                else if (c == '/' && current[1] == '/')
                {
                    current += 2;
                    while (current[0] != 0 && current[0] != '\n') ++current;
                }
                
                else if (c == '/' && current[1] == '*')
                {
                    current += 2;
                    
                    U32 indent_level = 0;
                    while (current[0] != 0)
                    {
                        if (current[0] == '*' && current[1] == '/')
                        {
                            current += 2;
                            if (indent_level-- == 0) break;
                        }
                        
                        else if (current[0] == '/' && current[1] == '*')
                        {
                            current      += 2;
                            indent_level += 1;
                        }
                    }
                }
                
                else break;
            }
            
            UMM offset_to_current = (current - lexer->start);
            lexer->current.column = offset_to_current - lexer->current.offset_to_line;
        }
        
        Token token = {Token_Error};
        
        token.text.position = lexer->current;
        switch (current[0])
        {
            case '\0': token.kind = Token_EndOfStream;  ++current; break;
            case '+':  token.kind = Token_Plus;         ++current; break;
            case '-':  token.kind = Token_Minus;        ++current; break;
            case '*':  token.kind = Token_Star;         ++current; break;
            case '/':  token.kind = Token_Slash;        ++current; break;
            case '%':  token.kind = Token_Percentage;   ++current; break;
            case '&':  token.kind = Token_Ampersand;    ++current; break;
            case '|':  token.kind = Token_Pipe;         ++current; break;
            case '~':  token.kind = Token_Tilde;        ++current; break;
            case '<':  token.kind = Token_Less;         ++current; break;
            case '>':  token.kind = Token_Greater;      ++current; break;
            case '\\': token.kind = Token_Backslash;    ++current; break;
            case '=':  token.kind = Token_Equals;       ++current; break;
            case '!':  token.kind = Token_Exclamation;  ++current; break;
            case '?':  token.kind = Token_Question;     ++current; break;
            case '@':  token.kind = Token_At;           ++current; break;
            case '#':  token.kind = Token_Hash;         ++current; break;
            case '$':  token.kind = Token_Cash;         ++current; break;
            case '^':  token.kind = Token_Hat;          ++current; break;
            case '\'': token.kind = Token_Quote;        ++current; break;
            case '.':  token.kind = Token_Period;       ++current; break;
            case ',':  token.kind = Token_Comma;        ++current; break;
            case ':':  token.kind = Token_Colon;        ++current; break;
            case ';':  token.kind = Token_Semicolon;    ++current; break;
            case '(':  token.kind = Token_OpenParen;    ++current; break;
            case ')':  token.kind = Token_CloseParen;   ++current; break;
            case '[':  token.kind = Token_OpenBracket;  ++current; break;
            case ']':  token.kind = Token_CloseBracket; ++current; break;
            case '{':  token.kind = Token_OpenBrace;    ++current; break;
            case '}':  token.kind = Token_CloseBrace;   ++current; break;
            default:
            {
                if (current[0] == '"')
                {
                    token.kind = Token_String;
                    
                    ++current;
                    token.string.data = current;
                    
                    while (current[0] != 0 && current[0] != '"')
                    {
                        if (current[0] == '/' && current[1] != 0) current += 2;
                        else                                                    current += 1;
                    }
                    
                    if (current[0] == '"')
                    {
                        token.string.size = current - token.string.data;
                        ++current;
                    }
                    
                    else
                    {
                        //// ERROR: Encountered EOF before end of string literal
                        token.kind = Token_Error;
                        
                        Text_Interval string_interval = {
                            .position = lexer->current,
                            .size     = (current - lexer->start) - (lexer->current.offset_to_line +
                                                                    lexer->current.column)
                        };
                        
                        LexerReport(lexer, ReportSeverity_Error, string_interval, 
                                    "Encountered EOF before end of string literal");
                    }
                }
                
                else if (IsAlpha(current[0]) ||(current[0] == '_' && IsAlpha(current[1])))
                {
                    token.kind        = Token_Identifier;
                    token.string.data = (U8*)current;
                    
                    while (current[0] == '_' || IsAlpha(current[0]) || IsDigit(current[0]))
                    {
                        ++current;
                        ++token.string.size;
                    }
                    
                    const char* keyword_strings[] = {
                        [Keyword_If]       = "if",
                        [Keyword_Else]     = "else",
                        [Keyword_While]    = "while",
                        [Keyword_Break]    = "break",
                        [Keyword_Continue] = "continue",
                        [Keyword_Using]    = "using",
                        [Keyword_Defer]    = "defer",
                        [Keyword_Return]   = "return",
                        [Keyword_Enum]     = "enum",
                        [Keyword_Struct]   = "struct",
                        [Keyword_Proc]     = "proc"
                    };
                    
                    for (U32 i = 0; i < ARRAY_COUNT(keyword_strings); ++i)
                    {
                        if (StringCStringCompare(token.string, keyword_strings[i]))
                        {
                            token.kind    = Token_Keyword;
                            token.keyword = i;
                            
                            break;
                        }
                    }
                }
                
                else if (current[0] == '_')
                {
                    token.kind = Token_Blank;
                }
                
                else if (IsDigit(current[0]))
                {
                    token.kind = Token_Number;
                    
                    bool is_hex   = false;
                    bool is_float = false;
                    
                    if (current[0] == '0' && (current[1] == 'x' || current[1] == 'h'))
                    {
                        is_hex   = true;
                        is_float = (current[1] == 'h');
                        
                        current += 2;
                    }
                    
                    U8 base = (is_hex ? 16 : 10);
                    
                    U64 u64 = 0;
                    F64 f64 = 0;
                    
                    bool integer_value_overflown = false;
                    bool float_value_oop         = false;
                    
                    F64 digit_multiplier = 1;
                    
                    UMM num_digits = 0;
                    U8* start = current;
                    for (;;)
                    {
                        U8 digit = 0;
                        
                        if (IsDigit(current[0]))
                        {
                            digit = current[0] - '0';
                        }
                        
                        else if (is_hex && (ToUpper(current[0]) >= 'A' &&
                                            ToUpper(current[0]) <= 'Z'))
                        {
                            digit = (ToUpper(current[0]) - 'A') + 10;
                        }
                        
                        else if (!is_hex && !is_float && current[0] == '.')
                        {
                            is_float = true;
                            ++current;
                            continue;
                        }
                        
                        else if (current[0] == '_')
                        {
                            ++current;
                            continue;
                        }
                        
                        else break;
                        
                        U64 new_u64 = u64 * base + digit;
                        F64 new_f64 = f64 * (is_float ? 1 : base) + digit * digit_multiplier;
                        
                        if (new_u64 < u64) integer_value_overflown                           = true;
                        if (digit != 0 && new_f64 == f64 || IS_INF(new_f64)) float_value_oop = true;
                        
                        u64 = new_u64;
                        f64 = new_f64;
                        
                        digit_multiplier *= (is_float ? 0.1 : 1);
                    }
                    
                    if (!is_float)
                    {
                        if (is_hex && num_digits == 0)
                        {
                            //// ERROR: Missing digits after hexadecimal literal prefix
                            token.kind = Token_Number;
                            
                            Text_Interval text_interval = {
                                .position = lexer->current,
                                .size     = 2,
                            };
                            
                            LexerReport(lexer, ReportSeverity_Error, text_interval,
                                        "Missing digits after hexadecimal literal prefix");
                        }
                        
                        else
                        {
                            if (!integer_value_overflown)
                            {
                                token.kind   = Token_Number;
                                token.number.is_f64 = false;
                                token.number.u64    = u64;
                                
                                if      (u64 <= U8_MAX)  token.number.min_fit_bits = 8;
                                else if (u64 <= U16_MAX) token.number.min_fit_bits = 16;
                                else if (u64 <= U32_MAX) token.number.min_fit_bits = 32;
                                else                     token.number.min_fit_bits = 64;
                            }
                            
                            else
                            {
                                //// ERROR: Integer constant is too large to be represented by any integer type
                                token.kind = Token_Error;
                                
                                Text_Interval text_interval = {
                                    .position = lexer->current,
                                    .size     = current - start
                                };
                                
                                LexerReport(lexer, ReportSeverity_Error, text_interval,
                                            "Integer constant is too large to be represented by any integer type");
                            }
                        }
                    }
                    
                    else if (!is_hex)
                    {
                        if (!float_value_oop)
                        {
                            token.kind   = Token_Number;
                            token.number.is_f64 = true;
                            token.number.f64    = f64;
                        }
                        
                        else
                        {
                            //// ERROR: Floating point constant cannot be represented precisely by any floating 
                            ////        point type
                            token.kind = Token_Error;
                            
                            Text_Interval text_interval = {
                                .position = lexer->current,
                                .size     = current - start
                            };
                            
                            LexerReport(lexer, ReportSeverity_Error, text_interval,
                                        "Floating point constant cannot be represented precisely by any floating point type");
                        }
                    }
                    
                    else
                    {
                        if (num_digits == 8)
                        {
                            NOT_IMPLEMENTED;
                        }
                        
                        else if (num_digits == 16)
                        {
                            NOT_IMPLEMENTED;
                        }
                        
                        else
                        {
                            //// ERROR: Length of hexadecimal floating point literal does not correspond to any 
                            ////        valid floating point type.
                            
                            token.kind = Token_Error;
                            
                            Text_Interval text_interval = {
                                .position = lexer->current,
                                .size     = current - start
                            };
                            
                            LexerReport(lexer, ReportSeverity_Error, text_interval,
                                        "Length of hexadecimal floating point literal does not correspond to any valid floating point type");
                        }
                    }
                }
            } break;
        }
        
        token.text.size = (current - lexer->start) - (lexer->current.offset_to_line + lexer->current.column);
        
        result = token;
        if (token.kind != Token_Error)
        {
            lexer->cache.token    = token;
            lexer->cache.is_valid = true;
        }
    }
    
    return result;
}

void
SkipPastToken(Lexer* lexer, Token token)
{
    lexer->current         = token.text.position;
    lexer->current.column += token.text.size;
    lexer->cache.is_valid = false;
}

Token
PeekNextToken(Lexer* lexer, Token token)
{
    Lexer tmp_lexer = *lexer;
    SkipPastToken(&tmp_lexer, token);
    return GetToken(&tmp_lexer);
}

inline Text_Interval
TextInterval(Text_Pos position, UMM size)
{
    Text_Interval interval = {
        .position = position,
        .size     = size
    };
    
    return interval;
}

inline Text_Interval
TextIntervalFromTokens(Token start, Token end)
{
    UMM offset_to_start_of_start_token = start.text.position.offset_to_line + start.text.position.column;
    UMM offset_to_end_of_end_token = end.text.position.offset_to_line + end.text.position.column + end.text.size;
    
    Text_Interval interval = {
        .position = start.text.position,
        .size     = offset_to_end_of_end_token - offset_to_start_of_start_token
    };
    
    return interval;
}

inline Text_Interval
TextIntervalFromEndpoints(Text_Pos start, Text_Pos end)
{
    Text_Interval interval = {
        .position = start,
        .size     = (end.offset_to_line - end.column) - (start.offset_to_line + start.column)
    };
    
    return interval;
}

inline Text_Interval
TextIntervalMerge(Text_Interval i0, Text_Interval i1)
{
    UMM i0_offset = i0.position.offset_to_line + i0.position.column;
    UMM i1_offset = i1.position.offset_to_line + i1.position.column;
    
    if (i0_offset > i1_offset || (i0_offset == i1_offset && i0.size > i1.size))
    {
        Text_Interval tmp = i0;
        i0 = i1;
        i1 = tmp;
        
        i0_offset = i0.position.offset_to_line + i0.position.column;
        i1_offset = i1.position.offset_to_line + i1.position.column;
    }
    
    Text_Interval interval = {
        .position = i0.position,
        .size     = (i1_offset - i0_offset) + i1.size
    };
    
    return interval;
}