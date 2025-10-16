// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbScanListScanInstItr.h"

#include "dbScanInst.h"
#include "dbScanList.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbScanListScanInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbScanListScanInstItr::reversible()
{
  return true;
}

bool dbScanListScanInstItr::orderReversed()
{
  return true;
}

void dbScanListScanInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbScanList* scan_list = (_dbScanList*) parent;
  uint current_id = scan_list->_first_scan_inst;
  uint new_head = 0;

  while (current_id != 0) {
    _dbScanInst* scan_inst = _scan_inst_tbl->getPtr(current_id);
    uint new_next = scan_inst->_prev_list_scan_inst;
    scan_inst->_prev_list_scan_inst = scan_inst->_next_list_scan_inst;
    scan_inst->_next_list_scan_inst = new_next;
    new_head = current_id;
    current_id = scan_inst->_prev_list_scan_inst;
  }

  scan_list->_first_scan_inst = new_head;
  // User Code End reverse
}

uint dbScanListScanInstItr::sequential()
{
  return 0;
}

uint dbScanListScanInstItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbScanListScanInstItr::begin(parent);
       id != dbScanListScanInstItr::end(parent);
       id = dbScanListScanInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbScanListScanInstItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbScanList* scan_list = (_dbScanList*) parent;
  return (uint) scan_list->_first_scan_inst;
  // User Code End begin
}

uint dbScanListScanInstItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbScanListScanInstItr::next(uint id, ...)
{
  // User Code Begin next
  _dbScanInst* scan_inst = _scan_inst_tbl->getPtr(id);
  return (uint) scan_inst->_next_list_scan_inst;
  // User Code End next
}

dbObject* dbScanListScanInstItr::getObject(uint id, ...)
{
  return _scan_inst_tbl->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp