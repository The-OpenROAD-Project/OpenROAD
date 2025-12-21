// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipRegion.h"

#include <string>

#include "dbChipBump.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "odb/db.h"
#include "odb/dbSet.h"
// User Code Begin Includes
#include "dbChip.h"
#include "dbChipInst.h"
#include "dbChipRegionInst.h"
#include "dbTech.h"
#include "odb/geom.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbChipRegion>;

bool _dbChipRegion::operator==(const _dbChipRegion& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (side_ != rhs.side_) {
    return false;
  }
  if (layer_ != rhs.layer_) {
    return false;
  }
  if (box_ != rhs.box_) {
    return false;
  }
  if (*chip_bump_tbl_ != *rhs.chip_bump_tbl_) {
    return false;
  }

  return true;
}

bool _dbChipRegion::operator<(const _dbChipRegion& rhs) const
{
  return true;
}

_dbChipRegion::_dbChipRegion(_dbDatabase* db)
{
  side_ = static_cast<uint8_t>(dbChipRegion::Side::FRONT);
  chip_bump_tbl_ = new dbTable<_dbChipBump>(
      db, this, (GetObjTbl_t) &_dbChipRegion::getObjectTable, dbChipBumpObj);
}

dbIStream& operator>>(dbIStream& stream, _dbChipRegion& obj)
{
  stream >> obj.name_;
  stream >> obj.side_;
  stream >> obj.layer_;
  stream >> obj.box_;
  if (obj.getDatabase()->isSchema(kSchemaChipBump)) {
    stream >> *obj.chip_bump_tbl_;
  }
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbChipRegion& obj)
{
  stream << obj.name_;
  stream << obj.side_;
  stream << obj.layer_;
  stream << obj.box_;
  stream << *obj.chip_bump_tbl_;
  return stream;
}

dbObjectTable* _dbChipRegion::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbChipBumpObj:
      return chip_bump_tbl_;
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}
void _dbChipRegion::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  chip_bump_tbl_->collectMemInfo(info.children["chip_bump_tbl_"]);
}

_dbChipRegion::~_dbChipRegion()
{
  delete chip_bump_tbl_;
}

////////////////////////////////////////////////////////////////////
//
// dbChipRegion - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbChipRegion::getName() const
{
  _dbChipRegion* obj = (_dbChipRegion*) this;
  return obj->name_;
}

void dbChipRegion::setBox(const Rect& box)
{
  _dbChipRegion* obj = (_dbChipRegion*) this;

  obj->box_ = box;
}

Rect dbChipRegion::getBox() const
{
  _dbChipRegion* obj = (_dbChipRegion*) this;
  return obj->box_;
}

dbSet<dbChipBump> dbChipRegion::getChipBumps() const
{
  _dbChipRegion* obj = (_dbChipRegion*) this;
  return dbSet<dbChipBump>(obj, obj->chip_bump_tbl_);
}

// User Code Begin dbChipRegionPublicMethods

Cuboid dbChipRegion::getCuboid() const
{
  _dbChipRegion* obj = (_dbChipRegion*) this;
  Rect box = obj->box_;
  int z = 0;
  if (getSide() == dbChipRegion::Side::FRONT) {
    z = getChip()->getThickness();
  } else if (getSide() == dbChipRegion::Side::BACK) {
    z = 0;
  } else {
    z = getChip()->getThickness() / 2;
  }
  return Cuboid(box.xMin(), box.yMin(), z, box.xMax(), box.yMax(), z);
}

dbChip* dbChipRegion::getChip() const
{
  _dbChipRegion* obj = (_dbChipRegion*) this;
  return (dbChip*) obj->getOwner();
}

dbChipRegion::Side dbChipRegion::getSide() const
{
  _dbChipRegion* obj = (_dbChipRegion*) this;
  return static_cast<dbChipRegion::Side>(obj->side_);
}

dbTechLayer* dbChipRegion::getLayer() const
{
  _dbChipRegion* obj = (_dbChipRegion*) this;
  dbChip* chip = (dbChip*) obj->getOwner();
  if (chip->getBlock() == nullptr || chip->getBlock()->getTech() == nullptr
      || obj->layer_ == 0) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) chip->getBlock()->getTech();
  return (dbTechLayer*) tech->layer_tbl_->getPtr(obj->layer_);
}

dbChipRegion* dbChipRegion::create(dbChip* chip,
                                   const std::string& name,
                                   dbChipRegion::Side side,
                                   dbTechLayer* layer)
{
  if (chip == nullptr) {
    return nullptr;
  }

  _dbChip* _chip = (_dbChip*) chip;
  utl::Logger* logger = _chip->getImpl()->getLogger();
  if (layer != nullptr) {
    if (chip->getBlock() == nullptr) {
      logger->error(
          utl::ODB,
          508,
          "Cannot create chip region {} for chip {} because chip has no block",
          name,
          chip->getName());
    }
    if (chip->getBlock()->getTech() != layer->getTech()) {
      logger->error(utl::ODB,
                    509,
                    "Cannot create chip region {} layer {} is not in the same "
                    "tech as the chip",
                    name,
                    layer->getName());
    }
  }
  if (chip->findChipRegion(name) != nullptr) {
    logger->error(
        utl::ODB,
        520,
        "Cannot create chip region {} for chip {} because chip region "
        "already exists",
        name,
        chip->getName());
  }
  _dbTechLayer* _layer = (_dbTechLayer*) layer;

  // Create a new chip region
  _dbChipRegion* chip_region = _chip->chip_region_tbl_->create();
  _chip->chip_region_map_[name] = chip_region->getOID();
  // Initialize the chip region
  chip_region->name_ = name;
  chip_region->side_ = static_cast<uint8_t>(side);
  chip_region->layer_ = (_layer != nullptr) ? _layer->getOID() : 0;
  chip_region->box_ = Rect();  // Initialize with empty rectangle

  return (dbChipRegion*) chip_region;
}
// User Code End dbChipRegionPublicMethods
}  // namespace odb
// Generator Code End Cpp
