// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipInstItr.h"

#include "dbChip.h"
#include "dbChipInst.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbChipInstItr::reversible()
{
  return true;
}

bool dbChipInstItr::orderReversed()
{
  return true;
}

void dbChipInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbChip* chip = (_dbChip*) parent;
  uint id = chip->chipinsts_;
  uint list = 0;

  while (id != 0) {
    _dbChipInst* chipinst = _chip_inst_tbl->getPtr(id);
    uint n = chipinst->chipinst_next_;
    chipinst->chipinst_next_ = list;
    list = id;
    id = n;
  }
  chip->chipinsts_ = list;
  // User Code End reverse
}

uint dbChipInstItr::sequential()
{
  return 0;
}

uint dbChipInstItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbChipInstItr::begin(parent); id != dbChipInstItr::end(parent);
       id = dbChipInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbChipInstItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbChip* chip = (_dbChip*) parent;
  return chip->chipinsts_;
  // User Code End begin
}

uint dbChipInstItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbChipInstItr::next(uint id, ...)
{
  // User Code Begin next
  _dbChipInst* chipinst = _chip_inst_tbl->getPtr(id);
  return chipinst->chipinst_next_;
  // User Code End next
}

dbObject* dbChipInstItr::getObject(uint id, ...)
{
  return _chip_inst_tbl->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp