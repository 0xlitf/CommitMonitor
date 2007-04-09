#pragma once
#include <vector>
#include <string>

using namespace std;

/**
 * This singleton class handles temporary files.
 * All temp files are deleted at the end of a run of SVN operations
 */
class CTempFiles
{
private:
	CTempFiles(void);
	~CTempFiles(void);
public:
	static CTempFiles& Instance();
	
	/**
	 * Returns a path to a temporary file.
	 * \param bRemoveAtEnd if true, the temp file is removed when this object
	 *                     goes out of scope.
	 */
	wstring				GetTempFilePath(bool bRemoveAtEnd);

private:

private:
	vector<wstring>		m_TempFileList;
};
