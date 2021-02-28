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
#pragma once

#include "dbCore.h"
#include "dbHashTable.h"
#include "dbMatrix.h"
#include "dbTypes.h"
#include "dbVector.h"
#include "odb.h"
// User Code Begin includes
// User Code End includes

namespace odb {

class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;
class _dbTechLayerCutClassRule;
template <class T>
class dbTable;
class _dbTechLayerSpacingEolRule;
class _dbTechLayerCutSpacingRule;
class _dbTechLayerMinStepRule;
class _dbTechLayerCornerSpacingRule;
class _dbTechLayerSpacingTablePrlRule;
class _dbTechLayerCutSpacingTableOrthRule;
class _dbTechLayerCutSpacingTableDefRule;
// User Code Begin Classes
class _dbTechLayerSpacingRule;
class _dbTechMinCutRule;
class _dbTechMinEncRule;
class _dbTechV55InfluenceEntry;
class _dbTechLayerAntennaRule;
// User Code End Classes

struct dbTechLayerFlags
{
  dbTechLayerType::Value        _type : 4;
  dbTechLayerDir::Value         _direction : 4;
  dbTechLayerMinStepType::Value _minstep_type : 2;
  uint                          _has_max_width : 1;
  uint                          _has_thickness : 1;
  uint                          _has_area : 1;
  uint                          _has_protrusion : 1;
  uint                          _has_alias : 1;
  uint                          _has_xy_pitch : 1;
  uint                          _has_xy_offset : 1;
  bool                          _rect_only : 1;
  bool                          _right_way_on_grid_only : 1;
  bool                          _right_way_on_grid_only_check_mask : 1;
  bool                          _rect_only_except_non_core_pins : 1;
  uint                          _lef58_type : 3;
  uint                          _spare_bits : 8;
};
// User Code Begin structs
// User Code End structs

class _dbTechLayer : public _dbObject
{
 public:
  // User Code Begin enums
  // User Code End enums

  dbTechLayerFlags _flags;

  dbTable<_dbTechLayerCutClassRule>*    _cut_class_rules_tbl;
  dbHashTable<_dbTechLayerCutClassRule> _cut_class_rules_hash;

  dbTable<_dbTechLayerSpacingEolRule>* _spacing_eol_rules_tbl;

  dbTable<_dbTechLayerCutSpacingRule>* _cut_spacing_rules_tbl;

  dbTable<_dbTechLayerMinStepRule>* _minstep_rules_tbl;

  dbTable<_dbTechLayerCornerSpacingRule>* _corner_spacing_rules_tbl;

  dbTable<_dbTechLayerSpacingTablePrlRule>* _spacing_table_prl_rules_tbl;

  dbTable<_dbTechLayerCutSpacingTableOrthRule>* _cut_spacing_table_orth_tbl;

  dbTable<_dbTechLayerCutSpacingTableDefRule>* _cut_spacing_table_def_tbl;

  // User Code Begin fields

  uint   _pitch_x;
  uint   _pitch_y;
  uint   _offset_x;
  uint   _offset_y;
  uint   _width;
  uint   _spacing;
  double _resistance;
  double _capacitance;
  double _edge_capacitance;
  uint   _wire_extension;
  uint   _number;
  uint   _rlevel;
  double _area;
  uint   _thickness;
  uint   _max_width;
  int    _min_width;
  int    _min_step;
  int    _min_step_max_length;
  int    _min_step_max_edges;

  struct
  {  // Protrusion
    uint _width;
    uint _length;
    uint _from_width;
  } _pt;
  char*                             _name;
  char*                             _alias;
  dbId<_dbTechLayer>                _upper;
  dbId<_dbTechLayer>                _lower;
  dbTable<_dbTechLayerSpacingRule>* _spacing_rules_tbl;

  dbTable<_dbTechMinCutRule>*        _min_cut_rules_tbl;
  dbTable<_dbTechMinEncRule>*        _min_enc_rules_tbl;
  dbTable<_dbTechV55InfluenceEntry>* _v55inf_tbl;
  dbVector<uint>                     _v55sp_length_idx;
  dbVector<uint>                     _v55sp_width_idx;
  dbMatrix<uint>                     _v55sp_spacing;

  dbVector<uint> _two_widths_sp_idx;
  dbVector<int>  _two_widths_sp_prl;
  dbMatrix<uint> _two_widths_sp_spacing;

  dbId<_dbTechLayerAntennaRule> _oxide1;
  dbId<_dbTechLayerAntennaRule> _oxide2;
  // User Code End fields
  _dbTechLayer(_dbDatabase*, const _dbTechLayer& r);
  _dbTechLayer(_dbDatabase*);
  ~_dbTechLayer();
  bool operator==(const _dbTechLayer& rhs) const;
  bool operator!=(const _dbTechLayer& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbTechLayer& rhs) const;
  void differences(dbDiff&             diff,
                   const char*         field,
                   const _dbTechLayer& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  dbObjectTable* getObjectTable(dbObjectType type);
  // User Code Begin methods
  // User Code End methods
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayer& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayer& obj);
// User Code Begin general
// User Code End general
}  // namespace odb
   // Generator Code End 1