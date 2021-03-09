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

// User Code Begin includes
// User Code End includes

namespace odb {

class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;
// User Code Begin Classes
class _dbTechLayer;
// User Code End Classes

struct dbTechLayerSpacingEolRuleFlags
{
  bool exact_width_valid_ : 1;
  bool wrong_dir_spacing_valid_ : 1;
  bool opposite_width_valid_ : 1;
  bool within_valid_ : 1;
  bool wrong_dir_within_valid_ : 1;
  bool same_mask_valid_ : 1;
  bool except_exact_width_valid_ : 1;
  bool fill_concave_corner_valid_ : 1;
  bool withcut_valid_ : 1;
  bool cut_class_valid_ : 1;
  bool with_cut_above_valid_ : 1;
  bool enclosure_end_valid_ : 1;
  bool enclosure_end_within_valid_ : 1;
  bool end_prl_spacing_valid_ : 1;
  bool prl_valid_ : 1;
  bool end_to_end_valid_ : 1;
  bool cut_spaces_valid_ : 1;
  bool extension_valid_ : 1;
  bool wrong_dir_extension_valid_ : 1;
  bool other_end_width_valid_ : 1;
  bool max_length_valid_ : 1;
  bool min_length_valid_ : 1;
  bool two_sides_valid_ : 1;
  bool equal_rect_width_valid_ : 1;
  bool parallel_edge_valid_ : 1;
  bool subtract_eol_width_valid_ : 1;
  bool par_prl_valid_ : 1;
  bool par_min_length_valid_ : 1;
  bool two_edges_valid_ : 1;
  bool same_metal_valid_ : 1;
  bool non_eol_corner_only_valid_ : 1;
  bool parallel_same_mask_valid_ : 1;
  bool enclose_cut_valid_ : 1;
  bool below_valid_ : 1;
  bool above_valid_ : 1;
  bool cut_spacing_valid_ : 1;
  bool all_cuts_valid_ : 1;
  bool to_concave_corner_valid_ : 1;
  bool min_adjacent_length_valid_ : 1;
  bool two_min_adj_length_valid_ : 1;
  bool to_notch_length_valid_ : 1;
  uint spare_bits_ : 23;
};
// User Code Begin structs
// User Code End structs

class _dbTechLayerSpacingEolRule : public _dbObject
{
 public:
  // User Code Begin enums
  // User Code End enums

  dbTechLayerSpacingEolRuleFlags flags_;
  int                            eol_space_;
  int                            eol_width_;
  int                            wrong_dir_space_;
  int                            opposite_width_;
  int                            eol_within_;
  int                            wrong_dir_within_;
  int                            exact_width_;
  int                            other_width_;
  int                            fill_triangle_;
  int                            cut_class_;
  int                            with_cut_space_;
  int                            enclosure_end_width_;
  int                            enclosure_end_within_;
  int                            end_prl_space_;
  int                            end_prl_;
  int                            end_to_end_space_;
  int                            one_cut_space_;
  int                            two_cut_space_;
  int                            extension_;
  int                            wrong_dir_extension_;
  int                            other_end_width_;
  int                            max_length_;
  int                            min_length_;
  int                            par_space_;
  int                            par_within_;
  int                            par_prl_;
  int                            par_min_length_;
  int                            enclose_dist_;
  int                            cut_to_metal_space_;
  int                            min_adj_length_;
  int                            min_adj_length1_;
  int                            min_adj_length2_;
  int                            notch_length_;

  // User Code Begin fields
  dbId<_dbTechLayer> _layer;
  // User Code End fields
  _dbTechLayerSpacingEolRule(_dbDatabase*, const _dbTechLayerSpacingEolRule& r);
  _dbTechLayerSpacingEolRule(_dbDatabase*);
  ~_dbTechLayerSpacingEolRule();
  bool operator==(const _dbTechLayerSpacingEolRule& rhs) const;
  bool operator!=(const _dbTechLayerSpacingEolRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerSpacingEolRule& rhs) const;
  void differences(dbDiff&                           diff,
                   const char*                       field,
                   const _dbTechLayerSpacingEolRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  // User Code Begin methods
  // User Code End methods
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerSpacingEolRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerSpacingEolRule& obj);
// User Code Begin general
// User Code End general
}  // namespace odb
   // Generator Code End Header