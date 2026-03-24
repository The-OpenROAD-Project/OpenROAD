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

class _dbTechLayerMaxSpacingRule : public _dbObject
{
 public:
  _dbTechLayerMaxSpacingRule(_dbDatabase*);

  bool operator==(const _dbTechLayerMaxSpacingRule& rhs) const;
  bool operator!=(const _dbTechLayerMaxSpacingRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerMaxSpacingRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  std::string cut_class_;
  int max_spacing_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerMaxSpacingRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerMaxSpacingRule& obj);
}  // namespace odb
   // Generator Code End Header
