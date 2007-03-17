#include "StdAfx.h"
#include "svn.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

SVN::SVN(void)
{
	parentpool = svn_pool_create(NULL);
	svn_client_create_context(&m_pctx, parentpool);

	Err = svn_config_ensure(NULL, parentpool);
	pool = svn_pool_create (parentpool);
	// set up the configuration
	//if (Err == 0)
	//	Err = svn_config_get_config (&(m_pctx->config), g_pConfigDir, parentpool);

	if (Err != 0)
	{
		//::MessageBox(NULL, this->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		svn_pool_destroy (pool);
		svn_pool_destroy (parentpool);
		exit(-1);
	}


	//set up the SVN_SSH param
	//CString tsvn_ssh = CRegString(_T("Software\\TortoiseSVN\\SSH"));
	//if (tsvn_ssh.IsEmpty())
	//	tsvn_ssh = CPathUtils::GetAppDirectory() + _T("TortoisePlink.exe");
	//tsvn_ssh.Replace('\\', '/');
	//if (!tsvn_ssh.IsEmpty())
	//{
	//	svn_config_t * cfg = (svn_config_t *)apr_hash_get (m_pctx->config, SVN_CONFIG_CATEGORY_CONFIG,
	//		APR_HASH_KEY_STRING);
	//	svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::GetUTF8(tsvn_ssh));
	//}
}

SVN::~SVN(void)
{
	svn_pool_destroy (parentpool);
}

