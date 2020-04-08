
typedef struct Imported_Scope
{
    // Access_Chain access_chain
    Scope_ID scope_id;
} Imported_Scope;

typedef struct Block
{
    Text_Interval text;
    Array(Imported_Scope) imported_scopes;
    Array(Declaration) declarations;
    Array(Statement) statements;
    Bucket_Array(Expression) expressions;
} Block;

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
    
    /// Primary
    ExprKind_Identifier,
    ExprKind_Number,
    ExprKind_String,
    ExprKind_Character,
    
    EXPRESSION_KIND_COUNT
};

enum EXPRESSION_FLAG
{
    ExprFlag_Invalid,
    
    // Statement and expression level
    ExprFlag_BoundsCheck,
    
    // Expression level
    ExprFlag_TypeEvalContext,
    ExprFlag_InlineCall,
    ExprFlag_Run,
    ExprFlag_ForeignLibrary,
    ExprFlag_DistinctType,
    
    EXPRESSION_FLAG_COUNT
};

typedef struct Named_Expression
{
    Identifier name;
    bool is_const;
    bool is_baked;
    bool is_used;
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
} Call_Expression;

typedef struct Proc_Expression
{
    Array(Named_Expression) args;
    Array(Named_Expression) return_values;
    bool is_deprecated;
    bool is_inlined;
    bool no_discard;
    bool is_foreign;
    String_Literal foreign_library;
    String_Literal deprecation_note;
    Block block;
} Proc_Expression;

typedef struct Struct_Expression
{
    Array(Named_Expression) args;
    Array(Named_Expression) members;
    U8 forced_alignment;
    bool is_strict;
    bool is_packed;
} Struct_Expression;

typedef struct Enum_Expression
{
    Array(Named_Expression) args;
    Array(Named_Expression) members;
    bool is_strict;
} Enum_Expression;

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
        
        Identifier identifier;
        String_Literal string;
        Character character;
        Number number;
    };
} Expression;









enum DECLARATION_KIND
{
    DeclKind_Invalid,
    
    DeclKind_VariableDecl,
    DeclKind_ConstantDecl,
    DeclKind_UsingDecl,
    
    DECLARATION_KIND_COUNT
};

typedef struct Code_Note
{
    Identifier name;
    Array(Expression*) args;
} Code_Note;

typedef struct Variable_Declaration
{
    Array(Expression*) names;
    Array(Expression*) types;
    Array(Expression*) values;
    bool is_used;
    bool is_uninitialized;
} Variable_Declaration;

typedef struct Constant_Declaration
{
    Array(Expression*) names;
    Array(Expression*) types;
    Array(Expression*) values;
} Constant_Declaration;

typedef struct Using_Declaration
{
    Array(Expression*) expressions;
} Using_Declaration;

typedef struct Declaration
{
    Enum32(DECLARATION_KIND) kind;
    Code_Note note;
    Text_Interval text;
    
    union
    {
        Variable_Declaration var_decl;
        Constant_Declaration const_decl;
        Using_Declaration using_decl;
    };
} Declaration;










enum STATEMENT_KIND
{
    StatementKind_Invalid,
    
    StatementKind_Block,
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

typedef struct If_Statement
{
    Expression* condition;
    Block true_block;
    Block false_block;
} If_Statement;

typedef struct While_Statement
{
    Expression* condition;
    Block block;
} While_Statement;

typedef struct Return_Statement
{
    Array(Named_Expression) values;
} Return_Statement;

typedef struct Defer_Statement
{
    Block block;
} Defer_Statement;

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
        Block block_stmnt;
        If_Statement if_stmnt;
        While_Statement while_stmnt;
        Return_Statement return_stmnt;
        Defer_Statement defer_stmnt;
        Assignment_Statement assignment_stmnt;
        Expression* expression_stmnt;
    }; 
} Statement;