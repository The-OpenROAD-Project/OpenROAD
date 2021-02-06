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

// Generator Code Begin 1
#include "dbTechLayer.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbSet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayerCutClassRule.h"
#include "dbTechLayerCutSpacingTableDefRule.h"
#include "dbTechLayerCutSpacingTableOrthRule.h"
// User Code Begin includes
#include "dbHashTable.hpp"
#include "dbTech.h"
#include "dbTechLayerAntennaRule.h"
#include "dbTechLayerCornerSpacingRule.h"
#include "dbTechLayerCutClassRule.h"
#include "dbTechLayerCutSpacingRule.h"
#include "dbTechLayerMinStepRule.h"
#include "dbTechLayerSpacingEolRule.h"
#include "dbTechLayerSpacingRule.h"
#include "dbTechLayerSpacingTablePrlRule.h"
#include "dbTechMinCutOrAreaRule.h"
#include "lefout.h"
// User Code End includes
namespace odb {

// User Code Begin definitions
// User Code End definitions
template class dbTable<_dbTechLayer>;

bool _dbTechLayer::operator==(const _dbTechLayer& rhs) const
{
  if (_flags._has_max_width != rhs._flags._has_max_width)
    return false;

  if (_flags._has_thickness != rhs._flags._has_thickness)
    return false;

  if (_flags._has_area != rhs._flags._has_area)
    return false;

  if (_flags._has_protrusion != rhs._flags._has_protrusion)
    return false;

  if (_flags._has_alias != rhs._flags._has_alias)
    return false;

  if (_flags._has_xy_pitch != rhs._flags._has_xy_pitch)
    return false;

  if (_flags._has_xy_offset != rhs._flags._has_xy_offset)
    return false;

  if (_flags._rect_only != rhs._flags._rect_only)
    return false;

  if (_flags._right_way_on_grid_only != rhs._flags._right_way_on_grid_only)
    return false;

  if (_flags._check_mask != rhs._flags._check_mask)
    return false;

  if (_flags._except_non_core_pins != rhs._flags._except_non_core_pins)
    return false;

  if (*_cut_class_rules_tbl != *rhs._cut_class_rules_tbl)
    return false;

  if (_cut_class_rules_hash != rhs._cut_class_rules_hash)
    return false;

  if (*_cut_spacing_table_orth_tbl != *rhs._cut_spacing_table_orth_tbl)
    return false;

  if (*_cut_spacing_table_def_tbl != *rhs._cut_spacing_table_def_tbl)
    return false;

  // User Code Begin ==
  if (_flags._type != rhs._flags._type)
    return false;

  if (_flags._direction != rhs._flags._direction)
    return false;

  if (_flags._minstep_type != rhs._flags._minstep_type)
    return false;

  if (_pitch_x != rhs._pitch_x)
    return false;

  if (_pitch_y != rhs._pitch_y)
    return false;

  if (_offset_x != rhs._offset_x)
    return false;

  if (_offset_y != rhs._offset_y)
    return false;

  if (_width != rhs._width)
    return false;

  if (_spacing != rhs._spacing)
    return false;

  if (_resistance != rhs._resistance)
    return false;

  if (_capacitance != rhs._capacitance)
    return false;

  if (_edge_capacitance != rhs._edge_capacitance)
    return false;

  if (_wire_extension != rhs._wire_extension)
    return false;

  if (_number != rhs._number)
    return false;

  if (_rlevel != rhs._rlevel)
    return false;

  if (_area != rhs._area)
    return false;

  if (_thickness != rhs._thickness)
    return false;

  if (_min_step != rhs._min_step)
    return false;

  if (_max_width != rhs._max_width)
    return false;

  if (_min_width != rhs._min_width)
    return false;

  if (_min_step_max_length != rhs._min_step_max_length)
    return false;

  if (_min_step_max_edges != rhs._min_step_max_edges)
    return false;

  if (_pt._width != rhs._pt._width)
    return false;

  if (_pt._length != rhs._pt._length)
    return false;

  if (_pt._from_width != rhs._pt._from_width)
    return false;

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0)
      return false;
  } else if (_name || rhs._name)
    return false;

  if (_alias && rhs._alias) {
    if (strcmp(_alias, rhs._alias) != 0)
      return false;
  } else if (_alias || rhs._alias)
    return false;

  if (_upper != rhs._upper)
    return false;

  if (_lower != rhs._lower)
    return false;

  if (*_spacing_rules_tbl != *rhs._spacing_rules_tbl)
    return false;

  if (*_spacing_eol_rules_tbl != *rhs._spacing_eol_rules_tbl)
    return false;

  if (*_cut_spacing_rules_tbl != *rhs._cut_spacing_rules_tbl)
    return false;

  if (*_minstep_rules_tbl != *rhs._minstep_rules_tbl)
    return false;

  if (*_corner_spacing_rules_tbl != *rhs._corner_spacing_rules_tbl)
    return false;

  if (*_spacing_table_prl_rules_tbl != *rhs._spacing_table_prl_rules_tbl)
    return false;

  if (*_cut_class_rules_tbl != *rhs._cut_class_rules_tbl)
    return false;

  if (*_min_cut_rules_tbl != *rhs._min_cut_rules_tbl)
    return false;

  if (*_min_enc_rules_tbl != *rhs._min_enc_rules_tbl)
    return false;

  if (*_v55inf_tbl != *rhs._v55inf_tbl)
    return false;

  if (_v55sp_length_idx != rhs._v55sp_length_idx)
    return false;

  if (_v55sp_width_idx != rhs._v55sp_width_idx)
    return false;

  if (_v55sp_spacing != rhs._v55sp_spacing)
    return false;

  if (_two_widths_sp_idx != rhs._two_widths_sp_idx)
    return false;

  if (_two_widths_sp_prl != rhs._two_widths_sp_prl)
    return false;

  if (_two_widths_sp_spacing != rhs._two_widths_sp_spacing)
    return false;

  if (_oxide1 != rhs._oxide1)
    return false;

  if (_oxide2 != rhs._oxide2)
    return false;
  // User Code End ==
  return true;
}
bool _dbTechLayer::operator<(const _dbTechLayer& rhs) const
{
  // User Code Begin <
  if (_number >= rhs._number)
    return false;
  // User Code End <
  return true;
}
void _dbTechLayer::differences(dbDiff&             diff,
                               const char*         field,
                               const _dbTechLayer& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(_flags._has_max_width);
  DIFF_FIELD(_flags._has_thickness);
  DIFF_FIELD(_flags._has_area);
  DIFF_FIELD(_flags._has_protrusion);
  DIFF_FIELD(_flags._has_alias);
  DIFF_FIELD(_flags._has_xy_pitch);
  DIFF_FIELD(_flags._has_xy_offset);
  DIFF_FIELD(_flags._rect_only);
  DIFF_FIELD(_flags._right_way_on_grid_only);
  DIFF_FIELD(_flags._check_mask);
  DIFF_FIELD(_flags._except_non_core_pins);
  DIFF_TABLE(_cut_class_rules_tbl);
  DIFF_HASH_TABLE(_cut_class_rules_hash);
  DIFF_TABLE(_cut_spacing_table_orth_tbl);
  DIFF_TABLE(_cut_spacing_table_def_tbl);
  // User Code Begin differences
  DIFF_FIELD(_flags._type);
  DIFF_FIELD(_flags._direction);
  DIFF_FIELD(_flags._minstep_type);
  DIFF_FIELD(_pitch_x);
  DIFF_FIELD(_pitch_y);
  DIFF_FIELD(_offset_x);
  DIFF_FIELD(_offset_y);
  DIFF_FIELD(_width);
  DIFF_FIELD(_spacing);
  DIFF_FIELD(_resistance);
  DIFF_FIELD(_capacitance);
  DIFF_FIELD(_edge_capacitance);
  DIFF_FIELD(_wire_extension);
  DIFF_FIELD(_number);
  DIFF_FIELD(_rlevel);
  DIFF_FIELD(_area);
  DIFF_FIELD(_thickness);
  DIFF_FIELD(_min_step);
  DIFF_FIELD(_max_width);
  DIFF_FIELD(_min_width);
  DIFF_FIELD(_min_step_max_length);
  DIFF_FIELD(_min_step_max_edges);
  DIFF_FIELD(_pt._length);
  DIFF_FIELD(_pt._from_width);
  DIFF_FIELD(_name);
  DIFF_FIELD(_alias);
  DIFF_FIELD(_upper);
  DIFF_FIELD(_lower);
  DIFF_TABLE_NO_DEEP(_spacing_rules_tbl);
  DIFF_TABLE_NO_DEEP(_spacing_eol_rules_tbl);
  DIFF_TABLE_NO_DEEP(_cut_spacing_rules_tbl);
  DIFF_TABLE_NO_DEEP(_minstep_rules_tbl);
  DIFF_TABLE_NO_DEEP(_corner_spacing_rules_tbl);
  DIFF_TABLE_NO_DEEP(_spacing_table_prl_rules_tbl);
  DIFF_TABLE_NO_DEEP(_cut_class_rules_tbl);
  DIFF_TABLE_NO_DEEP(_min_cut_rules_tbl);
  DIFF_TABLE_NO_DEEP(_min_enc_rules_tbl);
  DIFF_TABLE_NO_DEEP(_v55inf_tbl);
  DIFF_VECTOR(_v55sp_length_idx);
  DIFF_VECTOR(_v55sp_width_idx);
  DIFF_MATRIX(_v55sp_spacing);
  DIFF_VECTOR(_two_widths_sp_idx);
  DIFF_VECTOR(_two_widths_sp_prl);
  DIFF_MATRIX(_two_widths_sp_spacing);
  DIFF_FIELD(_oxide1);
  DIFF_FIELD(_oxide2);
  // User Code End differences
  DIFF_END
}
void _dbTechLayer::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._has_max_width);
  DIFF_OUT_FIELD(_flags._has_thickness);
  DIFF_OUT_FIELD(_flags._has_area);
  DIFF_OUT_FIELD(_flags._has_protrusion);
  DIFF_OUT_FIELD(_flags._has_alias);
  DIFF_OUT_FIELD(_flags._has_xy_pitch);
  DIFF_OUT_FIELD(_flags._has_xy_offset);
  DIFF_OUT_FIELD(_flags._rect_only);
  DIFF_OUT_FIELD(_flags._right_way_on_grid_only);
  DIFF_OUT_FIELD(_flags._check_mask);
  DIFF_OUT_FIELD(_flags._except_non_core_pins);
  DIFF_OUT_TABLE(_cut_class_rules_tbl);
  DIFF_OUT_HASH_TABLE(_cut_class_rules_hash);
  DIFF_OUT_TABLE(_cut_spacing_table_orth_tbl);
  DIFF_OUT_TABLE(_cut_spacing_table_def_tbl);

  // User Code Begin out
  DIFF_OUT_FIELD(_flags._type);
  DIFF_OUT_FIELD(_flags._direction);
  DIFF_OUT_FIELD(_flags._minstep_type);
  DIFF_OUT_FIELD(_pitch_x);
  DIFF_OUT_FIELD(_pitch_y);
  DIFF_OUT_FIELD(_offset_x);
  DIFF_OUT_FIELD(_offset_y);
  DIFF_OUT_FIELD(_width);
  DIFF_OUT_FIELD(_spacing);
  DIFF_OUT_FIELD(_resistance);
  DIFF_OUT_FIELD(_capacitance);
  DIFF_OUT_FIELD(_edge_capacitance);
  DIFF_OUT_FIELD(_wire_extension);
  DIFF_OUT_FIELD(_number);
  DIFF_OUT_FIELD(_rlevel);
  DIFF_OUT_FIELD(_area);
  DIFF_OUT_FIELD(_thickness);
  DIFF_OUT_FIELD(_min_step);
  DIFF_OUT_FIELD(_max_width);
  DIFF_OUT_FIELD(_min_width);
  DIFF_OUT_FIELD(_min_step_max_length);
  DIFF_OUT_FIELD(_min_step_max_edges);
  DIFF_OUT_FIELD(_pt._length);
  DIFF_OUT_FIELD(_pt._from_width);
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_alias);
  DIFF_OUT_FIELD(_upper);
  DIFF_OUT_FIELD(_lower);
  DIFF_OUT_TABLE_NO_DEEP(_spacing_rules_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_spacing_eol_rules_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_cut_spacing_rules_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_minstep_rules_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_corner_spacing_rules_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_spacing_table_prl_rules_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_cut_class_rules_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_min_cut_rules_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_min_enc_rules_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_v55inf_tbl);
  DIFF_OUT_VECTOR(_v55sp_length_idx);
  DIFF_OUT_VECTOR(_v55sp_width_idx);
  DIFF_OUT_MATRIX(_v55sp_spacing);
  DIFF_OUT_VECTOR(_two_widths_sp_idx);
  DIFF_OUT_VECTOR(_two_widths_sp_prl);
  DIFF_OUT_MATRIX(_two_widths_sp_spacing);
  DIFF_OUT_FIELD(_oxide1);
  DIFF_OUT_FIELD(_oxide2);
  // User Code End out
  DIFF_END
}
_dbTechLayer::_dbTechLayer(_dbDatabase* db)
{
  uint* _flags_bit_field = (uint*) &_flags;
  *_flags_bit_field      = 0;
  _cut_class_rules_tbl   = new dbTable<_dbTechLayerCutClassRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerCutClassRuleObj);
  ZALLOCATED(_cut_class_rules_tbl);
  _cut_class_rules_hash.setTable(_cut_class_rules_tbl);
  _cut_spacing_table_orth_tbl
      = new dbTable<_dbTechLayerCutSpacingTableOrthRule>(
          db,
          this,
          (GetObjTbl_t) &_dbTechLayer::getObjectTable,
          dbTechLayerCutSpacingTableOrthRuleObj);
  ZALLOCATED(_cut_spacing_table_orth_tbl);
  _cut_spacing_table_def_tbl = new dbTable<_dbTechLayerCutSpacingTableDefRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerCutSpacingTableDefRuleObj);
  ZALLOCATED(_cut_spacing_table_def_tbl);
  // User Code Begin constructor
  _flags._type         = dbTechLayerType::ROUTING;
  _flags._direction    = dbTechLayerDir::NONE;
  _flags._minstep_type = dbTechLayerMinStepType();
  _pitch_x             = 0;
  _pitch_y             = 0;
  _offset_x            = 0;
  _offset_y            = 0;
  _width               = 0;
  _spacing             = 0;
  _resistance          = 0.0;
  _capacitance         = 0.0;
  _edge_capacitance    = 0.0;
  _wire_extension      = 0;
  _number              = 0;
  _rlevel              = 0;
  _area                = 0.0;
  _thickness           = 0;
  _min_step            = -1;
  _pt._width           = 0;
  _pt._length          = 0;
  _pt._from_width      = 0;
  _max_width           = MAX_INT;
  _min_width           = 0;
  _min_step_max_length = -1;
  _min_step_max_edges  = -1;
  _v55sp_length_idx.clear();
  _v55sp_width_idx.clear();
  _v55sp_spacing.clear();
  _name  = 0;
  _alias = 0;

  _spacing_rules_tbl = new dbTable<_dbTechLayerSpacingRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerSpacingRuleObj);
  ZALLOCATED(_spacing_rules_tbl);

  _spacing_eol_rules_tbl = new dbTable<_dbTechLayerSpacingEolRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerSpacingEolRuleObj);
  ZALLOCATED(_spacing_eol_rules_tbl);

  _cut_spacing_rules_tbl = new dbTable<_dbTechLayerCutSpacingRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerCutSpacingRuleObj);
  ZALLOCATED(_cut_spacing_rules_tbl);

  _minstep_rules_tbl = new dbTable<_dbTechLayerMinStepRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerMinStepRuleObj);
  ZALLOCATED(_minstep_rules_tbl);

  _corner_spacing_rules_tbl = new dbTable<_dbTechLayerCornerSpacingRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerCornerSpacingRuleObj);
  ZALLOCATED(_corner_spacing_rules_tbl);

  _spacing_table_prl_rules_tbl = new dbTable<_dbTechLayerSpacingTablePrlRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechLayerSpacingTablePrlRuleObj);
  ZALLOCATED(_spacing_table_prl_rules_tbl);

  _min_cut_rules_tbl = new dbTable<_dbTechMinCutRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechMinCutRuleObj,
      8,
      3);
  ZALLOCATED(_min_cut_rules_tbl);

  _min_enc_rules_tbl = new dbTable<_dbTechMinEncRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechMinEncRuleObj,
      8,
      3);
  ZALLOCATED(_min_enc_rules_tbl);

  _v55inf_tbl = new dbTable<_dbTechV55InfluenceEntry>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayer::getObjectTable,
      dbTechV55InfluenceEntryObj,
      8,
      3);
  ZALLOCATED(_v55inf_tbl);
  // User Code End constructor
}
_dbTechLayer::_dbTechLayer(_dbDatabase* db, const _dbTechLayer& r)
{
  _flags._has_max_width          = r._flags._has_max_width;
  _flags._has_thickness          = r._flags._has_thickness;
  _flags._has_area               = r._flags._has_area;
  _flags._has_protrusion         = r._flags._has_protrusion;
  _flags._has_alias              = r._flags._has_alias;
  _flags._has_xy_pitch           = r._flags._has_xy_pitch;
  _flags._has_xy_offset          = r._flags._has_xy_offset;
  _flags._rect_only              = r._flags._rect_only;
  _flags._right_way_on_grid_only = r._flags._right_way_on_grid_only;
  _flags._check_mask             = r._flags._check_mask;
  _flags._except_non_core_pins   = r._flags._except_non_core_pins;
  _flags._spare_bits             = r._flags._spare_bits;
  _cut_class_rules_tbl           = new dbTable<_dbTechLayerCutClassRule>(
      db, this, *r._cut_class_rules_tbl);
  ZALLOCATED(_cut_class_rules_tbl);
  _cut_class_rules_hash.setTable(_cut_class_rules_tbl);
  _cut_spacing_table_orth_tbl
      = new dbTable<_dbTechLayerCutSpacingTableOrthRule>(
          db, this, *r._cut_spacing_table_orth_tbl);
  ZALLOCATED(_cut_spacing_table_orth_tbl);
  _cut_spacing_table_def_tbl = new dbTable<_dbTechLayerCutSpacingTableDefRule>(
      db, this, *r._cut_spacing_table_def_tbl);
  ZALLOCATED(_cut_spacing_table_def_tbl);
  // User Code Begin CopyConstructor
  _flags                 = r._flags;
  _pitch_x               = r._pitch_x;
  _pitch_y               = r._pitch_y;
  _offset_x              = r._offset_x;
  _offset_y              = r._offset_y;
  _width                 = r._width;
  _spacing               = r._spacing;
  _resistance            = r._resistance;
  _capacitance           = r._capacitance;
  _edge_capacitance      = r._edge_capacitance;
  _wire_extension        = r._wire_extension;
  _number                = r._number;
  _rlevel                = r._rlevel;
  _area                  = r._area;
  _thickness             = r._thickness;
  _max_width             = r._max_width;
  _min_width             = r._min_width;
  _min_step              = r._min_step;
  _min_step_max_length   = r._min_step_max_length;
  _min_step_max_edges    = r._min_step_max_edges;
  _pt                    = r._pt;
  _name                  = NULL;
  _alias                 = NULL;
  _upper                 = r._upper;
  _lower                 = r._lower;
  _v55sp_length_idx      = r._v55sp_length_idx;
  _v55sp_width_idx       = r._v55sp_width_idx;
  _v55sp_spacing         = r._v55sp_spacing;
  _two_widths_sp_idx     = r._two_widths_sp_idx;
  _two_widths_sp_prl     = r._two_widths_sp_prl;
  _two_widths_sp_spacing = r._two_widths_sp_spacing;
  _oxide1                = r._oxide1;
  _oxide2                = r._oxide2;

  if (r._name) {
    _name = strdup(r._name);
    ZALLOCATED(_name);
  }

  if (r._alias) {
    _alias = strdup(r._alias);
    ZALLOCATED(_alias);
  }

  _spacing_rules_tbl
      = new dbTable<_dbTechLayerSpacingRule>(db, this, *r._spacing_rules_tbl);
  ZALLOCATED(_spacing_rules_tbl);

  _spacing_eol_rules_tbl = new dbTable<_dbTechLayerSpacingEolRule>(
      db, this, *r._spacing_eol_rules_tbl);
  ZALLOCATED(_spacing_eol_rules_tbl);

  _cut_spacing_rules_tbl = new dbTable<_dbTechLayerCutSpacingRule>(
      db, this, *r._cut_spacing_rules_tbl);
  ZALLOCATED(_cut_spacing_rules_tbl);

  _minstep_rules_tbl
      = new dbTable<_dbTechLayerMinStepRule>(db, this, *r._minstep_rules_tbl);
  ZALLOCATED(_minstep_rules_tbl);

  _corner_spacing_rules_tbl = new dbTable<_dbTechLayerCornerSpacingRule>(
      db, this, *r._corner_spacing_rules_tbl);
  ZALLOCATED(_corner_spacing_rules_tbl);

  _spacing_table_prl_rules_tbl = new dbTable<_dbTechLayerSpacingTablePrlRule>(
      db, this, *r._spacing_table_prl_rules_tbl);
  ZALLOCATED(_spacing_table_prl_rules_tbl);

  _min_cut_rules_tbl
      = new dbTable<_dbTechMinCutRule>(db, this, *r._min_cut_rules_tbl);
  ZALLOCATED(_min_cut_rules_tbl);

  _min_enc_rules_tbl
      = new dbTable<_dbTechMinEncRule>(db, this, *r._min_enc_rules_tbl);
  ZALLOCATED(_min_enc_rules_tbl);

  _v55inf_tbl = new dbTable<_dbTechV55InfluenceEntry>(db, this, *r._v55inf_tbl);
  ZALLOCATED(_v55inf_tbl);
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayer& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream >> *_flags_bit_field;
  stream >> *obj._cut_class_rules_tbl;
  stream >> obj._cut_class_rules_hash;
  stream >> *obj._cut_spacing_table_orth_tbl;
  stream >> *obj._cut_spacing_table_def_tbl;
  // User Code Begin >>
  stream >> obj._pitch_x;
  stream >> obj._pitch_y;
  stream >> obj._offset_x;
  stream >> obj._offset_y;
  stream >> obj._width;
  stream >> obj._spacing;
  stream >> obj._resistance;
  stream >> obj._capacitance;
  stream >> obj._edge_capacitance;
  stream >> obj._wire_extension;
  stream >> obj._number;
  stream >> obj._rlevel;
  stream >> obj._area;
  stream >> obj._thickness;
  stream >> obj._min_step;
  stream >> obj._min_step_max_length;
  stream >> obj._min_step_max_edges;
  stream >> obj._max_width;
  stream >> obj._min_width;
  stream >> obj._pt._width;
  stream >> obj._pt._length;
  stream >> obj._pt._from_width;
  stream >> obj._name;
  stream >> obj._alias;
  stream >> obj._lower;
  stream >> obj._upper;
  stream >> *obj._spacing_rules_tbl;
  stream >> *obj._spacing_eol_rules_tbl;
  stream >> *obj._cut_spacing_rules_tbl;
  stream >> *obj._minstep_rules_tbl;
  stream >> *obj._corner_spacing_rules_tbl;
  stream >> *obj._spacing_table_prl_rules_tbl;
  stream >> *obj._min_cut_rules_tbl;
  stream >> *obj._min_enc_rules_tbl;
  stream >> *obj._v55inf_tbl;
  stream >> obj._v55sp_length_idx;
  stream >> obj._v55sp_width_idx;
  stream >> obj._v55sp_spacing;
  stream >> obj._two_widths_sp_idx;
  stream >> obj._two_widths_sp_prl;
  stream >> obj._two_widths_sp_spacing;
  stream >> obj._oxide1;
  stream >> obj._oxide2;
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbTechLayer& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream << *_flags_bit_field;
  stream << *obj._cut_class_rules_tbl;
  stream << obj._cut_class_rules_hash;
  stream << *obj._cut_spacing_table_orth_tbl;
  stream << *obj._cut_spacing_table_def_tbl;
  // User Code Begin <<
  stream << obj._pitch_x;
  stream << obj._pitch_y;
  stream << obj._offset_x;
  stream << obj._offset_y;
  stream << obj._width;
  stream << obj._spacing;
  stream << obj._resistance;
  stream << obj._capacitance;
  stream << obj._edge_capacitance;
  stream << obj._wire_extension;
  stream << obj._number;
  stream << obj._rlevel;
  stream << obj._area;
  stream << obj._thickness;
  stream << obj._min_step;
  stream << obj._min_step_max_length;
  stream << obj._min_step_max_edges;
  stream << obj._max_width;
  stream << obj._min_width;
  stream << obj._pt._width;
  stream << obj._pt._length;
  stream << obj._pt._from_width;
  stream << obj._name;
  stream << obj._alias;
  stream << obj._lower;
  stream << obj._upper;
  stream << *obj._spacing_rules_tbl;
  stream << *obj._spacing_eol_rules_tbl;
  stream << *obj._cut_spacing_rules_tbl;
  stream << *obj._minstep_rules_tbl;
  stream << *obj._corner_spacing_rules_tbl;
  stream << *obj._spacing_table_prl_rules_tbl;
  stream << *obj._min_cut_rules_tbl;
  stream << *obj._min_enc_rules_tbl;
  stream << *obj._v55inf_tbl;
  stream << obj._v55sp_length_idx;
  stream << obj._v55sp_width_idx;
  stream << obj._v55sp_spacing;
  stream << obj._two_widths_sp_idx;
  stream << obj._two_widths_sp_prl;
  stream << obj._two_widths_sp_spacing;
  stream << obj._oxide1;
  stream << obj._oxide2;
  // User Code End <<
  return stream;
}

dbObjectTable* _dbTechLayer::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbTechLayerCutClassRuleObj:
      return _cut_class_rules_tbl;
    case dbTechLayerCutSpacingTableOrthRuleObj:
      return _cut_spacing_table_orth_tbl;
    case dbTechLayerCutSpacingTableDefRuleObj:
      return _cut_spacing_table_def_tbl;
      // User Code Begin getObjectTable
    case dbTechLayerSpacingRuleObj:
      return _spacing_rules_tbl;

    case dbTechLayerSpacingEolRuleObj:
      return _spacing_eol_rules_tbl;

    case dbTechLayerCutSpacingRuleObj:
      return _cut_spacing_rules_tbl;

    case dbTechLayerMinStepRuleObj:
      return _minstep_rules_tbl;

    case dbTechLayerCornerSpacingRuleObj:
      return _corner_spacing_rules_tbl;

    case dbTechLayerSpacingTablePrlRuleObj:
      return _spacing_table_prl_rules_tbl;

    case dbTechMinCutRuleObj:
      return _min_cut_rules_tbl;

    case dbTechMinEncRuleObj:
      return _min_enc_rules_tbl;

    case dbTechV55InfluenceEntryObj:
      return _v55inf_tbl;
    // User Code End getObjectTable
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}
_dbTechLayer::~_dbTechLayer()
{
  delete _cut_class_rules_tbl;
  delete _cut_spacing_table_orth_tbl;
  delete _cut_spacing_table_def_tbl;
  // User Code Begin Destructor
  if (_name)
    free((void*) _name);

  if (_spacing_rules_tbl)
    delete _spacing_rules_tbl;

  if (_spacing_eol_rules_tbl)
    delete _spacing_eol_rules_tbl;

  if (_cut_spacing_rules_tbl)
    delete _cut_spacing_rules_tbl;

  if (_minstep_rules_tbl)
    delete _minstep_rules_tbl;

  if (_corner_spacing_rules_tbl)
    delete _corner_spacing_rules_tbl;

  if (_spacing_table_prl_rules_tbl)
    delete _spacing_table_prl_rules_tbl;

  if (_min_cut_rules_tbl)
    delete _min_cut_rules_tbl;

  if (_min_enc_rules_tbl)
    delete _min_enc_rules_tbl;

  if (_v55inf_tbl)
    delete _v55inf_tbl;
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbTechLayer - Methods
//
////////////////////////////////////////////////////////////////////

dbSet<dbTechLayerCutClassRule> dbTechLayer::getTechLayerCutClassRules() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerCutClassRule>(obj, obj->_cut_class_rules_tbl);
}

dbTechLayerCutClassRule* dbTechLayer::findTechLayerCutClassRule(
    const char* name) const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return (dbTechLayerCutClassRule*) obj->_cut_class_rules_hash.find(name);
}

dbSet<dbTechLayerCutSpacingTableOrthRule>
dbTechLayer::getTechLayerCutSpacingTableOrthRules() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerCutSpacingTableOrthRule>(
      obj, obj->_cut_spacing_table_orth_tbl);
}

dbSet<dbTechLayerCutSpacingTableDefRule>
dbTechLayer::getTechLayerCutSpacingTableDefRules() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;
  return dbSet<dbTechLayerCutSpacingTableDefRule>(
      obj, obj->_cut_spacing_table_def_tbl);
}

void dbTechLayer::setRectOnly(bool _rect_only)
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  obj->_flags._rect_only = _rect_only;
}

bool dbTechLayer::isRectOnly() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  return obj->_flags._rect_only;
}

void dbTechLayer::setRightWayOnGridOnly(bool _right_way_on_grid_only)
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  obj->_flags._right_way_on_grid_only = _right_way_on_grid_only;
}

bool dbTechLayer::isRightWayOnGridOnly() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  return obj->_flags._right_way_on_grid_only;
}

void dbTechLayer::setCheckMask(bool _check_mask)
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  obj->_flags._check_mask = _check_mask;
}

bool dbTechLayer::isCheckMask() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  return obj->_flags._check_mask;
}

void dbTechLayer::setExceptNonCorePins(bool _except_non_core_pins)
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  obj->_flags._except_non_core_pins = _except_non_core_pins;
}

bool dbTechLayer::isExceptNonCorePins() const
{
  _dbTechLayer* obj = (_dbTechLayer*) this;

  return obj->_flags._except_non_core_pins;
}

// User Code Begin dbTechLayerPublicMethods

std::string dbTechLayer::getName() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_name;
}

const char* dbTechLayer::getConstName() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_name;
}

bool dbTechLayer::hasAlias()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_flags._has_alias == 1;
}

std::string dbTechLayer::getAlias()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  if (layer->_alias == NULL) {
    return "";
  }

  return layer->_alias;
}

void dbTechLayer::setAlias(const char* alias)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  if (layer->_alias)
    free((void*) layer->_alias);

  layer->_flags._has_alias = 1;
  layer->_alias            = strdup(alias);
  ZALLOCATED(layer->_alias);
}

uint dbTechLayer::getWidth() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_width;
}

void dbTechLayer::setWidth(int width)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->_width       = width;
}

int dbTechLayer::getSpacing()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_spacing;
}

void dbTechLayer::setSpacing(int spacing)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->_spacing     = spacing;
}

double dbTechLayer::getEdgeCapacitance()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_edge_capacitance;
}

void dbTechLayer::setEdgeCapacitance(double cap)
{
  _dbTechLayer* layer      = (_dbTechLayer*) this;
  layer->_edge_capacitance = cap;
}

uint dbTechLayer::getWireExtension()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_wire_extension;
}

void dbTechLayer::setWireExtension(uint ext)
{
  _dbTechLayer* layer    = (_dbTechLayer*) this;
  layer->_wire_extension = ext;
}

int dbTechLayer::getSpacing(int w, int l)
{
  _dbTechLayer*                 layer = (_dbTechLayer*) this;
  dbSet<dbTechLayerSpacingRule> v54rules;

  bool found_spacing = false;
  uint spacing       = MAX_INT;

  bool found_over_spacing = false;
  uint over_spacing       = MAX_INT;
  uint width              = (uint) w;
  uint length             = (uint) l;

  if (getV54SpacingRules(v54rules)) {
    dbSet<dbTechLayerSpacingRule>::iterator ritr;
    dbTechLayerSpacingRule*                 cur_rule;
    uint                                    rmin, rmax;

    for (ritr = v54rules.begin(); ritr != v54rules.end(); ++ritr) {
      cur_rule = *ritr;
      if (cur_rule->getRange(rmin, rmax)) {
        if ((width >= rmin) && (width <= rmax)) {
          spacing       = MIN(spacing, cur_rule->getSpacing());
          found_spacing = true;
        }
        if (width > rmax) {
          found_over_spacing = true;
          over_spacing       = MIN(over_spacing, cur_rule->getSpacing());
        }
      }
    }
  }

  std::vector<std::vector<uint>> v55rules;
  uint                           i, j;
  if (getV55SpacingTable(v55rules)) {
    for (i = 1; (i < layer->_v55sp_width_idx.size())
                && (width > layer->_v55sp_width_idx[i]);
         i++)
      ;
    for (j = 1; (j < layer->_v55sp_length_idx.size())
                && (length > layer->_v55sp_length_idx[j]);
         j++)
      ;
    found_spacing = true;
    spacing       = v55rules[i - 1][j - 1];
  }

  if ((!found_spacing) && (found_over_spacing)) {
    found_spacing = true;
    spacing       = over_spacing;
  }

  return (found_spacing) ? spacing : layer->_spacing;
}

//
// Get the low end of the uppermost range for wide wire design rules.
//
void dbTechLayer::getMaxWideDRCRange(int& owidth, int& olength)
{
  _dbTechLayer*                 layer = (_dbTechLayer*) this;
  dbSet<dbTechLayerSpacingRule> v54rules;

  owidth  = getWidth();
  olength = owidth;

  if (getV54SpacingRules(v54rules)) {
    dbSet<dbTechLayerSpacingRule>::iterator ritr;
    uint                                    rmin, rmax;

    for (ritr = v54rules.begin(); ritr != v54rules.end(); ++ritr) {
      if ((*ritr)->getRange(rmin, rmax)) {
        if (rmin > (uint) owidth) {
          owidth  = rmin;
          olength = rmin;
        }
      }
    }
  }

  if (hasV55SpacingRules()) {
    owidth  = layer->_v55sp_width_idx[layer->_v55sp_width_idx.size() - 1];
    olength = layer->_v55sp_length_idx[layer->_v55sp_length_idx.size() - 1];
  }
}

//
// Get the low end of the lowermost range for wide wire design rules.
//
void dbTechLayer::getMinWideDRCRange(int& owidth, int& olength)
{
  _dbTechLayer*                 layer = (_dbTechLayer*) this;
  dbSet<dbTechLayerSpacingRule> v54rules;

  owidth  = getWidth();
  olength = owidth;

  if (getV54SpacingRules(v54rules)) {
    dbSet<dbTechLayerSpacingRule>::iterator ritr;
    uint                                    rmin, rmax;
    bool                                    range_found = false;

    for (ritr = v54rules.begin(); ritr != v54rules.end(); ++ritr) {
      if ((*ritr)->getRange(rmin, rmax)) {
        if ((rmin < (uint) owidth) || !range_found) {
          owidth  = rmin;
          olength = rmin;
        }
      }
    }
  }

  if (hasV55SpacingRules()) {
    owidth  = layer->_v55sp_width_idx[1];
    olength = layer->_v55sp_length_idx[1];
  }
}

bool dbTechLayer::getV54SpacingRules(
    dbSet<dbTechLayerSpacingRule>& sp_rules) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  sp_rules = dbSet<dbTechLayerSpacingRule>(layer, layer->_spacing_rules_tbl);
  return true;
}

dbSet<dbTechLayerSpacingEolRule> dbTechLayer::getEolSpacingRules() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  return dbSet<dbTechLayerSpacingEolRule>(layer, layer->_spacing_eol_rules_tbl);
}

dbSet<dbTechLayerCutSpacingRule> dbTechLayer::getCutSpacingRules() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  return dbSet<dbTechLayerCutSpacingRule>(layer, layer->_cut_spacing_rules_tbl);
}

dbSet<dbTechLayerMinStepRule> dbTechLayer::getMinStepRules() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  return dbSet<dbTechLayerMinStepRule>(layer, layer->_minstep_rules_tbl);
}

dbSet<dbTechLayerCornerSpacingRule> dbTechLayer::getCornerSpacingRules() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  return dbSet<dbTechLayerCornerSpacingRule>(layer,
                                             layer->_corner_spacing_rules_tbl);
}

dbSet<dbTechLayerSpacingTablePrlRule> dbTechLayer::getSpacingTablePrlRules()
    const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  return dbSet<dbTechLayerSpacingTablePrlRule>(
      layer, layer->_spacing_table_prl_rules_tbl);
}

bool dbTechLayer::hasV55SpacingRules() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return ((layer->_v55sp_length_idx.size() > 0)
          && (layer->_v55sp_width_idx.size() > 0)
          && (layer->_v55sp_spacing.numElems() > 0));
}

void dbTechLayer::printV55SpacingRules(lefout& writer) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  fprintf(writer.out(), "SPACINGTABLE\n");
  fprintf(writer.out(), "  PARALLELRUNLENGTH");
  dbVector<uint>::const_iterator v55_itr;
  uint                           wddx, lndx;

  for (v55_itr = layer->_v55sp_length_idx.begin();
       v55_itr != layer->_v55sp_length_idx.end();
       v55_itr++)
    fprintf(writer.out(), " %.3f", writer.lefdist(*v55_itr));

  for (wddx = 0, v55_itr = layer->_v55sp_width_idx.begin();
       v55_itr != layer->_v55sp_width_idx.end();
       wddx++, v55_itr++) {
    fprintf(writer.out(), "\n");
    fprintf(writer.out(), "  WIDTH %.3f\t", writer.lefdist(*v55_itr));
    for (lndx = 0; lndx < layer->_v55sp_spacing.numCols(); lndx++)
      fprintf(writer.out(),
              " %.3f",
              writer.lefdist(layer->_v55sp_spacing(wddx, lndx)));
  }

  fprintf(writer.out(), " ;\n");
}

bool dbTechLayer::getV55SpacingTable(
    std::vector<std::vector<uint>>& sptbl) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  if (layer->_v55sp_spacing.numElems() == 0)
    return false;

  uint i, j;
  sptbl.clear();
  sptbl.resize(layer->_v55sp_spacing.numRows());
  std::vector<uint> tmpvec;
  tmpvec.reserve(layer->_v55sp_spacing.numCols());
  for (i = 0; i < layer->_v55sp_spacing.numRows(); i++) {
    tmpvec.clear();
    for (j = 0; j < layer->_v55sp_spacing.numCols(); j++)
      tmpvec.push_back(layer->_v55sp_spacing(i, j));
    sptbl[i] = tmpvec;
  }

  return true;
}

void dbTechLayer::initV55LengthIndex(uint numelems)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->_v55sp_length_idx.reserve(numelems);
}

void dbTechLayer::addV55LengthEntry(uint length)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->_v55sp_length_idx.push_back(length);
}

void dbTechLayer::initV55WidthIndex(uint numelems)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->_v55sp_width_idx.reserve(numelems);
}

void dbTechLayer::addV55WidthEntry(uint width)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->_v55sp_width_idx.push_back(width);
}

void dbTechLayer::initV55SpacingTable(uint numrows, uint numcols)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->_v55sp_spacing.resize(numrows, numcols);
}

void dbTechLayer::addV55SpacingTableEntry(uint inrow, uint incol, uint spacing)
{
  _dbTechLayer* layer                 = (_dbTechLayer*) this;
  layer->_v55sp_spacing(inrow, incol) = spacing;
}

bool dbTechLayer::getV55InfluenceRules(
    std::vector<dbTechV55InfluenceEntry*>& inf_tbl)
{
  inf_tbl.clear();
  dbSet<dbTechV55InfluenceEntry> entries = getV55InfluenceEntries();

  if (entries.size() == 0)
    return false;

  dbSet<dbTechV55InfluenceEntry>::iterator itr;

  for (itr = entries.begin(); itr != entries.end(); ++itr)
    inf_tbl.push_back(*itr);

  return true;
}

bool dbTechLayer::hasTwoWidthsSpacingRules() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return ((layer->_two_widths_sp_idx.size() > 0)
          && (layer->_two_widths_sp_spacing.numElems() > 0));
}

void dbTechLayer::printTwoWidthsSpacingRules(lefout& writer) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  fprintf(writer.out(), "SPACINGTABLE TWOWIDTHS");
  dbVector<uint>::const_iterator itr;
  uint                           wddx, lndx;

  for (wddx = 0, itr = layer->_two_widths_sp_idx.begin();
       itr != layer->_two_widths_sp_idx.end();
       wddx++, itr++) {
    fprintf(writer.out(), "\n  WIDTH %.3f\t", writer.lefdist(*itr));
    for (lndx = 0; lndx < layer->_two_widths_sp_spacing.numCols(); lndx++)
      fprintf(writer.out(),
              " %.3f",
              writer.lefdist(layer->_two_widths_sp_spacing(wddx, lndx)));
  }

  fprintf(writer.out(), " ;\n");
}

uint dbTechLayer::getTwoWidthsSpacingTableEntry(uint row, uint col) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_two_widths_sp_spacing(row, col);
}

uint dbTechLayer::getTwoWidthsSpacingTableNumWidths() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_two_widths_sp_idx.size();
}

uint dbTechLayer::getTwoWidthsSpacingTableWidth(uint row) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_two_widths_sp_idx.at(row);
}

bool dbTechLayer::getTwoWidthsSpacingTableHasPRL(uint row) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_two_widths_sp_prl.at(row) >= 0;
}

uint dbTechLayer::getTwoWidthsSpacingTablePRL(uint row) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_two_widths_sp_prl.at(row);
}

bool dbTechLayer::getTwoWidthsSpacingTable(
    std::vector<std::vector<uint>>& sptbl) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;

  if (layer->_two_widths_sp_spacing.numElems() == 0)
    return false;

  uint i, j;
  sptbl.clear();
  sptbl.resize(layer->_two_widths_sp_spacing.numRows());
  std::vector<uint> tmpvec;
  tmpvec.reserve(layer->_two_widths_sp_spacing.numCols());
  for (i = 0; i < layer->_two_widths_sp_spacing.numRows(); i++) {
    tmpvec.clear();
    for (j = 0; j < layer->_two_widths_sp_spacing.numCols(); j++)
      tmpvec.push_back(layer->_two_widths_sp_spacing(i, j));
    sptbl[i] = tmpvec;
  }

  return true;
}

void dbTechLayer::initTwoWidths(uint num_widths)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->_two_widths_sp_idx.reserve(num_widths);
  layer->_two_widths_sp_spacing.resize(num_widths, num_widths);
}

void dbTechLayer::addTwoWidthsIndexEntry(uint width, int parallel_run_length)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->_two_widths_sp_idx.push_back(width);
  layer->_two_widths_sp_prl.push_back(parallel_run_length);
}

void dbTechLayer::addTwoWidthsSpacingTableEntry(uint inrow,
                                                uint incol,
                                                uint spacing)
{
  _dbTechLayer* layer                         = (_dbTechLayer*) this;
  layer->_two_widths_sp_spacing(inrow, incol) = spacing;
}

bool dbTechLayer::getMinimumCutRules(std::vector<dbTechMinCutRule*>& cut_rules)
{
  cut_rules.clear();
  dbSet<dbTechMinCutRule> rules = getMinCutRules();

  if (rules.size() == 0)
    return false;

  dbSet<dbTechMinCutRule>::iterator itr;

  for (itr = rules.begin(); itr != rules.end(); ++itr)
    cut_rules.push_back((dbTechMinCutRule*) *itr);

  return true;
}

dbSet<dbTechMinCutRule> dbTechLayer::getMinCutRules()
{
  dbSet<dbTechMinCutRule> rules;
  _dbTechLayer*           layer = (_dbTechLayer*) this;
  rules = dbSet<dbTechMinCutRule>(layer, layer->_min_cut_rules_tbl);
  return rules;
}

dbSet<dbTechMinEncRule> dbTechLayer::getMinEncRules()
{
  dbSet<dbTechMinEncRule> rules;
  _dbTechLayer*           layer = (_dbTechLayer*) this;
  rules = dbSet<dbTechMinEncRule>(layer, layer->_min_enc_rules_tbl);
  return rules;
}

dbSet<dbTechV55InfluenceEntry> dbTechLayer::getV55InfluenceEntries()
{
  dbSet<dbTechV55InfluenceEntry> rules;
  _dbTechLayer*                  layer = (_dbTechLayer*) this;
  rules = dbSet<dbTechV55InfluenceEntry>(layer, layer->_v55inf_tbl);
  return rules;
}

bool dbTechLayer::getMinEnclosureRules(
    std::vector<dbTechMinEncRule*>& enc_rules)
{
  enc_rules.clear();

  dbSet<dbTechMinEncRule> rules = getMinEncRules();

  if (rules.size() == 0)
    return false;

  dbSet<dbTechMinEncRule>::iterator itr;

  for (itr = rules.begin(); itr != rules.end(); ++itr)
    enc_rules.push_back(*itr);

  return true;
}

dbTechLayerAntennaRule* dbTechLayer::createDefaultAntennaRule()
{
  _dbTechLayer*            layer = (_dbTechLayer*) this;
  _dbTechLayerAntennaRule* r
      = (_dbTechLayerAntennaRule*) getDefaultAntennaRule();

  // Reinitialize the object to its default state...
  if (r != NULL) {
    r->~_dbTechLayerAntennaRule();
    new (r) _dbTechLayerAntennaRule(layer->getDatabase());
    r->_layer = getImpl()->getOID();
  } else {
    _dbTech* tech  = (_dbTech*) layer->getOwner();
    r              = tech->_antenna_rule_tbl->create();
    layer->_oxide1 = r->getOID();
    r->_layer      = getImpl()->getOID();
  }

  return (dbTechLayerAntennaRule*) r;
}

dbTechLayerAntennaRule* dbTechLayer::createOxide2AntennaRule()
{
  _dbTechLayer*            layer = (_dbTechLayer*) this;
  _dbTechLayerAntennaRule* r
      = (_dbTechLayerAntennaRule*) getOxide2AntennaRule();

  // Reinitialize the object to its default state...
  if (r != NULL) {
    r->~_dbTechLayerAntennaRule();
    new (r) _dbTechLayerAntennaRule(layer->getDatabase());
    r->_layer = getImpl()->getOID();
  } else {
    _dbTech* tech  = (_dbTech*) layer->getOwner();
    r              = tech->_antenna_rule_tbl->create();
    layer->_oxide2 = r->getOID();
    r->_layer      = getImpl()->getOID();
  }

  return (dbTechLayerAntennaRule*) r;
}

bool dbTechLayer::hasDefaultAntennaRule() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return (layer->_oxide1 != 0);
}

bool dbTechLayer::hasOxide2AntennaRule() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return (layer->_oxide2 != 0);
}

dbTechLayerAntennaRule* dbTechLayer::getDefaultAntennaRule() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  _dbTech*      tech  = (_dbTech*) layer->getOwner();

  if (layer->_oxide1 == 0)
    return NULL;

  return (dbTechLayerAntennaRule*) tech->_antenna_rule_tbl->getPtr(
      layer->_oxide1);
}

dbTechLayerAntennaRule* dbTechLayer::getOxide2AntennaRule() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  _dbTech*      tech  = (_dbTech*) layer->getOwner();

  if (layer->_oxide2 == 0)
    return NULL;

  return (dbTechLayerAntennaRule*) tech->_antenna_rule_tbl->getPtr(
      layer->_oxide2);
}

void dbTechLayer::writeAntennaRulesLef(lefout& writer) const
{
  bool prt_model = (hasDefaultAntennaRule() && hasOxide2AntennaRule());

  if (prt_model)
    fprintf(writer.out(), "    ANTENNAMODEL OXIDE1 ;\n");
  if (hasDefaultAntennaRule())
    getDefaultAntennaRule()->writeLef(writer);

  if (prt_model)
    fprintf(writer.out(), "    ANTENNAMODEL OXIDE2 ;\n");
  if (hasOxide2AntennaRule())
    getOxide2AntennaRule()->writeLef(writer);
}

bool dbTechLayer::getThickness(uint& inthk) const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  if (layer->_flags._has_thickness) {
    inthk = layer->_thickness;
    return true;
  }

  return false;
}

void dbTechLayer::setThickness(uint thickness)
{
  _dbTechLayer* layer          = (_dbTechLayer*) this;
  layer->_flags._has_thickness = 1;
  layer->_thickness            = thickness;
}

bool dbTechLayer::hasArea() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return (layer->_flags._has_area);
}

double  // Now denominated in squm
dbTechLayer::getArea() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  if (layer->_flags._has_area)
    return layer->_area;

  return 0.0;  // Default
}

void dbTechLayer::setArea(double area)
{
  _dbTechLayer* layer     = (_dbTechLayer*) this;
  layer->_flags._has_area = 1;
  layer->_area            = area;
}

bool dbTechLayer::hasMaxWidth() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return (layer->_flags._has_max_width);
}

uint dbTechLayer::getMaxWidth() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  if (layer->_flags._has_max_width)
    return layer->_max_width;

  return MAX_INT;  // Default
}

void dbTechLayer::setMaxWidth(uint max_width)
{
  _dbTechLayer* layer          = (_dbTechLayer*) this;
  layer->_flags._has_max_width = 1;
  layer->_max_width            = max_width;
}

uint dbTechLayer::getMinWidth() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_min_width;
}

void dbTechLayer::setMinWidth(uint min_width)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->_min_width   = min_width;
}

bool dbTechLayer::hasMinStep() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return (layer->_min_step >= 0);
}

uint dbTechLayer::getMinStep() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  if (layer->_min_step >= 0)
    return layer->_min_step;

  return 0;  // Default
}

void dbTechLayer::setMinStep(uint min_step)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->_min_step    = min_step;
}

bool dbTechLayer::hasProtrusion() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return (layer->_flags._has_protrusion);
}

uint dbTechLayer::getProtrusionWidth() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  if (layer->_flags._has_protrusion)
    return layer->_pt._width;

  return 0;  // Default
}

uint dbTechLayer::getProtrusionLength() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  if (layer->_flags._has_protrusion)
    return layer->_pt._length;

  return 0;  // Default
}

uint dbTechLayer::getProtrusionFromWidth() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  if (layer->_flags._has_protrusion)
    return layer->_pt._from_width;

  return 0;  // Default
}

void dbTechLayer::setProtrusion(uint pt_width,
                                uint pt_length,
                                uint pt_from_width)
{
  _dbTechLayer* layer           = (_dbTechLayer*) this;
  layer->_flags._has_protrusion = 1;
  layer->_pt._width             = pt_width;
  layer->_pt._length            = pt_length;
  layer->_pt._from_width        = pt_from_width;
}

int dbTechLayer::getPitch()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_pitch_x;
}

int dbTechLayer::getPitchX()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_pitch_x;
}

int dbTechLayer::getPitchY()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_pitch_y;
}

void dbTechLayer::setPitch(int pitch)
{
  _dbTechLayer* layer         = (_dbTechLayer*) this;
  layer->_pitch_x             = pitch;
  layer->_pitch_y             = pitch;
  layer->_flags._has_xy_pitch = false;
}

void dbTechLayer::setPitchXY(int pitch_x, int pitch_y)
{
  _dbTechLayer* layer         = (_dbTechLayer*) this;
  layer->_pitch_x             = pitch_x;
  layer->_pitch_y             = pitch_y;
  layer->_flags._has_xy_pitch = true;
}

bool dbTechLayer::hasXYPitch()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_flags._has_xy_pitch;
}

int dbTechLayer::getOffset()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_offset_x;
}

int dbTechLayer::getOffsetX()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_offset_x;
}

int dbTechLayer::getOffsetY()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_offset_y;
}

void dbTechLayer::setOffset(int offset)
{
  _dbTechLayer* layer          = (_dbTechLayer*) this;
  layer->_offset_x             = offset;
  layer->_offset_y             = offset;
  layer->_flags._has_xy_offset = false;
}

void dbTechLayer::setOffsetXY(int offset_x, int offset_y)
{
  _dbTechLayer* layer          = (_dbTechLayer*) this;
  layer->_offset_x             = offset_x;
  layer->_offset_y             = offset_y;
  layer->_flags._has_xy_offset = true;
}

bool dbTechLayer::hasXYOffset()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_flags._has_xy_offset;
}

dbTechLayerDir dbTechLayer::getDirection()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return dbTechLayerDir(layer->_flags._direction);
}

void dbTechLayer::setDirection(dbTechLayerDir direction)
{
  _dbTechLayer* layer      = (_dbTechLayer*) this;
  layer->_flags._direction = direction.getValue();
}

dbTechLayerMinStepType dbTechLayer::getMinStepType() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return dbTechLayerMinStepType(layer->_flags._minstep_type);
}

void dbTechLayer::setMinStepType(dbTechLayerMinStepType type)
{
  _dbTechLayer* layer         = (_dbTechLayer*) this;
  layer->_flags._minstep_type = type.getValue();
}

bool dbTechLayer::hasMinStepMaxLength() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_min_step_max_length >= 0;
}

uint dbTechLayer::getMinStepMaxLength() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_min_step_max_length;
}

void dbTechLayer::setMinStepMaxLength(uint length)
{
  _dbTechLayer* layer         = (_dbTechLayer*) this;
  layer->_min_step_max_length = length;
}

bool dbTechLayer::hasMinStepMaxEdges() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_min_step_max_edges >= 0;
}

uint dbTechLayer::getMinStepMaxEdges() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_min_step_max_edges;
}

void dbTechLayer::setMinStepMaxEdges(uint edges)
{
  _dbTechLayer* layer        = (_dbTechLayer*) this;
  layer->_min_step_max_edges = edges;
}

dbTechLayerType dbTechLayer::getType()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return dbTechLayerType(layer->_flags._type);
}

double dbTechLayer::getResistance()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_resistance;
}

void dbTechLayer::setResistance(double resistance)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->_resistance  = resistance;
}

double dbTechLayer::getCapacitance()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_capacitance;
}

void dbTechLayer::setCapacitance(double capacitance)
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  layer->_capacitance = capacitance;
}

int dbTechLayer::getNumber() const
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_number;
}

int dbTechLayer::getRoutingLevel()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  return layer->_rlevel;
}

dbTechLayer* dbTechLayer::getLowerLayer()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  _dbTech*      tech  = (_dbTech*) layer->getOwner();

  if (layer->_lower == 0)
    return NULL;

  return (dbTechLayer*) tech->_layer_tbl->getPtr(layer->_lower);
}

dbTechLayer* dbTechLayer::getUpperLayer()
{
  _dbTechLayer* layer = (_dbTechLayer*) this;
  _dbTech*      tech  = (_dbTech*) layer->getOwner();

  if (layer->_upper == 0)
    return NULL;

  return (dbTechLayer*) tech->_layer_tbl->getPtr(layer->_upper);
}

dbTech* dbTechLayer::getTech()
{
  return (dbTech*) getImpl()->getOwner();
}

dbTechLayer* dbTechLayer::create(dbTech*         tech_,
                                 const char*     name_,
                                 dbTechLayerType type)
{
  if (type.getValue() == dbTechLayerType::NONE)
    return NULL;

  if (tech_->findLayer(name_))
    return NULL;

  _dbTech*      tech  = (_dbTech*) tech_;
  _dbTechLayer* layer = tech->_layer_tbl->create();
  layer->_name        = strdup(name_);
  ZALLOCATED(layer->_name);
  layer->_number      = tech->_layer_cnt++;
  layer->_flags._type = type.getValue();

  if (type.getValue() == dbTechLayerType::ROUTING) {
    layer->_rlevel = ++tech->_rlayer_cnt;
  }

  if (tech->_bottom == 0) {
    tech->_bottom = layer->getOID();
    tech->_top    = layer->getOID();
    return (dbTechLayer*) layer;
  }

  _dbTechLayer* top = tech->_layer_tbl->getPtr(tech->_top);
  top->_upper       = layer->getOID();
  layer->_lower     = top->getOID();
  tech->_top        = layer->getOID();

  return (dbTechLayer*) layer;
}

dbTechLayer* dbTechLayer::getTechLayer(dbTech* tech_, uint dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechLayer*) tech->_layer_tbl->getPtr(dbid_);
}

// User Code End dbTechLayerPublicMethods
}  // namespace odb
   // Generator Code End 1
