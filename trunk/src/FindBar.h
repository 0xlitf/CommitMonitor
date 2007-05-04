#pragma once
#include "basedialog.h"


/**
 * FindBar.
 * A search bar similar to the one found in FireFox
 */
class CFindBar : public CDialog
{
public:
	CFindBar();
	~CFindBar(void);

    void                    SetParent(HWND hParent) {m_hParent = hParent;}
protected:
	LRESULT CALLBACK		DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT					DoCommand(int id, int msg);

	void					DoFind(bool bFindPrev);

private:
	HWND					m_hParent;
	HBITMAP					m_hBmp;
};
