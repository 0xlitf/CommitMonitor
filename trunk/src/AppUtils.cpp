#include "StdAfx.h"
#include "AppUtils.h"


#include <shlwapi.h>
#include <shlobj.h>

#pragma comment(lib, "shlwapi.lib")

CAppUtils::CAppUtils(void)
{
}

CAppUtils::~CAppUtils(void)
{
}

wstring CAppUtils::GetAppDataDir()
{
	WCHAR path[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path);
	PathAppend(path, _T("CommitMonitor"));
	if (!PathFileExists(path))
		CreateDirectory(path, NULL);
	return wstring(path);
}

wstring CAppUtils::ConvertDate(apr_time_t time)
{
	apr_time_exp_t exploded_time = {0};
	SYSTEMTIME systime;
	TCHAR timebuf[1024];
	TCHAR datebuf[1024];

	LCID locale = MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT);

	apr_time_exp_lt (&exploded_time, time);

	systime.wDay = (WORD)exploded_time.tm_mday;
	systime.wDayOfWeek = (WORD)exploded_time.tm_wday;
	systime.wHour = (WORD)exploded_time.tm_hour;
	systime.wMilliseconds = (WORD)(exploded_time.tm_usec/1000);
	systime.wMinute = (WORD)exploded_time.tm_min;
	systime.wMonth = (WORD)exploded_time.tm_mon+1;
	systime.wSecond = (WORD)exploded_time.tm_sec;
	systime.wYear = (WORD)exploded_time.tm_year+1900;

	GetDateFormat(locale, DATE_SHORTDATE, &systime, NULL, datebuf, 1024);
	GetTimeFormat(locale, 0, &systime, NULL, timebuf, 1024);
	wstring sRet = datebuf;
	sRet += _T(" ");
	sRet += timebuf;
	return sRet;
}

void CAppUtils::SearchReplace(wstring& str, const wstring& toreplace, const wstring& replacewith)
{
	wstring result;
	wstring::size_type pos = 0;
	while(true) 
	{
		wstring::size_type next = str.find(toreplace, pos);
		result.append(str, pos, next-pos);
		if( next != std::string::npos ) 
		{
			result.append(replacewith);
			pos = next + toreplace.size();
		} 
		else 
		{
			break;  // exit loop
		}
	}
	str.swap(result);
}

bool CAppUtils::LaunchApplication(const wstring& sCommandLine, bool bWaitForStartup)
{
	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));

	TCHAR * cmdbuf = new TCHAR[sCommandLine.length()+1];
	_tcscpy_s(cmdbuf, sCommandLine.length()+1, sCommandLine.c_str());

	if (CreateProcess(NULL, cmdbuf, NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
	{
		delete [] cmdbuf;
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
			);
		::MessageBox(NULL, (LPCWSTR)lpMsgBuf, _T("CommitMonitor"), MB_ICONERROR);
		LocalFree(lpMsgBuf);
		return false;
	}
	delete [] cmdbuf;

	if (bWaitForStartup)
	{
		WaitForInputIdle(process.hProcess, 10000);
	}

	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return true;
}
