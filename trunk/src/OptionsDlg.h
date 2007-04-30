#pragma once
#include "basedialog.h"


/**
 * options dialog.
 */
class COptionsDlg : public CDialog
{
public:
	COptionsDlg(HWND hParent);
	~COptionsDlg(void);

	void					SetHiddenWnd(HWND hWnd) {m_hHiddenWnd = hWnd;}
protected:
	LRESULT CALLBACK		DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT					DoCommand(int id);

private:
	HWND					m_hParent;
	HWND					m_hHiddenWnd;
};
