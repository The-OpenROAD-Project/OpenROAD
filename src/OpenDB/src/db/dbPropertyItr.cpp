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

#include "dbPropertyItr.h"

#include "dbProperty.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbPropertyItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbPropertyItr::reversible()
{
  return true;
}

bool dbPropertyItr::orderReversed()
{
  return true;
}

void dbPropertyItr::reverse(dbObject* parent)
{
  _dbObject*     parent_impl = parent->getImpl();
  dbObjectTable* table       = parent_impl->getTable();
  uint           id          = table->getPropList(parent_impl->getOID());

  if (id) {
    uint list = 0;

    while (id != 0) {
      _dbProperty* p = _prop_tbl->getPtr(id);
      uint         n = p->_next;
      p->_next       = list;
      list           = id;
      id             = n;
    }

    table->setPropList(parent_impl->getOID(), list);
  }
}

uint dbPropertyItr::sequential()
{
  return 0;
}

uint dbPropertyItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbPropertyItr::begin(parent); id != dbPropertyItr::end(parent);
       id = dbPropertyItr::next(id))
    ++cnt;

  return cnt;
}

uint dbPropertyItr::begin(dbObject* parent)
{
  dbObjectTable* table = parent->getImpl()->getTable();
  uint           id    = table->getPropList(parent->getImpl()->getOID());
  return id;
}

uint dbPropertyItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbPropertyItr::next(uint id, ...)
{
  _dbProperty* prop = _prop_tbl->getPtr(id);
  return prop->_next;
}

dbObject* dbPropertyItr::getObject(uint id, ...)
{
  return _prop_tbl->getPtr(id);
}

}  // namespace odb
