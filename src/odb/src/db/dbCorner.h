// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>

#include "dbCore.h"
// User Code Begin Includes
// User Code End Includes

namespace odb {
// User Code Begin Consts
// User Code End Consts
class dbIStream;
class dbOStream;
class _dbDatabase;
// User Code Begin Classes
// User Code End Classes

// User Code Begin Types
// User Code End Types

// User Code Begin Structs
// User Code End Structs

class _dbCorner : public _dbObject
{
 public:
  // User Code Begin Enums
  // User Code End Enums

  _dbCorner(_dbDatabase*);

  bool operator==(const _dbCorner& rhs) const;
  bool operator!=(const _dbCorner& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbCorner& rhs) const;
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods
  // User Code End Methods

  std::string name_;

  // User Code Begin Fields
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbCorner& obj);
dbOStream& operator<<(dbOStream& stream, const _dbCorner& obj);
// User Code Begin General
// User Code End General
}  // namespace odb
// Generator Code End Header