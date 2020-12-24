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

#include "dbSWireItr.h"

#include "dbBlock.h"
#include "dbNet.h"
#include "dbSWire.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbSWireItr - Methods
//
////////////////////////////////////////////////////////////////////

//
// BTerms are ordered by io-type and cannot be reversed.
//
bool dbSWireItr::reversible()
{
  return true;
}

bool dbSWireItr::orderReversed()
{
  return true;
}

void dbSWireItr::reverse(dbObject* parent)
{
  _dbNet* net  = (_dbNet*) parent;
  uint    id   = net->_swires;
  uint    list = 0;

  while (id != 0) {
    _dbSWire* swire    = _swire_tbl->getPtr(id);
    uint      n        = swire->_next_swire;
    swire->_next_swire = list;
    list               = id;
    id                 = n;
  }

  net->_swires = list;
}

uint dbSWireItr::sequential()
{
  return 0;
}

uint dbSWireItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbSWireItr::begin(parent); id != dbSWireItr::end(parent);
       id = dbSWireItr::next(id))
    ++cnt;

  return cnt;
}

uint dbSWireItr::begin(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  return net->_swires;
}

uint dbSWireItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbSWireItr::next(uint id, ...)
{
  _dbSWire* swire = _swire_tbl->getPtr(id);
  return swire->_next_swire;
}

dbObject* dbSWireItr::getObject(uint id, ...)
{
  return _swire_tbl->getPtr(id);
}

}  // namespace odb
