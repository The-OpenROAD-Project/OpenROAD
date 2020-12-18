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

#include "dbSBoxItr.h"

#include "dbBlock.h"
#include "dbSBox.h"
#include "dbSWire.h"
#include "dbTable.h"

namespace odb {

bool dbSBoxItr::reversible()
{
  return true;
}

bool dbSBoxItr::orderReversed()
{
  return true;
}

void dbSBoxItr::reverse(dbObject* parent)
{
  switch (parent->getImpl()->getType()) {
    case dbSWireObj: {
      _dbSWire* wire = (_dbSWire*) parent;
      uint      id   = wire->_wires;
      uint      list = 0;

      while (id != 0) {
        _dbSBox* b   = _box_tbl->getPtr(id);
        uint     n   = b->_next_box;
        b->_next_box = list;
        list         = id;
        id           = n;
      }

      wire->_wires = list;
      break;
    }

    default:
      break;
  }
}

uint dbSBoxItr::sequential()
{
  return 0;
}

uint dbSBoxItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbSBoxItr::begin(parent); id != dbSBoxItr::end(parent);
       id = dbSBoxItr::next(id))
    ++cnt;

  return cnt;
}

uint dbSBoxItr::begin(dbObject* parent)
{
  switch (parent->getImpl()->getType()) {
    case dbSWireObj: {
      _dbSWire* wire = (_dbSWire*) parent;
      return wire->_wires;
    }

    default:
      break;
  }

  return 0;
}

uint dbSBoxItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbSBoxItr::next(uint id, ...)
{
  _dbSBox* box = _box_tbl->getPtr(id);
  return box->_next_box;
}

dbObject* dbSBoxItr::getObject(uint id, ...)
{
  return _box_tbl->getPtr(id);
}

}  // namespace odb
