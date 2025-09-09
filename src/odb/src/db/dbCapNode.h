// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbDatabase.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

class _dbNet;
class _dbCCSeg;
class _dbDatabase;
class dbIStream;
class dbOStream;

struct _dbCapNodeFlags
{
  uint _internal : 1;
  uint _iterm : 1;
  uint _bterm : 1;
  uint _branch : 1;
  uint _foreign : 1;
  uint _childrenCnt : 5;
  uint _select : 1;
  uint _name : 1;
  uint _sort_index : 20;
};

class _dbCapNode : public _dbObject
{
 public:
  enum Fields  // dbJournal field names
  {
    FLAGS,
    NODE_NUM,
    CAPACITANCE,
    ADDCAPNCAPACITANCE,
    SETNET,
    SETNEXT
  };

  // PERSISTANT-MEMBERS
  _dbCapNodeFlags _flags;
  uint _node_num;  // rc-network node-id
  dbId<_dbNet> _net;
  dbId<_dbCapNode> _next;
  dbId<_dbCCSeg> _cc_segs;

  _dbCapNode(_dbDatabase*);
  _dbCapNode(_dbDatabase*, const _dbCapNode& n);
  ~_dbCapNode();

  bool operator==(const _dbCapNode& rhs) const;
  bool operator!=(const _dbCapNode& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbCapNode& rhs) const
  {
    _dbCapNode* o1 = (_dbCapNode*) this;
    _dbCapNode* o2 = (_dbCapNode*) &rhs;
    return o1->getOID() < o2->getOID();
  }

  void collectMemInfo(MemInfo& info);
};

inline _dbCapNode::_dbCapNode(_dbDatabase*)
{
  _flags._internal = 0;
  _flags._iterm = 0;
  _flags._bterm = 0;
  _flags._branch = 0;
  _flags._foreign = 0;
  _flags._childrenCnt = 0;
  _flags._select = 0;
  _flags._name = 0;
  _flags._sort_index = 0;
  _net = 0;
  _node_num = 0;
}

inline _dbCapNode::_dbCapNode(_dbDatabase*, const _dbCapNode& n)
    : _flags(n._flags), _node_num(n._node_num), _net(n._net), _next(n._next)
{
}

inline _dbCapNode::~_dbCapNode()
{
}

inline dbOStream& operator<<(dbOStream& stream, const _dbCapNode& seg)
{
  uint* bit_field = (uint*) &seg._flags;
  stream << *bit_field;

  stream << seg._node_num;
  stream << seg._net;
  stream << seg._next;
  stream << seg._cc_segs;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbCapNode& seg)
{
  uint* bit_field = (uint*) &seg._flags;
  stream >> *bit_field;
  stream >> seg._node_num;
  stream >> seg._net;
  stream >> seg._next;
  stream >> seg._cc_segs;
  return stream;
}

}  // namespace odb
