// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>

#include "dbCore.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

struct dbTechLayerKeepOutZoneRuleFlags
{
  bool same_mask : 1;
  bool same_metal : 1;
  bool diff_metal : 1;
  bool except_aligned_side : 1;
  bool except_aligned_end : 1;
  uint32_t spare_bits : 27;
};

class _dbTechLayerKeepOutZoneRule : public _dbObject
{
 public:
  _dbTechLayerKeepOutZoneRule(_dbDatabase*);

  bool operator==(const _dbTechLayerKeepOutZoneRule& rhs) const;
  bool operator!=(const _dbTechLayerKeepOutZoneRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerKeepOutZoneRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerKeepOutZoneRuleFlags flags_;
  std::string first_cut_class_;
  std::string second_cut_class_;
  int aligned_spacing_;
  int side_extension_;
  int forward_extension_;
  int end_side_extension_;
  int end_forward_extension_;
  int side_side_extension_;
  int side_forward_extension_;
  int spiral_extension_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerKeepOutZoneRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerKeepOutZoneRule& obj);
}  // namespace odb
   // Generator Code End Header
