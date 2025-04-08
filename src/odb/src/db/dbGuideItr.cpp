// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGuideItr.h"

#include "dbGuide.h"
#include "dbTable.h"
#include "dbTable.hpp"
// User Code Begin Includes
#include "dbNet.h"
// User Code End Includes

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbGuideItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbGuideItr::reversible()
{
  return true;
}

bool dbGuideItr::orderReversed()
{
  return true;
}

void dbGuideItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbNet* _parent = (_dbNet*) parent;
  uint id = _parent->guides_;
  uint list = 0;

  while (id != 0) {
    _dbGuide* _child = _guide_tbl->getPtr(id);
    uint n = _child->guide_next_;
    _child->guide_next_ = list;
    list = id;
    id = n;
  }
  _parent->guides_ = list;
  // User Code End reverse
}

uint dbGuideItr::sequential()
{
  return 0;
}

uint dbGuideItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbGuideItr::begin(parent); id != dbGuideItr::end(parent);
       id = dbGuideItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbGuideItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbNet* _parent = (_dbNet*) parent;
  return _parent->guides_;
  // User Code End begin
}

uint dbGuideItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbGuideItr::next(uint id, ...)
{
  // User Code Begin next
  _dbGuide* _guide = _guide_tbl->getPtr(id);
  return _guide->guide_next_;
  // User Code End next
}

dbObject* dbGuideItr::getObject(uint id, ...)
{
  return _guide_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
