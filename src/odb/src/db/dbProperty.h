// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
// User Code Begin Includes
#include "odb/dbObject.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
// User Code Begin Classes
class _dbNameCache;
class dbPropertyItr;
// User Code End Classes

// User Code Begin Types
enum PropTypeEnum : uint32_t
{
  // Do not change the order of this enum.
  kDbStringProp = 0,
  kDbBoolProp = 1,
  kDbIntProp = 2,
  kDbDoubleProp = 3
};
// User Code End Types

struct dbPropertyFlags
{
  PropTypeEnum type : 4;
  uint32_t owner_type : 8;
  uint32_t spare_bits : 20;
};

class _dbProperty : public _dbObject
{
 public:
  _dbProperty(_dbDatabase*);

  bool operator==(const _dbProperty& rhs) const;
  bool operator!=(const _dbProperty& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbProperty& rhs) const;
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods
  _dbProperty(_dbDatabase*, const _dbProperty& n);
  static dbTable<_dbProperty>* getPropTable(dbObject* object);
  static _dbNameCache* getNameCache(dbObject* object);
  static dbPropertyItr* getItr(dbObject* object);
  static _dbProperty* createProperty(dbObject* object,
                                     const char* name,
                                     PropTypeEnum type);
  // User Code End Methods

  dbPropertyFlags flags_;
  uint32_t name_;
  dbId<_dbProperty> next_;
  uint32_t owner_;
  std::variant<std::string, bool, int, double> value_;
};
dbIStream& operator>>(dbIStream& stream, _dbProperty& obj);
dbOStream& operator<<(dbOStream& stream, const _dbProperty& obj);
}  // namespace odb
// Generator Code End Header
