#include "compiler_api.h"

// TODO(soimn): Remove these
#include <stdlib.h> // malloc & free
#include <stdio.h>  // printf and putc
#include <stdarg.h> // va_*

#define CONST_STRING(str) (String){.data = (u8*)(str), .size = sizeof(str) - 1}

#define U8_MAX  ((u8)0xFF)
#define U16_MAX ((u16)0xFFFF)
#define U32_MAX ((u32)0xFFFFFFFF)
#define U64_MAX ((u64)0xFFFFFFFFFFFFFFFF)

#define I8_MAX  ((i8)(U8_MAX >> 1))
#define I16_MAX ((i16)(U16_MAX >> 1))
#define I32_MAX ((i32)(U32_MAX >> 1))
#define I64_MAX ((i64)(U64_MAX >> 1))

#define I8_MIN  ((i8) 1 << 7)
#define I16_MIN ((i16)1 << 15)
#define I32_MIN ((i32)1 << 31)
#define I64_MIN ((i64)1 << 63)

#define CONCAT_(x, y) x##y
#define CONCAT(x, y) CONCAT_(x, y)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define BYTES(X)     (umm)(X)
#define KILOBYTES(X) (BYTES(X)     * 1024ULL)
#define MEGABYTES(X) (KILOBYTES(X) * 1024ULL)
#define GIGABYTES(X) (MEGABYTES(X) * 1024ULL)

#ifndef OTUS_DISABLE_ASSERT
#define ASSERT(EX) ((EX) ? 0 : *(volatile int*)0)
#else
#define ASSERT(EX)
#endif

#define INVALID_CODE_PATH ASSERT(!"INVALID_CODE_PATH")
#define INVALID_DEFAULT_CASE default: ASSERT(!"INVALID_DEFAULT_CASE")
#define NOT_IMPLEMENTED ASSERT(!"NOT_IMPLEMENTED")

#define OFFSETOF(T, E) (umm)&((T*)0)->E
#define ALIGNOF(T) (u8)OFFSETOF(struct { char c; T type; }, type)

#define ARRAY_COUNT(EX) (sizeof(EX) / sizeof((EX)[0]))

#define Slice_ElementAt(array, type, index) (type*)((u8*)(array)->data + sizeof(type) * (index))
#define DynamicArray_ElementAt(array, type, index) (type*)((u8*)(array)->data + sizeof(type) * (index))
#define SLICE_TO_DYNAMIC_ARRAY(slice) (Dynamic_Array){.data = (slice).data, .size = (slice).size}

//////////////////////////////////////////

void*
System_AllocateMemory(umm size)
{
    void* result = malloc(size);
    ASSERT(result != 0 && (((umm)result + 7) & ~7) == (umm)result);
    
    for (u8* scan = result; scan < (u8*)result + size; ++scan)
    {
        *scan = 0;
    }
    
    return result;
}

void
System_FreeMemory(void* memory)
{
    free(memory);
}

#define Arg_List va_list
#define ARG_LIST_START va_start
#define ARG_LIST_GET_ARG va_arg

void
System_PrintChar(char c)
{
    putchar(c);
}

void
System_PrintString(String string)
{
    printf("%.*s", (int)string.size, string.data);
}

struct Memory_Arena;
bool
System_ReadEntireFile(String path, struct Memory_Arena* arena, String* content)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

//////////////////////////////////////////

Text_Interval
TextInterval(Text_Pos pos, u32 size)
{
    Text_Interval interval = {
        .pos  = pos,
        .size = size
    };
    
    return interval;
}

Text_Interval
TextInterval_Merge(Text_Interval i0, Text_Interval i1)
{
    ASSERT(i0.pos.file == i1.pos.file);
    ASSERT(i0.pos.offset_to_line + i0.pos.column + i0.size <= i1.pos.offset_to_line + i1.pos.column + i1.size);
    
    Text_Interval interval = {
        .pos  = i0.pos,
        .size = (i1.size + i1.pos.column + i1.pos.offset_to_line) - (i0.pos.column + i0.pos.offset_to_line)
    };
    
    return interval;
}

Text_Interval
TextInterval_FromEndpoints(Text_Pos p0, Text_Pos p1)
{
    ASSERT(p0.file == p1.file);
    ASSERT(p0.offset_to_line + p0.column <= p1.offset_to_line + p1.column);
    
    u32 size = (p1.column + p1.offset_to_line) - (p0.column + p0.offset_to_line);
    
    return TextInterval(p0, size);
}

//////////////////////////////////////////

#include "memory.h"
#include "string.h"

typedef struct Compiler_State
{
    Workspace workspace;
    Memory_Arena persistent_memory;
    Memory_Arena temp_memory;
    Declaration_Iterator declaration_iterator;
} Compiler_State;

//////////////////////////////////////////

bool ParseFile(Compiler_State* compiler_state, Package_ID package_id, File_ID file_id);
bool SemaValidateDeclarationAndGenerateSymbolInfo(Compiler_State* compiler_state, Declaration declaration);
bool TypeCheckDeclaration(Compiler_State* compiler_state, Declaration declaration);

API_FUNC Workspace*
Workspace_Open(Workspace_Options workspace_options, String main_file)
{
    Compiler_State* compiler_state = System_AllocateMemory(sizeof(Compiler_State));
    ZeroStruct(compiler_state);
    
    NOT_IMPLEMENTED;
    
    return &compiler_state->workspace;
}

API_FUNC void
Workspace_Close(Workspace* workspace)
{
    Compiler_State* compiler_state = (Compiler_State*)workspace;
    
    Arena_FreeAll(&compiler_state->persistent_memory);
    Arena_FreeAll(&compiler_state->temp_memory);
    
    NOT_IMPLEMENTED;
}

API_FUNC bool
Workspace_PopDeclaration(Workspace* workspace, Declaration* declaration)
{
    bool is_empty = false;
    
    Compiler_State* compiler_state = (Compiler_State*)workspace;
    
    DeclarationPool_AdvanceIterator(&workspace->meta_pool, &compiler_state->declaration_iterator, true);
    
    if (compiler_state->declaration_iterator.current == 0) is_empty = true;
    else
    {
        Declaration_Iterator* it = &compiler_state->declaration_iterator;
        
        *declaration = *it->current;
        it->block->free_map &= ~(1 << (it->index % 64));
    }
    
    return !is_empty;
}

API_FUNC bool
Workspace_PushDeclaration(Workspace* workspace, Declaration declaration, Enum8(DECLARATION_STAGE_KIND) stage)
{
    bool encountered_errors = false;
    
    Compiler_State* compiler_state = (Compiler_State*)workspace;
    
    if (stage < workspace->prep_option) stage = workspace->prep_option;
    
    if (stage == DeclarationStage_SemaChecked || stage == DeclarationStage_TypeChecked)
    {
        encountered_errors = !SemaValidateDeclarationAndGenerateSymbolInfo(compiler_state, declaration);
        
        if (stage == DeclarationStage_TypeChecked)
        {
            encountered_errors = !TypeCheckDeclaration(compiler_state, declaration);
        }
    }
    
    if (!encountered_errors)
    {
        Declaration_Pool_Block* block = workspace->meta_pool.first;
        while (block != 0)
        {
            if (block->free_map != ~(u64)0) break;
            else block = block->next;
        }
        
        if (block == 0)
        {
            void* memory = Arena_Allocate(&compiler_state->persistent_memory,
                                          sizeof(Declaration_Pool_Block) + sizeof(Declaration) * 64,
                                          ALIGNOF(u64));
            
            Declaration_Pool_Block* new_block = memory;
            ZeroStruct(new_block);
            
            new_block->declarations.data = new_block + 1;
            new_block->declarations.size = 64;
            
            if (workspace->meta_pool.first) workspace->meta_pool.current->next = new_block;
            else                            workspace->meta_pool.first         = new_block;
            
            workspace->meta_pool.current = new_block;
            block                        = workspace->meta_pool.current;
            
            if (workspace->meta_pool.first == new_block)
            {
                compiler_state->declaration_iterator = DeclarationPool_CreateIterator(&workspace->meta_pool);
            }
        }
        
        u64 free_spot = (block->free_map + 1) & ~block->free_map;
        
        // NOTE(soimn): free_spot_index = log_2(free_spot)
        //              the bit pattern of a float is proportional to its log
        f32 f = (f32)free_spot;
        umm free_spot_index = *(u32*)&f / (1 << 23) - 127;
        
        *((Declaration*)block->declarations.data + free_spot_index) = declaration;
        
        block->free_map |= free_spot;
    }
    
    return !encountered_errors;
}

API_FUNC
bool
Workspace_CommitDeclaration(Workspace* workspace, Declaration declaration)
{
    bool encountered_errors = false;
    
    Compiler_State* compiler_state = (Compiler_State*)workspace;
    
    encountered_errors = (!SemaValidateDeclarationAndGenerateSymbolInfo(compiler_state, declaration) ||
                          !TypeCheckDeclaration(compiler_state, declaration));
    
    
    if (!encountered_errors)
    {
        Declaration_Pool_Block* block = workspace->commit_pool.current;
        
        if (block == 0)
        {
            void* memory = Arena_Allocate(&compiler_state->persistent_memory,
                                          sizeof(Declaration_Pool_Block) + sizeof(Declaration) * 64,
                                          ALIGNOF(u64));
            
            Declaration_Pool_Block* new_block = memory;
            ZeroStruct(new_block);
            
            new_block->declarations.data = new_block + 1;
            new_block->declarations.size = 64;
            
            if (workspace->meta_pool.first) workspace->meta_pool.current->next = new_block;
            else                            workspace->meta_pool.first         = new_block;
            
            workspace->meta_pool.current = new_block;
            block                        = workspace->meta_pool.current;
        }
        
        umm free_spot = block->free_map + 1;
        
        // NOTE(soimn): free_spot_index = log_2(free_spot)
        //              the bit pattern of a float is proportional to its log
        f32 f = (f32)free_spot;
        umm free_spot_index = *(u32*)&f / (1 << 23) - 127;
        
        *((Declaration*)block->declarations.data + free_spot_index) = declaration;
        
        block->free_map <<= 1;
        block->free_map += 1;
    }
    
    return !encountered_errors;
}

//////////////////////////////////////////

#include "lexer.h"
#include "parser.h"
#include "checker.h"