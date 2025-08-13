// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <string>
#include <variant>

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
// User Code Begin Classes
class _dbNameCache;
class dbPropertyItr;
// User Code End Classes

struct dbPropertyFlags
{
  uint _type : 4;
  uint _owner_type : 8;
  uint _spare_bits : 20;
};

// User Code Begin Structs
enum _PropTypeEnum
{
  // Do not change the order of this enum.
  DB_STRING_PROP = 0,
  DB_BOOL_PROP = 1,
  DB_INT_PROP = 2,
  DB_DOUBLE_PROP = 3
};
// User Code End Structs

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
                                     _PropTypeEnum type);
  // User Code End Methods

  dbPropertyFlags flags_;
  uint _name;
  dbId<_dbProperty> _next;
  uint _owner;
  std::variant<std::string, bool, int, double> _value;
};
dbIStream& operator>>(dbIStream& stream, _dbProperty& obj);
dbOStream& operator<<(dbOStream& stream, const _dbProperty& obj);
}  // namespace odb
// Generator Code End Header