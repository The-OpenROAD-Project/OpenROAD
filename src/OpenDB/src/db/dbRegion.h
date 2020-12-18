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

#pragma once

#include "dbCore.h"
#include "dbId.h"
#include "dbTypes.h"
#include "geom.h"
#include "odb.h"

namespace odb {

class _dbDatabase;
class _dbInst;
class _dbBox;
class dbIStream;
class dbOStream;
class dbDiff;

struct _dbRegionFlags
{
  dbRegionType::Value _type : 4;
  uint                _invalid : 1;
  uint                _spare_bits : 27;
};

class _dbRegion : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  _dbRegionFlags  _flags;
  char*           _name;
  dbId<_dbInst>   _insts;
  dbId<_dbBox>    _boxes;
  dbId<_dbRegion> _parent;
  dbId<_dbRegion> _children;
  dbId<_dbRegion> _next_child;

  _dbRegion(_dbDatabase*);
  _dbRegion(_dbDatabase*, const _dbRegion& b);
  ~_dbRegion();

  bool operator==(const _dbRegion& rhs) const;
  bool operator!=(const _dbRegion& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbRegion& rhs) const;

  void differences(dbDiff& diff, const char* field, const _dbRegion& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

dbOStream& operator<<(dbOStream& stream, const _dbRegion& r);
dbIStream& operator>>(dbIStream& stream, _dbRegion& r);

}  // namespace odb
