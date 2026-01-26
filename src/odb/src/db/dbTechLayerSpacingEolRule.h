// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
// User Code Begin Includes
#include "odb/dbId.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
// User Code Begin Classes
class _dbTechLayer;
// User Code End Classes

struct dbTechLayerSpacingEolRuleFlags
{
  bool exact_width_valid : 1;
  bool wrong_dir_spacing_valid : 1;
  bool opposite_width_valid : 1;
  bool within_valid : 1;
  bool wrong_dir_within_valid : 1;
  bool same_mask_valid : 1;
  bool except_exact_width_valid : 1;
  bool fill_concave_corner_valid : 1;
  bool withcut_valid : 1;
  bool cut_class_valid : 1;
  bool with_cut_above_valid : 1;
  bool enclosure_end_valid : 1;
  bool enclosure_end_within_valid : 1;
  bool end_prl_spacing_valid : 1;
  bool prl_valid : 1;
  bool end_to_end_valid : 1;
  bool cut_spaces_valid : 1;
  bool extension_valid : 1;
  bool wrong_dir_extension_valid : 1;
  bool other_end_width_valid : 1;
  bool max_length_valid : 1;
  bool min_length_valid : 1;
  bool two_sides_valid : 1;
  bool equal_rect_width_valid : 1;
  bool parallel_edge_valid : 1;
  bool subtract_eol_width_valid : 1;
  bool par_prl_valid : 1;
  bool par_min_length_valid : 1;
  bool two_edges_valid : 1;
  bool same_metal_valid : 1;
  bool non_eol_corner_only_valid : 1;
  bool parallel_same_mask_valid : 1;
  bool enclose_cut_valid : 1;
  bool below_valid : 1;
  bool above_valid : 1;
  bool cut_spacing_valid : 1;
  bool all_cuts_valid : 1;
  bool to_concave_corner_valid : 1;
  bool min_adjacent_length_valid : 1;
  bool two_min_adj_length_valid : 1;
  bool to_notch_length_valid : 1;
  uint32_t spare_bits : 23;
};

class _dbTechLayerSpacingEolRule : public _dbObject
{
 public:
  _dbTechLayerSpacingEolRule(_dbDatabase*);

  bool operator==(const _dbTechLayerSpacingEolRule& rhs) const;
  bool operator!=(const _dbTechLayerSpacingEolRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerSpacingEolRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerSpacingEolRuleFlags flags_;
  int eol_space_;
  int eol_width_;
  int wrong_dir_space_;
  int opposite_width_;
  int eol_within_;
  int wrong_dir_within_;
  int exact_width_;
  int other_width_;
  int fill_triangle_;
  int cut_class_;
  int with_cut_space_;
  int enclosure_end_width_;
  int enclosure_end_within_;
  int end_prl_space_;
  int end_prl_;
  int end_to_end_space_;
  int one_cut_space_;
  int two_cut_space_;
  int extension_;
  int wrong_dir_extension_;
  int other_end_width_;
  int max_length_;
  int min_length_;
  int par_space_;
  int par_within_;
  int par_prl_;
  int par_min_length_;
  int enclose_dist_;
  int cut_to_metal_space_;
  int min_adj_length_;
  int min_adj_length1_;
  int min_adj_length2_;
  int notch_length_;

  // User Code Begin Fields
  dbId<_dbTechLayer> layer_;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerSpacingEolRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerSpacingEolRule& obj);
}  // namespace odb
   // Generator Code End Header
