#include "lib7zip.h"

#ifdef S_OK
#undef S_OK
#endif

#include "CPP/7zip/Archive/IArchive.h"
#include "CPP/7zip/MyVersion.h"
#include "CPP/Windows/PropVariant.h"
#include "CPP/Common/MyCom.h"
#include "CPP/7zip/ICoder.h"
#include "CPP/7zip/IPassword.h"
#include "CPP/7zip/Common/FileStreams.h"

#include "HelperFuncs.h"

extern bool Create7ZipArchiveItem(C7ZipArchive * pArchive, 
								  IInArchive * pInArchive,
								  unsigned int nIndex,
								  C7ZipArchiveItem ** ppItem);

class C7ZipOutStreamWrap:
	public IOutStream,
	public CMyUnknownImp
{
public:
	C7ZipOutStreamWrap(C7ZipOutStream * pOutStream) : m_pOutStream(pOutStream) {}
	virtual ~C7ZipOutStreamWrap() {}

public:
	MY_UNKNOWN_IMP1(IOutStream)

	STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
	{
		return m_pOutStream->Seek(offset, seekOrigin, newPosition);
	}

#if MY_VER_MAJOR > 9 || (MY_VER_MAJOR == 9 && MY_VER_MINOR>=20)
	STDMETHOD(SetSize)(UInt64 newSize)
#else
	STDMETHOD(SetSize)(Int64 newSize)
#endif
	{
		return m_pOutStream->SetSize(newSize);
	}

	STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize)
	{
		return m_pOutStream->Write(data, size, processedSize);
	}

private:
	C7ZipOutStream * m_pOutStream;
};

class CArchiveExtractCallback:
	public IArchiveExtractCallback,
	public ICryptoGetTextPassword,
	public CMyUnknownImp
{
public:
	MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

	// IProgress
	STDMETHOD(SetTotal)(UInt64 size);
	STDMETHOD(SetCompleted)(const UInt64 *completeValue);

	// IArchiveExtractCallback
	STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode);
	STDMETHOD(PrepareOperation)(Int32 askExtractMode);
	STDMETHOD(SetOperationResult)(Int32 resultEOperationResult);

	// ICryptoGetTextPassword
	STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

private:
	C7ZipOutStreamWrap * _outFileStreamSpec;
	CMyComPtr<ISequentialOutStream> _outFileStream;

	C7ZipOutStream * m_pOutStream;
public:
	CArchiveExtractCallback(C7ZipOutStream * pOutStream) : 
		m_pOutStream(pOutStream)
	{
	}
};

class C7ZipArchiveImpl : public virtual C7ZipArchive
{
public:
	C7ZipArchiveImpl(C7ZipLibrary * pLibrary, IInArchive * pInArchive);
	virtual ~C7ZipArchiveImpl();

public:
	virtual bool GetItemCount(unsigned int * pNumItems);
	virtual bool GetItemInfo(unsigned int index, C7ZipArchiveItem ** ppArchiveItem);
	virtual bool Extract(unsigned int index, C7ZipOutStream * pOutStream);
	virtual bool Extract(const C7ZipArchiveItem * pArchiveItem, C7ZipOutStream * pOutStream);

	virtual void Close();

	virtual bool Initialize();

private:
	C7ZipLibrary * m_pLibrary;
	CMyComPtr<IInArchive> m_pInArchive;
	C7ZipObjectPtrArray m_ArchiveItems;
};

C7ZipArchiveImpl::C7ZipArchiveImpl(C7ZipLibrary * pLibrary, IInArchive * pInArchive) :
m_pLibrary(pLibrary),
m_pInArchive(pInArchive)
{
}

C7ZipArchiveImpl::~C7ZipArchiveImpl()
{
}

bool C7ZipArchiveImpl::GetItemCount(unsigned int * pNumItems)
{
	*pNumItems = (unsigned int)m_ArchiveItems.size();

	return true;
}

bool C7ZipArchiveImpl::GetItemInfo(unsigned int index, C7ZipArchiveItem ** ppArchiveItem)
{
	if (index < m_ArchiveItems.size())
	{
		*ppArchiveItem = dynamic_cast<C7ZipArchiveItem *>(m_ArchiveItems[(int)index]);

		return true;
	}

	*ppArchiveItem = NULL;
	return false;
}

bool C7ZipArchiveImpl::Extract(unsigned int index, C7ZipOutStream * pOutStream)
{
	if (index < m_ArchiveItems.size())
	{
		return Extract(dynamic_cast<const C7ZipArchiveItem *>(m_ArchiveItems[(int)index]), pOutStream);
	}

	return false;
}

bool C7ZipArchiveImpl::Extract(const C7ZipArchiveItem * pArchiveItem, C7ZipOutStream * pOutStream)
{
	CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback(pOutStream);
	CMyComPtr<IArchiveExtractCallback> extractCallback(extractCallbackSpec);

	UInt32 nArchiveIndex = pArchiveItem->GetArchiveIndex();

	return m_pInArchive->Extract(&nArchiveIndex, 1, false, extractCallbackSpec) == S_OK;
}

void C7ZipArchiveImpl::Close()
{
	m_pInArchive->Close();
}

bool C7ZipArchiveImpl::Initialize()
{
	UInt32 numItems = 0;

	RBOOLOK(m_pInArchive->GetNumberOfItems(&numItems));

	for(UInt32 i = 0; i < numItems; i++)
	{
		C7ZipArchiveItem * pItem = NULL;

		if (Create7ZipArchiveItem(this, m_pInArchive, i, &pItem))
		{
			m_ArchiveItems.push_back(pItem);
		}
	}

	return true;
}

bool Create7ZipArchive(C7ZipLibrary * pLibrary, IInArchive * pInArchive, C7ZipArchive ** ppArchive)
{
	C7ZipArchiveImpl * pArchive = new C7ZipArchiveImpl(pLibrary, pInArchive);

	if (pArchive->Initialize())
	{
		*ppArchive = pArchive;

		return true;
	}

	delete pArchive;
	*ppArchive = NULL;

	return false;
}

STDMETHODIMP CArchiveExtractCallback::SetTotal(UInt64 /* size */)
{
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UInt64 * /* completeValue */)
{
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::GetStream(UInt32 index,
												ISequentialOutStream **outStream, Int32 askExtractMode)
{
	if (askExtractMode != NArchive::NExtract::NAskMode::kExtract)
		return S_OK;


	_outFileStreamSpec = new C7ZipOutStreamWrap(m_pOutStream);
	CMyComPtr<ISequentialOutStream> outStreamLoc(_outFileStreamSpec);

	_outFileStream = outStreamLoc;
	*outStream = outStreamLoc.Detach();
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
{
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetOperationResult(Int32 operationResult)
{
	switch(operationResult)
	{
	case NArchive::NExtract::NOperationResult::kOK:
		break;
	default:
		{
			switch(operationResult)
			{
			case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
				break;
			case NArchive::NExtract::NOperationResult::kCRCError:
				break;
			case NArchive::NExtract::NOperationResult::kDataError:
				break;
			default:
				break;
			}
		}
	}

	_outFileStream.Release();

	return S_OK;
}


STDMETHODIMP CArchiveExtractCallback::CryptoGetTextPassword(BSTR *password)
{
#ifdef _WIN32
	return StringToBstr(L"", password);
#else
	CMyComBSTR temp(L"");

	*password = temp.MyCopy();

	return S_OK;
#endif
}

