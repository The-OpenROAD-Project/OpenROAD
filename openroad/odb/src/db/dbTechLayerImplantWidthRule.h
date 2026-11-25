// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbTechLayer;

struct dbTechLayerImplantWidthRuleFlags
{
  bool layer_any : 1;
  bool zero_prl : 1;
  bool except_corner_touch : 1;
  uint32_t spare_bits : 29;
};

class _dbTechLayerImplantWidthRule : public _dbObject
{
 public:
  _dbTechLayerImplantWidthRule(_dbDatabase*);

  bool operator==(const _dbTechLayerImplantWidthRule& rhs) const;
  bool operator!=(const _dbTechLayerImplantWidthRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerImplantWidthRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerImplantWidthRuleFlags flags_;
  int min_width_;
  dbId<_dbTechLayer> second_layer_;
  int max_width_;
  int length_;
  int cell_height_;
  std::string check_implant_group_;
  dbVector<dbId<_dbTechLayer>> within_layers_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerImplantWidthRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerImplantWidthRule& obj);
}  // namespace odb
// Generator Code End Header
