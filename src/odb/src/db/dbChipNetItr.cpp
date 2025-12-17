// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipNetItr.h"

#include "dbChip.h"
#include "dbChipNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipNetItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbChipNetItr::reversible() const
{
  return true;
}

bool dbChipNetItr::orderReversed() const
{
  return true;
}

void dbChipNetItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbChip* chip = (_dbChip*) parent;
  uint id = chip->nets_;
  uint list = 0;

  while (id != 0) {
    _dbChipNet* chipnet = chip_net_tbl_->getPtr(id);
    uint n = chipnet->chip_net_next_;
    chipnet->chip_net_next_ = list;
    list = id;
    id = n;
  }
  chip->nets_ = list;
  // User Code End reverse
}

uint dbChipNetItr::sequential() const
{
  return 0;
}

uint dbChipNetItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbChipNetItr::begin(parent); id != dbChipNetItr::end(parent);
       id = dbChipNetItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbChipNetItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbChip* chip = (_dbChip*) parent;
  return chip->nets_;
  // User Code End begin
}

uint dbChipNetItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbChipNetItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbChipNet* chipnet = chip_net_tbl_->getPtr(id);
  return chipnet->chip_net_next_;
  // User Code End next
}

dbObject* dbChipNetItr::getObject(uint id, ...)
{
  return chip_net_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp
