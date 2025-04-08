// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbMPin.h"

#include <vector>

#include "dbAccessPoint.h"
#include "dbBlock.h"
#include "dbBoxItr.h"
#include "dbMPinItr.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbPolygonItr.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"

namespace odb {

template class dbTable<_dbMTerm>;

_dbMPin::_dbMPin(_dbDatabase* db)
{
}

_dbMPin::_dbMPin(_dbDatabase* db, const _dbMPin& p)
    : _mterm(p._mterm), _geoms(p._geoms), _next_mpin(p._next_mpin)
{
}

_dbMPin::~_dbMPin()
{
}

dbOStream& operator<<(dbOStream& stream, const _dbMPin& mpin)
{
  stream << mpin._mterm;
  stream << mpin._geoms;
  stream << mpin._poly_geoms;
  stream << mpin._next_mpin;
  stream << mpin.aps_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbMPin& mpin)
{
  stream >> mpin._mterm;
  stream >> mpin._geoms;
  _dbDatabase* db = mpin.getImpl()->getDatabase();
  if (db->isSchema(db_schema_polygon)) {
    stream >> mpin._poly_geoms;
  }
  stream >> mpin._next_mpin;
  stream >> mpin.aps_;
  return stream;
}

bool _dbMPin::operator==(const _dbMPin& rhs) const
{
  if (_mterm != rhs._mterm) {
    return false;
  }

  if (_geoms != rhs._geoms) {
    return false;
  }

  if (_next_mpin != rhs._next_mpin) {
    return false;
  }

  if (aps_ != rhs.aps_) {
    return false;
  }

  return true;
}

void _dbMPin::addAccessPoint(uint idx, _dbAccessPoint* ap)
{
  if (aps_.size() <= idx) {
    aps_.resize(idx + 1);
  }
  aps_[idx].push_back(ap->getOID());
}

////////////////////////////////////////////////////////////////////
//
// dbMPin - Methods
//
////////////////////////////////////////////////////////////////////

dbMTerm* dbMPin::getMTerm()
{
  _dbMPin* pin = (_dbMPin*) this;
  _dbMaster* master = (_dbMaster*) pin->getOwner();
  return (dbMTerm*) master->_mterm_tbl->getPtr(pin->_mterm);
}

dbMaster* dbMPin::getMaster()
{
  return (dbMaster*) getImpl()->getOwner();
}

dbSet<dbBox> dbMPin::getGeometry(bool include_decomposed_polygons)
{
  _dbMPin* pin = (_dbMPin*) this;
  _dbMaster* master = (_dbMaster*) pin->getOwner();
  if (include_decomposed_polygons) {
    return dbSet<dbBox>(pin, master->_box_itr);
  }
  return dbSet<dbBox>(pin, master->_pbox_box_itr);
}

dbSet<dbPolygon> dbMPin::getPolygonGeometry()
{
  _dbMPin* pin = (_dbMPin*) this;
  _dbMaster* master = (_dbMaster*) pin->getOwner();
  return dbSet<dbPolygon>(pin, master->_pbox_itr);
}

Rect dbMPin::getBBox()
{
  Rect bbox;
  bbox.mergeInit();
  for (dbBox* box : getGeometry()) {
    Rect rect = box->getBox();
    bbox.merge(rect);
  }
  return bbox;
}

std::vector<std::vector<odb::dbAccessPoint*>> dbMPin::getPinAccess() const
{
  _dbMPin* pin = (_dbMPin*) this;
  // TODO: fix for multi chip block heirarchy
  _dbBlock* block = (_dbBlock*) getDb()->getChip()->getBlock();
  std::vector<std::vector<odb::dbAccessPoint*>> result;
  for (const auto& pa : pin->aps_) {
    result.push_back(std::vector<odb::dbAccessPoint*>());
    for (const auto& ap : pa) {
      result.back().push_back((dbAccessPoint*) block->ap_tbl_->getPtr(ap));
    }
  }
  return result;
}

dbMPin* dbMPin::create(dbMTerm* mterm_)
{
  _dbMTerm* mterm = (_dbMTerm*) mterm_;
  _dbMaster* master = (_dbMaster*) mterm->getOwner();
  _dbMPin* mpin = master->_mpin_tbl->create();
  mpin->_mterm = mterm->getOID();
  mpin->_next_mpin = mterm->_pins;
  mterm->_pins = mpin->getOID();
  return (dbMPin*) mpin;
}

dbMPin* dbMPin::getMPin(dbMaster* master_, uint dbid_)
{
  _dbMaster* master = (_dbMaster*) master_;
  return (dbMPin*) master->_mpin_tbl->getPtr(dbid_);
}

void _dbMPin::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  MemInfo& ap_info = info.children_["aps"];
  for (const auto& v : aps_) {
    ap_info.add(v);
  }
}
}  // namespace odb
