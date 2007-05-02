#include "StdAfx.h"
#include "Resource.h"
#include "MainDlg.h"

#include "URLDlg.h"
#include "OptionsDlg.h"
#include "AppUtils.h"
#include "DirFileEnum.h"
#include "SysImageList.h"
#include <algorithm>
#include <assert.h>

CMainDlg::CMainDlg(HWND hParent) 
	: m_nDragMode(DRAGMODE_NONE)
	, m_oldx(-1)
	, m_oldy(-1)
	, m_boldFont(NULL)
	, m_pURLInfos(NULL)
	, m_bBlockListCtrlUI(false)
	, m_hTreeControl(NULL)
	, m_hListControl(NULL)
	, m_hLogMsgControl(NULL)
	, m_nIconFolder(0)
	, m_nOpenIconFolder(0)
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
	if (m_hToolbarImages)
		ImageList_Destroy(m_hToolbarImages);
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

#define MAINDLG_TOOLBARBUTTONCOUNT	9
	TBBUTTON tbb[MAINDLG_TOOLBARBUTTONCOUNT];
	// create an image list containing the icons for the toolbar
	m_hToolbarImages = ImageList_Create(24, 24, ILC_COLOR32 | ILC_MASK, MAINDLG_TOOLBARBUTTONCOUNT, 4);
	if (m_hToolbarImages == NULL)
		return false;
	int index = 0;
	HICON hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_ADD));
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
	tbb[index].idCommand = ID_MAIN_SHOWDIFF; 
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
			m_hTreeControl = ::GetDlgItem(*this, IDC_URLTREE);
			m_hListControl = ::GetDlgItem(*this, IDC_MONITOREDURLS);
			m_hLogMsgControl = ::GetDlgItem(*this, IDC_LOGINFO);
			::SendMessage(m_hTreeControl, TVM_SETUNICODEFORMAT, 1, 0);
			assert(m_pURLInfos);
			m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();
			m_nOpenIconFolder = SYS_IMAGE_LIST().GetDirOpenIconIndex();
			TreeView_SetImageList(m_hTreeControl, SYS_IMAGE_LIST(), LVSIL_SMALL);
			TreeView_SetImageList(m_hTreeControl, SYS_IMAGE_LIST(), TVSIL_NORMAL);
			RefreshURLTree();

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

			::SetTimer(*this, TIMER_REFRESH, 1000, NULL);
		}
		break;
	case WM_SIZE:
		{
			DoResize(LOWORD(lParam), HIWORD(lParam));
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
			if (wParam == TIMER_REFRESH)
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
						tv.itemex.stateMask = TVIS_SELECTED|TVIS_BOLD;
						tv.itemex.pszText = str;
						tv.itemex.cchTextMax = it->second.name.size()+9;
						TreeView_GetItem(m_hTreeControl, &tv.itemex);
						wstring sTitle = wstring(str);
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
						if (sTitle.compare(str) != 0)
						{
							TreeView_SetItem(m_hTreeControl, &tv.itemex);
							if (tv.itemex.state & TVIS_SELECTED)
							{
								m_bBlockListCtrlUI = true;
								TreeItemSelected(m_hTreeControl, tv.itemex.hItem);
								m_bBlockListCtrlUI = false;
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
						tv.itemex.iImage = m_nIconFolder;
						tv.itemex.iSelectedImage = m_nOpenIconFolder;
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
					HMENU hMenu = ::LoadMenu(hResource, MAKEINTRESOURCE(IDR_TREEPOPUP));
					hMenu = ::GetSubMenu(hMenu, 0);

					int cmd = ::TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY , pt.x, pt.y, NULL, *this, NULL);
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
					HMENU hMenu = ::LoadMenu(hResource, MAKEINTRESOURCE(IDR_LISTPOPUP));
					hMenu = ::GetSubMenu(hMenu, 0);

					// set the default entry
					MENUITEMINFO iinfo = {0};
					iinfo.cbSize = sizeof(MENUITEMINFO);
					iinfo.fMask = MIIM_STATE;
					GetMenuItemInfo(hMenu, 0, MF_BYPOSITION, &iinfo);
					iinfo.fState |= MFS_DEFAULT;
					SetMenuItemInfo(hMenu, 0, MF_BYPOSITION, &iinfo);

					int cmd = ::TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY , pt.x, pt.y, NULL, *this, NULL);
					switch (cmd)
					{
					case ID_MAIN_SHOWDIFF:
					case ID_MAIN_REMOVE:
						{
							::SendMessage(*this, WM_COMMAND, MAKELONG(cmd, 0), 0);
						}
					}
				}
			}
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
			EndDialog(*this, IDCANCEL);
			SYS_IMAGE_LIST().Cleanup();
		}
		break;
	case IDCANCEL:
		{
			int res = ::MessageBox(*this, _T("Do you really want to quit the CommitMonitor?\nIf you quit, monitoring will stop.\nIf you just want to close the dialog, use the \"OK\" button.\n\nAre you sure you want to quit the CommitMonitor?"),
				_T("CommitMonitor"), MB_ICONQUESTION|MB_YESNO);
			if (res != IDYES)
				break;
			EndDialog(*this, IDCANCEL);
			SYS_IMAGE_LIST().Cleanup();
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
						_stprintf_s(question, 4096, _T("Do you really want to delete the project\n%s ?"), mask.c_str());
						if (::MessageBox(*this, question, _T("CommitMonitor"), MB_ICONQUESTION|MB_YESNO)==IDYES)
						{
							// delete all fetched and stored diff files
							mask += _T("*.*");
							CSimpleFileFind sff(CAppUtils::GetAppDataDir(), mask.c_str());
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
							TreeView_DeleteItem(m_hTreeControl, hItem);
							if (hPrev == NULL)
								hPrev = TreeView_GetRoot(m_hTreeControl);
							if ((hPrev)&&(hPrev != TVI_ROOT))
								TreeView_SelectItem(m_hTreeControl, hPrev);
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
					m_pURLInfos->ReleaseWriteData();
					RefreshURLTree();
				}
				else
					m_pURLInfos->ReleaseWriteData();
			}
		}
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
			}
			RefreshURLTree();
		}
		break;
	case ID_MAIN_SHOWDIFF:
		{
			ShowDiff();
		}
		break;
	case ID_MISC_OPTIONS:
		{
			COptionsDlg dlg(*this);
			dlg.SetHiddenWnd(m_hParent);
			dlg.DoModal(hResource, IDD_OPTIONS, NULL);
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

bool CMainDlg::ShowDiff()
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
		for (int i=0; i<ListView_GetItemCount(m_hListControl); ++i)
		{
			item.mask = LVIF_PARAM|LVIF_STATE;
			item.stateMask = LVIS_SELECTED;
			item.iItem = i;
			ListView_GetItem(m_hListControl, &item);
			if (item.state & LVIS_SELECTED)
			{
				SVNLogEntry * pLogEntry = (SVNLogEntry*)item.lParam;
				// find the diff name
				_stprintf_s(buf, 4096, _T("%s_%ld"), pRead->find(*(wstring*)itemex.lParam)->second.name.c_str(), pLogEntry->revision);
				wstring diffFileName = CAppUtils::GetAppDataDir();
				diffFileName += _T("\\");
				diffFileName += wstring(buf);
				// construct a title for the diff viewer
				_stprintf_s(buf, 4096, _T("%s, revision %ld"), pRead->find(*(wstring*)itemex.lParam)->second.name.c_str(), pLogEntry->revision);
				wstring title = wstring(buf);
				// start the diff viewer
				TCHAR apppath[4096];
				GetModuleFileName(NULL, apppath, 4096);
				wstring cmd;
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
				CAppUtils::LaunchApplication(cmd);
			}
		}
	}
	m_pURLInfos->ReleaseReadOnlyData();
	return TRUE;
}

/******************************************************************************/
/* tree handling                                                              */
/******************************************************************************/
void CMainDlg::RefreshURLTree()
{
	// the m_URLInfos member must be up-to-date here

	m_bBlockListCtrlUI = true;
	// first clear the controls (the data)
	ListView_DeleteAllItems(m_hListControl);
	TreeView_SelectItem(m_hTreeControl, NULL);
	TreeView_DeleteAllItems(m_hTreeControl);
	SetWindowText(m_hLogMsgControl, _T(""));
	SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_MAIN_SHOWDIFF, MAKELONG(false, 0));
	SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_MAIN_EDIT, MAKELONG(false, 0));
	SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_MAIN_REMOVE, MAKELONG(false, 0));

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
		tv.itemex.iSelectedImage = m_nOpenIconFolder;
		tv.itemex.iImage = m_nIconFolder;
		TreeView_InsertItem(m_hTreeControl, &tv);
		TreeView_Expand(m_hTreeControl, tv.hParent, TVE_EXPAND);
		delete [] str;
	}
	m_pURLInfos->ReleaseReadOnlyData();
	m_bBlockListCtrlUI = false;
	::InvalidateRect(m_hListControl, NULL, true);
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

void CMainDlg::OnSelectTreeItem(LPNMTREEVIEW lpNMTreeView)
{
	HTREEITEM hSelectedItem = lpNMTreeView->itemNew.hItem;
	SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_MAIN_EDIT, 
		MAKELONG(!!(lpNMTreeView->itemNew.state & TVIS_SELECTED), 0));
	SetRemoveButtonState();
	if (lpNMTreeView->itemNew.state & TVIS_SELECTED)
		TreeItemSelected(lpNMTreeView->hdr.hwndFrom, hSelectedItem);
	else
	{
		ListView_DeleteAllItems(m_hListControl);
		SetWindowText(m_hLogMsgControl, _T(""));
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
			ListView_InsertItem(m_hListControl, &item);
			_tcscpy_s(buf, 1024, CAppUtils::ConvertDate(it->second.date).c_str());
			ListView_SetItemText(m_hListControl, i, 1, buf);
			_tcscpy_s(buf, 1024, it->second.author.c_str());
			ListView_SetItemText(m_hListControl, i, 2, buf);
		}
		m_bBlockListCtrlUI = false;
		ListView_SetColumnWidth(m_hListControl, 0, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(m_hListControl, 1, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(m_hListControl, 2, LVSCW_AUTOSIZE_USEHEADER);

		::InvalidateRect(m_hListControl, NULL, false);
	}
	m_pURLInfos->ReleaseReadOnlyData();
}

/******************************************************************************/
/* list view handling                                                         */
/******************************************************************************/
void CMainDlg::OnSelectListItem(LPNMLISTVIEW lpNMListView)
{
	if (lpNMListView->uNewState & LVIS_SELECTED)
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
            if (!pLogEntry->read)
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
                    }
                    else
                    {
                        _stprintf_s(str, uinfo->name.size()+10, _T("%s"), uinfo->name.c_str());
                        itemex.state = 0;
                        itemex.stateMask = TVIS_BOLD;
                    }
                    itemex.pszText = str;
                    itemex.mask = TVIF_TEXT|TVIF_STATE;
                    TreeView_SetItem(m_hTreeControl, &itemex);
                }
				// the icon in the system tray needs to be changed back
				// to 'normal'
				::SendMessage(m_hParent, COMMITMONITOR_CHANGEDINFO, (WPARAM)false, (LPARAM)0);
            }
			TCHAR buf[1024];
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
			SetWindowText(m_hLogMsgControl, msg.c_str());

			// find the diff name
			_stprintf_s(buf, 1024, _T("%s_%ld"), pRead->find(*(wstring*)itemex.lParam)->second.name.c_str(), pLogEntry->revision);
			wstring diffFileName = CAppUtils::GetAppDataDir();
			diffFileName += _T("\\");
			diffFileName += wstring(buf);
			SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_MAIN_SHOWDIFF, MAKELONG(!!PathFileExists(diffFileName.c_str()), 0));
		}
		m_pURLInfos->ReleaseReadOnlyData();
	}
}

void CMainDlg::OnDblClickListItem(LPNMITEMACTIVATE /*lpnmitem*/)
{
	ShowDiff();
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
	if (pnkd->wVKey == VK_DELETE)
	{
		// remove the selected entry
		RemoveSelectedListItems();
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
				_stprintf_s(buf, 4096, _T("%s_%ld"), pWrite->find(*(wstring*)itemex.lParam)->second.name.c_str(), pLogEntry->revision);
				wstring diffFileName = CAppUtils::GetAppDataDir();
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
	RECT tree, list, log, ok;
	HWND hOK = GetDlgItem(*this, IDOK);
	::GetClientRect(m_hTreeControl, &tree);
	::GetClientRect(m_hListControl, &list);
	::GetClientRect(m_hLogMsgControl, &log);
	::GetClientRect(hOK, &ok);
	HDWP hdwp = BeginDeferWindowPos(5);
	hdwp = DeferWindowPos(hdwp, m_hwndToolbar, *this, 0, 0, width, m_topmarg, SWP_NOZORDER|SWP_NOACTIVATE);
	hdwp = DeferWindowPos(hdwp, m_hTreeControl, *this, 0, m_topmarg, m_xSliderPos, height-m_topmarg-m_bottommarg+5, SWP_NOZORDER|SWP_NOACTIVATE);
	hdwp = DeferWindowPos(hdwp, m_hListControl, *this, m_xSliderPos+4, m_topmarg, width-m_xSliderPos, m_ySliderPos-m_topmarg+4, SWP_NOZORDER|SWP_NOACTIVATE);
	hdwp = DeferWindowPos(hdwp, m_hLogMsgControl, *this, m_xSliderPos+4, m_ySliderPos+8, width-m_xSliderPos-4, height-m_bottommarg-m_ySliderPos-4, SWP_NOZORDER|SWP_NOACTIVATE);
	hdwp = DeferWindowPos(hdwp, hOK, *this, width-ok.right+ok.left, height-ok.bottom+ok.top, ok.right-ok.left, ok.bottom-ok.top, SWP_NOZORDER|SWP_NOACTIVATE);
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
			GetWindowRect(m_hTreeControl, &treelist);
			treelist.right = point2.x - 2;
			MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
			DeferWindowPos(hdwp, m_hTreeControl, NULL, 
				treelist.left, treelist.top, treelist.right-treelist.left, treelist.bottom-treelist.top,
				SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);

			GetWindowRect(m_hListControl, &treelist);
			treelist.left = point2.x + 2;
			MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
			DeferWindowPos(hdwp, m_hListControl, NULL, 
				treelist.left, treelist.top, treelist.right-treelist.left, treelist.bottom-treelist.top,
				SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);

			GetWindowRect(m_hLogMsgControl, &treelist);
			treelist.left = point2.x + 2;
			MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
			DeferWindowPos(hdwp, m_hLogMsgControl, NULL, 
				treelist.left, treelist.top, treelist.right-treelist.left, treelist.bottom-treelist.top,
				SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
		}
		else
		{
			GetWindowRect(m_hListControl, &treelist);
			treelist.bottom = point3.y - 2;
			MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
			DeferWindowPos(hdwp, m_hListControl, NULL,
				treelist.left, treelist.top, treelist.right-treelist.left, treelist.bottom-treelist.top,
				SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);

			GetWindowRect(m_hLogMsgControl, &treelist);
			treelist.top = point3.y + 2;
			MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
			DeferWindowPos(hdwp, m_hLogMsgControl, NULL, 
				treelist.left, treelist.top, treelist.right-treelist.left, treelist.bottom-treelist.top,
				SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
		}
		EndDeferWindowPos(hdwp);
	}

	// initialize the window position infos
	GetClientRect(m_hTreeControl, &rect);
	m_xSliderPos = rect.right+4;
	GetClientRect(m_hListControl, &rect);
	m_ySliderPos = rect.bottom+m_topmarg;


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

LRESULT CALLBACK CMainDlg::TreeProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	CMainDlg *pThis = (CMainDlg*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (uMessage == WM_SETFOCUS)
	{
		pThis->SetRemoveButtonState();
	}
	return CallWindowProc(pThis->m_oldTreeWndProc, hWnd, uMessage, wParam, lParam);
}
