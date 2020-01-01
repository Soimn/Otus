#pragma once

#include "common.h"
#include "memory.h"

#define INVALID_IDENTIFIER_ID 0
struct Identifier
{
    String string;
    char peek[4];
    Identifier_ID id;
};

enum SYMBOL_TABLE_ENTRY_TYPE
{
    SymbolTableEntry_Constant,
    SymbolTableEntry_Function,
    SymbolTableEntry_Variable,
    SymbolTableEntry_StructMember,
    SymbolTableEntry_EnumMember,
    SymbolTableEntry_Type,
};

struct Symbol_Table_Entry
{
    Enum32(SYMBOL_TABLE_ENTRY_TYPE) type;
    U32 statement_index;
    Type_ID type_id;
    // TODO(soimn): Tie a function symbol to the function body
};

// TODO(soimn): It would be interesting to modify this value depending on he most common statement count per scope 
//              gathered from a previous compilation. Might not be a huge speedup, but it could maybe be 
//              benefitial
#define SYMBOL_TABLE_LOCAL_SYMBOL_STORAGE_SIZE 10
struct Symbol_Table
{
    Symbol_Table_Entry first_few_symbols[SYMBOL_TABLE_LOCAL_SYMBOL_STORAGE_SIZE];
    Bucket_Array remaining_symbols;
    U32 total_symbol_count;
};

enum TYPE_TABLE_ENTRY_TYPE
{
    TypeTableEntry_TypeName,
    TypeTableEntry_Struct,
    TypeTableEntry_Pointer,
    TypeTableEntry_Reference,
    TypeTableEntry_Function,
    
    TypeTableEntry_FixedArray,
    TypeTableEntry_DynamicArray,
    TypeTableEntry_ArraySlice,
};

struct Type_Table_Entry
{
    Enum32(TYPE_TABLE_ENTRY_TYPE) type;
    
    union
    {
        /// TypeName
        Symbol_ID symbol_id;
        
        /// Struct
        Dynamic_Array members;
        
        /// Pointer and reference
        Type_ID type_of_pointer;
        
        /// Fixed-, DynamicArray and ArraySlice
        struct
        {
            Type_ID type_of_array_entries;
            UMM fixed_array_size;
        };
    };
};

struct File
{
    String file_path;
    String file_name;
    String file_contents;
    
    Dynamic_Array loaded_files;
    
    Parser_AST parser_ast;
    // Sema_AST sema_ast
    
    bool has_undergone_parsing;
    bool has_undergone_sema;
};

inline Identifier_ID
EnsureIdentifierExistsAndRetrieveIdentifierID(Module* module, String string)
{
    HARD_ASSERT(string.data != 0 && string.size != 0);
    HARD_ASSERT(string.size < U32_MAX); // IMPORTANT TODO(soimn): Do something about this
    
    Identifier target = {};
    target.string = string;
    
    for (U32 i = 0; i < MIN(ARRAY_COUNT(target.peek), string.size); ++i)
    {
        target.peek[i] = string.data[i];
    }
    
    auto CompareIdentifiers = [](Identifier p0, Identifier p1) -> I8
    {
        I8 comparison_result = 0;
        
        for (U32 i = 0; i < ARRAY_COUNT(p0.peek); ++i)
        {
            if (p0.peek[i] > p1.peek[i])
            {
                comparison_result = 1;
                break;
            }
            
            else if (p0.peek[i] < p1.peek[i])
            {
                comparison_result = -1;
                break;
            }
        }
        
        if (comparison_result == 0)
        {
            for (U32 i = ARRAY_COUNT(p0.peek); i < MIN(p0.string.size, p1.string.size); ++i)
            {
                if (p0.string.data[i] > p1.string.data[i])
                {
                    comparison_result = 1;
                }
                
                else if (p0.string.data[i] < p1.string.data[i])
                {
                    comparison_result = -1;
                }
            }
            
            if (comparison_result == 0)
            {
                if (p0.string.size < p1.string.size) comparison_result = -1;
                else if (p0.string.size > p1.string.size) comparison_result = 1;
            }
        }
        
        return comparison_result;
    };
    
    U32 identifier_count = ElementCount(&module->identifier_table);
    
    Identifier_ID result = INVALID_IDENTIFIER_ID;
    if (identifier_count != 0)
    {
        // IMPORTANT NOTE(soimn): This assumes identifier_table is a Dynamic_Array
        Identifier* left  = (Identifier*)ElementAt(&module->identifier_table, 0);
        Identifier* right = (Identifier*)ElementAt(&module->identifier_table, identifier_count - 1);
        
        while (left != right)
        {
            Identifier* mid = (right - left) / 2 + left;
            
            I8 comparison_result = CompareIdentifiers(*mid, target);
            
            if (comparison_result == 0)
            {
                result = mid->id;
            }
            
            else if (comparison_result == 1)
            {
                right = mid;
            }
            
            else
            {
                left = mid;
            }
            
            if (left + 1 == right) break;
        }
    }
    
    if (result == INVALID_IDENTIFIER_ID)
    {
        if (identifier_count == 0)
        {
            void* null_identifier = PushElement(&module->identifier_table);
            ZeroSize(null_identifier, sizeof(Identifier));
        }
        
        target.id = (Identifier_ID)ElementCount(&module->identifier_table);
        PushElement(&module->identifier_table);
        
        Identifier* new_identifier = 0;
        
        for (auto it = Iterate(&module->identifier_table);
             it.current;
             Advance(&it))
        {
            I8 comparison_result = CompareIdentifiers(target, *(Identifier*)it.current);
            
            // NOTE(soimn): If this fires there is something wrong with the contents of the identifier table, as 
            //              this was not true earlier as the only way control flow reaches this point if ther was 
            //              not found an equivalent identifier earlier in the function.
            HARD_ASSERT(comparison_result != 0);
            
            // NOTE(soimn): insert allways if this is the last element
            if (it.current_index == it.size - 1)
            {
                *(Identifier*)it.current = target;
                new_identifier = (Identifier*)it.current;
            }
            
            else if (comparison_result == 1)
            {
                new_identifier = (Identifier*)it.current;
                
                Identifier carry = *(Identifier*)it.current;
                for (; it.current; Advance(&it))
                {
                    Identifier temp = *(Identifier*)it.current;
                    *(Identifier*)it.current = carry;
                    carry = temp;
                }
                
                *new_identifier = target;
            }
        }
        
        new_identifier->string.data = (U8*)PushSize(&module->universal_arena, (U32)string.size, alignof(char));
        Copy(string.data, new_identifier->string.data, string.size);
    }
    
    return result;
}

inline Symbol_Table_ID
AddSymbolTable(Module* module)
{
    Symbol_Table_ID result = (Symbol_Table_ID)ElementCount(&module->symbol_table_array);
    PushElement(&module->symbol_table_array);
    return result;
}

inline Symbol_ID
AddSymbolToSymbolTable(Module* module, Symbol_Table_ID symbol_table_id, Symbol_Table_Entry symbol)
{
    Symbol_Table* table = (Symbol_Table*)ElementAt(&module->symbol_table_array, symbol_table_id);
    
    // TODO(soimn): If the symbol table was not found there is a compiler bug
    HARD_ASSERT(table != 0);
    
    if (table->total_symbol_count >= ARRAY_COUNT(table->first_few_symbols))
    {
        *(Symbol_Table_Entry*)PushElement(&table->remaining_symbols) = symbol;
    }
    
    else
    {
        table->first_few_symbols[table->total_symbol_count] = symbol;
    }
    
    Symbol_ID result = table->total_symbol_count;
    ++table->total_symbol_count;
    
    return result;
}

inline bool
LoadFileForCompilation(Module* module, String file_path, File_ID* out_file_id)
{
    bool encountered_errors = false;
    
    // TODO(soimn): Search module->files and check if this file is already loaded
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}