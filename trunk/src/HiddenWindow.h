// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007-2011 - Stefan Kueng

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
#pragma once
#include <shellapi.h>
#include "BaseWindow.h"
#include "MainDlg.h"
#include "UrlInfo.h"
#include "Registry.h"
#include "resource.h"

#pragma comment(lib, "shell32.lib")

/// the timer IDs
#define IDT_MONITOR     101
#define IDT_ANIMATE     102
#define IDT_POPUP       103

/// timer elapse time, set to 1 minute
#define TIMER_ELAPSE    60000
#define TIMER_ANIMATE   400

#define ALERTTYPE_NEWCOMMITS L"new commits"
#define ALERTTYPE_FAILEDCONNECT L"connection error"
#define ALERTTYPE_NEWPROJECTS L"new projects"

typedef struct
{
    wstring             sProject;
    wstring             sTitle;
    wstring             sText;
    wstring             sAlertType;
} popupData;

class CHiddenWindow : public CWindow
{
public:
    CHiddenWindow(HINSTANCE hInst, const WNDCLASSEX* wcx = NULL);
    ~CHiddenWindow(void);

    /**
     * Registers the window class and creates the window.
     */
    bool                RegisterAndCreateWindow();

    INT_PTR             ShowDialog();

    bool                StopThread(DWORD wait);
    void                RemoveTrayIcon();

    DWORD               RunThread();

    void                SetTask(bool b) {m_bIsTask = b;}

    void                ShowPopup(wstring& title, wstring& text, const wchar_t *alertType);

    void                Save() { m_UrlInfos.Save(); }
protected:
    /// the message handler for this window
    LRESULT CALLBACK    WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT             HandleCustomMessages(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    void                DoTimer(bool bForce);
    void                DoAnimate();
    void                ShowTrayIcon(bool newCommits);

private:
    UINT                COMMITMONITOR_SHOWDLGMSG;
    UINT                WM_TASKBARCREATED;
    LONG32              snarlGlobalMsg;

    int                 m_nIcon;
    NOTIFYICONDATA      m_SystemTray;
    HICON               m_hIconNormal;
    HICON               m_hIconNew0;
    HICON               m_hIconNew1;
    HICON               m_hIconNew2;
    HICON               m_hIconNew3;

    CUrlInfos           m_UrlInfos;
    DWORD               m_ThreadRunning;
    bool                m_bRun;
    HANDLE              m_hMonitorThread;

    bool                m_bMainDlgShown;
    bool                m_bMainDlgRemovedItems;
    HWND                m_hMainDlg;

    CRegStdDWORD        regShowTaskbarIcon;

    bool                m_bIsTask;
    bool                m_bNewerVersionAvailable;

    wstring             m_UrlToWorkOn;
    CRegStdString       m_regLastSelectedProject;

    std::vector<popupData>  m_popupData;

    typedef BOOL(__stdcall *PFNCHANGEWINDOWMESSAGEFILTER)(UINT message, DWORD dwFlag);
    static PFNCHANGEWINDOWMESSAGEFILTER m_pChangeWindowMessageFilter;
};

extern CHiddenWindow *hiddenWindowPointer;