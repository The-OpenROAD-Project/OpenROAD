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
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

class _dbMPin;
class _dbTarget;
class _dbDatabase;
class _dbTechAntennaAreaElement;
class _dbTechAntennaPinModel;
class dbIStream;
class dbOStream;

struct _dbMTermFlags
{
  dbIoType::Value _io_type : 4;
  dbSigType::Value _sig_type : 4;
  dbMTermShapeType::Value _shape_type : 2;
  uint _mark : 1;
  uint _spare_bits : 21;
};

class _dbMTerm : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  _dbMTermFlags _flags;
  uint _order_id;
  char* _name;
  dbId<_dbMTerm> _next_entry;
  dbId<_dbMTerm> _next_mterm;
  dbId<_dbMPin> _pins;
  dbId<_dbTarget> _targets;
  dbId<_dbTechAntennaPinModel> _oxide1;
  dbId<_dbTechAntennaPinModel> _oxide2;

  dbVector<_dbTechAntennaAreaElement*> _par_met_area;
  dbVector<_dbTechAntennaAreaElement*> _par_met_sidearea;
  dbVector<_dbTechAntennaAreaElement*> _par_cut_area;
  dbVector<_dbTechAntennaAreaElement*> _diffarea;

  void* _sta_port;  // not saved

  friend dbOStream& operator<<(dbOStream& stream, const _dbMTerm& mterm);
  friend dbIStream& operator>>(dbIStream& stream, _dbMTerm& mterm);

  _dbMTerm(_dbDatabase* db);
  ~_dbMTerm();

  bool operator==(const _dbMTerm& rhs) const;
  bool operator!=(const _dbMTerm& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);
};

inline _dbMTerm::_dbMTerm(_dbDatabase*)
{
  _flags._io_type = dbIoType::INPUT;
  _flags._sig_type = dbSigType::SIGNAL;
  _flags._shape_type = dbMTermShapeType::NONE;
  _flags._mark = 0;
  _flags._spare_bits = 0;
  _order_id = 0;
  _name = nullptr;
  _par_met_area.clear();
  _par_met_sidearea.clear();
  _par_cut_area.clear();
  _diffarea.clear();
  _sta_port = nullptr;
}

}  // namespace odb
