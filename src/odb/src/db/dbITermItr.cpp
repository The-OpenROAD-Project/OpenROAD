// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbITermItr.h"

#include "dbBlock.h"
#include "dbITerm.h"
#include "dbInst.h"
#include "dbInstHdr.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/dbId.h"
#include "odb/dbObject.h"

namespace odb {

////////////////////////////////////////////////
//
// dbNetITermItr - Methods
//
////////////////////////////////////////////////

bool dbNetITermItr::reversible() const
{
  return true;
}

bool dbNetITermItr::orderReversed() const
{
  return true;
}

void dbNetITermItr::reverse(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  uint id = net->iterms_;
  uint list = 0;

  while (id != 0) {
    _dbITerm* iterm = iterm_tbl_->getPtr(id);
    uint n = iterm->next_net_iterm_;
    iterm->next_net_iterm_ = list;
    list = id;
    id = n;
  }

  uint prev = 0;
  id = list;

  while (id != 0) {
    _dbITerm* iterm = iterm_tbl_->getPtr(id);
    iterm->prev_net_iterm_ = prev;
    prev = id;
    id = iterm->next_net_iterm_;
  }

  net->iterms_ = list;
}

uint dbNetITermItr::sequential() const
{
  return 0;
}

uint dbNetITermItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbNetITermItr::begin(parent); id != dbNetITermItr::end(parent);
       id = dbNetITermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbNetITermItr::begin(dbObject* parent) const
{
  _dbNet* net = (_dbNet*) parent;
  return (uint) net->iterms_;
}

uint dbNetITermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbNetITermItr::next(uint id, ...) const
{
  _dbITerm* iterm = iterm_tbl_->getPtr(id);
  return iterm->next_net_iterm_;
}

dbObject* dbNetITermItr::getObject(uint id, ...)
{
  return iterm_tbl_->getPtr(id);
}

////////////////////////////////////////////////
//
// dbInstITermItr - Methods
//
////////////////////////////////////////////////

bool dbInstITermItr::reversible() const
{
  return false;
}

bool dbInstITermItr::orderReversed() const
{
  return false;
}

void dbInstITermItr::reverse(dbObject* /* unused: parent */)
{
}

uint dbInstITermItr::sequential() const
{
  return 0;
}

uint dbInstITermItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbInstITermItr::begin(parent); id != dbInstITermItr::end(parent);
       id = dbInstITermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbInstITermItr::begin(dbObject* parent) const
{
  _dbInst* inst = (_dbInst*) parent;

  if (inst->iterms_.empty()) {
    return 0;
  }

  return inst->iterms_[0];
}

uint dbInstITermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbInstITermItr::next(uint id, ...) const
{
  _dbITerm* iterm = _iterm_tbl->getPtr(id);
  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  _dbInst* inst = block->inst_tbl_->getPtr(iterm->inst_);
  uint cnt = inst->iterms_.size();
  uint idx = iterm->flags_.mterm_idx + 1;

  if (idx == cnt) {
    return 0;
  }

  dbId<_dbITerm> next = inst->iterms_[idx];
  return next;
}

dbObject* dbInstITermItr::getObject(uint id, ...)
{
  return _iterm_tbl->getPtr(id);
}

}  // namespace odb
