// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007-2011 - Stefan Kueng

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
#include "StdAfx.h"
#pragma warning(push)
#include "svn.h"
#include "svn_sorts.h"
#pragma warning(pop)

#include "AppUtils.h"
#include "version.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


SVN::SVN(void)
{
    parentpool = svn_pool_create(NULL);
    svn_error_clear(svn_client_create_context(&m_pctx, parentpool));

    Err = svn_config_ensure(NULL, parentpool);
    pool = svn_pool_create (parentpool);
    // set up the configuration
    if (Err == 0)
        Err = svn_config_get_config (&(m_pctx->config), NULL, parentpool);

    if (Err == 0)
    {
        //set up the SVN_SSH param
        wstring tsvn_ssh = (LPCTSTR)CRegStdString(_T("Software\\TortoiseSVN\\SSH"));
        if (!tsvn_ssh.empty())
        {
            svn_config_t * cfg = (svn_config_t *)apr_hash_get (m_pctx->config, SVN_CONFIG_CATEGORY_CONFIG,
                APR_HASH_KEY_STRING);
            svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::StdGetUTF8(tsvn_ssh).c_str());
        }
    }
    else
        svn_error_clear(Err);
    Err = svn_ra_initialize(parentpool);

    // set up authentication

    svn_auth_provider_object_t *provider;

    /* The whole list of registered providers */
    apr_array_header_t *providers = apr_array_make (pool, 10, sizeof (svn_auth_provider_object_t *));

    svn_config_t * cfg_config = (svn_config_t *)apr_hash_get(m_pctx->config, SVN_CONFIG_CATEGORY_CONFIG, APR_HASH_KEY_STRING);

    /* Populate the registered providers with the platform-specific providers */
    svn_auth_get_platform_specific_client_providers(&providers, cfg_config, pool);

    /* The main disk-caching auth providers, for both
    'username/password' creds and 'username' creds.  */
    svn_auth_get_simple_provider2 (&provider, svn_auth_plaintext_prompt, this, pool);
    APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
    svn_auth_get_username_provider (&provider, pool);
    APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

    /* The server-cert, client-cert, and client-cert-password providers. */
    svn_auth_get_platform_specific_provider (&provider, "windows", "ssl_server_trust", pool);
    if (provider)
        APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;
    svn_auth_get_ssl_server_trust_file_provider (&provider, pool);
    APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
    svn_auth_get_ssl_client_cert_file_provider (&provider, pool);
    APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
    svn_auth_get_ssl_client_cert_pw_file_provider2 (&provider, svn_auth_plaintext_passphrase_prompt, this, pool);
    APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

    /* Two prompting providers, one for username/password, one for
    just username. */
    //svn_auth_get_simple_prompt_provider (&provider, (svn_auth_simple_prompt_func_t)simpleprompt, this, 3, /* retry limit */ pool);
    //APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
    //svn_auth_get_username_prompt_provider (&provider, (svn_auth_username_prompt_func_t)userprompt, this, 3, /* retry limit */ pool);
    //APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

    /* Three prompting providers for server-certs, client-certs,
    and client-cert-passphrases.  */
    svn_auth_get_ssl_server_trust_prompt_provider (&provider, sslserverprompt, this, pool);
    APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
    svn_auth_get_ssl_client_cert_prompt_provider (&provider, sslclientprompt, this, 2, pool);
    APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
    svn_auth_get_ssl_client_cert_pw_prompt_provider (&provider, sslpwprompt, this, 2, pool);
    APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

    /* Build an authentication baton to give to libsvn_client. */
    svn_auth_open (&auth_baton, providers, pool);
    m_pctx->auth_baton = auth_baton;

    svn_auth_set_parameter(auth_baton, SVN_AUTH_PARAM_NON_INTERACTIVE, "");
    svn_auth_set_parameter(auth_baton, SVN_AUTH_PARAM_DONT_STORE_PASSWORDS, "");
    svn_auth_set_parameter(auth_baton, SVN_AUTH_PARAM_NO_AUTH_CACHE, "");

    m_pctx->cancel_func = cancel;
    m_pctx->cancel_baton = this;
    m_pctx->progress_func = progress_func;
    m_pctx->progress_baton = this;
    // create a client name string to be used for all http/https connections
    char namestring[MAX_PATH] = {0};
    sprintf_s(namestring, MAX_PATH, "CommitMonitor-%d.%d.%d.%d", CM_VERMAJOR, CM_VERMINOR, CM_VERMICRO, CM_VERBUILD);
    m_pctx->client_name = apr_pstrdup(pool, namestring);

    //set up the SVN_SSH param
    wstring tsvn_ssh = (LPCTSTR)CRegStdString(_T("Software\\TortoiseSVN\\SSH"));
    if (tsvn_ssh.empty())
    {
        // maybe the user has TortoiseSVN installed?
        // if so, try to use TortoisePlink with the default params for SSH
        tsvn_ssh = (LPCTSTR)CRegStdString(_T("Software\\TortoiseSVN\\Directory"), _T(""), false, HKEY_LOCAL_MACHINE);
        if (!tsvn_ssh.empty())
        {
            tsvn_ssh += _T("\\bin\\TortoisePlink.exe");
        }
    }
    CAppUtils::SearchReplace(tsvn_ssh, _T("\\"), _T("/"));
    if (!tsvn_ssh.empty())
    {
        svn_config_t * cfg = (svn_config_t *)apr_hash_get (m_pctx->config, SVN_CONFIG_CATEGORY_CONFIG,
            APR_HASH_KEY_STRING);
        svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::StdGetUTF8(tsvn_ssh).c_str());
    }
    m_bCanceled = false;
    m_pProgressDlg = NULL;
}

SVN::~SVN(void)
{
    svn_error_clear(Err);
    svn_pool_destroy (parentpool);
}

svn_error_t* SVN::cancel(void *baton)
{
    SVN * pSVN = (SVN*)baton;
    if (pSVN->m_bCanceled)
    {
        return svn_error_create(SVN_ERR_CANCELLED, NULL, "user canceled");
    }
    return SVN_NO_ERROR;
}

svn_error_t* SVN::sslserverprompt(svn_auth_cred_ssl_server_trust_t **cred_p, void * /*baton*/, 
    const char * /*realm*/, apr_uint32_t /*failures*/, 
    const svn_auth_ssl_server_cert_info_t * /*cert_info*/, 
    svn_boolean_t /*may_save*/, apr_pool_t *pool)
{
    *cred_p = (svn_auth_cred_ssl_server_trust_t*)apr_pcalloc (pool, sizeof (**cred_p));
    (*cred_p)->may_save = FALSE;
    return SVN_NO_ERROR;
}

svn_error_t* SVN::svn_auth_plaintext_prompt(svn_boolean_t *may_save_plaintext, const char * /*realmstring*/, void * /*baton*/, apr_pool_t * /*pool*/)
{
    // never save passwords in plaintext
    // we never save passwords in CommitMonitor anyway, since we have the auth info
    // stored in the monitored-url info.
    // So we shouldn't even get here.
    *may_save_plaintext = false;
    return SVN_NO_ERROR;
}

svn_error_t* SVN::svn_auth_plaintext_passphrase_prompt(svn_boolean_t *may_save_plaintext, const char * /*realmstring*/, void * /*baton*/, apr_pool_t * /*pool*/)
{
    // never save passwords in plaintext
    // we never save passwords in CommitMonitor anyway, since we have the auth info
    // stored in the monitored-url info.
    // So we shouldn't even get here.
    *may_save_plaintext = false;
    return SVN_NO_ERROR;
}

svn_error_t* SVN::sslclientprompt(svn_auth_cred_ssl_client_cert_t **cred, void * /*baton*/, const char * /*realm*/, svn_boolean_t /*may_save*/, apr_pool_t * /*pool*/)
{
    *cred = NULL;
    return SVN_NO_ERROR;
}

svn_error_t* SVN::sslpwprompt(svn_auth_cred_ssl_client_cert_pw_t **cred, void *baton, const char * /*realm*/, svn_boolean_t may_save, apr_pool_t *pool)
{
    SVN* svn = (SVN *)baton;
    svn_auth_cred_ssl_client_cert_pw_t *ret = (svn_auth_cred_ssl_client_cert_pw_t *)apr_pcalloc (pool, sizeof (*ret));

    // return the password from the auth info
    ret->password = apr_pstrdup(pool, CUnicodeUtils::StdGetUTF8(svn->password).c_str());
    ret->may_save = may_save;
    *cred = ret;
    return SVN_NO_ERROR;
}

wstring SVN::GetLastErrorMsg()
{
    wstring msg;
    char errbuf[256];

    if (Err != NULL)
    {
        svn_error_t * ErrPtr = Err;
        if (ErrPtr->message)
        {
            msg = CUnicodeUtils::StdGetUnicode(ErrPtr->message);
        }
        else
        {
            /* Is this a Subversion-specific error code? */
            if ((ErrPtr->apr_err > APR_OS_START_USEERR)
                && (ErrPtr->apr_err <= APR_OS_START_CANONERR))
                msg = CUnicodeUtils::StdGetUnicode(svn_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)));
            /* Otherwise, this must be an APR error code. */
            else
            {
                svn_error_t *temp_err = NULL;
                const char * err_string = NULL;
                temp_err = svn_utf_cstring_to_utf8(&err_string, apr_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)-1), ErrPtr->pool);
                if (temp_err)
                {
                    svn_error_clear (temp_err);
                    msg = _T("Can't recode error string from APR");
                }
                else
                {
                    msg = CUnicodeUtils::StdGetUnicode(err_string);
                }
            }

        }

        while (ErrPtr->child)
        {
            ErrPtr = ErrPtr->child;
            msg += _T("\n");
            if (ErrPtr->message)
            {
                msg += CUnicodeUtils::StdGetUnicode(ErrPtr->message);
            }
            else
            {
                /* Is this a Subversion-specific error code? */
                if ((ErrPtr->apr_err > APR_OS_START_USEERR)
                    && (ErrPtr->apr_err <= APR_OS_START_CANONERR))
                    msg += CUnicodeUtils::StdGetUnicode(svn_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)));
                /* Otherwise, this must be an APR error code. */
                else
                {
                    svn_error_t *temp_err = NULL;
                    const char * err_string = NULL;
                    temp_err = svn_utf_cstring_to_utf8(&err_string, apr_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)-1), ErrPtr->pool);
                    if (temp_err)
                    {
                        svn_error_clear (temp_err);
                        msg += _T("Can't recode error string from APR");
                    }
                    else
                    {
                        msg += CUnicodeUtils::StdGetUnicode(err_string);
                    }
                }

            }
        }
        return msg;
    } // if (Err != NULL)
    return msg;
}

void SVN::SetAuthInfo(const wstring& username, const wstring& password)
{
    this->password = password;
    if (m_pctx)
    {
        if (!username.empty())
        {
            svn_auth_set_parameter(m_pctx->auth_baton, 
                SVN_AUTH_PARAM_DEFAULT_USERNAME, apr_pstrdup(parentpool, CUnicodeUtils::StdGetUTF8(username).c_str()));
            svn_auth_set_parameter(m_pctx->auth_baton, 
                SVN_AUTH_PARAM_DEFAULT_PASSWORD, apr_pstrdup(parentpool, CUnicodeUtils::StdGetUTF8(password).c_str()));
        }
    }
}

bool SVN::GetFile(wstring sUrl, wstring sFile)
{
    svn_error_clear(Err);
    m_bCanceled = false;
    // we always use the HEAD revision to fetch a file
    apr_file_t * file;
    svn_stream_t * stream;
    apr_status_t status;
    SVNPool localpool(pool);

    // if the file already exists, delete it before recreating it
    ::DeleteFile(sFile.c_str());

    status = apr_file_open(&file, CUnicodeUtils::StdGetANSI(sFile).c_str(), 
        APR_WRITE | APR_CREATE | APR_TRUNCATE, APR_OS_DEFAULT, localpool);
    if (status)
    {
        Err = svn_error_wrap_apr(status, NULL);
        return false;
    }
    stream = svn_stream_from_aprfile2(file, true, localpool);

    svn_opt_revision_t pegrev, rev;
    pegrev.kind = svn_opt_revision_head;
    rev.kind = svn_opt_revision_head;

    const char * urla = svn_uri_canonicalize (CAppUtils::PathEscape(CUnicodeUtils::StdGetUTF8(sUrl)).c_str(), localpool);
    Err = svn_client_cat2(stream, urla, 
        &pegrev, &rev, m_pctx, localpool);

    apr_file_close(file);

    return (Err == NULL);
}

wstring SVN::GetRootUrl(const wstring& path)
{
    svn_error_clear(Err);
    m_bCanceled = false;
    SVNPool localpool(pool);
    m_arInfo.clear();
    m_pos = 0;

    svn_opt_revision_t peg, rev;
    peg.kind = svn_opt_revision_head;
    rev.kind = svn_opt_revision_head;

    const char * urla = svn_uri_canonicalize (CAppUtils::PathEscape(CUnicodeUtils::StdGetUTF8(path)).c_str(), localpool);

    Err = svn_client_info3 (urla, &peg, &rev, svn_depth_empty, false, false, NULL, infoReceiver, this, m_pctx, localpool);
    if (Err != NULL)
        return NULL;
    if (m_arInfo.size() == 0)
        return NULL;

    return m_arInfo[0].reposRoot;
}

svn_error_t * SVN::infoReceiver(void* baton, const char * path, const svn_client_info2_t* info, apr_pool_t * /*pool*/)
{
    if ((path == NULL)||(info == NULL))
        return NULL;

    SVN * pThis = (SVN *)baton;

    SCCSInfoData data;
    if (info->URL)
        data.url = CUnicodeUtils::StdGetUnicode(info->URL);
    data.rev = info->rev;
    data.kind = info->kind;
    if (info->repos_root_URL)
        data.reposRoot = CUnicodeUtils::StdGetUnicode(info->repos_root_URL);
    if (info->repos_UUID)
        data.reposUUID = CUnicodeUtils::StdGetUnicode(info->repos_UUID);
    data.lastchangedrev = info->last_changed_rev;
    data.lastchangedtime = info->last_changed_date/1000000L;
    if (info->last_changed_author)
        data.author = CUnicodeUtils::StdGetUnicode(info->last_changed_author);

    if (info->lock)
    {
        if (info->lock->path)
            data.lock_path = CUnicodeUtils::StdGetUnicode(info->lock->path);
        if (info->lock->token)
            data.lock_token = CUnicodeUtils::StdGetUnicode(info->lock->token);
        if (info->lock->owner)
            data.lock_owner = CUnicodeUtils::StdGetUnicode(info->lock->owner);
        if (info->lock->comment)
            data.lock_comment = CUnicodeUtils::StdGetUnicode(info->lock->comment);
        data.lock_davcomment = !!info->lock->is_dav_comment;
        data.lock_createtime = info->lock->creation_date/1000000L;
        data.lock_expirationtime = info->lock->expiration_date/1000000L;
    }

    data.hasWCInfo = info->wc_info != NULL;
    if (data.hasWCInfo)
    {
        data.schedule = info->wc_info->schedule;
        if (info->wc_info->copyfrom_url)
            data.copyfromurl = CUnicodeUtils::StdGetUnicode(info->wc_info->copyfrom_url);
        data.copyfromrev = info->wc_info->copyfrom_rev;
    }
    pThis->m_arInfo.push_back(data);
    return NULL;
}

svn_revnum_t SVN::GetHEADRevision(const wstring& repo, const wstring& url)
{
    UNREFERENCED_PARAMETER(repo);

    svn_error_clear(Err);
    m_bCanceled = false;
    svn_ra_session_t *ra_session = NULL;
    SVNPool localpool(pool);
    svn_revnum_t rev = 0;

    // make sure the url is canonical.
    const char * urla = svn_uri_canonicalize (CAppUtils::PathEscape(CUnicodeUtils::StdGetUTF8(url)).c_str(), localpool);

    if (urla == NULL)
        return rev;

    Err = svn_client_open_ra_session (&ra_session, urla, m_pctx, localpool);
    if (Err)
        return rev;

    Err = svn_ra_get_latest_revnum(ra_session, &rev, localpool);

    return rev;
}

bool SVN::GetLog(const wstring& repo, const wstring& url, svn_revnum_t startrev, svn_revnum_t endrev)
{
    UNREFERENCED_PARAMETER(repo);

    svn_error_clear(Err);
    m_bCanceled = false;
    SVNPool localpool(pool);

    apr_array_header_t *targets = apr_array_make (pool, 1, sizeof(const char *));
    (*((const char **) apr_array_push (targets))) = 
        svn_uri_canonicalize (CAppUtils::PathEscape(CUnicodeUtils::StdGetUTF8(url)).c_str(), localpool);

    svn_opt_revision_t end;
    end.kind = svn_opt_revision_number;
    end.value.number = endrev;

    svn_opt_revision_t start;
    start.kind = svn_opt_revision_number;
    start.value.number = startrev;

    m_logs.clear();

    int limit = 0;
    CRegStdDWORD numlogs = CRegStdDWORD(_T("Software\\CommitMonitor\\NumLogs"), 30);
    if ((startrev <= 1)||(endrev <= 1))
        limit = DWORD(numlogs);

    svn_opt_revision_range_t revision_range = {start, end};

    apr_array_header_t* revision_ranges
        = apr_array_make (localpool, 1, sizeof(apr_array_header_t*));
    *(svn_opt_revision_range_t**)apr_array_push (revision_ranges)
        = &revision_range;

    Err = svn_client_log5 (targets, 
        &start,
        revision_ranges,
        limit,
        true,
        true,
        false,
        NULL,
        logReceiver,
        (void *)this, m_pctx, localpool);

    return (Err == NULL);
}

svn_error_t* SVN::logReceiver(void *baton, svn_log_entry_t *log_entry, apr_pool_t *pool)
{
    svn_error_t * error = NULL;
    SCCSLogEntry logEntry;
    SVN * svn = (SVN *)baton;

    static const std::string svnLog (SVN_PROP_REVISION_LOG);
    static const std::string svnDate (SVN_PROP_REVISION_DATE);
    static const std::string svnAuthor (SVN_PROP_REVISION_AUTHOR);

    logEntry.revision = log_entry->revision;

    if (   (log_entry->revision != SVN_INVALID_REVNUM)
        && (log_entry->revprops != NULL))
    {
        for ( apr_hash_index_t *index
            = apr_hash_first (pool, log_entry->revprops)
            ; index != NULL
            ; index = apr_hash_next (index))
        {
            // extract next entry from hash

            const char* key = NULL;
            ptrdiff_t keyLen;
            const char** val = NULL;

            apr_hash_this ( index
                , reinterpret_cast<const void**>(&key)
                , &keyLen
                , reinterpret_cast<void**>(&val));

            // decode / dispatch it

            std::string name = key;
            std::string value = *val;

            if (name == svnLog)
                logEntry.message = CUnicodeUtils::StdGetUnicode(value);
            else if (name == svnAuthor)
                logEntry.author = CUnicodeUtils::StdGetUnicode(value);
            else if (name == svnDate)
            {
                if (value[0])
                    svn_time_from_cstring (&logEntry.date, *val, pool);
            }
        }
    }

    if (log_entry->changed_paths2)
    {
        apr_array_header_t *sorted_paths;
        sorted_paths = svn_sort__hash(log_entry->changed_paths2, svn_sort_compare_items_as_paths, pool);
        for (int i = 0; i < sorted_paths->nelts; i++)
        {
            SCCSLogChangedPaths changedPaths;
            svn_sort__item_t *item = &(APR_ARRAY_IDX (sorted_paths, i, svn_sort__item_t));
            const char *path = (const char *)item->key;
            svn_log_changed_path2_t *log_changedpaths = (svn_log_changed_path2_t *)apr_hash_get (log_entry->changed_paths2, item->key, item->klen);
            wstring path_native = CUnicodeUtils::StdGetUnicode(path);
            changedPaths.action = log_changedpaths->action;
            if (log_changedpaths->copyfrom_path && SVN_IS_VALID_REVNUM (log_changedpaths->copyfrom_rev))
            {
                changedPaths.copyfrom_path = CUnicodeUtils::StdGetUnicode(log_changedpaths->copyfrom_path);
                changedPaths.copyfrom_revision = log_changedpaths->copyfrom_rev;
            }
            changedPaths.kind = log_changedpaths->node_kind;
            changedPaths.text_modified = log_changedpaths->text_modified;
            changedPaths.props_modified = log_changedpaths->props_modified;
            logEntry.m_changedPaths[path_native] = changedPaths;
        }
    }
    svn->m_logs[log_entry->revision] = logEntry;

    return error;
}

wstring SVN::GetOptionsString(bool bIgnoreEOL, bool bIgnoreSpaces, bool bIgnoreAllSpaces)
{
    wstring opts;
    if (bIgnoreEOL)
        opts += _T("--ignore-eol-style ");
    if (bIgnoreAllSpaces)
        opts += _T("-w");
    else if (bIgnoreSpaces)
        opts += _T("-b");
    return opts;
}

bool SVN::Diff(const wstring& url1, svn_revnum_t pegrevision, svn_revnum_t revision1,
    svn_revnum_t revision2, bool ignoreancestry, bool nodiffdeleted, 
    bool ignorecontenttype,  const wstring& options, bool bAppend, 
    const wstring& outputfile, const wstring& errorfile)
{
    svn_error_clear(Err);
    m_bCanceled = false;
    bool del = FALSE;
    apr_file_t * outfile;
    apr_file_t * errfile;
    apr_array_header_t *opts;

    SVNPool localpool(pool);

    opts = svn_cstring_split(CUnicodeUtils::StdGetUTF8(options).c_str(), " \t\n\r", TRUE, localpool);

    apr_int32_t flags = APR_WRITE | APR_CREATE | APR_BINARY;
    if (bAppend)
        flags |= APR_APPEND;
    else
        flags |= APR_TRUNCATE;
    Err = svn_io_file_open (&outfile, CUnicodeUtils::StdGetUTF8(outputfile).c_str(),
        flags,
        APR_OS_DEFAULT, localpool);
    if (Err)
        return false;

    wstring workingErrorFile;
    if (errorfile.empty())
    {
        workingErrorFile = CAppUtils::GetTempFilePath();
        del = TRUE;
    }
    else
    {
        workingErrorFile = errorfile;
    }

    Err = svn_io_file_open (&errfile, CUnicodeUtils::StdGetUTF8(workingErrorFile).c_str(),
        APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY,
        APR_OS_DEFAULT, localpool);
    if (Err)
        return false;

    svn_opt_revision_t rev1;
    rev1.kind = svn_opt_revision_number;
    rev1.value.number = revision1;

    svn_opt_revision_t rev2;
    rev2.kind = svn_opt_revision_number;
    rev2.value.number = revision2;

    svn_opt_revision_t peg;
    peg.kind = svn_opt_revision_number;
    peg.value.number = pegrevision;


    Err = svn_client_diff_peg5 (opts,
        svn_uri_canonicalize (CAppUtils::PathEscape(CUnicodeUtils::StdGetUTF8(url1)).c_str(), localpool),
        &peg,
        &rev1,
        &rev2,
        NULL,
        svn_depth_infinity,
        ignoreancestry,
        nodiffdeleted,
        true,   // show copies as adds
        ignorecontenttype,
        false,  // use git diff format
        APR_LOCALE_CHARSET,
        outfile,
        errfile,
        NULL,
        m_pctx,
        localpool);
    if (Err)
    {
        return false;
    }
    if (del)
    {
        svn_io_remove_file2 (CUnicodeUtils::StdGetUTF8(workingErrorFile).c_str(), true, localpool);
    }
    return true;
}

wstring SVN::CanonicalizeURL(const wstring& url)
{
    m_bCanceled = false;
    SVNPool localpool(pool);
    return CUnicodeUtils::StdGetUnicode(string(svn_uri_canonicalize (CUnicodeUtils::StdGetUTF8(url).c_str(), localpool)));
}

void SVN::SetAndClearProgressInfo(CProgressDlg * pProgressDlg, bool bShowProgressBar/* = false*/)
{
    m_bCanceled = false;
    m_progressWnd = NULL;
    m_pProgressDlg = pProgressDlg;
    progress_total = 0;
    progress_lastprogress = 0;
    progress_lasttotal = 0;
    progress_lastTicks = GetTickCount();
    m_bShowProgressBar = bShowProgressBar;
}

void SVN::progress_func(apr_off_t progress, apr_off_t total, void *baton, apr_pool_t * /*pool*/)
{
    TCHAR formatbuf[4096];
    SVN * pSVN = (SVN*)baton;
    if ((pSVN==0)||((pSVN->m_progressWnd == 0)&&(pSVN->m_pProgressDlg == 0)))
        return;
    apr_off_t delta = progress;
    if ((progress >= pSVN->progress_lastprogress)&&(total == pSVN->progress_lasttotal))
        delta = progress - pSVN->progress_lastprogress;
    pSVN->progress_lastprogress = progress;
    pSVN->progress_lasttotal = total;

    DWORD ticks = GetTickCount();
    pSVN->progress_vector.push_back(delta);
    pSVN->progress_total += delta;
    if ((pSVN->progress_lastTicks + 1000) < ticks)
    {
        double divby = (double(ticks - pSVN->progress_lastTicks)/1000.0);
        if (divby == 0)
            divby = 1;
        pSVN->m_SVNProgressMSG.overall_total = pSVN->progress_total;
        pSVN->m_SVNProgressMSG.progress = progress;
        pSVN->m_SVNProgressMSG.total = total;
        pSVN->progress_lastTicks = ticks;
        apr_off_t average = 0;
        for (std::vector<apr_off_t>::iterator it = pSVN->progress_vector.begin(); it != pSVN->progress_vector.end(); ++it)
        {
            average += *it;
        }
        average = apr_off_t(double(average) / divby);
        pSVN->m_SVNProgressMSG.BytesPerSecond = average;
        if (average < 1024)
        {
            _stprintf_s(formatbuf, 4096, _T("%ld Bytes/s"), average);
            pSVN->m_SVNProgressMSG.SpeedString = formatbuf;
        }
        else
        {
            double averagekb = (double)average / 1024.0;
            _stprintf_s(formatbuf, 4096, _T("%.2f kBytes/s"), averagekb);
            pSVN->m_SVNProgressMSG.SpeedString = formatbuf;
        }
        if (pSVN->m_pProgressDlg)
        {
            if ((pSVN->m_bShowProgressBar && (progress > 1000) && (total > 0)))
                pSVN->m_pProgressDlg->SetProgress64(progress, total);

            wstring sTotal;
            if (pSVN->m_SVNProgressMSG.overall_total < 1024)
                _stprintf_s(formatbuf, 4096, _T("%I64d Bytes transferred"), pSVN->m_SVNProgressMSG.overall_total);
            else if (pSVN->m_SVNProgressMSG.overall_total < 1200000)
                _stprintf_s(formatbuf, 4096, _T("%I64d kBytes transferred"), pSVN->m_SVNProgressMSG.overall_total / 1024);
            else
                _stprintf_s(formatbuf, 4096, _T("%.2f MBytes transferred"), (double)((double)pSVN->m_SVNProgressMSG.overall_total / 1024000.0));
            sTotal = formatbuf;
            _stprintf_s(formatbuf, 4096, _T("%s, at %s"), sTotal.c_str(), pSVN->m_SVNProgressMSG.SpeedString.c_str());

            pSVN->m_pProgressDlg->SetLine(2, formatbuf);
            if (pSVN->m_pProgressDlg->HasUserCancelled())
            {
                pSVN->m_bCanceled = true;
            }
        }
        pSVN->progress_vector.clear();
    }
    return;
}
