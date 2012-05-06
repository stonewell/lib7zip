
#ifdef _WIN32
bool LoadDllFromFolder(C7ZipDllHandler * pMainHandler, 
                       const wstring & folder_name, 
                       C7ZipObjectPtrArray & handlers)
{
    WIN32_FIND_DATA data;

    wstring path = pMainHandler->GetHandlerPath() + 
        L"\\" + 
        folder_name + 
        L"\\*.*";

    HANDLE hFind = ::FindFirstFile(path.c_str(), &data);

    if (hFind == INVALID_HANDLE_VALUE)
        return false;

    do
    {
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
        {
            wstring dirname = data.cFileName;

            if (dirname == L"." || dirname == L"..")
            {
            }
            else
            {
                LoadDllFromFolder(pMainHandler, data.cFileName, handlers);
            }
        }
        else
        {
            void * pHandler = LoadLibrary(data.cFileName);

            C7ZipDllHandler * p7ZipHandler = new C7ZipDllHandler(pMainHandler->GetLibrary(), 
                pHandler);

            if (p7ZipHandler->IsInitialized())
            {
                handlers.push_back(p7ZipHandler);
            }
            else
            {
                delete p7ZipHandler;
            }
        }
    }
    while(::FindNextFile(hFind, &data));

    ::FindClose(hFind);

    return true;
}

wstring GetHandlerPath(void * pHandler)
{
    wchar_t buf[255] = {0};

    if (GetModuleFileName((HMODULE)pHandler, buf, 254) > 0)
    {
        wstring path = buf;

        size_t pos = path.rfind(L"\\");

        if (pos != wstring::npos)
        {
            return path.substr(0, pos);
        }
    }

    return L".";
}
#endif //_WIN_32
