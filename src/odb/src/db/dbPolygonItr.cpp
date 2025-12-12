// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "dbPolygonItr.h"

#include "dbMPin.h"
#include "dbMaster.h"
#include "dbPolygon.h"
#include "dbTable.hpp"
#include "odb/dbObject.h"

namespace odb {

bool dbPolygonItr::reversible() const
{
  return true;
}

bool dbPolygonItr::orderReversed() const
{
  return true;
}

void dbPolygonItr::reverse(dbObject* parent)
{
  switch (parent->getImpl()->getType()) {
    case dbMasterObj: {
      _dbMaster* master = (_dbMaster*) parent;
      uint id = master->poly_obstructions_;
      uint list = 0;

      while (id != 0) {
        _dbPolygon* b = pbox_tbl_->getPtr(id);
        uint n = b->next_pbox_;
        b->next_pbox_ = list;
        list = id;
        id = n;
      }

      master->poly_obstructions_ = list;
      break;
    }

    case dbMPinObj: {
      _dbMPin* pin = (_dbMPin*) parent;
      uint id = pin->poly_geoms_;
      uint list = 0;

      while (id != 0) {
        _dbPolygon* b = pbox_tbl_->getPtr(id);
        uint n = b->next_pbox_;
        b->next_pbox_ = list;
        list = id;
        id = n;
      }

      pin->poly_geoms_ = list;
      break;
    }

    default:
      break;
  }
}

uint dbPolygonItr::sequential() const
{
  return 0;
}

uint dbPolygonItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbPolygonItr::begin(parent); id != dbPolygonItr::end(parent);
       id = dbPolygonItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbPolygonItr::begin(dbObject* parent) const
{
  switch (parent->getImpl()->getType()) {
    case dbMasterObj: {
      _dbMaster* master = (_dbMaster*) parent;
      return master->poly_obstructions_;
    }

    case dbMPinObj: {
      _dbMPin* pin = (_dbMPin*) parent;
      return pin->poly_geoms_;
    }

    default:
      break;
  }

  return 0;
}

uint dbPolygonItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbPolygonItr::next(uint id, ...) const
{
  _dbPolygon* box = pbox_tbl_->getPtr(id);
  return box->next_pbox_;
}

dbObject* dbPolygonItr::getObject(uint id, ...)
{
  return pbox_tbl_->getPtr(id);
}

}  // namespace odb
