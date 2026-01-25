// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipConn.h"

#include <cstdint>
#include <string>
#include <vector>

#include "dbChip.h"
#include "dbChipInst.h"
#include "dbChipRegion.h"
#include "dbChipRegionInst.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbChipConn>;

bool _dbChipConn::operator==(const _dbChipConn& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (thickness_ != rhs.thickness_) {
    return false;
  }
  if (chip_ != rhs.chip_) {
    return false;
  }
  if (chip_conn_next_ != rhs.chip_conn_next_) {
    return false;
  }
  if (top_region_ != rhs.top_region_) {
    return false;
  }
  if (bottom_region_ != rhs.bottom_region_) {
    return false;
  }

  return true;
}

bool _dbChipConn::operator<(const _dbChipConn& rhs) const
{
  return true;
}

_dbChipConn::_dbChipConn(_dbDatabase* db)
{
  thickness_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbChipConn& obj)
{
  stream >> obj.name_;
  stream >> obj.thickness_;
  stream >> obj.chip_;
  stream >> obj.chip_conn_next_;
  stream >> obj.top_region_;
  stream >> obj.top_region_path_;
  stream >> obj.bottom_region_;
  stream >> obj.bottom_region_path_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbChipConn& obj)
{
  stream << obj.name_;
  stream << obj.thickness_;
  stream << obj.chip_;
  stream << obj.chip_conn_next_;
  stream << obj.top_region_;
  stream << obj.top_region_path_;
  stream << obj.bottom_region_;
  stream << obj.bottom_region_path_;
  return stream;
}

void _dbChipConn::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbChipConn - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbChipConn::getName() const
{
  _dbChipConn* obj = (_dbChipConn*) this;
  return obj->name_;
}

void dbChipConn::setThickness(int thickness)
{
  _dbChipConn* obj = (_dbChipConn*) this;

  obj->thickness_ = thickness;
}

int dbChipConn::getThickness() const
{
  _dbChipConn* obj = (_dbChipConn*) this;
  return obj->thickness_;
}

// User Code Begin dbChipConnPublicMethods

dbChip* dbChipConn::getParentChip() const
{
  _dbChipConn* obj = (_dbChipConn*) this;
  _dbDatabase* _db = (_dbDatabase*) obj->getOwner();
  return (dbChip*) _db->chip_tbl_->getPtr(obj->chip_);
}

dbChipRegionInst* dbChipConn::getTopRegion() const
{
  _dbChipConn* obj = (_dbChipConn*) this;
  _dbDatabase* _db = (_dbDatabase*) obj->getOwner();
  return (dbChipRegionInst*) _db->chip_region_inst_tbl_->getPtr(
      obj->top_region_);
}

dbChipRegionInst* dbChipConn::getBottomRegion() const
{
  _dbChipConn* obj = (_dbChipConn*) this;
  _dbDatabase* _db = (_dbDatabase*) obj->getOwner();
  if (!obj->bottom_region_.isValid()) {
    return nullptr;
  }
  return (dbChipRegionInst*) _db->chip_region_inst_tbl_->getPtr(
      obj->bottom_region_);
}

std::vector<dbChipInst*> dbChipConn::getTopRegionPath() const
{
  _dbChipConn* obj = (_dbChipConn*) this;
  _dbDatabase* _db = (_dbDatabase*) obj->getOwner();
  std::vector<dbChipInst*> top_region_path;
  top_region_path.reserve(obj->top_region_path_.size());
  for (const auto& chipinst_id : obj->top_region_path_) {
    top_region_path.emplace_back(
        (dbChipInst*) _db->chip_inst_tbl_->getPtr(chipinst_id));
  }
  return top_region_path;
}

std::vector<dbChipInst*> dbChipConn::getBottomRegionPath() const
{
  _dbChipConn* obj = (_dbChipConn*) this;
  _dbDatabase* _db = (_dbDatabase*) obj->getOwner();
  std::vector<dbChipInst*> bottom_region_path;
  bottom_region_path.reserve(obj->bottom_region_path_.size());
  for (const auto& chipinst_id : obj->bottom_region_path_) {
    bottom_region_path.emplace_back(
        (dbChipInst*) _db->chip_inst_tbl_->getPtr(chipinst_id));
  }
  return bottom_region_path;
}

static std::vector<dbId<_dbChipInst>> extractChipInstsPath(
    dbChip* parent_chip,
    const std::vector<dbChipInst*>& chip_insts)
{
  _dbDatabase* _db = (_dbDatabase*) parent_chip->getImpl()->getOwner();
  utl::Logger* logger = _db->getLogger();
  std::vector<dbId<_dbChipInst>> chip_insts_path;
  chip_insts_path.reserve(chip_insts.size());
  for (auto chipinst : chip_insts) {
    if (chipinst->getParentChip() != parent_chip) {
      logger->error(utl::ODB,
                    510,
                    "Cannot create chip connection. ChipInst {} is not a child "
                    "of chip {}",
                    chipinst->getName(),
                    parent_chip->getName());
    }
    chip_insts_path.emplace_back(chipinst->getImpl()->getOID());
    parent_chip = chipinst->getMasterChip();
  }
  return chip_insts_path;
}

dbChipConn* dbChipConn::create(
    const std::string& name,
    dbChip* parent_chip,
    const std::vector<dbChipInst*>& top_region_path,
    dbChipRegionInst* top_region,
    const std::vector<dbChipInst*>& bottom_region_path,
    dbChipRegionInst* bottom_region)
{
  if (parent_chip == nullptr) {
    return nullptr;
  }
  _dbDatabase* _db = (_dbDatabase*) parent_chip->getImpl()->getOwner();
  if (top_region == nullptr || top_region_path.empty()) {
    _db->getLogger()->error(utl::ODB,
                            511,
                            "Cannot create chip connection {}. Top region is "
                            "not specified correctly",
                            name);
  }
  if (top_region->getChipInst() != top_region_path.back()) {
    _db->getLogger()->error(utl::ODB,
                            515,
                            "Cannot create chip connection {}. Top region path "
                            "does not match top region",
                            name);
  }
  if (bottom_region != nullptr && bottom_region_path.empty()) {
    _db->getLogger()->error(
        utl::ODB,
        517,
        "Cannot create chip connection {}. Bottom region path "
        "is empty for non-empty bottom region",
        name);
  }
  if (bottom_region != nullptr
      && bottom_region->getChipInst() != bottom_region_path.back()) {
    _db->getLogger()->error(
        utl::ODB,
        518,
        "Cannot create chip connection {}. Bottom region path "
        "does not match bottom region",
        name);
  }
  _dbChipConn* obj = (_dbChipConn*) _db->chip_conn_tbl_->create();
  _dbChip* chip = (_dbChip*) parent_chip;
  obj->name_ = name;
  obj->thickness_ = 0;
  obj->chip_ = chip->getOID();
  obj->top_region_ = top_region->getImpl()->getOID();
  if (bottom_region != nullptr) {
    obj->bottom_region_ = bottom_region->getImpl()->getOID();
  }
  obj->top_region_path_ = extractChipInstsPath(parent_chip, top_region_path);
  if (bottom_region != nullptr) {
    obj->bottom_region_path_
        = extractChipInstsPath(parent_chip, bottom_region_path);
  }

  obj->chip_conn_next_ = chip->conns_;
  chip->conns_ = obj->getOID();
  return (dbChipConn*) obj;
}

void dbChipConn::destroy(dbChipConn* chipConn)
{
  _dbChipConn* obj = (_dbChipConn*) chipConn;
  _dbDatabase* _db = (_dbDatabase*) obj->getOwner();
  // Remove from chip's list of connections
  _dbChip* chip = (_dbChip*) chipConn->getParentChip();
  if (chip->conns_ == obj->getOID()) {
    chip->conns_ = obj->chip_conn_next_;
  } else {
    uint32_t id = chip->conns_;
    while (id != 0) {
      _dbChipConn* _chipconn = _db->chip_conn_tbl_->getPtr(id);
      if (_chipconn->chip_conn_next_ == obj->getOID()) {
        _chipconn->chip_conn_next_ = obj->chip_conn_next_;
        break;
      }
      id = _chipconn->chip_conn_next_;
    }
  }
  // Destroy the connection
  _db->chip_conn_tbl_->destroy(obj);
}

// User Code End dbChipConnPublicMethods
}  // namespace odb
// Generator Code End Cpp
