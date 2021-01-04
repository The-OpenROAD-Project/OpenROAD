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

#include "dbBPinItr.h"

#include "dbBPin.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbBPinItr - Methods
//
////////////////////////////////////////////////////////////////////

//
// BPins are ordered by io-type and cannot be reversed.
//
bool dbBPinItr::reversible()
{
  return true;
}

bool dbBPinItr::orderReversed()
{
  return true;
}

void dbBPinItr::reverse(dbObject* parent)
{
  _dbBTerm* bterm = (_dbBTerm*) parent;
  uint      id    = bterm->_bpins;
  uint      list  = 0;

  while (id != 0) {
    _dbBPin* bpin    = _bpin_tbl->getPtr(id);
    uint     n       = bpin->_next_bpin;
    bpin->_next_bpin = list;
    list             = id;
    id               = n;
  }

  bterm->_bpins = list;
}

uint dbBPinItr::sequential()
{
  return 0;
}

uint dbBPinItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbBPinItr::begin(parent); id != dbBPinItr::end(parent);
       id = dbBPinItr::next(id))
    ++cnt;

  return cnt;
}

uint dbBPinItr::begin(dbObject* parent)
{
  _dbBTerm* bterm = (_dbBTerm*) parent;
  return bterm->_bpins;
}

uint dbBPinItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbBPinItr::next(uint id, ...)
{
  _dbBPin* bpin = _bpin_tbl->getPtr(id);
  return bpin->_next_bpin;
}

dbObject* dbBPinItr::getObject(uint id, ...)
{
  return _bpin_tbl->getPtr(id);
}

}  // namespace odb
