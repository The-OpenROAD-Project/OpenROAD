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
#include "odb.h"
// User Code Begin Includes
// User Code End Includes

namespace odb {

class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;
class _dbPowerSwitch;
class _dbIsolation;
class _dbGroup;
// User Code Begin Classes
// User Code End Classes

// User Code Begin Structs
// User Code End Structs

class _dbPowerDomain : public _dbObject
{
 public:
  // User Code Begin Enums
  // User Code End Enums

  char* _name;
  dbId<_dbPowerDomain> _next_entry;
  dbVector<std::string> _elements;
  dbVector<dbId<_dbPowerSwitch>> _power_switch;
  dbVector<dbId<_dbIsolation>> _isolation;
  dbId<_dbGroup> _group;
  bool _top;
  dbId<_dbPowerDomain> _parent;
  int _x1;
  int _x2;
  int _y1;
  int _y2;

  // User Code Begin Fields
  // User Code End Fields
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
  // User Code Begin Methods
  // User Code End Methods
};
dbIStream& operator>>(dbIStream& stream, _dbPowerDomain& obj);
dbOStream& operator<<(dbOStream& stream, const _dbPowerDomain& obj);
// User Code Begin General
// User Code End General
}  // namespace odb
   // Generator Code End Header