#pragma once
#include <string>
#include <vector>

#include "SVN.h"
#include "SerializeUtils.h"

class CUrlInfo
{
public:
	CUrlInfo(void);
	~CUrlInfo(void);

	wstring						username;
	wstring						password;

	wstring						url;
	wstring						name;
	__time64_t					lastchecked;
	svn_revnum_t				lastcheckedrev;

	int							minutesinterval;
	bool						fetchdiffs;

	map<svn_revnum_t,SVNLogEntry> logentries;

	bool Save(HANDLE hFile) const
	{
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

		if (!CSerializeUtils::SaveNumber(hFile, CSerializeUtils::SerializeType_Map))
			return false;
		if (!CSerializeUtils::SaveNumber(hFile, logentries.size()))
			return false;
		for (map<svn_revnum_t,SVNLogEntry>::const_iterator it = logentries.begin(); it != logentries.end(); ++it)
		{
			if (!CSerializeUtils::SaveNumber(hFile, it->first))
				return false;
			if (!it->second.Save(hFile))
				return false;
		}
		return true;
	}
	bool Load(HANDLE hFile)
	{
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
};

class CUrlInfos
{
public:
	CUrlInfos(void);
	~CUrlInfos(void);

	void						Save(LPCWSTR filename);
	void						Load(LPCWSTR filename);

	map<wstring,CUrlInfo>		infos;

	bool Save(HANDLE hFile) const
	{
		// first save the size of the map
		if (!CSerializeUtils::SaveNumber(hFile, CSerializeUtils::SerializeType_Map))
			return false;
		if (!CSerializeUtils::SaveNumber(hFile, infos.size()))
			return false;
		for (map<wstring,CUrlInfo>::const_iterator it = infos.begin(); it != infos.end(); ++it)
		{
			if (!CSerializeUtils::SaveString(hFile, it->first))
				return false;
			if (!it->second.Save(hFile))
				return false;
		}
		return true;
	}
	bool Load(HANDLE hFile)
	{
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
};

