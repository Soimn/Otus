enum SYMBOL_KIND
{
    Symbol_Variable,
    Symbol_Constant,
    Symbol_Proc,
    Symbol_Enum,
    Symbol_Package,
    // TODO(soimn): Symbol_Label,
};

typedef struct Symbol
{
    Enum32(SYMBOL_KIND) kind;
    String_ID name;
    Type_ID type;
    Text_Interval text;
    
    union
    {
        Scope_Index using_index;
        Package_ID package;
    } import_info;
    
    bool is_using;
} Symbol;

typedef struct Symbol_Table
{
    Bucket_Array(Symbol) symbols;
} Symbol_Table;

typedef struct Scope
{
    Array(Statement) statements;
    Symbol_Table_ID symbol_table; // NOTE(soimn): This is hidden from the metaprogram
} Scope;

enum EXPRESSION_KIND
{
    Expression_Invalid = 0,
    
    // Binary
    Expression_Add,
    Expression_Sub,
    Expression_Mul,
    Expression_Div,
    Expression_Mod,
    Expression_And,
    Expression_or,
    Expression_BitAnd,
    Expression_BitOr,
    Expression_BitXor,
    Expression_BitLShift,
    Expression_BitRShift,
    Expression_CmpEqual,
    Expression_CmpNotEqual,
    Expression_CmpLess,
    Expression_CmpGreater,
    Expression_CmpLessOrEqual,
    Expression_CmpGreaterOrEqual,
    Expression_Member,
    Expression_PolymorphicAlias,
    
    // Prefix unary
    Expression_Neg,
    Expression_BitNot,
    Expression_Reference,
    Expression_Dereference,
    Expression_SliceType,
    Expression_ArrayType,
    Expression_DynamicArrayType,
    Expression_PolymorphicName,
    
    // Postfix unary
    Expression_Subscript,
    Expression_Slice,
    Expression_Call,
    
    // Types
    Expression_ProcPointer,
    Expression_Struct,
    Expression_Union,
    Expression_Enum,
    
    // Special
    Expression_Proc,
    Expression_Run,
    
    // Literals
    Expression_Identifier,
    Expression_Number,
    Expression_Boolean,
    Expression_String,
    Expression_Character,
    Expression_Codepoint,
    Expression_StructLiteral,
};

typedef struct Named_Value
{
    String name;
    struct Expression* value;
} Named_Value;

typedef struct Procedure_Parameter
{
    String name;
    struct Expression* type;
    struct Expression* value;
    bool is_const;
    bool is_baked;
} Procedure_Parameter;

typedef struct Procedure
{
    Array(Procedure_Parameter) parameters;
    Array(Named_Value) return_values;
    Scope body;
} Procedure;

typedef struct Structure
{
    Array(Named_Value) parameters;
    Array(Named_Value) members;
    bool is_union;
} Structure;

typedef struct Enumeration
{
    struct Expression* member_type;
    Array(Named_Value) members;
} Enumeration;

typedef struct Expression
{
    Enum32(EXPRESSION_KIND) kind;
    Text_Interval text;
    bool is_type;
    
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
            struct Expression* size;
            struct Expression* elem_type;
        } array_type;
        
        struct
        {
            struct Expression* array;
            struct Expression* index;
        } subscript;
        
        struct
        {
            struct Expression* array;
            struct Expression* start;
            struct Expression* length;
        } slice;
        
        struct
        {
            struct Expression* pointer;
            Array(Named_Value) parameters;
        } call_or_type;
        
        Procedure procedure;
        Structure structure;
        Enumeration enumeration;
        
        String string;
        Number number;
        char character;
        U32 codepoint;
    };
} Expression;

typedef struct Attribute_Parameter
{
    union
    {
        Number number;
        String string;
    };
    
    bool is_number;
} Attribute_Parameter;

typedef struct Attribute
{
    String name;
    Array(Attribute_Parameter) parameters;
} Attribute;

enum DECLARATION_KIND
{
    Declaration_Var,
    Declaration_Const,
    Declaration_Proc,
    Declaration_Enum,
    Declaration_Struct,
    Declaration_Union
};

typedef struct Declaration
{
    Enum32(DECLARATION_KIND) kind;
    Text_Interval text;
    
    Array(Attribute) attributes;
    Array(String) names;
    Array(Expression) types;
    
    union
    {
        struct
        {
            Array(Expression) values;
            bool is_unitialized;
        } var;
        
        struct
        {
            Array(Expression) values;
        } constant;
        
        Procedure procedure;
        Structure structure;
        Enumeration enumeration;
    };
} Declaration;

enum STATEMENT_KIND
{
    Statement_Import,
    Statement_Load,
    Statement_ConstAssert,
    Statement_ConstIf,
    Statement_Scope,
    Statement_Declaration,
    Statement_If,
    Statement_For,
    Statement_Defer,
    Statement_Using,
    Statement_Return,
    Statement_Assignment,
    Statement_Expression,
};

typedef struct Statement
{
    Enum32(STATEMENT_KIND) kind;
    Text_Interval text;
    
    union
    {
        struct
        {
            Package_ID package_id;
            String alias;
        } import;
        
        struct
        {
            File_ID file_id;
        } load;
        
        struct
        {
            Expression* expression;
            Expression* message;
            Array(Expression) parameters;
        } const_assert;
        
        struct
        {
            Expression* condition;
            Array(Statement) true_body;
            Array(Statement) false_body;
        } const_if;
        
        Scope scope;
        Declaration declaration;
        
        struct
        {
            Expression* condition;
            Scope true_body;
            Scope false_body;
        } if_statment;
        
        struct
        {
            String label;
            Declaration* declaration;
            Expression* condition;
            struct Statement* statement;
            Scope body;
        } for_loop;
        
        struct Statement* defer_statement;
        
        struct
        {
            Array(Named_Value) values;
        } return_statement;
        
        struct
        {
            Enum32(EXPRESSION_KIND) operator;
            Expression* left;
            Expression* right;
        } assignment;
        
        Expression* expression;
    };
} Statement;