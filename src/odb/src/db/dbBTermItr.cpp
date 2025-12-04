// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBTermItr.h"

#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/dbObject.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbNetBTermItr - Methods
//
////////////////////////////////////////////////////////////////////

//
// BTerms are ordered by io-type and cannot be reversed.
//
bool dbNetBTermItr::reversible() const
{
  return true;
}

bool dbNetBTermItr::orderReversed() const
{
  return true;
}

void dbNetBTermItr::reverse(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  uint id = net->bterms_;
  uint list = 0;

  while (id != 0) {
    _dbBTerm* bterm = bterm_tbl_->getPtr(id);
    uint n = bterm->next_bterm_;
    bterm->next_bterm_ = list;
    list = id;
    id = n;
  }

  uint prev = 0;
  id = list;

  while (id != 0) {
    _dbBTerm* bterm = bterm_tbl_->getPtr(id);
    bterm->prev_bterm_ = prev;
    prev = id;
    id = bterm->next_bterm_;
  }

  net->bterms_ = list;
}

uint dbNetBTermItr::sequential() const
{
  return 0;
}

uint dbNetBTermItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbNetBTermItr::begin(parent); id != dbNetBTermItr::end(parent);
       id = dbNetBTermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbNetBTermItr::begin(dbObject* parent) const
{
  _dbNet* net = (_dbNet*) parent;
  return net->bterms_;
}

uint dbNetBTermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbNetBTermItr::next(uint id, ...) const
{
  _dbBTerm* bterm = bterm_tbl_->getPtr(id);
  return bterm->next_bterm_;
}

dbObject* dbNetBTermItr::getObject(uint id, ...)
{
  return bterm_tbl_->getPtr(id);
}

}  // namespace odb
