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
#include "dbModInstModITermItr.h"

#include "dbModITerm.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModInstModITermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModInstModITermItr::reversible()
{
  return true;
}

bool dbModInstModITermItr::orderReversed()
{
  return true;
}

void dbModInstModITermItr::reverse(dbObject* parent)
{
}

uint dbModInstModITermItr::sequential()
{
  return 0;
}

uint dbModInstModITermItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbModInstModITermItr::begin(parent);
       id != dbModInstModITermItr::end(parent);
       id = dbModInstModITermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModInstModITermItr::begin(dbObject* parent)
{
}

uint dbModInstModITermItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbModInstModITermItr::next(uint id, ...)
{
}

dbObject* dbModInstModITermItr::getObject(uint id, ...)
{
  return _moditerm_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp