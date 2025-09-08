// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipNetItr.h"

#include "dbChip.h"
#include "dbChipNet.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipNetItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbChipNetItr::reversible()
{
  return true;
}

bool dbChipNetItr::orderReversed()
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
    _dbChipNet* chipnet = _chip_net_tbl->getPtr(id);
    uint n = chipnet->chip_net_next_;
    chipnet->chip_net_next_ = list;
    list = id;
    id = n;
  }
  chip->nets_ = list;
  // User Code End reverse
}

uint dbChipNetItr::sequential()
{
  return 0;
}

uint dbChipNetItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbChipNetItr::begin(parent); id != dbChipNetItr::end(parent);
       id = dbChipNetItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbChipNetItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbChip* chip = (_dbChip*) parent;
  return chip->nets_;
  // User Code End begin
}

uint dbChipNetItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbChipNetItr::next(uint id, ...)
{
  // User Code Begin next
  _dbChipNet* chipnet = _chip_net_tbl->getPtr(id);
  return chipnet->chip_net_next_;
  // User Code End next
}

dbObject* dbChipNetItr::getObject(uint id, ...)
{
  return _chip_net_tbl->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp