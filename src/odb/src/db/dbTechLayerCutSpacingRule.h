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
class _dbTechLayer;
class _dbTechLayerCutClassRule;
// User Code Begin Classes
// User Code End Classes

struct dbTechLayerCutSpacingRuleFlags
{
  bool center_to_center_ : 1;
  bool same_net_ : 1;
  bool same_metal_ : 1;
  bool same_via_ : 1;
  uint cut_spacing_type_ : 3;
  bool stack_ : 1;
  bool orthogonal_spacing_valid_ : 1;
  bool above_width_enclosure_valid_ : 1;
  bool short_edge_only_ : 1;
  bool concave_corner_width_ : 1;
  bool concave_corner_parallel_ : 1;
  bool concave_corner_edge_length_ : 1;
  bool concave_corner_ : 1;
  bool extension_valid_ : 1;
  bool non_eol_convex_corner_ : 1;
  bool eol_width_valid_ : 1;
  bool min_length_valid_ : 1;
  bool above_width_valid_ : 1;
  bool mask_overlap_ : 1;
  bool wrong_direction_ : 1;
  uint adjacent_cuts_ : 2;
  bool exact_aligned_ : 1;
  bool cut_class_to_all_ : 1;
  bool no_prl_ : 1;
  bool same_mask_ : 1;
  bool except_same_pgnet_ : 1;
  bool side_parallel_overlap_ : 1;
  bool except_same_net_ : 1;
  bool except_same_metal_ : 1;
  bool except_same_metal_overlap_ : 1;
  bool except_same_via_ : 1;
  bool above_ : 1;
  bool except_two_edges_ : 1;
  bool two_cuts_valid_ : 1;
  bool same_cut_ : 1;
  bool long_edge_only_ : 1;
  bool prl_valid_ : 1;
  bool below_ : 1;
  bool par_within_enclosure_valid_ : 1;
  uint spare_bits_ : 22;
};
// User Code Begin Structs
// User Code End Structs

class _dbTechLayerCutSpacingRule : public _dbObject
{
 public:
  // User Code Begin Enums
  // User Code End Enums

  dbTechLayerCutSpacingRuleFlags flags_;
  int cut_spacing_;
  dbId<_dbTechLayer> second_layer_;
  int orthogonal_spacing_;
  int width_;
  int enclosure_;
  int edge_length_;
  int par_within_;
  int par_enclosure_;
  int edge_enclosure_;
  int adj_enclosure_;
  int above_enclosure_;
  int above_width_;
  int min_length_;
  int extension_;
  int eol_width_;
  uint num_cuts_;      // EXACTALIGNED exactAlignedCut | EXCEPTSAMEVIA numCuts
  int within_;         // WITHIN cutWithin | PARALLELWITHIN within |
                       // SAMEMETALSHAREDEDGE parwithin
  int second_within_;  // WITHIN cutWithin cutWithin2
  dbId<_dbTechLayerCutClassRule> cut_class_;
  uint two_cuts_;
  uint prl_;
  uint par_length_;
  int cut_area_;

  // User Code Begin Fields
  // User Code End Fields
  _dbTechLayerCutSpacingRule(_dbDatabase*, const _dbTechLayerCutSpacingRule& r);
  _dbTechLayerCutSpacingRule(_dbDatabase*);
  ~_dbTechLayerCutSpacingRule();
  bool operator==(const _dbTechLayerCutSpacingRule& rhs) const;
  bool operator!=(const _dbTechLayerCutSpacingRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerCutSpacingRule& rhs) const;
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbTechLayerCutSpacingRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  // User Code Begin Methods
  // User Code End Methods
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutSpacingRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerCutSpacingRule& obj);
// User Code Begin General
// User Code End General
}  // namespace odb
   // Generator Code End Header
