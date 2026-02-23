// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipNet.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "dbChip.h"
#include "dbChipBumpInst.h"
#include "dbChipInst.h"
#include "dbChipRegionInst.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbChipNet>;

bool _dbChipNet::operator==(const _dbChipNet& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (chip_ != rhs.chip_) {
    return false;
  }
  if (chip_net_next_ != rhs.chip_net_next_) {
    return false;
  }

  return true;
}

bool _dbChipNet::operator<(const _dbChipNet& rhs) const
{
  return true;
}

_dbChipNet::_dbChipNet(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbChipNet& obj)
{
  stream >> obj.name_;
  stream >> obj.chip_;
  stream >> obj.chip_net_next_;
  stream >> obj.bump_insts_paths_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbChipNet& obj)
{
  stream << obj.name_;
  stream << obj.chip_;
  stream << obj.chip_net_next_;
  stream << obj.bump_insts_paths_;
  return stream;
}

void _dbChipNet::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbChipNet - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbChipNet::getName() const
{
  _dbChipNet* obj = (_dbChipNet*) this;
  return obj->name_;
}

// User Code Begin dbChipNetPublicMethods
dbChip* dbChipNet::getChip() const
{
  _dbChipNet* obj = (_dbChipNet*) this;
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  return (dbChip*) db->chip_tbl_->getPtr(obj->chip_);
}

uint32_t dbChipNet::getNumBumpInsts() const
{
  _dbChipNet* obj = (_dbChipNet*) this;
  return obj->bump_insts_paths_.size();
}

dbChipBumpInst* dbChipNet::getBumpInst(uint32_t index,
                                       std::vector<dbChipInst*>& path) const
{
  _dbChipNet* obj = (_dbChipNet*) this;
  if (index >= obj->bump_insts_paths_.size()) {
    return nullptr;
  }
  _dbDatabase* db = (_dbDatabase*) obj->getOwner();
  const auto& bump_inst_path = obj->bump_insts_paths_[index];

  // Fill the path vector
  path.clear();
  for (const auto& inst_id : bump_inst_path.first) {
    path.push_back((dbChipInst*) db->chip_inst_tbl_->getPtr(inst_id));
  }

  return (dbChipBumpInst*) db->chip_bump_inst_tbl_->getPtr(
      bump_inst_path.second);
}

void dbChipNet::addBumpInst(dbChipBumpInst* bump_inst,
                            const std::vector<dbChipInst*>& path)
{
  _dbChipNet* obj = (_dbChipNet*) this;
  if (bump_inst == nullptr) {
    obj->getLogger()->error(utl::ODB,
                            479,
                            "Cannot add nullptr bump inst to chip {} net {}",
                            obj->name_);
  }
  std::vector<dbId<_dbChipInst>> path_ids;
  path_ids.reserve(path.size());
  for (const auto& inst : path) {
    if (inst == nullptr) {
      obj->getLogger()->error(utl::ODB,
                              480,
                              "Invalid inst in path for bump inst in net {}",
                              obj->name_);
    }
    path_ids.emplace_back(inst->getImpl()->getOID());
  }

  obj->bump_insts_paths_.emplace_back(std::move(path_ids),
                                      bump_inst->getImpl()->getOID());
}

dbChipNet* dbChipNet::create(dbChip* chip, const std::string& name)
{
  _dbChip* _chip = (_dbChip*) chip;
  _dbDatabase* db = _chip->getDatabase();

  _dbChipNet* chip_net = db->chip_net_tbl_->create();
  chip_net->name_ = name;
  chip_net->chip_ = _chip->getOID();

  // Add to chip's net list
  chip_net->chip_net_next_ = _chip->nets_;
  _chip->nets_ = chip_net->getOID();

  return (dbChipNet*) chip_net;
}

void dbChipNet::destroy(dbChipNet* net)
{
  _dbChipNet* _net = (_dbChipNet*) net;
  _dbDatabase* db = (_dbDatabase*) _net->getOwner();
  _dbChip* chip = (_dbChip*) net->getChip();

  // Remove from chip's net list
  if (chip->nets_ == _net->getOID()) {
    chip->nets_ = _net->chip_net_next_;
  } else {
    _dbChipNet* prev = nullptr;
    _dbChipNet* current = (_dbChipNet*) db->chip_net_tbl_->getPtr(chip->nets_);
    while (current && current != _net) {
      prev = current;
      current
          = (_dbChipNet*) db->chip_net_tbl_->getPtr(current->chip_net_next_);
    }
    if (current && prev) {
      prev->chip_net_next_ = current->chip_net_next_;
    }
  }

  // Destroy the chip net object
  db->chip_net_tbl_->destroy(_net);
}
// User Code End dbChipNetPublicMethods
}  // namespace odb
// Generator Code End Cpp
