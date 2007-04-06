// CommitMonitor.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "CommitMonitor.h"
#include "HiddenWindow.h"
#include "MainDlg.h"

#include "apr_general.h"

// Global Variables:
HINSTANCE hInst;								// current instance

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);


	// we need some of the common controls
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_LINK_CLASS|ICC_LISTVIEW_CLASSES|ICC_PAGESCROLLER_CLASS
		|ICC_PROGRESS_CLASS|ICC_STANDARD_CLASSES|ICC_TAB_CLASSES|ICC_TREEVIEW_CLASSES
		|ICC_UPDOWN_CLASS|ICC_USEREX_CLASSES|ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icex);

	int argc = 0;
	const char* const * argv = NULL;
	apr_app_initialize(&argc, &argv, NULL);

	// first create a hidden window which serves as our main window for receiving
	// the window messages, starts the monitoring thread and handles the icon
	// in the tray area.

	MSG msg;
	HACCEL hAccelTable;

	hInst = hInstance;

	INITCOMMONCONTROLSEX used = {
		sizeof(INITCOMMONCONTROLSEX),
		ICC_STANDARD_CLASSES | ICC_BAR_CLASSES
	};
	InitCommonControlsEx(&used);

	CHiddenWindow hiddenWindow(hInst);


	if (hiddenWindow.RegisterAndCreateWindow())
	{
		hAccelTable = LoadAccelerators(hInst, MAKEINTRESOURCE(IDC_COMMITMONITOR));

		if (true)	// for now, start the dialog every time
		{
			hiddenWindow.ShowDialog();
		}

		// Main message loop:
		while (GetMessage(&msg, NULL, 0, 0))
		{
			if (!TranslateAccelerator(hiddenWindow, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		return (int) msg.wParam;
	}


	apr_terminate();
	return (int) 0;
}



