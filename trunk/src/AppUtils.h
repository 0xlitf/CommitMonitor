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

#pragma once

#include <string>
#include <vector>
#include "svn_time.h"

class CAppUtils
{
public:
    CAppUtils(void);
    ~CAppUtils(void);

    static std::wstring             GetDataDir();
    static std::wstring             GetAppDataDir();
    static std::wstring             GetAppDirectory(HMODULE hMod = NULL);
    static std::wstring             GetAppName(HMODULE hMod = NULL);
    static std::wstring             ConvertDate(apr_time_t time);
    static void                     SearchReplace(std::wstring& str, const std::wstring& toreplace, const std::wstring& replacewith);
    static std::vector<std::wstring> tokenize_str(const std::wstring& str, const std::wstring& delims);
    static bool                     LaunchApplication(const std::wstring& sCommandLine, bool bWaitForStartup = false, bool bWaitForExit = false, bool bHideWindow = false);
    static std::wstring             GetTempFilePath();
    static std::wstring             ConvertName(const std::wstring& name);
    static std::string              PathEscape(const std::string& path);
    static bool                     IsWow64();
    static std::wstring             GetTSVNPath();
    static std::wstring             GetVersionStringFromExe(LPCTSTR path);
    static bool                     ExtractBinResource(const std::wstring& strCustomResName, int nResourceId, const std::wstring& strOutputPath);
    static bool                     WriteAsciiStringToClipboard(const std::wstring& sClipdata, HWND hOwningWnd);
    static bool                     IsFullscreenWindowActive();
    static void                     CreateUUIDString(std::wstring& sUuid);
    static bool                     IsWindowCovered(HWND hWnd);
};
