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
class _dbTechLayer;
class _dbTechLayerCutClassRule;
// User Code Begin Classes
// User Code End Classes

struct dbTechLayerCutSpacingRuleFlags
{
  bool _center_to_center : 1;
  bool _same_net : 1;
  bool _same_metal : 1;
  bool _same_via : 1;
  uint _cut_spacing_type : 3;
  bool _stack : 1;
  bool _orthogonal_spacing_valid : 1;
  bool _above_width_enclosure_valid : 1;
  bool _short_edge_only : 1;
  bool _concave_corner_width : 1;
  bool _concave_corner_parallel : 1;
  bool _concave_corner_edge_length : 1;
  bool _concave_corner : 1;
  bool _extension_valid : 1;
  bool _non_eol_convex_corner : 1;
  bool _eol_width_valid : 1;
  bool _min_length_valid : 1;
  bool _above_width_valid : 1;
  bool _mask_overlap : 1;
  bool _wrong_direction : 1;
  uint _adjacent_cuts : 2;
  bool _exact_aligned : 1;
  bool _cut_class_to_all : 1;
  bool _no_prl : 1;
  bool _same_mask : 1;
  bool _except_same_pgnet : 1;
  bool _side_parallel_overlap : 1;
  bool _except_same_net : 1;
  bool _except_same_metal : 1;
  bool _except_same_metal_overlap : 1;
  bool _except_same_via : 1;
  bool _above : 1;
  bool _except_two_edges : 1;
  bool _two_cuts_valid : 1;
  bool _same_cut : 1;
  bool _long_edge_only : 1;
  bool _below : 1;
  bool _par_within_enclosure_valid : 1;
  uint _spare_bits : 23;
};
// User Code Begin structs
// User Code End structs

class _dbTechLayerCutSpacingRule : public _dbObject
{
 public:
  // User Code Begin enums
  // User Code End enums

  dbTechLayerCutSpacingRuleFlags _flags;
  int                            _cut_spacing;
  dbId<_dbTechLayer>             _second_layer;
  int                            _orthogonal_spacing;
  int                            _width;
  int                            _enclosure;
  int                            _edge_length;
  int                            _par_within;
  int                            _par_enclosure;
  int                            _edge_enclosure;
  int                            _adj_enclosure;
  int                            _above_enclosure;
  int                            _above_width;
  int                            _min_length;
  int                            _extension;
  int                            _eol_width;
  uint _num_cuts;  // EXACTALIGNED exactAlignedCut | EXCEPTSAMEVIA numCuts
  int  _within;    // WITHIN cutWithin | PARALLELWITHIN within |
                   // SAMEMETALSHAREDEDGE parwithin
  int                            _second_within;  // WITHIN cutWithin cutWithin2
  dbId<_dbTechLayerCutClassRule> _cut_class;
  uint                           _two_cuts;
  uint                           _par_length;
  int                            _cut_area;

  // User Code Begin fields
  // User Code End fields
  _dbTechLayerCutSpacingRule(_dbDatabase*, const _dbTechLayerCutSpacingRule& r);
  _dbTechLayerCutSpacingRule(_dbDatabase*);
  ~_dbTechLayerCutSpacingRule();
  bool operator==(const _dbTechLayerCutSpacingRule& rhs) const;
  bool operator!=(const _dbTechLayerCutSpacingRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerCutSpacingRule& rhs) const;
  void differences(dbDiff&                           diff,
                   const char*                       field,
                   const _dbTechLayerCutSpacingRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  // User Code Begin methods
  // User Code End methods
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutSpacingRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerCutSpacingRule& obj);
// User Code Begin general
// User Code End general
}  // namespace odb
   // Generator Code End 1