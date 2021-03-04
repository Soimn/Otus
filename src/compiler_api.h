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
    Expr_IsEqual,
    Expr_IsNotEqual,
    Expr_IsLess,
    Expr_IsGreater,
    Expr_IsLessOrEqual,
    Expr_IsGreaterOrEqual,
    Expr_Chain,
    Expr_IsIn,
    Expr_IsNotIn,
    Expr_Interval,
    
    // Unary
    Expr_Neg,
    Expr_Not,
    Expr_BitNot,
    Expr_Reference,
    Expr_Dereference,
    
    // Postfix binary
    Expr_Member,
    
    // Postfix unary
    Expr_Call,
    Expr_Subscript,
    
    // Types
    Expr_Slice,
    Expr_Array,
    Expr_DynamicArray,
    Expr_Pointer,
    Expr_Struct,
    Expr_Enum,
    Expr_Proc,
    Expr_Polymorphic,
    
    // Literals
    Expr_Number,
    Expr_String,
    Expr_Identifier,
    Expr_Boolean,
    Expr_StructLiteral,
    Expr_ArrayLiteral,
    
    // Special
    Expr_NamedArgument,
    Expr_Compound,
    Expr_Cast,
    Expr_Transmute,
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
        
        Number number;
        String string;
        bool boolean;
        
        struct
        {
            struct Expression* name;
            struct Expression* value;
        } named_argument;
        
        struct
        {
            struct Expression* expression;
        } compound;
        
        struct
        {
            struct Expression* type;
            struct Expression* operand;
        } cast_expression;
        
        struct
        {
            struct Expression* type;
            struct Expression* operand;
        } transmute_expression;
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

typedef struct Package
{
    String name;
    String path;
    Slice(File_ID) files;
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
} Workspace;

//////////////////////////////////////////

typedef struct Workspace_Options
{
    Slice(Path_Prefix) path_prefixes;
} Workspace_Options;

API_FUNC Workspace* Workspace_Open(Workspace_Options workspace_options, String main_file);
API_FUNC void Workspace_Close(Workspace* workspace);

typedef struct Declaration
{
    Statement* ast;
    Package_ID package;
    String link_name; // NOTE(soimn): size 0 to use default
} Declaration;

API_FUNC void Workspace_CommitDeclaration(Workspace* workspace, Declaration declaration);

// TODO(soimn):
// * Parsing
//   - Foreign imports
//   - "forward" declarations
// * API
//   - error reporting