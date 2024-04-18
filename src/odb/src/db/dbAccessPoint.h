///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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
// User Code Begin Includes
#include <array>
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;
class _dbTechLayer;
class _dbLib;
class _dbMaster;
class _dbMPin;
class _dbBPin;
class _dbITerm;
class _dbObject;

class _dbAccessPoint : public _dbObject
{
 public:
  _dbAccessPoint(_dbDatabase*, const _dbAccessPoint& r);
  _dbAccessPoint(_dbDatabase*);

  ~_dbAccessPoint() = default;

  bool operator==(const _dbAccessPoint& rhs) const;
  bool operator!=(const _dbAccessPoint& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbAccessPoint& rhs) const;
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbAccessPoint& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  // User Code Begin Methods
  void setMPin(_dbMPin* mpin);
  // User Code End Methods

  Point point_;
  dbId<_dbTechLayer> layer_;
  dbId<_dbLib> lib_;
  dbId<_dbMaster> master_;
  dbId<_dbMPin> mpin_;
  dbId<_dbBPin> bpin_;
  std::array<bool, 6> accesses_;
  dbAccessType::Value low_type_;
  dbAccessType::Value high_type_;
  // list of iterms that prefer this access point
  dbVector<dbId<_dbITerm>> iterms_;
  // list of vias by num of cuts
  dbVector<dbVector<std::pair<dbObjectType, dbId<_dbObject>>>> vias_;
  dbVector<std::tuple<Rect, bool, bool>> path_segs_;
};
dbIStream& operator>>(dbIStream& stream, _dbAccessPoint& obj);
dbOStream& operator<<(dbOStream& stream, const _dbAccessPoint& obj);
}  // namespace odb
   // Generator Code End Header
