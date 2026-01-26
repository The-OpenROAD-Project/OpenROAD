// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBoxItr.h"

#include <cassert>
#include <cstdint>

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
#include "dbTechVia.h"
#include "dbVia.h"
#include "odb/dbId.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"

namespace odb {

template <uint32_t page_size>
bool dbBoxItr<page_size>::reversible() const
{
  return true;
}

template <uint32_t page_size>
bool dbBoxItr<page_size>::orderReversed() const
{
  return true;
}

template <uint32_t page_size>
void dbBoxItr<page_size>::reverse(dbObject* parent)
{
  switch (parent->getImpl()->getType()) {
    case dbRegionObj: {
      _dbRegion* region = (_dbRegion*) parent;
      uint32_t id = region->boxes_;
      uint32_t list = 0;

      while (id != 0) {
        _dbBox* b = box_tbl_->getPtr(id);
        uint32_t n = b->next_box_;
        b->next_box_ = list;
        list = id;
        id = n;
      }

      region->boxes_ = list;
      break;
    }

    case dbBTermObj:
      break;

    case dbViaObj: {
      _dbVia* via = (_dbVia*) parent;
      uint32_t id = via->boxes_;
      uint32_t list = 0;

      while (id != 0) {
        _dbBox* b = box_tbl_->getPtr(id);
        uint32_t n = b->next_box_;
        b->next_box_ = list;
        list = id;
        id = n;
      }

      via->boxes_ = list;
      break;
    }

    case dbMasterObj: {
      _dbMaster* master = (_dbMaster*) parent;
      uint32_t id = master->obstructions_;
      uint32_t list = 0;

      while (id != 0) {
        _dbBox* b = box_tbl_->getPtr(id);
        uint32_t n = b->next_box_;
        b->next_box_ = list;
        list = id;
        id = n;
      }

      master->obstructions_ = list;
      break;
    }

    case dbMPinObj: {
      _dbMPin* pin = (_dbMPin*) parent;
      uint32_t id = pin->geoms_;
      uint32_t list = 0;

      while (id != 0) {
        _dbBox* b = box_tbl_->getPtr(id);
        uint32_t n = b->next_box_;
        b->next_box_ = list;
        list = id;
        id = n;
      }

      pin->geoms_ = list;
      break;
    }

    case dbTechViaObj: {
      _dbTechVia* via = (_dbTechVia*) parent;
      uint32_t id = via->boxes_;
      uint32_t list = 0;

      while (id != 0) {
        _dbBox* b = box_tbl_->getPtr(id);
        uint32_t n = b->next_box_;
        b->next_box_ = list;
        list = id;
        id = n;
      }

      via->boxes_ = list;
      break;
    }

    case dbBPinObj: {
      _dbBPin* bpin = (_dbBPin*) parent;
      uint32_t id = bpin->boxes_;
      uint32_t list = 0;

      while (id != 0) {
        _dbBox* b = box_tbl_->getPtr(id);
        uint32_t n = b->next_box_;
        b->next_box_ = list;
        list = id;
        id = n;
      }

      bpin->boxes_ = list;
      break;
    }

    case dbPolygonObj: {
      _dbPolygon* pbox = (_dbPolygon*) parent;
      uint32_t id = pbox->boxes_;
      uint32_t list = 0;

      while (id != 0) {
        _dbBox* b = box_tbl_->getPtr(id);
        uint32_t n = b->next_box_;
        b->next_box_ = list;
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

template <uint32_t page_size>
uint32_t dbBoxItr<page_size>::sequential() const
{
  return 0;
}

template <uint32_t page_size>
uint32_t dbBoxItr<page_size>::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbBoxItr::begin(parent); id != dbBoxItr::end(parent);
       id = dbBoxItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

template <uint32_t page_size>
uint32_t dbBoxItr<page_size>::begin(dbObject* parent) const
{
  switch (parent->getImpl()->getType()) {
    case dbRegionObj: {
      _dbRegion* region = (_dbRegion*) parent;
      return region->boxes_;
    }

    case dbViaObj: {
      _dbVia* via = (_dbVia*) parent;
      return via->boxes_;
    }

    case dbMasterObj: {
      _dbMaster* master = (_dbMaster*) parent;
      if (include_polygons_ && master->poly_obstructions_) {
        dbId<_dbPolygon> pid = master->poly_obstructions_;
        _dbPolygon* pbox = pbox_tbl_->getPtr(pid);
        while (pbox != nullptr && pbox->boxes_ == 0) {
          // move to next pbox
          pid = pbox->next_pbox_;
          pbox = pbox_tbl_->getPtr(pid);
        }
        if (pbox != nullptr) {
          return pbox->boxes_;
        }
      }
      return master->obstructions_;
    }

    case dbMPinObj: {
      _dbMPin* pin = (_dbMPin*) parent;
      if (include_polygons_ && pin->poly_geoms_) {
        dbId<_dbPolygon> pid = pin->poly_geoms_;
        _dbPolygon* pbox = pbox_tbl_->getPtr(pid);
        while (pbox != nullptr && pbox->boxes_ == 0) {
          // move to next pbox
          pid = pbox->next_pbox_;
          pbox = pbox_tbl_->getPtr(pid);
        }
        if (pbox != nullptr) {
          return pbox->boxes_;
        }
      }
      return pin->geoms_;
    }

    case dbTechViaObj: {
      _dbTechVia* via = (_dbTechVia*) parent;
      return via->boxes_;
    }

    case dbBPinObj: {
      _dbBPin* pin = (_dbBPin*) parent;
      return pin->boxes_;
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

template <uint32_t page_size>
uint32_t dbBoxItr<page_size>::end(dbObject* /* unused: parent */) const
{
  return 0;
}

template <uint32_t page_size>
uint32_t dbBoxItr<page_size>::next(uint32_t id, ...) const
{
  _dbBox* box = box_tbl_->getPtr(id);

  if (!include_polygons_ || box->next_box_ != 0) {
    // return next box if available or when not considering polygons
    return box->next_box_;
  }

  if (box->flags_.owner_type == dbBoxOwner::PBOX) {
    // if owner is dbPolygon need to check for next dbPolygon
    dbId<_dbPolygon> pid = box->owner_;
    _dbPolygon* box_pbox = pbox_tbl_->getPtr(pid);

    _dbPolygon* pbox = box_pbox;
    if (pbox->next_pbox_ != 0) {
      // move to next pbox
      pbox = pbox_tbl_->getPtr(pbox->next_pbox_);
      while (pbox != nullptr && pbox->boxes_ == 0) {
        // move to next pbox
        pid = pbox->next_pbox_;
        pbox = pbox_tbl_->getPtr(pid);
      }
      if (pbox != nullptr) {
        return pbox->boxes_;
      }
    }

    // end of polygons

    // return non-polygon list from owner
    if (box_pbox->flags_.owner_type == dbBoxOwner::MASTER) {
      _dbMaster* master = (_dbMaster*) box_pbox->getOwner();
      return master->obstructions_;
    }
    if (box_pbox->flags_.owner_type == dbBoxOwner::MPIN) {
      _dbMaster* master = (_dbMaster*) box_pbox->getOwner();
      _dbMPin* pin = (_dbMPin*) master->mpin_tbl_->getPtr(box_pbox->owner_);
      return pin->geoms_;
    }

    // this should not be possible unless new types with polygons are added
    assert(0);
  }

  return 0;
}

template <uint32_t page_size>
dbObject* dbBoxItr<page_size>::getObject(uint32_t id, ...)
{
  return box_tbl_->getPtr(id);
}

template class dbBoxItr<8>;
template class dbBoxItr<128>;
template class dbBoxItr<1024>;

}  // namespace odb
