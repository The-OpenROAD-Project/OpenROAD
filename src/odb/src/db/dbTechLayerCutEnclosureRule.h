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
#include "odb.h"

// User Code Begin Includes
// User Code End Includes

namespace odb {

class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;
class _dbTechLayerCutClassRule;
// User Code Begin Classes
// User Code End Classes

struct dbTechLayerCutEnclosureRuleFlags
{
  uint type_ : 2;
  bool cut_class_valid_ : 1;
  bool above_ : 1;
  bool below_ : 1;
  bool eol_min_length_valid_ : 1;
  bool eol_only_ : 1;
  bool short_edge_only_ : 1;
  bool side_spacing_valid_ : 1;
  bool end_spacing_valid_ : 1;
  bool off_center_line_ : 1;
  bool width_valid_ : 1;
  bool include_abutted_ : 1;
  bool except_extra_cut_ : 1;
  bool prl_ : 1;
  bool no_shared_edge_ : 1;
  bool length_valid_ : 1;
  bool extra_cut_valid_ : 1;
  bool extra_only : 1;
  bool redundant_cut_valid_ : 1;
  bool parallel_valid_ : 1;
  bool second_parallel_valid : 1;
  bool second_par_within_valid_ : 1;
  bool below_enclosure_valid_ : 1;
  bool concave_corners_valid_ : 1;
  uint spare_bits_ : 7;
};
// User Code Begin Structs
// User Code End Structs

class _dbTechLayerCutEnclosureRule : public _dbObject
{
 public:
  // User Code Begin Enums
  // User Code End Enums

  dbTechLayerCutEnclosureRuleFlags flags_;
  dbId<_dbTechLayerCutClassRule> cut_class_;
  int eol_width_;
  int eol_min_length_;
  int first_overhang_;
  int second_overhang_;
  int spacing_;
  int extension_;
  int forward_extension_;
  int backward_extension_;
  int min_width_;
  int cut_within_;
  int min_length_;
  int par_length_;
  int second_par_length_;
  int par_within_;
  int second_par_within_;
  int below_enclosure_;
  uint num_corners_;

  // User Code Begin Fields
  // User Code End Fields
  _dbTechLayerCutEnclosureRule(_dbDatabase*,
                               const _dbTechLayerCutEnclosureRule& r);
  _dbTechLayerCutEnclosureRule(_dbDatabase*);
  ~_dbTechLayerCutEnclosureRule();
  bool operator==(const _dbTechLayerCutEnclosureRule& rhs) const;
  bool operator!=(const _dbTechLayerCutEnclosureRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerCutEnclosureRule& rhs) const;
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbTechLayerCutEnclosureRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  // User Code Begin Methods
  // User Code End Methods
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutEnclosureRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCutEnclosureRule& obj);
// User Code Begin General
// User Code End General
}  // namespace odb
   // Generator Code End Header
