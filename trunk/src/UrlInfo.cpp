#include "StdAfx.h"
#include "UrlInfo.h"
#include "AppUtils.h"

CUrlInfo::CUrlInfo(void) : lastchecked(0)
	, lastcheckedrev(0)
	, minutesinterval(90)
	, fetchdiffs(false)
	, parentpath(false)
{
}

CUrlInfo::~CUrlInfo(void)
{
}

bool CUrlInfo::Save(HANDLE hFile)
{
	if (!CSerializeUtils::SaveNumber(hFile, URLINFO_VERSION))
		return false;
	if (!CSerializeUtils::SaveString(hFile, username))
		return false;
	if (!CSerializeUtils::SaveString(hFile, password))
		return false;
	if (!CSerializeUtils::SaveString(hFile, name))
		return false;
	if (!CSerializeUtils::SaveNumber(hFile, lastchecked))
		return false;
	if (!CSerializeUtils::SaveNumber(hFile, lastcheckedrev))
		return false;
	if (!CSerializeUtils::SaveNumber(hFile, minutesinterval))
		return false;
	if (!CSerializeUtils::SaveNumber(hFile, fetchdiffs))
		return false;
	if (!CSerializeUtils::SaveNumber(hFile, parentpath))
		return false;
	if (!CSerializeUtils::SaveString(hFile, error))
		return false;

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

bool CUrlInfo::Load(HANDLE hFile)
{
	unsigned __int64 version = 0;
	if (!CSerializeUtils::LoadNumber(hFile, version))
		return false;
	unsigned __int64 value = 0;
	if (!CSerializeUtils::LoadString(hFile, username))
		return false;
	if (!CSerializeUtils::LoadString(hFile, password))
		return false;
	if (!CSerializeUtils::LoadString(hFile, name))
		return false;
	if (!CSerializeUtils::LoadNumber(hFile, value))
		return false;
	lastchecked = value;
	if (!CSerializeUtils::LoadNumber(hFile, value))
		return false;
	lastcheckedrev = (svn_revnum_t)value;
	if (!CSerializeUtils::LoadNumber(hFile, value))
		return false;
	minutesinterval = (int)value;
	if (!CSerializeUtils::LoadNumber(hFile, value))
		return false;
	fetchdiffs = !!value;
	if (!CSerializeUtils::LoadNumber(hFile, value))
		return false;
	parentpath = !!value;
	if (!CSerializeUtils::LoadString(hFile, error))
		return false;

	logentries.clear();
	if (!CSerializeUtils::LoadNumber(hFile, value))
		return false;
	if (CSerializeUtils::SerializeType_Map == value)
	{
		if (CSerializeUtils::LoadNumber(hFile, value))
		{
			for (unsigned __int64 i=0; i<value; ++i)
			{
				unsigned __int64 key;
				SVNLogEntry logentry;
				if (!CSerializeUtils::LoadNumber(hFile, key))
					return false;
				if (!logentry.Load(hFile))
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

void CUrlInfos::Load()
{
	wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
	if (!PathFileExists(urlfile.c_str()))
		return;
	return Load(urlfile.c_str());
}

void CUrlInfos::Save()
{
	wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
	return Save(urlfile.c_str());
}

void CUrlInfos::Save(LPCWSTR filename)
{
	HANDLE hFile = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_DELETE, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN|FILE_ATTRIBUTE_COMPRESSED, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	guard.AcquireReaderLock();
	bool bSuccess = Save(hFile);
	guard.ReleaseReaderLock();
	CloseHandle(hFile);
	if (bSuccess)
	{
		// rename the file to the original requested name
		TRACE(_T("data saved\n"));
	}
}

void CUrlInfos::Load(LPCWSTR filename)
{
	HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_DELETE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	guard.AcquireWriterLock();
	Load(hFile);
	guard.ReleaseWriterLock();
	TRACE(_T("data loaded\n"));
	CloseHandle(hFile);
}

bool CUrlInfos::Save(HANDLE hFile)
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

bool CUrlInfos::Load(HANDLE hFile)
{
	unsigned __int64 version = 0;
	if (!CSerializeUtils::LoadNumber(hFile, version))
		return false;
	infos.clear();
	unsigned __int64 value = 0;
	if (!CSerializeUtils::LoadNumber(hFile, value))
		return false;
	if (CSerializeUtils::SerializeType_Map == value)
	{
		if (CSerializeUtils::LoadNumber(hFile, value))
		{
			for (unsigned __int64 i=0; i<value; ++i)
			{
				wstring key;
				CUrlInfo info;
				if (!CSerializeUtils::LoadString(hFile, key))
					return false;
				if (!info.Load(hFile))
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

