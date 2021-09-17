///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbId.h"
#include "dbTypes.h"
#include "odb.h"

namespace odb {

class _dbNet;
class _dbCCSeg;
class _dbDatabase;
class dbIStream;
class dbOStream;
class dbDiff;

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

  void differences(dbDiff& diff,
                   const char* field,
                   const _dbCapNode& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
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
