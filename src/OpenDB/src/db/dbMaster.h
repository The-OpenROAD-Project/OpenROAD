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
#include "dbHashTable.h"
#include "dbTypes.h"
#include "odb.h"

namespace odb {

template <class T>
class dbTable;
class _dbMTerm;
class _dbBox;
class _dbMPin;
class _dbTarget;
class _dbSite;
class _dbDatabase;
class _dbTechAntennaPinModel;
class dbBoxItr;
class dbMPinItr;
class dbTargetItr;
class dbIStream;
class dbOStream;
class dbDiff;

struct dbMasterFlags
{
  uint                _frozen : 1;
  uint                _x_symmetry : 1;
  uint                _y_symmetry : 1;
  uint                _R90_symmetry : 1;
  dbMasterType::Value _type : 6;
  uint                _mark : 1;
  uint                _sequential : 1;
  uint                _special_power : 1;
  uint                _spare_bits_19 : 19;
};

class _dbMaster : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  dbMasterFlags                    _flags;
  int                              _x;
  int                              _y;
  uint                             _height;
  uint                             _width;
  uint                             _mterm_cnt;
  uint                             _id;
  char*                            _name;
  dbId<_dbMaster>                  _next_entry;
  dbId<_dbMaster>                  _leq;
  dbId<_dbMaster>                  _eeq;
  dbId<_dbBox>                     _obstructions;
  dbId<_dbSite>                    _site;
  dbHashTable<_dbMTerm>            _mterm_hash;
  dbTable<_dbMTerm>*               _mterm_tbl;
  dbTable<_dbMPin>*                _mpin_tbl;
  dbTable<_dbTarget>*              _target_tbl;
  dbTable<_dbBox>*                 _box_tbl;
  dbTable<_dbTechAntennaPinModel>* _antenna_pin_model_tbl;

  void* _sta_cell;  // not saved

  // NON-PERSISTANT-MEMBERS
  dbBoxItr*    _box_itr;
  dbMPinItr*   _mpin_itr;
  dbTargetItr* _target_itr;
  int          _clocked_mterm_index;
  int          _output_mterm_index;

  _dbMaster(_dbDatabase* db);
  _dbMaster(_dbDatabase* db, const _dbMaster& m);
  ~_dbMaster();
  bool operator==(const _dbMaster& rhs) const;
  bool operator!=(const _dbMaster& rhs) const { return !operator==(rhs); }
  void differences(dbDiff& diff, const char* field, const _dbMaster& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  dbObjectTable* getObjectTable(dbObjectType type);
};

dbOStream& operator<<(dbOStream& stream, const _dbMaster& master);
dbIStream& operator>>(dbIStream& stream, _dbMaster& master);

}  // namespace odb
