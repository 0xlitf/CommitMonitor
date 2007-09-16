#include "StdAfx.h"
#include "Resource.h"
#include "URLDlg.h"

#include "SVN.h"


CURLDlg::CURLDlg(void)
{
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

LRESULT CURLDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			InitDialog(hwndDlg, IDI_COMMITMONITOR);
			// initialize the controls
			SetWindowText(GetDlgItem(*this, IDC_URLTOMONITOR), info.url.c_str());
			WCHAR buf[20];
			_stprintf_s(buf, 20, _T("%ld"), max(info.minutesinterval, info.minminutesinterval));
			SetWindowText(GetDlgItem(*this, IDC_CHECKTIME), buf);
			SetWindowText(GetDlgItem(*this, IDC_PROJECTNAME), info.name.c_str());
			SetWindowText(GetDlgItem(*this, IDC_USERNAME), info.username.c_str());
			SetWindowText(GetDlgItem(*this, IDC_PASSWORD), info.password.c_str());
			SendMessage(GetDlgItem(*this, IDC_CREATEDIFFS), BM_SETCHECK, info.fetchdiffs ? BST_CHECKED : BST_UNCHECKED, NULL);
			if (info.disallowdiffs)
				EnableWindow(GetDlgItem(*this, IDC_CREATEDIFFS), FALSE);

			HWND hwndTT;				// handle to the ToolTip control
			TOOLINFO ti = {0};
			hwndTT = CreateWindowEx(WS_EX_TOPMOST,
				TOOLTIPS_CLASS,
				NULL,
				WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,		
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				*this,
				NULL,
				hResource,
				NULL
				);

			ti.cbSize = sizeof(TOOLINFO);
			ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS | TTF_PARSELINKS;
			ti.hwnd = hwndTT;
			ti.hinst = hResource;
			ti.uId = (UINT_PTR)GetDlgItem(*this, IDC_CREATEDIFFS);
			ti.lpszText = _T("Fetches the diff for each revision automatically\nPlease do NOT enable this for repositories which are not on your LAN!");
			SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);	
			ti.uId = (UINT_PTR)GetDlgItem(*this, IDC_PROJECTNAME);
			ti.lpszText = _T("Enter here a name for the project");
			SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
			ti.uId = (UINT_PTR)GetDlgItem(*this, IDC_CHECKTIME);
			if (info.minminutesinterval)
			{
				TCHAR infobuf[MAX_PATH] = {0};
				_stprintf_s(infobuf, MAX_PATH, _T("Interval for repository update checks.\nMiminum set by svnrobots.txt file to %ld minutes."), info.minminutesinterval);
				ti.lpszText = infobuf;
			}
			else
			{
				ti.lpszText = _T("Interval for repository update checks");
			}
			SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);	


			SendMessage(hwndTT, TTM_SETMAXTIPWIDTH, 0, 600);
			SendMessage(hwndTT, TTM_ACTIVATE, 1, 0);	
		}
		return TRUE;
	case WM_COMMAND:
		return DoCommand(LOWORD(wParam));
	case WM_NOTIFY:
		{
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CURLDlg::DoCommand(int id)
{
	switch (id)
	{
	case IDOK:
		{
			SVN svn;
			int len = GetWindowTextLength(GetDlgItem(*this, IDC_URLTOMONITOR));
			WCHAR * buffer = new WCHAR[len+1];
			GetDlgItemText(*this, IDC_URLTOMONITOR, buffer, len+1);
			info.url = svn.CanonicalizeURL(wstring(buffer, len));
			delete [] buffer;

			len = GetWindowTextLength(GetDlgItem(*this, IDC_PROJECTNAME));
			buffer = new WCHAR[len+1];
			GetDlgItemText(*this, IDC_PROJECTNAME, buffer, len+1);
			info.name = wstring(buffer, len);

			len = GetWindowTextLength(GetDlgItem(*this, IDC_CHECKTIME));
			buffer = new WCHAR[len+1];
			GetDlgItemText(*this, IDC_CHECKTIME, buffer, len+1);
			info.minutesinterval = _ttoi(buffer);
			if ((info.minminutesinterval)&&(info.minminutesinterval < info.minutesinterval))
				info.minutesinterval = info.minminutesinterval;
			delete [] buffer;

			len = GetWindowTextLength(GetDlgItem(*this, IDC_USERNAME));
			buffer = new WCHAR[len+1];
			GetDlgItemText(*this, IDC_USERNAME, buffer, len+1);
			info.username = wstring(buffer, len);
			delete [] buffer;

			len = GetWindowTextLength(GetDlgItem(*this, IDC_PASSWORD));
			buffer = new WCHAR[len+1];
			GetDlgItemText(*this, IDC_PASSWORD, buffer, len+1);
			info.password = wstring(buffer, len);
			delete [] buffer;
			info.fetchdiffs = (SendMessage(GetDlgItem(*this, IDC_CREATEDIFFS), BM_GETCHECK, 0, 0) == BST_CHECKED);

			// make sure this entry gets checked again as soon as the next timer fires
			info.lastchecked = 0;
		}
		// fall through
	case IDCANCEL:
		EndDialog(*this, id);
		break;
	}
	return 1;
}

