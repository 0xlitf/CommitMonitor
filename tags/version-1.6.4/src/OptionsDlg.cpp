// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007-2009 - Stefan Kueng

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
#include "StdAfx.h"
#include "Resource.h"
#include "OptionsDlg.h"
#include "PasswordDlg.h"
#include "Registry.h"
#include "AppUtils.h"
#include <string>
#include <Commdlg.h>

using namespace std;

COptionsDlg::COptionsDlg(HWND hParent) : m_pURLInfos(NULL)
{
	m_hParent = hParent;
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
			AddToolTip(IDC_ANIMATEICON, _T("Animates the system tray icon as long as there are unread commits"));
			AddToolTip(IDC_USETSVN, _T("If TortoiseSVN is installed, use it for showing the differences of commits"));
			AddToolTip(IDC_CHECKNEWER, _T("Automatically check for newer versions of CommitMonitor"));
			AddToolTip(IDC_NOTIFYCONNECTERROR, _T("When a repository can not be checked due to connection problems,\nchange/animate the icon as if new commits were available."));

			// initialize the controls
			bool bShowTaskbarIcon = !!(DWORD)CRegStdDWORD(_T("Software\\CommitMonitor\\TaskBarIcon"), TRUE);
			bool bStartWithWindows = !wstring(CRegStdString(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run\\CommitMonitor"))).empty();
			bool bAnimateIcon = !!CRegStdDWORD(_T("Software\\CommitMonitor\\Animate"), TRUE);
			bool bPlaySound = !!CRegStdDWORD(_T("Software\\CommitMonitor\\PlaySound"), TRUE);
			bool bUseTSVN = !!CRegStdDWORD(_T("Software\\CommitMonitor\\UseTSVN"), TRUE);
			bool bIndicateConnectErrors = !!CRegStdDWORD(_T("Software\\CommitMonitor\\IndicateConnectErrors"), TRUE);
			bool bLeftMenu = !!CRegStdDWORD(_T("Software\\CommitMonitor\\LeftClickMenu"), FALSE);
			CRegStdString diffViewer = CRegStdString(_T("Software\\CommitMonitor\\DiffViewer"));
			CRegStdString notifySound = CRegStdString(_T("Software\\CommitMonitor\\NotificationSound"));
			CRegStdDWORD updatecheck = CRegStdDWORD(_T("Software\\CommitMonitor\\CheckNewer"), TRUE);
			CRegStdDWORD numlogs = CRegStdDWORD(_T("Software\\CommitMonitor\\NumLogs"), 30);
			TCHAR numBuf[30] = {0};
			_stprintf_s(numBuf, 30, _T("%ld"), DWORD(numlogs));
			SendDlgItemMessage(*this, IDC_TASKBAR_ALWAYSON, BM_SETCHECK, bShowTaskbarIcon ? BST_CHECKED : BST_UNCHECKED, NULL);
			SendDlgItemMessage(*this, IDC_AUTOSTART, BM_SETCHECK, bStartWithWindows ? BST_CHECKED : BST_UNCHECKED, NULL);
			SendDlgItemMessage(*this, IDC_ANIMATEICON, BM_SETCHECK, bAnimateIcon ? BST_CHECKED : BST_UNCHECKED, NULL);
			SendDlgItemMessage(*this, IDC_USETSVN, BM_SETCHECK, bUseTSVN ? BST_CHECKED : BST_UNCHECKED, NULL);
			SetDlgItemText(*this, IDC_DIFFVIEWER, wstring(diffViewer).c_str());
			SetDlgItemText(*this, IDC_NOTIFICATIONSOUNDPATH, wstring(notifySound).c_str());
			SendDlgItemMessage(*this, IDC_NOTIFICATIONSOUND, BM_SETCHECK, bPlaySound ? BST_CHECKED : BST_UNCHECKED, NULL);
			SendDlgItemMessage(*this, IDC_CHECKNEWER, BM_SETCHECK, DWORD(updatecheck) ? BST_CHECKED : BST_UNCHECKED, NULL);
			SendDlgItemMessage(*this, IDC_NOTIFYCONNECTERROR, BM_SETCHECK, bIndicateConnectErrors ? BST_CHECKED : BST_UNCHECKED, NULL);
			SendDlgItemMessage(*this, IDC_LEFTMENU, BM_SETCHECK, bLeftMenu ? BST_CHECKED : BST_UNCHECKED, NULL);
			wstring tsvninstalled = CAppUtils::GetTSVNPath();
			if (tsvninstalled.empty())
				::EnableWindow(GetDlgItem(*this, IDC_USETSVN), FALSE);
			SetDlgItemText(*this, IDC_NUMLOGS, numBuf);
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
			CRegStdDWORD updatecheck = CRegStdDWORD(_T("Software\\CommitMonitor\\CheckNewer"), TRUE);
			CRegStdDWORD numlogs = CRegStdDWORD(_T("Software\\CommitMonitor\\NumLogs"), 30);
			CRegStdDWORD regIndicateErrors = CRegStdDWORD(_T("Software\\CommitMonitor\\IndicateConnectErrors"), TRUE);
			CRegStdDWORD regLeftMenu = CRegStdDWORD(_T("Software\\CommitMonitor\\LeftClickMenu"), FALSE);
			bool bShowTaskbarIcon = !!SendDlgItemMessage(*this, IDC_TASKBAR_ALWAYSON, BM_GETCHECK, 0, NULL);
			bool bStartWithWindows = !!SendDlgItemMessage(*this, IDC_AUTOSTART, BM_GETCHECK, 0, NULL);
			bool bAnimateIcon = !!SendDlgItemMessage(*this, IDC_ANIMATEICON, BM_GETCHECK, 0, NULL);
			bool bPlaySound = !!SendDlgItemMessage(*this, IDC_NOTIFICATIONSOUND, BM_GETCHECK, 0, NULL);
			bool bUseTSVN = !!SendDlgItemMessage(*this, IDC_USETSVN, BM_GETCHECK, 0, NULL);
			bool bUpdateCheck = !!SendDlgItemMessage(*this, IDC_CHECKNEWER, BM_GETCHECK, 0, NULL);
			bool bIndicateConnectErrors = !!SendDlgItemMessage(*this, IDC_NOTIFYCONNECTERROR, BM_GETCHECK, 0, NULL);
			bool bLeftMenu = !!SendDlgItemMessage(*this, IDC_LEFTMENU, BM_GETCHECK, 0, NULL);
			regShowTaskbarIcon = bShowTaskbarIcon;
			regAnimateIcon = bAnimateIcon;
			regPlaySound = bPlaySound;
			regUseTSVN = bUseTSVN;
			updatecheck = bUpdateCheck;
			regIndicateErrors = bIndicateConnectErrors;
			regLeftMenu = bLeftMenu;
			::SendMessage(m_hHiddenWnd, COMMITMONITOR_CHANGEDINFO, 0, 0);
			if (bStartWithWindows)
			{
				TCHAR buf[MAX_PATH*4];
				GetModuleFileName(NULL, buf, MAX_PATH*4);
                wstring cmd = wstring(buf);
                cmd += _T(" /hidden");
				regStartWithWindows = cmd;
			}
			else
				regStartWithWindows.removeValue();

			int len = ::GetWindowTextLength(GetDlgItem(*this, IDC_DIFFVIEWER));
			TCHAR * divi = new TCHAR[len+1];
			::GetDlgItemText(*this, IDC_DIFFVIEWER, divi, len+1);
			wstring dv = wstring(divi);
			delete [] divi;
			CRegStdString diffViewer = CRegStdString(_T("Software\\CommitMonitor\\DiffViewer"));
			if (!dv.empty())
				diffViewer = dv;
			else
				diffViewer.removeValue();

			len = ::GetWindowTextLength(GetDlgItem(*this, IDC_NOTIFICATIONSOUNDPATH));
			divi = new TCHAR[len+1];
			::GetDlgItemText(*this, IDC_NOTIFICATIONSOUNDPATH, divi, len+1);
			wstring ns = wstring(divi);
			delete [] divi;
			CRegStdString notifySound = CRegStdString(_T("Software\\CommitMonitor\\NotificationSound"));
			if (!ns.empty())
				notifySound = ns;
			else
				notifySound.removeValue();

			len = ::GetWindowTextLength(GetDlgItem(*this, IDC_NUMLOGS));
			divi = new TCHAR[len+1];
			::GetDlgItemText(*this, IDC_NUMLOGS, divi, len+1);
			DWORD nLogs = _ttol(divi);
			numlogs = nLogs;
			delete [] divi;
		}
		// fall through
	case IDCANCEL:
		EndDialog(*this, id);
		break;
	case IDC_EXPORT:
		{
			if (m_pURLInfos)
			{
				CPasswordDlg dlg(*this);
				if (dlg.DoModal(hResource, IDD_PASSWORD, *this) == IDOK)
				{
					OPENFILENAME ofn = {0};		// common dialog box structure
					TCHAR szFile[MAX_PATH] = {0};  // buffer for file name
					// Initialize OPENFILENAME
					ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.hwndOwner = *this;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
					ofn.lpstrTitle = _T("Export monitored projects to...\0");
					ofn.Flags = OFN_HIDEREADONLY|OFN_DONTADDTORECENT|OFN_EXPLORER|OFN_ENABLESIZING|OFN_OVERWRITEPROMPT;
					ofn.lpstrFilter = _T("CommitMonitor Projects\0*.cmprj;*.com\0All files\0*.*\0\0");
					ofn.nFilterIndex = 1;
					// Display the Open dialog box. 
					if (GetSaveFileName(&ofn)==TRUE)
					{
						if (wcscmp(&szFile[wcslen(szFile)-6], L".cmprj"))
							wcscat_s(szFile, MAX_PATH, L".cmprj");
						m_pURLInfos->Export(szFile, dlg.password.c_str());
					}
				}
			}
		}
		break;
	case IDC_IMPORT:
		{
			if (m_pURLInfos)
			{
				CPasswordDlg dlg(*this);
				if (dlg.DoModal(hResource, IDD_PASSWORD, *this) == IDOK)
				{
					OPENFILENAME ofn = {0};		// common dialog box structure
					TCHAR szFile[MAX_PATH] = {0};  // buffer for file name
					// Initialize OPENFILENAME
					ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.hwndOwner = *this;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
					ofn.lpstrTitle = _T("Import monitored projects from...\0");
					ofn.Flags = OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_DONTADDTORECENT;
					ofn.lpstrFilter = _T("CommitMonitor Projects\0*.cmprj;*.com\0All files\0*.*\0\0");
					ofn.nFilterIndex = 1;
					// Display the Open dialog box. 
					if (GetOpenFileName(&ofn)==TRUE)
					{
						m_pURLInfos->Import(szFile, dlg.password.c_str());
					}
				}
			}
		}
		break;
	case IDC_DIFFBROWSE:
		{
			OPENFILENAME ofn = {0};		// common dialog box structure
			TCHAR szFile[MAX_PATH] = {0};  // buffer for file name
			// Initialize OPENFILENAME
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = *this;
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
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
			OPENFILENAME ofn = {0};		// common dialog box structure
			TCHAR szFile[MAX_PATH] = {0};  // buffer for file name
			// Initialize OPENFILENAME
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = *this;
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
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

