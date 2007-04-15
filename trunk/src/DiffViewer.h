#pragma once
#include "Platform.h"
#include "Scintilla.h"

class CDiffViewer
{
public:
	CDiffViewer(void);
	~CDiffViewer(void);

	bool				Initialize();

	LRESULT				SendEditor(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
	HWND				GetHWND() { return m_hWnd; }
	bool				LoadFile(LPCTSTR filename);

private:
	void				SetAStyle(int style, COLORREF fore, COLORREF back=::GetSysColor(COLOR_WINDOW), 
									int size=-1, const char *face=0);

private:
	LRESULT				m_directFunction;
	LRESULT				m_directPointer;

	HWND				m_hWnd;
};
