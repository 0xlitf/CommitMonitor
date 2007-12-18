// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007 - Stefan Kueng

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
#pragma once
#include <vector>
#include <map>

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
#include "SerializeUtils.h"
#include "ProgressDlg.h"

typedef std::wstring wide_string;
#ifndef stdstring
#	ifdef UNICODE
#		define stdstring wide_string
#	else
#		define stdstring std::string
#	endif
#endif

#include <string>

using namespace std;

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

class SVNLogChangedPaths
{
public:
	SVNLogChangedPaths()
		: action(0)
	{

	}

	wchar_t				action;
	svn_revnum_t		copyfrom_revision;
	stdstring			copyfrom_path;

	bool Save(FILE * hFile) const
	{
		if (!CSerializeUtils::SaveNumber(hFile, action))
			return false;
		if (!CSerializeUtils::SaveNumber(hFile, copyfrom_revision))
			return false;
		if (!CSerializeUtils::SaveString(hFile, copyfrom_path))
			return false;
		return true;
	}
	bool Load(FILE * hFile)
	{
		unsigned __int64 value;
		if (!CSerializeUtils::LoadNumber(hFile, value))
			return false;
		action = (wchar_t)value;
		if (!CSerializeUtils::LoadNumber(hFile, value))
			return false;
		copyfrom_revision = (svn_revnum_t)value;
		if (!CSerializeUtils::LoadString(hFile, copyfrom_path))
			return false;
		return true;
	}
	bool Load(const unsigned char *& buf)
	{
		unsigned __int64 value;
		if (!CSerializeUtils::LoadNumber(buf, value))
			return false;
		action = (wchar_t)value;
		if (!CSerializeUtils::LoadNumber(buf, value))
			return false;
		copyfrom_revision = (svn_revnum_t)value;
		if (!CSerializeUtils::LoadString(buf, copyfrom_path))
			return false;
		return true;
	}
};

class SVNLogEntry
{
public:
	SVNLogEntry() 
		: read(false)
		, revision(0)
		, date(0)
	{

	}

	bool				read;
	svn_revnum_t		revision;
	stdstring			author;
	apr_time_t			date;
	stdstring			message;
	map<stdstring, SVNLogChangedPaths>	m_changedPaths;

	bool Save(FILE * hFile) const
	{
		if (!CSerializeUtils::SaveNumber(hFile, read))
			return false;
		if (!CSerializeUtils::SaveNumber(hFile, revision))
			return false;
		if (!CSerializeUtils::SaveString(hFile, author))
			return false;
		if (!CSerializeUtils::SaveNumber(hFile, date))
			return false;
		if (!CSerializeUtils::SaveString(hFile, message))
			return false;

		if (!CSerializeUtils::SaveNumber(hFile, CSerializeUtils::SerializeType_Map))
			return false;
		if (!CSerializeUtils::SaveNumber(hFile, m_changedPaths.size()))
			return false;
		for (map<stdstring,SVNLogChangedPaths>::const_iterator it = m_changedPaths.begin(); it != m_changedPaths.end(); ++it)
		{
			if (!CSerializeUtils::SaveString(hFile, it->first))
				return false;
			if (!it->second.Save(hFile))
				return false;
		}
		return true;
	}
	bool Load(FILE * hFile)
	{
		unsigned __int64 value = 0;
		if (!CSerializeUtils::LoadNumber(hFile, value))
			return false;
		read = !!value;
		if (!CSerializeUtils::LoadNumber(hFile, value))
			return false;
		revision = (svn_revnum_t)value;
		if (!CSerializeUtils::LoadString(hFile, author))
			return false;
		if (!CSerializeUtils::LoadNumber(hFile, value))
			return false;
		date = value;
		if (!CSerializeUtils::LoadString(hFile, message))
			return false;

		m_changedPaths.clear();
		if (!CSerializeUtils::LoadNumber(hFile, value))
			return false;
		if (CSerializeUtils::SerializeType_Map == value)
		{
			if (CSerializeUtils::LoadNumber(hFile, value))
			{
				for (unsigned __int64 i=0; i<value; ++i)
				{
					wstring key;
					SVNLogChangedPaths cpaths;
					if (!CSerializeUtils::LoadString(hFile, key))
						return false;
					if (!cpaths.Load(hFile))
						return false;
					m_changedPaths[key] = cpaths;
				}
				return true;
			}
		}
		return false;
	}

	bool Load(const unsigned char *& buf)
	{
		unsigned __int64 value = 0;
		if (!CSerializeUtils::LoadNumber(buf, value))
			return false;
		read = !!value;
		if (!CSerializeUtils::LoadNumber(buf, value))
			return false;
		revision = (svn_revnum_t)value;
		if (!CSerializeUtils::LoadString(buf, author))
			return false;
		if (!CSerializeUtils::LoadNumber(buf, value))
			return false;
		date = value;
		if (!CSerializeUtils::LoadString(buf, message))
			return false;

		m_changedPaths.clear();
		if (!CSerializeUtils::LoadNumber(buf, value))
			return false;
		if (CSerializeUtils::SerializeType_Map == value)
		{
			if (CSerializeUtils::LoadNumber(buf, value))
			{
				for (unsigned __int64 i=0; i<value; ++i)
				{
					wstring key;
					SVNLogChangedPaths cpaths;
					if (!CSerializeUtils::LoadString(buf, key))
						return false;
					if (!cpaths.Load(buf))
						return false;
					m_changedPaths[key] = cpaths;
				}
				return true;
			}
		}
		return false;
	}

};

class SVN
{
public:
	SVN(void);
	~SVN(void);

	void SetAuthInfo(const stdstring& username, const stdstring& password);

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
	 * Returns the info of the next file in the file list. If no more files are in the list then NULL is returned.
	 * See GetFirstFileInfo() for details.
	 */
	const SVNInfoData * GetNextFileInfo();

	svn_revnum_t GetHEADRevision(const stdstring& url);

	bool GetLog(const stdstring& url, svn_revnum_t startrev, svn_revnum_t endrev);
	map<svn_revnum_t,SVNLogEntry> m_logs;		///< contains the gathered log information

	bool Diff(const wstring& url1, svn_revnum_t pegrevision, svn_revnum_t revision1,
		svn_revnum_t revision2, bool ignoreancestry, bool nodiffdeleted, 
		bool ignorecontenttype,  const wstring& options, bool bAppend, 
		const wstring& outputfile, const wstring& errorfile);


	wstring CanonicalizeURL(const wstring& url);
	wstring GetLastErrorMsg();

	/**
	 * Sets and clears the progress info which is shown during lengthy operations.
	 * \param pProgressDlg the CProgressDlg object to show the progress info on.
	 * \param bShowProgressBar set to true if the progress bar should be shown. Only makes
	 * sense if the total amount of the progress is known beforehand. Otherwise the
	 * progressbar is always "empty".
	 */
	void SetAndClearProgressInfo(CProgressDlg * pProgressDlg, bool bShowProgressBar = false);

	struct SVNProgress
	{
		apr_off_t progress;			///< operation progress
		apr_off_t total;			///< operation progress
		apr_off_t overall_total;	///< total bytes transferred, use SetAndClearProgressInfo() to reset this
		apr_off_t BytesPerSecond;	///< Speed in bytes per second
		wstring	  SpeedString;		///< String for speed. Either "xxx Bytes/s" or "xxx kBytes/s"
	};

	bool						m_bCanceled;
	svn_error_t *				Err;			///< Global error object struct
private:
	apr_pool_t *				parentpool;		///< the main memory pool
	apr_pool_t *				pool;			///< 'root' memory pool
	svn_client_ctx_t * 			m_pctx;			///< pointer to client context
	svn_auth_baton_t *			auth_baton;

	vector<SVNInfoData>			m_arInfo;		///< contains all gathered info structs.
	unsigned int				m_pos;			///< the current position of the vector.

	SVNProgress					m_SVNProgressMSG;
	HWND						m_progressWnd;
	CProgressDlg *				m_pProgressDlg;
	bool						m_progressWndIsCProgress;
	bool						m_bShowProgressBar;
	apr_off_t					progress_total;
	apr_off_t					progress_averagehelper;
	apr_off_t					progress_lastprogress;
	apr_off_t					progress_lasttotal;
	DWORD						progress_lastTicks;
	std::vector<apr_off_t>		progress_vector;

private:
	static svn_error_t *		cancel(void *baton);
	static svn_error_t *		infoReceiver(void* baton, const char * path, 
											const svn_info_t* info, apr_pool_t * pool);
	static svn_error_t *		logReceiver(void* baton, apr_hash_t* ch_paths, 
											svn_revnum_t rev, const char* author, 
											const char* date, const char* msg, apr_pool_t* pool);
	static svn_error_t*			sslserverprompt(svn_auth_cred_ssl_server_trust_t **cred_p, 
											void *baton, const char *realm, 
											apr_uint32_t failures, 
											const svn_auth_ssl_server_cert_info_t *cert_info, 
											svn_boolean_t may_save, apr_pool_t *pool);
	static void					progress_func(apr_off_t progress, apr_off_t total, 
											void *baton, apr_pool_t *pool);

};


