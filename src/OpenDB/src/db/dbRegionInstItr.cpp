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

#include "dbRegionInstItr.h"

#include "dbBlock.h"
#include "dbInst.h"
#include "dbRegion.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////
//
// dbRegionInstItr - Methods
//
////////////////////////////////////////////////

bool dbRegionInstItr::reversible()
{
  return true;
}

bool dbRegionInstItr::orderReversed()
{
  return true;
}

void dbRegionInstItr::reverse(dbObject* parent)
{
  _dbRegion* region = (_dbRegion*) parent;
  uint       id     = region->_insts;
  uint       list   = 0;

  while (id != 0) {
    _dbInst* inst      = _inst_tbl->getPtr(id);
    uint     n         = inst->_region_next;
    inst->_region_next = list;
    list               = id;
    id                 = n;
  }

  uint prev = 0;
  id        = list;

  while (id != 0) {
    _dbInst* inst      = _inst_tbl->getPtr(id);
    inst->_region_prev = prev;
    prev               = id;
    id                 = inst->_region_next;
  }

  region->_insts = list;
}

uint dbRegionInstItr::sequential()
{
  return 0;
}

uint dbRegionInstItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbRegionInstItr::begin(parent); id != dbRegionInstItr::end(parent);
       id = dbRegionInstItr::next(id))
    ++cnt;

  return cnt;
}

uint dbRegionInstItr::begin(dbObject* parent)
{
  _dbRegion* region = (_dbRegion*) parent;
  return (uint) region->_insts;
}

uint dbRegionInstItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbRegionInstItr::next(uint id, ...)
{
  _dbInst* inst = _inst_tbl->getPtr(id);
  return inst->_region_next;
}

dbObject* dbRegionInstItr::getObject(uint id, ...)
{
  return _inst_tbl->getPtr(id);
}

}  // namespace odb
