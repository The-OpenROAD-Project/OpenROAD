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

#include "dbBlockItr.h"

#include <algorithm>

#include "dbBlock.h"
#include "dbTable.h"

namespace odb {

bool dbBlockItr::reversible()
{
  return true;
}

bool dbBlockItr::orderReversed()
{
  return false;
}

void dbBlockItr::reverse(dbObject* parent)
{
  _dbBlock* block = (_dbBlock*) parent;
  std::reverse(block->_children.begin(), block->_children.end());
}

uint dbBlockItr::sequential()
{
  return 0;
}

uint dbBlockItr::size(dbObject* parent)
{
  _dbBlock* block = (_dbBlock*) parent;
  return block->_children.size();
}

uint dbBlockItr::begin(dbObject*)
{
  return 0;
}

uint dbBlockItr::end(dbObject* parent)
{
  _dbBlock* block = (_dbBlock*) parent;
  return block->_children.size();
}

uint dbBlockItr::next(uint id, ...)
{
  return ++id;
}

// dbBlockItr.cpp:54:5: warning: undefined behavior when second parameter of
// ‘va_start’ is declared with ‘register’ storage [-Wvarargs]
//     va_start(ap,id);

// dbObject * dbBlockItr::getObject( uint id, ... )
dbObject* dbBlockItr::getObject(uint id, ...)
{
  va_list ap;
  va_start(ap, id);
  _dbBlock* parent = (_dbBlock*) va_arg(ap, dbObject*);
  va_end(ap);
  uint cid = parent->_children[id];
  return _block_tbl->getPtr(cid);
}

}  // namespace odb
