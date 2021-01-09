#ifdef _WIN32

#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)

typedef signed __int8  i8;
typedef signed __int16 i16;
typedef signed __int32 i32;
typedef signed __int64 i64;

typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

#elif __linux__

#define EXPORT __attribute__((__visible__))
#define IMPORT

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
typedef i32 Package_ID;

#define INVALID_FILE_ID -1
#define INVALID_PACKAGE_ID -1

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
    Expr_PolymorphicAlias,
    
    // Literals
    Expr_Number,
    Expr_String,
    Expr_Identifier,
    Expr_StructLiteral,
    Expr_ArrayLiteral,
    Expr_Range,
    
    // Special
    Expr_NamedArgument,
    Expr_Empty,
    
    // Ambiguities
    Expr_SubscriptButMaybeLiteral,
    Expr_CallButMaybeSpecialization,
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
    Enum8(AST_EXPR_KIND) kind;
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
        
        struct
        {
            struct Expression* min;
            struct Expression* max;
            bool is_open;
        } range_literal;
        
        String string;
        Number number;
        
        struct
        {
            struct Expression* name;
            struct Expression* value;
        } named_argument;
        
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
    Statement_StructMember,
    Statement_EnumMember,
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
            Slice(Expression*) types;
            Slice(Expression*) values;
            Slice(Expression*) names;
            bool is_uninitialized;
        } variable_declaration;
        
        struct
        {
            Slice(Expression*) types;
            Slice(Expression*) values;
            Slice(Expression*) names;
            bool is_distinct;
        } constant_declaration;
        
        struct
        {
            Enum8(AST_EXPR_KIND) op;
            Slice(Expression*) left_side;
            Slice(Expression*) right_side;
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
        
        struct
        {
            struct Expression* name;
            struct Expression* type;
            bool is_using;
        } struct_member;
        
        struct
        {
            struct Expression* name;
            struct Expression* value;
        } enum_member;
    };
} Statement;

//////////////////////////////////////////

typedef i32 Symbol_ID;
typedef i32 Type_ID;

enum TYPE_KIND
{
    Type_Invalid = 0,
    
    Type_Int,
    Type_Float,
    Type_Bool,
    Type_Any,
    // distinct alias
    
    Type_Struct,
    Type_Union,
    Type_Enum,
    Type_Proc,
    
    Type_Pointer,
    Type_Array,
    Type_Slice,
    Type_DynamicArray,
};

typedef struct Type
{
    Enum8(TYPE_KIND) kind;
} Type;

enum SYMBOL_KIND
{
    Symbol_Invalid = 0,
    
    Symbol_Variable,
    Symbol_Constant,
    Symbol_Package,
    Symbol_Alias,
    Symbol_Type,
    Symbol_Procedure,
};

typedef struct Symbol
{
    Enum8(SYMBOL_KIND) kind;
} Symbol;

enum TOP_LEVEL_KIND
{
    _Invalid = 0,
    
    _Procedure,
    _Structure,
    _Enumeration,
    _Variable,
    _Constant,
    _Import,
    _Statement,
} TOP_LEVEL_KIND;

typedef struct Top_Level
{
    Enum8(TOP_LEVEL_KIND) kind;
    Package_ID package;
    Symbol_ID symbol;
    Statement* statement;
    bool is_private;  // NOTE(soimn): Private or public, package level
    bool is_exported; // NOTE(soimn): Shared library, exported or "hidden"
} Top_Level;

// TODO(soimn): Function that takes a top level AST Node and generates symbol data for it
// TODO(soimn): The sema stage rips apart multiple * declarations if possible

//////////////////////////////////////////

typedef struct Package
{
    String path;
} Package;

typedef struct Path_Prefix
{
    String name;
    String path;
} Path_Prefix;

typedef struct Workspace
{
    void* compiler_state;
    Slice(String) file_paths;
    Slice(Package) packages;
    Slice(Path_Prefix) path_prefixes;
} Workspace;