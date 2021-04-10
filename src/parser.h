typedef struct Parser_State
{
    Compiler_State* compiler_state;
    Bucket_Array(Token) tokens;
} Parser_State;

bool
ParseCodeNote(Parser_State state)
{
}

// literals
bool
ParsePrimaryExpression(Parser_State state)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
ParseTypeExpression(Parser_State state)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

// call, subscript, slice, member access
bool
ParsePostfixUnaryExpression(Parser_State state)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

// *, &, !, ~, +, -, cast
bool
ParsePrefixUnaryExpression(Parser_State state)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

// <<, >>, .., ..<
// NOTE(soimn): Special case (..expr) requires checking for .. and ..< before parsing expression
bool
ParseBitShiftLevelExpression(Parser_State state)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

// *, /, %,  &
bool
ParseMulLevelExpression(Parser_State state)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

// +, -, |, ^
bool
ParseAddLevelExpression(Parser_State state)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

// in, not_in
bool
ParseMembershipExpression(Parser_State state)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

// ==, !=, <, >, <=, >=, <=>
bool
ParseComparativeExpression(Parser_State state)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

// &&
bool
ParseLogicalAndExpression(Parser_State state)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

// ||
bool
ParseLogicalOrExpression(Parser_State state)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
ParseExpression(Parser_State state)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
ParseStatement(Parser_State state)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
ParseScope(Parser_State state)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
ParseFile(Compiler_State* compiler_state, )
{
    bool encountered_errors = false;
    
    Bucket_Array tokens = LexFile(file_contents, token_arena, string_arena);
    
    Token* last_token = BucketArray_ElementAt(&tokens, BucketArray_ElementCount(&tokens));
    
    if (last_token.kind == Token_Invalid) encountered_errors = true;
    else
    {
        Parser_State state = {
            .compiler_state = compiler_state,
            .tokens         = tokens,
        };
    }
    
    return !encountered_errors;
}

void
ParsePackage(Compiler_State* compiler_state, Package_ID package_id)
{
    
}