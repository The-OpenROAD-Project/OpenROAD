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

#pragma once

#include <iostream>

#include "dbCore.h"
#include "odb/odb.h"

namespace utl {
class Logger;
}

namespace odb {

//
// When changing the database schema please add a #define to refer to the schema
// changes. Use the define statement along with the isSchema(rev) method:
//
// GOOD:
//
//    if ( db->isSchema(db_schema_initial) )
//    {
//     ....
//    }
//
// Don't use a revision number in the code, because it is hard to read:
//
// BAD:
//
//    if ( db->_schema_minor > 33 )
//    {
//     ....
//    }
//

//
// Schema Revisions
//
const uint db_schema_major = 0;  // Not used...
const uint db_schema_initial = 57;

const uint db_schema_minor = 91;  // Current revision number

// Revision where the layers ranges, for signals and clock nets,
// were moved from GlobalRouter to dbBlock
const uint db_schema_dbblock_layers_ranges = 91;

// Revision where via layer was added to dbGuide
const uint db_schema_db_guide_via_layer = 90;

// Revision where blocked regions for IO pins were added to dbBlock
const uint db_schema_dbblock_blocked_regions_for_pins = 89;

// Revision where odb::modITerm,modBTerm,modNet made doubly linked for
// hiearchical port removal
const uint db_schema_hier_port_removal = 89;

// Revision where odb::Polygon was added
const uint db_schema_polygon = 88;

// Revision where _dbTechLayer::max_spacing_rules_tbl_ was added
const uint db_schema_max_spacing = 87;

// Revision where bus ports added to odb
const uint db_schema_odb_busport = 86;

// Revision where constraint region was added to dbBTerm
const uint db_schema_bterm_constraint_region = 85;

// Revision where GRT layer adjustment was relocated to dbTechLayer
const uint db_schema_layer_adjustment = 84;

// Revision where scan structs are added
const uint db_schema_add_scan = 83;

// Revision where _dbTechLayer::two_wires_forbidden_spc_rules_tbl_ was added
const uint db_schema_lef58_two_wires_forbidden_spacing = 82;
// Revision where hierarchy schema with modnets, modbterms, moditerms introduced
const uint db_schema_update_hierarchy = 81;
// Revision where dbPowerSwitch changed from strings to structs
const uint db_schema_update_db_power_switch = 80;

// Revision where dbGCellGrid::GCellData moved to uint8_t
const uint db_schema_smaler_gcelldata = 79;

// Revision where _dbBox / flags.mask was added
const uint db_schema_dbbox_mask = 78;

const uint db_schema_level_shifter_cell = 77;

const uint db_schema_power_domain_voltage = 76;

// Revision where _dbTechLayer::wrongdir_spacing_rules_tbl_ was added
const uint db_schema_wrongdir_spacing = 75;

// Revision where _dbLevelShifter was added
const uint db_schema_level_shifter = 74;

// Revision where _dbSite::_row_pattern/_parent_lib/_parent_site were added
const uint db_schema_site_row_pattern = 73;

// Revision where _dbMaster::_lib_for_site was added
const uint db_schema_dbmaster_lib_for_site = 72;

// Revision where _dbObstruction::_except_pg_nets was added
const uint db_schema_except_pg_nets_obstruction = 71;

// Revision where _dbTechLayer::forbidden_spacing_rules_tbl_ was added
const uint db_schema_lef58_forbidden_spacing = 70;

// Revision where upf power switch mapping was added.
const uint db_schema_upf_power_switch_mapping = 69;

// Revision where _component_shift_mask is added to _dbBlock.
const uint db_schema_block_component_mask_shift = 68;

// Revision where _minExtModelIndex & _maxExtModelIndex removed from
// _dbBlock.
const uint db_schema_block_ext_model_index = 67;

// Revision where _tech moved to _dbBlock & _dbLib from _dbDatabase.
// Added name to dbTech.
const uint db_schema_block_tech = 66;

// Revision where _dbGCellGrid switch to using dbMatrix
const uint db_schema_gcell_grid_matrix = 65;

// Revision where _dbBoxFlags shifted _mark bit to _layer_id
const uint db_schema_box_layer_bits = 64;

// Revision where _dbTechLayer::keepout_zone_rules_tbl_ was added
const uint db_schema_keepout_zone = 63;

// Revision where _dbBlock::_net_tracks_tbl was added
const uint db_schema_net_tracks = 62;

// Revision where _dbTechLayer::_first_last_pitch was added
const uint db_schema_lef58_pitch = 61;

// Revision where _dbTechLayer::wrong_way_width_ was added
const uint db_schema_wrongway_width = 60;

// Revision where dbGlobalConnect was added
const uint db_schema_add_global_connect = 58;

template <class T>
class dbTable;
class _dbProperty;
class dbPropertyItr;
class _dbNameCache;
class _dbTech;
class _dbChip;
class _dbLib;
class dbOStream;
class dbIStream;
class dbDiff;

class _dbDatabase : public _dbObject
{
 public:
  // PERSISTANT_MEMBERS
  uint _magic1;
  uint _magic2;
  uint _schema_major;
  uint _schema_minor;
  uint _master_id;  // for a unique id across all libraries
  dbId<_dbChip> _chip;

  // NON_PERSISTANT_MEMBERS
  dbTable<_dbTech>* _tech_tbl;
  dbTable<_dbLib>* _lib_tbl;
  dbTable<_dbChip>* _chip_tbl;
  dbTable<_dbProperty>* _prop_tbl;
  _dbNameCache* _name_cache;
  dbPropertyItr* _prop_itr;
  int _unique_id;

  utl::Logger* _logger;

  _dbDatabase(_dbDatabase* db);
  _dbDatabase(_dbDatabase* db, int id);
  _dbDatabase(_dbDatabase* db, const _dbDatabase& d);
  ~_dbDatabase();

  utl::Logger* getLogger() const;

  bool operator==(const _dbDatabase& rhs) const;
  bool operator!=(const _dbDatabase& rhs) const { return !operator==(rhs); }
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbDatabase& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  bool isSchema(uint rev) { return _schema_minor >= rev; }
  bool isLessThanSchema(uint rev) { return _schema_minor < rev; }
  dbObjectTable* getObjectTable(dbObjectType type);
};

dbOStream& operator<<(dbOStream& stream, const _dbDatabase& db);
dbIStream& operator>>(dbIStream& stream, _dbDatabase& db);

}  // namespace odb
