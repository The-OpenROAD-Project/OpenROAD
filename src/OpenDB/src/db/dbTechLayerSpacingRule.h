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

class _dbTechLayerSpacingRule : public _dbObject
{
 public:
  // PERSISTENT-MEMBERS
  TechLayerSpacingRule::_Flword _flags;
  uint                          _spacing;
  uint                          _length_or_influence;
  uint                          _r1min;
  uint                          _r1max;
  uint                          _r2min;
  uint                          _r2max;
  uint                          _cut_area;
  dbId<_dbTechLayer>            _layer;
  dbId<_dbTechLayer>            _cut_layer_below;

  _dbTechLayerSpacingRule(_dbDatabase*, const _dbTechLayerSpacingRule& r);
  _dbTechLayerSpacingRule(_dbDatabase*);
  ~_dbTechLayerSpacingRule();

  bool operator==(const _dbTechLayerSpacingRule& rhs) const;
  bool operator!=(const _dbTechLayerSpacingRule& rhs) const
  {
    return !operator==(rhs);
  }
  void differences(dbDiff&                        diff,
                   const char*                    field,
                   const _dbTechLayerSpacingRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

inline _dbTechLayerSpacingRule::_dbTechLayerSpacingRule(
    _dbDatabase*,
    const _dbTechLayerSpacingRule& r)
    : _flags(r._flags),
      _spacing(r._spacing),
      _length_or_influence(r._length_or_influence),
      _r1min(r._r1min),
      _r1max(r._r1max),
      _r2min(r._r2min),
      _r2max(r._r2max),
      _cut_area(r._cut_area),
      _layer(r._layer),
      _cut_layer_below(r._cut_layer_below)
{
}

inline _dbTechLayerSpacingRule::_dbTechLayerSpacingRule(_dbDatabase*)
{
  _flags._rule                 = TechLayerSpacingRule::DEFAULT;
  _flags._except_same_pgnet    = false;
  _flags._cut_stacking         = false;
  _flags._cut_center_to_center = false;
  _flags._cut_same_net         = false;
  _flags._cut_parallel_overlap = false;
  _flags._spare_bits           = 0;
  _spacing                     = 0;
  _length_or_influence         = 0;
  _r1min                       = 0;
  _r1max                       = 0;
  _r2min                       = 0;
  _r2max                       = 0;
  _cut_area                    = 0;
}

inline _dbTechLayerSpacingRule::~_dbTechLayerSpacingRule()
{
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerSpacingRule& rule);
dbIStream& operator>>(dbIStream& stream, _dbTechLayerSpacingRule& rule);

///  This structure defines entries in the V5.5 influence spacing rule table.

class _dbTechV55InfluenceEntry : public _dbObject
{
 public:
  uint _width;
  uint _within;
  uint _spacing;

  _dbTechV55InfluenceEntry(_dbDatabase* db, const _dbTechV55InfluenceEntry& e);
  _dbTechV55InfluenceEntry(_dbDatabase* db);
  ~_dbTechV55InfluenceEntry();
  bool operator==(const _dbTechV55InfluenceEntry& rhs) const;
  bool operator!=(const _dbTechV55InfluenceEntry& rhs) const
  {
    return !operator==(rhs);
  }
  void differences(dbDiff&                         diff,
                   const char*                     field,
                   const _dbTechV55InfluenceEntry& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

inline _dbTechV55InfluenceEntry::_dbTechV55InfluenceEntry(
    _dbDatabase* /* unused: db */,
    const _dbTechV55InfluenceEntry& e)
    : _width(e._width), _within(e._within), _spacing(e._spacing)
{
}

inline _dbTechV55InfluenceEntry::_dbTechV55InfluenceEntry(
    _dbDatabase* /* unused: db */)
{
  _width   = 0;
  _within  = 0;
  _spacing = 0;
}

inline _dbTechV55InfluenceEntry::~_dbTechV55InfluenceEntry()
{
}

dbOStream& operator<<(dbOStream&                      stream,
                      const _dbTechV55InfluenceEntry& infitem);
dbIStream& operator>>(dbIStream& stream, _dbTechV55InfluenceEntry& infitem);

}  // namespace odb
