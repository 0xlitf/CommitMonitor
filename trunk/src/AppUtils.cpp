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