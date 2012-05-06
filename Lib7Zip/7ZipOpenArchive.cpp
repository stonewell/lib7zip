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
#include "7ZipDllHandler.h"
#include "7ZipCodecInfo.h"
#include "7ZipFormatInfo.h"
#include "7ZipArchiveOpenCallback.h"
#include "7ZipCompressCodecsInfo.h"
#include "7ZipInStreamWrapper.h"

const UInt64 kMaxCheckStartPosition = 1 << 22;

extern bool Create7ZipArchive(C7ZipLibrary * pLibrary, IInArchive * pInArchive, C7ZipArchive ** pArchive);

static int CreateInArchive(pU7ZipFunctions pFunctions,
						   const C7ZipObjectPtrArray & formatInfos,
						   wstring ext,
						   CMyComPtr<IInArchive> &archive)
{
	for (C7ZipObjectPtrArray::const_iterator it = formatInfos.begin();
		 it != formatInfos.end();it++) {
		const C7ZipFormatInfo * pInfo = dynamic_cast<const C7ZipFormatInfo *>(*it);

		for(WStringArray::const_iterator extIt = pInfo->Exts.begin(); extIt != pInfo->Exts.end(); extIt++) {
			if (MyStringCompareNoCase((*extIt).c_str(), ext.c_str()) == 0) {
				return pFunctions->v.CreateObject(&pInfo->m_ClassID, 
												  &IID_IInArchive, (void **)&archive);
			}
		}
	}

	return CLASS_E_CLASSNOTAVAILABLE;
}

static bool InternalOpenArchive(C7ZipLibrary * pLibrary,
								C7ZipDllHandler * pHandler,
								C7ZipInStream * pInStream,
								C7ZipArchiveOpenCallback * pOpenCallBack,
								C7ZipArchive ** ppArchive);

bool Lib7ZipOpenArchive(C7ZipLibrary * pLibrary,
					 C7ZipDllHandler * pHandler,
					 C7ZipInStream * pInStream,
					 C7ZipArchive ** ppArchive)
{
	C7ZipArchiveOpenCallback * pOpenCallBack = new C7ZipArchiveOpenCallback(NULL);

	return InternalOpenArchive(pLibrary, pHandler, pInStream, pOpenCallBack, ppArchive);
}

bool Lib7ZipOpenMultiVolumeArchive(C7ZipLibrary * pLibrary,
								C7ZipDllHandler * pHandler,
								C7ZipMultiVolumes * pMultiVolumes,
								C7ZipArchive ** ppArchive)
{
	wstring firstVolumeName = pMultiVolumes->GetFirstVolumeName();

	if (!pMultiVolumes->MoveToVolume(firstVolumeName))
		return false;

	C7ZipInStream * pInStream = pMultiVolumes->OpenCurrentVolumeStream();

	if (pInStream == NULL)
		return false;
	
	C7ZipArchiveOpenCallback * pOpenCallBack = new C7ZipArchiveOpenCallback(pMultiVolumes);

	return InternalOpenArchive(pLibrary, pHandler, pInStream, pOpenCallBack, ppArchive);
}

static bool InternalOpenArchive(C7ZipLibrary * pLibrary,
								C7ZipDllHandler * pHandler,
								C7ZipInStream * pInStream,
								C7ZipArchiveOpenCallback * pOpenCallBack,
								C7ZipArchive ** ppArchive)
{
	CMyComPtr<IInArchive> archive = NULL;
	CMyComPtr<ISetCompressCodecsInfo> setCompressCodecsInfo = NULL;
	CMyComPtr<IInArchiveGetStream> getStream = NULL;
	wstring extension = pInStream->GetExt();

	C7ZipInStreamWrapper * pArchiveStream = new C7ZipInStreamWrapper(pInStream);

	CMyComPtr<IInStream> inStream(pArchiveStream); 

	CMyComPtr<IArchiveOpenCallback> openCallBack(pOpenCallBack);

	do {
		RBOOLOK(CreateInArchive(pHandler->GetFunctions(),
								pHandler->GetFormatInfoArray(),
								extension,
								archive));

		if (archive == NULL)
			return false;

		archive.QueryInterface(IID_ISetCompressCodecsInfo, (void **)&setCompressCodecsInfo);

		if (setCompressCodecsInfo) {
			C7ZipCompressCodecsInfo * pCompressCodecsInfo =
				new C7ZipCompressCodecsInfo(pLibrary);
			RBOOLOK(setCompressCodecsInfo->SetCompressCodecsInfo(pCompressCodecsInfo));
		}

		RBOOLOK(archive->Open(inStream, &kMaxCheckStartPosition, openCallBack));

		UInt32 mainSubfile;
		{
			NCOM::CPropVariant prop;
			RINOK(archive->GetArchiveProperty(kpidMainSubfile, &prop));
			if (prop.vt == VT_UI4)
				mainSubfile = prop.ulVal;
			else {
				break;
			}
			UInt32 numItems;
			RINOK(archive->GetNumberOfItems(&numItems));
			if (mainSubfile >= numItems)
				break;
		}

		if (archive->QueryInterface(IID_IInArchiveGetStream, (void **)&getStream) != S_OK || !getStream)
			break;
    
		CMyComPtr<ISequentialInStream> subSeqStream;
		if (getStream->GetStream(mainSubfile, &subSeqStream) != S_OK || !subSeqStream)
			break;
    
		inStream = NULL;
		if (subSeqStream.QueryInterface(IID_IInStream, &inStream) != S_OK || !inStream)
			break;
    
		wstring path;

		RBOOLOK(GetArchiveItemPath(archive, mainSubfile, path));

		CMyComPtr<IArchiveOpenSetSubArchiveName> setSubArchiveName;

		openCallBack->QueryInterface(IID_IArchiveOpenSetSubArchiveName, (void **)&setSubArchiveName);
		if (setSubArchiveName) {
			setSubArchiveName->SetSubArchiveName(path.c_str());
		}

		RBOOLOK(GetFilePathExt(path, extension));
	} while(true);

	if (archive == NULL)
		return false;

	return Create7ZipArchive(pLibrary, archive, ppArchive);
}
