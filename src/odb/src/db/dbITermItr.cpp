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

namespace odb {

////////////////////////////////////////////////
//
// dbNetITermItr - Methods
//
////////////////////////////////////////////////

bool dbNetITermItr::reversible()
{
  return true;
}

bool dbNetITermItr::orderReversed()
{
  return true;
}

void dbNetITermItr::reverse(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  uint id = net->_iterms;
  uint list = 0;

  while (id != 0) {
    _dbITerm* iterm = _iterm_tbl->getPtr(id);
    uint n = iterm->_next_net_iterm;
    iterm->_next_net_iterm = list;
    list = id;
    id = n;
  }

  uint prev = 0;
  id = list;

  while (id != 0) {
    _dbITerm* iterm = _iterm_tbl->getPtr(id);
    iterm->_prev_net_iterm = prev;
    prev = id;
    id = iterm->_next_net_iterm;
  }

  net->_iterms = list;
}

uint dbNetITermItr::sequential()
{
  return 0;
}

uint dbNetITermItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbNetITermItr::begin(parent); id != dbNetITermItr::end(parent);
       id = dbNetITermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbNetITermItr::begin(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  return (uint) net->_iterms;
}

uint dbNetITermItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbNetITermItr::next(uint id, ...)
{
  _dbITerm* iterm = _iterm_tbl->getPtr(id);
  return iterm->_next_net_iterm;
}

dbObject* dbNetITermItr::getObject(uint id, ...)
{
  return _iterm_tbl->getPtr(id);
}

////////////////////////////////////////////////
//
// dbInstITermItr - Methods
//
////////////////////////////////////////////////

bool dbInstITermItr::reversible()
{
  return false;
}

bool dbInstITermItr::orderReversed()
{
  return false;
}

void dbInstITermItr::reverse(dbObject* /* unused: parent */)
{
}

uint dbInstITermItr::sequential()
{
  return 0;
}

uint dbInstITermItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbInstITermItr::begin(parent); id != dbInstITermItr::end(parent);
       id = dbInstITermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbInstITermItr::begin(dbObject* parent)
{
  _dbInst* inst = (_dbInst*) parent;

  if (inst->_iterms.empty()) {
    return 0;
  }

  return inst->_iterms[0];
}

uint dbInstITermItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbInstITermItr::next(uint id, ...)
{
  _dbITerm* iterm = _iterm_tbl->getPtr(id);
  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  _dbInst* inst = block->_inst_tbl->getPtr(iterm->_inst);
  uint cnt = inst->_iterms.size();
  uint idx = iterm->_flags._mterm_idx + 1;

  if (idx == cnt) {
    return 0;
  }

  dbId<_dbITerm> next = inst->_iterms[idx];
  return next;
}

dbObject* dbInstITermItr::getObject(uint id, ...)
{
  return _iterm_tbl->getPtr(id);
}

}  // namespace odb
