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
#include "odb.h"

namespace odb {

class _dbNet;
class _dbMTerm;
class _dbInst;
class _dbITerm;
class _dbDatabase;
class dbIStream;
class dbOStream;
class dbDiff;

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
  dbId<_dbInst> _inst;
  dbId<_dbITerm> _next_net_iterm;
  dbId<_dbITerm> _prev_net_iterm;
  uint32_t _sta_vertex_id;  // not saved

  _dbITerm(_dbDatabase*);
  _dbITerm(_dbDatabase*, const _dbITerm& i);
  ~_dbITerm();
  bool operator==(const _dbITerm& rhs) const;
  bool operator!=(const _dbITerm& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbITerm& rhs) const;
  void differences(dbDiff& diff, const char* field, const _dbITerm& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

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
      _sta_vertex_id(0)
{
}

inline _dbITerm::~_dbITerm()
{
}

inline dbOStream& operator<<(dbOStream& stream, const _dbITerm& iterm)
{
  uint* bit_field = (uint*) &iterm._flags;
  stream << *bit_field;
  stream << iterm._ext_id;
  stream << iterm._net;
  stream << iterm._inst;
  stream << iterm._next_net_iterm;
  stream << iterm._prev_net_iterm;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbITerm& iterm)
{
  uint* bit_field = (uint*) &iterm._flags;
  stream >> *bit_field;
  stream >> iterm._ext_id;
  stream >> iterm._net;
  stream >> iterm._inst;
  stream >> iterm._next_net_iterm;
  stream >> iterm._prev_net_iterm;
  return stream;
}

}  // namespace odb
