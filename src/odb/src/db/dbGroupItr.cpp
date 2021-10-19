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
#include "dbGroupItr.h"

#include "dbGroup.h"
#include "dbTable.h"
// User Code Begin Includes
#include "dbModule.h"
// User Code End Includes

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbGroupItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbGroupItr::reversible()
{
  return true;
}

bool dbGroupItr::orderReversed()
{
  return true;
}

void dbGroupItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbGroup* _parent = (_dbGroup*) parent;
  uint id = _parent->_groups;
  uint list = 0;

  while (id != 0) {
    _dbGroup* _child = _group_tbl->getPtr(id);
    uint n = _child->_group_next;
    _child->_group_next = list;
    list = id;
    id = n;
  }
  _parent->_groups = list;
  // User Code End reverse
}

uint dbGroupItr::sequential()
{
  return 0;
}

uint dbGroupItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbGroupItr::begin(parent); id != dbGroupItr::end(parent);
       id = dbGroupItr::next(id))
    ++cnt;

  return cnt;
}

uint dbGroupItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbGroup* _parent = (_dbGroup*) parent;
  return _parent->_groups;
  // User Code End begin
}

uint dbGroupItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbGroupItr::next(uint id, ...)
{
  // User Code Begin next
  _dbGroup* _child = _group_tbl->getPtr(id);
  return _child->_group_next;
  // User Code End next
}

dbObject* dbGroupItr::getObject(uint id, ...)
{
  return _group_tbl->getPtr(id);
}
// User Code Begin Methods
// User Code End Methods
}  // namespace odb
   // Generator Code End Cpp