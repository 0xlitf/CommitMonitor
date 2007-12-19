// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "StdAfx.h"
#include <fstream>
#include <sstream>
#include <Urlmon.h>
#pragma comment(lib, "Urlmon.lib")

#include "HiddenWindow.h"
#include "resource.h"
#include "MainDlg.h"
#include "SVN.h"
#include "Callback.h"
#include "AppUtils.h"
#include "StatusBarMsgWnd.h"

#include <boost/regex.hpp>
using namespace boost;

// for Vista
#define MSGFLT_ADD 1


extern HINSTANCE hInst;

DWORD WINAPI MonitorThread(LPVOID lpParam);

CHiddenWindow::PFNCHANGEWINDOWMESSAGEFILTER CHiddenWindow::m_pChangeWindowMessageFilter = NULL;

CHiddenWindow::CHiddenWindow(HINSTANCE hInst, const WNDCLASSEX* wcx /* = NULL*/) 
	: CWindow(hInst, wcx)
	, m_ThreadRunning(0)
	, m_bRun(true)
	, m_hMonitorThread(NULL)
	, m_bMainDlgShown(false)
	, m_bMainDlgRemovedItems(false)
	, m_hMainDlg(NULL)
	, m_nIcon(0)
	, regShowTaskbarIcon(_T("Software\\CommitMonitor\\TaskBarIcon"), TRUE)
	, m_bIsTask(false)
{
	m_hIconNew0 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NOTIFYNEW0));
	m_hIconNew1 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NOTIFYNEW1));
	m_hIconNew2 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NOTIFYNEW2));
	m_hIconNew3 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NOTIFYNEW3));
	m_hIconNormal = LoadIcon(hInst, MAKEINTRESOURCE(IDI_COMMITMONITOR));
	ZeroMemory(&m_SystemTray, sizeof(m_SystemTray));
}

CHiddenWindow::~CHiddenWindow(void)
{
	DestroyIcon(m_hIconNew0);
	DestroyIcon(m_hIconNew1);
	DestroyIcon(m_hIconNew2);
	DestroyIcon(m_hIconNew3);
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
			WM_TASKBARCREATED = RegisterWindowMessage(_T("TaskbarCreated"));
			// On Vista, the message TasbarCreated may be blocked by the message filter.
			// We try to change the filter here to get this message through. If even that
			// fails, then we can't do much about it and the task bar icon won't show up again.
			HMODULE hLib = LoadLibrary(_T("user32.dll"));
			if (hLib)
			{
				m_pChangeWindowMessageFilter = (CHiddenWindow::PFNCHANGEWINDOWMESSAGEFILTER)GetProcAddress(hLib, "ChangeWindowMessageFilter");
				if (m_pChangeWindowMessageFilter)
				{
					(*m_pChangeWindowMessageFilter)(WM_TASKBARCREATED, MSGFLT_ADD);
				}
			}
			ShowWindow(m_hwnd, SW_HIDE);
			ShowTrayIcon(false);
			m_UrlInfos.Load();
			return true;
		}
	}
	return false;
}

INT_PTR CHiddenWindow::ShowDialog()
{
	return ::SendMessage(*this, COMMITMONITOR_SHOWDLGMSG, 0, 0);
}

LRESULT CHiddenWindow::HandleCustomMessages(HWND /*hwnd*/, UINT uMsg, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (uMsg == COMMITMONITOR_SHOWDLGMSG)
	{
		if (m_bMainDlgShown)
        {
            // bring the dialog to front
			if (m_hMainDlg == NULL)
				m_hMainDlg = FindWindow(NULL, _T("Commit Monitor"));
            SetWindowPos(m_hMainDlg, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
            SetForegroundWindow(m_hMainDlg);
			return TRUE;
        }
        m_bMainDlgShown = true;
		m_bMainDlgRemovedItems = false;
		CMainDlg dlg(*this);
		dlg.SetUrlInfos(&m_UrlInfos);
		dlg.DoModal(hInst, IDD_MAINDLG, NULL);
		m_hMainDlg = NULL;
		m_UrlInfos.Save();
		m_bMainDlgShown = false;
		return TRUE;
	}
	else if (uMsg == WM_TASKBARCREATED)
	{
		bool bNew = m_SystemTray.hIcon == m_hIconNew1;
		m_SystemTray.hIcon = NULL;
		TRACE(_T("Taskbar created!\n"));
		ShowTrayIcon(bNew);
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
			switch (wParam)
			{
			case IDT_MONITOR:
				DoTimer(false);
				break;
			case IDT_ANIMATE:
				DoAnimate();
				break;
			}
		}
		break;
	case COMMITMONITOR_GETALL:
		DoTimer(true);
		break;
	case COMMITMONITOR_POPUP:
		{
			popupData * pData = (popupData*)lParam;
			if (pData)
			{
				CStatusBarMsgWnd * popup = new CStatusBarMsgWnd(hResource);
				popup->Show(pData->sTitle.c_str(), pData->sText.c_str(), IDI_COMMITMONITOR, *this, COMMITMONITOR_POPUPCLICK);
                ShowTrayIcon(true);
			}
		}
		break;
    case COMMITMONITOR_POPUPCLICK:
        ShowDialog();
        break;
	case COMMITMONITOR_SAVEINFO:
		{
			wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
			m_UrlInfos.Save(urlfile.c_str());
			return TRUE;
		}
		break;
	case COMMITMONITOR_REMOVEDURL:
		{
			m_bMainDlgRemovedItems = true;
		}
		break;
	case COMMITMONITOR_CHANGEDINFO: 		    
		ShowTrayIcon(!!wParam);
		return TRUE;
	case COMMITMONITOR_TASKBARCALLBACK:
		{
			switch (lParam)
			{
			case WM_MOUSEMOVE:
				{
					// find the number of unread items
					int nNewCommits = 0;
					const map<wstring, CUrlInfo> * pRead = m_UrlInfos.GetReadOnlyData();
					for (map<wstring, CUrlInfo>::const_iterator it = pRead->begin(); it != pRead->end(); ++it)
					{
						for (map<svn_revnum_t,SVNLogEntry>::const_iterator logit = it->second.logentries.begin(); logit != it->second.logentries.end(); ++logit)
						{
							if (!logit->second.read)
								nNewCommits++;
						}
					}
					m_UrlInfos.ReleaseReadOnlyData();
					// update the tool tip data
					m_SystemTray.cbSize = sizeof(NOTIFYICONDATA);
					m_SystemTray.hWnd   = *this;
					m_SystemTray.uID    = 1;
					m_SystemTray.uFlags = NIF_TIP;
					if (nNewCommits)
					{
						if (nNewCommits == 1)
							_stprintf_s(m_SystemTray.szTip, sizeof(m_SystemTray.szTip)/sizeof(TCHAR), _T("CommitMonitor - %d new commit"), nNewCommits);
						else
							_stprintf_s(m_SystemTray.szTip, sizeof(m_SystemTray.szTip)/sizeof(TCHAR), _T("CommitMonitor - %d new commits"), nNewCommits);
					}
					else
						_tcscpy_s(m_SystemTray.szTip, sizeof(m_SystemTray.szTip)/sizeof(TCHAR), _T("CommitMonitor"));
					Shell_NotifyIcon(NIM_MODIFY, &m_SystemTray);
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
		break;
	case COMMITMONITOR_SETWINDOWHANDLE:
		m_hMainDlg = (HWND)wParam;
		break;
	case COMMITMONITOR_INFOTEXT:
		if (m_hMainDlg)
			SendMessage(m_hMainDlg, COMMITMONITOR_INFOTEXT, 0, lParam);
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

void CHiddenWindow::DoTimer(bool bForce)
{
	TRACE(_T("timer fired\n"));
	// Restart the timer with 60 seconds
	::SetTimer(*this, IDT_MONITOR, TIMER_ELAPSE, NULL);

	if (m_ThreadRunning)
	{
		if (m_hMainDlg)
			SendMessage(m_hMainDlg, COMMITMONITOR_INFOTEXT, 0, (LPARAM)_T("Repositories are currently checked, please wait..."));
		return;
	}
	// go through all url infos and check if
	// we need to refresh them
	if (m_UrlInfos.IsEmpty())
	{
		if (m_bIsTask)
			::PostQuitMessage(0);
		return;
	}
	bool bStartThread = false;
	__time64_t currenttime = NULL;
	_time64(&currenttime);

	if (bForce)
	{
		// reset the 'last checked times' of all urls
		map<wstring,CUrlInfo> * pInfos = m_UrlInfos.GetWriteData();
		for (map<wstring,CUrlInfo>::iterator it = pInfos->begin(); it != pInfos->end(); ++it)
		{
			it->second.lastchecked = 0;
		}
		m_UrlInfos.ReleaseWriteData();
		m_UrlInfos.Save();
	}

	const map<wstring,CUrlInfo> * pInfos = m_UrlInfos.GetReadOnlyData();
	for (map<wstring,CUrlInfo>::const_iterator it = pInfos->begin(); it != pInfos->end(); ++it)
	{
		if ((it->second.lastchecked + (it->second.minutesinterval*60)) < currenttime)
		{
			bStartThread = true;
			break;
		}
	}
	m_UrlInfos.ReleaseReadOnlyData();

	if ((bStartThread)&&(m_ThreadRunning == 0))
	{
		// start the monitoring thread to update the infos
		if (m_hMonitorThread)
		{
			CloseHandle(m_hMonitorThread);
			m_hMonitorThread = NULL;
		}
		DWORD dwThreadId = 0;
		m_hMonitorThread = CreateThread( 
			NULL,              // no security attribute 
			0,                 // default stack size 
			MonitorThread, 
			(LPVOID)this,      // thread parameter 
			0,                 // not suspended 
			&dwThreadId);      // returns thread ID 

		if (m_hMonitorThread == NULL) 
			return;
	}
	SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
}

void CHiddenWindow::ShowTrayIcon(bool newCommits)
{
	TRACE(_T("changing tray icon to %s\n"), (newCommits ? _T("\"new commits\"") : _T("\"normal\"")));

	DWORD msg = m_SystemTray.hIcon ? NIM_MODIFY : NIM_ADD;
	if ((!newCommits)&&(regShowTaskbarIcon.read() == FALSE))
	{
		m_SystemTray.hIcon = NULL;
		msg = NIM_DELETE;
	}
	m_SystemTray.cbSize = sizeof(NOTIFYICONDATA);
	m_SystemTray.hWnd   = *this;
	m_SystemTray.uID    = 1;
	m_SystemTray.hIcon  = newCommits ? m_hIconNew1 : m_hIconNormal;
	m_SystemTray.uFlags = NIF_MESSAGE | NIF_ICON;
	m_SystemTray.uCallbackMessage = COMMITMONITOR_TASKBARCALLBACK;
	Shell_NotifyIcon(msg, &m_SystemTray);
	m_nIcon = 2;
	if ((!newCommits)&&(DWORD(regShowTaskbarIcon) == FALSE))
	{
		m_SystemTray.hIcon = NULL;
	}
	if ((newCommits)&&(m_SystemTray.hIcon)&&(DWORD(CRegStdWORD(_T("Software\\CommitMonitor\\Animate"), TRUE))))
		SetTimer(*this, IDT_ANIMATE, TIMER_ANIMATE, NULL);
	else
		KillTimer(*this, IDT_ANIMATE);
}

void CHiddenWindow::DoAnimate()
{
	m_SystemTray.cbSize = sizeof(NOTIFYICONDATA);
	m_SystemTray.hWnd   = *this;
	m_SystemTray.uID    = 1;
	switch (m_nIcon)
	{
	case 0:
		m_SystemTray.hIcon  = m_hIconNew0;
		break;
	default:
	case 1:
	case 5:
		m_SystemTray.hIcon  = m_hIconNew1;
		break;
	case 2:
	case 4:
		m_SystemTray.hIcon  = m_hIconNew2;
		break;
	case 3:
		m_SystemTray.hIcon  = m_hIconNew3;
		break;
	}
	m_SystemTray.uFlags = NIF_MESSAGE | NIF_ICON;
	m_SystemTray.uCallbackMessage = COMMITMONITOR_TASKBARCALLBACK;
	Shell_NotifyIcon(NIM_MODIFY, &m_SystemTray);
	m_nIcon++;
	if (m_nIcon >= 6)
		m_nIcon = 0;
}

DWORD CHiddenWindow::RunThread()
{
	m_ThreadRunning = TRUE;
	m_bRun = true;
	__time64_t currenttime = NULL;
	_time64(&currenttime);
	bool bNewEntries = false;

	if (::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)!=S_OK)
	{
		SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
		return 1;
	}

	// load a copy of the url data
	CUrlInfos urlinfoReadOnly;
	wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
	if (!PathFileExists(urlfile.c_str()))
	{
		if (m_bIsTask)
			::PostQuitMessage(0);
		SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
		return 0;
	}
	TCHAR infotextbuf[1024];
	urlinfoReadOnly.Load(urlfile.c_str());
	TRACE(_T("monitor thread started\n"));
	const map<wstring,CUrlInfo> * pUrlInfoReadOnly = urlinfoReadOnly.GetReadOnlyData();
	map<wstring,CUrlInfo>::const_iterator it = pUrlInfoReadOnly->begin();
	for (; (it != pUrlInfoReadOnly->end()) && m_bRun; ++it)
	{
		int mit = max(it->second.minutesinterval, it->second.minminutesinterval);
		SendMessage(*this, COMMITMONITOR_INFOTEXT, 0, (LPARAM)_T(""));
		if ((it->second.lastchecked + (mit*60)) < currenttime)
		{
			TRACE(_T("checking %s for updates\n"), it->first.c_str());
			// get the highest revision of the repository
			SVN svn;
			svn.SetAuthInfo(it->second.username, it->second.password);
			if (m_hMainDlg)
			{
				_stprintf_s(infotextbuf, 1024, _T("checking %s ..."), it->first.c_str());
				SendMessage(*this, COMMITMONITOR_INFOTEXT, 0, (LPARAM)infotextbuf);
			}
			svn_revnum_t headrev = svn.GetHEADRevision(it->first);
			if (headrev > it->second.lastcheckedrev)
			{
				TRACE(_T("%s has updates! Last checked revision was %ld, HEAD revision is %ld\n"), it->first.c_str(), it->second.lastcheckedrev, headrev);
				if (m_hMainDlg)
				{
					_stprintf_s(infotextbuf, 1024, _T("getting log for %s"), it->first.c_str());
					SendMessage(*this, COMMITMONITOR_INFOTEXT, 0, (LPARAM)infotextbuf);
				}
				if (svn.GetLog(it->first, headrev, it->second.lastcheckedrev + 1))
				{
					TRACE(_T("log fetched for %s\n"), it->first.c_str());
					
					// only block the object for a short time
					map<wstring,CUrlInfo> * pWrite = m_UrlInfos.GetWriteData();
					map<wstring,CUrlInfo>::iterator writeIt = pWrite->find(it->first);
					if (writeIt != pWrite->end())
					{
						writeIt->second.lastcheckedrev = headrev;
						writeIt->second.lastchecked = currenttime;
					}
					m_UrlInfos.ReleaseWriteData();

					wstring sPopupText;
					int nNewCommits = 0;
					for (map<svn_revnum_t,SVNLogEntry>::const_iterator logit = svn.m_logs.begin(); logit != svn.m_logs.end(); ++logit)
					{
						// again, only block for a short time
						map<wstring,CUrlInfo> * pWrite = m_UrlInfos.GetWriteData();
						map<wstring,CUrlInfo>::iterator writeIt = pWrite->find(it->first);
						if (writeIt != pWrite->end())
						{
							writeIt->second.logentries[logit->first] = logit->second;
							if ((!writeIt->second.ignoreSelf)||(logit->second.author.compare(writeIt->second.username)))
							{
								bNewEntries = true;
								nNewCommits++;
							}
							else
								// set own commit as already read
								writeIt->second.logentries[logit->first].read = true;
							writeIt->second.error.clear();
						}
						m_UrlInfos.ReleaseWriteData();
						TCHAR buf[4096];
						// popup info text
						if (!sPopupText.empty())
							sPopupText += _T(", ");
						sPopupText += logit->second.author;
						if ((!it->second.disallowdiffs)&&(it->second.fetchdiffs))
						{
							// first, find a name where to store the diff for that revision
							_stprintf_s(buf, 4096, _T("%s_%ld"), it->second.name.c_str(), logit->first);
							wstring diffFileName = CAppUtils::GetAppDataDir();
							diffFileName += _T("/");
							diffFileName += wstring(buf);
							// do we already have that diff?
							if (!PathFileExists(diffFileName.c_str()))
							{
								// get the diff
								if (m_hMainDlg)
								{
									_stprintf_s(infotextbuf, 1024, _T("getting diff for %s, revision %ld"), it->first.c_str(), logit->first);
									SendMessage(*this, COMMITMONITOR_INFOTEXT, 0, (LPARAM)infotextbuf);
								}
								if (!svn.Diff(it->first, logit->first, logit->first-1, logit->first, true, true, false, wstring(), false, diffFileName, wstring()))
								{
									TRACE(_T("Diff not fetched for %s, revision %ld because of an error\n"), it->first.c_str(), logit->first);
									DeleteFile(diffFileName.c_str());
								}
								else
									TRACE(_T("Diff fetched for %s, revision %ld\n"), it->first.c_str(), logit->first);
								if (!m_bRun)
									break;
							}
						}
						if ((it->second.lastcheckedrobots + (60*60*24*2)) < currenttime)
						{
							wstring sRobotsURL = it->first;
							sRobotsURL += _T("/svnrobots.txt");
							wstring sRootRobotsURL;
							wstring sDomainRobotsURL = sRobotsURL.substr(0, sRobotsURL.find('/', sRobotsURL.find(':')+3))+ _T("/svnrobots.txt");
							const SVNInfoData * data = svn.GetFirstFileInfo(it->first, headrev, headrev);
							if (data)
							{
								sRootRobotsURL = data->reposRoot;
								sRootRobotsURL += _T("/svnrobots.txt");
							}
							wstring sFile = CAppUtils::GetTempFilePath();
							string in;
							CCallback * callback = new CCallback;
							callback->SetAuthData(it->second.username, it->second.password);
							if ((!sDomainRobotsURL.empty())&&(URLDownloadToFile(NULL, sDomainRobotsURL.c_str(), sFile.c_str(), 0, callback) == S_OK))
							{
								ifstream fs(sFile.c_str());
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
								}
							}
							else if ((!sRootRobotsURL.empty())&&(svn.Cat(sRootRobotsURL, sFile)))
							{
								ifstream fs(sFile.c_str());
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
									fs.close();
								}
							}
							else if (svn.Cat(sRobotsURL, sFile))
							{
								ifstream fs(sFile.c_str());
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
									fs.close();
								}
							}
							delete callback;
							DeleteFile(sFile.c_str());
							// the format of the svnrobots.txt file is as follows:
							// # comment
							// disallowautodiff
							// checkinterval = XXX
							//
							// with 'checkinterval' being the minimum amount of time to wait
							// between checks in minutes.

							istringstream iss(in);
							string line;
							int minutes = 0;
							bool disallowdiffs = false;
							while (getline(iss, line)) 
							{
								if (line.length())
								{
									if (line.at(0) != '#')
									{
										if (line.compare("disallowautodiff") == 0)
										{
											disallowdiffs = true;
										}
										else if ((line.length() > 13) && (line.substr(0, 13).compare("checkinterval") == 0))
										{
											string num = line.substr(line.find('=')+1);
											minutes = atoi(num.c_str());
										}
									}
								}
							}
							// again, only block for a short time
							map<wstring,CUrlInfo> * pWrite = m_UrlInfos.GetWriteData();
							map<wstring,CUrlInfo>::iterator writeIt = pWrite->find(it->first);
							if (writeIt != pWrite->end())
							{
								writeIt->second.lastcheckedrobots = currenttime;
								writeIt->second.disallowdiffs = disallowdiffs;
								writeIt->second.minminutesinterval = minutes;
								bNewEntries = true;
							}
							m_UrlInfos.ReleaseWriteData();
						}
					}
					// prepare notification strings
					if (bNewEntries)
					{
						TCHAR sTitle[1024] = {0};
						if (nNewCommits == 1)
							_stprintf_s(sTitle, 1024, _T("%s\nhas %d new commit"), it->second.name.c_str(), nNewCommits);
						else
							_stprintf_s(sTitle, 1024, _T("%s\nhas %d new commits"), it->second.name.c_str(), nNewCommits);
						popupData data;
						data.sText = sPopupText;
						data.sTitle = wstring(sTitle);
						::SendMessage(*this, COMMITMONITOR_POPUP, 0, (LPARAM)&data);
					}
					bNewEntries = false;
				}
				else
				{
					// only block the object for a short time
					map<wstring,CUrlInfo> * pWrite = m_UrlInfos.GetWriteData();
					map<wstring,CUrlInfo>::iterator writeIt = pWrite->find(it->first);
					if (writeIt != pWrite->end())
					{
						writeIt->second.lastchecked = currenttime;
						writeIt->second.error = svn.GetLastErrorMsg();
					}
					m_UrlInfos.ReleaseWriteData();
				}
			}
			else if (headrev > 0)
			{
				// only block the object for a short time
				map<wstring,CUrlInfo> * pWrite = m_UrlInfos.GetWriteData();
				map<wstring,CUrlInfo>::iterator writeIt = pWrite->find(it->first);
				if (writeIt != pWrite->end())
				{
					writeIt->second.lastchecked = currenttime;
					writeIt->second.error = svn.GetLastErrorMsg();
				}
				m_UrlInfos.ReleaseWriteData();
				if (m_hMainDlg)
				{
					_stprintf_s(infotextbuf, 1024, _T("no new commits for %s"), it->first.c_str());
					SendMessage(*this, COMMITMONITOR_INFOTEXT, 0, (LPARAM)infotextbuf);
				}
			}
			else
			{
				// only block the object for a short time
				map<wstring,CUrlInfo> * pWrite = m_UrlInfos.GetWriteData();
				map<wstring,CUrlInfo>::iterator writeIt = pWrite->find(it->first);
				if (writeIt != pWrite->end())
				{
					writeIt->second.lastchecked = currenttime;
					writeIt->second.error = svn.GetLastErrorMsg();
				}
				m_UrlInfos.ReleaseWriteData();

				// if we can't fetch the HEAD revision, it might be because the URL points to an SVNParentPath
				// instead of pointing to an actual repository.

				// we have to include the authentication in the URL itself
				wstring tempfile = CAppUtils::GetTempFilePath();
				CCallback * callback = new CCallback;
				callback->SetAuthData(it->second.username, it->second.password);
				DeleteFile(tempfile.c_str());
				wstring projName = it->second.name;
                wstring parentpathurl = it->first;
                wstring parentpathurl2 = parentpathurl + _T("/");
                HRESULT hResUDL = URLDownloadToFile(NULL, parentpathurl2.c_str(), tempfile.c_str(), 0, callback);
                if (hResUDL != S_OK)
                {
                    hResUDL = URLDownloadToFile(NULL, parentpathurl.c_str(), tempfile.c_str(), 0, callback);
                }
				if (hResUDL == S_OK)
				{
					if (callback)
					{
						delete callback;
						callback = NULL;
					}
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
						fs.close();
						DeleteFile(tempfile.c_str());

						// make sure this is a html page from an SVNParentPathList
						// we do this by checking for header titles looking like
						// "<h2>Revision XX: /</h2> - if we find that, it's a html
						// page from inside a repository
                        const char * reTitle = "<\\s*h2\\s*>[^/]+/\\s*<\\s*/\\s*h2\\s*>";
                        // xsl transformed pages don't have an easy way to determine
                        // the inside from outside of a repository.
                        // We therefore check for <index rev="0" to make sure it's either
                        // an empty repository or really an SVNParentPathList
                        const char * reTitle2 = "<\\s*index\\s*rev\\s*=\\s*\"0\"";
                        regex titex = regex(reTitle, regex::normal | regbase::icase);
                        regex titex2 = regex(reTitle2, regex::normal | regbase::icase);
						string::const_iterator start = in.begin();
						string::const_iterator end = in.end();
						match_results<string::const_iterator> fwhat;
						if (regex_search(start, end, fwhat, titex, match_default))
						{
							TRACE(_T("found repository url instead of SVNParentPathList\n"));
							continue;
						}

						const char * re = "<\\s*LI\\s*>\\s*<\\s*A\\s+[^>]*HREF\\s*=\\s*\"([^\"]*)\"\\s*>([^<]+)<\\s*/\\s*A\\s*>\\s*<\\s*/\\s*LI\\s*>";
						const char * re2 = "<\\s*DIR\\s*name\\s*=\\s*\"([^\"]*)\"\\s*HREF\\s*=\\s*\"([^\"]*)\"\\s*/\\s*>";

						regex expression = regex(re, regex::normal | regbase::icase);
						regex expression2 = regex(re2, regex::normal | regbase::icase);
						start = in.begin();
						end = in.end();
						match_results<string::const_iterator> what;
						match_flag_type flags = match_default;
						bool hasNewEntries = false;
						int nCountNewEntries = 0;
						wstring popupText;
						while (regex_search(start, end, what, expression, flags))	
						{
							// what[0] contains the whole string
							// what[1] contains the url part.
							// what[2] contains the name
							wstring url = CUnicodeUtils::StdGetUnicode(string(what[1].first, what[1].second));
							url = it->first + _T("/") + url;
							url = svn.CanonicalizeURL(url);

							map<wstring,CUrlInfo> * pWrite = m_UrlInfos.GetWriteData();
							map<wstring,CUrlInfo>::iterator writeIt = pWrite->find(url);
							if ((!m_bMainDlgRemovedItems)&&(writeIt == pWrite->end()))
							{
								// we found a new URL, add it to our list
								CUrlInfo newinfo;
								newinfo.url = url;
								newinfo.name = CUnicodeUtils::StdGetUnicode(string(what[2].first, what[2].second));
								newinfo.name.erase(newinfo.name.find_last_not_of(_T("/ ")) + 1);
								newinfo.username = it->second.username;
								newinfo.password = it->second.password;
								newinfo.fetchdiffs = it->second.fetchdiffs;
								newinfo.minutesinterval = it->second.minutesinterval;
								(*pWrite)[url] = newinfo;
								hasNewEntries = true;
								nCountNewEntries++;
								if (!popupText.empty())
									popupText += _T(", ");
								popupText += newinfo.name;
							}
							writeIt = pWrite->find(it->first);
							if (writeIt != pWrite->end())
								writeIt->second.parentpath = true;
							m_UrlInfos.ReleaseWriteData();

							// update search position:
							start = what[0].second;		 
							// update flags:
							flags |= match_prev_avail;
							flags |= match_not_bob;
						}
						start = in.begin();
						end = in.end();
                        if (!regex_search(start, end, fwhat, titex2, match_default))
                        {
                            TRACE(_T("found repository url instead of SVNParentPathList\n"));
                            continue;
                        }
						while (regex_search(start, end, what, expression2, flags))	 
						{
							// what[0] contains the whole string
							// what[1] contains the url part.
							// what[2] contains the name
							wstring url = CUnicodeUtils::StdGetUnicode(string(what[1].first, what[1].second));
							url = it->first + _T("/") + url;
							url = svn.CanonicalizeURL(url);

							map<wstring,CUrlInfo> * pWrite = m_UrlInfos.GetWriteData();
							map<wstring,CUrlInfo>::iterator writeIt = pWrite->find(url);
							if ((!m_bMainDlgRemovedItems)&&(writeIt == pWrite->end()))
							{
								// we found a new URL, add it to our list
								CUrlInfo newinfo;
								newinfo.url = url;
								newinfo.name = CUnicodeUtils::StdGetUnicode(string(what[2].first, what[2].second));
								newinfo.name.erase(newinfo.name.find_last_not_of(_T("/ ")) + 1);
								newinfo.username = it->second.username;
								newinfo.password = it->second.password;
								newinfo.fetchdiffs = it->second.fetchdiffs;
								newinfo.minutesinterval = it->second.minutesinterval;
								(*pWrite)[url] = newinfo;
								hasNewEntries = true;
								nCountNewEntries++;
								if (!popupText.empty())
									popupText += _T(", ");
								popupText += newinfo.name;
							}
                            writeIt = pWrite->find(it->first);
                            if (writeIt != pWrite->end())
                                writeIt->second.parentpath = true;
							m_UrlInfos.ReleaseWriteData();

							// update search position:
							start = what[0].second;		 
							// update flags:
							flags |= match_prev_avail;
							flags |= match_not_bob;
						}
						if (hasNewEntries)
						{
							it = pUrlInfoReadOnly->begin();
							TCHAR popupTitle[1024] = {0};
							_stprintf_s(popupTitle, 1024, _T("%s\nhas %d new projects"), projName.c_str(), nCountNewEntries);
							popupData data;
							data.sText = popupText;
							data.sTitle = wstring(popupTitle);
							::SendMessage(*this, COMMITMONITOR_POPUP, 0, (LPARAM)&data);
                            bNewEntries = false;
						}
					}
				}
				if (callback)
				{
					delete callback;
					callback = NULL;
				}
				DeleteFile(tempfile.c_str());
			}
		}
	}
	SendMessage(*this, COMMITMONITOR_INFOTEXT, 0, (LPARAM)_T(""));
	// save the changed entries
	::PostMessage(*this, COMMITMONITOR_SAVEINFO, (WPARAM)true, (LPARAM)0);
	if (bNewEntries)
		::PostMessage(*this, COMMITMONITOR_CHANGEDINFO, (WPARAM)true, (LPARAM)0);

	if ((!bNewEntries)&&(m_bIsTask))
		::PostQuitMessage(0);

    urlinfoReadOnly.ReleaseReadOnlyData();
	TRACE(_T("monitor thread ended\n"));
	m_bMainDlgRemovedItems = false;
	m_ThreadRunning = FALSE;

	SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);

	::CoUninitialize();
	return 0L;
}

DWORD WINAPI MonitorThread(LPVOID lpParam)
{
	CHiddenWindow * pThis = (CHiddenWindow*)lpParam;
	if (pThis)
		return pThis->RunThread();
	SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
	return 0L;
}

void CHiddenWindow::StopThread()
{
	m_bRun = false;
	if (m_hMonitorThread)
	{
		WaitForSingleObject(m_hMonitorThread, 2000);
		if (m_ThreadRunning)
		{
			// we gave the thread a chance to quit. Since the thread didn't
			// listen to us we have to kill it.
			TerminateThread(m_hMonitorThread, (DWORD)-1);
			m_ThreadRunning = false;
		}
		CloseHandle(m_hMonitorThread);
		m_hMonitorThread = NULL;
	}
}
