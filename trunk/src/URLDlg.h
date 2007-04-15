#pragma once
#include "basedialog.h"

#include "UrlInfo.h"

/**
 * url dialog.
 */
class CURLDlg : public CDialog
{
public:
	CURLDlg(void);
	~CURLDlg(void);

	void					SetInfo(const CUrlInfo * pURLInfo = NULL);
	CUrlInfo *				GetInfo() {return &info;}

protected:
	LRESULT CALLBACK		DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT					DoCommand(int id);

	bool					OnSetCursor(HWND hWnd, UINT nHitTest, UINT message);
	bool					OnMouseMove(UINT nFlags, POINT point);
	bool					OnLButtonDown(UINT nFlags, POINT point);
	bool					OnLButtonUp(UINT nFlags, POINT point);
	void					DrawXorBar(HDC hDC, LONG x1, LONG y1, LONG width, LONG height);

private:
	CUrlInfo				info;
};
