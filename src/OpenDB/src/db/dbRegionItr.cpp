///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "dbRegionItr.h"

#include "dbMaster.h"
#include "dbRegion.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbRegionItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbRegionItr::reversible()
{
  return true;
}

bool dbRegionItr::orderReversed()
{
  return true;
}

void dbRegionItr::reverse(dbObject* parent)
{
  _dbRegion* region = (_dbRegion*) parent;
  uint       id     = region->_children;
  uint       list   = 0;

  while (id != 0) {
    _dbRegion* pin   = _region_tbl->getPtr(id);
    uint       n     = pin->_next_child;
    pin->_next_child = list;
    list             = id;
    id               = n;
  }

  region->_children = list;
}

uint dbRegionItr::sequential()
{
  return 0;
}

uint dbRegionItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbRegionItr::begin(parent); id != dbRegionItr::end(parent);
       id = dbRegionItr::next(id))
    ++cnt;

  return cnt;
}

uint dbRegionItr::begin(dbObject* parent)
{
  _dbRegion* region = (_dbRegion*) parent;
  return region->_children;
}

uint dbRegionItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbRegionItr::next(uint id, ...)
{
  _dbRegion* region = _region_tbl->getPtr(id);
  return region->_next_child;
}

dbObject* dbRegionItr::getObject(uint id, ...)
{
  return _region_tbl->getPtr(id);
}

}  // namespace odb
