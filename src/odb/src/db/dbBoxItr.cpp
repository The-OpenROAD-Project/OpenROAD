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

#include "dbBoxItr.h"

#include "dbBPin.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbBox.h"
#include "dbMPin.h"
#include "dbMaster.h"
#include "dbNet.h"
#include "dbRegion.h"
#include "dbTable.h"
#include "dbTechVia.h"
#include "dbVia.h"

namespace odb {

bool dbBoxItr::reversible()
{
  return true;
}

bool dbBoxItr::orderReversed()
{
  return true;
}

void dbBoxItr::reverse(dbObject* parent)
{
  switch (parent->getImpl()->getType()) {
    case dbRegionObj: {
      _dbRegion* region = (_dbRegion*) parent;
      uint id = region->_boxes;
      uint list = 0;

      while (id != 0) {
        _dbBox* b = _box_tbl->getPtr(id);
        uint n = b->_next_box;
        b->_next_box = list;
        list = id;
        id = n;
      }

      region->_boxes = list;
      break;
    }

    case dbBTermObj:
      break;

    case dbViaObj: {
      _dbVia* via = (_dbVia*) parent;
      uint id = via->_boxes;
      uint list = 0;

      while (id != 0) {
        _dbBox* b = _box_tbl->getPtr(id);
        uint n = b->_next_box;
        b->_next_box = list;
        list = id;
        id = n;
      }

      via->_boxes = list;
      break;
    }

    case dbMasterObj: {
      _dbMaster* master = (_dbMaster*) parent;
      uint id = master->_obstructions;
      uint list = 0;

      while (id != 0) {
        _dbBox* b = _box_tbl->getPtr(id);
        uint n = b->_next_box;
        b->_next_box = list;
        list = id;
        id = n;
      }

      master->_obstructions = list;
      break;
    }

    case dbMPinObj: {
      _dbMPin* pin = (_dbMPin*) parent;
      uint id = pin->_geoms;
      uint list = 0;

      while (id != 0) {
        _dbBox* b = _box_tbl->getPtr(id);
        uint n = b->_next_box;
        b->_next_box = list;
        list = id;
        id = n;
      }

      pin->_geoms = list;
      break;
    }

    case dbTechViaObj: {
      _dbTechVia* via = (_dbTechVia*) parent;
      uint id = via->_boxes;
      uint list = 0;

      while (id != 0) {
        _dbBox* b = _box_tbl->getPtr(id);
        uint n = b->_next_box;
        b->_next_box = list;
        list = id;
        id = n;
      }

      via->_boxes = list;
      break;
    }

    case dbBPinObj: {
      _dbBPin* bpin = (_dbBPin*) parent;
      uint id = bpin->_boxes;
      uint list = 0;

      while (id != 0) {
        _dbBox* b = _box_tbl->getPtr(id);
        uint n = b->_next_box;
        b->_next_box = list;
        list = id;
        id = n;
      }

      bpin->_boxes = list;
      break;
    }

    default:
      break;
  }
}

uint dbBoxItr::sequential()
{
  return 0;
}

uint dbBoxItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbBoxItr::begin(parent); id != dbBoxItr::end(parent);
       id = dbBoxItr::next(id))
    ++cnt;

  return cnt;
}

uint dbBoxItr::begin(dbObject* parent)
{
  switch (parent->getImpl()->getType()) {
    case dbRegionObj: {
      _dbRegion* region = (_dbRegion*) parent;
      return region->_boxes;
    }

    case dbViaObj: {
      _dbVia* via = (_dbVia*) parent;
      return via->_boxes;
    }

    case dbMasterObj: {
      _dbMaster* master = (_dbMaster*) parent;
      return master->_obstructions;
    }

    case dbMPinObj: {
      _dbMPin* pin = (_dbMPin*) parent;
      return pin->_geoms;
    }

    case dbTechViaObj: {
      _dbTechVia* via = (_dbTechVia*) parent;
      return via->_boxes;
    }

    case dbBPinObj: {
      _dbBPin* pin = (_dbBPin*) parent;
      return pin->_boxes;
    }

    default:
      break;
  }

  return 0;
}

uint dbBoxItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbBoxItr::next(uint id, ...)
{
  _dbBox* box = _box_tbl->getPtr(id);
  return box->_next_box;
}

dbObject* dbBoxItr::getObject(uint id, ...)
{
  return _box_tbl->getPtr(id);
}

}  // namespace odb
