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
			GetWindowText(GetDlgItem(*this, IDC_URLTOMONITOR), buffer, len);
			info.url = wstring(buffer, len);
			delete [] buffer;
			len = GetWindowTextLength(GetDlgItem(*this, IDC_CHECKTIME));
			buffer = new WCHAR[len+1];
			GetWindowText(GetDlgItem(*this, IDC_CHECKTIME), buffer, len);
			info.minutesinterval = _ttoi(buffer);
			delete [] buffer;
			info.fetchdiffs = (SendMessage(GetDlgItem(*this, IDC_CREATEDIFFS), BM_GETCHECK, 0, 0) == BST_CHECKED);
		}
		// fall through
	case IDCANCEL:
		EndDialog(*this, id);
		break;
	case IDC_CHECKURL:
		{
			// to check if an URL is valid, we have to
			// * do an 'svn info' on the URL
			// * get the html page from the URL, it may be the SVNParentPath
			int len = GetWindowTextLength(GetDlgItem(*this, IDC_URLTOMONITOR));
			TCHAR * buf = new TCHAR[len+1];
			GetWindowText(GetDlgItem(*this, IDC_URLTOMONITOR), buf, len);
			stdstring sUrl = stdstring(buf);
			delete [] buf;
			SVN svn;
			const SVNInfoData * info = svn.GetFirstFileInfo(sUrl, -1, -1, false);
			if (info)
			{
				// the URL is valid (i.e. we could get a Subversion info from it)
			}
			else
			{
				// the URL seems not valid (or the server is not reachable)
				// try fetching the html page from it (if it's a http/https URL)
				// and parse the page - maybe it's the page of an SVNParentPath
				// option from where we can get all the repositories to monitor
			}
		}
		break;
	}
	return 1;
}

