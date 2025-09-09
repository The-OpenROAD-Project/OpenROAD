// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbVector.h"  // disconnect the child-iterm
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

class _dbBox;
class _dbInstHdr;
class _dbHier;
class _dbITerm;
class _dbRegion;
class _dbDatabase;
class _dbModule;
class _dbGroup;
class dbInst;
class dbIStream;
class dbOStream;

struct _dbInstFlags
{
  dbOrientType::Value _orient : 4;
  dbPlacementStatus::Value _status : 4;
  uint _user_flag_1 : 1;
  uint _user_flag_2 : 1;
  uint _user_flag_3 : 1;
  uint _physical_only : 1;
  uint _dont_touch : 1;
  dbSourceType::Value _source : 4;
  uint _eco_create : 1;
  uint _eco_destroy : 1;
  uint _eco_modify : 1;
  uint _level : 11;
};

class _dbInst : public _dbObject
{
 public:
  enum Field  // dbJournalField name
  {
    FLAGS,
    ORIGIN
  };

  _dbInstFlags _flags;
  char* _name;
  int _x;
  int _y;
  int _weight;
  dbId<_dbInst> _next_entry;
  dbId<_dbInstHdr> _inst_hdr;
  dbId<_dbBox> _bbox;
  dbId<_dbRegion> _region;
  dbId<_dbModule> _module;
  dbId<_dbGroup> _group;
  dbId<_dbInst> _region_next;
  dbId<_dbInst> _module_next;
  dbId<_dbInst> _group_next;
  dbId<_dbInst> _region_prev;
  dbId<_dbInst> _module_prev;
  dbId<_dbHier> _hierarchy;
  dbVector<uint> _iterms;
  dbId<_dbBox> _halo;
  uint pin_access_idx_;

  _dbInst(_dbDatabase*);
  _dbInst(_dbDatabase*, const _dbInst& i);
  ~_dbInst();

  bool operator==(const _dbInst& rhs) const;
  bool operator!=(const _dbInst& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbInst& rhs) const;
  void collectMemInfo(MemInfo& info);
  static void setInstBBox(_dbInst* inst);
};

dbOStream& operator<<(dbOStream& stream, const _dbInst& inst);
dbIStream& operator>>(dbIStream& stream, _dbInst& inst);

}  // namespace odb
