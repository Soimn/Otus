#ifdef _WIN32

typedef signed __int8  s8;
typedef signed __int16 s16;
typedef signed __int32 s32;
typedef signed __int64 s64;

typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

#elif __linux__

typedef __INT8_TYPE__  s8;
typedef __INT16_TYPE__ s16;
typedef __INT32_TYPE__ s32;
typedef __INT64_TYPE__ s64;

typedef __UINT8_TYPE__  u8;
typedef __UINT16_TYPE__ u16;
typedef __UINT32_TYPE__ u32;
typedef __UINT64_TYPE__ u64;

#else

#error "unsupported platform"

#endif

typedef u64 umm;
typedef s64 smm;

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

typedef struct String
{
    u8* data;
    umm size;
} String;

typedef struct Array
{
    void* allocator;
    void* data;
    umm size;
    umm capacity;
} Array;

#define Array(T) Array

typedef struct Slice
{
    void* data;
    umm size;
} Slice;

#define Slice(T) Slice

typedef struct Number
{
    u64 integer;
    f64 floating;
    
    b8 is_float;
    b8 is_negative;
} Number;

/////////////////////////////////////////////

#define INVALID_ID (s64)-1
typedef s64 Package_ID;
typedef s64 File_ID;

typedef u64 Declaration_ID;
typedef u64 Scope_ID;
typedef u64 Type_ID;
typedef u64 Symbol_ID;

/////////////////////////////////////////////

typedef struct Named_Value
{
    Slice(Code_Note) notes;
    struct Expression* name;
    struct Expression* value;
} Named_Argument;

typedef struct Code_Note
{
    String name;
    Slice(Named_Value) arguments;
} Code_Note;

enum EXPRESSION_KIND
{
    Expression_Invalid = 0,
    Expression_None,
    
    // unary - arithmetic, bitwise and logical
    Expression_Plus,
    Expression_Minus,
    Expression_BitNot,
    Expression_Not,
    
    // binary - arithmetic, bitwise and logical
    Expression_Add,
    Expression_Sub,
    Expression_Mul,
    Expression_Div,
    Expression_Remainder,
    Expression_BitAnd,
    Expression_BitOr,
    Expression_BitXor,
    Expression_BitShiftLeft,
    Expression_BitShiftRight,
    Expression_And,
    Expression_Or,
    
    // binary - comparative
    Expression_IsEqual,
    Expression_IsNotEqual,
    Expression_IsLess,
    Expression_IsLessOrEqual,
    Expression_IsGreater,
    Expression_IsGreaterOrEqual,
    
    // binary - special
    Expression_Member,
    Expression_Chain,
    Expression_In,
    Expression_NotIn,
    Expression_ClosedInterval,
    Expression_HalfOpenInterval,
    
    // unary
    Expression_Reference,
    Expression_Dereference,
    Expression_Call,
    Expression_Subscript,
    Expression_Slice,
    Expression_Cast,
    
    // unary - types
    Expression_SliceType,
    Expression_ArrayType,
    Expression_PointerType,
    Expression_PolymorphicType,
    
    // primary - types
    Expression_Struct,
    Expression_Enum,
    Expression_Proc,
    Expression_Macro,
    
    // primary
    Expression_Compound,
    Expression_StructLiteral,
    Expression_ArrayLiteral,
    Expression_Identifier,
    Expression_Number,
    Expression_Character,
    Expression_String,
    Expression_Boolean,
    
    // Sema
    Expression_ResolvedIdentifier,
};

typedef struct Proc_Parameter
{
    Slice(Code_Note) notes;
    Slice(Expression) names;
    struct Expression* type;
    struct Expression* value;
    bool is_using;
} Proc_Parameter;

typedef struct Expression
{
    Slice(Code_Note) notes;
    Enum8(EXPRESSION_KIND) kind;
    
    union
    {
        struct
        {
            struct Expression* left;
            struct Expression* right;
        };
        
        struct Expression* operand;
        
        struct
        {
            struct Expression* pointer;
            Slice(Named_Value) arguments;
        } call_expression;
        
        struct
        {
            struct Expression* pointer;
            struct Expression* start;
            struct Expression* end;
            struct Expression* step;
        } slice_expression;
        
        struct
        {
            struct Expression* pointer;
            struct Expression* index;
        } subscript_expression;
        
        struct
        {
            struct Expression* type;
            struct Expression* operand;
        } cast_expression;
        
        struct
        {
            struct Expression* size;
        } array_type;
        
        struct
        {
            Slice(Named_Value) parameters;
            Slice(AST_Node) members;
            bool is_declaration;
        } struct_expression;
        
        struct
        {
            Slice(Named_Value) members;
            bool is_declaration;
        } enum_expression;
        
        struct
        {
            Slice(Proc_Parameter) parameters;
            Slice(Proc_Parameter) return_values;
            struct Expression* where_expression;
            struct AST_Node* body;
            bool is_declaration;
        } proc_expression;
        
        struct
        {
            Slice(Proc_Parameter) parameters;
            struct Expression* where_expression;
            struct AST_Node* body;
            bool is_declaration;
        } macro_expression;
        
        struct Expression* compound_expression;
        
        struct
        {
            struct Expression* type;
            Slice(Named_Value) arguments;
        } struct_literal, array_literal;
        
        String identifier;
        Number number;
        u32 character;
        String string;
        bool boolean;
        
        Symbol_ID symbol;
    };
} Expression;

enum AST_NODE_KIND
{
    AST_Invalid = 0,
    
    AST_Expression,
    AST_Block,
    AST_If,
    AST_When,
    AST_While,
    AST_For,
    AST_Continue,
    AST_Break,
    AST_Using,
    AST_Defer,
    AST_Return,
    AST_Assignment,
    AST_Constant,
    AST_Variable,
    AST_Import,
    AST_Load,
    AST_Foreign,
};

typedef struct AST_Node
{
    Slice(Code_Note) notes;
    Enum8(AST_NODE_KIND) kind;
    
    union
    {
        Expression* expression_statement;
        
        struct
        {
            Expression* label;
            Scope_ID scope_id;
            Slice(AST_Node) nodes;
            bool is_do_block;
        } block_statement;
        
        struct
        {
            Expression* label;
            struct AST_Node* init;
            Expression* condition;
            struct AST_Node* true_body;
            struct AST_Node* false_body;
        } if_statement, when_statement;
        
        struct
        {
            Expression* label;
            struct AST_Node* init;
            Expression* condition;
            struct AST_Node* step;
            struct AST_Node* body;
        } while_statement;
        
        struct
        {
            Expression* label;
            Slice(Expression) iterator_symbols;
            Expression* iterator;
            struct AST_Node* body;
        } for_statement;
        
        struct
        {
            Expression* label;
        } continue_statement, break_statement;
        
        struct
        {
            Expression* symbol;
        } using_statement;
        
        struct
        {
            struct AST_Node* statement;
        } defer_statement;
        
        struct
        {
            Slice(Named_Value) arguments;
        } return_statement;
        
        struct
        {
            Enum8(EXPRESSION_KIND) operation;
            Slice(Expression) lhs, rhs;
        } assignment_statement;
        
        struct
        {
            Slice(Expression) names;
            Slice(Expression) types;
            Slice(Expression) values;
            bool is_declaration;
            bool is_using;
        } constant_declaration, variable_declaration;
        
        struct
        {
            Expression* path;
            Expression* alias;
        } import_declaration;
        
        struct
        {
            Expression* path;
        } load_declaration;
        
        struct AST_Node* foreign_declaration;
    };
} AST_Node;

/////////////////////////////////////////////

enum ERROR_CODE
{
    Error_Success = 0,
    
    Error_LexerErrorsStart,
    Error_FailedToLoadFile = Error_LexerErrorsStart,
    Error_InvalidDigitCountHexFloat,
    Error_FloatTruncatedToZero,
    Error_IntegerTooLarge,
    Error_UnterminatedString,
    Error_UnterminatedCharacterLiteral,
    Error_MissingDigitsInCodepointEscapeSequence,
    Error_CodepointOutOfRange,
    Error_IllegalCharacterSize,
    Error_LexerErrorsEnd = Error_IllegalCharacterSize,
    /*
    Error_ParserErrorsStart,
    Error_ = Error_ParserErrorsStart,
    Error_ParserErrorsEnd = Error_,
    
    Error_SemaErrorsStart,
    Error_ = Error_SemaErrorsStart,
    Error_SemaErrorsEnd = Error_,
    
    Error_TypeCheckErrorsStart,
    Error_ = Error_TypeCheckErrorsStart,
    Error_TypeCheckErrorsEnd = Error_,
    
    Error_CodeGenErrorsStart,
    Error_ = Error_CodeGenErrorsStart,
    Error_CodeGenErrorsEnd = Error_,
    
    Error_InternalErrorsStart,
    Error_ = Error_InternalErrorsStart,
    Error_InternalErrorsEnd = Error_,
*/
};

/////////////////////////////////////////////

// What about local functions and polymorphism?
// What about "which decls belong to this package/file queries"?
// How should the symbol tables be organized?
// What kinds of symbols are there?

typedef struct Declaration
{
    Declaration_ID id;
    Package_ID package;
    u32 flags;
    AST_Node* ast;
    Symbol symbol;
    Symbol_Table symbol_table;
} Declaration;

typedef struct Declaration_Block
{
    u64 committed_map;
    u64 exists_map;
    Declaration declarations[64];
} Declaration_Block;

typedef struct Declaration_Iterator
{
    Array(Declaration_Block)* blocks;
    u32 block_index;
    u32 block_offset;
} Declaration_Iterator;

/////////////////////////////////////////////

typedef struct Path_Prefix
{
    String name;
    String path;
} Path_Prefix;

typedef struct Workspace_Options
{
    Slice(Path_Prefix) path_prefixes;
} Workspace_Options;

typedef struct Workspace
{
    Array(Declaration_Block) declaration_blocks;
    Slice(Path_Prefix) path_prefixes;
    bool has_errors;
} Workspace;

/////////////////////////////////////////////

Workspace* OpenWorkspace(Workspace_Options options, String main_file);
void CloseWorkspace(Workspace* workspace);

ParseString(Workspace* workspace, String string);

bool CommitDeclaration(Workspace* workspace, Declaration_ID declaration_id);
bool CommitDeclaration(Workspace* workspace, Declaration_Iterator iterator);

AddNewDeclaration(Workspace* workspace, Package_ID package, AST_Node* ast, Symbol_Table symbol_table)
// decl funcs
// sym funcs
// type funcs
// bc funcs