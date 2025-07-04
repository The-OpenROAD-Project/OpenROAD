// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <utility>
#include <vector>

#include "dbCore.h"
#include "dbHashTable.h"
#include "dbVector.h"
#include "odb/dbMatrix.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

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
  uint num_masks_ : 2;
  dbTechLayerType::Value type_ : 4;
  dbTechLayerDir::Value direction_ : 4;
  dbTechLayerMinStepType::Value minstep_type_ : 2;
  bool has_max_width_ : 1;
  bool has_thickness_ : 1;
  bool has_area_ : 1;
  bool has_protrusion_ : 1;
  bool has_alias_ : 1;
  bool has_xy_pitch_ : 1;
  bool has_xy_offset_ : 1;
  bool rect_only_ : 1;
  bool right_way_on_grid_only_ : 1;
  bool right_way_on_grid_only_check_mask_ : 1;
  bool rect_only_except_non_core_pins_ : 1;
  uint lef58_type_ : 5;
  uint spare_bits_ : 4;
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
  uint getV55RowIdx(const int& rowVal) const;
  uint getV55ColIdx(const int& colVal) const;
  uint getTwIdx(int width, int prl) const;
  // User Code End Methods

  dbTechLayerFlags flags_;
  uint wrong_way_width_;
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

  // User Code Begin Fields

  uint _pitch_x;
  uint _pitch_y;
  uint _offset_x;
  uint _offset_y;
  uint _width;
  uint _spacing;
  double _resistance;
  double _capacitance;
  double _edge_capacitance;
  uint _wire_extension;
  uint _number;
  uint _rlevel;
  double _area;
  uint _thickness;
  uint _max_width;
  int _min_width;
  int _min_step;
  int _min_step_max_length;
  int _min_step_max_edges;
  int _first_last_pitch;

  struct
  {  // Protrusion
    uint _width;
    uint _length;
    uint _from_width;
  } _pt;
  char* _name;
  char* _alias;
  dbId<_dbTechLayer> _upper;
  dbId<_dbTechLayer> _lower;
  dbTable<_dbTechLayerSpacingRule>* _spacing_rules_tbl;

  dbTable<_dbTechMinCutRule, 8>* _min_cut_rules_tbl;
  dbTable<_dbTechMinEncRule, 8>* _min_enc_rules_tbl;
  dbTable<_dbTechV55InfluenceEntry, 8>* _v55inf_tbl;
  dbVector<uint> _v55sp_length_idx;
  dbVector<uint> _v55sp_width_idx;
  dbMatrix<uint> _v55sp_spacing;

  dbVector<uint> _two_widths_sp_idx;
  dbVector<int> _two_widths_sp_prl;
  dbMatrix<uint> _two_widths_sp_spacing;

  dbId<_dbTechLayerAntennaRule> _oxide1;
  dbId<_dbTechLayerAntennaRule> _oxide2;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayer& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayer& obj);
}  // namespace odb
   // Generator Code End Header
