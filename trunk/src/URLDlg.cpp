// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007-2008 - Stefan Kueng

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
#include "Resource.h"
#include <algorithm>
#include "URLDlg.h"

#include "SVN.h"
#include <cctype>
#include <regex>
using namespace std;

CURLDlg::CURLDlg(void)
{
}

CURLDlg::~CURLDlg(void)
{
}

void CURLDlg::SetInfo(const CUrlInfo * pURLInfo /* = NULL */)
{
	if (pURLInfo == NULL)
		return;
	info = *pURLInfo;
}

LRESULT CURLDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			InitDialog(hwndDlg, IDI_COMMITMONITOR);

			AddToolTip(IDC_PROJECTNAME, _T("Enter here a name for the project"));
			AddToolTip(IDC_URLTOMONITOR, _T("URL to the repository, or the SVNParentPath URL"));
			AddToolTip(IDC_IGNORESELF, _T("If enabled, commits from you won't show a notification"));
			if (info.minminutesinterval)
			{
				TCHAR infobuf[MAX_PATH] = {0};
				_stprintf_s(infobuf, MAX_PATH, _T("Interval for repository update checks.\nMiminum set by svnrobots.txt file to %ld minutes."), info.minminutesinterval);
				AddToolTip(IDC_CHECKTIME, infobuf);
			}
			else
				AddToolTip(IDC_CHECKTIME, _T("Interval for repository update checks"));

			// initialize the controls
			SetDlgItemText(*this, IDC_URLTOMONITOR, info.url.c_str());
			WCHAR buf[20];
			_stprintf_s(buf, 20, _T("%ld"), max(info.minutesinterval, info.minminutesinterval));
			SetDlgItemText(*this, IDC_CHECKTIME, buf);
			SetDlgItemText(*this, IDC_PROJECTNAME, info.name.c_str());
			SetDlgItemText(*this, IDC_USERNAME, info.username.c_str());
			SetDlgItemText(*this, IDC_PASSWORD, info.password.c_str());
			SetDlgItemText(*this, IDC_IGNOREUSERS, info.ignoreUsers.c_str());
		}
		return TRUE;
	case WM_COMMAND:
		return DoCommand(LOWORD(wParam));
	case WM_NOTIFY:
		{
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CURLDlg::DoCommand(int id)
{
	switch (id)
	{
	case IDOK:
		{
			SVN svn;
			int len = GetWindowTextLength(GetDlgItem(*this, IDC_URLTOMONITOR));
			WCHAR * buffer = new WCHAR[len+1];
			GetDlgItemText(*this, IDC_URLTOMONITOR, buffer, len+1);
			info.url = svn.CanonicalizeURL(wstring(buffer, len));
			delete [] buffer;

			wstring tempurl = info.url.substr(0, 7);
			std::transform(tempurl.begin(), tempurl.end(), tempurl.begin(), std::tolower);

			if (tempurl.compare(_T("file://")) == 0)
			{
				::MessageBox(*this, _T("file:/// urls are not supported!"), _T("CommitMonitor"), MB_ICONERROR);
				return 1;
			}

			len = GetWindowTextLength(GetDlgItem(*this, IDC_PROJECTNAME));
			buffer = new WCHAR[len+1];
			GetDlgItemText(*this, IDC_PROJECTNAME, buffer, len+1);
			info.name = wstring(buffer, len);

			len = GetWindowTextLength(GetDlgItem(*this, IDC_CHECKTIME));
			buffer = new WCHAR[len+1];
			GetDlgItemText(*this, IDC_CHECKTIME, buffer, len+1);
			info.minutesinterval = _ttoi(buffer);
			if ((info.minminutesinterval)&&(info.minminutesinterval > info.minutesinterval))
				info.minutesinterval = info.minminutesinterval;
			delete [] buffer;

			len = GetWindowTextLength(GetDlgItem(*this, IDC_USERNAME));
			buffer = new WCHAR[len+1];
			GetDlgItemText(*this, IDC_USERNAME, buffer, len+1);
			info.username = wstring(buffer, len);
			delete [] buffer;

			len = GetWindowTextLength(GetDlgItem(*this, IDC_PASSWORD));
			buffer = new WCHAR[len+1];
			GetDlgItemText(*this, IDC_PASSWORD, buffer, len+1);
			info.password = wstring(buffer, len);
			delete [] buffer;

			len = GetWindowTextLength(GetDlgItem(*this, IDC_IGNOREUSERS));
			buffer = new WCHAR[len+1];
			GetDlgItemText(*this, IDC_IGNOREUSERS, buffer, len+1);
			info.ignoreUsers = wstring(buffer, len);
			delete [] buffer;

			// make sure this entry gets checked again as soon as the next timer fires
			info.lastchecked = 0;
		}
		// fall through
	case IDCANCEL:
		EndDialog(*this, id);
		break;
	}
	return 1;
}

