// CommitMonitor.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "CommitMonitor.h"
#include "HiddenWindow.h"
#include "MainDlg.h"

#include "apr_general.h"

// Global Variables:
HINSTANCE hInst;								// current instance
HANDLE g_mutex = 0;

#define APPNAME_MUTEX _T("CommitMonitor_{3802F59C-BEBD-49b9-A345-F99CBA2FBD0D}")

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	::CoInitialize(NULL);

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
	setlocale(LC_ALL, ""); 

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

	//only one instance of this application allowed
	g_mutex = ::CreateMutex(NULL, FALSE, APPNAME_MUTEX);

	if (g_mutex != NULL)
	{   
		if(::GetLastError()==ERROR_ALREADY_EXISTS)
		{
			//an instance of this app is already running
			HWND hWnd = FindWindow(ResString(hInst, IDS_APP_TITLE), NULL);		//try finding the running instance of this app
			if (hWnd)
			{
				UINT COMMITMONITOR_SHOWDLGMSG = RegisterWindowMessage(_T("CommitMonitor_ShowDlgMsg"));
				PostMessage(hWnd, COMMITMONITOR_SHOWDLGMSG ,0 ,0);				//open the window of the already running app
				SetForegroundWindow(hWnd);										//set the window to front
			}
			apr_terminate();
			return FALSE;
		}		
	}


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
	hiddenWindow.StopThread();

	::CoUninitialize();
	apr_terminate();
	return (int) 0;
}



