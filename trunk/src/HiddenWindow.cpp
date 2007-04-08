#include "StdAfx.h"
#include "HiddenWindow.h"
#include "resource.h"
#include "MainDlg.h"
#include "SVN.h"
#include "AppUtils.h"

extern HINSTANCE hInst;

DWORD WINAPI MonitorThread(LPVOID lpParam);

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
			COMMITMONITOR_CHANGEDINFO = RegisterWindowMessage(_T("CommitMonitor_ChangedInfo"));
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
	// the custom messages are not constant, therefore we can't handle them in the
	// switch-case below
	if (uMsg == COMMITMONITOR_SHOWDLGMSG)
	{
		CMainDlg dlg;
		dlg.DoModal(hInst, IDD_MAINDLG, NULL);
		wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
		if (PathFileExists(urlfile.c_str()))
			m_UrlInfos.Load(urlfile.c_str());
	}
	else if (uMsg == COMMITMONITOR_CHANGEDINFO)
	{
		wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
		if (PathFileExists(urlfile.c_str()))
			m_UrlInfos.Save(urlfile.c_str());
	}

	switch (uMsg)
	{
	case WM_CREATE:
		{
			m_hwnd = hwnd;
			// set the timers we use to start the monitoring threads
			::SetTimer(*this, IDT_MONITOR, 10, NULL);
		}
		break;
	case WM_TIMER:
		{
			if (wParam == IDT_MONITOR)
			{
				DoTimer();
			}
		}
		break;
	case WM_COMMAND:
		{
			return DoCommand(LOWORD(wParam));
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

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
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

void CHiddenWindow::DoTimer()
{
	// go through all url infos and check if
	// we need to refresh them
	if (m_UrlInfos.infos.empty())
		return;
	bool bStartThread = false;
	__time64_t currenttime = NULL;
	_time64(&currenttime);
	for (map<wstring,CUrlInfo>::const_iterator it = m_UrlInfos.infos.begin(); it != m_UrlInfos.infos.end(); ++it)
	{
		if ((it->second.lastchecked + (it->second.minutesinterval*60000)) < currenttime)
		{
			bStartThread = true;
			break;
		}
	}
	if ((bStartThread)&&(m_ThreadRunning == 0))
	{
		::SetTimer(*this, IDT_MONITOR, TIMER_ELAPSE, NULL);

		// start the monitoring thread to update the infos
		DWORD dwThreadId = 0;
		HANDLE hMonitorThread = CreateThread( 
			NULL,              // no security attribute 
			0,                 // default stack size 
			MonitorThread, 
			(LPVOID)this,      // thread parameter 
			0,                 // not suspended 
			&dwThreadId);      // returns thread ID 

		if (hMonitorThread == NULL) 
			return;
		else 
			CloseHandle(hMonitorThread); 
	}
}

DWORD CHiddenWindow::RunThread()
{
	m_ThreadRunning = TRUE;
	__time64_t currenttime = NULL;
	_time64(&currenttime);
	bool bNewEntries = false;

	for (map<wstring,CUrlInfo>::iterator it = m_UrlInfos.infos.begin(); it != m_UrlInfos.infos.end(); ++it)
	{
		if ((it->second.lastchecked + (it->second.minutesinterval*60000)) < currenttime)
		{
			// get the highest revision of the repository
			SVN svn;
			svn.SetAuthInfo(it->second.username, it->second.password);
			svn_revnum_t headrev = svn.GetHEADRevision(it->first);
			if (headrev > it->second.lastcheckedrev)
			{
				if (svn.GetLog(it->first, headrev, it->second.lastcheckedrev))
				{
					it->second.lastcheckedrev = headrev;
					it->second.lastchecked = currenttime;
					bNewEntries = true;
					for (map<svn_revnum_t,SVNLogEntry>::const_iterator logit = svn.m_logs.begin(); logit != svn.m_logs.end(); ++logit)
					{
						it->second.logentries[logit->first] = logit->second;
					}
				}
			}
		}
	}
	if (bNewEntries)
	{
		// save the changed entries
		::PostMessage(*this, COMMITMONITOR_CHANGEDINFO, 0, 0);
	}
	m_ThreadRunning = FALSE;
	return 0L;
}

DWORD WINAPI MonitorThread(LPVOID lpParam)
{
	CHiddenWindow * pThis = (CHiddenWindow*)lpParam;
	if (pThis)
		return pThis->RunThread();
	return 0L;
}

