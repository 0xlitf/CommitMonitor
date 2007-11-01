#include "StdAfx.h"
#include "UrlInfo.h"
#include "AppUtils.h"

CUrlInfo::CUrlInfo(void) : lastchecked(0)
	, lastcheckedrev(0)
	, minutesinterval(90)
	, minminutesinterval(0)
	, fetchdiffs(false)
	, disallowdiffs(false)
	, parentpath(false)
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
	if (!CSerializeUtils::SaveString(hFile, password))
		return false;
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
	if (!CSerializeUtils::SaveNumber(hFile, parentpath))
		return false;
	if (!CSerializeUtils::SaveString(hFile, error))
		return false;

	if (!CSerializeUtils::SaveNumber(hFile, CSerializeUtils::SerializeType_Map))
		return false;
	if (!CSerializeUtils::SaveNumber(hFile, logentries.size()))
		return false;

    // prevent caching more than 1000 revisions - this is a commit monitor, not a full featured
    // log dialog!
    while (logentries.size() > 1000)
        logentries.erase(logentries.begin());

	for (map<svn_revnum_t,SVNLogEntry>::iterator it = logentries.begin(); it != logentries.end(); ++it)
	{
		if (!CSerializeUtils::SaveNumber(hFile, it->first))
			return false;
		if (!it->second.Save(hFile))
			return false;
	}
	return true;
}

bool CUrlInfo::Load(FILE * hFile)
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
	if (version > 1)
	{
		if (!CSerializeUtils::LoadNumber(hFile, value))
			return false;
		lastcheckedrobots = (int)value;
	}
	if (!CSerializeUtils::LoadNumber(hFile, value))
		return false;
	minutesinterval = (int)value;
	if (version > 1)
	{
		if (!CSerializeUtils::LoadNumber(hFile, value))
			return false;
		minminutesinterval = (int)value;
	}
	if (!CSerializeUtils::LoadNumber(hFile, value))
		return false;
	fetchdiffs = !!value;
	if (version > 1)
	{
		if (!CSerializeUtils::LoadNumber(hFile, value))
			return false;
		disallowdiffs = !!value;
	}
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
#ifdef _DEBUG
	DWORD dwStartTicks = GetTickCount();
#endif
    FILE * hFile = NULL;
    _tfopen_s(&hFile, filename, _T("w+b"));
	if (hFile == NULL)
		return;
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
	}
}

void CUrlInfos::Load(LPCWSTR filename)
{
#ifdef _DEBUG
	DWORD dwStartTicks = GetTickCount();
#endif
    FILE * hFile = NULL;
    _tfopen_s(&hFile, filename, _T("rb"));
	if (hFile == NULL)
		return;
    char filebuffer[4096];
    setvbuf(hFile, filebuffer, _IOFBF, 4096);

    guard.AcquireWriterLock();
	Load(hFile);
	guard.ReleaseWriterLock();
	TRACE(_T("data loaded\n"));
	fclose(hFile);
#ifdef _DEBUG
	TCHAR timerbuf[MAX_PATH] = {0};
	_stprintf_s(timerbuf, MAX_PATH, _T("time needed for loading all url info: %ld ms\n"), GetTickCount()-dwStartTicks);
	TRACE(timerbuf);
#endif

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

bool CUrlInfos::Load(FILE * hFile)
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

