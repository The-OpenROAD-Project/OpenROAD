// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <variant>

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

template <class T>
class dbTable;
class _dbDatabase;
class _dbProperty;
class dbPropertyItr;
class dbIStream;
class dbOStream;
class _dbNameCache;

enum _PropTypeEnum
{
  // Do not change the order of this enum.
  DB_STRING_PROP = 0,
  DB_BOOL_PROP = 1,
  DB_INT_PROP = 2,
  DB_DOUBLE_PROP = 3
};

struct _dbPropertyFlags
{
  _PropTypeEnum _type : 4;
  uint _owner_type : 8;
  uint _spare_bits : 20;
};

class _dbProperty : public _dbObject
{
 public:
  _dbPropertyFlags _flags;
  uint _name;
  dbId<_dbProperty> _next;
  uint _owner;

  std::variant<std::string, bool, int, double> _value;

  _dbProperty(_dbDatabase*);
  _dbProperty(_dbDatabase*, const _dbProperty& n);
  ~_dbProperty();

  bool operator==(const _dbProperty& rhs) const;
  bool operator!=(const _dbProperty& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbProperty& rhs) const;
  void collectMemInfo(MemInfo& info);

  static dbTable<_dbProperty>* getPropTable(dbObject* object);
  static _dbNameCache* getNameCache(dbObject* object);
  static dbPropertyItr* getItr(dbObject* object);
  static _dbProperty* createProperty(dbObject* object,
                                     const char* name,
                                     _PropTypeEnum type);
};

dbOStream& operator<<(dbOStream& stream, const _dbProperty& prop);
dbIStream& operator>>(dbIStream& stream, _dbProperty& prop);

}  // namespace odb
