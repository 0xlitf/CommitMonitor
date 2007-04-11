#include "StdAfx.h"
#include <fstream>
#include <Urlmon.h>
#pragma comment(lib, "Urlmon.lib")

#include "HiddenWindow.h"
#include "resource.h"
#include "MainDlg.h"
#include "SVN.h"
#include "Callback.h"
#include "AppUtils.h"
#include "TempFile.h"

#include <boost/regex.hpp>
using namespace boost;




extern HINSTANCE hInst;

DWORD WINAPI MonitorThread(LPVOID lpParam);

CHiddenWindow::CHiddenWindow(HINSTANCE hInst, const WNDCLASSEX* wcx /* = NULL*/) 
	: CWindow(hInst, wcx)
	, m_ThreadRunning(0)
	, m_bMainDlgShown(false)
{
	m_hIconNew = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NOTIFYNEW));
	m_hIconNormal = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NOTIFYNORMAL));
	ZeroMemory(&m_SystemTray, sizeof(m_SystemTray));
}

CHiddenWindow::~CHiddenWindow(void)
{
	DestroyIcon(m_hIconNew);
	DestroyIcon(m_hIconNormal);

	Shell_NotifyIcon(NIM_DELETE, &m_SystemTray);
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
			COMMITMONITOR_TASKBARCALLBACK = RegisterWindowMessage(_T("CommitMonitor_TaskbarCallback"));	
			ShowWindow(m_hwnd, SW_HIDE);
			ShowTrayIcon(false);
			return true;
		}
	}
	return false;
}

INT_PTR CHiddenWindow::ShowDialog()
{
	return ::SendMessage(*this, COMMITMONITOR_SHOWDLGMSG, 0, 0);
}

LRESULT CHiddenWindow::HandleCustomMessages(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == COMMITMONITOR_SHOWDLGMSG)
	{
		if (m_bMainDlgShown)
			return TRUE;
		m_bMainDlgShown = true;
		CMainDlg dlg(*this);
		dlg.DoModal(hInst, IDD_MAINDLG, NULL);
		wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
		if (PathFileExists(urlfile.c_str()))
			m_UrlInfos.Load(urlfile.c_str());
		m_bMainDlgShown = false;
		return TRUE;
	}
	else if (uMsg == COMMITMONITOR_CHANGEDINFO)
	{
		if (wParam)
		{
			wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
			if (PathFileExists(urlfile.c_str()))
				m_UrlInfos.Save(urlfile.c_str());
		}
		ShowTrayIcon(!!wParam);
		return TRUE;
	}
	else if (uMsg == COMMITMONITOR_TASKBARCALLBACK)
	{
		switch (lParam)
		{
		case WM_MOUSEMOVE:
			{
				// update the tool tip data
			}
			break;
		case WM_LBUTTONDBLCLK:
			{
				// show the main dialog
				ShowDialog();
			}
			break;
		case NIN_KEYSELECT:
		case NIN_SELECT:
		case WM_RBUTTONUP:
		case WM_CONTEXTMENU:
			{
				POINT pt;
				GetCursorPos( &pt );

				HMENU hMenu = ::LoadMenu(hInst, MAKEINTRESOURCE(IDC_COMMITMONITOR));
				hMenu = ::GetSubMenu(hMenu, 0);

				// set the default entry
				MENUITEMINFO iinfo = {0};
				iinfo.cbSize = sizeof(MENUITEMINFO);
				iinfo.fMask = MIIM_STATE;
				GetMenuItemInfo(hMenu, 0, MF_BYPOSITION, &iinfo);
				iinfo.fState |= MFS_DEFAULT;
				SetMenuItemInfo(hMenu, 0, MF_BYPOSITION, &iinfo);

				// show the menu
				::SetForegroundWindow(*this);
				int cmd = ::TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY , pt.x, pt.y, NULL, *this, NULL);
				::PostMessage(*this, WM_NULL, 0, 0);

				switch( cmd )
				{
				case IDM_EXIT:
					::PostQuitMessage(0);
					break;
				case ID_POPUP_OPENCOMMITMONITOR:
					ShowDialog();
					break;
				}
			}
			break;
		}
		return TRUE;
	}
	return 0L;
}

LRESULT CALLBACK CHiddenWindow::WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// the custom messages are not constant, therefore we can't handle them in the
	// switch-case below
	HandleCustomMessages(hwnd, uMsg, wParam, lParam);
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
	TRACE(_T("timer fired\n"));
	// Restart the timer with 60 seconds
	::SetTimer(*this, IDT_MONITOR, TIMER_ELAPSE, NULL);
	// go through all url infos and check if
	// we need to refresh them
	if (m_UrlInfos.infos.empty())
		return;
	bool bStartThread = false;
	__time64_t currenttime = NULL;
	_time64(&currenttime);
	for (map<wstring,CUrlInfo>::const_iterator it = m_UrlInfos.infos.begin(); it != m_UrlInfos.infos.end(); ++it)
	{
		if ((it->second.lastchecked + (it->second.minutesinterval*60)) < currenttime)
		{
			bStartThread = true;
			break;
		}
	}
	if ((bStartThread)&&(m_ThreadRunning == 0))
	{
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

void CHiddenWindow::ShowTrayIcon(bool newCommits)
{
	TRACE(_T("changing tray icon to %s\n"), (newCommits ? _T("\"new commits\"") : _T("\"normal\"")));

	DWORD msg = m_SystemTray.hIcon ? NIM_MODIFY : NIM_ADD;
	m_SystemTray.cbSize = sizeof(NOTIFYICONDATA);
	m_SystemTray.hWnd   = *this;
	m_SystemTray.uID    = 1;
	m_SystemTray.hIcon  = newCommits ? m_hIconNew : m_hIconNormal;
	m_SystemTray.uFlags = NIF_MESSAGE | NIF_ICON;
	m_SystemTray.uCallbackMessage = COMMITMONITOR_TASKBARCALLBACK;
	Shell_NotifyIcon(msg, &m_SystemTray);
}

DWORD CHiddenWindow::RunThread()
{
	m_ThreadRunning = TRUE;
	__time64_t currenttime = NULL;
	_time64(&currenttime);
	bool bNewEntries = false;

	if (::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)!=S_OK)
	{
		return 1;
	}

	TRACE(_T("monitor thread started\n"));
	for (map<wstring,CUrlInfo>::iterator it = m_UrlInfos.infos.begin(); it != m_UrlInfos.infos.end(); ++it)
	{
		if ((it->second.lastchecked + (it->second.minutesinterval*60)) < currenttime)
		{
			TRACE(_T("checking %s for updates\n"), it->first.c_str());
			// get the highest revision of the repository
			SVN svn;
			svn.SetAuthInfo(it->second.username, it->second.password);
			svn_revnum_t headrev = svn.GetHEADRevision(it->first);
			if (headrev > it->second.lastcheckedrev)
			{
				TRACE(_T("%s has updates! Last checked revision was %ld, HEAD revision is %ld\n"), it->first.c_str(), it->second.lastcheckedrev, headrev);
				if (svn.GetLog(it->first, headrev, it->second.lastcheckedrev))
				{
					TRACE(_T("log fetched for %s\n"), it->first.c_str());
					it->second.lastcheckedrev = headrev;
					it->second.lastchecked = currenttime;
					bNewEntries = true;
					for (map<svn_revnum_t,SVNLogEntry>::const_iterator logit = svn.m_logs.begin(); logit != svn.m_logs.end(); ++logit)
					{
						it->second.logentries[logit->first] = logit->second;
						if (it->second.fetchdiffs)
						{
							// first, find a name where to store the diff for that revision
							TCHAR buf[4096];
							_stprintf_s(buf, 4096, _T("%s_%ld"), it->second.name.c_str(), logit->first);
							wstring diffFileName = CAppUtils::GetAppDataDir();
							diffFileName += _T("/");
							diffFileName += wstring(buf);
							// get the diff
							svn.Diff(it->first, logit->first - 1, it->first, logit->first, false, true, false, wstring(), false, diffFileName, wstring());
							TRACE(_T("Diff fetched for %s, revision %ld\n"), it->first.c_str(), logit->first);
						}
					}
				}
			}
			else
			{
				// if we can't fetch the HEAD revision, it might be because the URL points to an SVNParentPath
				// instead of pointing to an actual repository.

				// we have to include the authentication in the URL itself
				size_t colonpos = it->first.find_first_of(':');
				wstring authurl = it->first.substr(0, colonpos+3);
				authurl += it->second.username;
				authurl += _T(":");
				authurl += it->second.password;
				authurl += _T("@");
				authurl += it->first.substr(colonpos+3);
				wstring tempfile = CTempFiles::Instance().GetTempFilePath(true);
				CCallback * callback = new CCallback;
				callback->SetAuthData(it->second.username, it->second.password);
				if (URLDownloadToFile(NULL, it->first.c_str(), tempfile.c_str(), 0, callback) == S_OK)
				{
					// we got a web page! But we can't be sure that it's the page from SVNParentPath.
					// Use a regex to parse the website and find out...
					ifstream fs(tempfile.c_str());
					string in;
					if (!fs.bad())
					{
						in.reserve(fs.rdbuf()->in_avail());
						char c;
						while (fs.get(c))
						{
							if (in.capacity() == in.size())
								in.reserve(in.capacity() * 3);
							in.append(1, c);
						}
						const char * re = "<\\s*LI\\s*>\\s*<\\s*A\\s+[^>]*href\\s*=\\s*\"([^\"]*)\"\\s*>([^/<])<\\s*/\\s*A\\s*>\\s*<\\s*/\\s*LI\\s*>";

						regex expression = regex(re, regex::normal | regbase::icase);
						string::const_iterator start = in.begin();
						string::const_iterator end = in.end();
						match_results<string::const_iterator> what;
						match_flag_type flags = match_default;
						while(regex_search(start, end, what, expression, flags))   
						{
							// what[0] contains the whole string
							// what[1] contains the url.
							// what[2] contains the name
							wstring url = CUnicodeUtils::StdGetUnicode(string(what[1].first, what[1].second));
							if (m_UrlInfos.infos.find(url) == m_UrlInfos.infos.end())
							{
								// we found a new URL, add it to our list
								CUrlInfo newinfo;
								newinfo.url = url;
								newinfo.name = CUnicodeUtils::StdGetUnicode(string(what[2].first, what[2].second));
								newinfo.username = it->second.username;
								newinfo.password = it->second.password;
								newinfo.fetchdiffs = it->second.fetchdiffs;
								newinfo.minutesinterval = it->second.minutesinterval;
								m_UrlInfos.infos[url] = newinfo;
							}
							// update search position:
							start = what[0].second;      
							// update flags:
							flags |= match_prev_avail;
							flags |= match_not_bob;
						}

					}
					delete callback;
				}
			}
		}
	}
	if (bNewEntries)
	{
		// save the changed entries
		::PostMessage(*this, COMMITMONITOR_CHANGEDINFO, (WPARAM)true, 0);
	}
	TRACE(_T("monitor thread ended\n"));
	m_ThreadRunning = FALSE;
	::CoUninitialize();
	return 0L;
}

DWORD WINAPI MonitorThread(LPVOID lpParam)
{
	CHiddenWindow * pThis = (CHiddenWindow*)lpParam;
	if (pThis)
		return pThis->RunThread();
	return 0L;
}

