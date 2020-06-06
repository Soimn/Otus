
enum REPORT_SEVERITY
{
    ReportSeverity_DebugInfo, // NOTE(soimn): General debug information, usually irrelevant to the end user
    ReportSeverity_Info,      // NOTE(soimn): General info about the compilation process
    ReportSeverity_Warning,   // NOTE(soimn): Telling the end user about possible errors in the code
    ReportSeverity_Error,     // NOTE(soimn): Telling the end user about present errors in the code
    ReportSeverity_Panic,     // NOTE(soimn): The reason for the compiler panicking during the compilation
    
    REPORT_SEVERITY_COUNT
};

enum COMPILATION_STAGE
{
    CompilationStage_Lexing,
    CompilationStage_Parsing,
    CompilationStage_SemanticAnalysis,
    CompilationStage_CodeGeneration,
    CompilationStage_Linking,
    
    COMPILATION_STAGE_COUNT
};

enum REPORT_STYLE
{
    ReportStyle_CompactTextView = 0x01, // NOTE(soimn): Display only one line of the text interval
    ReportStyle_MessageOnly     = 0x02, // NOTE(soimn): Only display the message, exclude the text view
    ReportStyle_NoFilePath      = 0x04, // NOTE(soimn): Do not prepend the file path to the report message
    ReportStyle_FileNameOnly    = 0x08, // NOTE(soimn): Limit all file paths to display the file name only
    ReportStyle_OneDirOnly      = 0x10, // NOTE(soimn): Limit all file paths to display only one directory
    ReportStyle_Colored         = 0x20, // NOTE(soimn): Display the text view in full color
};

enum WORK_KIND
{
    WorkKind_Invalid,
    
    WorkKind_Parsing,
    WorkKind_Sema,
};

typedef struct Worker_Context
{
    struct Workspace* workspace;
    Memory_Arena arena;
} Worker_Context;

typedef struct Work
{
    Enum32(WORK_KIND) kind;
    
    union
    {
        // Parsing and lexing
        File_ID file_id;
    };
} Work;

typedef struct Source_File
{
    String path;
    String contents;
} Source_File;

// NOTE(soimn): The function ParserImportSourceFile requires that mounting point paths are terminated with a path 
//              separator
typedef struct Mounting_Point
{
    String name;
    String path;
} Mounting_Point;

typedef struct Build_Options
{
    Enum32(REPORT_SEVERITY) min_report_severity;
    Flag32(REPORT_STYLE) report_style[REPORT_SEVERITY_COUNT][COMPILATION_STAGE_COUNT];
    Flag32(EXPRESSION_FLAG) default_expression_flags;
    bool export_by_default; // NOTE(soimn): Export all global names by default
    bool inline_by_default; // NOTE(soimn): Inline all procedures by default
    bool strict_by_default; // NOTE(soimn): Structs and enums are strict by default
    bool packed_by_default; // NOTE(soimn): Structs are packed by default
    bool no_discard_by_default; // NOTE(soimn): All procedures are no_discard by default
    
    String default_mounting_point_path;
    Array(Mounting_Point) mounting_points;
} Build_Options;

#define WORKSPACE_MAX_WORKER_COUNT 8
typedef struct Workspace
{
    Memory_Allocator page_allocator;
    Memory_Arena report_arena;
    Memory_Arena string_arena;
    Memory_Arena file_arena;
    
    Bucket_Array(String) identifier_storage;
    Bucket_Array(String) string_storage;
    Mutex string_mutex;
    
    Bucket_Array(String) imported_directories;
    Bucket_Array(Source_File) source_files;
    Bucket_Array(Scope) file_scopes;
    Mutex file_mutex;
    
    Worker_Context workers[WORKSPACE_MAX_WORKER_COUNT];
    Array(Work) work_array;
    Mutex work_mutex;
    
    // NOTE(soimn): The idea behind using a report buffer instead of printing the reports straight away is to 
    //              allow the compiler to supress unnecesary warnings, errors and other reports which are not 
    //              filtered out by the severity level.
    Array(Report_Data) report_buffer;
    Mutex report_mutex;
    
    Build_Options build_options;
    
    Scope_ID scope_count;
    
    bool encountered_errors;
    
} Workspace;

/*
void
WorkspaceMainThread(Workspace* workspace)
{
    // Init
    // Setup workers
    // load main file
    // parse main file
    // kick workers (parse remaining files)
    // do work while waiting on workers
    // Setup everything for sema
    // kick workers
    // ...
}

void
WorkerThreadLoop(Worker_Context* context)
{
    // Check work queue
    // fetch work
    // predicate on work type (e.g. load, parse, sema, type check, code gen., etc.)
    // Do work
}
*/

typedef struct Report_Data
{
    Enum32(REPORT_SEVERITY) severity;
    Enum32(COMPILATION_STAGE) stage;
    String message;
} Report_Data;

#define REPORT_TEXT_VIEW_SIZE_Y 5
#define REPORT_TEXT_VIEW_SIZE_X 60

void
Report(Workspace* workspace, Enum32(REPORT_SEVERITY) severity, Enum32(COMPILATION_STAGE) compilation_stage,
       String format_string, Arg_List arg_list, Text_Interval* text)
{
    Enum32(REPORT_STYLE) style = workspace->build_options.report_style[severity][compilation_stage];
    
    if (severity >= workspace->build_options.min_report_severity)
    {
        String message = {0};
        UMM required_buffer_size = 0;
        
        String file_path = {0};
        if (!(style & ReportStyle_NoFilePath || style & ReportStyle_MessageOnly))
        {
            Source_File* source_file = BucketArray_ElementAt(&workspace->source_files, text->position.file_id);
            
            if (style & ReportStyle_FileNameOnly)
            {
                for (UMM i = 0; i < source_file->path.size; ++i)
                {
                    if (source_file->path.data[i] == '/')
                    {
                        file_path.data = &source_file->path.data[i + 1];
                        file_path.size = 0;
                    }
                    
                    else ++file_path.size;
                }
            }
            
            else if (style & ReportStyle_OneDirOnly)
            {
                file_path.data = source_file->path.data;
                U8* next_jump  = source_file->path.data;
                
                for (UMM i = 0; i < source_file->path.size; ++i)
                {
                    if (source_file->path.data[i] == '/')
                    {
                        file_path.size = next_jump - file_path.data;
                        file_path.data = next_jump;
                        
                        next_jump = &source_file->path.data[i + 1];
                    }
                    
                    else ++file_path.size;
                }
            }
            
            else
            {
                file_path = source_file->path;
            }
            
            required_buffer_size = FormatCString(message, "%S(%U:%U): %S", file_path,
                                                 text->position.line, text->position.column,
                                                 format_string);
        }
        
        required_buffer_size += FormatString(message, format_string, arg_list);
        
        LockMutex(workspace->report_mutex);
        
        Report_Data* report = Array_Append(&workspace->report_buffer);
        report->severity = severity;
        report->stage    = compilation_stage;
        
        report->message.data = MemoryArena_AllocSize(&workspace->report_arena, required_buffer_size + 1,
                                                     ALIGNOF(U8));
        report->message.size = required_buffer_size + 1;
        
        UnlockMutex(workspace->report_mutex);
        
        message.data = report->message.data;
        message.size = report->message.size - 1;
        
        UMM used_bytes = FormatCString(message, "%S(%U:%U): %S", file_path, text->position.line,
                                       text->position.column,format_string);
        
        message.data += used_bytes;
        message.size -= used_bytes;
        
        used_bytes = FormatString(message, format_string, arg_list);
        
        message.data += used_bytes;
        message.size -= used_bytes;
        
        ASSERT(message.size == 0);
        
        // NOTE(soimn): C string compatibility
        report->message.data[report->message.size - 1] = 0;
    }
}

void
AddSourceFile(Workspace* workspace, String path)
{
    LockMutex(workspace->file_mutex);
    
    bool is_imported = false;
    for (Bucket_Array_Iterator it = BucketArray_Iterate(&workspace->source_files);
         it.current;
         BucketArrayIterator_Advance(&it))
    {
        Source_File* it_file = it.current;
        
        if (StringCompare(path, it_file->path))
        {
            is_imported = true;
        }
    }
    
    File_ID imported_file_id = 0;
    if (!is_imported)
    {
        imported_file_id = BucketArray_ElementCount(&workspace->source_files);
        Source_File* imported_file = BucketArray_Append(&workspace->source_files);
        
        imported_file->path.size = path.size;
        imported_file->path.data = MemoryArena_AllocSize(&workspace->file_arena,
                                                         path.size + 1,
                                                         ALIGNOF(U8));
        
        Copy(path.data, imported_file->path.data, path.size);
        imported_file->path.data[imported_file->path.size] = 0;
    }
    
    UnlockMutex(workspace->file_mutex);
    
    if (!is_imported)
    {
        LockMutex(workspace->work_mutex);
        
        Work* work = Array_Append(&workspace->work_array);
        work->kind = WorkKind_Parsing;
        work->file_id = imported_file_id;
        
        UnlockMutex(workspace->work_mutex);
    }
}

void
AddAllSourceFilesInDirectory(Workspace* workspace, String dir_path)
{
    LockMutex(workspace->file_mutex);
    
    bool has_been_imported = false;
    for (Bucket_Array_Iterator it = BucketArray_Iterate(&workspace->imported_directories);
         it.current;
         BucketArrayIterator_Advance(&it))
    {
        if (DoesStringStartWith(dir_path, *(String*)it.current))
        {
            has_been_imported = true;
            break;
        }
    }
    
    UnlockMutex(workspace->file_mutex);
    
    if (!has_been_imported)
    {
        NOT_IMPLEMENTED;
    }
}