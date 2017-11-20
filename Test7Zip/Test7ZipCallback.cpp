// Test7ZipCallback.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../Lib7Zip/lib7zip.h"
#include <iostream>
#include <sstream>

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
		m_strFileExt(L"rar")
	{
		
		wprintf(L"fileName.c_str(): %s\n", fileName.c_str());
		m_pFile = fopen(fileName.c_str(), "rb");
		if (m_pFile) {
			fseek(m_pFile, 0, SEEK_END);
			m_nFileSize = ftell(m_pFile);
			fseek(m_pFile, 0, SEEK_SET);

			int pos = m_strFileName.find_last_of(".");

			if (pos != m_strFileName.npos) {
#ifdef _WIN32
				std::string tmp = m_strFileName.substr(pos + 1);
				int nLen = MultiByteToWideChar(CP_ACP, 0, tmp.c_str(), -1, NULL, 0);
				LPWSTR lpszW = new WCHAR[nLen];
				MultiByteToWideChar(CP_ACP, 0, 
									tmp.c_str(), -1, lpszW, nLen);
				m_strFileExt = lpszW;
				// free the string
				delete[] lpszW;
#else
				m_strFileExt = L"rar";
#endif
			}
			wprintf(L"Ext:%ls\n", m_strFileExt.c_str());
		}
		else {
			wprintf(L"fileName.c_str(): %s cant open\n", fileName.c_str());
		}
	}

	virtual ~TestInStream()
	{
		if (m_pFile) {
			fclose(m_pFile);
			m_pFile = NULL;
		}
	}

public:
	virtual wstring GetExt() const
	{
		wprintf(L"GetExt:%ls\n", m_strFileExt.c_str());
		return m_strFileExt;
	}

	virtual int Read(void *data, unsigned int size, unsigned int *processedSize)
	{
		if (!m_pFile)
			return 1;

		int count = fread(data, 1, size, m_pFile);
		wprintf(L"Read:%d %d\n", size, count);

		if (count >= 0) {
			if (processedSize != NULL)
				*processedSize = count;

			return 0;
		}

		return 1;
	}

	virtual int Seek(__int64 offset, unsigned int seekOrigin, unsigned __int64 *newPosition)
	{
		if (!m_pFile)
			return 1;

		int result = fseek(m_pFile, (long)offset, seekOrigin);

		if (!result) {
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

class TestSequentialOutStream : public C7ZipSequentialOutStream
{
private:
	FILE * m_pFile;
	std::string m_strFileName;
	wstring m_strFileExt;
	int m_nFileSize;
public:
	TestSequentialOutStream(std::string fileName) :
	  m_strFileName(fileName),
	  m_strFileExt(L"7z")
	{
		printf("Writing to %s\n", fileName.c_str());

		m_pFile = fopen(fileName.c_str(), "wb");
		m_nFileSize = 0;

		int pos = m_strFileName.find_last_of(".");

		if (pos != m_strFileName.npos)
		{
#ifdef _WIN32
			std::string tmp = m_strFileName.substr(pos + 1);
			int nLen = MultiByteToWideChar(CP_ACP, 0, tmp.c_str(), -1, NULL, 0);
			LPWSTR lpszW = new WCHAR[nLen];
			MultiByteToWideChar(CP_ACP, 0, 
			   tmp.c_str(), -1, lpszW, nLen);
			m_strFileExt = lpszW;
			// free the string
			delete[] lpszW;
#else
			m_strFileExt = L"7z";
#endif
		}
		wprintf(L"Ext:%ls\n", m_strFileExt.c_str());
	}

	virtual ~TestSequentialOutStream()
	{
		printf("Closing %s\n", m_strFileName.c_str());
		fclose(m_pFile);
	}

public:
	virtual int Write(const void *data, unsigned int size, unsigned int *processedSize)
	{
		int count = fwrite(data, 1, size, m_pFile);
		wprintf(L"Write:%d %d\n", size, count);

		if (count >= 0)
		{
			if (processedSize != NULL)
				*processedSize = count;

			m_nFileSize += count;
			return 0;
		}

		return 1;
	}
};

class TestExtractCallback : public C7ZipExtractCallback
{
public:
	TestExtractCallback() {
		// muffin
	}

	// IProgress
  void SetTotal(unsigned __int64 size) {
		wprintf(L"SetTotal(%llu)\n", size);
	}
  void SetCompleted(const unsigned __int64 *completeValue) {
		wprintf(L"SetCompleted(%llu)\n", completeValue ? *completeValue : 0);
	}

	// IArchiveExtractCallback
	C7ZipSequentialOutStream *GetStream(int index) {
		std::ostringstream os;
		os << "item-" << index << ".dat";
		auto stream = new TestSequentialOutStream(os.str());
		return stream;
	}

	void SetOperationResult(int operationResult) {
		wprintf(L"SetOperationResult(%d)\n", operationResult);
	}
};

#ifdef _WIN32
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char * argv[])
#endif
{
	C7ZipLibrary lib;

	if (!lib.Initialize()) {
		wprintf(L"initialize fail!\n");
		return 1;
	}

	C7ZipArchive * pArchive = NULL;

	TestInStream stream("TestRar5.rar");
	if (lib.OpenArchive(&stream, &pArchive, true)) {
		unsigned int numItems = 0;

		pArchive->GetItemCount(&numItems);

		wprintf(L"Listing %d items: \n", numItems);
		wprintf(L"=======================\n");

		// auto testOut = TestSequentialOutStream("CallbackResult.txt");
		// unsigned int singleIndex = 0;
		// if (!pArchive->Extract(singleIndex, &testOut)) {
		// 	wprintf(L"extracting single from Test7Zip.7z failed :(\n");
		// }

		auto indices = new unsigned int[numItems];
		for (unsigned int i = 0; i < numItems; i++) {
			indices[i] = i;
		}
		unsigned int numIndices = numItems;

		auto pCallback = new TestExtractCallback();
		auto extractSuccess = pArchive->ExtractSeveral(indices, numIndices, pCallback);
		delete[] indices;
		if (!extractSuccess) {
			wprintf(L"extracting several from Test7Zip.7z failed :(\n");
		}
	} else {
		wprintf(L"open archive TestRar5.rar fail\n");
	}

	if (pArchive != NULL)
		delete pArchive;

	return 0;
}

