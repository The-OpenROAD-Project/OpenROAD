// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbTechLayerCutClassRule;

struct dbTechLayerCutEnclosureRuleFlags
{
  uint type_ : 2;
  bool cut_class_valid_ : 1;
  bool above_ : 1;
  bool below_ : 1;
  bool eol_min_length_valid_ : 1;
  bool eol_only_ : 1;
  bool short_edge_on_eol_ : 1;
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
  uint num_corners_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutEnclosureRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCutEnclosureRule& obj);
}  // namespace odb
   // Generator Code End Header
