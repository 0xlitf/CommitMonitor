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
#include "MappedInFile.h"


void CMappedInFile::MapToMemory (const std::wstring& fileName)
{
	file = CreateFile ( fileName.c_str()
					  , GENERIC_READ
					  , FILE_SHARE_READ
					  , NULL
					  , OPEN_EXISTING
					  , FILE_ATTRIBUTE_NORMAL
					  , NULL);
	if (file == INVALID_HANDLE_VALUE)
		return;

	LARGE_INTEGER fileSize;
	fileSize.QuadPart = 0;
    GetFileSizeEx (file, &fileSize);
	size = (size_t)fileSize.QuadPart;

    mapping = CreateFileMapping (file, NULL, PAGE_READONLY, 0, 0, NULL);
	if (mapping == NULL)
	{
		UnMap();
		return;
	}

	LPVOID address = MapViewOfFile (mapping, FILE_MAP_READ, 0, 0, 0);
    if (address == NULL)
	{
		UnMap();
		return;
	}

	buffer = reinterpret_cast<const unsigned char*>(address);
}

void CMappedInFile::UnMap()
{
	if (buffer != NULL)
		UnmapViewOfFile (buffer);

	if (mapping != INVALID_HANDLE_VALUE)
		CloseHandle (mapping);

	if (file != INVALID_HANDLE_VALUE)
		CloseHandle (file);
}

CMappedInFile::CMappedInFile (const std::wstring& fileName)
    : file (INVALID_HANDLE_VALUE)
	, mapping (INVALID_HANDLE_VALUE)
	, buffer (NULL)
	, size (0)
{
	MapToMemory (fileName);
}

CMappedInFile::~CMappedInFile()
{
	UnMap();
}
