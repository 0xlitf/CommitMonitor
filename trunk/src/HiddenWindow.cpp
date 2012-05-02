// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007-2012 - Stefan Kueng

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

#include "stdafx.h"
#include <fstream>
#include <sstream>
#include <Urlmon.h>
#pragma comment(lib, "Urlmon.lib")

#include "HiddenWindow.h"
#include "resource.h"
#include "MainDlg.h"
#include "SCCS.h"
#include "SVN.h"
#include "Accurev.h"
#include "Callback.h"
#include "AppUtils.h"
#include "StatusBarMsgWnd.h"
#include "OptionsDlg.h"
#include "version.h"
#include "SnarlInterface.h"

#include <cctype>
#include <regex>
#include <set>
using namespace std;

// for Vista
#define MSGFLT_ADD 1


extern HINSTANCE hInst;

CHiddenWindow *hiddenWindowPointer = NULL;

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
    , m_regLastSelectedProject(_T("Software\\CommitMonitor\\LastSelectedProject"))
    , m_bIsTask(false)
    , m_bNewerVersionAvailable(false)
{
    m_hIconNew0 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NOTIFYNEW0));
    m_hIconNew1 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NOTIFYNEW1));
    m_hIconNew2 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NOTIFYNEW2));
    m_hIconNew3 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NOTIFYNEW3));
    m_hIconNormal = LoadIcon(hInst, MAKEINTRESOURCE(IDI_COMMITMONITOR));
    ZeroMemory(&m_SystemTray, sizeof(m_SystemTray));

    hiddenWindowPointer = this;
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

void CHiddenWindow::RemoveTrayIcon()
{
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
    wcx.hIconSm = LoadIcon(wcx.hInstance, MAKEINTRESOURCE(IDI_COMMITMONITOR));
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
            Snarl::SnarlInterface snarlIface;
            snarlGlobalMsg = snarlIface.GetGlobalMsg();
            return true;
        }
    }
    return false;
}

INT_PTR CHiddenWindow::ShowDialog()
{
    return ::SendMessage(*this, COMMITMONITOR_SHOWDLGMSG, 0, 0);
}

LRESULT CHiddenWindow::HandleCustomMessages(HWND /*hwnd*/, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    if (uMsg == COMMITMONITOR_SHOWDLGMSG)
    {
        if (m_bMainDlgShown)
        {
            // bring the dialog to front
            if (m_hMainDlg == NULL)
                m_hMainDlg = FindWindow(NULL, _T("Commit Monitor"));
            if (!CAppUtils::IsWindowCovered(m_hMainDlg))
                SendMessage(m_hMainDlg, WM_CLOSE, 0, 0);
            else
            {
                if (IsIconic(m_hMainDlg))
                    ShowWindow(m_hMainDlg, SW_RESTORE);
                else
                    SetWindowPos(m_hMainDlg, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
                SetForegroundWindow(m_hMainDlg);
            }
            return TRUE;
        }
        m_bMainDlgShown = true;
        m_bMainDlgRemovedItems = false;
        CMainDlg dlg(*this);
        dlg.SetLastSelectedProject(m_regLastSelectedProject);
        dlg.SetUrlInfos(&m_UrlInfos);
        dlg.SetUpdateAvailable(m_bNewerVersionAvailable);
        dlg.DoModal(hInst, IDD_MAINDLG, NULL, IDC_COMMITMONITOR);
        m_regLastSelectedProject = dlg.GetLastSelectedProject();
        m_bNewerVersionAvailable = false;
        ShowTrayIcon(false);
        m_hMainDlg = NULL;
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
    else if (uMsg == (UINT)snarlGlobalMsg)
    {
        if (wParam == Snarl::SNARL_LAUNCHED)
        {
            Snarl::SnarlInterface snarlIface;
            if ((snarlIface.GetVersionEx() != Snarl::M_FAILED)&&(Snarl::SnarlInterface::GetSnarlWindow() != NULL))
            {
                wstring imgPath = CAppUtils::GetAppDataDir()+L"\\CM.png";
                if (CAppUtils::ExtractBinResource(_T("PNG"), IDB_COMMITMONITOR, imgPath))
                {
                    // register with Snarl
                    snarlIface.RegisterApp(_T("CommitMonitor"), imgPath.c_str(), imgPath.c_str(), *this);
                    snarlIface.RegisterAlert(_T("CommitMonitor"), ALERTTYPE_NEWPROJECTS);
                    snarlIface.RegisterAlert(_T("CommitMonitor"), ALERTTYPE_NEWCOMMITS);
                    snarlIface.RegisterAlert(_T("CommitMonitor"), ALERTTYPE_FAILEDCONNECT);
                }
            }
        }
        if (wParam == Snarl::SNARL_SHOW_APP_UI)
        {
            SendMessage(*this, COMMITMONITOR_SHOWDLGMSG, 0, 0);
        }
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
            // we wait a minute before starting the initial monitoring
            // to avoid problems after booting up
            // See issue #63 for details:
            // http://code.google.com/p/commitmonitor/issues/detail?id=63
            ::SetTimer(*this, IDT_MONITOR, 60000, NULL);
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
            case IDT_POPUP:
                {
                    if (CAppUtils::IsFullscreenWindowActive())
                    {
                        // restart the timer and wait until no fullscreen app is active
                        SetTimer(hwnd, IDT_POPUP, 5000, NULL);
                    }
                    else
                    {
                        CStatusBarMsgWnd * popup = new CStatusBarMsgWnd(hResource);
                        if (m_popupData.size() == 1)
                        {
                            popup->Show(m_popupData[0].sTitle.c_str(), m_popupData[0].sText.c_str(), IDI_COMMITMONITOR, *this, COMMITMONITOR_POPUPCLICK);
                        }
                        else
                        {
                            // only show one popup for all the notifications
                            TCHAR sTitle[1024] = {0};
                            _stprintf_s(sTitle, 1024, _T("%d projects have updates"), m_popupData.size());
                            std::wstring sText;
                            for (std::vector<popupData>::const_iterator it = m_popupData.begin(); it != m_popupData.end(); ++it)
                            {
                                if (sText.size())
                                    sText += _T(", ");
                                sText += it->sProject;
                            }
                            popup->Show(sTitle, sText.c_str(), IDI_COMMITMONITOR, *this, COMMITMONITOR_POPUPCLICK);
                        }
                        m_popupData.clear();
                        KillTimer(hwnd, IDT_POPUP);
                    }
                }
                break;
            }
        }
        break;
    case WM_POWERBROADCAST:
        {
            switch (wParam)
            {
            case PBT_APMRESUMEAUTOMATIC:
            case PBT_APMRESUMESUSPEND:
            case PBT_APMRESUMECRITICAL:
                // waking up again
                // we wait a minute before starting the initial monitoring
                // to avoid problems after booting up
                // See issue #63 for details:
                // http://code.google.com/p/commitmonitor/issues/detail?id=63
                ::SetTimer(*this, IDT_MONITOR, 60000, NULL);
                break;
            case PBT_APMSUSPEND:
                // going to sleep
                ::KillTimer(*this, IDT_MONITOR);
                break;
            }
        }
        break;
    case COMMITMONITOR_GETALL:
        if (lParam)
            m_UrlToWorkOn = wstring((wchar_t*)lParam);
        DoTimer(true);
        break;
    case COMMITMONITOR_POPUP:
        {
            popupData * pData = (popupData*)lParam;
            if (pData)
            {
                Snarl::SnarlInterface snarlIface;
                if ((snarlIface.GetVersionEx() == Snarl::M_FAILED)||(Snarl::SnarlInterface::GetSnarlWindow() == NULL))
                {
                    if (CRegStdDWORD(L"Software\\CommitMonitor\\ShowPopups", TRUE))
                    {
                        m_popupData.push_back(*pData);
                        SetTimer(hwnd, IDT_POPUP, 5000, NULL);
                    }
                }
                else
                {
                    wstring iconPath = CAppUtils::GetAppDataDir()+L"\\CM.png";
                    if (!PathFileExists(iconPath.c_str()))
                        iconPath = _T("");
                    snarlIface.ShowNotification(pData->sAlertType.c_str(), pData->sTitle.c_str(), pData->sText.c_str(), 5, iconPath.c_str(), *this, COMMITMONITOR_POPUPCLICK);
                }
                ShowTrayIcon(true);
            }
        }
        break;
    case COMMITMONITOR_POPUPCLICK:
        if ((wParam != Snarl::SNARL_NOTIFICATION_TIMED_OUT)&&(wParam != Snarl::SNARL_NOTIFICATION_CLOSED))
            ShowDialog();
        break;
    case COMMITMONITOR_SAVEINFO:
        {
            m_UrlInfos.Save();
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
                        for (map<svn_revnum_t,SCCSLogEntry>::const_iterator logit = it->second.logentries.begin(); logit != it->second.logentries.end(); ++logit)
                        {
                            if (!logit->second.read)
                                nNewCommits++;
                        }
                    }
                    m_UrlInfos.ReleaseReadOnlyData();
                    // update the tool tip data
                    m_SystemTray.cbSize = sizeof(NOTIFYICONDATA);
                    m_SystemTray.hWnd   = *this;
                    m_SystemTray.uFlags = NIF_MESSAGE | NIF_TIP;
                    m_SystemTray.uCallbackMessage = COMMITMONITOR_TASKBARCALLBACK;
                    if (nNewCommits)
                    {
                        if (nNewCommits == 1)
                            _stprintf_s(m_SystemTray.szTip, _countof(m_SystemTray.szTip), _T("CommitMonitor - %d new commit"), nNewCommits);
                        else
                            _stprintf_s(m_SystemTray.szTip, _countof(m_SystemTray.szTip), _T("CommitMonitor - %d new commits"), nNewCommits);
                    }
                    else
                        _tcscpy_s(m_SystemTray.szTip, _countof(m_SystemTray.szTip), _T("CommitMonitor"));
                    if (Shell_NotifyIcon(NIM_MODIFY, &m_SystemTray) == FALSE)
                    {
                        Shell_NotifyIcon(NIM_DELETE, &m_SystemTray);
                        Shell_NotifyIcon(NIM_ADD, &m_SystemTray);
                    }
                }
                break;
            case WM_LBUTTONDBLCLK:
                {
                    if (CRegStdDWORD(_T("Software\\CommitMonitor\\LeftClickMenu"), FALSE) != 0)
                    {
                        // show the main dialog
                        ShowDialog();
                    }
                }
                break;
            case WM_LBUTTONDOWN:
                if (CRegStdDWORD(_T("Software\\CommitMonitor\\LeftClickMenu"), FALSE) == 0)
                {
                    // show the main dialog
                    ShowDialog();
                    return TRUE;
                }
                break;
            case WM_LBUTTONUP:
            case NIN_KEYSELECT:
            case NIN_SELECT:
            case WM_RBUTTONUP:
            case WM_CONTEXTMENU:
                {
                    if (lParam == WM_LBUTTONUP)
                    {
                        if (CRegStdDWORD(_T("Software\\CommitMonitor\\LeftClickMenu"), FALSE) == 0)
                        {
                            return TRUE;
                        }
                    }
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

                    // destroy all popup windows
                    HWND hPopup = FindWindow(L"StatusBarMsgWnd_{BAB03407-CF65-4942-A1D5-063FA1CA8530}", NULL);
                    while (hPopup)
                    {
                        DestroyWindow(hPopup);
                        hPopup = FindWindow(L"StatusBarMsgWnd_{BAB03407-CF65-4942-A1D5-063FA1CA8530}", NULL);
                    }
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
                    case ID_POPUP_CHECKNOW:
                        DoTimer(true);
                        break;
                    case ID_POPUP_OPTIONS:
                        {
                            COptionsDlg dlg(*this);
                            dlg.SetHiddenWnd(*this);
                            dlg.SetUrlInfos(&m_UrlInfos);
                            dlg.DoModal(hResource, IDD_OPTIONS, *this);
                        }
                        break;
                    case ID_POPUP_MARKALLASREAD:
                        {
                            map<wstring,CUrlInfo> * pWrite = m_UrlInfos.GetWriteData();
                            for (map<wstring, CUrlInfo>::iterator it = pWrite->begin(); it != pWrite->end(); ++it)
                            {
                                CUrlInfo * pInfo = &it->second;
                                for (map<svn_revnum_t,SCCSLogEntry>::iterator logit = pInfo->logentries.begin(); logit != pInfo->logentries.end(); ++logit)
                                {
                                    logit->second.read = true;
                                }
                            }
                            m_UrlInfos.ReleaseWriteData();
                            ShowTrayIcon(false);
                        }
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
        if (!StopThread(2000))
        {
            TerminateProcess(GetCurrentProcess(), 0);
        }
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        ::DestroyWindow(m_hwnd);
        break;
    case WM_QUERYENDSESSION:
        if (!StopThread(2000))
        {
            TerminateProcess(GetCurrentProcess(), 0);
        }
        PostQuitMessage(0);
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
            SendMessage(m_hMainDlg, COMMITMONITOR_INFOTEXT, 0, (LPARAM)_T("Repositories are currently being checked, please wait..."));
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
            if (m_UrlToWorkOn.size())
            {
                if (it->second.url.compare(m_UrlToWorkOn) == 0)
                    it->second.lastchecked = 0;
            }
            else
                it->second.lastchecked = 0;
        }
        m_UrlInfos.ReleaseWriteData();
    }

    bool bAllRead = true;
    bool bHasErrors = false;
    const map<wstring,CUrlInfo> * pInfos = m_UrlInfos.GetReadOnlyData();
    for (map<wstring,CUrlInfo>::const_iterator it = pInfos->begin(); it != pInfos->end(); ++it)
    {
        CUrlInfo inf = it->second;
        if ((!it->second.error.empty())&&(!it->second.parentpath))
            bHasErrors = true;

        // go through the log entries and find unread items
        for (map<svn_revnum_t,SCCSLogEntry>::const_iterator rit = it->second.logentries.begin(); rit != it->second.logentries.end(); ++rit)
        {
            if (!rit->second.read)
            {
                bAllRead = false;
                break;
            }
        }

        if ((it->second.lastchecked + (it->second.minutesinterval*60)) < currenttime)
            bStartThread = true;
    }
    m_UrlInfos.ReleaseReadOnlyData();

    if (bAllRead && !bHasErrors && DWORD(CRegStdDWORD(_T("Software\\CommitMonitor\\IndicateConnectErrors"), TRUE)))
    {
        // no errors (anymore) and all items are marked as read:
        // stop the animated icon
        KillTimer(*this, IDT_ANIMATE);
        ShowTrayIcon(false);
    }
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
    bool bClearIcon = ((!newCommits)&&(regShowTaskbarIcon.read() == FALSE));
    if (bClearIcon)
    {
        msg = NIM_DELETE;
    }
    m_SystemTray.cbSize = sizeof(NOTIFYICONDATA);
    m_SystemTray.hWnd   = *this;
    if (bClearIcon)
        m_SystemTray.hIcon = NULL;
    else
        m_SystemTray.hIcon  = newCommits ? m_hIconNew1 : m_hIconNormal;
    m_SystemTray.uFlags = NIF_MESSAGE | NIF_ICON;
    m_SystemTray.uCallbackMessage = COMMITMONITOR_TASKBARCALLBACK;
    if (Shell_NotifyIcon(msg, &m_SystemTray) == FALSE)
    {
        if (msg == NIM_MODIFY)
        {
            Shell_NotifyIcon(NIM_DELETE, &m_SystemTray);
            Shell_NotifyIcon(NIM_ADD, &m_SystemTray);
        }
    }
    m_nIcon = 2;
    if ((newCommits)&&(m_SystemTray.hIcon)&&(DWORD(CRegStdDWORD(_T("Software\\CommitMonitor\\Animate"), TRUE))))
        SetTimer(*this, IDT_ANIMATE, TIMER_ANIMATE, NULL);
    else
        KillTimer(*this, IDT_ANIMATE);
}

void CHiddenWindow::DoAnimate()
{
    m_SystemTray.cbSize = sizeof(NOTIFYICONDATA);
    m_SystemTray.hWnd   = *this;
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
    if (Shell_NotifyIcon(NIM_MODIFY, &m_SystemTray) == FALSE)
    {
        Shell_NotifyIcon(NIM_DELETE, &m_SystemTray);
        Shell_NotifyIcon(NIM_ADD, &m_SystemTray);
    }
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
    bool bNeedsSaving = false;

    if (::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)!=S_OK)
    {
        SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
        return 1;
    }

    // to avoid blocking access to the urls info for the whole time this thread is running,
    // create a copy of it here. Copying the whole structure is relatively fast and therefore
    // won't block anything for too long.
    map<wstring,CUrlInfo> urlinfoReadOnly = *m_UrlInfos.GetReadOnlyData();
    m_UrlInfos.ReleaseReadOnlyData();

    TCHAR infotextbuf[1024];
    TRACE(_T("monitor thread started\n"));
    const map<wstring,CUrlInfo> * pUrlInfoReadOnly = &urlinfoReadOnly;
    map<wstring,CUrlInfo>::const_iterator it = pUrlInfoReadOnly->begin();
    for (; (it != pUrlInfoReadOnly->end()) && m_bRun; ++it)
    {
        int mit = max(it->second.minutesinterval, it->second.minminutesinterval);
        SendMessage(*this, COMMITMONITOR_INFOTEXT, 0, (LPARAM)_T(""));
        if (m_UrlToWorkOn.size())
        {
            if (it->second.url.compare(m_UrlToWorkOn))
                continue;
        }
        if (((it->second.monitored)&&((it->second.lastchecked + (mit*60)) < currenttime))||(m_UrlToWorkOn.size()))
        {
            m_UrlToWorkOn.clear();
            if ((it->second.errNr == SVN_ERR_RA_NOT_AUTHORIZED)&&(!it->second.error.empty()))
                continue;   // don't check if the last error was 'not authorized'
            TRACE(_T("checking %s for updates\n"), it->first.c_str());
            // get the highest revision of the repository
            SCCS *pSCCS;
            SVN svnAccess;
            ACCUREV accurevAccess;

            switch (it->second.sccs) {
              default:
              case CUrlInfo::SCCS_SVN:
                pSCCS = (SCCS *)&svnAccess;
                break;

              case CUrlInfo::SCCS_ACCUREV:
                pSCCS = (SCCS *)&accurevAccess;
                break;
            }


            pSCCS->SetAuthInfo(it->second.username, it->second.password);
            if (m_hMainDlg)
            {
                _stprintf_s(infotextbuf, 1024, _T("checking %s ..."), it->first.c_str());
                SendMessage(*this, COMMITMONITOR_INFOTEXT, 0, (LPARAM)infotextbuf);
            }
            svn_revnum_t headrev = pSCCS->GetHEADRevision(it->second.accurevRepo, it->first);
            if ((pSCCS->Err)&&(pSCCS->Err->apr_err == SVN_ERR_RA_NOT_AUTHORIZED))
            {
                // only block the object for a short time
                map<wstring,CUrlInfo> * pWrite = m_UrlInfos.GetWriteData();
                map<wstring,CUrlInfo>::iterator writeIt = pWrite->find(it->first);
                if (writeIt != pWrite->end())
                {
                    writeIt->second.lastchecked = currenttime;
                    writeIt->second.error = pSCCS->GetLastErrorMsg();
                    writeIt->second.errNr = pSCCS->Err->apr_err;
                    // no need to save here just for this
                }
                m_UrlInfos.ReleaseWriteData();
                continue;
            }
            if (!m_bRun)
                continue;
            std::set<std::wstring> authors;
            if (headrev > it->second.lastcheckedrev)
            {
                TRACE(_T("%s has updates! Last checked revision was %ld, HEAD revision is %ld\n"), it->first.c_str(), it->second.lastcheckedrev, headrev);
                if (m_hMainDlg)
                {
                    _stprintf_s(infotextbuf, 1024, _T("getting log for %s"), it->first.c_str());
                    SendMessage(*this, COMMITMONITOR_INFOTEXT, 0, (LPARAM)infotextbuf);
                }
                int nNewCommits = 0;        // commits without ignored ones
                int nTotalNewCommits = 0;   // all commits, including ignored ones
                if (pSCCS->GetLog(it->second.accurevRepo, it->first, headrev, it->second.lastcheckedrev + 1))
                {
                    TRACE(_T("log fetched for %s\n"), it->first.c_str());
                    if (!m_bRun)
                        continue;

                    // only block the object for a short time
                    map<wstring,CUrlInfo> * pWrite = m_UrlInfos.GetWriteData();
                    map<wstring,CUrlInfo>::iterator writeIt = pWrite->find(it->first);
                    if (writeIt != pWrite->end())
                    {
                        writeIt->second.lastcheckedrev = headrev;
                        writeIt->second.lastchecked = currenttime;
                        // no need to save just for this
                    }
                    m_UrlInfos.ReleaseWriteData();

                    if (!m_bRun)
                        continue;

                    wstring sPopupText;
                    bool hadError = !it->second.error.empty();
                    for (map<svn_revnum_t,SCCSLogEntry>::iterator logit = pSCCS->m_logs.begin(); logit != pSCCS->m_logs.end(); ++logit)
                    {
                        // again, only block for a short time
                        pWrite = m_UrlInfos.GetWriteData();
                        writeIt = pWrite->find(it->first);
                        bool bIgnore = false;
                        bool bEntryExists = false;
                        if (writeIt != pWrite->end())
                        {
                            map<svn_revnum_t,SCCSLogEntry>::iterator existIt = writeIt->second.logentries.find(logit->first);
                            bEntryExists = existIt != writeIt->second.logentries.end();
                            bool readState = false;
                            if (bEntryExists)
                                readState = existIt->second.read;
                            logit->second.read = readState;

                            writeIt->second.logentries[logit->first] = logit->second;

                            if (!bEntryExists)
                            {
                                wstring author1 = logit->second.author;
                                std::transform(author1.begin(), author1.end(), author1.begin(), std::tolower);
                                authors.insert(author1);
                                if (writeIt->second.includeUsers.size() > 0)
                                {
                                    wstring s1 = writeIt->second.includeUsers;
                                    std::transform(s1.begin(), s1.end(), s1.begin(), std::tolower);
                                    CAppUtils::SearchReplace(s1, _T("\r\n"), _T("\n"));
                                    vector<wstring> includeVector = CAppUtils::tokenize_str(s1, _T("\n"));
                                    bool bInclude = false;
                                    for (vector<wstring>::iterator inclIt = includeVector.begin(); inclIt != includeVector.end(); ++inclIt)
                                    {
                                        if (author1.compare(*inclIt) == 0)
                                        {
                                            bInclude = true;
                                            break;
                                        }
                                    }
                                    bIgnore = !bInclude;
                                }

                                wstring s1 = writeIt->second.ignoreUsers;
                                std::transform(s1.begin(), s1.end(), s1.begin(), std::tolower);
                                CAppUtils::SearchReplace(s1, _T("\r\n"), _T("\n"));
                                vector<wstring> ignoreVector = CAppUtils::tokenize_str(s1, _T("\n"));
                                for (vector<wstring>::iterator ignoreIt = ignoreVector.begin(); ignoreIt != ignoreVector.end(); ++ignoreIt)
                                {
                                    if (author1.compare(*ignoreIt) == 0)
                                    {
                                        bIgnore = true;
                                        break;
                                    }
                                }
                                nTotalNewCommits++;
                                if (!bIgnore)
                                {
                                    bNewEntries = true;
                                    nNewCommits++;
                                }
                                else
                                    // set own commit as already read
                                    writeIt->second.logentries[logit->first].read = true;
                            }
                            writeIt->second.error.clear();
                        }
                        bNeedsSaving = true;
                        m_UrlInfos.ReleaseWriteData();
                        if (!m_bRun)
                            continue;
                        // popup info text
                        if ((!bIgnore)&&(!bEntryExists))
                        {
                            if (!sPopupText.empty())
                                sPopupText += _T(", ");
                            sPopupText += logit->second.author;
                        }
                        if ((!it->second.disallowdiffs)&&(it->second.fetchdiffs))
                        {
                            TCHAR buf[4096];
                            // first, find a name where to store the diff for that revision
                            _stprintf_s(buf, 4096, _T("%s_%ld.diff"), it->second.name.c_str(), logit->first);
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
                                if (!pSCCS->Diff(it->first, logit->first, logit->first-1, logit->first, true, true, false, wstring(), false, diffFileName, wstring()))
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
                    }
                    if ((it->second.lastcheckedrobots + (60*60*24*2)) < currenttime)
                    {
                        wstring sRobotsURL = it->first;
                        sRobotsURL += _T("/svnrobots.txt");
                        wstring sRootRobotsURL;
                        wstring sDomainRobotsURL = sRobotsURL.substr(0, sRobotsURL.find('/', sRobotsURL.find(':')+3))+ _T("/svnrobots.txt");
                        sRootRobotsURL = pSCCS->GetRootUrl(it->first);
                        if (!sRootRobotsURL.empty())
                            sRootRobotsURL += _T("/svnrobots.txt");
                        wstring sFile = CAppUtils::GetTempFilePath();
                        string in;
                        CCallback * callback = new CCallback;
                        callback->SetAuthData(it->second.username, it->second.password);
                        if ((!sDomainRobotsURL.empty())&&(URLDownloadToFile(NULL, sDomainRobotsURL.c_str(), sFile.c_str(), 0, callback) == S_OK))
                        {
                            if (!m_bRun)
                                continue;
                            ifstream fs(sFile.c_str());
                            if (!fs.bad())
                            {
                                in.reserve((unsigned int)fs.rdbuf()->in_avail());
                                char c;
                                while (fs.get(c))
                                {
                                    if (in.capacity() == in.size())
                                        in.reserve(in.capacity() * 3);
                                    in.append(1, c);
                                }
                            }
                        }
                        else if ((!sRootRobotsURL.empty())&&(pSCCS->GetFile(sRootRobotsURL, sFile)))
                        {
                            ifstream fs(sFile.c_str());
                            if (!fs.bad())
                            {
                                in.reserve((unsigned int)fs.rdbuf()->in_avail());
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
                        else if (pSCCS->GetFile(sRobotsURL, sFile))
                        {
                            ifstream fs(sFile.c_str());
                            if (!fs.bad())
                            {
                                in.reserve((unsigned int)fs.rdbuf()->in_avail());
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
                        pWrite = m_UrlInfos.GetWriteData();
                        writeIt = pWrite->find(it->first);
                        if (writeIt != pWrite->end())
                        {
                            writeIt->second.lastcheckedrobots = currenttime;
                            writeIt->second.disallowdiffs = disallowdiffs;
                            writeIt->second.minminutesinterval = minutes;
                            // no need to save just for this
                        }
                        m_UrlInfos.ReleaseWriteData();
                    }
                    // prepare notification strings
                    if ((bNewEntries)||(!hadError && !it->second.error.empty()))
                    {
                        popupData data;
                        TCHAR sTitle[1024] = {0};
                        if (!it->second.error.empty() && DWORD(CRegStdDWORD(_T("Software\\CommitMonitor\\IndicateConnectErrors"), TRUE)))
                        {
                            _stprintf_s(sTitle, 1024, _T("%s\nfailed to connect!"), it->second.name.c_str());
                            sPopupText = it->second.error;
                        }
                        else
                        {
                            data.sProject = it->second.name;
                            if (nNewCommits == 1)
                            _stprintf_s(sTitle, 1024, _T("%s\nhas %d new commit"), it->second.name.c_str(), nNewCommits);
                        else
                            _stprintf_s(sTitle, 1024, _T("%s\nhas %d new commits"), it->second.name.c_str(), nNewCommits);
                        }
                        data.sText = sPopupText;
                        data.sTitle = wstring(sTitle);
                        data.sAlertType = ALERTTYPE_NEWCOMMITS;
                        // check if there still are unread items
                        bool bUnread = false;
                        pWrite = m_UrlInfos.GetWriteData();
                        writeIt = pWrite->find(it->first);
                        for (map<svn_revnum_t,SCCSLogEntry>::const_iterator lit = writeIt->second.logentries.begin(); lit != writeIt->second.logentries.end(); ++lit)
                        {
                            if (!lit->second.read)
                            {
                                bUnread = true;
                                break;
                            }
                        }
                        // no need to save just for this
                        m_UrlInfos.ReleaseWriteData();
                        if (bUnread)
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
                        writeIt->second.error = pSCCS->GetLastErrorMsg();
                        if (pSCCS->Err)
                            writeIt->second.errNr = pSCCS->Err->apr_err;
                        // no need to save just for this
                    }
                    m_UrlInfos.ReleaseWriteData();
                }
                // call the custom script
                if ((nTotalNewCommits > 0)&&(!it->second.callcommand.empty()))
                {
                    if ((!it->second.noexecuteignored)||(nNewCommits > 0))
                    {
                        // replace "%revision" with the new HEAD revision
                        wstring tag(_T("%revision"));
                        wstring commandline = it->second.callcommand;
                        // prepare the revision
                        TCHAR revBuf[40] = {0};
                        _stprintf_s(revBuf, 40, _T("%ld"), headrev);
                        wstring srev = revBuf;
                        CAppUtils::SearchReplace(commandline, tag, srev);

                        // replace "%url" with the repository url
                        tag = _T("%url");
                        CAppUtils::SearchReplace(commandline, tag, it->second.url);

                        // replace "%project" with the project name
                        tag = _T("%project");
                        CAppUtils::SearchReplace(commandline, tag, it->second.name);

                        // replace "%usernames" with a list of usernames for all the commits,
                        // separated by ";" (in case a username contains that char, you're out of luck, sorry)
                        tag = _T("%usernames");
                        std::wstring a;
                        for (auto autit = authors.cbegin(); autit != authors.cend(); ++autit)
                        {
                            if (!a.empty())
                                a += L";";
                            a += *autit;
                        }
                        CAppUtils::SearchReplace(commandline, tag, a);

                        CAppUtils::LaunchApplication(commandline);
                    }
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
                    bool hadError = !writeIt->second.error.empty();
                    writeIt->second.error = pSCCS->GetLastErrorMsg();
                    if (pSCCS->Err)
                        writeIt->second.errNr = pSCCS->Err->apr_err;
                    TCHAR sTitle[1024] = {0};
                    if (!writeIt->second.error.empty() && DWORD(CRegStdDWORD(_T("Software\\CommitMonitor\\IndicateConnectErrors"), TRUE)))
                    {
                        if (!hadError)
                        {
                            _stprintf_s(sTitle, 1024, _T("%s\nfailed to connect!"), it->second.name.c_str());
                            popupData data;
                            data.sText = pSCCS->GetLastErrorMsg();
                            data.sTitle = wstring(sTitle);
                            data.sAlertType = ALERTTYPE_FAILEDCONNECT;
                            ::SendMessage(*this, COMMITMONITOR_POPUP, 0, (LPARAM)&data);
                        }
                    }
                    // no need to save just for this
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
                    bool hadError = !writeIt->second.error.empty();
                    writeIt->second.error = pSCCS->GetLastErrorMsg();
                    if (pSCCS->Err)
                        writeIt->second.errNr = pSCCS->Err->apr_err;
                    TCHAR sTitle[1024] = {0};
                    if (!writeIt->second.error.empty() && DWORD(CRegStdDWORD(_T("Software\\CommitMonitor\\IndicateConnectErrors"), TRUE)))
                    {
                        if (!hadError)
                        {
                            _stprintf_s(sTitle, 1024, _T("%s\nfailed to connect!"), it->second.name.c_str());
                            popupData data;
                            data.sText = pSCCS->GetLastErrorMsg();
                            data.sTitle = wstring(sTitle);
                            data.sAlertType = ALERTTYPE_FAILEDCONNECT;
                            ::SendMessage(*this, COMMITMONITOR_POPUP, 0, (LPARAM)&data);
                        }
                    }
                    // no need to save just for this
                }
                m_UrlInfos.ReleaseWriteData();
                // if we already have log entries, then there's no need to
                // check whether the url points to an SVNParentPath: it points
                // to a repository, but we got an error for some reason when
                // trying to find the HEAD revision
                if (pSCCS->Err && (it->second.logentries.size() == 0)&&(it->second.lastcheckedrev == 0)&&((pSCCS->Err->apr_err == SVN_ERR_RA_DAV_RELOCATED)||(pSCCS->Err->apr_err == SVN_ERR_RA_DAV_REQUEST_FAILED)))
                {
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
                    if (!m_bRun)
                        continue;
                    if (hResUDL != S_OK)
                    {
                        hResUDL = URLDownloadToFile(NULL, parentpathurl.c_str(), tempfile.c_str(), 0, callback);
                    }
                    if (!m_bRun)
                        continue;
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
                            in.reserve((unsigned int)fs.rdbuf()->in_avail());
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
                            // some repositories show
                            // "<h2>projectname - Revision XX: /trunk</h2>
                            const char * reTitle = "<\\s*h2\\s*>[^/]+Revision\\s*\\d+:[^<]+<\\s*/\\s*h2\\s*>";
                            // xsl transformed pages don't have an easy way to determine
                            // the inside from outside of a repository.
                            // We therefore check for <index rev="0" to make sure it's either
                            // an empty repository or really an SVNParentPathList
                            const char * reTitle2 = "<\\s*index\\s*rev\\s*=\\s*\"0\"";
                            const tr1::regex titex(reTitle, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
                            const tr1::regex titex2(reTitle2, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
                            tr1::match_results<string::const_iterator> fwhat;
                            if (tr1::regex_search(in.begin(), in.end(), titex, tr1::regex_constants::match_default))
                            {
                                TRACE(_T("found repository url instead of SVNParentPathList\n"));
                                continue;
                            }

                            const char * re = "<\\s*LI\\s*>\\s*<\\s*A\\s+[^>]*HREF\\s*=\\s*\"([^\"]*)\"\\s*>([^<]+)<\\s*/\\s*A\\s*>\\s*<\\s*/\\s*LI\\s*>";
                            const char * re2 = "<\\s*DIR\\s*name\\s*=\\s*\"([^\"]*)\"\\s*HREF\\s*=\\s*\"([^\"]*)\"\\s*/\\s*>";

                            const tr1::regex expression(re, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
                            const tr1::regex expression2(re2, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
                            tr1::match_results<string::const_iterator> what;
                            bool hasNewEntries = false;
                            int nCountNewEntries = 0;
                            wstring popupText;
                            const tr1::sregex_iterator end;
                            for (tr1::sregex_iterator i(in.begin(), in.end(), expression); i != end && m_bRun; ++i)
                            {
                                const tr1::smatch match = *i;
                                // what[0] contains the whole string
                                // what[1] contains the url part.
                                // what[2] contains the name
                                wstring url = CUnicodeUtils::StdGetUnicode(string(match[1]));
                                url = it->first + _T("/") + url;
                                url = pSCCS->CanonicalizeURL(url);

                                pWrite = m_UrlInfos.GetWriteData();
                                writeIt = pWrite->find(url);
                                if ((!m_bMainDlgRemovedItems)&&(writeIt == pWrite->end()))
                                {
                                    // we found a new URL, add it to our list
                                    CUrlInfo newinfo;
                                    newinfo.url = url;
                                    newinfo.name = CUnicodeUtils::StdGetUnicode(string(match[2]));
                                    newinfo.name.erase(newinfo.name.find_last_not_of(_T("/ ")) + 1);
                                    newinfo.username = it->second.username;
                                    newinfo.password = it->second.password;
                                    newinfo.fetchdiffs = it->second.fetchdiffs;
                                    newinfo.minutesinterval = it->second.minutesinterval;
                                    newinfo.ignoreUsers = it->second.ignoreUsers;
                                    newinfo.callcommand = it->second.callcommand;
                                    newinfo.webviewer = it->second.webviewer;
                                    (*pWrite)[url] = newinfo;
                                    hasNewEntries = true;
                                    nCountNewEntries++;
                                    if (!popupText.empty())
                                        popupText += _T(", ");
                                    popupText += newinfo.name;
                                    bNeedsSaving = true;
                                }
                                writeIt = pWrite->find(it->first);
                                if (writeIt != pWrite->end())
                                {
                                    writeIt->second.parentpath = true;
                                }
                                m_UrlInfos.ReleaseWriteData();
                            }
                            if (!regex_search(in.begin(), in.end(), titex2))
                            {
                                TRACE(_T("found repository url instead of SVNParentPathList\n"));
                                continue;
                            }
                            for (tr1::sregex_iterator i(in.begin(), in.end(), expression2); i != end && m_bRun; ++i)
                            {
                                const tr1::smatch match = *i;
                                // what[0] contains the whole string
                                // what[1] contains the url part.
                                // what[2] contains the name
                                wstring url = CUnicodeUtils::StdGetUnicode(string(match[1]));
                                url = it->first + _T("/") + url;
                                url = pSCCS->CanonicalizeURL(url);

                                pWrite = m_UrlInfos.GetWriteData();
                                writeIt = pWrite->find(url);
                                if ((!m_bMainDlgRemovedItems)&&(writeIt == pWrite->end()))
                                {
                                    // we found a new URL, add it to our list
                                    CUrlInfo newinfo;
                                    newinfo.url = url;
                                    newinfo.name = CUnicodeUtils::StdGetUnicode(string(match[2]));
                                    newinfo.name.erase(newinfo.name.find_last_not_of(_T("/ ")) + 1);
                                    newinfo.username = it->second.username;
                                    newinfo.password = it->second.password;
                                    newinfo.fetchdiffs = it->second.fetchdiffs;
                                    newinfo.minutesinterval = it->second.minutesinterval;
                                    newinfo.ignoreUsers = it->second.ignoreUsers;
                                    newinfo.callcommand = it->second.callcommand;
                                    newinfo.webviewer = it->second.webviewer;
                                    (*pWrite)[url] = newinfo;
                                    hasNewEntries = true;
                                    nCountNewEntries++;
                                    if (!popupText.empty())
                                        popupText += _T(", ");
                                    popupText += newinfo.name;
                                    bNeedsSaving = true;
                                }
                                writeIt = pWrite->find(it->first);
                                if (writeIt != pWrite->end())
                                {
                                    writeIt->second.parentpath = true;
                                }
                                m_UrlInfos.ReleaseWriteData();
                            }
                            if (hasNewEntries)
                            {
                                it = pUrlInfoReadOnly->begin();
                                TCHAR popupTitle[1024] = {0};
                                _stprintf_s(popupTitle, 1024, _T("%s\nhas %d new projects"), projName.c_str(), nCountNewEntries);
                                popupData data;
                                data.sText = popupText;
                                data.sTitle = wstring(popupTitle);
                                data.sAlertType = ALERTTYPE_NEWPROJECTS;
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
    }


    // check for newer versions
    if (m_bRun && (CRegStdDWORD(_T("Software\\CommitMonitor\\CheckNewer"), TRUE) != FALSE))
    {
        time_t now;
        struct tm ptm;

        time(&now);
        if ((now != 0) && (localtime_s(&ptm, &now)==0))
        {
            int week = 0;
            // we don't calculate the real 'week of the year' here
            // because just to decide if we should check for an update
            // that's not needed.
            week = ptm.tm_yday / 7;

            CRegStdDWORD oldweek = CRegStdDWORD(_T("Software\\CommitMonitor\\CheckNewerWeek"), (DWORD)-1);
            if (((DWORD)oldweek) == -1)
                oldweek = week;     // first start of CommitMonitor, no update check needed
            else
            {
                if ((DWORD)week != oldweek)
                {
                    oldweek = week;
                    //
                    wstring temp;
                    wstring tempfile = CAppUtils::GetTempFilePath();

                    CRegStdString checkurluser = CRegStdString(_T("Software\\CommitMonitor\\UpdateCheckURL"), _T(""));
                    CRegStdString checkurlmachine = CRegStdString(_T("Software\\CommitMonitor\\UpdateCheckURL"), _T(""), FALSE, HKEY_LOCAL_MACHINE);
                    wstring sCheckURL = (wstring)checkurluser;
                    if (sCheckURL.empty())
                    {
                        sCheckURL = (wstring)checkurlmachine;
                        if (sCheckURL.empty())
                            sCheckURL = _T("http://commitmonitor.googlecode.com/svn/trunk/version.txt");
                    }
                    HRESULT res = URLDownloadToFile(NULL, sCheckURL.c_str(), tempfile.c_str(), 0, NULL);
                    if (res == S_OK)
                    {
                        ifstream File;
                        File.open(tempfile.c_str());
                        if (File.good())
                        {
                            char line[1024];
                            char * pLine = line;
                            File.getline(line, sizeof(line));
                            string vertemp = line;
                            int major = 0;
                            int minor = 0;
                            int micro = 0;
                            int build = 0;

                            major = atoi(pLine);
                            pLine = strchr(pLine, '.');
                            if (pLine)
                            {
                                pLine++;
                                minor = atoi(pLine);
                                pLine = strchr(pLine, '.');
                                if (pLine)
                                {
                                    pLine++;
                                    micro = atoi(pLine);
                                    pLine = strchr(pLine, '.');
                                    if (pLine)
                                    {
                                        pLine++;
                                        build = atoi(pLine);
                                    }
                                }
                            }
                            if (major > CM_VERMAJOR)
                                m_bNewerVersionAvailable = true;
                            else if ((minor > CM_VERMINOR)&&(major == CM_VERMAJOR))
                                m_bNewerVersionAvailable = true;
                            else if ((micro > CM_VERMICRO)&&(minor == CM_VERMINOR)&&(major == CM_VERMAJOR))
                                m_bNewerVersionAvailable = true;
                            else if ((build > CM_VERBUILD)&&(micro == CM_VERMICRO)&&(minor == CM_VERMINOR)&&(major == CM_VERMAJOR))
                                m_bNewerVersionAvailable = true;
                        }
                        File.close();
                    }
                }
            }
        }
    }

    SendMessage(*this, COMMITMONITOR_INFOTEXT, 0, (LPARAM)_T(""));
    if (bNeedsSaving)
    {
        // save the changed entries
        ::PostMessage(*this, COMMITMONITOR_SAVEINFO, (WPARAM)true, (LPARAM)0);
    }
    if (bNewEntries)
        ::PostMessage(*this, COMMITMONITOR_CHANGEDINFO, (WPARAM)true, (LPARAM)0);

    if ((!bNewEntries)&&(m_bIsTask))
        ::PostQuitMessage(0);

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

bool CHiddenWindow::StopThread(DWORD wait)
{
    bool bRet = true;
    m_bRun = false;
    if (m_hMonitorThread)
    {
        WaitForSingleObject(m_hMonitorThread, wait);
        if (m_ThreadRunning)
        {
            bRet = false;
        }
        CloseHandle(m_hMonitorThread);
        m_hMonitorThread = NULL;
    }

    return bRet;
}

void CHiddenWindow::ShowPopup(wstring& title, wstring& text, const wchar_t *alertType)
{
    popupData data;
    data.sText = text;
    data.sTitle = title;
    data.sAlertType = alertType;
    ::SendMessage(*this, COMMITMONITOR_POPUP, 0, (LPARAM)&data);
}
