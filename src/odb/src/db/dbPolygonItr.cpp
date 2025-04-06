// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "dbPolygonItr.h"

#include "dbMPin.h"
#include "dbMaster.h"
#include "dbPolygon.h"
#include "dbTable.h"

namespace odb {

bool dbPolygonItr::reversible()
{
  return true;
}

bool dbPolygonItr::orderReversed()
{
  return true;
}

void dbPolygonItr::reverse(dbObject* parent)
{
  switch (parent->getImpl()->getType()) {
    case dbMasterObj: {
      _dbMaster* master = (_dbMaster*) parent;
      uint id = master->_poly_obstructions;
      uint list = 0;

      while (id != 0) {
        _dbPolygon* b = pbox_tbl_->getPtr(id);
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
        _dbPolygon* b = pbox_tbl_->getPtr(id);
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

uint dbPolygonItr::sequential()
{
  return 0;
}

uint dbPolygonItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbPolygonItr::begin(parent); id != dbPolygonItr::end(parent);
       id = dbPolygonItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbPolygonItr::begin(dbObject* parent)
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

uint dbPolygonItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbPolygonItr::next(uint id, ...)
{
  _dbPolygon* box = pbox_tbl_->getPtr(id);
  return box->next_pbox_;
}

dbObject* dbPolygonItr::getObject(uint id, ...)
{
  return pbox_tbl_->getPtr(id);
}

}  // namespace odb
