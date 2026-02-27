// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "dbCore.h"
#include "dbHashTable.h"
#include "dbVector.h"
#include "odb/dbMatrix.h"
#include "odb/dbTypes.h"
// User Code Begin Includes
#include "odb/dbId.h"
#include "odb/dbObject.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbTechLayerCutClassRule;
class _dbTechLayerSpacingEolRule;
class _dbTechLayerCutSpacingRule;
class _dbTechLayerMinStepRule;
class _dbTechLayerCornerSpacingRule;
class _dbTechLayerSpacingTablePrlRule;
class _dbTechLayerCutSpacingTableOrthRule;
class _dbTechLayerCutSpacingTableDefRule;
class _dbTechLayerCutEnclosureRule;
class _dbTechLayerEolExtensionRule;
class _dbTechLayerArraySpacingRule;
class _dbTechLayerEolKeepOutRule;
class _dbTechLayerMaxSpacingRule;
class _dbTechLayerWidthTableRule;
class _dbTechLayerMinCutRule;
class _dbTechLayerAreaRule;
class _dbTechLayerForbiddenSpacingRule;
class _dbTechLayerKeepOutZoneRule;
class _dbTechLayerWrongDirSpacingRule;
class _dbTechLayerTwoWiresForbiddenSpcRule;
class _dbTechLayerVoltageSpacing;
// User Code Begin Classes
class _dbTechLayerSpacingRule;
class _dbTechMinCutRule;
class _dbTechMinEncRule;
class _dbTechV55InfluenceEntry;
class _dbTechLayerAntennaRule;
class dbTrackGrid;
// User Code End Classes

struct dbTechLayerFlags
{
  uint32_t num_masks : 2;
  dbTechLayerType::Value type : 4;
  dbTechLayerDir::Value direction : 4;
  dbTechLayerMinStepType::Value minstep_type : 2;
  bool has_max_width : 1;
  bool has_thickness : 1;
  bool has_area : 1;
  bool has_protrusion : 1;
  bool has_alias : 1;
  bool has_xy_pitch : 1;
  bool has_xy_offset : 1;
  bool rect_only : 1;
  bool right_way_on_grid_only : 1;
  bool right_way_on_grid_only_check_mask : 1;
  bool rect_only_except_non_core_pins : 1;
  uint32_t lef58_type : 5;
  uint32_t spare_bits : 4;
};

class _dbTechLayer : public _dbObject
{
 public:
  _dbTechLayer(_dbDatabase*);

  ~_dbTechLayer();

  bool operator==(const _dbTechLayer& rhs) const;
  bool operator!=(const _dbTechLayer& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbTechLayer& rhs) const;
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods
  uint32_t getV55RowIdx(const int& rowVal) const;
  uint32_t getV55ColIdx(const int& colVal) const;
  uint32_t getTwIdx(int width, int prl) const;
  // User Code End Methods

  dbTechLayerFlags flags_;
  uint32_t wrong_way_width_;
  uint32_t wrong_way_min_width_;
  float layer_adjustment_;
  std::vector<std::pair<int, int>> orth_spacing_tbl_;
  dbTable<_dbTechLayerCutClassRule>* cut_class_rules_tbl_;
  dbHashTable<_dbTechLayerCutClassRule> cut_class_rules_hash_;
  dbTable<_dbTechLayerSpacingEolRule>* spacing_eol_rules_tbl_;
  dbTable<_dbTechLayerCutSpacingRule>* cut_spacing_rules_tbl_;
  dbTable<_dbTechLayerMinStepRule>* minstep_rules_tbl_;
  dbTable<_dbTechLayerCornerSpacingRule>* corner_spacing_rules_tbl_;
  dbTable<_dbTechLayerSpacingTablePrlRule>* spacing_table_prl_rules_tbl_;
  dbTable<_dbTechLayerCutSpacingTableOrthRule>* cut_spacing_table_orth_tbl_;
  dbTable<_dbTechLayerCutSpacingTableDefRule>* cut_spacing_table_def_tbl_;
  dbTable<_dbTechLayerCutEnclosureRule>* cut_enc_rules_tbl_;
  dbTable<_dbTechLayerEolExtensionRule>* eol_ext_rules_tbl_;
  dbTable<_dbTechLayerArraySpacingRule>* array_spacing_rules_tbl_;
  dbTable<_dbTechLayerEolKeepOutRule>* eol_keep_out_rules_tbl_;
  dbTable<_dbTechLayerMaxSpacingRule>* max_spacing_rules_tbl_;
  dbTable<_dbTechLayerWidthTableRule>* width_table_rules_tbl_;
  dbTable<_dbTechLayerMinCutRule>* min_cuts_rules_tbl_;
  dbTable<_dbTechLayerAreaRule>* area_rules_tbl_;
  dbTable<_dbTechLayerForbiddenSpacingRule>* forbidden_spacing_rules_tbl_;
  dbTable<_dbTechLayerKeepOutZoneRule>* keepout_zone_rules_tbl_;
  dbTable<_dbTechLayerWrongDirSpacingRule>* wrongdir_spacing_rules_tbl_;
  dbTable<_dbTechLayerTwoWiresForbiddenSpcRule>*
      two_wires_forbidden_spc_rules_tbl_;
  dbTable<_dbTechLayerVoltageSpacing>* voltage_spacing_rules_tbl_;

  // User Code Begin Fields

  uint32_t pitch_x_;
  uint32_t pitch_y_;
  uint32_t offset_x_;
  uint32_t offset_y_;
  uint32_t width_;
  uint32_t spacing_;
  double resistance_;
  double capacitance_;
  double edge_capacitance_;
  uint32_t wire_extension_;
  uint32_t number_;
  uint32_t rlevel_;
  double area_;
  uint32_t thickness_;
  uint32_t max_width_;
  int min_width_;
  int min_step_;
  int min_step_max_length_;
  int min_step_max_edges_;
  int first_last_pitch_;

  struct
  {  // Protrusion
    uint32_t width;
    uint32_t length;
    uint32_t from_width;
  } pt_;
  char* name_;
  char* alias_;
  dbId<_dbTechLayer> upper_;
  dbId<_dbTechLayer> lower_;
  dbTable<_dbTechLayerSpacingRule>* spacing_rules_tbl_;

  dbTable<_dbTechMinCutRule, 8>* min_cut_rules_tbl_;
  dbTable<_dbTechMinEncRule, 8>* min_enc_rules_tbl_;
  dbTable<_dbTechV55InfluenceEntry, 8>* v55inf_tbl_;
  dbVector<uint32_t> v55sp_length_idx_;
  dbVector<uint32_t> v55sp_width_idx_;
  dbMatrix<uint32_t> v55sp_spacing_;

  dbVector<uint32_t> two_widths_sp_idx_;
  dbVector<int> two_widths_sp_prl_;
  dbMatrix<uint32_t> two_widths_sp_spacing_;

  dbId<_dbTechLayerAntennaRule> oxide1_;
  dbId<_dbTechLayerAntennaRule> oxide2_;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayer& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayer& obj);
}  // namespace odb
   // Generator Code End Header
