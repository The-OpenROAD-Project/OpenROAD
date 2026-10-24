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
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbChipBump>;

bool _dbChipBump::operator==(const _dbChipBump& rhs) const
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

// The net and port of a bump live on its dbInst; the stored net_ and
// bterm_ are still written but no longer read for answers. A bump's pin
// is its inst's first iterm (bump masters have exactly one pin).
dbNet* dbChipBump::getNet() const
{
  return getInst()->getITerm(0U)->getNet();
}

dbBTerm* dbChipBump::getBTerm() const
{
  return getInst()->getBumpBTerm();
}

void dbChipBump::setNet(dbNet* net)
{
  _dbChipBump* obj = (_dbChipBump*) this;
  dbITerm* pin = getInst()->getITerm(0U);
  obj->net_ = net != nullptr ? net->getId() : 0;
  if (net != nullptr) {
    pin->connect(net);
  } else {
    pin->disconnect();
  }
}

void dbChipBump::setBTerm(dbBTerm* bterm)
{
  _dbChipBump* obj = (_dbChipBump*) this;
  obj->bterm_ = bterm->getId();
  _dbBTerm* _bterm = (_dbBTerm*) bterm;
  _bterm->chip_region_ = obj->chip_region_;
  _bterm->chip_bump_ = obj->getOID();
  getInst()->setBumpBTerm(bterm);
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
  }
  if (inst->getChipBump() != nullptr) {
    logger->error(
        utl::ODB,
        534,
        "Cannot create chip bump. Inst {} already has an associated chip bump",
        inst->getName());
  }

  _dbInst::checkBumpMaster(_inst, _chip_region);

  _dbChipBump* obj = _chip_region->chip_bump_tbl_->create();
  obj->inst_ = _inst->getOID();
  obj->chip_ = _chip->getOID();
  obj->chip_region_ = _chip_region->getOID();
  if (_inst->chip_region_ != _chip_region->getOID()) {
    _dbInst::unlinkChipRegion(_inst);
    _dbInst::linkChipRegion(_inst, _chip_region);
  }
  _inst->bump_ = obj->getOID();
  return (dbChipBump*) obj;
}

// User Code End dbChipBumpPublicMethods
}  // namespace odb
// Generator Code End Cpp
