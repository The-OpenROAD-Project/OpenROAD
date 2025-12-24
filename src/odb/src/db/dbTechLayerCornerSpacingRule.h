// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <utility>

#include "dbCore.h"
// User Code Begin Includes
#include "dbVector.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

struct dbTechLayerCornerSpacingRuleFlags
{
  bool corner_type : 1;
  bool same_mask : 1;
  bool corner_only : 1;
  bool except_eol : 1;
  bool except_jog_length : 1;
  bool edge_length_valid : 1;
  bool include_shape : 1;
  bool min_length_valid : 1;
  bool except_notch : 1;
  bool except_notch_length_valid : 1;
  bool except_same_net : 1;
  bool except_same_metal : 1;
  bool corner_to_corner : 1;
  uint32_t spare_bits : 19;
};

class _dbTechLayerCornerSpacingRule : public _dbObject
{
 public:
  _dbTechLayerCornerSpacingRule(_dbDatabase*);

  bool operator==(const _dbTechLayerCornerSpacingRule& rhs) const;
  bool operator!=(const _dbTechLayerCornerSpacingRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerCornerSpacingRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerCornerSpacingRuleFlags flags_;
  int within_;
  int eol_width_;
  int jog_length_;
  int edge_length_;
  int min_length_;
  int except_notch_length_;

  // User Code Begin Fields
  dbVector<int> width_tbl_;
  dbVector<std::pair<int, int>> spacing_tbl_;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerCornerSpacingRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCornerSpacingRule& obj);
}  // namespace odb
   // Generator Code End Header
