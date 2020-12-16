///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "dbITermItr.h"

#include "dbBlock.h"
#include "dbITerm.h"
#include "dbInst.h"
#include "dbInstHdr.h"
#include "dbNet.h"
#include "dbTable.h"

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
  _dbNet* net  = (_dbNet*) parent;
  uint    id   = net->_iterms;
  uint    list = 0;

  while (id != 0) {
    _dbITerm* iterm        = _iterm_tbl->getPtr(id);
    uint      n            = iterm->_next_net_iterm;
    iterm->_next_net_iterm = list;
    list                   = id;
    id                     = n;
  }

  uint prev = 0;
  id        = list;

  while (id != 0) {
    _dbITerm* iterm        = _iterm_tbl->getPtr(id);
    iterm->_prev_net_iterm = prev;
    prev                   = id;
    id                     = iterm->_next_net_iterm;
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
       id = dbNetITermItr::next(id))
    ++cnt;

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
       id = dbInstITermItr::next(id))
    ++cnt;

  return cnt;
}

uint dbInstITermItr::begin(dbObject* parent)
{
  _dbInst* inst = (_dbInst*) parent;

  if (inst->_iterms.size() == 0)
    return 0;

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
  _dbInst*  inst  = block->_inst_tbl->getPtr(iterm->_inst);
  uint      cnt   = inst->_iterms.size();
  uint      idx   = iterm->_flags._mterm_idx + 1;

  if (idx == cnt)
    return 0;

  dbId<_dbITerm> next = inst->_iterms[idx];
  return next;
}

dbObject* dbInstITermItr::getObject(uint id, ...)
{
  return _iterm_tbl->getPtr(id);
}

}  // namespace odb
