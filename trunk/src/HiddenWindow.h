#pragma once
#include <shellapi.h>
#include "BaseWindow.h"
#include "MainDlg.h"
#include "UrlInfo.h"
#include "resource.h"

#pragma comment(lib, "shell32.lib")

/// the timer IDs
#define IDT_MONITOR		101
#define IDT_ANIMATE		102

/// timer elapse time, set to 1 minute
#define TIMER_ELAPSE	60000
#define TIMER_ANIMATE	400

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
	LRESULT				HandleCustomMessages(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	void				DoTimer(bool bForce);
	void				DoAnimate();
	void				ShowTrayIcon(bool newCommits);

private:
	UINT				COMMITMONITOR_SHOWDLGMSG;
	UINT				WM_TASKBARCREATED;

	int					m_nIcon;
	NOTIFYICONDATA		m_SystemTray;
	HICON				m_hIconNormal;
	HICON				m_hIconNew0;
	HICON				m_hIconNew1;
	HICON				m_hIconNew2;
	HICON				m_hIconNew3;

	CUrlInfos			m_UrlInfos;
	DWORD				m_ThreadRunning;
	bool				m_bRun;
	HANDLE				m_hMonitorThread;

	bool				m_bMainDlgShown;
	bool				m_bMainDlgRemovedItems;
	HWND				m_hMainDlg;

	CRegStdWORD			regShowTaskbarIcon;


	typedef BOOL(__stdcall *PFNCHANGEWINDOWMESSAGEFILTER)(UINT message, DWORD dwFlag);
	static PFNCHANGEWINDOWMESSAGEFILTER m_pChangeWindowMessageFilter;
};
