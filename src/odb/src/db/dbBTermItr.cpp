// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBTermItr.h"

#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbNetBTermItr - Methods
//
////////////////////////////////////////////////////////////////////

//
// BTerms are ordered by io-type and cannot be reversed.
//
bool dbNetBTermItr::reversible()
{
  return true;
}

bool dbNetBTermItr::orderReversed()
{
  return true;
}

void dbNetBTermItr::reverse(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  uint id = net->_bterms;
  uint list = 0;

  while (id != 0) {
    _dbBTerm* bterm = _bterm_tbl->getPtr(id);
    uint n = bterm->_next_bterm;
    bterm->_next_bterm = list;
    list = id;
    id = n;
  }

  uint prev = 0;
  id = list;

  while (id != 0) {
    _dbBTerm* bterm = _bterm_tbl->getPtr(id);
    bterm->_prev_bterm = prev;
    prev = id;
    id = bterm->_next_bterm;
  }

  net->_bterms = list;
}

uint dbNetBTermItr::sequential()
{
  return 0;
}

uint dbNetBTermItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbNetBTermItr::begin(parent); id != dbNetBTermItr::end(parent);
       id = dbNetBTermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbNetBTermItr::begin(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  return net->_bterms;
}

uint dbNetBTermItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbNetBTermItr::next(uint id, ...)
{
  _dbBTerm* bterm = _bterm_tbl->getPtr(id);
  return bterm->_next_bterm;
}

dbObject* dbNetBTermItr::getObject(uint id, ...)
{
  return _bterm_tbl->getPtr(id);
}

}  // namespace odb
