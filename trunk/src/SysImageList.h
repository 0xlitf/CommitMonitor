#pragma once
#include <string>

using namespace std;

/**
 * Provides simplified access to the system icons. Only small system icons
 * are supported.
 *
 * \note This class is implemented as a singleton.
 * The singleton instance is created when first accessed. See SYS_IMAGE_LIST() function
 * easy access of the singleton instance. All 
 */
class CSysImageList
{
// Singleton constructor and destructor (private)
private:
	CSysImageList();
	~CSysImageList();

// Singleton specific operations
public:
	/**
	 * Returns a reference to the one and only instance of this class.
	 */
	static CSysImageList& GetInstance();
	/**
	 * Frees all allocated resources (if necessary). Don't call this
	 * function when the image list is currently bound to any control!
	 */
	static void Cleanup();

// Operations
public:
	/**
	 * Returns the icon index for a directory.
	 */
	int GetDirIconIndex() const;
	/**
	 * Returns the icon index for a directory that's open (e.g. for a tree control)
	 */
	int GetDirOpenIconIndex() const;
	/**
	 * Returns the icon index for a file which has no special icon associated.
	 */
	int GetDefaultIconIndex() const;
	/**
	 * Returns the icon index for the specified \a file. Only the file extension
	 * is used to determine the file's icon.
	 */
	int GetFileIconIndex(const wstring& file) const;

	operator HIMAGELIST() {return m_hSystemImageList;}

private:
	static CSysImageList *instance;
	HIMAGELIST m_hSystemImageList;
};


/**
 * \relates CSysImageList
 * Singleton access for CSysImageList.
 */
inline CSysImageList& SYS_IMAGE_LIST() { return CSysImageList::GetInstance(); }
