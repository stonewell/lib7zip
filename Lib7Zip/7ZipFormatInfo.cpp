#include "lib7zip.h"

#ifdef S_OK
#undef S_OK
#endif

#include "CPP/7zip/Archive/IArchive.h"
#include "CPP/Windows/PropVariant.h"
#include "CPP/Common/MyCom.h"
#include "CPP/7zip/ICoder.h"
#include "CPP/7zip/IPassword.h"
#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
using namespace NWindows;

#include "HelperFuncs.h"
#include "7ZipFunctions.h"
#include "7ZipFormatInfo.h"

/*------------------------ C7ZipFormatInfo ---------------------*/
C7ZipFormatInfo::C7ZipFormatInfo()
{
    m_Name.clear();
    memset(&m_ClassID,0,sizeof(GUID));
    m_UpdateEnabled = false;
    m_KeepName = false;
    m_StartSignature.clear();
	m_FinishSignature.clear();

    Exts.clear();
    AddExts.clear();
}

C7ZipFormatInfo::~C7ZipFormatInfo()
{
}

bool LoadFormats(pU7ZipFunctions pFunctions, C7ZipObjectPtrArray & formats)
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

		wstring startSignature;
		wstring finishSignature;

		startSignature.clear();
		finishSignature.clear();

        ReadStringProp(pFunctions->v.GetHandlerProperty, pFunctions->v.GetHandlerProperty2, i, NArchive::kStartSignature, startSignature);
        ReadStringProp(pFunctions->v.GetHandlerProperty, pFunctions->v.GetHandlerProperty2, i, NArchive::kFinishSignature,  finishSignature);
		
        C7ZipFormatInfo * pInfo = new C7ZipFormatInfo();
        pInfo->m_Name = name;
        pInfo->m_KeepName = keepName;
        pInfo->m_ClassID = classID;
        pInfo->m_UpdateEnabled = updateEnabled;
		pInfo->m_StartSignature = startSignature;
		pInfo->m_FinishSignature = finishSignature;

        SplitString(ext, pInfo->Exts);
        SplitString(addExt, pInfo->AddExts);

        pInfo->FormatIndex = i;
        formats.push_back(pInfo);
    }

    return true;
}

