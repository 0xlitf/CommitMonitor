#include "StdAfx.h"
#include "UrlInfo.h"

CUrlInfo::CUrlInfo(void) : lastchecked(0)
	, lastcheckedrev(0)
	, minutesinterval(90)
	, fetchdiffs(false)
{
}

CUrlInfo::~CUrlInfo(void)
{
}


CUrlInfos::CUrlInfos(void)
{
}

CUrlInfos::~CUrlInfos(void)
{
}

void CUrlInfos::Save(LPCWSTR filename)
{
	HANDLE hFile = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_DELETE, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN|FILE_ATTRIBUTE_COMPRESSED, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	bool bSuccess = Save(hFile);
	CloseHandle(hFile);
	if (bSuccess)
	{
		// rename the file to the original requested name
		TRACE(_T("data saved\n"));
	}
}

void CUrlInfos::Load(LPCWSTR filename)
{
	HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_DELETE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	Load(hFile);
	TRACE(_T("data loaded\n"));
	CloseHandle(hFile);
}