#pragma once
#include <string>
#include <vector>

#include "SVN.h"
#include "SerializeUtils.h"
#include "ReaderWriterLock.h"

#define URLINFO_VERSION		2
#define URLINFOS_VERSION	1

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
	__time64_t					lastcheckedrobots;

	int							minutesinterval;
	int							minminutesinterval;
	bool						fetchdiffs;
	bool						disallowdiffs;

	map<svn_revnum_t,SVNLogEntry> logentries;

	bool						parentpath;
	wstring						error;

	bool						Save(FILE * hFile);
	bool						Load(const unsigned char *& buf);
};

class CUrlInfos
{
public:
	CUrlInfos(void);
	~CUrlInfos(void);

	void						Save();
	void						Load();
	void						Save(LPCWSTR filename);
	void						Load(LPCWSTR filename);
	bool						IsEmpty();

	const map<wstring,CUrlInfo> *	GetReadOnlyData();
	map<wstring,CUrlInfo> *		GetWriteData();
	void						ReleaseReadOnlyData();
	void						ReleaseWriteData();

protected:
	bool						Save(FILE * hFile);
	bool						Load(const unsigned char *& buf);

private:
	map<wstring,CUrlInfo>		infos;
	CReaderWriterLock			guard;
};

