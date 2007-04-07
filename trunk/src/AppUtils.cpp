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