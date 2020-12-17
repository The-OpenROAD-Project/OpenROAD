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
#include "dbHashTable.h"
#include "dbId.h"
#include "dbTypes.h"
#include "odb.h"

namespace odb {

template <class T>
class dbTable;
class dbIStream;
class dbOStream;
class dbDiff;

struct dbSiteFlags
{
  uint               _x_symmetry : 1;
  uint               _y_symmetry : 1;
  uint               _R90_symmetry : 1;
  dbSiteClass::Value _class : 4;
  uint               _spare_bits : 25;
};

class _dbSite : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  dbSiteFlags   _flags;
  char*         _name;
  uint          _height;
  uint          _width;
  dbId<_dbSite> _next_entry;

  _dbSite(_dbDatabase*, const _dbSite& s);
  _dbSite(_dbDatabase*);
  ~_dbSite();

  bool operator==(const _dbSite& rhs) const;
  bool operator!=(const _dbSite& rhs) const { return !operator==(rhs); }
  void differences(dbDiff& diff, const char* field, const _dbSite& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

inline dbOStream& operator<<(dbOStream& stream, const _dbSite& site)
{
  uint* bit_field = (uint*) &site._flags;
  stream << *bit_field;
  stream << site._name;
  stream << site._height;
  stream << site._width;
  stream << site._next_entry;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbSite& site)
{
  uint* bit_field = (uint*) &site._flags;
  stream >> *bit_field;
  stream >> site._name;
  stream >> site._height;
  stream >> site._width;
  stream >> site._next_entry;
  return stream;
}

}  // namespace odb
