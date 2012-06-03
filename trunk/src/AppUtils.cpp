// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007-2010, 2012 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "stdafx.h"
#include "AppUtils.h"
#include "Registry.h"
#include "ClipboardHelper.h"
#include "UnicodeUtils.h"

#include <shlwapi.h>
#include <shlobj.h>
#include <fstream>
#include <cctype>
#include <algorithm>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "version.lib")


static const char iri_escape_chars[256] = {
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,

    /* 128 */
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0
};

const char uri_autoescape_chars[256] = {
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 1, 0, 0,

    /* 64 */
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 0, 1,
    0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 1, 0,

    /* 128 */
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,

    /* 192 */
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
};

static const char uri_char_validity[256] = {
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 1, 0, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 1, 0, 0,

    /* 64 */
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 0, 1,
    0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 1, 0,

    /* 128 */
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,

    /* 192 */
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
};

CAppUtils::CAppUtils(void)
{
}

CAppUtils::~CAppUtils(void)
{
}

std::string CAppUtils::PathEscape(const std::string& path)
{
    std::string ret2;
    int c;
    for (int i=0; path[i]; ++i)
    {
        c = (unsigned char)path[i];
        if (iri_escape_chars[c])
        {
            // no escaping needed for that char
            ret2 += (unsigned char)path[i];
        }
        else
        {
            // char needs escaping
            char temp[7] = {0};
            sprintf_s(temp, 7, "%%%02X", (unsigned char)c);
            ret2 += temp;
        }
    }
    std::string ret;
    for (size_t i=0; i < ret2.size(); ++i)
    {
        c = (unsigned char)ret2[i];
        if (uri_autoescape_chars[c])
        {
            // no escaping needed for that char
            ret += (unsigned char)ret2[i];
        }
        else
        {
            // char needs escaping
            char temp[7] = {0};
            sprintf_s(temp, 7, "%%%02X", (unsigned char)c);
            ret += temp;
        }
    }

    return ret;
}

std::wstring CAppUtils::GetDataDir()
{
    bool bPortable = false;

    std::wstring appname = CAppUtils::GetAppName();
    std::transform(appname.begin(), appname.end(), appname.begin(), std::tolower);
    if ((appname.find(_T("portable")) != std::wstring::npos) ||
        (appname.find(_T("local")) != std::wstring::npos))
        bPortable = true;

    if (bPortable)
    {
        return CAppUtils::GetAppDirectory();
    }
    return CAppUtils::GetAppDataDir();
}

std::wstring CAppUtils::GetAppDataDir()
{
    WCHAR path[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path);
    PathAppend(path, _T("CommitMonitor"));
    if (!PathFileExists(path))
        CreateDirectory(path, NULL);
    return std::wstring(path);
}

std::wstring CAppUtils::GetAppDirectory(HMODULE hMod /* = NULL */)
{
    TCHAR pathbuf[MAX_PATH] = {0};
    std::wstring path;
    DWORD bufferlen = MAX_PATH;
    GetModuleFileName(hMod, pathbuf, bufferlen);
    path = pathbuf;
    path = path.substr(0, path.find_last_of('\\'));

    return path;
}

std::wstring CAppUtils::GetAppName(HMODULE hMod /* = NULL */)
{
    TCHAR pathbuf[MAX_PATH] = {0};
    std::wstring path;
    DWORD bufferlen = MAX_PATH;
    GetModuleFileName(hMod, pathbuf, bufferlen);
    path = pathbuf;
    path = path.substr(path.find_last_of('\\')+1);

    return path;
}


/* Number of micro-seconds between the beginning of the Windows epoch
* (Jan. 1, 1601) and the Unix epoch (Jan. 1, 1970)
*/
#define APR_DELTA_EPOCH_IN_USEC   APR_TIME_C(11644473600000000);

__inline void AprTimeToFileTime(LPFILETIME pft, apr_time_t t)
{
    LONGLONG ll;
    t += APR_DELTA_EPOCH_IN_USEC;
    ll = t * 10;
    pft->dwLowDateTime = (DWORD)ll;
    pft->dwHighDateTime = (DWORD) (ll >> 32);
    return;
}

std::wstring CAppUtils::ConvertDate(apr_time_t time)
{
    FILETIME ft = {0};
    AprTimeToFileTime(&ft, time);

    // Convert UTC to local time
    SYSTEMTIME systemtime;
    FileTimeToSystemTime(&ft,&systemtime);

    SYSTEMTIME localsystime;
    SystemTimeToTzSpecificLocalTime(NULL, &systemtime,&localsystime);

    TCHAR timebuf[1024] = {0};
    TCHAR datebuf[1024] = {0};

    LCID locale = MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT);

    GetDateFormat(locale, DATE_SHORTDATE, &localsystime, NULL, datebuf, 1024);
    GetTimeFormat(locale, 0, &localsystime, NULL, timebuf, 1024);

    std::wstring sRet = datebuf;
    sRet += _T(" ");
    sRet += timebuf;

    return sRet;
}

void CAppUtils::SearchReplace(std::wstring& str, const std::wstring& toreplace, const std::wstring& replacewith)
{
    std::wstring result;
    std::wstring::size_type pos = 0;
    for ( ; ; ) // while (true)
    {
        std::wstring::size_type next = str.find(toreplace, pos);
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

std::vector<std::wstring> CAppUtils::tokenize_str(const std::wstring& str, const std::wstring& delims)
{
    // Skip delims at beginning, find start of first token
    std::wstring::size_type lastPos = str.find_first_not_of(delims, 0);
    // Find next delimiter @ end of token
    std::wstring::size_type pos     = str.find_first_of(delims, lastPos);

    // output vector
    std::vector<std::wstring> tokens;

    while (std::wstring::npos != pos || std::wstring::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delims.  Note the "not_of". this is beginning of token
        lastPos = str.find_first_not_of(delims, pos);
        // Find next delimiter at end of token.
        pos     = str.find_first_of(delims, lastPos);
    }

    return tokens;
}

bool CAppUtils::LaunchApplication(const std::wstring& sCommandLine, bool bWaitForStartup, bool bWaitForExit, bool bHideWindow)
{
    STARTUPINFO startup;
    PROCESS_INFORMATION process;
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    memset(&process, 0, sizeof(process));

    std::unique_ptr<TCHAR[]> cmdbuf(new TCHAR[sCommandLine.length()+1]);
    _tcscpy_s(cmdbuf.get(), sCommandLine.length()+1, sCommandLine.c_str());

    if (bHideWindow) {
      // Make sure the command window starts hidden
      startup.dwFlags = STARTF_USESHOWWINDOW;
      startup.wShowWindow = SW_HIDE;
    }

    if (CreateProcess(NULL, cmdbuf.get(), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
    {
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
        ::MessageBox(NULL, (LPCWSTR)lpMsgBuf, _T("CommitMonitor"), MB_ICONERROR|MB_TASKMODAL);
        LocalFree(lpMsgBuf);
        return false;
    }

    if (bWaitForStartup)
    {
        WaitForInputIdle(process.hProcess, 10000);
    }

    // Total hack - just waits for up to 30 seconds!
    if (bWaitForExit)
    {
        WaitForSingleObject(process.hProcess, (30000));
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}

std::wstring CAppUtils::GetTempFilePath()
{
    DWORD len = ::GetTempPath(0, NULL);
    std::unique_ptr<TCHAR[]> temppath(new TCHAR[len+1]);
    std::unique_ptr<TCHAR[]> tempF(new TCHAR[len+50]);
    ::GetTempPath (len+1, temppath.get());
    std::wstring tempfile;
    ::GetTempFileName (temppath.get(), TEXT("cm_"), 0, tempF.get());
    tempfile = std::wstring(tempF.get());
    //now create the tempfile, so that subsequent calls to GetTempFile() return
    //different filenames.
    HANDLE hFile = CreateFile(tempfile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
    CloseHandle(hFile);
    return tempfile;
}

std::wstring CAppUtils::ConvertName(const std::wstring& name)
{
    TCHAR convertedName[4096] = {0};
    _tcscpy_s(convertedName, 4096, name.c_str());
    int cI = 0;
    while (convertedName[cI])
    {
        switch (convertedName[cI])
        {
        case '/':
        case '\\':
        case '?':
        case ':':
        case '*':
        case '.':
        case '<':
        case '>':
        case '\"':
        case '|':
            convertedName[cI] = '_';
        }
        cI++;
    }
    return std::wstring(convertedName);
}

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

LPFN_ISWOW64PROCESS fnIsWow64Process;

bool CAppUtils::IsWow64()
{
    BOOL bIsWow64 = false;

    fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

    if (NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
        {
            // handle error
        }
    }
    return !!bIsWow64;
}

std::wstring CAppUtils::GetTSVNPath()
{
    std::wstring sRet;
    CRegStdString tsvninstalled = CRegStdString(_T("Software\\TortoiseSVN\\ProcPath"), _T(""), false, HKEY_LOCAL_MACHINE);
    sRet = std::wstring(tsvninstalled);
    if (sRet.empty())
    {
        if (IsWow64())
        {
            CRegStdString tsvninstalled64 = CRegStdString(_T("Software\\TortoiseSVN\\ProcPath"), _T(""), false, HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY);
            sRet = std::wstring(tsvninstalled64);
        }
    }
    return sRet;
}

std::wstring CAppUtils::GetVersionStringFromExe(LPCTSTR path)
{
    struct TRANSARRAY
    {
        WORD wLanguageID;
        WORD wCharacterSet;
    };

    std::wstring sVersion;
    DWORD dwReserved,dwBufferSize;
    dwBufferSize = GetFileVersionInfoSize((LPTSTR)path,&dwReserved);

    if (dwBufferSize > 0)
    {
        LPVOID pBuffer = (void*) malloc(dwBufferSize);

        if (pBuffer != (void*) NULL)
        {
            UINT        nInfoSize = 0,
                nFixedLength = 0;
            LPSTR       lpVersion = NULL;
            VOID*       lpFixedPointer;
            TRANSARRAY* lpTransArray;
            TCHAR       strLangProduktVersion[MAX_PATH];

            GetFileVersionInfo((LPTSTR)path,
                dwReserved,
                dwBufferSize,
                pBuffer);

            VerQueryValue(  pBuffer,
                _T("\\VarFileInfo\\Translation"),
                &lpFixedPointer,
                &nFixedLength);
            lpTransArray = (TRANSARRAY*) lpFixedPointer;

            _stprintf_s(strLangProduktVersion, MAX_PATH,
                _T("\\StringFileInfo\\%04x%04x\\ProductVersion"),
                lpTransArray[0].wLanguageID,
                lpTransArray[0].wCharacterSet);

            VerQueryValue(pBuffer,
                (LPTSTR)strLangProduktVersion,
                (LPVOID *)&lpVersion,
                &nInfoSize);

            sVersion = (LPCTSTR)lpVersion;
            free(pBuffer);
        }
    }

    return sVersion;
}

bool CAppUtils::ExtractBinResource(const std::wstring& strCustomResName, int nResourceId, const std::wstring& strOutputPath)
{
    HGLOBAL hResourceLoaded;        // handle to loaded resource
    HRSRC hRes;                     // handle/ptr. to res. info.
    char *lpResLock;                // pointer to resource data
    DWORD dwSizeRes;
    std::wstring strAppLocation;

    strAppLocation = CAppUtils::GetAppDirectory();

    // find location of the resource and get handle to it
    hRes = FindResource(NULL, MAKEINTRESOURCE(nResourceId), strCustomResName.c_str());
    if (hRes == NULL)
        return false;

    // loads the specified resource into global memory.
    hResourceLoaded = LoadResource(NULL, hRes);
    if (hResourceLoaded == NULL)
        return false;

    // get a pointer to the loaded resource!
    lpResLock = (char*)LockResource(hResourceLoaded);
    if (lpResLock == NULL)
        return false;

    // determine the size of the resource, so we know how much to write out to file!
    dwSizeRes = SizeofResource(NULL, hRes);

    try
    {
        std::ofstream outputFile(strOutputPath.c_str(), std::ios::binary);

        outputFile.write((const char*)lpResLock, dwSizeRes);
        outputFile.close();
    }
    catch (std::exception e)
    {
        return false;
    }

    return true;
}

bool CAppUtils::WriteAsciiStringToClipboard(const std::wstring& sClipdata, HWND hOwningWnd)
{
    CClipboardHelper clipboardHelper;
    if (clipboardHelper.Open(hOwningWnd))
    {
        EmptyClipboard();
        HGLOBAL hClipboardData = CClipboardHelper::GlobalAlloc((sClipdata.size()+1)*sizeof(WCHAR));
        if (hClipboardData)
        {
            WCHAR* pchData = (WCHAR*)GlobalLock(hClipboardData);
            if (pchData)
            {
                _tcscpy_s(pchData, sClipdata.size()+1, (LPCWSTR)sClipdata.c_str());
                GlobalUnlock(hClipboardData);
                if (SetClipboardData(CF_UNICODETEXT, hClipboardData))
                {
                    // no need to also set CF_TEXT : the OS does this
                    // automatically.
                    return true;
                }
            }
        }
    }
    return false;
}


bool CAppUtils::IsFullscreenWindowActive()
{
    HWND hwnd = GetForegroundWindow();
    RECT rcWindow;
    GetWindowRect(hwnd, &rcWindow);

    HMONITOR hm = MonitorFromRect(&rcWindow,MONITOR_DEFAULTTONULL);
    if (!hm)
        return false;

    MONITORINFO mi = {sizeof (mi)};
    GetMonitorInfo(hm,&mi);

    return !!EqualRect(&rcWindow, &mi.rcMonitor);
}

void CAppUtils::CreateUUIDString(std::wstring& sUuid) {
    UUID Uuid;
    CoCreateGuid(&Uuid);
    RPC_WSTR pUIDStr;
    // Convert Unique ID to String
    UuidToString( &Uuid, &pUIDStr );

    sUuid = std::wstring((WCHAR *)pUIDStr);

    // Free allocated string memory
    RpcStringFree( &pUIDStr );
}

bool CAppUtils::IsWindowCovered( HWND hWnd )
{
    RECT wndRect = {0};
    GetWindowRect(hWnd, &wndRect);

    for (long i = wndRect.left; i < wndRect.right; i = i + (((wndRect.right-wndRect.left)/10)+1))
    {
        for (long j = wndRect.top; j < wndRect.bottom; j = j + (((wndRect.bottom-wndRect.top)/10)+1))
        {
            POINT pt;
            pt.x = i;
            pt.y = j;
            HWND hPointWnd = WindowFromPoint(pt);
            bool covered = true;
            while (hPointWnd)
            {
                if (hPointWnd==hWnd)
                {
                    covered = false;
                    break;
                }
                hPointWnd = GetParent(hPointWnd);
            }
            if (covered)
                return true;
        }
    }
    return false;
}
