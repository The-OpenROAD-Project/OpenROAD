// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <boost/container/flat_map.hpp>
#include <cstdint>
#include <map>

#include "dbCore.h"
#include "dbDatabase.h"
#include "odb/db.h"
#include "odb/dbId.h"
#include "odb/odb.h"

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
  uint _mterm_idx : 20;  // index into inst-hdr-mterm-vector
  uint _spare_bits : 7;
  uint _clocked : 1;
  uint _mark : 1;
  uint _spef : 1;       // Spef flag
  uint _special : 1;    // Special net connection.
  uint _connected : 1;  // terminal is physically connected
};

class _dbITerm : public _dbObject
{
 public:
  enum Field  // dbJournal field name
  {
    FLAGS
  };

  dbITermFlags _flags;
  uint _ext_id;
  dbId<_dbNet> _net;
  dbId<_dbModNet> _mnet;
  dbId<_dbInst> _inst;
  dbId<_dbITerm> _next_net_iterm;
  dbId<_dbITerm> _prev_net_iterm;
  dbId<_dbITerm> _next_modnet_iterm;
  dbId<_dbITerm> _prev_modnet_iterm;
  uint32_t _sta_vertex_id;  // not saved
  boost::container::flat_map<dbId<_dbMPin>, dbId<_dbAccessPoint>> aps_;

  _dbITerm(_dbDatabase*);
  _dbITerm(_dbDatabase*, const _dbITerm& i);

  bool operator==(const _dbITerm& rhs) const;
  bool operator!=(const _dbITerm& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbITerm& rhs) const;
  void collectMemInfo(MemInfo& info);

  _dbMTerm* getMTerm() const;
  _dbInst* getInst() const;
};

inline _dbITerm::_dbITerm(_dbDatabase*)
{
  _flags._mterm_idx = 0;
  _flags._spare_bits = 0;
  _flags._clocked = 0;
  _flags._mark = 0;
  _flags._spef = 0;
  _flags._special = 0;
  _flags._connected = 0;
  _ext_id = 0;
  _sta_vertex_id = 0;
}

inline _dbITerm::_dbITerm(_dbDatabase*, const _dbITerm& i)
    : _flags(i._flags),
      _ext_id(i._ext_id),
      _net(i._net),
      _inst(i._inst),
      _next_net_iterm(i._next_net_iterm),
      _prev_net_iterm(i._prev_net_iterm),
      _next_modnet_iterm(i._next_modnet_iterm),
      _prev_modnet_iterm(i._prev_modnet_iterm),
      _sta_vertex_id(0)
{
}

inline dbOStream& operator<<(dbOStream& stream, const _dbITerm& iterm)
{
  dbBlock* block = (dbBlock*) (iterm.getOwner());
  _dbDatabase* db = (_dbDatabase*) (block->getDataBase());
  uint* bit_field = (uint*) &iterm._flags;
  stream << *bit_field;
  stream << iterm._ext_id;
  stream << iterm._net;
  stream << iterm._inst;
  stream << iterm._next_net_iterm;
  stream << iterm._prev_net_iterm;
  if (db->isSchema(db_schema_update_hierarchy)) {
    stream << iterm._mnet;
    stream << iterm._next_modnet_iterm;
    stream << iterm._prev_modnet_iterm;
  }
  stream << iterm.aps_;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbITerm& iterm)
{
  dbBlock* block = (dbBlock*) (iterm.getOwner());
  _dbDatabase* db = (_dbDatabase*) (block->getDataBase());
  uint* bit_field = (uint*) &iterm._flags;
  stream >> *bit_field;
  stream >> iterm._ext_id;
  stream >> iterm._net;
  stream >> iterm._inst;
  stream >> iterm._next_net_iterm;
  stream >> iterm._prev_net_iterm;
  if (db->isSchema(db_schema_update_hierarchy)) {
    stream >> iterm._mnet;
    stream >> iterm._next_modnet_iterm;
    stream >> iterm._prev_modnet_iterm;
  }
  stream >> iterm.aps_;
  return stream;
}

}  // namespace odb
