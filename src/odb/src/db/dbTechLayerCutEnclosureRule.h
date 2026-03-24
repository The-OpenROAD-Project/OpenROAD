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
class _dbTechLayerCutClassRule;

struct dbTechLayerCutEnclosureRuleFlags
{
  uint32_t type : 2;
  bool cut_class_valid : 1;
  bool above : 1;
  bool below : 1;
  bool eol_min_length_valid : 1;
  bool eol_only : 1;
  bool short_edge_on_eol : 1;
  bool side_spacing_valid : 1;
  bool end_spacing_valid : 1;
  bool off_center_line : 1;
  bool width_valid : 1;
  bool include_abutted : 1;
  bool except_extra_cut : 1;
  bool prl : 1;
  bool no_shared_edge : 1;
  bool length_valid : 1;
  bool extra_cut_valid : 1;
  bool extra_only : 1;
  bool redundant_cut_valid : 1;
  bool parallel_valid : 1;
  bool second_parallel_valid : 1;
  bool second_par_within_valid : 1;
  bool below_enclosure_valid : 1;
  bool concave_corners_valid : 1;
  uint32_t spare_bits : 7;
};

class _dbTechLayerCutEnclosureRule : public _dbObject
{
 public:
  _dbTechLayerCutEnclosureRule(_dbDatabase*);

  bool operator==(const _dbTechLayerCutEnclosureRule& rhs) const;
  bool operator!=(const _dbTechLayerCutEnclosureRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerCutEnclosureRule& rhs) const;
  void collectMemInfo(MemInfo& info);

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
  uint32_t num_corners_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutEnclosureRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCutEnclosureRule& obj);
}  // namespace odb
   // Generator Code End Header
