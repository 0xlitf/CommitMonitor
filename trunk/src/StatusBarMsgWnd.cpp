#include "stdafx.h"
#include "StatusBarMsgWnd.h"
#include "resource.h"
#include "Registry.h"
#include <ShellAPI.h>
#include <assert.h>

#include "MemDC.h"

#pragma comment(lib, "Winmm")

int CStatusBarMsgWnd::m_counter = 0;
vector<int> CStatusBarMsgWnd::m_slots;

CStatusBarMsgWnd::~CStatusBarMsgWnd()
{
	if (m_icon)
		::DestroyIcon(m_icon);
}

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

void CStatusBarMsgWnd::Show(LPCTSTR title, LPCTSTR text, UINT icon, HWND hParentWnd, UINT messageOnClick, int stay /* = 10 */)
{
	m_title = wstring(title);
	m_text = wstring(text);
	m_hParentWnd = hParentWnd;
	m_messageOnClick = messageOnClick;
	m_stay = stay;
	m_icon = (HICON)::LoadImage(hResource, MAKEINTRESOURCE(icon), IMAGE_ICON, 
		STATUSBARMSGWND_ICONSIZE, STATUSBARMSGWND_ICONSIZE, LR_DEFAULTCOLOR);

	::SystemParametersInfo(SPI_GETWORKAREA, 0, &m_workarea, 0);

	// find the taskbar position
	APPBARDATA apdata = {0};
	apdata.cbSize = sizeof(APPBARDATA);
	apdata.hWnd = FindWindow(_T("Shell_TrayWnd"), NULL); 
	SHAppBarMessage(ABM_GETTASKBARPOS, &apdata);
	m_uEdge = apdata.uEdge;

	m_ShowTicks = 0;
	m_counter++;
	// find an empty slot
	m_thiscounter = 0;
	for (vector<int>::iterator it = m_slots.begin(); it != m_slots.end(); ++it)
	{
		if (*it)
			m_thiscounter++;
		else
		{
			*it = 1;
			break;
		}
	}
	if (m_thiscounter >= (int)m_slots.size())
		m_slots.push_back(1);
	SetTimer(*this, STATUSBARMSGWND_SHOWTIMER, 10, NULL);
	// play the notification sound
	CRegStdWORD regPlay(_T("Software\\CommitMonitor\\PlaySound"), TRUE);
	if (DWORD(regPlay))
	{
		CRegStdString regSound(_T("Software\\CommitMonitor\\NotificationSound"));
		if (wstring(regSound).empty())
			PlaySound(_T("MailBeep"), NULL, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
		else
			PlaySound(wstring(regSound).c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
	}
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
	case WM_LBUTTONUP:
		// user clicked on the popup window
		::SendMessage(m_hParentWnd, m_messageOnClick, wParam, lParam);
		break;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_PAINT:
		{
			RECT rect;
			if (GetUpdateRect(*this, &rect, false))
			{
				::GetClientRect(*this, &rect);
				// adjust the rectangle according to which part is hidden
				switch (m_uEdge)
				{
				case ABE_BOTTOM:
					rect.bottom = rect.top + m_height;
					break;
				case ABE_LEFT:
					rect.left = rect.right - m_width;
					break;
				case ABE_RIGHT:
					rect.right = rect.left + m_width;
					break;
				case ABE_TOP:
					rect.top = rect.bottom - m_height;
					break;
				default:
					break;
				}
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);
				CMemDC memdc(hdc);
				OnPaint(memdc, &rect);
				EndPaint(hwnd, &ps);
			}
		}
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
};

void CStatusBarMsgWnd::OnPaint(HDC hDC, LPRECT pRect)
{
	// erase the background
	SetBkColor(hDC, ::GetSysColor(COLOR_WINDOW));
	::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, pRect, NULL, 0, NULL);

	// draw a border
	DrawEdge(hDC, pRect, EDGE_BUMP, BF_ADJUST | BF_RECT | BF_SOFT);

	// draw the icon on the left, but centered vertically
	DrawIconEx(hDC, pRect->left + 2, 
		(pRect->bottom - pRect->top - STATUSBARMSGWND_ICONSIZE)/2, m_icon, 
		STATUSBARMSGWND_ICONSIZE, STATUSBARMSGWND_ICONSIZE, 0, NULL, DI_NORMAL);

	// draw the title
	HFONT hFont = CreateFont(-MulDiv(10, GetDeviceCaps(hDC, LOGPIXELSY), 72), 0, 0, 0, 
		FW_BOLD, false, false, false, ANSI_CHARSET, OUT_DEFAULT_PRECIS, 
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, _T("MS Shell Dlg"));
	HFONT hFontOld = (HFONT)SelectObject(hDC, (HGDIOBJ)hFont);
	SetTextColor(hDC, ::GetSysColor(COLOR_WINDOWTEXT));
	RECT titlerect = *pRect;
	titlerect.left += (STATUSBARMSGWND_ICONSIZE + 5);
	titlerect.top += 2;
	titlerect.right -= 5;
	TCHAR * textbuf = new TCHAR[m_title.size()+1];
	_tcscpy_s(textbuf, m_title.size()+1, m_title.c_str());
	DrawTextEx(hDC, textbuf, m_title.length(), &titlerect, DT_CALCRECT|DT_CENTER|DT_WORD_ELLIPSIS, NULL);
	titlerect.right = pRect->right-5;
	DrawTextEx(hDC, textbuf, m_title.length(), &titlerect, DT_CENTER|DT_WORD_ELLIPSIS, NULL);
	delete [] textbuf;
	SelectObject(hDC, hFontOld);
	DeleteObject(hFont);

	// draw a separator line
	HPEN hPen = CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_3DSHADOW));
	HPEN hOldPen = (HPEN)::SelectObject(hDC, hPen);
	MoveToEx(hDC, titlerect.left, titlerect.bottom+4, NULL);
	LineTo(hDC, titlerect.right, titlerect.bottom+4);
	::SelectObject(hDC, hOldPen);
	DeleteObject(hPen);
	hPen = CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_3DHIGHLIGHT));
	hOldPen = (HPEN)::SelectObject(hDC, hPen);
	MoveToEx(hDC, titlerect.left, titlerect.bottom+5, NULL);
	LineTo(hDC, titlerect.right, titlerect.bottom+5);
	::SelectObject(hDC, hOldPen);
	DeleteObject(hPen);

	// draw the status text
	hFont = CreateFont(-MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72), 0, 0, 0, 
		FW_NORMAL, false, false, false, ANSI_CHARSET, OUT_DEFAULT_PRECIS, 
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, _T("MS Shell Dlg"));
	hFontOld = (HFONT)SelectObject(hDC, (HGDIOBJ)hFont);
	SetTextColor(hDC, ::GetSysColor(COLOR_GRAYTEXT));
	RECT statusrect = *pRect;
	statusrect.left += (STATUSBARMSGWND_ICONSIZE + 5);
	statusrect.top = (titlerect.bottom + 8);
	textbuf = new TCHAR[m_text.size()+1];
	_tcscpy_s(textbuf, m_text.size()+1, m_text.c_str());
	//DrawTextEx(hDC, textbuf, m_text.length(), &statusrect, DT_CALCRECT|DT_CENTER|DT_WORD_ELLIPSIS, NULL);
	DrawTextEx(hDC, textbuf, m_text.length(), &statusrect, DT_CENTER|DT_WORD_ELLIPSIS, NULL);
	delete [] textbuf;
	SelectObject(hDC, hFontOld);
	DeleteObject(hFont);
}

LRESULT CStatusBarMsgWnd::DoTimer()
{
	bool finished = false;
	if (((m_uEdge == ABE_TOP)||(m_uEdge == ABE_BOTTOM)) &&
		(m_ShowTicks >= ((m_stay+2)*m_height)))
		finished = true;
	if (((m_uEdge == ABE_LEFT)||(m_uEdge == ABE_RIGHT)) &&
		(m_ShowTicks >= ((m_stay+2)*m_width)))
		finished = true;
	if (finished)
	{
		::KillTimer(*this, STATUSBARMSGWND_SHOWTIMER);
		DestroyWindow(*this);
		m_slots[m_thiscounter] = 0;
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
	if ((m_ShowTicks > m_width) && (m_ShowTicks < ((m_stay+1)*m_width)))
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
	xPos += ((m_thiscounter)*m_width);

	RECT popupRect;
	popupRect.left = m_workarea.left + ((m_thiscounter)*m_width);
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
	if ((m_ShowTicks > m_height) && (m_ShowTicks < ((m_stay+1)*m_height)))
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
	yPos += ((m_thiscounter)*m_height);

	RECT popupRect;
	popupRect.left = m_workarea.right - m_width;
	popupRect.right = m_workarea.right;
	popupRect.top = m_workarea.top + ((m_thiscounter)*m_height);
	popupRect.bottom = yPos;
	::SetWindowPos(*this, HWND_TOPMOST,
		popupRect.left, popupRect.top, popupRect.right-popupRect.left, popupRect.bottom-popupRect.top,
		SWP_NOACTIVATE|SWP_SHOWWINDOW);
	m_ShowTicks += 2;
}

void CStatusBarMsgWnd::ShowFromRight()
{
	LONG xPos = m_workarea.right;
	if ((m_ShowTicks > m_width) && (m_ShowTicks < ((m_stay+1)*m_width)))
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
	xPos -= ((m_thiscounter)*m_width);

	RECT popupRect;
	popupRect.left = xPos;
	popupRect.right = m_workarea.right - ((m_thiscounter)*m_width);
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
	if ((m_ShowTicks > m_height) && (m_ShowTicks < ((m_stay+1)*m_height)))
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
	yPos -= ((m_thiscounter)*m_height);

	RECT popupRect;
	popupRect.left = m_workarea.right - m_width;
	popupRect.right = m_workarea.right;
	popupRect.top = yPos;
	popupRect.bottom = m_workarea.bottom - ((m_thiscounter)*m_height);
	::SetWindowPos(*this, HWND_TOPMOST,
		popupRect.left, popupRect.top, popupRect.right-popupRect.left, popupRect.bottom-popupRect.top,
		SWP_NOACTIVATE|SWP_SHOWWINDOW);
	m_ShowTicks += 2;
}