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
#include "odb.h"

// User Code Begin Includes
// User Code End Includes

namespace odb {

class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;
// User Code Begin Classes
// User Code End Classes

struct dbTechLayerMinStepRuleFlags
{
  bool max_edges_valid_ : 1;
  bool min_adj_length1_valid_ : 1;
  bool no_between_eol_ : 1;
  bool min_adj_length2_valid_ : 1;
  bool convex_corner_ : 1;
  bool min_between_length_valid_ : 1;
  bool except_same_corners_ : 1;
  uint spare_bits_ : 25;
};
// User Code Begin Structs
// User Code End Structs

class _dbTechLayerMinStepRule : public _dbObject
{
 public:
  // User Code Begin Enums
  // User Code End Enums

  dbTechLayerMinStepRuleFlags flags_;
  int                         min_step_length_;
  uint                        max_edges_;
  int                         min_adj_length1_;
  int                         min_adj_length2_;
  int                         eol_width_;
  int                         min_between_length_;

  // User Code Begin Fields
  // User Code End Fields
  _dbTechLayerMinStepRule(_dbDatabase*, const _dbTechLayerMinStepRule& r);
  _dbTechLayerMinStepRule(_dbDatabase*);
  ~_dbTechLayerMinStepRule();
  bool operator==(const _dbTechLayerMinStepRule& rhs) const;
  bool operator!=(const _dbTechLayerMinStepRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerMinStepRule& rhs) const;
  void differences(dbDiff&                        diff,
                   const char*                    field,
                   const _dbTechLayerMinStepRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  // User Code Begin Methods
  // User Code End Methods
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerMinStepRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerMinStepRule& obj);
// User Code Begin General
// User Code End General
}  // namespace odb
   // Generator Code End Header