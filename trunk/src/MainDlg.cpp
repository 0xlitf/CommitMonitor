#include "StdAfx.h"
#include "Resource.h"
#include "MainDlg.h"

#include "URLDlg.h"
#include "AppUtils.h"
#include "DirFileEnum.h"
#include <algorithm>


CMainDlg::CMainDlg(HWND hParent) 
	: m_nDragMode(DRAGMODE_NONE)
	, m_oldx(-1)
	, m_oldy(-1)
	, m_boldFont(NULL)
	, m_pURLInfos(NULL)
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
	COMMITMONITOR_CHANGEDINFO = RegisterWindowMessage(_T("CommitMonitor_ChangedInfo"));
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
			assert(m_pURLInfos);
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
			EndDialog(*this, IDCANCEL);
		}
		break;
	case IDC_URLDELETE:
		{
			HWND hTreeControl = GetDlgItem(*this, IDC_URLTREE);
			HTREEITEM hItem = TreeView_GetSelection(hTreeControl);
			if (hItem)
			{
				TVITEMEX itemex = {0};
				itemex.hItem = hItem;
				itemex.mask = TVIF_PARAM;
				TreeView_GetItem(hTreeControl, &itemex);
				map<wstring,CUrlInfo> * pWrite = m_pURLInfos->GetWriteData();
				if (pWrite->find(*(wstring*)itemex.lParam) != pWrite->end())
				{
					// delete all fetched and stored diff files
					wstring mask = (*pWrite)[(*(wstring*)itemex.lParam)].name;
					mask += _T("*.*");
					CSimpleFileFind sff(CAppUtils::GetAppDataDir(), mask.c_str());
					while (sff.FindNextFileNoDots())
					{
						DeleteFile(sff.GetFilePath().c_str());
					}

					pWrite->erase(*(wstring*)itemex.lParam);
					::SendMessage(m_hParent, COMMITMONITOR_CHANGEDINFO, (WPARAM)false, 0);
					TreeView_DeleteItem(hTreeControl, hItem);
				}
				m_pURLInfos->ReleaseData();
				RefreshURLTree();
			}
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
				const map<wstring,CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
				if (pRead->find(*(wstring*)itemex.lParam) != pRead->end())
				{
					dlg.SetInfo(&pRead->find(*(wstring*)itemex.lParam)->second);
					m_pURLInfos->ReleaseData();
					dlg.DoModal(hResource, IDD_URLCONFIG, *this);
					CUrlInfo * inf = dlg.GetInfo();
					map<wstring,CUrlInfo> * pWrite = m_pURLInfos->GetWriteData();
					if ((inf)&&inf->url.size())
					{
						pWrite->erase(*(wstring*)itemex.lParam);
						(*pWrite)[inf->url] = *inf;
					}
					m_pURLInfos->ReleaseData();
					RefreshURLTree();
				}
				else
					m_pURLInfos->ReleaseData();
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
				map<wstring,CUrlInfo> * pWrite = m_pURLInfos->GetWriteData();
				if ((inf)&&inf->url.size())
				{
					(*pWrite)[inf->url] = *inf;
				}
				m_pURLInfos->ReleaseData();
			}
			RefreshURLTree();
		}
		break;
	case IDC_SHOWDIFF:
		{
			TCHAR buf[4096];
			// find the revision we have to show the diff for
			HWND hListView = GetDlgItem(*this, IDC_MONITOREDURLS);
			int selCount = ListView_GetSelectedCount(hListView);
			if (selCount <= 0)
				return FALSE;	//nothing selected, nothing to show

			HWND hTreeControl = GetDlgItem(*this, IDC_URLTREE);
			HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeControl);
			// get the url this entry refers to
			TVITEMEX itemex = {0};
			itemex.hItem = hSelectedItem;
			itemex.mask = TVIF_PARAM;
			TreeView_GetItem(hTreeControl, &itemex);
			const map<wstring,CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
			if (pRead->find(*(wstring*)itemex.lParam) != pRead->end())
			{
				LVITEM item = {0};
				for (int i=0; i<ListView_GetItemCount(hListView); ++i)
				{
					item.mask = LVIF_PARAM|LVIF_STATE;
					item.stateMask = LVIS_SELECTED;
					item.iItem = i;
					ListView_GetItem(hListView, &item);
					if (item.state & LVIS_SELECTED)
					{
						SVNLogEntry * pLogEntry = (SVNLogEntry*)item.lParam;
						// find the diff name
						_stprintf_s(buf, 4096, _T("%s_%ld"), pRead->find(*(wstring*)itemex.lParam)->second.name.c_str(), pLogEntry->revision);
						//_stprintf_s(buf, 4096, _T("%s_%ld"), m_URLInfos.infos[(*(wstring*)itemex.lParam)].name.c_str(), pLogEntry->revision);
						wstring diffFileName = CAppUtils::GetAppDataDir();
						diffFileName += _T("\\");
						diffFileName += wstring(buf);
						// start the diff viewer
						wstring cmd = _T("notepad.exe \"");
						cmd += diffFileName;
						cmd += _T("\"");
						CAppUtils::LaunchApplication(cmd);
					}
				}
			}
			m_pURLInfos->ReleaseData();
		}
		break;
	default:
		return 0;
	}
	return 1;
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
	const map<wstring, CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
	for (map<wstring, CUrlInfo>::const_iterator it = pRead->begin(); it != pRead->end(); ++it)
	{
		TVINSERTSTRUCT tv = {0};
		tv.hParent = FindParentTreeNode(it->first);
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
		TreeView_InsertItem(hTreeControl, &tv);
		TreeView_Expand(hTreeControl, tv.hParent, TVE_EXPAND);
		delete [] str;
	}
	m_pURLInfos->ReleaseData();
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
			m_pURLInfos->ReleaseData();
			// we found a parent URL, but now we have to find it in the
			// tree view
			return FindTreeNode(parenturl);
		}
		m_pURLInfos->ReleaseData();
		pos = parenturl.find_last_of('/');
		parenturl = parenturl.substr(0, pos);
		if (pos == string::npos)
			parenturl.clear();
	} while (!parenturl.empty());
	return TVI_ROOT;
}

HTREEITEM CMainDlg::FindTreeNode(const wstring& url)
{
	HWND hTreeControl = GetDlgItem(*this, IDC_URLTREE);
	HTREEITEM hItem = TreeView_GetRoot(hTreeControl);
	TVITEM item;
	item.mask = TVIF_PARAM;
	while (hItem)
	{
		item.hItem = hItem;
		TreeView_GetItem(hTreeControl, &item);
		if (url.compare(*(wstring*)item.lParam) == 0)
			return hItem;
		hItem = TreeView_GetNextSibling(hTreeControl, hItem);
	};
	return TVI_ROOT;
}

void CMainDlg::OnSelectTreeItem(LPNMTREEVIEW lpNMTreeView)
{
	HTREEITEM hSelectedItem = lpNMTreeView->itemNew.hItem;
	// get the url this entry refers to
	TVITEMEX itemex = {0};
	itemex.hItem = hSelectedItem;
	itemex.mask = TVIF_PARAM;
	TreeView_GetItem(lpNMTreeView->hdr.hwndFrom, &itemex);
	const map<wstring,CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
	if (pRead->find(*(wstring*)itemex.lParam) != pRead->end())
	{
		const CUrlInfo * info = &pRead->find(*(wstring*)itemex.lParam)->second;
		HWND hLogInfo = GetDlgItem(*this, IDC_LOGINFO);
		HWND hLogList = GetDlgItem(*this, IDC_MONITOREDURLS);


		DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
		ListView_DeleteAllItems(hLogList);

		int c = Header_GetItemCount(ListView_GetHeader(hLogList))-1;
		while (c>=0)
			ListView_DeleteColumn(hLogList, c--);

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
	m_pURLInfos->ReleaseData();
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
            if (!pLogEntry->read)
            {
                pLogEntry->read = true;
                // refresh the name of the tree item to indicate the new
                // number of unread log messages
                // e.g. instead of 'TortoiseSVN (3)', show now 'TortoiseSVN (2)'
                HWND hTreeControl = GetDlgItem(*this, IDC_URLTREE);
                HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeControl);
                // get the url this entry refers to
                TVITEMEX itemex = {0};
                itemex.hItem = hSelectedItem;
                itemex.mask = TVIF_PARAM;
                TreeView_GetItem(hTreeControl, &itemex);
				const map<wstring,CUrlInfo> * pRead = m_pURLInfos->GetReadOnlyData();
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
                    }
                    else
                    {
                        _stprintf_s(str, uinfo->name.size()+10, _T("%s"), uinfo->name.c_str());
                        itemex.state = 0;
                        itemex.stateMask = TVIS_BOLD;
                    }
                    itemex.pszText = str;
                    itemex.mask = TVIF_TEXT|TVIF_STATE;
                    TreeView_SetItem(hTreeControl, &itemex);
                }
				m_pURLInfos->ReleaseData();
            }
			// the icon in the system tray needs to be changed back
			// to 'normal'
			::SendMessage(m_hParent, COMMITMONITOR_CHANGEDINFO, (WPARAM)false, 0);
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
		map<wstring,CUrlInfo> * pWrite = m_pURLInfos->GetWriteData();
		if (pWrite->find(*(wstring*)itemex.lParam) != pWrite->end())
		{
			LVITEM item = {0};
			int i = 0;
			TCHAR buf[4096];
			while (i<ListView_GetItemCount(hListView))
			{
				item.mask = LVIF_PARAM|LVIF_STATE;
				item.stateMask = LVIS_SELECTED;
				item.iItem = i;
				item.lParam = 0;
				ListView_GetItem(hListView, &item);
				if (item.state & LVIS_SELECTED)
				{
					SVNLogEntry * pLogEntry = (SVNLogEntry*)item.lParam;
					// find the diff name
					_stprintf_s(buf, 4096, _T("%s_%ld"), pWrite->find(*(wstring*)itemex.lParam)->second.name.c_str(), pLogEntry->revision);
					wstring diffFileName = CAppUtils::GetAppDataDir();
					diffFileName += _T("\\");
					diffFileName += wstring(buf);
					DeleteFile(diffFileName.c_str());

					pWrite->find((*(wstring*)itemex.lParam))->second.logentries.erase(pLogEntry->revision);
					ListView_DeleteItem(hListView, i);
				}
				else
					++i;
			}
		}
		m_pURLInfos->ReleaseData();
	}
}

/******************************************************************************/
/* tree and list view resizing                                                */
/******************************************************************************/

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

				// maybe we are below the log message list control?
				if (pt.y > rect.bottom)
				{
					::GetWindowRect(::GetDlgItem(*this, IDC_LOGINFO), &rect);
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
	::GetWindowRect(::GetDlgItem(*this, IDC_MONITOREDURLS), &list);
	::GetWindowRect(::GetDlgItem(*this, IDC_URLTREE), &tree);
	::GetWindowRect(::GetDlgItem(*this, IDC_LOGINFO), &logrect);
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
	::GetWindowRect(::GetDlgItem(*this, IDC_MONITOREDURLS), &list);
	::GetWindowRect(::GetDlgItem(*this, IDC_URLTREE), &tree);
	::GetWindowRect(::GetDlgItem(*this, IDC_LOGINFO), &logrect);
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

	if ((point.y < treelist.top) || 
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

	HDC hDC;
	RECT rect, tree, list, treelist, treelistclient, logrect, loglist, loglistclient;

	if (m_nDragMode == DRAGMODE_NONE)
		return false;

	// create an union of the tree and list control rectangle
	::GetWindowRect(::GetDlgItem(*this, IDC_MONITOREDURLS), &list);
	::GetWindowRect(::GetDlgItem(*this, IDC_URLTREE), &tree);
	::GetWindowRect(::GetDlgItem(*this, IDC_LOGINFO), &logrect);
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


	hDC = GetDC(*this);
	if (m_nDragMode == DRAGMODE_HORIZONTAL)
		DrawXorBar(hDC, m_oldx+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);
	else
		DrawXorBar(hDC, loglistclient.left, m_oldy+2, loglistclient.right-loglistclient.left-2, 4);


	ReleaseDC(*this, hDC);

	m_oldx = point.x;
	m_oldy = point.y;

	ReleaseCapture();

	//position the child controls
	HDWP hdwp = BeginDeferWindowPos(3);
	if (hdwp)
	{
		if (m_nDragMode == DRAGMODE_HORIZONTAL)
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
		}
		else
		{
			GetWindowRect(GetDlgItem(*this, IDC_MONITOREDURLS), &treelist);
			treelist.bottom = point3.y - 2;
			MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
			DeferWindowPos(hdwp, GetDlgItem(*this, IDC_MONITOREDURLS), NULL,
				treelist.left, treelist.top, treelist.right-treelist.left, treelist.bottom-treelist.top,
				SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);

			GetWindowRect(GetDlgItem(*this, IDC_LOGINFO), &treelist);
			treelist.top = point3.y + 2;
			MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
			DeferWindowPos(hdwp, GetDlgItem(*this, IDC_LOGINFO), NULL, 
				treelist.left, treelist.top, treelist.right-treelist.left, treelist.bottom-treelist.top,
				SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
		}
		EndDeferWindowPos(hdwp);
	}
	m_nDragMode = DRAGMODE_NONE;

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
