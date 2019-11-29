#pragma once

struct AST_Module
{
    File_ID file;
    
    Bucket_Array container;
    struct AST_Statement* first_statement;
    
    bool is_valid;
};

enum AST_EXPRESSION_TYPE
{
    ASTExp_Invalid,
    
    /// Arithmetic
    ASTExp_Plus,
    ASTExp_PreIncrement,
    ASTExp_PostIncrement,
    ASTExp_Minus,
    ASTExp_PreDecrement,
    ASTExp_PostDecrement,
    ASTExp_Multiply,
    ASTExp_Divide,
    ASTExp_Modulus,
    
    /// Assignment - arithmetic
    ASTExp_PlusEquals,
    ASTExp_MinusEquals,
    ASTExp_MultiplyEquals,
    ASTExp_DivideEquals,
    ASTExp_ModulusEquals,
    
    /// Logical
    ASTExp_LogicalNot,
    ASTExp_LogicalAnd,
    ASTExp_LogicalOr,
    
    /// Comparative
    ASTExp_IsEqual,
    ASTExp_IsNotEqual,
    ASTExp_IsGreaterThan,
    ASTExp_IsGreaterThanOrEqual,
    ASTExp_IsLessThan,
    ASTExp_IsLessThanOrEqual,
    
    /// Bitwise
    ASTExp_Not,
    ASTExp_And,
    ASTExp_Or,
    ASTExp_XOR,
    ASTExp_LeftShift,
    ASTExp_RightShift,
    
    /// Assignment - bitwise
    ASTExp_AndEquals,
    ASTExp_OrEquals,
    ASTExp_XOREquals,
    ASTExp_LeftShiftEquals,
    ASTExp_RightShiftEquals,
    
    /// Literals
    ASTExp_NumericLiteral,
    ASTExp_StringLiteral,
    
    /// Special
    ASTExp_FunctionCall,
    ASTExp_Subscript,
    ASTExp_Member,
    ASTExp_Ternary,
    ASTExp_Dereference,
    ASTExp_Reference,
    ASTExp_TypeCast,
    ASTExp_Identifier,
    
    ASTExp_Ambiguity,
};

struct AST_Expression
{
    Enum32(AST_EXPRESSION_TYPE) type;
    AST_Expression* next;
    
    union
    {
        /// ASTExp_Ambiguity
        AST_Expression* interpretations[3];
        
        struct
        {
            File_Loc location;
            union
            {
                /// Unary and binary operators
                struct
                {
                    // NOTE(soimn): Only left is used when the expression is unary
                    AST_Expression* left;
                    AST_Expression* right;
                };
                
                /// Function call
                struct
                {
                    String function_name;
                    AST_Expression* arguments;
                };
                
                /// Member access
                struct
                {
                    AST_Expression* member_namespace;
                    String member_name;
                };
                
                /// Ternary
                struct
                {
                    AST_Expression* condition;
                    AST_Expression* true_exp;
                    AST_Expression* false_exp;
                };
                
                /// Type cast
                struct
                {
                    // TODO(soimn): How to handle unresolved types?
                    //AST_TypeSpecifier type;
                    AST_Expression* exp_to_cast;
                };
                
                /// Identifier
                String identifier_name;
                
                /// Numeric literal
                Number number;
                
                /// String literal
                String string;
            };
        };
    };
};

enum AST_STATEMENT_TYPE
{
    ASTStmt_Invalid,
    
    ASTStmt_Expression,
    ASTStmt_If,
    ASTStmt_While,
    ASTStmt_Defer,
    ASTStmt_Return,
};

struct AST_Statement
{
    File_Loc location;
    
    Enum32(AST_STATEMENT_TYPE) type;
    
    AST_Statement* next;
    
    union
    {
        /// ASTStmt_Expression and ASTStmt_Return
        AST_Expression* expression;
        
        /// ASTStmt_If and ASTStmt_While
        struct
        {
            AST_Expression* condition;
            
            AST_Statement* true_body;
            
            // NOTE(soimn): While has no false body
            AST_Statement* false_body;
        };
        
        /// ASTStmt_Defer
        AST_Statement* defer_statement;
    };
};

enum AST_DECLARATION_TYPE
{
    ASTDecl_Invalid,
    
    ASTDecl_Variable,
    ASTDecl_Constant,
    ASTDecl_Struct,
    ASTDecl_Function,
};

struct AST_Declaration
{
    File_Loc location;
    
    Enum32(AST_DECLARATION_TYPE) type;
    
    String name;
    
    union
    {
        /// Struct and function
        AST_Statement* body;
        
        /// Var and constant
        AST_Expression* value;
    };
};

enum AST_NODE_TYPE
{
    ASTNode_Invalid,
    
    ASTNode_Expressionm,
    ASTNode_Statement,
    ASTNode_Declaration,
};

struct AST_Node
{
    union
    {
        AST_Expression expression;
        AST_Statement statement;
        AST_Declaration declaration;
    };
    
    Enum8(AST_NODE_TYPE) type;
};

inline void*
PushNode(AST_Module* module, Enum8(AST_NODE_TYPE) type)
{
    AST_Node* node = (AST_Node*)PushElement(&module->container);
    
    node->type = type;
    
    return node;
}