///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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

// Generator Code Begin Cpp
#include "dbAccessPoint.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"
// User Code Begin Includes
#include <algorithm>

#include "dbBPin.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbITerm.h"
#include "dbLib.h"
#include "dbMPin.h"
#include "dbMaster.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechVia.h"
#include "dbVia.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbAccessPoint>;

bool _dbAccessPoint::operator==(const _dbAccessPoint& rhs) const
{
  if (point_ != rhs.point_) {
    return false;
  }
  if (layer_ != rhs.layer_) {
    return false;
  }
  if (lib_ != rhs.lib_) {
    return false;
  }
  if (master_ != rhs.master_) {
    return false;
  }
  if (mpin_ != rhs.mpin_) {
    return false;
  }
  if (bpin_ != rhs.bpin_) {
    return false;
  }

  return true;
}

bool _dbAccessPoint::operator<(const _dbAccessPoint& rhs) const
{
  if (point_ >= rhs.point_) {
    return false;
  }
  if (layer_ >= rhs.layer_) {
    return false;
  }
  if (lib_ >= rhs.lib_) {
    return false;
  }
  if (master_ >= rhs.master_) {
    return false;
  }
  if (mpin_ >= rhs.mpin_) {
    return false;
  }
  if (bpin_ >= rhs.bpin_) {
    return false;
  }

  return true;
}

void _dbAccessPoint::differences(dbDiff& diff,
                                 const char* field,
                                 const _dbAccessPoint& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(point_);
  DIFF_FIELD(layer_);
  DIFF_FIELD(lib_);
  DIFF_FIELD(master_);
  DIFF_FIELD(mpin_);
  DIFF_FIELD(bpin_);
  DIFF_END
}

void _dbAccessPoint::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(point_);
  DIFF_OUT_FIELD(layer_);
  DIFF_OUT_FIELD(lib_);
  DIFF_OUT_FIELD(master_);
  DIFF_OUT_FIELD(mpin_);
  DIFF_OUT_FIELD(bpin_);

  DIFF_END
}

_dbAccessPoint::_dbAccessPoint(_dbDatabase* db)
{
  low_type_ = dbAccessType::OnGrid;
  high_type_ = dbAccessType::OnGrid;
  // User Code Begin Constructor
  accesses_.fill(false);
  // User Code End Constructor
}

_dbAccessPoint::_dbAccessPoint(_dbDatabase* db, const _dbAccessPoint& r)
{
  point_ = r.point_;
  layer_ = r.layer_;
  lib_ = r.lib_;
  master_ = r.master_;
  mpin_ = r.mpin_;
  bpin_ = r.bpin_;
  // User Code Begin CopyConstructor
  iterms_ = r.iterms_;
  low_type_ = r.low_type_;
  high_type_ = r.high_type_;
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbAccessPoint& obj)
{
  stream >> obj.point_;
  stream >> obj.layer_;
  stream >> obj.lib_;
  stream >> obj.master_;
  stream >> obj.mpin_;
  stream >> obj.bpin_;
  stream >> obj.accesses_;
  stream >> obj.iterms_;
  stream >> obj.vias_;
  stream >> obj.path_segs_;
  // User Code Begin >>
  int8_t low, high;
  stream >> low;
  stream >> high;
  obj.low_type_ = static_cast<dbAccessType::Value>(low);
  obj.high_type_ = static_cast<dbAccessType::Value>(high);
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbAccessPoint& obj)
{
  stream << obj.point_;
  stream << obj.layer_;
  stream << obj.lib_;
  stream << obj.master_;
  stream << obj.mpin_;
  stream << obj.bpin_;
  stream << obj.accesses_;
  stream << obj.iterms_;
  stream << obj.vias_;
  stream << obj.path_segs_;
  // User Code Begin <<
  int8_t low = static_cast<int8_t>(obj.low_type_);
  int8_t high = static_cast<int8_t>(obj.high_type_);
  stream << low;
  stream << high;
  // User Code End <<
  return stream;
}

// User Code Begin PrivateMethods
void _dbAccessPoint::setMPin(_dbMPin* mpin)
{
  mpin_ = mpin->getOID();
  auto master = ((dbMPin*) mpin)->getMaster();
  master_ = master->getImpl()->getOID();
  auto lib = master->getLib();
  lib_ = lib->getImpl()->getOID();
}

// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbAccessPoint - Methods
//
////////////////////////////////////////////////////////////////////

void dbAccessPoint::setPoint(Point point)
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;

  obj->point_ = point;
}

Point dbAccessPoint::getPoint() const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  return obj->point_;
}

void dbAccessPoint::setLayer(dbTechLayer* layer)
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;

  obj->layer_ = layer->getImpl()->getOID();
}

// User Code Begin dbAccessPointPublicMethods

void dbAccessPoint::addSegment(const Rect& segment,
                               const bool& begin_style_trunc,
                               const bool& end_style_trunc)
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  std::tuple path_seg
      = std::make_tuple(segment, begin_style_trunc, end_style_trunc);
  obj->path_segs_.push_back(std::move(path_seg));
}

const std::vector<std::tuple<Rect, bool, bool>>& dbAccessPoint::getSegments()
    const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  return obj->path_segs_;
}

void dbAccessPoint::setAccesses(const std::vector<dbDirection>& accesses)
{
  for (const auto& dir : accesses) {
    setAccess(true, dir);
  }
}

void dbAccessPoint::getAccesses(std::vector<dbDirection>& tbl) const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  for (int dir = 0; dir < 6; dir++) {
    if (obj->accesses_[dir]) {
      tbl.push_back(dbDirection::Value(dir + 1));
    }
  }
}

void dbAccessPoint::setLowType(dbAccessType low_type_)
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;

  obj->low_type_ = low_type_;
}

dbAccessType dbAccessPoint::getLowType() const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  return obj->low_type_;
}

void dbAccessPoint::setHighType(dbAccessType high_type_)
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;

  obj->high_type_ = high_type_;
}

dbAccessType dbAccessPoint::getHighType() const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  return obj->high_type_;
}

void dbAccessPoint::setAccess(bool access, dbDirection dir)
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  switch (dir) {
    case dbDirection::EAST:
    case dbDirection::SOUTH:
    case dbDirection::WEST:
    case dbDirection::NORTH:
    case dbDirection::UP:
    case dbDirection::DOWN:
      obj->accesses_[dir - 1] = access;
      return;
    case dbDirection::NONE:
      break;
  }
  getImpl()->getLogger()->error(
      utl::ODB, 1100, "Access direction is of unknown type");
}

bool dbAccessPoint::hasAccess(dbDirection dir) const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  switch (dir) {
    case dbDirection::EAST:
    case dbDirection::SOUTH:
    case dbDirection::WEST:
    case dbDirection::NORTH:
    case dbDirection::UP:
    case dbDirection::DOWN:
      return obj->accesses_[dir - 1];
      break;
    case dbDirection::NONE:
      return (std::count(obj->accesses_.begin(), obj->accesses_.end(), true)
              > 0);
  }
  getImpl()->getLogger()->error(
      utl::ODB, 1101, "Access direction is of unknown type");
}

dbTechLayer* dbAccessPoint::getLayer() const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  dbDatabase* db = (dbDatabase*) obj->getDatabase();
  _dbTech* tech = (_dbTech*) db->getTech();
  return (dbTechLayer*) tech->_layer_tbl->getPtr(obj->layer_);
}

dbMPin* dbAccessPoint::getMPin() const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  if (!obj->mpin_.isValid()) {
    return nullptr;
  }
  _dbDatabase* db = obj->getDatabase();
  auto lib = (_dbLib*) db->_lib_tbl->getPtr(obj->lib_);
  auto master = (_dbMaster*) lib->_master_tbl->getPtr(obj->master_);
  return (dbMPin*) master->_mpin_tbl->getPtr(obj->mpin_);
}

dbBPin* dbAccessPoint::getBPin() const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  if (!obj->bpin_.isValid()) {
    return nullptr;
  }
  _dbBlock* block = (_dbBlock*) obj->getOwner();
  return (dbBPin*) block->_bpin_tbl->getPtr(obj->bpin_);
}
std::vector<std::vector<dbObject*>> dbAccessPoint::getVias() const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  dbDatabase* db = (dbDatabase*) obj->getDatabase();
  _dbTech* tech = (_dbTech*) db->getTech();
  _dbBlock* block = (_dbBlock*) obj->getOwner();
  std::vector<std::vector<dbObject*>> result;
  for (const auto& cutVias : obj->vias_) {
    result.push_back(std::vector<dbObject*>());
    for (const auto& [type, id] : cutVias) {
      if (type == dbObjectType::dbViaObj) {
        result.back().push_back(
            (dbObject*) block->_via_tbl->getPtr(dbId<_dbVia>(id)));
      } else {
        result.back().push_back(
            (dbTechVia*) tech->_via_tbl->getPtr(dbId<_dbTechVia>(id)));
      }
    }
  }
  return result;
}

void dbAccessPoint::addTechVia(int num_cuts, dbTechVia* via)
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  if (num_cuts > obj->vias_.size()) {
    obj->vias_.resize(num_cuts);
  }
  obj->vias_[num_cuts - 1].push_back(
      {via->getObjectType(), via->getImpl()->getOID()});
}

void dbAccessPoint::addBlockVia(int num_cuts, dbVia* via)
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  if (num_cuts > obj->vias_.size()) {
    obj->vias_.resize(num_cuts);
  }
  obj->vias_[num_cuts - 1].push_back(
      {via->getObjectType(), via->getImpl()->getOID()});
}

dbAccessPoint* dbAccessPoint::create(dbBlock* block_, dbMPin* pin, uint idx)
{
  _dbBlock* block = (_dbBlock*) block_;
  _dbAccessPoint* ap = block->ap_tbl_->create();
  _dbMPin* mpin = (_dbMPin*) pin;
  ap->setMPin(mpin);
  mpin->addAccessPoint(idx, ap);
  return ((dbAccessPoint*) ap);
}

dbAccessPoint* dbAccessPoint::create(dbBPin* pin)
{
  _dbBlock* block = (_dbBlock*) pin->getBTerm()->getBlock();
  _dbAccessPoint* ap = block->ap_tbl_->create();
  _dbBPin* bpin = (_dbBPin*) pin;
  ap->bpin_ = bpin->getOID();
  bpin->aps_.push_back(ap->getOID());
  return ((dbAccessPoint*) ap);
}

dbAccessPoint* dbAccessPoint::getAccessPoint(dbBlock* block_, uint dbid)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbAccessPoint*) block->ap_tbl_->getPtr(dbid);
}

void dbAccessPoint::destroy(dbAccessPoint* ap)
{
  _dbBlock* block = (_dbBlock*) ap->getImpl()->getOwner();
  _dbAccessPoint* _ap = (_dbAccessPoint*) ap;
  if (ap->getBPin() != nullptr) {
    _dbBPin* pin = (_dbBPin*) ap->getBPin();
    auto ap_itr = pin->aps_.begin();
    while (ap_itr != pin->aps_.end()) {
      if (*ap_itr == ap->getImpl()->getOID()) {
        pin->aps_.erase(ap_itr);
        break;
      }
      ++ap_itr;
    }
  } else {
    _dbMPin* pin = (_dbMPin*) ap->getMPin();
    bool found = false;
    for (auto& aps : pin->aps_) {
      if (found) {
        break;
      }
      auto ap_itr = aps.begin();
      while (ap_itr != aps.end()) {
        if (*ap_itr == ap->getImpl()->getOID()) {
          aps.erase(ap_itr);
          found = true;
          break;
        }
        ++ap_itr;
      }
    }
    for (const auto& iterm_id : _ap->iterms_) {
      _dbITerm* iterm = block->_iterm_tbl->getPtr(iterm_id);
      auto ap_itr = iterm->aps_.begin();
      while (ap_itr != iterm->aps_.end()) {
        if ((*ap_itr).second == ap->getImpl()->getOID()) {
          iterm->aps_.erase(ap_itr);
          break;
        }
        ++ap_itr;
      }
    }
  }
  dbProperty::destroyProperties(ap);
  block->ap_tbl_->destroy((_dbAccessPoint*) ap);
}
// User Code End dbAccessPointPublicMethods
}  // namespace odb
// Generator Code End Cpp
