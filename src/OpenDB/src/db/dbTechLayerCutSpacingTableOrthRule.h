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

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb.h"
// User Code Begin includes
// User Code End includes

namespace odb {

class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;
// User Code Begin Classes
// User Code End Classes

// User Code Begin structs
// User Code End structs

class _dbTechLayerCutSpacingTableOrthRule : public _dbObject
{
 public:
  // User Code Begin enums
  // User Code End enums

  dbVector<std::pair<int, int>> spacing_tbl_;  //{cutWithin, orthoSpacing}

  // User Code Begin fields
  // User Code End fields
  _dbTechLayerCutSpacingTableOrthRule(
      _dbDatabase*,
      const _dbTechLayerCutSpacingTableOrthRule& r);
  _dbTechLayerCutSpacingTableOrthRule(_dbDatabase*);
  ~_dbTechLayerCutSpacingTableOrthRule();
  bool operator==(const _dbTechLayerCutSpacingTableOrthRule& rhs) const;
  bool operator!=(const _dbTechLayerCutSpacingTableOrthRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerCutSpacingTableOrthRule& rhs) const;
  void differences(dbDiff&                                    diff,
                   const char*                                field,
                   const _dbTechLayerCutSpacingTableOrthRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  // User Code Begin methods
  // User Code End methods
};
dbIStream& operator>>(dbIStream&                           stream,
                      _dbTechLayerCutSpacingTableOrthRule& obj);
dbOStream& operator<<(dbOStream&                                 stream,
                      const _dbTechLayerCutSpacingTableOrthRule& obj);
// User Code Begin general
// User Code End general
}  // namespace odb
   // Generator Code End Header