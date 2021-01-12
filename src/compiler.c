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

/*
bool
ResolvePathPrefix(Compiler_State* compiler_state, String* path_string, String* prefix)
{
    String prefix_name = {0};
    
    for (umm i = 0; i < path_string.size; ++i)
    {
        if (path_string.data[i] == ':')
        {
            prefix_name.data = path_string.data;
            prefix_name.size = i;
        }
    }
    
    // NOTE(soimn): For error messages, replaced with path if successfull
    *prefix = prefix_name;
    
    bool encountered_errors = false;
    
    if      (prefix_name.data == 0 && prefix_name.size == 0); // NOTE(soimn): Do nothing, path is valid
    else if (prefix_name.data != 0 && prefix_name.size == 0) encountered_errors = true;
    else
    {
        Slice* path_prefixes = &compiler_state->workspace.path_prefixes;
        
        umm i = 0;
        for (; i < path_prefixes->size; ++i)
        {
            if (StringCompare(prefix_name, path_prefixes[i].name))
            {
                break;
            }
        }
        
        if (i == path_prefixes.size) encountered_errors = true;
        else
        {
            *prefix = path_prefixes[i].path;
            
            path_string->data += prefix_name.size - 1;
            path_string->size -= prefix_name.size - 1;
            
            String result;
            result.size = prefix->size + path_string->size;
            result.data = Arena_Allocate(compiler_state->temp_memory, result.size, ALIGNOF(u8));
            
            Copy(prefix->data, result->data, prefix->size);
            Copy(path_string->data, result->data + prefix->size, path_string->size);
            
            *path_string = result;
        }
    }
    
    return !encountered_errors;
}

File_ID
RegisterFilePath(Compiler_State* compiler_state, String path)
{
    File_ID result = INVALID_FILE_ID;
    
    for (umm i = 0; i < compiler_state->workspace.file_paths.size; ++i)
    {
        if (StringCompare(DynamicArray_ElementAt(compiler_state->workspace.file_paths, String, i), path))
        {
            result = i;
            break;
        }
    }
    
    if (result == INVALID_FILE_ID)
    {
        Dynamic_Array* file_paths = compiler_state->workspace.file_paths;
        
        if (compiler_state->workspace.file_paths.size == compiler_state->workspace.file_paths.capacity)
        {
            umm new_cap = MAX(file_paths->capacity * 1.5, 256);
            
            void* memory = Arena_Allocate(&compiler_state->persistent_memory, new_cap * sizeof(String), ALIGNOF(String));
            
            Copy(file_paths->data, memory, file_paths->size * sizeof(String));
            
            Arena_FreeSize(&compiler_state->persistent_memory, file_paths->data, file_paths->capacity);
            
            file_paths->data     = memory;
            file_paths->capacity = new_cap;
        }
        
        result = file_paths->size;
        
        *((String*)file_paths->data + file_paths->size) = path;
        file_paths->size += 1;
    }
    
    return result;
}
*/
//////////////////////////////////////////

#include "memory.h"
#include "string.h"

typedef struct Compiler_State
{
    Workspace workspace;
    Memory_Arena persistent_memory;
    Memory_Arena temp_memory;
} Compiler_State;

//////////////////////////////////////////

API_FUNC Workspace*
Workspace_Open(Slice(Path_Prefix) path_prefixes, String main_file)
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

API_FUNC Declaration_Iterator
Workspace_IterateDeclarations(Workspace* workspace)
{
    Declaration_Iterator it = {
        .block   = workspace->check_pool.first,
        .index   = 0,
        .current = 0
    };
    
    while (it.block != 0 && it.block->free_map == 0)
    {
        it.index += 64;
        it.block  = it.block->next;
    }
    
    if (it.block != 0)
    {
        u64 first_decl = (~it.block->free_map + 1) & it.block->free_map;
        
        // NOTE(soimn): index += log_2(first_decl)
        //              the bit pattern of a float is proportional to its log
        f32 f      = first_decl;
        it.index += *(u32*)&f / (1 << 23) - 127;
        
        it.current = (Declaration*)it.block->declarations.data + it.index % 64;
    }
    
    return it;
}

API_FUNC void
Workspace_AdvanceIterator(Workspace* workspace, Declaration_Iterator* it, bool should_loop)
{
    it->index  += 1;
    it->current = 0;
    if (it->index % 64 == 0) it->block = it->block->next;
    
    bool looped_once = false;
    for (;;)
    {
        if (it->block == 0)
        {
            it->block = workspace->check_pool.first;
            
            if (!should_loop || looped_once) break;
            else looped_once = true;
        }
        
        u64 free_map = it->block->free_map & (~0ULL << (it->index % 64));
        
        if (free_map != 0)
        {
            u64 first_decl = (~it->block->free_map + 1) & it->block->free_map;
            
            // NOTE(soimn): index += log_2(first_decl)
            //              the bit pattern of a float is proportional to its log
            f32 f      = first_decl;
            it->index += *(u32*)&f / (1 << 23) - 127;
            
            it->current = (Declaration*)it->block->declarations.data + it->index % 64;
        }
        
        else
        {
            it->index += 64 - it->index % 64;
            it->block  = it->block->next;
        }
    }
}

API_FUNC Declaration
Workspace_CheckoutDeclaration(Workspace* workspace, Declaration_Iterator it)
{
    Declaration result = {0};
    
    if (it.current != 0)
    {
        result = *it.current;
        it.block->free_map &= ~(1 << (it.index % 64));
        
        ZeroStruct(it.current);
    }
    
    return result;
}

void
AddDeclarationToPool(Compiler_State* compiler_state, Declaration_Pool* pool, Declaration declaration)
{
    Declaration_Pool_Block* block = pool->first;
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
        
        if (pool->first) pool->current->next = new_block;
        else                             pool->first         = new_block;
        
        pool->current = new_block;
        block         = pool->current;
    }
    
    u64 free_spot = (block->free_map + 1) & ~block->free_map;
    
    // NOTE(soimn): free_spot_index = log_2(free_spot)
    //              the bit pattern of a float is proportional to its log
    f32 f = free_spot;
    umm free_spot_index = *(u32*)&f / (1 << 23) - 127;
    
    Declaration* free_decl = (Declaration*)block->declarations.data + free_spot_index;
    
    void RegenerateSymbolInfoForDeclaration(Compiler_State* compiler_state, Declaration* old_decl, Declaration* decl);
    
    RegenerateSymbolInfoForDeclaration(compiler_state, &declaration, free_decl);
}

API_FUNC void
Workspace_CheckinDeclaration(Workspace* workspace, Declaration declaration)
{
    AddDeclarationToPool((Compiler_State*)workspace, &workspace->check_pool, declaration);
}

API_FUNC void
Workspace_CommitDeclaration(Workspace* workspace, Declaration declaration)
{
    AddDeclarationToPool((Compiler_State*)workspace, &workspace->commit_pool, declaration);
}

//////////////////////////////////////////

#include "lexer.h"
#include "parser.h"
#include "checker.h"