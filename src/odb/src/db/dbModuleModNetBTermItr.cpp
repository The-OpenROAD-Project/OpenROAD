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
#include "dbModuleModNetBTermItr.h"

#include "dbBTerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModNetBTermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModNetBTermItr::reversible()
{
  return true;
}

bool dbModuleModNetBTermItr::orderReversed()
{
  return true;
}

void dbModuleModNetBTermItr::reverse(dbObject* parent)
{
}

uint dbModuleModNetBTermItr::sequential()
{
  return 0;
}

uint dbModuleModNetBTermItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbModuleModNetBTermItr::begin(parent);
       id != dbModuleModNetBTermItr::end(parent);
       id = dbModuleModNetBTermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModuleModNetBTermItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbModNet* mod_net = (_dbModNet*) parent;
  return mod_net->_bterms;
  // User Code End begin
}

uint dbModuleModNetBTermItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbModuleModNetBTermItr::next(uint id, ...)
{
  // User Code Begin next
  _dbBTerm* _bterm = _bterm_tbl->getPtr(id);
  return _bterm->_next_modnet_bterm;
  // User Code End next
}

dbObject* dbModuleModNetBTermItr::getObject(uint id, ...)
{
  return _bterm_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
