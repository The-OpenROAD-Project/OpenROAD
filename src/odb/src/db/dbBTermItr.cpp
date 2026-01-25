// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBTermItr.h"

#include <cstdint>

#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbNet.h"
#include "dbTable.h"
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
  uint32_t id = net->bterms_;
  uint32_t list = 0;

  while (id != 0) {
    _dbBTerm* bterm = bterm_tbl_->getPtr(id);
    uint32_t n = bterm->next_bterm_;
    bterm->next_bterm_ = list;
    list = id;
    id = n;
  }

  uint32_t prev = 0;
  id = list;

  while (id != 0) {
    _dbBTerm* bterm = bterm_tbl_->getPtr(id);
    bterm->prev_bterm_ = prev;
    prev = id;
    id = bterm->next_bterm_;
  }

  net->bterms_ = list;
}

uint32_t dbNetBTermItr::sequential() const
{
  return 0;
}

uint32_t dbNetBTermItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbNetBTermItr::begin(parent); id != dbNetBTermItr::end(parent);
       id = dbNetBTermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbNetBTermItr::begin(dbObject* parent) const
{
  _dbNet* net = (_dbNet*) parent;
  return net->bterms_;
}

uint32_t dbNetBTermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbNetBTermItr::next(uint32_t id, ...) const
{
  _dbBTerm* bterm = bterm_tbl_->getPtr(id);
  return bterm->next_bterm_;
}

dbObject* dbNetBTermItr::getObject(uint32_t id, ...)
{
  return bterm_tbl_->getPtr(id);
}

}  // namespace odb
