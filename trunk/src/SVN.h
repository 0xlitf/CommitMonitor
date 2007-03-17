#pragma once
#include "apr_general.h"
#include "svn_pools.h"
#include "svn_client.h"
#include "svn_path.h"
#include "svn_wc.h"
#include "svn_utf.h"
#include "svn_config.h"
#include "svn_error_codes.h"
#include "svn_subst.h"
#include "svn_repos.h"
#include "svn_time.h"


class SVN
{
public:
	SVN(void);
	~SVN(void);

private:
	apr_pool_t *				parentpool;		///< the main memory pool
	apr_pool_t *				pool;			///< 'root' memory pool
	svn_client_ctx_t * 			m_pctx;			///< pointer to client context
	svn_error_t *				Err;			///< Global error object struct
};


