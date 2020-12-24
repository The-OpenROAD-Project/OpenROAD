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

class _dbCapNode;
class _dbDatabase;
class dbIStream;
class dbOStream;
class dbDiff;

struct _dbCCSegFlags
{
  uint _spef_mark_1 : 1;
  uint _mark : 1;
  uint _inFileCnt : 4;
  uint _spare_bits : 26;
};

class _dbCCSeg : public _dbObject
{
 public:
  enum Fields  // dbJournal field names
  {
    FLAGS,
    CAPACITANCE,
    ADDCCCAPACITANCE,
    SWAPCAPNODE,
    LINKCCSEG,
    UNLINKCCSEG,
    SETALLCCCAP
  };

  // PERSISTANT-MEMBERS
  _dbCCSegFlags    _flags;
  dbId<_dbCapNode> _cap_node[2];
  dbId<_dbCCSeg>   _next[2];

  _dbCCSeg(_dbDatabase* db);
  _dbCCSeg(_dbDatabase* db, const _dbCCSeg& s);
  ~_dbCCSeg();

  int             idx(dbId<_dbCapNode> n) { return n == _cap_node[0] ? 0 : 1; }
  dbId<_dbCCSeg>& next(dbId<_dbCapNode> n) { return _next[idx(n)]; }

  bool operator==(const _dbCCSeg& rhs) const;
  bool operator!=(const _dbCCSeg& rhs) const { return !operator==(rhs); }
  void differences(dbDiff& diff, const char* field, const _dbCCSeg& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  bool operator<(const _dbCCSeg& rhs) const
  {
    _dbCCSeg* o1 = (_dbCCSeg*) this;
    _dbCCSeg* o2 = (_dbCCSeg*) &rhs;
    return o1->getOID() < o2->getOID();
  }
};

inline _dbCCSeg::_dbCCSeg(_dbDatabase*)
{
  _flags._inFileCnt   = 0;
  _flags._spef_mark_1 = 0;
  _flags._mark        = 0;
  _flags._spare_bits  = 0;
}

inline _dbCCSeg::_dbCCSeg(_dbDatabase*, const _dbCCSeg& s) : _flags(s._flags)
{
  _cap_node[0] = s._cap_node[0];
  _cap_node[1] = s._cap_node[1];
  _next[0]     = s._next[0];
  _next[1]     = s._next[1];
}

inline _dbCCSeg::~_dbCCSeg()
{
}

inline dbOStream& operator<<(dbOStream& stream, const _dbCCSeg& seg)
{
  uint* bit_field = (uint*) &seg._flags;
  stream << *bit_field;
  stream << seg._cap_node[0];
  stream << seg._cap_node[1];
  stream << seg._next[0];
  stream << seg._next[1];
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbCCSeg& seg)
{
  uint* bit_field = (uint*) &seg._flags;
  stream >> *bit_field;
  stream >> seg._cap_node[0];
  stream >> seg._cap_node[1];
  stream >> seg._next[0];
  stream >> seg._next[1];
  return stream;
}

}  // namespace odb
