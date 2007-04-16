#pragma once
#include <string>
#include "basedialog.h"
#include "UrlInfo.h"

#define REPOBROWSER_CTRL_MIN_WIDTH 50
#define REPOBROWSER_CTRL_MIN_HEIGHT 40

#define DRAGMODE_NONE			0
#define DRAGMODE_HORIZONTAL		1
#define DRAGMODE_VERTICAL		2

using namespace std;

/**
 * main dialog.
 */
class CMainDlg : public CDialog
{
public:
	CMainDlg(HWND hParent);
	~CMainDlg(void);

	void					SetUrlInfos(CUrlInfos * pUrlInfos) {m_pURLInfos = pUrlInfos;}

protected:
	LRESULT CALLBACK		DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT					DoCommand(int id);

	bool					OnSetCursor(HWND hWnd, UINT nHitTest, UINT message);
	bool					OnMouseMove(UINT nFlags, POINT point);
	bool					OnLButtonDown(UINT nFlags, POINT point);
	bool					OnLButtonUp(UINT nFlags, POINT point);
	void					DrawXorBar(HDC hDC, LONG x1, LONG y1, LONG width, LONG height);

	void					RefreshURLTree();
	HTREEITEM				FindParentTreeNode(const wstring& url);
	HTREEITEM				FindTreeNode(const wstring& url);
	void					OnSelectTreeItem(LPNMTREEVIEW lpNMTreeView);
	void					OnSelectListItem(LPNMLISTVIEW lpNMListView);
	LRESULT					OnCustomDrawListItem(LPNMLVCUSTOMDRAW lpNMCustomDraw);
	void					OnKeyDownListItem(LPNMLVKEYDOWN pnkd);

	void					SaveURLInfo();
	void					LoadURLInfo();
private:	
	HWND					m_hParent;
	UINT					COMMITMONITOR_CHANGEDINFO;

	int						m_nDragMode;
	LONG					m_oldx, m_oldy;

	HFONT					m_boldFont;

	CUrlInfos *				m_pURLInfos;

	bool					m_bBlockListCtrlUI;
};
