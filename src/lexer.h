#define CURRENT_TOKEN_SIZE(token, offset)\
((offset) - ((token)->text.pos.column + (token)->text.pos.offset_to_line))

#define SINGLE_OR_EQUAL(ch, single_kind, equal_kind) \
case ch:                                         \
{                                                \
if (content[offset] == '=')                  \
{                                            \
offset += 1;                             \
token->kind = equal_kind;                \
}                                            \
else token->kind = single_kind;              \
} break

#define SINGLE_DOUBLE_OR_EQUAL(ch, single_kind, double_kind, equal_kind)       \
case ch:                                                                   \
{                                                                          \
if (content[offset] == '=' || content[offset] == ch)                   \
{                                                                      \
offset += 1;                                                       \
token->kind = (content[offset] == '=' ? equal_kind : double_kind); \
}                                                                      \
else token->kind = single_kind;                                        \
} break

#define SINGLE_DOUBLE_EQUAL_OR_DOUBLEEQUAL(ch, single_kind, double_kind, single_equal_kind, double_equal_kind) \
case ch:                                                                                                   \
{                                                                                                          \
if (content[offset] == ch)                                                                             \
{                                                                                                      \
offset += 1;                                                                                       \
if (content[offset] == '=')                                                                        \
{                                                                                                  \
offset += 1;                                                                                   \
token->kind = double_equal_kind;                                                               \
}                                                                                                  \
else token->kind = double_kind;                                                                    \
}                                                                                                      \
else                                                                                                   \
{                                                                                                      \
if (content[offset] == '=')                                                                        \
{                                                                                                  \
offset += 1;                                                                                   \
token->kind = single_equal_kind;                                                               \
}                                                                                                  \
else token->kind = single_kind;                                                                    \
}                                                                                                      \
} break


enum TOKEN_KIND
{
    Token_Invalid = 0,
    
    Token_Plus,
    Token_Minus,
    Token_Asterisk,
    Token_Slash,
    Token_Percentage,
    Token_Exclamation,
    Token_Ampersand,
    Token_Equal,
    Token_Pipe,
    Token_Tilde,
    Token_Colon,
    Token_Less,
    Token_Greater,
    
    Token_ColonColon,
    Token_AmpersandAmpersand,
    Token_PipePipe,
    Token_LessLess,
    Token_GreaterGreater,
    
    Token_MinusMinusMinus,
    Token_MinusGreater,
    
    Token_PlusEqual,
    Token_MinusEqual,
    Token_AsteriskEqual,
    Token_SlashEqual,
    Token_PercentageEqual,
    Token_ExclamationEqual,
    Token_EqualEqual,
    Token_TildeEqual,
    Token_ColonEqual,
    Token_AmpersandEqual,
    Token_PipeEqual,
    Token_LessEqual,
    Token_GreaterEqual,
    
    Token_LessLessEqual,
    Token_GreaterGreaterEqual,
    
    Token_Period,
    Token_PeriodPeriod,
    Token_PeriodPeriodLess,
    
    Token_At,
    Token_Cash,
    Token_Hash,
    Token_QuestionMark,
    Token_Hat,
    Token_Comma,
    Token_Semicolon,
    Token_OpenParen,
    Token_CloseParen,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenBrace,
    Token_CloseBrace,
    Token_Underscore,
    
    Token_Number,
    Token_String,
    Token_Character,
    Token_Identifier,
    
    Token_EndOfStream,
    
    TOKEN_KIND_COUNT
};

enum KEYWORD_KIND
{
    Keyword_Invalid = 0,
    
    Keyword_Import,
    Keyword_Foreign,
    Keyword_As,
    Keyword_Defer,
    Keyword_Return,
    Keyword_Using,
    Keyword_Proc,
    Keyword_Where,
    Keyword_Struct,
    Keyword_Union,
    Keyword_Enum,
    Keyword_If,
    Keyword_Else,
    Keyword_When,
    Keyword_While,
    Keyword_For,
    Keyword_Break,
    Keyword_Continue,
    Keyword_In,
    Keyword_NotIn,
    Keyword_Do,
    Keyword_True,
    Keyword_False,
    
    KEYWORD_KIND_COUNT
};

// NOTE(soimn): This is defined as a macro to circumvent msvc bullshit
#define EMIT_KEYWORD_STRINGS_TABLE()                   \
String KeywordStrings[KEYWORD_KIND_COUNT] = {      \
[Keyword_Import]   = CONST_STRING("import"),   \
[Keyword_Foreign]  = CONST_STRING("foreign"),  \
[Keyword_As]       = CONST_STRING("as"),       \
[Keyword_Defer]    = CONST_STRING("defer"),    \
[Keyword_Return]   = CONST_STRING("return"),   \
[Keyword_Using]    = CONST_STRING("using"),    \
[Keyword_Proc]     = CONST_STRING("proc"),     \
[Keyword_Where]    = CONST_STRING("where"),    \
[Keyword_Struct]   = CONST_STRING("struct"),   \
[Keyword_Union]    = CONST_STRING("union"),    \
[Keyword_Enum]     = CONST_STRING("enum"),     \
[Keyword_If]       = CONST_STRING("if"),       \
[Keyword_Else]     = CONST_STRING("else"),     \
[Keyword_When]     = CONST_STRING("when"),     \
[Keyword_While]    = CONST_STRING("while"),    \
[Keyword_For]      = CONST_STRING("for"),      \
[Keyword_Break]    = CONST_STRING("break"),    \
[Keyword_Continue] = CONST_STRING("continue"), \
[Keyword_In]       = CONST_STRING("in"),       \
[Keyword_NotIn]    = CONST_STRING("not_in"),   \
[Keyword_Do]       = CONST_STRING("do"),       \
[Keyword_True]     = CONST_STRING("true"),     \
[Keyword_False]    = CONST_STRING("false"),    \
}

typedef struct Token
{
    Text_Interval text;
    
    union
    {
        Number number;
        
        struct
        {
            String string;
            Enum8(KEYWORD_KIND) keyword;
            bool ends_with_space;
        };
        
        u32 character;
    };
    
    Enum8(TOKEN_KIND) kind;
    
} Token;

void
ReportLexerError(Compiler_State* compiler_state, Text_Interval text, Text_Interval highlight, const char* message, ...)
{
    String* path = DynamicArray_ElementAt(&compiler_state->workspace.file_paths, String, highlight.pos.file);
    
    Print("%S(%u:%u): ", *path, highlight.pos.line, highlight.pos.column);
    
    Arg_List arg_list;
    ARG_LIST_START(arg_list, message);
    
    PrintArgList(message, arg_list);
}

bool
LexFile(Compiler_State* compiler_state, File_ID file_id, Bucket_Array(Token)* token_array)
{
    bool encountered_errors = false;
    
    String* path = DynamicArray_ElementAt(&compiler_state->workspace.file_paths, String, file_id);
    
    String file_content;
    if (!System_ReadEntireFile(*path, &compiler_state->temp_memory, &file_content))
    {
        // TODO(soimn): How should this error be communicated to the user?
        // NOTE(soimn): The file path has already been checked and the existance of the file verified. If
        //              this error occurrs, the target file has either been deleted during compilation,
        //              or a wierd OS error has happened.
        //// ERROR: Failed to load file
        encountered_errors = true;
    }
    
    else
    {
        ASSERT(file_content.data[file_content.size] == 0);
        
        u8* content        = file_content.data;
        u32 offset         = 0;
        u32 offset_to_line = 0;
        u32 line           = 0;
        
        while (!encountered_errors)
        {
            Token* token = BucketArray_AppendElement(token_array);
            
            { /// Eat all whitespace and comments
                bool is_comment        = false;
                umm comment_nest_level = 0;
                
                while (content[offset] != 0)
                {
                    if (content[offset] == '\n')
                    {
                        offset_to_line = offset;
                        line          += 1;
                        
                        offset += 1;
                    }
                    
                    else if (content[offset] == ' '  ||
                             content[offset] == '\r' ||
                             content[offset] == '\t' ||
                             content[offset] == '\v' ||
                             content[offset] == '\f')
                    {
                        offset += 1;
                    }
                    
                    else if (!is_comment && (content[offset]     == '/' &&
                                             content[offset + 1] == '/'))
                    {
                        is_comment = true;
                        
                        offset += 2;
                    }
                    
                    else if (content[offset] == '/' && content[offset + 1] == '*')
                    {
                        offset += 2;
                        
                        if (comment_nest_level != 0 || !is_comment)
                        {
                            comment_nest_level += 1;
                        }
                    }
                    
                    else if (content[offset] == '*' && content[offset + 1] == '/')
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
            }
            
            token->text.pos.file           = file_id;
            token->text.pos.offset_to_line = offset_to_line;
            token->text.pos.line           = line;
            token->text.pos.column         = offset - offset_to_line;
            
            char c = content[offset];
            offset += (c != 0 ? 1 : 0);
            
            switch (c)
            {
                case 0: token->kind = Token_EndOfStream; break;
                
                case '@':  token->kind = Token_At;           break;
                case '#':  token->kind = Token_Hash;         break;
                case '$':  token->kind = Token_Cash;         break;
                case '?':  token->kind = Token_QuestionMark; break;
                case '^':  token->kind = Token_Hat;          break;
                case ',':  token->kind = Token_Comma;        break;
                case ';':  token->kind = Token_Semicolon;    break;
                case '(':  token->kind = Token_OpenParen;    break;
                case ')':  token->kind = Token_CloseParen;   break;
                case '[':  token->kind = Token_OpenBracket;  break;
                case ']':  token->kind = Token_CloseBracket; break;
                case '{':  token->kind = Token_OpenBrace;    break;
                case '}':  token->kind = Token_CloseBrace;   break;
                
                SINGLE_OR_EQUAL('+', Token_Plus, Token_PlusEqual);
                SINGLE_OR_EQUAL('*', Token_Asterisk, Token_AsteriskEqual);
                SINGLE_OR_EQUAL('/', Token_Slash, Token_SlashEqual);
                SINGLE_OR_EQUAL('%', Token_Percentage, Token_PercentageEqual);
                SINGLE_OR_EQUAL('!', Token_Exclamation, Token_ExclamationEqual);
                SINGLE_OR_EQUAL('=', Token_Equal, Token_EqualEqual);
                SINGLE_OR_EQUAL('~', Token_Tilde, Token_TildeEqual);
                
                SINGLE_DOUBLE_OR_EQUAL(':', Token_Colon, Token_ColonColon, Token_ColonEqual);
                SINGLE_DOUBLE_OR_EQUAL('&', Token_Ampersand, Token_AmpersandAmpersand, Token_AmpersandEqual);
                SINGLE_DOUBLE_OR_EQUAL('|', Token_Pipe, Token_PipePipe, Token_PipeEqual);
                
                SINGLE_DOUBLE_EQUAL_OR_DOUBLEEQUAL('<', Token_Less, Token_LessLess, Token_LessEqual, Token_LessLessEqual);
                SINGLE_DOUBLE_EQUAL_OR_DOUBLEEQUAL('>', Token_Greater, Token_GreaterGreater, Token_GreaterEqual, Token_GreaterGreaterEqual);
                
                case '-':
                {
                    if (content[offset] == '-' && content[offset + 1] == '-')
                    {
                        offset += 2;
                        token->kind = Token_MinusMinusMinus;
                    }
                    
                    else if (content[offset] == '=')
                    {
                        offset += 1;
                        token->kind = Token_MinusEqual;
                    }
                    
                    else if (content[offset] == '>')
                    {
                        offset += 1;
                        token->kind = Token_MinusGreater;
                    }
                    
                    else token->kind = Token_Minus;
                } break;
                
                default:
                {
                    if (IsAlpha(c) || c == '_')
                    {
                        if (c == '_' && !(IsAlpha(content[offset]) ||
                                          IsDigit(content[offset]) ||
                                          content[offset] == '_'))
                        {
                            token->kind = Token_Underscore;
                        }
                        
                        else
                        {
                            token->kind = Token_Identifier;
                            
                            token->string.data = content + offset - 1;
                            
                            bool is_only_underscores = (c == '_');
                            while (content[offset] == '_'   ||
                                   IsAlpha(content[offset]) ||
                                   IsDigit(content[offset]))
                            {
                                is_only_underscores = (is_only_underscores && (content[offset] != '_'));
                                
                                token->string.size += 1;
                                offset     += 1;
                            }
                            
                            if (is_only_underscores)
                            {
                                //// ERROR
                                
                                Text_Interval report_text      = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                Text_Interval report_highlight = report_text;
                                
                                ReportLexerError(compiler_state, report_text, report_highlight,
                                                 "An identifier must contain at least one alphanumeric character");
                                
                                encountered_errors = true;
                            }
                            
                            else
                            {
                                
                                EMIT_KEYWORD_STRINGS_TABLE();
                                
                                for (umm i = 0; i < KEYWORD_KIND_COUNT; ++i)
                                {
                                    if (StringCompare(token->string, KeywordStrings[i]))
                                    {
                                        token->keyword = (u8)i;
                                        break;
                                    }
                                }
                                
                                token->ends_with_space = (content[offset] == 0    || content[offset] == ' '  ||
                                                          content[offset] == '\t' || content[offset] == '\v' ||
                                                          content[offset] == '\n' || content[offset] == '\r' ||
                                                          content[offset] == '\f');
                            }
                        }
                    }
                    
                    else if (IsDigit(c) || c == '.')
                    {
                        if (c == '.' && !IsDigit(content[offset]))
                        {
                            if (content[offset] == '.')
                            {
                                if (content[offset + 1] == '<')
                                {
                                    offset += 2;
                                    token->kind = Token_PeriodPeriodLess;
                                }
                                
                                else
                                {
                                    offset += 1;
                                    token->kind = Token_PeriodPeriod;
                                }
                            }
                            
                            else token->kind = Token_Period;
                        }
                        
                        else
                        {
                            bool is_hex                   = false;
                            bool is_binary                = false;
                            bool is_float                 = (c == '.');
                            umm digit_count               = 0;
                            bool contains_non_zero_digits = false;
                            
                            if (c == '0')
                            {
                                if      (content[offset] == 'x') is_hex    = true;
                                else if (content[offset] == 'b') is_binary = true;
                                else if (content[offset] == 'h') is_float  = true, is_hex = true;
                                
                                if (is_hex || is_binary) offset += 1;
                            }
                            
                            u64 integer  = 0;
                            f64 floating = 0;
                            
                            u8 base = (is_hex ? 16  : (!is_binary ? 10 : 2));
                            
                            if  (!is_float && !is_hex && !is_binary)
                            {
                                integer     = c - '0';
                                floating    = c - '0';
                                digit_count = 1;
                                contains_non_zero_digits = (c != '0');
                            }
                            
                            bool integer_overflown = false;
                            f64 floating_digit_adj = 0.1;
                            while (!encountered_errors)
                            {
                                u8 digit = 0;
                                
                                if (IsDigit(content[offset]))
                                {
                                    digit = content[offset] - '0';
                                    
                                    if (is_binary && digit > 1)
                                    {
                                        //// ERROR
                                        
                                        Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                        Text_Interval report_highlight = report_text;
                                        
                                        report_highlight.pos.column += report_text.size - 1;
                                        report_highlight.size        = 1;
                                        
                                        ReportLexerError(compiler_state, report_text, report_highlight, "Invalid binary digit '%u'", digit);
                                        
                                        encountered_errors = true;
                                    }
                                }
                                
                                else if (is_hex && (ToUpper(content[offset]) >= 'A' &&
                                                    ToUpper(content[offset]) <= 'F'))
                                {
                                    digit = (ToUpper(content[offset])) + 10;
                                }
                                
                                else if (content[offset] == '.' && !(is_float || is_hex || is_binary))
                                {
                                    is_float = true;
                                }
                                
                                else if (content[offset] == '_')
                                {
                                    if (content[offset - 1] != '.'); // NOTE(soimn): Ignore
                                    else
                                    {
                                        //// ERROR
                                        
                                        Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                        Text_Interval report_highlight = report_text;
                                        
                                        report_highlight.pos.column += report_text.size - 1;
                                        report_highlight.size        = 1;
                                        
                                        ReportLexerError(compiler_state, report_text, report_highlight,
                                                         "Encountered digit separator with no preceeding digit");
                                        
                                        encountered_errors = true;
                                    }
                                }
                                
                                else break;
                                
                                if (!(content[offset] == '.' || content[offset] == '_'))
                                {
                                    u64 new_integer   = integer * base + digit;
                                    integer_overflown = (integer_overflown || new_integer < integer);
                                    
                                    integer = new_integer;
                                    
                                    if (is_float) floating += digit * floating_digit_adj;
                                    else          floating  = floating * 10 + digit;
                                    
                                    digit_count             += 1;
                                    contains_non_zero_digits = (contains_non_zero_digits || digit != 0);
                                    
                                    if (is_float) floating_digit_adj /= 10;
                                }
                                
                                offset += 1;
                            }
                            
                            if (!encountered_errors)
                            {
                                if (digit_count == 0)
                                {
                                    // NOTE(soimn): Zero digits before and after decimal point has already been handled.
                                    //              The only remaining possibility for zero digit literals are hex and
                                    //              binary literals
                                    ASSERT(is_hex || is_binary);
                                    
                                    //// ERROR
                                    
                                    Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                    Text_Interval report_highlight = report_text;
                                    
                                    ReportLexerError(compiler_state, report_text, report_highlight,
                                                     "Missing digits after %s literal prefix",
                                                     (is_hex ? "hexadecimal" : "binary"));
                                    
                                    encountered_errors = true;
                                }
                                
                                else if (content[offset - 1] == '_' && (!is_float || ToUpper(content[offset]) != 'E'))
                                {
                                    //// ERROR
                                    Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                    Text_Interval report_highlight = report_text;
                                    
                                    ReportLexerError(compiler_state, report_text, report_highlight,
                                                     "Missing digits after digit separator");
                                    
                                    encountered_errors = true;
                                }
                                
                                else
                                {
                                    if (is_float)
                                    {
                                        if (is_hex)
                                        {
                                            if (digit_count == 8)
                                            {
                                                token->kind = Token_Number;
                                                token->number.is_float     = true;
                                                token->number.min_fit_bits = 32;
                                                
                                                // IMPORTANT NOTE(soimn): This assumes that integers and floats are
                                                //                        represented with the same endianess
                                                // HACK(soimn): Replace this with something more sane
                                                u32 i = (u32)integer;
                                                Copy(&i, &token->number.floating, sizeof(f32));
                                            }
                                            
                                            else if (digit_count == 16)
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
                                                
                                                ReportLexerError(compiler_state, report_text, report_highlight,
                                                                 "Invalid number of digits in hexadecimal floating point literal");
                                                
                                                encountered_errors = true;
                                            }
                                        }
                                        
                                        else
                                        {
                                            if (ToUpper(content[offset]) == 'E')
                                            {
                                                offset += 1;
                                                
                                                u64 exponent = 0;
                                                i8 sign      = 1;
                                                
                                                if (content[offset] == '-' ||
                                                    content[offset] == '+')
                                                {
                                                    sign = (content[offset] == '-' ? -1 : 1);
                                                    
                                                    offset += 1;
                                                }
                                                
                                                while (!encountered_errors)
                                                {
                                                    if (IsDigit(content[offset]))
                                                    {
                                                        u64 new_exponent = exponent * 10 + (content[offset] - '0');
                                                        
                                                        if (new_exponent < exponent)
                                                        {
                                                            //// ERROR
                                                            
                                                            Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                                            Text_Interval report_highlight = report_text;
                                                            
                                                            ReportLexerError(compiler_state, report_text, report_highlight,
                                                                             "Exponent too large");
                                                            
                                                            encountered_errors = true;
                                                        }
                                                        
                                                        else exponent = new_exponent;
                                                        
                                                        offset        += 1;
                                                    }
                                                    
                                                    else if (content[offset] == '_')
                                                    {
                                                        if (content[offset - 1] != '-' && content[offset - 1] != '+') offset += 1;
                                                        else
                                                        {
                                                            //// ERROR
                                                            
                                                            Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                                            Text_Interval report_highlight = report_text;
                                                            
                                                            report_highlight.pos.column += report_text.size - 1;
                                                            report_highlight.size        = 1;
                                                            
                                                            ReportLexerError(compiler_state, report_text, report_highlight,
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
                                                
                                                // NOTE(soimn): Ignore the sign bit by shifting left once
                                                if (contains_non_zero_digits && (flt_bits << 1) == 0)
                                                {
                                                    //// ERROR
                                                    
                                                    Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                                    Text_Interval report_highlight = report_text;
                                                    
                                                    ReportLexerError(compiler_state, report_text, report_highlight,
                                                                     "Floating point literal truncated to zero");
                                                    
                                                    encountered_errors = true;
                                                }
                                                
                                                // NOTE(soimn): Is infinity
                                                else if ((flt_bits & (U64_MAX >> 1)) == ((umm)0x7FF << (64 - 12)))
                                                {
                                                    //// ERROR
                                                    
                                                    Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                                    Text_Interval report_highlight = report_text;
                                                    
                                                    ReportLexerError(compiler_state, report_text, report_highlight,
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
                                                    volatile f32 f = (f32)floating;
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
                                            
                                            ReportLexerError(compiler_state, report_text, report_highlight, "Integer literal too large");
                                            
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
                    
                    else if (c == '"' || c == '\'')
                    {
                        bool is_character = (c == '\'');
                        
                        String raw_string = {
                            .data = content + offset,
                            .size = 0
                        };
                        
                        while (content[offset] != 0 &&
                               content[offset] != c)
                        {
                            if (content[offset] == '\\' &&
                                content[offset] != 0)
                            {
                                offset += 2;
                            }
                            
                            else offset += 1;
                        }
                        
                        raw_string.size = (offset + content) - raw_string.data;
                        
                        if (content[offset] == 0)
                        {
                            //// ERROR
                            
                            Text_Interval report_text      = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                            Text_Interval report_highlight = report_text;
                            
                            ReportLexerError(compiler_state, report_text, report_highlight, "Unterminated %s literal", (is_character ? "character" : "string"));
                            
                            encountered_errors = true;
                        }
                        
                        else
                        {
                            offset += 1;
                            
                            String resolved_string = {0};
                            
                            if (raw_string.size != 0)
                            {
                                // TODO(soimn): Find out where to store strings
                                resolved_string.data = Arena_Allocate(&compiler_state->persistent_memory, raw_string.size, ALIGNOF(u8));
                                
                                for (umm i = 0; i < raw_string.size; )
                                {
                                    if (raw_string.data[i] == '\\')
                                    {
                                        c  = raw_string.data[i + 1];
                                        i += 2;
                                        ASSERT(raw_string.size > i);
                                        
                                        // TODO(soimn): Should ansi escape codes be supported?
                                        if      (c == 'a') resolved_string.data[resolved_string.size++] = '\a';
                                        else if (c == 'b') resolved_string.data[resolved_string.size++] = '\b';
                                        else if (c == 'f') resolved_string.data[resolved_string.size++] = '\f';
                                        else if (c == 'n') resolved_string.data[resolved_string.size++] = '\n';
                                        else if (c == 'r') resolved_string.data[resolved_string.size++] = '\r';
                                        else if (c == 't') resolved_string.data[resolved_string.size++] = '\t';
                                        else if (c == 'v') resolved_string.data[resolved_string.size++] = '\v';
                                        else if (c == 'u' || c == 'x')
                                        {
                                            u32 codepoint = 0;
                                            for (umm j = i; !encountered_errors && i < j + (c == 'u' ? 6 : 2); ++i)
                                            {
                                                if (i < raw_string.size && IsDigit(raw_string.data[i]))
                                                {
                                                    codepoint = codepoint * 10 + (raw_string.data[i] - '0');
                                                }
                                                
                                                else if (i < raw_string.size &&
                                                         ToUpper(raw_string.data[i]) >= 'A' &&
                                                         ToUpper(raw_string.data[i]) <= 'F')
                                                {
                                                    codepoint = codepoint * 10 + (ToUpper(raw_string.data[i]) - 'A') + 10;
                                                }
                                                
                                                else
                                                {
                                                    //// ERROR
                                                    
                                                    Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                                    Text_Interval report_highlight = report_text;
                                                    
                                                    ReportLexerError(compiler_state, report_text, report_highlight,
                                                                     "Missing digits in codepoint/byte escape sequence");
                                                    
                                                    encountered_errors = true;
                                                }
                                            }
                                            
                                            if (!encountered_errors)
                                            {
                                                if (codepoint <= U8_MAX)
                                                {
                                                    resolved_string.data[resolved_string.size++] = (u8)codepoint;
                                                }
                                                
                                                else if (codepoint <= 0x7FF)
                                                {
                                                    resolved_string.data[resolved_string.size++] = (u8)(0xC0 | (codepoint & 0x7C0) >> 6);
                                                    resolved_string.data[resolved_string.size++] = (u8)(0x80 | (codepoint & 0x03F) >> 0);
                                                }
                                                
                                                else if (codepoint <= 0xFFFF)
                                                {
                                                    resolved_string.data[resolved_string.size++] = (u8)(0xE0 | (codepoint & 0xF000) >> 12);
                                                    resolved_string.data[resolved_string.size++] = (u8)(0x80 | (codepoint & 0x0FC0) >> 6);
                                                    resolved_string.data[resolved_string.size++] = (u8)(0x80 | (codepoint & 0x003F) >> 0);
                                                }
                                                
                                                else if (codepoint <= 0x10FFFF)
                                                {
                                                    resolved_string.data[resolved_string.size++] = (u8)(0xF0 | (codepoint & 0x1C0000) >> 18);
                                                    resolved_string.data[resolved_string.size++] = (u8)(0x80 | (codepoint & 0x03F000) >> 12);
                                                    resolved_string.data[resolved_string.size++] = (u8)(0x80 | (codepoint & 0x000FC0) >> 6);
                                                    resolved_string.data[resolved_string.size++] = (u8)(0x80 | (codepoint & 0x00003F) >> 0);
                                                }
                                                
                                                else
                                                {
                                                    //// ERROR
                                                    
                                                    Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                                    Text_Interval report_highlight = report_text;
                                                    
                                                    ReportLexerError(compiler_state, report_text, report_highlight,
                                                                     "Codepoint is out of UTF-8 range");
                                                    
                                                    encountered_errors = true;
                                                }
                                            }
                                        }
                                        
                                        else
                                        {
                                            resolved_string.data[resolved_string.size++] = c;
                                        }
                                    }
                                    
                                    else
                                    {
                                        resolved_string.data[resolved_string.size++] = raw_string.data[i];
                                        i += 1;
                                    }
                                }
                            }
                            
                            if (!encountered_errors)
                            {
                                if (is_character)
                                {
                                    if (resolved_string.size == 0)
                                    {
                                        //// ERROR
                                        
                                        Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                        Text_Interval report_highlight = report_text;
                                        
                                        ReportLexerError(compiler_state, report_text, report_highlight, "Character literals must contain one character");
                                        
                                        encountered_errors = true;
                                    }
                                    
                                    else if (resolved_string.size > 4                                             ||
                                             resolved_string.size > 3 && (resolved_string.data[0] & 0xF0) != 0xF0 ||
                                             resolved_string.size > 2 && (resolved_string.data[0] & 0xF0) != 0xE0 ||
                                             resolved_string.size > 1 && (resolved_string.data[0] & 0xF0) != 0xC0)
                                    {
                                        //// ERROR
                                        
                                        Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                                        Text_Interval report_highlight = report_text;
                                        
                                        ReportLexerError(compiler_state, report_text, report_highlight, "Character literals cannot contain more than one character");
                                        
                                        encountered_errors = true;
                                    }
                                    
                                    else
                                    {
                                        // TODO(soimn): This does not account for endianess, is this a problem?
                                        Copy(resolved_string.data, &token->character, resolved_string.size);
                                    }
                                }
                                
                                else token->string = resolved_string;
                            }
                        }
                    }
                    
                    else
                    {
                        //// ERROR
                        
                        Text_Interval report_text = TextInterval(token->text.pos, CURRENT_TOKEN_SIZE(token, offset));
                        Text_Interval report_highlight = report_text;
                        
                        ReportLexerError(compiler_state, report_text, report_highlight, "Encountered an invalid token");
                        
                        encountered_errors = true;
                    }
                    
                } break;
            }
            
            token->text.size = offset - (token->text.pos.column + token->text.pos.offset_to_line);
            
            if (encountered_errors) token->kind = Token_Invalid;
            
            if (token->kind == Token_EndOfStream) break;
        }
    }
    
    return !encountered_errors;
}

#undef CURRENT_TOKEN_SIZE
#undef SINGLE_OR_EQUAL
#undef SINGLE_DOUBLE_OR_EQUAL
#undef SINGLE_DOUBLE_EQUAL_OR_DOUBLEEQUAL