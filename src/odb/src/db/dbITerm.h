// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>

#include "boost/container/flat_map.hpp"
#include "dbCore.h"
#include "dbDatabase.h"
#include "odb/db.h"
#include "odb/dbId.h"

namespace odb {

class _dbNet;
class _dbModNet;
class _dbMTerm;
class _dbInst;
class _dbITerm;
class _dbDatabase;
class dbIStream;
class dbOStream;
class _dbAccessPoint;
class _dbMPin;

struct dbITermFlags
{
  // note: number of bits must add up to 32 !!!
  uint32_t mterm_idx : 20;  // index into inst-hdr-mterm-vector
  uint32_t spare_bits : 7;
  uint32_t clocked : 1;
  uint32_t mark : 1;
  uint32_t spef : 1;       // Spef flag
  uint32_t special : 1;    // Special net connection.
  uint32_t connected : 1;  // terminal is physically connected
};

class _dbITerm : public _dbObject
{
 public:
  enum Field  // dbJournal field name
  {
    kFlags
  };

  _dbITerm(_dbDatabase*);
  _dbITerm(_dbDatabase*, const _dbITerm& i);

  bool operator==(const _dbITerm& rhs) const;
  bool operator!=(const _dbITerm& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbITerm& rhs) const;
  void collectMemInfo(MemInfo& info);

  _dbMTerm* getMTerm() const;
  _dbInst* getInst() const;

  dbITermFlags flags_;
  uint32_t ext_id_;
  dbId<_dbNet> net_;
  dbId<_dbModNet> mnet_;
  dbId<_dbInst> inst_;
  dbId<_dbITerm> next_net_iterm_;
  dbId<_dbITerm> prev_net_iterm_;
  dbId<_dbITerm> next_modnet_iterm_;
  dbId<_dbITerm> prev_modnet_iterm_;
  uint32_t sta_vertex_id_;  // not saved
  boost::container::flat_map<dbId<_dbMPin>, dbId<_dbAccessPoint>> aps_;
};

inline _dbITerm::_dbITerm(_dbDatabase*)
{
  // For pointer tagging the bottom 3 bits.
  static_assert(alignof(_dbITerm) % 8 == 0);
  flags_.mterm_idx = 0;
  flags_.spare_bits = 0;
  flags_.clocked = 0;
  flags_.mark = 0;
  flags_.spef = 0;
  flags_.special = 0;
  flags_.connected = 0;
  ext_id_ = 0;
  sta_vertex_id_ = 0;
}

inline _dbITerm::_dbITerm(_dbDatabase*, const _dbITerm& i)
    : flags_(i.flags_),
      ext_id_(i.ext_id_),
      net_(i.net_),
      inst_(i.inst_),
      next_net_iterm_(i.next_net_iterm_),
      prev_net_iterm_(i.prev_net_iterm_),
      next_modnet_iterm_(i.next_modnet_iterm_),
      prev_modnet_iterm_(i.prev_modnet_iterm_),
      sta_vertex_id_(0)
{
}

inline dbOStream& operator<<(dbOStream& stream, const _dbITerm& iterm)
{
  uint32_t* bit_field = (uint32_t*) &iterm.flags_;
  stream << *bit_field;
  stream << iterm.ext_id_;
  stream << iterm.net_;
  stream << iterm.inst_;
  stream << iterm.next_net_iterm_;
  stream << iterm.prev_net_iterm_;
  stream << iterm.mnet_;
  stream << iterm.next_modnet_iterm_;
  stream << iterm.prev_modnet_iterm_;
  stream << iterm.aps_;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbITerm& iterm)
{
  dbBlock* block = (dbBlock*) (iterm.getOwner());
  _dbDatabase* db = (_dbDatabase*) (block->getDataBase());
  uint32_t* bit_field = (uint32_t*) &iterm.flags_;
  stream >> *bit_field;
  stream >> iterm.ext_id_;
  stream >> iterm.net_;
  stream >> iterm.inst_;
  stream >> iterm.next_net_iterm_;
  stream >> iterm.prev_net_iterm_;
  if (db->isSchema(kSchemaUpdateHierarchy)) {
    stream >> iterm.mnet_;
    stream >> iterm.next_modnet_iterm_;
    stream >> iterm.prev_modnet_iterm_;
  }
  stream >> iterm.aps_;
  return stream;
}

}  // namespace odb
