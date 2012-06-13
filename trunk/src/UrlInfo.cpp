// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007-2012 - Stefan Kueng

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
#include "UrlInfo.h"
#include "AppUtils.h"
#include "MappedInFile.h"
#include "SimpleIni.h"
#include "Blowfish.h"
#include <Wincrypt.h>

#pragma comment(lib, "Crypt32.lib")

CUrlInfo::CUrlInfo(void) : lastchecked(0)
    , lastcheckedrev(0)
    , minutesinterval(90)
    , minminutesinterval(0)
    , fetchdiffs(false)
    , disallowdiffs(false)
    , parentpath(false)
    , monitored(true)
    , errNr(0)
    , maxentries(1000)
    , sccs(SCCS_SVN)
    , noexecuteignored(false)
{
}

CUrlInfo::~CUrlInfo(void)
{
}

bool CUrlInfo::Save(FILE * hFile)
{
    if (!CSerializeUtils::SaveNumber(hFile, URLINFO_VERSION))
        return false;
    if (!CSerializeUtils::SaveString(hFile, username))
        return false;
    // encrypt the password
    DATA_BLOB blob, outblob;
    std::string encpwd = CUnicodeUtils::StdGetUTF8(password);
    encpwd = "encrypted_" + encpwd;
    blob.cbData = (DWORD)encpwd.size();
    blob.pbData = (BYTE*)encpwd.c_str();
    if (CryptProtectData(&blob, _T("CommitMonitorLogin"), NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &outblob))
    {
        if (!CSerializeUtils::SaveBuffer(hFile, outblob.pbData, outblob.cbData))
        {
            LocalFree(outblob.pbData);
            return false;
        }
        LocalFree(outblob.pbData);
    }
    if (!CSerializeUtils::SaveString(hFile, name))
        return false;
    if (!CSerializeUtils::SaveNumber(hFile, lastchecked))
        return false;
    if (!CSerializeUtils::SaveNumber(hFile, lastcheckedrev))
        return false;
    if (!CSerializeUtils::SaveNumber(hFile, lastcheckedrobots))
        return false;
    if (!CSerializeUtils::SaveNumber(hFile, minutesinterval))
        return false;
    if (!CSerializeUtils::SaveNumber(hFile, minminutesinterval))
        return false;
    if (!CSerializeUtils::SaveNumber(hFile, fetchdiffs))
        return false;
    if (!CSerializeUtils::SaveNumber(hFile, disallowdiffs))
        return false;
    if (!CSerializeUtils::SaveNumber(hFile, monitored))
        return false;
    if (!CSerializeUtils::SaveString(hFile, ignoreUsers))
        return false;
    if (!CSerializeUtils::SaveString(hFile, includeUsers))
        return false;
    if (!CSerializeUtils::SaveNumber(hFile, parentpath))
        return false;
    if (!CSerializeUtils::SaveString(hFile, error))
        return false;
    if (!CSerializeUtils::SaveNumber(hFile, errNr))
        return false;
    if (!CSerializeUtils::SaveString(hFile, callcommand))
        return false;
    if (!CSerializeUtils::SaveNumber(hFile, noexecuteignored))
        return false;
    if (!CSerializeUtils::SaveString(hFile, webviewer))
        return false;
    if (!CSerializeUtils::SaveNumber(hFile, maxentries))
        return false;

    // RA Sewell: Version 14
    if (!CSerializeUtils::SaveNumber(hFile, sccs))
        return false;
    if (!CSerializeUtils::SaveString(hFile, accurevRepo))
        return false;

    // prevent caching more than URLINFO_MAXENTRIES revisions - this is a commit monitor, not a full featured
    // log dialog!
    while (logentries.size() > (size_t)min(URLINFO_MAXENTRIES, maxentries))
        logentries.erase(logentries.begin());

    if (!CSerializeUtils::SaveNumber(hFile, CSerializeUtils::SerializeType_Map))
        return false;
    if (!CSerializeUtils::SaveNumber(hFile, logentries.size()))
        return false;

    for (auto it = logentries.begin(); it != logentries.end(); ++it)
    {
        if (!CSerializeUtils::SaveNumber(hFile, it->first))
            return false;
        if (!it->second.Save(hFile))
            return false;
    }
    return true;
}

bool CUrlInfo::Load(const unsigned char *& buf)
{
    unsigned __int64 version = 0;
    if (!CSerializeUtils::LoadNumber(buf, version))
        return false;
    unsigned __int64 value = 0;
    if (!CSerializeUtils::LoadString(buf, username))
        return false;

    const unsigned char * buf2 = buf;
    BYTE * pbData = NULL;
    size_t len = 0;
    if (!CSerializeUtils::LoadBuffer(buf, pbData, len))
    {
        buf = buf2;
        if (!CSerializeUtils::LoadString(buf, password))
            return false;
    }

    // decrypt the password
    DATA_BLOB blob, outblob;
    blob.cbData = (DWORD)len;
    blob.pbData = pbData;
    if (CryptUnprotectData(&blob, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &outblob))
    {
        std::string encpwd = std::string((const char*)outblob.pbData, outblob.cbData);
        if (_strnicmp(encpwd.c_str(), "encrypted_", 10) == 0)
        {
            encpwd = encpwd.substr(10);
            password = CUnicodeUtils::StdGetUnicode(encpwd);
        }
        LocalFree(outblob.pbData);
    }
    delete [] pbData;

    if (!CSerializeUtils::LoadString(buf, name))
        return false;
    if (!CSerializeUtils::LoadNumber(buf, value))
        return false;
    lastchecked = value;
    if (!CSerializeUtils::LoadNumber(buf, value))
        return false;
    lastcheckedrev = (svn_revnum_t)value;
    if (version > 1)
    {
        if (!CSerializeUtils::LoadNumber(buf, value))
            return false;
        lastcheckedrobots = (int)value;
    }
    if (!CSerializeUtils::LoadNumber(buf, value))
        return false;
    minutesinterval = (int)value;
    if (version > 1)
    {
        if (!CSerializeUtils::LoadNumber(buf, value))
            return false;
        minminutesinterval = (int)value;
    }
    if ((version < 4)||(version >= 12))
    {
        if (!CSerializeUtils::LoadNumber(buf, value))
            return false;
        fetchdiffs = !!value;
    }
    if (version > 1)
    {
        if (!CSerializeUtils::LoadNumber(buf, value))
            return false;
        disallowdiffs = !!value;
    }
    if (version >= 9)
    {
        if (!CSerializeUtils::LoadNumber(buf, value))
            return false;
        monitored = !!value;
    }
    if (version >= 3)
    {
        if (version >= 5)
        {
            if (!CSerializeUtils::LoadString(buf, ignoreUsers))
                return false;
        }
        else
        {
            if (!CSerializeUtils::LoadNumber(buf, value))
                return false;
            if (value)
                ignoreUsers = username;
        }
    }

    if (version >= 13)
    {
        if (!CSerializeUtils::LoadString(buf, includeUsers))
            return false;
    }

    if (!CSerializeUtils::LoadNumber(buf, value))
        return false;
    parentpath = !!value;
    if (!CSerializeUtils::LoadString(buf, error))
        return false;
    if (version >= 10)
    {
        if (!CSerializeUtils::LoadNumber(buf, value))
            return false;
        errNr = (apr_status_t)value;
    }
    if (version >= 6)
    {
        if (!CSerializeUtils::LoadString(buf, callcommand))
            return false;
    }
    if (version >= 8)
    {
        if (!CSerializeUtils::LoadNumber(buf, value))
            return false;
        noexecuteignored = !!value;
    }
    else
        noexecuteignored = false;
    if (version >= 7)
    {
        if (!CSerializeUtils::LoadString(buf, webviewer))
            return false;
    }
    if (version >= 11)
    {
        if (!CSerializeUtils::LoadNumber(buf, value))
            return false;
        maxentries = (int)value;
    }
    else
        maxentries = URLINFO_MAXENTRIES;

    if (version >= 14)
    {
        if (!CSerializeUtils::LoadNumber(buf, value))
            return false;
        sccs = (SCCS_TYPE)value;

        if (!CSerializeUtils::LoadString(buf, accurevRepo))
            return false;
    }
    else
    {
        sccs = SCCS_SVN;
    }


    logentries.clear();
    if (!CSerializeUtils::LoadNumber(buf, value))
        return false;
    if (CSerializeUtils::SerializeType_Map == value)
    {
        if (CSerializeUtils::LoadNumber(buf, value))
        {
            // we had a bug where the size could be bigger than URLINFO_MAXENTRIES, but
            // only the first URLINFO_MAXENTRIES entries were actually saved.
            // instead of bailing out if the value is bigger than URLINFO_MAXENTRIES, we
            // adjust it to the max saved values instead.
            // in case the value is out of range for other reasons,
            // the further serialization should bail out soon enough.
            if (value > URLINFO_MAXENTRIES)
                value = URLINFO_MAXENTRIES;
            for (unsigned __int64 i=0; i<value; ++i)
            {
                unsigned __int64 key;
                SCCSLogEntry logentry;
                if (!CSerializeUtils::LoadNumber(buf, key))
                    return false;
                if (!logentry.Load(buf))
                    return false;
                logentries[(svn_revnum_t)key] = logentry;
            }
            return true;
        }
    }
    return false;
}

CUrlInfos::CUrlInfos(void)
{
}

CUrlInfos::~CUrlInfos(void)
{
}

bool CUrlInfos::Load()
{
    std::wstring urlfile = CAppUtils::GetDataDir() + _T("\\urls");
    std::wstring urlfilebak = CAppUtils::GetDataDir() + _T("\\urls_backup");
    if (!PathFileExists(urlfile.c_str()))
    {
        return false;
    }
    if (Load(urlfile.c_str()))
    {
        // urls file successfully loaded: create a backup copy
        CopyFile(urlfile.c_str(), urlfilebak.c_str(), FALSE);
        return true;
    }
    else
    {
        // loading the file failed. Check whether there's a backup
        // file available to load instead
        return Load(urlfilebak.c_str());
    }
}

void CUrlInfos::Save()
{
    bool bExit = false;
    const std::map<std::wstring,CUrlInfo> * pInfos = GetReadOnlyData();
    if (pInfos->empty())
    {
        // empty project list: don't save it!
        // See issue #267 for why: http://code.google.com/p/commitmonitor/issues/detail?id=267
        bExit = true;
    }
    ReleaseReadOnlyData();
    if (bExit)
        return;

    std::wstring urlfile = CAppUtils::GetDataDir() + _T("\\urls");
    std::wstring urlfilenew = CAppUtils::GetDataDir() + _T("\\urls_new");
    if (Save(urlfilenew.c_str()))
    {
        DeleteFile(urlfile.c_str());
        MoveFile(urlfilenew.c_str(), urlfile.c_str());
    }
}

bool CUrlInfos::Save(LPCWSTR filename)
{
#ifdef _DEBUG
    DWORD dwStartTicks = GetTickCount();
#endif
    FILE * hFile = NULL;
    _tfopen_s(&hFile, filename, _T("w+b"));
    if (hFile == NULL)
        return false;
    char filebuffer[4096] = {0};
    setvbuf(hFile, filebuffer, _IOFBF, 4096);

    guard.AcquireReaderLock();
    bool bSuccess = Save(hFile);
    guard.ReleaseReaderLock();
    fclose(hFile);
    if (bSuccess)
    {
        // rename the file to the original requested name
        TRACE(_T("data saved\n"));
#ifdef _DEBUG
        TCHAR timerbuf[MAX_PATH] = {0};
        _stprintf_s(timerbuf, MAX_PATH, _T("time needed for saving all url info: %ld ms\n"), GetTickCount()-dwStartTicks);
        TRACE(timerbuf);
#endif
        return true;
    }
    return false;
}

bool CUrlInfos::Load(LPCWSTR filename)
{
    bool bRet = false;
#ifdef _DEBUG
    DWORD dwStartTicks = GetTickCount();
#endif
    CMappedInFile file(filename);
    guard.AcquireWriterLock();
    const unsigned char * buf = file.GetBuffer();
    if (buf)
        bRet = Load(buf);
    guard.ReleaseWriterLock();
    TRACE(_T("data loaded\n"));
#ifdef _DEBUG
    TCHAR timerbuf[MAX_PATH] = {0};
    _stprintf_s(timerbuf, MAX_PATH, _T("time needed for loading all url info: %ld ms\n"), GetTickCount()-dwStartTicks);
    TRACE(timerbuf);
#endif
    return bRet;
}

bool CUrlInfos::Save(FILE * hFile)
{
    if (!CSerializeUtils::SaveNumber(hFile, URLINFOS_VERSION))
        return false;
    // first save the size of the map
    if (!CSerializeUtils::SaveNumber(hFile, CSerializeUtils::SerializeType_Map))
        return false;
    if (!CSerializeUtils::SaveNumber(hFile, infos.size()))
        return false;
    for (auto it = infos.begin(); it != infos.end(); ++it)
    {
        if (!CSerializeUtils::SaveString(hFile, it->first))
            return false;
        if (!it->second.Save(hFile))
            return false;
    }
    return true;
}

bool CUrlInfos::Load(const unsigned char *& buf)
{
    unsigned __int64 version = 0;
    if (!CSerializeUtils::LoadNumber(buf, version))
        return false;
    infos.clear();
    unsigned __int64 value = 0;
    if (!CSerializeUtils::LoadNumber(buf, value))
        return false;
    if (CSerializeUtils::SerializeType_Map == value)
    {
        if (CSerializeUtils::LoadNumber(buf, value))
        {
            for (unsigned __int64 i=0; i<value; ++i)
            {
                std::wstring key;
                CUrlInfo info;
                if (!CSerializeUtils::LoadString(buf, key))
                    return false;
                if (!info.Load(buf))
                    return false;
                info.url = key;
                infos[key] = info;
            }
            return true;
        }
    }
    return false;
}

bool CUrlInfos::IsEmpty()
{
    bool bIsEmpty = true;
    guard.AcquireReaderLock();
    bIsEmpty = (infos.empty());
    guard.ReleaseReaderLock();
    return bIsEmpty;
}

std::string CUrlInfos::CalcMD5(LPCWSTR s)
{
    HCRYPTPROV hCryptProv;
    HCRYPTHASH hHash = 0;
    BYTE bHash[0x7f];
    DWORD dwHashLen= 16; // The MD5 algorithm always returns 16 bytes.
    DWORD cbContent= (DWORD)wcslen(s)*sizeof(WCHAR);
    BYTE* pbContent= (BYTE*)s;
    std::string retHash;

    if (CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET))
    {
        if (CryptCreateHash(hCryptProv,
                            CALG_MD5,   // algorithm identifier definitions see: wincrypt.h
                            0, 0, &hHash))
        {
            if (CryptHashData(hHash, pbContent, cbContent, 0))
            {

                if (CryptGetHashParam(hHash, HP_HASHVAL, bHash, &dwHashLen, 0))
                {
                    // Make a string version of the numeric digest value
                    char tmpBuf[3];
                    for (int i = 0; i<16; i++)
                    {
                        sprintf_s(tmpBuf, 3, "%02x", bHash[i]);
                        retHash += tmpBuf;
                    }
                }
            }
        }
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hCryptProv, 0);

    return retHash;
}

bool CUrlInfos::Export(LPCWSTR filename, LPCWSTR password)
{
    FILE * hFile = NULL;
    _tfopen_s(&hFile, filename, _T("w+b"));
    if (hFile == NULL)
        return false;

    std::string pwHash = CalcMD5(password);

    guard.AcquireReaderLock();

    CSimpleIni iniFile;
    CBlowFish blower((const unsigned char*)pwHash.c_str(), 16);
    WCHAR numberBuf[1024];

    for (auto it = infos.begin(); it != infos.end(); ++it)
    {
        iniFile.SetValue(it->first.c_str(), L"username", it->second.username.c_str());
        iniFile.SetValue(it->first.c_str(), L"url", it->second.url.c_str());
        iniFile.SetValue(it->first.c_str(), L"name", it->second.name.c_str());
        std::wstring ignoreUsers = it->second.ignoreUsers;
        CAppUtils::SearchReplace(ignoreUsers, L"\r\n", L"\t");
        iniFile.SetValue(it->first.c_str(), L"ignoreUsers", ignoreUsers.c_str());
        std::wstring includeUsers = it->second.includeUsers;
        CAppUtils::SearchReplace(includeUsers, L"\r\n", L"\t");
        iniFile.SetValue(it->first.c_str(), L"includeUsers", includeUsers.c_str());
        iniFile.SetValue(it->first.c_str(), L"callcommand", it->second.callcommand.c_str());
        iniFile.SetValue(it->first.c_str(), L"webviewer", it->second.webviewer.c_str());

        swprintf_s(numberBuf, 1024, L"%ld", it->second.minutesinterval);
        iniFile.SetValue(it->first.c_str(), L"minutesinterval", numberBuf);
        swprintf_s(numberBuf, 1024, L"%ld", it->second.minminutesinterval);
        iniFile.SetValue(it->first.c_str(), L"minminutesinterval", numberBuf);
        swprintf_s(numberBuf, 1024, L"%ld", (int)it->second.disallowdiffs);
        iniFile.SetValue(it->first.c_str(), L"disallowdiffs", numberBuf);
        swprintf_s(numberBuf, 1024, L"%ld", it->second.maxentries);
        iniFile.SetValue(it->first.c_str(), L"maxentries", numberBuf);
        swprintf_s(numberBuf, 1024, L"%ld", it->second.noexecuteignored);
        iniFile.SetValue(it->first.c_str(), L"noexecuteignored", numberBuf);
        swprintf_s(numberBuf, 1024, L"%ld", it->second.monitored);
        iniFile.SetValue(it->first.c_str(), L"monitored", numberBuf);

        if (it->second.password.size())
        {
            // encrypt the password
            size_t bufSize = ((it->second.password.size() / 8) + 1) * 8;
            std::unique_ptr<WCHAR[]> pwBuf(new WCHAR[bufSize + 1]);
            SecureZeroMemory(pwBuf.get(), bufSize*sizeof(WCHAR));
            wcscpy_s(pwBuf.get(), bufSize, it->second.password.c_str());
            BYTE * pByteBuf = (BYTE*)pwBuf.get();
            blower.Encrypt(pByteBuf, bufSize*sizeof(WCHAR));
            WCHAR tmpBuf[3];
            std::wstring encryptedPassword;
            for (unsigned int i = 0; i < bufSize*sizeof(WCHAR); ++i)
            {
                swprintf_s(tmpBuf, 3, L"%02x", pByteBuf[i]);
                encryptedPassword += tmpBuf;
            }

            iniFile.SetValue(it->first.c_str(), L"password", encryptedPassword.c_str());
        }
    }

    SI_Error err = iniFile.SaveFile(hFile);
    fclose(hFile);
    guard.ReleaseReaderLock();

    return (err == SI_OK);
}

bool CUrlInfos::Import(LPCWSTR filename, LPCWSTR password)
{
    CSimpleIni iniFile;
    if (iniFile.LoadFile(filename) != SI_OK)
        return false;

    std::string pwHash = CalcMD5(password);
    guard.AcquireWriterLock();
    CBlowFish blower((const unsigned char*)pwHash.c_str(), 16);

    CSimpleIni::TNamesDepend sections;
    iniFile.GetAllSections(sections);
    for (CSimpleIni::TNamesDepend::iterator it = sections.begin(); it != sections.end(); ++it)
    {
        CUrlInfo info;

        info.username = std::wstring(iniFile.GetValue(*it, _T("username"), _T("")));
        info.url = std::wstring(iniFile.GetValue(*it, _T("url"), _T("")));
        info.name = std::wstring(iniFile.GetValue(*it, _T("name"), _T("")));
        info.ignoreUsers = std::wstring(iniFile.GetValue(*it, _T("ignoreUsers"), _T("")));
        CAppUtils::SearchReplace(info.ignoreUsers, L"\t", L"\r\n");
        info.includeUsers = std::wstring(iniFile.GetValue(*it, L"includeUsers", L""));
        CAppUtils::SearchReplace(info.includeUsers, L"\t", L"\r\n");
        info.callcommand = std::wstring(iniFile.GetValue(*it, _T("callcommand"), _T("")));
        info.webviewer = std::wstring(iniFile.GetValue(*it, _T("webviewer"), _T("")));

        info.minutesinterval = _wtol(iniFile.GetValue(*it, L"minutesinterval", L""));
        info.minminutesinterval = _wtol(iniFile.GetValue(*it, L"minminutesinterval", L""));
        info.disallowdiffs = !!_wtol(iniFile.GetValue(*it, L"disallowdiffs", L""));
        info.maxentries = _wtol(iniFile.GetValue(*it, L"maxentries", L""));
        info.noexecuteignored = !!_wtol(iniFile.GetValue(*it, L"noexecuteignored", L""));
        info.monitored = !!_wtol(iniFile.GetValue(*it, L"monitored", L"1"));

        std::wstring unencryptedPassword = std::wstring(iniFile.GetValue(*it, _T("password"), _T("")));
        if (unencryptedPassword.size())
        {
            // decrypt the password
            std::unique_ptr<BYTE[]> pPwBuf(new BYTE[unencryptedPassword.size()/2]);
            SecureZeroMemory(pPwBuf.get(), unencryptedPassword.size()/2);
            const WCHAR * pUnencryptedString = unencryptedPassword.c_str();
            for (unsigned int i = 0; i < unencryptedPassword.size()/2; ++i)
            {
                WCHAR tmpBuf[3];
                wcsncpy_s(tmpBuf, 3, &pUnencryptedString[i*2], 2);
                WCHAR * stopString;
                pPwBuf[i] = (BYTE)wcstol(tmpBuf, &stopString, 16);
            }
            blower.Decrypt(pPwBuf.get(), unencryptedPassword.size()/2);
            WCHAR * pDecryptedPW = (WCHAR*)pPwBuf.get();
            std::wstring plainPw = pDecryptedPW;
            info.password = plainPw;
        }

        if ((infos.size())&&(infos.find(info.url) != infos.end()))
        {
            CUrlInfo existingUrlInfo = infos.find(info.url)->second;
            existingUrlInfo.username = info.username;
            existingUrlInfo.url = info.url;
            existingUrlInfo.name = info.name;
            existingUrlInfo.ignoreUsers = info.ignoreUsers;
            existingUrlInfo.callcommand = info.callcommand;
            existingUrlInfo.webviewer = info.webviewer;
            existingUrlInfo.minutesinterval = info.minutesinterval;
            existingUrlInfo.minminutesinterval = info.minminutesinterval;
            existingUrlInfo.disallowdiffs = info.disallowdiffs;
            existingUrlInfo.maxentries = info.maxentries;
            existingUrlInfo.noexecuteignored = info.noexecuteignored;
            existingUrlInfo.monitored = info.monitored;

            infos[existingUrlInfo.url] = existingUrlInfo;
        }
        else
            infos[info.url] = info;
    }


    guard.ReleaseWriterLock();
    return true;
}

const std::map<std::wstring,CUrlInfo> * CUrlInfos::GetReadOnlyData()
{
    guard.AcquireReaderLock();
    return &infos;
}

std::map<std::wstring,CUrlInfo> * CUrlInfos::GetWriteData()
{
    guard.AcquireWriterLock();
    return &infos;
}

void CUrlInfos::ReleaseReadOnlyData()
{
    guard.ReleaseReaderLock();
}

void CUrlInfos::ReleaseWriteData()
{
    guard.ReleaseWriterLock();
}
