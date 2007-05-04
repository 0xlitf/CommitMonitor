#include "StdAfx.h"
#include "Resource.h"
#include "FindBar.h"
#include "Registry.h"
#include <string>
#include <Commdlg.h>

using namespace std;

CFindBar::CFindBar()
{
}

CFindBar::~CFindBar(void)
{
}

LRESULT CFindBar::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
		}
		return TRUE;
	case WM_COMMAND:
		return DoCommand(LOWORD(wParam));
	default:
		return FALSE;
	}
}

LRESULT CFindBar::DoCommand(int id)
{
    bool bFindPrev = false;
	switch (id)
	{
    case IDC_FINDPREV:
        bFindPrev = true;
    case IDC_FINDNEXT:
        {
            int len = ::GetWindowTextLength(GetDlgItem(*this, IDC_FINDTEXT));
            TCHAR * findtext = new TCHAR[len+1];
            ::GetWindowText(GetDlgItem(*this, IDC_FINDTEXT), findtext, len+1);
            wstring ft = wstring(findtext);
            delete [] findtext;
            bool bCaseSensitive = !!SendMessage(GetDlgItem(*this, IDC_MATCHCASECHECK), BM_GETCHECK, 0, NULL);
            if (bFindPrev)
                ::SendMessage(m_hParent, COMMITMONITOR_FINDMSGPREV, (WPARAM)bCaseSensitive, (LPARAM)ft.c_str());
            else
                ::SendMessage(m_hParent, COMMITMONITOR_FINDMSGNEXT, (WPARAM)bCaseSensitive, (LPARAM)ft.c_str());
        }
        break;
	}
	return 1;
}

