// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <utility>

#include "dbCore.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

class _dbTechLayerForbiddenSpacingRule : public _dbObject
{
 public:
  _dbTechLayerForbiddenSpacingRule(_dbDatabase*);

  bool operator==(const _dbTechLayerForbiddenSpacingRule& rhs) const;
  bool operator!=(const _dbTechLayerForbiddenSpacingRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerForbiddenSpacingRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  std::pair<int, int> forbidden_spacing_;
  int width_;
  int within_;
  int prl_;
  int two_edges_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerForbiddenSpacingRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerForbiddenSpacingRule& obj);
}  // namespace odb
   // Generator Code End Header
