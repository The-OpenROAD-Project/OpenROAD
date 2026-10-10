// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipITermItr.h"

#include <cstdint>

#include "dbChipITerm.h"
#include "dbChipRegionInst.h"
#include "dbTable.h"
// User Code Begin Includes
#include "odb/dbObject.h"
// User Code End Includes

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipITermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbChipITermItr::reversible() const
{
  return true;
}

bool dbChipITermItr::orderReversed() const
{
  return true;
}

void dbChipITermItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbChipRegionInst* chip_region_inst = (_dbChipRegionInst*) parent;
  uint32_t id = chip_region_inst->chip_iterms_;
  uint32_t list = 0;

  while (id != 0) {
    _dbChipITerm* chip_iterm = chip_iterm_tbl_->getPtr(id);
    uint32_t n = chip_iterm->region_next_;
    chip_iterm->region_next_ = list;
    list = id;
    id = n;
  }
  chip_region_inst->chip_iterms_ = list;
  // User Code End reverse
}

uint32_t dbChipITermItr::sequential() const
{
  return 0;
}

uint32_t dbChipITermItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbChipITermItr::begin(parent); id != dbChipITermItr::end(parent);
       id = dbChipITermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbChipITermItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbChipRegionInst* chip_region_inst = (_dbChipRegionInst*) parent;
  return chip_region_inst->chip_iterms_;
  // User Code End begin
}

uint32_t dbChipITermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbChipITermItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbChipITerm* chip_iterm = chip_iterm_tbl_->getPtr(id);
  return chip_iterm->region_next_;
  // User Code End next
}

dbObject* dbChipITermItr::getObject(uint32_t id, ...)
{
  return chip_iterm_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp
