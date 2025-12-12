// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbSBoxItr.h"

#include "dbBlock.h"
#include "dbSBox.h"
#include "dbSWire.h"
#include "dbTable.h"
#include "odb/dbObject.h"

namespace odb {

bool dbSBoxItr::reversible() const
{
  return true;
}

bool dbSBoxItr::orderReversed() const
{
  return true;
}

void dbSBoxItr::reverse(dbObject* parent)
{
  switch (parent->getImpl()->getType()) {
    case dbSWireObj: {
      _dbSWire* wire = (_dbSWire*) parent;
      uint id = wire->wires_;
      uint list = 0;

      while (id != 0) {
        _dbSBox* b = box_tbl_->getPtr(id);
        uint n = b->next_box_;
        b->next_box_ = list;
        list = id;
        id = n;
      }

      wire->wires_ = list;
      break;
    }

    default:
      break;
  }
}

uint dbSBoxItr::sequential() const
{
  return 0;
}

uint dbSBoxItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbSBoxItr::begin(parent); id != dbSBoxItr::end(parent);
       id = dbSBoxItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbSBoxItr::begin(dbObject* parent) const
{
  switch (parent->getImpl()->getType()) {
    case dbSWireObj: {
      _dbSWire* wire = (_dbSWire*) parent;
      return wire->wires_;
    }

    default:
      break;
  }

  return 0;
}

uint dbSBoxItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbSBoxItr::next(uint id, ...) const
{
  _dbSBox* box = box_tbl_->getPtr(id);
  return box->next_box_;
}

dbObject* dbSBoxItr::getObject(uint id, ...)
{
  return box_tbl_->getPtr(id);
}

}  // namespace odb
