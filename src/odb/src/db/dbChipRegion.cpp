// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipRegion.h"

#include <string>

#include "dbChipBump.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
#include "odb/dbSet.h"
// User Code Begin Includes
#include <algorithm>

#include "dbChip.h"
#include "dbChipInst.h"
#include "dbChipRegionInst.h"
#include "dbTech.h"
#include "odb/dbTypes.h"
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

// Returns the region's cuboid in the master chip's coordinate system.
// Note: This differs from dbChipRegionInst::getCuboid() which returns
// the transformed cuboid in the parent's coordinate system.
Cuboid dbChipRegion::getCuboid() const
{
  _dbChipRegion* obj = (_dbChipRegion*) this;
  dbTechLayer* layer = getLayer();
  const int chip_thick = getChip()->getThickness();

  int z_bot = 0;
  int z_top = 0;

  if (layer) {
    dbTech* tech = layer->getTech();
    int total_tech_thick = 0;
    int layer_z = 0;
    uint32_t target_thick = 0;

    if (tech) {
      for (dbTechLayer* l : tech->getLayers()) {
        uint32_t thick = 0;
        if (!l->getThickness(thick)) {
          continue;
        }

        total_tech_thick += thick;
        if (l == layer) {
          target_thick = thick;
          layer_z = total_tech_thick;
        }
      }
    }

    const auto side = getSide();
    if (side == Side::BACK) {
      z_top = layer_z;
      z_bot = layer_z - target_thick;
    } else if (side == Side::FRONT || side == Side::INTERNAL
               || side == Side::INTERNAL_EXT) {
      z_top = std::max(0, chip_thick - (total_tech_thick - layer_z));
      z_bot = z_top - target_thick;
    } else {
      z_bot = z_top = chip_thick / 2;
    }
  } else {
    const auto side = getSide();
    if (side == Side::FRONT) {
      z_bot = z_top = chip_thick;
    } else if (side == Side::BACK) {
      z_bot = z_top = 0;
    } else {
      z_bot = z_top = chip_thick / 2;
    }
  }

  return Cuboid(obj->box_.xMin(),
                obj->box_.yMin(),
                z_bot,
                obj->box_.xMax(),
                obj->box_.yMax(),
                z_top);
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

  dbTech* tech
      = chip->getBlock() ? chip->getBlock()->getTech() : chip->getTech();
  if (!tech) {
    return nullptr;
  }

  if (obj->layer_ != 0) {
    return (dbTechLayer*) ((_dbTech*) tech)->layer_tbl_->getPtr(obj->layer_);
  }

  if (auto* prop
      = dbStringProperty::find((dbChipRegion*) this, "3dblox_layer")) {
    return tech->findLayer(prop->getValue().c_str());
  }

  return nullptr;
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
