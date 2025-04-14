// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

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
