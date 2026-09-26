// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbHashTable.h"
#include "odb/dbId.h"
// User Code Begin Includes
#include <iostream>
#include <set>

#include "dbChipRegionInstItr.h"
#include "odb/dbDatabaseObserver.h"
#include "odb/dbObject.h"
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
//    if ( db->isSchema(kSchemaInitial )
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
// Magic number is: ATHENADB
inline constexpr uint32_t kMagic1 = 0x41544845;  // ATHE
inline constexpr uint32_t kMagic2 = 0x4E414442;  // NADB

inline constexpr uint32_t kSchemaMajor = 0;  // Not used...

// The oldest revision the format has ever had. Nothing parses this directly
// any more -- see kSchemaOldestReadable below -- but it is the lower bound a
// converter chain has to reach back to.
inline constexpr uint32_t kSchemaInitial = 57;

inline constexpr uint32_t kSchemaMinor = 141;  // Current revision number

// Revision where _dbPolygon::min_spacing_ was added
inline constexpr uint32_t kSchemaPolygonMinSpacing = 141;

// The oldest revision this build parses itself.
//
// Everything between kSchemaInitial and this is read by an external
// converter instead (see odb/dbSchemaUpgrade.h and //src/odb/converter):
// a binary built from the OpenROAD source archive that already knew how to
// read it. That code is immutable and needs no upkeep, whereas every
// isSchema() branch left in the tree does, so this floor is what decides how
// much compatibility code odb has to carry. Raising it makes every
// isSchema(rev) test with rev <= the new floor unconditionally true, and
// those branches can then be deleted.
//
// The window is roughly a year of schema revisions. Moving it means adding a
// snapshot to //MODULE.bazel first, so files in the range being dropped
// still open.
inline constexpr uint32_t kSchemaOldestReadable = 119;

// Revision where LEF58_MUSTJOINALLPORTS was added
inline constexpr uint32_t kSchemaMustJoinAllPorts = 140;

// Revision where dbTech::extraction_rules_file_ was removed
inline constexpr uint32_t kSchemaRemoveTechExtractionRulesFile = 139;

// Revision where _dbBox::min_spacing_ was added
inline constexpr uint32_t kSchemaDbBoxMinSpacing = 138;

// Revision where dbChipCapNode/dbChipRSeg inter-chip parasitics were added
inline constexpr uint32_t kSchemaChipParasitics = 137;

// Revision where dbNet::disable_auto_taper flag was added
inline constexpr uint32_t kSchemaNetDisableAutoTaper = 136;

// Revision where dbTech::extraction_rules_file_ was added
inline constexpr uint32_t kSchemaTechExtractionRulesFile = 135;

// Revision where the per-corner child-block feature for parasitics was removed
inline constexpr uint32_t kSchemaRemovePerCornerBlock = 134;

// Revision where the corner data (corner count + corner/factor lists) was
// removed from dbExtControl
inline constexpr uint32_t kSchemaRemoveExtControlCornerData = 133;

// Revision where dbInst::bump_ was added
inline constexpr uint32_t kSchemaInstBump = 132;
// Revision where all areas in the are switched to be stored as int64_t
inline constexpr uint32_t kSchemaStoreAreaAsInt64 = 131;

// Revision where dbAlignmentMarkerRule was added
inline constexpr uint32_t kSchemaChipAlignmentMarkerRule = 130;

// Revision where _dbTechLayerAntennaRule was modified to use ARuleRatio for
// gate_plus_diff
inline constexpr uint32_t kSchemaLef58AntennaGatePlusDiff = 129;

// Revision where dbChipPath was added to dbChip
inline constexpr uint32_t kSchemaChipPath = 128;

// Revision where chip_bump_ back-reference was added to dbBTerm
inline constexpr uint32_t kSchemaBtermChipBump = 127;

// Revision where dbTechLayer::wrong_way_min_width_ was added
inline constexpr uint32_t kSchemaTechLayerMinWidthWrongway = 126;

// Revision where dbTechLayer::voltage_spacings_ was added
inline constexpr uint32_t kSchemaVoltageSpacingTables = 125;

// Revision where _dbDatabase::hierarchy_ was added
inline constexpr uint32_t kSchemaHierarchyFlag = 124;

// Revision where dbMarkerCategory was moved from dbBlock to dbChip
inline constexpr uint32_t kSchemaChipMarkerCategories = 123;

// Revision where dbTech::dbu_per_micron_ was removed
inline constexpr uint32_t kSchemaRemoveDbuPerMicron = 122;

// Revision where core area is stored as a polygon
inline constexpr uint32_t kSchemaCoreAreaIsPolygon = 121;

// Revision where _dbDatabase::dbu_per_micron_ was added
inline constexpr uint32_t kSchemaDbuPerMicron = 120;

// Revision where dbChipBump was added
inline constexpr uint32_t kSchemaChipBump = 117;

// Revision where dbChipRegion was added
inline constexpr uint32_t kSchemaChipRegion = 116;

// Revision where dbChipInst was added
inline constexpr uint32_t kSchemaChipInst = 115;

// Revision where dbChip was extended with new fields
inline constexpr uint32_t kSchemaChipExtended = 112;

// Revision where _dbTechLayer::max_spacing_rules_tbl_ was added
inline constexpr uint32_t kSchemaMaxSpacing = 87;

// Revision where _dbTechLayer::two_wires_forbidden_spc_rules_tbl_ was added
inline constexpr uint32_t kSchemaLef58TwoWiresForbiddenSpacing = 82;

// Revision where _dbTechLayer::wrongdir_spacing_rules_tbl_ was added
inline constexpr uint32_t kSchemaWrongdirSpacing = 75;

// Revision where _dbTechLayer::forbidden_spacing_rules_tbl_ was added
inline constexpr uint32_t kSchemaLef58ForbiddenSpacing = 70;

// Revision where _dbTechLayer::keepout_zone_rules_tbl_ was added
inline constexpr uint32_t kSchemaKeepoutZone = 63;

// User Code End Consts
class dbIStream;
class dbOStream;
class _dbAlignmentMarkerRule;
class _dbChip;
class _dbProperty;
class _dbChipInst;
class _dbChipRegionInst;
class _dbChipConn;
class _dbChipBumpInst;
class _dbChipNet;
class _dbUnfoldedChipInst;
class _dbUnfoldedChipRegionInst;
class _dbUnfoldedChipBumpInst;
class _dbUnfoldedChipConn;
class _dbUnfoldedChipNet;
// User Code Begin Classes
class dbPropertyItr;
class dbChipInstItr;
class dbChipRegionInstItr;
class dbChipConnItr;
class dbChipBumpInstItr;
class dbChipNetItr;
class dbUnfoldedChipRegionInstItr;
class dbUnfoldedChipBumpInstItr;
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
  bool isSchema(uint32_t rev) const { return schema_minor_ >= rev; }
  bool isLessThanSchema(uint32_t rev) { return schema_minor_ < rev; }
  // User Code End Methods

  uint32_t magic1_;
  uint32_t magic2_;
  uint32_t schema_major_;
  uint32_t schema_minor_;
  uint32_t master_id_;
  dbId<_dbChip> chip_;
  uint32_t dbu_per_micron_;
  dbTable<_dbAlignmentMarkerRule>* alignment_marker_rule_tbl_;
  dbTable<_dbChip, 2>* chip_tbl_;
  dbHashTable<_dbChip, 2> chip_hash_;
  dbTable<_dbProperty>* prop_tbl_;
  dbTable<_dbChipInst>* chip_inst_tbl_;
  dbTable<_dbChipRegionInst>* chip_region_inst_tbl_;
  dbTable<_dbChipConn>* chip_conn_tbl_;
  dbTable<_dbChipBumpInst>* chip_bump_inst_tbl_;
  dbTable<_dbChipNet>* chip_net_tbl_;
  dbTable<_dbUnfoldedChipInst>* unfolded_chip_inst_tbl_;
  dbTable<_dbUnfoldedChipRegionInst>* unfolded_chip_region_inst_tbl_;
  dbTable<_dbUnfoldedChipBumpInst>* unfolded_chip_bump_inst_tbl_;
  dbTable<_dbUnfoldedChipConn>* unfolded_chip_conn_tbl_;
  dbTable<_dbUnfoldedChipNet>* unfolded_chip_net_tbl_;

  // User Code Begin Fields
  dbTable<_dbTech, 2>* tech_tbl_;
  dbTable<_dbLib>* lib_tbl_;
  dbTable<_dbGDSLib, 2>* gds_lib_tbl_;
  _dbNameCache* name_cache_;
  dbPropertyItr* prop_itr_;
  dbChipInstItr* chip_inst_itr_;
  dbChipRegionInstItr* chip_region_inst_itr_;
  dbChipConnItr* chip_conn_itr_;
  dbChipBumpInstItr* chip_bump_inst_itr_;
  dbUnfoldedChipRegionInstItr* unfolded_region_itr_;
  dbUnfoldedChipBumpInstItr* unfolded_bump_itr_;
  dbChipNetItr* chip_net_itr_;
  int unique_id_;
  bool hierarchy_;

  utl::Logger* logger_;
  std::set<dbDatabaseObserver*> observers_;

  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbDatabase& obj);
dbOStream& operator<<(dbOStream& stream, const _dbDatabase& obj);
}  // namespace odb
// Generator Code End Header
