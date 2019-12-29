#pragma once

#include "common.h"
#include "ast.h"
#include "module.h"

inline void
SemaRegisterAllSymbolDeclarationsInScope(Module* module, Parser_AST_Node* scope_node);

inline void
SemaRegisterAllSymbolDeclarationsInStatement(Module* module, Parser_AST_Node* statement)
{
    if (statement->node_type == ASTNode_Scope)
    {
        SemaRegisterAllSymbolDeclarationsInScope(module, statement);
    }
    
    else if (statement->node_type == ASTNode_If || statement->node_type == ASTNode_While)
    {
        SemaRegisterAllSymbolDeclarationsInScope(module, statement->true_body);
        
        if (statement->false_body)
        {
            SemaRegisterAllSymbolDeclarationsInScope(module, statement->false_body);
        }
    }
    
    else if (statement->node_type == ASTNode_Defer)
    {
        NOT_IMPLEMENTED;
        //SemaRegisterAllSymbolDeclarationsInStatement(module, statement->statement);
    }
    
    else if (statement->node_type == ASTNode_VarDecl || statement->node_type == ASTNode_ConstDecl || statement->node_type == ASTNode_FuncDecl)
    {
        EnsureIdentifierExistsAndRetrieveIdentifierID(module, statement->name);
        
        // ...
    }
    
    else if (statement->node_type == ASTNode_StructDecl)
    {
        NOT_IMPLEMENTED;
    }
    
    else if (statement->node_type == ASTNode_EnumDecl)
    {
        NOT_IMPLEMENTED;
    }
}

inline void
SemaRegisterAllSymbolDeclarationsInScope(Module* module, Parser_AST_Node* scope_node)
{
    Parser_AST_Node* scan = scope_node->scope;
    
    while (scan)
    {
        SemaRegisterAllSymbolDeclarationsInStatement(module, scan);
        scan = scan->next;
    }
}

inline void
SemaRegisterAllSymbolDeclarations(Module* module, File* file)
{
    
    Parser_AST* ast = &file->parser_ast;
    
    SemaRegisterAllSymbolDeclarationsInScope(module, ast->root);
}