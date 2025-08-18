// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipConn.h"

#include <string>
#include <vector>

#include "dbChip.h"
#include "dbChipInst.h"
#include "dbChipRegion.h"
#include "dbChipRegionInst.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
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
}

dbIStream& operator>>(dbIStream& stream, _dbChipConn& obj)
{
  stream >> obj.name_;
  stream >> obj.thickness_;
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
  return (dbChipRegionInst*) _db->chip_region_inst_tbl_->getPtr(
      obj->bottom_region_);
}

std::vector<dbChipInst*> dbChipConn::getTopRegionPath() const
{
  _dbChipConn* obj = (_dbChipConn*) this;
  _dbDatabase* _db = (_dbDatabase*) obj->getOwner();
  std::vector<dbChipInst*> top_region_path;
  for (auto chipinst_id : obj->top_region_path_) {
    top_region_path.push_back(
        (dbChipInst*) _db->chip_inst_tbl_->getPtr(chipinst_id));
  }
  return top_region_path;
}

std::vector<dbChipInst*> dbChipConn::getBottomRegionPath() const
{
  _dbChipConn* obj = (_dbChipConn*) this;
  _dbDatabase* _db = (_dbDatabase*) obj->getOwner();
  std::vector<dbChipInst*> bottom_region_path;
  for (auto chipinst_id : obj->bottom_region_path_) {
    bottom_region_path.push_back(
        (dbChipInst*) _db->chip_inst_tbl_->getPtr(chipinst_id));
  }
  return bottom_region_path;
}

dbChipConn* dbChipConn::create(const std::string& name,
                               dbChipRegionInst* top_region,
                               dbChipRegionInst* bottom_region,
                               std::vector<dbChipInst*> top_region_path,
                               std::vector<dbChipInst*> bottom_region_path)
{
  _dbDatabase* _db = (_dbDatabase*) top_region->getImpl()->getOwner();
  _dbChipConn* obj = (_dbChipConn*) _db->chip_conn_tbl_->create();
  obj->name_ = name;
  obj->thickness_ = 0;
  obj->top_region_ = top_region->getImpl()->getOID();
  obj->bottom_region_ = bottom_region->getImpl()->getOID();
  std::vector<dbId<_dbChipInst>> top_region_path_ids;
  for (auto chipinst : top_region_path) {
    top_region_path_ids.push_back(chipinst->getImpl()->getOID());
  }
  obj->top_region_path_ = top_region_path_ids;
  std::vector<dbId<_dbChipInst>> bottom_region_path_ids;
  for (auto chipinst : bottom_region_path) {
    bottom_region_path_ids.push_back(chipinst->getImpl()->getOID());
  }
  obj->bottom_region_path_ = bottom_region_path_ids;
  return (dbChipConn*) obj;
}

void dbChipConn::destroy(dbChipConn* chipConn)
{
  _dbChipConn* obj = (_dbChipConn*) chipConn;
  _dbDatabase* _db = (_dbDatabase*) obj->getOwner();
  _db->chip_conn_tbl_->destroy(obj);
}

// User Code End dbChipConnPublicMethods
}  // namespace odb
// Generator Code End Cpp