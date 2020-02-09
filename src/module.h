enum SYMBOL_TYPE
{
    SymbolType_VarName,
    SymbolType_StructName,
    SymbolType_EnumName,
    SymbolType_ProcName,
    //SymbolType_Alias,
};

struct Symbol
{
    String_ID name;
    Enum32(SYMBOL_TYPE) type;
    bool is_global;
};

struct Scope_Chain
{
    Scope_Chain* next;
    Scope_Info* scope;
    U32 current_decl_index;
};

struct Imported_Symbol_Table
{
    // NOTE(soimn): If this id is not equal to INVALID_ID, only symbol lookups in this table from a root with an 
    //              id equal to the restriction_id are valid
    //              #import sets this to INVALID_ID, whilst #import_local sets this to the root node's table id
    Symbol_Table_ID restriction_id;
    
    Symbol_Table_ID table_id;
    U32 num_decls_before_valid;
};

#define SYMBOL_TABLE_LOCAL_CACHE_SIZE 10
#define SYMBOL_TABLE_BUCKET_SIZE 10
struct Symbol_Table
{
    Symbol local_cache[SYMBOL_TABLE_LOCAL_CACHE_SIZE];
    Bucket_Array(Symbol) symbols;
    U32 total_symbol_count;
    
    Bucket_Array(Imported_Symbol_Table) imported_tables;
};

struct Parser_Context
{
    Scope_Info* scope;
    bool global_by_default;
    bool allow_scope_directives;
    bool allow_subscopes;
    bool allow_using;
    bool allow_defer;
    bool allow_return;
    bool allow_loop_jmp;
    bool allow_ctrl_structs;
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
};

enum COMPILATION_STAGE
{
    CompStage_Invalid    = 0,
    CompStage_Init       = 1, // The file is initialized
    CompStage_Parsed     = 2, // The file has undergone parsing
    CompStage_Validated  = 3, // The file has undergone sema
    CompStage_Generated  = 4, // The file has undergone code gen
};

struct File
{
    Enum32(COMPILATION_STAGE) stage;
    String file_path;
    String file_dir;
    String file_contents;
    Parser_State parser_state;
    Symbol_Table_ID file_table_id;
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
    
    Mutex file_array_mutex;
};

inline Symbol_Table_ID
PushSymbolTable(Module* module)
{
    Symbol_Table_ID table_id = (Symbol_Table_ID)ElementCount(&module->symbol_tables);
    PushElement(&module->symbol_tables);
    
    return table_id;
}

inline Symbol_ID
PushSymbol(Module* module, Symbol_Table_ID table_id, String_ID name, Enum32(SYMBOL_TYPE) type)
{
    Symbol_Table* table = (Symbol_Table*)ElementAt(&module->symbol_tables, table_id);
    
    Symbol* symbol = 0;
    
    if (table->total_symbol_count < SYMBOL_TABLE_LOCAL_CACHE_SIZE)
    {
        symbol = &table->local_cache[table->total_symbol_count];
    }
    
    else
    {
        symbol = (Symbol*)PushElement(&table->symbols);
    }
    
    Symbol_ID symbol_id = INDIRECT_ID_ASSEMBLE(table_id, table->total_symbol_count);
    ++table->total_symbol_count;
    
    symbol->name = name;
    symbol->type = type;
    
    return symbol_id;
}