#include "StdAfx.h"
#include "Resource.h"
#include "MainDlg.h"

#include "URLDlg.h"
#include "AppUtils.h"
#include "DirFileEnum.h"
#include <algorithm>
#include <assert.h>

CMainDlg::CMainDlg(HWND hParent) 
	: m_nDragMode(DRAGMODE_NONE)
	, m_oldx(-1)
	, m_oldy(-1)
	, m_boldFont(NULL)
	, m_pURLInfos(NULL)
	, m_bBlockListCtrlUI(false)
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
			::SendMessage(::GetDlgItem(*this, IDC_URLTREE), TVM_SETUNICODEFORMAT, 1, 0);
			assert(m_pURLInfos);
			RefreshURLTree();

			// initialize the window position infos
			RECT rect;
			GetClientRect(m_hwndToolbar, &rect);
			m_topmarg = rect.bottom+2;
			GetClientRect(GetDlgItem(*this, IDC_URLTREE), &rect);
			m_xSliderPos = rect.right+4;
			GetClientRect(GetDlgItem(*this, IDC_MONITOREDURLS), &rect);
			m_ySliderPos = rect.bottom+m_topmarg;
			GetClientRect(GetDlgItem(*this, IDC_LOGINFO), &rect);
			m_bottommarg = rect.bottom+4+m_ySliderPos;
			GetClientRect(*this, &rect);
			m_bottommarg = rect.bottom - m_bottommarg;

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
				HWND hTreeControl = GetDlgItem(*this, IDC_URLTREE);
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
						TreeView_GetItem(hTreeControl, &tv.itemex);
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
							TreeView_SetItem(hTreeControl, &tv.itemex);
							if (tv.itemex.state & TVIS_SELECTED)
							{
								m_bBlockListCtrlUI = true;
								TreeItemSelected(hTreeControl, tv.itemex.hItem);
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
						TreeView_InsertItem(hTreeControl, &tv);
						TreeView_Expand(hTreeControl, tv.hParent, TVE_EXPAND);
						m_bBlockListCtrlUI = false;
					}
					delete [] str;
				}
				m_pURLInfos->ReleaseReadOnlyData();
				::InvalidateRect(GetDlgItem(*this, IDC_MONITOREDURLS), NULL, true);
			}
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
		{
			EndDialog(*this, IDCANCEL);
		}
		break;
	case IDCANCEL:
		{
			int res = ::MessageBox(*this, _T("Do you really want to quit the CommitMonitor?\nIf you quit, monitoring will stop.\nIf you just want to close the dialog, use the \"OK\" button.\n\nAre you sure you want to quit the CommitMonitor?"),
				_T("CommitMonitor"), MB_ICONQUESTION|MB_YESNO);
			if (res != IDYES)
				break;
			EndDialog(*this, IDCANCEL);
			PostQuitMessage(IDOK);
		}
		break;
	case ID_MAIN_REMOVE:
		{
			HWND hTreeControl = GetDlgItem(*this, IDC_URLTREE);
			HWND hListControl = GetDlgItem(*this, IDC_MONITOREDURLS);
			// which control has the focus?
			HWND hFocus = ::GetFocus();
			if (hFocus == hTreeControl)
			{
				HTREEITEM hItem = TreeView_GetSelection(hTreeControl);
				if (hItem)
				{
					TVITEMEX itemex = {0};
					itemex.hItem = hItem;
					itemex.mask = TVIF_PARAM;
					TreeView_GetItem(hTreeControl, &itemex);
					map<wstring,CUrlInfo> * pWrite = m_pURLInfos->GetWriteData();
					HTREEITEM hPrev = TVI_ROOT;
					if (pWrite->find(*(wstring*)itemex.lParam) != pWrite->end())
					{
						wstring mask = (*pWrite)[(*(wstring*)itemex.lParam)].name;
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

							pWrite->erase(*(wstring*)itemex.lParam);
							::SendMessage(m_hParent, COMMITMONITOR_CHANGEDINFO, (WPARAM)false, (LPARAM)false);
							hPrev = TreeView_GetPrevSibling(hTreeControl, hItem);
							m_pURLInfos->ReleaseWriteData();
							TreeView_DeleteItem(hTreeControl, hItem);
							if (hPrev == NULL)
								hPrev = TreeView_GetRoot(hTreeControl);
							if ((hPrev)&&(hPrev != TVI_ROOT))
								TreeView_SelectItem(hTreeControl, hPrev);
						}
						else
							m_pURLInfos->ReleaseWriteData();
					}
					else
						m_pURLInfos->ReleaseWriteData();
				}
			}
			else if (hFocus == hListControl)
			{
				RemoveSelectedListItems();
			}
		}
		break;
	case ID_MAIN_EDIT:
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
						CAppUtils::LaunchApplication(cmd);
					}
				}
			}
			m_pURLInfos->ReleaseReadOnlyData();
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

	m_bBlockListCtrlUI = true;
	HWND hTreeControl = GetDlgItem(*this, IDC_URLTREE);
	// first clear the controls (the data)
	ListView_DeleteAllItems(GetDlgItem(*this, IDC_MONITOREDURLS));
	TreeView_SelectItem(hTreeControl, NULL);
	TreeView_DeleteAllItems(hTreeControl);
	SetWindowText(GetDlgItem(*this, IDC_LOGINFO), _T(""));
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
	m_pURLInfos->ReleaseReadOnlyData();
	m_bBlockListCtrlUI = false;
	::InvalidateRect(GetDlgItem(*this, IDC_MONITOREDURLS), NULL, true);
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
	HWND hTreeControl = GetDlgItem(*this, IDC_URLTREE);
	if (hItem == TVI_ROOT)
		hItem = TreeView_GetRoot(hTreeControl);
	TVITEM item;
	item.mask = TVIF_PARAM;
	while (hItem)
	{
		item.hItem = hItem;
		TreeView_GetItem(hTreeControl, &item);
		if (url.compare(*(wstring*)item.lParam) == 0)
			return hItem;
		HTREEITEM hChild = TreeView_GetChild(hTreeControl, hItem);
		if (hChild)
		{
			item.hItem = hChild;
			TreeView_GetItem(hTreeControl, &item);
			hChild = FindTreeNode(url, hChild);
			if (hChild != TVI_ROOT)
				return hChild;
		}
		hItem = TreeView_GetNextSibling(hTreeControl, hItem);
	};
	return TVI_ROOT;
}

void CMainDlg::OnSelectTreeItem(LPNMTREEVIEW lpNMTreeView)
{
	HTREEITEM hSelectedItem = lpNMTreeView->itemNew.hItem;
	SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_MAIN_EDIT, 
		MAKELONG(!!(lpNMTreeView->itemNew.state & TVIS_SELECTED), 0));
	SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_MAIN_REMOVE, 
		MAKELONG(!!(lpNMTreeView->itemNew.state & TVIS_SELECTED), 0));
	if (lpNMTreeView->itemNew.state & TVIS_SELECTED)
		TreeItemSelected(lpNMTreeView->hdr.hwndFrom, hSelectedItem);
	else
	{
		ListView_DeleteAllItems(GetDlgItem(*this, IDC_MONITOREDURLS));
		SetWindowText(GetDlgItem(*this, IDC_LOGINFO), _T(""));
	}
	HWND hMsgView = GetDlgItem(*this, IDC_LOGINFO);
	SetWindowText(hMsgView, _T(""));
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
		HWND hLogList = GetDlgItem(*this, IDC_MONITOREDURLS);

		m_bBlockListCtrlUI = true;
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
		m_bBlockListCtrlUI = false;
		ListView_SetColumnWidth(hLogList, 0, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(hLogList, 1, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(hLogList, 2, LVSCW_AUTOSIZE_USEHEADER);

		::InvalidateRect(hLogList, NULL, false);
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
		HWND hListView = GetDlgItem(*this, IDC_MONITOREDURLS);
		LVITEM item = {0};
		item.mask = LVIF_PARAM;
		item.iItem = lpNMListView->iItem;
		ListView_GetItem(hListView, &item);
		SVNLogEntry * pLogEntry = (SVNLogEntry*)item.lParam;
		if (pLogEntry)
		{
			HWND hTreeControl = GetDlgItem(*this, IDC_URLTREE);
			HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeControl);
			// get the url this entry refers to
			TVITEMEX itemex = {0};
			itemex.hItem = hSelectedItem;
			itemex.mask = TVIF_PARAM;
			TreeView_GetItem(hTreeControl, &itemex);
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
                    TreeView_SetItem(hTreeControl, &itemex);
                }
            }
			// the icon in the system tray needs to be changed back
			// to 'normal'
			::SendMessage(m_hParent, COMMITMONITOR_CHANGEDINFO, (WPARAM)false, (LPARAM)false);
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
	HWND hListView = GetDlgItem(*this, IDC_MONITOREDURLS);
	int selCount = ListView_GetSelectedCount(hListView);
	if (selCount <= 0)
		return;	//nothing selected, nothing to remove
	int nFirstDeleted = -1;
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
		if (ListView_GetItemCount(hListView) > nFirstDeleted)
		{
			ListView_SetItemState(hListView, nFirstDeleted, LVIS_SELECTED, LVIS_SELECTED);
		}
		else
		{
			ListView_SetItemState(hListView, ListView_GetItemCount(hListView)-1, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
}
/******************************************************************************/
/* tree, list view and dialog resizing                                        */
/******************************************************************************/

void CMainDlg::DoResize(int width, int height)
{
	// when we get here, the controls haven't been resized yet
	RECT tree, list, log, ok;
	HWND hTree = GetDlgItem(*this, IDC_URLTREE);
	HWND hList = GetDlgItem(*this, IDC_MONITOREDURLS);
	HWND hLog = GetDlgItem(*this, IDC_LOGINFO);
	HWND hOK = GetDlgItem(*this, IDOK);
	::GetClientRect(hTree, &tree);
	::GetClientRect(hList, &list);
	::GetClientRect(hLog, &log);
	::GetClientRect(hOK, &ok);
	HDWP hdwp = BeginDeferWindowPos(5);
	hdwp = DeferWindowPos(hdwp, m_hwndToolbar, *this, 0, 0, width, m_topmarg, SWP_NOZORDER|SWP_NOACTIVATE);
	hdwp = DeferWindowPos(hdwp, hTree, *this, 0, m_topmarg, m_xSliderPos, height-m_topmarg-m_bottommarg+5, SWP_NOZORDER|SWP_NOACTIVATE);
	hdwp = DeferWindowPos(hdwp, hList, *this, m_xSliderPos+4, m_topmarg, width-m_xSliderPos, m_ySliderPos-m_topmarg+4, SWP_NOZORDER|SWP_NOACTIVATE);
	hdwp = DeferWindowPos(hdwp, hLog, *this, m_xSliderPos+4, m_ySliderPos+8, width-m_xSliderPos-4, height-m_bottommarg-m_ySliderPos-4, SWP_NOZORDER|SWP_NOACTIVATE);
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

	// initialize the window position infos
	GetClientRect(GetDlgItem(*this, IDC_URLTREE), &rect);
	m_xSliderPos = rect.right+4;
	GetClientRect(GetDlgItem(*this, IDC_MONITOREDURLS), &rect);
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
