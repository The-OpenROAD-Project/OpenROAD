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

#include "dbCCSegItr.h"

#include <stdarg.h>

#include "dbBlock.h"
#include "dbCCSeg.h"
#include "dbCapNode.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbCCSegItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbCCSegItr::reversible()
{
  return true;
}

bool dbCCSegItr::orderReversed()
{
  return true;
}

void dbCCSegItr::reverse(dbObject* parent)
{
  _dbCapNode* node = (_dbCapNode*) parent;
  uint        id   = node->_cc_segs;
  uint        pid  = parent->getId();
  uint        list = 0;

  while (id != 0) {
    _dbCCSeg* seg  = _seg_tbl->getPtr(id);
    uint      n    = seg->next(pid);
    seg->next(pid) = list;
    list           = id;
    id             = n;
  }

  node->_cc_segs = list;
}

uint dbCCSegItr::sequential()
{
  return 0;
}

uint dbCCSegItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbCCSegItr::begin(parent); id != dbCCSegItr::end(parent);
       id = dbCCSegItr::next(id))
    ++cnt;

  return cnt;
}

uint dbCCSegItr::begin(dbObject* parent)
{
  _dbCapNode* node = (_dbCapNode*) parent;
  return node->_cc_segs;
}

uint dbCCSegItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbCCSegItr::next(uint id, ...)
{
  va_list ap;
  va_start(ap, id);
  uint pid = va_arg(ap, uint);
  va_end(ap);
  _dbCCSeg* seg = _seg_tbl->getPtr(id);
  return seg->next(pid);
}

dbObject* dbCCSegItr::getObject(uint id, ...)
{
  return _seg_tbl->getPtr(id);
}

}  // namespace odb
