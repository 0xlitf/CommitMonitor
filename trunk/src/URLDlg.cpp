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

void CURLDlg::SetInfo(CUrlInfo * pURLInfo /* = NULL */)
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
			_stprintf_s(buf, 20, _T("%ld"), info.minutesinterval);
			SetWindowText(GetDlgItem(*this, IDC_CHECKTIME), buf);
			SetWindowText(GetDlgItem(*this, IDC_PROJECTNAME), info.name.c_str());
			SetWindowText(GetDlgItem(*this, IDC_USERNAME), info.username.c_str());
			SetWindowText(GetDlgItem(*this, IDC_PASSWORD), info.password.c_str());
			SendMessage(GetDlgItem(*this, IDC_CREATEDIFFS), BM_SETCHECK, info.fetchdiffs ? BST_CHECKED : BST_UNCHECKED, NULL);
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
			int len = GetWindowTextLength(GetDlgItem(*this, IDC_URLTOMONITOR));
			WCHAR * buffer = new WCHAR[len+1];
			GetWindowText(GetDlgItem(*this, IDC_URLTOMONITOR), buffer, len+1);
			info.url = wstring(buffer, len);
			delete [] buffer;

			len = GetWindowTextLength(GetDlgItem(*this, IDC_PROJECTNAME));
			buffer = new WCHAR[len+1];
			GetWindowText(GetDlgItem(*this, IDC_PROJECTNAME), buffer, len+1);
			info.name = wstring(buffer, len);

			len = GetWindowTextLength(GetDlgItem(*this, IDC_CHECKTIME));
			buffer = new WCHAR[len+1];
			GetWindowText(GetDlgItem(*this, IDC_CHECKTIME), buffer, len+1);
			info.minutesinterval = _ttoi(buffer);
			delete [] buffer;

			len = GetWindowTextLength(GetDlgItem(*this, IDC_USERNAME));
			buffer = new WCHAR[len+1];
			GetWindowText(GetDlgItem(*this, IDC_USERNAME), buffer, len+1);
			info.username = wstring(buffer, len);
			delete [] buffer;

			len = GetWindowTextLength(GetDlgItem(*this, IDC_PASSWORD));
			buffer = new WCHAR[len+1];
			GetWindowText(GetDlgItem(*this, IDC_PASSWORD), buffer, len+1);
			info.password = wstring(buffer, len);
			delete [] buffer;
			info.fetchdiffs = (SendMessage(GetDlgItem(*this, IDC_CREATEDIFFS), BM_GETCHECK, 0, 0) == BST_CHECKED);
		}
		// fall through
	case IDCANCEL:
		EndDialog(*this, id);
		break;
	}
	return 1;
}

