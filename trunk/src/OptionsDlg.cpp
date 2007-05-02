#include "StdAfx.h"
#include "Resource.h"
#include "OptionsDlg.h"
#include "Registry.h"
#include <string>
#include <Commdlg.h>

using namespace std;

COptionsDlg::COptionsDlg(HWND hParent)
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
			// initialize the controls
			bool bShowTaskbarIcon = !!(DWORD)CRegStdWORD(_T("Software\\CommitMonitor\\TaskBarIcon"), FALSE);
			bool bStartWithWindows = !wstring(CRegStdString(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run\\CommitMonitor"))).empty();
            CRegStdString diffViewer = CRegStdString(_T("Software\\CommitMonitor\\DiffViewer"));
			SendMessage(GetDlgItem(*this, IDC_TASKBAR_ALWAYSON), BM_SETCHECK, bShowTaskbarIcon ? BST_CHECKED : BST_UNCHECKED, NULL);
			SendMessage(GetDlgItem(*this, IDC_AUTOSTART), BM_SETCHECK, bStartWithWindows ? BST_CHECKED : BST_UNCHECKED, NULL);
            SetWindowText(GetDlgItem(*this, IDC_DIFFVIEWER), wstring(diffViewer).c_str());
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
			CRegStdWORD regShowTaskbarIcon = CRegStdWORD(_T("Software\\CommitMonitor\\TaskBarIcon"), FALSE);
			CRegStdString regStartWithWindows = CRegStdString(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run\\CommitMonitor"));
			bool bShowTaskbarIcon = !!SendMessage(GetDlgItem(*this, IDC_TASKBAR_ALWAYSON), BM_GETCHECK, 0, NULL);
			bool bStartWithWindows = !!SendMessage(GetDlgItem(*this, IDC_AUTOSTART), BM_GETCHECK, 0, NULL);
			regShowTaskbarIcon = bShowTaskbarIcon;
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
            ::GetWindowText(GetDlgItem(*this, IDC_DIFFVIEWER), divi, len+1);
            wstring dv = wstring(divi);
            delete [] divi;
            CRegStdString diffViewer = CRegStdString(_T("Software\\CommitMonitor\\DiffViewer"));
            if (!dv.empty())
                diffViewer = dv;
            else
                diffViewer.removeValue();
		}
		// fall through
	case IDCANCEL:
		EndDialog(*this, id);
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
				SetWindowText(GetDlgItem(*this, IDC_DIFFVIEWER), szFile);
			}
		}
		break;
	}
	return 1;
}

