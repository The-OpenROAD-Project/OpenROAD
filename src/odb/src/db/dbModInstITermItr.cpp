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
#include "dbModInstITermItr.h"

#include "dbModITerm.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModInstITermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModInstITermItr::reversible()
{
  return true;
}

bool dbModInstITermItr::orderReversed()
{
  return true;
}

void dbModInstITermItr::reverse(dbObject* parent)
{
}

uint dbModInstITermItr::sequential()
{
  return 0;
}

uint dbModInstITermItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbModInstITermItr::begin(parent);
       id != dbModInstITermItr::end(parent);
       id = dbModInstITermItr::next(id))
    ++cnt;

  return cnt;
}

uint dbModInstITermItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbModInst* modinst = (_dbModInst*) parent;
  return modinst->_moditerms;
  // User Code End begin
}

uint dbModInstITermItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbModInstITermItr::next(uint id, ...)
{
  // User Code Begin next
  _dbModInstITerm* iterm = _moditerm_tbl->getPtr(id);
  return iterm->_next_entry;
  // User Code End next
}

dbObject* dbModInstITermItr::getObject(uint id, ...)
{
  return _moditerm_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
