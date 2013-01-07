// CommitMonitor - simple checker for new commits in accurev repositories

// Copyright (C) 2011-2013 - Stefan Kueng
// Copyright (C) 2010 - Richard Sewell

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

// TODO: Implement XML parsing rather than the horribly hacky string parsing
// TODO: Fix the reading of data from pipes to be asychronous

#include "stdafx.h"
#include "Accurev.h"
#include "AppUtils.h"
#include "version.h"
#include "HiddenWindow.h"

#include <iostream>

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ACCU_COMM_FAILURE   _T("Accurev comm failure")
#define ACCU_NO_ERROR       _T("")

// Local functions
static inline void char2wchar(char *pChar, wchar_t *pwChar, int length);
static inline std::wstring& replaceAll(std::wstring& context, const std::wstring& from, const std::wstring& to);
static inline std::wstring &trim(std::wstring &s);
static inline std::wstring &ltrim(std::wstring &s);
static inline std::wstring &rtrim(std::wstring &s);

ACCUREV::ACCUREV(void)
{
  Err = &errInt;
  ClearErrors();
}

ACCUREV::~ACCUREV(void)
{

}

std::wstring ACCUREV::GetLastErrorMsg()
{
    return pErrorString;
}

void ACCUREV::SetAuthInfo(const std::wstring& username, const std::wstring& password)
{
  ClearErrors();

  if (!AccuLogin(username, password))
  {
    std::wstring title = ACCU_COMM_FAILURE;
    std::wstring text = _T("Could not log in as ") + username;
    hiddenWindowPointer->ShowPopup(title, text, ALERTTYPE_FAILEDCONNECT);
    SetError(ACCU_COMM_FAILURE);
  }
}

bool ACCUREV::GetFile(std::wstring sUrl, std::wstring sFile)
{
    ClearErrors();
    return (Err == NULL);
}

std::wstring ACCUREV::GetRootUrl(const std::wstring& /*path*/)
{
    ClearErrors();
    return L"";
}

svn_revnum_t ACCUREV::GetHEADRevision(const std::wstring& repo, const std::wstring& url)
{
    svn_revnum_t revNum = (svn_revnum_t)0;

    ClearErrors();

    if (!AccuGetLastPromote(repo, url, (long *)&revNum))
    {
      std::wstring title = ACCU_COMM_FAILURE;
      std::wstring text = _T("Could not get history for ") + repo + L":" + url;
      hiddenWindowPointer->ShowPopup(title, text, ALERTTYPE_FAILEDCONNECT);
      SetError(ACCU_COMM_FAILURE);
    }

    return (svn_revnum_t)revNum;
}

bool ACCUREV::GetLog(const std::wstring& repo, const std::wstring& url, svn_revnum_t startrev, svn_revnum_t endrev)
{
    bool retVal = false;
    std::wstring rawLog;

    ClearErrors();

    m_logs.clear();

    // Limit the maximum number of transactions to go back to 50000 (we need a limit!)
    // NOTE: If overlapped I/O was implemented on the pipe reader, this limit would not
    // be needed (and the pipe buffer could be a lot smaller.
    long limit = 50000;

    if ((startrev - endrev) > limit)
    {
        endrev = startrev - limit;
        if (endrev < 1) endrev = 1;
    }

    if (!AccuGetHistory(repo, url, (long)startrev, (long)endrev, rawLog))
    {
      std::wstring title = ACCU_COMM_FAILURE;
      std::wstring text = _T("Could not get history for ") + repo + L":" + url;
      hiddenWindowPointer->ShowPopup(title, text, ALERTTYPE_FAILEDCONNECT);
      SetError(ACCU_COMM_FAILURE);
    }
    else {
      logParser(repo, url, rawLog);
      retVal = true;
    }

    return retVal;
}

bool ACCUREV::Diff(const std::wstring& url1, svn_revnum_t pegrevision, svn_revnum_t revision1,
               svn_revnum_t revision2, bool ignoreancestry, bool nodiffdeleted,
               bool ignorecontenttype,  const std::wstring& options, bool bAppend,
               const std::wstring& outputfile, const std::wstring& errorfile)
{
    UNREFERENCED_PARAMETER(url1);
    UNREFERENCED_PARAMETER(pegrevision);
    UNREFERENCED_PARAMETER(revision1);
    UNREFERENCED_PARAMETER(revision2);
    UNREFERENCED_PARAMETER(ignoreancestry);
    UNREFERENCED_PARAMETER(nodiffdeleted);
    UNREFERENCED_PARAMETER(ignorecontenttype);
    UNREFERENCED_PARAMETER(options);
    UNREFERENCED_PARAMETER(bAppend);
    UNREFERENCED_PARAMETER(outputfile);
    UNREFERENCED_PARAMETER(errorfile);


    return true;
}

std::wstring ACCUREV::CanonicalizeURL(const std::wstring& url)
{
    return url;
}

void ACCUREV::SetAndClearProgressInfo(CProgressDlg * pProgressDlg, bool bShowProgressBar/* = false*/)
{
    UNREFERENCED_PARAMETER(pProgressDlg);
    UNREFERENCED_PARAMETER(bShowProgressBar);
}

#define MAX_LOG_ENTRIES 1000
#if 0   // Old log parser (parses non-xml output)

bool ACCUREV::logParser(const wstring& repo, const wstring& url, const wstring& rawLog) {
    bool retVal = false;

    svn_error_t * error = NULL;
    SVNLogEntry logEntry;
    wstring dateTemp;
    wstring wDateTime;
    string sDateTime;

    int entryCount = 0;

    // Split into lines
    int iLineTokenNo = 0;
    const LPWSTR szLineTokens = L"\r\n";
    LPWSTR szNextLineToken = NULL;
    LPWSTR szLineToken = wcstok_s((wchar_t *)rawLog.c_str(), szLineTokens, &szNextLineToken);

    while (szLineToken != NULL)
    {
      // Detect transaction line
      if (wstring(szLineToken).find(L"transaction") == 0) {
        // Add transaction to the revision log
        if (logEntry.revision != 0) {
          m_logs[logEntry.revision] = logEntry;
          if (++entryCount >= MAX_LOG_ENTRIES) {
            logEntry.revision = 0;
            break;
          }
        }

        // Clear the logEntry object
        logEntry.revision = 0;
        logEntry.author = wstring();
        logEntry.date = (apr_time_t)0;
        logEntry.message = wstring();
        logEntry.m_changedPaths.clear();

        // Split transaction line into transactionNo:promote:date:user
        int iLogTokenNo = 0;
        const LPWSTR szLogTokens = L" ;";
        LPWSTR szNextLogToken = NULL;
        LPWSTR szLogToken = wcstok_s(szLineToken, szLogTokens, &szNextLogToken);

        while (szLogToken != NULL)
        {
          if (iLogTokenNo == 1) {
            logEntry.revision = _wtoi(szLogToken);
          }
          else if (iLogTokenNo == 3) {
            // Date
            dateTemp = wstring(szLogToken);
          }
          else if (iLogTokenNo == 4) {
            // Time & Date & convert to unix time
            wDateTime.clear();
            wDateTime.append(replaceAll(dateTemp, L"/", L"-"));
            wDateTime.append(L"T");
            wDateTime.append(szLogToken);
            wDateTime.append(L".000000Z");
            sDateTime = CUnicodeUtils::StdGetANSI(wDateTime);

            svn_time_from_cstring (&logEntry.date, sDateTime.c_str(), NULL);
          }
          else if (iLogTokenNo == 6) {
            // User
            logEntry.author = wstring(szLogToken);
          }
          szLogToken = wcstok_s(NULL, szLogTokens, &szNextLogToken);
          iLogTokenNo++;
        }

      }
      else if (wstring(szLineToken).find(L"  #") == 0) {
        // Parse the log entry
        LPWSTR pLogEntry = szLineToken + 3;
        logEntry.message.append(pLogEntry);
        logEntry.message.append(L"\n");
      }
      else if (wstring(szLineToken).find(L"  /./") == 0) {
        // Parse the file line
        LPWSTR pFileLine = szLineToken + 5;
        SVNLogChangedPaths changedPaths;
        changedPaths.action = L'M';

        logEntry.m_changedPaths[pFileLine] = changedPaths;
      }

      szLineToken = wcstok_s(NULL, szLineTokens, &szNextLineToken);
      iLineTokenNo++;
    }

    // Add final transaction to the revision log
    if (logEntry.revision != 0) {
      m_logs[logEntry.revision] = logEntry;
    }

    return retVal;
}

#else

bool ACCUREV::logParser(const std::wstring& repo, const std::wstring& url, const std::wstring& rawLog) {
    bool retVal = false;

    SCCSLogEntry logEntry;
    std::wstring dateTemp;
    std::wstring wDateTime;
    std::wstring wVirtualVersion, wRealVersion, wPath;
    BOOL bIssueRetrieved = FALSE;
    std::wstring wIssueNo;

    int *pIssueNos = NULL;
    int issueNoCount = 0;

    int entryCount = 0;

    // Split into lines
    int iLineTokenNo = 0;
    const LPWSTR szLineTokens = L"\r\n";
    LPWSTR szNextLineToken = NULL;
    LPWSTR szLineToken = wcstok_s((wchar_t *)rawLog.c_str(), szLineTokens, &szNextLineToken);

    while (szLineToken != NULL)
    {
      std::wstring wsLine(szLineToken);
      wsLine = trim (wsLine);

      // Detect transaction line
      if (wsLine.find(L"id=\"") == 0) {
        // Add transaction to the revision log
        if (logEntry.revision != 0) {
          m_logs[logEntry.revision] = logEntry;
          if (++entryCount >= MAX_LOG_ENTRIES) {
            logEntry.revision = 0;
            break;
          }
        }

        // Free allocated memory
        if (pIssueNos) {
          issueNoCount = 0;
          free(pIssueNos);
          pIssueNos = NULL;
        }

        // Clear the logEntry object
        logEntry.revision = 0;
        logEntry.author = std::wstring();
        logEntry.date = (apr_time_t)0;
        logEntry.message = std::wstring();
        logEntry.m_changedPaths.clear();

        // Parse out transaction
        wsLine.erase(wsLine.size() - 1, std::wstring::npos);
        wsLine.erase(0, 4);
        logEntry.revision = _wtoi(wsLine.c_str());
      }
      else if (wsLine.find(L"time=\"") == 0) {
        wsLine.erase(wsLine.size() - 1, std::wstring::npos);
        wsLine.erase(0, 6);
        wsLine.append(L"000000");
        logEntry.date = _wtoi64(wsLine.c_str());
        //svn_time_from_cstring (&logEntry.date, wsLine.c_str(), NULL);
      }
      else if (wsLine.find(L"user=\"") == 0) {
        wsLine.erase(wsLine.size() - 2, std::wstring::npos);
        wsLine.erase(0, 6);
        logEntry.author = std::wstring(wsLine);
      }
      else if (wsLine.find(L"<comment>") == 0) {
        // Parse the log entry
        boolean bEndofLogComment = false;

        // Handle multi-line comments
        while (!bEndofLogComment && (szLineToken != NULL))
        {
          std::wstring commentLine(szLineToken);
          commentLine = rtrim (commentLine);

          // Strip XML tag off the front
          if (commentLine.find(L"    <comment>") == 0) {
             commentLine.erase(0, 13);
          }

          // Strip the XML tag off the back
          size_t endTagPos = commentLine.rfind(L"</comment>");

          if (endTagPos != std::wstring::npos) {
            commentLine.erase(endTagPos, std::wstring::npos);
            bEndofLogComment = true;
          }

          // Append the comment line
          logEntry.message.append(commentLine);
          logEntry.message.append(L"\n");

          if (endTagPos == std::wstring::npos) {
            // Skip to the next token
            szLineToken = wcstok_s(NULL, szLineTokens, &szNextLineToken);
            iLineTokenNo++;
          }
        }
      }
      else if (wsLine.find(L"path=\"\\.\\") == 0) {
        // Parse the file line
        wsLine.erase(wsLine.size() - 1, std::wstring::npos);
        wsLine.erase(0, 9);
        wPath = wsLine;
      }
      else if (wsLine.find(L"virtual=\"") == 0) {
        // Parse the file line
        wsLine.erase(wsLine.size() - 1, std::wstring::npos);
        wsLine.erase(0, 9);
        wVirtualVersion = wsLine;
      }
      else if (wsLine.find(L"real=\"") == 0) {
        // Parse the file line
        wsLine.erase(wsLine.size() - 1, std::wstring::npos);
        wsLine.erase(0, 6);
        wRealVersion = wsLine;

        wPath.append(L" ");
        wPath.append(wVirtualVersion);
        wPath.append(L" (");
        wPath.append(wRealVersion);
        wPath.append(L")");

        SCCSLogChangedPaths changedPaths;
        changedPaths.action = L'M';

        logEntry.m_changedPaths[wPath.c_str()] = changedPaths;
      }
      else if (wsLine.find(L"<issueNum>") == 0) {
        // Get Issue information if not already done so for this issue
        wsLine.erase(wsLine.size() - 11, std::wstring::npos);
        wsLine.erase(0, 10);
        long issueNo = _wtoi(wsLine.c_str());

        bIssueRetrieved = FALSE;
        for (int i = 0; i<issueNoCount; i++) {
          if (pIssueNos[i] == issueNo) {
            bIssueRetrieved = TRUE;
            break;
          }
        }
        if (!bIssueRetrieved) {
          // Add the issue to the list of known issues
          pIssueNos = (int *)realloc(pIssueNos, (issueNoCount+1) * sizeof(int));
          pIssueNos[issueNoCount++] = issueNo;

          // Retrieve the JIRA link information
          std::wstring rawIssueListLog;
          if (AccuIssueList(repo, url, issueNo, rawIssueListLog)) {
            issueParser(rawIssueListLog, logEntry);
          }

          bIssueRetrieved = TRUE;
        }
      }

      szLineToken = wcstok_s(NULL, szLineTokens, &szNextLineToken);
      iLineTokenNo++;
    }

    /* There is no need to add the final transation with the XML version
     * because there is an 'id=' item in the XML at the end which the
     * above parsing detects as a new entry, but we never complete it
     * ... a total hack!
     */
#if 0
    // Add final transaction to the revision log
    if (logEntry.revision != 0) {
      m_logs[logEntry.revision] = logEntry;

      // Free allocated memory
      if (pIssueNos) free(pIssueNos);
    }
#else
    // Free allocated memory
    if (pIssueNos) free(pIssueNos);
#endif

    return retVal;
}

bool ACCUREV::issueParser(const std::wstring& rawLog, SCCSLogEntry& logEntry) {
  bool retVal = false;


  // Split into lines
  int iLineTokenNo = 0;
  const LPWSTR szLineTokens = L"\r\n";
  LPWSTR szNextLineToken = NULL;
  LPWSTR szLineToken = wcstok_s((wchar_t *)rawLog.c_str(), szLineTokens, &szNextLineToken);
  std::wstring wsJiraSummary;

  while (szLineToken != NULL)
  {
    std::wstring wsLine(szLineToken);
    wsLine = trim (wsLine);

    // Detect issue fixversion line
    if (wsLine.find(L"fid=\"21\">") == 0) {
      wsLine.erase(wsLine.size() - 13, std::wstring::npos);
      wsLine.erase(0, 9);
      wsJiraSummary = wsLine;
    }
    // Detect issueKey line
    else if (wsLine.find(L"fid=\"34\">") == 0) {
      wsLine.erase(wsLine.size() - 11, std::wstring::npos);
      wsLine.erase(0, 9);
      if (wsJiraSummary.size() == 0) {
        wsJiraSummary = wsLine + L" ";
      }
      else {
        wsJiraSummary = wsLine + L" (" + wsJiraSummary + L") ";
      }
    }
    // Detect issue summary line
    else if (wsLine.find(L"fid=\"4\">") == 0) {
      wsLine.erase(wsLine.size() - 10, std::wstring::npos);
      wsLine.erase(0, 8);
      wsJiraSummary = wsJiraSummary + std::wstring(wsLine) + L"\n";

      // Append the JIRA to the comment log (at the front)
      logEntry.message = wsJiraSummary + logEntry.message;
    }

    szLineToken = wcstok_s(NULL, szLineTokens, &szNextLineToken);
    iLineTokenNo++;
  }

  return retVal;
}

#endif


//////////////////////////////////////////////////////////////////////////
// Accurev access functions
//////////////////////////////////////////////////////////////////////////

// accurev login "chsslr@audatex.com" "password"
bool ACCUREV::AccuLogin(const std::wstring& username, const std::wstring& password)
{
  bool retVal;
  std::wstring stdOut, stdErr;

  // Perform accurev login
  retVal = (ExecuteAccurev(L"login \"" + username + L"\" \"" + password + L"\"", 60, stdOut, stdErr) == 0);

  if (retVal) {
    if (stdErr.size() > 0) retVal = false;
  }

  return retVal;
}

bool ACCUREV::AccuGetLastPromote(const std::wstring& repo, const std::wstring& url, long *pTransactionNo)
{
  bool retVal;
  std::wstring stdOut, stdErr;

  // Perform accurev hist command
  retVal = (ExecuteAccurev(L"hist -p " + repo + L" -s " + url + L" -k promote -t now.1", 60, stdOut, stdErr) == 0);

  if (retVal) {
    retVal = false;

    if ((stdOut.size() > 0) && (stdErr.size() == 0)) {
      // Parse out the transaction number
      int iTokenNo = 0;
      const LPWSTR szTokens = L" ;";
        LPWSTR szNextToken = NULL;
      LPWSTR szToken = wcstok_s((wchar_t *)stdOut.c_str(), szTokens, &szNextToken);

      while (szToken != NULL)
      {
        if (iTokenNo == 1) {
          *pTransactionNo = _wtoi(szToken);
        }

        szToken = wcstok_s(NULL, szTokens, &szNextToken);
        iTokenNo++;

        if (iTokenNo > 1) break;
      }

      retVal = true;
    }

  }

  return retVal;
}

bool ACCUREV::AccuGetHistory(const std::wstring& repo, const std::wstring& url, long startrev, long endrev, std::wstring& rawLog)
{
  bool retVal;
  std::wstring stdErr;

  rawLog.clear();

  // Perform accurev hist command
  wchar_t start[64];
  wchar_t end[64];
  _itow_s(startrev, start, 10);
  _itow_s(endrev, end, 10);
  // Non-xml version: wstring cmd = L"hist -p " + repo + L" -s " + url + L" -k promote -t \"" + start + L"-" + end + L"\"";
  std::wstring cmd = L"hist -p " + repo + L" -s " + url + L" -k promote -fx -t \"" + start + L"-" + end + L"\"";
  retVal = (ExecuteAccurev(cmd, 60, rawLog, stdErr) == 0);

  if (retVal) {
    retVal = false;

    if ((rawLog.size() > 0) && (stdErr.size() == 0)) {
      retVal = true;
    }

  }

  return retVal;
}

bool ACCUREV::AccuIssueList(const std::wstring& repo, const std::wstring& url, long issueNo, std::wstring& rawLog)
{
  UNREFERENCED_PARAMETER(repo);

  bool retVal;
  std::wstring stdErr;

  rawLog.clear();

  // Perform accurev hist command
  wchar_t sIssueNo[64];
  _itow_s(issueNo, sIssueNo, 10);
  std::wstring cmd = L"issuelist -s " + url + L" -fx -I " + sIssueNo;
  retVal = (ExecuteAccurev(cmd, 60, rawLog, stdErr) == 0);

  if (retVal) {
    retVal = false;

    if ((rawLog.size() > 0) && (stdErr.size() == 0)) {
      retVal = true;
    }

  }

  return retVal;
}


#define BUFSIZE 8192
#define PIPESIZE (1024 * 1024 * 2) // 2MB

size_t ACCUREV::ExecuteAccurev(std::wstring Parameters, size_t SecondsToWait, std::wstring& stdOut, std::wstring& stdErr)
{
    std::wstring fullPathToExe = (std::wstring)CRegStdString(_T("Software\\CommitMonitor\\AccurevExe"));
    //size_t iMyCounter = 0, iReturnVal = 0;
    size_t iReturnVal = 0;
    std::wstring sTempStr = L"";
    //int msToWait = SecondsToWait * 1000;

    // Create a job, and add original process to it. Then set JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE so that when
    // the main process is killed, the spawned processes are also killed
    HANDLE jo = CreateJobObject(NULL, NULL);
    AssignProcessToJobObject(jo, GetCurrentProcess());

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject( jo, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));

    /* - NOTE - should check here to see if the exe even exists */
    // TODO!


    std::wstring execCmd = L"\"" + fullPathToExe + L"\" " + Parameters;

    // Build the command line string
    wchar_t * pwszParam = new wchar_t[execCmd.size() + 1];
    const wchar_t* pchrTemp = execCmd.c_str();
    wcscpy_s(pwszParam, execCmd.size() + 1, pchrTemp);


    /* Create STDOUT and STDERR pipes */
    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;
    HANDLE hChildStd_ERR_Rd = NULL;
    HANDLE hChildStd_ERR_Wr = NULL;
    HANDLE hChildStd_IN_Rd = NULL;
    HANDLE hChildStd_IN_Wr = NULL;

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, PIPESIZE);    // Create a pipe for the child process's STDOUT.
    SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);         // Ensure the read handle to the pipe for STDOUT is not inherited.

    CreatePipe(&hChildStd_ERR_Rd, &hChildStd_ERR_Wr, &saAttr, PIPESIZE);    // Create a pipe for the child process's STDOUT.
    SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);         // Ensure the read handle to the pipe for STDOUT is not inherited.

    CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0);
    SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0);

    CloseHandle(hChildStd_IN_Wr);

    /* CreateProcess API initialization */
    STARTUPINFOW siStartupInfo;
    PROCESS_INFORMATION piProcessInfo;
    memset(&siStartupInfo, 0, sizeof(siStartupInfo));
    memset(&piProcessInfo, 0, sizeof(piProcessInfo));
    siStartupInfo.cb = sizeof(siStartupInfo);

    // Make sure the command window starts hidden
    siStartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    siStartupInfo.wShowWindow = SW_HIDE;

    // Configure usage of stdio handles
    siStartupInfo.hStdError = hChildStd_ERR_Wr;
    siStartupInfo.hStdOutput = hChildStd_OUT_Wr;
    siStartupInfo.hStdInput = hChildStd_IN_Rd;
    siStartupInfo.dwFlags |= STARTF_USESTDHANDLES;

    /* TODO: This code is wrong. The thread blocks on the pipe when it is full, so the pipe
     * should be read while the process is running. This would require overlapped I/O and
     * I can't be bothered to do that right now. */

    DWORD dwExitCode = 0;
    if (CreateProcessW(NULL,
                       pwszParam, 0, 0, true,
                       CREATE_DEFAULT_ERROR_MODE, 0, 0,
                       &siStartupInfo, &piProcessInfo) != false)
    {
         /* Watch the process, waiting for completion */
        dwExitCode = WaitForSingleObject(piProcessInfo.hProcess, DWORD(SecondsToWait * 1000));

        // Close the write end of the pipe before reading from the
        // read end of the pipe, to control child process execution.
        // The pipe is assumed to have enough buffer space to hold the
        // data the child process has already written to it.
        CloseHandle(hChildStd_ERR_Wr);
        CloseHandle(hChildStd_OUT_Wr);

        if (dwExitCode == 0) {
          DWORD dwRead;
          CHAR chBuf[BUFSIZE+1];    // +1 for NULL
          //wchar_t wBuf[BUFSIZE+1];
          BOOL bSuccess = FALSE;

          // Read stdOut pipe
          for (;;)
          {
            bSuccess = ReadFile( hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
            if( ! bSuccess || dwRead == 0 ) break;

            // Add to output string
            chBuf[dwRead] = 0; // NULL terminate
            //char2wchar(chBuf, wBuf, dwRead+1);
            //stdOut.append(wBuf, dwRead);
            stdOut.append(CUnicodeUtils::StdGetUnicode(chBuf));
          }

          // Read stdErr pipe
          for (;;)
          {
            bSuccess = ReadFile( hChildStd_ERR_Rd, chBuf, BUFSIZE, &dwRead, NULL);
            if( ! bSuccess || dwRead == 0 ) break;

            // Add to output string
            chBuf[dwRead] = 0; // NULL terminate
            //char2wchar(chBuf, wBuf, dwRead+1);
            //stdErr.append(wBuf, dwRead);
            stdErr.append(CUnicodeUtils::StdGetUnicode(chBuf));
          }
        }
        else {
          iReturnVal = dwExitCode;
        }
    }
    else
    {
        /* CreateProcess failed */
        CloseHandle(hChildStd_ERR_Wr);
        CloseHandle(hChildStd_OUT_Wr);

        iReturnVal = GetLastError();
    }

    /* Free memory */
    delete[]pwszParam;
    pwszParam = 0;

    /* Release handles */
    CloseHandle(piProcessInfo.hProcess);
    CloseHandle(piProcessInfo.hThread);

    /* Release pipe handles */
    CloseHandle(hChildStd_OUT_Rd);
    CloseHandle(hChildStd_ERR_Rd);
    CloseHandle(hChildStd_IN_Rd);

    // Close job
    CloseHandle(jo);

    return iReturnVal;
}

void ACCUREV::ClearErrors() {
  pErrorString = ACCU_NO_ERROR;
}

void ACCUREV::SetError(const wchar_t *pErrorString) {
  this->pErrorString = pErrorString;
}

static inline void char2wchar(char *pChar, wchar_t *pwChar, int length)
{
    char *pchBuf, *chpwBuf;

    pchBuf = pChar;
    chpwBuf = (char *)pwChar;
    for (int i=0; i<length; i++) {
      *chpwBuf++ = *pchBuf++;
      *chpwBuf++ = 0;
    }
}

// replace all in a string
static inline std::wstring& replaceAll(std::wstring& context, const std::wstring& from, const std::wstring& to)
{
    size_t lookHere = 0;
    size_t foundHere;
    while((foundHere = context.find(from, lookHere)) != std::wstring::npos)
    {
          context.replace(foundHere, from.size(), to);
          lookHere = foundHere + to.size();
    }
    return context;
}

// trim from both ends
static inline std::wstring &trim(std::wstring &s) {
    return ltrim(rtrim(s));
}

// trim from start
static inline std::wstring &ltrim(std::wstring &s) {
    size_t sSize = s.size();
    size_t i = 0;

    while (isspace(s[i]) && (i < sSize)) i++;
    if ((i > 0) && (i < sSize)) s.erase(0, i);
    return s;
}

// trim from end
static inline std::wstring &rtrim(std::wstring &s) {
    size_t sSize = s.size();
    if (sSize == 0)
        return s;

    size_t i = sSize-1;

    while (isspace(s[i]) && (i >= 0)) i--;
    if ((i > 0) && (i < sSize-1)) s.erase(i+1, std::wstring::npos);

    return s;
}
