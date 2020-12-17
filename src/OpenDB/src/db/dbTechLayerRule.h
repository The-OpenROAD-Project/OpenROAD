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
class _dbTech;
class _dbBlock;
class _dbTechLayer;
class _dbTechNonDefaultRule;
class dbIStream;
class dbOStream;
class dbDiff;

struct _dbTechLayerRuleFlags
{
  uint _block_rule : 1;
  uint _spare_bits : 31;
};

class _dbTechLayerRule : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  _dbTechLayerRuleFlags       _flags;
  uint                        _width;
  uint                        _spacing;
  double                      _resistance;
  double                      _capacitance;
  double                      _edge_capacitance;
  uint                        _wire_extension;
  dbId<_dbTechNonDefaultRule> _non_default_rule;
  dbId<_dbTechLayer>          _layer;

  _dbTechLayerRule(_dbDatabase*);
  _dbTechLayerRule(_dbDatabase*, const _dbTechLayerRule& r);
  ~_dbTechLayerRule();

  _dbTech*  getTech();
  _dbBlock* getBlock();

  bool operator==(const _dbTechLayerRule& rhs) const;
  bool operator!=(const _dbTechLayerRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerRule& rhs) const
  {
    if (_layer < rhs._layer)
      return true;

    if (_layer > rhs._layer)
      return false;

    return _non_default_rule < rhs._non_default_rule;
  }

  void differences(dbDiff&                 diff,
                   const char*             field,
                   const _dbTechLayerRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerRule& rule);
dbIStream& operator>>(dbIStream& stream, _dbTechLayerRule& rule);

}  // namespace odb
