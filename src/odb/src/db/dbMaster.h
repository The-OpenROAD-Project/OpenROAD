// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbHashTable.h"
#include "odb/dbId.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

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
template <uint page_size>
class dbBoxItr;
class dbPolygonItr;
class dbMPinItr;
class dbIStream;
class dbOStream;

struct dbMasterFlags
{
  uint _frozen : 1;
  uint _x_symmetry : 1;
  uint _y_symmetry : 1;
  uint _R90_symmetry : 1;
  dbMasterType::Value _type : 6;
  uint _mark : 1;
  uint _sequential : 1;
  uint _special_power : 1;
  uint _spare_bits_19 : 19;
};

class _dbMaster : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  dbMasterFlags _flags;
  int _x;
  int _y;
  uint _height;
  uint _width;
  uint _mterm_cnt;
  uint _id;
  char* _name;
  dbId<_dbMaster> _next_entry;
  dbId<_dbMaster> _leq;
  dbId<_dbMaster> _eeq;
  dbId<_dbBox> _obstructions;
  dbId<_dbPolygon> _poly_obstructions;
  dbId<_dbLib> _lib_for_site;
  dbId<_dbSite> _site;
  dbHashTable<_dbMTerm, 4> _mterm_hash;
  dbTable<_dbMTerm, 4>* _mterm_tbl;
  dbTable<_dbMPin, 4>* _mpin_tbl;
  dbTable<_dbBox, 8>* _box_tbl;
  dbTable<_dbPolygon, 8>* _poly_box_tbl;
  dbTable<_dbTechAntennaPinModel, 8>* _antenna_pin_model_tbl;
  dbTable<_dbMasterEdgeType, 8>* edge_types_tbl_;

  void* _sta_cell;  // not saved

  // NON-PERSISTANT-MEMBERS
  dbBoxItr<8>* _box_itr;
  dbPolygonItr* _pbox_itr;
  dbBoxItr<8>* _pbox_box_itr;
  dbMPinItr* _mpin_itr;

  _dbMaster(_dbDatabase* db);
  ~_dbMaster();
  bool operator==(const _dbMaster& rhs) const;
  bool operator!=(const _dbMaster& rhs) const { return !operator==(rhs); }
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);
};

dbOStream& operator<<(dbOStream& stream, const _dbMaster& master);
dbIStream& operator>>(dbIStream& stream, _dbMaster& master);

}  // namespace odb
