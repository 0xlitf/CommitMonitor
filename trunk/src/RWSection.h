#pragma once
#include <assert.h>

class CRWSection
{
public:
	CRWSection();
	~CRWSection();

	void WaitToRead();
	void WaitToWrite();
	void Done();
	bool IsWriter() {return ((m_nWaitingWriters > 0) || (m_nActive < 0));}
#if defined (DEBUG) || defined (_DEBUG)
	void AssertWriting() {assert((m_nWaitingWriters || (m_nActive < 0)));}
#else
	void AssertWriting() {;}
#endif
private:
	int					m_nWaitingReaders;	// Number of readers waiting for access
	int					m_nWaitingWriters;	// Number of writers waiting for access
	int					m_nActive;			// Number of threads accessing the section
	CRITICAL_SECTION	m_cs;
	HANDLE				m_hReaders;
	HANDLE				m_hWriters;
};