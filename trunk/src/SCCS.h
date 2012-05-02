// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2010-2012 - Stefan Kueng

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

#pragma warning(push)
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
#include "svn_props.h"
#pragma warning(pop)

#include "SVNPool.h"
#include "UnicodeUtils.h"
#include "Registry.h"
#include "SerializeUtils.h"
#include "ProgressDlg.h"

#include <string>

using namespace std;

class SCCSInfoData
{
public:
    SCCSInfoData(){}

    std::wstring        url;
    svn_revnum_t        rev;
    svn_node_kind_t     kind;
    std::wstring        reposRoot;
    std::wstring        reposUUID;
    svn_revnum_t        lastchangedrev;
    __time64_t          lastchangedtime;
    std::wstring        author;

    std::wstring        lock_path;
    std::wstring        lock_token;
    std::wstring        lock_owner;
    std::wstring        lock_comment;
    bool                lock_davcomment;
    __time64_t          lock_createtime;
    __time64_t          lock_expirationtime;

    bool                hasWCInfo;
    svn_wc_schedule_t   schedule;
    std::wstring        copyfromurl;
    svn_revnum_t        copyfromrev;
};

class SCCSLogChangedPaths
{
#define SCCSLOGCHANGEDPATHSVERSION 1
public:
    SCCSLogChangedPaths()
        : action(0)
        , text_modified(svn_tristate_unknown)
        , props_modified(svn_tristate_unknown)
        , kind(svn_node_unknown)
    {

    }

    wchar_t             action;
    svn_revnum_t        copyfrom_revision;
    std::wstring        copyfrom_path;
    svn_tristate_t      text_modified;
    svn_tristate_t      props_modified;
    svn_node_kind_t     kind;

    bool Save(FILE * hFile) const
    {
        if (!CSerializeUtils::SaveNumber(hFile, SCCSLOGCHANGEDPATHSVERSION))
            return false;
        if (!CSerializeUtils::SaveNumber(hFile, action))
            return false;
        if (!CSerializeUtils::SaveNumber(hFile, copyfrom_revision))
            return false;
        if (!CSerializeUtils::SaveString(hFile, copyfrom_path))
            return false;
        if (!CSerializeUtils::SaveNumber(hFile, text_modified))
            return false;
        if (!CSerializeUtils::SaveNumber(hFile, props_modified))
            return false;
        if (!CSerializeUtils::SaveNumber(hFile, kind))
            return false;
        return true;
    }
    bool Load(FILE * hFile)
    {
        unsigned __int64 version = 0;
        unsigned __int64 value;
        if (!CSerializeUtils::LoadNumber(hFile, value))
            return false;
        if (value < 'A')
        {
            version = value;
        }
        action = (wchar_t)value;
        if (version > 0)
        {
            if (!CSerializeUtils::LoadNumber(hFile, value))
                return false;
            action = (wchar_t)value;
        }
        if (!CSerializeUtils::LoadNumber(hFile, value))
            return false;
        copyfrom_revision = (svn_revnum_t)value;
        if (!CSerializeUtils::LoadString(hFile, copyfrom_path))
            return false;
        if (version > 0)
        {
            if (!CSerializeUtils::LoadNumber(hFile, value))
                return false;
            text_modified = (svn_tristate_t)value;
            if (!CSerializeUtils::LoadNumber(hFile, value))
                return false;
            props_modified = (svn_tristate_t)value;
            if (!CSerializeUtils::LoadNumber(hFile, value))
                return false;
            kind = (svn_node_kind_t)value;
        }
        return true;
    }
    bool Load(const unsigned char *& buf)
    {
        unsigned __int64 version = 0;
        unsigned __int64 value;
        if (!CSerializeUtils::LoadNumber(buf, value))
            return false;
        if (value < 'A')
        {
            version = value;
        }
        action = (wchar_t)value;
        if (version > 0)
        {
            if (!CSerializeUtils::LoadNumber(buf, value))
                return false;
            action = (wchar_t)value;
        }
        if (!CSerializeUtils::LoadNumber(buf, value))
            return false;
        copyfrom_revision = (svn_revnum_t)value;
        if (!CSerializeUtils::LoadString(buf, copyfrom_path))
            return false;
        if (version > 0)
        {
            if (!CSerializeUtils::LoadNumber(buf, value))
                return false;
            text_modified = (svn_tristate_t)value;
            if (!CSerializeUtils::LoadNumber(buf, value))
                return false;
            props_modified = (svn_tristate_t)value;
            if (!CSerializeUtils::LoadNumber(buf, value))
                return false;
            kind = (svn_node_kind_t)value;
        }
        return true;
    }
};

class SCCSLogEntry
{
public:
    SCCSLogEntry()
        : read(false)
        , revision(0)
        , date(0)
    {

    }

    bool                read;
    svn_revnum_t        revision;
    std::wstring        author;
    apr_time_t          date;
    std::wstring        message;
    map<std::wstring, SCCSLogChangedPaths>   m_changedPaths;

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
        for (map<std::wstring,SCCSLogChangedPaths>::const_iterator it = m_changedPaths.begin(); it != m_changedPaths.end(); ++it)
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
                    SCCSLogChangedPaths cpaths;
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
                    SCCSLogChangedPaths cpaths;
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

class SCCS
{
public:
    SCCS(void);
    ~SCCS(void);

    virtual void SetAuthInfo(const std::wstring& username, const std::wstring& password) = 0;

    virtual bool GetFile(std::wstring sUrl, std::wstring sFile) = 0;

    /**
     * returns the info for the \a path.
     * \param path a path or an url
     * \param pegrev the peg revision to use
     * \param revision the revision to get the info for
     * \param recurse if TRUE, then GetNextFileInfo() returns the info also
     * for all children of \a path.
     */
    virtual wstring GetRootUrl(const std::wstring& path) = 0;
    virtual size_t GetFileCount() = 0;

    virtual svn_revnum_t GetHEADRevision(const std::wstring& repo, const std::wstring& url) = 0;

    virtual bool GetLog(const std::wstring& repo, const std::wstring& url, svn_revnum_t startrev, svn_revnum_t endrev) = 0;


    virtual bool Diff(const wstring& url1, svn_revnum_t pegrevision, svn_revnum_t revision1,
        svn_revnum_t revision2, bool ignoreancestry, bool nodiffdeleted,
        bool ignorecontenttype,  const wstring& options, bool bAppend,
        const wstring& outputfile, const wstring& errorfile) = 0;

    virtual wstring CanonicalizeURL(const wstring& url) { return url; }
    virtual wstring GetLastErrorMsg() = 0;

    /**
     * Sets and clears the progress info which is shown during lengthy operations.
     * \param pProgressDlg the CProgressDlg object to show the progress info on.
     * \param bShowProgressBar set to true if the progress bar should be shown. Only makes
     * sense if the total amount of the progress is known beforehand. Otherwise the
     * progressbar is always "empty".
     */
    virtual void SetAndClearProgressInfo(CProgressDlg * pProgressDlg, bool bShowProgressBar = false) = 0;

    struct SVNProgress
    {
        apr_off_t progress;         ///< operation progress
        apr_off_t total;            ///< operation progress
        apr_off_t overall_total;    ///< total bytes transferred, use SetAndClearProgressInfo() to reset this
        apr_off_t BytesPerSecond;   ///< Speed in bytes per second
        wstring   SpeedString;      ///< String for speed. Either "xxx Bytes/s" or "xxx kBytes/s"
    };

    bool                        m_bCanceled;
    svn_error_t *               Err;            ///< Global error object struct
    map<svn_revnum_t,SCCSLogEntry> m_logs;       ///< contains the gathered log information
};
