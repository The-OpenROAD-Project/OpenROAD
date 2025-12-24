// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbDatabase.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"

namespace odb {

class _dbNet;
class _dbDatabase;
class dbIStream;
class dbOStream;

struct _dbRSegFlags
{
  uint32_t path_dir : 1;       // 0 == low to hi coord
  uint32_t allocated_cap : 1;  // 0, cap points to target node cap
                               // 1, cap is allocated
  uint32_t update_cap : 1;
  uint32_t spare_bits_29 : 29;
};

class _dbRSeg : public _dbObject
{
 public:
  enum Field  // dbJournal field names
  {
    kFlags,
    kSource,
    kTarget,
    kResistance,
    kCapacitance,
    kCoordinates,
    kAddRSegCapacitance,
    kAddRSegResistance
  };

  _dbRSeg(_dbDatabase*, const _dbRSeg& s);
  _dbRSeg(_dbDatabase*);

  bool operator==(const _dbRSeg& rhs) const;
  bool operator!=(const _dbRSeg& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);
  bool operator<(const _dbRSeg& rhs) const
  {
    _dbRSeg* o1 = (_dbRSeg*) this;
    _dbRSeg* o2 = (_dbRSeg*) &rhs;
    return o1->getOID() < o2->getOID();
  }

  // PERSISTANT-MEMBERS
  _dbRSegFlags flags_;
  uint32_t source_;  // rc-network node-id
  uint32_t target_;  // rc-network node-id
  int xcoord_;
  int ycoord_;
  dbId<_dbRSeg> next_;
};

inline _dbRSeg::_dbRSeg(_dbDatabase*, const _dbRSeg& s)
    : flags_(s.flags_),
      source_(s.source_),
      target_(s.target_),
      xcoord_(s.xcoord_),
      ycoord_(s.ycoord_),
      next_(s.next_)
{
}

inline _dbRSeg::_dbRSeg(_dbDatabase*)
{
  flags_.spare_bits_29 = 0;
  flags_.update_cap = 0;
  flags_.path_dir = 0;
  flags_.allocated_cap = 0;

  source_ = 0;
  target_ = 0;
  xcoord_ = 0;
  ycoord_ = 0;
}

inline dbOStream& operator<<(dbOStream& stream, const _dbRSeg& seg)
{
  uint32_t* bit_field = (uint32_t*) &seg.flags_;
  stream << *bit_field;
  stream << seg.source_;
  stream << seg.target_;
  stream << seg.xcoord_;
  stream << seg.ycoord_;
  stream << seg.next_;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbRSeg& seg)
{
  uint32_t* bit_field = (uint32_t*) &seg.flags_;
  stream >> *bit_field;
  stream >> seg.source_;
  stream >> seg.target_;
  stream >> seg.xcoord_;
  stream >> seg.ycoord_;
  stream >> seg.next_;
  return stream;
}

}  // namespace odb
