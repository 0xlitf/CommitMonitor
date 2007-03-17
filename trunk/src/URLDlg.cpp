#include "StdAfx.h"
#include "Resource.h"
#include "URLDlg.h"

CURLDlg::CURLDlg(void)
{
}

CURLDlg::~CURLDlg(void)
{
}

LRESULT CURLDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
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
		}
		break;
	}
	return 1;
}

