struct Parser_Context
{
    Scope_Info* scope;
    bool allow_scope_directives;
    bool allow_loose_expressions;
    bool allow_subscopes;
    bool allow_using;
    bool allow_defer;
    bool allow_return;
    bool allow_loop_jmp;
    bool allow_ctrl_structs;
    bool allow_var_decls;
    bool allow_proc_decls;
    bool allow_struct_decls;
    bool allow_enum_decls;
    bool allow_const_decls;
};

struct Lexer_State
{
    U8* current;
    U8* line_start;
    UMM column;
    UMM line;
};

struct Parser_State
{
    Lexer_State lexer_state;
    AST* ast;
    struct File* file;
    struct Module* module;
    
    bool export_by_default;
};

enum COMPILATION_STAGE
{
    CompStage_Invalid    = 0,
    CompStage_Init       = 1, // The file has been initialized
    CompStage_Parsed     = 2, // The file has been parsed
    CompStage_Validated  = 3, // The file has undergone sema
    CompStage_Generated  = 4, // The file has undergone code gen
};

struct File
{
    Enum32(COMPILATION_STAGE) stage;
    B32 is_worked;
    
    String file_path;
    String file_contents;
    Parser_State parser_state;
    AST ast;
};

struct Module
{
    Memory_Arena main_arena;
    Memory_Arena string_arena;
    Memory_Arena file_arena;
    
    Bucket_Array(File) files;
    Bucket_Array(String) string_table;
    Bucket_Array(Symbol_Table) symbol_tables;
};

enum SYMBOL_TYPE
{
    Symbol_Var,
    Symbol_TypeName,
    Symbol_Constant,
    Symbol_EnumValue,
    Symbol_Func,
};

struct Symbol
{
    Enum32(SYMBOL_TYPE) symbol_type;
    String_ID name;
    Type_ID type;
    
    // NOTE(soimn): Add information about functions
};

#define SYMBOL_TABLE_BUCKET_SIZE 10
typedef Bucket_Array Symbol_Table;

struct Imported_Symbol_Table
{
    // NOTE(soimn): This stores the chain of symbols needed to access the same symbols as the ones in the 
    //              referenced table, from the current scope. Only usefull when using statements are involved.
    Symbol_ID* access_chain;
    
    Symbol_Table_ID id;
};

struct Scoped_Symbol_Table
{
    Imported_Symbol_Table table;
    U32 num_decls_before_valid;
    
    B32 is_exported;
};

struct Type
{
    /*Symbol_ID symbol;
    Symbol_Table_ID table;
    Bucket_Array(Imported_Symbol_Table) imported_tables;*/
};

struct Type_Table
{
};