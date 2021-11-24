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
#include "dbId.h"
#include "dbTypes.h"
#include "dbVector.h"
#include "odb.h"

namespace odb {

class _dbITerm;
class _dbBTerm;
class _dbWire;
class _dbSWire;
class _dbCapNode;
class _dbRSeg;
class _dbCCSeg;
class _dbTechNonDefaultRule;
class _dbDatabase;
class _dbGroup;
class dbIStream;
class dbOStream;
class dbDiff;

struct _dbNetFlags
{
  dbSigType::Value _sig_type : 4;
  dbWireType::Value _wire_type : 4;
  uint _special : 1;
  uint _wild_connect : 1;
  uint _wire_ordered : 1;
  uint _buffered : 1;
  uint _disconnected : 1;  // this flag is only valid if wire_ordered == true
  uint _spef : 1;
  uint _select : 1;
  uint _mark : 1;
  uint _mark_1 : 1;
  uint _wire_altered : 1;
  uint _extracted : 1;
  uint _rc_graph : 1;
  uint _reduced : 1;
  uint _set_io : 1;
  uint _io : 1;
  uint _dont_touch : 1;
  uint _size_only : 1;
  uint _fixed_bump : 1;
  dbSourceType::Value _source : 4;
  uint _rc_disconnected : 1;
  uint _block_rule : 1;
};

class _dbNet : public _dbObject
{
 public:
  enum Field  // dbJournal field name
  {
    FLAGS,
    NON_DEFAULT_RULE,
    TERM_EXTID,
    HEAD_CAPNODE,
    HEAD_RSEG,
    REVERSE_RSEG,
    INVALIDATETIMING,
  };

  // PERSISTANT-MEMBERS
  _dbNetFlags _flags;
  char* _name;
  union
  {
    float _gndc_calibration_factor;
    float _refCC;
  };
  union
  {
    float _cc_calibration_factor;
    float _dbCC;
    float _CcMatchRatio;
  };
  dbId<_dbNet> _next_entry;
  dbId<_dbITerm> _iterms;
  dbId<_dbBTerm> _bterms;
  dbId<_dbWire> _wire;
  dbId<_dbWire> _global_wire;
  dbId<_dbSWire> _swires;
  dbId<_dbCapNode> _cap_nodes;
  dbId<_dbRSeg> _r_segs;
  dbId<_dbTechNonDefaultRule> _non_default_rule;
  dbVector<dbId<_dbGroup>> _groups;
  int _weight;
  int _xtalk;
  float _ccAdjustFactor;
  uint _ccAdjustOrder;
  // NON PERSISTANT-MEMBERS
  int _drivingIterm;

  _dbNet(_dbDatabase*);
  _dbNet(_dbDatabase*, const _dbNet& n);
  ~_dbNet();

  bool operator==(const _dbNet& rhs) const;
  bool operator!=(const _dbNet& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbNet& rhs) const;
  void differences(dbDiff& diff, const char* field, const _dbNet& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

dbOStream& operator<<(dbOStream& stream, const _dbNet& net);
dbIStream& operator>>(dbIStream& stream, _dbNet& net);

}  // namespace odb
