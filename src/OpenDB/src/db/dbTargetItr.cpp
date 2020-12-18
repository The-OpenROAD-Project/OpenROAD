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

#include "dbTargetItr.h"

#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "dbTarget.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbTargetItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbTargetItr::reversible()
{
  return true;
}

bool dbTargetItr::orderReversed()
{
  return true;
}

void dbTargetItr::reverse(dbObject* parent)
{
  _dbMTerm* mterm = (_dbMTerm*) parent;
  uint      id    = mterm->_targets;
  uint      list  = 0;

  while (id != 0) {
    _dbTarget* target = _target_tbl->getPtr(id);
    uint       n      = target->_next;
    target->_next     = list;
    list              = id;
    id                = n;
  }

  mterm->_targets = list;
}

uint dbTargetItr::sequential()
{
  return 0;
}

uint dbTargetItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbTargetItr::begin(parent); id != dbTargetItr::end(parent);
       id = dbTargetItr::next(id))
    ++cnt;

  return cnt;
}

uint dbTargetItr::begin(dbObject* parent)
{
  _dbMTerm* mterm = (_dbMTerm*) parent;
  return mterm->_targets;
}

uint dbTargetItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbTargetItr::next(uint id, ...)
{
  _dbTarget* target = _target_tbl->getPtr(id);
  return target->_next;
}

dbObject* dbTargetItr::getObject(uint id, ...)
{
  return _target_tbl->getPtr(id);
}

}  // namespace odb
