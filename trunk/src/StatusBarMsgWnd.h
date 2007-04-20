#pragma once
#include "BaseWindow.h"
#include <string>

using namespace std;

#define STATUSBARMSGWND_SHOWTIMER		101

class CStatusBarMsgWnd : public CWindow
{
public:
	CStatusBarMsgWnd(HINSTANCE hInst, const WNDCLASSEX* wcx = NULL) 
		: CWindow(hInst, wcx) 
		, m_width(200)
		, m_height(200)
	{
		RegisterAndCreateWindow();
	}

	void				Show(LPCTSTR title, LPCTSTR text, HWND hParentWnd, UINT messageOnClick);
private:
	// deconstructor private to prevent creating an instance on the stack
	// --> must be created on the heap!
	~CStatusBarMsgWnd(void) {}


protected:
	/**
	 * Registers the window class and creates the window.
	 */
	bool RegisterAndCreateWindow();
	/// the message handler for this window
	LRESULT CALLBACK	WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT				DoTimer();

	void				ShowFromLeft();
	void				ShowFromTop();
	void				ShowFromRight();
	void				ShowFromBottom();

private:
	wstring				m_title;
	wstring				m_text;
	UINT				m_messageOnClick;
	HWND				m_hParentWnd;

	UINT				m_uEdge;
	RECT				m_workarea;
	int					m_ShowTicks;

	LONG				m_width;
	LONG				m_height;

	int					m_thiscounter;
	static int			m_counter;
};

