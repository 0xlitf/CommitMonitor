// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007-2008 - Stefan Kueng

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
#include "UrlInfo.h"
#include "AppUtils.h"
#include "MappedInFile.h"
#include <Wincrypt.h>

#pragma comment(lib, "Crypt32.lib")

CUrlInfo::CUrlInfo(void) : lastchecked(0)
	, lastcheckedrev(0)
	, minutesinterval(90)
	, minminutesinterval(0)
	, disallowdiffs(false)
	, parentpath(false)
	, ignoreSelf(false)
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
	string encpwd = CUnicodeUtils::StdGetUTF8(password);
	encpwd = "encrypted_" + encpwd;
	blob.cbData = encpwd.size();
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
	if (!CSerializeUtils::SaveNumber(hFile, disallowdiffs))
		return false;
	if (!CSerializeUtils::SaveNumber(hFile, ignoreSelf))
		return false;
	if (!CSerializeUtils::SaveNumber(hFile, parentpath))
		return false;
	if (!CSerializeUtils::SaveString(hFile, error))
		return false;

    // prevent caching more than 1000 revisions - this is a commit monitor, not a full featured
    // log dialog!
    while (logentries.size() > 1000)
        logentries.erase(logentries.begin());

	if (!CSerializeUtils::SaveNumber(hFile, CSerializeUtils::SerializeType_Map))
		return false;
	if (!CSerializeUtils::SaveNumber(hFile, logentries.size()))
		return false;

	for (map<svn_revnum_t,SVNLogEntry>::iterator it = logentries.begin(); it != logentries.end(); ++it)
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
	blob.cbData = len;
	blob.pbData = pbData;
	if (CryptUnprotectData(&blob, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &outblob))
	{
		string encpwd = string((const char*)outblob.pbData, outblob.cbData);
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
	if (version < 4)
	{
		if (!CSerializeUtils::LoadNumber(buf, value))
			return false;
	}
	if (version > 1)
	{
		if (!CSerializeUtils::LoadNumber(buf, value))
			return false;
		disallowdiffs = !!value;
	}
	if (version >= 3)
	{
		if (!CSerializeUtils::LoadNumber(buf, value))
			return false;
		ignoreSelf = !!value;
	}

	if (!CSerializeUtils::LoadNumber(buf, value))
		return false;
	parentpath = !!value;
	if (!CSerializeUtils::LoadString(buf, error))
		return false;

	logentries.clear();
	if (!CSerializeUtils::LoadNumber(buf, value))
		return false;
	if (CSerializeUtils::SerializeType_Map == value)
	{
		if (CSerializeUtils::LoadNumber(buf, value))
		{
			// we had a bug where the size could be bigger than 1000, but
			// only the first 1000 entries were actually saved.
			// instead of bailing out if the value is bigger than 1000, we
			// adjust it to the max saved values instead.
			// in case the value is out of range for other reasons,
			// the further serialization should bail out soon enough.
			if (value > 1000)
				value = 1000;
			for (unsigned __int64 i=0; i<value; ++i)
			{
				unsigned __int64 key;
				SVNLogEntry logentry;
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
	wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
	wstring urlfilebak = CAppUtils::GetAppDataDir() + _T("\\urls_backup");
	if (!PathFileExists(urlfile.c_str()))
		return false;
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
	wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
	wstring urlfilenew = CAppUtils::GetAppDataDir() + _T("\\urls_new");
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
    char filebuffer[4096];
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
	for (map<wstring,CUrlInfo>::iterator it = infos.begin(); it != infos.end(); ++it)
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
				wstring key;
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
	bIsEmpty = (infos.size() == 0);
	guard.ReleaseReaderLock();
	return bIsEmpty;
}

const map<wstring,CUrlInfo> * CUrlInfos::GetReadOnlyData()
{
	guard.AcquireReaderLock();
	return &infos;
}

map<wstring,CUrlInfo> * CUrlInfos::GetWriteData()
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

