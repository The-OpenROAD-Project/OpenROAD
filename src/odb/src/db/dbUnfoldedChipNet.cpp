// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbUnfoldedChipNet.h"

#include "dbChipNet.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbUnfoldedChipBumpInst.h"
#include "odb/db.h"
// User Code Begin Includes
#include <vector>
// User Code End Includes
namespace odb {
template class dbTable<_dbUnfoldedChipNet>;

bool _dbUnfoldedChipNet::operator==(const _dbUnfoldedChipNet& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (chip_net_ != rhs.chip_net_) {
    return false;
  }

  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbUnfoldedChipNet::operator<(const _dbUnfoldedChipNet& rhs) const
{
  return true;
}

_dbUnfoldedChipNet::_dbUnfoldedChipNet(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbUnfoldedChipNet& obj)
{
  stream >> obj.chip_net_;
  stream >> obj.connected_bumps_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedChipNet& obj)
{
  stream << obj.chip_net_;
  stream << obj.connected_bumps_;
  return stream;
}

void _dbUnfoldedChipNet::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["connected_bumps"].add(connected_bumps_);
}

////////////////////////////////////////////////////////////////////
//
// dbUnfoldedChipNet - Methods
//
////////////////////////////////////////////////////////////////////

dbChipNet* dbUnfoldedChipNet::getChipNet() const
{
  _dbUnfoldedChipNet* obj = (_dbUnfoldedChipNet*) this;
  if (obj->chip_net_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbChipNet*) par->chip_net_tbl_->getPtr(obj->chip_net_);
}

// User Code Begin dbUnfoldedChipNetPublicMethods
std::vector<dbUnfoldedChipBumpInst*> dbUnfoldedChipNet::getConnectedBumps()
    const
{
  _dbUnfoldedChipNet* obj = (_dbUnfoldedChipNet*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  std::vector<dbUnfoldedChipBumpInst*> bumps;
  bumps.reserve(obj->connected_bumps_.size());
  for (const auto& id : obj->connected_bumps_) {
    bumps.push_back(
        (dbUnfoldedChipBumpInst*) db->unfolded_chip_bump_inst_tbl_->getPtr(id));
  }
  return bumps;
}
// User Code End dbUnfoldedChipNetPublicMethods
}  // namespace odb
// Generator Code End Cpp