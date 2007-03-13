#pragma once
#include "basedialog.h"

/**
 * main dialog.
 * hosts the UI's of all the rules.
 */
class CMainDlg : public CDialog
{
public:
	CMainDlg(void);
	~CMainDlg(void);

protected:
	LRESULT CALLBACK		DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT					DoCommand(int id);

protected:
};
