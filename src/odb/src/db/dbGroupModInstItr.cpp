// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGroupModInstItr.h"

#include "dbGroup.h"
#include "dbModInst.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbGroupModInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbGroupModInstItr::reversible()
{
  return true;
}

bool dbGroupModInstItr::orderReversed()
{
  return true;
}

void dbGroupModInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbGroup* _parent = (_dbGroup*) parent;
  uint id = _parent->_modinsts;
  uint list = 0;

  while (id != 0) {
    _dbModInst* modinst = _modinst_tbl->getPtr(id);
    uint n = modinst->_group_next;
    modinst->_group_next = list;
    list = id;
    id = n;
  }
  _parent->_modinsts = list;
  // User Code End reverse
}

uint dbGroupModInstItr::sequential()
{
  return 0;
}

uint dbGroupModInstItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbGroupModInstItr::begin(parent);
       id != dbGroupModInstItr::end(parent);
       id = dbGroupModInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbGroupModInstItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbGroup* _parent = (_dbGroup*) parent;
  return _parent->_modinsts;
  // User Code End begin
}

uint dbGroupModInstItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbGroupModInstItr::next(uint id, ...)
{
  // User Code Begin next
  _dbModInst* modinst = _modinst_tbl->getPtr(id);
  return modinst->_group_next;
  // User Code End next
}

dbObject* dbGroupModInstItr::getObject(uint id, ...)
{
  return _modinst_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
