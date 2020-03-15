struct Comp_State
{
    
};

enum EXPR_TYPE
{
    ExprType_Invalid,
    
    ExprType_Negate,
    // ExprType_Inc,
    // ExprType_Dec,
    ExprType_Add,
    ExprType_Sub,
    ExprType_Mul,
    ExprType_Div,
    ExprType_Mod,
    ExprType_BitAnd,
    ExprType_BitOr,
    ExprType_BitNot,
    ExprType_BitShiftLeft,
    ExprType_BitShiftRight,
    ExprType_LogicalAnd,
    ExprType_LogicalOr,
    ExprType_LogicalNot,
    
    ExprType_AddEQ,
    ExprType_SubEQ,
    ExprType_MulEQ,
    ExprType_DivEQ,
    ExprType_ModEQ,
    ExprType_BitAndEQ,
    ExprType_BitOrEQ,
    ExprType_BitNotEQ,
    ExprType_BitShiftLeftEQ,
    ExprType_BitShiftRightEQ,
    ExprType_LogicalAndEQ,
    ExprType_LogicalOrEQ,
    ExprType_LogicalNotEQ,
    
    ExprType_CmpEQ,
    ExprType_CmpNotEQ,
    ExprType_CmpLess,
    ExprType_CmpGreater,
    ExprType_CmpLessOrEQ,
    ExprType_CmpGreaterOrEQ,
    
    ExprType_Dereference,
    ExprType_Reference,
    ExprType_Slice,
    ExprType_Subscript,
    ExprType_Member,
    ExprType_Call,
    ExprType_Cast,
    
    ExprType_StructLiteral,
    ExprType_ArrayLiteral,
    
    ExprType_Sequence,
    
    ExprType_Identifier,
    ExprType_Blank,
    ExprType_Number,
    ExprType_String,
};

struct Expression
{
    Enum32(EXPR_TYPE) type;
    U64 flags;
    
    union
    {
        struct
        {
            Expression* left;
            Expression* right;
        };
        
        Expression* operand;
        
        struct
        {
            Expression* slice_expr;
            Expression* slice_start;
            Expression* slice_length;
        };
        
        struct
        {
            Expression* subscript_expr;
            Expression* subscript_index;
        };
        
        struct
        {
            Expression* proc_expr;
            // Array(Expression*) proc_args;
        };
        
        struct
        {
            Expression* cast_expr;
            // Type cast_type;
        };
        
        struct
        {
            // Type struct_type;
            // Array(Expression*) struct_values;
        };
        
        struct
        {
            // Type array_type;
            // Number array_size;
            // Array(Expression*) array_values;
        };
        
        // Array(Expression*) sequence;
        
        // Identifier identifier;
        // Number number;
        // String string;
    };
};

enum DECL_TYPE
{
    DeclType_Constant,
    DeclType_Variable,
    DeclType_Struct,
    DeclType_Enum,
    DeclType_Proc,
};

struct Declaration
{
    Enum32(DECL_TYPE) type;
    U64 flags;
    
    // Symbol symbol;
    
    union
    {
        struct
        {
            // Type const_type;
            Expression const_value;
        };
        
        struct
        {
            // Type var_type;
            // bool is_uninitialized;
            Expression var_value;
        };
        
        struct
        {
            // Array(Declaration*) struct_args;
            // Array(Declaration*) struct_members;
        };
        
        struct
        {
            // Type enum_type;
            // Array(Expression*) enum_members;
        };
        
        struct
        {
            // Array(Declaration*) proc_args;
            // Type proc_return;
            // Statement* proc_scope;
        };
    };
};

enum STATEMENT_TYPE
{
    StatementType_Expression,
    StatementType_Declaration,
    StatementType_Scope,
    StatementType_If,
    StatementType_For,
    StatementType_While,
    StatementType_DoWhile,
    StatementType_Continue,
    StatementType_Break,
    StatementType_Defer,
    StatementType_Return,
    StatementType_Using,
};

struct Statement
{
    Enum32(STATEMENT_TYPE) type;
    U64 flags;
    
    union
    {
        Scope_ID scope;
        Declaration declaration;
        Expression expression;
        
        struct
        {
            Expression if_condition;
            Statement* if_true_scope;
            Statement* if_false_scope;
        };
        
        struct
        {
            Expression while_condition;
            Statement* while_scope;
        };
        
        struct
        {
            Expression do_while_condition;
            Statement* do_while_scope;
        };
        
        Expression label;
        
        Statement* defer_statement;
        Expression return_value;
        Expression using_value;
    };
};

struct Scope
{
    
};