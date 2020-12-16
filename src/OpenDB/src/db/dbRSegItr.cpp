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

#include "dbRSegItr.h"

#include "dbBlock.h"
#include "dbNet.h"
#include "dbRSeg.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbRSegItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbRSegItr::reversible()
{
  return true;
}

bool dbRSegItr::orderReversed()
{
  return true;
}

void dbRSegItr::reverse(dbObject* parent)
{
  _dbNet* net  = (_dbNet*) parent;
  uint    id   = net->_r_segs;
  uint    list = 0;

  while (id != 0) {
    _dbRSeg* seg = _seg_tbl->getPtr(id);
    uint     n   = seg->_next;
    seg->_next   = list;
    list         = id;
    id           = n;
  }

  net->_r_segs = list;
}

uint dbRSegItr::sequential()
{
  return 0;
}

uint dbRSegItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbRSegItr::begin(parent); id != dbRSegItr::end(parent);
       id = dbRSegItr::next(id))
    ++cnt;

  return cnt;
}

uint dbRSegItr::begin(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  if (net->_r_segs == 0)
    return 0;
  _dbRSeg* seg = _seg_tbl->getPtr(net->_r_segs);
  return seg->_next;
}

uint dbRSegItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbRSegItr::next(uint id, ...)
{
  _dbRSeg* seg = _seg_tbl->getPtr(id);
  return seg->_next;
}

dbObject* dbRSegItr::getObject(uint id, ...)
{
  return _seg_tbl->getPtr(id);
}

}  // namespace odb
