// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbTechLayer;

class _dbTechLayerCoreEdgeLengthRule : public _dbObject
{
 public:
  _dbTechLayerCoreEdgeLengthRule(_dbDatabase*);

  bool operator==(const _dbTechLayerCoreEdgeLengthRule& rhs) const;
  bool operator!=(const _dbTechLayerCoreEdgeLengthRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerCoreEdgeLengthRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  int min_length_;
  int max_length_;
  // Indexed in lockstep with except_adjacent_lengths_ and
  // except_adjacent_exact_
  dbVector<int> except_adjacent_edge_lengths_;
  dbVector<int> except_adjacent_lengths_;
  dbVector<int> except_adjacent_exact_;
  dbVector<dbId<_dbTechLayer>> within_layers_;
  dbVector<dbId<_dbTechLayer>> except_layers_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerCoreEdgeLengthRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCoreEdgeLengthRule& obj);
}  // namespace odb
// Generator Code End Header
