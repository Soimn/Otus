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
    
    // Type
    ParseExpr_StaticArray,
    ParseExpr_DynamicArray,
    ParseExpr_Slice,
    ParseExpr_TypeReference,
    ParseExpr_FunctionType,
    ParseExpr_FunctionArgument,
    
    PARSE_EXPRESSION_TYPE_COUNT
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
        
        /// Unary expressions and type tags
        Parse_Node* operand;
        
        /// Function call
        struct
        {
            Parse_Node* call_function;
            Parse_Node* call_arguments;
        };
        
        /// Function type
        struct
        {
            Parse_Node* function_args;
            Parse_Node* function_return_type;
        };
        
        /// Function and lambda declaration
        Parse_Node* function_body;
        
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
        
        /// Scope, defer and return
        Parse_Node* statement;
        
        /// Compound
        Parse_Node* expression;
    };
    
    union
    {
        /// Enum declaration, cast expression, var decl, func and lambda decl, function pointer
        Parse_Node* type;
        
        /// Static array
        Number array_size;
        
        String_ID string;
        Number number;
    };
};

struct Parser_State
{
    Module* module;
    File* file;
    Parse_Tree* tree;
    Lexer* lexer;
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

inline bool ParseType(Parser_State state, Parse_Node**  result);
inline bool ParseExpression(Parser_State state, Parse_Node** result);
inline bool ParseScope(Parser_State state, Parse_Node** result, bool require_braces);
inline bool ParseStatement(Parser_State state, Parse_Node** result);

// NOTE(soimn): The only reason this is a separate function is to allow catching name omission errors in the 
//              parsing stage
inline bool
ParseFunctionType(Parser_State state, Parse_Node** result, bool allow_omitting_names)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    ASSERT(token.type == Token_Identifier && StringCompare(token.string, CONST_STRING("proc")));
    
    *result = PushNode(state, ParseNode_Expression, ParseExpr_FunctionType);
    
    SkipPastToken(state.lexer, token);
    
    if (RequireAndSkipToken(state.lexer, Token_OpenParen))
    {
        Parse_Node** current_arg = &(*result)->function_args;
        while (!encountered_errors)
        {
            token = GetToken(state.lexer);
            
            if (token.type == Token_CloseParen)
            {
                SkipPastToken(state.lexer, token);
                break;
            }
            
            else
            {
                *current_arg = PushNode(state, ParseNode_Expression, ParseExpr_FunctionArgument);
                (*current_arg)->name = INVALID_ID;
                
                Token peek = PeekNextToken(state.lexer, token);
                if (token.type == Token_Identifier && peek.type == Token_Colon)
                {
                    (*current_arg)->name = RegisterString(state, token.string);
                    
                    SkipPastToken(state.lexer, peek);
                    token = GetToken(state.lexer);
                }
                
                else if (!allow_omitting_names)
                {
                    ASSERT(false); //// ERROR: Omitting argument name is not allowed in this context
                    encountered_errors = true;
                }
                
                if (!encountered_errors && token.type != Token_Equals)
                {
                    if (ParseType(state, &(*current_arg)->type))
                    {
                        token = GetToken(state.lexer);
                    }
                    
                    else
                    {
                        ASSERT(false); //// ERROR
                        encountered_errors = true;
                    }
                }
                
                if (!encountered_errors && token.type == Token_Equals)
                {
                    if ((*current_arg)->name != INVALID_ID)
                    {
                        SkipPastToken(state.lexer, token);
                        
                        if (ParseExpression(state, &(*current_arg)->value))
                        {
                            token = GetToken(state.lexer);
                        }
                        
                        else
                        {
                            ASSERT(false); //// ERROR
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        ASSERT(false); //// ERROR: Assignment is not allowed on arguments with omitted names
                        encountered_errors = true;
                    }
                }
                
                if (!encountered_errors && token.type != Token_CloseParen)
                {
                    if (RequireAndSkipToken(state.lexer, Token_Comma))
                    {
                        // Succeeded
                    }
                    
                    else
                    {
                        ASSERT(false); //// ERROR: Missing separating comma in function argument list
                        encountered_errors = true;
                    }
                }
            }
        }
        
        token = GetToken(state.lexer);
        if (token.type == Token_RightArrow)
        {
            SkipPastToken(state.lexer, token);
            
            if (ParseType(state, &(*result)->function_return_type))
            {
                // Succeeded
            }
            
            else
            {
                ASSERT(false); //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        ASSERT(false); //// ERROR: Missing argument list of function type
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseType(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    
    Parse_Node** current = result;
    while (!encountered_errors)
    {
        if (token.type == Token_Identifier)
        {
            if (StringCompare(token.string, CONST_STRING("proc")))
            {
                if (ParseFunctionType(state, current, true))
                {
                    // Succeeded
                }
                
                else
                {
                    ASSERT(false); //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                *current = PushNode(state, ParseNode_Expression, ParseExpr_Identifier);
                (*current)->string = RegisterString(state, token.string);
                SkipPastToken(state.lexer, token);
            }
            
            break;
        }
        
        else if (token.type == Token_OpenBracket)
        {
            SkipPastToken(state.lexer, token);
            token = GetToken(state.lexer);
            
            Enum32(PARSE_EXPRESSION_TYPE) type = ParseExpr_Invalid;
            
            if (token.type == Token_PeriodPeriod)    type = ParseExpr_DynamicArray;
            else if (token.type == Token_Number)     type = ParseExpr_StaticArray;
            else if (token.type == Token_CloseBrace) type = ParseExpr_Slice;
            
            if (type != ParseExpr_Invalid)
            {
                *current = PushNode(state, ParseNode_Expression, type);
                (*result)->array_size = token.number;
                
                if (token.type != ParseExpr_Slice) SkipPastToken(state.lexer, token);
                
                if (RequireAndSkipToken(state.lexer, Token_CloseBracket))
                {
                    // Succeeded
                }
                
                else
                {
                    ASSERT(false); //// ERROR: Missing closing bracket after array type tag in type
                    encountered_errors = true;
                }
            }
            
            else
            {
                ASSERT(false); //// ERROR
                encountered_errors = true;
            }
            
        }
        
        else if (token.type == Token_And)
        {
            SkipPastToken(state.lexer, token);
            *current = PushNode(state, ParseNode_Expression, ParseExpr_TypeReference);
        }
        
        else
        {
            ASSERT(false); //// ERROR: Encountered an invalid token in type
            encountered_errors = true;
        }
        
        if (!encountered_errors) current = &(*current)->operand;
    }
    
    return !encountered_errors;
}

// NOTE(soimn): Precendence levels
// 0. postfix unary operators and primary expressions
// 1. prefix unary operators
// 2. multiplication, division, modulus, binary and
// 3. addition, subtraction, binary or
// 4. comparison
// 5. logical and
// 6. logical or
// 7. assignment

inline bool
ParsePrimaryOrPostfixUnaryExpression(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    bool is_empty = false;
    
    Token token = GetToken(state.lexer);
    
    // Number
    if (token.type == Token_Number)
    {
        if (*result == 0)
        {
            *result = PushNode(state, ParseNode_Expression, ParseExpr_Number);
            (*result)->number = token.number;
            
            SkipPastToken(state.lexer, token);
        }
        
        else
        {
            ASSERT(false); //// ERROR: Missing terminating semicolon after expression before numeric literal
            encountered_errors = true;
        }
    }
    
    // String
    else if (token.type == Token_String)
    {
        if (*result == 0)
        {
            *result = PushNode(state, ParseNode_Expression, ParseExpr_String);
            (*result)->string = RegisterString(state, token.string);
            
            SkipPastToken(state.lexer, token);
        }
        
        else
        {
            ASSERT(false); //// ERROR: Missing terminating semicolon after expression before string literal
            encountered_errors = true;
        }
    }
    
    // Identifier or cast expression
    else if (token.type == Token_Identifier)
    {
        // Cast expression
        if (*result == 0 && StringCompare(token.string, CONST_STRING("cast")))
        {
            *result = PushNode(state, ParseNode_Expression, ParseExpr_TypeCast);
            
            SkipPastToken(state.lexer, token);
            
            if (RequireAndSkipToken(state.lexer, Token_OpenParen))
            {
                if (ParseType(state, &(*result)->type))
                {
                    if (RequireAndSkipToken(state.lexer, Token_CloseParen))
                    {
                        if (ParseExpression(state, &(*result)->operand))
                        {
                            // Succeeded
                        }
                        
                        else
                        {
                            ASSERT(false); //// ERROR
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        ASSERT(false); //// ERROR: Missing closing parenthesis after target type in cast expression
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    ASSERT(false); //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                ASSERT(false); //// ERROR: Missing target type in cast expression
                encountered_errors = true;
            }
        }
        
        else if (*result == 0 && StringCompare(token.string, CONST_STRING("proc")))
        {
            *result = PushNode(state, ParseNode_Expression, ParseExpr_LambdaDecl);
            
            if (ParseFunctionType(state, &(*result)->type, false) &&
                ParseScope(state, &(*result)->function_body, true))
            {
                // Succeeded
            }
            
            else
            {
                ASSERT(false); //// ERROR
                encountered_errors = true;
            }
        }
        
        // Identifier
        else if (*result == 0 || (*result)->expr_type == ParseExpr_Member)
        {
            Parse_Node* new_node = PushNode(state, ParseNode_Expression, ParseExpr_Identifier);
            new_node->string = RegisterString(state, token.string);
            
            SkipPastToken(state.lexer, token);
            
            if (*result != 0 && (*result)->expr_type == ParseExpr_Member)
            {
                (*result)->right = new_node;
            }
            
            else
            {
                *result = new_node;
            }
        }
        
        else
        {
            ASSERT(false); //// ERROR: Missing terminating semicolon after expression before identifier
            encountered_errors = true;
        }
    }
    
    // Compound expression or function call
    else if (token.type == Token_OpenParen)
    {
        SkipPastToken(state.lexer, token);
        
        // Compound expression
        if (*result == 0)
        {
            *result = PushNode(state, ParseNode_Expression, ParseExpr_Compound);
            
            if (ParseExpression(state, &(*result)->expression))
            {
                if (RequireAndSkipToken(state.lexer, Token_CloseParen))
                {
                    // Succeeded
                }
                
                else
                {
                    ASSERT(false); //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                ASSERT(false); //// ERROR
                encountered_errors = true;
            }
        }
        
        // Function call
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
                        SkipPastToken(state.lexer, token);
                        break;
                    }
                    
                    else
                    {
                        if (ParseExpression(state, current_arg))
                        {
                            current_arg = &(*current_arg)->next;
                            
                            token = GetToken(state.lexer);
                            if (token.type != Token_CloseParen &&
                                !RequireAndSkipToken(state.lexer, Token_Comma))
                            {
                                ASSERT(false); //// ERROR
                                encountered_errors = true;
                            }
                        }
                        
                        else
                        {
                            ASSERT(false); //// ERROR
                            encountered_errors = true;
                        }
                    }
                }
            }
            
            else
            {
                ASSERT(false); //// ERROR: Invalid use of function call operator with member operator as left operand
                encountered_errors = true;
            }
        }
    }
    
    // Subscript
    else if (token.type == Token_OpenBracket)
    {
        SkipPastToken(state.lexer, token);
        
        if (*result == 0)
        {
            ASSERT(false); //// ERROR: Missing primary expression before subscript
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
                    if (RequireAndSkipToken(state.lexer, Token_CloseBracket))
                    {
                        // succeeded
                    }
                    
                    else
                    {
                        ASSERT(false); //// ERROR: Missing closing bracket in subscript expression
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    ASSERT(false); //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                ASSERT(false); //// ERROR: Invalid use of subscript operator with left operand as member operator
                encountered_errors = true;
            }
        }
    }
    
    // Member
    else if (token.type == Token_Period)
    {
        SkipPastToken(state.lexer, token);
        
        if (*result == 0)
        {
            ASSERT(false); //// ERROR: Missing primary expression before member operator
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
            }
            
            else
            {
                ASSERT(false); //// ERROR: Invalid use of member operator with ___ as left operand
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        if (*result == 0)
        {
            ASSERT(false); //// ERROR: Missing primary expression in expression
            encountered_errors = true;
        }
        
        else if ((*result)->expr_type == ParseExpr_Member && (*result)->right == 0)
        {
            ASSERT(false); //// ERROR: Missing right operand of member operator
            encountered_errors = true;
        }
        
        else
        {
            // Empty expression
            is_empty = true;
        }
    }
    
    if (!encountered_errors && !is_empty)
    {
        if (ParsePrimaryOrPostfixUnaryExpression(state, result))
        {
            // succeeded
        }
        
        else
        {
            ASSERT(false); //// ERROR
            encountered_errors = true;
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
        SkipPastToken(state.lexer, token);
        
        *result = PushNode(state, ParseNode_Expression, type);
        
        if (ParsePrefixUnaryExpression(state, &(*result)->operand))
        {
            // succeeded
        }
        
        else
        {
            ASSERT(false); //// ERROR
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
            ASSERT(false); //// ERROR
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

inline bool
ParseMultLevelExpression(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    if (ParsePrefixUnaryExpression(state, result))
    {
        Enum32(PARSE_EXPRESSION_TYPE) type = ParseExpr_Invalid;
        
        Token token = GetToken(state.lexer);
        switch (token.type)
        {
            case Token_Asterisk:   type = ParseExpr_Multiplication;    break;
            case Token_Slash:      type = ParseExpr_Division;          break;
            case Token_Percentage: type = ParseExpr_Modulus;           break;
            case Token_And:        type = ParseExpr_BitwiseAnd;        break;
            case Token_LTLT:       type = ParseExpr_BitwiseLeftShift;  break;
            case Token_GTGT:       type = ParseExpr_BitwiseRightShift; break;
        }
        
        if (type != ParseExpr_Invalid)
        {
            SkipPastToken(state.lexer, token);
            
            Parse_Node* left = *result;
            
            *result = PushNode(state, ParseNode_Expression, type);
            (*result)->left = left;
            
            if (ParseMultLevelExpression(state, &(*result)->right))
            {
                // succeeded
            }
            
            else
            {
                ASSERT(false); //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        ASSERT(false); //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseAddLevelExpression(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseMultLevelExpression(state, result))
    {
        Enum32(PARSE_EXPRESSION_TYPE) type = ParseExpr_Invalid;
        
        Token token = GetToken(state.lexer);
        switch (token.type)
        {
            case Token_Plus:  type = ParseExpr_Addition;    break;
            case Token_Minus: type = ParseExpr_Subtraction; break;
            case Token_Pipe:  type = ParseExpr_BitwiseOr;   break;
        }
        
        if (type != ParseExpr_Invalid)
        {
            SkipPastToken(state.lexer, token);
            
            Parse_Node* left = *result;
            
            *result = PushNode(state, ParseNode_Expression, type);
            (*result)->left = left;
            
            if (ParseAddLevelExpression(state, &(*result)->right))
            {
                // succeeded
            }
            
            else
            {
                ASSERT(false); //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        ASSERT(false); //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseComparativeExpression(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseAddLevelExpression(state, result))
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
            SkipPastToken(state.lexer, token);
            
            Parse_Node* left = *result;
            
            *result = PushNode(state, ParseNode_Expression, type);
            (*result)->left = left;
            
            if (ParseComparativeExpression(state, &(*result)->right))
            {
                // succeeded
            }
            
            else
            {
                ASSERT(false); //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        ASSERT(false); //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseLogicalAndExpression(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseComparativeExpression(state, result))
    {
        Token token = GetToken(state.lexer);
        if (token.type == Token_AndAnd)
        {
            SkipPastToken(state.lexer, token);
            
            Parse_Node* left = *result;
            
            *result = PushNode(state, ParseNode_Expression, ParseExpr_LogicalAnd);
            (*result)->left = left;
            
            if (ParseLogicalAndExpression(state, &(*result)->right))
            {
                // succeeded
            }
            
            else
            {
                ASSERT(false); //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        ASSERT(false); //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseLogicalOrExpression(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseLogicalAndExpression(state, result))
    {
        Token token = GetToken(state.lexer);
        if (token.type == Token_PipePipe)
        {
            SkipPastToken(state.lexer, token);
            
            Parse_Node* left = *result;
            
            *result = PushNode(state, ParseNode_Expression, ParseExpr_LogicalOr);
            (*result)->left = left;
            
            if (ParseLogicalOrExpression(state, &(*result)->right))
            {
                // succeeded
            }
            
            else
            {
                ASSERT(false); //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        ASSERT(false); //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseExpression(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseLogicalOrExpression(state, result))
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
            SkipPastToken(state.lexer, token);
            
            Parse_Node* left = *result;
            
            *result = PushNode(state, ParseNode_Expression, type);
            (*result)->left = left;
            
            if (ParseExpression(state, &(*result)->right))
            {
                // succeeded
            }
            
            else
            {
                ASSERT(false); //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        ASSERT(false); //// ERROR
        encountered_errors = true;
    }
    
    return !encountered_errors;
}

inline bool
ParseScope(Parser_State state, Parse_Node** result, bool require_braces)
{
    bool encountered_errors = false;
    
    *result = PushNode(state, ParseNode_Scope);
    
    Token token = GetToken(state.lexer);
    
    bool is_brace_delimited = false;
    if (token.type == Token_OpenBrace)
    {
        is_brace_delimited = true;
        SkipPastToken(state.lexer, token);
        token = GetToken(state.lexer);
    }
    
    if (require_braces && !is_brace_delimited)
    {
        ASSERT(false); //// ERROR: Scope requires braces, but none where encountered
        encountered_errors = true;
    }
    
    else
    {
        Parse_Node** current = &(*result)->statement;
        do
        {
            if (token.type == Token_CloseBrace)
            {
                if (is_brace_delimited)
                {
                    SkipPastToken(state.lexer, token);
                    break;
                }
                
                else
                {
                    ASSERT(false); //// ERROR: Encountered a closing brace before end of statement
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
                    ASSERT(false); //// ERROR
                    encountered_errors = true;
                }
            }
            
            token = GetToken(state.lexer);
        } while (!encountered_errors && is_brace_delimited);
    }
    
    return !encountered_errors;
}

inline bool
ParseStatement(Parser_State state, Parse_Node** result)
{
    bool encountered_errors = false;
    
    Token token = GetToken(state.lexer);
    
    if (token.type == Token_EndOfStream || token.type == Token_Error)
    {
        ASSERT(false); //// ERROR
        encountered_errors = true;
    }
    
    else if (token.type == Token_Semicolon)
    {
        SkipPastToken(state.lexer, token);
    }
    
    // Compiler directive
    // TODO(soimn): Implement a proper system for handling compiler directives
    else if (token.type == Token_Hash)
    {
        SkipPastToken(state.lexer, token);
        token = GetAndSkipToken(state.lexer);
        
        if (token.type == Token_Identifier)
        {
            if (StringCompare(token.string, CONST_STRING("import")))
            {
                token = GetAndSkipToken(state.lexer);
                
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
                        ASSERT(false); //// ERROR
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    ASSERT(false); //// ERROR: Missing path string after "import" compiler directive
                    encountered_errors = true;
                }
                
                if (!encountered_errors &&
                    !RequireAndSkipToken(state.lexer, Token_Semicolon))
                {
                    ASSERT(false); //// ERROR: Missing semicolon after compiler directive
                    encountered_errors = true;
                }
            }
            
            else
            {
                ASSERT(false); //// ERROR: Unknown compiler directive
                encountered_errors = true;
            }
        }
        
        else
        {
            ASSERT(false); //// ERROR: Missing name of compiler directive
            encountered_errors = true;
        }
    }
    
    else if (token.type == Token_Identifier)
    {
        Token peek = PeekNextToken(state.lexer, token);
        
        // Variable declaration
        if (peek.type == Token_Colon)
        {
            *result = PushNode(state, ParseNode_VarDecl);
            (*result)->name = RegisterString(state, token.string);
            
            SkipPastToken(state.lexer, peek);
            
            token = GetToken(state.lexer);
            
            if (token.type != Token_Equals)
            {
                if (ParseType(state, &(*result)->type))
                {
                    token = GetToken(state.lexer);
                }
                
                else
                {
                    ASSERT(false); //// ERROR
                    encountered_errors = true;
                }
            }
            
            if (token.type == Token_Equals)
            {
                SkipPastToken(state.lexer, token);
                
                if (ParseExpression(state, &(*result)->value))
                {
                    // Succeeded
                }
                
                else
                {
                    ASSERT(false); //// ERROR
                    encountered_errors = true;
                }
            }
            
            if (!RequireAndSkipToken(state.lexer, Token_Semicolon))
            {
                ASSERT(false); //// ERROR: Missing semicolon after variable declaration
                encountered_errors = true;
            }
        }
        
        // Constant name declaration
        else if (peek.type == Token_ColonColon)
        {
            String name = token.string;
            SkipPastToken(state.lexer, peek);
            
            token = GetToken(state.lexer);
            
            if (token.type == Token_Identifier)
            {
                if (StringCompare(token.string, CONST_STRING("proc")))
                {
                    *result = PushNode(state, ParseNode_FuncDecl);
                    (*result)->name = RegisterString(state, name);
                    
                    if (ParseFunctionType(state, &(*result)->type, false) &&
                        ParseScope(state, &(*result)->function_body, true))
                    {
                        // succeeded
                    }
                    
                    else
                    {
                        ASSERT(false); //// ERROR
                        encountered_errors = true;
                    }
                }
                
                else if (StringCompare(token.string, CONST_STRING("struct")))
                {
                    *result = PushNode(state, ParseNode_StructDecl);
                    (*result)->name = RegisterString(state, name);
                    
                    SkipPastToken(state.lexer, token);
                    if (ParseScope(state, &(*result)->members, true))
                    {
                        // succeeded
                    }
                    
                    else
                    {
                        ASSERT(false); //// ERROR
                        encountered_errors = true;
                    }
                }
                
                else if (StringCompare(token.string, CONST_STRING("enum")))
                {
                    *result = PushNode(state, ParseNode_EnumDecl);
                    (*result)->name = RegisterString(state, name);
                    
                    SkipPastToken(state.lexer, token);
                    token = GetToken(state.lexer);
                    
                    if (ParseScope(state, &(*result)->members, true))
                    {
                        // succeeded
                    }
                    
                    else
                    {
                        ASSERT(false); //// ERROR
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    *result = PushNode(state, ParseNode_ConstDecl);
                    (*result)->name = RegisterString(state, name);
                    
                    if (ParseExpression(state, &(*result)->value))
                    {
                        if (!RequireAndSkipToken(state.lexer, Token_Semicolon))
                        {
                            ASSERT(false); //// ERROR
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        ASSERT(false); //// ERROR
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
                    if (!RequireAndSkipToken(state.lexer, Token_Semicolon))
                    {
                        ASSERT(false); //// ERROR
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    ASSERT(false); //// ERROR
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
            
            SkipPastToken(state.lexer, token);
            
            if (RequireAndSkipToken(state.lexer, Token_OpenParen))
            {
                if (ParseExpression(state, &(*result)->condition))
                {
                    if (RequireAndSkipToken(state.lexer, Token_CloseParen))
                    {
                        if (ParseScope(state, &(*result)->true_body, false))
                        {
                            // succeeded
                        }
                        
                        else
                        {
                            ASSERT(false); //// ERROR
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        ASSERT(false); //// ERROR: Missing closing paren after if statement condition
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    ASSERT(false); //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                ASSERT(false); //// ERROR: Missing opening paren after if keyword
                encountered_errors = true;
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("else")))
        {
            *result = PushNode(state, ParseNode_Else);
            
            if (ParseScope(state, &(*result)->false_body, false))
            {
                // succeeded
            }
            
            else
            {
                ASSERT(false); //// ERROR
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
            
            if (RequireAndSkipToken(state.lexer, Token_Semicolon))
            {
                // succeeded
            }
            
            else
            {
                ASSERT(false); //// ERROR
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
            
            SkipPastToken(state.lexer, token);
            
            *result = PushNode(state, type);
            
            if (ParseStatement(state, &(*result)->statement))
            {
                // succeeded
            }
            
            else
            {
                ASSERT(false); //// ERROR
                encountered_errors = true;
            }
        }
        
        // Expression
        else
        {
            if (ParseExpression(state, result))
            {
                if (!RequireAndSkipToken(state.lexer, Token_Semicolon))
                {
                    ASSERT(false); //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                ASSERT(false); //// ERROR
                encountered_errors = true;
            }
        }
    }
    
    // Expression
    else
    {
        if (ParseExpression(state, result))
        {
            if (!RequireAndSkipToken(state.lexer, Token_Semicolon))
            {
                ASSERT(false); //// ERROR
                encountered_errors = true;
            }
        }
        
        else
        {
            ASSERT(false); //// ERROR
            encountered_errors = true;
        }
    }
    
    return !encountered_errors;
}

inline void
DEBUGPrintParseNode(Module* module, Parse_Node* node, U32 indentation_level = 0)
{
    for (U32 i = 0; i < indentation_level; ++i) PrintChar('\t');
    
    switch (node->node_type)
    {
        case ParseNode_Invalid: Print("ParseNode_Invalid"); break;
        case ParseNode_Scope: Print("ParseNode_Scope"); break;
        case ParseNode_If: Print("ParseNode_If"); break;
        case ParseNode_Else: Print("ParseNode_Else"); break;
        case ParseNode_While: Print("ParseNode_While"); break;
        case ParseNode_Break: Print("ParseNode_Break"); break;
        case ParseNode_Continue: Print("ParseNode_Continue"); break;
        case ParseNode_Defer: Print("ParseNode_Defer"); break;
        case ParseNode_Return: Print("ParseNode_Return"); break;
        case ParseNode_Using: Print("ParseNode_Using"); break;
        case ParseNode_VarDecl: Print("ParseNode_VarDecl"); break;
        case ParseNode_StructDecl: Print("ParseNode_StructDecl"); break;
        case ParseNode_EnumDecl: Print("ParseNode_EnumDecl"); break;
        case ParseNode_ConstDecl: Print("ParseNode_ConstDecl"); break;
        case ParseNode_FuncDecl: Print("ParseNode_FuncDecl"); break;
        case ParseNode_Expression:
        {
            switch (node->expr_type)
            {
                case ParseExpr_Invalid: Print("ParseExpr_Invalid"); break;
                case ParseExpr_UnaryPlus: Print("ParseExpr_UnaryPlus"); break;
                case ParseExpr_UnaryMinus: Print("ParseExpr_UnaryMinus"); break;
                case ParseExpr_Increment: Print("ParseExpr_Increment"); break;
                case ParseExpr_Decrement: Print("ParseExpr_Decrement"); break;
                case ParseExpr_BitwiseNot: Print("ParseExpr_BitwiseNot"); break;
                case ParseExpr_LogicalNot: Print("ParseExpr_LogicalNot"); break;
                case ParseExpr_Reference: Print("ParseExpr_Reference"); break;
                case ParseExpr_Dereference: Print("ParseExpr_Dereference"); break;
                case ParseExpr_Addition: Print("ParseExpr_Addition"); break;
                case ParseExpr_Subtraction: Print("ParseExpr_Subtraction"); break;
                case ParseExpr_Multiplication: Print("ParseExpr_Multiplication"); break;
                case ParseExpr_Division: Print("ParseExpr_Division"); break;
                case ParseExpr_Modulus: Print("ParseExpr_Modulus"); break;
                case ParseExpr_BitwiseAnd: Print("ParseExpr_BitwiseAnd"); break;
                case ParseExpr_BitwiseOr: Print("ParseExpr_BitwiseOr"); break;
                case ParseExpr_BitwiseXOR: Print("ParseExpr_BitwiseXOR"); break;
                case ParseExpr_BitwiseLeftShift: Print("ParseExpr_BitwiseLeftShift"); break;
                case ParseExpr_BitwiseRightShift: Print("ParseExpr_BitwiseRightShift"); break;
                case ParseExpr_LogicalAnd: Print("ParseExpr_LogicalAnd"); break;
                case ParseExpr_LogicalOr: Print("ParseExpr_LogicalOr"); break;
                case ParseExpr_AddEquals: Print("ParseExpr_AddEquals"); break;
                case ParseExpr_SubEquals: Print("ParseExpr_SubEquals"); break;
                case ParseExpr_MultEquals: Print("ParseExpr_MultEquals"); break;
                case ParseExpr_DivEquals: Print("ParseExpr_DivEquals"); break;
                case ParseExpr_ModEquals: Print("ParseExpr_ModEquals"); break;
                case ParseExpr_BitwiseAndEquals: Print("ParseExpr_BitwiseAndEquals"); break;
                case ParseExpr_BitwiseOrEquals: Print("ParseExpr_BitwiseOrEquals"); break;
                case ParseExpr_BitwiseXOREquals: Print("ParseExpr_BitwiseXOREquals"); break;
                case ParseExpr_BitwiseLeftShiftEquals: Print("ParseExpr_BitwiseLeftShiftEquals"); break;
                case ParseExpr_BitwiseRightShiftEquals: Print("ParseExpr_BitwiseRightShiftEquals"); break;
                case ParseExpr_Equals: Print("ParseExpr_Equals"); break;
                case ParseExpr_IsEqual: Print("ParseExpr_IsEqual"); break;
                case ParseExpr_IsNotEqual: Print("ParseExpr_IsNotEqual"); break;
                case ParseExpr_IsGreaterThan: Print("ParseExpr_IsGreaterThan"); break;
                case ParseExpr_IsLessThan: Print("ParseExpr_IsLessThan"); break;
                case ParseExpr_IsGreaterThanOrEqual: Print("ParseExpr_IsGreaterThanOrEqual"); break;
                case ParseExpr_IsLessThanOrEqual: Print("ParseExpr_IsLessThanOrEqual"); break;
                case ParseExpr_Subscript: Print("ParseExpr_Subscript"); break;
                case ParseExpr_Member: Print("ParseExpr_Member"); break;
                case ParseExpr_Identifier: Print("ParseExpr_Identifier"); break;
                case ParseExpr_Number: Print("ParseExpr_Number"); break;
                case ParseExpr_String: Print("ParseExpr_String"); break;
                case ParseExpr_LambdaDecl: Print("ParseExpr_LambdaDecl"); break;
                case ParseExpr_FunctionCall: Print("ParseExpr_FunctionCall"); break;
                case ParseExpr_TypeCast: Print("ParseExpr_TypeCast"); break;
                case ParseExpr_Compound: Print("ParseExpr_Compound"); break;
                case ParseExpr_StaticArray: Print("ParseExpr_StaticArray"); break;
                case ParseExpr_DynamicArray: Print("ParseExpr_DynamicArray"); break;
                case ParseExpr_Slice: Print("ParseExpr_Slice"); break;
                case ParseExpr_TypeReference: Print("ParseExpr_TypeReference"); break;
                case ParseExpr_FunctionType: Print("ParseExpr_FunctionType"); break;
                case ParseExpr_FunctionArgument: Print("ParseExpr_FunctionArgument"); break;
            }
        } break;
    }
    
    if (node->node_type == ParseNode_Expression)
    {
        if (node->expr_type == ParseExpr_Number)
        {
            if (node->number.is_integer) Print(" : %U", node->number.u64);
            else                         Print(" : FLOAT");
        }
        
        else if (node->expr_type == ParseExpr_Identifier)
        {
            Print(" : %S", *(String*)ElementAt(&module->string_storage, node->string));
        }
        
        else if (node->expr_type == ParseExpr_StaticArray)
        {
            PrintChar('\n');
            for (U32 i = 0; i < indentation_level + 1; ++i) PrintChar('\t');
            Print("- Size: %U", node->array_size.u64);
        }
    }
    
    if (node->node_type == ParseNode_FuncDecl ||
        node->node_type == ParseNode_Expression && node->expr_type == ParseExpr_LambdaDecl)
    {
        PrintChar('\n');
        for (U32 i = 0; i < indentation_level + 1; ++i) PrintChar('\t');
        Print("- Arguments:\n");
        
        Parse_Node* scan = node->type->function_args;
        while (scan)
        {
            PrintParseNode(module, scan, indentation_level + 2);
            
            scan = scan->next;
        }
    }
    
    PrintChar('\n');
    
    if (node->node_type == ParseNode_Scope)
    {
        PrintParseNode(module, node->statement, indentation_level + 1);
    }
    
    else
    {
        for (U32 i = 0; i < ARRAY_COUNT(node->children); ++i)
        {
            if (node->children[i])
            {
                PrintParseNode(module, node->children[i], indentation_level + 1);
            }
        }
        
        if (node->next)
        {
            PrintParseNode(module, node->next, indentation_level);
        }
    }
}

inline bool
ParseFile(Module* module, File* file)
{
    bool encountered_errors = false;
    
    Lexer lexer = LexFile(file);
    
    Parser_State state = {};
    state.module = module;
    state.file   = file;
    state.tree   = &file->parse_tree;
    state.lexer  = &lexer;
    
    state.file->imported_files = BUCKET_ARRAY(&module->main_arena, File_ID, 8);
    
    state.tree->nodes = BUCKET_ARRAY(&module->parser_arena, Parse_Node, 128);
    state.tree->root  = PushNode(state, ParseNode_Scope);
    
    
    Parse_Node** current_node = &state.tree->root->statement;
    
    Token token = GetToken(state.lexer);
    while (!encountered_errors && token.type != Token_EndOfStream)
    {
        if (ParseStatement(state, current_node))
        {
            current_node = &(*current_node)->next;
        }
        
        else
        {
            ASSERT(false); //// ERROR: Failed to parse statement
            encountered_errors = true;
        }
        
        token = GetToken(state.lexer);
    }
    
    DEBUGPrintParseNode(module, state.tree->root);
    
    return !encountered_errors;
}