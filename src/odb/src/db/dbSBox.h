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

#include "dbBox.h"
#include "dbId.h"
#include "dbObject.h"
#include "dbTypes.h"
#include "geom.h"
#include "odb.h"

namespace odb {

class _dbDatabase;
class dbIStream;
class dbOStream;
class dbDiff;

struct _dbSBoxFlags
{
  dbWireShapeType::Value _wire_type : 6;
  uint _direction : 2;  // 0 = undefiend, 1 = horizontal, 2 = vertical, 3 =
                        // octilinear
  uint _spare_bits : 24;
};

class _dbSBox : public _dbBox
{
 public:
  // PERSISTANT-MEMBERS
  _dbSBoxFlags _sflags;

  _dbSBox(_dbDatabase*, const _dbSBox& b);
  _dbSBox(_dbDatabase*);
  ~_dbSBox();

  bool operator==(const _dbSBox& rhs) const;
  bool operator!=(const _dbSBox& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbSBox& rhs) const;
  void differences(dbDiff& diff, const char* field, const _dbSBox& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  int equal(const _dbSBox& rhs) const;
};

inline _dbSBox::_dbSBox(_dbDatabase* db, const _dbSBox& b)
    : _dbBox(db, b), _sflags(b._sflags)
{
}

inline _dbSBox::_dbSBox(_dbDatabase* db) : _dbBox(db)
{
  _sflags._wire_type = dbWireShapeType::COREWIRE;
  _sflags._direction = 0;
  _sflags._spare_bits = 0;
}

inline _dbSBox::~_dbSBox()
{
}

inline dbOStream& operator<<(dbOStream& stream, const _dbSBox& box)
{
  stream << (_dbBox&) box;
  uint* bit_field = (uint*) &box._sflags;
  stream << *bit_field;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbSBox& box)
{
  stream >> (_dbBox&) box;
  uint* bit_field = (uint*) &box._sflags;
  stream >> *bit_field;
  return stream;
}

}  // namespace odb
