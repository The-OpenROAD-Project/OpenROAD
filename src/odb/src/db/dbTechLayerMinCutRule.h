// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <map>
#include <string>

#include "dbCore.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

struct dbTechLayerMinCutRuleFlags
{
  bool per_cut_class_ : 1;
  bool within_cut_dist_valid : 1;
  bool from_above_ : 1;
  bool from_below_ : 1;
  bool length_valid_ : 1;
  bool area_valid_ : 1;
  bool area_within_dist_valid_ : 1;
  bool same_metal_overlap : 1;
  bool fully_enclosed_ : 1;
  uint spare_bits_ : 23;
};

class _dbTechLayerMinCutRule : public _dbObject
{
 public:
  _dbTechLayerMinCutRule(_dbDatabase*);

  bool operator==(const _dbTechLayerMinCutRule& rhs) const;
  bool operator!=(const _dbTechLayerMinCutRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerMinCutRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerMinCutRuleFlags flags_;
  int num_cuts_;
  std::map<std::string, int> cut_class_cuts_map_;
  int width_;
  int within_cut_dist;
  int length_;
  int length_within_dist_;
  int area_;
  int area_within_dist_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerMinCutRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerMinCutRule& obj);
}  // namespace odb
   // Generator Code End Header
