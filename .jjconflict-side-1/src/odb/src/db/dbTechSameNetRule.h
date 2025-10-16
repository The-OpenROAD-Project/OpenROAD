// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

class _dbDatabase;
class _dbTechSameNet;
class _dbTechNonDefaultRule;
class _dbTechLayer;
class dbIStream;
class dbOStream;

struct _dbTechSameNetRuleFlags
{
  uint _stack : 1;
  uint _spare_bits : 31;
};

class _dbTechSameNetRule : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  _dbTechSameNetRuleFlags _flags;
  uint _spacing;
  dbId<_dbTechLayer> _layer_1;
  dbId<_dbTechLayer> _layer_2;

  _dbTechSameNetRule(_dbDatabase*, const _dbTechSameNetRule& r);
  _dbTechSameNetRule(_dbDatabase*);
  ~_dbTechSameNetRule();

  bool operator==(const _dbTechSameNetRule& rhs) const;
  bool operator!=(const _dbTechSameNetRule& rhs) const
  {
    return !operator==(rhs);
  }
  void collectMemInfo(MemInfo& info);
};

inline _dbTechSameNetRule::_dbTechSameNetRule(_dbDatabase*,
                                              const _dbTechSameNetRule& r)
    : _flags(r._flags),
      _spacing(r._spacing),
      _layer_1(r._layer_1),
      _layer_2(r._layer_2)
{
}

inline _dbTechSameNetRule::_dbTechSameNetRule(_dbDatabase*)
{
  _flags._stack = 0;
  _flags._spare_bits = 0;
  _spacing = 0;
}

inline _dbTechSameNetRule::~_dbTechSameNetRule()
{
}

inline dbOStream& operator<<(dbOStream& stream, const _dbTechSameNetRule& rule)
{
  uint* bit_field = (uint*) &rule._flags;
  stream << *bit_field;
  stream << rule._spacing;
  stream << rule._layer_1;
  stream << rule._layer_2;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbTechSameNetRule& rule)
{
  uint* bit_field = (uint*) &rule._flags;
  stream >> *bit_field;
  stream >> rule._spacing;
  stream >> rule._layer_1;
  stream >> rule._layer_2;
  return stream;
}

}  // namespace odb
