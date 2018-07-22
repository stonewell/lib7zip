#ifdef _WIN32
#include "lib7zip.h"

#ifdef S_OK
#undef S_OK
#endif

#include "C/7zVersion.h"
#include "CPP/Common/Common.h"
#include "CPP/7zip/Archive/IArchive.h"
#include "CPP/Windows/PropVariant.h"
#include "CPP/Common/MyCom.h"
#include "CPP/7zip/ICoder.h"
#include "CPP/7zip/IPassword.h"
#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
using namespace NWindows;

#include "HelperFuncs.h"
#include "SelfPath.h"
#include "7ZipFunctions.h"
#include "7ZipDllHandler.h"

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

HMODULE Load7ZLibrary(const wstring & library)
{
    auto absolute_library = DirName(SelfPath()) + kSeparator + NarrowString(library) + ".dll";
    auto wabsolute_library = WidenString(absolute_library);
	HMODULE pModule = LoadLibrary(wabsolute_library.c_str());
    if (!pModule) {
        fprintf(stderr, "Could not find 7-zip library at: %s\n", absolute_library.c_str());
        fflush(stderr);
    }
    return pModule;
}

void Free7ZLibrary(HMODULE pModule)
{
    ::FreeLibrary(pModule);
}
#endif //_WIN_32
