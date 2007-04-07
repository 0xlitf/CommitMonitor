#pragma once
#include "BaseWindow.h"
#include "UrlInfo.h"

/// the timer ID
#define IDT_MONITOR		101
/// timer elapse time, set to 1 minute
#define TIMER_ELAPSE	60000

class CHiddenWindow : public CWindow
{
public:
	CHiddenWindow(HINSTANCE hInst, const WNDCLASSEX* wcx = NULL) 
		: CWindow(hInst, wcx)
		, m_ThreadRunning(0)
	{

	}
	~CHiddenWindow(void);

	/**
	 * Registers the window class and creates the window.
	 */
	bool				RegisterAndCreateWindow();

	INT_PTR				ShowDialog();


	DWORD				RunThread();
protected:
	/// the message handler for this window
	LRESULT CALLBACK	WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Handles all the WM_COMMAND window messages (e.g. menu commands)
	LRESULT				DoCommand(int id);

private:
	void				DoTimer();

private:
	UINT				COMMITMONITOR_SHOWDLGMSG;
	UINT				COMMITMONITOR_CHANGEDINFO;
	CUrlInfos			m_UrlInfos;
	DWORD				m_ThreadRunning;
};
