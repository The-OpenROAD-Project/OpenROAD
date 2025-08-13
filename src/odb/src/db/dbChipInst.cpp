// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipInst.h"

#include <string>

#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
// User Code Begin Includes
#include "dbChip.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbChipInst>;

bool _dbChipInst::operator==(const _dbChipInst& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (loc_ != rhs.loc_) {
    return false;
  }
  if (master_chip_ != rhs.master_chip_) {
    return false;
  }
  if (parent_chip_ != rhs.parent_chip_) {
    return false;
  }
  if (chipinst_next_ != rhs.chipinst_next_) {
    return false;
  }

  return true;
}

bool _dbChipInst::operator<(const _dbChipInst& rhs) const
{
  if (name_ >= rhs.name_) {
    return false;
  }
  if (loc_ >= rhs.loc_) {
    return false;
  }

  return true;
}

_dbChipInst::_dbChipInst(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbChipInst& obj)
{
  stream >> obj.name_;
  stream >> obj.loc_;
  stream >> obj.master_chip_;
  stream >> obj.parent_chip_;
  stream >> obj.chipinst_next_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbChipInst& obj)
{
  stream << obj.name_;
  stream << obj.loc_;
  stream << obj.master_chip_;
  stream << obj.parent_chip_;
  stream << obj.chipinst_next_;
  return stream;
}

void _dbChipInst::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbChipInst - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbChipInst::getName() const
{
  _dbChipInst* obj = (_dbChipInst*) this;
  return obj->name_;
}

void dbChipInst::setLoc(Point3D loc)
{
  _dbChipInst* obj = (_dbChipInst*) this;

  obj->loc_ = loc;
}

Point3D dbChipInst::getLoc() const
{
  _dbChipInst* obj = (_dbChipInst*) this;
  return obj->loc_;
}

dbChip* dbChipInst::getMasterChip() const
{
  _dbChipInst* obj = (_dbChipInst*) this;
  if (obj->master_chip_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbChip*) par->_chip_tbl->getPtr(obj->master_chip_);
}

dbChip* dbChipInst::getParentChip() const
{
  _dbChipInst* obj = (_dbChipInst*) this;
  if (obj->parent_chip_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbChip*) par->_chip_tbl->getPtr(obj->parent_chip_);
}

// User Code Begin dbChipInstPublicMethods
void dbChipInst::setOrient(dbOrientType orient)
{
  _dbChipInst* obj = (_dbChipInst*) this;
  obj->orient_ = orient;
}

dbOrientType dbChipInst::getOrient() const
{
  _dbChipInst* obj = (_dbChipInst*) this;
  return obj->orient_;
}

dbChipInst* dbChipInst::create(dbChip* parentChip,
                               dbChip* masterChip,
                               const std::string& name)
{
  if (parentChip == nullptr || masterChip == nullptr) {
    return nullptr;
  }

  _dbChip* parent = (_dbChip*) parentChip;
  _dbChip* master = (_dbChip*) masterChip;
  _dbDatabase* db = (_dbDatabase*) parent->getOwner();

  // Create a new chip instance
  _dbChipInst* chipinst = db->chip_inst_tbl_->create();

  // Initialize the chip instance
  chipinst->name_ = name;
  chipinst->loc_ = Point3D(0, 0, 0);  // Default location
  chipinst->orient_
      = dbOrientType::R0;  // Default orientation (already set in constructor)
  chipinst->master_chip_ = master->getOID();
  chipinst->parent_chip_ = parent->getOID();

  // Link the chip instance to the parent chip's linked list
  chipinst->chipinst_next_ = parent->chipinsts_;
  parent->chipinsts_ = chipinst->getOID();

  return (dbChipInst*) chipinst;
}

void dbChipInst::destroy(dbChipInst* chipInst)
{
  if (chipInst == nullptr) {
    return;
  }

  _dbChipInst* inst = (_dbChipInst*) chipInst;
  _dbDatabase* db = (_dbDatabase*) inst->getOwner();

  // Get parent chip
  _dbChip* parent = db->_chip_tbl->getPtr(inst->parent_chip_);

  // Remove from parent's linked list
  if (parent->chipinsts_ == inst->getOID()) {
    // This is the first in the list
    parent->chipinsts_ = inst->chipinst_next_;
  } else {
    // Find the previous node and unlink
    uint prev_id = parent->chipinsts_;
    while (prev_id != 0) {
      _dbChipInst* prev = db->chip_inst_tbl_->getPtr(prev_id);
      if (prev->chipinst_next_ == inst->getOID()) {
        prev->chipinst_next_ = inst->chipinst_next_;
        break;
      }
      prev_id = prev->chipinst_next_;
    }
  }
  // Destroy the chip instance
  db->chip_inst_tbl_->destroy(inst);
}
// User Code End dbChipInstPublicMethods
}  // namespace odb
// Generator Code End Cpp