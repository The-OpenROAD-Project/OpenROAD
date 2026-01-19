// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"

namespace odb {

class _dbDatabase;
class _dbTechSameNet;
class _dbTechNonDefaultRule;
class _dbTechLayer;
class dbIStream;
class dbOStream;

struct _dbTechSameNetRuleFlags
{
  uint32_t stack : 1;
  uint32_t spare_bits : 31;
};

class _dbTechSameNetRule : public _dbObject
{
 public:
  _dbTechSameNetRule(_dbDatabase*, const _dbTechSameNetRule& r);
  _dbTechSameNetRule(_dbDatabase*);

  bool operator==(const _dbTechSameNetRule& rhs) const;
  bool operator!=(const _dbTechSameNetRule& rhs) const
  {
    return !operator==(rhs);
  }
  void collectMemInfo(MemInfo& info);

  // PERSISTANT-MEMBERS
  _dbTechSameNetRuleFlags flags_;
  uint32_t spacing_;
  dbId<_dbTechLayer> layer_1_;
  dbId<_dbTechLayer> layer_2_;
};

inline _dbTechSameNetRule::_dbTechSameNetRule(_dbDatabase*,
                                              const _dbTechSameNetRule& r)
    : flags_(r.flags_),
      spacing_(r.spacing_),
      layer_1_(r.layer_1_),
      layer_2_(r.layer_2_)
{
}

inline _dbTechSameNetRule::_dbTechSameNetRule(_dbDatabase*)
{
  flags_.stack = 0;
  flags_.spare_bits = 0;
  spacing_ = 0;
}

inline dbOStream& operator<<(dbOStream& stream, const _dbTechSameNetRule& rule)
{
  uint32_t* bit_field = (uint32_t*) &rule.flags_;
  stream << *bit_field;
  stream << rule.spacing_;
  stream << rule.layer_1_;
  stream << rule.layer_2_;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbTechSameNetRule& rule)
{
  uint32_t* bit_field = (uint32_t*) &rule.flags_;
  stream >> *bit_field;
  stream >> rule.spacing_;
  stream >> rule.layer_1_;
  stream >> rule.layer_2_;
  return stream;
}

}  // namespace odb
