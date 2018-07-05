#include "lib7zip.h"

#ifdef S_OK
#undef S_OK
#endif

#if !defined(_WIN32) && !defined(_OS2)
#include "CPP/myWindows/StdAfx.h"
#include "CPP/include_windows/windows.h"
#endif

#include "CPP/Common/Common.h"
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

class C7ZipSequentialOutStreamWrap:
	public ISequentialOutStream,
	public CMyUnknownImp
{
public:
	C7ZipSequentialOutStreamWrap(C7ZipSequentialOutStream * pSequentialOutStream) : m_pSequentialOutStream(pSequentialOutStream) {}
	virtual ~C7ZipSequentialOutStreamWrap() {}

public:
	MY_UNKNOWN_IMP1(ISequentialOutStream)

	STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize)
	{
		return m_pSequentialOutStream->Write(data, size, processedSize);
	}

private:
	C7ZipSequentialOutStream * m_pSequentialOutStream;
};

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
	C7ZipSequentialOutStreamWrap * _outFileStreamSpec;
	CMyComPtr<ISequentialOutStream> _outFileStream;

	C7ZipSequentialOutStream * m_pSequentialOutStream;
	const C7ZipArchive * m_pArchive;
	const C7ZipArchiveItem * m_pItem;
public:
	CArchiveExtractCallback(C7ZipSequentialOutStream * pSequentialOutStream,const C7ZipArchive * pArchive,const C7ZipArchiveItem * pItem) : 
		m_pSequentialOutStream(pSequentialOutStream),
		m_pArchive(pArchive),
		m_pItem(pItem)
	{
	}
};

class CCustomArchiveExtractCallback:
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
	C7ZipExtractCallback *m_pCallback;

	C7ZipSequentialOutStreamWrap * _outFileStreamSpec;
	CMyComPtr<ISequentialOutStream> _outFileStream;
	C7ZipSequentialOutStream * m_pSequentialOutStream;
public:
	CCustomArchiveExtractCallback(C7ZipExtractCallback *pCallback) :
		m_pCallback(pCallback) {

	}
};


class C7ZipArchiveImpl : public virtual C7ZipArchive
{
public:
	C7ZipArchiveImpl(C7ZipLibrary * pLibrary, IInArchive * pInArchive, const wstring &formatName);
	virtual ~C7ZipArchiveImpl();

public:
	virtual bool GetItemCount(unsigned int * pNumItems);
	virtual bool GetItemInfo(unsigned int index, C7ZipArchiveItem ** ppArchiveItem);
	virtual bool Extract(unsigned int index, C7ZipSequentialOutStream * pSequentialOutStream);
	virtual bool Extract(unsigned int index, C7ZipSequentialOutStream * pSequentialOutStream, const wstring & pwd);
	virtual bool Extract(const C7ZipArchiveItem * pArchiveItem, C7ZipSequentialOutStream * pSequentialOutStream);
	virtual bool ExtractSeveral(unsigned int *indexList, int numIndices, C7ZipExtractCallback *extractCallback);

	virtual void Close();

	virtual bool Initialize();

	virtual wstring GetArchivePassword() const;
	virtual void SetArchivePassword(const wstring & password);
	virtual wstring GetArchiveFormat() const;
	virtual bool IsPasswordSet() const;

	virtual bool GetUInt64Property(lib7zip::PropertyIndexEnum propertyIndex,
											   unsigned __int64 & val) const;
	virtual bool GetBoolProperty(lib7zip::PropertyIndexEnum propertyIndex,
								 bool & val) const;
	virtual bool GetStringProperty(lib7zip::PropertyIndexEnum propertyIndex,
									  wstring & val) const;
	virtual bool GetFileTimeProperty(lib7zip::PropertyIndexEnum propertyIndex,
									 unsigned __int64 & val) const;
private:
	C7ZipLibrary * m_pLibrary;
	CMyComPtr<IInArchive> m_pInArchive;
	C7ZipObjectPtrArray m_ArchiveItems;
	wstring m_Password;
	wstring m_Format;
};

C7ZipArchiveImpl::C7ZipArchiveImpl(C7ZipLibrary * pLibrary, IInArchive * pInArchive, const wstring &formatName) :
m_pLibrary(pLibrary),
m_pInArchive(pInArchive),
m_Format(formatName)
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

bool C7ZipArchiveImpl::Extract(unsigned int index, C7ZipSequentialOutStream * pSequentialOutStream)
{
	if (index < m_ArchiveItems.size())
	{
		return Extract(dynamic_cast<const C7ZipArchiveItem *>(m_ArchiveItems[(int)index]), pSequentialOutStream);
	}

	return false;
}

bool C7ZipArchiveImpl::Extract(unsigned int index, C7ZipSequentialOutStream * pSequentialOutStream, const wstring & pwd)
{
	if (index < m_ArchiveItems.size())
	{
		C7ZipArchiveItem * pItem = dynamic_cast<C7ZipArchiveItem *>(m_ArchiveItems[(int)index]);
		pItem->SetArchiveItemPassword(pwd);

		return Extract(pItem, pSequentialOutStream);
	}

	return false;
}

bool C7ZipArchiveImpl::Extract(const C7ZipArchiveItem * pArchiveItem, C7ZipSequentialOutStream * pSequentialOutStream)
{
	CArchiveExtractCallback *extractCallbackSpec = 
		new CArchiveExtractCallback(pSequentialOutStream, this, pArchiveItem);
	CMyComPtr<IArchiveExtractCallback> extractCallback(extractCallbackSpec);

	UInt32 nArchiveIndex = pArchiveItem->GetArchiveIndex();

	return m_pInArchive->Extract(&nArchiveIndex, 1, false, extractCallbackSpec) == S_OK;
}

bool C7ZipArchiveImpl::ExtractSeveral(unsigned int *indexList, int numIndices, C7ZipExtractCallback *pCallback) {
	CCustomArchiveExtractCallback *extractCallbackSpec = new CCustomArchiveExtractCallback(pCallback);
	CMyComPtr<IArchiveExtractCallback> extractCallback(extractCallbackSpec);

	return m_pInArchive->Extract(indexList, numIndices, false, extractCallbackSpec) == S_OK;
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

wstring C7ZipArchiveImpl::GetArchivePassword() const
{
	return m_Password;
}

wstring C7ZipArchiveImpl::GetArchiveFormat() const
{
	return m_Format;
}

void C7ZipArchiveImpl::SetArchivePassword(const wstring & password)
{
	m_Password = password;
}

bool C7ZipArchiveImpl::IsPasswordSet() const
{
	return !(m_Password == L"");
}

bool Create7ZipArchive(C7ZipLibrary * pLibrary, IInArchive * pInArchive, C7ZipArchive ** ppArchive, const wstring &formatName)
{
	C7ZipArchiveImpl * pArchive = new C7ZipArchiveImpl(pLibrary, pInArchive, formatName);

	if (pArchive->Initialize())
	{
		*ppArchive = pArchive;

		return true;
	}

	delete pArchive;
	*ppArchive = NULL;

	return false;
}

//-------------------------------

STDMETHODIMP CCustomArchiveExtractCallback::SetTotal(UInt64 size)
{
	m_pCallback->SetTotal(size);
	return S_OK;
}

STDMETHODIMP CCustomArchiveExtractCallback::SetCompleted(const UInt64 * completeValue)
{
	m_pCallback->SetCompleted(completeValue);
	return S_OK;
}

STDMETHODIMP CCustomArchiveExtractCallback::GetStream(UInt32 index,
												ISequentialOutStream **outStream, Int32 askExtractMode)
{
	if (askExtractMode != NArchive::NExtract::NAskMode::kExtract) {
		// nothing to do then!
		return S_OK;
	}

	m_pSequentialOutStream = m_pCallback->GetStream(index);
	if (!m_pSequentialOutStream) {
		// then just don't extract it I guess?
		return S_OK;
	}

	_outFileStreamSpec = new C7ZipSequentialOutStreamWrap(m_pSequentialOutStream);
	CMyComPtr<ISequentialOutStream> outStreamLoc(_outFileStreamSpec);

	_outFileStream = outStreamLoc;
	*outStream = outStreamLoc.Detach();
	return S_OK;
}

STDMETHODIMP CCustomArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
{
	return S_OK;
}

STDMETHODIMP CCustomArchiveExtractCallback::SetOperationResult(Int32 operationResult)
{
	switch(operationResult)
	{
	case NArchive::NExtract::NOperationResult::kOK:
		break;
	default:
		{
			switch(operationResult)
			{
			default:
				break;
			}
		}
	}

	_outFileStream.Release();
	m_pSequentialOutStream = NULL;

	m_pCallback->SetOperationResult(operationResult);

	return S_OK;
}

STDMETHODIMP CCustomArchiveExtractCallback::CryptoGetTextPassword(BSTR *password)
{
	return E_NOTIMPL;
}

// ------------------------------

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


	_outFileStreamSpec = new C7ZipSequentialOutStreamWrap(m_pSequentialOutStream);
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
	wstring strPassword(L"");

	if (m_pItem->IsPasswordSet())
		strPassword = m_pItem->GetArchiveItemPassword();
	else if (m_pArchive->IsPasswordSet())
		strPassword = m_pArchive->GetArchivePassword();
	
#ifdef _WIN32
	return StringToBstr(strPassword.c_str(), password);
#else
	*password = ::SysAllocString(strPassword.c_str());
	
	return S_OK;
#endif
}

bool C7ZipArchiveImpl::GetUInt64Property(lib7zip::PropertyIndexEnum propertyIndex,
											 unsigned __int64 & val) const
{
	int p7zip_index = 0;

	switch(propertyIndex) {
	case lib7zip::kpidSize:
		p7zip_index = kpidSize;
		break;
	case lib7zip::kpidPackSize: //(Packed Size)
		p7zip_index = kpidPackSize;
		break;
	case lib7zip::kpidAttrib: //(Attributes)
		p7zip_index = kpidAttrib;
		break;
	case lib7zip::kpidPhySize: //(Physical Size)
		p7zip_index = kpidPhySize;
		break;
	case lib7zip::kpidHeadersSize: //(Headers Size)
		p7zip_index = kpidHeadersSize;
		break;
	case lib7zip::kpidChecksum: //(Checksum)
		p7zip_index = kpidChecksum;
		break;
	case lib7zip::kpidTotalSize: //(Total Size)
		p7zip_index = kpidTotalSize;
		break;
	case lib7zip::kpidFreeSpace: //(Free Space)
		p7zip_index = kpidFreeSpace;
		break;
	case lib7zip::kpidClusterSize: //(Cluster Size)
		p7zip_index = kpidClusterSize;
		break;
	default:
		return false;
	}

	NWindows::NCOM::CPropVariant prop;

	if (m_pInArchive->GetArchiveProperty(p7zip_index, &prop) != 0)
		return false;

	if (prop.vt == VT_UI8 || prop.vt == VT_UI4) {
		val = ConvertPropVariantToUInt64(prop);
		return true;
	}

	return false;
}

bool C7ZipArchiveImpl::GetBoolProperty(lib7zip::PropertyIndexEnum propertyIndex,
										   bool & val) const
{
	int p7zip_index = 0;

	switch(propertyIndex) {
	case lib7zip::kpidSolid: //(Solid)
		p7zip_index = kpidSolid;
		break;
	case lib7zip::kpidEncrypted: //(Encrypted)
		p7zip_index = kpidEncrypted;
		break;
	default:
		return false;
	}

	NWindows::NCOM::CPropVariant prop;

	if (m_pInArchive->GetArchiveProperty(p7zip_index, &prop) == 0 && 
		prop.vt == VT_BOOL) {
		val = prop.bVal;
		return true;
	}

	return false;
}

bool C7ZipArchiveImpl::GetStringProperty(lib7zip::PropertyIndexEnum propertyIndex,
					   wstring & val) const
{
	int p7zip_index = 0;

	switch(propertyIndex) {
	case lib7zip::kpidComment: //(Comment)
		p7zip_index = kpidComment;
		break;
	case lib7zip::kpidCharacts: //(Characteristics)
		p7zip_index = kpidCharacts;
		break;
	case lib7zip::kpidCreatorApp: //(Creator Application)
		p7zip_index = kpidCreatorApp;
		break;
	case lib7zip::kpidVolumeName: //(Label)
		p7zip_index = kpidVolumeName;
		break;
	case lib7zip::kpidPath: //(FullPath)
		p7zip_index = kpidPath;
		break;
	case lib7zip::kpidUser: //(User)
		p7zip_index = kpidUser;
		break;
	case lib7zip::kpidGroup: //(Group)
		p7zip_index = kpidGroup;
		break;
	default:
		return false;
	}

	NWindows::NCOM::CPropVariant prop;

	if (!m_pInArchive->GetArchiveProperty(p7zip_index, &prop) &&
		prop.vt == VT_BSTR) {
		val = prop.bstrVal;
		return true;
	}

	return false;
}

bool C7ZipArchiveImpl::GetFileTimeProperty(lib7zip::PropertyIndexEnum propertyIndex,
											 unsigned __int64 & val) const
{
	int p7zip_index = 0;

	switch(propertyIndex) {
	case lib7zip::kpidCTime: //(Created)
		p7zip_index = kpidCTime;
		break;
	case lib7zip::kpidATime: //(Accessed)
		p7zip_index = kpidATime;
		break;
	case lib7zip::kpidMTime: //(Modified)
		p7zip_index = kpidMTime;
		break;
	default:
		return false;
	}

	NWindows::NCOM::CPropVariant prop;

	if (m_pInArchive->GetArchiveProperty(p7zip_index, &prop) != 0)
		return false;

	if (prop.vt == VT_FILETIME) {
		unsigned __int64 tmp_val = 0;
		memmove(&tmp_val, &prop.filetime, sizeof(unsigned __int64));
		val = tmp_val;
		return true;
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

