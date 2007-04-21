#pragma once
#include <shellapi.h>
#include "BaseWindow.h"
#include "MainDlg.h"
#include "UrlInfo.h"
#include "resource.h"

#pragma comment(lib, "shell32.lib")

/// the timer ID
#define IDT_MONITOR		101
/// timer elapse time, set to 1 minute
#define TIMER_ELAPSE	1000//60000

typedef struct
{
	wstring				sTitle;
	wstring				sText;
} popupData;

class CHiddenWindow : public CWindow
{
public:
	CHiddenWindow(HINSTANCE hInst, const WNDCLASSEX* wcx = NULL); 
	~CHiddenWindow(void);

	/**
	 * Registers the window class and creates the window.
	 */
	bool				RegisterAndCreateWindow();

	INT_PTR				ShowDialog();

	void				StopThread();

	DWORD				RunThread();
protected:
	/// the message handler for this window
	LRESULT CALLBACK	WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Handles all the WM_COMMAND window messages (e.g. menu commands)
	LRESULT				DoCommand(int id);
	LRESULT				HandleCustomMessages(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	void				DoTimer();
	void				ShowTrayIcon(bool newCommits);

private:
	UINT				COMMITMONITOR_SHOWDLGMSG;
	UINT				WM_TASKBARCREATED;

	NOTIFYICONDATA		m_SystemTray;
	HICON				m_hIconNormal;
	HICON				m_hIconNew;

	CUrlInfos			m_UrlInfos;
	DWORD				m_ThreadRunning;
	bool				m_bRun;
	HANDLE				m_hMonitorThread;

	bool				m_bMainDlgShown;

	typedef BOOL(__stdcall *PFNCHANGEWINDOWMESSAGEFILTER)(UINT message, DWORD dwFlag);
	static PFNCHANGEWINDOWMESSAGEFILTER m_pChangeWindowMessageFilter;
};
