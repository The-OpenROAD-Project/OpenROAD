// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <map>

#include "dbCore.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

struct dbTechLayerVoltageSpacingFlags
{
  bool tocut_above : 1;
  bool tocut_below : 1;
  uint32_t spare_bits : 30;
};

class _dbTechLayerVoltageSpacing : public _dbObject
{
 public:
  _dbTechLayerVoltageSpacing(_dbDatabase*);

  bool operator==(const _dbTechLayerVoltageSpacing& rhs) const;
  bool operator!=(const _dbTechLayerVoltageSpacing& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerVoltageSpacing& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerVoltageSpacingFlags flags_;
  std::map<float, int> table_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerVoltageSpacing& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerVoltageSpacing& obj);
}  // namespace odb
// Generator Code End Header