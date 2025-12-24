// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbVector.h"  // disconnect the child-iterm
#include "odb/dbId.h"
#include "odb/dbTypes.h"

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
  dbOrientType::Value orient : 4;
  dbPlacementStatus::Value status : 4;
  uint32_t user_flag_1 : 1;
  uint32_t user_flag_2 : 1;
  uint32_t user_flag_3 : 1;
  uint32_t physical_only : 1;
  uint32_t dont_touch : 1;
  dbSourceType::Value source : 4;
  uint32_t eco_create : 1;
  uint32_t eco_destroy : 1;
  uint32_t eco_modify : 1;
  uint32_t level : 11;
};

class _dbInst : public _dbObject
{
 public:
  enum Field  // dbJournalField name
  {
    kFlags,
    kOrigin,
    kName
  };

  _dbInst(_dbDatabase*);
  _dbInst(_dbDatabase*, const _dbInst& i);
  ~_dbInst();

  bool operator==(const _dbInst& rhs) const;
  bool operator!=(const _dbInst& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbInst& rhs) const;
  void collectMemInfo(MemInfo& info);
  static void setInstBBox(_dbInst* inst);

  _dbInstFlags flags_;
  char* name_;
  int x_;
  int y_;
  int weight_;
  dbId<_dbInst> next_entry_;
  dbId<_dbInstHdr> inst_hdr_;
  dbId<_dbBox> bbox_;
  dbId<_dbRegion> region_;
  dbId<_dbModule> module_;
  dbId<_dbGroup> group_;
  dbId<_dbInst> region_next_;
  dbId<_dbInst> module_next_;
  dbId<_dbInst> group_next_;
  dbId<_dbInst> region_prev_;
  dbId<_dbInst> module_prev_;
  dbId<_dbHier> hierarchy_;
  dbVector<uint32_t> iterms_;
  dbId<_dbBox> halo_;
  uint32_t pin_access_idx_;
};

dbOStream& operator<<(dbOStream& stream, const _dbInst& inst);
dbIStream& operator>>(dbIStream& stream, _dbInst& inst);

}  // namespace odb
