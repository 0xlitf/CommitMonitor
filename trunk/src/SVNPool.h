// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007, 2012 - Stefan Kueng

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
