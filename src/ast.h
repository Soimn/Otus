enum SYMBOL_KIND
{
    Symbol_Variable,
    Symbol_Constant,
    Symbol_Proc,
    Symbol_Enum,
    Symbol_Package,
    // NOTE(soimn): Handle labels in the scope chain
};

typedef struct Symbol
{
    Enum32(SYMBOL_KIND) kind;
    Text_Interval text;
    
    union
    {
        struct
        {
            String_ID name;
            Type_ID type;
            Scope_Index using_index;
        };
        
        struct
        {
            Package_ID package;
            String_ID name;
            String_ID alias;
        } import;
    };
    
} Symbol;

typedef struct Symbol_Table
{
    Bucket_Array(Symbol) symbols;
} Symbol_Table;

typedef struct Scope
{
    struct Statement* first_statement;
    struct Statement* last_statement;
    HIDDEN(Symbol_Table_ID) symbol_table_id;
} Scope;

enum EXPRESSION_KIND
{
    Expression_Invalid = 0,
    
    // Ternary
    Expression_Ternary,
    
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
    
    // Prefix unary
    Expression_Neg,
    Expression_BitNot,
    Expression_LogicalNot,
    Expression_Reference,
    Expression_Dereference,
    
    // Postfix unary
    Expression_Subscript,
    Expression_Call,
    
    // Type related
    Expression_PolymorphicAlias, // binary
    Expression_PolymorphicName,  // prefix unary
    Expression_PointerType,      // prefix unary
    Expression_ArrayType,
    Expression_ProcPointer,
    Expression_Struct,
    Expression_Enum,
    
    // Special
    Expression_Member,      // binary
    Expression_Proc,
    Expression_Run,         // unary
    Expression_ArrayExpand, // prefix unary // IMPORTANT
    
    // Literals
    Expression_Identifier,
    Expression_Number,
    Expression_Boolean,
    Expression_String,
    Expression_Character,
    Expression_DataLiteral
};

typedef struct Procedure_Parameter
{
    struct Expression* type;
    struct Expression* value;
    String name;
    bool is_using;
    bool is_const;
    bool is_baked;
} Procedure_Parameter;

// TODO(soimn): Consider adding a #required directive that signals to the compiler that a certain return value 
//              must allways be captured
typedef struct Procedure_Return_Value
{
    String name;
    struct Expression* type;
} Procedure_Return_Value;

typedef struct Procedure
{
    Array(Procedure_Parameter) parameters;
    Array(Procedure_Return_Value) return_values;
    Scope body;
    bool has_body;
    bool is_inline;
    bool is_foreign;
} Procedure;

typedef struct Structure_Member
{
    struct Expression* type;
    String name;
    bool is_using;
    bool is_strictly_ordered;
    bool is_packed;
} Structure_Member;

typedef struct Structure_Parameter
{
    struct Expression* type;
    struct Expression* value;
    String name;
    bool is_using;
} Structure_Parameter;

typedef struct Structure
{
    Array(Structure_Parameter) parameters;
    Array(Structure_Member) members;
    bool is_union;
} Structure;

typedef struct Enumeration_Member
{
    String name;
    struct Expression* value;
} Enumeration_Member;

typedef struct Enumeration
{
    struct Expression* type;
    Array(Enumeration_Member) members;
} Enumeration;

typedef struct Named_Argument
{
    String name;
    struct Expression* value;
} Named_Argument;

typedef struct Expression
{
    Enum32(EXPRESSION_KIND) kind;
    
    HIDDEN(Text_Interval) text;
    
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
        
        // NOTE(soimn): If size is null this is either span type or a dynamic array type, depending on is_dynamic
        struct
        {
            struct Expression* elem_type;
            struct Expression* size;
            bool is_dynamic;
        } array_type;
        
        struct
        {
            struct Expression* array;
            struct Expression* start;
            struct Expression* length;
            bool is_span;
        } subscript;
        
        struct
        {
            struct Expression* pointer;
            Array(Named_Argument) arguments;
        } call;
        
        struct
        {
            struct Expression* type;
            struct Expression* operand;
        } cast;
        
        struct
        {
            struct Expression* type;
            Array(Named_Argument) arguments;
        } data_literal;
        
        Procedure procedure;
        Structure structure;
        Enumeration enumeration;
        
        String string;
        Number number;
        U32 character;
        bool boolean;
    };
} Expression;

typedef struct Attribute_Argument
{
    union
    {
        Number number;
        String string;
        bool boolean;
    };
    
    bool is_number;
    bool is_string;
} Attribute_Argument;

typedef struct Attribute
{
    String name;
    Array(Attribute_Argument) arguments;
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
    
    HIDDEN(Text_Interval) text;
    HIDDEN(bool) is_exported;
    
    String package_name;
    
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
            bool is_distinct;
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
    
    Statement_Scope,
    Statement_Declaration,
    Statement_ConstAssert,
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
    struct Statement* next;
    
    HIDDEN(Text_Interval) text;
    
    union
    {
        // NOTE(soimn): This is not a statement in the language. The only reason this is defined as a statement 
        // here is to allow imports inside of nested constant if statements
        struct
        {
            Package_ID package_id;
            String alias;
            bool is_exported;
            bool is_foreign;
        } import;
        
        // NOTE(soimn): This is not a statement in the language. The only reason this is defined as a statement 
        //              here is to allow loads inside of nested constant if statements
        struct
        {
            File_ID file_id;
            bool is_exported;
        } load;
        
        Scope scope;
        Declaration declaration;
        
        struct
        {
            Expression* condition;
            String message;
        } const_assert_statement;
        
        struct
        {
            Expression* condition;
            Scope true_body;
            Scope false_body;
            bool is_const;
        } if_statement;
        
        struct
        {
            String label;
            Declaration* declaration;
            Expression* condition;
            struct Statement* statement;
            Scope body;
        } for_statement;
        
        struct
        {
            Scope body;
        } defer_statement;
        
        struct
        {
            Expression* path;
        } using_statement;
        
        struct
        {
            Array(Named_Argument) arguments;
        } return_statement;
        
        struct
        {
            Enum32(EXPRESSION_KIND) operator;
            Array(Expression*) left;
            Array(Expression*) right;
        } assignment_statement;
        
        Expression* expression;
    };
} Statement;