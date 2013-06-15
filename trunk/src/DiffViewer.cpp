// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007-2009, 2012-2013 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "stdafx.h"
#include "DiffViewer.h"
#include "SciLexer.h"
#include "Registry.h"
#include "UnicodeUtils.h"
#include "resource.h"

#include <stdio.h>
#include <memory>

CDiffViewer::CDiffViewer(HINSTANCE hInst, const WNDCLASSEX* wcx /* = NULL*/)
    : CWindow(hInst, wcx)
    , m_bShowFindBar(false)
    , m_directFunction(NULL)
    , m_directPointer(NULL)
    , m_hWndEdit(NULL)
    , m_bMatchCase(false)
{
    Scintilla_RegisterClasses(hInst);
    SetWindowTitle(_T("CommitMonitorDiff"));
}

CDiffViewer::~CDiffViewer(void)
{
}

bool CDiffViewer::RegisterAndCreateWindow()
{
    WNDCLASSEX wcx;

    // Fill in the window class structure with default parameters
    wcx.cbSize = sizeof(WNDCLASSEX);
    wcx.style = CS_HREDRAW | CS_VREDRAW;
    wcx.lpfnWndProc = CWindow::stWinMsgHandler;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hInstance = hResource;
    wcx.hCursor = NULL;
    ResString clsname(hResource, IDS_APP_TITLE);
    wcx.lpszClassName = clsname;
    wcx.hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_DIFF));
    wcx.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
    wcx.lpszMenuName = NULL;
    wcx.hIconSm = LoadIcon(wcx.hInstance, MAKEINTRESOURCE(IDI_DIFF));
    if (RegisterWindow(&wcx))
    {
        if (Create(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN, NULL))
        {
            m_FindBar.SetParent(*this);
            m_FindBar.Create(hResource, IDD_FINDBAR, *this);
            ShowWindow(*this, SW_SHOW);
            UpdateWindow(*this);
            return true;
        }
    }
    return false;
}

LRESULT CALLBACK CDiffViewer::WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        {
            m_hwnd = hwnd;
            Initialize();
        }
        break;
    case WM_COMMAND:
        {
            return DoCommand(LOWORD(wParam));
        }
        break;
    case WM_MOUSEWHEEL:
        {
            if (GET_KEYSTATE_WPARAM(wParam) == MK_SHIFT)
            {
                // scroll sideways
                SendEditor(SCI_LINESCROLL, -GET_WHEEL_DELTA_WPARAM(wParam)/40, 0);
            }
            else
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        break;
    case WM_SIZE:
        {
            RECT rect;
            GetClientRect(*this, &rect);
            if (m_bShowFindBar)
            {
                ::SetWindowPos(m_hWndEdit, HWND_TOP,
                    rect.left, rect.top,
                    rect.right-rect.left, rect.bottom-rect.top-30,
                    SWP_SHOWWINDOW);
                ::SetWindowPos(m_FindBar, HWND_TOP,
                    rect.left, rect.bottom-30,
                    rect.right-rect.left, 30,
                    SWP_SHOWWINDOW);
            }
            else
            {
                ::SetWindowPos(m_hWndEdit, HWND_TOP,
                    rect.left, rect.top,
                    rect.right-rect.left, rect.bottom-rect.top,
                    SWP_SHOWWINDOW);
                ::ShowWindow(m_FindBar, SW_HIDE);
            }
        }
        break;
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO * mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = 100;
            mmi->ptMinTrackSize.y = 100;
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        {
            CRegStdDWORD w = CRegStdDWORD(_T("Software\\CommitMonitor\\DiffViewerWidth"), (DWORD)CW_USEDEFAULT);
            CRegStdDWORD h = CRegStdDWORD(_T("Software\\CommitMonitor\\DiffViewerHeight"), (DWORD)CW_USEDEFAULT);
            CRegStdDWORD p = CRegStdDWORD(_T("Software\\CommitMonitor\\DiffViewerPos"), 0);
            RECT rect;
            ::GetWindowRect(*this, &rect);
            w = rect.right-rect.left;
            h = rect.bottom-rect.top;
            p = MAKELONG(rect.left, rect.top);
        }
        ::DestroyWindow(m_hwnd);
        break;
    case COMMITMONITOR_FINDMSGNEXT:
        {
            SendEditor(SCI_CHARRIGHT);
            SendEditor(SCI_SEARCHANCHOR);
            m_bMatchCase = !!wParam;
            m_findtext = (LPCTSTR)lParam;
            SendEditor(SCI_SEARCHNEXT, m_bMatchCase ? SCFIND_MATCHCASE : 0, (LPARAM)CUnicodeUtils::StdGetUTF8(m_findtext).c_str());
            SendEditor(SCI_SCROLLCARET);
        }
        break;
    case COMMITMONITOR_FINDMSGPREV:
        {
            SendEditor(SCI_SEARCHANCHOR);
            m_bMatchCase = !!wParam;
            m_findtext = (LPCTSTR)lParam;
            SendEditor(SCI_SEARCHPREV, m_bMatchCase ? SCFIND_MATCHCASE : 0, (LPARAM)CUnicodeUtils::StdGetUTF8(m_findtext).c_str());
            SendEditor(SCI_SCROLLCARET);
        }
        break;
    case COMMITMONITOR_FINDEXIT:
        {
            if (IsWindowVisible(m_FindBar))
            {
                RECT rect;
                GetClientRect(*this, &rect);
                m_bShowFindBar = false;
                ::ShowWindow(m_FindBar, SW_HIDE);
                ::SetWindowPos(m_hWndEdit, HWND_TOP,
                    rect.left, rect.top,
                    rect.right-rect.left, rect.bottom-rect.top,
                    SWP_SHOWWINDOW);
            }
            else
                PostQuitMessage(0);
        }
        break;
    case COMMITMONITOR_FINDRESET:
        SendEditor(SCI_SETSELECTIONSTART, 0);
        SendEditor(SCI_SETSELECTIONEND, 0);
        SendEditor(SCI_SEARCHANCHOR);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
};

LRESULT CDiffViewer::DoCommand(int id)
{
    switch (id)
    {
    case IDM_EXIT:
        ::PostQuitMessage(0);
        return 0;
    case IDM_SHOWFINDBAR:
        {
            m_bShowFindBar = true;
            ::ShowWindow(m_FindBar, SW_SHOW);
            RECT rect;
            GetClientRect(*this, &rect);
            ::SetWindowPos(m_hWndEdit, HWND_TOP,
                rect.left, rect.top,
                rect.right-rect.left, rect.bottom-rect.top-30,
                SWP_SHOWWINDOW);
            ::SetWindowPos(m_FindBar, HWND_TOP,
                rect.left, rect.bottom-30,
                rect.right-rect.left, 30,
                SWP_SHOWWINDOW);
            ::SetFocus(m_FindBar);
            SendEditor(SCI_SETSELECTIONSTART, 0);
            SendEditor(SCI_SETSELECTIONEND, 0);
            SendEditor(SCI_SEARCHANCHOR);
        }
        break;
    case IDM_FINDNEXT:
        SendEditor(SCI_CHARRIGHT);
        SendEditor(SCI_SEARCHANCHOR);
        SendEditor(SCI_SEARCHNEXT, m_bMatchCase ? SCFIND_MATCHCASE : 0, (LPARAM)CUnicodeUtils::StdGetUTF8(m_findtext).c_str());
        SendEditor(SCI_SCROLLCARET);
        break;
    case IDM_FINDPREV:
        SendEditor(SCI_SEARCHANCHOR);
        SendEditor(SCI_SEARCHPREV, m_bMatchCase ? SCFIND_MATCHCASE : 0, (LPARAM)CUnicodeUtils::StdGetUTF8(m_findtext).c_str());
        SendEditor(SCI_SCROLLCARET);
        break;
    case IDM_FINDEXIT:
        {
            if (!m_bShowFindBar)
            {
                ::PostQuitMessage(0);
                return 0;
            }
            RECT rect;
            GetClientRect(*this, &rect);
            m_bShowFindBar = false;
            ::ShowWindow(m_FindBar, SW_HIDE);
            ::SetWindowPos(m_hWndEdit, HWND_TOP,
                rect.left, rect.top,
                rect.right-rect.left, rect.bottom-rect.top,
                SWP_SHOWWINDOW);
        }
        break;
    case IDM_PRINT:
        {
            PRINTDLGEX pdlg = {0};
            pdlg.lStructSize = sizeof(PRINTDLGEX);
            pdlg.hwndOwner = *this;
            pdlg.hInstance = NULL;
            pdlg.Flags = PD_USEDEVMODECOPIESANDCOLLATE | PD_ALLPAGES | PD_RETURNDC | PD_NOCURRENTPAGE | PD_NOPAGENUMS;
            pdlg.nMinPage = 1;
            pdlg.nMaxPage = 0xffffU; // We do not know how many pages in the document
            pdlg.nCopies = 1;
            pdlg.hDC = 0;
            pdlg.nStartPage = START_PAGE_GENERAL;

            // See if a range has been selected
            size_t startPos = SendEditor(SCI_GETSELECTIONSTART);
            size_t endPos = SendEditor(SCI_GETSELECTIONEND);

            if (startPos == endPos)
                pdlg.Flags |= PD_NOSELECTION;
            else
                pdlg.Flags |= PD_SELECTION;

            HRESULT hResult = PrintDlgEx(&pdlg);
            if ((hResult != S_OK) || (pdlg.dwResultAction != PD_RESULT_PRINT))
                return 0;

            // reset all indicators
            size_t endpos = SendEditor(SCI_GETLENGTH);
            for (int i = INDIC_CONTAINER; i <= INDIC_MAX; ++i)
            {
                SendEditor(SCI_SETINDICATORCURRENT, i);
                SendEditor(SCI_INDICATORCLEARRANGE, 0, endpos);
            }
            // store and reset UI settings
            int viewws = (int)SendEditor(SCI_GETVIEWWS);
            SendEditor(SCI_SETVIEWWS, 0);
            int edgemode = (int)SendEditor(SCI_GETEDGEMODE);
            SendEditor(SCI_SETEDGEMODE, EDGE_NONE);
            SendEditor(SCI_SETWRAPVISUALFLAGS, SC_WRAPVISUALFLAG_END);

            HDC hdc = pdlg.hDC;

            RECT rectMargins, rectPhysMargins;
            POINT ptPage;
            POINT ptDpi;

            // Get printer resolution
            ptDpi.x = GetDeviceCaps(hdc, LOGPIXELSX);    // dpi in X direction
            ptDpi.y = GetDeviceCaps(hdc, LOGPIXELSY);    // dpi in Y direction

            // Start by getting the physical page size (in device units).
            ptPage.x = GetDeviceCaps(hdc, PHYSICALWIDTH);   // device units
            ptPage.y = GetDeviceCaps(hdc, PHYSICALHEIGHT);  // device units

            // Get the dimensions of the unprintable
            // part of the page (in device units).
            rectPhysMargins.left = GetDeviceCaps(hdc, PHYSICALOFFSETX);
            rectPhysMargins.top = GetDeviceCaps(hdc, PHYSICALOFFSETY);

            // To get the right and lower unprintable area,
            // we take the entire width and height of the paper and
            // subtract everything else.
            rectPhysMargins.right = ptPage.x                        // total paper width
                - GetDeviceCaps(hdc, HORZRES)                       // printable width
                - rectPhysMargins.left;                             // left unprintable margin

            rectPhysMargins.bottom = ptPage.y                       // total paper height
                - GetDeviceCaps(hdc, VERTRES)                       // printable height
                - rectPhysMargins.top;                              // right unprintable margin

            RECT pagesetupMargin;
            CRegStdDWORD m_regMargLeft   = CRegStdDWORD(L"Software\\TortoiseSVN\\UDiffpagesetupmarginleft", 2540);
            CRegStdDWORD m_regMargTop    = CRegStdDWORD(L"Software\\TortoiseSVN\\UDiffpagesetupmargintop", 2540);
            CRegStdDWORD m_regMargRight  = CRegStdDWORD(L"Software\\TortoiseSVN\\UDiffpagesetupmarginright", 2540);
            CRegStdDWORD m_regMargBottom = CRegStdDWORD(L"Software\\TortoiseSVN\\UDiffpagesetupmarginbottom", 2540);

            pagesetupMargin.left   = (long)(DWORD)m_regMargLeft;
            pagesetupMargin.top    = (long)(DWORD)m_regMargTop;
            pagesetupMargin.right  = (long)(DWORD)m_regMargRight;
            pagesetupMargin.bottom = (long)(DWORD)m_regMargBottom;

            if (pagesetupMargin.left != 0 || pagesetupMargin.right != 0 ||
                pagesetupMargin.top != 0 || pagesetupMargin.bottom != 0)
            {
                RECT rectSetup;

                // Convert the hundredths of millimeters (HiMetric) or
                // thousandths of inches (HiEnglish) margin values
                // from the Page Setup dialog to device units.
                // (There are 2540 hundredths of a mm in an inch.)

                TCHAR localeInfo[3];
                GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, localeInfo, 3);

                if (localeInfo[0] == '0')
                {
                    // Metric system. '1' is US System
                    rectSetup.left      = MulDiv (pagesetupMargin.left, ptDpi.x, 2540);
                    rectSetup.top       = MulDiv (pagesetupMargin.top, ptDpi.y, 2540);
                    rectSetup.right     = MulDiv(pagesetupMargin.right, ptDpi.x, 2540);
                    rectSetup.bottom    = MulDiv(pagesetupMargin.bottom, ptDpi.y, 2540);
                }
                else
                {
                    rectSetup.left      = MulDiv(pagesetupMargin.left, ptDpi.x, 1000);
                    rectSetup.top       = MulDiv(pagesetupMargin.top, ptDpi.y, 1000);
                    rectSetup.right     = MulDiv(pagesetupMargin.right, ptDpi.x, 1000);
                    rectSetup.bottom    = MulDiv(pagesetupMargin.bottom, ptDpi.y, 1000);
                }

                // Don't reduce margins below the minimum printable area
                rectMargins.left    = max(rectPhysMargins.left, rectSetup.left);
                rectMargins.top     = max(rectPhysMargins.top, rectSetup.top);
                rectMargins.right   = max(rectPhysMargins.right, rectSetup.right);
                rectMargins.bottom  = max(rectPhysMargins.bottom, rectSetup.bottom);
            }
            else
            {
                rectMargins.left    = rectPhysMargins.left;
                rectMargins.top     = rectPhysMargins.top;
                rectMargins.right   = rectPhysMargins.right;
                rectMargins.bottom  = rectPhysMargins.bottom;
            }

            // rectMargins now contains the values used to shrink the printable
            // area of the page.

            // Convert device coordinates into logical coordinates
            DPtoLP(hdc, (LPPOINT) &rectMargins, 2);
            DPtoLP(hdc, (LPPOINT)&rectPhysMargins, 2);

            // Convert page size to logical units and we're done!
            DPtoLP(hdc, (LPPOINT) &ptPage, 1);


            DOCINFO di = {sizeof(DOCINFO), 0, 0, 0, 0};
            di.lpszDocName = L"CommitMonitor UDiff";
            di.lpszOutput = 0;
            di.lpszDatatype = 0;
            di.fwType = 0;
            if (::StartDoc(hdc, &di) < 0)
            {
                ::DeleteDC(hdc);
                return 0;
            }

            size_t lengthDoc = SendEditor(SCI_GETLENGTH);
            size_t lengthDocMax = lengthDoc;
            size_t lengthPrinted = 0;

            // Requested to print selection
            if (pdlg.Flags & PD_SELECTION)
            {
                if (startPos > endPos)
                {
                    lengthPrinted = endPos;
                    lengthDoc = startPos;
                }
                else
                {
                    lengthPrinted = startPos;
                    lengthDoc = endPos;
                }

                if (lengthDoc > lengthDocMax)
                    lengthDoc = lengthDocMax;
            }

            // We must subtract the physical margins from the printable area
            Sci_RangeToFormat frPrint;
            frPrint.hdc             = hdc;
            frPrint.hdcTarget       = hdc;
            frPrint.rc.left         = rectMargins.left - rectPhysMargins.left;
            frPrint.rc.top          = rectMargins.top - rectPhysMargins.top;
            frPrint.rc.right        = ptPage.x - rectMargins.right - rectPhysMargins.left;
            frPrint.rc.bottom       = ptPage.y - rectMargins.bottom - rectPhysMargins.top;
            frPrint.rcPage.left     = 0;
            frPrint.rcPage.top      = 0;
            frPrint.rcPage.right    = ptPage.x - rectPhysMargins.left - rectPhysMargins.right - 1;
            frPrint.rcPage.bottom   = ptPage.y - rectPhysMargins.top - rectPhysMargins.bottom - 1;

            // Print each page
            while (lengthPrinted < lengthDoc)
            {
                ::StartPage(hdc);

                frPrint.chrg.cpMin = (long)lengthPrinted;
                frPrint.chrg.cpMax = (long)lengthDoc;

                lengthPrinted = SendEditor(SCI_FORMATRANGE, true, reinterpret_cast<LPARAM>(&frPrint));

                ::EndPage(hdc);
            }

            SendEditor(SCI_FORMATRANGE, FALSE, 0);

            ::EndDoc(hdc);
            ::DeleteDC(hdc);

            if (pdlg.hDevMode != NULL)
                GlobalFree(pdlg.hDevMode);
            if (pdlg.hDevNames != NULL)
                GlobalFree(pdlg.hDevNames);
            if (pdlg.lpPageRanges != NULL)
                GlobalFree(pdlg.lpPageRanges);

            // reset the UI
            SendEditor(SCI_SETVIEWWS, viewws);
            SendEditor(SCI_SETEDGEMODE, edgemode);
            SendEditor(SCI_SETWRAPVISUALFLAGS, SC_WRAPVISUALFLAG_NONE);
        }
        break;
    default:
        break;
    };
    return 1;
}

LRESULT CDiffViewer::SendEditor(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (m_directFunction)
    {
        return ((SciFnDirect) m_directFunction)(m_directPointer, Msg, wParam, lParam);
    }
    return ::SendMessage(m_hWndEdit, Msg, wParam, lParam);
}

bool CDiffViewer::Initialize()
{
    CRegStdDWORD pos(_T("Software\\CommitMonitor\\DiffViewerPos"), 0);
    CRegStdDWORD width(_T("Software\\CommitMonitor\\DiffViewerWidth"), (DWORD)640);
    CRegStdDWORD height(_T("Software\\CommitMonitor\\DiffViewerHeight"), (DWORD)480);
    if (DWORD(pos) && DWORD(width) && DWORD(height))
    {
        RECT rc;
        rc.left = LOWORD(DWORD(pos));
        rc.top = HIWORD(DWORD(pos));
        rc.right = rc.left + DWORD(width);
        rc.bottom = rc.top + DWORD(height);
        HMONITOR hMon = MonitorFromRect(&rc, MONITOR_DEFAULTTONULL);
        if (hMon)
        {
            // only restore the window position if the monitor is valid
            MoveWindow(*this, LOWORD(DWORD(pos)), HIWORD(DWORD(pos)),
                DWORD(width), DWORD(height), FALSE);
        }
    }

    m_hWndEdit = ::CreateWindow(
        _T("Scintilla"),
        _T("Source"),
        WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        *this,
        0,
        hResource,
        0);
    if (m_hWndEdit == NULL)
        return false;

    RECT rect;
    GetClientRect(*this, &rect);
    ::SetWindowPos(m_hWndEdit, HWND_TOP,
        rect.left, rect.top,
        rect.right-rect.left, rect.bottom-rect.top,
        SWP_SHOWWINDOW);

    m_directFunction = SendMessage(m_hWndEdit, SCI_GETDIRECTFUNCTION, 0, 0);
    m_directPointer = SendMessage(m_hWndEdit, SCI_GETDIRECTPOINTER, 0, 0);

    // Set up the global default style. These attributes are used wherever no explicit choices are made.
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
    SendEditor(SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED);

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
        size_t lenFile = fread(data, 1, sizeof(data), fp);
        bool bUTF8 = IsUTF8(data, (int)lenFile);
        while (lenFile > 0)
        {
            SendEditor(SCI_ADDTEXT, lenFile,
                reinterpret_cast<LPARAM>(static_cast<char *>(data)));
            lenFile = fread(data, 1, sizeof(data), fp);
        }
        fclose(fp);
        SendEditor(SCI_SETCODEPAGE, bUTF8 ? SC_CP_UTF8 : GetACP());
    }
    else
    {
        return false;
    }

    SendEditor(SCI_SETUNDOCOLLECTION, 1);
    ::SetFocus(m_hWndEdit);
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
    SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)"revision");
    SendEditor(SCI_COLOURISE, 0, -1);
    ::ShowWindow(m_hWndEdit, SW_SHOW);
    return true;
}

void CDiffViewer::SetTitle(LPCTSTR title)
{
    size_t len = _tcslen(title);
    std::unique_ptr<TCHAR[]> pBuf(new TCHAR[len+40]);
    _stprintf_s(pBuf.get(), len+40, _T("%s - CMDiff"), title);
    SetWindowTitle(std::wstring(pBuf.get()));
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

bool CDiffViewer::IsUTF8(LPVOID pBuffer, int cb)
{
    if (cb < 2)
        return true;
    UINT16 * pVal = (UINT16 *)pBuffer;
    UINT8 * pVal2 = (UINT8 *)(pVal+1);
    // scan the whole buffer for a 0x0000 sequence
    // if found, we assume a binary file
    for (int i=0; i<(cb-2); i=i+2)
    {
        if (0x0000 == *pVal++)
            return false;
    }
    pVal = (UINT16 *)pBuffer;
    if (*pVal == 0xFEFF)
        return false;
    if (cb < 3)
        return false;
    if (*pVal == 0xBBEF)
    {
        if (*pVal2 == 0xBF)
            return true;
    }
    // check for illegal UTF8 chars
    pVal2 = (UINT8 *)pBuffer;
    for (int i=0; i<cb; ++i)
    {
        if ((*pVal2 == 0xC0)||(*pVal2 == 0xC1)||(*pVal2 >= 0xF5))
            return false;
        pVal2++;
    }
    pVal2 = (UINT8 *)pBuffer;
    bool bUTF8 = false;
    for (int i=0; i<(cb-3); ++i)
    {
        if ((*pVal2 & 0xE0)==0xC0)
        {
            pVal2++;i++;
            if ((*pVal2 & 0xC0)!=0x80)
                return false;
            bUTF8 = true;
        }
        if ((*pVal2 & 0xF0)==0xE0)
        {
            pVal2++;i++;
            if ((*pVal2 & 0xC0)!=0x80)
                return false;
            pVal2++;i++;
            if ((*pVal2 & 0xC0)!=0x80)
                return false;
            bUTF8 = true;
        }
        if ((*pVal2 & 0xF8)==0xF0)
        {
            pVal2++;i++;
            if ((*pVal2 & 0xC0)!=0x80)
                return false;
            pVal2++;i++;
            if ((*pVal2 & 0xC0)!=0x80)
                return false;
            pVal2++;i++;
            if ((*pVal2 & 0xC0)!=0x80)
                return false;
            bUTF8 = true;
        }
        pVal2++;
    }
    if (bUTF8)
        return true;
    return false;
}
