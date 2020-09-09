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