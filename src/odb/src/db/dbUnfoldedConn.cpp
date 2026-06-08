// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbUnfoldedConn.h"

#include "dbChipConn.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbUnfoldedChipRegionInst.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbUnfoldedConn>;

bool _dbUnfoldedConn::operator==(const _dbUnfoldedConn& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (chip_conn_ != rhs.chip_conn_) {
    return false;
  }
  if (top_region_ != rhs.top_region_) {
    return false;
  }
  if (bottom_region_ != rhs.bottom_region_) {
    return false;
  }

  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbUnfoldedConn::operator<(const _dbUnfoldedConn& rhs) const
{
  return true;
}

_dbUnfoldedConn::_dbUnfoldedConn(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbUnfoldedConn& obj)
{
  stream >> obj.chip_conn_;
  stream >> obj.top_region_;
  stream >> obj.bottom_region_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedConn& obj)
{
  stream << obj.chip_conn_;
  stream << obj.top_region_;
  stream << obj.bottom_region_;
  return stream;
}

void _dbUnfoldedConn::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbUnfoldedConn - Methods
//
////////////////////////////////////////////////////////////////////

dbChipConn* dbUnfoldedConn::getChipConn() const
{
  _dbUnfoldedConn* obj = (_dbUnfoldedConn*) this;
  if (obj->chip_conn_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbChipConn*) par->chip_conn_tbl_->getPtr(obj->chip_conn_);
}

dbUnfoldedChipRegionInst* dbUnfoldedConn::getTopRegion() const
{
  _dbUnfoldedConn* obj = (_dbUnfoldedConn*) this;
  if (obj->top_region_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbUnfoldedChipRegionInst*)
      par->unfolded_chip_region_inst_tbl_->getPtr(obj->top_region_);
}

dbUnfoldedChipRegionInst* dbUnfoldedConn::getBottomRegion() const
{
  _dbUnfoldedConn* obj = (_dbUnfoldedConn*) this;
  if (obj->bottom_region_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbUnfoldedChipRegionInst*)
      par->unfolded_chip_region_inst_tbl_->getPtr(obj->bottom_region_);
}

}  // namespace odb
// Generator Code End Cpp