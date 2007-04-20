#include "stdafx.h"
#include "StatusBarMsgWnd.h"
#include <ShellAPI.h>
#include <assert.h>

int CStatusBarMsgWnd::m_counter = 0;

bool CStatusBarMsgWnd::RegisterAndCreateWindow()
{
	WNDCLASSEX wcx; 

	// Fill in the window class structure with default parameters 
	wcx.cbSize = sizeof(WNDCLASSEX);
	wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW | CS_CLASSDC;
	wcx.lpfnWndProc = CWindow::stWinMsgHandler;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	wcx.hInstance = hResource;
	wcx.hCursor = NULL;
	wcx.lpszClassName = _T("StatusBarMsgWnd_{BAB03407-CF65-4942-A1D5-063FA1CA8530}");
	wcx.hIcon = NULL;
	wcx.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wcx.lpszMenuName = NULL;
	wcx.hIconSm	= NULL;
	if (RegisterWindow(&wcx))
	{
		if (CreateEx(WS_EX_TOOLWINDOW, WS_POPUP, NULL))
		{
			return true;
		}
	}
	return false;
}

void CStatusBarMsgWnd::Show(LPCTSTR title, LPCTSTR text, HWND hParentWnd, UINT messageOnClick)
{
	m_title = wstring(title);
	m_text = wstring(text);
	m_hParentWnd = hParentWnd;
	m_messageOnClick = messageOnClick;

	::SystemParametersInfo(SPI_GETWORKAREA, 0, &m_workarea, 0);

	// find the taskbar position
	APPBARDATA apdata = {0};
	apdata.cbSize = sizeof(APPBARDATA);
	apdata.hWnd = FindWindow(_T("Shell_TrayWnd"), NULL); 
	SHAppBarMessage(ABM_GETTASKBARPOS, &apdata);
	m_uEdge = apdata.uEdge;

	m_ShowTicks = 0;
	m_counter++;
	m_thiscounter = m_counter;
	SetTimer(*this, STATUSBARMSGWND_SHOWTIMER, 1, NULL);
}

LRESULT CALLBACK CStatusBarMsgWnd::WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		m_hwnd = hwnd;
		break;
	case WM_TIMER:
		return DoTimer();
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps); 
			RECT rect;
			::GetClientRect(*this, &rect);
			SetBkColor(hdc, ::GetSysColor(COLOR_WINDOW));
			::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
			EndPaint(hwnd, &ps);
		}
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
};

LRESULT CStatusBarMsgWnd::DoTimer()
{
	if (m_ShowTicks >= (3*m_height))
	{
		::KillTimer(*this, STATUSBARMSGWND_SHOWTIMER);
		DestroyWindow(*this);
		m_counter--;
		assert(m_counter >= 0);
		delete this;
		return 0;
	}
	switch (m_uEdge)
	{
	case ABE_BOTTOM:
		ShowFromBottom();
		break;
	case ABE_LEFT:
		ShowFromLeft();
		break;
	case ABE_RIGHT:
		ShowFromRight();
		break;
	case ABE_TOP:
		ShowFromTop();
		break;
	default:
		break;
	}
	return TRUE;
}

void CStatusBarMsgWnd::ShowFromLeft()
{
	LONG xPos = m_workarea.left;
	if ((m_ShowTicks > m_width) && (m_ShowTicks < (2*m_width)))
	{
		// completely visible
		xPos += m_width;
	}
	else if (m_ShowTicks > m_width)
	{
		// going left again
		xPos += (m_width - (m_ShowTicks % m_width));
	}
	else
	{
		// expanding to the right
		xPos += m_ShowTicks;
	}
	xPos += ((m_thiscounter-1)*m_width);

	RECT popupRect;
	popupRect.left = m_workarea.left + ((m_thiscounter-1)*m_width);
	popupRect.right = xPos;
	popupRect.top = m_workarea.top;
	popupRect.bottom = m_workarea.top + m_height;
	::SetWindowPos(*this, HWND_TOPMOST,
		popupRect.left, popupRect.top, popupRect.right-popupRect.left, popupRect.bottom-popupRect.top,
		SWP_NOACTIVATE|SWP_SHOWWINDOW);
	m_ShowTicks += 2;
}

void CStatusBarMsgWnd::ShowFromTop()
{
	LONG yPos = m_workarea.top;
	if ((m_ShowTicks > m_height) && (m_ShowTicks < (2*m_height)))
	{
		// completely visible
		yPos += m_height;
	}
	else if (m_ShowTicks > m_height)
	{
		// going up again
		yPos += (m_height - (m_ShowTicks % m_height));
	}
	else
	{
		// expanding down
		yPos += m_ShowTicks;
	}
	yPos += ((m_thiscounter-1)*m_height);

	RECT popupRect;
	popupRect.left = m_workarea.right - m_width;
	popupRect.right = m_workarea.right;
	popupRect.top = m_workarea.top + ((m_thiscounter-1)*m_height);
	popupRect.bottom = yPos;
	::SetWindowPos(*this, HWND_TOPMOST,
		popupRect.left, popupRect.top, popupRect.right-popupRect.left, popupRect.bottom-popupRect.top,
		SWP_NOACTIVATE|SWP_SHOWWINDOW);
	m_ShowTicks += 2;
}

void CStatusBarMsgWnd::ShowFromRight()
{
	LONG xPos = m_workarea.right;
	if ((m_ShowTicks > m_width) && (m_ShowTicks < (2*m_width)))
	{
		// completely visible
		xPos -= m_width;
	}
	else if (m_ShowTicks > m_width)
	{
		// going right again
		xPos -= (m_width - (m_ShowTicks % m_width));
	}
	else
	{
		// expanding to the left
		xPos -= m_ShowTicks;
	}
	xPos -= ((m_thiscounter-1)*m_width);

	RECT popupRect;
	popupRect.left = xPos;
	popupRect.right = m_workarea.right - ((m_thiscounter-1)*m_width);
	popupRect.top = m_workarea.top;
	popupRect.bottom = m_workarea.top + m_height;
	::SetWindowPos(*this, HWND_TOPMOST,
		popupRect.left, popupRect.top, popupRect.right-popupRect.left, popupRect.bottom-popupRect.top,
		SWP_NOACTIVATE|SWP_SHOWWINDOW);
	m_ShowTicks += 2;
}

void CStatusBarMsgWnd::ShowFromBottom()
{
	LONG yPos = m_workarea.bottom;
	if ((m_ShowTicks > m_height) && (m_ShowTicks < (2*m_height)))
	{
		// completely visible
		yPos -= m_height;
	}
	else if (m_ShowTicks > m_height)
	{
		// going down again
		yPos -= (m_height - (m_ShowTicks % m_height));
	}
	else
	{
		// rising
		yPos -= m_ShowTicks;
	}
	yPos -= ((m_thiscounter-1)*m_height);

	RECT popupRect;
	popupRect.left = m_workarea.right - m_width;
	popupRect.right = m_workarea.right;
	popupRect.top = yPos;
	popupRect.bottom = m_workarea.bottom - ((m_thiscounter-1)*m_height);
	::SetWindowPos(*this, HWND_TOPMOST,
		popupRect.left, popupRect.top, popupRect.right-popupRect.left, popupRect.bottom-popupRect.top,
		SWP_NOACTIVATE|SWP_SHOWWINDOW);
	m_ShowTicks += 2;
}