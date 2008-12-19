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
#pragma once
#include <string>
#include <vector>

#include "SVN.h"
#include "SerializeUtils.h"
#include "ReaderWriterLock.h"

#define URLINFO_VERSION		5
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
	bool						disallowdiffs;
	wstring						ignoreUsers;

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
	bool						Load();
	bool						Save(LPCWSTR filename);
	bool						Load(LPCWSTR filename);
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

