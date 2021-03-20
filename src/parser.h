typedef struct Parser_State
{
    Compiler_State* compiler_state;
    Bucket_Array(Token) tokens;
} Parser_State;

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