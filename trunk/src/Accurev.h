// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2011 - Stefan Kueng

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
#include <vector>
#include <map>

#include "SCCS.h"

#include <string>

using namespace std;

class ACCUREV : public SCCS
{
public:
    ACCUREV(void);
    ~ACCUREV(void);

    void SetAuthInfo(const std::wstring& username, const std::wstring& password);

    bool GetFile(std::wstring sUrl, std::wstring sFile);

    wstring GetRootUrl(const std::wstring& path);
    size_t GetFileCount() {return (size_t)0;}

    svn_revnum_t GetHEADRevision(const std::wstring& repo, const std::wstring& url);

    bool GetLog(const std::wstring& repo, const std::wstring& url, svn_revnum_t startrev, svn_revnum_t endrev);

    bool Diff(const wstring& url1, svn_revnum_t pegrevision, svn_revnum_t revision1,
        svn_revnum_t revision2, bool ignoreancestry, bool nodiffdeleted,
        bool ignorecontenttype,  const wstring& options, bool bAppend,
        const wstring& outputfile, const wstring& errorfile);

    wstring CanonicalizeURL(const wstring& url);
    wstring GetLastErrorMsg();

    void SetAndClearProgressInfo(CProgressDlg * pProgressDlg, bool bShowProgressBar = false);

private:

  bool logParser(const wstring& repo, const wstring& url, const wstring& rawLog);
  bool issueParser(const wstring& rawLog, SCCSLogEntry& logEntry);

  // Accurev command line calls
  bool AccuLogin(const wstring& username, const wstring& password);
  bool AccuGetLastPromote(const wstring& repo, const wstring& url, long *pTransactionNo);
  bool AccuGetHistory(const wstring& repo, const wstring& url, long startrev, long endrev, wstring& rawLog);
  bool AccuIssueList(const wstring& repo, const wstring& url, long issueNo, wstring& rawLog);

  size_t ExecuteAccurev(std::wstring Parameters, size_t SecondsToWait, wstring& stdOut, wstring& stdErr);

  void ClearErrors();
  void SetError(const wchar_t *pErrorString);

private:
  svn_error_t errInt;
  const wchar_t *pErrorString;
};
