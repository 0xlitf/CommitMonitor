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
#include "resource.h"
#include <algorithm>
#include "URLDlg.h"
#include "StringUtils.h"

#include "SVN.h"
#include <cctype>
#include <regex>

CURLDlg::CURLDlg(void)
{
    sSCCS[0] = _T("SVN");
    sSCCS[1] = _T("Accurev");
}

CURLDlg::~CURLDlg(void)
{
}

void CURLDlg::SetInfo(const CUrlInfo * pURLInfo /* = NULL */)
{
    if (pURLInfo == NULL)
        return;
    info = *pURLInfo;
}

void CURLDlg::ClearForTemplate()
{
    info.name.clear();
    info.logentries.clear();
    info.error.clear();
    info.errNr = 0;
    info.lastchecked = 0;
    info.lastcheckedrev = 0;
    info.lastcheckedrobots = 0;

}

void CURLDlg::SetSCCS(CUrlInfo::SCCS_TYPE sccs)
{
    // SCCS specific initialization
    switch (sccs)
    {
    default:
    case CUrlInfo::SCCS_SVN:
        AddToolTip(IDC_URLTOMONITOR, _T("URL to the repository, or the SVNParentPath URL"));
        SetDlgItemText(*this, IDC_REPOLABEL, _T(""));
        SetDlgItemText(*this, IDC_URLTOMONITORLABEL, _T("URL to monitor"));
        SetDlgItemText(*this, IDC_URLGROUP, _T("SVN repository settings"));
        ShowWindow(GetDlgItem(*this, IDC_ACCUREVREPO), SW_HIDE);
        SendMessage(GetDlgItem(*this, IDC_SCCSCOMBO), CB_SETCURSEL, (WPARAM)sccs, 0);
        break;

    case CUrlInfo::SCCS_ACCUREV:
        AddToolTip(IDC_URLTOMONITOR, _T("Accurev stream name"));
        SetDlgItemText(*this, IDC_REPOLABEL, _T("Accurev repository"));
        SetDlgItemText(*this, IDC_URLTOMONITORLABEL, _T("Accurev stream"));
        SetDlgItemText(*this, IDC_URLGROUP, _T("Accurev repository settings"));
        ShowWindow(GetDlgItem(*this, IDC_ACCUREVREPO), SW_SHOW);
        SendMessage(GetDlgItem(*this, IDC_SCCSCOMBO), CB_SETCURSEL, (WPARAM)sccs, 1);
        break;
    }
}

LRESULT CURLDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            InitDialog(hwndDlg, IDI_COMMITMONITOR);

            // Default SCCS to SVN if it is not set
            if (info.sccs < 0 || info.sccs >= CUrlInfo::SCCS_LEN)
            {
                info.sccs = CUrlInfo::SCCS_SVN;
            }

            // SCCS Generic initialization
            AddToolTip(IDC_CREATEDIFFS, _T("Fetches the diff for each revision automatically\nPlease do NOT enable this for repositories which are not on your LAN!"));
            AddToolTip(IDC_PROJECTNAME, _T("Enter here a name for the project"));
            AddToolTip(IDC_URLTOMONITOR, _T("URL to the repository, or the SVNParentPath URL"));
            AddToolTip(IDC_SCCSCOMBO, _T("Source code control system to use"));
            AddToolTip(IDC_ACCUREVREPO, _T("Accurev repository name"));
            AddToolTip(IDC_IGNORESELF, _T("If enabled, commits from you won't show a notification"));
            AddToolTip(IDC_SCRIPT, _T("enter here a command which gets called after new revisions were detected.\n\n%revision gets replaced with the new HEAD revision\n%url gets replaced with the url of the project\n%project gets replaced with the project name\n%username gets replaced with a list of usernames\n\nExample command line:\nTortoiseProc.exe /command:update /rev:%revision /path:\"path\\to\\working\\copy\""));
            AddToolTip(IDC_WEBDIFF, _T("URL to a web viewer\n%revision gets replaced with the new HEAD revision\n%url gets replaced with the url of the project\n%project gets replaced with the project name"));
            AddToolTip(IDC_IGNOREUSERS, _T("Newline separated list of usernames to ignore"));
            AddToolTip(IDC_INCLUDEUSERS, _T("Newline separated list of users to monitor"));

            if (info.minminutesinterval)
            {
                TCHAR infobuf[MAX_PATH] = {0};
                _stprintf_s(infobuf, _countof(infobuf), _T("Interval for repository update checks.\nMiminum set by svnrobots.txt file to %ld minutes."), info.minminutesinterval);
                AddToolTip(IDC_CHECKTIME, infobuf);
            }
            else
                AddToolTip(IDC_CHECKTIME, _T("Interval for repository update checks"));

            // initialize the controls
            SetDlgItemText(*this, IDC_ACCUREVREPO, info.accurevRepo.c_str());
            SetDlgItemText(*this, IDC_URLTOMONITOR, info.url.c_str());
            WCHAR buf[20];
            _stprintf_s(buf, _countof(buf), _T("%ld"), max(info.minutesinterval, info.minminutesinterval));
            SetDlgItemText(*this, IDC_CHECKTIME, buf);
            SetDlgItemText(*this, IDC_PROJECTNAME, info.name.c_str());
            SetDlgItemText(*this, IDC_USERNAME, info.username.c_str());
            SetDlgItemText(*this, IDC_PASSWORD, info.password.c_str());
            SendMessage(GetDlgItem(*this, IDC_CREATEDIFFS), BM_SETCHECK, info.fetchdiffs ? BST_CHECKED : BST_UNCHECKED, NULL);
            if (info.disallowdiffs)
                EnableWindow(GetDlgItem(*this, IDC_CREATEDIFFS), FALSE);
            SetDlgItemText(*this, IDC_IGNOREUSERS, info.ignoreUsers.c_str());
            SetDlgItemText(*this, IDC_INCLUDEUSERS, info.includeUsers.c_str());
            _stprintf_s(buf, _countof(buf), _T("%ld"), min(URLINFO_MAXENTRIES, info.maxentries));
            SetDlgItemText(*this, IDC_MAXLOGENTRIES, buf);
            SetDlgItemText(*this, IDC_SCRIPT, info.callcommand.c_str());
            SetDlgItemText(*this, IDC_WEBDIFF, info.webviewer.c_str());
            SendMessage(GetDlgItem(*this, IDC_EXECUTEIGNORED), BM_SETCHECK, info.noexecuteignored ? BST_CHECKED : BST_UNCHECKED, NULL);

            ExtendFrameIntoClientArea(0, 0, 0, IDC_URLGROUP);
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDOK));
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDCANCEL));

            // SCCS specific initialization

            // Fill combobox
            for (int i=0;i<CUrlInfo::SCCS_LEN;i++)
            {
                SendMessage(GetDlgItem(*this, IDC_SCCSCOMBO), CB_ADDSTRING,
                    0, (LPARAM)sSCCS[i].c_str());
            }
            SetSCCS(info.sccs);
        }
        return TRUE;
    case WM_COMMAND:
        return DoCommand(LOWORD(wParam), HIWORD(wParam));
    case WM_NOTIFY:
        {
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

LRESULT CURLDlg::DoCommand(int id, int cmd)
{
    switch (id)
    {
    case IDOK:
        {
            SVN svn;
            std::wstring tempurl;

            info.sccs = (CUrlInfo::SCCS_TYPE)SendMessage(GetDlgItem(*this, IDC_SCCSCOMBO), CB_GETCURSEL, 0, 0);

            switch (info.sccs)
            {
            default:
            case CUrlInfo::SCCS_SVN:
                {
                    auto buffer = GetDlgItemText(IDC_URLTOMONITOR);
                    info.url = svn.CanonicalizeURL(std::wstring(buffer.get()));
                    CStringUtils::trim(info.url);

                    tempurl = info.url.substr(0, 7);
                    std::transform(tempurl.begin(), tempurl.end(), tempurl.begin(), std::tolower);

                    if (tempurl.compare(_T("file://")) == 0)
                    {
                        ::MessageBox(*this, _T("file:/// urls are not supported!"), _T("CommitMonitor"), MB_ICONERROR);
                        return 1;
                    }
                }
                break;

            case CUrlInfo::SCCS_ACCUREV:
                {
                    auto buffer = GetDlgItemText(IDC_URLTOMONITOR);
                    info.url = std::wstring(buffer.get());
                    CStringUtils::trim(info.url);

                    buffer = GetDlgItemText(IDC_ACCUREVREPO);
                    info.accurevRepo = std::wstring(buffer.get());
                    CStringUtils::trim(info.accurevRepo);
                }
                break;
            }

            auto buffer = GetDlgItemText(IDC_PROJECTNAME);
            info.name = std::wstring(buffer.get());
            CStringUtils::trim(info.name);
            if (info.name.size()==0)
            {
                EDITBALLOONTIP ebt = {0};
                ebt.cbStruct = sizeof(EDITBALLOONTIP);
                ebt.pszTitle = _T("Project name");
                ebt.pszText = _T("You must provide a name for the project!");
                ebt.ttiIcon = TTI_ERROR;
                if (!::SendMessage(GetDlgItem(*this, IDC_PROJECTNAME), EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt))
                {
                    ::MessageBox(*this, _T("You must provide a name for the project!"), _T("Project name"), MB_ICONERROR);
                }
                return 0;
            }

            buffer = GetDlgItemText(IDC_CHECKTIME);
            info.minutesinterval = _ttoi(buffer.get());
            if ((info.minminutesinterval)&&(info.minminutesinterval > info.minutesinterval))
                info.minutesinterval = info.minminutesinterval;

            buffer = GetDlgItemText(IDC_USERNAME);
            info.username = std::wstring(buffer.get());
            CStringUtils::trim(info.username);

            buffer = GetDlgItemText(IDC_PASSWORD);
            info.password = std::wstring(buffer.get());
            info.fetchdiffs = (SendMessage(GetDlgItem(*this, IDC_CREATEDIFFS), BM_GETCHECK, 0, 0) == BST_CHECKED);

            buffer = GetDlgItemText(IDC_MAXLOGENTRIES);
            if (_ttoi(buffer.get()) > URLINFO_MAXENTRIES)
            {
                EDITBALLOONTIP ebt = {0};
                ebt.cbStruct = sizeof(EDITBALLOONTIP);
                ebt.pszTitle = _T("Invalid value!");
                ebt.pszText = _T("The value for the maximum number of log entries to keep\nmust be between 0 and 100000");
                ebt.ttiIcon = TTI_ERROR;
                if (!::SendMessage(GetDlgItem(*this, IDC_MAXLOGENTRIES), EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt))
                {
                    ::MessageBox(*this, _T("The value must be between 0 and 100000"), _T("Invalid Value!"), MB_ICONERROR);
                }
                return 0;
            }
            info.maxentries = _ttoi(buffer.get());
            info.maxentries = min(URLINFO_MAXENTRIES, info.maxentries);
            info.maxentries = max(10, info.maxentries);

            buffer = GetDlgItemText(IDC_IGNOREUSERS);
            info.ignoreUsers = std::wstring(buffer.get());
            CStringUtils::trim(info.ignoreUsers);

            buffer = GetDlgItemText(IDC_INCLUDEUSERS);
            info.includeUsers = std::wstring(buffer.get());
            CStringUtils::trim(info.includeUsers);

            buffer = GetDlgItemText(IDC_SCRIPT);
            info.callcommand = std::wstring(buffer.get());
            CStringUtils::trim(info.callcommand);

            buffer = GetDlgItemText(IDC_WEBDIFF);
            info.webviewer = std::wstring(buffer.get());
            CStringUtils::trim(info.webviewer);

            info.noexecuteignored = !!SendMessage(GetDlgItem(*this, IDC_EXECUTEIGNORED), BM_GETCHECK, 0, NULL);

            // make sure this entry gets checked again as soon as the next timer fires
            info.lastchecked = 0;
        }
        // fall through
    case IDCANCEL:
        EndDialog(*this, id);
        break;

    case IDC_SCCSCOMBO:
        switch(cmd) // Find out what message it was
        {
        case CBN_SELCHANGE: // This means that list item has changed
            info.sccs = (CUrlInfo::SCCS_TYPE)SendMessage(GetDlgItem(*this, IDC_SCCSCOMBO), CB_GETCURSEL, 0, 0);
            SetSCCS(info.sccs);
            break;
        }
        break;

    }

    return 1;
}
