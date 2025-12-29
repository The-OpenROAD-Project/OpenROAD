// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"

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
  dbIoType::Value io_type : 4;
  dbSigType::Value sig_type : 4;
  dbMTermShapeType::Value shape_type : 2;
  uint32_t mark : 1;
  uint32_t spare_bits : 21;
};

class _dbMTerm : public _dbObject
{
 public:
  _dbMTerm(_dbDatabase* db);
  ~_dbMTerm();

  bool operator==(const _dbMTerm& rhs) const;
  bool operator!=(const _dbMTerm& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);

  friend dbOStream& operator<<(dbOStream& stream, const _dbMTerm& mterm);
  friend dbIStream& operator>>(dbIStream& stream, _dbMTerm& mterm);

  // PERSISTANT-MEMBERS
  _dbMTermFlags flags_;
  uint32_t order_id_;
  char* name_;
  dbId<_dbMTerm> next_entry_;
  dbId<_dbMTerm> next_mterm_;
  dbId<_dbMPin> pins_;
  dbId<_dbTarget> targets_;
  dbId<_dbTechAntennaPinModel> oxide1_;
  dbId<_dbTechAntennaPinModel> oxide2_;

  dbVector<_dbTechAntennaAreaElement*> par_met_area_;
  dbVector<_dbTechAntennaAreaElement*> par_met_sidearea_;
  dbVector<_dbTechAntennaAreaElement*> par_cut_area_;
  dbVector<_dbTechAntennaAreaElement*> diffarea_;

  void* sta_port_;  // not saved
};

inline _dbMTerm::_dbMTerm(_dbDatabase*)
{
  flags_.io_type = dbIoType::INPUT;
  flags_.sig_type = dbSigType::SIGNAL;
  flags_.shape_type = dbMTermShapeType::NONE;
  flags_.mark = 0;
  flags_.spare_bits = 0;
  order_id_ = 0;
  name_ = nullptr;
  par_met_area_.clear();
  par_met_sidearea_.clear();
  par_cut_area_.clear();
  diffarea_.clear();
  sta_port_ = nullptr;
}

}  // namespace odb
