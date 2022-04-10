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

#include <cstdint>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbId.h"
#include "dbTypes.h"
#include "odb.h"

namespace odb {

class _dbNet;
class _dbBox;
class _dbBlock;
class _dbBPin;
class _dbITerm;
class _dbDatabase;
class dbIStream;
class dbOStream;
class dbDiff;

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
  dbId<_dbBTerm> _next_bterm;
  dbId<_dbBTerm> _prev_bterm;
  dbId<_dbBlock> _parent_block;  // Up hierarchy: TWG
  dbId<_dbITerm> _parent_iterm;  // Up hierarchy: TWG
  dbId<_dbBPin> _bpins;          // Up hierarchy: TWG
  dbId<_dbBTerm> _ground_pin;
  dbId<_dbBTerm> _supply_pin;
  std::uint32_t _sta_vertex_id;  // not saved

  _dbBTerm(_dbDatabase*);
  _dbBTerm(_dbDatabase*, const _dbBTerm& b);
  ~_dbBTerm();

  void connectNet(_dbNet* net, _dbBlock* block);
  void disconnectNet(_dbBTerm* bterm, _dbBlock* block);
  bool operator==(const _dbBTerm& rhs) const;
  bool operator!=(const _dbBTerm& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbBTerm& rhs) const;
  void differences(dbDiff& diff, const char* field, const _dbBTerm& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

dbOStream& operator<<(dbOStream& stream, const _dbBTerm& bterm);
dbIStream& operator>>(dbIStream& stream, _dbBTerm& bterm);

}  // namespace odb
