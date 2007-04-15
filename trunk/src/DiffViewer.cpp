#include "StdAfx.h"
#include "DiffViewer.h"
#include "SciLexer.h"

#include <stdio.h>

extern HINSTANCE hInst;

CDiffViewer::CDiffViewer(void)
{
	Scintilla_RegisterClasses(hInst);
}

CDiffViewer::~CDiffViewer(void)
{
}

LRESULT CDiffViewer::SendEditor(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (m_directFunction)
	{
		return ((SciFnDirect) m_directFunction)(m_directPointer, Msg, wParam, lParam);
	}
	return ::SendMessage(m_hWnd, Msg, wParam, lParam);	
}

bool CDiffViewer::Initialize()
{
	m_hWnd = ::CreateWindow(
		_T("Scintilla"),
		_T("Source"),
		WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX | WS_SYSMENU | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
		0, 0,
		640, 480,
		NULL,
		0,
		hInst,
		0);
	if (m_hWnd == NULL)
		return false;
	m_directFunction = SendMessage(m_hWnd, SCI_GETDIRECTFUNCTION, 0, 0);
	m_directPointer = SendMessage(m_hWnd, SCI_GETDIRECTPOINTER, 0, 0);

	// Set up the global default style. These attributes are used wherever no explicit choices are made.
	//SetAStyle(STYLE_DEFAULT, black, white, (DWORD)CRegStdWORD(_T("Software\\TortoiseMerge\\LogFontSize"), 10), 
	//	((stdstring)(CRegStdString(_T("Software\\TortoiseMerge\\LogFontName"), _T("Courier New")))).c_str());
	SetAStyle(STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOWTEXT), ::GetSysColor(COLOR_WINDOW), 10, "Courier New");
	SendEditor(SCI_SETTABWIDTH, 4);
	SendEditor(SCI_SETREADONLY, TRUE);
	LRESULT pix = SendEditor(SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)"_99999");
	SendEditor(SCI_SETMARGINWIDTHN, 0, pix);
	SendEditor(SCI_SETMARGINWIDTHN, 1);
	SendEditor(SCI_SETMARGINWIDTHN, 2);
	//Set the default windows colors for edit controls
	SendEditor(SCI_STYLESETFORE, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOWTEXT));
	SendEditor(SCI_STYLESETBACK, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOW));
	SendEditor(SCI_SETSELFORE, TRUE, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
	SendEditor(SCI_SETSELBACK, TRUE, ::GetSysColor(COLOR_HIGHLIGHT));
	SendEditor(SCI_SETCARETFORE, ::GetSysColor(COLOR_WINDOWTEXT));

	return true;
}

bool CDiffViewer::LoadFile(LPCTSTR filename)
{
	SendEditor(SCI_SETREADONLY, FALSE);
	SendEditor(SCI_CLEARALL);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_CANCEL);
	SendEditor(SCI_SETUNDOCOLLECTION, 0);

	FILE *fp = NULL;
	_tfopen_s(&fp, filename, _T("rb"));
	if (fp) 
	{
		//SetTitle();
		char data[4096];
		int lenFile = fread(data, 1, sizeof(data), fp);
		while (lenFile > 0) 
		{
			SendEditor(SCI_ADDTEXT, lenFile,
				reinterpret_cast<LPARAM>(static_cast<char *>(data)));
			lenFile = fread(data, 1, sizeof(data), fp);
		}
		fclose(fp);
	}
	else 
	{
		return false;
	}

	SendEditor(SCI_SETUNDOCOLLECTION, 1);
	::SetFocus(m_hWnd);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_GOTOPOS, 0);
	SendEditor(SCI_SETREADONLY, TRUE);

	SendEditor(SCI_CLEARDOCUMENTSTYLE, 0, 0);
	SendEditor(SCI_SETSTYLEBITS, 5, 0);

	//SetAStyle(SCE_DIFF_DEFAULT, RGB(0, 0, 0));
	SetAStyle(SCE_DIFF_COMMAND, RGB(0x0A, 0x24, 0x36));
	SetAStyle(SCE_DIFF_POSITION, RGB(0xFF, 0, 0));
	SetAStyle(SCE_DIFF_HEADER, RGB(0x80, 0, 0), RGB(0xFF, 0xFF, 0x80));
	SetAStyle(SCE_DIFF_COMMENT, RGB(0, 0x80, 0));
	SendEditor(SCI_STYLESETBOLD, SCE_DIFF_COMMENT, TRUE);
	SetAStyle(SCE_DIFF_DELETED, ::GetSysColor(COLOR_WINDOWTEXT), RGB(0xFF, 0x80, 0x80));
	SetAStyle(SCE_DIFF_ADDED, ::GetSysColor(COLOR_WINDOWTEXT), RGB(0x80, 0xFF, 0x80));

	SendEditor(SCI_SETLEXER, SCLEX_DIFF);
	//SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)"Revision");
	SendEditor(SCI_COLOURISE, 0, -1);
	::ShowWindow(m_hWnd, SW_SHOW);
	return true;
}

void CDiffViewer::SetAStyle(int style, COLORREF fore, COLORREF back, int size, const char *face) 
{
	SendEditor(SCI_STYLESETFORE, style, fore);
	SendEditor(SCI_STYLESETBACK, style, back);
	if (size >= 1)
		SendEditor(SCI_STYLESETSIZE, style, size);
	if (face) 
		SendEditor(SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(face));
}
