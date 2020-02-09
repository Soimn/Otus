enum AST_NODE_TYPE
{
    ASTNode_Invalid,
    
    ASTNode_Root,
    ASTNode_Scope,
    
    ASTNode_If,
    ASTNode_Else,
    ASTNode_While,
    ASTNode_Break,
    ASTNode_Continue,
    
    ASTNode_Using,
    ASTNode_Defer,
    ASTNode_Return,
    
    ASTNode_FuncDecl,
    ASTNode_StructDecl,
    ASTNode_EnumDecl,
    ASTNode_ConstDecl,
    ASTNode_VarDecl,
    
    ASTNode_Expression,
};

enum AST_EXPR_TYPE
{
    ASTExpr_Invalid,
    
    ASTExpr_Add,
    ASTExpr_Sub,
    ASTExpr_Mul,
    ASTExpr_Div,
    ASTExpr_Mod,
    
    ASTExpr_BitAnd,
    ASTExpr_BitOr,
    ASTExpr_BitNot,
    ASTExpr_BitShiftLeft,
    ASTExpr_BitShiftRight,
    
    ASTExpr_LogicalAnd,
    ASTExpr_LogicalOr,
    ASTExpr_LogicalNot,
    
    ASTExpr_CmpEqual,
    ASTExpr_CmpNotEqual,
    ASTExpr_CmpLess,
    ASTExpr_CmpLessOrEqual,
    ASTExpr_CmpGreater,
    ASTExpr_CmpGreaterOrEqual,
    
    ASTExpr_TypeCast,
    ASTExpr_Subscript,
    ASTExpr_Slice,
    ASTExpr_Member,
    ASTExpr_Call,
    ASTExpr_Reference,
    ASTExpr_Dereference,
    
    ASTExpr_Assignment, // TODO(soimn): Should this be split up into the different types of assignment?
    ASTExpr_Increment,
    ASTExpr_Decrement,
    
    ASTExpr_Identifier,
    ASTExpr_Number,
    ASTExpr_String,
};

struct Scope_Info
{
    Symbol_Table_ID table;
    U32 num_decls;
};

struct AST_Node
{
    Enum32(AST_NODE_TYPE) node_type;
    Enum32(AST_EXPR_TYPE) expr_type;
    
    AST_Node* next;
    
    union
    {
        AST_Node* first_in_scope;
        
        struct
        {
            AST_Node* left;
            AST_Node* right;
            AST_Node* extra;
        };
    };
    
    union
    {
        Scope_Info scope;
        
        Symbol_ID symbol;
    };
};

struct AST
{
    AST_Node* root;
    Bucket_Array(AST_Node) nodes;
};