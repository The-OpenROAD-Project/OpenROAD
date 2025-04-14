// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbHashTable.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

template <class T>
class dbTable;
class dbIStream;
class dbOStream;
class _dbLib;
class _dbSite;

struct dbSiteFlags
{
  uint _x_symmetry : 1;
  uint _y_symmetry : 1;
  uint _R90_symmetry : 1;
  dbSiteClass::Value _class : 4;
  uint _is_hybrid : 1;
  uint _spare_bits : 24;
};

struct OrientedSiteInternal
{
  dbId<_dbLib> lib;
  dbId<_dbSite> site;
  dbOrientType orientation;
  bool operator==(const OrientedSiteInternal& rhs) const;
};

dbOStream& operator<<(dbOStream& stream, const OrientedSiteInternal& s);
dbIStream& operator>>(dbIStream& stream, OrientedSiteInternal& s);

class _dbSite : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  dbSiteFlags _flags;
  char* _name;
  int _height;
  int _width;
  dbId<_dbSite> _next_entry;
  dbVector<OrientedSiteInternal> _row_pattern;

  _dbSite(_dbDatabase*, const _dbSite& s);
  _dbSite(_dbDatabase*);
  ~_dbSite();

  bool operator==(const _dbSite& rhs) const;
  bool operator!=(const _dbSite& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);
};

dbOStream& operator<<(dbOStream& stream, const _dbSite& site);
dbIStream& operator>>(dbIStream& stream, _dbSite& site);
}  // namespace odb
