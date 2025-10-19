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

struct dbTechLayerMinStepRuleFlags
{
  bool max_edges_valid_ : 1;
  bool min_adj_length1_valid_ : 1;
  bool no_between_eol_ : 1;
  bool min_adj_length2_valid_ : 1;
  bool convex_corner_ : 1;
  bool min_between_length_valid_ : 1;
  bool except_same_corners_ : 1;
  bool concave_corner_ : 1;
  bool except_rectangle_ : 1;
  bool no_adjacent_eol_ : 1;
  uint spare_bits_ : 22;
};

class _dbTechLayerMinStepRule : public _dbObject
{
 public:
  _dbTechLayerMinStepRule(_dbDatabase*);

  bool operator==(const _dbTechLayerMinStepRule& rhs) const;
  bool operator!=(const _dbTechLayerMinStepRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerMinStepRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerMinStepRuleFlags flags_;
  int min_step_length_;
  uint max_edges_;
  int min_adj_length1_;
  int min_adj_length2_;
  int eol_width_;
  int min_between_length_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerMinStepRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerMinStepRule& obj);
}  // namespace odb
   // Generator Code End Header
