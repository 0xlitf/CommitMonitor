#include "StdAfx.h"
#include "Resource.h"
#include "MainDlg.h"

CMainDlg::CMainDlg(void)
{
}

CMainDlg::~CMainDlg(void)
{
}

LRESULT CMainDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

LRESULT CMainDlg::DoCommand(int id)
{
	switch (id)
	{
	case IDOK:
		break;
	case IDCANCEL:
		PostQuitMessage(IDCANCEL);
		return 0;
		break;
	}
	return 1;
}

