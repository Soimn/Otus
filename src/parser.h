struct Parse_Tree_Node
{
    Enum32(AST_NODE_TYPE) node_type;
    Enum32(AST_EXPRESSION_TYPE) expr_type;
    
    Parse_Tree_Node* next;
    String name;
    
    union
    {
        Parse_Tree_Node* children[3];
        
        /// Binary expressions
        struct
        {
            Parse_Tree_Node* left;
            Parse_Tree_Node* right;
        };
        
        /// Unary expressions
        Parse_Tree_Node* operand;
        
        /// Function call
        struct
        {
            Parse_Tree_Node* call_function;
            Parse_Tree_Node* call_arguments;
        };
        
        /// Function and lambda declaration
        Parse_Tree_Node* function_body;
        
        /// If and while
        struct
        {
            Parse_Tree_Node* condition;
            Parse_Tree_Node* true_body;
        };
        
        /// Else
        Parse_Tree_Node* false_body;
        
        /// Variable and constant declarations, return statement, enum and struct member, function argument
        Parse_Tree_Node* value;
        
        /// Struct and enum declarations
        Parse_Tree_Node* members;
        
        Parse_Tree_Node* defer_statement;
        
        Parse_Tree_Node* scope;
    };
    
    union
    {
        /// Function and lambda declaration
        struct
        {
            String function_type;
            String return_value;
        };
        
        /// Enum declaration, cast expression and variable declaration
        String type_string;
        
        String identifier;
        String string_literal;
        Number number_literal;
    };
};

struct Parser_State
{
    File* file;
    Parse_Tree* tree;
    Lexer* lexer;
};

inline Parse_Tree_Node*
PushNode(Parse_Tree* tree, Enum32(AST_NODE_TYPE) node_type, Enum32(AST_EXPRESSION_TYPE) expr_type)
{
    ASSERT((node_type != ASTNode_Expression && expr_type == 0) || (node_type == ASTNode_Expression && expr_type != 0));
    
    Parse_Tree_Node* node = (Parse_Tree_Node*)PushElement(&tree->nodes);
    node->node_type = node_type;
    node->expr_type = expr_type;
    
    return node;
}

inline bool ParseDeclaration(Parser_State state, Parse_Tree_Node** result);
inline bool ParseStatement(Parser_State state, Parse_Tree_Node** result);
inline bool ParseScope(Parser_State state, Parse_Tree_Node** scope_node);

// TODO(soimn): Rewrite the binary expression parsing functions to use loops instead of recursion and insert 
//              guards against illegal expressions

inline bool
ParseUnaryExpression(Parser_State state, Parse_Tree_Node** result)
{
    bool encountered_errors = false;
    
    for (;;)
    {
        Token token = PeekToken(state.lexer);
        
        if (token.type == Token_Plus || token.type == Token_Minus || token.type == Token_Exclamation || token.type == Token_Tilde)
        {
            NOT_IMPLEMENTED;
        }
        
        else if (token.type == Token_Identifier && StringCompare(token.string, CONST_STRING("cast")))
        {
            NOT_IMPLEMENTED;
        }
        
        else
        {
            NOT_IMPLEMENTED;
            break;
        }
    }
    
    return !encountered_errors;
}

inline bool
ParseBitwiseBinaryExpression(Parser_State state, Parse_Tree_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseUnaryExpression(state, result))
    {
        Token token = PeekToken(state.lexer);
        
        Enum32(AST_EXPRESSION_TYPE) type = ASTExpr_Invalid;
        
        switch(token.type)
        {
            case Token_And:  type = ASTExpr_BitwiseAnd;              break;
            case Token_Pipe: type = ASTExpr_BitwiseOr;               break;
            case Token_Hat:  type = ASTExpr_BitwiseXOR;              break;
            case Token_GTGT: type = ASTExpr_BitwiseLeftShiftEquals;  break;
            case Token_LTLT: type = ASTExpr_BitwiseRightShiftEquals; break;
        }
        
        if (type != ASTExpr_Invalid)
        {
            SkipToken(state.lexer);
            
            Parse_Tree_Node* left = *result;
            
            *result = PushNode(state.tree, ASTNode_Expression, type);
            (*result)->left = left;
            
            if (ParseBitwiseBinaryExpression(state, &(*result)->right))
            {
                // Succeeded
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
ParseMultiplicativeExpression(Parser_State state, Parse_Tree_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseBitwiseBinaryExpression(state, result))
    {
        Token token = PeekToken(state.lexer);
        
        if (token.type == Token_Asterisk || token.type == Token_Slash || token.type == Token_Percentage)
        {
            Enum32(AST_EXPRESSION_TYPE) type;
            
            if (token.type == Token_Asterisk)   type = ASTExpr_Multiplication;
            else if (token.type == Token_Slash) type = ASTExpr_Division;
            else                                type = ASTExpr_Modulus;
            
            SkipToken(state.lexer);
            
            Parse_Tree_Node* left = *result;
            
            *result = PushNode(state.tree, ASTNode_Expression, type);
            (*result)->left = left;
            
            if (ParseMultiplicativeExpression(state, &(*result)->right))
            {
                // Succeeded
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
ParseAdditiveExpression(Parser_State state, Parse_Tree_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseMultiplicativeExpression(state, result))
    {
        Token token = PeekToken(state.lexer);
        
        if (token.type == Token_Plus || token.type == Token_Minus)
        {
            Enum32(AST_EXPRESSION_TYPE) type = (token.type == Token_Plus ? ASTExpr_Addition : ASTExpr_Subtraction);
            SkipToken(state.lexer);
            
            Parse_Tree_Node* left = *result;
            
            *result = PushNode(state.tree, ASTNode_Expression, type);
            (*result)->left = left;
            
            if (ParseAdditiveExpression(state, &(*result)->right))
            {
                // Succeeded
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
ParseRelationalExpression(Parser_State state, Parse_Tree_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseAdditiveExpression(state, result))
    {
        Token token = PeekToken(state.lexer);
        
        Enum32(AST_EXPRESSION_TYPE) type = ASTExpr_Invalid;
        
        switch(token.type)
        {
            case Token_EQEQ:          type = ASTExpr_IsEqual;              break;
            case Token_ExclamationEQ: type = ASTExpr_IsNotEqual;           break;
            case Token_GreaterThan:   type = ASTExpr_IsGreaterThan;        break;
            case Token_LessThan:      type = ASTExpr_IsLessThan;           break;
            case Token_GTEQ:          type = ASTExpr_IsGreaterThanOrEqual; break;
            case Token_LTEQ:          type = ASTExpr_IsLessThanOrEqual;    break;
        }
        
        if (type != ASTExpr_Invalid)
        {
            SkipToken(state.lexer);
            
            Parse_Tree_Node* left = *result;
            
            *result = PushNode(state.tree, ASTNode_Expression, type);
            (*result)->left = left;
            
            if (ParseRelationalExpression(state, &(*result)->right))
            {
                // Succeeded
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
ParseLogicalAndExpression(Parser_State state, Parse_Tree_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseRelationalExpression(state, result))
    {
        Token token = PeekToken(state.lexer);
        
        if (token.type == Token_AndAnd)
        {
            SkipToken(state.lexer);
            
            Parse_Tree_Node* left = *result;
            
            *result = PushNode(state.tree, ASTNode_Expression, ASTExpr_LogicalAnd);
            (*result)->left = left;
            
            if (ParseLogicalAndExpression(state, &(*result)->right))
            {
                // Succeeded
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
ParseLogicalOrExpression(Parser_State state, Parse_Tree_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseLogicalAndExpression(state, result))
    {
        Token token = PeekToken(state.lexer);
        
        if (token.type == Token_PipePipe)
        {
            SkipToken(state.lexer);
            
            Parse_Tree_Node* left = *result;
            
            *result = PushNode(state.tree, ASTNode_Expression, ASTExpr_LogicalOr);
            (*result)->left = left;
            
            if (ParseLogicalOrExpression(state, &(*result)->right))
            {
                // Succeeded
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
ParseExpression(Parser_State state, Parse_Tree_Node** result)
{
    bool encountered_errors = false;
    
    if (ParseLogicalOrExpression(state, result))
    {
        Token token = PeekToken(state.lexer);
        
        Enum32(AST_EXPRESSION_TYPE) type = ASTExpr_Invalid;
        
        switch(token.type)
        {
            case Token_Equals:       type = ASTExpr_Equals;                  break;
            case Token_PlusEQ:       type = ASTExpr_AddEquals;               break;
            case Token_MinusEQ:      type = ASTExpr_SubEquals;               break;
            case Token_AsteriskEQ:   type = ASTExpr_MultEquals;              break;
            case Token_SlashEQ:      type = ASTExpr_DivEquals;               break;
            case Token_PercentageEQ: type = ASTExpr_ModEquals;               break;
            case Token_AndEQ:        type = ASTExpr_BitwiseAndEquals;        break;
            case Token_PipeEQ:       type = ASTExpr_BitwiseOrEquals;         break;
            case Token_HatEQ:        type = ASTExpr_BitwiseXOREquals;        break;
            case Token_LTLTEQ:       type = ASTExpr_BitwiseLeftShiftEquals;  break;
            case Token_GTGTEQ:       type = ASTExpr_BitwiseRightShiftEquals; break;
        }
        
        if (type != ASTExpr_Invalid)
        {
            SkipToken(state.lexer);
            
            Parse_Tree_Node* left = *result;
            
            *result = PushNode(state.tree, ASTNode_Expression, type);
            (*result)->left = left;
            
            if (ParseExpression(state, &(*result)->right))
            {
                // Succeeded
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
ParseScope(Parser_State state, Parse_Tree_Node** scope_node)
{
    bool encountered_errors = false;
    
    *scope_node = PushNode(state.tree, ASTNode_Scope, 0);
    Parse_Tree_Node** current_statement = &(*scope_node)->scope;
    
    bool is_brace_delimited = false;
    
    if (RequiredToken(state.lexer, Token_OpenBrace))
    {
        is_brace_delimited = true;
    }
    
    do
    {
        if (RequiredToken(state.lexer, Token_CloseBrace))
        {
            if (is_brace_delimited) break;
            else
            {
                //// ERROR: Encountered a closing brace before semicolon
                encountered_errors = true;
            }
        }
        
        else
        {
            if (ParseStatement(state, current_statement))
            {
                current_statement = &(*current_statement)->next;
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
    } while (!encountered_errors && is_brace_delimited);
    
    return encountered_errors;
}

inline bool
ParseStatement(Parser_State state, Parse_Tree_Node** result)
{
    bool encountered_errors = false;
    
    Token token = PeekToken(state.lexer);
    
    if (token.type == Token_Semicolon)
    {
        SkipToken(state.lexer);
    }
    
    else if (token.type == Token_Identifier)
    {
        if (StringCompare(token.string, CONST_STRING("if")) || 
            StringCompare(token.string, CONST_STRING("while")))
        {
            Enum32(AST_NODE_TYPE) type;
            
            if (StringCompare(token.string, CONST_STRING("if"))) type = ASTNode_If;
            else                                                 type = ASTNode_While;
            
            SkipToken(state.lexer);
            
            *result = PushNode(state.tree, type, 0);
            
            if (RequiredToken(state.lexer, Token_OpenParen))
            {
                if (ParseExpression(state, &(*result)->condition))
                {
                    if (RequiredToken(state.lexer, Token_CloseParen))
                    {
                        if (ParseScope(state, &(*result)->true_body))
                        {
                            // Succeeded
                        }
                        
                        else
                        {
                            //// ERROR
                            encountered_errors = true;
                        }
                    }
                    
                    else
                    {
                        //// ERROR: Missing close paren after condition in if/while statement
                        encountered_errors = true;
                    }
                }
                
                else
                {
                    //// ERROR: Failed to parse condition
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Missing open paren after if/while keyword in if/while statement
                encountered_errors =  true;
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("else")))
        {
            SkipToken(state.lexer);
            
            *result = PushNode(state.tree, ASTNode_Else, 0);
            
            if (ParseScope(state, &(*result)->false_body))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("defer")))
        {
            *result = PushNode(state.tree, ASTNode_Defer, 0);
            
            SkipToken(state.lexer);
            
            if (ParseScope(state, &(*result)->defer_statement))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("return")))
        {
            *result = PushNode(state.tree, ASTNode_Return, 0);
            
            SkipToken(state.lexer);
            
            if (ParseExpression(state, &(*result)->value))
            {
                if (RequiredToken(state.lexer, Token_Semicolon))
                {
                    // Succeeded
                }
                
                else
                {
                    //// ERROR: Missing semicolon after statement
                }
            }
            
            else
            {
                //// ERROR
                encountered_errors = true;
            }
        }
        
        else if (StringCompare(token.string, CONST_STRING("break")) ||
                 StringCompare(token.string, CONST_STRING("continue")))
        {
            Enum32(AST_NODE_TYPE) type;
            
            if (StringCompare(token.string, CONST_STRING("break"))) type = ASTNode_Break;
            else                                                    type = ASTNode_Continue;
            
            SkipToken(state.lexer);
            
            *result = PushNode(state.tree, type, 0);
            
            if (RequiredToken(state.lexer, Token_Semicolon))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Missing semicolon after statement
            }
        }
        
        else
        {
            token = PeekToken(state.lexer, 2);
            
            if (token.type == Token_Colon || token.type == Token_ColonColon)
            {
                if (ParseDeclaration(state, result))
                {
                    // Succeeded
                }
                
                else
                {
                    //// ERROR
                    encountered_errors = true;
                }
            }
            
            else
            {
                if (ParseExpression(state, result))
                {
                    if (RequiredToken(state.lexer, Token_Semicolon))
                    {
                        // Succeeded
                    }
                    
                    else
                    {
                        //// ERROR: Missing semicolon after statement
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
        if (ParseExpression(state, result))
        {
            if (RequiredToken(state.lexer, Token_Semicolon))
            {
                // Succeeded
            }
            
            else
            {
                //// ERROR: Missing semicolon after statement
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
ParseDeclaration(Parser_State state, Parse_Tree_Node** result)
{
    bool encountered_errors = false;
    
    Token name_token      = GetToken(state.lexer);
    Token separator_token = GetToken(state.lexer);
    
    ASSERT(name_token.type == Token_Identifier &&
           (separator_token.type == Token_Colon || separator_token.type == Token_ColonColon));
    
    if (separator_token.type == Token_Colon)
    {
        *result = PushNode(state.tree, ASTNode_VarDecl, 0);
        
        Token token = PeekToken(state.lexer);
        if (token.type != Token_Equals)
        {
            /// Parse type
            NOT_IMPLEMENTED;
        }
        
        if (token.type == Token_Equals)
        {
            SkipToken(state.lexer);
            
            if (ParseExpression(state, &(*result)->value))
            {
                if (RequiredToken(state.lexer, Token_Semicolon))
                {
                    // Succeeded
                }
                
                else
                {
                    //// ERROR: Missing semicolon after variable declaration
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Failed to parse right hand side of assignment expression
                encountered_errors = true;
            }
        }
    }
    
    else
    {
        Token token = PeekToken(state.lexer);
        
        bool is_handled = false;
        if (token.type == Token_Identifier)
        {
            if (StringCompare(token.string, CONST_STRING("proc")))
            {
                *result = PushNode(state.tree, ASTNode_FuncDecl, 0);
                
                /// Parse function declaration
                NOT_IMPLEMENTED;
            }
            
            else if (StringCompare(token.string, CONST_STRING("struct")))
            {
                *result = PushNode(state.tree, ASTNode_StructDecl, 0);
                
                /// Parse struct declaration
                NOT_IMPLEMENTED;
            }
            
            else if (StringCompare(token.string, CONST_STRING("enum")))
            {
                *result = PushNode(state.tree, ASTNode_EnumDecl, 0);
                
                /// Parse enum declaration
                NOT_IMPLEMENTED;
            }
        }
        
        if (!encountered_errors && !is_handled)
        {
            *result = PushNode(state.tree, ASTNode_ConstDecl, 0);
            
            if (ParseExpression(state, &(*result)->value))
            {
                if (RequiredToken(state.lexer, Token_Semicolon))
                {
                    // Succeeded
                }
                
                else
                {
                    //// ERROR: Missing semicolon after constant declaration
                    encountered_errors = true;
                }
            }
            
            else
            {
                //// ERROR: Failed to parse value of constant declaration
                encountered_errors = true;
            }
        }
    }
    
    return !encountered_errors;
}

inline bool
ParseFile(Module* module, File_ID file_id)
{
    bool encountered_errors = false;
    
    File* file       = (File*)ElementAt(&module->files, file_id);
    Parse_Tree* tree = &file->parse_tree;
    
    tree->nodes = BUCKET_ARRAY(&module->parser_arena, Parse_Tree_Node, 128);
    
    tree->root = (Parse_Tree_Node*)PushElement(&tree->nodes);
    tree->root->node_type = ASTNode_Scope;
    
    Lexer lexer = LexFile(file);
    
    
    
    Parser_State state = {};
    state.file   = file;
    state.tree   = tree;
    state.lexer  = &lexer;
    
    Parse_Tree_Node** current_node = &tree->root->scope;
    
    Token token = PeekToken(state.lexer);
    while (token.type != Token_EndOfStream)
    {
        Token peek = PeekToken(state.lexer, 2);
        
        if (token.type == Token_Unknown || token.type == Token_Error)
        {
            //// ERROR
            encountered_errors = true;
        }
        
        else if (token.type == Token_Hash)
        {
            NOT_IMPLEMENTED;
        }
        
        else if (token.type == Token_Identifier && (peek.type == Token_Colon || peek.type == Token_ColonColon))
        {
            if (ParseDeclaration(state, current_node))
            {
                current_node = &(*current_node)->next;
            }
            
            else
            {
                //// ERROR: Failed to parse declaration
                encountered_errors = true;
            }
        }
        
        else
        {
            //// ERROR: Illegal use of token in global scope
            encountered_errors = true;
        }
        
        token = PeekToken(state.lexer);
    }
    
    return !encountered_errors;
}