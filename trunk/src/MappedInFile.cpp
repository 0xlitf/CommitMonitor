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
	if (mapping == INVALID_HANDLE_VALUE)
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
