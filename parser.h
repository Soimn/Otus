#pragma once

#include "common.h"
#include "memory.h"
#include "lexer.h"

struct Statement
{
    union
    {
        Bucket_Array statement_list;
        Statement* compound_statement;
        
        struct
        {
            
        } function_declaration;
        
        // type identifier
        struct
        {
            
        } variable_declaration;
        
        // [{type identifier} x N] '=' expression/IsFunction
        struct
        {
            
        } compound_variable_declaration;
    };
};

struct Expression
{
    union
    {
        struct
        {
            
        } function_call;
    };
};