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
#include "geom.h"
#include "odb.h"

namespace odb {

class _dbTechLayer;
class _dbTarget;
class _dbDatabase;
class _dbMTerm;
class dbIStream;
class dbOStream;
class dbDiff;

struct _dbTargetFlags
{
  uint _spare_bits : 32;
};

class _dbTarget : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  _dbTargetFlags     _flags;
  Point              _point;
  dbId<_dbMTerm>     _mterm;
  dbId<_dbTechLayer> _layer;
  dbId<_dbTarget>    _next;

  _dbTarget(_dbDatabase*, const _dbTarget& t);
  _dbTarget(_dbDatabase*);
  ~_dbTarget();

  bool operator==(const _dbTarget& rhs) const;
  bool operator!=(const _dbTarget& rhs) const { return !operator==(rhs); }
  void differences(dbDiff& diff, const char* field, const _dbTarget& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

inline _dbTarget::_dbTarget(_dbDatabase*, const _dbTarget& t)
    : _flags(t._flags),
      _point(t._point),
      _mterm(t._mterm),
      _layer(t._layer),
      _next(t._next)
{
}

inline _dbTarget::_dbTarget(_dbDatabase*)
{
  _flags._spare_bits = 0;
}

inline _dbTarget::~_dbTarget()
{
}

inline dbOStream& operator<<(dbOStream& stream, const _dbTarget& target)
{
  uint* bit_field = (uint*) &target._flags;
  stream << *bit_field;
  stream << target._point;
  stream << target._mterm;
  stream << target._layer;
  stream << target._next;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbTarget& target)
{
  uint* bit_field = (uint*) &target._flags;
  stream >> *bit_field;
  stream >> target._point;
  stream >> target._mterm;
  stream >> target._layer;
  stream >> target._next;
  return stream;
}

}  // namespace odb
