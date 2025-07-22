// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "dbScanInstItr.h"

#include "dbScanInst.h"
#include "dbScanList.h"
#include "dbTable.h"

namespace odb {

bool dbScanListScanInstItr::reversible()
{
  return false;
}

bool dbScanListScanInstItr::orderReversed()
{
  return false;
}

void dbScanListScanInstItr::reverse(dbObject* /* unused: parent */)
{
}

uint dbScanListScanInstItr::sequential()
{
  return 0;
}

uint dbScanListScanInstItr::size(dbObject* parent)
{
  uint cnt = 0;
  for (uint id = begin(parent); id != end(parent); id = next(id)) {
    ++cnt;
  }
  return cnt;
}

uint dbScanListScanInstItr::begin(dbObject* parent)
{
  _dbScanList* scan_list = (_dbScanList*) parent;
  return (uint) scan_list->_first_scan_inst;
}

uint dbScanListScanInstItr::end(dbObject* parent)
{
  return 0;
}

uint dbScanListScanInstItr::next(uint id, ...)
{
  _dbScanInst* scan_inst = _scan_inst_tbl->getPtr(id);
  return (uint) scan_inst->_next_list_scan_inst;
}

dbObject* dbScanListScanInstItr::getObject(uint id, ...)
{
  return _scan_inst_tbl->getPtr(id);
}

}  // namespace odb