#include "common.h"

#ifdef _WIN32
#include "windows_layer.h"
#else
#error "unsupported platform"
#endif

IMPORT Workspace OpenWorkspace(Workspace_Options workspace_options);
IMPORT bool ParseAllFiles(Workspace* workspace, String main_file);
IMPORT Declaration* FetchNextDeclaration(Workspace* workspace);
IMPORT bool TypeCheckDeclaration(Workspace* workspace, Declaration* declaration);
IMPORT void SubmitNewDeclaration(Workspace* workspace, Declaration* declaration);
IMPORT void ResubmitCurrentDeclaration(Workspace* workspace);
IMPORT bool GenerateBytecodeForDeclaration(Workspace* workspace, Declaration* declaration);

bool
InspectNextDeclaration(Workspace* workspace, Declaration** declaration)
{
    *declaration = FetchNextDeclaration(&workspace);
    return (*declaration != 0 && TypeCheckDeclaration(&workspace, declaration));
}

bool
CommitDeclaration(Workspace* workspace, Declaration* declaration)
{
    GenerateBytecodeForDeclaration(workspace, declaration);
}

int
main(int argc, const char** argv)
{
    Path_Prefix path_prefixes[] = {
        {CONST_STRING("core"), CONST_STRING(".\\..\\core\\")},
    };
    
    Workspace_Options workspace_options = {
        .path_prefixes     = path_prefixes,
        .path_prefix_count = ARRAY_COUNT(path_prefixes),
        
        .default_opt_level = OptLevel_Debug,
    };
    
    Workspace workspace = OpenWorkspace(workspace_options);
    
    String main_file = ;
    ParseAllFiles(&workspace, main_file);
    
    for (Declaration* declaration = 0; InspectNextDeclaration(&workspace, &declaration);)
    {
        // Modify
        SubmitNewDeclaration;
        ResubmitCurrentDeclaration;
        CommitCurrentDeclaration;
    }
    
    CloseWorkspace(&workspace);
}