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

class _dbNet;
class _dbModNet;
class _dbBox;
class _dbBlock;
class _dbBPin;
class _dbITerm;
class _dbDatabase;
class dbIStream;
class dbOStream;

struct _dbBTermFlags
{
  dbIoType::Value io_type : 4;
  dbSigType::Value sig_type : 4;
  uint32_t orient : 4;  // This field is not used anymore. Replaced by bpin...
  uint32_t status : 4;  // This field is not used anymore. Replaced by bpin...
  uint32_t spef : 1;
  uint32_t special : 1;
  uint32_t mark : 1;
  uint32_t spare_bits : 13;
};

//
// block terminal
//
class _dbBTerm : public _dbObject
{
 public:
  enum Field  // dbJournalField name
  {
    kFlags
  };

  _dbBTerm(_dbDatabase*);
  _dbBTerm(_dbDatabase*, const _dbBTerm& b);
  ~_dbBTerm();

  void connectNet(_dbNet* net, _dbBlock* block);
  void connectModNet(_dbModNet* net, _dbBlock* block);
  void disconnectNet(_dbBTerm* bterm, _dbBlock* block);
  void disconnectModNet(_dbBTerm* bterm, _dbBlock* block);
  void setMirroredConstraintRegion(const Rect& region, _dbBlock* block);

  bool operator==(const _dbBTerm& rhs) const;
  bool operator!=(const _dbBTerm& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbBTerm& rhs) const;
  void collectMemInfo(MemInfo& info);

  // PERSISTANT-MEMBERS
  _dbBTermFlags flags_;
  uint32_t ext_id_;
  char* name_;
  dbId<_dbBTerm> next_entry_;
  dbId<_dbNet> net_;
  dbId<_dbModNet> mnet_;
  dbId<_dbBTerm> next_bterm_;
  dbId<_dbBTerm> prev_bterm_;
  dbId<_dbBTerm> next_modnet_bterm_;
  dbId<_dbBTerm> prev_modnet_bterm_;
  dbId<_dbBlock> parent_block_;  // Up hierarchy
  dbId<_dbITerm> parent_iterm_;  // Up hierarchy
  dbId<_dbBPin> bpins_;          // Up hierarchy
  dbId<_dbBTerm> ground_pin_;
  dbId<_dbBTerm> supply_pin_;
  std::uint32_t sta_vertex_id_;  // not saved
  Rect constraint_region_;
  dbId<_dbBTerm> mirrored_bterm_;
  bool is_mirrored_;
};

dbOStream& operator<<(dbOStream& stream, const _dbBTerm& bterm);
dbIStream& operator>>(dbIStream& stream, _dbBTerm& bterm);

}  // namespace odb
