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
#include "odb.h"

namespace odb {

class _dbInst;
class _dbBox;
class _dbDatabase;
class dbIStream;
class dbOStream;
class dbDiff;

struct _dbObstructionFlags
{
  uint _slot_obs : 1;
  uint _fill_obs : 1;
  uint _pushed_down : 1;
  uint _has_min_spacing : 1;
  uint _has_effective_width : 1;
  uint _spare_bits : 27;
};

class _dbObstruction : public _dbObject
{
 public:
  _dbObstructionFlags _flags;
  dbId<_dbInst>       _inst;
  dbId<_dbBox>        _bbox;
  int                 _min_spacing;
  int                 _effective_width;

  _dbObstruction(_dbDatabase*, const _dbObstruction& o);
  _dbObstruction(_dbDatabase*);
  ~_dbObstruction();

  bool operator==(const _dbObstruction& rhs) const;
  bool operator!=(const _dbObstruction& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbObstruction& rhs) const;
  void differences(dbDiff&               diff,
                   const char*           field,
                   const _dbObstruction& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

dbOStream& operator<<(dbOStream& stream, const _dbObstruction& obs);
dbIStream& operator>>(dbIStream& stream, _dbObstruction& obs);

}  // namespace odb
