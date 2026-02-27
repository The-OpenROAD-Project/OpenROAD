// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipInst.h"

#include <string>
#include <unordered_map>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include <cstdint>

#include "dbChip.h"
#include "dbChipBumpInst.h"
#include "dbChipRegionInst.h"
#include "odb/dbSet.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbChipInst>;

bool _dbChipInst::operator==(const _dbChipInst& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (origin_ != rhs.origin_) {
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
  if (chip_region_insts_ != rhs.chip_region_insts_) {
    return false;
  }

  return true;
}

bool _dbChipInst::operator<(const _dbChipInst& rhs) const
{
  if (name_ >= rhs.name_) {
    return false;
  }
  if (origin_ >= rhs.origin_) {
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
  stream >> obj.origin_;
  stream >> obj.orient_;
  stream >> obj.master_chip_;
  stream >> obj.parent_chip_;
  stream >> obj.chipinst_next_;
  if (obj.getDatabase()->isSchema(kSchemaChipRegion)) {
    stream >> obj.chip_region_insts_;
  }
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbChipInst& obj)
{
  stream << obj.name_;
  stream << obj.origin_;
  stream << obj.orient_;
  stream << obj.master_chip_;
  stream << obj.parent_chip_;
  stream << obj.chipinst_next_;
  stream << obj.chip_region_insts_;
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

dbOrientType3D dbChipInst::getOrient() const
{
  _dbChipInst* obj = (_dbChipInst*) this;
  return obj->orient_;
}

dbChip* dbChipInst::getMasterChip() const
{
  _dbChipInst* obj = (_dbChipInst*) this;
  if (obj->master_chip_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbChip*) par->chip_tbl_->getPtr(obj->master_chip_);
}

dbChip* dbChipInst::getParentChip() const
{
  _dbChipInst* obj = (_dbChipInst*) this;
  if (obj->parent_chip_ == 0) {
    return nullptr;
  }
  _dbDatabase* par = (_dbDatabase*) obj->getOwner();
  return (dbChip*) par->chip_tbl_->getPtr(obj->parent_chip_);
}

// User Code Begin dbChipInstPublicMethods

dbTransform dbChipInst::getTransform() const
{
  _dbChipInst* obj = (_dbChipInst*) this;
  return dbTransform(obj->orient_, obj->origin_);
}

void dbChipInst::setOrient(dbOrientType3D orient)
{
  _dbChipInst* obj = (_dbChipInst*) this;

  obj->orient_ = orient;
}

void dbChipInst::setLoc(const Point3D& loc)
{
  _dbChipInst* obj = (_dbChipInst*) this;
  dbChip* chip = getMasterChip();
  Cuboid cuboid = chip->getCuboid();
  dbTransform t(getOrient());
  t.apply(cuboid);
  const int dx = loc.x() - cuboid.lll().x();
  const int dy = loc.y() - cuboid.lll().y();
  const int dz = loc.z() - cuboid.lll().z();
  cuboid.moveDelta(dx, dy, dz);
  obj->origin_ = Point3D(dx, dy, dz);
}

Point3D dbChipInst::getLoc() const
{
  return getCuboid().lll();
}

Rect dbChipInst::getBBox() const
{
  Rect box = getMasterChip()->getBBox();
  getTransform().apply(box);
  return box;
}

Cuboid dbChipInst::getCuboid() const
{
  Cuboid cuboid = getMasterChip()->getCuboid();
  getTransform().apply(cuboid);
  return cuboid;
}

dbSet<dbChipRegionInst> dbChipInst::getRegions() const
{
  _dbChipInst* _chipinst = (_dbChipInst*) this;
  _dbDatabase* _db = (_dbDatabase*) _chipinst->getOwner();
  return dbSet<dbChipRegionInst>(_chipinst, _db->chip_region_inst_itr_);
}

dbChipRegionInst* dbChipInst::findChipRegionInst(
    dbChipRegion* chip_region) const
{
  if (chip_region == nullptr) {
    return nullptr;
  }
  _dbChipInst* obj = (_dbChipInst*) this;
  auto it = obj->region_insts_map_.find(chip_region->getId());
  if (it != obj->region_insts_map_.end()) {
    auto db = (_dbDatabase*) obj->getOwner();
    return (dbChipRegionInst*) db->chip_region_inst_tbl_->getPtr((*it).second);
  }
  return nullptr;
}

dbChipRegionInst* dbChipInst::findChipRegionInst(const std::string& name) const
{
  return findChipRegionInst(getMasterChip()->findChipRegion(name));
}

dbChipInst* dbChipInst::create(dbChip* parent_chip,
                               dbChip* master_chip,
                               const std::string& name)
{
  if (parent_chip == nullptr || master_chip == nullptr) {
    return nullptr;
  }
  _dbChip* _parent = (_dbChip*) parent_chip;
  _dbChip* _master = (_dbChip*) master_chip;
  _dbDatabase* db = (_dbDatabase*) _parent->getOwner();
  if (parent_chip->getChipType() != dbChip::ChipType::HIER) {
    db->getLogger()->error(
        utl::ODB,
        506,
        "Cannot create chip instance {} in non-hierarchy chip {}",
        name,
        parent_chip->getName());
  }
  if (parent_chip == master_chip) {
    db->getLogger()->error(
        utl::ODB, 507, "Cannot create chip instance {} in itself", name);
  }
  if (parent_chip->findChipInst(name) != nullptr) {
    db->getLogger()->error(utl::ODB,
                           514,
                           "Chip instance {} already exists in parent chip {}",
                           name,
                           parent_chip->getName());
  }

  // Create a new chip instance
  _dbChipInst* chipinst = db->chip_inst_tbl_->create();

  // Initialize the chip instance
  chipinst->name_ = name;
  chipinst->origin_ = Point3D(0, 0, 0);  // Default location
  chipinst->master_chip_ = _master->getOID();
  chipinst->parent_chip_ = _parent->getOID();

  // Link the chip instance to the parent chip's linked list
  chipinst->chipinst_next_ = _parent->chipinsts_;
  _parent->chipinsts_ = chipinst->getOID();
  _parent->chipinsts_map_[name] = chipinst->getOID();

  // create chipRegionInsts
  for (auto region : master_chip->getChipRegions()) {
    _dbChipRegionInst* regioninst = db->chip_region_inst_tbl_->create();
    regioninst->region_ = region->getImpl()->getOID();
    regioninst->parent_chipinst_ = chipinst->getOID();
    regioninst->chip_region_inst_next_ = chipinst->chip_region_insts_;
    chipinst->chip_region_insts_ = regioninst->getOID();
    chipinst->region_insts_map_[region->getId()] = regioninst->getOID();
    // create chipBumpInsts
    for (auto bump : region->getChipBumps()) {
      _dbChipBumpInst* bumpinst = db->chip_bump_inst_tbl_->create();
      bumpinst->chip_bump_ = bump->getImpl()->getOID();
      bumpinst->chip_region_inst_ = regioninst->getOID();
      bumpinst->region_next_ = regioninst->chip_bump_insts_;
      regioninst->chip_bump_insts_ = bumpinst->getOID();
    }
    // reverse the chip_bump_insts_ list
    ((dbChipRegionInst*) regioninst)->getChipBumpInsts().reverse();
  }
  // reverse the chip_region_insts_ list
  ((dbChipInst*) chipinst)->getRegions().reverse();
  return (dbChipInst*) chipinst;
}

void dbChipInst::destroy(dbChipInst* chipInst)
{
  if (chipInst == nullptr) {
    return;
  }

  _dbChipInst* inst = (_dbChipInst*) chipInst;
  _dbDatabase* db = (_dbDatabase*) inst->getOwner();

  // remove regions
  uint32_t region_inst_id = inst->chip_region_insts_;
  while (region_inst_id != 0) {
    _dbChipRegionInst* region_inst
        = db->chip_region_inst_tbl_->getPtr(region_inst_id);
    region_inst_id = region_inst->chip_region_inst_next_;
    db->chip_region_inst_tbl_->destroy(region_inst);
    // remove chipBumpInsts
    uint32_t bump_inst_id = region_inst->chip_bump_insts_;
    while (bump_inst_id != 0) {
      _dbChipBumpInst* bump_inst
          = db->chip_bump_inst_tbl_->getPtr(bump_inst_id);
      bump_inst_id = bump_inst->region_next_;
      db->chip_bump_inst_tbl_->destroy(bump_inst);
    }
  }
  // Get parent chip
  _dbChip* parent = db->chip_tbl_->getPtr(inst->parent_chip_);
  parent->chipinsts_map_.erase(inst->name_);

  // Remove from parent's linked list
  if (parent->chipinsts_ == inst->getOID()) {
    // This is the first in the list
    parent->chipinsts_ = inst->chipinst_next_;
  } else {
    // Find the previous node and unlink
    uint32_t prev_id = parent->chipinsts_;
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
