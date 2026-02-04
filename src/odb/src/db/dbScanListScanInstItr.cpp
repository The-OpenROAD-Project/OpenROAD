// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbScanListScanInstItr.h"

#include <cstdint>

#include "dbScanInst.h"
#include "dbScanList.h"
#include "dbTable.h"

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
  uint32_t current_id = scan_list->first_scan_inst_;
  uint32_t new_head = 0;

  while (current_id != 0) {
    _dbScanInst* scan_inst = scan_inst_tbl_->getPtr(current_id);
    uint32_t new_next = scan_inst->prev_list_scan_inst_;
    scan_inst->prev_list_scan_inst_ = scan_inst->next_list_scan_inst_;
    scan_inst->next_list_scan_inst_ = new_next;
    new_head = current_id;
    current_id = scan_inst->prev_list_scan_inst_;
  }

  scan_list->first_scan_inst_ = new_head;
  // User Code End reverse
}

uint32_t dbScanListScanInstItr::sequential() const
{
  return 0;
}

uint32_t dbScanListScanInstItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbScanListScanInstItr::begin(parent);
       id != dbScanListScanInstItr::end(parent);
       id = dbScanListScanInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbScanListScanInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbScanList* scan_list = (_dbScanList*) parent;
  return (uint32_t) scan_list->first_scan_inst_;
  // User Code End begin
}

uint32_t dbScanListScanInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbScanListScanInstItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbScanInst* scan_inst = scan_inst_tbl_->getPtr(id);
  return (uint32_t) scan_inst->next_list_scan_inst_;
  // User Code End next
}

dbObject* dbScanListScanInstItr::getObject(uint32_t id, ...)
{
  return scan_inst_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp
