#pragma once
#include "basedialog.h"
#include "UrlInfo.h"

#define REPOBROWSER_CTRL_MIN_WIDTH 50

/**
 * main dialog.
 */
class CMainDlg : public CDialog
{
public:
	CMainDlg(void);
	~CMainDlg(void);

protected:
	LRESULT CALLBACK		DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT					DoCommand(int id);

	bool					OnSetCursor(HWND hWnd, UINT nHitTest, UINT message);
	bool					OnMouseMove(UINT nFlags, POINT point);
	bool					OnLButtonDown(UINT nFlags, POINT point);
	bool					OnLButtonUp(UINT nFlags, POINT point);
	void					DrawXorBar(HDC hDC, LONG x1, LONG y1, LONG width, LONG height);

	void					RefreshURLTree();

	void					SaveURLInfo();
	void					LoadURLInfo();
private:
	bool					m_bThreadRunning;

	bool					m_bDragMode;
	LONG					m_oldx, m_oldy;

	CUrlInfos				m_URLInfos;
};
