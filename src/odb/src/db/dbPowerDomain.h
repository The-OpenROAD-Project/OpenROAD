///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;
class _dbPowerSwitch;
class _dbIsolation;
class _dbGroup;
class _dbLevelShifter;

class _dbPowerDomain : public _dbObject
{
 public:
  _dbPowerDomain(_dbDatabase*, const _dbPowerDomain& r);
  _dbPowerDomain(_dbDatabase*);

  ~_dbPowerDomain();

  bool operator==(const _dbPowerDomain& rhs) const;
  bool operator!=(const _dbPowerDomain& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbPowerDomain& rhs) const;
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbPowerDomain& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  char* _name;
  dbId<_dbPowerDomain> _next_entry;
  dbVector<std::string> _elements;
  dbVector<dbId<_dbPowerSwitch>> _power_switch;
  dbVector<dbId<_dbIsolation>> _isolation;
  dbId<_dbGroup> _group;
  bool _top;
  dbId<_dbPowerDomain> _parent;
  Rect _area;
  dbVector<dbId<_dbLevelShifter>> _levelshifters;
  float _voltage;
};
dbIStream& operator>>(dbIStream& stream, _dbPowerDomain& obj);
dbOStream& operator<<(dbOStream& stream, const _dbPowerDomain& obj);
}  // namespace odb
   // Generator Code End Header