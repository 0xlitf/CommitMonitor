#include "apr_general.h"

/**
 * This class encapsulates an apr_pool taking care of destroying it at end of scope
 * Use this class in preference to doing svn_pool_create and then trying to remember all 
 * the svn_pool_destroys which might be needed.
 */
class SVNPool
{
public:
	SVNPool();
	explicit SVNPool(apr_pool_t* parentPool);
	~SVNPool();
private:
	// Not implemented - we don't want any copying of these objects
	SVNPool(const SVNPool& rhs);
	SVNPool& operator=(SVNPool& rhs);

public:
	operator apr_pool_t*();

private:
	apr_pool_t* m_pool;
};

