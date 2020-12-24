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

// Generator Code Begin 1
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
  bool _exact_width_valid : 1;
  bool _wrong_dir_spacing_valid : 1;
  bool _opposite_width_valid : 1;
  bool _within_valid : 1;
  bool _wrong_dir_within_valid : 1;
  bool _same_mask_valid : 1;
  bool _except_exact_width_valid : 1;
  bool _fill_concave_corner_valid : 1;
  bool _withcut_valid : 1;
  bool _cut_class_valid : 1;
  bool _with_cut_above_valid : 1;
  bool _enclosure_end_valid : 1;
  bool _enclosure_end_within_valid : 1;
  bool _end_prl_spacing_valid : 1;
  bool _prl_valid : 1;
  bool _end_to_end_valid : 1;
  bool _cut_spaces_valid : 1;
  bool _extension_valid : 1;
  bool _wrong_dir_extension_valid : 1;
  bool _other_end_width_valid : 1;
  bool _max_length_valid : 1;
  bool _min_length_valid : 1;
  bool _two_sides_valid : 1;
  bool _equal_rect_width_valid : 1;
  bool _parallel_edge_valid : 1;
  bool _subtract_eol_width_valid : 1;
  bool _par_prl_valid : 1;
  bool _par_min_length_valid : 1;
  bool _two_edges_valid : 1;
  bool _same_metal_valid : 1;
  bool _non_eol_corner_only_valid : 1;
  bool _parallel_same_mask_valid : 1;
  bool _enclose_cut_valid : 1;
  bool _below_valid : 1;
  bool _above_valid : 1;
  bool _cut_spacing_valid : 1;
  bool _all_cuts_valid : 1;
  bool _to_concave_corner_valid : 1;
  bool _min_adjacent_length_valid : 1;
  bool _two_min_adj_length_valid : 1;
  bool _to_notch_length_valid : 1;
  uint _spare_bits : 23;
};
// User Code Begin structs
// User Code End structs

class _dbTechLayerSpacingEolRule : public _dbObject
{
 public:
  // User Code Begin enums
  // User Code End enums

  dbTechLayerSpacingEolRuleFlags _flags;
  int                            _eol_space;
  int                            _eol_width;
  int                            _wrong_dir_space;
  int                            _opposite_width;
  int                            _eol_within;
  int                            _wrong_dir_within;
  int                            _exact_width;
  int                            _other_width;
  int                            _fill_triangle;
  int                            _cut_class;
  int                            _with_cut_space;
  int                            _enclosure_end_width;
  int                            _enclosure_end_within;
  int                            _end_prl_space;
  int                            _end_prl;
  int                            _end_to_end_space;
  int                            _one_cut_space;
  int                            _two_cut_space;
  int                            _extension;
  int                            _wrong_dir_extension;
  int                            _other_end_width;
  int                            _max_length;
  int                            _min_length;
  int                            _par_space;
  int                            _par_within;
  int                            _par_prl;
  int                            _par_min_length;
  int                            _enclose_dist;
  int                            _cut_to_metal_space;
  int                            _min_adj_length;
  int                            _min_adj_length1;
  int                            _min_adj_length2;
  int                            _notch_length;

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
   // Generator Code End 1