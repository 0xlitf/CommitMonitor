// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007, 2009-2011 - Stefan Kueng

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

#include "SCCS.h"
/*
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
*/

#include <string>

using namespace std;


class SVN : public SCCS
{
public:
    SVN(void);
    ~SVN(void);

    void SetAuthInfo(const std::wstring& username, const std::wstring& password);

    bool GetFile(std::wstring sUrl, std::wstring sFile);

    /**
     * returns the info for the \a path.
     * \param path a path or an url
     * \param pegrev the peg revision to use
     * \param revision the revision to get the info for
     * \param recurse if TRUE, then GetNextFileInfo() returns the info also
     * for all children of \a path.
     */
    wstring GetRootUrl(const std::wstring& path);
    size_t GetFileCount() {return m_arInfo.size();}

    svn_revnum_t GetHEADRevision(const std::wstring& repo, const std::wstring& url);

    bool GetLog(const std::wstring& repo, const std::wstring& url, svn_revnum_t startrev, svn_revnum_t endrev);
    //map<svn_revnum_t,SVNLogEntry> m_logs;       ///< contains the gathered log information

    bool Diff(const wstring& url1, svn_revnum_t pegrevision, svn_revnum_t revision1,
        svn_revnum_t revision2, bool ignoreancestry, bool nodiffdeleted,
        bool ignorecontenttype,  const wstring& options, bool bAppend,
        const wstring& outputfile, const wstring& errorfile);

    static wstring GetOptionsString(bool bIgnoreEOL, bool bIgnoreSpaces, bool bIgnoreAllSpaces);

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

private:
    apr_pool_t *                parentpool;     ///< the main memory pool
    apr_pool_t *                pool;           ///< 'root' memory pool
    svn_client_ctx_t *          m_pctx;         ///< pointer to client context
    svn_auth_baton_t *          auth_baton;

    vector<SCCSInfoData>         m_arInfo;       ///< contains all gathered info structs.
    unsigned int                m_pos;          ///< the current position of the vector.

    SVNProgress                 m_SVNProgressMSG;
    HWND                        m_progressWnd;
    CProgressDlg *              m_pProgressDlg;
    bool                        m_progressWndIsCProgress;
    bool                        m_bShowProgressBar;
    apr_off_t                   progress_total;
    apr_off_t                   progress_averagehelper;
    apr_off_t                   progress_lastprogress;
    apr_off_t                   progress_lasttotal;
    DWORD                       progress_lastTicks;
    std::vector<apr_off_t>      progress_vector;
    std::wstring                password;

private:
    static svn_error_t *        cancel(void *baton);
    static svn_error_t *        infoReceiver(void* baton, const char * path,
                                            const svn_info_t* info, apr_pool_t * pool);
    static svn_error_t *        logReceiver(void* baton, apr_hash_t* ch_paths,
                                            svn_revnum_t rev, const char* author,
                                            const char* date, const char* msg, apr_pool_t* pool);
    static svn_error_t*         sslserverprompt(svn_auth_cred_ssl_server_trust_t **cred_p,
                                            void *baton, const char *realm,
                                            apr_uint32_t failures,
                                            const svn_auth_ssl_server_cert_info_t *cert_info,
                                            svn_boolean_t may_save, apr_pool_t *pool);
    static svn_error_t*         sslclientprompt(svn_auth_cred_ssl_client_cert_t **cred,
                                            void *baton, const char * realm,
                                            svn_boolean_t may_save, apr_pool_t *pool);
    static svn_error_t*         sslpwprompt(svn_auth_cred_ssl_client_cert_pw_t **cred,
                                            void *baton, const char * realm,
                                            svn_boolean_t may_save, apr_pool_t *pool);
    static svn_error_t*         svn_auth_plaintext_prompt(svn_boolean_t *may_save_plaintext,
                                            const char *realmstring, void *baton,
                                            apr_pool_t *pool);
    static svn_error_t*         svn_auth_plaintext_passphrase_prompt(svn_boolean_t *may_save_plaintext,
                                            const char *realmstring, void *baton,
                                            apr_pool_t *pool);
    static void                 progress_func(apr_off_t progress, apr_off_t total,
                                            void *baton, apr_pool_t *pool);

};
