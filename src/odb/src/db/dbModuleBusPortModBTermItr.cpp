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
#include "dbModuleBusPortModBTermItr.h"

#include "dbBusPort.h"
#include "dbModBTerm.h"
#include "dbModule.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleBusPortModBTermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleBusPortModBTermItr::reversible()
{
  return true;
}

bool dbModuleBusPortModBTermItr::orderReversed()
{
  return true;
}

void dbModuleBusPortModBTermItr::reverse(dbObject* parent)
{
}

uint dbModuleBusPortModBTermItr::sequential()
{
  return 0;
}

uint dbModuleBusPortModBTermItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbModuleBusPortModBTermItr::begin(parent);
       id != dbModuleBusPortModBTermItr::end(parent);
       id = dbModuleBusPortModBTermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModuleBusPortModBTermItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbBusPort* _busport = (_dbBusPort*) parent;
  _iter = _modbterm_tbl->getPtr(_busport->_members);
  _size = abs(_busport->_from - _busport->_to) + 1;
  _ix = 0;
  return _busport->_members;
  // User Code End begin
}

uint dbModuleBusPortModBTermItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbModuleBusPortModBTermItr::next(uint id, ...)
{
  // User Code Begin next
  _dbModBTerm* lmodbterm = _modbterm_tbl->getPtr(id);
  _ix++;
  uint ret = lmodbterm->_next_entry;
  _iter = _modbterm_tbl->getPtr(ret);
  return ret;
  // User Code End next
}

dbObject* dbModuleBusPortModBTermItr::getObject(uint id, ...)
{
  return _modbterm_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
