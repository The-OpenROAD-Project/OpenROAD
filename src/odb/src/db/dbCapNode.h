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
class _dbCCSeg;
class _dbDatabase;
class dbIStream;
class dbOStream;

struct _dbCapNodeFlags
{
  uint32_t internal : 1;
  uint32_t iterm : 1;
  uint32_t bterm : 1;
  uint32_t branch : 1;
  uint32_t foreign : 1;
  uint32_t childrenCnt : 5;
  uint32_t select : 1;
  uint32_t name : 1;
  uint32_t sort_index : 20;
};

class _dbCapNode : public _dbObject
{
 public:
  enum Fields  // dbJournal field names
  {
    kFlags,
    kNodeNum,
    kCapacitance,
    kAddCapnCapacitance,
    kSetNet,
    kSetNext
  };

  _dbCapNode(_dbDatabase*);
  _dbCapNode(_dbDatabase*, const _dbCapNode& n);

  bool operator==(const _dbCapNode& rhs) const;
  bool operator!=(const _dbCapNode& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbCapNode& rhs) const
  {
    _dbCapNode* o1 = (_dbCapNode*) this;
    _dbCapNode* o2 = (_dbCapNode*) &rhs;
    return o1->getOID() < o2->getOID();
  }

  void collectMemInfo(MemInfo& info);

  // PERSISTANT-MEMBERS
  _dbCapNodeFlags flags_;
  uint32_t node_num_;  // rc-network node-id
  dbId<_dbNet> net_;
  dbId<_dbCapNode> next_;
  dbId<_dbCCSeg> cc_segs_;
};

inline _dbCapNode::_dbCapNode(_dbDatabase*)
{
  flags_.internal = 0;
  flags_.iterm = 0;
  flags_.bterm = 0;
  flags_.branch = 0;
  flags_.foreign = 0;
  flags_.childrenCnt = 0;
  flags_.select = 0;
  flags_.name = 0;
  flags_.sort_index = 0;
  net_ = 0;
  node_num_ = 0;
}

inline _dbCapNode::_dbCapNode(_dbDatabase*, const _dbCapNode& n)
    : flags_(n.flags_), node_num_(n.node_num_), net_(n.net_), next_(n.next_)
{
}

inline dbOStream& operator<<(dbOStream& stream, const _dbCapNode& seg)
{
  uint32_t* bit_field = (uint32_t*) &seg.flags_;
  stream << *bit_field;

  stream << seg.node_num_;
  stream << seg.net_;
  stream << seg.next_;
  stream << seg.cc_segs_;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbCapNode& seg)
{
  uint32_t* bit_field = (uint32_t*) &seg.flags_;
  stream >> *bit_field;
  stream >> seg.node_num_;
  stream >> seg.net_;
  stream >> seg.next_;
  stream >> seg.cc_segs_;
  return stream;
}

}  // namespace odb
