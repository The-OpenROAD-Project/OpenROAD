// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <utility>

#include "dbCore.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbTechLayer;

struct dbTechLayerAreaRuleFlags
{
  bool except_rectangle : 1;
  uint32_t overlap : 2;
  uint32_t spare_bits : 29;
};

class _dbTechLayerAreaRule : public _dbObject
{
 public:
  _dbTechLayerAreaRule(_dbDatabase*);

  bool operator==(const _dbTechLayerAreaRule& rhs) const;
  bool operator!=(const _dbTechLayerAreaRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerAreaRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerAreaRuleFlags flags_;
  int area_;
  int except_min_width_;
  int except_edge_length_;
  std::pair<int, int> except_edge_lengths_;
  std::pair<int, int> except_min_size_;
  std::pair<int, int> except_step_;
  dbId<_dbTechLayer> trim_layer_;
  int mask_;
  int rect_width_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerAreaRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerAreaRule& obj);
}  // namespace odb
   // Generator Code End Header
