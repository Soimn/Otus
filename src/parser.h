typedef struct Parser_State
{
    Bucket_Array(Token) tokens;
} Parser_State;

ParseFile()
{
    Bucket_Array tokens = LexFile(file_contents, token_arena, string_arena);
    
    Parser_State state = {
        .tokens = tokens,
    };
}