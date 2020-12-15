typedef struct Parser_State
{
    Workspace* workspace;
    Token* cursor;
    umm peek_index;
} Parser_State;

inline Token
GetToken(Parser_State state)
{
    state->peek_index = 0;
    return *state->cursor;
}

inline Token
PeekNextToken(Parser_State state)
{
    return state->cursor[state->peek_index++];
}

inline void
SkipTokens(Parser_State state)
{
    state->cursor    += state->peek_index + 1;
    state->peek_index = 0;
}

inline bool
RequireToken(Parser_State state, Enum8(TOKEN_KIND) kind)
{
    Token token = GetToken(state);
    
    if (token.kind == kind) SkipTokens(state);
    
    return (token.kind == kind);
}

//////////////////////////////////////////

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
ResolvePathString(Workspace* workspace, String path_string, String* resolved_path, bool is_file_path)
{
    bool encountered_errors = false;
    
    String prefix_name = {
        .data = path_string.data,
        .size = 0
    };
    
    String extension = {0};
    
    for (umm i = 0; i < path_string.size; ++i)
    {
        if (path_string.data[i] == ':' && prefix_name.size == 0)
        {
            prefix_name.size = i;
        }
        
        else if (path_string.data[i] == '\\' ||
                 path_string.data[i] == ':')
        {
            //// ERROR: Wrong path separator
            encountered_errors = true;
            break;
        }
        
        else if (path_string.data[i] == '.')
        {
            extension = (String){
                .data = path_string.data + (i + 1),
                .size = path_string.size - (i + 1)
            };
            
            if (extension.size == 0)
            {
                //// ERROR: Missing extension after period
                encountered_errors = true;
            }
            
            break;
        }
    }
    
    if (!encountered_errors)
    {
        if (extension.size == 0)
        {
            if (is_file_path)
            {
                //// ERROR: Missing extension
                encountered_errors = true;
            }
        }
        
        else
        {
            if (!is_file_path)
            {
                //// ERROR: Path to directory should not have an extension
                encountered_errors = true;
            }
            
            else if (!StringCompare(extension, CONST_STRING("os")))
            {
                //// ERROR: Wrong extension
                encountered_errors = true;
            }
        }
    }
    
    if (!encountered_errors)
    {
        String prefix_path = {.size = 0};
        String file_path   = path_string;
        
        if (prefix_name.size != 0)
        {
            file_path.data += prefix_path.size + 1;
            file_path.size -= prefix_path.size + 1;
            
            Path_Prefix* found_prefix = 0;
            for (imm i = 0; i < workspace->path_prefixes.size; ++i)
            {
                Path_Prefix* prefix = path_(Path_Prefix*)DynamicArray_ElementAt(&workspace->path_prefixes, i);
                
                if (StringCompare(prefix_name, prefix->name))
                {
                    found_prefix = prefix;
                    break;
                }
            }
            
            if (found_prefix != 0) prefix_string = found_prefix->path;
            else
            {
                //// ERROR: No path prefix with the name ''
                encountered_errors = true;
            }
        }
        
        if (!encountered_errors)
        {
            resolved_path->data = Arena_Allocate(TempArena(workspace), prefix_path.size + file_path.size, ALIGNOF(u8));
            resolved_path->size = prefix_path.size + file_path.size;
            
            Copy(prefix_path.data, resolved_path->data, prefix_path.size);
            Copy(file_path.data, resolved_path->data + prefix_path.size, file_path.size);
        }
    }
    
    return !encountered_errors;
}

bool
ParseFile(Workspace* workspace, File_ID file_id)
{
    ASSERT(file_id != INVALID_FILE_ID);
    
    bool encountered_errors = false;
    
    File* file = DynamicArray_ElementAt(&workspace->files, file_id);
    
    Parser_State state = {
        .workspace  = workspace,
        .cursor     = (Token*)file->tokens.data,
        .peek_index = 0
    };
    
    while (!encountered_errors && state.cursor != Token_EndOfStream)
    {
        Token token = GetToken(&state);
        
        // import "prefix:path/module_name" as AltName;
        // #include "prefix:path/file_name"
        
        if (token.kind == Token_Identifier && token.keyword == Keyword_Import)
        {
            SkipTokens(&state);
            token = GetToken(&state);
            
            if (token.kind != Token_String)
            {
                //// ERROR: Missing path
                encountered_errors = true;
            }
            
            else
            {
                String resolved_path;
                if (!ResolvePathString(workspace, token.string, &resolved_path)) encountered_errors = true;
                else
                {
                    NOT_IMPLEMENTED;
                }
            }
        }
        
        else
        {
            NOT_IMPLEMENTED;
        }
    }
    
    return !encountered_errors;
}