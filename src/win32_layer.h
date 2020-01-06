#include <windows.h>

// HACK(soimn): Rework this
inline File_ID
LoadFileForCompilation(Module* module, String path)
{
    File_ID file_id = INVALID_ID;
    
    Memory_Arena temp_memory = {};
    
    HANDLE file_handle = INVALID_HANDLE_VALUE;
    
    int required_path_length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCCH)path.data, (int)path.size, 0, 0);
    
    LPWSTR wide_path = 0;
    
    if (required_path_length != 0)
    {
        U32 wide_path_size = (U32)RoundSize(sizeof(WCHAR), alignof(WCHAR)) * (required_path_length + 1);
        wide_path = (LPWSTR)PushSize(&temp_memory, wide_path_size, alignof(WCHAR));
        ZeroSize(wide_path, wide_path_size);
    }
    
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCCH)path.data, (int)path.size, wide_path, required_path_length);
    
    file_handle = CreateFileW(wide_path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    
    
    if (file_handle != INVALID_HANDLE_VALUE)
    {
        U32 higher_file_size = 0;
        U32 lower_file_size  = GetFileSize(file_handle, (LPDWORD)&higher_file_size);
        
        U64 file_size = (U64)lower_file_size | ((U64)higher_file_size << 32);
        
        if (file_size < U32_MAX)
        {
            U8* file_contents = (U8*)PushSize(&module->file_content_arena, (U32)file_size + 1, 1);
            ZeroSize(file_contents, file_size + 1);
            
            U32 bytes_read = 0;
            if (ReadFile(file_handle, file_contents, (U32)file_size, (LPDWORD)&bytes_read, 0) && bytes_read == file_size)
            {
                file_id = (File_ID)ElementCount(&module->files);
                
                File* file = (File*)PushElement(&module->files);
                file->file_contents.data = file_contents;
                file->file_contents.size = bytes_read;
            }
            
            else
            {
                //// ERROR
                Print("Failed to read contents of file. File: %S\n", path);
            }
        }
        
        else
        {
            //// ERROR
            Print("Failed to read contents of file, because it is too large. File: %S\n", path);
        }
        
        CloseHandle(file_handle);
    }
    
    else
    {
        //// ERROR: could not open file
        Print("Failed to open the file: %S\n", path);
    }
    
    ClearArena(&temp_memory);
    
    return file_id;
}