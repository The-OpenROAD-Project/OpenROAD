// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbUnfoldedNet.h"

#include "dbChipNet.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbUnfoldedBump.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbUnfoldedNet>;

bool _dbUnfoldedNet::operator==(const _dbUnfoldedNet& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (chip_net_ != rhs.chip_net_) {
    return false;
  }

  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbUnfoldedNet::operator<(const _dbUnfoldedNet& rhs) const
{
  return true;
}

_dbUnfoldedNet::_dbUnfoldedNet(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbUnfoldedNet& obj)
{
  stream >> obj.chip_net_;
  stream >> obj.connected_bumps_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedNet& obj)
{
  stream << obj.chip_net_;
  stream << obj.connected_bumps_;
  return stream;
}

void _dbUnfoldedNet::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["connected_bumps"].add(connected_bumps_);
}

////////////////////////////////////////////////////////////////////
//
// dbUnfoldedNet - Methods
//
////////////////////////////////////////////////////////////////////

dbChipNet* dbUnfoldedNet::getChipNet() const
{
  _dbUnfoldedNet* obj = (_dbUnfoldedNet*) this;
  if (obj->chip_net_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbChipNet*) par->chip_net_tbl_->getPtr(obj->chip_net_);
}

// User Code Begin dbUnfoldedNetPublicMethods
std::vector<dbUnfoldedBump*> dbUnfoldedNet::getConnectedBumps() const
{
  _dbUnfoldedNet* obj = (_dbUnfoldedNet*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  std::vector<dbUnfoldedBump*> bumps;
  bumps.reserve(obj->connected_bumps_.size());
  for (const auto& id : obj->connected_bumps_) {
    bumps.push_back((dbUnfoldedBump*) db->unfolded_bump_tbl_->getPtr(id));
  }
  return bumps;
}
// User Code End dbUnfoldedNetPublicMethods
}  // namespace odb
// Generator Code End Cpp