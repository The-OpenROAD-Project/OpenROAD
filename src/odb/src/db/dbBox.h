// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbDatabase.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace odb {

class _dbDatabase;
class _dbPolygon;
class _dbTechVia;
class _dbTechLayer;
class _dbVia;
class dbIStream;
class dbOStream;

struct _dbBoxFlagsBackwardCompatability
{
  dbBoxOwner::Value owner_type : 4;
  uint32_t visited : 1;
  uint32_t mark : 1;
  uint32_t octilinear : 1;
  uint32_t is_tech_via : 1;
  uint32_t is_block_via : 1;
  uint32_t layer_id : 8;
  uint32_t via_id : 15;
};

struct _dbBoxFlagsWithoutMask
{
  dbBoxOwner::Value owner_type : 4;
  uint32_t visited : 1;
  uint32_t octilinear : 1;
  uint32_t is_tech_via : 1;
  uint32_t is_block_via : 1;
  uint32_t layer_id : 9;
  uint32_t via_id : 15;
};

struct _dbBoxFlags
{
  dbBoxOwner::Value owner_type : 4;
  uint32_t visited : 1;
  uint32_t octilinear : 1;
  uint32_t is_tech_via : 1;
  uint32_t is_block_via : 1;
  uint32_t layer_id : 9;
  uint32_t via_id : 13;
  uint32_t layer_mask : 2;
};

static_assert(sizeof(_dbBoxFlagsBackwardCompatability) == 4,
              "_dbBoxFlagsBackwardCompatability too large");
static_assert(sizeof(_dbBoxFlagsWithoutMask) == 4,
              "_dbBoxFlagsWithoutMask too large");
static_assert(sizeof(_dbBoxFlags) == 4, "_dbBoxFlags too large");

class _dbBox : public _dbObject
{
 public:
  enum Type
  {
    kBlockVia,
    kTechVia,
    kBox
  };
  union dbBoxShape
  {
    Rect rect;
    Oct oct;
  };

  _dbBox(_dbDatabase*);
  _dbBox(_dbDatabase*, const _dbBox& b);

  bool operator==(const _dbBox& rhs) const;
  bool operator!=(const _dbBox& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbBox& rhs) const;
  int equal(const _dbBox& rhs) const;
  void collectMemInfo(MemInfo& info) const;
  bool isOct() const;

  _dbTechLayer* getTechLayer() const;
  _dbTechVia* getTechVia() const;
  _dbVia* getBlockVia() const;

  Point getViaXY() const;
  Type getType() const;
  void checkMask(int mask) const;

  // PERSISTANT-MEMBERS
  _dbBoxFlags flags_;
  dbBoxShape shape_ = {Rect()};
  uint32_t owner_;
  dbId<_dbBox> next_box_;
  int design_rule_width_;
};

dbOStream& operator<<(dbOStream& stream, const _dbBox& box);
dbIStream& operator>>(dbIStream& stream, _dbBox& box);

}  // namespace odb
