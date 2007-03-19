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

LRESULT CURLDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			InitDialog(hwndDlg, IDI_COMMITMONITOR);
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

