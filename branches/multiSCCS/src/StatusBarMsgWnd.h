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
#pragma once
#include "BaseWindow.h"
#include <string>
#include <vector>

using namespace std;

#define STATUSBARMSGWND_SHOWTIMER       101
#define STATUSBARMSGWND_ICONSIZE        32
class CStatusBarMsgWnd : public CWindow
{
public:
    CStatusBarMsgWnd(HINSTANCE hInst, const WNDCLASSEX* wcx = NULL)
        : CWindow(hInst, wcx)
        , m_width(200)
        , m_height(80)
        , m_icon(NULL)
    {
        RegisterAndCreateWindow();
    }

    void                Show(LPCTSTR title, LPCTSTR text, UINT icon, HWND hParentWnd, UINT messageOnClick, int stay = 10);
private:
    // deconstructor private to prevent creating an instance on the stack
    // --> must be created on the heap!
    ~CStatusBarMsgWnd(void);


protected:
    virtual void        OnPaint(HDC hDC, LPRECT pRect);
    /**
     * Registers the window class and creates the window.
     */
    bool RegisterAndCreateWindow();
    /// the message handler for this window
    LRESULT CALLBACK    WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT             DoTimer();

    void                ShowFromLeft();
    void                ShowFromTop();
    void                ShowFromRight();
    void                ShowFromBottom();

private:
    wstring             m_title;
    wstring             m_text;
    HICON               m_icon;
    UINT                m_messageOnClick;
    HWND                m_hParentWnd;

    UINT                m_uEdge;
    RECT                m_workarea;
    int                 m_ShowTicks;

    LONG                m_width;
    LONG                m_height;
    int                 m_stay;

    int                 m_thiscounter;
    static int          m_counter;
    static vector<int>  m_slots;
};

