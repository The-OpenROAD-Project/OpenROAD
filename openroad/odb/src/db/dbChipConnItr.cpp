// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipConnItr.h"

#include <cstdint>

#include "dbChip.h"
#include "dbChipConn.h"
#include "dbTable.h"
// User Code Begin Includes
#include "odb/dbObject.h"
// User Code End Includes

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipConnItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbChipConnItr::reversible() const
{
  return true;
}

bool dbChipConnItr::orderReversed() const
{
  return true;
}

void dbChipConnItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbChip* chip = (_dbChip*) parent;
  uint32_t id = chip->conns_;
  uint32_t list = 0;

  while (id != 0) {
    _dbChipConn* chipconn = chip_conn_tbl_->getPtr(id);
    uint32_t n = chipconn->chip_conn_next_;
    chipconn->chip_conn_next_ = list;
    list = id;
    id = n;
  }
  chip->conns_ = list;
  // User Code End reverse
}

uint32_t dbChipConnItr::sequential() const
{
  return 0;
}

uint32_t dbChipConnItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbChipConnItr::begin(parent); id != dbChipConnItr::end(parent);
       id = dbChipConnItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbChipConnItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbChip* chip = (_dbChip*) parent;
  return chip->conns_;
  // User Code End begin
}

uint32_t dbChipConnItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbChipConnItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbChipConn* chipconn = chip_conn_tbl_->getPtr(id);
  return chipconn->chip_conn_next_;
  // User Code End next
}

dbObject* dbChipConnItr::getObject(uint32_t id, ...)
{
  return chip_conn_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp
