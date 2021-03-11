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

// Generator Code Begin Cpp
#include "dbGroupInstItr.h"

#include "dbGroup.h"
#include "dbInst.h"
#include "dbTable.h"
// User Code Begin Includes
// User Code End Includes

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbGroupInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbGroupInstItr::reversible()
{
  return true;
}

bool dbGroupInstItr::orderReversed()
{
  return true;
}

void dbGroupInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbGroup* _parent = (_dbGroup*) parent;
  uint      id      = _parent->_insts;
  uint      list    = 0;

  while (id != 0) {
    _dbInst* inst     = _inst_tbl->getPtr(id);
    uint     n        = inst->_group_next;
    inst->_group_next = list;
    list              = id;
    id                = n;
  }
  _parent->_insts = list;
  // User Code End reverse
}

uint dbGroupInstItr::sequential()
{
  return 0;
}

uint dbGroupInstItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbGroupInstItr::begin(parent); id != dbGroupInstItr::end(parent);
       id = dbGroupInstItr::next(id))
    ++cnt;

  return cnt;
}

uint dbGroupInstItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbGroup* _parent = (_dbGroup*) parent;
  return _parent->_insts;
  // User Code End begin
}

uint dbGroupInstItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbGroupInstItr::next(uint id, ...)
{
  // User Code Begin next
  _dbInst* inst = _inst_tbl->getPtr(id);
  return inst->_group_next;
  // User Code End next
}

dbObject* dbGroupInstItr::getObject(uint id, ...)
{
  return _inst_tbl->getPtr(id);
}
// User Code Begin Methods
// User Code End Methods
}  // namespace odb
   // Generator Code End Cpp