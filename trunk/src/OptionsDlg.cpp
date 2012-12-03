// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007-2012 - Stefan Kueng

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
#include "OptionsDlg.h"
#include "PasswordDlg.h"
#include "Registry.h"
#include "AppUtils.h"
#include <string>
#include <Commdlg.h>


COptionsDlg::COptionsDlg(HWND hParent)
    : m_pURLInfos(NULL)
    , m_hHiddenWnd(NULL)
	, m_hParent(hParent)
{
}

COptionsDlg::~COptionsDlg(void)
{
}

LRESULT COptionsDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            InitDialog(hwndDlg, IDI_COMMITMONITOR);

            AddToolTip(IDC_AUTOSTART, _T("Starts the CommitMonitor automatically when Windows starts up."));
            AddToolTip(IDC_TASKBAR_ALWAYSON, _T("If disabled, the taskbar icon is only shown if new commits are available.\nThe CommitMonitor can be shown by 'starting' it again."));
            AddToolTip(IDC_DIFFVIEWER, _T("Path to a viewer for unified diff files."));
            AddToolTip(IDC_DIFFVIEWERLABEL, _T("Path to a viewer for unified diff files."));
            AddToolTip(IDC_ACCUEXELOCATION, _T("Path to accurev executable."));
            AddToolTip(IDC_ACCUEXELOCATIONLABEL, _T("Path to accurev executable."));
            AddToolTip(IDC_ACCUDIFFCMD, _T("Path to a viewer for accurev diffs."));
            AddToolTip(IDC_ACCUDIFFCMDLABEL, _T("Path to a viewer for accurev diffs."));


            AddToolTip(IDC_ANIMATEICON, _T("Animates the system tray icon as long as there are unread commits"));
            AddToolTip(IDC_USETSVN, _T("If TortoiseSVN is installed, use it for showing the differences of commits"));
            AddToolTip(IDC_CHECKNEWER, _T("Automatically check for newer versions of CommitMonitor"));
            AddToolTip(IDC_NOTIFYCONNECTERROR, _T("When a repository can not be checked due to connection problems,\nchange/animate the icon as if new commits were available."));
            AddToolTip(IDC_IGNOREEOL, _T("Ignores end-of-line changes"));
            AddToolTip(IDC_IGNORESPACES, _T("Ignores changes in whitespaces in the middle of lines"));
            AddToolTip(IDC_IGNOREALLSPACES, _T("Ignores all whitespace changes"));
            AddToolTip(IDC_SHOWPOPUPS, L"Shows new commits with a popup window");

            // initialize the controls
            bool bShowTaskbarIcon = !!(DWORD)CRegStdDWORD(_T("Software\\CommitMonitor\\TaskBarIcon"), TRUE);
            bool bStartWithWindows = !std::wstring((LPCTSTR)CRegStdString(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run\\CommitMonitor"))).empty();
            bool bAnimateIcon = !!CRegStdDWORD(_T("Software\\CommitMonitor\\Animate"), TRUE);
            bool bPlaySound = !!CRegStdDWORD(_T("Software\\CommitMonitor\\PlaySound"), TRUE);
            bool bUseTSVN = !!CRegStdDWORD(_T("Software\\CommitMonitor\\UseTSVN"), TRUE);
            bool bIndicateConnectErrors = !!CRegStdDWORD(_T("Software\\CommitMonitor\\IndicateConnectErrors"), TRUE);
            bool bLeftMenu = !!CRegStdDWORD(_T("Software\\CommitMonitor\\LeftClickMenu"), FALSE);
            bool bLastUnread = !!CRegStdDWORD(_T("Software\\CommitMonitor\\ShowLastUnread"), FALSE);
            bool bWebViewer = !!CRegStdDWORD(_T("Software\\CommitMonitor\\DblClickWebViewer"), FALSE);
            bool bShowPopups = !!CRegStdDWORD(L"Software\\CommitMonitor\\ShowPopups", TRUE);

            CRegStdString diffViewer = CRegStdString(_T("Software\\CommitMonitor\\DiffViewer"));
            CRegStdString accurevExe = CRegStdString(_T("Software\\CommitMonitor\\AccurevExe"));
            CRegStdString accurevDiffCmd = CRegStdString(_T("Software\\CommitMonitor\\AccurevDiffCmd"));
            CRegStdString notifySound = CRegStdString(_T("Software\\CommitMonitor\\NotificationSound"));
            CRegStdDWORD updatecheck = CRegStdDWORD(_T("Software\\CommitMonitor\\CheckNewer"), FALSE);
            CRegStdDWORD numlogs = CRegStdDWORD(_T("Software\\CommitMonitor\\NumLogs"), 30);
            TCHAR numBuf[30] = {0};
            _stprintf_s(numBuf, 30, _T("%ld"), DWORD(numlogs));
            SendDlgItemMessage(*this, IDC_TASKBAR_ALWAYSON, BM_SETCHECK, bShowTaskbarIcon ? BST_CHECKED : BST_UNCHECKED, NULL);
            SendDlgItemMessage(*this, IDC_AUTOSTART, BM_SETCHECK, bStartWithWindows ? BST_CHECKED : BST_UNCHECKED, NULL);
            SendDlgItemMessage(*this, IDC_ANIMATEICON, BM_SETCHECK, bAnimateIcon ? BST_CHECKED : BST_UNCHECKED, NULL);
            SendDlgItemMessage(*this, IDC_USETSVN, BM_SETCHECK, bUseTSVN ? BST_CHECKED : BST_UNCHECKED, NULL);
            SetDlgItemText(*this, IDC_DIFFVIEWER, std::wstring(diffViewer).c_str());

            SetDlgItemText(*this, IDC_ACCUEXELOCATION, std::wstring(accurevExe).c_str());
            SetDlgItemText(*this, IDC_ACCUDIFFCMD, std::wstring(accurevDiffCmd).c_str());

            SetDlgItemText(*this, IDC_NOTIFICATIONSOUNDPATH, std::wstring(notifySound).c_str());
            SendDlgItemMessage(*this, IDC_NOTIFICATIONSOUND, BM_SETCHECK, bPlaySound ? BST_CHECKED : BST_UNCHECKED, NULL);
            SendDlgItemMessage(*this, IDC_CHECKNEWER, BM_SETCHECK, DWORD(updatecheck) ? BST_CHECKED : BST_UNCHECKED, NULL);
            SendDlgItemMessage(*this, IDC_NOTIFYCONNECTERROR, BM_SETCHECK, bIndicateConnectErrors ? BST_CHECKED : BST_UNCHECKED, NULL);
            SendDlgItemMessage(*this, IDC_LEFTMENU, BM_SETCHECK, bLeftMenu ? BST_CHECKED : BST_UNCHECKED, NULL);
            SendDlgItemMessage(*this, IDC_SHOWLASTUNREAD, BM_SETCHECK, bLastUnread ? BST_CHECKED : BST_UNCHECKED, NULL);
            SendDlgItemMessage(*this, IDC_WEBVIEWER, BM_SETCHECK, bWebViewer ? BST_CHECKED : BST_UNCHECKED, NULL);
            SendDlgItemMessage(*this, IDC_SHOWPOPUPS, BM_SETCHECK, bShowPopups ? BST_CHECKED : BST_UNCHECKED, NULL);

            std::wstring tsvninstalled = CAppUtils::GetTSVNPath();
            std::wstring sVer = CAppUtils::GetVersionStringFromExe(tsvninstalled.c_str());
            if (tsvninstalled.empty() || (_tstoi(sVer.substr(3, 4).c_str()) < 5))
                DialogEnableWindow(IDC_USETSVN, FALSE);
            SetDlgItemText(*this, IDC_NUMLOGS, numBuf);

            CRegStdString diffParams = CRegStdString(_T("Software\\CommitMonitor\\DiffParameters"));
            bool ignoreeol = std::wstring(diffParams).find(_T("--ignore-eol-style")) != std::wstring::npos;
            bool ignorewhitespaces = std::wstring(diffParams).find(_T("-b")) != std::wstring::npos;
            bool ignoreallwhitespaces = std::wstring(diffParams).find(_T("-w")) != std::wstring::npos;
            SendDlgItemMessage(*this, IDC_IGNOREEOL, BM_SETCHECK, ignoreeol ? BST_CHECKED : BST_UNCHECKED, NULL);
            SendDlgItemMessage(*this, IDC_IGNORESPACES, BM_SETCHECK, ignorewhitespaces ? BST_CHECKED : BST_UNCHECKED, NULL);
            SendDlgItemMessage(*this, IDC_IGNOREALLSPACES, BM_SETCHECK, ignoreallwhitespaces ? BST_CHECKED : BST_UNCHECKED, NULL);

            ExtendFrameIntoClientArea(0, 0, 0, IDC_AEROLABEL);
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDOK));
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDCANCEL));
        }
        return TRUE;
    case WM_COMMAND:
        return DoCommand(LOWORD(wParam));
    default:
        return FALSE;
    }
}

LRESULT COptionsDlg::DoCommand(int id)
{
    switch (id)
    {
    case IDOK:
        {
            CRegStdDWORD regShowTaskbarIcon = CRegStdDWORD(_T("Software\\CommitMonitor\\TaskBarIcon"), TRUE);
            CRegStdString regStartWithWindows = CRegStdString(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run\\CommitMonitor"));
            CRegStdDWORD regAnimateIcon = CRegStdDWORD(_T("Software\\CommitMonitor\\Animate"), TRUE);
            CRegStdDWORD regPlaySound = CRegStdDWORD(_T("Software\\CommitMonitor\\PlaySound"), TRUE);
            CRegStdDWORD regUseTSVN = CRegStdDWORD(_T("Software\\CommitMonitor\\UseTSVN"), TRUE);
            CRegStdDWORD updatecheck = CRegStdDWORD(_T("Software\\CommitMonitor\\CheckNewer"), FALSE);
            CRegStdDWORD numlogs = CRegStdDWORD(_T("Software\\CommitMonitor\\NumLogs"), 30);
            CRegStdDWORD regIndicateErrors = CRegStdDWORD(_T("Software\\CommitMonitor\\IndicateConnectErrors"), TRUE);
            CRegStdDWORD regLeftMenu = CRegStdDWORD(_T("Software\\CommitMonitor\\LeftClickMenu"), FALSE);
            CRegStdDWORD regLastUnread = CRegStdDWORD(_T("Software\\CommitMonitor\\ShowLastUnread"), FALSE);
            CRegStdDWORD regWebViewer = CRegStdDWORD(_T("Software\\CommitMonitor\\DblClickWebViewer"), FALSE);
            CRegStdDWORD regShowPopups = CRegStdDWORD(_T("Software\\CommitMonitor\\ShowPopups"), TRUE);

            bool bShowTaskbarIcon = !!SendDlgItemMessage(*this, IDC_TASKBAR_ALWAYSON, BM_GETCHECK, 0, NULL);
            bool bStartWithWindows = !!SendDlgItemMessage(*this, IDC_AUTOSTART, BM_GETCHECK, 0, NULL);
            bool bAnimateIcon = !!SendDlgItemMessage(*this, IDC_ANIMATEICON, BM_GETCHECK, 0, NULL);
            bool bPlaySound = !!SendDlgItemMessage(*this, IDC_NOTIFICATIONSOUND, BM_GETCHECK, 0, NULL);
            bool bUseTSVN = !!SendDlgItemMessage(*this, IDC_USETSVN, BM_GETCHECK, 0, NULL);
            bool bUpdateCheck = !!SendDlgItemMessage(*this, IDC_CHECKNEWER, BM_GETCHECK, 0, NULL);
            bool bIndicateConnectErrors = !!SendDlgItemMessage(*this, IDC_NOTIFYCONNECTERROR, BM_GETCHECK, 0, NULL);
            bool bLeftMenu = !!SendDlgItemMessage(*this, IDC_LEFTMENU, BM_GETCHECK, 0, NULL);
            bool bLastUnread = !!SendDlgItemMessage(*this, IDC_SHOWLASTUNREAD, BM_GETCHECK, 0, NULL);
            bool bWebViewer = !!SendDlgItemMessage(*this, IDC_WEBVIEWER, BM_GETCHECK, 0, NULL);
            bool bShowPopups = !!SendDlgItemMessage(*this, IDC_SHOWPOPUPS, BM_GETCHECK, 0, NULL);
            regShowTaskbarIcon = bShowTaskbarIcon;
            regAnimateIcon = bAnimateIcon;
            regPlaySound = bPlaySound;
            regUseTSVN = bUseTSVN;
            updatecheck = bUpdateCheck;
            regIndicateErrors = bIndicateConnectErrors;
            regLeftMenu = bLeftMenu;
            regLastUnread = bLastUnread;
            regWebViewer = bWebViewer;
            regShowPopups = bShowPopups;
            ::SendMessage(m_hHiddenWnd, COMMITMONITOR_CHANGEDINFO, 0, 0);
            if (bStartWithWindows)
            {
                TCHAR buf[MAX_PATH*4];
                GetModuleFileName(NULL, buf, MAX_PATH*4);
                std::wstring cmd = std::wstring(buf);
                cmd += _T(" /hidden");
                regStartWithWindows = cmd;
            }
            else
                regStartWithWindows.removeValue();

            int len = ::GetWindowTextLength(GetDlgItem(*this, IDC_DIFFVIEWER));
            std::unique_ptr<TCHAR[]> divi(new TCHAR[len+1]);
            ::GetDlgItemText(*this, IDC_DIFFVIEWER, divi.get(), len+1);
            std::wstring dv = std::wstring(divi.get());
            CRegStdString diffViewer = CRegStdString(_T("Software\\CommitMonitor\\DiffViewer"));
            if (!dv.empty())
                diffViewer = dv;
            else
                diffViewer.removeValue();

            // RA Sewell
            len = ::GetWindowTextLength(GetDlgItem(*this, IDC_ACCUEXELOCATION));
            divi = std::unique_ptr<WCHAR[]>(new TCHAR[len+1]);
            ::GetDlgItemText(*this, IDC_ACCUEXELOCATION, divi.get(), len+1);
            dv = std::wstring(divi.get());
            CRegStdString accurevExe = CRegStdString(_T("Software\\CommitMonitor\\AccurevExe"));
            if (!dv.empty())
                accurevExe = dv;
            else
                accurevExe.removeValue();

            len = ::GetWindowTextLength(GetDlgItem(*this, IDC_ACCUDIFFCMD));
            divi = std::unique_ptr<WCHAR[]>(new TCHAR[len+1]);
            ::GetDlgItemText(*this, IDC_ACCUDIFFCMD, divi.get(), len+1);
            dv = std::wstring(divi.get());
            CRegStdString accurevDiffCmd = CRegStdString(_T("Software\\CommitMonitor\\AccurevDiffCmd"));
            if (!dv.empty())
                accurevDiffCmd = dv;
            else
                accurevDiffCmd.removeValue();



            len = ::GetWindowTextLength(GetDlgItem(*this, IDC_NOTIFICATIONSOUNDPATH));
            divi = std::unique_ptr<WCHAR[]>(new TCHAR[len+1]);
            ::GetDlgItemText(*this, IDC_NOTIFICATIONSOUNDPATH, divi.get(), len+1);
            std::wstring ns = std::wstring(divi.get());
            CRegStdString notifySound = CRegStdString(_T("Software\\CommitMonitor\\NotificationSound"));
            if (!ns.empty())
                notifySound = ns;
            else
                notifySound.removeValue();

            len = ::GetWindowTextLength(GetDlgItem(*this, IDC_NUMLOGS));
            divi = std::unique_ptr<WCHAR[]>(new TCHAR[len+1]);
            ::GetDlgItemText(*this, IDC_NUMLOGS, divi.get(), len+1);
            DWORD nLogs = _ttol(divi.get());
            numlogs = nLogs;

            CRegStdString diffParams = CRegStdString(_T("Software\\CommitMonitor\\DiffParameters"));
            bool ignoreeol = !!SendDlgItemMessage(*this, IDC_IGNOREEOL, BM_GETCHECK, 0, NULL);
            bool ignorewhitespaces = !!SendDlgItemMessage(*this, IDC_IGNORESPACES, BM_GETCHECK, 0, NULL);
            bool ignoreallwhitespaces = !!SendDlgItemMessage(*this, IDC_IGNOREALLSPACES, BM_GETCHECK, 0, NULL);
            diffParams = SVN::GetOptionsString(ignoreeol, ignorewhitespaces, ignoreallwhitespaces);
        }
        // fall through
    case IDCANCEL:
        EndDialog(*this, id);
        break;
    case IDC_IGNOREALLSPACES:
        {
            bool ignoreallwhitespaces = !!SendDlgItemMessage(*this, IDC_IGNOREALLSPACES, BM_GETCHECK, 0, NULL);
            if (ignoreallwhitespaces)
                SendDlgItemMessage(*this, IDC_IGNORESPACES, BM_SETCHECK, BST_UNCHECKED, NULL);
        }
        break;
    case IDC_IGNORESPACES:
        {
            bool ignorewhitespaces = !!SendDlgItemMessage(*this, IDC_IGNORESPACES, BM_GETCHECK, 0, NULL);
            if (ignorewhitespaces)
                SendDlgItemMessage(*this, IDC_IGNOREALLSPACES, BM_SETCHECK, BST_UNCHECKED, NULL);
        }
        break;
    case IDC_EXPORT:
        {
            if (m_pURLInfos)
            {
                OPENFILENAME ofn = {0};     // common dialog box structure
                TCHAR szFile[MAX_PATH] = {0};  // buffer for file name
                // Initialize OPENFILENAME
                ofn.lStructSize = sizeof(OPENFILENAME);
                ofn.hwndOwner = *this;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = _countof(szFile);
                ofn.lpstrTitle = _T("Export monitored projects to...\0");
                ofn.Flags = OFN_HIDEREADONLY|OFN_DONTADDTORECENT|OFN_EXPLORER|OFN_ENABLESIZING|OFN_OVERWRITEPROMPT;
                ofn.lpstrFilter = _T("CommitMonitor Projects\0*.cmprj;*.com\0All files\0*.*\0\0");
                ofn.nFilterIndex = 1;
                // Display the Open dialog box.
                if (GetSaveFileName(&ofn)==TRUE)
                {
                    if (wcscmp(&szFile[wcslen(szFile)-6], L".cmprj"))
                        wcscat_s(szFile, MAX_PATH, L".cmprj");
                    CPasswordDlg dlg(*this);
                    INT_PTR ret = dlg.DoModal(hResource, IDD_PASSWORD, *this);
                    if (ret == IDOK)
                        m_pURLInfos->Export(szFile, dlg.password.c_str());
                }
            }
        }
        break;
    case IDC_IMPORT:
        {
            if (m_pURLInfos)
            {
                OPENFILENAME ofn = {0};     // common dialog box structure
                TCHAR szFile[MAX_PATH] = {0};  // buffer for file name
                // Initialize OPENFILENAME
                ofn.lStructSize = sizeof(OPENFILENAME);
                ofn.hwndOwner = *this;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = _countof(szFile);
                ofn.lpstrTitle = _T("Import monitored projects from...\0");
                ofn.Flags = OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_DONTADDTORECENT;
                ofn.lpstrFilter = _T("CommitMonitor Projects\0*.cmprj;*.com\0All files\0*.*\0\0");
                ofn.nFilterIndex = 1;
                // Display the Open dialog box.
                if (GetOpenFileName(&ofn)==TRUE)
                {
                    CPasswordDlg dlg(*this);
                    INT_PTR ret = dlg.DoModal(hResource, IDD_PASSWORD, *this);
                    if (ret == IDOK)
                        m_pURLInfos->Import(szFile, dlg.password.c_str());
                }
            }
        }
        break;
    case IDC_DIFFBROWSE:
        {
            OPENFILENAME ofn = {0};     // common dialog box structure
            TCHAR szFile[MAX_PATH] = {0};  // buffer for file name
            // Initialize OPENFILENAME
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = *this;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = _countof(szFile);
            ofn.lpstrTitle = _T("Select Diff Viewer...\0");
            ofn.Flags = OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_DONTADDTORECENT;
            ofn.lpstrFilter = _T("Programs\0*.exe;*.com\0All files\0*.*\0\0");
            ofn.nFilterIndex = 1;
            // Display the Open dialog box.
            if (GetOpenFileName(&ofn)==TRUE)
            {
                SetDlgItemText(*this, IDC_DIFFVIEWER, szFile);
            }
        }
        break;
    case IDC_SOUNDBROWSE:
        {
            OPENFILENAME ofn = {0};     // common dialog box structure
            TCHAR szFile[MAX_PATH] = {0};  // buffer for file name
            // Initialize OPENFILENAME
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = *this;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = _countof(szFile);
            ofn.lpstrTitle = _T("Select Notification Sound...\0");
            ofn.Flags = OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_DONTADDTORECENT;
            ofn.lpstrFilter = _T("Sound Files\0*.wav;*.mp3\0All files\0*.*\0\0");
            ofn.nFilterIndex = 1;
            // Display the Open dialog box.
            if (GetOpenFileName(&ofn)==TRUE)
            {
                SetDlgItemText(*this, IDC_NOTIFICATIONSOUNDPATH, szFile);
            }
        }
        break;
    }
    return 1;
}
