// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007, 2010 - Stefan Kueng

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
#include "basedialog.h"
#include "UrlInfo.h"
#include "AeroControls.h"

/**
 * url dialog.
 */
class CURLDlg : public CDialog
{
public:
    CURLDlg(void);
    ~CURLDlg(void);

    void                    SetInfo(const CUrlInfo * pURLInfo = NULL);
    CUrlInfo *              GetInfo() {return &info;}
    void                    ClearForTemplate();

protected:
    void                    SetSCCS(CUrlInfo::SCCS_TYPE sccs);
    LRESULT CALLBACK        DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT                 DoCommand(int id, int cmd);

    bool                    OnSetCursor(HWND hWnd, UINT nHitTest, UINT message);
    bool                    OnMouseMove(UINT nFlags, POINT point);
    bool                    OnLButtonDown(UINT nFlags, POINT point);
    bool                    OnLButtonUp(UINT nFlags, POINT point);
    void                    DrawXorBar(HDC hDC, LONG x1, LONG y1, LONG width, LONG height);

private:
    CUrlInfo                info;
    AeroControlBase         m_aerocontrols;
    wstring                 sSCCS[CUrlInfo::SCCS_LEN];
};
