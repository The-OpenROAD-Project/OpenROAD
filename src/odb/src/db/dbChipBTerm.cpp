// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipBTerm.h"

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
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbChipBTerm>;

bool _dbChipBTerm::operator==(const _dbChipBTerm& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
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
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbChipBTerm::operator<(const _dbChipBTerm& rhs) const
{
  return true;
}

_dbChipBTerm::_dbChipBTerm(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbChipBTerm& obj)
{
  stream >> obj.inst_;
  stream >> obj.chip_;
  stream >> obj.chip_region_;
  stream >> obj.net_;
  stream >> obj.bterm_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbChipBTerm& obj)
{
  stream << obj.inst_;
  stream << obj.chip_;
  stream << obj.chip_region_;
  stream << obj.net_;
  stream << obj.bterm_;
  return stream;
}

void _dbChipBTerm::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbChipBTerm - Methods
//
////////////////////////////////////////////////////////////////////

// User Code Begin dbChipBTermPublicMethods

dbChip* dbChipBTerm::getChip() const
{
  _dbChipBTerm* obj = (_dbChipBTerm*) this;
  _dbDatabase* db = obj->getDatabase();
  return (dbChip*) db->chip_tbl_->getPtr(obj->chip_);
}

dbChipRegion* dbChipBTerm::getChipRegion() const
{
  _dbChipBTerm* obj = (_dbChipBTerm*) this;
  _dbChip* chip = (_dbChip*) getChip();
  return (dbChipRegion*) chip->chip_region_tbl_->getPtr(obj->chip_region_);
}

dbInst* dbChipBTerm::getInst() const
{
  _dbChipBTerm* obj = (_dbChipBTerm*) this;
  if (obj->inst_ == 0) {
    return nullptr;
  }
  _dbBlock* block = (_dbBlock*) getChip()->getBlock();
  return (dbInst*) block->inst_tbl_->getPtr(obj->inst_);
}

dbNet* dbChipBTerm::getNet() const
{
  _dbChipBTerm* obj = (_dbChipBTerm*) this;
  if (obj->net_ == 0) {
    return nullptr;
  }
  _dbBlock* block = (_dbBlock*) getChip()->getBlock();
  return (dbNet*) block->net_tbl_->getPtr(obj->net_);
}

dbBTerm* dbChipBTerm::getBTerm() const
{
  _dbChipBTerm* obj = (_dbChipBTerm*) this;
  if (obj->bterm_ == 0) {
    return nullptr;
  }
  _dbBlock* block = (_dbBlock*) getChip()->getBlock();
  return (dbBTerm*) block->bterm_tbl_->getPtr(obj->bterm_);
}

void dbChipBTerm::setNet(dbNet* net)
{
  _dbChipBTerm* obj = (_dbChipBTerm*) this;
  obj->net_ = net->getId();
}

void dbChipBTerm::setBTerm(dbBTerm* bterm)
{
  _dbChipBTerm* obj = (_dbChipBTerm*) this;
  obj->bterm_ = bterm->getId();
  _dbBTerm* _bterm = (_dbBTerm*) bterm;
  _bterm->chip_region_ = obj->chip_region_;
  _bterm->chip_bterm_ = obj->getOID();
}

dbChipBTerm* dbChipBTerm::create(dbChipRegion* chip_region, dbInst* inst)
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
  }
  if (inst->getChipBTerm() != nullptr) {
    logger->error(
        utl::ODB,
        534,
        "Cannot create chip bump. Inst {} already has an associated chip bump",
        inst->getName());
  }

  _dbChipBTerm* obj = _chip_region->chip_bterm_tbl_->create();
  obj->inst_ = _inst->getOID();
  obj->chip_ = _chip->getOID();
  obj->chip_region_ = _chip_region->getOID();
  _inst->chip_region_ = _chip_region->getOID();
  _inst->bump_ = obj->getOID();
  return (dbChipBTerm*) obj;
}

// User Code End dbChipBTermPublicMethods
}  // namespace odb
// Generator Code End Cpp
