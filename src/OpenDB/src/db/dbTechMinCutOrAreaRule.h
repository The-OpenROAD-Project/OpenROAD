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
#include "odb.h"

namespace odb {

class _dbDatabase;
class _dbTechLayer;
class dbIStream;
class dbOStream;
class dbDiff;

class _dbTechMinCutRule : public _dbObject
{
 public:
  // PERSISTENT-MEMBERS
  TechMinCutRule::_Flword _flags;
  uint                    _num_cuts;
  uint                    _width;
  int                     _cut_distance;
  uint                    _length;
  uint                    _distance;

  _dbTechMinCutRule(_dbDatabase* db, const _dbTechMinCutRule& r);
  _dbTechMinCutRule(_dbDatabase* db);
  ~_dbTechMinCutRule();

  bool operator==(const _dbTechMinCutRule& rhs) const;
  bool operator!=(const _dbTechMinCutRule& rhs) const
  {
    return !operator==(rhs);
  }
  void differences(dbDiff&                  diff,
                   const char*              field,
                   const _dbTechMinCutRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

inline _dbTechMinCutRule::_dbTechMinCutRule(_dbDatabase* /* unused: db */,
                                            const _dbTechMinCutRule& r)
    : _flags(r._flags),
      _num_cuts(r._num_cuts),
      _width(r._width),
      _cut_distance(r._cut_distance),
      _length(r._length),
      _distance(r._distance)
{
}

inline _dbTechMinCutRule::_dbTechMinCutRule(_dbDatabase* /* unused: db */)
{
  _flags._rule        = TechMinCutRule::NONE;
  _flags._cuts_length = 0;
  _flags._spare_bits  = 0;
  _num_cuts           = 0;
  _width              = 0;
  _cut_distance       = -1;
  _length             = 0;
  _distance           = 0;
}

inline _dbTechMinCutRule::~_dbTechMinCutRule()
{
}

dbOStream& operator<<(dbOStream& stream, const _dbTechMinCutRule& rule);
dbIStream& operator>>(dbIStream& stream, _dbTechMinCutRule& rule);

class _dbTechMinEncRule : public _dbObject
{
 public:
  // PERSISTENT-MEMBERS

  struct _Flword
  {
    uint _has_width : 1;
    uint _spare_bits : 31;
  } _flags;

  uint _area;
  uint _width;

  _dbTechMinEncRule(_dbDatabase* db);
  _dbTechMinEncRule(_dbDatabase* db, const _dbTechMinEncRule& r);
  ~_dbTechMinEncRule();
  bool operator==(const _dbTechMinEncRule& rhs) const;
  bool operator!=(const _dbTechMinEncRule& rhs) const
  {
    return !operator==(rhs);
  }
  void differences(dbDiff&                  diff,
                   const char*              field,
                   const _dbTechMinEncRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

inline _dbTechMinEncRule::_dbTechMinEncRule(_dbDatabase* /* unused: db */,
                                            const _dbTechMinEncRule& r)
    : _flags(r._flags), _area(r._area), _width(r._width)
{
}

inline _dbTechMinEncRule::_dbTechMinEncRule(_dbDatabase* /* unused: db */)
{
  _flags._has_width  = 0;
  _flags._spare_bits = 0;
  _area              = 0;
  _width             = 0;
}

inline _dbTechMinEncRule::~_dbTechMinEncRule()
{
}

dbOStream& operator<<(dbOStream& stream, const _dbTechMinEncRule& rule);
dbIStream& operator>>(dbIStream& stream, _dbTechMinEncRule& rule);

}  // namespace odb
