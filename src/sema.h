#pragma once

#include "common.h"
#include "ast.h"
#include "module.h"

inline void
SemaRegisterAllSymbolsAndReferencedTypeNamesInScope(Module* module, AST_Node* scope_node);

inline void
SemaRegisterAllSymbolsAndReferencedTypeNamesInStatement(Module* module, AST_Node* statement)
{
    if (statement->node_type == ASTNode_Scope)
    {
        SemaRegisterAllSymbolsAndReferencedTypeNamesInScope(module, statement);
    }
    
    else if (statement->node_type == ASTNode_If || statement->node_type == ASTNode_While)
    {
        SemaRegisterAllSymbolsAndReferencedTypeNamesInScope(module, statement->true_body);
        
        if (statement->false_body)
        {
            SemaRegisterAllSymbolsAndReferencedTypeNamesInScope(module, statement->false_body);
        }
    }
    
    else if (statement->node_type == ASTNode_Defer)
    {
        SemaRegisterAllSymbolsAndReferencedTypeNamesInStatement(module, statement->statement);
    }
    
    else if (statement->node_type == ASTNode_VarDecl || statement->node_type == ASTNode_ConstDecl || statement->node_type == ASTNode_FuncDecl)
    {
        EnsureIdentifierExistsAndRetrieveIdentifierID(module, statement->name.string);
    }
    
    else if (statement->node_type == ASTNode_StructDecl || statement->node_type == ASTNode_EnumDecl)
    {
        EnsureIdentifierExistsAndRetrieveIdentifierID(module, statement->name.string);
        
        AST_Node* scan = statement->members;
        while (scan)
        {
            SemaRegisterAllSymbolsAndReferencedTypeNamesInStatement(module, scan);
            scan = scan->next;
        }
    }
}

inline void
SemaRegisterAllSymbolsAndReferencedTypeNamesInScope(Module* module, AST_Node* scope_node)
{
    AST_Node* scan = scope_node->scope;
    
    while (scan)
    {
        SemaRegisterAllSymbolsAndReferencedTypeNamesInStatement(module, scan);
        scan = scan->next;
    }
}

inline void
SemaRegisterAllSymbolsAndReferencedTypeNames(Module* module, File* file)
{
    
    AST* ast = &file->ast;
    
    SemaRegisterAllSymbolsAndReferencedTypeNamesInScope(module, ast->root);
}