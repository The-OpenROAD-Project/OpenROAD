// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbTechLayer;
class _dbTechLayerCutClassRule;

struct dbTechLayerCutSpacingRuleFlags
{
  bool center_to_center : 1;
  bool same_net : 1;
  bool same_metal : 1;
  bool same_via : 1;
  uint32_t cut_spacing_type : 3;
  bool stack : 1;
  bool orthogonal_spacing_valid : 1;
  bool above_width_enclosure_valid : 1;
  bool short_edge_only : 1;
  bool concave_corner_width : 1;
  bool concave_corner_parallel : 1;
  bool concave_corner_edge_length : 1;
  bool concave_corner : 1;
  bool extension_valid : 1;
  bool non_eol_convex_corner : 1;
  bool eol_width_valid : 1;
  bool min_length_valid : 1;
  bool above_width_valid : 1;
  bool mask_overlap : 1;
  bool wrong_direction : 1;
  uint32_t adjacent_cuts : 2;
  bool exact_aligned : 1;
  bool cut_class_to_all : 1;
  bool no_prl : 1;
  bool same_mask : 1;
  bool except_same_pgnet : 1;
  bool side_parallel_overlap : 1;
  bool except_same_net : 1;
  bool except_same_metal : 1;
  bool except_same_metal_overlap : 1;
  bool except_same_via : 1;
  bool above : 1;
  bool except_two_edges : 1;
  bool two_cuts_valid : 1;
  bool same_cut : 1;
  bool long_edge_only : 1;
  bool prl_valid : 1;
  bool below : 1;
  bool par_within_enclosure_valid : 1;
  uint32_t spare_bits : 22;
};

class _dbTechLayerCutSpacingRule : public _dbObject
{
 public:
  _dbTechLayerCutSpacingRule(_dbDatabase*);

  bool operator==(const _dbTechLayerCutSpacingRule& rhs) const;
  bool operator!=(const _dbTechLayerCutSpacingRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerCutSpacingRule& rhs) const;
  void collectMemInfo(MemInfo& info);

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
  // EXACTALIGNED exactAlignedCut | EXCEPTSAMEVIA numCuts
  uint32_t num_cuts_;
  // WITHIN cutWithin | PARALLELWITHIN within | SAMEMETALSHAREDEDGE parwithin
  int within_;
  // WITHIN cutWithin cutWithin2
  int second_within_;
  dbId<_dbTechLayerCutClassRule> cut_class_;
  uint32_t two_cuts_;
  uint32_t prl_;
  uint32_t par_length_;
  int cut_area_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutSpacingRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerCutSpacingRule& obj);
}  // namespace odb
   // Generator Code End Header
