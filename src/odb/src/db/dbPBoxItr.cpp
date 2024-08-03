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

#include "dbPBoxItr.h"

#include "dbMPin.h"
#include "dbMaster.h"
#include "dbPBox.h"
#include "dbTable.h"

namespace odb {

bool dbPBoxItr::reversible()
{
  return true;
}

bool dbPBoxItr::orderReversed()
{
  return true;
}

void dbPBoxItr::reverse(dbObject* parent)
{
  switch (parent->getImpl()->getType()) {
    case dbMasterObj: {
      _dbMaster* master = (_dbMaster*) parent;
      uint id = master->_poly_obstructions;
      uint list = 0;

      while (id != 0) {
        _dbPBox* b = pbox_tbl_->getPtr(id);
        uint n = b->next_pbox_;
        b->next_pbox_ = list;
        list = id;
        id = n;
      }

      master->_poly_obstructions = list;
      break;
    }

    case dbMPinObj: {
      _dbMPin* pin = (_dbMPin*) parent;
      uint id = pin->_poly_geoms;
      uint list = 0;

      while (id != 0) {
        _dbPBox* b = pbox_tbl_->getPtr(id);
        uint n = b->next_pbox_;
        b->next_pbox_ = list;
        list = id;
        id = n;
      }

      pin->_poly_geoms = list;
      break;
    }

    default:
      break;
  }
}

uint dbPBoxItr::sequential()
{
  return 0;
}

uint dbPBoxItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbPBoxItr::begin(parent); id != dbPBoxItr::end(parent);
       id = dbPBoxItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbPBoxItr::begin(dbObject* parent)
{
  switch (parent->getImpl()->getType()) {
    case dbMasterObj: {
      _dbMaster* master = (_dbMaster*) parent;
      return master->_poly_obstructions;
    }

    case dbMPinObj: {
      _dbMPin* pin = (_dbMPin*) parent;
      return pin->_poly_geoms;
    }

    default:
      break;
  }

  return 0;
}

uint dbPBoxItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbPBoxItr::next(uint id, ...)
{
  _dbPBox* box = pbox_tbl_->getPtr(id);
  return box->next_pbox_;
}

dbObject* dbPBoxItr::getObject(uint id, ...)
{
  return pbox_tbl_->getPtr(id);
}

}  // namespace odb
