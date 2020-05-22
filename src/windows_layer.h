void
InitMutex(Mutex* mutex)
{
    HANDLE handle = CreateMutexA(0, 0, 0);
    ASSERT(handle != 0);
    
    mutex->platform_handle = (U64)handle;
}

void
LockMutex(Mutex mutex)
{
    U32 status = WaitForSingleObject((HANDLE)mutex.platform_handle, GREMLIN_MUTEX_TIMEOUT_MS);
    ASSERT(status == WAIT_OBJECT_0);
}

void
UnlockMutex(Mutex mutex)
{
    BOOL succeeded = ReleaseMutex((HANDLE)mutex.platform_handle);
    ASSERT(succeeded);
}

U32
GetPageSize()
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    
    return info.dwPageSize;
}

void*
AllocatePage()
{
    return VirtualAlloc(0, 0, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void
FreePage(void* page_ptr)
{
    VirtualFree(page_ptr, 0, MEM_RELEASE);
}

bool
OpenFileForReading(String path, File_Handle* handle)
{
    bool succeeded = false;
    
    NOT_IMPLEMENTED;
    
    return succeeded;
}

bool
DoesDirectoryExist(String path)
{
    bool result = false;
    
    NOT_IMPLEMENTED;
    
    return result;
}

bool
LoadFileContents(File_Handle* handle, String* contents)
{
    bool succeeded = false;
    
    NOT_IMPLEMENTED;
    
    return succeeded;
}

void
CloseFileHandle(File_Handle* handle)
{
    NOT_IMPLEMENTED;
}
