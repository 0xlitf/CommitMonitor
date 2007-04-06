#include "StdAfx.h"
#include "HiddenWindow.h"
#include "resource.h"
#include "MainDlg.h"

extern HINSTANCE hInst;
extern const UINT COMMITMONITOR_SHOWDLGMSG;

CHiddenWindow::~CHiddenWindow(void)
{
}

bool CHiddenWindow::RegisterAndCreateWindow()
{
	WNDCLASSEX wcx; 

	// Fill in the window class structure with default parameters 
	wcx.cbSize = sizeof(WNDCLASSEX);
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = CWindow::stWinMsgHandler;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	wcx.hInstance = hResource;
	wcx.hCursor = LoadCursor(NULL, IDC_SIZEWE);
	wcx.lpszClassName = ResString(hResource, IDS_APP_TITLE);
	wcx.hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_COMMITMONITOR));
	wcx.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
	wcx.lpszMenuName = MAKEINTRESOURCE(IDC_COMMITMONITOR);
	wcx.hIconSm	= LoadIcon(wcx.hInstance, MAKEINTRESOURCE(IDI_COMMITMONITOR));
	if (RegisterWindow(&wcx))
	{
		if (Create(WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, NULL))
		{
			COMMITMONITOR_SHOWDLGMSG = RegisterWindowMessage(_T("CommitMonitor_ShowDlgMsg"));
			ShowWindow(m_hwnd, SW_HIDE);
			return true;
		}
	}
	return false;
}

INT_PTR CHiddenWindow::ShowDialog()
{
	return ::SendMessage(*this, COMMITMONITOR_SHOWDLGMSG, 0, 0);
}

LRESULT CALLBACK CHiddenWindow::WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == COMMITMONITOR_SHOWDLGMSG)
	{
		CMainDlg dlg;
		dlg.DoModal(hInst, IDD_MAINDLG, NULL);
	}
	switch (uMsg)
	{
	case WM_CREATE:
		{
			m_hwnd = hwnd;
		}
		break;
	case WM_COMMAND:
		{
			return DoCommand(LOWORD(wParam));
		}
		break;
	case WM_NOTIFY:
		{
			LPNMHDR pNMHDR = (LPNMHDR)lParam;
			if (pNMHDR->code == TTN_GETDISPINFO)
			{
				LPTOOLTIPTEXT lpttt; 

				lpttt = (LPTOOLTIPTEXT) lParam; 
				lpttt->hinst = hResource; 

				// Specify the resource identifier of the descriptive 
				// text for the given button. 
				TCHAR stringbuf[MAX_PATH] = {0};
				MENUITEMINFO mii;
				mii.cbSize = sizeof(MENUITEMINFO);
				mii.fMask = MIIM_TYPE;
				mii.dwTypeData = stringbuf;
				mii.cch = sizeof(stringbuf)/sizeof(TCHAR);
				GetMenuItemInfo(GetMenu(*this), lpttt->hdr.idFrom, FALSE, &mii);
				lpttt->lpszText = stringbuf;
			}
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		::DestroyWindow(m_hwnd);
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
};

LRESULT CHiddenWindow::DoCommand(int id)
{
	switch (id) 
	{
	default:
		break;
	};
	return 1;
}
