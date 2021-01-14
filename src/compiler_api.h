#ifdef _WIN32

#ifdef OTUS_DLL_EXPORTS
# define API_FUNC __declspec(dllexport)
#else
# define API_FUNC __declspec(dllimport)
#endif

typedef signed __int8  i8;
typedef signed __int16 i16;
typedef signed __int32 i32;
typedef signed __int64 i64;

typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

#elif __linux__

#ifdef OTUS_DLL_EXPORTS
# define API_FUNC __attribute__ ((visibility("default")))
#else
# define API_FUNC __attribute__ ((visibility("default")))
#endif

typedef __INT8_TYPE__  i8;
typedef __INT16_TYPE__ i16;
typedef __INT32_TYPE__ i32;
typedef __INT64_TYPE__ i64;

typedef __UINT8_TYPE__  u8;
typedef __UINT16_TYPE__ u16;
typedef __UINT32_TYPE__ u32;
typedef __UINT64_TYPE__ u64;

#else

#error "unsupported platform"

#endif

typedef u64 umm;
typedef i64 imm;

typedef float  f32;
typedef double f64;

typedef u8  b8;
typedef u16 b16;
typedef u32 b32;
typedef u64 b64;

typedef b8 bool;

#define false 0
#define true 1

#define Enum8(NAME)  u8
#define Enum16(NAME) u16
#define Enum32(NAME) u32
#define Enum64(NAME) u64

#define Flag8(NAME)  u8
#define Flag16(NAME) u16
#define Flag32(NAME) u32
#define Flag64(NAME) u64

typedef struct String
{
    u8* data;
    umm size;
} String;

typedef struct Slice
{
    void* data;
    umm size;
} Slice;

#define Slice(T) Slice

typedef struct Dynamic_Array
{
    void* allocator;
    void* data;
    u64 size;
    u64 capacity;
} Dynamic_Array;

#define Dynamic_Array(T) Dynamic_Array

//////////////////////////////////////////

typedef i32 File_ID;
typedef u32 Package_ID;
typedef u32 Scope_ID;
typedef u32 Type_ID;
typedef struct Symbol_ID { Scope_ID scope; u32 index; } Symbol_ID;


#define INVALID_FILE_ID -1

//////////////////////////////////////////

typedef struct Number
{
    union
    {
        u64 integer;
        f64 floating;
    };
    
    bool is_float;
    u8 min_fit_bits;
} Number;

typedef struct Text_Pos
{
    File_ID file;
    u32 offset_to_line;
    u32 line;
    u32 column;
} Text_Pos;

typedef struct Text_Interval
{
    Text_Pos pos;
    u32 size;
} Text_Interval;

//////////////////////////////////////////

enum EXPR_KIND
{
    Expr_Invalid = 0,
    
    // Ternary
    Expr_Ternary,
    
    // Binary
    Expr_Add,
    Expr_Sub,
    Expr_Mul,
    Expr_Div,
    Expr_Mod,
    Expr_And,
    Expr_Or,
    Expr_BitAnd,
    Expr_BitOr,
    Expr_BitXor,
    Expr_Member,
    Expr_IsEqual,
    Expr_IsNotEqual,
    Expr_IsLess,
    Expr_IsGreater,
    Expr_IsLessOrEqual,
    Expr_IsGreaterOrEqual,
    
    // Unary
    Expr_Neg,
    Expr_Not,
    Expr_BitNot,
    Expr_Reference,
    Expr_Dereference,
    
    // Postfix
    Expr_Call,
    Expr_Subscript,
    
    // Types
    Expr_Slice,
    Expr_Array,
    Expr_DynamicArray,
    Expr_Pointer,
    Expr_Struct,
    Expr_Union,
    Expr_Enum,
    Expr_Proc,
    Expr_Polymorphic,
    
    // Literals
    Expr_Number,
    Expr_String,
    Expr_Identifier,
    Expr_StructLiteral,
    Expr_ArrayLiteral,
    Expr_Interval,
    
    // Special
    Expr_NamedArgument,
    Expr_Compound,
    Expr_Empty,
};

typedef struct Attribute
{
    String name;
    Slice(Expression*) arguments;
} Attribute;

typedef struct Directive
{
    String name;
    Slice(Expression*) arguments;
} Directive;

typedef struct Expression
{
    Enum8(EXPR_KIND) kind;
    Text_Interval text;
    Slice(Directive) directives;
    
    union
    {
        struct
        {
            struct Expression* condition;
            struct Expression* true_expr;
            struct Expression* false_expr;
        } ternary_expression;
        
        struct
        {
            struct Expression* left;
            struct Expression* right;
        } binary_expression;
        
        struct
        {
            struct Expression* operand;
        } unary_expression;
        
        struct
        {
            struct Expression* min;
            struct Expression* max;
            bool is_open;
        } interval_expression;
        
        struct
        {
            struct Expression* pointer;
            struct Expression* index;
            struct Expression* length;
            struct Expression* step;
        } subscript_expression;
        
        struct
        {
            struct Expression* pointer;
            Slice(Expression*) arguments;
        } call_expression;
        
        struct
        {
            struct Expression* size;
            struct Expression* type;
        } array_type;
        
        struct
        {
            struct Statement* parameters;
            struct Statement* return_values;
            struct Expression* where_expression;
            struct Statement* body;
        } proc;
        
        struct
        {
            struct Statement* parameters;
            struct Expression* where_expression;
            struct Statement* members;
            bool is_union;
        } structure;
        
        struct
        {
            struct Expression* type;
            struct Statement* members;
        } enumeration;
        
        struct
        {
            struct Expression* type;
            Slice(Expression*) arguments;
        } struct_literal;
        
        struct
        {
            struct Expression* type;
            Slice(Expression*) elements;
        } array_literal;
        
        String string;
        Number number;
        
        struct
        {
            struct Expression* name;
            struct Expression* value;
        } named_argument;
        
        struct
        {
            struct Expression* expression;
        } compound;
    };
    
} Expression;

enum STATEMENT_KIND
{
    Statement_Invalid = 0,
    
    Statement_Empty,
    Statement_Block,
    Statement_Expression,
    
    Statement_ImportDecl,
    Statement_VariableDecl,
    Statement_ConstantDecl,
    
    Statement_If,
    Statement_When,
    Statement_While,
    Statement_Break,
    Statement_Continue,
    
    Statement_Return,
    Statement_Using,
    Statement_Defer,
    
    Statement_Assignment,
    
    Statement_Parameter,
    Statement_ReturnValue,
};

typedef struct Statement
{
    Enum8(STATEMENT_KIND) kind;
    Text_Interval text;
    Slice(Attribute) attributes;
    
    union
    {
        struct
        {
            Scope_ID scope_id;
            Expression* label;
            Slice(Expression*) expressions;
            Slice(Statement*) statements;
        } block;
        
        struct
        {
            String alias;
            Package_ID package_id;
            bool is_using;
        } import_declaration;
        
        struct
        {
            Expression* label;
            struct Statement* init;
            Expression* condition;
            struct Statement* true_body;
            struct Statement* false_body;
        } if_statement;
        
        struct
        {
            Expression* label;
            Expression* condition;
            struct Statement* true_body;
            struct Statement* false_body;
        } when_statement;
        
        struct
        {
            Expression* label;
            struct Statement* init;
            Expression* condition;
            struct Statement* step;
            struct Statement* body;
        } while_statement;
        
        struct
        {
            Expression* label;
        } break_statement, continue_statement;
        
        struct
        {
            struct Statement* statement;
        } defer_statement;
        
        struct
        {
            Expression* symbol_path;
            Expression* alias;
        } using_statement;
        
        struct
        {
            Slice(Expression*) arguments;
        } return_statement;
        
        struct
        {
            union
            {
                struct
                {
                    Slice(Expression*) names;
                    Slice(Expression*) types;
                    Slice(Expression*) values;
                };
                
                struct
                {
                    Expression* name;
                    Expression* type;
                    Expression* value;
                };
            };
            
            bool is_multi;
            bool is_uninitialized;
            bool is_using;
        } variable_declaration;
        
        struct
        {
            union
            {
                struct
                {
                    Slice(Expression*) names;
                    Slice(Expression*) types;
                    Slice(Expression*) values;
                };
                
                struct
                {
                    Expression* name;
                    Expression* type;
                    Expression* value;
                };
            };
            
            bool is_multi;
            bool is_distinct;
            bool is_using;
        } constant_declaration;
        
        struct
        {
            union
            {
                struct
                {
                    Slice(Expression*) left_exprs;
                    Slice(Expression*) right_exprs;
                };
                
                struct
                {
                    struct Expression* left_expr;
                    struct Expression* right_expr;
                };
            };
            
            Enum8(AST_EXPR_KIND) op;
            bool is_multi;
        } assignment_statement;
        
        Expression* expression;
        
        struct
        {
            struct Expression* name;
            struct Expression* type;
            struct Expression* default_value;
            bool is_using;
        } parameter;
        
        struct
        {
            struct Expression* name;
            struct Expression* type;
        } return_value;
    };
} Statement;

//////////////////////////////////////////

enum TYPE_KIND
{
    Type_Invalid = 0,
    
    Type_Int,
    Type_Float,
    Type_Bool,
    Type_Any,
    Type_Alias,
    
    Type_Struct,
    Type_Enum,
    Type_Proc,
    
    Type_Pointer,
    Type_Array,
    Type_Slice,
    Type_DynamicArray,
};

typedef struct Value
{
    Enum8(TYPE_KIND) kind;
    
    union
    {
        i64 int64;
        u64 uint64;
        f32 float32;
        f64 float64;
        bool boolean;
        
        struct
        {
            u64 data;
            Type_ID type;
        } any;
        
        struct
        {
            Slice(Value) members;
        } structure;
        
        struct
        {
            Slice(Value) members;
        } enumeration;
        
        u64 pointer;
        
        struct
        {
            Slice(Value) elements;
        } array;
        
        struct
        {
            u64 pointer;
            u64 size;
        } slice;
        
        struct
        {
            u64 allocator;
            u64 pointer;
            u64 size;
            u64 capacity;
        } dynamic_array;
    };
} Value;

typedef struct Type_Field_Info
{
    String name;
    Type_ID type;
    Value default_value;
} Type_Field_Info;

typedef struct Type
{
    Enum8(TYPE_KIND) kind;
    
    union
    {
        struct
        {
            u8 size;
            bool is_signed;
        } integer;
        
        struct
        {
            u8 size;
        } floating;
        
        struct
        {
            Type_ID type;
            bool is_distinct;
        } alias;
        
        struct
        {
            Slice(Type_Field_Info) members;
            bool is_union;
        } structure;
        
        struct
        {
            Slice(Type_Field_Info) members;
        } enumeration;
        
        struct
        {
            Slice(Type_Field_Info) parameters;
            Slice(Type_Field_Info) return_values;
        } procedure;
        
        struct
        {
            Type_ID type;
        } pointer;
        
        struct
        {
            Type_ID type;
            u64 size;
        } array;
        
        struct
        {
            Type_ID type;
        } slice;
        
        struct
        {
            Type_ID type;
        } dynamic_array;
    };
} Type;

enum SYMBOL_KIND
{
    Symbol_Invalid = 0,
    
    Symbol_Variable,
    Symbol_Constant,
    Symbol_Alias,
    Symbol_Type,
    Symbol_Procedure,
    Symbol_Package,
};

typedef struct Symbol
{
    String name;
    
    union
    {
        Type_ID type;
        Symbol_ID aliased_symbol;
        Package_ID package;
    };
    
    Enum8(SYMBOL_KIND) kind;
    bool is_public;
} Symbol;

typedef struct Scope
{
    Slice(Symbol) symbols;
    Slice(Expression*) expressions;
    Slice(Statement*) statements;
} Scope;

//////////////////////////////////////////

enum DECLARATION_KIND
{
    Declaration_Invalid = 0,
    
    Declaration_Procedure,
    Declaration_Structure,
    Declaration_Enumeration,
    Declaration_Variable,
    Declaration_Constant,
    Declaration_Import,
};

enum DECLARATION_STAGE_KIND
{
    DeclarationStage_Parsed,
    DeclarationStage_SemaChecked,
    DeclarationStage_TypeChecked,
};

typedef struct Declaration
{
    Enum8(DECLARATION_KIND) kind;
    Enum8(DECLARATION_STAGE_KIND) stage;
    
    Package_ID package;
    Statement* ast;
    
    u64 user_data;
} Declaration;

typedef struct Declaration_Pool_Block
{
    struct Declaration_Pool_Block* next;
    u64 free_map;
    Slice(Declaration) declarations;
} Declaration_Pool_Block;

typedef struct Declaration_Pool
{
    Declaration_Pool_Block* first;
    Declaration_Pool_Block* current;
} Declaration_Pool;

typedef struct Declaration_Iterator
{
    Declaration_Pool_Block* block;
    u64 index;
    Declaration* current;
} Declaration_Iterator;

//////////////////////////////////////////

Declaration_Iterator
DeclarationPool_CreateIterator(Declaration_Pool* pool)
{
    Declaration_Iterator it = {
        .block   = pool->first,
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
        f32 f     = first_decl;
        it.index += *(u32*)&f / (1 << 23) - 127;
        
        it.current = (Declaration*)it.block->declarations.data + it.index % 64;
    }
    
    return it;
}

void
DeclarationPool_AdvanceIterator(Declaration_Pool* pool, Declaration_Iterator* it, bool should_loop)
{
    if (pool->first == 0) it->current = 0;
    else
    {
        it->index  += 1;
        it->current = 0;
        if (it->index % 64 == 0) it->block = it->block->next;
        
        bool looped_once = false;
        for (;;)
        {
            if (it->block == 0)
            {
                it->block = pool->first;
                
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
}

//////////////////////////////////////////

typedef struct Package
{
    String name;
    String path;
    Slice(File_ID) files;
    //Dynamic_Array(Symbol) symbols;
} Package;

typedef struct Path_Prefix
{
    String name;
    String path;
} Path_Prefix;

typedef struct Workspace
{
    Dynamic_Array(String) file_paths;
    Dynamic_Array(Package) packages;
    Slice(Path_Prefix) path_prefixes;
    
    Declaration_Pool meta_pool;
    Declaration_Pool commit_pool;
} Workspace;

//////////////////////////////////////////

API_FUNC Workspace* Workspace_Open(Slice(Path_Prefix) path_prefixes, String main_file);
API_FUNC void Workspace_Close(Workspace* workspace);
API_FUNC bool Workspace_PopDeclaration(Workspace* workspace, Declaration* declaration);
API_FUNC bool Workspace_PushDeclaration(Workspace* workspace, Declaration declaration, Enum8(DECLARATION_STAGE_KIND) stage);
API_FUNC bool Workspace_CommitDeclaration(Workspace* workspace, Declaration declaration);