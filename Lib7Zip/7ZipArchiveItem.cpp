#include "lib7zip.h"

#ifdef S_OK
#undef S_OK
#endif

#include "CPP/7zip/Archive/IArchive.h"
#include "CPP/Windows/PropVariant.h"
#include "CPP/Common/MyCom.h"
#include "CPP/7zip/ICoder.h"
#include "CPP/7zip/IPassword.h"
#include "CPP/7zip/Common/FileStreams.h"

#include "HelperFuncs.h"

const wchar_t *kEmptyFileAlias = L"[Content]";

class C7ZipArchiveItemImpl : public virtual C7ZipArchiveItem
{
public:
	C7ZipArchiveItemImpl(IInArchive * pInArchive,
						 unsigned int nIndex);
	virtual ~C7ZipArchiveItemImpl();

public:
	virtual wstring GetFullPath() const;
	virtual UInt64 GetSize() const;
	virtual bool IsDir() const;
	virtual bool IsEncrypted() const;
	virtual unsigned int GetArchiveIndex() const;

private:
	CMyComPtr<IInArchive> m_pInArchive;
	unsigned int m_nIndex;
};

C7ZipArchiveItemImpl::C7ZipArchiveItemImpl(IInArchive * pInArchive,
										   unsigned int nIndex) :
	m_pInArchive(pInArchive),
	m_nIndex(nIndex)
{
}

C7ZipArchiveItemImpl::~C7ZipArchiveItemImpl()
{
}

wstring C7ZipArchiveItemImpl::GetFullPath() const
{
	// Get Name
	NWindows::NCOM::CPropVariant prop;
	wstring fullPath = kEmptyFileAlias;

	if (!m_pInArchive->GetProperty(m_nIndex, kpidPath, &prop)) {
		if (prop.vt == VT_BSTR)
			fullPath = prop.bstrVal;
	}

	return fullPath;
}

UInt64 C7ZipArchiveItemImpl::GetSize() const
{
	// Get uncompressed size
	NWindows::NCOM::CPropVariant prop;
	if (m_pInArchive->GetProperty(m_nIndex, kpidSize, &prop) != 0)
		return 0;

	UInt64 size = 0;

	if (prop.vt == VT_UI8 || prop.vt == VT_UI4)
		size = ConvertPropVariantToUInt64(prop);

	return size;
}

bool C7ZipArchiveItemImpl::IsEncrypted() const
{
	// Check if encrypted (password protected)
	NWindows::NCOM::CPropVariant prop;
	bool isEncrypted = false;
	if (m_pInArchive->GetProperty(m_nIndex, kpidEncrypted, &prop) == 0 && prop.vt == VT_BOOL)
		isEncrypted = prop.bVal;
	return isEncrypted;
}

bool C7ZipArchiveItemImpl::IsDir() const
{
	// Check IsDir
	NWindows::NCOM::CPropVariant prop;
	bool isDir = false;
	IsArchiveItemFolder(m_pInArchive, m_nIndex, isDir);

	return isDir;
}

unsigned int C7ZipArchiveItemImpl::GetArchiveIndex() const
{
	return m_nIndex;
}

bool Create7ZipArchiveItem(C7ZipArchive * pArchive, 
						   IInArchive * pInArchive,
						   unsigned int nIndex,
						   C7ZipArchiveItem ** ppItem)
{
	*ppItem = new C7ZipArchiveItemImpl(pInArchive, nIndex);

	return true;
}
