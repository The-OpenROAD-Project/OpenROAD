// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "dbHashTable.h"
#include "odb/dbId.h"
#include "odb/odb.h"
// User Code Begin Includes
#include <iostream>
#include <set>

#include "dbChipRegionInstItr.h"
#include "odb/dbDatabaseObserver.h"
namespace utl {
class Logger;
}
// User Code End Includes

namespace odb {
// User Code Begin Consts
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
inline constexpr uint db_schema_major = 0;  // Not used...
inline constexpr uint db_schema_initial = 57;

inline constexpr uint db_schema_minor = 118;  // Current revision number

// Revision where dbTech was moved from dbBlock to dbChip
inline constexpr uint db_schema_chip_tech = 118;

// Revision where dbChipBump was added
inline constexpr uint db_schema_chip_bump = 117;

// Revision where dbChipRegion was added
inline constexpr uint db_schema_chip_region = 116;

// Revision where dbChipInst was added
inline constexpr uint db_schema_chip_inst = 115;

// Revision where dbChip hash table was added
inline constexpr uint db_schema_chip_hash_table = 114;

// Revision where unique net/inst indices were added to dbBlock
inline constexpr uint db_schema_unique_indices = 113;

// Revision where dbChip was extended with new fields
inline constexpr uint db_schema_chip_extended = 112;

// Revision where the map which associates instances to their
// scan version was added
inline constexpr uint db_schema_map_insts_to_scan_insts = 111;

// Revision where the ownership of the scan insts was changed
// from the its scan list to the block
inline constexpr uint db_schema_block_owns_scan_insts = 110;

// Revision where is_connect_to_term_ flag was added to dbGuide
inline constexpr uint db_schema_guide_connected_to_term = 109;

// Revision where dbTable's mask/shift are compile constants
inline constexpr uint db_schema_table_mask_shift = 108;

// Revision where dbBTerm top layer grid was added to dbBlock
inline constexpr uint db_schema_bterm_top_layer_grid = 107;

// Revision where die area is converted to a polygon
inline constexpr uint db_schema_die_area_is_polygon = 106;

// Revision where check for mirrored constraint on bterm was added
inline constexpr uint db_schema_bterm_is_mirrored = 105;

// Revision where support for pin groups was added
inline constexpr uint db_schema_block_pin_groups = 104;

// Revision where support for mirrored pins was added
inline constexpr uint db_schema_bterm_mirrored_pin = 103;

// Revision where support for LEF58_CELLEDGESPACINGTABLE was added
inline constexpr uint db_schema_cell_edge_spc_tbl = 102;

// Revision where dbMasterEdgeType was added
inline constexpr uint db_schema_master_edge_type = 101;

// Revision where dbTarget was removed
inline constexpr uint db_rm_target = 100;

// Revision where mask information was added to track grids
inline constexpr uint db_track_mask = 99;

// Revision where the jumper insertion flag is added to dbNet
inline constexpr uint db_schema_has_jumpers = 98;

// Revision where the is_congested flag was added to dbGuide
inline constexpr uint db_schema_db_guide_congested = 97;

// Revision where the dbMarkerGroup/Categories were added to dbBlock
inline constexpr uint db_schema_dbmarkergroup = 96;

// Revision where orthogonal spacing table support added
inline constexpr uint db_schema_orth_spc_tbl = 95;

// Revision where unused hashes removed
inline constexpr uint db_schema_db_remove_hash = 94;

// Revision where the dbGDSLib is added to dbDatabase
inline constexpr uint db_schema_gds_lib_in_block = 93;

// Reverted Revision where unused hashes removed
inline constexpr uint reverted_db_schema_db_remove_hash = 92;

// Revision where the layers ranges, for signals and clock nets,
// were moved from GlobalRouter to dbBlock
inline constexpr uint db_schema_dbblock_layers_ranges = 91;

// Revision where via layer was added to dbGuide
inline constexpr uint db_schema_db_guide_via_layer = 90;

// Revision where blocked regions for IO pins were added to dbBlock
inline constexpr uint db_schema_dbblock_blocked_regions_for_pins = 89;

// Revision where odb::modITerm,modBTerm,modNet made doubly linked for
// hiearchical port removal
inline constexpr uint db_schema_hier_port_removal = 89;

// Revision where odb::Polygon was added
inline constexpr uint db_schema_polygon = 88;

// Revision where _dbTechLayer::max_spacing_rules_tbl_ was added
inline constexpr uint db_schema_max_spacing = 87;

// Revision where bus ports added to odb
inline constexpr uint db_schema_odb_busport = 86;

// Revision where constraint region was added to dbBTerm
inline constexpr uint db_schema_bterm_constraint_region = 85;

// Revision where GRT layer adjustment was relocated to dbTechLayer
inline constexpr uint db_schema_layer_adjustment = 84;

// Revision where scan structs are added
inline constexpr uint db_schema_add_scan = 83;

// Revision where _dbTechLayer::two_wires_forbidden_spc_rules_tbl_ was added
inline constexpr uint db_schema_lef58_two_wires_forbidden_spacing = 82;
// Revision where hierarchy schema with modnets, modbterms, moditerms introduced
inline constexpr uint db_schema_update_hierarchy = 81;
// Revision where dbPowerSwitch changed from strings to structs
inline constexpr uint db_schema_update_db_power_switch = 80;

// Revision where dbGCellGrid::GCellData moved to uint8_t
inline constexpr uint db_schema_smaler_gcelldata = 79;

// Revision where _dbBox / flags.mask was added
inline constexpr uint db_schema_dbbox_mask = 78;

inline constexpr uint db_schema_level_shifter_cell = 77;

inline constexpr uint db_schema_power_domain_voltage = 76;

// Revision where _dbTechLayer::wrongdir_spacing_rules_tbl_ was added
inline constexpr uint db_schema_wrongdir_spacing = 75;

// Revision where _dbLevelShifter was added
inline constexpr uint db_schema_level_shifter = 74;

// Revision where _dbSite::_row_pattern/_parent_lib/_parent_site were added
inline constexpr uint db_schema_site_row_pattern = 73;

// Revision where _dbMaster::_lib_for_site was added
inline constexpr uint db_schema_dbmaster_lib_for_site = 72;

// Revision where _dbObstruction::_except_pg_nets was added
inline constexpr uint db_schema_except_pg_nets_obstruction = 71;

// Revision where _dbTechLayer::forbidden_spacing_rules_tbl_ was added
inline constexpr uint db_schema_lef58_forbidden_spacing = 70;

// Revision where upf power switch mapping was added.
inline constexpr uint db_schema_upf_power_switch_mapping = 69;

// Revision where _component_shift_mask is added to _dbBlock.
inline constexpr uint db_schema_block_component_mask_shift = 68;

// Revision where _minExtModelIndex & _maxExtModelIndex removed from
// _dbBlock.
inline constexpr uint db_schema_block_ext_model_index = 67;

// Revision where _tech moved to _dbBlock & _dbLib from _dbDatabase.
// Added name to dbTech.
inline constexpr uint db_schema_block_tech = 66;

// Revision where _dbGCellGrid switch to using dbMatrix
inline constexpr uint db_schema_gcell_grid_matrix = 65;

// Revision where _dbBoxFlags shifted _mark bit to _layer_id
inline constexpr uint db_schema_box_layer_bits = 64;

// Revision where _dbTechLayer::keepout_zone_rules_tbl_ was added
inline constexpr uint db_schema_keepout_zone = 63;

// Revision where _dbBlock::_net_tracks_tbl was added
inline constexpr uint db_schema_net_tracks = 62;

// Revision where _dbTechLayer::_first_last_pitch was added
inline constexpr uint db_schema_lef58_pitch = 61;

// Revision where _dbTechLayer::wrong_way_width_ was added
inline constexpr uint db_schema_wrongway_width = 60;

// Revision where dbGlobalConnect was added
inline constexpr uint db_schema_add_global_connect = 58;

// User Code End Consts
class dbIStream;
class dbOStream;
class _dbChip;
class _dbProperty;
class _dbChipInst;
class _dbChipRegionInst;
class _dbChipConn;
class _dbChipBumpInst;
class _dbChipNet;
// User Code Begin Classes
class dbPropertyItr;
class dbChipInstItr;
class dbChipRegionInstItr;
class dbChipConnItr;
class dbChipBumpInstItr;
class dbChipNetItr;
class _dbNameCache;
class _dbTech;
class _dbLib;
class _dbGDSLib;
// User Code End Classes

class _dbDatabase : public _dbObject
{
 public:
  _dbDatabase(_dbDatabase*);

  ~_dbDatabase();

  bool operator==(const _dbDatabase& rhs) const;
  bool operator!=(const _dbDatabase& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbDatabase& rhs) const;
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods
  _dbDatabase(_dbDatabase* db, int id);
  utl::Logger* getLogger() const;
  bool isSchema(uint rev) const { return _schema_minor >= rev; }
  bool isLessThanSchema(uint rev) { return _schema_minor < rev; }
  // User Code End Methods

  uint _magic1;
  uint _magic2;
  uint _schema_major;
  uint _schema_minor;
  uint _master_id;
  dbId<_dbChip> _chip;
  dbTable<_dbChip, 2>* chip_tbl_;
  dbHashTable<_dbChip, 2> chip_hash_;
  dbTable<_dbProperty>* _prop_tbl;
  dbTable<_dbChipInst>* chip_inst_tbl_;
  dbTable<_dbChipRegionInst>* chip_region_inst_tbl_;
  dbTable<_dbChipConn>* chip_conn_tbl_;
  dbTable<_dbChipBumpInst>* chip_bump_inst_tbl_;
  dbTable<_dbChipNet>* chip_net_tbl_;

  // User Code Begin Fields
  dbTable<_dbTech, 2>* _tech_tbl;
  dbTable<_dbLib>* _lib_tbl;
  dbTable<_dbGDSLib, 2>* _gds_lib_tbl;
  _dbNameCache* _name_cache;
  dbPropertyItr* _prop_itr;
  dbChipInstItr* chip_inst_itr_;
  dbChipRegionInstItr* chip_region_inst_itr_;
  dbChipConnItr* chip_conn_itr_;
  dbChipBumpInstItr* chip_bump_inst_itr_;
  dbChipNetItr* chip_net_itr_;
  int _unique_id;

  utl::Logger* _logger;
  std::set<dbDatabaseObserver*> observers_;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbDatabase& obj);
dbOStream& operator<<(dbOStream& stream, const _dbDatabase& obj);
}  // namespace odb
// Generator Code End Header
