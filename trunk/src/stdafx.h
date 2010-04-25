// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER                  // Specifies that the minimum required platform is Windows Vista.
#define WINVER 0x0600           // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINDOWS          // Specifies that the minimum required platform is Windows 98.
#define _WIN32_WINDOWS 0x0410   // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE               // Specifies that the minimum required platform is Internet Explorer 7.0.
#define _WIN32_IE 0x0700        // Change this to the appropriate value to target other versions of IE.
#endif

#ifndef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_LONGHORN
#endif

// Windows Header Files:
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>
#include <windows.h>
#include <windowsx.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <commctrl.h>
#include <shlwapi.h>

#include "Debug.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

#ifndef WIN64
#   pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#   ifdef _DEBUG
#       pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.VC80.CRT' version='8.0.50608.0' processorArchitecture='X86' publicKeyToken='1fc8b3b9a1e18e3b' language='*'\"")
#   endif
#else
#   pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif


#define COMMITMONITOR_POPUP             (WM_APP+1)
#define COMMITMONITOR_CHANGEDINFO       (WM_APP+2)
#define COMMITMONITOR_TASKBARCALLBACK   (WM_APP+3)
#define COMMITMONITOR_REMOVEDURL        (WM_APP+4)
#define COMMITMONITOR_SAVEINFO          (WM_APP+5)
#define COMMITMONITOR_FINDMSGPREV       (WM_APP+6)
#define COMMITMONITOR_FINDMSGNEXT       (WM_APP+7)
#define COMMITMONITOR_FINDEXIT          (WM_APP+8)
#define COMMITMONITOR_FINDRESET         (WM_APP+9)
#define COMMITMONITOR_SETWINDOWHANDLE   (WM_APP+10)
#define COMMITMONITOR_INFOTEXT          (WM_APP+11)
#define COMMITMONITOR_GETALL            (WM_APP+12)
#define COMMITMONITOR_POPUPCLICK        (WM_APP+13)
#define COMMITMONITOR_LISTCTRLDBLCLICK  (WM_APP+14)
