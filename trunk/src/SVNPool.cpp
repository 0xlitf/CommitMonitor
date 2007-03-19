#include "StdAfx.h"
#include "SVNPool.h"

#include "svn_pools.h"

SVNPool::SVNPool()
{
	m_pool = svn_pool_create(NULL);
}

SVNPool::SVNPool(apr_pool_t* parentPool)
{
	m_pool = svn_pool_create(parentPool);
}

SVNPool::~SVNPool()
{
	svn_pool_destroy(m_pool);
}

SVNPool::operator apr_pool_t*()
{
	return m_pool;
}

