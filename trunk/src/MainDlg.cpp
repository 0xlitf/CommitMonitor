#include "StdAfx.h"
#include "Resource.h"
#include "MainDlg.h"

#include "URLDlg.h"
#include "AppUtils.h"
#include <algorithm>


CMainDlg::CMainDlg(void) : m_bThreadRunning(false)
	, m_bDragMode(false)
	, m_oldx(-1)
	, m_oldy(-1)
	, m_boldFont(NULL)
{
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
}

LRESULT CMainDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			InitDialog(hwndDlg, IDI_COMMITMONITOR);
			::SendMessage(::GetDlgItem(*this, IDC_URLTREE), TVM_SETUNICODEFORMAT, 1, 0);
			wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
			if (PathFileExists(urlfile.c_str()))
				m_URLInfos.Load(urlfile.c_str());
			RefreshURLTree();
		}
		break;
	case WM_COMMAND:
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
	case WM_NOTIFY:
		{
			HWND hTreeCtrl = GetDlgItem(*this, IDC_URLTREE);
			HWND hListCtrl = GetDlgItem(*this, IDC_MONITOREDURLS);
			LPNMHDR lpnmhdr = (LPNMHDR)lParam;
			if ((lpnmhdr->code == TVN_SELCHANGED)&&(lpnmhdr->hwndFrom == hTreeCtrl))
			{
				OnSelectTreeItem((LPNMTREEVIEW)lParam);
				return TRUE;
			}
			if ((lpnmhdr->code == LVN_ITEMCHANGED)&&(lpnmhdr->hwndFrom == hListCtrl))
			{
				OnSelectListItem((LPNMLISTVIEW)lParam);
			}
			if ((lpnmhdr->code == LVN_KEYDOWN)&&(lpnmhdr->hwndFrom == hListCtrl))
			{
				OnKeyDownListItem((LPNMLVKEYDOWN)lParam);
			}
			if ((lpnmhdr->code == NM_CUSTOMDRAW)&&(lpnmhdr->hwndFrom == hListCtrl))
			{
				return OnCustomDrawListItem((LPNMLVCUSTOMDRAW)lParam);
			}
			return FALSE;
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
		break;
	case IDCANCEL:
		{
			wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
			m_URLInfos.Save(urlfile.c_str());
			EndDialog(*this, IDCANCEL);
		}
		break;
	case IDC_URLEDIT:
		{
			CURLDlg dlg;
			HWND hTreeControl = GetDlgItem(*this, IDC_URLTREE);
			HTREEITEM hItem = TreeView_GetSelection(hTreeControl);
			if (hItem)
			{
				TVITEMEX itemex = {0};
				itemex.hItem = hItem;
				itemex.mask = TVIF_PARAM;
				TreeView_GetItem(hTreeControl, &itemex);
				if (m_URLInfos.infos.find(*(wstring*)itemex.lParam) != m_URLInfos.infos.end())
				{
					dlg.SetInfo(&m_URLInfos.infos[*(wstring*)itemex.lParam]);
					dlg.DoModal(hResource, IDD_URLCONFIG, *this);
					CUrlInfo * inf = dlg.GetInfo();
					if ((inf)&&inf->url.size())
					{
						m_URLInfos.infos.erase(*(wstring*)itemex.lParam);
						m_URLInfos.infos[inf->url] = *inf;
					}
					SaveURLInfo();
					RefreshURLTree();
				}
			}
		}
		break;
	case IDC_ADDURL:
		{
			CURLDlg dlg;
			dlg.DoModal(hResource, IDD_URLCONFIG, *this);
			CUrlInfo * inf = dlg.GetInfo();
			if ((inf)&&inf->url.size())
			{
				m_URLInfos.infos[inf->url] = *inf;
			}
			SaveURLInfo();
			RefreshURLTree();
		}
		break;
	default:
		return 0;
	}
	return 1;
}

/******************************************************************************/
/* data persistence                                                           */
/******************************************************************************/
void CMainDlg::SaveURLInfo()
{
	wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
	m_URLInfos.Save(urlfile.c_str());
}

void CMainDlg::LoadURLInfo()
{
	wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
	if (PathFileExists(urlfile.c_str()))
		m_URLInfos.Load(urlfile.c_str());
	RefreshURLTree();
}

/******************************************************************************/
/* tree handling                                                              */
/******************************************************************************/
void CMainDlg::RefreshURLTree()
{
	// the m_URLInfos member must be up-to-date here

	HWND hTreeControl = GetDlgItem(*this, IDC_URLTREE);
	// first clear the tree control
	TreeView_DeleteAllItems(hTreeControl);
	// now add a tree item for every entry in m_URLInfos
	for (map<wstring, CUrlInfo>::const_iterator it = m_URLInfos.infos.begin(); it != m_URLInfos.infos.end(); ++it)
	{
		TVINSERTSTRUCT tv = {0};
		tv.hParent = TVI_ROOT;
		tv.hInsertAfter = TVI_SORT;
		tv.itemex.mask = TVIF_TEXT|TVIF_PARAM|TVIF_STATE;
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
		HTREEITEM hItem = TreeView_InsertItem(hTreeControl, &tv);
		delete [] str;
		//if ((hItem)&&(it->second.subentries.size()))
		//{
		//	for (map<wstring, wstring>::const_iterator it2 = it->second.subentries.begin(); it2 != it->second.subentries.end(); ++it2)
		//	{
		//		TVINSERTSTRUCT tv2 = {0};
		//		tv2.hParent = hItem;
		//		tv2.hInsertAfter = TVI_SORT;
		//		tv2.itemex.mask = TVIF_TEXT;
		//		WCHAR * str = new WCHAR[it->first.size()+1];
		//		_tcscpy_s(str, it->first.size()+1, it->first.c_str());
		//		tv.itemex.pszText = str;
		//		TreeView_InsertItem(hTreeControl, &tv2);
		//		delete [] str;
		//	}
		//}
	}

}

void CMainDlg::OnSelectTreeItem(LPNMTREEVIEW lpNMTreeView)
{
	HTREEITEM hSelectedItem = lpNMTreeView->itemNew.hItem;
	// get the url this entry refers to
	TVITEMEX itemex = {0};
	itemex.hItem = hSelectedItem;
	itemex.mask = TVIF_PARAM;
	TreeView_GetItem(lpNMTreeView->hdr.hwndFrom, &itemex);
	if (m_URLInfos.infos.find(*(wstring*)itemex.lParam) != m_URLInfos.infos.end())
	{
		CUrlInfo * info = &m_URLInfos.infos[*(wstring*)itemex.lParam];
		HWND hLogInfo = GetDlgItem(*this, IDC_LOGINFO);
		HWND hLogList = GetDlgItem(*this, IDC_MONITOREDURLS);


		DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
		ListView_DeleteAllItems(hLogList);
		ListView_SetExtendedListViewStyle(hLogList, exStyle);
		LVCOLUMN lvc = {0};
		lvc.mask = LVCF_TEXT;
		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = -1;
		lvc.pszText = _T("revision");
		ListView_InsertColumn(hLogList, 0, &lvc);
		lvc.pszText = _T("date");
		ListView_InsertColumn(hLogList, 1, &lvc);
		lvc.pszText = _T("author");
		ListView_InsertColumn(hLogList, 2, &lvc);

		LVITEM item = {0};
		int i = 0;
		TCHAR buf[1024];
		for (map<svn_revnum_t,SVNLogEntry>::const_iterator it = info->logentries.begin(); it != info->logentries.end(); ++it)
		{
			item.mask = LVIF_TEXT|LVIF_PARAM;
			item.iItem = i;
			item.lParam = (LPARAM)&it->second;
			_stprintf_s(buf, 1024, _T("%ld"), it->first);
			item.pszText = buf;
			ListView_InsertItem(hLogList, &item);
			_tcscpy_s(buf, 1024, CAppUtils::ConvertDate(it->second.date).c_str());
			ListView_SetItemText(hLogList, i, 1, buf);
			_tcscpy_s(buf, 1024, it->second.author.c_str());
			ListView_SetItemText(hLogList, i, 2, buf);
		}
		ListView_SetColumnWidth(hLogList, 0, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(hLogList, 1, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(hLogList, 2, LVSCW_AUTOSIZE_USEHEADER);

		ListView_SetItemState(hLogList, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	}

}

/******************************************************************************/
/* list view handling                                                         */
/******************************************************************************/
void CMainDlg::OnSelectListItem(LPNMLISTVIEW lpNMListView)
{
	if (lpNMListView->uNewState & LVIS_SELECTED)
	{
		HWND hListView = GetDlgItem(*this, IDC_MONITOREDURLS);
		LVITEM item = {0};
		item.mask = LVIF_PARAM;
		item.iItem = lpNMListView->iItem;
		ListView_GetItem(hListView, &item);
		SVNLogEntry * pLogEntry = (SVNLogEntry*)item.lParam;
		if (pLogEntry)
		{
			// set the entry as read
			pLogEntry->read = true;
			TCHAR buf[1024];
			HWND hMsgView = GetDlgItem(*this, IDC_LOGINFO);
			wstring msg = pLogEntry->message.c_str();
			msg += _T("\n\n-------------------------------\n");
			// now add all changed paths, one path per line
			for (map<stdstring, SVNLogChangedPaths>::const_iterator it = pLogEntry->m_changedPaths.begin(); it != pLogEntry->m_changedPaths.end(); ++it)
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
			SetWindowText(hMsgView, msg.c_str());
		}
	}
}

LRESULT CMainDlg::OnCustomDrawListItem(LPNMLVCUSTOMDRAW lpNMCustomDraw)
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
	if (pnkd->wVKey == VK_DELETE)
	{
		// remove the selected entry
		HWND hListView = GetDlgItem(*this, IDC_MONITOREDURLS);
		int selCount = ListView_GetSelectedCount(hListView);
		if (selCount <= 0)
			return;	//nothing selected, nothing to remove

		HWND hTreeControl = GetDlgItem(*this, IDC_URLTREE);
		HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeControl);
		// get the url this entry refers to
		TVITEMEX itemex = {0};
		itemex.hItem = hSelectedItem;
		itemex.mask = TVIF_PARAM;
		TreeView_GetItem(hTreeControl, &itemex);
		if (m_URLInfos.infos.find(*(wstring*)itemex.lParam) != m_URLInfos.infos.end())
		{
			LVITEM item = {0};
			int i = 0;
			while (i<ListView_GetItemCount(hListView))
			{
				item.mask = LVIF_PARAM|LVIF_STATE;
				item.stateMask = LVIS_SELECTED;
				item.iItem = i;
				ListView_GetItem(hListView, &item);
				if (item.state & LVIS_SELECTED)
				{
					SVNLogEntry * pLogEntry = (SVNLogEntry*)item.lParam;
					m_URLInfos.infos[(*(wstring*)itemex.lParam)].logentries.erase(pLogEntry->revision);
					ListView_DeleteItem(hListView, i);
				}
				else
					++i;
			}
		}
	}
}

/******************************************************************************/
/* tree and list view resizing                                                */
/******************************************************************************/

bool CMainDlg::OnSetCursor(HWND hWnd, UINT nHitTest, UINT message) 
{
	UNREFERENCED_PARAMETER(message);
	UNREFERENCED_PARAMETER(nHitTest);
	if (m_bThreadRunning)
	{
		HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
		SetCursor(hCur);
		return TRUE;
	}
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

			::GetWindowRect(::GetDlgItem(*this, IDC_URLTREE), &rect);
			if ((pt.x > rect.right)&&
				(pt.y >= rect.top)&&
				(pt.y <= rect.bottom))
			{
				// but left of the list control?
				::GetWindowRect(::GetDlgItem(*this, IDC_MONITOREDURLS), &rect);
				if (pt.x < rect.left)
				{
					HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZEWE));
					SetCursor(hCur);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

bool CMainDlg::OnMouseMove(UINT nFlags, POINT point)
{
	HDC hDC;
	RECT rect, tree, list, treelist, treelistclient;

	if (m_bDragMode == false)
		return false;

	// create an union of the tree and list control rectangle
	::GetWindowRect(::GetDlgItem(*this, IDC_MONITOREDURLS), &list);
	::GetWindowRect(::GetDlgItem(*this, IDC_URLTREE), &tree);
	UnionRect(&treelist, &tree, &list);
	treelistclient = treelist;
	MapWindowPoints(NULL, *this, (LPPOINT)&treelistclient, 2);

	//convert the mouse coordinates relative to the top-left of
	//the window
	ClientToScreen(*this, &point);
	GetClientRect(*this, &rect);
	MapWindowPoints(*this, NULL, (LPPOINT)&rect, 2);
	point.x -= rect.left;
	point.y -= treelist.top;

	//same for the window coordinates - make them relative to 0,0
	OffsetRect(&treelist, -treelist.left, -treelist.top);

	if (point.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

	if ((nFlags & MK_LBUTTON) && (point.x != m_oldx))
	{
		hDC = GetDC(*this);

		if (hDC)
		{
			DrawXorBar(hDC, m_oldx+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);
			DrawXorBar(hDC, point.x+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);

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
	RECT rect, tree, list, treelist, treelistclient;

	// create an union of the tree and list control rectangle
	::GetWindowRect(::GetDlgItem(*this, IDC_MONITOREDURLS), &list);
	::GetWindowRect(::GetDlgItem(*this, IDC_URLTREE), &tree);
	UnionRect(&treelist, &tree, &list);
	treelistclient = treelist;
	MapWindowPoints(NULL, *this, (LPPOINT)&treelistclient, 2);

	//convert the mouse coordinates relative to the top-left of
	//the window
	ClientToScreen(*this, &point);
	GetClientRect(*this, &rect);
	MapWindowPoints(*this, NULL, (LPPOINT)&rect, 2);
	point.x -= rect.left;
	point.y -= treelist.top;

	//same for the window coordinates - make them relative to 0,0
	OffsetRect(&treelist, -treelist.left, -treelist.top);

	if (point.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

	if ((point.y < treelist.top) || 
		(point.y > treelist.bottom))
		return false;

	m_bDragMode = true;

	SetCapture(*this);

	hDC = GetDC(*this);
	DrawXorBar(hDC, point.x+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);
	ReleaseDC(*this, hDC);

	m_oldx = point.x;
	m_oldy = point.y;

	return true;
}

bool CMainDlg::OnLButtonUp(UINT nFlags, POINT point)
{
	UNREFERENCED_PARAMETER(nFlags);

	HDC hDC;
	RECT rect, tree, list, treelist, treelistclient;

	if (m_bDragMode == FALSE)
		return false;

	// create an union of the tree and list control rectangle
	::GetWindowRect(::GetDlgItem(*this, IDC_MONITOREDURLS), &list);
	::GetWindowRect(::GetDlgItem(*this, IDC_URLTREE), &tree);
	UnionRect(&treelist, &tree, &list);
	treelistclient = treelist;
	MapWindowPoints(NULL, *this, (LPPOINT)&treelistclient, 2);

	ClientToScreen(*this, &point);
	GetClientRect(*this, &rect);
	MapWindowPoints(*this, NULL, (LPPOINT)&rect, 2);

	POINT point2 = point;
	if (point2.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point2.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point2.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point2.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

	point.x -= rect.left;
	point.y -= treelist.top;

	OffsetRect(&treelist, -treelist.left, -treelist.top);

	if (point.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

	hDC = GetDC(*this);
	DrawXorBar(hDC, m_oldx+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);			
	ReleaseDC(*this, hDC);

	m_oldx = point.x;
	m_oldy = point.y;

	m_bDragMode = false;
	ReleaseCapture();

	//position the child controls
	HDWP hdwp = BeginDeferWindowPos(3);
	if (hdwp)
	{
		GetWindowRect(GetDlgItem(*this, IDC_URLTREE), &treelist);
		treelist.right = point2.x - 2;
		MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
		DeferWindowPos(hdwp, GetDlgItem(*this, IDC_URLTREE), NULL, 
			treelist.left, treelist.top, treelist.right-treelist.left, treelist.bottom-treelist.top,
			SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);

		GetWindowRect(GetDlgItem(*this, IDC_MONITOREDURLS), &treelist);
		treelist.left = point2.x + 2;
		MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
		DeferWindowPos(hdwp, GetDlgItem(*this, IDC_MONITOREDURLS), NULL, 
			treelist.left, treelist.top, treelist.right-treelist.left, treelist.bottom-treelist.top,
			SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);

		GetWindowRect(GetDlgItem(*this, IDC_LOGINFO), &treelist);
		treelist.left = point2.x + 2;
		MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
		DeferWindowPos(hdwp, GetDlgItem(*this, IDC_LOGINFO), NULL, 
			treelist.left, treelist.top, treelist.right-treelist.left, treelist.bottom-treelist.top,
			SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
		EndDeferWindowPos(hdwp);
	}

	return true;
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
