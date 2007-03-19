#pragma once
#include <vector>

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

#include "SVNPool.h"
#include "UnicodeUtils.h"
#include "Registry.h"

typedef std::wstring wide_string;
#ifndef stdstring
#	ifdef UNICODE
#		define stdstring wide_string
#	else
#		define stdstring std::string
#	endif
#endif

class SVNInfoData
{
public:
	SVNInfoData(){}

	stdstring			url;
	svn_revnum_t		rev;
	svn_node_kind_t		kind;
	stdstring			reposRoot;
	stdstring			reposUUID;
	svn_revnum_t		lastchangedrev;
	__time64_t			lastchangedtime;
	stdstring			author;

	stdstring			lock_path;
	stdstring			lock_token;
	stdstring			lock_owner;
	stdstring			lock_comment;
	bool				lock_davcomment;
	__time64_t			lock_createtime;
	__time64_t			lock_expirationtime;

	bool				hasWCInfo;
	svn_wc_schedule_t	schedule;
	stdstring			copyfromurl;
	svn_revnum_t		copyfromrev;
	__time64_t			texttime;
	__time64_t			proptime;
	stdstring			checksum;
	stdstring			conflict_old;
	stdstring			conflict_new;
	stdstring			conflict_wrk;
	stdstring			prejfile;
};


class SVN
{
public:
	SVN(void);
	~SVN(void);

	bool Cat(stdstring sUrl, stdstring sFile);

	/**
	 * returns the info for the \a path.
	 * \param path a path or an url
	 * \param pegrev the peg revision to use
	 * \param revision the revision to get the info for
	 * \param recurse if TRUE, then GetNextFileInfo() returns the info also
	 * for all children of \a path.
	 */
	const SVNInfoData * GetFirstFileInfo(stdstring path, svn_revnum_t pegrev, svn_revnum_t revision, bool recurse = false);
	size_t GetFileCount() {return m_arInfo.size();}
	/**
	 * Returns the info of the next file in the filelist. If no more files are in the list then NULL is returned.
	 * See GetFirstFileInfo() for details.
	 */
	const SVNInfoData * GetNextFileInfo();

private:
	apr_pool_t *				parentpool;		///< the main memory pool
	apr_pool_t *				pool;			///< 'root' memory pool
	svn_client_ctx_t * 			m_pctx;			///< pointer to client context
	svn_error_t *				Err;			///< Global error object struct
	svn_auth_baton_t *			auth_baton;

	std::vector<SVNInfoData>	m_arInfo;		///< contains all gathered info structs.
	unsigned int				m_pos;			///< the current position of the vector
	static svn_error_t *		infoReceiver(void* baton, const char * path, const svn_info_t* info, apr_pool_t * pool);
};


