// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbBox.h"
#include "odb/dbId.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace odb {

class _dbDatabase;
class dbIStream;
class dbOStream;

struct _dbSBoxFlags
{
  dbWireShapeType::Value wire_type : 6;
  uint32_t direction : 2;  // 0 = undefiend, 1 = horizontal, 2 = vertical, 3 =
                           // octilinear
  uint32_t via_bottom_mask : 2;
  uint32_t via_cut_mask : 2;
  uint32_t via_top_mask : 2;
  uint32_t spare_bits : 18;
};

class _dbSBox : public _dbBox
{
 public:
  _dbSBox(_dbDatabase*, const _dbSBox& b);
  _dbSBox(_dbDatabase*);

  bool operator==(const _dbSBox& rhs) const;
  bool operator!=(const _dbSBox& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbSBox& rhs) const;
  int equal(const _dbSBox& rhs) const;

  // PERSISTANT-MEMBERS
  _dbSBoxFlags sflags_;
};

dbOStream& operator<<(dbOStream& stream, const _dbSBox& box);
dbIStream& operator>>(dbIStream& stream, _dbSBox& box);

}  // namespace odb
