#pragma once
#include <string>

using namespace std;

/**
 * class that maps arbitrary files into memory.
 */
class CMappedInFile
{
public:

	CMappedInFile (const std::wstring& fileName);
	virtual ~CMappedInFile();

	const unsigned char* GetBuffer() const;
	size_t GetSize() const;

private:
	HANDLE file;
	HANDLE mapping;
	const unsigned char* buffer;
	size_t size;
	void MapToMemory (const std::wstring& fileName);
	void UnMap();
};

inline const unsigned char* CMappedInFile::GetBuffer() const
{
	return buffer;
}

inline size_t CMappedInFile::GetSize() const
{
	return size;
}
