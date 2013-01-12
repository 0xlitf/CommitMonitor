// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2008, 2010, 2012 - Stefan Kueng

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
#include "resource.h"
#include "UpdateDlg.h"
#include "Registry.h"
#include "version.h"
#include <string>
#include <Commdlg.h>


CUpdateDlg::CUpdateDlg(HWND hParent)
    : m_hParent(hParent)
{
}

CUpdateDlg::~CUpdateDlg(void)
{
}

LRESULT CUpdateDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            InitDialog(hwndDlg, IDI_COMMITMONITOR);
            // initialize the controls
            m_link.ConvertStaticToHyperlink(hwndDlg, IDC_WEBURL, _T("http://tools.tortoisesvn.net/CommitMonitor.html"));

            ExtendFrameIntoClientArea((UINT)-1, (UINT)-1, (UINT)-1, (UINT)-1);
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDC_INFOLABEL));
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDC_INFOLABEL2));
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDOK));
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDC_WEBURL));
        }
        return TRUE;
    case WM_COMMAND:
        return DoCommand(LOWORD(wParam));
    default:
        return FALSE;
    }
}

LRESULT CUpdateDlg::DoCommand(int id)
{
    switch (id)
    {
    case IDOK:
        // fall through
    case IDCANCEL:
        EndDialog(*this, id);
        break;
    }
    return 1;
}
