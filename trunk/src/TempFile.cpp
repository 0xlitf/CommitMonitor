#include "StdAfx.h"
#include "Registry.h"
#include "TempFile.h"

CTempFiles::CTempFiles(void)
{
}

CTempFiles::~CTempFiles(void)
{
	for (vector<wstring>::iterator it = m_TempFileList.begin(); it != m_TempFileList.end(); ++it)
	{
		DeleteFile(it->c_str());
	}
}

CTempFiles& CTempFiles::Instance()
{
	static CTempFiles instance;
	return instance;
}

wstring CTempFiles::GetTempFilePath(bool bRemoveAtEnd)
{
	DWORD len = ::GetTempPath(0, NULL);
	TCHAR * temppath = new TCHAR[len+1];
	TCHAR * tempF = new TCHAR[len+50];
	::GetTempPath (len+1, temppath);
	wstring tempfile;
	wstring possibletempfile;
	::GetTempFileName (temppath, TEXT("cmm"), 0, tempF);
	tempfile = wstring(tempF);
	// now create the tempfile, so that subsequent calls to GetTempFile() return
	// different filenames.
	HANDLE hFile = CreateFile(tempfile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
	CloseHandle(hFile);
	delete temppath;
	delete tempF;
	if (bRemoveAtEnd)
		m_TempFileList.push_back(tempfile);
	return tempfile;
}