enum AST_NODE_TYPE
{
    ASTNode_Invalid,
    
    ASTNode_Root,
    ASTNode_Scope,
    
    ASTNode_IfElse,
    ASTNode_ConstIfElse,
    ASTNode_While,
    ASTNode_Break,
    ASTNode_Continue,
    
    ASTNode_Using,
    ASTNode_Defer,
    ASTNode_Return,
    
    ASTNode_FuncDecl,
    ASTNode_StructDecl,
    ASTNode_EnumDecl,
    ASTNode_EnumMemberDecl,
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
    ASTExpr_TypeTransmute,
    ASTExpr_Subscript,
    ASTExpr_Slice,
    ASTExpr_Member,
    ASTExpr_Call,
    ASTExpr_Reference,
    ASTExpr_Dereference,
    
    ASTExpr_Assignment,
    ASTExpr_AddEQ,
    ASTExpr_SubEQ,
    ASTExpr_MulEQ,
    ASTExpr_DivEQ,
    ASTExpr_ModEQ,
    ASTExpr_BitAndEQ,
    ASTExpr_BitOrEQ,
    ASTExpr_LogicalAndEQ,
    ASTExpr_LogicalOrEQ,
    ASTExpr_BitShiftRightEQ,
    ASTExpr_BitShiftLeftEQ,
    
    ASTExpr_Negate,
    ASTExpr_Increment,
    ASTExpr_Decrement,
    
    ASTExpr_Identifier,
    ASTExpr_Number,
    ASTExpr_String,
    
    ASTExpr_Type,
    ASTExpr_ReferenceType,
    ASTExpr_SliceType,
    ASTExpr_StaticArrayType,
    ASTExpr_DynamicArrayType,
    ASTExpr_ProcType,
};

struct Using_Import
{
    Symbol_ID* access_chain;
    Symbol_Table_ID table_id;
    U32 num_decls_before_valid;
};

struct Scope_Info
{
    Symbol_Table_ID table;
    U32 num_decls;
    Bucket_Array(Using_Import) using_imports;
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
        
        struct
        {
            AST_Node* if_condition;
            AST_Node* true_body;
            AST_Node* false_body;
        };
        
        struct
        {
            AST_Node* while_condition;
            AST_Node* while_body;
        };
        
        struct
        {
            AST_Node* call_pointer;
            AST_Node* call_arguments;
        };
        
        struct
        {
            AST_Node* slice_pointer;
            AST_Node* slice_start;
            AST_Node* slice_length;
        };
        
        AST_Node* operand;
        
        AST_Node* return_value;
        AST_Node* using_statement;
        AST_Node* defer_statement;
        
        struct
        {
            AST_Node* var_value;
            AST_Node* var_type;
        };
        
        struct
        {
            AST_Node* function_args;
            AST_Node* function_return;
            AST_Node* function_body;
        };
        
        struct
        {
            AST_Node* enum_members;
            AST_Node* enum_type;
        };
        
        AST_Node* struct_members;
        AST_Node* enum_member_value;
        AST_Node* const_value;
        
        struct
        {
            AST_Node* type_base;
            AST_Node* type_modifiers;
        };
        
        struct
        {
            AST_Node* function_arg_types;
            AST_Node* function_return_types;
        };
        
        AST_Node* static_array_size;
        
        AST_Node* cast_transmute_type;
    };
    
    union
    {
        struct
        {
            Scope_Info root_scope;
            Bucket_Array(File_ID) imported_files;
        };
        
        Scope_Info scope;
        
        Symbol_ID symbol;
        
        String_ID function_name;
        String_ID identifier;
        Number number;
        String string;
    };
};

#define AST_NODE_BUCKET_SIZE 512
struct AST
{
    AST_Node* root;
    Bucket_Array(AST_Node) nodes;
};