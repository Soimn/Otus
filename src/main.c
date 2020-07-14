#include "common.h"
#include "string.h"
#include "memory.h"
#include "lexer.h"
#include "ast.h"
#include "workspace.h"

#ifdef _WIN32
#include "windows_layer.h"
#else
#error "unsupported platform"
#endif

IMPORT bool Workspace_Open(Workspace_ID* workspace, Workspace_Options options, String file_path);
IMPORT void Workspace_Close(Workspace_ID workspace);
IMPORT bool Workspace_InspectNextDeclaration(Workspace_ID workspace, Declaration* declaration);
IMPORT bool Workspace_ModifyCurrentDeclaration(Workspace_ID workspace, Declaration declaration);
IMPORT bool Workspace_InjectDeclaration(Workspace_ID workspace, Package_ID package, Declaration declaration);
IMPORT bool Workspace_InjectCode(Workspace_ID workspace, Package_ID package, String code);
IMPORT bool Workspace_GenerateBinary(Workspace_ID workspace, Binary_Options options);

#include <stdio.h>

int
main(int argc, const char** argv)
{
    struct
    {
        Enum32(OPTIMIZATION_LEVEL) opt_level;
        bool generate_bin;
        bool build_dll;
        String file_path;
    } args = {0};
    
    args.opt_level      = OptLevel_Fast;
    args.generate_bin   = true;
    args.build_dll      = false;
    args.file_path.data = 0;
    args.file_path.size = 0;
    
    if (argc == 1)
    {
        fprintf(stderr, "ERROR: Invalid number of arguments");
    }
    
    else if (argc == 2)
    {
        args.file_path.data = (U8*)argv[1];
        for (char* scan = (char*)argv[1]; *scan; ++scan) ++args.file_path.size;
    }
    
    else
    {
        NOT_IMPLEMENTED;
    }
    
    Workspace_Options workspace_options = {
        .opt_level = args.opt_level,
    };
    
    Workspace_ID workspace;
    Workspace_Open(&workspace, workspace_options, args.file_path);
    
    for (Declaration decl; Workspace_InspectNextDeclaration(workspace, &decl); )
    {
        NOT_IMPLEMENTED;
    }
    
    if (args.generate_bin)
    {
        Binary_Options binary_options = {
            .build_dll = args.build_dll
        };
        
        Workspace_GenerateBinary(workspace, binary_options);
    }
    
    Workspace_Close(workspace);
}