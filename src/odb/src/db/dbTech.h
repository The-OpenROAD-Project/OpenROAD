// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <string>

#include "dbCore.h"
#include "dbHashTable.hpp"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbMatrix.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"

namespace odb {

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
template <uint32_t page_size>
class dbBoxItr;
class dbIStream;
class dbOStream;

struct _dbTechFlags
{
  dbOnOffType::Value namecase : 1;
  dbOnOffType::Value has_wire_ext : 1;
  dbOnOffType::Value no_wire_ext : 1;
  dbOnOffType::Value has_clearance_measure : 1;
  dbClMeasureType::Value clearance_measure : 2;
  dbOnOffType::Value has_use_min_spacing_obs : 1;
  dbOnOffType::Value use_min_spacing_obs : 1;
  dbOnOffType::Value has_use_min_spacing_pin : 1;
  dbOnOffType::Value use_min_spacing_pin : 1;
  uint32_t spare_bits : 22;
};

class _dbTech : public _dbObject
{
 public:
  _dbTech(_dbDatabase* db);
  ~_dbTech();

  double getLefVersion() const;
  std::string getLefVersionStr() const;
  void setLefVersion(double inver);

  bool operator==(const _dbTech& rhs) const;
  bool operator!=(const _dbTech& rhs) const { return !operator==(rhs); }
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);

 private:
  double version_;

 public:
  // PERSISTANT-MEMBERS
  std::string name_;
  int via_cnt_;
  int layer_cnt_;
  int rlayer_cnt_;
  int lef_units_;
  int manufacturing_grid_;
  _dbTechFlags flags_;
  dbId<_dbTechLayer> bottom_;
  dbId<_dbTechLayer> top_;
  dbId<_dbTechNonDefaultRule> non_default_rules_;
  dbVector<dbId<_dbTechSameNetRule>> samenet_rules_;
  dbMatrix<dbId<_dbTechSameNetRule>> samenet_matrix_;
  dbHashTable<_dbTechVia> via_hash_;

  // NON-PERSISTANT-STREAMED-MEMBERS
  dbTable<_dbTechLayer>* layer_tbl_;
  dbTable<_dbTechVia>* via_tbl_;
  dbTable<_dbTechNonDefaultRule, 4>* non_default_rule_tbl_;
  dbTable<_dbTechLayerRule, 4>* layer_rule_tbl_;
  dbTable<_dbBox>* box_tbl_;
  dbTable<_dbTechSameNetRule, 16>* samenet_rule_tbl_;
  dbTable<_dbTechLayerAntennaRule, 16>* antenna_rule_tbl_;
  dbTable<_dbTechViaRule, 16>* via_rule_tbl_;
  dbTable<_dbTechViaLayerRule, 16>* via_layer_rule_tbl_;
  dbTable<_dbTechViaGenerateRule, 16>* via_generate_rule_tbl_;
  dbTable<_dbProperty>* prop_tbl_;
  dbTable<_dbMetalWidthViaMap>* metal_width_via_map_tbl_;
  dbTable<_dbCellEdgeSpacing>* cell_edge_spacing_tbl_;

  _dbNameCache* name_cache_;

  // NON-PERSISTANT-NON-STREAMED-MEMBERS
  dbTechLayerItr* layer_itr_;
  dbBoxItr<128>* box_itr_;
  dbPropertyItr* prop_itr_;
};

dbOStream& operator<<(dbOStream& stream, const _dbTech& tech);
dbIStream& operator>>(dbIStream& stream, _dbTech& tech);

}  // namespace odb
