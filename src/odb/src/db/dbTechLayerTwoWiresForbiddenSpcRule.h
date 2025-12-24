// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

struct dbTechLayerTwoWiresForbiddenSpcRuleFlags
{
  bool min_exact_span_length : 1;
  bool max_exact_span_length : 1;
  uint32_t spare_bits : 30;
};

class _dbTechLayerTwoWiresForbiddenSpcRule : public _dbObject
{
 public:
  _dbTechLayerTwoWiresForbiddenSpcRule(_dbDatabase*);

  bool operator==(const _dbTechLayerTwoWiresForbiddenSpcRule& rhs) const;
  bool operator!=(const _dbTechLayerTwoWiresForbiddenSpcRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerTwoWiresForbiddenSpcRule& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbTechLayerTwoWiresForbiddenSpcRuleFlags flags_;
  int min_spacing_;
  int max_spacing_;
  int min_span_length_;
  int max_span_length_;
  int prl_;
};
dbIStream& operator>>(dbIStream& stream,
                      _dbTechLayerTwoWiresForbiddenSpcRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerTwoWiresForbiddenSpcRule& obj);
}  // namespace odb
   // Generator Code End Header
