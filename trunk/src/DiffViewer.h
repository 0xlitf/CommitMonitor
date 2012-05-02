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
#include "Platform.h"
#include "Scintilla.h"
#include "FindBar.h"

class CDiffViewer : public CWindow
{
public:
    CDiffViewer(HINSTANCE hInst, const WNDCLASSEX* wcx = NULL);
    ~CDiffViewer(void);

    /**
     * Registers the window class and creates the window.
     */
    bool                RegisterAndCreateWindow();

    bool                Initialize();

    LRESULT             SendEditor(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
    HWND                GetHWNDEdit() { return m_hWndEdit; }
    bool                LoadFile(LPCTSTR filename);
    void                SetTitle(LPCTSTR title);

protected:
    /// the message handler for this window
    LRESULT CALLBACK    WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    /// Handles all the WM_COMMAND window messages (e.g. menu commands)
    LRESULT             DoCommand(int id);

private:
    void                SetAStyle(int style, COLORREF fore, COLORREF back=::GetSysColor(COLOR_WINDOW),
                                    int size=-1, const char *face=0);
    bool                IsUTF8(LPVOID pBuffer, int cb);

private:
    LRESULT             m_directFunction;
    LRESULT             m_directPointer;

    HWND                m_hWndEdit;

    CFindBar            m_FindBar;
    bool                m_bShowFindBar;
    bool                m_bMatchCase;
    std::wstring        m_findtext;
};
