// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <tuple>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
// User Code Begin Includes
#include <utility>
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbTechLayer;
// User Code Begin Classes
class _dbTechLayerCutClassRule;
// User Code End Classes

struct dbTechLayerCutSpacingTableDefRuleFlags
{
  bool default_valid : 1;
  bool same_mask : 1;
  bool same_net : 1;
  bool same_metal : 1;
  bool same_via : 1;
  bool layer_valid : 1;
  bool no_stack : 1;
  bool non_zero_enclosure : 1;
  bool prl_for_aligned_cut : 1;
  bool center_to_center_valid : 1;
  bool center_and_edge_valid : 1;
  bool no_prl : 1;
  bool prl_valid : 1;
  bool max_x_y : 1;
  bool end_extension_valid : 1;
  bool side_extension_valid : 1;
  bool exact_aligned_spacing_valid : 1;
  bool horizontal : 1;
  bool prl_horizontal : 1;
  bool vertical : 1;
  bool prl_vertical : 1;
  bool non_opposite_enclosure_spacing_valid : 1;
  bool opposite_enclosure_resize_spacing_valid : 1;
  uint32_t spare_bits : 9;
};

class _dbTechLayerCutSpacingTableDefRule : public _dbObject
{
 public:
  _dbTechLayerCutSpacingTableDefRule(_dbDatabase*);

  bool operator==(const _dbTechLayerCutSpacingTableDefRule& rhs) const;
  bool operator!=(const _dbTechLayerCutSpacingTableDefRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerCutSpacingTableDefRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerCutSpacingTableDefRuleFlags flags_;
  int default_;
  dbId<_dbTechLayer> second_layer_;
  dbVector<std::pair<std::string, std::string>> prl_for_aligned_cut_tbl_;
  dbVector<std::pair<std::string, std::string>> center_to_center_tbl_;
  dbVector<std::pair<std::string, std::string>> center_and_edge_tbl_;
  int prl_;
  dbVector<std::tuple<std::string, std::string, int>> prl_tbl_;
  int extension_;
  dbVector<std::pair<std::string, int>> end_extension_tbl_;
  dbVector<std::pair<std::string, int>> side_extension_tbl_;
  dbVector<std::pair<std::string, int>> exact_aligned_spacing_tbl_;
  dbVector<std::pair<std::string, int>> non_opp_enc_spacing_tbl_;
  dbVector<std::tuple<std::string, int, int, int>> opp_enc_spacing_tbl_;
  dbVector<dbVector<std::pair<int, int>>> spacing_tbl_;
  std::map<std::string, uint32_t> row_map_;
  std::map<std::string, uint32_t> col_map_;
};
dbIStream& operator>>(dbIStream& stream,
                      _dbTechLayerCutSpacingTableDefRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCutSpacingTableDefRule& obj);
}  // namespace odb
   // Generator Code End Header
