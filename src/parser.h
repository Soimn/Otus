enum PARSE_NODE_TYPE
{
    ParseNode_Invalid,
    
    ParseNode_Expression,
    
    ParseNode_Scope,
    
    ParseNode_If,
    ParseNode_Else,
    ParseNode_While,
    ParseNode_Break,
    ParseNode_Continue,
    ParseNode_Defer,
    ParseNode_Return,
    ParseNode_Using,
    
    ParseNode_VarDecl,
    ParseNode_StructDecl,
    ParseNode_EnumDecl,
    ParseNode_ConstDecl,
    ParseNode_FuncDecl,
    
    PARSE_NODE_TYPE_COUNT
};

enum PARSE_EXPRESSION_TYPE
{
    ParseExpr_Invalid,
    
    // Unary
    ParseExpr_UnaryPlus,
    ParseExpr_UnaryMinus,
    ParseExpr_Increment,
    ParseExpr_Decrement,
    
    ParseExpr_BitwiseNot,
    
    ParseExpr_LogicalNot,
    
    ParseExpr_Reference,
    ParseExpr_Dereference,
    
    // Binary
    ParseExpr_Addition,
    ParseExpr_Subtraction,
    ParseExpr_Multiplication,
    ParseExpr_Division,
    ParseExpr_Modulus,
    
    ParseExpr_BitwiseAnd,
    ParseExpr_BitwiseOr,
    ParseExpr_BitwiseXOR,
    ParseExpr_BitwiseLeftShift,
    ParseExpr_BitwiseRightShift,
    
    ParseExpr_LogicalAnd,
    ParseExpr_LogicalOr,
    
    ParseExpr_AddEquals,
    ParseExpr_SubEquals,
    ParseExpr_MultEquals,
    ParseExpr_DivEquals,
    ParseExpr_ModEquals,
    ParseExpr_BitwiseAndEquals,
    ParseExpr_BitwiseOrEquals,
    ParseExpr_BitwiseXOREquals,
    ParseExpr_BitwiseLeftShiftEquals,
    ParseExpr_BitwiseRightShiftEquals,
    
    ParseExpr_Equals,
    
    ParseExpr_IsEqual,
    ParseExpr_IsNotEqual,
    ParseExpr_IsGreaterThan,
    ParseExpr_IsLessThan,
    ParseExpr_IsGreaterThanOrEqual,
    ParseExpr_IsLessThanOrEqual,
    
    ParseExpr_Subscript,
    ParseExpr_Member,
    
    // Special
    ParseExpr_Identifier,
    ParseExpr_Number,
    ParseExpr_String,
    ParseExpr_LambdaDecl,
    ParseExpr_FunctionCall,
    ParseExpr_TypeCast,
    ParseExpr_Compound,
    
    PARSE_EXPRESSION_TYPE_COUNT
};

struct Parse_Tree_Type
{
    // IMPORTANT TODO(soimn): Fill this
};

struct Parse_Node
{
    Enum32(PARSE_NODE_TYPE) node_type;
    Enum32(PARSE_EXPRESSION_TYPE) expr_type;
    
    Parse_Node* next;
    String_ID name;
    
    union
    {
        Parse_Node* children[3];
        
        /// Binary expressions
        struct
        {
            Parse_Node* left;
            Parse_Node* right;
        };
        
        /// Unary expressions and compound
        Parse_Node* operand;
        
        /// Function call
        struct
        {
            Parse_Node* call_function;
            Parse_Node* call_arguments;
        };
        
        /// Function and lambda declaration
        struct
        {
            Parse_Node* function_args;
            Parse_Node* function_body;
        };
        
        /// If and while
        struct
        {
            Parse_Node* condition;
            Parse_Node* true_body;
        };
        
        /// Else
        Parse_Node* false_body;
        
        /// Variable and constant declarations
        Parse_Node* value;
        
        /// Struct and enum declarations
        Parse_Node* members;
        
        /// Defer and return
        Parse_Node* statement;
        
        Parse_Node* scope;
    };
    
    union
    {
        /// Enum declaration, cast expression, function return type and variable declaration
        Parse_Tree_Type type;
        
        String_ID string;
        Number number;
    };
};

struct Parser_State
{
    Module* module;
    File* file;
    Parse_Tree* tree;
    Lexer lexer;
};

inline Parse_Node*
PushNode(Parser_State state, Enum32(PARSE_NODE_TYPE) node_type, Enum32(PARSE_EXPRESSION_TYPE) expr_type = 0)
{
    ASSERT((node_type != ParseNode_Expression && expr_type == 0) || (node_type == ParseNode_Expression && expr_type != 0));
    
    Parse_Node* node = (Parse_Node*)PushElement(&state.tree->nodes);
    node->node_type = node_type;
    node->expr_type = expr_type;
    
    return node;
}

inline String_ID
RegisterString(Parser_State state, String string)
{
    String_ID string_id = INVALID_ID;
    
    for (Bucket_Array_Iterator it = Iterate(&state.module->string_storage);
         it.current;
         Advance(&it))
    {
        if (StringCompare(string, *(String*)it.current))
        {
            string_id = (String_ID)it.current_index;
        }
    }
    
    if (string_id == INVALID_ID)
    {
        string_id = (String_ID)ElementCount(&state.module->string_storage);
        
        String* entry = (String*)PushElement(&state.module->string_storage);
        entry->size = string.size;
        entry->data = (U8*)PushSize(&state.module->string_arena, (U32)string.size, alignof(U8));
        
        Copy(string.data, entry->data, entry->size);
    }
    
    return string_id;
}

File_ID LoadAndParseFile(Module* module, String path);

inline bool ParseType(Parser_State state, Parse_Node** result);
inline bool ParseFunction(Parser_State state, Parse_Node* function_node);
inline bool ParseExpression(Parser_State state, Parse_Node** result);
inline bool ParseScope(Parser_State state, Parse_Node** result);
inline bool ParseStatement(Parser_State state, Parse_Node** result);

inline bool
ParseType(Parser_State state, Parse_Tree_Type* type)
{
    bool encountered_errors = false;
    
    NOT_IMPLEMENTED;
    
    return !encountered_errors;
}

inline bool
ParseFunction(Parser_State state, Parse_Node* function_node)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    
    ASSERT(token.type == Token_Identifier && StringCompare(token.string, CONST_STRING("proc")));
    SkipPastToken(&state.lexer, token);
    
    if (RequireAndSkipToken(&state.lexer, Token_OpenParen))
    {
        Parse_Node** current_arg = &function_node->function_args;
        while (!encountered_errors)
        {
            token = GetToken(state.lexer);
            
            if (token.type == Token_CloseParen)
            {
                SkipPastToken(&state.lexer, token);
                break;
            }
            
            else
            {
                if (ParseStatement(state, current_arg))
                {
                    current_arg = &(*current_arg)->next;
                    
                    token = GetToken(state.lexer);
                    if (token.type != Token_CloseParen &&
                        !RequireAndSkipToken(&state.lexer, Token_Comma))
                    {
                        //// ERROR: Missing terminating parenthesis after function argument list
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
        }
    }
    
    else
    {
        //// ERROR: Missing argument list in function declaration
        encountered_errors = true;
    }
    
    if (!encountered_errors)
    {
        token = GetToken(state.lexer);
        if (token.type == Token_OpenBrace)
        {
            if (ParseStatement(state, &function_node->function_body))
            {
                // succeeded
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Missing function body
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

// NOTE(soimn): Precendence levels
// 0. postfix unary operators and primary expressions
// 1. prefix unary operators
// 2. multiplication, division, modulus
// *. bitwise binary operators
// 3. addition, subtraction
// 4. comparison
// 5. logical
// 6. assignment

// TODO(soimn): See if there is a way to clean up this ugly mess
inline bool
ParsePrimaryOrPostfixUnaryExpression(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    
    /// Number
    if (token.type == Token_Number)
    {
        if (*result == 0)
        {
            *result = PushNode(state, ParseNode_Expression, ParseExpr_Number);
            (*result)->number = token.number;
            
            SkipPastToken(&state.lexer, token);
            
            if (ParsePrimaryOrPostfixUnaryExpression(state, result))
            {
                // succeeded
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Missing terminating semicolon in expression before numeric literal
            encountered_errors = true;
        }
    }
    
    /// String
    else if (token.type == Token_String)
    {
        if (*result == 0)
        {
            *result = PushNode(state, ParseNode_Expression, ParseExpr_String);
            (*result)->string = RegisterString(state, token.string);
            
            SkipPastToken(&state.lexer, token);
            
            if (ParsePrimaryOrPostfixUnaryExpression(state, result))
            {
                // succeeded
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Missing terminating semicolon in expression before string literal
            encountered_errors = true;
        }
    }
    
    /// Identifier
    else if (token.type == Token_Identifier)
    {
        if (*result == 0 || (*result)->expr_type == ParseExpr_Member)
        {
            Parse_Node* new_node = PushNode(state, ParseNode_Expression, ParseExpr_Identifier);
            new_node->string = RegisterString(state, token.string);
            
            SkipPastToken(&state.lexer, token);
            
            if (*result != 0 && (*result)->expr_type == ParseExpr_Member)
            {
                (*result)->right = new_node;
            }
            
            else
            {
                *result = new_node;
            }
            
            if (ParsePrimaryOrPostfixUnaryExpression(state, result))
            {
                // succeeded
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Missing terminating semicolon after expression before identifier
            encountered_errors = true;
        }
    }
    
    /// Increment / decrement
    else if (token.type == Token_PlusPlus || token.type == Token_MinusMinus)
    {
        if (*result == 0)
        {
            //// ERROR: Missing primary expression before increment / decrement operator
            encountered_errors = true;
        }
        
        else
        {
            if ((*result)->expr_type != ParseExpr_Member)
            {
                SkipPastToken(&state.lexer, token);
                
                Parse_Node* operand = *result;
                
                Enum32(PARSE_EXPRESSION_TYPE) type = (token.type == Token_PlusPlus
                                                      ? ParseExpr_Increment
                                                      : ParseExpr_Decrement);
                *result = PushNode(state, ParseNode_Expression, type);
                (*result)->operand = operand;
                
                if (ParsePrimaryOrPostfixUnaryExpression(state, result))
                {
                    // succeeded
                }
                
                else
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Invalid use of increment / decrement operator with member operator as operand
                encountered_errors = true;
            }
        }
    }
    
    /// Lambda declaration
    else if (token.type == Token_Identifier && StringCompare(token.string, CONST_STRING("proc")))
    {
        if (*result == 0)
        {
            *result = PushNode(state, ParseNode_Expression, ParseExpr_LambdaDecl);
            
            if (ParseFunction(state, *result))
            {
                if (ParsePrimaryOrPostfixUnaryExpression(state, result))
                {
                    // succeeded
                }
                
                else
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Missing terminating semicolon after expression before lambda declaration
            encountered_errors = true;
        }
    }
    
    /// Compound expression or function call
    else if (token.type == Token_OpenParen)
    {
        SkipPastToken(&state.lexer, token);
        
        /// Compound expression
        if (*result == 0)
        {
            *result = PushNode(state, ParseNode_Expression, ParseExpr_Compound);
            
            if (ParseExpression(state, &(*result)->operand))
            {
                if (RequireAndSkipToken(&state.lexer, Token_CloseParen))
                {
                    // succeeded
                }
                
                else
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        /// Function call
        else
        {
            if ((*result)->expr_type != ParseExpr_Member)
            {
                Parse_Node* call_function = *result;
                
                *result = PushNode(state, ParseNode_Expression, ParseExpr_FunctionCall);
                (*result)->call_function = call_function;
                
                Parse_Node** current_arg = &(*result)->call_arguments;
                while (!encountered_errors)
                {
                    token = GetToken(state.lexer);
                    
                    if (token.type == Token_CloseParen)
                    {
                        SkipPastToken(&state.lexer, token);
                        break;
                    }
                    
                    else
                    {
                        if (ParseExpression(state, current_arg))
                        {
                            current_arg = &(*current_arg)->next;
                            
                            token = GetToken(state.lexer);
                            if (token.type != Token_CloseParen &&
                                !RequireAndSkipToken(&state.lexer, Token_Comma))
                            {
                                //// ERROR
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            //// ERROR
                            encountered_errors = true;
                        }
                    }
                }
            }
            
            else
            {
                //// ERROR: Invalid use of function call operator with member operator as left operand
                encountered_errors = true;
            }
        }
        
        if (!encountered_errors)
        {
            if (ParsePrimaryOrPostfixUnaryExpression(state, result))
            {
                // succeeded
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    /// Subscript
    else if (token.type == Token_OpenBracket)
    {
        SkipPastToken(&state.lexer, token);
        
        if (*result == 0)
        {
            //// ERROR: Missing primary expression before subscript
            encountered_errors = true;
        }
        
        else
        {
            if ((*result)->expr_type != ParseExpr_Member)
            {
                Parse_Node* left = *result;
                
                *result = PushNode(state, ParseNode_Expression, ParseExpr_Subscript);
                (*result)->left = left;
                
                if (ParseExpression(state, &(*result)->right))
                {
                    if (RequireAndSkipToken(&state.lexer, Token_CloseBracket))
                    {
                        if (ParsePrimaryOrPostfixUnaryExpression(state, result))
                        {
                            // succeeded
                        }
                        
                        else
                        {
                            //// ERROR
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Missing closing bracket in subscript expression
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Invalid use of subscript operator with left operand as member operator
                encountered_errors = true;
            }
        }
    }
    
    /// Member
    else if (token.type == Token_Period)
    {
        SkipPastToken(&state.lexer, token);
        
        if (*result == 0)
        {
            //// ERROR: Missing primary expression before member operator
            encountered_errors = true;
        }
        
        else
        {
            if ((*result)->expr_type == ParseExpr_Identifier ||
                (*result)->expr_type == ParseExpr_Subscript  ||
                (*result)->expr_type == ParseExpr_FunctionCall)
            {
                Parse_Node* left = *result;
                
                *result = PushNode(state, ParseNode_Expression, ParseExpr_Member);
                (*result)->left = left;
                
                if (ParsePrimaryOrPostfixUnaryExpression(state, result))
                {
                    // succeeded
                }
                
                else
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Invalid use of member operator with ___ as left operand
                encountered_errors = true;
            }
        }
    }
    
    /// Empty
    else
    {
        if (*result == 0)
        {
            //// ERROR: Missing primary expression in expression
            encountered_errors = true;
        }
        
        else if ((*result)->expr_type == ParseExpr_Member && (*result)->right == 0)
        {
            //// ERROR: Missing right operand of member operator
            encountered_errors = true;
        }
        
        else
        {
            // succeeded
        }
    }
    
    return !encountered_errors;
}

inline bool
ParsePrefixUnaryExpression(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    
    Enum32(PARSE_EXPRESSION_TYPE) type = ParseExpr_Invalid;
    
    switch (token.type)
    {
        case Token_Plus:        type = ParseExpr_UnaryPlus;   break;
        case Token_Minus:       type = ParseExpr_UnaryMinus;  break;
        case Token_Tilde:       type = ParseExpr_BitwiseNot;  break;
        case Token_Exclamation: type = ParseExpr_LogicalNot;  break;
        case Token_Asterisk:    type = ParseExpr_Dereference; break;
        case Token_And:         type = ParseExpr_Reference;   break;
    }
    
    if (type != ParseExpr_Invalid)
    {
        SkipPastToken(&state.lexer, token);
        
        *result = PushNode(state, ParseNode_Expression, type);
        
        if (ParsePrefixUnaryExpression(state, &(*result)->operand))
        {
            // succeeded
        }
        
        else
        {
            //// ERROR
            encountered_errors = true;
        }
    }
    
    else
    {
        if (ParsePrimaryOrPostfixUnaryExpression(state, result))
        {
            // succeeded
        }
        
        else
        {
            //// ERROR
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

inline bool
ParseMultLevelArithmeticExpression(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    if (ParsePrefixUnaryExpression(state, result))
    {
        Enum32(PARSE_EXPRESSION_TYPE) type = ParseExpr_Invalid;
        
        Token token = GetToken(state.lexer);
        switch (token.type)
        {
            case Token_Asterisk:   type = ParseExpr_Multiplication; break;
            case Token_Slash:      type = ParseExpr_Division; break;
            case Token_Percentage: type = ParseExpr_Modulus; break;
            case Token_And:        type = ParseExpr_BitwiseAnd; break;
            case Token_Pipe:       type = ParseExpr_BitwiseOr; break;
        }
        
        if (type != ParseExpr_Invalid)
        {
            SkipPastToken(&state.lexer, token);
            
            Parse_Node* left = *result;
            
            *result = PushNode(state, ParseNode_Expression, type);
            (*result)->left = left;
            
            if (ParseMultLevelArithmeticExpression(state, &(*result)->right))
            {
                // succeeded
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseAddLevelArithmeticExpression(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseMultLevelArithmeticExpression(state, result))
    {
        Enum32(PARSE_EXPRESSION_TYPE) type = ParseExpr_Invalid;
        
        Token token = GetToken(state.lexer);
        switch (token.type)
        {
            case Token_Plus:  type = ParseExpr_Addition;    break;
            case Token_Minus: type = ParseExpr_Subtraction; break;
        }
        
        if (type != ParseExpr_Invalid)
        {
            SkipPastToken(&state.lexer, token);
            
            Parse_Node* left = *result;
            
            *result = PushNode(state, ParseNode_Expression, type);
            (*result)->left = left;
            
            if (ParseAddLevelArithmeticExpression(state, &(*result)->right))
            {
                // succeeded
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseComparativeExpression(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseAddLevelArithmeticExpression(state, result))
    {
        Enum32(PARSE_EXPRESSION_TYPE) type = ParseExpr_Invalid;
        
        Token token = GetToken(state.lexer);
        switch (token.type)
        {
            case Token_EQEQ:          type = ParseExpr_IsEqual;              break;
            case Token_ExclamationEQ: type = ParseExpr_IsNotEqual;           break;
            case Token_GreaterThan:   type = ParseExpr_IsGreaterThan;        break;
            case Token_LessThan:      type = ParseExpr_IsLessThan;           break;
            case Token_GTEQ:          type = ParseExpr_IsGreaterThanOrEqual; break;
            case Token_LTEQ:          type = ParseExpr_IsLessThanOrEqual;    break;
        }
        
        if (type != ParseExpr_Invalid)
        {
            SkipPastToken(&state.lexer, token);
            
            Parse_Node* left = *result;
            
            *result = PushNode(state, ParseNode_Expression, type);
            (*result)->left = left;
            
            if (ParseComparativeExpression(state, &(*result)->right))
            {
                // succeeded
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseLogicalExpression(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseComparativeExpression(state, result))
    {
        Enum32(PARSE_EXPRESSION_TYPE) type = ParseExpr_Invalid;
        
        Token token = GetToken(state.lexer);
        switch (token.type)
        {
            case Token_AndAnd:   type = ParseExpr_LogicalAnd; break;
            case Token_PipePipe: type = ParseExpr_LogicalOr;  break;
        }
        
        if (type != ParseExpr_Invalid)
        {
            SkipPastToken(&state.lexer, token);
            
            Parse_Node* left = *result;
            
            *result = PushNode(state, ParseNode_Expression, type);
            (*result)->left = left;
            
            if (ParseLogicalExpression(state, &(*result)->right))
            {
                // succeeded
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseExpression(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseLogicalExpression(state, result))
    {
        Enum32(PARSE_EXPRESSION_TYPE) type = ParseExpr_Invalid;
        
        Token token = GetToken(state.lexer);
        switch (token.type)
        {
            case Token_Equals:       type = ParseExpr_Equals;                  break;
            case Token_PlusEQ:       type = ParseExpr_AddEquals;               break;
            case Token_MinusEQ:      type = ParseExpr_SubEquals;               break;
            case Token_AsteriskEQ:   type = ParseExpr_MultEquals;              break;
            case Token_SlashEQ:      type = ParseExpr_DivEquals;               break;
            case Token_PercentageEQ: type = ParseExpr_ModEquals;               break;
            case Token_AndEQ:        type = ParseExpr_BitwiseAndEquals;        break;
            case Token_PipeEQ:       type = ParseExpr_BitwiseOrEquals;         break;
            case Token_GTGTEQ:       type = ParseExpr_BitwiseLeftShiftEquals;  break;
            case Token_LTLTEQ:       type = ParseExpr_BitwiseRightShiftEquals; break;
        }
        
        if (type != ParseExpr_Invalid)
        {
            SkipPastToken(&state.lexer, token);
            
            Parse_Node* left = *result;
            
            *result = PushNode(state, ParseNode_Expression, type);
            (*result)->left = left;
            
            if (ParseExpression(state, &(*result)->right))
            {
                // succeeded
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseScope(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    *result = PushNode(state, ParseNode_Scope);
    
    Token token = GetToken(state.lexer);
    
    bool is_brace_delimited = false;
    if (token.type == Token_OpenBrace)
    {
        is_brace_delimited = true;
        SkipPastToken(&state.lexer, token);
        token = GetToken(state.lexer);
    }
    
    Parse_Node** current = result;
    do
    {
        if (token.type == Token_CloseBrace)
        {
            if (is_brace_delimited)
            {
                SkipPastToken(&state.lexer, token);
                break;
            }
            
            else
            {
                //// ERROR: Encountered a closing brace before end of statement
                encountered_errors = true;
            }
        }
        
        else
        {
            if (ParseStatement(state, current))
            {
                // NOTE(soimn): This is to prevent an access viloation on empty statements
                if (current)
                {
                    current = &(*current)->next;
                }
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        token = GetToken(state.lexer);
    } while (!encountered_errors && is_brace_delimited);
    
    return !encountered_errors;
}

inline bool
ParseStatement(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    
    if (token.type == Token_EndOfStream || token.type == Token_Error)
    {
        //// ERROR
        encountered_errors = true;
    }
    
    else if (token.type == Token_Semicolon)
    {
        SkipPastToken(&state.lexer, token);
    }
    
    // Compiler directive
    // TODO(soimn): Implement a proper system for handling compiler directives
    else if (token.type == Token_Hash)
    {
        SkipPastToken(&state.lexer, token);
        token = GetAndSkipToken(&state.lexer);
        
        if (token.type == Token_Identifier)
        {
            if (StringCompare(token.string, CONST_STRING("import")))
            {
                token = GetAndSkipToken(&state.lexer);
                
                if (token.type == Token_String)
                {
                    File_ID file_id = LoadAndParseFile(state.module, token.string);
                    
                    if (file_id != INVALID_ID)
                    {
                        File_ID* entry = (File_ID*)PushElement(&state.file->imported_files);
                        *entry = file_id;
                    }
                    
                    else
                    {
                        //// ERROR
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Missing path string after "import" compiler directive
                    encountered_errors = true;
                }
                
                if (!encountered_errors &&
                    !RequireAndSkipToken(&state.lexer, Token_Semicolon))
                {
                    //// ERROR: Missing semicolon after compiler directive
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Unknown compiler directive
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Missing name of compiler directive
            encountered_errors = true;
        }
    }
    
    // Declaration or expression
    else if (token.type == Token_Identifier)
    {
        Token peek = PeekNextToken(state.lexer, token);
        
        // Variable declaration
        if (peek.type == Token_Colon)
        {
            *result = PushNode(state, ParseNode_VarDecl);
            (*result)->name = RegisterString(state, token.string);
            
            SkipPastToken(&state.lexer, peek);
            
            token = GetToken(state.lexer);
            
            if (token.type != Token_Equals)
            {
                if (ParseType(state, &(*result)->type))
                {
                    token = GetToken(state.lexer);
                }
                
                else
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
            
            if (token.type == Token_Equals)
            {
                SkipPastToken(&state.lexer, token);
                
                if (ParseExpression(state, &(*result)->value))
                {
                    // Succeeded
                }
                
                else
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
            
            if (!RequireAndSkipToken(&state.lexer, Token_Semicolon))
            {
                //// ERROR: Missing semicolon after variable declaration
                encountered_errors = true;
            }
        }
        
        // Constant name declaration
        else if (peek.type == Token_ColonColon)
        {
            String name = token.string;
            SkipPastToken(&state.lexer, peek);
            
            token = GetToken(state.lexer);
            
            if (token.type == Token_Identifier)
            {
                if (StringCompare(token.string, CONST_STRING("proc")))
                {
                    *result = PushNode(state, ParseNode_FuncDecl);
                    (*result)->name = RegisterString(state, name);
                    
                    if (ParseFunction(state, *result))
                    {
                        // succeeded
                    }
                    
                    else
                    {
                        //// ERROR
                        encountered_errors = true;
                    }
                }
                
                else if (StringCompare(token.string, CONST_STRING("struct")))
                {
                    *result = PushNode(state, ParseNode_StructDecl);
                    (*result)->name = RegisterString(state, name);
                    
                    SkipPastToken(&state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (token.type == Token_OpenBrace)
                    {
                        if (ParseScope(state, &(*result)->members))
                        {
                            // succeeded
                        }
                        
                        else
                        {
                            //// ERROR
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Missing body of struct declaration
                        encountered_errors = true;
                    }
                }
                
                else if (StringCompare(token.string, CONST_STRING("enum")))
                {
                    *result = PushNode(state, ParseNode_EnumDecl);
                    (*result)->name = RegisterString(state, name);
                    
                    SkipPastToken(&state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (token.type == Token_OpenBrace)
                    {
                        if (ParseScope(state, &(*result)->members))
                        {
                            // succeeded
                        }
                        
                        else
                        {
                            //// ERROR
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Missing body of enum declaration
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    *result = PushNode(state, ParseNode_ConstDecl);
                    (*result)->name = RegisterString(state, name);
                    
                    if (ParseExpression(state, &(*result)->value))
                    {
                        if (!RequireAndSkipToken(&state.lexer, Token_Semicolon))
                        {
                            //// ERROR
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR
                        encountered_errors = true;
                    }
                }
            }
            
            else
            {
                *result = PushNode(state, ParseNode_ConstDecl);
                (*result)->name = RegisterString(state, name);
                
                if (ParseExpression(state, &(*result)->value))
                {
                    if (!RequireAndSkipToken(&state.lexer, Token_Semicolon))
                    {
                        //// ERROR
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("if")) ||
                 StringCompare(token.string, CONST_STRING("while")))
        {
            Enum32(PARSE_NODE_TYPE) type = (StringCompare(token.string, CONST_STRING("if")) 
                                            ? ParseNode_If 
                                            : ParseNode_While);
            *result = PushNode(state, type);
            
            SkipPastToken(&state.lexer, token);
            
            if (RequireAndSkipToken(&state.lexer, Token_OpenParen))
            {
                if (ParseExpression(state, &(*result)->condition))
                {
                    if (RequireAndSkipToken(&state.lexer, Token_CloseParen))
                    {
                        if (ParseScope(state, &(*result)->true_body))
                        {
                            // succeeded
                        }
                        
                        else
                        {
                            //// ERROR
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Missing closing paren after if statement condition
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Missing opening paren after if keyword
                encountered_errors = true;
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("else")))
        {
            *result = PushNode(state, ParseNode_Else);
            
            if (ParseScope(state, &(*result)->false_body))
            {
                // succeeded
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("continue")) ||
                 StringCompare(token.string, CONST_STRING("break")))
        {
            Enum32(PARSE_NODE_TYPE) type = (StringCompare(token.string, CONST_STRING("continue"))
                                            ? ParseNode_Continue
                                            : ParseNode_Break);
            *result = PushNode(state, type);
            
            if (RequireAndSkipToken(&state.lexer, Token_Semicolon))
            {
                // succeeded
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("defer"))  ||
                 StringCompare(token.string, CONST_STRING("return")) ||
                 StringCompare(token.string, CONST_STRING("using")))
        {
            Enum32(PARSE_NODE_TYPE) type = ParseExpr_Invalid;
            
            if (StringCompare(token.string, CONST_STRING("defer")))       type = ParseNode_Defer;
            else if (StringCompare(token.string, CONST_STRING("return"))) type = ParseNode_Return;
            else                                                          type = ParseNode_Using;
            
            *result = PushNode(state, type);
            
            if (ParseStatement(state, &(*result)->statement))
            {
                // succeeded
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        // Expression
        else
        {
            if (ParseExpression(state, result))
            {
                if (!RequireAndSkipToken(&state.lexer, Token_Semicolon))
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    // Expression
    else
    {
        if (ParseExpression(state, result))
        {
            if (!RequireAndSkipToken(&state.lexer, Token_Semicolon))
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

inline bool
ParseFile(Module* module, File* file)
{
    bool encountered_errors = false;
    
    Parser_State state = {};
    state.module = module;
    state.file   = file;
    state.tree   = &file->parse_tree;
    state.lexer  = LexFile(file);
    
    state.file->imported_files = BUCKET_ARRAY(&module->main_arena, File_ID, 8);
    
    state.tree->nodes = BUCKET_ARRAY(&module->parser_arena, Parse_Node, 128);
    state.tree->root  = PushNode(state, ParseNode_Scope);
    
    
    Parse_Node** current_node = &state.tree->root->scope;
    
    Token token = GetToken(state.lexer);
    while (token.type != Token_EndOfStream)
    {
        if (ParseStatement(state, current_node))
        {
            current_node = &(*current_node)->next;
        }
        
        else
        {
            //// ERROR: Failed to parse statement
            encountered_errors = true;
        }
        
        token = GetToken(state.lexer);
    }
    
    return !encountered_errors;
}