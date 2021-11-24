///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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

#include "dbGroupPowerNetItr.h"

#include <algorithm>

#include "dbBlock.h"
#include "dbGroup.h"
#include "dbNet.h"
#include "dbTable.h"

namespace odb {

bool dbGroupPowerNetItr::reversible()
{
  return true;
}

bool dbGroupPowerNetItr::orderReversed()
{
  return false;
}

void dbGroupPowerNetItr::reverse(dbObject* parent)
{
  _dbGroup* group = (_dbGroup*) parent;
  std::reverse(group->_power_nets.begin(), group->_power_nets.end());
}

uint dbGroupPowerNetItr::sequential()
{
  return 0;
}

uint dbGroupPowerNetItr::size(dbObject* parent)
{
  _dbGroup* group = (_dbGroup*) parent;
  return group->_power_nets.size();
}

uint dbGroupPowerNetItr::begin(dbObject*)
{
  return 0;
}

uint dbGroupPowerNetItr::end(dbObject* parent)
{
  _dbGroup* group = (_dbGroup*) parent;
  return group->_power_nets.size();
}

uint dbGroupPowerNetItr::next(uint id, ...)
{
  return ++id;
}

dbObject* dbGroupPowerNetItr::getObject(uint id, ...)
{
  va_list ap;
  va_start(ap, id);
  _dbGroup* parent = (_dbGroup*) va_arg(ap, dbObject*);
  va_end(ap);
  uint nid = parent->_power_nets[id];
  return _net_tbl->getPtr(nid);
}

}  // namespace odb
