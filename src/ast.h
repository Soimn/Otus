#pragma once

struct AST_Unit
{
    File_ID file;
    
    Bucket_Array container;
    struct AST_Statement* first_statement;
    
    bool is_valid;
};

struct AST_TypeTag
{
    union
    {
        String string;
        
        struct
        {
            struct AST_Statement* arguments;
            String return_type;
        };
    };
    
    bool is_func;
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
    ASTExp_Lambda,
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
                
                /// Lambda
                struct
                {
                    AST_Statement* func_body;
                    AST_Statement* func_arguments;
                    String return_type_string;
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
                    String type_string;
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
    
    ASTStmt_VarDecl,
    ASTStmt_FuncDecl,
    ASTStmt_ConstDecl,
    ASTStmt_StructDecl,
    
    ASTStmt_Compound,
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
        
        struct
        {
            String name;
            
            union
            {
                /// ASTStmt_VarDecl
                struct
                {
                    AST_Expression* value;
                    bool is_func;
                    
                    AST_TypeTag type;
                };
                
                /// ASTStmt_FuncDecl
                struct
                {
                    AST_Statement* func_body;
                    AST_Statement* func_arguments;
                    String return_type_string;
                };
                
                /// ASTStmt_ConstDecl
                AST_Expression* const_value;
                
                /// ASTStmt_StructDecl
                AST_Statement* struct_body;
            };
        };
        
        /// ASTStmt_Compound
        AST_Statement* body;
    };
};

struct AST_Node
{
    union
    {
        AST_Expression expression;
        AST_Statement statement;
    };
    
    bool is_statement;
    bool is_expression;
};

inline AST_Expression*
PushExpression(AST_Unit* unit)
{
    AST_Node* node = (AST_Node*)PushElement(&unit->container);
    
    *node = {};
    node->is_expression = true;
    
    return (AST_Expression*)&node->expression;
}

inline AST_Statement*
PushStatement(AST_Unit* unit)
{
    AST_Node* node = (AST_Node*)PushElement(&unit->container);
    
    *node = {};
    node->is_statement = true;
    
    return (AST_Statement*)&node->statement;
}

inline void
DeleteExpression(AST_Unit* unit, AST_Expression* expression)
{
    // TODO(soimn): Remove node and all children from the unit container
}