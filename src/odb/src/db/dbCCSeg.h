// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbDatabase.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"

namespace odb {

class _dbCapNode;
class _dbDatabase;
class dbIStream;
class dbOStream;

struct _dbCCSegFlags
{
  uint32_t spef_mark_1 : 1;
  uint32_t mark : 1;
  uint32_t inFileCnt : 4;
  uint32_t spare_bits : 26;
};

class _dbCCSeg : public _dbObject
{
 public:
  enum Fields  // dbJournal field names
  {
    kFlags,
    kCapacitance,
    kAddCcCapacitance,
    kSwapCapNode,
    kLinkCcSeg,
    kUnlinkCcSeg,
    kSetAllCcCap
  };

  _dbCCSeg(_dbDatabase* db);
  _dbCCSeg(_dbDatabase* db, const _dbCCSeg& s);

  int idx(dbId<_dbCapNode> n) { return n == cap_node_[0] ? 0 : 1; }
  dbId<_dbCCSeg>& next(dbId<_dbCapNode> n) { return next_[idx(n)]; }

  bool operator==(const _dbCCSeg& rhs) const;
  bool operator!=(const _dbCCSeg& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);

  bool operator<(const _dbCCSeg& rhs) const
  {
    _dbCCSeg* o1 = (_dbCCSeg*) this;
    _dbCCSeg* o2 = (_dbCCSeg*) &rhs;
    return o1->getOID() < o2->getOID();
  }

  // PERSISTANT-MEMBERS
  _dbCCSegFlags flags_;
  dbId<_dbCapNode> cap_node_[2];
  dbId<_dbCCSeg> next_[2];
};

inline _dbCCSeg::_dbCCSeg(_dbDatabase*)
{
  flags_.inFileCnt = 0;
  flags_.spef_mark_1 = 0;
  flags_.mark = 0;
  flags_.spare_bits = 0;
}

inline _dbCCSeg::_dbCCSeg(_dbDatabase*, const _dbCCSeg& s) : flags_(s.flags_)
{
  cap_node_[0] = s.cap_node_[0];
  cap_node_[1] = s.cap_node_[1];
  next_[0] = s.next_[0];
  next_[1] = s.next_[1];
}

inline dbOStream& operator<<(dbOStream& stream, const _dbCCSeg& seg)
{
  uint32_t* bit_field = (uint32_t*) &seg.flags_;
  stream << *bit_field;
  stream << seg.cap_node_[0];
  stream << seg.cap_node_[1];
  stream << seg.next_[0];
  stream << seg.next_[1];
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbCCSeg& seg)
{
  uint32_t* bit_field = (uint32_t*) &seg.flags_;
  stream >> *bit_field;
  stream >> seg.cap_node_[0];
  stream >> seg.cap_node_[1];
  stream >> seg.next_[0];
  stream >> seg.next_[1];
  return stream;
}

}  // namespace odb
