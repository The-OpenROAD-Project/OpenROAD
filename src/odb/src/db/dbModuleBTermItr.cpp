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
#include "dbModuleBTermItr.h"

#include "dbModBTerm.h"
#include "dbModBTerm.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleBTermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleBTermItr::reversible()
{
  return true;
}

bool dbModuleBTermItr::orderReversed()
{
  return true;
}

void dbModuleBTermItr::reverse(dbObject* parent)
{
}

uint dbModuleBTermItr::sequential()
{
  return 0;
}

uint dbModuleBTermItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbModuleBTermItr::begin(parent);
       id != dbModuleBTermItr::end(parent);
       id = dbModuleBTermItr::next(id))
    ++cnt;

  return cnt;
}

uint dbModuleBTermItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbModule* _module = (_dbModule*)parent;
  _dbModBTerm* _modbterm = _module-> _modbterms;
  // User Code End begin
}

uint dbModuleBTermItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbModuleBTermItr::next(uint id, ...)
{
  // User Code Begin next
  _dbModITerm* modbterm = _modbterm_tbl->getPtr(id);
  return modbterm->_next_entry;
  // User Code End next  
}

dbObject* dbModuleBTermItr::getObject(uint id, ...)
{
  return _modbterm_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
