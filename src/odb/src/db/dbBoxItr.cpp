// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBoxItr.h"

#include "dbBPin.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbBox.h"
#include "dbMPin.h"
#include "dbMaster.h"
#include "dbNet.h"
#include "dbPolygon.h"
#include "dbRegion.h"
#include "dbTable.h"
#include "dbTable.hpp"
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

    case dbPolygonObj: {
      _dbPolygon* pbox = (_dbPolygon*) parent;
      uint id = pbox->boxes_;
      uint list = 0;

      while (id != 0) {
        _dbBox* b = _box_tbl->getPtr(id);
        uint n = b->_next_box;
        b->_next_box = list;
        list = id;
        id = n;
      }

      pbox->boxes_ = list;
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
       id = dbBoxItr::next(id)) {
    ++cnt;
  }

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
      if (include_polygons_ && master->_poly_obstructions) {
        dbId<_dbPolygon> pid = master->_poly_obstructions;
        _dbPolygon* pbox = _pbox_tbl->getPtr(pid);
        while (pbox != nullptr && pbox->boxes_ == 0) {
          // move to next pbox
          pid = pbox->next_pbox_;
          pbox = _pbox_tbl->getPtr(pid);
        }
        if (pbox != nullptr) {
          return pbox->boxes_;
        }
      }
      return master->_obstructions;
    }

    case dbMPinObj: {
      _dbMPin* pin = (_dbMPin*) parent;
      if (include_polygons_ && pin->_poly_geoms) {
        dbId<_dbPolygon> pid = pin->_poly_geoms;
        _dbPolygon* pbox = _pbox_tbl->getPtr(pid);
        while (pbox != nullptr && pbox->boxes_ == 0) {
          // move to next pbox
          pid = pbox->next_pbox_;
          pbox = _pbox_tbl->getPtr(pid);
        }
        if (pbox != nullptr) {
          return pbox->boxes_;
        }
      }
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

    case dbPolygonObj: {
      _dbPolygon* box = (_dbPolygon*) parent;
      return box->boxes_;
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

  if (!include_polygons_ || box->_next_box != 0) {
    // return next box if available or when not considering polygons
    return box->_next_box;
  }

  if (box->_flags._owner_type == dbBoxOwner::PBOX) {
    // if owner is dbPolygon need to check for next dbPolygon
    dbId<_dbPolygon> pid = box->_owner;
    _dbPolygon* box_pbox = _pbox_tbl->getPtr(pid);

    _dbPolygon* pbox = box_pbox;
    if (pbox->next_pbox_ != 0) {
      // move to next pbox
      pbox = _pbox_tbl->getPtr(pbox->next_pbox_);
      while (pbox != nullptr && pbox->boxes_ == 0) {
        // move to next pbox
        pid = pbox->next_pbox_;
        pbox = _pbox_tbl->getPtr(pid);
      }
      if (pbox != nullptr) {
        return pbox->boxes_;
      }
    }

    // end of polygons

    // return non-polygon list from owner
    if (box_pbox->flags_.owner_type_ == dbBoxOwner::MASTER) {
      _dbMaster* master = (_dbMaster*) box_pbox->getOwner();
      return master->_obstructions;
    }
    if (box_pbox->flags_.owner_type_ == dbBoxOwner::MPIN) {
      _dbMaster* master = (_dbMaster*) box_pbox->getOwner();
      _dbMPin* pin = (_dbMPin*) master->_mpin_tbl->getPtr(box_pbox->owner_);
      return pin->_geoms;
    }

    // this should not be possible unless new types with polygons are added
    ZASSERT(0);
  }

  return 0;
}

dbObject* dbBoxItr::getObject(uint id, ...)
{
  return _box_tbl->getPtr(id);
}

}  // namespace odb
