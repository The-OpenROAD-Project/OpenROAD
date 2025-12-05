// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbScanListScanInstItr.h"

#include "dbScanInst.h"
#include "dbScanList.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbScanListScanInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbScanListScanInstItr::reversible() const
{
  return true;
}

bool dbScanListScanInstItr::orderReversed() const
{
  return true;
}

void dbScanListScanInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbScanList* scan_list = (_dbScanList*) parent;
  uint current_id = scan_list->first_scan_inst_;
  uint new_head = 0;

  while (current_id != 0) {
    _dbScanInst* scan_inst = scan_inst_tbl_->getPtr(current_id);
    uint new_next = scan_inst->prev_list_scan_inst_;
    scan_inst->prev_list_scan_inst_ = scan_inst->next_list_scan_inst_;
    scan_inst->next_list_scan_inst_ = new_next;
    new_head = current_id;
    current_id = scan_inst->prev_list_scan_inst_;
  }

  scan_list->first_scan_inst_ = new_head;
  // User Code End reverse
}

uint dbScanListScanInstItr::sequential() const
{
  return 0;
}

uint dbScanListScanInstItr::size(dbObject* parent) const
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

uint dbScanListScanInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbScanList* scan_list = (_dbScanList*) parent;
  return (uint) scan_list->first_scan_inst_;
  // User Code End begin
}

uint dbScanListScanInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbScanListScanInstItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbScanInst* scan_inst = scan_inst_tbl_->getPtr(id);
  return (uint) scan_inst->next_list_scan_inst_;
  // User Code End next
}

dbObject* dbScanListScanInstItr::getObject(uint id, ...)
{
  return scan_inst_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp
