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

enum TOKEN_KIND
{
    Token_Invalid = 0,
    
    Token_Plus,
    Token_Minus,
    Token_Asterisk,
    Token_Slash,
    Token_Percentage,
    Token_Exclamation,
    Token_Ampersand,
    Token_Equal,
    Token_Pipe,
    Token_Tilde,
    Token_Colon,
    Token_Less,
    Token_Greater,
    
    Token_AmpersandAmpersand,
    Token_PipePipe,
    Token_LessLess,
    Token_GreaterGreater,
    
    Token_PlusEqual,
    Token_MinusEqual,
    Token_AsteriskEqual,
    Token_SlashEqual,
    Token_PercentageEqual,
    Token_ExclamationEqual,
    Token_EqualEqual,
    Token_TildeEqual,
    Token_ColonEqual,
    Token_AmpersandEqual,
    Token_PipeEqual,
    Token_LessEqual,
    Token_GreaterEqual,
    
    Token_LessLessEqual,
    Token_GreaterGreaterEqual,
    
    Token_Period,
    Token_PeriodPeriod,
    
    Token_At,
    Token_Cash,
    Token_Hash,
    Token_QuestionMark,
    Token_Hat,
    Token_Quote,
    Token_Comma,
    Token_Semicolon,
    Token_OpenParen,
    Token_CloseParen,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenBrace,
    Token_CloseBrace,
    Token_Underscore,
    
    Token_Number,
    Token_String,
    Token_Identifier,
    
    Token_EndOfStream,
    
    TOKEN_KIND_COUNT
};

enum KEYWORD_KIND
{
    Keyword_Invalid = 0,
    
    Keyword_Package,
    Keyword_Import,
    Keyword_As,
    Keyword_Defer,
    Keyword_Return,
    Keyword_Using,
    Keyword_Proc,
    Keyword_Where,
    Keyword_Struct,
    Keyword_Union,
    Keyword_Enum,
    Keyword_If,
    Keyword_Else,
    Keyword_When,
    Keyword_While,
    Keyword_For,
    Keyword_Break,
    Keyword_Continue,
    Keyword_Inline,
    Keyword_In,
    Keyword_NotIn,
    Keyword_Do,
    
    KEYWORD_KIND_COUNT
};

typedef struct Token
{
    Text_Interval text;
    
    union
    {
        Number number;
        
        struct
        {
            String string;
            Enum8(KEYWORD_KIND) keyword;
        };
    };
    
    Enum8(TOKEN_KIND) kind;
    
} Token;

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
    
    // Unary
    Expr_Neg,
    Expr_Not,
    Expr_BitNot,
    Expr_Reference,
    Expr_Dereference,
    
    // Special
    Expr_Cast,
    Expr_Call,
    
    // Types
    Expr_Slice,
    Expr_Array,
    Expr_DynamicArray,
    Expr_Pointer,
    Expr_Struct,
    Expr_Union,
    Expr_Enum,
    Expr_Proc,
    
    // Literals
    Expr_Number,
    Expr_String,
    Expr_Identifier,
    Expr_StructLiteral,
    Expr_ArrayLiteral,
};

enum AST_NODE_KIND
{
    AST_Invalid = 0,
    
    AST_Empty, // Usefull for @note; statements
    
    AST_Expression,
    
    AST_ImportDecl,
    AST_VariableDecl,
    AST_ConstantDecl,
    
    AST_Scope,
    
    AST_If,
    AST_When, // #if
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
    Dynamic_Array(AST_Node) arguments;
} Attribute;

typedef struct Scope
{
    Dynamic_Array(AST_Node) expressions;
    Dynamic_Array(AST_Node) statements;
} Scope;

typedef struct AST_Node
{
    Enum8(AST_NODE_KIND) kind;
    Enum8(AST_EXPR_KIND) expr_kind;
    Text_Interval text;
    Dynamic_Array(Attribute) attributes;
    
    union
    {
        Scope scope;
        
        struct
        {
            String alias;
            Package_ID package_id;
            bool is_using;
        } import;
        
        struct
        {
            struct AST_Node* init;
            struct AST_Node* condition;
            struct AST_Node* true_body;
            struct AST_Node* false_body;
        } if_statement;
        
        struct AST_Node* expression;
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

//////////////////////////////////////////


// IMPORTANT
// Memory compatibility
// Bucket_Array vs Slice vs Dynamic_Array, how to interface?


// Problem: How to store declarations
// Declarations must be stored such that lookup, creation and deletion is fast without too much fragmentation
// Idea: Store declaration data and all child nodes in one contigous(?) region of memory, such that tree traversal may be faster and memory can be freed with a single free list entry
// Idea: The parser uses the TempArena, until an entire top-level declaration is parsed, then the declaration is copied into persistent memory
