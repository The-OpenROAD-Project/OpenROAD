// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayer.h"

#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayerAreaRule.h"
#include "dbTechLayerArraySpacingRule.h"
#include "dbTechLayerCornerSpacingRule.h"
#include "dbTechLayerCutClassRule.h"
#include "dbTechLayerCutEnclosureRule.h"
#include "dbTechLayerCutSpacingRule.h"
#include "dbTechLayerCutSpacingTableDefRule.h"
#include "dbTechLayerCutSpacingTableOrthRule.h"
#include "dbTechLayerEolExtensionRule.h"
#include "dbTechLayerEolKeepOutRule.h"
#include "dbTechLayerForbiddenSpacingRule.h"
#include "dbTechLayerKeepOutZoneRule.h"
#include "dbTechLayerMaxSpacingRule.h"
#include "dbTechLayerMinCutRule.h"
#include "dbTechLayerMinStepRule.h"
#include "dbTechLayerSpacingEolRule.h"
#include "dbTechLayerSpacingTablePrlRule.h"
#include "dbTechLayerTwoWiresForbiddenSpcRule.h"
#include "dbTechLayerVoltageSpacing.h"
#include "dbTechLayerWidthTableRule.h"
#include "dbTechLayerWrongDirSpacingRule.h"
#include "odb/db.h"
#include "odb/dbSet.h"
// User Code Begin Includes
#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <ranges>
#include <string>

#include "dbCommon.h"
#include "dbHashTable.hpp"
#include "dbTech.h"
#include "dbTechLayerAntennaRule.h"
#include "dbTechLayerSpacingRule.h"
#include "dbTechMinCutOrAreaRule.h"
#include "dbVector.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"
#include "odb/lefout.h"
#include "spdlog/fmt/ostr.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbTechLayer>;

bool _dbTechLayer::operator==(const _dbTechLayer& rhs) const
{
  if (flags_.num_masks != rhs.flags_.num_masks) {
    return false;
  }
  if (flags_.has_max_width != rhs.flags_.has_max_width) {
    return false;
  }
  if (flags_.has_thickness != rhs.flags_.has_thickness) {
    return false;
  }
  if (flags_.has_area != rhs.flags_.has_area) {
    return false;
  }
  if (flags_.has_protrusion != rhs.flags_.has_protrusion) {
    return false;
  }
  if (flags_.has_alias != rhs.flags_.has_alias) {
    return false;
  }
  if (flags_.has_xy_pitch != rhs.flags_.has_xy_pitch) {
    return false;
  }
  if (flags_.has_xy_offset != rhs.flags_.has_xy_offset) {
    return false;
  }
  if (flags_.rect_only != rhs.flags_.rect_only) {
    return false;
  }
  if (flags_.right_way_on_grid_only != rhs.flags_.right_way_on_grid_only) {
    return false;
  }
  if (flags_.right_way_on_grid_only_check_mask
      != rhs.flags_.right_way_on_grid_only_check_mask) {
    return false;
  }
  if (flags_.rect_only_except_non_core_pins
      != rhs.flags_.rect_only_except_non_core_pins) {
    return false;
  }
  if (flags_.lef58_type != rhs.flags_.lef58_type) {
    return false;
  }
  if (wrong_way_width_ != rhs.wrong_way_width_) {
    return false;
  }
  if (wrong_way_min_width_ != rhs.wrong_way_min_width_) {
    return false;
  }
  if (layer_adjustment_ != rhs.layer_adjustment_) {
    return false;
  }
  if (*cut_class_rules_tbl_ != *rhs.cut_class_rules_tbl_) {
    return false;
  }
  if (cut_class_rules_hash_ != rhs.cut_class_rules_hash_) {
    return false;
  }
  if (*spacing_eol_rules_tbl_ != *rhs.spacing_eol_rules_tbl_) {
    return false;
  }
  if (*cut_spacing_rules_tbl_ != *rhs.cut_spacing_rules_tbl_) {
    return false;
  }
  if (*minstep_rules_tbl_ != *rhs.minstep_rules_tbl_) {
    return false;
  }
  if (*corner_spacing_rules_tbl_ != *rhs.corner_spacing_rules_tbl_) {
    return false;
  }
  if (*spacing_table_prl_rules_tbl_ != *rhs.spacing_table_prl_rules_tbl_) {
    return false;
  }
  if (*cut_spacing_table_orth_tbl_ != *rhs.cut_spacing_table_orth_tbl_) {
    return false;
  }
  if (*cut_spacing_table_def_tbl_ != *rhs.cut_spacing_table_def_tbl_) {
    return false;
  }
  if (*cut_enc_rules_tbl_ != *rhs.cut_enc_rules_tbl_) {
    return false;
  }
  if (*eol_ext_rules_tbl_ != *rhs.eol_ext_rules_tbl_) {
    return false;
  }
  if (*array_spacing_rules_tbl_ != *rhs.array_spacing_rules_tbl_) {
    return false;
  }
  if (*eol_keep_out_rules_tbl_ != *rhs.eol_keep_out_rules_tbl_) {
    return false;
  }
  if (*max_spacing_rules_tbl_ != *rhs.max_spacing_rules_tbl_) {
    return false;
  }
  if (*width_table_rules_tbl_ != *rhs.width_table_rules_tbl_) {
    return false;
  }
  if (*min_cuts_rules_tbl_ != *rhs.min_cuts_rules_tbl_) {
    return false;
  }
  if (*area_rules_tbl_ != *rhs.area_rules_tbl_) {
    return false;
  }
  if (*forbidden_spacing_rules_tbl_ != *rhs.forbidden_spacing_rules_tbl_) {
    return false;
  }
  if (*keepout_zone_rules_tbl_ != *rhs.keepout_zone_rules_tbl_) {
    return false;
  }
  if (*wrongdir_spacing_rules_tbl_ != *rhs.wrongdir_spacing_rules_tbl_) {
    return false;
  }
  if (*two_wires_forbidden_spc_rules_tbl_
      != *rhs.two_wires_forbidden_spc_rules_tbl_) {
    return false;
  }
  if (*voltage_spacing_rules_tbl_ != *rhs.voltage_spacing_rules_tbl_) {
    return false;
  }

  // User Code Begin ==
  if (flags_.type != rhs.flags_.type) {
    return false;
  }

  if (flags_.direction != rhs.flags_.direction) {
    return false;
  }

  if (flags_.minstep_type != rhs.flags_.minstep_type) {
    return false;
  }

  if (pitch_x_ != rhs.pitch_x_) {
    return false;
  }

  if (pitch_y_ != rhs.pitch_y_) {
    return false;
  }

  if (offset_x_ != rhs.offset_x_) {
    return false;
  }

  if (offset_y_ != rhs.offset_y_) {
    return false;
  }

  if (width_ != rhs.width_) {
    return false;
  }

  if (spacing_ != rhs.spacing_) {
    return false;
  }

  if (resistance_ != rhs.resistance_) {
    return false;
  }

  if (capacitance_ != rhs.capacitance_) {
    return false;
  }

  if (edge_capacitance_ != rhs.edge_capacitance_) {
    return false;
  }

  if (wire_extension_ != rhs.wire_extension_) {
    return false;
  }

  if (number_ != rhs.number_) {
    return false;
  }

  if (rlevel_ != rhs.rlevel_) {
    return false;
  }

  if (area_ != rhs.area_) {
    return false;
  }

  if (thickness_ != rhs.thickness_) {
    return false;
  }

  if (min_step_ != rhs.min_step_) {
    return false;
  }

  if (max_width_ != rhs.max_width_) {
    return false;
  }

  if (min_width_ != rhs.min_width_) {
    return false;
  }

  if (min_step_max_length_ != rhs.min_step_max_length_) {
    return false;
  }

  if (min_step_max_edges_ != rhs.min_step_max_edges_) {
    return false;
  }

  if (first_last_pitch_ != rhs.first_last_pitch_) {
    return false;
  }

  if (pt_.width != rhs.pt_.width) {
    return false;
  }

  if (pt_.length != rhs.pt_.length) {
    return false;
  }

  if (pt_.from_width != rhs.pt_.from_width) {
    return false;
  }

  if (name_ && rhs.name_) {
    if (strcmp(name_, rhs.name_) != 0) {
      return false;
    }
  } else if (name_ || rhs.name_) {
    return false;
  }

  if (alias_ && rhs.alias_) {
    if (strcmp(alias_, rhs.alias_) != 0) {
      return false;
    }
  } else if (alias_ || rhs.alias_) {
    return false;
  }

  if (upper_ != rhs.upper_) {
    return false;
  }

  if (lower_ != rhs.lower_) {
    return false;
  }

  if (*spacing_rules_tbl_ != *rhs.spacing_rules_tbl_) {
    return false;
  }

  if (*min_cut_rules_tbl_ != *rhs.min_cut_rules_tbl_) {
    return false;
  }

  if (*min_enc_rules_tbl_ != *rhs.min_enc_rules_tbl_) {
    return false;
  }

  if (*v55inf_tbl_ != *rhs.v55inf_tbl_) {
    return false;
  }

  if (v55sp_length_idx_ != rhs.v55sp_length_idx_) {
    return false;
  }

  if (v55sp_width_idx_ != rhs.v55sp_width_idx_) {
    return false;
  }

  if (v55sp_spacing_ != rhs.v55sp_spacing_) {
    return false;
  }

  if (two_widths_sp_idx_ != rhs.two_widths_sp_idx_) {
    return false;
  }

  if (two_widths_sp_prl_ != rhs.two_widths_sp_prl_) {
    return false;
  }

  if (two_widths_sp_spacing_ != rhs.two_widths_sp_spacing_) {
    return false;
  }

  if (oxide1_ != rhs.oxide1_) {
    return false;
  }

  if (oxide2_ != rhs.oxide2_) {
    return false;
  }

  // User Code End ==
  return true;
}

bool _dbTechLayer::operator<(const _dbTechLayer& rhs) const
{
  // User Code Begin <
  if (number_ >= rhs.number_) {
    return false;
  }
  // User Code End <
  return true;
}

_dbTechLayer::_dbTechLayer(_dbDatabase* db)
{
  flags_ = {};
  wrong_way_width_ = 0;
  wrong_way_min_width_ = 0;
  layer_adjustment_ = 0;
  cut_class_rules_tbl_ = new dbTable<_dbTechLayerCutClassRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerCutClassRuleObj);
  cut_class_rules_hash_.setTable(cut_class_rules_tbl_);
  spacing_eol_rules_tbl_ = new dbTable<_dbTechLayerSpacingEolRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerSpacingEolRuleObj);
  cut_spacing_rules_tbl_ = new dbTable<_dbTechLayerCutSpacingRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerCutSpacingRuleObj);
  minstep_rules_tbl_ = new dbTable<_dbTechLayerMinStepRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerMinStepRuleObj);
  corner_spacing_rules_tbl_ = new dbTable<_dbTechLayerCornerSpacingRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerCornerSpacingRuleObj);
  spacing_table_prl_rules_tbl_ = new dbTable<_dbTechLayerSpacingTablePrlRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerSpacingTablePrlRuleObj);
  cut_spacing_table_orth_tbl_
      = new dbTable<_dbTechLayerCutSpacingTableOrthRule>(
          db,
          this,
          (GetObjTbl_t) &_dbTechLayer::getObjectTable,
          dbTechLayerCutSpacingTableOrthRuleObj);
  cut_spacing_table_def_tbl_ = new dbTable<_dbTechLayerCutSpacingTableDefRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerCutSpacingTableDefRuleObj);
  cut_enc_rules_tbl_ = new dbTable<_dbTechLayerCutEnclosureRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerCutEnclosureRuleObj);
  eol_ext_rules_tbl_ = new dbTable<_dbTechLayerEolExtensionRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerEolExtensionRuleObj);
  array_spacing_rules_tbl_ = new dbTable<_dbTechLayerArraySpacingRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerArraySpacingRuleObj);
  eol_keep_out_rules_tbl_ = new dbTable<_dbTechLayerEolKeepOutRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerEolKeepOutRuleObj);
  max_spacing_rules_tbl_ = new dbTable<_dbTechLayerMaxSpacingRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerMaxSpacingRuleObj);
  width_table_rules_tbl_ = new dbTable<_dbTechLayerWidthTableRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerWidthTableRuleObj);
  min_cuts_rules_tbl_ = new dbTable<_dbTechLayerMinCutRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerMinCutRuleObj);
  area_rules_tbl_ = new dbTable<_dbTechLayerAreaRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerAreaRuleObj);
  forbidden_spacing_rules_tbl_ = new dbTable<_dbTechLayerForbiddenSpacingRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerForbiddenSpacingRuleObj);
  keepout_zone_rules_tbl_ = new dbTable<_dbTechLayerKeepOutZoneRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerKeepOutZoneRuleObj);
  wrongdir_spacing_rules_tbl_ = new dbTable<_dbTechLayerWrongDirSpacingRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerWrongDirSpacingRuleObj);
  two_wires_forbidden_spc_rules_tbl_
      = new dbTable<_dbTechLayerTwoWiresForbiddenSpcRule>(
          db,
          this,
          (GetObjTbl_t) &_dbTechLayer::getObjectTable,
          dbTechLayerTwoWiresForbiddenSpcRuleObj);
  voltage_spacing_rules_tbl_ = new dbTable<_dbTechLayerVoltageSpacing>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerVoltageSpacingObj);
  // User Code Begin Constructor
  flags_.type = dbTechLayerType::ROUTING;
  flags_.direction = dbTechLayerDir::NONE;
  flags_.minstep_type = dbTechLayerMinStepType();
  flags_.num_masks = 1;
  pitch_x_ = 0;
  pitch_y_ = 0;
  offset_x_ = 0;
  offset_y_ = 0;
  width_ = 0;
  spacing_ = 0;
  resistance_ = 0.0;
  capacitance_ = 0.0;
  edge_capacitance_ = 0.0;
  wire_extension_ = 0;
  number_ = 0;
  rlevel_ = 0;
  area_ = 0.0;
  thickness_ = 0;
  min_step_ = -1;
  pt_.width = 0;
  pt_.length = 0;
  pt_.from_width = 0;
  max_width_ = MAX_INT;
  min_width_ = 0;
  min_step_max_length_ = -1;
  min_step_max_edges_ = -1;
  first_last_pitch_ = -1;
  v55sp_length_idx_.clear();
  v55sp_width_idx_.clear();
  v55sp_spacing_.clear();
  name_ = nullptr;
  alias_ = nullptr;

  spacing_rules_tbl_ = new dbTable<_dbTechLayerSpacingRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerSpacingRuleObj);

  min_cut_rules_tbl_ = new dbTable<_dbTechMinCutRule, 8>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechMinCutRuleObj);

  min_enc_rules_tbl_ = new dbTable<_dbTechMinEncRule, 8>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechMinEncRuleObj);

  v55inf_tbl_ = new dbTable<_dbTechV55InfluenceEntry, 8>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechV55InfluenceEntryObj);
  // User Code End Constructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayer& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  if (obj.getDatabase()->isSchema(kSchemaTechLayerMinWidthWrongway)) {
    stream >> obj.wrong_way_min_width_;
  }
  if (obj.getDatabase()->isSchema(kSchemaOrthSpcTbl)) {
    stream >> obj.orth_spacing_tbl_;
  }
  stream >> *obj.cut_class_rules_tbl_;
  stream >> obj.cut_class_rules_hash_;
  stream >> *obj.spacing_eol_rules_tbl_;
  stream >> *obj.cut_spacing_rules_tbl_;
  stream >> *obj.minstep_rules_tbl_;
  stream >> *obj.corner_spacing_rules_tbl_;
  stream >> *obj.spacing_table_prl_rules_tbl_;
  stream >> *obj.cut_spacing_table_orth_tbl_;
  stream >> *obj.cut_spacing_table_def_tbl_;
  stream >> *obj.cut_enc_rules_tbl_;
  stream >> *obj.eol_ext_rules_tbl_;
  stream >> *obj.array_spacing_rules_tbl_;
  stream >> *obj.eol_keep_out_rules_tbl_;
  if (obj.getDatabase()->isSchema(kSchemaMaxSpacing)) {
    stream >> *obj.max_spacing_rules_tbl_;
  }
  stream >> *obj.width_table_rules_tbl_;
  stream >> *obj.min_cuts_rules_tbl_;
  stream >> *obj.area_rules_tbl_;
  if (obj.getDatabase()->isSchema(kSchemaLef58ForbiddenSpacing)) {
    stream >> *obj.forbidden_spacing_rules_tbl_;
  }
  if (obj.getDatabase()->isSchema(kSchemaKeepoutZone)) {
    stream >> *obj.keepout_zone_rules_tbl_;
  }
  if (obj.getDatabase()->isSchema(kSchemaWrongdirSpacing)) {
    stream >> *obj.wrongdir_spacing_rules_tbl_;
  }
  if (obj.getDatabase()->isSchema(kSchemaLef58TwoWiresForbiddenSpacing)) {
    stream >> *obj.two_wires_forbidden_spc_rules_tbl_;
  }
  if (obj.getDatabase()->isSchema(kSchemaVoltageSpacingTables)) {
    stream >> *obj.voltage_spacing_rules_tbl_;
  }
  // User Code Begin >>
  if (obj.getDatabase()->isSchema(kSchemaLayerAdjustment)) {
    stream >> obj.layer_adjustment_;
  } else {
    obj.layer_adjustment_ = 0.0;
  }
  stream >> obj.pitch_x_;
  stream >> obj.pitch_y_;
  stream >> obj.offset_x_;
  stream >> obj.offset_y_;
  stream >> obj.width_;
  stream >> obj.spacing_;
  stream >> obj.resistance_;
  stream >> obj.capacitance_;
  stream >> obj.edge_capacitance_;
  stream >> obj.wire_extension_;
  stream >> obj.number_;
  stream >> obj.rlevel_;
  stream >> obj.area_;
  stream >> obj.thickness_;
  stream >> obj.min_step_;
  stream >> obj.min_step_max_length_;
  stream >> obj.min_step_max_edges_;
  stream >> obj.max_width_;
  stream >> obj.min_width_;
  stream >> obj.pt_.width;
  stream >> obj.pt_.length;
  stream >> obj.pt_.from_width;
  stream >> obj.name_;
  stream >> obj.alias_;
  stream >> obj.lower_;
  stream >> obj.upper_;
  stream >> *obj.spacing_rules_tbl_;
  stream >> *obj.min_cut_rules_tbl_;
  stream >> *obj.min_enc_rules_tbl_;
  stream >> *obj.v55inf_tbl_;
  stream >> obj.v55sp_length_idx_;
  stream >> obj.v55sp_width_idx_;
  stream >> obj.v55sp_spacing_;
  stream >> obj.two_widths_sp_idx_;
  stream >> obj.two_widths_sp_prl_;
  stream >> obj.two_widths_sp_spacing_;
  stream >> obj.oxide1_;
  stream >> obj.oxide2_;
  if (obj.getDatabase()->isSchema(kSchemaWrongwayWidth)) {
    stream >> obj.wrong_way_width_;
  } else {
    obj.wrong_way_width_ = obj.width_;
    for (auto rule : ((dbTechLayer*) &obj)->getTechLayerWidthTableRules()) {
      if (rule->isWrongDirection()) {
        obj.wrong_way_width_ = *rule->getWidthTable().begin();
        break;
      }
    }
  }
  if (obj.getDatabase()->isSchema(kSchemaLef58Pitch)) {
    stream >> obj.first_last_pitch_;
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayer& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.wrong_way_min_width_;
  stream << obj.orth_spacing_tbl_;
  stream << *obj.cut_class_rules_tbl_;
  stream << obj.cut_class_rules_hash_;
  stream << *obj.spacing_eol_rules_tbl_;
  stream << *obj.cut_spacing_rules_tbl_;
  stream << *obj.minstep_rules_tbl_;
  stream << *obj.corner_spacing_rules_tbl_;
  stream << *obj.spacing_table_prl_rules_tbl_;
  stream << *obj.cut_spacing_table_orth_tbl_;
  stream << *obj.cut_spacing_table_def_tbl_;
  stream << *obj.cut_enc_rules_tbl_;
  stream << *obj.eol_ext_rules_tbl_;
  stream << *obj.array_spacing_rules_tbl_;
  stream << *obj.eol_keep_out_rules_tbl_;
  stream << *obj.max_spacing_rules_tbl_;
  stream << *obj.width_table_rules_tbl_;
  stream << *obj.min_cuts_rules_tbl_;
  stream << *obj.area_rules_tbl_;
  stream << *obj.forbidden_spacing_rules_tbl_;
  stream << *obj.keepout_zone_rules_tbl_;
  stream << *obj.wrongdir_spacing_rules_tbl_;
  stream << *obj.two_wires_forbidden_spc_rules_tbl_;
  stream << *obj.voltage_spacing_rules_tbl_;
  // User Code Begin <<
  stream << obj.layer_adjustment_;
  stream << obj.pitch_x_;
  stream << obj.pitch_y_;
  stream << obj.offset_x_;
  stream << obj.offset_y_;
  stream << obj.width_;
  stream << obj.spacing_;
  stream << obj.resistance_;
  stream << obj.capacitance_;
  stream << obj.edge_capacitance_;
  stream << obj.wire_extension_;
  stream << obj.number_;
  stream << obj.rlevel_;
  stream << obj.area_;
  stream << obj.thickness_;
  stream << obj.min_step_;
  stream << obj.min_step_max_length_;
  stream << obj.min_step_max_edges_;
  stream << obj.max_width_;
  stream << obj.min_width_;
  stream << obj.pt_.width;
  stream << obj.pt_.length;
  stream << obj.pt_.from_width;
  stream << obj.name_;
  stream << obj.alias_;
  stream << obj.lower_;
  stream << obj.upper_;
  stream << *obj.spacing_rules_tbl_;
  stream << *obj.min_cut_rules_tbl_;
  stream << *obj.min_enc_rules_tbl_;
  stream << *obj.v55inf_tbl_;
  stream << obj.v55sp_length_idx_;
  stream << obj.v55sp_width_idx_;
  stream << obj.v55sp_spacing_;
  stream << obj.two_widths_sp_idx_;
  stream << obj.two_widths_sp_prl_;
  stream << obj.two_widths_sp_spacing_;
  stream << obj.oxide1_;
  stream << obj.oxide2_;
  stream << obj.wrong_way_width_;
  stream << obj.first_last_pitch_;
  // User Code End <<
  return stream;
}

dbObjectTable* _dbTechLayer::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbTechLayerCutClassRuleObj:
      return cut_class_rules_tbl_;
    case dbTechLayerSpacingEolRuleObj:
      return spacing_eol_rules_tbl_;
    case dbTechLayerCutSpacingRuleObj:
      return cut_spacing_rules_tbl_;
    case dbTechLayerMinStepRuleObj:
      return minstep_rules_tbl_;
    case dbTechLayerCornerSpacingRuleObj:
      return corner_spacing_rules_tbl_;
    case dbTechLayerSpacingTablePrlRuleObj:
      return spacing_table_prl_rules_tbl_;
    case dbTechLayerCutSpacingTableOrthRuleObj:
      return cut_spacing_table_orth_tbl_;
    case dbTechLayerCutSpacingTableDefRuleObj:
      return cut_spacing_table_def_tbl_;
    case dbTechLayerCutEnclosureRuleObj:
      return cut_enc_rules_tbl_;
    case dbTechLayerEolExtensionRuleObj:
      return eol_ext_rules_tbl_;
    case dbTechLayerArraySpacingRuleObj:
      return array_spacing_rules_tbl_;
    case dbTechLayerEolKeepOutRuleObj:
      return eol_keep_out_rules_tbl_;
    case dbTechLayerMaxSpacingRuleObj:
      return max_spacing_rules_tbl_;
    case dbTechLayerWidthTableRuleObj:
      return width_table_rules_tbl_;
    case dbTechLayerMinCutRuleObj:
      return min_cuts_rules_tbl_;
    case dbTechLayerAreaRuleObj:
      return area_rules_tbl_;
    case dbTechLayerForbiddenSpacingRuleObj:
      return forbidden_spacing_rules_tbl_;
    case dbTechLayerKeepOutZoneRuleObj:
      return keepout_zone_rules_tbl_;
    case dbTechLayerWrongDirSpacingRuleObj:
      return wrongdir_spacing_rules_tbl_;
    case dbTechLayerTwoWiresForbiddenSpcRuleObj:
      return two_wires_forbidden_spc_rules_tbl_;
    case dbTechLayerVoltageSpacingObj:
      return voltage_spacing_rules_tbl_;
      // User Code Begin getObjectTable
    case dbTechLayerSpacingRuleObj:
      return spacing_rules_tbl_;

    case dbTechMinCutRuleObj:
      return min_cut_rules_tbl_;

    case dbTechMinEncRuleObj:
      return min_enc_rules_tbl_;

    case dbTechV55InfluenceEntryObj:
      return v55inf_tbl_;
    // User Code End getObjectTable
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}
void _dbTechLayer::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  cut_class_rules_tbl_->collectMemInfo(info.children["cut_class_rules_tbl_"]);

  spacing_eol_rules_tbl_->collectMemInfo(
      info.children["spacing_eol_rules_tbl_"]);

  cut_spacing_rules_tbl_->collectMemInfo(
      info.children["cut_spacing_rules_tbl_"]);

  minstep_rules_tbl_->collectMemInfo(info.children["minstep_rules_tbl_"]);

  corner_spacing_rules_tbl_->collectMemInfo(
      info.children["corner_spacing_rules_tbl_"]);

  spacing_table_prl_rules_tbl_->collectMemInfo(
      info.children["spacing_table_prl_rules_tbl_"]);

  cut_spacing_table_orth_tbl_->collectMemInfo(
      info.children["cut_spacing_table_orth_tbl_"]);

  cut_spacing_table_def_tbl_->collectMemInfo(
      info.children["cut_spacing_table_def_tbl_"]);

  cut_enc_rules_tbl_->collectMemInfo(info.children["cut_enc_rules_tbl_"]);

  eol_ext_rules_tbl_->collectMemInfo(info.children["eol_ext_rules_tbl_"]);

  array_spacing_rules_tbl_->collectMemInfo(
      info.children["array_spacing_rules_tbl_"]);

  eol_keep_out_rules_tbl_->collectMemInfo(
      info.children["eol_keep_out_rules_tbl_"]);

  max_spacing_rules_tbl_->collectMemInfo(
      info.children["max_spacing_rules_tbl_"]);

  width_table_rules_tbl_->collectMemInfo(
      info.children["width_table_rules_tbl_"]);

  min_cuts_rules_tbl_->collectMemInfo(info.children["min_cuts_rules_tbl_"]);

  area_rules_tbl_->collectMemInfo(info.children["area_rules_tbl_"]);

  forbidden_spacing_rules_tbl_->collectMemInfo(
      info.children["forbidden_spacing_rules_tbl_"]);

  keepout_zone_rules_tbl_->collectMemInfo(
      info.children["keepout_zone_rules_tbl_"]);

  wrongdir_spacing_rules_tbl_->collectMemInfo(
      info.children["wrongdir_spacing_rules_tbl_"]);

  two_wires_forbidden_spc_rules_tbl_->collectMemInfo(
      info.children["two_wires_forbidden_spc_rules_tbl_"]);

  voltage_spacing_rules_tbl_->collectMemInfo(
      info.children["voltage_spacing_rules_tbl_"]);

  // User Code Begin collectMemInfo
  info.children["orth_spacing"].add(orth_spacing_tbl_);
  info.children["cut_class_rules_hash"].add(cut_class_rules_hash_);
  info.children["name"].add(name_);
  info.children["alias"].add(alias_);
  spacing_rules_tbl_->collectMemInfo(info.children["spacing_rules_tbl"]);
  min_cut_rules_tbl_->collectMemInfo(info.children["min_cut_rules_tbl"]);
  min_enc_rules_tbl_->collectMemInfo(info.children["min_enc_rules_tbl"]);
  v55inf_tbl_->collectMemInfo(info.children["v55inf_tbl"]);
  info.children["v55sp_length_idx"].add(v55sp_length_idx_);
  info.children["v55sp_width_idx"].add(v55sp_width_idx_);
  info.children["v55sp_spacing"].add(v55sp_spacing_);
  info.children["two_widths_sp_idx"].add(two_widths_sp_idx_);
  info.children["two_widths_sp_prl"].add(two_widths_sp_prl_);
  info.children["two_widths_sp_spacing"].add(two_widths_sp_spacing_);
  // User Code End collectMemInfo
}

_dbTechLayer::~_dbTechLayer()
{
  delete cut_class_rules_tbl_;
  delete spacing_eol_rules_tbl_;
  delete cut_spacing_rules_tbl_;
  delete minstep_rules_tbl_;
  delete corner_spacing_rules_tbl_;
  delete spacing_table_prl_rules_tbl_;
  delete cut_spacing_table_orth_tbl_;
  delete cut_spacing_table_def_tbl_;
  delete cut_enc_rules_tbl_;
  delete eol_ext_rules_tbl_;
  delete array_spacing_rules_tbl_;
  delete eol_keep_out_rules_tbl_;
  delete max_spacing_rules_tbl_;
  delete width_table_rules_tbl_;
  delete min_cuts_rules_tbl_;
  delete area_rules_tbl_;
  delete forbidden_spacing_rules_tbl_;
  delete keepout_zone_rules_tbl_;
  delete wrongdir_spacing_rules_tbl_;
  delete two_wires_forbidden_spc_rules_tbl_;
  delete voltage_spacing_rules_tbl_;
  // User Code Begin Destructor
  if (name_) {
    free((void*) name_);
  }

  {
    delete spacing_rules_tbl_;
  }

  {
    delete min_cut_rules_tbl_;
  }

  {
    delete min_enc_rules_tbl_;
  }

  {
    delete v55inf_tbl_;
  }
  // User Code End Destructor
}

// User Code Begin PrivateMethods
uint32_t _dbTechLayer::getV55RowIdx(const int& rowVal) const
{
  auto pos = --(std::ranges::lower_bound(v55sp_width_idx_, rowVal));
  return std::max(0, (int) std::distance(v55sp_width_idx_.begin(), pos));
}
uint32_t _dbTechLayer::getV55ColIdx(const int& colVal) const
{
  auto pos = --(std::ranges::lower_bound(v55sp_length_idx_, colVal));
  return std::max(0, (int) std::distance(v55sp_length_idx_.begin(), pos));
}
uint32_t _dbTechLayer::getTwIdx(const int width, const int prl) const
{
  auto pos = std::ranges::lower_bound(two_widths_sp_idx_, width);
  if (pos != two_widths_sp_idx_.begin()) {
    --pos;
  }
  int idx = std::max(0, (int) std::distance(two_widths_sp_idx_.begin(), pos));
  for (; idx >= 0; idx--) {
    if (prl >= two_widths_sp_prl_[idx]) {
      return idx;
    }
  }
  return 0;
}
// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbTechLayer - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayer::setWrongWayWidth(uint32_t wrong_way_width)
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  obj->wrong_way_width_ = wrong_way_width;
}

uint32_t dbTechLayer::getWrongWayWidth() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return obj->wrong_way_width_;
}

void dbTechLayer::setWrongWayMinWidth(uint32_t wrong_way_min_width)
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  obj->wrong_way_min_width_ = wrong_way_min_width;
}

uint32_t dbTechLayer::getWrongWayMinWidth() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return obj->wrong_way_min_width_;
}

void dbTechLayer::setLayerAdjustment(float layer_adjustment)
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  obj->layer_adjustment_ = layer_adjustment;
}

float dbTechLayer::getLayerAdjustment() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return obj->layer_adjustment_;
}

void dbTechLayer::getOrthSpacingTable(
    std::vector<std::pair<int, int>>& tbl) const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  tbl = obj->orth_spacing_tbl_;
}

dbSet<dbTechLayerCutClassRule> dbTechLayer::getTechLayerCutClassRules() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerCutClassRule>(obj, obj->cut_class_rules_tbl_);
}

dbTechLayerCutClassRule* dbTechLayer::findTechLayerCutClassRule(
    const char* name) const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return (dbTechLayerCutClassRule*) obj->cut_class_rules_hash_.find(name);
}

dbSet<dbTechLayerSpacingEolRule> dbTechLayer::getTechLayerSpacingEolRules()
    const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerSpacingEolRule>(obj, obj->spacing_eol_rules_tbl_);
}

dbSet<dbTechLayerCutSpacingRule> dbTechLayer::getTechLayerCutSpacingRules()
    const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerCutSpacingRule>(obj, obj->cut_spacing_rules_tbl_);
}

dbSet<dbTechLayerMinStepRule> dbTechLayer::getTechLayerMinStepRules() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerMinStepRule>(obj, obj->minstep_rules_tbl_);
}

dbSet<dbTechLayerCornerSpacingRule>
dbTechLayer::getTechLayerCornerSpacingRules() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerCornerSpacingRule>(obj,
                                             obj->corner_spacing_rules_tbl_);
}

dbSet<dbTechLayerSpacingTablePrlRule>
dbTechLayer::getTechLayerSpacingTablePrlRules() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerSpacingTablePrlRule>(
      obj, obj->spacing_table_prl_rules_tbl_);
}

dbSet<dbTechLayerCutSpacingTableOrthRule>
dbTechLayer::getTechLayerCutSpacingTableOrthRules() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerCutSpacingTableOrthRule>(
      obj, obj->cut_spacing_table_orth_tbl_);
}

dbSet<dbTechLayerCutSpacingTableDefRule>
dbTechLayer::getTechLayerCutSpacingTableDefRules() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerCutSpacingTableDefRule>(
      obj, obj->cut_spacing_table_def_tbl_);
}

dbSet<dbTechLayerCutEnclosureRule> dbTechLayer::getTechLayerCutEnclosureRules()
    const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerCutEnclosureRule>(obj, obj->cut_enc_rules_tbl_);
}

dbSet<dbTechLayerEolExtensionRule> dbTechLayer::getTechLayerEolExtensionRules()
    const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerEolExtensionRule>(obj, obj->eol_ext_rules_tbl_);
}

dbSet<dbTechLayerArraySpacingRule> dbTechLayer::getTechLayerArraySpacingRules()
    const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerArraySpacingRule>(obj, obj->array_spacing_rules_tbl_);
}

dbSet<dbTechLayerEolKeepOutRule> dbTechLayer::getTechLayerEolKeepOutRules()
    const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerEolKeepOutRule>(obj, obj->eol_keep_out_rules_tbl_);
}

dbSet<dbTechLayerMaxSpacingRule> dbTechLayer::getTechLayerMaxSpacingRules()
    const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerMaxSpacingRule>(obj, obj->max_spacing_rules_tbl_);
}

dbSet<dbTechLayerWidthTableRule> dbTechLayer::getTechLayerWidthTableRules()
    const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerWidthTableRule>(obj, obj->width_table_rules_tbl_);
}

dbSet<dbTechLayerMinCutRule> dbTechLayer::getTechLayerMinCutRules() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerMinCutRule>(obj, obj->min_cuts_rules_tbl_);
}

dbSet<dbTechLayerAreaRule> dbTechLayer::getTechLayerAreaRules() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerAreaRule>(obj, obj->area_rules_tbl_);
}

dbSet<dbTechLayerForbiddenSpacingRule>
dbTechLayer::getTechLayerForbiddenSpacingRules() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerForbiddenSpacingRule>(
      obj, obj->forbidden_spacing_rules_tbl_);
}

dbSet<dbTechLayerKeepOutZoneRule> dbTechLayer::getTechLayerKeepOutZoneRules()
    const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerKeepOutZoneRule>(obj, obj->keepout_zone_rules_tbl_);
}

dbSet<dbTechLayerWrongDirSpacingRule>
dbTechLayer::getTechLayerWrongDirSpacingRules() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerWrongDirSpacingRule>(
      obj, obj->wrongdir_spacing_rules_tbl_);
}

dbSet<dbTechLayerTwoWiresForbiddenSpcRule>
dbTechLayer::getTechLayerTwoWiresForbiddenSpcRules() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerTwoWiresForbiddenSpcRule>(
      obj, obj->two_wires_forbidden_spc_rules_tbl_);
}

dbSet<dbTechLayerVoltageSpacing> dbTechLayer::getTechLayerVoltageSpacings()
    const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerVoltageSpacing>(obj, obj->voltage_spacing_rules_tbl_);
}

void dbTechLayer::setRectOnly(bool rect_only)
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  obj->flags_.rect_only = rect_only;
}

bool dbTechLayer::isRectOnly() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  return obj->flags_.rect_only;
}

void dbTechLayer::setRightWayOnGridOnly(bool right_way_on_grid_only)
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  obj->flags_.right_way_on_grid_only = right_way_on_grid_only;
}

bool dbTechLayer::isRightWayOnGridOnly() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  return obj->flags_.right_way_on_grid_only;
}

void dbTechLayer::setRightWayOnGridOnlyCheckMask(
    bool right_way_on_grid_only_check_mask)
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  obj->flags_.right_way_on_grid_only_check_mask
      = right_way_on_grid_only_check_mask;
}

bool dbTechLayer::isRightWayOnGridOnlyCheckMask() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  return obj->flags_.right_way_on_grid_only_check_mask;
}

void dbTechLayer::setRectOnlyExceptNonCorePins(
    bool rect_only_except_non_core_pins)
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  obj->flags_.rect_only_except_non_core_pins = rect_only_except_non_core_pins;
}

bool dbTechLayer::isRectOnlyExceptNonCorePins() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  return obj->flags_.rect_only_except_non_core_pins;
}

// User Code Begin dbTechLayerPublicMethods

void dbTechLayer::setLef58Type(LEF58_TYPE type)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->flags_.lef58_type = (uint32_t) type;
  if ((type == odb::dbTechLayer::MIMCAP
       || type == odb::dbTechLayer::STACKEDMIMCAP)
      && getType() == dbTechLayerType::ROUTING) {
    _dbTech* tech = (_dbTech*) layer->getOwner();
    layer->rlevel_ = 0;
    --tech->rlayer_cnt_;
  }
}

dbTechLayer::LEF58_TYPE dbTechLayer::getLef58Type() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return (dbTechLayer::LEF58_TYPE) layer->flags_.lef58_type;
}

std::string dbTechLayer::getLef58TypeString() const
{
  switch (getLef58Type()) {
    case NONE:
      return "NONE";
    case NWELL:
      return "NWELL";
    case PWELL:
      return "PWELL";
    case ABOVEDIEEDGE:
      return "ABOVEDIEEDGE";
    case BELOWDIEEDGE:
      return "BELOWDIEEDGE";
    case DIFFUSION:
      return "DIFFUSION";
    case TRIMPOLY:
      return "TRIMPOLY";
    case MIMCAP:
      return "MIMCAP";
    case STACKEDMIMCAP:
      return "STACKEDMIMCAP";
    case TSV:
      return "TSV";
    case PASSIVATION:
      return "PASSIVATION";
    case HIGHR:
      return "HIGHR";
    case TRIMMETAL:
      return "TRIMMETAL";
    case REGION:
      return "REGION";
    case MEOL:
      return "MEOL";
    case WELLDISTANCE:
      return "WELLDISTANCE";
    case CPODE:
      return "CPODE";
    case TSVMETAL:
      return "TSVMETAL";
    case PADMETAL:
      return "PADMETAL";
    case POLYROUTING:
      return "POLYROUTING";
  }

  return "Unknown";
}

std::string dbTechLayer::getName() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->name_;
}

const char* dbTechLayer::getConstName() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->name_;
}

bool dbTechLayer::hasAlias()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->flags_.has_alias == 1;
}

std::string dbTechLayer::getAlias()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  if (layer->alias_ == nullptr) {
    return "";
  }

  return layer->alias_;
}

void dbTechLayer::setAlias(const char* alias)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  if (layer->alias_) {
    free((void*) layer->alias_);
  }

  layer->flags_.has_alias = true;
  layer->alias_ = safe_strdup(alias);
}

uint32_t dbTechLayer::getWidth() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->width_;
}

void dbTechLayer::setWidth(int width)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->width_ = width;
  if (layer->wrong_way_width_ == 0) {
    layer->wrong_way_width_ = width;
  }
}

int dbTechLayer::getSpacing()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->spacing_;
}

void dbTechLayer::setSpacing(int spacing)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->spacing_ = spacing;
}

double dbTechLayer::getEdgeCapacitance()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->edge_capacitance_;
}

void dbTechLayer::setEdgeCapacitance(double cap)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->edge_capacitance_ = cap;
}

uint32_t dbTechLayer::getWireExtension()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->wire_extension_;
}

void dbTechLayer::setWireExtension(uint32_t ext)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->wire_extension_ = ext;
}

int dbTechLayer::getSpacing(int w, int l)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  bool found_spacing = false;
  uint32_t spacing = MAX_INT;

  bool found_over_spacing = false;
  uint32_t over_spacing = MAX_INT;
  uint32_t width = (uint32_t) w;
  uint32_t length = (uint32_t) l;

  for (auto cur_rule : getV54SpacingRules()) {
    uint32_t rmin, rmax;
    if (cur_rule->getRange(rmin, rmax)) {
      if ((width >= rmin) && (width <= rmax)) {
        spacing = std::min(spacing, cur_rule->getSpacing());
        found_spacing = true;
      }
      if (width > rmax) {
        found_over_spacing = true;
        over_spacing = std::min(over_spacing, cur_rule->getSpacing());
      }
    }
  }

  std::vector<std::vector<uint32_t>> v55rules;
  uint32_t i, j;
  if (getV55SpacingTable(v55rules)) {
    for (i = 1; (i < layer->v55sp_width_idx_.size())
                && (width > layer->v55sp_width_idx_[i]);
         i++) {
      ;
    }
    for (j = 1; (j < layer->v55sp_length_idx_.size())
                && (length > layer->v55sp_length_idx_[j]);
         j++) {
      ;
    }
    found_spacing = true;
    spacing = v55rules[i - 1][j - 1];
  }

  if ((!found_spacing) && (found_over_spacing)) {
    found_spacing = true;
    spacing = over_spacing;
  }

  return (found_spacing) ? spacing : layer->spacing_;
}

//
// Get the low end of the uppermost range for wide wire design rules.
//
void dbTechLayer::getMaxWideDRCRange(int& owidth, int& olength)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  owidth = getWidth();
  olength = owidth;

  for (auto rule : getV54SpacingRules()) {
    uint32_t rmin, rmax;
    if (rule->getRange(rmin, rmax)) {
      if (rmin > (uint32_t) owidth) {
        owidth = rmin;
        olength = rmin;
      }
    }
  }

  if (hasV55SpacingRules()) {
    owidth = layer->v55sp_width_idx_[layer->v55sp_width_idx_.size() - 1];
    olength = layer->v55sp_length_idx_[layer->v55sp_length_idx_.size() - 1];
  }
}

//
// Get the low end of the lowermost range for wide wire design rules.
//
void dbTechLayer::getMinWideDRCRange(int& owidth, int& olength)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  owidth = getWidth();
  olength = owidth;

  for (auto rule : getV54SpacingRules()) {
    uint32_t rmin, rmax;
    if (rule->getRange(rmin, rmax)) {
      if (rmin < (uint32_t) owidth) {
        owidth = rmin;
        olength = rmin;
      }
    }
  }

  if (hasV55SpacingRules()) {
    owidth = layer->v55sp_width_idx_[1];
    olength = layer->v55sp_length_idx_[1];
  }
}

dbSet<dbTechLayerSpacingRule> dbTechLayer::getV54SpacingRules() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return dbSet<dbTechLayerSpacingRule>(layer, layer->spacing_rules_tbl_);
}

bool dbTechLayer::hasV55SpacingRules() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return ((!layer->v55sp_length_idx_.empty())
          && (!layer->v55sp_width_idx_.empty())
          && (layer->v55sp_spacing_.numElems() > 0));
}

bool dbTechLayer::getV55SpacingWidthsAndLengths(
    std::vector<uint32_t>& width_idx,
    std::vector<uint32_t>& length_idx) const
{
  if (!hasV55SpacingRules()) {
    return false;
  }
  _dbTechLayer* layer = (_dbTechLayer*) this;
  width_idx = layer->v55sp_width_idx_;
  length_idx = layer->v55sp_length_idx_;
  return true;
}

void dbTechLayer::printV55SpacingRules(lefout& writer) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  fmt::print(writer.out(), "SPACINGTABLE\n");
  fmt::print(writer.out(), "  PARALLELRUNLENGTH");
  dbVector<uint32_t>::const_iterator v55_itr;
  uint32_t wddx, lndx;

  for (v55_itr = layer->v55sp_length_idx_.begin();
       v55_itr != layer->v55sp_length_idx_.end();
       v55_itr++) {
    fmt::print(writer.out(), " {:.3f}", writer.lefdist(*v55_itr));
  }

  for (wddx = 0, v55_itr = layer->v55sp_width_idx_.begin();
       v55_itr != layer->v55sp_width_idx_.end();
       wddx++, v55_itr++) {
    fmt::print(writer.out(), "\n");
    fmt::print(writer.out(), "  WIDTH {:.3f}\t", writer.lefdist(*v55_itr));
    for (lndx = 0; lndx < layer->v55sp_spacing_.numCols(); lndx++) {
      fmt::print(writer.out(),
                 " {:.3f}",
                 writer.lefdist(layer->v55sp_spacing_(wddx, lndx)));
    }
  }

  fmt::print(writer.out(), " ;\n");
}

bool dbTechLayer::getV55SpacingTable(
    std::vector<std::vector<uint32_t>>& sptbl) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  if (layer->v55sp_spacing_.numElems() == 0) {
    return false;
  }

  uint32_t i, j;
  sptbl.clear();
  sptbl.resize(layer->v55sp_spacing_.numRows());
  std::vector<uint32_t> tmpvec;
  tmpvec.reserve(layer->v55sp_spacing_.numCols());
  for (i = 0; i < layer->v55sp_spacing_.numRows(); i++) {
    tmpvec.clear();
    for (j = 0; j < layer->v55sp_spacing_.numCols(); j++) {
      tmpvec.push_back(layer->v55sp_spacing_(i, j));
    }
    sptbl[i] = tmpvec;
  }

  return true;
}

int dbTechLayer::findV55Spacing(const int width, const int prl) const
{
  if (!hasV55SpacingRules()) {
    return 0;
  }
  _dbTechLayer* layer = (_dbTechLayer*) this;
  uint32_t rowIdx = layer->getV55RowIdx(width);
  uint32_t colIdx = layer->getV55ColIdx(prl);
  return layer->v55sp_spacing_(rowIdx, colIdx);
}

void dbTechLayer::initV55LengthIndex(uint32_t numelems)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->v55sp_length_idx_.reserve(numelems);
}

void dbTechLayer::addV55LengthEntry(uint32_t length)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->v55sp_length_idx_.push_back(length);
}

void dbTechLayer::initV55WidthIndex(uint32_t numelems)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->v55sp_width_idx_.reserve(numelems);
}

void dbTechLayer::addV55WidthEntry(uint32_t width)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->v55sp_width_idx_.push_back(width);
}

void dbTechLayer::initV55SpacingTable(uint32_t numrows, uint32_t numcols)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->v55sp_spacing_.resize(numrows, numcols);
}

void dbTechLayer::addV55SpacingTableEntry(uint32_t inrow,
                                          uint32_t incol,
                                          uint32_t spacing)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->v55sp_spacing_(inrow, incol) = spacing;
}

bool dbTechLayer::hasTwoWidthsSpacingRules() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return ((!layer->two_widths_sp_idx_.empty())
          && (layer->two_widths_sp_spacing_.numElems() > 0));
}

void dbTechLayer::printTwoWidthsSpacingRules(lefout& writer) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  fmt::print(writer.out(), "SPACINGTABLE TWOWIDTHS");
  dbVector<uint32_t>::const_iterator itr;
  uint32_t wddx, lndx;

  for (wddx = 0, itr = layer->two_widths_sp_idx_.begin();
       itr != layer->two_widths_sp_idx_.end();
       wddx++, itr++) {
    fmt::print(writer.out(), "\n  WIDTH {:.3f}\t", writer.lefdist(*itr));
    for (lndx = 0; lndx < layer->two_widths_sp_spacing_.numCols(); lndx++) {
      fmt::print(writer.out(),
                 " {:.3f}",
                 writer.lefdist(layer->two_widths_sp_spacing_(wddx, lndx)));
    }
  }

  fmt::print(writer.out(), " ;\n");
}

uint32_t dbTechLayer::getTwoWidthsSpacingTableEntry(uint32_t row,
                                                    uint32_t col) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->two_widths_sp_spacing_(row, col);
}

uint32_t dbTechLayer::getTwoWidthsSpacingTableNumWidths() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->two_widths_sp_idx_.size();
}

uint32_t dbTechLayer::getTwoWidthsSpacingTableWidth(uint32_t row) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->two_widths_sp_idx_.at(row);
}

bool dbTechLayer::getTwoWidthsSpacingTableHasPRL(uint32_t row) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->two_widths_sp_prl_.at(row) >= 0;
}

uint32_t dbTechLayer::getTwoWidthsSpacingTablePRL(uint32_t row) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->two_widths_sp_prl_.at(row);
}

bool dbTechLayer::getTwoWidthsSpacingTable(
    std::vector<std::vector<uint32_t>>& sptbl) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  if (layer->two_widths_sp_spacing_.numElems() == 0) {
    return false;
  }

  uint32_t i, j;
  sptbl.clear();
  sptbl.resize(layer->two_widths_sp_spacing_.numRows());
  std::vector<uint32_t> tmpvec;
  tmpvec.reserve(layer->two_widths_sp_spacing_.numCols());
  for (i = 0; i < layer->two_widths_sp_spacing_.numRows(); i++) {
    tmpvec.clear();
    for (j = 0; j < layer->two_widths_sp_spacing_.numCols(); j++) {
      tmpvec.push_back(layer->two_widths_sp_spacing_(i, j));
    }
    sptbl[i] = tmpvec;
  }

  return true;
}

void dbTechLayer::initTwoWidths(uint32_t num_widths)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->two_widths_sp_idx_.reserve(num_widths);
  layer->two_widths_sp_spacing_.resize(num_widths, num_widths);
}

void dbTechLayer::addTwoWidthsIndexEntry(uint32_t width,
                                         int parallel_run_length)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->two_widths_sp_idx_.push_back(width);
  layer->two_widths_sp_prl_.push_back(parallel_run_length);
}

void dbTechLayer::addTwoWidthsSpacingTableEntry(uint32_t inrow,
                                                uint32_t incol,
                                                uint32_t spacing)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->two_widths_sp_spacing_(inrow, incol) = spacing;
}

int dbTechLayer::findTwSpacing(const int width1,
                               const int width2,
                               const int prl) const
{
  if (!hasTwoWidthsSpacingRules()) {
    return 0;
  }
  auto reqPrl = std::max(0, prl);
  _dbTechLayer* layer = (_dbTechLayer*) this;
  auto rowIdx = layer->getTwIdx(width1, reqPrl);
  auto colIdx = layer->getTwIdx(width2, reqPrl);
  return layer->two_widths_sp_spacing_(rowIdx, colIdx);
}

bool dbTechLayer::getMinimumCutRules(std::vector<dbTechMinCutRule*>& cut_rules)
{
  cut_rules.clear();

  for (dbTechMinCutRule* rule : getMinCutRules()) {
    cut_rules.push_back(rule);
  }

  return !cut_rules.empty();
}

dbSet<dbTechMinCutRule> dbTechLayer::getMinCutRules()
{
  dbSet<dbTechMinCutRule> rules;
  _dbTechLayer* layer = (_dbTechLayer*) this;
  rules = dbSet<dbTechMinCutRule>(layer, layer->min_cut_rules_tbl_);
  return rules;
}

dbSet<dbTechMinEncRule> dbTechLayer::getMinEncRules()
{
  dbSet<dbTechMinEncRule> rules;
  _dbTechLayer* layer = (_dbTechLayer*) this;
  rules = dbSet<dbTechMinEncRule>(layer, layer->min_enc_rules_tbl_);
  return rules;
}

dbSet<dbTechV55InfluenceEntry> dbTechLayer::getV55InfluenceRules()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return dbSet<dbTechV55InfluenceEntry>(layer, layer->v55inf_tbl_);
}

bool dbTechLayer::getMinEnclosureRules(
    std::vector<dbTechMinEncRule*>& enc_rules)
{
  enc_rules.clear();

  for (dbTechMinEncRule* rule : getMinEncRules()) {
    enc_rules.push_back(rule);
  }

  return !enc_rules.empty();
}

dbTechLayerAntennaRule* dbTechLayer::createDefaultAntennaRule()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  _dbTechLayerAntennaRule* r
      = (_dbTechLayerAntennaRule*) getDefaultAntennaRule();

  // Reinitialize the object to its default state...
  if (r != nullptr) {
    r->~_dbTechLayerAntennaRule();
    new (r) _dbTechLayerAntennaRule(layer->getDatabase());
    r->layer_ = getImpl()->getOID();
  } else {
    _dbTech* tech = (_dbTech*) layer->getOwner();
    r = tech->antenna_rule_tbl_->create();
    layer->oxide1_ = r->getOID();
    r->layer_ = getImpl()->getOID();
  }

  return (dbTechLayerAntennaRule*) r;
}

dbTechLayerAntennaRule* dbTechLayer::createOxide2AntennaRule()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  _dbTechLayerAntennaRule* r
      = (_dbTechLayerAntennaRule*) getOxide2AntennaRule();

  // Reinitialize the object to its default state...
  if (r != nullptr) {
    r->~_dbTechLayerAntennaRule();
    new (r) _dbTechLayerAntennaRule(layer->getDatabase());
    r->layer_ = getImpl()->getOID();
  } else {
    _dbTech* tech = (_dbTech*) layer->getOwner();
    r = tech->antenna_rule_tbl_->create();
    layer->oxide2_ = r->getOID();
    r->layer_ = getImpl()->getOID();
  }

  return (dbTechLayerAntennaRule*) r;
}

bool dbTechLayer::hasDefaultAntennaRule() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return (layer->oxide1_ != 0);
}

bool dbTechLayer::hasOxide2AntennaRule() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return (layer->oxide2_ != 0);
}

dbTechLayerAntennaRule* dbTechLayer::getDefaultAntennaRule() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  _dbTech* tech = (_dbTech*) layer->getOwner();

  if (layer->oxide1_ == 0) {
    return nullptr;
  }

  return (dbTechLayerAntennaRule*) tech->antenna_rule_tbl_->getPtr(
      layer->oxide1_);
}

dbTechLayerAntennaRule* dbTechLayer::getOxide2AntennaRule() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  _dbTech* tech = (_dbTech*) layer->getOwner();

  if (layer->oxide2_ == 0) {
    return nullptr;
  }

  return (dbTechLayerAntennaRule*) tech->antenna_rule_tbl_->getPtr(
      layer->oxide2_);
}

void dbTechLayer::writeAntennaRulesLef(lefout& writer) const
{
  bool prt_model = (hasDefaultAntennaRule() && hasOxide2AntennaRule());

  if (prt_model) {
    fmt::print(writer.out(), "    ANTENNAMODEL OXIDE1 ;\n");
  }
  if (hasDefaultAntennaRule()) {
    getDefaultAntennaRule()->writeLef(writer);
  }

  if (prt_model) {
    fmt::print(writer.out(), "    ANTENNAMODEL OXIDE2 ;\n");
  }
  if (hasOxide2AntennaRule()) {
    getOxide2AntennaRule()->writeLef(writer);
  }
}

uint32_t dbTechLayer::getNumMasks() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->flags_.num_masks;
}

void dbTechLayer::setNumMasks(uint32_t number)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  if (number < 1 || number > 3) {
    getImpl()->getLogger()->error(
        utl::ODB, 282, "setNumMask {} not in range [1,3]", number);
  }
  layer->flags_.num_masks = number;
}

bool dbTechLayer::getThickness(uint32_t& inthk) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  if (layer->flags_.has_thickness) {
    inthk = layer->thickness_;
    return true;
  }

  return false;
}

void dbTechLayer::setThickness(uint32_t thickness)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->flags_.has_thickness = true;
  layer->thickness_ = thickness;
}

bool dbTechLayer::hasArea() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return (layer->flags_.has_area);
}

double  // Now denominated in squm
dbTechLayer::getArea() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  if (layer->flags_.has_area) {
    return layer->area_;
  }

  return 0.0;  // Default
}

void dbTechLayer::setArea(double area)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->flags_.has_area = true;
  layer->area_ = area;
}

bool dbTechLayer::hasMaxWidth() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return (layer->flags_.has_max_width);
}

uint32_t dbTechLayer::getMaxWidth() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  if (layer->flags_.has_max_width) {
    return layer->max_width_;
  }

  return MAX_INT;  // Default
}

void dbTechLayer::setMaxWidth(uint32_t max_width)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->flags_.has_max_width = true;
  layer->max_width_ = max_width;
}

uint32_t dbTechLayer::getMinWidth() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->min_width_;
}

void dbTechLayer::setMinWidth(uint32_t min_width)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->min_width_ = min_width;
}

bool dbTechLayer::hasMinStep() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return (layer->min_step_ >= 0);
}

uint32_t dbTechLayer::getMinStep() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  if (layer->min_step_ >= 0) {
    return layer->min_step_;
  }

  return 0;  // Default
}

void dbTechLayer::setMinStep(uint32_t min_step)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->min_step_ = min_step;
}

bool dbTechLayer::hasProtrusion() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return (layer->flags_.has_protrusion);
}

uint32_t dbTechLayer::getProtrusionWidth() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  if (layer->flags_.has_protrusion) {
    return layer->pt_.width;
  }

  return 0;  // Default
}

uint32_t dbTechLayer::getProtrusionLength() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  if (layer->flags_.has_protrusion) {
    return layer->pt_.length;
  }

  return 0;  // Default
}

uint32_t dbTechLayer::getProtrusionFromWidth() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  if (layer->flags_.has_protrusion) {
    return layer->pt_.from_width;
  }

  return 0;  // Default
}

void dbTechLayer::setProtrusion(uint32_t pt_width,
                                uint32_t pt_length,
                                uint32_t pt_from_width)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->flags_.has_protrusion = true;
  layer->pt_.width = pt_width;
  layer->pt_.length = pt_length;
  layer->pt_.from_width = pt_from_width;
}

int dbTechLayer::getPitch()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->pitch_x_;
}

int dbTechLayer::getPitchX()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->pitch_x_;
}

int dbTechLayer::getPitchY()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->pitch_y_;
}

int dbTechLayer::getFirstLastPitch()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->first_last_pitch_;
}

void dbTechLayer::setPitch(int pitch)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->pitch_x_ = pitch;
  layer->pitch_y_ = pitch;
  layer->flags_.has_xy_pitch = false;
}

void dbTechLayer::setPitchXY(int pitch_x, int pitch_y)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->pitch_x_ = pitch_x;
  layer->pitch_y_ = pitch_y;
  layer->flags_.has_xy_pitch = true;
}

void dbTechLayer::setFirstLastPitch(int first_last_pitch)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->first_last_pitch_ = first_last_pitch;
}

bool dbTechLayer::hasXYPitch()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->flags_.has_xy_pitch;
}

int dbTechLayer::getOffset()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->offset_x_;
}

int dbTechLayer::getOffsetX()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->offset_x_;
}

int dbTechLayer::getOffsetY()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->offset_y_;
}

void dbTechLayer::setOffset(int offset)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->offset_x_ = offset;
  layer->offset_y_ = offset;
  layer->flags_.has_xy_offset = false;
}

void dbTechLayer::setOffsetXY(int offset_x, int offset_y)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->offset_x_ = offset_x;
  layer->offset_y_ = offset_y;
  layer->flags_.has_xy_offset = true;
}

bool dbTechLayer::hasXYOffset()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->flags_.has_xy_offset;
}

dbTechLayerDir dbTechLayer::getDirection()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return dbTechLayerDir(layer->flags_.direction);
}

void dbTechLayer::setDirection(dbTechLayerDir direction)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->flags_.direction = direction.getValue();
}

dbTechLayerMinStepType dbTechLayer::getMinStepType() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return dbTechLayerMinStepType(layer->flags_.minstep_type);
}

void dbTechLayer::setMinStepType(dbTechLayerMinStepType type)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->flags_.minstep_type = type.getValue();
}

bool dbTechLayer::hasMinStepMaxLength() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->min_step_max_length_ >= 0;
}

uint32_t dbTechLayer::getMinStepMaxLength() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->min_step_max_length_;
}

void dbTechLayer::setMinStepMaxLength(uint32_t length)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->min_step_max_length_ = length;
}

bool dbTechLayer::hasMinStepMaxEdges() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->min_step_max_edges_ >= 0;
}

uint32_t dbTechLayer::getMinStepMaxEdges() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->min_step_max_edges_;
}

void dbTechLayer::setMinStepMaxEdges(uint32_t edges)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->min_step_max_edges_ = edges;
}

dbTechLayerType dbTechLayer::getType()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return dbTechLayerType(layer->flags_.type);
}

double dbTechLayer::getResistance()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->resistance_;
}

void dbTechLayer::setResistance(double resistance)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->resistance_ = resistance;
}

double dbTechLayer::getCapacitance()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->capacitance_;
}

void dbTechLayer::setCapacitance(double capacitance)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->capacitance_ = capacitance;
}

int dbTechLayer::getNumber() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->number_;
}

int dbTechLayer::getRoutingLevel()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->rlevel_;
}

dbTechLayer* dbTechLayer::getLowerLayer()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  _dbTech* tech = (_dbTech*) layer->getOwner();

  if (layer->lower_ == 0) {
    return nullptr;
  }

  return (dbTechLayer*) tech->layer_tbl_->getPtr(layer->lower_);
}

dbTechLayer* dbTechLayer::getUpperLayer()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  _dbTech* tech = (_dbTech*) layer->getOwner();

  if (layer->upper_ == 0) {
    return nullptr;
  }

  return (dbTechLayer*) tech->layer_tbl_->getPtr(layer->upper_);
}

dbTech* dbTechLayer::getTech() const
{
  return (dbTech*) getImpl()->getOwner();
}

bool dbTechLayer::hasOrthSpacingTable() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return !layer->orth_spacing_tbl_.empty();
}

void dbTechLayer::addOrthSpacingTableEntry(const int within, const int spacing)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->orth_spacing_tbl_.emplace_back(within, spacing);
}

dbTechLayer* dbTechLayer::create(dbTech* tech_,
                                 const char* name_,
                                 dbTechLayerType type)
{
  if (type.getValue() == dbTechLayerType::NONE) {
    return nullptr;
  }

  if (tech_->findLayer(name_)) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) tech_;
  _dbTechLayer* layer = tech->layer_tbl_->create();
  layer->name_ = safe_strdup(name_);
  layer->number_ = tech->layer_cnt_++;
  layer->flags_.type = type.getValue();

  if (type.getValue() == dbTechLayerType::ROUTING) {
    layer->rlevel_ = ++tech->rlayer_cnt_;
  }

  if (tech->bottom_ == 0) {
    tech->bottom_ = layer->getOID();
    tech->top_ = layer->getOID();
    return (dbTechLayer*) layer;
  }

  _dbTechLayer* top = tech->layer_tbl_->getPtr(tech->top_);
  top->upper_ = layer->getOID();
  layer->lower_ = top->getOID();
  tech->top_ = layer->getOID();

  return (dbTechLayer*) layer;
}

dbTechLayer* dbTechLayer::getTechLayer(dbTech* tech_, uint32_t dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechLayer*) tech->layer_tbl_->getPtr(dbid_);
}

// User Code End dbTechLayerPublicMethods
}  // namespace odb
   // Generator Code End Cpp
