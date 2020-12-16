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
class _dbTechSameNet;
class _dbTechNonDefaultRule;
class _dbTechLayer;
class dbIStream;
class dbOStream;
class dbDiff;

struct _dbTechSameNetRuleFlags
{
  uint _stack : 1;
  uint _spare_bits : 31;
};

class _dbTechSameNetRule : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  _dbTechSameNetRuleFlags _flags;
  uint                    _spacing;
  dbId<_dbTechLayer>      _layer_1;
  dbId<_dbTechLayer>      _layer_2;

  _dbTechSameNetRule(_dbDatabase*, const _dbTechSameNetRule& r);
  _dbTechSameNetRule(_dbDatabase*);
  ~_dbTechSameNetRule();

  bool operator==(const _dbTechSameNetRule& rhs) const;
  bool operator!=(const _dbTechSameNetRule& rhs) const
  {
    return !operator==(rhs);
  }
  void differences(dbDiff&                   diff,
                   const char*               field,
                   const _dbTechSameNetRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

inline _dbTechSameNetRule::_dbTechSameNetRule(_dbDatabase*,
                                              const _dbTechSameNetRule& r)
    : _flags(r._flags),
      _spacing(r._spacing),
      _layer_1(r._layer_1),
      _layer_2(r._layer_2)
{
}

inline _dbTechSameNetRule::_dbTechSameNetRule(_dbDatabase*)
{
  _flags._stack      = 0;
  _flags._spare_bits = 0;
  _spacing           = 0;
}

inline _dbTechSameNetRule::~_dbTechSameNetRule()
{
}

inline dbOStream& operator<<(dbOStream& stream, const _dbTechSameNetRule& rule)
{
  uint* bit_field = (uint*) &rule._flags;
  stream << *bit_field;
  stream << rule._spacing;
  stream << rule._layer_1;
  stream << rule._layer_2;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbTechSameNetRule& rule)
{
  uint* bit_field = (uint*) &rule._flags;
  stream >> *bit_field;
  stream >> rule._spacing;
  stream >> rule._layer_1;
  stream >> rule._layer_2;
  return stream;
}

}  // namespace odb
