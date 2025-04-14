// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <utility>

#include "dbCore.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbTechLayer;

struct dbTechLayerAreaRuleFlags
{
  bool except_rectangle_ : 1;
  uint overlap_ : 2;
  uint spare_bits_ : 29;
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
