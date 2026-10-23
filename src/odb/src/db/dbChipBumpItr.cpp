// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipBumpItr.h"

#include <cstdint>

#include "dbBlock.h"
#include "dbChip.h"
#include "dbChipRegion.h"
#include "dbInst.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipBumpItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbChipBumpItr::reversible() const
{
  return false;
}

bool dbChipBumpItr::orderReversed() const
{
  return false;
}

void dbChipBumpItr::reverse(dbObject* parent)
{
}

uint32_t dbChipBumpItr::sequential() const
{
  return 0;
}

uint32_t dbChipBumpItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbChipBumpItr::begin(parent); id != dbChipBumpItr::end(parent);
       id = dbChipBumpItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbChipBumpItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbChip* chip = (_dbChip*) parent;
  return firstBumpFrom(chip, chip->chip_region_tbl_->begin(parent));
  // User Code End begin
}

uint32_t dbChipBumpItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbChipBumpItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbInst* inst = inst_tbl_->getPtr(id);
  if (inst->chip_region_next_ != 0) {
    return inst->chip_region_next_;
  }
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbChip* chip = (_dbChip*) block->getOwner();
  return firstBumpFrom(chip, chip->chip_region_tbl_->next(inst->chip_region_));
  // User Code End next
}

dbObject* dbChipBumpItr::getObject(uint32_t id, ...)
{
  return inst_tbl_->getPtr(id);
}
// User Code Begin Methods
uint32_t dbChipBumpItr::firstBumpFrom(_dbChip* chip, uint32_t region_id)
{
  dbTable<_dbChipRegion>* tbl = chip->chip_region_tbl_;
  for (uint32_t id = region_id; id != 0; id = tbl->next(id)) {
    const uint32_t bumps = tbl->getPtr(id)->bumps_;
    if (bumps != 0) {
      return bumps;
    }
  }
  return 0;
}
// User Code End Methods
}  // namespace odb
// Generator Code End Cpp