// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>

#include "dbCore.h"
#include "dbHashTable.hpp"
#include "dbVector.h"
#include "odb/dbMatrix.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

template <class T>
class dbTable;

class _dbProperty;
class dbPropertyItr;
class _dbNameCache;
class _dbTechLayer;
class _dbTechLayerRule;
class _dbTechVia;
class _dbTechNonDefaultRule;
class _dbTechSameNetRule;
class _dbTechLayerAntennaRule;
class _dbTechViaRule;
class _dbTechViaLayerRule;
class _dbTechViaGenerateRule;
class _dbBox;
class _dbDatabase;
class _dbMetalWidthViaMap;
class _dbCellEdgeSpacing;
class dbTechLayerItr;
class dbBoxItr;
class dbIStream;
class dbOStream;

struct _dbTechFlags
{
  dbOnOffType::Value _namecase : 1;
  dbOnOffType::Value _haswireext : 1;
  dbOnOffType::Value _nowireext : 1;
  dbOnOffType::Value _hasclmeas : 1;
  dbClMeasureType::Value _clmeas : 2;
  dbOnOffType::Value _hasminspobs : 1;
  dbOnOffType::Value _minspobs : 1;
  dbOnOffType::Value _hasminsppin : 1;
  dbOnOffType::Value _minsppin : 1;
  uint _spare_bits : 22;
};

class _dbTech : public _dbObject
{
 private:
  double _version;

 public:
  // PERSISTANT-MEMBERS
  std::string _name;
  int _via_cnt;
  int _layer_cnt;
  int _rlayer_cnt;
  int _lef_units;
  int _dbu_per_micron;
  int _mfgrid;
  _dbTechFlags _flags;
  dbId<_dbTechLayer> _bottom;
  dbId<_dbTechLayer> _top;
  dbId<_dbTechNonDefaultRule> _non_default_rules;
  dbVector<dbId<_dbTechSameNetRule>> _samenet_rules;
  dbMatrix<dbId<_dbTechSameNetRule>> _samenet_matrix;
  dbHashTable<_dbTechVia> _via_hash;

  // NON-PERSISTANT-STREAMED-MEMBERS
  dbTable<_dbTechLayer>* _layer_tbl;
  dbTable<_dbTechVia>* _via_tbl;
  dbTable<_dbTechNonDefaultRule>* _non_default_rule_tbl;
  dbTable<_dbTechLayerRule>* _layer_rule_tbl;
  dbTable<_dbBox>* _box_tbl;
  dbTable<_dbTechSameNetRule>* _samenet_rule_tbl;
  dbTable<_dbTechLayerAntennaRule>* _antenna_rule_tbl;
  dbTable<_dbTechViaRule>* _via_rule_tbl;
  dbTable<_dbTechViaLayerRule>* _via_layer_rule_tbl;
  dbTable<_dbTechViaGenerateRule>* _via_generate_rule_tbl;
  dbTable<_dbProperty>* _prop_tbl;
  dbTable<_dbMetalWidthViaMap>* _metal_width_via_map_tbl;
  dbTable<_dbCellEdgeSpacing>* cell_edge_spacing_tbl_;

  _dbNameCache* _name_cache;

  // NON-PERSISTANT-NON-STREAMED-MEMBERS
  dbTechLayerItr* _layer_itr;
  dbBoxItr* _box_itr;
  dbPropertyItr* _prop_itr;

  double _getLefVersion() const;
  std::string _getLefVersionStr() const;
  void _setLefVersion(double inver);

  _dbTech(_dbDatabase* db);
  ~_dbTech();

  bool operator==(const _dbTech& rhs) const;
  bool operator!=(const _dbTech& rhs) const { return !operator==(rhs); }
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);
};

dbOStream& operator<<(dbOStream& stream, const _dbTech& tech);
dbIStream& operator>>(dbIStream& stream, _dbTech& tech);

}  // namespace odb
