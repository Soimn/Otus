enum SYMBOL_TYPE
{
    Symbol_Variable,
    Symbol_Function,
    Symbol_Constant,
    Symbol_Type,
    // Symbol_Alias, // TODO(soimn): Should aliases be defined by a separate symbol type?
};

struct Symbol
{
    String text;
    File_And_Line file_and_line;
    Enum32(SYMBOL_TYPE) type;
};

#define SYMBOL_TABLE_LOCAL_CACHE_SIZE 10
struct Symbol_Table
{
    Symbol local_cache[SYMBOL_TABLE_LOCAL_CACHE_SIZE];
    Bucket_Array(Symbol) symbols;
    UMM total_symbol_count;
};

inline Symbol_Table_ID
PushSymbolTable(Module* module)
{
    NOT_IMPLEMENTED;
}

inline Symbol_ID
PushSymbol(Symbol_Table_ID symbol_table)
{
    NOT_IMPLEMENTED;
}

inline Symbol*
GetSymbolByID(Symbol_ID symbol_id)
{
    NOT_IMPLEMENTED;
}