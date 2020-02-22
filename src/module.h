struct Parser_Context
{
    Scope_Info* scope;
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
    struct Module* module;
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
    B32 is_currently_used;
    
    String absolute_file_path;
    String file_contents;
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
    Bucket_Array(Export_Info) export_table;
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
    
    union
    {
        // Variable - offset of memory from base address
        U64 address_offset;
        
        // Enum member
        Number enum_member_value;
        
        // Constant
        Number constant_value;
        
        // Function
        // Function_Body_ID function_body_id; ?
    };
};

#define SYMBOL_TABLE_BUCKET_SIZE 10
typedef Bucket_Array Symbol_Table;

struct Export_Info
{
    Bucket_Array(Symbol_ID) exported_symbols;
    Bucket_Array(File_ID) exported_files;
    Bucket_Array(Using_Import) exported_using_imports;
};