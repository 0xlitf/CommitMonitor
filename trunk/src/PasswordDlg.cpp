// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2009, 2012 - Stefan Kueng

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
#include "PasswordDlg.h"
#include "Registry.h"
#include "AppUtils.h"
#include <string>
#include <Commdlg.h>


CPasswordDlg::CPasswordDlg(HWND hParent)
{
    m_hParent = hParent;
}

CPasswordDlg::~CPasswordDlg(void)
{
}

LRESULT CPasswordDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            InitDialog(hwndDlg, IDI_COMMITMONITOR);
            DialogEnableWindow(IDOK, false);

            ExtendFrameIntoClientArea(0, IDC_PW1, 0, IDC_PW2);
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDC_INFOLABEL));
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDOK));
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDCANCEL));
        }
        return TRUE;
    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE)
        {
            int len = ::GetWindowTextLength(GetDlgItem(*this, IDC_PW1));
            std::unique_ptr<TCHAR[]> pwBuf(new TCHAR[len+1]);
            ::GetDlgItemText(*this, IDC_PW1, pwBuf.get(), len+1);
            std::wstring pw1 = std::wstring(pwBuf.get());
            len = ::GetWindowTextLength(GetDlgItem(*this, IDC_PW2));
            pwBuf = std::unique_ptr<WCHAR[]>(new TCHAR[len+1]);
            ::GetDlgItemText(*this, IDC_PW2, pwBuf.get(), len+1);
            std::wstring pw2 = std::wstring(pwBuf.get());

            DialogEnableWindow(IDOK, pw1.compare(pw2) == 0);
        }
        else
            return DoCommand(LOWORD(wParam));
    default:
        return FALSE;
    }
}

LRESULT CPasswordDlg::DoCommand(int id)
{
    switch (id)
    {
    case IDOK:
        {
            int len = ::GetWindowTextLength(GetDlgItem(*this, IDC_PW1));
            std::unique_ptr<TCHAR[]> pwBuf(new TCHAR[len+1]);
            ::GetDlgItemText(*this, IDC_PW1, pwBuf.get(), len+1);
            password = std::wstring(pwBuf.get());
            EndDialog(*this, id);
        }
        break;
    case IDCANCEL:
        password.clear();
        EndDialog(*this, id);
        break;
    }
    return 1;
}
