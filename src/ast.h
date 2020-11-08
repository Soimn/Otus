enum EXPRESION_KIND
{
    Expression_Invalid = 0,
    
    Expression_Add,
    Expression_Sub,
    Expression_Mul,
    Expression_Div,
    Expression_Mod,
    Expression_BitAnd,
    Expression_BitOr,
    Expression_BitShiftL,
    Expression_BitShiftR,
    Expression_And,
    Expression_Or,
    Expression_CmpEqual,
    Expression_CmpLess,
    Expression_CmpLessEQ,
    Expression_CmpGreater,
    Expression_CmpGreaterEQ,
    Expression_Member,
    
    Expression_Neg,
    Expression_BitNot,
    Expression_Not,
    Expression_Reference,
    Expression_Dereference,
    
    Expression_Subscript,
    Expression_Call,
    
    Expression_PolymorphName,
    Expression_PolymorphAlias,
    Expression_Pointer,
    Expression_Array,
    
    Expression_Proc,
    Expression_Struct,
    Expression_Enum,
    
    Expression_Identifier,
    Expression_Boolean,
    Expression_Number,
    Expression_String,
    Expression_Character,
    Expression_StructLiteral,
};

enum PARAMETER_FLAG
{
    ParameterFlag_None    = 0x000,
    ParameterFlag_Using   = 0x001,
    ParameterFlag_Const   = 0x002,
    ParameterFlag_Baked   = 0x004,
    ParameterFlag_NoAlias = 0x008,
};

typedef struct Parameter
{
    String name;
    struct Expression* type;
    struct Expression* default_value;
    Flag32(PARAMETER_FLAG) flags;
} Parameter;

typedef struct Expression
{
    u8 kind;
    
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
            struct Expression* start;
            struct Expression* end;
            struct Expression* step;
        } subscript;
        
        struct
        {
            struct Expression* pointer;
            Array(Expression*) arguments;
        } call;
        
        struct
        {
            struct Expression* type;
            struct Expression* size;
            bool is_dynamic;
            bool is_slice;
        } array;
        
        struct
        {
            Array(Parameter) parameters;
            Array(Parameter) return_values;
        } procedure;
        
        struct
        {
            
        } structure;
        
        struct
        {
            
        } enumeration;
        
        String string;
        Number number;
        u32 character;
    };
} Expression;

typedef struct Declaration
{
    Bucket_Array(Expression) expressions;
} Declaration;

typedef struct Statement
{
    
} Statement;