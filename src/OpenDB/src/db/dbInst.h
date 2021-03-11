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
#include "dbDatabase.h"
#include "dbId.h"
#include "dbTypes.h"
#include "dbVector.h"  // disconnect the child-iterm
#include "odb.h"

namespace odb {

class _dbBox;
class _dbInstHdr;
class _dbHier;
class _dbITerm;
class _dbRegion;
class _dbDatabase;
class _dbModule;
class _dbGroup;
class dbInst;
class dbIStream;
class dbOStream;
class dbDiff;

struct _dbInstFlags
{
  dbOrientType::Value      _orient : 4;
  dbPlacementStatus::Value _status : 4;
  uint                     _user_flag_1 : 1;
  uint                     _user_flag_2 : 1;
  uint                     _user_flag_3 : 1;
  uint                     _size_only : 1;
  uint                     _dont_touch : 1;
  uint                     _dont_size : 1;
  dbSourceType::Value      _source : 4;
  uint                     _eco_create : 1;
  uint                     _eco_destroy : 1;
  uint                     _eco_modify : 1;
  uint                     _input_cone : 1;
  uint                     _inside_cone : 1;
  uint                     _level : 9;
};

class _dbInst : public _dbObject
{
 public:
  enum Field  // dbJournalField name
  {
    FLAGS,
    ORIGIN,
    INVALIDATETIMING
  };

  _dbInstFlags     _flags;
  char*            _name;
  int              _x;
  int              _y;
  int              _weight;
  dbId<_dbInst>    _next_entry;
  dbId<_dbInstHdr> _inst_hdr;
  dbId<_dbBox>     _bbox;
  dbId<_dbRegion>  _region;
  dbId<_dbModule>  _module;
  dbId<_dbGroup>   _group;
  dbId<_dbInst>    _region_next;
  dbId<_dbInst>    _module_next;
  dbId<_dbInst>    _group_next;
  dbId<_dbInst>    _region_prev;
  dbId<_dbHier>    _hierarchy;
  dbVector<uint>   _iterms;
  dbId<_dbBox>     _halo;

  _dbInst(_dbDatabase*);
  _dbInst(_dbDatabase*, const _dbInst& i);
  ~_dbInst();

  bool operator==(const _dbInst& rhs) const;
  bool operator!=(const _dbInst& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbInst& rhs) const;
  void differences(dbDiff& diff, const char* field, const _dbInst& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  static void setInstBBox(_dbInst* inst);
};

dbOStream& operator<<(dbOStream& stream, const _dbInst& inst);
dbIStream& operator>>(dbIStream& stream, _dbInst& inst);

}  // namespace odb
