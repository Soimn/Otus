typedef struct Block
{
    Text_Interval text;
    
    Array(Statement) statements;
    Array(Expression) expressions;
    
    HIDDEN(bool) export_by_default;
} Block;

typedef struct Scope
{
    Block block;
    
    HIDDEN(Array(Symbol)) symbols;
    HIDDEN(Scope_ID) scope_id;
} Scope;

typedef struct Scope_Chain
{
    struct Scope_Chain* next;
    Scope* scope;
} Scope_Chain;

enum EXPRESSION_KIND
{
    ExprKind_Invalid,
    
    /// Binary
    ExprKind_Add,                // +
    ExprKind_Sub,                // -
    ExprKind_Mul,                // *
    ExprKind_Div,                // /
    ExprKind_Mod,                // %
    ExprKind_And,                // &&
    ExprKind_Or,                 // ||
    ExprKind_BitAnd,             // &
    ExprKind_BitOr,              // |
    ExprKind_BitShiftLeft,       // <<
    ExprKind_BitShiftRight,      // >>
    ExprKind_CmpEqual,           // ==
    ExprKind_CmpNotEqual,        // !=
    ExprKind_CmpGreater,         // >
    ExprKind_CmpLess,            // <
    ExprKind_CmpGreaterOrEqual,  // >=
    ExprKind_CmpLessOrEqual,     // <=
    ExprKind_Member,             // Member name access (e.g. entity.health)
    
    /// Unary
    ExprKind_Not,                // !expr
    ExprKind_BitNot,             // ~expr
    ExprKind_Neg,                // -expr
    ExprKind_Reference,          // ^expr
    ExprKind_Dereference,        // expr^
    ExprKind_SliceType,          // []expr
    ExprKind_ArrayType,          // [expr]expr
    ExprKind_DynamicArrayType,   // [..]expr
    ExprKind_Polymorphic,        // $expr
    
    /// Special
    ExprKind_Subscript,          // expr[expr]
    ExprKind_Slice,              // expr[expr:expr]
    ExprKind_TypeLiteral,        // expr{expr, expr, ...}
    ExprKind_Call,
    ExprKind_Struct,
    ExprKind_Enum,
    ExprKind_Proc,
    ExprKind_Run,
    
    /// Primary
    ExprKind_Identifier,
    ExprKind_Number,
    ExprKind_String,
    ExprKind_Character,
    ExprKind_Trilean,
    
    EXPRESSION_KIND_COUNT
};

enum EXPRESSION_FLAG
{
    ExprFlag_BoundsCheck      = 0x01,
    ExprFlag_TypeEvalContext  = 0x02,
    ExprFlag_DistinctType     = 0x10,
};

typedef struct Named_Expression
{
    Identifier name;
    bool is_const;
    bool is_baked;
    bool is_using;
    struct Expression* type;
    struct Expression* value;
} Named_Expression;

typedef struct Binary_Expression
{
    struct Expression* left;
    struct Expression* right;
} Binary_Expression;

typedef struct Unary_Expression
{
    struct Expression* operand;
} Unary_Expression;

typedef struct Array_Expression
{
    struct Expression* pointer;
    struct Expression* start;
    struct Expression* length;
} Array_Expression;

typedef struct Call_Expression
{
    struct Expression* pointer;
    Array(Named_Expression) args;
    tri forced_inline;
} Call_Expression;

typedef struct Struct_Expression
{
    Array(Named_Expression) args;
    Array(Named_Expression) members;
    struct Expression* alignment;
    bool is_strict;
    bool is_packed;
} Struct_Expression;

typedef struct Enum_Expression
{
    Array(Named_Expression) args;
    Array(Named_Expression) members;
    bool is_strict;
} Enum_Expression;

typedef struct Proc_Expression
{
    Array(Named_Expression) args;
    Array(Named_Expression) return_values;
    struct Expression* foreign_library;
    struct Expression* deprecation_notice;
    bool is_deprecated;
    bool is_inlined;
    bool no_discard;
    bool is_foreign;
    bool has_body;
    Scope scope;
    HIDDEN(bool) is_polymorphic;
} Proc_Expression;

typedef struct Expression
{
    Enum32(EXPRESSION_KIND) kind;
    Flag32(EXPRESSION_FLAG) flags;
    Text_Interval text;
    
    union
    {
        Binary_Expression binary_expr;
        Unary_Expression unary_expr;
        Array_Expression array_expr;
        Call_Expression call_expr;
        
        Proc_Expression proc_expr;
        Struct_Expression struct_expr;
        Enum_Expression enum_expr;
        
        struct Expression* run_expr;
        
        Identifier identifier;
        String_Literal string;
        Character character;
        Number number;
        tri trilean;
    };
} Expression;








enum STATEMENT_KIND
{
    StatementKind_Invalid,
    
    StatementKind_ImportDecl,
    StatementKind_VarDecl,
    StatementKind_ConstDecl,
    StatementKind_UsingDecl,
    
    StatementKind_Scope,
    StatementKind_If,
    StatementKind_While,
    StatementKind_Break,
    StatementKind_Continue,
    StatementKind_Return,
    StatementKind_Defer,
    StatementKind_Assignment,
    StatementKind_Expression,
    
    STATEMENT_KIND_COUNT
};

typedef struct Code_Note
{
    String name;
    Text_Interval text;
} Code_Note;

typedef struct Import_Declaration
{
    File_ID file_id;
    Identifier alias;
} Import_Declaration;

typedef struct Variable_Declaration
{
    Array(Code_Note) code_note;
    Array(Identifier) names;
    Array(Expression*) types;
    Array(Expression*) values;
    bool is_using;
    bool is_uninitialized;
    
    bool is_exported;
} Variable_Declaration;

typedef struct Constant_Declaration
{
    Array(Code_Note) code_note;
    Array(Identifier) names;
    Array(Expression*) types;
    Array(Expression*) values;
    
    bool is_exported;
} Constant_Declaration;

typedef struct Using_Declaration
{
    Array(Expression*) expressions;
} Using_Declaration;

typedef struct If_Statement
{
    Expression* condition;
    Scope true_scope;
    Scope false_scope;
    
    bool is_const;
} If_Statement;

typedef struct While_Statement
{
    Expression* condition;
    Scope scope;
} While_Statement;

typedef struct Return_Statement
{
    Array(Named_Expression) values;
} Return_Statement;

typedef struct Assignment_Statement
{
    Enum32(EXPRESSION_KIND) kind;
    Array(Expression*) left;
    Array(Expression*) right;
} Assignment_Statement;

typedef struct Statement
{
    Enum32(STATEMENT_KIND) kind;
    Text_Interval text;
    
    union
    {
        Scope scope;
        
        Import_Declaration import_decl;
        Variable_Declaration var_decl;
        Constant_Declaration const_decl;
        Using_Declaration using_decl;
        
        If_Statement if_stmnt;
        While_Statement while_stmnt;
        Return_Statement return_stmnt;
        Assignment_Statement assignment_stmnt;
        Expression* expression_stmnt;
    }; 
} Statement;