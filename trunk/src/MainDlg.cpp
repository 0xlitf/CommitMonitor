// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007-2009 - Stefan Kueng

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
#include "MainDlg.h"

#include "URLDlg.h"
#include "OptionsDlg.h"
#include "AboutDlg.h"
#include "UpdateDlg.h"
#include "AppUtils.h"
#include "DirFileEnum.h"
#include <algorithm>
#include <assert.h>
#include <cctype>
#include <regex>

#define FILTERBOXHEIGHT 20
#define FILTERLABELWIDTH 50

CMainDlg::CMainDlg(HWND hParent) 
	: m_nDragMode(DRAGMODE_NONE)
	, m_oldx(-1)
	, m_oldy(-1)
	, m_boldFont(NULL)
	, m_font(NULL)
	, m_pURLInfos(NULL)
	, m_bBlockListCtrlUI(false)
	, m_hTreeControl(NULL)
	, m_hListControl(NULL)
	, m_hLogMsgControl(NULL)
	, m_hToolbarImages(NULL)
	, m_hImgList(NULL)
	, m_bNewerVersionAvailable(false)
	, m_refreshNeeded(false)
{
	m_hParent = hParent;
	// use the default GUI font, create a copy of it and
	// change the copy to BOLD (leave the rest of the font
	// the same)
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONT lf = {0};
	GetObject(hFont, sizeof(LOGFONT), &lf);
	lf.lfWeight = FW_BOLD;
	m_boldFont = CreateFontIndirect(&lf);
}

CMainDlg::~CMainDlg(void)
{
	if (m_boldFont)
		DeleteObject(m_boldFont);
	if (m_font)
		DeleteObject(m_font);
	if (m_hToolbarImages)
		ImageList_Destroy(m_hToolbarImages);
	if (m_hImgList)
		ImageList_Destroy(m_hImgList);
}

bool CMainDlg::CreateToolbar()
{
	m_hwndToolbar = CreateWindowEx(0, 
		TOOLBARCLASSNAME, 
		(LPCTSTR)NULL,
		WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS, 
		0, 0, 0, 0, 
		*this,
		(HMENU)IDR_MAINDLG, 
		hResource, 
		NULL);
	if (m_hwndToolbar == INVALID_HANDLE_VALUE)
		return false;

	SendMessage(m_hwndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);

#define MAINDLG_TOOLBARBUTTONCOUNT	11
	TBBUTTON tbb[MAINDLG_TOOLBARBUTTONCOUNT];
	// create an image list containing the icons for the toolbar
	m_hToolbarImages = ImageList_Create(24, 24, ILC_COLOR32 | ILC_MASK, MAINDLG_TOOLBARBUTTONCOUNT, 4);
	if (m_hToolbarImages == NULL)
		return false;
	int index = 0;
	HICON hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_GETALL));
	tbb[index].iBitmap = ImageList_AddIcon(m_hToolbarImages, hIcon); 
	tbb[index].idCommand = ID_MAIN_CHECKREPOSITORIESNOW; 
	tbb[index].fsState = TBSTATE_ENABLED|BTNS_SHOWTEXT; 
	tbb[index].fsStyle = BTNS_BUTTON; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = (INT_PTR)_T("Check Now"); 

	hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_ADD));
	tbb[index].iBitmap = ImageList_AddIcon(m_hToolbarImages, hIcon); 
	tbb[index].idCommand = ID_MAIN_ADDPROJECT; 
	tbb[index].fsState = TBSTATE_ENABLED|BTNS_SHOWTEXT; 
	tbb[index].fsStyle = BTNS_BUTTON; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = (INT_PTR)_T("Add Project"); 

	tbb[index].iBitmap = 0; 
	tbb[index].idCommand = 0; 
	tbb[index].fsState = TBSTATE_ENABLED; 
	tbb[index].fsStyle = BTNS_SEP; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = 0; 

	hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_EDIT));
	tbb[index].iBitmap = ImageList_AddIcon(m_hToolbarImages, hIcon); 
	tbb[index].idCommand = ID_MAIN_EDIT; 
	tbb[index].fsState = BTNS_SHOWTEXT; 
	tbb[index].fsStyle = BTNS_BUTTON; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = (INT_PTR)_T("Edit"); 

	hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_REMOVE));
	tbb[index].iBitmap = ImageList_AddIcon(m_hToolbarImages, hIcon); 
	tbb[index].idCommand = ID_MAIN_REMOVE; 
	tbb[index].fsState = BTNS_SHOWTEXT; 
	tbb[index].fsStyle = BTNS_BUTTON; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = (INT_PTR)_T("Remove"); 

	tbb[index].iBitmap = 0; 
	tbb[index].idCommand = 0; 
	tbb[index].fsState = TBSTATE_ENABLED; 
	tbb[index].fsStyle = BTNS_SEP; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = 0; 

	hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_DIFF));
	tbb[index].iBitmap = ImageList_AddIcon(m_hToolbarImages, hIcon); 
	tbb[index].idCommand = ID_MAIN_SHOWDIFFCHOOSE; 
	tbb[index].fsState = BTNS_SHOWTEXT; 
	tbb[index].fsStyle = BTNS_BUTTON; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = (INT_PTR)_T("Show Diff"); 

	tbb[index].iBitmap = 0; 
	tbb[index].idCommand = 0; 
	tbb[index].fsState = TBSTATE_ENABLED; 
	tbb[index].fsStyle = BTNS_SEP; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = 0; 

	hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_MARKASREAD));
	tbb[index].iBitmap = ImageList_AddIcon(m_hToolbarImages, hIcon); 
	tbb[index].idCommand = ID_POPUP_MARKALLASREAD; 
	tbb[index].fsState = TBSTATE_ENABLED|BTNS_SHOWTEXT; 
	tbb[index].fsStyle = BTNS_BUTTON; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = (INT_PTR)_T("Mark all as read"); 

	hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_OPTIONS));
	tbb[index].iBitmap = ImageList_AddIcon(m_hToolbarImages, hIcon); 
	tbb[index].idCommand = ID_MISC_OPTIONS; 
	tbb[index].fsState = TBSTATE_ENABLED|BTNS_SHOWTEXT; 
	tbb[index].fsStyle = BTNS_BUTTON; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = (INT_PTR)_T("Options"); 

	hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_ABOUT));
	tbb[index].iBitmap = ImageList_AddIcon(m_hToolbarImages, hIcon); 
	tbb[index].idCommand = ID_MISC_ABOUT; 
	tbb[index].fsState = TBSTATE_ENABLED|BTNS_SHOWTEXT; 
	tbb[index].fsStyle = BTNS_BUTTON; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = (INT_PTR)_T("About"); 

	SendMessage(m_hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM)m_hToolbarImages);
	SendMessage(m_hwndToolbar, TB_ADDBUTTONS, (WPARAM)index, (LPARAM) (LPTBBUTTON) &tbb); 
	SendMessage(m_hwndToolbar, TB_AUTOSIZE, 0, 0); 
	ShowWindow(m_hwndToolbar, SW_SHOW); 
	return true; 
}

LRESULT CMainDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			InitDialog(hwndDlg, IDI_COMMITMONITOR);

			CreateToolbar();
			AddToolTip(IDC_FILTERSTRING, _T("Enter a filter string\nPrepend the string with an '-' to negate the filter."));

			m_hTreeControl = ::GetDlgItem(*this, IDC_URLTREE);
			m_hListControl = ::GetDlgItem(*this, IDC_MONITOREDURLS);
			m_hLogMsgControl = ::GetDlgItem(*this, IDC_LOGINFO);
			m_hFilterControl = ::GetDlgItem(*this, IDC_FILTERSTRING);
			::SendMessage(m_hTreeControl, TVM_SETUNICODEFORMAT, 1, 0);
			assert(m_pURLInfos);
			m_hImgList = ImageList_Create(16, 16, ILC_COLOR32, 6, 6);
			if (m_hImgList)
			{
				HICON hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_PARENTPATHFOLDER));
				ImageList_AddIcon(m_hImgList, hIcon);
				DestroyIcon(hIcon);

				hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_PARENTPATHFOLDEROPEN));
				ImageList_AddIcon(m_hImgList, hIcon);
				DestroyIcon(hIcon);

				hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_REPOURL));
				ImageList_AddIcon(m_hImgList, hIcon);
				DestroyIcon(hIcon);

				hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_REPOURLNEW));
				ImageList_AddIcon(m_hImgList, hIcon);
				DestroyIcon(hIcon);

				hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_REPOURLFAIL));
				ImageList_AddIcon(m_hImgList, hIcon);
				DestroyIcon(hIcon);

				hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_REPOURLINACTIVE));
				ImageList_AddIcon(m_hImgList, hIcon);
				DestroyIcon(hIcon);

				TreeView_SetImageList(m_hTreeControl, m_hImgList, LVSIL_SMALL);
				TreeView_SetImageList(m_hTreeControl, m_hImgList, LVSIL_NORMAL);
			}

			LOGFONT lf = {0};
			HDC hDC = ::GetDC(m_hLogMsgControl);
			lf.lfHeight = -MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72);
			lf.lfCharSet = DEFAULT_CHARSET;
			_tcscpy_s(lf.lfFaceName, 32, _T("Courier New"));
			m_font = ::CreateFontIndirect(&lf);
			ReleaseDC(m_hLogMsgControl, hDC);
			::SendMessage(m_hLogMsgControl, WM_SETFONT, (WPARAM)m_font, 1);

			// initialize the window position infos
			RECT rect;
			GetClientRect(m_hwndToolbar, &rect);
			m_topmarg = rect.bottom+2;
			GetClientRect(m_hTreeControl, &rect);
			m_xSliderPos = rect.right+4;
			GetClientRect(m_hListControl, &rect);
			m_ySliderPos = rect.bottom+m_topmarg;
			GetClientRect(m_hLogMsgControl, &rect);
			m_bottommarg = rect.bottom+4+m_ySliderPos;
			GetClientRect(*this, &rect);
			m_bottommarg = rect.bottom - m_bottommarg;

			// subclass the tree view control to intercept the WM_SETFOCUS messages
			m_oldTreeWndProc = (WNDPROC)SetWindowLongPtr(m_hTreeControl, GWLP_WNDPROC, (LONG)TreeProc);
			SetWindowLongPtr(m_hTreeControl, GWLP_USERDATA, (LONG)this);
			m_oldFilterWndProc = (WNDPROC)SetWindowLongPtr(m_hFilterControl, GWLP_WNDPROC, (LONG)FilterProc);
			SetWindowLongPtr(m_hFilterControl, GWLP_USERDATA, (LONG)this);

			m_ListCtrl.SubClassListCtrl(m_hListControl);

			::SetTimer(*this, TIMER_REFRESH, 1000, NULL);
			SendMessage(m_hParent, COMMITMONITOR_SETWINDOWHANDLE, (WPARAM)(HWND)*this, NULL);

			CRegStdDWORD regXY(_T("Software\\CommitMonitor\\XY"));
			if (DWORD(regXY))
			{
				CRegStdDWORD regWHWindow(_T("Software\\CommitMonitor\\WHWindow"));
				if (DWORD(regWHWindow))
				{
					CRegStdDWORD regWH(_T("Software\\CommitMonitor\\WH"));
					if (DWORD(regWH))
					{
						// x,y position and width/height are valid
						//
						// check whether the rectangle is at least partly
						// visible in at least one monitor
						RECT rc = {0};
						rc.left = HIWORD(DWORD(regXY));
						rc.top = LOWORD(DWORD(regXY));
						rc.right = HIWORD(DWORD(regWHWindow)) + rc.left;
						rc.bottom = LOWORD(DWORD(regWHWindow)) + rc.top;
						if (MonitorFromRect(&rc, MONITOR_DEFAULTTONULL))
						{
							SetWindowPos(*this, HWND_TOP, rc.left, rc.top, HIWORD(DWORD(regWHWindow)), LOWORD(DWORD(regWHWindow)), SWP_SHOWWINDOW);
							DoResize(HIWORD(DWORD(regWH)), LOWORD(DWORD(regWH)));
							// now restore the slider positions
							CRegStdDWORD regHorzPos(_T("Software\\CommitMonitor\\HorzPos"));
							if (DWORD(regHorzPos))
							{
								POINT pt;
								pt.x = pt.y = DWORD(regHorzPos)+2;	// +2 because the slider is 4 pixels wide
								PositionChildWindows(pt, true, false);
							}
							CRegStdDWORD regVertPos(_T("Software\\CommitMonitor\\VertPos"));
							if (DWORD(regVertPos))
							{
								POINT pt;
								pt.x = pt.y = DWORD(regVertPos)+2;	// +2 because the slider is 4 pixels wide
								PositionChildWindows(pt, false, false);
							}
							// adjust the slider position infos
							GetClientRect(m_hTreeControl, &rect);
							m_xSliderPos = rect.right+4;
							GetClientRect(m_hListControl, &rect);
							m_ySliderPos = rect.bottom+m_topmarg;
						}
					}
				}
			}

			CRegStdDWORD regMaximized(_T("Software\\CommitMonitor\\Maximized"));
			if( DWORD(regMaximized) )
			{
				ShowWindow(*this, SW_MAXIMIZE);

				// now restore the slider positions
				CRegStdDWORD regHorzPos(_T("Software\\CommitMonitor\\HorzPosZoomed"));
				if (DWORD(regHorzPos))
				{
					POINT pt;
					pt.x = pt.y = DWORD(regHorzPos)+2;	// +2 because the slider is 4 pixels wide
					PositionChildWindows(pt, true, false);
				}
				CRegStdDWORD regVertPos(_T("Software\\CommitMonitor\\VertPosZoomed"));
				if (DWORD(regVertPos))
				{
					POINT pt;
					pt.x = pt.y = DWORD(regVertPos)+2;	// +2 because the slider is 4 pixels wide
					PositionChildWindows(pt, false, false);
				}
				// adjust the slider position infos
				GetClientRect(m_hTreeControl, &rect);
				m_xSliderPos = rect.right+4;
				GetClientRect(m_hListControl, &rect);
				m_ySliderPos = rect.bottom+m_topmarg;
			}
			RefreshURLTree(true);

			ExtendFrameIntoClientArea(0, 0, 0, IDC_URLTREE);
			m_aerocontrols.SubclassControl(GetDlgItem(*this, IDC_INFOLABEL));
			m_aerocontrols.SubclassControl(GetDlgItem(*this, IDOK));
			m_aerocontrols.SubclassControl(GetDlgItem(*this, IDC_EXIT));

			if (m_bNewerVersionAvailable)
			{
				CUpdateDlg dlg(*this);
				dlg.DoModal(hResource, IDD_NEWERNOTIFYDLG, *this);
			}
		}
		break;
	case WM_SIZE:
		{
			if (wParam == SIZE_MINIMIZED)
			{
				EndDialog(*this, IDCANCEL);
				return 0;
			}
			DoResize(LOWORD(lParam), HIWORD(lParam));
		}
		break;
	case WM_SYSCOMMAND:
		{
			CRegStdDWORD regMaximized(_T("Software\\CommitMonitor\\Maximized"));
			if ((wParam & 0xFFF0) == SC_MAXIMIZE)
			{
				regMaximized = 1;
			}
			
			if ((wParam & 0xFFF0) == SC_RESTORE)
			{
				regMaximized = 0;
			}
			SaveWndPosition();

			return FALSE;
		}
		break;
	case WM_MOVING:
		{
#define STICKYSIZE 3
			LPRECT pRect = (LPRECT)lParam;
			if (pRect)
			{
				HMONITOR hMonitor = MonitorFromRect(pRect, MONITOR_DEFAULTTONEAREST);
				if (hMonitor)
				{
					MONITORINFO minfo = {0};
					minfo.cbSize = sizeof(minfo);
					if (GetMonitorInfo(hMonitor, &minfo))
					{
						int width = pRect->right - pRect->left;
						int heigth = pRect->bottom - pRect->top;
						if (abs(pRect->left - minfo.rcWork.left) < STICKYSIZE)
						{
							pRect->left = minfo.rcWork.left;
							pRect->right = pRect->left + width;
						}
						if (abs(pRect->right - minfo.rcWork.right) < STICKYSIZE)
						{
							pRect->right = minfo.rcWork.right;
							pRect->left = pRect->right - width;
						}
						if (abs(pRect->top - minfo.rcWork.top) < STICKYSIZE)
						{
							pRect->top = minfo.rcWork.top;
							pRect->bottom = pRect->top + heigth;
						}
						if (abs(pRect->bottom - minfo.rcWork.bottom) < STICKYSIZE)
						{
							pRect->bottom = minfo.rcWork.bottom;
							pRect->top = pRect->bottom - heigth;
						}
					}
				}
			}
		}
		break;
	case WM_GETMINMAXINFO:
		{
			MINMAXINFO * mmi = (MINMAXINFO*)lParam;
			mmi->ptMinTrackSize.x = m_xSliderPos + 100;
			mmi->ptMinTrackSize.y = m_ySliderPos + 100;
			return 0;
		}
		break;
	case WM_COMMAND:
		if ((HIWORD(wParam) == EN_CHANGE)&&((HWND)lParam == m_hFilterControl))
		{
			// start the filter timer
			::SetTimer(*this, TIMER_FILTER, FILTER_ELAPSE, NULL);
		}
		return DoCommand(LOWORD(wParam));
		break;
	case WM_SETCURSOR:
		{
			return OnSetCursor((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
		}
		break;
	case WM_MOUSEMOVE:
		{
			POINT pt = {LOWORD(lParam), HIWORD(lParam)};
			return OnMouseMove(wParam, pt);
		}
		break;
	case WM_LBUTTONDOWN:
		{
			POINT pt = {LOWORD(lParam), HIWORD(lParam)};
			return OnLButtonDown(wParam, pt);
		}
		break;
	case WM_LBUTTONUP:
		{
			POINT pt = {LOWORD(lParam), HIWORD(lParam)};
			return OnLButtonUp(wParam, pt);
		}
		break;
	case WM_TIMER:
		{
			if (wParam == TIMER_LABEL)
			{
				SetDlgItemText(*this, IDC_INFOLABEL, _T(""));
				KillTimer(*this, TIMER_LABEL);
			}
			else if (wParam == TIMER_FILTER)
			{
				KillTimer(*this, TIMER_FILTER);
				TreeItemSelected(m_hTreeControl, TreeView_GetSelection(m_hTreeControl));
			}
			else if (wParam == TIMER_REFRESH)
			{
				const map<wstring, CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
				for (map<wstring, CUrlInfo>::const_iterator it = pRead->begin(); it != pRead->end(); ++it)
				{
					TVINSERTSTRUCT tv = {0};
					tv.hParent = FindParentTreeNode(it->first);
					tv.hInsertAfter = TVI_SORT;
					tv.itemex.mask = TVIF_TEXT|TVIF_PARAM|TVIF_STATE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
					WCHAR * str = new WCHAR[it->second.name.size()+10];
					// find out if there are some unread entries
					int unread = 0;
					for (map<svn_revnum_t,SVNLogEntry>::const_iterator logit = it->second.logentries.begin(); logit != it->second.logentries.end(); ++logit)
					{
						if (!logit->second.read)
							unread++;
					}
					tv.itemex.pszText = str;
					tv.itemex.lParam = (LPARAM)&it->first;
					HTREEITEM directItem = FindTreeNode(it->first);
					if (directItem != TVI_ROOT)
					{
						// The node already exists, just update the information
						tv.itemex.hItem = directItem;
						tv.itemex.stateMask = TVIS_SELECTED|TVIS_BOLD|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
						tv.itemex.pszText = str;
						tv.itemex.cchTextMax = it->second.name.size()+9;
						TreeView_GetItem(m_hTreeControl, &tv.itemex);
						wstring sTitle = wstring(str);
						bool bRequiresUpdate = false;
						if (unread)
						{
							_stprintf_s(str, it->second.name.size()+10, _T("%s (%d)"), it->second.name.c_str(), unread);
							tv.itemex.state |= TVIS_BOLD;
							tv.itemex.stateMask = TVIS_BOLD;
						}
						else
						{
							_tcscpy_s(str, it->second.name.size()+1, it->second.name.c_str());
							tv.itemex.state &= ~TVIS_BOLD;
							tv.itemex.stateMask = TVIS_BOLD;
						}
						if (it->second.parentpath)
						{
							bRequiresUpdate = (tv.itemex.iImage != 0) || (tv.itemex.iSelectedImage != 1);
							tv.itemex.iImage = 0;
							tv.itemex.iSelectedImage = 1;
						}
						else
						{

							if (!it->second.error.empty())
							{
								bRequiresUpdate = tv.itemex.iImage != 4;
								tv.itemex.iImage = 4;
								tv.itemex.iSelectedImage = 4;
							}
							else if (unread)
							{
								bRequiresUpdate = tv.itemex.iImage != 3;
								tv.itemex.iImage = 3;
								tv.itemex.iSelectedImage = 3;
							}
							else if (!it->second.monitored)
							{
								bRequiresUpdate = tv.itemex.iImage != 5;
								tv.itemex.iImage = 5;
								tv.itemex.iSelectedImage = 5;
							}
							else
							{
								bRequiresUpdate = tv.itemex.iImage != 2;
								tv.itemex.iImage = 2;
								tv.itemex.iSelectedImage = 2;
							}
						}
						if ((bRequiresUpdate)||(sTitle.compare(str) != 0)||
							((tv.itemex.state & TVIS_SELECTED)&&(m_refreshNeeded)))
						{
							m_refreshNeeded = false;
							TreeView_SetItem(m_hTreeControl, &tv.itemex);
							if (tv.itemex.state & TVIS_SELECTED)
							{
								m_bBlockListCtrlUI = true;
								int listCount = ListView_GetItemCount(m_hListControl);
								int listSelMark = ListView_GetSelectionMark(m_hListControl);
								TreeItemSelected(m_hTreeControl, tv.itemex.hItem);
								// re-set the currently selected item
								int itemsAdded = ListView_GetItemCount(m_hListControl) - listCount;
								m_bBlockListCtrlUI = false;
								if (ListView_GetItemState(m_hListControl, listSelMark + itemsAdded, LVIS_SELECTED) & LVIS_SELECTED)
								{
									ListView_SetSelectionMark(m_hListControl, listSelMark + itemsAdded);
									ListView_SetItemState(m_hListControl, listSelMark + itemsAdded, LVIS_SELECTED, LVIS_SELECTED);
								}
							}
						}
					}
					else
					{
						if (unread)
						{
							_stprintf_s(str, it->second.name.size()+10, _T("%s (%d)"), it->second.name.c_str(), unread);
							tv.itemex.state = TVIS_BOLD;
							tv.itemex.stateMask = TVIS_BOLD;
						}
						else
						{
							_tcscpy_s(str, it->second.name.size()+1, it->second.name.c_str());
							tv.itemex.state = 0;
							tv.itemex.stateMask = TVIS_BOLD;
						}
						m_bBlockListCtrlUI = true;
						if (it->second.parentpath)
						{
							tv.itemex.iImage = 0;
							tv.itemex.iSelectedImage = 1;
						}
						else
						{
							if (!it->second.error.empty())
							{
								tv.itemex.iImage = 4;
								tv.itemex.iSelectedImage = 4;
							}
							else if (unread)
							{
								tv.itemex.iImage = 3;
								tv.itemex.iSelectedImage = 3;
							}
							else
							{
								tv.itemex.iImage = 2;
								tv.itemex.iSelectedImage = 2;
							}
						}
						TreeView_InsertItem(m_hTreeControl, &tv);
						TreeView_Expand(m_hTreeControl, tv.hParent, TVE_EXPAND);
						m_bBlockListCtrlUI = false;
					}
					delete [] str;
				}
				m_pURLInfos->ReleaseReadOnlyData();
				::InvalidateRect(m_hListControl, NULL, true);
			}
		}
		break;
	case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr = (LPNMHDR)lParam;
			if ((lpnmhdr->code == TVN_SELCHANGED)&&(lpnmhdr->hwndFrom == m_hTreeControl))
			{
				OnSelectTreeItem((LPNMTREEVIEW)lParam);
				return TRUE;
			}
			if ((lpnmhdr->code == LVN_ITEMCHANGING)&&(lpnmhdr->hwndFrom == m_hListControl))
			{
				LPNMLISTVIEW lpNMListView = (LPNMLISTVIEW)lParam;
				if ((lpNMListView)&&(((lpNMListView->uOldState ^ lpNMListView->uNewState) & LVIS_SELECTED)&&(m_ListCtrl.InfoTextShown())))
				{
					return TRUE;
				}
			}
			if ((lpnmhdr->code == LVN_ITEMCHANGED)&&(lpnmhdr->hwndFrom == m_hListControl))
			{
				OnSelectListItem((LPNMLISTVIEW)lParam);
			}
			if ((lpnmhdr->code == LVN_KEYDOWN)&&(lpnmhdr->hwndFrom == m_hListControl))
			{
				OnKeyDownListItem((LPNMLVKEYDOWN)lParam);
			}
			if ((lpnmhdr->code == NM_CUSTOMDRAW)&&(lpnmhdr->hwndFrom == m_hListControl))
			{
				return OnCustomDrawListItem((LPNMLVCUSTOMDRAW)lParam);
			}
			if ((lpnmhdr->code == NM_CUSTOMDRAW)&&(lpnmhdr->hwndFrom == m_hTreeControl))
			{
				return OnCustomDrawTreeItem((LPNMTVCUSTOMDRAW)lParam);
			}
			if ((lpnmhdr->code == NM_DBLCLK)&&(lpnmhdr->hwndFrom == m_hListControl))
			{
				OnDblClickListItem((LPNMITEMACTIVATE)lParam);
			}
			return FALSE;
		}
		break;
	case WM_CONTEXTMENU:
		{
			POINT pt;
			pt.x = GET_X_LPARAM(lParam);
			pt.y = GET_Y_LPARAM(lParam);

			if (HWND(wParam) == m_hTreeControl)
			{
				TVHITTESTINFO hittest = {0};
				if (pt.x == -1 && pt.y == -1)
				{
					hittest.hItem = TreeView_GetSelection(m_hTreeControl);
					if (hittest.hItem)
					{
						hittest.flags = TVHT_ONITEM;
						RECT rect;
						TreeView_GetItemRect(m_hTreeControl, hittest.hItem, &rect, TRUE);
						pt.x = rect.left + ((rect.right-rect.left)/2);
						pt.y = rect.top + ((rect.bottom - rect.top)/2);
						ClientToScreen(m_hTreeControl, &pt);
					}
				}
				else
				{
					POINT clPt = pt;
					::ScreenToClient(m_hTreeControl, &clPt);
					hittest.pt = clPt;
					TreeView_HitTest(m_hTreeControl, &hittest);
				}
				if (hittest.flags & TVHT_ONITEM)
				{
					HTREEITEM hSel = TreeView_GetSelection(m_hTreeControl);
					m_bBlockListCtrlUI = true;
					TreeView_SelectItem(m_hTreeControl, hittest.hItem);
					m_bBlockListCtrlUI = false;

					HMENU hMenu = ::LoadMenu(hResource, MAKEINTRESOURCE(IDR_TREEPOPUP));
					hMenu = ::GetSubMenu(hMenu, 0);
					TVITEMEX itemex = {0};
					itemex.hItem = hittest.hItem;
					itemex.mask = TVIF_PARAM;
					TreeView_GetItem(m_hTreeControl, &itemex);
					const map<wstring,CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
					if (pRead->find(*(wstring*)itemex.lParam) != pRead->end())
					{
						const CUrlInfo * info = &pRead->find(*(wstring*)itemex.lParam)->second;
						if (info)
						{
							CheckMenuItem(hMenu, ID_POPUP_ACTIVE, MF_BYCOMMAND | (info->monitored ? MF_CHECKED : MF_UNCHECKED));
							if (!info->parentpath)
							{
								// remove the 'mark all as read' since this is not a parent (SVNParentPath) item
								DeleteMenu(hMenu, ID_POPUP_MARKALLASREAD, MF_BYCOMMAND);
							}
						}
					}
					m_pURLInfos->ReleaseReadOnlyData();

					int cmd = ::TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY , pt.x, pt.y, NULL, *this, NULL);
					m_bBlockListCtrlUI = true;
					TreeView_SelectItem(m_hTreeControl, hSel);
					m_bBlockListCtrlUI = false;
					switch (cmd)
					{
					case ID_MAIN_EDIT:
					case ID_MAIN_REMOVE:
						{
							HTREEITEM hSel = TreeView_GetSelection(m_hTreeControl);
							m_bBlockListCtrlUI = true;
							TreeView_SelectItem(m_hTreeControl, hittest.hItem);
							m_bBlockListCtrlUI = false;
							::SendMessage(*this, WM_COMMAND, MAKELONG(cmd, 0), 0);
							m_bBlockListCtrlUI = true;
							TreeView_SelectItem(m_hTreeControl, hSel);
							m_bBlockListCtrlUI = false;
						}
						break;
					case ID_POPUP_MARKALLASREAD:
						MarkAllAsRead(hittest.hItem, true);
						break;
					case ID_POPUP_CHECKNOW:
						CheckNow(hittest.hItem);
						break;
					case ID_POPUP_MARKNODEASREAD:
						MarkAllAsRead(hittest.hItem, false);
						break;
					case ID_POPUP_REFRESHALL:
						RefreshAll(hittest.hItem);
						break;
					case ID_POPUP_ACTIVE:
						map<wstring,CUrlInfo> * pWrite = m_pURLInfos->GetWriteData();
						if (pWrite->find(*(wstring*)itemex.lParam) != pWrite->end())
						{
							CUrlInfo * info = &pWrite->find(*(wstring*)itemex.lParam)->second;
							if (info)
							{
								info->monitored = !info->monitored;
							}
						}
						m_pURLInfos->ReleaseWriteData();
						::SendMessage(m_hParent, COMMITMONITOR_CHANGEDINFO, (WPARAM)false, (LPARAM)0);
						break;
					}
				}
			}
			else if (HWND(wParam) == m_hListControl)
			{
				LVHITTESTINFO hittest = {0};
				if (pt.x == -1 && pt.y == -1)
				{
					hittest.iItem = ListView_GetSelectionMark(m_hListControl);
					if (hittest.iItem >= 0)
					{
						hittest.flags = LVHT_ONITEM;
						RECT rect;
						ListView_GetItemRect(m_hListControl, hittest.iItem, &rect, LVIR_LABEL);
						pt.x = rect.left + ((rect.right-rect.left)/2);
						pt.y = rect.top + ((rect.bottom - rect.top)/2);
						ClientToScreen(m_hListControl, &pt);
					}
				}
				else
				{
					POINT clPt = pt;
					::ScreenToClient(m_hListControl, &clPt);
					hittest.pt = clPt;
					ListView_HitTest(m_hListControl, &hittest);
				}
				if (hittest.flags & LVHT_ONITEM)
				{
					HMENU hMenu = NULL;
					wstring tsvninstalled = CAppUtils::GetTSVNPath();
					if (tsvninstalled.empty())
						hMenu = ::LoadMenu(hResource, MAKEINTRESOURCE(IDR_LISTPOPUP));
					else
						hMenu = ::LoadMenu(hResource, MAKEINTRESOURCE(IDR_LISTPOPUPTSVN));
					hMenu = ::GetSubMenu(hMenu, 0);

					UINT uItem = 0;
					
					if ((!wstring(tsvninstalled).empty()) && (!DWORD(CRegStdDWORD(_T("Software\\CommitMonitor\\UseTSVN"), TRUE))))
						uItem = 1;
					// set the default entry
					MENUITEMINFO iinfo = {0};
					iinfo.cbSize = sizeof(MENUITEMINFO);
					iinfo.fMask = MIIM_STATE;
					GetMenuItemInfo(hMenu, uItem, MF_BYPOSITION, &iinfo);
					iinfo.fState |= MFS_DEFAULT;
					SetMenuItemInfo(hMenu, uItem, MF_BYPOSITION, &iinfo);

					// enable the "Open WebViewer" entry if there is one specified
					// get the url this entry refers to
					TVITEMEX itemex = {0};
					itemex.hItem = TreeView_GetSelection(m_hTreeControl);
					itemex.mask = TVIF_PARAM;
					TreeView_GetItem(m_hTreeControl, &itemex);
					const map<wstring,CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
					if (pRead->find(*(wstring*)itemex.lParam) != pRead->end())
					{
						const CUrlInfo * info = &pRead->find(*(wstring*)itemex.lParam)->second;
						if ((info)&&(!info->webviewer.empty()))
						{
							uItem = wstring(tsvninstalled).empty() ? 1 : 2;
							GetMenuItemInfo(hMenu, uItem, MF_BYPOSITION, &iinfo);
							iinfo.fState &= ~MFS_DISABLED;
							SetMenuItemInfo(hMenu, uItem, MF_BYPOSITION, &iinfo);
						}
					}
					m_pURLInfos->ReleaseReadOnlyData();

					int cmd = ::TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY , pt.x, pt.y, NULL, *this, NULL);
					switch (cmd)
					{
					case ID_POPUP_MARKASUNREAD:
						{
							const map<wstring,CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
							HTREEITEM hSelectedItem = TreeView_GetSelection(m_hTreeControl);
							// get the url this entry refers to
							TVITEMEX itemex = {0};
							itemex.hItem = hSelectedItem;
							itemex.mask = TVIF_PARAM;
							TreeView_GetItem(m_hTreeControl, &itemex);
							if (itemex.lParam != 0)
							{
								LVITEM item = {0};
								int nItemCount = ListView_GetItemCount(m_hListControl);
								for (int i=0; i<nItemCount; ++i)
								{
									item.mask = LVIF_PARAM|LVIF_STATE;
									item.stateMask = LVIS_SELECTED;
									item.iItem = i;
									ListView_GetItem(m_hListControl, &item);
									if (item.state & LVIS_SELECTED)
									{
										SVNLogEntry * pLogEntry = (SVNLogEntry*)item.lParam;
										if (pLogEntry)
										{
											// set the entry as unread
											if (pLogEntry->read)
											{
												pLogEntry->read = false;
												// refresh the name of the tree item to indicate the new
												// number of unread log messages
												// e.g. instead of 'TortoiseSVN (2)', show now 'TortoiseSVN (3)'
												if (pRead->find(*(wstring*)itemex.lParam) != pRead->end())
												{
													const CUrlInfo * uinfo = &pRead->find(*(wstring*)itemex.lParam)->second;
													// count the number of unread messages
													int unread = 0;
													for (map<svn_revnum_t,SVNLogEntry>::const_iterator it = uinfo->logentries.begin(); it != uinfo->logentries.end(); ++it)
													{
														if (!it->second.read)
															unread++;
													}
													WCHAR * str = new WCHAR[uinfo->name.size()+10];
													if (unread)
													{
														_stprintf_s(str, uinfo->name.size()+10, _T("%s (%d)"), uinfo->name.c_str(), unread);
														itemex.state = TVIS_BOLD;
														itemex.stateMask = TVIS_BOLD;
														itemex.iImage = 3;
														itemex.iSelectedImage = 3;
													}
													else
													{
														_stprintf_s(str, uinfo->name.size()+10, _T("%s"), uinfo->name.c_str());
														itemex.state = 0;
														itemex.stateMask = TVIS_BOLD;
														itemex.iImage = 2;
														itemex.iSelectedImage = 2;
													}

													itemex.pszText = str;
													itemex.mask = TVIF_TEXT|TVIF_STATE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
													m_refreshNeeded = true;
													TreeView_SetItem(m_hTreeControl, &itemex);
												}
											}
										}
									}
								}
							}
							m_pURLInfos->ReleaseReadOnlyData();
						}
						break;
					case ID_MAIN_SHOWDIFFTSVN:
					case ID_MAIN_SHOWDIFF:
					case ID_MAIN_REMOVE:
						{
							::SendMessage(*this, WM_COMMAND, MAKELONG(cmd, 0), 0);
						}
						break;
					case ID_POPUP_OPENWEBVIEWER:
						{
							TVITEMEX itemex = {0};
							itemex.hItem = TreeView_GetSelection(m_hTreeControl);
							itemex.mask = TVIF_PARAM;
							TreeView_GetItem(m_hTreeControl, &itemex);
							const map<wstring,CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
							if (pRead->find(*(wstring*)itemex.lParam) != pRead->end())
							{
								const CUrlInfo * info = &pRead->find(*(wstring*)itemex.lParam)->second;
								if ((info)&&(!info->webviewer.empty()))
								{
									// replace "%revision" with the new HEAD revision
									wstring tag(_T("%revision"));
									wstring commandline = info->webviewer;
									wstring::iterator it_begin = search(commandline.begin(), commandline.end(), tag.begin(), tag.end());
									if (it_begin != commandline.end())
									{
										// find the revision
										LVITEM item = {0};
										int nItemCount = ListView_GetItemCount(m_hListControl);
										for (int i=0; i<nItemCount; ++i)
										{
											item.mask = LVIF_PARAM|LVIF_STATE;
											item.stateMask = LVIS_SELECTED;
											item.iItem = i;
											ListView_GetItem(m_hListControl, &item);
											if (item.state & LVIS_SELECTED)
											{
												SVNLogEntry * pLogEntry = (SVNLogEntry*)item.lParam;
												if (pLogEntry)
												{
													// prepare the revision
													TCHAR revBuf[40] = {0};
													_stprintf_s(revBuf, 40, _T("%ld"), pLogEntry->revision);
													wstring srev = revBuf;
													wstring::iterator it_end= it_begin + tag.size();
													commandline.replace(it_begin, it_end, srev);
													break;
												}
											}
										}
									}
									// replace "%url" with the repository url
									tag = _T("%url");
									it_begin = search(commandline.begin(), commandline.end(), tag.begin(), tag.end());
									if (it_begin != commandline.end())
									{
										wstring::iterator it_end= it_begin + tag.size();
										commandline.replace(it_begin, it_end, info->url);
									}
									// replace "%project" with the project name
									tag = _T("%project");
									it_begin = search(commandline.begin(), commandline.end(), tag.begin(), tag.end());
									if (it_begin != commandline.end())
									{
										wstring::iterator it_end= it_begin + tag.size();
										commandline.replace(it_begin, it_end, info->name);
									}
									if (!commandline.empty())
									{
										ShellExecute(*this, _T("open"), commandline.c_str(), NULL, NULL, SW_SHOWNORMAL);
									}
								}
							}
							m_pURLInfos->ReleaseReadOnlyData();
						}
					}
				}
			}
		}
		break;
	case COMMITMONITOR_INFOTEXT:
		{
			if (lParam)
			{
				SetDlgItemText(*this, IDC_INFOLABEL, (LPCTSTR)lParam);
			}
		}
		break;
	case COMMITMONITOR_LISTCTRLDBLCLICK:
		{
			// clear the error so it won't show up again
			TVITEMEX itemex = {0};
			itemex.hItem = TreeView_GetSelection(m_hTreeControl);
			itemex.mask = TVIF_PARAM;
			TreeView_GetItem(m_hTreeControl, &itemex);
			map<wstring,CUrlInfo> * pWrite = m_pURLInfos->GetWriteData();
			if (pWrite->find(*(wstring*)itemex.lParam) != pWrite->end())
			{
				CUrlInfo * info = &pWrite->find(*(wstring*)itemex.lParam)->second;
				info->error.clear();
			}
			m_pURLInfos->ReleaseWriteData();
			::InvalidateRect(m_hTreeControl, NULL, FALSE);
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CMainDlg::DoCommand(int id)
{
	switch (id)
	{
	case IDOK:
		{
			if (::GetFocus() != GetDlgItem(*this, IDOK))
			{
				// focus is not on the OK/Hide button
				if ((GetFocus() == m_hListControl)&&((GetKeyState(VK_MENU)&0x8000)==0))
				{
					::SendMessage(*this, WM_COMMAND, MAKELONG(ID_MAIN_SHOWDIFFCHOOSE, 0), 0);
				}
				if ((GetKeyState(VK_MENU)&0x8000)==0)
					break;
			}
		}
		// intentional fall-through
	case IDCANCEL:
		{
			SaveWndPosition();
			EndDialog(*this, IDCANCEL);
		}
		break;
	case IDC_EXIT:
		{
			int res = ::MessageBox(*this, _T("Do you really want to quit the CommitMonitor?\nIf you quit, monitoring will stop.\nIf you just want to close the dialog, use the \"Hide\" button.\n\nAre you sure you want to quit the CommitMonitor?"),
				_T("CommitMonitor"), MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2);
			if (res != IDYES)
				break;
			EndDialog(*this, IDCANCEL);
			PostQuitMessage(IDOK);
		}
		break;
	case ID_MAIN_REMOVE:
		{
			// which control has the focus?
			HWND hFocus = ::GetFocus();
			if (hFocus == m_hTreeControl)
			{
				HTREEITEM hItem = TreeView_GetSelection(m_hTreeControl);
				if (hItem)
				{
					TVITEMEX itemex = {0};
					itemex.hItem = hItem;
					itemex.mask = TVIF_PARAM;
					TreeView_GetItem(m_hTreeControl, &itemex);
					map<wstring,CUrlInfo> * pWrite = m_pURLInfos->GetWriteData();
					HTREEITEM hPrev = TVI_ROOT;
					map<wstring,CUrlInfo>::iterator it = pWrite->find(*(wstring*)itemex.lParam);
					if (it != pWrite->end())
					{
						wstring mask = it->second.name;
						// ask the user if he really wants to remove the url
						TCHAR question[4096] = {0};
						_stprintf_s(question, 4096, _T("Do you really want to stop monitoring the project\n%s ?"), mask.c_str());
						if (::MessageBox(*this, question, _T("CommitMonitor"), MB_ICONQUESTION|MB_YESNO)==IDYES)
						{
							// delete all fetched and stored diff files
							mask += _T("*.*");
							CSimpleFileFind sff(CAppUtils::GetDataDir(), mask.c_str());
							while (sff.FindNextFileNoDots())
							{
								DeleteFile(sff.GetFilePath().c_str());
							}

							int unread = 0;
							for (map<svn_revnum_t,SVNLogEntry>::const_iterator logit = it->second.logentries.begin(); logit != it->second.logentries.end(); ++logit)
							{
								if (!logit->second.read)
									unread++;
							}
							pWrite->erase(it);
							if (unread)
								::SendMessage(m_hParent, COMMITMONITOR_CHANGEDINFO, (WPARAM)false, (LPARAM)0);
							::SendMessage(m_hParent, COMMITMONITOR_REMOVEDURL, 0, 0);
							hPrev = TreeView_GetPrevSibling(m_hTreeControl, hItem);
							m_pURLInfos->ReleaseWriteData();
							m_pURLInfos->Save();
							TreeView_DeleteItem(m_hTreeControl, hItem);
							if (hPrev == NULL)
								hPrev = TreeView_GetRoot(m_hTreeControl);
							if ((hPrev)&&(hPrev != TVI_ROOT))
								TreeView_SelectItem(m_hTreeControl, hPrev);
							else
							{
								// no more tree items, deactivate the remove button and clear the list control
								SetRemoveButtonState();
								ListView_DeleteAllItems(m_hListControl);
								SetWindowText(m_hLogMsgControl, _T(""));
							}
						}
						else
							m_pURLInfos->ReleaseWriteData();
					}
					else
						m_pURLInfos->ReleaseWriteData();
				}
			}
			else if (hFocus == m_hListControl)
			{
				RemoveSelectedListItems();
			}
		}
		break;
	case ID_MAIN_EDIT:
		{
			CURLDlg dlg;
			HTREEITEM hItem = TreeView_GetSelection(m_hTreeControl);
			if (hItem)
			{
				TVITEMEX itemex = {0};
				itemex.hItem = hItem;
				itemex.mask = TVIF_PARAM;
				TreeView_GetItem(m_hTreeControl, &itemex);
				const map<wstring,CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
				if (pRead->find(*(wstring*)itemex.lParam) != pRead->end())
				{
					dlg.SetInfo(&pRead->find(*(wstring*)itemex.lParam)->second);
					m_pURLInfos->ReleaseReadOnlyData();
					dlg.DoModal(hResource, IDD_URLCONFIG, *this);
					CUrlInfo * inf = dlg.GetInfo();
					map<wstring,CUrlInfo> * pWrite = m_pURLInfos->GetWriteData();
					if ((inf)&&inf->url.size())
					{
						pWrite->erase(*(wstring*)itemex.lParam);
						(*pWrite)[inf->url] = *inf;
					}
					m_pURLInfos->Save();
					m_pURLInfos->ReleaseWriteData();
					RefreshURLTree(false);
				}
				else
					m_pURLInfos->ReleaseWriteData();
			}
		}
		break;
	case ID_MAIN_CHECKREPOSITORIESNOW:
		SendMessage(m_hParent, COMMITMONITOR_GETALL, 0, 0);
		break;
	case ID_MAIN_ADDPROJECT:
		{
			CURLDlg dlg;
			dlg.DoModal(hResource, IDD_URLCONFIG, *this);
			CUrlInfo * inf = dlg.GetInfo();
			if ((inf)&&inf->url.size())
			{
				map<wstring,CUrlInfo> * pWrite = m_pURLInfos->GetWriteData();
				if ((inf)&&inf->url.size())
				{
					(*pWrite)[inf->url] = *inf;
				}
				m_pURLInfos->ReleaseWriteData();
                m_pURLInfos->Save();
			}
			RefreshURLTree(false);
		}
		break;
	case ID_MAIN_SHOWDIFF:
		{
			ShowDiff(false);
		}
		break;
	case ID_MAIN_SHOWDIFFTSVN:
		{
			ShowDiff(true);
		}
		break;
	case ID_MAIN_SHOWDIFFCHOOSE:
		{
			wstring tsvninstalled = CAppUtils::GetTSVNPath();
			wstring sVer = CAppUtils::GetVersionStringFromExe(tsvninstalled.c_str());
			bool bUseTSVN = !(tsvninstalled.empty()) && (_tstoi(sVer.substr(3, 4).c_str()) > 4);
			bUseTSVN = bUseTSVN && !!CRegStdDWORD(_T("Software\\CommitMonitor\\UseTSVN"), TRUE);
			
			ShowDiff(bUseTSVN);
		}
		break;
	case ID_MISC_OPTIONS:
		{
			COptionsDlg dlg(*this);
			dlg.SetHiddenWnd(m_hParent);
			dlg.SetUrlInfos(m_pURLInfos);
			dlg.DoModal(hResource, IDD_OPTIONS, *this);
		}
		break;
	case ID_MISC_ABOUT:
		{
			::KillTimer(*this, TIMER_REFRESH);
			CAboutDlg dlg(*this);
			dlg.SetHiddenWnd(m_hParent);
			dlg.DoModal(hResource, IDD_ABOUTBOX, *this);
			::SetTimer(*this, TIMER_REFRESH, 1000, NULL);
		}
		break;
	case ID_POPUP_MARKALLASREAD:
		{
			CURLDlg dlg;
			HTREEITEM hItem = TreeView_GetSelection(m_hTreeControl);
			if (hItem)
			{
				MarkAllAsRead(hItem, true);
			}
		}
		break;
	default:
		return 0;
	}
	return 1;
}

void CMainDlg::SetRemoveButtonState()
{
	HWND hFocus = ::GetFocus();
	if (hFocus == m_hListControl)
	{
		SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_MAIN_REMOVE, MAKELONG(ListView_GetSelectedCount(m_hListControl) > 0, 0));
	}
	else
	{
		SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_MAIN_REMOVE, MAKELONG(TreeView_GetSelection(m_hTreeControl)!=0, 0));
	}
}

bool CMainDlg::ShowDiff(bool bUseTSVN)
{
	TCHAR buf[4096];
	// find the revision we have to show the diff for
	int selCount = ListView_GetSelectedCount(m_hListControl);
	if (selCount <= 0)
		return FALSE;	//nothing selected, nothing to show

	HTREEITEM hSelectedItem = TreeView_GetSelection(m_hTreeControl);
	// get the url this entry refers to
	TVITEMEX itemex = {0};
	itemex.hItem = hSelectedItem;
	itemex.mask = TVIF_PARAM;
	TreeView_GetItem(m_hTreeControl, &itemex);
	const map<wstring,CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
	if (pRead->find(*(wstring*)itemex.lParam) != pRead->end())
	{
		LVITEM item = {0};
		int nItemCount = ListView_GetItemCount(m_hListControl);
		for (int i=0; i<nItemCount; ++i)
		{
			item.mask = LVIF_PARAM|LVIF_STATE;
			item.stateMask = LVIS_SELECTED;
			item.iItem = i;
			ListView_GetItem(m_hListControl, &item);
			if (item.state & LVIS_SELECTED)
			{
				SVNLogEntry * pLogEntry = (SVNLogEntry*)item.lParam;
				// find the diff name
				const CUrlInfo * pInfo = &pRead->find(*(wstring*)itemex.lParam)->second;
				// in case the project name has 'path' chars in it, we have to remove those first
				_stprintf_s(buf, 4096, _T("%s_%ld.diff"), CAppUtils::ConvertName(pInfo->name).c_str(), pLogEntry->revision);
				wstring diffFileName = CAppUtils::GetDataDir();
				diffFileName += _T("\\");
				diffFileName += wstring(buf);
				// construct a title for the diff viewer
				_stprintf_s(buf, 4096, _T("%s, revision %ld"), pInfo->name.c_str(), pLogEntry->revision);
				wstring title = wstring(buf);
				// start the diff viewer
				wstring cmd;
				wstring tsvninstalled = CAppUtils::GetTSVNPath();
				wstring sVer = CAppUtils::GetVersionStringFromExe(tsvninstalled.c_str());
				if ((bUseTSVN)&&(!tsvninstalled.empty())&&(_tstoi(sVer.substr(3, 4).c_str()) > 4))
				{
					// yes, we have TSVN installed
					// call TortoiseProc to do the diff for us
					cmd = wstring(tsvninstalled);
					cmd += _T(" /command:diff /path:\"");
					cmd += pInfo->url;
					cmd += _T("\" /startrev:");

					TCHAR numBuf[100] = {0};
					_stprintf_s(numBuf, 100, _T("%ld"), pLogEntry->revision-1);
					cmd += numBuf;
					cmd += _T(" /endrev:"); 
					_stprintf_s(numBuf, 100, _T("%ld"), pLogEntry->revision);
					cmd += numBuf;
					CAppUtils::LaunchApplication(cmd);
				}
				else
				{
					TCHAR apppath[4096];
					GetModuleFileName(NULL, apppath, 4096);
					CRegStdString diffViewer = CRegStdString(_T("Software\\CommitMonitor\\DiffViewer"));
					if (wstring(diffViewer).empty())
					{
						cmd = apppath;
						cmd += _T(" /patchfile:\"");
					}
					else
					{
						cmd = (wstring)diffViewer;
						cmd += _T(" \"");
					}
					cmd += diffFileName;
					cmd += _T("\"");
					if (wstring(diffViewer).empty())
					{
						cmd += _T(" /title:\"");
						cmd += title;
						cmd += _T("\"");
					}
					// Check if the diff file exists. If it doesn't, we have to fetch
					// the diff first
					if (!PathFileExists(diffFileName.c_str()))
					{
						// fetch the diff
						SVN svn;
						svn.SetAuthInfo(pInfo->username, pInfo->password);
						CProgressDlg progDlg;
						svn.SetAndClearProgressInfo(&progDlg);
						progDlg.SetTitle(_T("Fetching Diff"));
						TCHAR dispbuf[MAX_PATH] = {0};
						_stprintf_s(dispbuf, MAX_PATH, _T("fetching diff of revision %ld"), pLogEntry->revision);
						progDlg.SetLine(1, dispbuf);
						progDlg.SetShowProgressBar(false);
						progDlg.ShowModeless(*this);
						if (!svn.Diff(pInfo->url, pLogEntry->revision, pLogEntry->revision-1, pLogEntry->revision, true, true, false, wstring(), false, diffFileName, wstring()))
						{
							progDlg.Stop();
							if (svn.Err->apr_err != SVN_ERR_CANCELLED)
								::MessageBox(*this, svn.GetLastErrorMsg().c_str(), _T("CommitMonitor"), MB_ICONERROR);
							DeleteFile(diffFileName.c_str());
						}
						else
						{
							TRACE(_T("Diff fetched for %s, revision %ld\n"), pInfo->url.c_str(), pLogEntry->revision);
							progDlg.Stop();
						}
					}
					if (PathFileExists(diffFileName.c_str()))
						CAppUtils::LaunchApplication(cmd);
				}
			}
		}
	}
	m_pURLInfos->ReleaseReadOnlyData();
	return TRUE;
}

/******************************************************************************/
/* tree handling                                                              */
/******************************************************************************/
void CMainDlg::RefreshURLTree(bool bSelectUnread)
{
	// the m_URLInfos member must be up-to-date here

	m_bBlockListCtrlUI = true;
	// first clear the controls (the data)
	ListView_DeleteAllItems(m_hListControl);
	TreeView_SelectItem(m_hTreeControl, NULL);
	TreeView_DeleteAllItems(m_hTreeControl);
	SetWindowText(m_hLogMsgControl, _T(""));
	SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_MAIN_SHOWDIFFTSVN, MAKELONG(false, 0));
	SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_MAIN_EDIT, MAKELONG(false, 0));
	SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_MAIN_REMOVE, MAKELONG(false, 0));

	HTREEITEM tvToSel = 0;

	// now add a tree item for every entry in m_URLInfos
	const map<wstring, CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
	for (map<wstring, CUrlInfo>::const_iterator it = pRead->begin(); it != pRead->end(); ++it)
	{
		TVINSERTSTRUCT tv = {0};
		tv.hParent = FindParentTreeNode(it->first);
		tv.hInsertAfter = TVI_SORT;
		tv.itemex.mask = TVIF_TEXT|TVIF_PARAM|TVIF_STATE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
		WCHAR * str = new WCHAR[it->second.name.size()+10];
		// find out if there are some unread entries
		int unread = 0;
		for (map<svn_revnum_t,SVNLogEntry>::const_iterator logit = it->second.logentries.begin(); logit != it->second.logentries.end(); ++logit)
		{
			if (!logit->second.read)
				unread++;
		}
		if (unread)
		{
			_stprintf_s(str, it->second.name.size()+10, _T("%s (%d)"), it->second.name.c_str(), unread);
			tv.itemex.state = TVIS_BOLD;
			tv.itemex.stateMask = TVIS_BOLD;
		}
		else
		{
			_tcscpy_s(str, it->second.name.size()+1, it->second.name.c_str());
			tv.itemex.state = 0;
			tv.itemex.stateMask = 0;
		}
		tv.itemex.pszText = str;
		tv.itemex.lParam = (LPARAM)&it->first;
		if (it->second.parentpath)
		{
			tv.itemex.iImage = 0;
			tv.itemex.iSelectedImage = 1;
		}
		else
		{
			if (!it->second.error.empty())
			{
				tv.itemex.iImage = 4;
				tv.itemex.iSelectedImage = 4;
			}
			else if (unread)
			{
				tv.itemex.iImage = 3;
				tv.itemex.iSelectedImage = 3;
			}
			else
			{
				tv.itemex.iImage = 2;
				tv.itemex.iSelectedImage = 2;
			}
		}
		HTREEITEM hItem = TreeView_InsertItem(m_hTreeControl, &tv);
		if ((unread)&&(tvToSel == 0))
			tvToSel = hItem;
		TreeView_Expand(m_hTreeControl, tv.hParent, TVE_EXPAND);
		delete [] str;
	}
	m_pURLInfos->ReleaseReadOnlyData();
	m_bBlockListCtrlUI = false;
	if ((tvToSel)&&(bSelectUnread))
	{
		TreeView_SelectItem(m_hTreeControl, tvToSel);
	}
	else if (tvToSel == NULL)
	{
		tvToSel = TreeView_GetRoot(m_hTreeControl);
		if (TreeView_GetChild(m_hTreeControl, tvToSel))
			tvToSel = TreeView_GetChild(m_hTreeControl, tvToSel);
		TreeView_SelectItem(m_hTreeControl, tvToSel);
	}
	::InvalidateRect(m_hListControl, NULL, true);
}

LRESULT CMainDlg::OnCustomDrawTreeItem(LPNMTVCUSTOMDRAW lpNMCustomDraw)
{
	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.
	LRESULT result =  CDRF_DODEFAULT;

	switch (lpNMCustomDraw->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		result = CDRF_NOTIFYITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT:
		{
			if (!m_bBlockListCtrlUI)
			{
				const map<wstring,CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
				const CUrlInfo * info = &pRead->find(*(wstring*)lpNMCustomDraw->nmcd.lItemlParam)->second;
				COLORREF crText = lpNMCustomDraw->clrText;

				if ((info)&&(!info->error.empty() && !info->parentpath))
				{
					crText = GetSysColor(COLOR_GRAYTEXT);
				}
				m_pURLInfos->ReleaseReadOnlyData();
				// Store the color back in the NMLVCUSTOMDRAW struct.
				lpNMCustomDraw->clrText = crText;
			}
		}
		break;
	}
	return result;
}

HTREEITEM CMainDlg::FindParentTreeNode(const wstring& url)
{
	size_t pos = url.find_last_of('/');
	wstring parenturl = url.substr(0, pos);
	do 
	{
		const map<wstring, CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
		if (pRead->find(parenturl) != pRead->end())
		{
			m_pURLInfos->ReleaseReadOnlyData();
			// we found a parent URL, but now we have to find it in the
			// tree view
			return FindTreeNode(parenturl);
		}
		m_pURLInfos->ReleaseReadOnlyData();
		pos = parenturl.find_last_of('/');
		parenturl = parenturl.substr(0, pos);
		if (pos == string::npos)
			parenturl.clear();
	} while (!parenturl.empty());
	return TVI_ROOT;
}

HTREEITEM CMainDlg::FindTreeNode(const wstring& url, HTREEITEM hItem)
{
	if (hItem == TVI_ROOT)
		hItem = TreeView_GetRoot(m_hTreeControl);
	TVITEM item;
	item.mask = TVIF_PARAM;
	while (hItem)
	{
		item.hItem = hItem;
		TreeView_GetItem(m_hTreeControl, &item);
		if (url.compare(*(wstring*)item.lParam) == 0)
			return hItem;
		HTREEITEM hChild = TreeView_GetChild(m_hTreeControl, hItem);
		if (hChild)
		{
			item.hItem = hChild;
			TreeView_GetItem(m_hTreeControl, &item);
			hChild = FindTreeNode(url, hChild);
			if (hChild != TVI_ROOT)
				return hChild;
		}
		hItem = TreeView_GetNextSibling(m_hTreeControl, hItem);
	};
	return TVI_ROOT;
}

bool CMainDlg::SelectNextWithUnread(HTREEITEM hItem)
{
	if (hItem == TVI_ROOT)
		hItem = TreeView_GetRoot(m_hTreeControl);
	TVITEM item;
	item.mask = TVIF_STATE;
	item.stateMask = TVIS_BOLD;
	while (hItem)
	{
		item.hItem = hItem;
		TreeView_GetItem(m_hTreeControl, &item);
		if (item.state & TVIS_BOLD)
		{
			TreeView_SelectItem(m_hTreeControl, hItem);
			TreeItemSelected(m_hTreeControl, hItem);
			ListView_SetSelectionMark(m_hListControl, 0);
			ListView_SetItemState(m_hListControl, 0, LVIS_SELECTED, LVIS_SELECTED);
			::SetFocus(m_hListControl);
			return true;
		}
		HTREEITEM hChild = TreeView_GetChild(m_hTreeControl, hItem);
		if (hChild)
		{
			item.hItem = hChild;
			TreeView_GetItem(m_hTreeControl, &item);
			if (SelectNextWithUnread(hChild))
				return true;
		}
		hItem = TreeView_GetNextSibling(m_hTreeControl, hItem);
	};
	return false;
}


void CMainDlg::OnSelectTreeItem(LPNMTREEVIEW lpNMTreeView)
{
	HTREEITEM hSelectedItem = lpNMTreeView->itemNew.hItem;
	SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_MAIN_EDIT, 
		MAKELONG(!!(lpNMTreeView->itemNew.state & TVIS_SELECTED), 0));
	SetRemoveButtonState();
	if (lpNMTreeView->itemNew.state & TVIS_SELECTED)
	{
		TreeItemSelected(lpNMTreeView->hdr.hwndFrom, hSelectedItem);
	}
	else
	{
		ListView_DeleteAllItems(m_hListControl);
		SetWindowText(m_hLogMsgControl, _T(""));
		SetDlgItemText(*this, IDC_INFOLABEL, _T(""));
	}
	SetWindowText(m_hLogMsgControl, _T(""));
}

void CMainDlg::TreeItemSelected(HWND hTreeControl, HTREEITEM hSelectedItem)
{
	// get the url this entry refers to
	TVITEMEX itemex = {0};
	itemex.hItem = hSelectedItem;
	itemex.mask = TVIF_PARAM;
	TreeView_GetItem(hTreeControl, &itemex);
	const map<wstring,CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
	if (pRead->find(*(wstring*)itemex.lParam) != pRead->end())
	{
		const CUrlInfo * info = &pRead->find(*(wstring*)itemex.lParam)->second;

		if ((!info->error.empty())&&(!info->parentpath))
		{
			// there was an error when we last tried to access this url.
			// Show a message box with the error.
			int len = info->error.length()+info->url.length()+1024;
			TCHAR * pBuf = new TCHAR[len];
			_stprintf_s(pBuf, len, _T("An error occurred the last time CommitMonitor\ntried to access the url: %s\n\n%s\n\nDoubleclick here to clear the error message."), info->url.c_str(), info->error.c_str());
			m_ListCtrl.SetInfoText(pBuf);
			delete [] pBuf;
		}
		else
			// remove the info text if there's no error
			m_ListCtrl.SetInfoText(_T(""));

		// show the last update time on the info label
		TCHAR updateTime[1000] = {0};
		struct tm upTime;
		if (_localtime64_s(&upTime, &info->lastchecked) == 0)
		{
			_tcsftime(updateTime, 1000, _T(" last checked: %X"), &upTime);
			SetDlgItemText(*this, IDC_INFOLABEL, updateTime);
		}


		m_bBlockListCtrlUI = true;
		DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
		ListView_DeleteAllItems(m_hListControl);

		int c = Header_GetItemCount(ListView_GetHeader(m_hListControl))-1;
		while (c>=0)
			ListView_DeleteColumn(m_hListControl, c--);

		ListView_SetExtendedListViewStyle(m_hListControl, exStyle);
		LVCOLUMN lvc = {0};
		lvc.mask = LVCF_TEXT;
		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = -1;
		lvc.pszText = _T("revision");
		ListView_InsertColumn(m_hListControl, 0, &lvc);
		lvc.pszText = _T("date");
		ListView_InsertColumn(m_hListControl, 1, &lvc);
		lvc.pszText = _T("author");
		ListView_InsertColumn(m_hListControl, 2, &lvc);
		lvc.pszText = _T("log message");
		ListView_InsertColumn(m_hListControl, 3, &lvc);

		LVITEM item = {0};
		TCHAR buf[1024];
		int iLastUnread = -1;

		int len = GetWindowTextLength(m_hFilterControl);
		WCHAR * buffer = new WCHAR[len+1];
		GetDlgItemText(*this, IDC_FILTERSTRING, buffer, len+1);
		wstring filterstring = wstring(buffer, len);
		bool bNegateFilter = filterstring[0] == '-';
		if (bNegateFilter)
		{
			filterstring = filterstring.substr(1);
		}
		wstring filterstringlower = filterstring;
		std::transform(filterstringlower.begin(), filterstringlower.end(), filterstringlower.begin(), std::tolower);

		delete [] buffer;

		for (map<svn_revnum_t,SVNLogEntry>::const_iterator it = info->logentries.begin(); it != info->logentries.end(); ++it)
		{
			// only add entries that match the filter string
			bool addEntry = true;
			bool bUseRegex = (filterstring[0] == '\\')&&(filterstring.size() > 1);

			if (bUseRegex)
			{
				try
				{
					const tr1::wregex regCheck(filterstring.substr(1), tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);

					addEntry = tr1::regex_search(it->second.author, regCheck);
					if (!addEntry)
					{
						addEntry = tr1::regex_search(it->second.message, regCheck);
						if (!addEntry)
						{
							_stprintf_s(buf, 1024, _T("%ld"), it->first);
							wstring s = wstring(buf);
							addEntry = tr1::regex_search(s, regCheck);
						}
					}
				}
				catch (exception) 
				{
					bUseRegex = false;
				}
				if (bNegateFilter)
					addEntry = !addEntry;
			}
			if (!bUseRegex)
			{
				// search plain text
				// note: \Q...\E doesn't seem to work with tr1 - it still
				// throws an exception if the regex in between is not a valid regex :(

				wstring s = it->second.author;
				std::transform(s.begin(), s.end(), s.begin(), std::tolower);
				addEntry = s.find(filterstringlower) != wstring::npos;

				if (!addEntry)
				{
					s = it->second.message;
					std::transform(s.begin(), s.end(), s.begin(), std::tolower);
					addEntry = s.find(filterstringlower) != wstring::npos;
					if (!addEntry)
					{
						_stprintf_s(buf, 1024, _T("%ld"), it->first);
						addEntry = s.find(buf) != wstring::npos;
					}
				}
				if (bNegateFilter)
					addEntry = !addEntry;
			}

			if (!addEntry)
				continue;

			item.mask = LVIF_TEXT|LVIF_PARAM;
			item.iItem = 0;
			item.lParam = (LPARAM)&it->second;
			_stprintf_s(buf, 1024, _T("%ld"), it->first);
			item.pszText = buf;
			ListView_InsertItem(m_hListControl, &item);
			if (it->second.date)
				_tcscpy_s(buf, 1024, CAppUtils::ConvertDate(it->second.date).c_str());
			else
				_tcscpy_s(buf, 1024, _T("(no date)"));
			ListView_SetItemText(m_hListControl, 0, 1, buf);
			if (it->second.author.size())
				_tcscpy_s(buf, 1024, it->second.author.c_str());
			else
				_tcscpy_s(buf, 1024, _T("(no author)"));
			ListView_SetItemText(m_hListControl, 0, 2, buf);
			wstring msg = it->second.message;
			std::remove(msg.begin(), msg.end(), '\r');
			std::replace(msg.begin(), msg.end(), '\n', ' ');
			std::replace(msg.begin(), msg.end(), '\t', ' ');
			_tcsncpy_s(buf, 1024, msg.c_str(), 1023);
			ListView_SetItemText(m_hListControl, 0, 3, buf);

			if ((iLastUnread < 0)&&(!it->second.read))
			{
				iLastUnread = 0;
			}
			if (iLastUnread >= 0)
				iLastUnread++;
		}
		m_bBlockListCtrlUI = false;
		ListView_SetColumnWidth(m_hListControl, 0, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(m_hListControl, 1, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(m_hListControl, 2, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(m_hListControl, 3, LVSCW_AUTOSIZE_USEHEADER);
		ListView_EnsureVisible(m_hListControl, iLastUnread-1, FALSE);

		::InvalidateRect(m_hListControl, NULL, false);
	}
	m_pURLInfos->ReleaseReadOnlyData();
}

void CMainDlg::MarkAllAsRead(HTREEITEM hItem, bool includingChildren)
{
	// get the url this entry refers to
	TVITEMEX itemex = {0};
	itemex.hItem = hItem;
	itemex.mask = TVIF_PARAM;
	TreeView_GetItem(m_hTreeControl, &itemex);
	map<wstring,CUrlInfo> * pWrite = m_pURLInfos->GetWriteData();
	bool bChanged = false;
	if (pWrite->find(*(wstring*)itemex.lParam) != pWrite->end())
	{
		CUrlInfo * info = &pWrite->find(*(wstring*)itemex.lParam)->second;

		for (map<svn_revnum_t,SVNLogEntry>::iterator it = info->logentries.begin(); it != info->logentries.end(); ++it)
		{
			if (!it->second.read)
				bChanged = true;
			it->second.read = true;
		}
		// refresh the name of the tree item to indicate the new
		// number of unread log messages
		WCHAR * str = new WCHAR[info->name.size()+10];
		_stprintf_s(str, info->name.size()+10, _T("%s"), info->name.c_str());
		itemex.state = 0;
		itemex.stateMask = TVIS_BOLD;
		itemex.pszText = str;
		if (info->parentpath)
		{
			itemex.iImage = 0;
			itemex.iSelectedImage = 1;
		}
		else
		{
			itemex.iImage = 2;
			itemex.iSelectedImage = 2;
		}
		itemex.mask = TVIF_TEXT|TVIF_STATE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
		TreeView_SetItem(m_hTreeControl, &itemex);
		delete [] str;
	}
	m_pURLInfos->ReleaseWriteData();
	if (includingChildren)
	{
		HTREEITEM hFirstChild = TreeView_GetChild(m_hTreeControl, hItem);
		if (hFirstChild)
		{
			MarkAllAsRead(hFirstChild, includingChildren);
			HTREEITEM hNextSibling = TreeView_GetNextSibling(m_hTreeControl, hFirstChild);
			while (hNextSibling)
			{
				MarkAllAsRead(hNextSibling, includingChildren);
				hNextSibling = TreeView_GetNextSibling(m_hTreeControl, hNextSibling);
			}
		}
	}
	if (bChanged)
		::SendMessage(m_hParent, COMMITMONITOR_CHANGEDINFO, (WPARAM)false, (LPARAM)0);
}

void CMainDlg::CheckNow(HTREEITEM hItem)
{
	// get the url this entry refers to
	TVITEMEX itemex = {0};
	itemex.hItem = hItem;
	itemex.mask = TVIF_PARAM;
	TreeView_GetItem(m_hTreeControl, &itemex);
	map<wstring,CUrlInfo> * pWrite = m_pURLInfos->GetWriteData();
	wstring url;
	if (pWrite->find(*(wstring*)itemex.lParam) != pWrite->end())
	{
		CUrlInfo * info = &pWrite->find(*(wstring*)itemex.lParam)->second;
		url = info->url;
	}
	m_pURLInfos->ReleaseWriteData();
	SendMessage(m_hParent, COMMITMONITOR_GETALL, 0, (LPARAM)url.c_str());
}

void CMainDlg::RefreshAll(HTREEITEM hItem)
{
	// get the url this entry refers to
	TVITEMEX itemex = {0};
	itemex.hItem = hItem;
	itemex.mask = TVIF_PARAM;
	TreeView_GetItem(m_hTreeControl, &itemex);
	map<wstring,CUrlInfo> * pWrite = m_pURLInfos->GetWriteData();
	wstring url;
	if (pWrite->find(*(wstring*)itemex.lParam) != pWrite->end())
	{
		CUrlInfo * info = &pWrite->find(*(wstring*)itemex.lParam)->second;

		svn_revnum_t lowestRev = 0;
		map<svn_revnum_t,SVNLogEntry>::iterator it = info->logentries.begin();
		if (it != info->logentries.end())
		{
			lowestRev = it->second.revision;
		}
		// set the 'last checked revision to the lowest revision so that
		// all the subsequent revisions are fetched again.
		info->lastcheckedrev = lowestRev > 0 ? lowestRev-1 : lowestRev;
		// and make sure this repository is checked even if the timeout has
		// not been reached yet on the next fetch round
		info->lastchecked = 0;
		url = info->url;
	}
	m_pURLInfos->ReleaseWriteData();
	SendMessage(m_hParent, COMMITMONITOR_GETALL, 0, (LPARAM)url.c_str());
}

/******************************************************************************/
/* list view handling                                                         */
/******************************************************************************/
void CMainDlg::OnSelectListItem(LPNMLISTVIEW lpNMListView)
{
	if ((m_bBlockListCtrlUI)||(m_ListCtrl.InfoTextShown()))
		return;
	if ((lpNMListView->uOldState ^ lpNMListView->uNewState) & LVIS_SELECTED)
	{
		const map<wstring,CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
		LVITEM item = {0};
		item.mask = LVIF_PARAM;
		item.iItem = lpNMListView->iItem;
		ListView_GetItem(m_hListControl, &item);
		SVNLogEntry * pLogEntry = (SVNLogEntry*)item.lParam;
		if (pLogEntry)
		{
			HTREEITEM hSelectedItem = TreeView_GetSelection(m_hTreeControl);
			// get the url this entry refers to
			TVITEMEX itemex = {0};
			itemex.hItem = hSelectedItem;
			itemex.mask = TVIF_PARAM;
			TreeView_GetItem(m_hTreeControl, &itemex);
			if (itemex.lParam == 0)
			{
				m_pURLInfos->ReleaseReadOnlyData();
				return;
			}
			// set the entry as read
            if ((!pLogEntry->read)&&(lpNMListView->uNewState & LVIS_SELECTED))
            {
                pLogEntry->read = true;
                // refresh the name of the tree item to indicate the new
                // number of unread log messages
                // e.g. instead of 'TortoiseSVN (3)', show now 'TortoiseSVN (2)'
                if (pRead->find(*(wstring*)itemex.lParam) != pRead->end())
                {
                    const CUrlInfo * uinfo = &pRead->find(*(wstring*)itemex.lParam)->second;
                    // count the number of unread messages
                    int unread = 0;
                    for (map<svn_revnum_t,SVNLogEntry>::const_iterator it = uinfo->logentries.begin(); it != uinfo->logentries.end(); ++it)
                    {
                        if (!it->second.read)
                            unread++;
                    }
                    WCHAR * str = new WCHAR[uinfo->name.size()+10];
                    if (unread)
                    {
                        _stprintf_s(str, uinfo->name.size()+10, _T("%s (%d)"), uinfo->name.c_str(), unread);
                        itemex.state = TVIS_BOLD;
                        itemex.stateMask = TVIS_BOLD;
						itemex.iImage = 3;
						itemex.iSelectedImage = 3;
                    }
                    else
                    {
                        _stprintf_s(str, uinfo->name.size()+10, _T("%s"), uinfo->name.c_str());
                        itemex.state = 0;
                        itemex.stateMask = TVIS_BOLD;
						itemex.iImage = 2;
						itemex.iSelectedImage = 2;
                    }

                    itemex.pszText = str;
                    itemex.mask = TVIF_TEXT|TVIF_STATE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
                    TreeView_SetItem(m_hTreeControl, &itemex);
                }
				// the icon in the system tray needs to be changed back
				// to 'normal'
				::SendMessage(m_hParent, COMMITMONITOR_CHANGEDINFO, (WPARAM)false, (LPARAM)0);
            }
			TCHAR buf[1024];
			wstring msg;
			if (ListView_GetSelectedCount(m_hListControl) > 1)
			{
				msg = _T("multiple log entries selected. Info for the last selected one:\n-------------------------------\n\n");
			}
			msg += pLogEntry->message.c_str();
			msg += _T("\n\n-------------------------------\n");
			// now add all changed paths, one path per line
			for (map<std::wstring, SVNLogChangedPaths>::const_iterator it = pLogEntry->m_changedPaths.begin(); it != pLogEntry->m_changedPaths.end(); ++it)
			{
				// action
				msg += it->second.action;
				msg += _T(" : ");
				msg += it->first;
				msg += _T("  ");
				if (!it->second.copyfrom_path.empty())
				{
					msg += _T("(copied from: ");
					msg += it->second.copyfrom_path;
					msg += _T(", revision ");
					_stprintf_s(buf, 1024, _T("%ld)\n"), it->second.copyfrom_revision);
					msg += wstring(buf);
				}
				else
					msg += _T("\n");
			}

			CAppUtils::SearchReplace(msg, _T("\n"), _T("\r\n"));
			SetWindowText(m_hLogMsgControl, msg.c_str());

			// find the diff name
			_stprintf_s(buf, 1024, _T("%s_%ld.diff"), pRead->find(*(wstring*)itemex.lParam)->second.name.c_str(), pLogEntry->revision);
			wstring diffFileName = CAppUtils::GetDataDir();
			diffFileName += _T("\\");
			diffFileName += wstring(buf);
			SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_MAIN_SHOWDIFFCHOOSE, MAKELONG(true, 0));
		}
		m_pURLInfos->ReleaseReadOnlyData();
	}
}

void CMainDlg::OnDblClickListItem(LPNMITEMACTIVATE /*lpnmitem*/)
{
	::SendMessage(*this, WM_COMMAND, MAKELONG(ID_MAIN_SHOWDIFFCHOOSE, 0), 0);
}

LRESULT CMainDlg::OnCustomDrawListItem(LPNMLVCUSTOMDRAW lpNMCustomDraw)
{
	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.
	LRESULT result =  CDRF_DODEFAULT;
	if (m_bBlockListCtrlUI)
		return result;
	switch (lpNMCustomDraw->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		result = CDRF_NOTIFYITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT:
		{
			SVNLogEntry * pLogEntry = (SVNLogEntry*)lpNMCustomDraw->nmcd.lItemlParam;

			if (!pLogEntry->read)
			{
				SelectObject(lpNMCustomDraw->nmcd.hdc, m_boldFont);
				// We changed the font, so we're returning CDRF_NEWFONT. This
				// tells the control to recalculate the extent of the text.
				result = CDRF_NEWFONT;
			}
		}
		break;
	}
	return result;
}

void CMainDlg::OnKeyDownListItem(LPNMLVKEYDOWN pnkd)
{
	switch (pnkd->wVKey)
	{
	case VK_DELETE:
		RemoveSelectedListItems();
		break;
	case 'A':
		if (GetKeyState(VK_CONTROL)&0x8000)
		{
			// select all
			int nCount = ListView_GetItemCount(m_hListControl);
			if (nCount > 1)
			{
				m_bBlockListCtrlUI = true;
				for (int i=0; i<(nCount-1); ++i)
				{
					ListView_SetItemState(m_hListControl, i, LVIS_SELECTED, LVIS_SELECTED);
				}
				m_bBlockListCtrlUI = false;
				ListView_SetItemState(m_hListControl, nCount-1, LVIS_SELECTED, LVIS_SELECTED);
				// clear the text of the selected log message: there are more than
				// one selected now
				SetWindowText(m_hLogMsgControl, _T(""));
			}
		}
		break;
	case 'N':	// next unread
		{
			int selMark = ListView_GetSelectionMark(m_hListControl);
			if (selMark >= 0)
			{
				// find the next unread message
				LVITEM item = {0};
				int i = selMark + 1;
				int nCount = ListView_GetItemCount(m_hListControl);
				do 
				{
					item.mask = LVIF_PARAM;
					item.iItem = i;
					if (ListView_GetItem(m_hListControl, &item))
					{
						SVNLogEntry * pLogEntry = (SVNLogEntry*)item.lParam;
						if ((pLogEntry)&&(!pLogEntry->read))
						{
							// we have the next unread
							ListView_SetSelectionMark(m_hListControl, i);
							ListView_SetItemState(m_hListControl, selMark, 0, LVIS_SELECTED);
							ListView_SetItemState(m_hListControl, i, LVIS_SELECTED, LVIS_SELECTED);
							break;
						}

					}
					++i;
				} while (i < nCount);
				
				if (i == nCount)
				{
					// no unread item found anymore.
					if (!SelectNextWithUnread())
					{
						// also no unread items in other projects
						int selMark = ListView_GetSelectionMark(m_hListControl);
						if (selMark < ListView_GetItemCount(m_hListControl))
						{
							ListView_SetItemState(m_hListControl, selMark, 0, LVIS_SELECTED);
							ListView_SetSelectionMark(m_hListControl, selMark+1);
							ListView_SetItemState(m_hListControl, selMark+1, LVIS_SELECTED, LVIS_SELECTED);
							ListView_EnsureVisible(m_hListControl, selMark+1, false);
						}
					}
				}
			}
		}
		break;
	case 'B':	// back one message
		{
			int selMark = ListView_GetSelectionMark(m_hListControl);
			if (selMark > 0)
			{
				ListView_SetItemState(m_hListControl, selMark, 0, LVIS_SELECTED);
				ListView_SetSelectionMark(m_hListControl, selMark-1);
				ListView_SetItemState(m_hListControl, selMark-1, LVIS_SELECTED, LVIS_SELECTED);
				ListView_EnsureVisible(m_hListControl, selMark-1, false);
			}
		}
		break;
	}
}

void CMainDlg::RemoveSelectedListItems()
{
	int selCount = ListView_GetSelectedCount(m_hListControl);
	if (selCount <= 0)
		return;	//nothing selected, nothing to remove
	int nFirstDeleted = -1;
	HTREEITEM hSelectedItem = TreeView_GetSelection(m_hTreeControl);
	// get the url this entry refers to
	TVITEMEX itemex = {0};
	itemex.hItem = hSelectedItem;
	itemex.mask = TVIF_PARAM;
	TreeView_GetItem(m_hTreeControl, &itemex);
	map<wstring,CUrlInfo> * pWrite = m_pURLInfos->GetWriteData();
	if (pWrite->find(*(wstring*)itemex.lParam) != pWrite->end())
	{
		LVITEM item = {0};
		int i = 0;
		TCHAR buf[4096];
		m_bBlockListCtrlUI = true;
		while (i<ListView_GetItemCount(m_hListControl))
		{
			item.mask = LVIF_PARAM|LVIF_STATE;
			item.stateMask = LVIS_SELECTED;
			item.iItem = i;
			item.lParam = 0;
			ListView_GetItem(m_hListControl, &item);
			if (item.state & LVIS_SELECTED)
			{
				SVNLogEntry * pLogEntry = (SVNLogEntry*)item.lParam;
				// find the diff name
				_stprintf_s(buf, 4096, _T("%s_%ld.diff"), pWrite->find(*(wstring*)itemex.lParam)->second.name.c_str(), pLogEntry->revision);
				wstring diffFileName = CAppUtils::GetDataDir();
				diffFileName += _T("\\");
				diffFileName += wstring(buf);
				DeleteFile(diffFileName.c_str());

				pWrite->find((*(wstring*)itemex.lParam))->second.logentries.erase(pLogEntry->revision);
				ListView_DeleteItem(m_hListControl, i);
				if (nFirstDeleted < 0)
					nFirstDeleted = i;
			}
			else
				++i;
		}
		m_bBlockListCtrlUI = false;
	}
	m_pURLInfos->ReleaseWriteData();
	if (nFirstDeleted >= 0)
	{
		if (ListView_GetItemCount(m_hListControl) > nFirstDeleted)
		{
			ListView_SetItemState(m_hListControl, nFirstDeleted, LVIS_SELECTED, LVIS_SELECTED);
		}
		else
		{
			ListView_SetItemState(m_hListControl, ListView_GetItemCount(m_hListControl)-1, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
	SetRemoveButtonState();
}

/******************************************************************************/
/* tree, list view and dialog resizing                                        */
/******************************************************************************/

void CMainDlg::DoResize(int width, int height)
{
	// when we get here, the controls haven't been resized yet
	RECT tree, list, log, ex, ok, label, filterlabel, filterbox;
	HWND hExit = GetDlgItem(*this, IDC_EXIT);
	HWND hOK = GetDlgItem(*this, IDOK);
	HWND hLabel = GetDlgItem(*this, IDC_INFOLABEL);
	HWND hFilterLabel = GetDlgItem(*this, IDC_FILTERLABEL);
	::GetClientRect(m_hTreeControl, &tree);
	::GetClientRect(m_hListControl, &list);
	::GetClientRect(m_hLogMsgControl, &log);
	::GetClientRect(hExit, &ex);
	::GetClientRect(hOK, &ok);
	::GetClientRect(hLabel, &label);
	::GetClientRect(hFilterLabel, &filterlabel);
	::GetClientRect(m_hFilterControl, &filterbox);
	::InvalidateRect(*this, NULL, TRUE);
	HDWP hdwp = BeginDeferWindowPos(9);
	hdwp = DeferWindowPos(hdwp, m_hwndToolbar, *this, 0, 0, width, m_topmarg, SWP_NOZORDER|SWP_NOACTIVATE);
	hdwp = DeferWindowPos(hdwp, hFilterLabel, *this, m_xSliderPos+4, m_topmarg+5, FILTERLABELWIDTH, 12, SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
	hdwp = DeferWindowPos(hdwp, m_hFilterControl, *this, m_xSliderPos+4+FILTERLABELWIDTH, m_topmarg+1, width-m_xSliderPos-4-FILTERLABELWIDTH-4, FILTERBOXHEIGHT-1, SWP_NOZORDER|SWP_NOACTIVATE);
	hdwp = DeferWindowPos(hdwp, m_hTreeControl, *this, 0, m_topmarg, m_xSliderPos, height-m_topmarg-m_bottommarg+FILTERBOXHEIGHT+4, SWP_NOZORDER|SWP_NOACTIVATE);
	hdwp = DeferWindowPos(hdwp, m_hListControl, *this, m_xSliderPos+4, m_topmarg+FILTERBOXHEIGHT, width-m_xSliderPos-4, m_ySliderPos-m_topmarg+4, SWP_NOZORDER|SWP_NOACTIVATE);
	hdwp = DeferWindowPos(hdwp, m_hLogMsgControl, *this, m_xSliderPos+4, m_ySliderPos+8+FILTERBOXHEIGHT, width-m_xSliderPos-4, height-m_bottommarg-m_ySliderPos-4, SWP_NOZORDER|SWP_NOACTIVATE);
	hdwp = DeferWindowPos(hdwp, hExit, *this, width-ok.right+ok.left-ex.right+ex.left-3, height-ex.bottom+ex.top, ex.right-ex.left, ex.bottom-ex.top, SWP_NOZORDER|SWP_NOACTIVATE);
	hdwp = DeferWindowPos(hdwp, hOK, *this, width-ok.right+ok.left, height-ok.bottom+ok.top, ok.right-ok.left, ok.bottom-ok.top, SWP_NOZORDER|SWP_NOACTIVATE);
	hdwp = DeferWindowPos(hdwp, hLabel, *this, 2, height-label.bottom+label.top+2, width-ok.right-ex.right-8, ex.bottom-ex.top, SWP_NOZORDER|SWP_NOACTIVATE);
	EndDeferWindowPos(hdwp);
}

bool CMainDlg::OnSetCursor(HWND hWnd, UINT nHitTest, UINT message) 
{
	UNREFERENCED_PARAMETER(message);
	UNREFERENCED_PARAMETER(nHitTest);
	if (hWnd == *this)
	{
		RECT rect;
		POINT pt;
		GetClientRect(*this, &rect);
		GetCursorPos(&pt);
		ScreenToClient(*this, &pt);
		if (PtInRect(&rect, pt))
		{
			ClientToScreen(*this, &pt);
			// are we right of the tree control?

			::GetWindowRect(m_hTreeControl, &rect);
			if ((pt.x > rect.right)&&
				(pt.y >= rect.top)&&
				(pt.y <= rect.bottom))
			{
				// but left of the list control?
				::GetWindowRect(m_hListControl, &rect);
				if (pt.x < rect.left)
				{
					HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZEWE));
					SetCursor(hCur);
					return TRUE;
				}

				// maybe we are below the log message list control?
				if (pt.y > rect.bottom)
				{
					::GetWindowRect(m_hLogMsgControl, &rect);
					if (pt.y < rect.top)
					{
						HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZENS));
						SetCursor(hCur);
						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}

bool CMainDlg::OnMouseMove(UINT nFlags, POINT point)
{
	HDC hDC;
	RECT rect, tree, list, treelist, treelistclient, logrect, loglist, loglistclient;

	if (m_nDragMode == DRAGMODE_NONE)
		return false;

	// create an union of the tree and list control rectangle
	::GetWindowRect(m_hListControl, &list);
	::GetWindowRect(m_hTreeControl, &tree);
	::GetWindowRect(m_hLogMsgControl, &logrect);
	UnionRect(&treelist, &tree, &list);
	treelistclient = treelist;
	MapWindowPoints(NULL, *this, (LPPOINT)&treelistclient, 2);

	UnionRect(&loglist, &logrect, &list);
	loglistclient = loglist;
	MapWindowPoints(NULL, *this, (LPPOINT)&loglistclient, 2);

	//convert the mouse coordinates relative to the top-left of
	//the window
	ClientToScreen(*this, &point);
	GetClientRect(*this, &rect);
	MapWindowPoints(*this, NULL, (LPPOINT)&rect, 2);
	point.x -= rect.left;
	point.y -= rect.top;

	//same for the window coordinates - make them relative to 0,0
	LONG tempy = rect.top;
	LONG tempx = rect.left;
	OffsetRect(&treelist, -tempx, -tempy);
	OffsetRect(&loglist, -tempx, -tempy);

	if (point.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.y > loglist.bottom-REPOBROWSER_CTRL_MIN_HEIGHT)
		point.y = loglist.bottom-REPOBROWSER_CTRL_MIN_HEIGHT;
	if (point.y < loglist.top+REPOBROWSER_CTRL_MIN_HEIGHT)
		point.y = loglist.top+REPOBROWSER_CTRL_MIN_HEIGHT;

	if ((nFlags & MK_LBUTTON) && ((point.x != m_oldx)||(point.y != m_oldy)))
	{
		hDC = GetDC(*this);

		if (hDC)
		{
			if (m_nDragMode == DRAGMODE_HORIZONTAL)
			{
				DrawXorBar(hDC, m_oldx+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);
				DrawXorBar(hDC, point.x+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);
			}
			else
			{
				DrawXorBar(hDC, loglistclient.left, m_oldy+2, loglistclient.right-loglistclient.left-2, 4);
				DrawXorBar(hDC, loglistclient.left, point.y+2, loglistclient.right-loglistclient.left-2, 4);
			}

			ReleaseDC(*this, hDC);
		}

		m_oldx = point.x;
		m_oldy = point.y;
	}

	return true;
}

bool CMainDlg::OnLButtonDown(UINT nFlags, POINT point)
{
	UNREFERENCED_PARAMETER(nFlags);

	HDC hDC;
	RECT rect, tree, list, treelist, treelistclient, logrect, loglist, loglistclient;

	// create an union of the tree and list control rectangle
	::GetWindowRect(m_hListControl, &list);
	::GetWindowRect(m_hTreeControl, &tree);
	::GetWindowRect(m_hLogMsgControl, &logrect);
	UnionRect(&treelist, &tree, &list);
	treelistclient = treelist;
	MapWindowPoints(NULL, *this, (LPPOINT)&treelistclient, 2);

	UnionRect(&loglist, &logrect, &list);
	loglistclient = loglist;
	MapWindowPoints(NULL, *this, (LPPOINT)&loglistclient, 2);

	//convert the mouse coordinates relative to the top-left of
	//the window
	ClientToScreen(*this, &point);
	GetClientRect(*this, &rect);
	MapWindowPoints(*this, NULL, (LPPOINT)&rect, 2);
	point.x -= rect.left;
	point.y -= rect.top;

	//same for the window coordinates - make them relative to 0,0
	LONG tempy = rect.top;
	LONG tempx = rect.left;
	OffsetRect(&treelist, -tempx, -tempy);
	OffsetRect(&loglist, -tempx, -tempy);

	if ((point.y < loglist.top) || 
		(point.y > treelist.bottom))
		return false;

	m_nDragMode = DRAGMODE_HORIZONTAL;
	if ((point.x+rect.left) > list.left)
		m_nDragMode = DRAGMODE_VERTICAL;

	if (point.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.y > loglist.bottom-REPOBROWSER_CTRL_MIN_HEIGHT)
		point.y = loglist.bottom-REPOBROWSER_CTRL_MIN_HEIGHT;
	if (point.y < loglist.top+REPOBROWSER_CTRL_MIN_HEIGHT)
		point.y = loglist.top+REPOBROWSER_CTRL_MIN_HEIGHT;

	SetCapture(*this);

	hDC = GetDC(*this);
	if (m_nDragMode == DRAGMODE_HORIZONTAL)
		DrawXorBar(hDC, point.x+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);
	else
		DrawXorBar(hDC, loglistclient.left, point.y+2, loglistclient.right-loglistclient.left-2, 4);

	ReleaseDC(*this, hDC);

	m_oldx = point.x;
	m_oldy = point.y;

	return true;
}

bool CMainDlg::OnLButtonUp(UINT nFlags, POINT point)
{
	UNREFERENCED_PARAMETER(nFlags);

	if (m_nDragMode == DRAGMODE_NONE)
		return false;

	PositionChildWindows(point, m_nDragMode == DRAGMODE_HORIZONTAL, true);

	//convert the mouse coordinates relative to the top-left of
	//the window
	ClientToScreen(*this, &point);
	RECT rect;
	GetClientRect(*this, &rect);
	MapWindowPoints(*this, NULL, (LPPOINT)&rect, 2);

	m_oldx = point.x;
	m_oldy = point.y;

	ReleaseCapture();

	// initialize the window position infos
	GetClientRect(m_hTreeControl, &rect);
	m_xSliderPos = rect.right+4;
	GetClientRect(m_hListControl, &rect);
	m_ySliderPos = rect.bottom+m_topmarg+FILTERBOXHEIGHT;

	m_nDragMode = DRAGMODE_NONE;

	return true;
}

void CMainDlg::PositionChildWindows(POINT point, bool bHorz, bool bShowBar)
{
	HDC hDC;
	RECT rect, tree, list, treelist, treelistclient, logrect, loglist, loglistclient;

	// create an union of the tree and list control rectangle
	::GetWindowRect(m_hListControl, &list);
	::GetWindowRect(m_hTreeControl, &tree);
	::GetWindowRect(m_hLogMsgControl, &logrect);
	UnionRect(&treelist, &tree, &list);
	treelistclient = treelist;
	MapWindowPoints(NULL, *this, (LPPOINT)&treelistclient, 2);

	UnionRect(&loglist, &logrect, &list);
	loglistclient = loglist;
	MapWindowPoints(NULL, *this, (LPPOINT)&loglistclient, 2);

	//convert the mouse coordinates relative to the top-left of
	//the window
	ClientToScreen(*this, &point);
	GetClientRect(*this, &rect);
	MapWindowPoints(*this, NULL, (LPPOINT)&rect, 2);

	POINT point2 = point;
	if (point2.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point2.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point2.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point2.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

	POINT point3 = point;
	if (point3.y < loglist.top+REPOBROWSER_CTRL_MIN_HEIGHT)
		point3.y = loglist.top+REPOBROWSER_CTRL_MIN_HEIGHT;
	if (point3.y > loglist.bottom-REPOBROWSER_CTRL_MIN_HEIGHT) 
		point3.y = loglist.bottom-REPOBROWSER_CTRL_MIN_HEIGHT;

	point.x -= rect.left;
	point.y -= rect.top;

	//same for the window coordinates - make them relative to 0,0
	LONG tempy = treelist.top;
	LONG tempx = treelist.left;
	OffsetRect(&treelist, -tempx, -tempy);
	OffsetRect(&loglist, -tempx, -tempy);

	if (point.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.y > loglist.bottom-REPOBROWSER_CTRL_MIN_HEIGHT)
		point.y = loglist.bottom-REPOBROWSER_CTRL_MIN_HEIGHT;
	if (point.y < loglist.top+REPOBROWSER_CTRL_MIN_HEIGHT)
		point.y = loglist.top+REPOBROWSER_CTRL_MIN_HEIGHT;


	if (bShowBar)
	{
		hDC = GetDC(*this);
		if (bHorz)
			DrawXorBar(hDC, m_oldx+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);
		else
			DrawXorBar(hDC, loglistclient.left, m_oldy+2, loglistclient.right-loglistclient.left-2, 4);


		ReleaseDC(*this, hDC);
	}

	//position the child controls
	HDWP hdwp = BeginDeferWindowPos(5);
	if (hdwp)
	{
		if (bHorz)
		{
			GetWindowRect(m_hTreeControl, &treelist);
			treelist.right = point2.x - 2;
			MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
			hdwp = DeferWindowPos(hdwp, m_hTreeControl, NULL, 
				treelist.left, treelist.top, treelist.right-treelist.left, treelist.bottom-treelist.top,
				SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);

			GetWindowRect(m_hListControl, &loglist);
			loglist.left = point2.x + 2;
			MapWindowPoints(NULL, *this, (LPPOINT)&loglist, 2);
			hdwp = DeferWindowPos(hdwp, GetDlgItem(*this, IDC_FILTERLABEL), NULL,
				loglist.left, treelist.top+5, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER|SWP_NOSIZE);

			hdwp = DeferWindowPos(hdwp, m_hFilterControl, NULL,
				loglist.left+FILTERLABELWIDTH, treelist.top, loglist.right-FILTERLABELWIDTH, FILTERBOXHEIGHT, 
				SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);

			hdwp = DeferWindowPos(hdwp, m_hListControl, NULL, 
				loglist.left, treelist.top+FILTERBOXHEIGHT, loglist.right-loglist.left, loglist.bottom-treelist.top-FILTERBOXHEIGHT,
				SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);

			GetWindowRect(m_hLogMsgControl, &treelist);
			treelist.left = point2.x + 2;
			MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
			hdwp = DeferWindowPos(hdwp, m_hLogMsgControl, NULL, 
				treelist.left, treelist.top, treelist.right-treelist.left, treelist.bottom-treelist.top,
				SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
		}
		else
		{
			GetWindowRect(m_hListControl, &treelist);
			treelist.bottom = point3.y - 2;
			MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
			hdwp = DeferWindowPos(hdwp, m_hListControl, NULL,
				treelist.left, treelist.top, treelist.right-treelist.left, treelist.bottom-treelist.top,
				SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);

			GetWindowRect(m_hLogMsgControl, &treelist);
			treelist.top = point3.y + 2;
			MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
			hdwp = DeferWindowPos(hdwp, m_hLogMsgControl, NULL, 
				treelist.left, treelist.top, treelist.right-treelist.left, treelist.bottom-treelist.top,
				SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
		}
		EndDeferWindowPos(hdwp);
	}
}

void CMainDlg::DrawXorBar(HDC hDC, LONG x1, LONG y1, LONG width, LONG height)
{
	static WORD _dotPatternBmp[8] = 
	{ 
		0x0055, 0x00aa, 0x0055, 0x00aa, 
		0x0055, 0x00aa, 0x0055, 0x00aa
	};

	HBITMAP hbm;
	HBRUSH  hbr, hbrushOld;

	hbm = CreateBitmap(8, 8, 1, 1, _dotPatternBmp);
	hbr = CreatePatternBrush(hbm);

	SetBrushOrgEx(hDC, x1, y1, NULL);
	hbrushOld = (HBRUSH)SelectObject(hDC, hbr);

	PatBlt(hDC, x1, y1, width, height, PATINVERT);

	SelectObject(hDC, hbrushOld);

	DeleteObject(hbr);
	DeleteObject(hbm);
}

void CMainDlg::SaveWndPosition()
{
	RECT rc;
	::GetWindowRect(*this, &rc);

	if (!IsZoomed(*this))
	{
		CRegStdDWORD regXY(_T("Software\\CommitMonitor\\XY"));
		regXY = MAKELONG(rc.top, rc.left);
		CRegStdDWORD regWHWindow(_T("Software\\CommitMonitor\\WHWindow"));
		regWHWindow = MAKELONG(rc.bottom-rc.top, rc.right-rc.left);
		::GetClientRect(*this, &rc);
		CRegStdDWORD regWH(_T("Software\\CommitMonitor\\WH"));
		regWH = MAKELONG(rc.bottom-rc.top, rc.right-rc.left);
	}
	if (IsZoomed(*this))
	{
		::GetWindowRect(m_hTreeControl, &rc);
		::MapWindowPoints(NULL, *this, (LPPOINT)&rc, 2);
		CRegStdDWORD regHorzPos(_T("Software\\CommitMonitor\\HorzPosZoomed"));
		regHorzPos = rc.right;
		CRegStdDWORD regVertPos(_T("Software\\CommitMonitor\\VertPosZoomed"));
		::GetWindowRect(m_hListControl, &rc);
		::MapWindowPoints(NULL, *this, (LPPOINT)&rc, 2);
		regVertPos = rc.bottom;
	}
	else
	{
		::GetWindowRect(m_hTreeControl, &rc);
		::MapWindowPoints(NULL, *this, (LPPOINT)&rc, 2);
		CRegStdDWORD regHorzPos(_T("Software\\CommitMonitor\\HorzPos"));
		regHorzPos = rc.right;
		CRegStdDWORD regVertPos(_T("Software\\CommitMonitor\\VertPos"));
		::GetWindowRect(m_hListControl, &rc);
		::MapWindowPoints(NULL, *this, (LPPOINT)&rc, 2);
		regVertPos = rc.bottom;
	}
}

LRESULT CALLBACK CMainDlg::TreeProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	CMainDlg *pThis = (CMainDlg*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (uMessage == WM_SETFOCUS)
	{
		pThis->SetRemoveButtonState();
	}
	return CallWindowProc(pThis->m_oldTreeWndProc, hWnd, uMessage, wParam, lParam);
}

LRESULT CALLBACK CMainDlg::FilterProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	CMainDlg *pThis = (CMainDlg*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (pThis == NULL)
		return 0;
	if (uMessage == WM_LBUTTONDBLCLK)
	{
		::SetWindowText(pThis->m_hFilterControl, _T(""));
	}
	return CallWindowProc(pThis->m_oldFilterWndProc, hWnd, uMessage, wParam, lParam);
}
