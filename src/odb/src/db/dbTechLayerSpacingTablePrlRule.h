// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <tuple>

#include "dbCore.h"
#include "dbVector.h"
// User Code Begin Includes
#include <map>
#include <utility>
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

struct dbTechLayerSpacingTablePrlRuleFlags
{
  bool wrong_direction : 1;
  bool same_mask : 1;
  bool exceept_eol : 1;
  uint32_t spare_bits : 29;
};

class _dbTechLayerSpacingTablePrlRule : public _dbObject
{
 public:
  _dbTechLayerSpacingTablePrlRule(_dbDatabase*);

  bool operator==(const _dbTechLayerSpacingTablePrlRule& rhs) const;
  bool operator!=(const _dbTechLayerSpacingTablePrlRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerSpacingTablePrlRule& rhs) const;
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods

  uint32_t getWidthIdx(int width) const;

  uint32_t getLengthIdx(int length) const;

  // User Code End Methods

  dbTechLayerSpacingTablePrlRuleFlags flags_;
  int eol_width_;
  dbVector<int> length_tbl_;
  dbVector<int> width_tbl_;
  dbVector<dbVector<int>> spacing_tbl_;
  dbVector<std::tuple<int, int, int>> influence_tbl_;

  // User Code Begin Fields
  std::map<uint32_t, std::pair<int, int>> _within_tbl_;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerSpacingTablePrlRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerSpacingTablePrlRule& obj);
}  // namespace odb
   // Generator Code End Header
