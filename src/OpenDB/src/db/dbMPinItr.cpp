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

#include "dbMPinItr.h"

#include "dbMPin.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbMPinItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbMPinItr::reversible()
{
  return true;
}

bool dbMPinItr::orderReversed()
{
  return true;
}

void dbMPinItr::reverse(dbObject* parent)
{
  _dbMTerm* mterm = (_dbMTerm*) parent;
  uint      id    = mterm->_pins;
  uint      list  = 0;

  while (id != 0) {
    _dbMPin* pin    = _mpin_tbl->getPtr(id);
    uint     n      = pin->_next_mpin;
    pin->_next_mpin = list;
    list            = id;
    id              = n;
  }

  mterm->_pins = list;
}

uint dbMPinItr::sequential()
{
  return 0;
}

uint dbMPinItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbMPinItr::begin(parent); id != dbMPinItr::end(parent);
       id = dbMPinItr::next(id))
    ++cnt;

  return cnt;
}

uint dbMPinItr::begin(dbObject* parent)
{
  _dbMTerm* mterm = (_dbMTerm*) parent;
  return mterm->_pins;
}

uint dbMPinItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbMPinItr::next(uint id, ...)
{
  _dbMPin* mpin = _mpin_tbl->getPtr(id);
  return mpin->_next_mpin;
}

dbObject* dbMPinItr::getObject(uint id, ...)
{
  return _mpin_tbl->getPtr(id);
}

}  // namespace odb
