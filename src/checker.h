typedef struct Sema_State
{
    Compiler_State* compiler_state;
} Sema_State;

bool
SemaValidateExpression(Sema_State state, Expression* expression)
{
    NOT_IMPLEMENTED;
}

bool
SemaValidateStatementAndGenerateSymbolInfo(Sema_State state, Statement* statement, Symbol* result)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
SemaValidateDeclarationAndGenerateSymbolInfo(Compiler_State* compiler_state, Declaration declaration)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

bool
TypeCheckDeclaration(Compiler_State* compiler_state, Declaration declaration)
{
    bool encountered_errors = false;
    
    // TODO(soimn): Struct specialization / cast fixup
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}