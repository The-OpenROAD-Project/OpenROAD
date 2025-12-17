// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGroupModInstItr.h"

#include "dbGroup.h"
#include "dbModInst.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbGroupModInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbGroupModInstItr::reversible() const
{
  return true;
}

bool dbGroupModInstItr::orderReversed() const
{
  return true;
}

void dbGroupModInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbGroup* _parent = (_dbGroup*) parent;
  uint id = _parent->modinsts_;
  uint list = 0;

  while (id != 0) {
    _dbModInst* modinst = modinst_tbl_->getPtr(id);
    uint n = modinst->group_next_;
    modinst->group_next_ = list;
    list = id;
    id = n;
  }
  _parent->modinsts_ = list;
  // User Code End reverse
}

uint dbGroupModInstItr::sequential() const
{
  return 0;
}

uint dbGroupModInstItr::size(dbObject* parent) const
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

uint dbGroupModInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbGroup* _parent = (_dbGroup*) parent;
  return _parent->modinsts_;
  // User Code End begin
}

uint dbGroupModInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbGroupModInstItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbModInst* modinst = modinst_tbl_->getPtr(id);
  return modinst->group_next_;
  // User Code End next
}

dbObject* dbGroupModInstItr::getObject(uint id, ...)
{
  return modinst_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
