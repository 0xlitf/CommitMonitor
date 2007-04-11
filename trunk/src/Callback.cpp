#include "StdAfx.h"
#include "Callback.h"

#include <urlmon.h>
#include <shlwapi.h>                    // for StrFormatByteSize()

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCallback::CCallback()
{
	m_cRef = 0;
}

CCallback::~CCallback()
{
}

STDMETHODIMP CCallback::Authenticate( HWND * phwnd, LPWSTR * pszUsername, LPWSTR * pszPassword)
{
	*phwnd = NULL;
	*pszUsername = (LPWSTR)CoTaskMemAlloc((m_username.size()+1)*2);
	_tcscpy_s(*pszUsername, m_username.size()+1, m_username.c_str());
	*pszPassword = (LPWSTR)CoTaskMemAlloc((m_password.size()+1)*2);
	_tcscpy_s(*pszPassword, m_password.size()+1, m_password.c_str());
	return S_OK;
}

