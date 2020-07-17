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
    Array(Expression) expressions;
    HIDDEN(Symbol_Table_ID) symbol_table_id;
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
    Expression_LogicalAnd,
    Expression_LogicalOr,
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
    Expression_LogicalNot,
    Expression_Reference,
    Expression_Dereference,
    Expression_SpanType,
    Expression_ArrayType,
    Expression_DynamicArrayType,
    Expression_PolymorphicName,
    
    // Postfix unary
    Expression_Subscript,
    Expression_Span,
    Expression_Call,
    
    // Types
    Expression_ProcPointer,
    Expression_Struct,
    Expression_Enum,
    
    // Special
    Expression_Proc,
    Expression_Run,
    Expression_Ternary,
    Expression_Cast,
    
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
            struct Expression* condition;
            struct Expression* true_expr;
            struct Expression* false_expr;
        } ternary;
        
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
        } span;
        
        struct
        {
            struct Expression* pointer;
            Array(Named_Value) parameters;
        } call;
        
        struct
        {
            struct Expression* type;
            struct Expression* operand;
        } cast;
        
        struct
        {
            struct Expression* type;
            Array(Named_Value) args;
        } struct_literal;
        
        Procedure procedure;
        Structure structure;
        Enumeration enumeration;
        
        String string;
        Number number;
        char character;
        U32 codepoint;
        bool boolean;
    };
} Expression;

typedef struct Attribute_Parameter
{
    union
    {
        Number number;
        String string;
        bool boolean;
    };
    
    bool is_number;
    bool is_string;
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
    Declaration_Struct,
    Declaration_Enum,
};

typedef struct Declaration
{
    Enum32(DECLARATION_KIND) kind;
    Text_Interval text;
    
    bool is_exported;
    
    Array(Attribute) attributes;
    Array(String) names;
    Array(Expression*) types;
    
    union
    {
        struct
        {
            Array(Expression*) values;
            bool is_uninitialized;
        } var;
        
        struct
        {
            Array(Expression*) values;
        } constant;
        
        Procedure procedure;
        Structure structure;
        Enumeration enumeration;
    };
} Declaration;

enum STATEMENT_KIND
{
    Statement_Scope,
    Statement_Declaration,
    Statement_If,
    Statement_For,
    Statement_Defer,
    Statement_Using,
    Statement_Return,
    Statement_Assignment,
    Statement_Expression,
    
    Statement_Import,
    Statement_Load,
    Statement_ConstAssert,
    Statement_ConstIf,
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
            bool is_exported;
        } import;
        
        struct
        {
            File_ID file_id;
            bool is_exported;
        } load;
        
        struct
        {
            Expression* condition;
            String message;
        } const_assert;
        
        struct
        {
            Expression* condition;
            Array(Statement) body;
        } const_if;
        
        Scope scope;
        Declaration declaration;
        
        struct
        {
            Expression* condition;
            Scope true_body;
            Scope false_body;
        } if_statement;
        
        struct
        {
            String label;
            Declaration* declaration;
            Expression* condition;
            struct Statement* statement;
            Scope body;
        } for_loop;
        
        Scope defer_statement;
        Expression* using_expression;
        
        struct
        {
            Array(Named_Value) values;
        } return_statement;
        
        struct
        {
            Enum32(EXPRESSION_KIND) operator;
            Array(String) left;
            Array(Expression*) right;
        } assignment;
        
        Expression* expression;
    };
} Statement;