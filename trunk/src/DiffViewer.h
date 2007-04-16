#pragma once
#include "BaseWindow.h"
#include "Platform.h"
#include "Scintilla.h"

class CDiffViewer : public CWindow
{
public:
	CDiffViewer(HINSTANCE hInst, const WNDCLASSEX* wcx = NULL);
	~CDiffViewer(void);

	/**
	 * Registers the window class and creates the window.
	 */
	bool RegisterAndCreateWindow();

	bool				Initialize();

	LRESULT				SendEditor(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
	HWND				GetHWNDEdit() { return m_hWndEdit; }
	bool				LoadFile(LPCTSTR filename);

protected:
	/// the message handler for this window
	LRESULT CALLBACK	WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Handles all the WM_COMMAND window messages (e.g. menu commands)
	LRESULT				DoCommand(int id);

private:
	void				SetAStyle(int style, COLORREF fore, COLORREF back=::GetSysColor(COLOR_WINDOW), 
									int size=-1, const char *face=0);
	bool                IsUTF8(LPVOID pBuffer, int cb);

private:
	LRESULT				m_directFunction;
	LRESULT				m_directPointer;

	HWND				m_hWndEdit;
};
