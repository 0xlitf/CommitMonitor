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
	DeleteObject(m_hBmp);
}

LRESULT CFindBar::DlgFunc(HWND /*hwndDlg*/, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			m_hBmp = ::LoadBitmap(hResource, MAKEINTRESOURCE(IDB_CANCELNORMAL));
			SendMessage(GetDlgItem(*this, IDC_FINDEXIT), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_hBmp);
		}
		return TRUE;
	case WM_COMMAND:
		return DoCommand(LOWORD(wParam), HIWORD(wParam));
	default:
		return FALSE;
	}
}

LRESULT CFindBar::DoCommand(int id, int msg)
{
    bool bFindPrev = false;
	switch (id)
	{
    case IDC_FINDPREV:
        bFindPrev = true;
    case IDC_FINDNEXT:
        {
			DoFind(bFindPrev);
        }
        break;
	case IDC_FINDEXIT:
		{
			::SendMessage(m_hParent, COMMITMONITOR_FINDEXIT, 0, 0);
		}
		break;
	case IDC_FINDTEXT:
		{
			if (msg == EN_CHANGE)
			{
				SendMessage(m_hParent, COMMITMONITOR_FINDRESET, 0, 0);
				DoFind(false);
			}
		}
		break;
	}
	return 1;
}

void CFindBar::DoFind(bool bFindPrev)
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