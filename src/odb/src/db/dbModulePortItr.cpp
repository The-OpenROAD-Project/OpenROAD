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
#include "dbModulePortItr.h"

#include "dbBlock.h"
#include "dbBusPort.h"
#include "dbModBTerm.h"
#include "dbModule.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModulePortItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModulePortItr::reversible()
{
  return true;
}

bool dbModulePortItr::orderReversed()
{
  return true;
}

void dbModulePortItr::reverse(dbObject* parent)
{
}

uint dbModulePortItr::sequential()
{
  return 0;
}

uint dbModulePortItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbModulePortItr::begin(parent); id != dbModulePortItr::end(parent);
       id = dbModulePortItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModulePortItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbModule* _module = (_dbModule*) parent;
  return _module->_modbterms;
  // User Code End begin
}

uint dbModulePortItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbModulePortItr::next(uint id, ...)
{
  // User Code Begin next
  _dbModBTerm* modbterm = _modbterm_tbl->getPtr(id);
  if (modbterm->_busPort != 0) {
    _dbBlock* block = (_dbBlock*) modbterm->getOwner();
    _dbBusPort* bus_port = block->_busport_tbl->getPtr(modbterm->_busPort);
    if (bus_port) {
      modbterm = _modbterm_tbl->getPtr(bus_port->_last);
    }
  }
  if (modbterm) {
    return modbterm->_next_entry;
  }
  return 0;
  // User Code End next
}

dbObject* dbModulePortItr::getObject(uint id, ...)
{
  return _modbterm_tbl->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp
