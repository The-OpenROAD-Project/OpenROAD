// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipBump.h"

#include "dbChip.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbInst.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbChipRegion.h"
#include "dbNet.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbChipBump>;

bool _dbChipBump::operator==(const _dbChipBump& rhs) const
{
  if (inst_ != rhs.inst_) {
    return false;
  }
  if (chip_ != rhs.chip_) {
    return false;
  }
  if (chip_region_ != rhs.chip_region_) {
    return false;
  }
  if (net_ != rhs.net_) {
    return false;
  }
  if (bterm_ != rhs.bterm_) {
    return false;
  }

  return true;
}

bool _dbChipBump::operator<(const _dbChipBump& rhs) const
{
  return true;
}

_dbChipBump::_dbChipBump(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbChipBump& obj)
{
  stream >> obj.inst_;
  stream >> obj.chip_;
  stream >> obj.chip_region_;
  stream >> obj.net_;
  stream >> obj.bterm_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbChipBump& obj)
{
  stream << obj.inst_;
  stream << obj.chip_;
  stream << obj.chip_region_;
  stream << obj.net_;
  stream << obj.bterm_;
  return stream;
}

void _dbChipBump::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbChipBump - Methods
//
////////////////////////////////////////////////////////////////////

// User Code Begin dbChipBumpPublicMethods

dbChip* dbChipBump::getChip() const
{
  _dbChipBump* obj = (_dbChipBump*) this;
  _dbDatabase* db = obj->getDatabase();
  return (dbChip*) db->chip_tbl_->getPtr(obj->chip_);
}

dbChipRegion* dbChipBump::getChipRegion() const
{
  _dbChipBump* obj = (_dbChipBump*) this;
  _dbChip* chip = (_dbChip*) getChip();
  return (dbChipRegion*) chip->chip_region_tbl_->getPtr(obj->chip_region_);
}

dbInst* dbChipBump::getInst() const
{
  _dbChipBump* obj = (_dbChipBump*) this;
  if (obj->inst_ == 0) {
    return nullptr;
  }
  _dbBlock* block = (_dbBlock*) getChip()->getBlock();
  return (dbInst*) block->inst_tbl_->getPtr(obj->inst_);
}

dbNet* dbChipBump::getNet() const
{
  _dbChipBump* obj = (_dbChipBump*) this;
  if (obj->net_ == 0) {
    return nullptr;
  }
  _dbBlock* block = (_dbBlock*) getChip()->getBlock();
  return (dbNet*) block->net_tbl_->getPtr(obj->net_);
}

dbBTerm* dbChipBump::getBTerm() const
{
  _dbChipBump* obj = (_dbChipBump*) this;
  if (obj->bterm_ == 0) {
    return nullptr;
  }
  _dbBlock* block = (_dbBlock*) getChip()->getBlock();
  return (dbBTerm*) block->bterm_tbl_->getPtr(obj->bterm_);
}

void dbChipBump::setNet(dbNet* net)
{
  _dbChipBump* obj = (_dbChipBump*) this;
  obj->net_ = net->getId();
}

void dbChipBump::setBTerm(dbBTerm* bterm)
{
  _dbChipBump* obj = (_dbChipBump*) this;
  obj->bterm_ = bterm->getId();
}

dbChipBump* dbChipBump::create(dbChipRegion* chip_region, dbInst* inst)
{
  if (chip_region == nullptr || inst == nullptr
      || chip_region->getChip() == nullptr
      || chip_region->getChip()->getBlock() == nullptr) {
    return nullptr;
  }
  _dbChipRegion* _chip_region = (_dbChipRegion*) chip_region;
  _dbChip* _chip = (_dbChip*) chip_region->getChip();
  _dbInst* _inst = (_dbInst*) inst;
  utl::Logger* logger = _chip->getLogger();
  if (inst->getBlock() != chip_region->getChip()->getBlock()) {
    logger->error(utl::ODB,
                  513,
                  "Cannot create chip bump. Inst {} is not in the same block "
                  "as the chip region {}",
                  inst->getName(),
                  chip_region->getName());
    return nullptr;
  }
  _dbChipBump* obj = _chip_region->chip_bump_tbl_->create();
  obj->inst_ = _inst->getOID();
  obj->chip_ = _chip->getOID();
  obj->chip_region_ = _chip_region->getOID();
  return (dbChipBump*) obj;
}

// User Code End dbChipBumpPublicMethods
}  // namespace odb
// Generator Code End Cpp
