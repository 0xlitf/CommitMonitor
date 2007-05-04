#pragma once
#include "basedialog.h"


/**
 * options dialog.
 */
class CFindBar : public CDialog
{
public:
	CFindBar();
	~CFindBar(void);

    void                    SetParent(HWND hParent) {m_hParent = hParent;}
protected:
	LRESULT CALLBACK		DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT					DoCommand(int id);

private:
	HWND					m_hParent;
};
