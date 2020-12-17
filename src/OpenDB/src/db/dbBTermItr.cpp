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

#include "dbBTermItr.h"

#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbNet.h"
#include "dbTable.h"

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
  _dbNet* net  = (_dbNet*) parent;
  uint    id   = net->_bterms;
  uint    list = 0;

  while (id != 0) {
    _dbBTerm* bterm    = _bterm_tbl->getPtr(id);
    uint      n        = bterm->_next_bterm;
    bterm->_next_bterm = list;
    list               = id;
    id                 = n;
  }

  uint prev = 0;
  id        = list;

  while (id != 0) {
    _dbBTerm* bterm    = _bterm_tbl->getPtr(id);
    bterm->_prev_bterm = prev;
    prev               = id;
    id                 = bterm->_next_bterm;
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
       id = dbNetBTermItr::next(id))
    ++cnt;

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
