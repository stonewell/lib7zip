// Test7Zip.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../Lib7Zip/lib7zip.h"
#include <iostream>

class TestInStream : public C7ZipInStream
{
private:
	FILE * m_pFile;
	std::string m_strFileName;
	wstring m_strFileExt;
	int m_nFileSize;
public:
	TestInStream(std::string fileName) :
	  m_strFileName(fileName),
	  m_strFileExt(L"001")
	{
		m_pFile = fopen(fileName.c_str(), "rb");
		fseek(m_pFile, 0, SEEK_END);
		m_nFileSize = ftell(m_pFile);
		fseek(m_pFile, 0, SEEK_SET);

		int pos = m_strFileName.find_last_of(".");

		if (pos != m_strFileName.npos)
		{
#ifdef _WIN32
			std::string tmp = m_strFileName.substr(pos + 1);
			int nLen = MultiByteToWideChar(CP_ACP, 0, tmp.c_str(), -1, NULL, NULL);
			LPWSTR lpszW = new WCHAR[nLen];
			MultiByteToWideChar(CP_ACP, 0, 
			   tmp.c_str(), -1, lpszW, nLen);
			m_strFileExt = lpszW;
			// free the string
			delete[] lpszW;
#else
			m_strFileExt = L"001";
#endif
		}
		wprintf(L"Ext:%ls\n", m_strFileExt.c_str());
	}

	virtual ~TestInStream()
	{
		fclose(m_pFile);
	}

public:
	virtual wstring GetExt() const
	{
		wprintf(L"GetExt:%ls\n", m_strFileExt.c_str());
		return m_strFileExt;
	}

	virtual int Read(void *data, unsigned int size, unsigned int *processedSize)
	{
		int count = fread(data, 1, size, m_pFile);
		wprintf(L"Read:%d %d\n", size, count);

		if (count >= 0)
		{
			if (processedSize != NULL)
				*processedSize = count;

			return 0;
		}

		return 1;
	}

	virtual int Seek(__int64 offset, unsigned int seekOrigin, unsigned __int64 *newPosition)
	{
		int result = fseek(m_pFile, (long)offset, seekOrigin);
		wprintf(L"Seek:%ld %ld\n", offset, result);
		if (!result)
		{
			if (newPosition)
				*newPosition = ftell(m_pFile);

			return 0;
		}

		return result;
	}

	virtual int GetSize(unsigned __int64 * size)
	{
		if (size)
			*size = m_nFileSize;
		return 0;
	}
};

#ifdef _WIN32
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char * argv[])
#endif
{
	C7ZipLibrary lib;

	if (!lib.Initialize())
	{
		wprintf(L"initialize fail!\n");
		return 1;
	}

	WStringArray exts;

	if (!lib.GetSupportedExts(exts))
	{
		wprintf(L"get supported exts fail\n");
		return 1;
	}

	size_t size = exts.size();

	for(size_t i = 0; i < size; i++)
	{
		wstring ext = exts[i];

		for(size_t j = 0; j < ext.size(); j++)
		{
			wprintf(L"%c", (char)(ext[j] &0xFF));
		}

		wprintf(L"\n");
	}

	C7ZipArchive * pArchive = NULL;

	TestInStream stream("test.7z.001");
	if (lib.OpenArchive(&stream, &pArchive))
	{
		unsigned int numItems = 0;

		pArchive->GetItemCount(&numItems);

		wprintf(L"%d\n", numItems);

		for(unsigned int i = 0;i < numItems;i++)
		{
			C7ZipArchiveItem * pArchiveItem = NULL;

			if (pArchive->GetItemInfo(i, &pArchiveItem))
			{
				wprintf(L"%d,%ls,%d\n", pArchiveItem->GetArchiveIndex(),
					pArchiveItem->GetFullPath().c_str(),
					pArchiveItem->IsDir());
			}
		}
	}
	else
	{
		wprintf(L"open archive test.7z.001 fail\n");
	}

	if (pArchive != NULL)
		delete pArchive;

	return 0;
}

