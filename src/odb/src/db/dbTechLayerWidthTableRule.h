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

namespace odb {
class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;

struct dbTechLayerWidthTableRuleFlags
{
  bool wrong_direction_ : 1;
  bool orthogonal_ : 1;
  uint spare_bits_ : 30;
};

class _dbTechLayerWidthTableRule : public _dbObject
{
 public:
  _dbTechLayerWidthTableRule(_dbDatabase*, const _dbTechLayerWidthTableRule& r);
  _dbTechLayerWidthTableRule(_dbDatabase*);

  ~_dbTechLayerWidthTableRule() = default;

  bool operator==(const _dbTechLayerWidthTableRule& rhs) const;
  bool operator!=(const _dbTechLayerWidthTableRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerWidthTableRule& rhs) const;
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbTechLayerWidthTableRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  dbTechLayerWidthTableRuleFlags flags_;
  dbVector<int> width_tbl_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerWidthTableRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerWidthTableRule& obj);
}  // namespace odb
   // Generator Code End Header