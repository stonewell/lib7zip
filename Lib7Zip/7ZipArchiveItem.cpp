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

class C7ZipArchiveItemImpl : public virtual C7ZipArchiveItem
{
public:
	C7ZipArchiveItemImpl(wstring fullPath, UInt64 size, bool isDir, bool isEncrypted, unsigned int nIndex);
	virtual ~C7ZipArchiveItemImpl();

public:
	virtual wstring GetFullPath() const;
	virtual UInt64 GetSize() const;
	virtual bool IsDir() const;
	virtual bool IsEncrypted() const;
	virtual unsigned int GetArchiveIndex() const;
	virtual wstring GetArchiveItemPassword() const;
	virtual void SetArchiveItemPassword(const wstring & password);
	bool IsPasswordSet() const;

private:
	wstring m_FullPath;
	UInt64 m_Size;
	bool m_bIsDir;
	bool m_bIsEncrypted;
	unsigned int m_nIndex;
	wstring m_Password;
};

C7ZipArchiveItemImpl::C7ZipArchiveItemImpl(wstring fullPath, UInt64 size, bool isDir, bool isEncrypted, unsigned int nIndex) :
m_FullPath(fullPath),
m_Size(size),
m_bIsDir(isDir),
m_bIsEncrypted(isEncrypted),
m_nIndex(nIndex)
{
}

C7ZipArchiveItemImpl::~C7ZipArchiveItemImpl()
{
}

wstring C7ZipArchiveItemImpl::GetFullPath() const
{
	return m_FullPath;
}

UInt64 C7ZipArchiveItemImpl::GetSize() const
{
	return m_Size;
}

bool C7ZipArchiveItemImpl::IsEncrypted() const
{
	return m_bIsEncrypted;
}

bool C7ZipArchiveItemImpl::IsDir() const
{
	return m_bIsDir;
}

unsigned int C7ZipArchiveItemImpl::GetArchiveIndex() const
{
	return m_nIndex;
}

wstring C7ZipArchiveItemImpl::GetArchiveItemPassword() const
{
	return m_Password;
}

void C7ZipArchiveItemImpl::SetArchiveItemPassword(const wstring & password)
{
	m_Password = password;
}

bool C7ZipArchiveItemImpl::IsPasswordSet() const
{
	return !(m_Password == L"");
}

bool Create7ZipArchiveItem(C7ZipArchive * pArchive, 
						   const wstring & fullpath, 
						   UInt64 size,
						   bool isDir, 
						   bool isEncrypted,
						   unsigned int nIndex,
						   C7ZipArchiveItem ** ppItem)
{
	*ppItem = new C7ZipArchiveItemImpl(fullpath, size, isDir,isEncrypted,  nIndex);

	return true;
}
