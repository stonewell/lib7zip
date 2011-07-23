#include "lib7zip.h"

#ifdef S_OK
#undef S_OK
#endif

#include "CPP/7zip/Archive/IArchive.h"
#include "CPP/Windows/PropVariant.h"
#include "CPP/Common/MyCom.h"
#include "CPP/7zip/ICoder.h"
#include "CPP/7zip/IPassword.h"

#include "stdlib.h"

#ifndef _WIN32_WCE
#include <errno.h>
#else
#include <winerror.h>
#endif

#ifdef _WIN32_WCE
#define myT(x) L##x
#else
#define myT(x) x
#endif

#ifndef _WIN32
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "dlfcn.h"
#include "unistd.h"
#include "dirent.h"
#define GetProcAddress dlsym
#define HMODULE void *
#endif

/*-------------- const defines ---------------------------*/
const UInt64 kMaxCheckStartPosition = 1 << 22;

const wchar_t kAnyStringWildcard = '*';

#include "HelperFuncs.h"

extern bool Create7ZipArchive(C7ZipLibrary * pLibrary, IInArchive * pInArchive, C7ZipArchive ** pArchive);

/*-------------- internal classes ------------------------*/
typedef union _union_7zip_functions
{
    unsigned char data[sizeof(void *) * 7];

    struct
    {
        GetMethodPropertyFunc GetMethodProperty;
        GetNumberOfMethodsFunc GetNumberOfMethods;
        GetNumberOfFormatsFunc GetNumberOfFormats;
        GetHandlerPropertyFunc GetHandlerProperty;
        GetHandlerPropertyFunc2 GetHandlerProperty2;
        CreateObjectFunc CreateObject;
        SetLargePageModeFunc SetLargePageMode;

        bool IsValid()
        {
            return GetMethodProperty != NULL &&
                GetNumberOfMethods != NULL &&
                CreateObject != NULL; 
        }
    } v;
} U7ZipFunctions, * pU7ZipFunctions;

class C7ZipCodecInfo : public virtual C7ZipObject
{
public:
    C7ZipCodecInfo();
    virtual ~C7ZipCodecInfo();

public:
    wstring m_Name;
    GUID m_ClassID;

    GUID Encoder;
    bool EncoderAssigned;

    GUID Decoder;
    bool DecoderAssigned;

    int CodecIndex;

    pU7ZipFunctions Functions;
};

class C7ZipFormatInfo : public virtual C7ZipObject
{
public:
    C7ZipFormatInfo();
    virtual ~C7ZipFormatInfo();

public:
    wstring m_Name;
    GUID m_ClassID;
    bool m_UpdateEnabled;
    bool m_KeepName;

    WStringArray Exts;
    WStringArray AddExts;

    int FormatIndex;
};

class CArchiveOpenCallback:
    public IArchiveOpenCallback,
    public ICryptoGetTextPassword,
	public IArchiveOpenVolumeCallback,
    public CMyUnknownImp
{
public:
    MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

    STDMETHOD(SetTotal)(const UInt64 *files, const UInt64 *bytes);
    STDMETHOD(SetCompleted)(const UInt64 *files, const UInt64 *bytes);

    STDMETHOD(CryptoGetTextPassword)(BSTR *password);

	// IArchiveOpenVolumeCallback
	STDMETHOD(GetProperty)(PROPID propID, PROPVARIANT *value);
	STDMETHOD(GetStream)(const wchar_t *name, IInStream **inStream);

    bool PasswordIsDefined;
    wstring Password;

    CArchiveOpenCallback() : PasswordIsDefined(false) {}
};

class CInFileStreamWrap:
    public IInStream,
    public IStreamGetSize,
    public CMyUnknownImp
{
public:
    CInFileStreamWrap(C7ZipInStream * pInStream);
    virtual ~CInFileStreamWrap() {}

public:
    MY_UNKNOWN_IMP2(IInStream, IStreamGetSize)

        STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
    STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);

    STDMETHOD(GetSize)(UInt64 *size);

private:
    C7ZipInStream * m_pInStream;
};

class C7ZipDllHandler : 
    public virtual C7ZipObject
{
public:
    C7ZipDllHandler(C7ZipLibrary * pLibrary, void * pHandler);
    virtual ~C7ZipDllHandler();

public:
    bool GetSupportedExts(WStringArray & exts);
    bool OpenArchive(C7ZipInStream * pInStream, C7ZipArchive ** ppArchive);
    bool IsInitialized() const { return m_bInitialized; }
    C7ZipLibrary * GetLibrary() const { return m_pLibrary; }
    const C7ZipObjectPtrArray & GetFormatInfoArray() const { return m_FormatInfoArray; }
    const C7ZipObjectPtrArray & GetCodecInfoArray() const { return m_CodecInfoArray; }
    pU7ZipFunctions GetFunctions() const { return const_cast<pU7ZipFunctions>(&m_Functions); }

#ifdef _WIN32
    wstring GetHandlerPath() const;
#else
    string GetHandlerPath() const;
#endif

private:
    C7ZipLibrary * m_pLibrary;
    bool m_bInitialized;
    void * m_pHandler;
    U7ZipFunctions m_Functions;
    C7ZipObjectPtrArray m_CodecInfoArray;
    C7ZipObjectPtrArray m_FormatInfoArray;

    void Initialize();
    void Deinitialize();
};

class C7ZipCompressCodecsInfo : public ICompressCodecsInfo,
    public CMyUnknownImp,
    public virtual C7ZipObject
{
public:
    C7ZipCompressCodecsInfo(C7ZipLibrary * pLibrary);
    virtual ~C7ZipCompressCodecsInfo();

    MY_UNKNOWN_IMP1(ICompressCodecsInfo)

    STDMETHOD(GetNumberOfMethods)(UInt32 *numMethods);
    STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value);
    STDMETHOD(CreateDecoder)(UInt32 index, const GUID *interfaceID, void **coder);
    STDMETHOD(CreateEncoder)(UInt32 index, const GUID *interfaceID, void **coder);

    void InitData();
private:
    C7ZipLibrary * m_pLibrary;
    C7ZipObjectPtrArray m_CodecInfoArray;
};

/*-------------- static functions ------------------------*/
#ifdef _WIN32
bool LoadDllFromFolder(C7ZipDllHandler * pMainHandler, const wstring & folder_name, C7ZipObjectPtrArray & handlers);
#else
bool LoadDllFromFolder(C7ZipDllHandler * pMainHandler, const string & folder_name, C7ZipObjectPtrArray & handlers);
#endif

static bool LoadFormats(pU7ZipFunctions pFunctions, C7ZipObjectPtrArray & formats)
{
    if (pFunctions->v.GetHandlerProperty == NULL &&
        pFunctions->v.GetHandlerProperty2 == NULL)
    {
        return false;
    }

    UInt32 numFormats = 1;

    if (pFunctions->v.GetNumberOfFormats != NULL)
    {
        RBOOLOK(pFunctions->v.GetNumberOfFormats(&numFormats));
    }

    if (pFunctions->v.GetHandlerProperty2 == NULL)
        numFormats = 1;

    for(UInt32 i = 0; i < numFormats; i++)
    {
        wstring name;
        bool updateEnabled = false;
        bool keepName = false;
        GUID classID;
        wstring ext, addExt;

        if (ReadStringProp(pFunctions->v.GetHandlerProperty, 
            pFunctions->v.GetHandlerProperty2, i, NArchive::kName, name) != S_OK)
            continue;

        NWindows::NCOM::CPropVariant prop;
        if (ReadProp(pFunctions->v.GetHandlerProperty, pFunctions->v.GetHandlerProperty2, 
            i, NArchive::kClassID, prop) != S_OK)
            continue;
        if (prop.vt != VT_BSTR)
            continue;

        classID = *(const GUID *)prop.bstrVal;

        if (ReadStringProp(pFunctions->v.GetHandlerProperty, pFunctions->v.GetHandlerProperty2, 
            i, NArchive::kExtension, ext) != S_OK)
            continue;

        if (ReadStringProp(pFunctions->v.GetHandlerProperty, pFunctions->v.GetHandlerProperty2, 
            i, NArchive::kAddExtension, addExt) != S_OK)
            continue;

        ReadBoolProp(pFunctions->v.GetHandlerProperty, pFunctions->v.GetHandlerProperty2, i, 
            NArchive::kUpdate, updateEnabled);

        if (updateEnabled)
        {
            ReadBoolProp(pFunctions->v.GetHandlerProperty, pFunctions->v.GetHandlerProperty2, 
                i, NArchive::kKeepName, keepName);
        }

        C7ZipFormatInfo * pInfo = new C7ZipFormatInfo();
        pInfo->m_Name = name;
        pInfo->m_KeepName = keepName;
        pInfo->m_ClassID = classID;
        pInfo->m_UpdateEnabled = updateEnabled;

        SplitString(ext, pInfo->Exts);
        SplitString(addExt, pInfo->AddExts);

        pInfo->FormatIndex = i;
        formats.push_back(pInfo);
    }

    return true;
}

static bool LoadCodecs(pU7ZipFunctions pFunctions, C7ZipObjectPtrArray & codecInfos)
{
    if (pFunctions->v.CreateObject == NULL)
        return false;

    if (pFunctions->v.GetMethodProperty == NULL)
        return false;

    UInt32 numMethods = 0;
    RBOOLOK(pFunctions->v.GetNumberOfMethods(&numMethods));

    for(UInt32 i = 0; i < numMethods; i++)
    {
        wstring name = L"";
        GUID classID;
        memset(&classID, 0, sizeof(GUID));

/*
        if(GetMethodPropertyString(pFunctions->v.GetMethodProperty, i, 
            NMethodPropID::kName, name) != S_OK)
            continue;

        if (GetMethodPropertyGUID(pFunctions->v.GetMethodProperty, i, 
            NMethodPropID::kID, classID) != S_OK)
            continue;
*/

        GUID encoder, decoder;
        bool encoderIsAssigned, decoderIsAssigned;

        if (GetCoderClass(pFunctions->v.GetMethodProperty, i, 
            NMethodPropID::kEncoder, encoder, encoderIsAssigned) != S_OK)
            continue;
        if (GetCoderClass(pFunctions->v.GetMethodProperty, i, 
            NMethodPropID::kDecoder, decoder, decoderIsAssigned) != S_OK)
            continue;

        C7ZipCodecInfo * pInfo = new C7ZipCodecInfo();
        pInfo->Functions = pFunctions;

        pInfo->m_Name = name;
        pInfo->m_ClassID = classID;

        pInfo->Encoder = encoder;
        pInfo->EncoderAssigned = encoderIsAssigned;

        pInfo->Decoder = decoder;
        pInfo->DecoderAssigned = decoderIsAssigned;

        pInfo->CodecIndex = i;
        codecInfos.push_back(pInfo);
    }

    return true;
}

static int CreateInArchive(pU7ZipFunctions pFunctions,
                       const C7ZipObjectPtrArray & formatInfos,
                       wstring ext,
                       CMyComPtr<IInArchive> &archive)
{
    for (C7ZipObjectPtrArray::const_iterator it = formatInfos.begin();
        it != formatInfos.end();it++)
    {
        const C7ZipFormatInfo * pInfo = dynamic_cast<const C7ZipFormatInfo *>(*it);

        for(WStringArray::const_iterator extIt = pInfo->Exts.begin(); extIt != pInfo->Exts.end(); extIt++)
        {
            if (MyStringCompareNoCase((*extIt).c_str(), ext.c_str()) == 0)
            {
                return pFunctions->v.CreateObject(&pInfo->m_ClassID, 
                    &IID_IInArchive, (void **)&archive);
            }
        }
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

static bool InternalOpenArchive(C7ZipLibrary * pLibrary,
                                C7ZipDllHandler * pHandler,
                                const wstring & ext, 
                                IInStream * pInStream,
                                C7ZipArchive ** ppArchive)
{
    CMyComPtr<IInArchive> archive = NULL;

    RBOOLOK(CreateInArchive(pHandler->GetFunctions(),
        pHandler->GetFormatInfoArray(),
        ext,
        archive));

    if (archive == NULL)
        return false;

    CMyComPtr<IInStream> inStream(pInStream); 

    CArchiveOpenCallback * pOpenCallBack = new CArchiveOpenCallback();

    CMyComPtr<IArchiveOpenCallback> openCallBack(pOpenCallBack);

    CMyComPtr<ISetCompressCodecsInfo> setCompressCodecsInfo;
    archive.QueryInterface(IID_ISetCompressCodecsInfo, (void **)&setCompressCodecsInfo);
    if (setCompressCodecsInfo)
    {
        C7ZipCompressCodecsInfo * pCompressCodecsInfo =
            new C7ZipCompressCodecsInfo(pLibrary);
        RBOOLOK(setCompressCodecsInfo->SetCompressCodecsInfo(pCompressCodecsInfo));
    }

    RBOOLOK(archive->Open(inStream, &kMaxCheckStartPosition, openCallBack));

    return Create7ZipArchive(pLibrary, archive, ppArchive);
}

/*------------------------ C7ZipObjectPtrArray ---------------------*/
C7ZipObjectPtrArray::C7ZipObjectPtrArray(bool auto_release) : m_bAutoRelease(auto_release)
{
}

C7ZipObjectPtrArray::~C7ZipObjectPtrArray()
{
    clear();
}

void C7ZipObjectPtrArray::clear()
{
    if (m_bAutoRelease)
    {
        for(C7ZipObjectPtrArray::iterator it = begin(); it != end(); it ++)
        {
            delete *it;
        }
    }

    std::vector<C7ZipObject *>::clear();
}

/*------------------------ C7ZipCodecInfo ---------------------*/
C7ZipCodecInfo::C7ZipCodecInfo()
{
    m_Name.clear();
    memset(&m_ClassID,0,sizeof(GUID));

    memset(&Encoder,0, sizeof(GUID));
    EncoderAssigned = false;

    memset(&Decoder,0, sizeof(GUID));
    DecoderAssigned = false;
}

C7ZipCodecInfo::~C7ZipCodecInfo()
{
}

/*------------------------ C7ZipFormatInfo ---------------------*/
C7ZipFormatInfo::C7ZipFormatInfo()
{
    m_Name.clear();
    memset(&m_ClassID,0,sizeof(GUID));
    m_UpdateEnabled = false;
    m_KeepName = false;

    Exts.clear();
    AddExts.clear();
}

C7ZipFormatInfo::~C7ZipFormatInfo()
{
}
/*------------------------ C7ZipLibrary ---------------------*/

C7ZipLibrary::C7ZipLibrary() :
m_bInitialized(false)
{
}

C7ZipLibrary::~C7ZipLibrary()
{
    Deinitialize();
}

void C7ZipLibrary::Deinitialize()
{
    if (!m_bInitialized)
        return;

    m_InternalObjectsArray.clear();

    m_bInitialized = false;
}

bool C7ZipLibrary::Initialize()
{
    if (m_bInitialized)
        return true;

#ifdef _WIN32
    void * pHandler = LoadLibrary(L"7z.dll");
#else
    void * pHandler = dlopen("7z.so", RTLD_LAZY | RTLD_GLOBAL);
#endif

    if (pHandler == NULL)
        return false;

    C7ZipDllHandler * p7ZipHandler = new C7ZipDllHandler(this, pHandler);

    if (p7ZipHandler->IsInitialized())
    {
        m_InternalObjectsArray.push_back(p7ZipHandler);

        m_bInitialized = true;

#ifdef _WIN32
        LoadDllFromFolder(p7ZipHandler, L"Codecs", m_InternalObjectsArray);
        LoadDllFromFolder(p7ZipHandler, L"Formats", m_InternalObjectsArray);
#else
        LoadDllFromFolder(p7ZipHandler, "Codecs", m_InternalObjectsArray);
        LoadDllFromFolder(p7ZipHandler, "Formats", m_InternalObjectsArray);
#endif
    }
    else
    {
        delete p7ZipHandler;
        m_bInitialized = false;
    }

    return m_bInitialized;
}

bool C7ZipLibrary::GetSupportedExts(WStringArray & exts)
{
    exts.clear();

    if (!m_bInitialized)
        return false;

    for(C7ZipObjectPtrArray::iterator it = m_InternalObjectsArray.begin(); 
        it != m_InternalObjectsArray.end(); it++)
    {
        C7ZipDllHandler * pHandler = dynamic_cast<C7ZipDllHandler *>(*it);

        if (pHandler != NULL)
        {
            pHandler->GetSupportedExts(exts);
        }
    }

    return true;
}

/*--------------------CArchiveOpenCallback------------------*/
STDMETHODIMP CArchiveOpenCallback::SetTotal(const UInt64 * /* files */, const UInt64 * /* bytes */)
{
    return S_OK;
}

STDMETHODIMP CArchiveOpenCallback::SetCompleted(const UInt64 * /* files */, const UInt64 * /* bytes */)
{
    return S_OK;
}

STDMETHODIMP CArchiveOpenCallback::CryptoGetTextPassword(BSTR *password)
{
    if (!PasswordIsDefined)
    {
        // You can ask real password here from user
        // Password = GetPassword(OutStream);
        // PasswordIsDefined = true;
        return E_ABORT;
    }

#ifdef _WIN32
    return StringToBstr(Password.c_str(), password);
#else
	CMyComBSTR temp(Password.c_str());

	*password = temp.MyCopy();

	return S_OK;
#endif
}

bool C7ZipLibrary::OpenArchive(C7ZipInStream * pInStream, C7ZipArchive ** ppArchive)
{
    if (!m_bInitialized)
        return false;

    for(C7ZipObjectPtrArray::iterator it = m_InternalObjectsArray.begin(); 
        it != m_InternalObjectsArray.end(); it++)
    {
        C7ZipDllHandler * pHandler = dynamic_cast<C7ZipDllHandler *>(*it);

        if (pHandler != NULL && pHandler->OpenArchive(pInStream, ppArchive))
        {
            return true;
        }
    }

    return false;
}

/*------------------- C7ZipArchive -----------*/
C7ZipArchive::C7ZipArchive()
{
}

C7ZipArchive::~C7ZipArchive()
{
}

/*-------------------- C7ZipArchiveItem ----------------------*/
C7ZipArchiveItem::C7ZipArchiveItem()
{
}

C7ZipArchiveItem::~C7ZipArchiveItem()
{
}

/*----------------- CInFileStreamWrap ---------------------*/
CInFileStreamWrap::CInFileStreamWrap(C7ZipInStream * pInStream) :
m_pInStream(pInStream)
{
}

STDMETHODIMP CInFileStreamWrap::Read(void *data, UInt32 size, UInt32 *processedSize)
{
    return m_pInStream->Read(data,size,processedSize);
}

STDMETHODIMP CInFileStreamWrap::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
{
    return m_pInStream->Seek(offset,seekOrigin,newPosition);
}

STDMETHODIMP CInFileStreamWrap::GetSize(UInt64 *size)
{
    return m_pInStream->GetSize(size);
}

/*------------------------------ C7ZipDllHandler ------------------------*/
C7ZipDllHandler::C7ZipDllHandler(C7ZipLibrary * pLibrary, void * pHandler) :
m_pLibrary(pLibrary),
m_pHandler(pHandler),
m_bInitialized(false)
{
    Initialize();
}

C7ZipDllHandler::~C7ZipDllHandler()
{
    Deinitialize();
}

void C7ZipDllHandler::Initialize()
{
    pU7ZipFunctions pFunctions = &m_Functions;

    pFunctions->v.GetMethodProperty = 
        (GetMethodPropertyFunc)GetProcAddress((HMODULE)m_pHandler, myT("GetMethodProperty"));
    pFunctions->v.GetNumberOfMethods = 
        (GetNumberOfMethodsFunc)GetProcAddress((HMODULE)m_pHandler, myT("GetNumberOfMethods"));
    pFunctions->v.GetNumberOfFormats = 
        (GetNumberOfFormatsFunc)GetProcAddress((HMODULE)m_pHandler, myT("GetNumberOfFormats"));
    pFunctions->v.GetHandlerProperty = 
        (GetHandlerPropertyFunc)GetProcAddress((HMODULE)m_pHandler, myT("GetHandlerProperty"));
    pFunctions->v.GetHandlerProperty2 = 
        (GetHandlerPropertyFunc2)GetProcAddress((HMODULE)m_pHandler, myT("GetHandlerProperty2"));
    pFunctions->v.CreateObject = 
        (CreateObjectFunc)GetProcAddress((HMODULE)m_pHandler, myT("CreateObject"));
    pFunctions->v.SetLargePageMode = 
        (SetLargePageModeFunc)GetProcAddress((HMODULE)m_pHandler, myT("SetLargePageMode"));

    if (pFunctions->v.IsValid())
    {
        m_bInitialized = LoadCodecs(pFunctions, m_CodecInfoArray);

        m_bInitialized |= LoadFormats(pFunctions, m_FormatInfoArray);
    }
}

void C7ZipDllHandler::Deinitialize()
{
#ifdef _WIN32
    ::FreeLibrary((HMODULE)m_pHandler);
#else
    dlclose(m_pHandler);
#endif

    m_CodecInfoArray.clear();
    m_FormatInfoArray.clear();

    m_bInitialized = false;
}

bool C7ZipDllHandler::GetSupportedExts(WStringArray & exts)
{
    if (!m_bInitialized)
        return false;

    for(C7ZipObjectPtrArray::iterator it = m_FormatInfoArray.begin(); it != m_FormatInfoArray.end(); it++)
    {
        C7ZipFormatInfo * pInfo = dynamic_cast<C7ZipFormatInfo *>(*it);

        for(WStringArray::iterator extIt = pInfo->Exts.begin(); extIt != pInfo->Exts.end(); extIt++)
        {
            exts.push_back(*extIt);
        }
    }

    return true;
}

bool C7ZipDllHandler::OpenArchive(C7ZipInStream * pInStream, C7ZipArchive ** ppArchive)
{
    wstring ext = pInStream->GetExt();

    if (ext.length() == 0)
    {
        //TODO: use signature to detect file format
        return false;
    }

    CInFileStreamWrap * pArchiveStream = new CInFileStreamWrap(pInStream);

    CMyComPtr<IInStream> inStream(pArchiveStream); 

    return InternalOpenArchive(m_pLibrary, this, ext, inStream, ppArchive);
}

#ifdef _WIN32
wstring C7ZipDllHandler::GetHandlerPath() const
{
    wchar_t buf[255] = {0};

    if (GetModuleFileName((HMODULE)m_pHandler, buf, 254) > 0)
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
#else
string C7ZipDllHandler::GetHandlerPath() const
{
    Dl_info info;

    memset(&info, 0, sizeof(Dl_info));

    if (dladdr((void *)m_Functions.v.CreateObject,&info))
    {
        if (info.dli_fname != NULL)
        {
            string path = info.dli_fname;

            size_t pos = path.rfind("/");

            if (pos != string::npos)
            {
                return path.substr(0, pos);
            }
        }
    }

    return ".";
}
#endif

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
#else

#if defined(__WXMAC__) || defined(__APPLE__)
int myselect(struct dirent * pDir );
#else
int myselect(const struct dirent * pDir );
#endif

static C7ZipObjectPtrArray * g_pHandlers = NULL;
static C7ZipLibrary * g_pLibrary = NULL;

bool LoadDllFromFolder(C7ZipDllHandler * pMainHandler, const string & folder_name, C7ZipObjectPtrArray & handlers)
{
    g_pHandlers = &handlers;
    g_pLibrary = pMainHandler->GetLibrary();

    string mainHandlerPath = pMainHandler->GetHandlerPath();
    string folderPath = mainHandlerPath + "/" + folder_name;

    char * current_dir = getcwd(NULL, 0);

    int result = chdir(folderPath.c_str());

    struct dirent **namelist = NULL;

    if (result == 0)
    {
        scandir( ".", &namelist,myselect,alphasort );
    }

    result = chdir(current_dir);

    free(current_dir);

    g_pHandlers = NULL;
    g_pLibrary = NULL;
    return true;
}

#if defined(__WXMAC__) || defined(__APPLE__)
int myselect(struct dirent * pDir )
#else
int myselect(const struct dirent * pDir )
#endif
{
    if ( NULL == pDir )
        return 0;

    const char * szEntryName = pDir->d_name;

    if ( ( strcasecmp( szEntryName,"." ) == 0 ) ||
        ( strcasecmp( szEntryName,".." ) == 0 ) )
    {
        return 0;
    }

    DIR * pTmpDir = NULL;

    if ( NULL == ( pTmpDir = opendir(szEntryName) ) )
    {
        if ( errno == ENOTDIR )
        {
            char * current_path = getcwd(NULL, 0);
            string path = current_path;
            path += "/";
            path += szEntryName;
            free(current_path);

            void * pHandler = dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);

            if (pHandler != NULL)
            {
                C7ZipDllHandler * p7ZipHandler = new C7ZipDllHandler(g_pLibrary, pHandler);

                if (p7ZipHandler->IsInitialized())
                {
                    g_pHandlers->push_back(p7ZipHandler);
                }
                else
                {
                    delete p7ZipHandler;
                }
            }
        }
    }
    else
    {
        closedir( pTmpDir );

        int result = chdir( szEntryName );

        struct dirent **namelist = NULL;

        scandir( ".",&namelist,myselect,alphasort );

        result = chdir( ".." );
    }

    return 0;
}
#endif

/*----------------------- C7ZipCompressCodecsInfo -------------------------*/
C7ZipCompressCodecsInfo::C7ZipCompressCodecsInfo(C7ZipLibrary * pLibrary) :
m_pLibrary(pLibrary),
m_CodecInfoArray(false)
{
    InitData();
}

C7ZipCompressCodecsInfo::~C7ZipCompressCodecsInfo()
{
}

void C7ZipCompressCodecsInfo::InitData()
{
    if (!m_pLibrary->IsInitialized())
        return;

    const C7ZipObjectPtrArray & handlers = 
        m_pLibrary->GetInternalObjectsArray();

    for(C7ZipObjectPtrArray::const_iterator it = handlers.begin(); 
        it != handlers.end(); it++)
    {
        C7ZipDllHandler * pHandler = dynamic_cast<C7ZipDllHandler *>(*it);

        if (pHandler != NULL)
        {
            const C7ZipObjectPtrArray & codecs = pHandler->GetCodecInfoArray();

            for(C7ZipObjectPtrArray::const_iterator itCodec = codecs.begin(); 
                itCodec != codecs.end(); itCodec++)
            {
                m_CodecInfoArray.push_back(*itCodec);
            }
        }
    }
}

HRESULT C7ZipCompressCodecsInfo::GetNumberOfMethods(UInt32 *numMethods)
{
    *numMethods = (UInt32)m_CodecInfoArray.size();

    return S_OK;
}

HRESULT C7ZipCompressCodecsInfo::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
    C7ZipCodecInfo * pCodec = dynamic_cast<C7ZipCodecInfo *>(m_CodecInfoArray[index]);

    if (propID == NMethodPropID::kDecoderIsAssigned)
    {
        NWindows::NCOM::CPropVariant propVariant;
        propVariant = pCodec->DecoderAssigned;
        propVariant.Detach(value);
        return S_OK;
    }
    if (propID == NMethodPropID::kEncoderIsAssigned)
    {
        NWindows::NCOM::CPropVariant propVariant;
        propVariant = pCodec->EncoderAssigned;
        propVariant.Detach(value);
        return S_OK;
    }
    return pCodec->Functions->v.GetMethodProperty(pCodec->CodecIndex, propID, value);
}

HRESULT C7ZipCompressCodecsInfo::CreateDecoder(UInt32 index, const GUID *interfaceID, void **coder)
{
    C7ZipCodecInfo * pCodec = dynamic_cast<C7ZipCodecInfo *>(m_CodecInfoArray[index]);

    if (pCodec->DecoderAssigned)
        return pCodec->Functions->v.CreateObject(&pCodec->Decoder,
            interfaceID,
            coder);
    return S_OK;
}

HRESULT C7ZipCompressCodecsInfo::CreateEncoder(UInt32 index, const GUID *interfaceID, void **coder)
{
    C7ZipCodecInfo * pCodec = dynamic_cast<C7ZipCodecInfo *>(m_CodecInfoArray[index]);

    if (pCodec->EncoderAssigned)
        return pCodec->Functions->v.CreateObject(&pCodec->Encoder,
            interfaceID,
            coder);
    return S_OK;
}

STDMETHODIMP CArchiveOpenCallback::GetProperty(PROPID propID, PROPVARIANT *value) 
{
	wprintf(L"GetProperty:%d\n", propID);
	return S_OK;
}

STDMETHODIMP CArchiveOpenCallback::GetStream(const wchar_t *name, IInStream **inStream)
{
	wprintf(L"GetSTream %ls\n", name);
	return S_OK;
}
