// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipInstItr.h"

#include <cstdint>

#include "dbChip.h"
#include "dbChipInst.h"
#include "dbTable.h"
// User Code Begin Includes
#include "odb/dbObject.h"
// User Code End Includes

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbChipInstItr::reversible() const
{
  return true;
}

bool dbChipInstItr::orderReversed() const
{
  return true;
}

void dbChipInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbChip* chip = (_dbChip*) parent;
  uint32_t id = chip->chipinsts_;
  uint32_t list = 0;

  while (id != 0) {
    _dbChipInst* chipinst = chip_inst_tbl_->getPtr(id);
    uint32_t n = chipinst->chipinst_next_;
    chipinst->chipinst_next_ = list;
    list = id;
    id = n;
  }
  chip->chipinsts_ = list;
  // User Code End reverse
}

uint32_t dbChipInstItr::sequential() const
{
  return 0;
}

uint32_t dbChipInstItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbChipInstItr::begin(parent); id != dbChipInstItr::end(parent);
       id = dbChipInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbChipInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbChip* chip = (_dbChip*) parent;
  return chip->chipinsts_;
  // User Code End begin
}

uint32_t dbChipInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbChipInstItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbChipInst* chipinst = chip_inst_tbl_->getPtr(id);
  return chipinst->chipinst_next_;
  // User Code End next
}

dbObject* dbChipInstItr::getObject(uint32_t id, ...)
{
  return chip_inst_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp
