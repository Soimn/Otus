#pragma once

#include "common.h"
#include "memory.h"

typedef U32 Identifier_ID;
typedef U64 Symbol_ID;
typedef U32 Type_ID;
typedef U32 File_ID;

#define INVALID_IDENTIFIER_ID 0
struct Identifier
{
    String string;
    char peek[4];
    Identifier_ID id;
};

struct Symbol_Table_Entry
{
    AST_Scope_ID scope_id;
    
    bool is_resolved;
    
    bool is_type_name;
    bool is_enum_name;
    bool is_function_name;
    bool is_variable_name;
    bool is_constant_name;
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
    char file_name_peek[4];
    
    Dynamic_Array loaded_files;
    
    AST ast;
};

struct Module
{
    // TODO(soimn): Change the storage members from dynamic arrays to memory arenas
    
    // NOTE(soimn): AddOrRetrieveIdentifierID uses the fact that this is a DynamicArray
    Dynamic_Array identifier_table   = DYNAMIC_ARRAY(Identifier, 256); 
    Dynamic_Array identifier_storage = DYNAMIC_ARRAY(U8, 1024);
    
    Bucket_Array files               = BUCKET_ARRAY(File, 8);
    Dynamic_Array file_path_storage  = DYNAMIC_ARRAY(char, 1024);
    
    Dynamic_Array symbol_table       = DYNAMIC_ARRAY(Symbol_Table_Entry, 256);
    Dynamic_Array type_table         = DYNAMIC_ARRAY(Type_Table_Entry, 256);
};

inline Identifier_ID
EnsureIdentifierExistsAndRetrieveIdentifierID(Module* module, String string)
{
    HARD_ASSERT(string.data != 0 && string.size != 0);
    
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
    
    UMM identifier_count = ElementCount(&module->identifier_table);
    
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
        
        new_identifier->string.data = (U8*)PushElement(&module->identifier_storage, string.size);
        Copy(string.data, new_identifier->string.data, string.size);
    }
    
    return result;
}

inline bool
LoadFileForCompilation(Module* module, String file_path, File_ID* out_file_id)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}