enum AST_NODE_TYPE
{
    ASTNode_Invalid,
    
    ASTNode_Expression,
    
    ASTNode_Scope,
    
    ASTNode_If,
    ASTNode_Else,
    ASTNode_While,
    ASTNode_Break,
    ASTNode_Continue,
    ASTNode_Defer,
    ASTNode_Return,
    
    ASTNode_VarDecl,
    ASTNode_StructDecl,
    ASTNode_EnumDecl,
    ASTNode_ConstDecl,
    ASTNode_FuncDecl,
    
    AST_NODE_TYPE_COUNT
};

enum AST_EXPRESSION_TYPE
{
    ASTExpr_Invalid,
    
    // Unary
    ASTExpr_UnaryPlus,
    ASTExpr_UnaryMinus,
    ASTExpr_Increment,
    ASTExpr_Decrement,
    
    ASTExpr_BitwiseNot,
    
    ASTExpr_LogicalNot,
    
    ASTExpr_Reference,
    ASTExpr_Dereference,
    
    // Binary
    ASTExpr_Addition,
    ASTExpr_Subtraction,
    ASTExpr_Multiplication,
    ASTExpr_Division,
    ASTExpr_Modulus,
    
    ASTExpr_BitwiseAnd,
    ASTExpr_BitwiseOr,
    ASTExpr_BitwiseXOR,
    ASTExpr_BitwiseLeftShift,
    ASTExpr_BitwiseRightShift,
    
    ASTExpr_LogicalAnd,
    ASTExpr_LogicalOr,
    
    ASTExpr_AddEquals,
    ASTExpr_SubEquals,
    ASTExpr_MultEquals,
    ASTExpr_DivEquals,
    ASTExpr_ModEquals,
    ASTExpr_BitwiseAndEquals,
    ASTExpr_BitwiseOrEquals,
    ASTExpr_BitwiseXOREquals,
    ASTExpr_BitwiseLeftShiftEquals,
    ASTExpr_BitwiseRightShiftEquals,
    
    ASTExpr_Equals,
    
    ASTExpr_IsEqual,
    ASTExpr_IsNotEqual,
    ASTExpr_IsGreaterThan,
    ASTExpr_IsLessThan,
    ASTExpr_IsGreaterThanOrEqual,
    ASTExpr_IsLessThanOrEqual,
    
    ASTExpr_Subscript,
    ASTExpr_Member,
    
    // Special
    ASTExpr_Identifier,
    ASTExpr_Number,
    ASTExpr_String,
    ASTExpr_LambdaDecl,
    ASTExpr_FunctionCall,
    ASTExpr_TypeCast,
    
    AST_EXPRESSION_TYPE_COUNT
};

inline Scope_ID
GetNewScopeID(Module* module)
{
    return (Scope_ID)module->total_scope_count++;
}