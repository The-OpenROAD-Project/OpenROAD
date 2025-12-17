// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipConnItr.h"

#include "dbChip.h"
#include "dbChipConn.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"

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
  uint id = chip->conns_;
  uint list = 0;

  while (id != 0) {
    _dbChipConn* chipconn = chip_conn_tbl_->getPtr(id);
    uint n = chipconn->chip_conn_next_;
    chipconn->chip_conn_next_ = list;
    list = id;
    id = n;
  }
  chip->conns_ = list;
  // User Code End reverse
}

uint dbChipConnItr::sequential() const
{
  return 0;
}

uint dbChipConnItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbChipConnItr::begin(parent); id != dbChipConnItr::end(parent);
       id = dbChipConnItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbChipConnItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbChip* chip = (_dbChip*) parent;
  return chip->conns_;
  // User Code End begin
}

uint dbChipConnItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbChipConnItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbChipConn* chipconn = chip_conn_tbl_->getPtr(id);
  return chipconn->chip_conn_next_;
  // User Code End next
}

dbObject* dbChipConnItr::getObject(uint id, ...)
{
  return chip_conn_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp
