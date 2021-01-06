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
    Expr_Spread,
    
    // Postfix
    Expr_Cast,
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
    Expr_Empty,
    
    // Ambiguities
    Expr_CallButMaybeCastOrSpecialization,
    Expr_SubscriptButMaybeLiteral,
};

enum AST_NODE_KIND
{
    AST_Invalid = 0,
    
    AST_Empty,
    AST_Expression,
    
    AST_ImportDecl,
    AST_VariableDecl,
    AST_ConstantDecl,
    
    AST_Scope,
    
    AST_If,
    AST_When,
    AST_While,
    AST_For,
    AST_Break,
    AST_Continue,
    
    AST_Return,
    AST_Using,
    AST_Defer,
    
    AST_Assignment,
};

typedef struct Attribute
{
    String name;
    Dynamic_Array(AST_Node*) arguments;
} Attribute;

typedef struct Scope
{
    Dynamic_Array(AST_Node*) expressions;
    Dynamic_Array(AST_Node*) statements;
} Scope;

typedef struct Named_Argument
{
    struct AST_Node* value;
    struct AST_Node* name;
} Named_Argument;

typedef struct Parameter
{
    Dynamic_Array(Attribute) attributes;
    struct AST_Node* name;
    struct AST_Node* type;
    struct AST_Node* value;
    bool is_using;
} Parameter;

typedef struct Return_Value
{
    Dynamic_Array(Attribute) attributes;
    struct AST_Node* type;
    struct AST_Node* name;
} Return_Value;

typedef struct Struct_Member
{
    Dynamic_Array(Attribute) attributes;
    struct AST_Node* name;
    struct AST_Node* type;
    bool is_using;
} Struct_Member;

typedef struct Enum_Member
{
    Dynamic_Array(Attribute) attributes;
    struct AST_Node* value;
    struct AST_Node* name;
} Enum_Member;

typedef struct AST_Node
{
    Enum8(AST_NODE_KIND) kind;
    Enum8(AST_EXPR_KIND) expr_kind;
    Text_Interval text;
    Dynamic_Array(Attribute) attributes;
    Dynamic_Array(String) directives;
    
    union
    {
        Scope scope;
        
        struct
        {
            String alias;
            Package_ID package_id;
            bool is_using;
        } import_declaration;
        
        struct
        {
            struct AST_Node* init;
            struct AST_Node* condition;
            struct AST_Node* true_body;
            struct AST_Node* false_body;
        } if_statement;
        
        struct
        {
            struct AST_Node* condition;
            struct AST_Node* true_body;
            struct AST_Node* false_body;
        } when_statement;
        
        struct
        {
            struct AST_Node* init;
            struct AST_Node* condition;
            struct AST_Node* step;
            struct AST_Node* body;
            String label;
        } while_statement;
        
        struct
        {
            String label;
        } for_statement;
        
        struct
        {
            String label;
        } break_statement, continue_statement;
        
        struct
        {
            struct AST_Node* scope;
        } defer_statement;
        
        struct
        {
            struct AST_Node* symbol_path;
            String alias;
        } using_statement;
        
        struct
        {
            Dynamic_Array(Named_Argument) arguments;
        } return_statement;
        
        struct
        {
            Dynamic_Array(AST_Node*) types;
            Dynamic_Array(AST_Node*) values;
            Dynamic_Array(String) names;
            bool is_uninitialized;
        } variable_declaration;
        
        struct
        {
            Dynamic_Array(AST_Node*) types;
            Dynamic_Array(AST_Node*) values;
            Dynamic_Array(String) names;
            bool is_distinct;
        } constant_declaration;
        
        struct
        {
            Enum8(AST_EXPR_KIND) op;
            Dynamic_Array(AST_Node*) left_side;
            Dynamic_Array(AST_Node*) right_side;
        } assignment_statement;
        
        struct
        {
            struct AST_Node* condition;
            struct AST_Node* true_expr;
            struct AST_Node* false_expr;
        } ternary_expression;
        
        struct
        {
            struct AST_Node* left;
            struct AST_Node* right;
        } binary_expression;
        
        struct
        {
            struct AST_Node* operand;
        } unary_expression;
        
        struct
        {
            struct AST_Node* operand;
            struct AST_Node* index;
            struct AST_Node* length;
            struct AST_Node* step;
        } subscript_expression;
        
        struct
        {
            struct AST_Node* pointer;
            Dynamic_Array(Named_Argument) arguments;
        } call_expression;
        
        struct
        {
            struct AST_Node* size;
            struct AST_Node* type;
        } array_type;
        
        struct
        {
            Dynamic_Array(Parameter) parameters;
            Dynamic_Array(Return_Value) return_values;
            struct AST_Node* where_expression;
            Dynamic_Array(Attributes) body_attributes;
            struct AST_Node* body;
        } proc;
        
        struct
        {
            Dynamic_Array(Parameter) parameters;
            Dynamic_Array(Struct_Member) members;
            struct AST_Node* where_expression;
            Dynamic_Array(Attribute) body_attributes;
            bool is_union;
        } structure;
        
        struct
        {
            struct AST_Node* type;
            Dynamic_Array(Enum_Member) members;
        } enumeration;
        
        struct
        {
            struct AST_Node* type;
            Dynamic_Array(Named_Argument) arguments;
        } struct_literal;
        
        struct
        {
            struct AST_Node* type;
            Dynamic_Array(AST_Node*) elements;
        } array_literal;
        
        String string;
        Number number;
    };
} AST_Node;

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