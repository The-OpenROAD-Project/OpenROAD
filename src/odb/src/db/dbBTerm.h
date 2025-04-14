// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbDatabase.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

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
  dbIoType::Value _io_type : 4;
  dbSigType::Value _sig_type : 4;
  uint _orient : 4;  // This field is not used anymore. Replaced by bpin...
  uint _status : 4;  // This field is not used anymore. Replaced by bpin...
  uint _spef : 1;
  uint _special : 1;
  uint _mark : 1;
  uint _spare_bits : 13;
};

//
// block terminal
//
class _dbBTerm : public _dbObject
{
 public:
  enum Field  // dbJournalField name
  {
    FLAGS
  };
  // PERSISTANT-MEMBERS
  _dbBTermFlags _flags;
  uint _ext_id;
  char* _name;
  dbId<_dbBTerm> _next_entry;
  dbId<_dbNet> _net;
  dbId<_dbModNet> _mnet;
  dbId<_dbBTerm> _next_bterm;
  dbId<_dbBTerm> _prev_bterm;
  dbId<_dbBTerm> _next_modnet_bterm;
  dbId<_dbBTerm> _prev_modnet_bterm;
  dbId<_dbBlock> _parent_block;  // Up hierarchy: TWG
  dbId<_dbITerm> _parent_iterm;  // Up hierarchy: TWG
  dbId<_dbBPin> _bpins;          // Up hierarchy: TWG
  dbId<_dbBTerm> _ground_pin;
  dbId<_dbBTerm> _supply_pin;
  std::uint32_t _sta_vertex_id;  // not saved
  Rect _constraint_region;
  dbId<_dbBTerm> _mirrored_bterm;
  bool _is_mirrored;

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
};

dbOStream& operator<<(dbOStream& stream, const _dbBTerm& bterm);
dbIStream& operator>>(dbIStream& stream, _dbBTerm& bterm);

}  // namespace odb
