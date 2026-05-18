// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbHashTable.h"
#include "odb/dbId.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"

namespace odb {

class _dbMTerm;
class _dbBox;
class _dbPolygon;
class _dbLib;
class _dbMPin;
class _dbSite;
class _dbDatabase;
class _dbTechAntennaPinModel;
class _dbMasterEdgeType;
template <uint32_t page_size>
class dbBoxItr;
class dbPolygonItr;
class dbMPinItr;
class dbIStream;
class dbOStream;

struct dbMasterFlags
{
  uint32_t frozen : 1;
  uint32_t x_symmetry : 1;
  uint32_t y_symmetry : 1;
  uint32_t R90_symmetry : 1;
  dbMasterType::Value type : 6;
  uint32_t mark : 1;
  uint32_t sequential : 1;
  uint32_t special_power : 1;
  uint32_t spare_bits_19 : 19;
};

class _dbMaster : public _dbObject
{
 public:
  _dbMaster(_dbDatabase* db);
  ~_dbMaster();
  bool operator==(const _dbMaster& rhs) const;
  bool operator!=(const _dbMaster& rhs) const { return !operator==(rhs); }
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);

  // PERSISTANT-MEMBERS
  dbMasterFlags flags_;
  int x_;
  int y_;
  uint32_t height_;
  uint32_t width_;
  uint32_t mterm_cnt_;
  uint32_t id_;
  char* name_;
  dbId<_dbMaster> next_entry_;
  dbId<_dbMaster> leq_;
  dbId<_dbMaster> eeq_;
  dbId<_dbBox> obstructions_;
  dbId<_dbPolygon> poly_obstructions_;
  dbId<_dbLib> lib_for_site_;
  dbId<_dbSite> site_;
  dbHashTable<_dbMTerm, 4> mterm_hash_;
  dbTable<_dbMTerm, 4>* mterm_tbl_;
  dbTable<_dbMPin, 4>* mpin_tbl_;
  dbTable<_dbBox, 8>* box_tbl_;
  dbTable<_dbPolygon, 8>* poly_box_tbl_;
  dbTable<_dbTechAntennaPinModel, 8>* antenna_pin_model_tbl_;
  dbTable<_dbMasterEdgeType, 8>* edge_types_tbl_;

  void* sta_cell_;  // not saved

  // NON-PERSISTANT-MEMBERS
  dbBoxItr<8>* box_itr_;
  dbPolygonItr* pbox_itr_;
  dbBoxItr<8>* pbox_box_itr_;
  dbMPinItr* mpin_itr_;
};

dbOStream& operator<<(dbOStream& stream, const _dbMaster& master);
dbIStream& operator>>(dbIStream& stream, _dbMaster& master);

}  // namespace odb
