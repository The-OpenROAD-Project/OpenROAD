// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbHashTable.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"

namespace odb {

class dbIStream;
class dbOStream;
class _dbLib;
class _dbSite;

struct dbSiteFlags
{
  uint32_t x_symmetry : 1;
  uint32_t y_symmetry : 1;
  uint32_t R90_symmetry : 1;
  dbSiteClass::Value site_class : 4;
  uint32_t is_hybrid : 1;
  uint32_t spare_bits : 24;
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
  _dbSite(_dbDatabase*, const _dbSite& s);
  _dbSite(_dbDatabase*);
  ~_dbSite();

  bool operator==(const _dbSite& rhs) const;
  bool operator!=(const _dbSite& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);

  // PERSISTANT-MEMBERS
  dbSiteFlags flags_;
  char* name_;
  int height_;
  int width_;
  dbId<_dbSite> next_entry_;
  dbVector<OrientedSiteInternal> row_pattern_;
};

dbOStream& operator<<(dbOStream& stream, const _dbSite& site);
dbIStream& operator>>(dbIStream& stream, _dbSite& site);
}  // namespace odb
