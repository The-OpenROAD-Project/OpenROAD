// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipRegion.h"

#include <string>

#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbChip.h"
#include "dbTech.h"
#include "dbTechLayer.h"
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

  return true;
}

bool _dbChipRegion::operator<(const _dbChipRegion& rhs) const
{
  return true;
}

_dbChipRegion::_dbChipRegion(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbChipRegion& obj)
{
  stream >> obj.name_;
  stream >> obj.side_;
  stream >> obj.layer_;
  stream >> obj.box_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbChipRegion& obj)
{
  stream << obj.name_;
  stream << obj.side_;
  stream << obj.layer_;
  stream << obj.box_;
  return stream;
}

void _dbChipRegion::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
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

// User Code Begin dbChipRegionPublicMethods
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
  return (dbTechLayer*) tech->_layer_tbl->getPtr(obj->layer_);
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
  _dbTechLayer* _layer = (_dbTechLayer*) layer;

  // Create a new chip region
  _dbChipRegion* chip_region = _chip->chip_region_tbl_->create();

  // Initialize the chip region
  chip_region->name_ = name;
  chip_region->side_ = static_cast<uint8_t>(side);
  chip_region->layer_ = (_layer != nullptr) ? _layer->getOID() : 0;
  chip_region->box_ = Rect();  // Initialize with empty rectangle

  return (dbChipRegion*) chip_region;
}

void dbChipRegion::destroy(dbChipRegion* chipRegion)
{
  if (chipRegion == nullptr) {
    return;
  }

  _dbChipRegion* chip_region = (_dbChipRegion*) chipRegion;
  _dbChip* chip = (_dbChip*) chip_region->getOwner();

  chip->chip_region_tbl_->destroy(chip_region);
}
// User Code End dbChipRegionPublicMethods
}  // namespace odb
// Generator Code End Cpp