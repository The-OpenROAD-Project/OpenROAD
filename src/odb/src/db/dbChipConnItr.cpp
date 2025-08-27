// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipConnItr.h"

#include "dbChip.h"
#include "dbChipConn.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipConnItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbChipConnItr::reversible()
{
  return true;
}

bool dbChipConnItr::orderReversed()
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
    _dbChipConn* chipconn = _chip_conn_tbl->getPtr(id);
    uint n = chipconn->chip_conn_next_;
    chipconn->chip_conn_next_ = list;
    list = id;
    id = n;
  }
  chip->conns_ = list;
  // User Code End reverse
}

uint dbChipConnItr::sequential()
{
  return 0;
}

uint dbChipConnItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbChipConnItr::begin(parent); id != dbChipConnItr::end(parent);
       id = dbChipConnItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbChipConnItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbChip* chip = (_dbChip*) parent;
  return chip->conns_;
  // User Code End begin
}

uint dbChipConnItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbChipConnItr::next(uint id, ...)
{
  // User Code Begin next
  _dbChipConn* chipconn = _chip_conn_tbl->getPtr(id);
  return chipconn->chip_conn_next_;
  // User Code End next
}

dbObject* dbChipConnItr::getObject(uint id, ...)
{
  return _chip_conn_tbl->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp