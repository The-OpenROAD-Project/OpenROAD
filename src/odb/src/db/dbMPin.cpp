// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbMPin.h"

#include <cstdint>
#include <vector>

#include "dbAccessPoint.h"
#include "dbBlock.h"
#include "dbBoxItr.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbMPinItr.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbPolygonItr.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/geom.h"

namespace odb {

template class dbTable<_dbMTerm>;

_dbMPin::_dbMPin(_dbDatabase* db)
{
}

_dbMPin::_dbMPin(_dbDatabase* db, const _dbMPin& p)
    : mterm_(p.mterm_), geoms_(p.geoms_), next_mpin_(p.next_mpin_)
{
}

dbOStream& operator<<(dbOStream& stream, const _dbMPin& mpin)
{
  stream << mpin.mterm_;
  stream << mpin.geoms_;
  stream << mpin.poly_geoms_;
  stream << mpin.next_mpin_;
  stream << mpin.aps_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbMPin& mpin)
{
  stream >> mpin.mterm_;
  stream >> mpin.geoms_;
  _dbDatabase* db = mpin.getImpl()->getDatabase();
  if (db->isSchema(kSchemaPolygon)) {
    stream >> mpin.poly_geoms_;
  }
  stream >> mpin.next_mpin_;
  stream >> mpin.aps_;
  return stream;
}

bool _dbMPin::operator==(const _dbMPin& rhs) const
{
  if (mterm_ != rhs.mterm_) {
    return false;
  }

  if (geoms_ != rhs.geoms_) {
    return false;
  }

  if (next_mpin_ != rhs.next_mpin_) {
    return false;
  }

  if (aps_ != rhs.aps_) {
    return false;
  }

  return true;
}

void _dbMPin::addAccessPoint(uint32_t idx, _dbAccessPoint* ap)
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
  return (dbMTerm*) master->mterm_tbl_->getPtr(pin->mterm_);
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
    return dbSet<dbBox>(pin, master->box_itr_);
  }
  return dbSet<dbBox>(pin, master->pbox_box_itr_);
}

dbSet<dbPolygon> dbMPin::getPolygonGeometry()
{
  _dbMPin* pin = (_dbMPin*) this;
  _dbMaster* master = (_dbMaster*) pin->getOwner();
  return dbSet<dbPolygon>(pin, master->pbox_itr_);
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
    result.emplace_back();
    for (const auto& ap : pa) {
      result.back().push_back((dbAccessPoint*) block->ap_tbl_->getPtr(ap));
    }
  }
  return result;
}

void dbMPin::clearPinAccess(const int pin_access_idx)
{
  _dbMPin* pin = (_dbMPin*) this;
  _dbBlock* block = (_dbBlock*) getDb()->getChip()->getBlock();
  if (pin->aps_.size() <= pin_access_idx) {
    return;
  }
  const auto aps = pin->aps_[pin_access_idx];
  for (const auto& ap : aps) {
    odb::dbAccessPoint::destroy(
        (odb::dbAccessPoint*) block->ap_tbl_->getPtr(ap));
  }
}

dbMPin* dbMPin::create(dbMTerm* mterm_)
{
  _dbMTerm* mterm = (_dbMTerm*) mterm_;
  _dbMaster* master = (_dbMaster*) mterm->getOwner();
  _dbMPin* mpin = master->mpin_tbl_->create();
  mpin->mterm_ = mterm->getOID();
  mpin->next_mpin_ = mterm->pins_;
  mterm->pins_ = mpin->getOID();
  return (dbMPin*) mpin;
}

dbMPin* dbMPin::getMPin(dbMaster* master_, uint32_t dbid_)
{
  _dbMaster* master = (_dbMaster*) master_;
  return (dbMPin*) master->mpin_tbl_->getPtr(dbid_);
}

void _dbMPin::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  MemInfo& ap_info = info.children["aps"];
  for (const auto& v : aps_) {
    ap_info.add(v);
  }
}
}  // namespace odb
