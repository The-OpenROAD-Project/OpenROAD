///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, OpenRoad Project
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

template <class T>
class dbTable;
class dbIStream;
class dbOStream;
class dbSite;
class dbLib;
class dbDiff;
class _dbTechLayer;

struct dbFillFlags
{
  uint _opc : 1;
  uint _mask_id : 2;
  uint _layer_id : 8;
  uint _spare_bits : 21;
};

class _dbFill : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  dbFillFlags _flags;
  Rect _rect;

  _dbFill(_dbDatabase*, const _dbFill& r);
  _dbFill(_dbDatabase*);

  _dbTechLayer* getTechLayer() const;

  bool operator==(const _dbFill& rhs) const;
  bool operator!=(const _dbFill& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbFill& rhs) const;
  void differences(dbDiff& diff, const char* field, const _dbFill& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

inline _dbFill::_dbFill(_dbDatabase*, const _dbFill& r)
    : _flags(r._flags), _rect(r._rect)
{
}

inline _dbFill::_dbFill(_dbDatabase*)
{
  _flags._opc = false;
  _flags._mask_id = 0;
  _flags._layer_id = 0;
  _flags._spare_bits = 0;
}

inline dbOStream& operator<<(dbOStream& stream, const _dbFill& fill)
{
  uint* bit_field = (uint*) &fill._flags;
  stream << *bit_field;
  stream << fill._rect;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbFill& fill)
{
  uint* bit_field = (uint*) &fill._flags;
  stream >> *bit_field;
  stream >> fill._rect;
  return stream;
}

}  // namespace odb
