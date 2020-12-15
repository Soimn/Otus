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
    umm size;
    umm capacity;
} Dynamic_Array;

#define Dynamic_Array(T) Dynamic_Array

//////////////////////////////////////////

typedef i32 File_ID;

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

enum TOKEN_KIND
{
    Token_Invalid = 0,
    
    Token_Plus,
    Token_Minus,
    Token_Asterisk,
    Token_Slash,
    Token_Percentage,
    Token_Exclamation,
    Token_At,
    Token_Hash,
    Token_Cash,
    Token_Ampersand,
    Token_Equals,
    Token_QuestionMark,
    Token_Pipe,
    Token_Tilde,
    Token_Hat,
    Token_Quote,
    Token_Period,
    Token_Colon,
    Token_Comma,
    Token_Semicolon,
    Token_Less,
    Token_Greater,
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
    Expr_SliceType,
    Expr_ArrayType,
    Expr_DynamicArrayType,
    Expr_PointerType,
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

enum DECL_KIND
{
    Decl_Invalid = 0,
    
    Decl_Variable,
    Decl_Constant,
    Decl_Import,
};

enum AST_NODE_KIND
{
    AST_Invalid = 0,
    
    AST_Directive,
    
    AST_Declaration,
    AST_Expression,
    
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
};

typedef struct Scope
{
    Dynamic_Array(AST_Node) expressions;
    Dynamic_Array(AST_Node) statements;
} Scope;

typedef struct AST_Node
{
    Enum8(AST_NODE_KIND) kind;
    Text_Interval text;
    
    union
    {
        
    };
} AST_Node;

//////////////////////////////////////////

typedef struct File
{
    String path;
    String name;
    String content;
    Dynamic_Array(Token) tokens;
} File;

typedef struct Module
{
    Dynamic_Array(File_ID) files;
} Module;

typedef struct Path_Prefix
{
    String name;
    String path;
} Path_Prefix;

typedef struct Workspace
{
    u8 compiler_state[sizeof(u64) * 4];
    Dynamic_Array(File) files;
    Dynamic_Array(Module) modules;
    Dynamic_Array(Path_Prefix) path_prefixes;
} Workspace;

//////////////////////////////////////////