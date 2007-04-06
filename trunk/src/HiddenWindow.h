#pragma once
#include "BaseWindow.h"

class CHiddenWindow : public CWindow
{
public:
	CHiddenWindow(HINSTANCE hInst, const WNDCLASSEX* wcx = NULL) : CWindow(hInst, wcx)
	{

	}
	~CHiddenWindow(void);

	/**
	 * Registers the window class and creates the window.
	 */
	bool				RegisterAndCreateWindow();

	INT_PTR				ShowDialog();
protected:
	/// the message handler for this window
	LRESULT CALLBACK	WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Handles all the WM_COMMAND window messages (e.g. menu commands)
	LRESULT				DoCommand(int id);

private:
	UINT				COMMITMONITOR_SHOWDLGMSG;

};
