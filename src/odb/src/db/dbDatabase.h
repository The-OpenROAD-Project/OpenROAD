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
inline constexpr uint kSchemaMajor = 0;  // Not used...
inline constexpr uint kSchemaInitial = 57;

inline constexpr uint kSchemaMinor = 123;  // Current revision number

// Revision where dbMarkerCategory was moved from dbBlock to dbChip
inline constexpr uint kSchemaChipMarkerCategories = 123;

// Revision where dbTech::dbu_per_micron_ was removed
inline constexpr uint kSchemaRemoveDbuPerMicron = 122;

// Revision where core area is stored as a polygon
inline constexpr uint kSchemaCoreAreaIsPolygon = 121;

// Revision where _dbDatabase::dbu_per_micron_ was added
inline constexpr uint kSchemaDbuPerMicron = 120;

// Revision where dbGCellGrid::GCellData moved to float (for cugr)
inline constexpr uint kSchemaFloatGCellData = 119;

// Revision where dbTech was moved from dbBlock to dbChip
inline constexpr uint kSchemaChipTech = 118;

// Revision where dbChipBump was added
inline constexpr uint kSchemaChipBump = 117;

// Revision where dbChipRegion was added
inline constexpr uint kSchemaChipRegion = 116;

// Revision where dbChipInst was added
inline constexpr uint kSchemaChipInst = 115;

// Revision where dbChip hash table was added
inline constexpr uint kSchemaChipHashTable = 114;

// Revision where unique net/inst indices were added to dbBlock
inline constexpr uint kSchemaUniqueIndices = 113;

// Revision where dbChip was extended with new fields
inline constexpr uint kSchemaChipExtended = 112;

// Revision where the map which associates instances to their
// scan version was added
inline constexpr uint kSchemaMapInstsToScanInsts = 111;

// Revision where the ownership of the scan insts was changed
// from the its scan list to the block
inline constexpr uint kSchemaBlockOwnsScanInsts = 110;

// Revision where is_connect_to_term_ flag was added to dbGuide
inline constexpr uint kSchemaGuideConnectedToTerm = 109;

// Revision where dbTable's mask/shift are compile constants
inline constexpr uint kSchemaTableMaskShift = 108;

// Revision where dbBTerm top layer grid was added to dbBlock
inline constexpr uint kSchemaBtermTopLayerGrid = 107;

// Revision where die area is converted to a polygon
inline constexpr uint kSchemaDieAreaIsPolygon = 106;

// Revision where check for mirrored constraint on bterm was added
inline constexpr uint kSchemaBtermIsMirrored = 105;

// Revision where support for pin groups was added
inline constexpr uint kSchemaBlockPinGroups = 104;

// Revision where support for mirrored pins was added
inline constexpr uint kSchemaBtermMirroredPin = 103;

// Revision where support for LEF58_CELLEDGESPACINGTABLE was added
inline constexpr uint kSchemaCellEdgeSpcTbl = 102;

// Revision where dbMasterEdgeType was added
inline constexpr uint kSchemaMasterEdgeType = 101;

// Revision where dbTarget was removed
inline constexpr uint kSchemaRmTarget = 100;

// Revision where mask information was added to track grids
inline constexpr uint kSchemaTrackMask = 99;

// Revision where the jumper insertion flag is added to dbNet
inline constexpr uint kSchemaHasJumpers = 98;

// Revision where the is_congested flag was added to dbGuide
inline constexpr uint kSchemaDbGuideCongested = 97;

// Revision where the dbMarkerGroup/Categories were added to dbBlock
inline constexpr uint kSchemaDbMarkerGroup = 96;

// Revision where orthogonal spacing table support added
inline constexpr uint kSchemaOrthSpcTbl = 95;

// Revision where unused hashes removed
inline constexpr uint kSchemaDbRemoveHash = 94;

// Revision where the dbGDSLib is added to dbDatabase
inline constexpr uint kSchemaGdsLibInBlock = 93;

// Reverted Revision where unused hashes removed
inline constexpr uint kSchemaRevertedDbSchemaDbRemoveHash = 92;

// Revision where the layers ranges, for signals and clock nets,
// were moved from GlobalRouter to dbBlock
inline constexpr uint kSchemaDbBlockLayersRanges = 91;

// Revision where via layer was added to dbGuide
inline constexpr uint kSchemaDbGuideViaLayer = 90;

// Revision where blocked regions for IO pins were added to dbBlock
inline constexpr uint kSchemaDbBlockBlockedRegionsForPins = 89;

// Revision where odb::modITerm,modBTerm,modNet made doubly linked for
// hiearchical port removal
inline constexpr uint kSchemaHierPortRemoval = 89;

// Revision where odb::Polygon was added
inline constexpr uint kSchemaPolygon = 88;

// Revision where _dbTechLayer::max_spacing_rules_tbl_ was added
inline constexpr uint kSchemaMaxSpacing = 87;

// Revision where bus ports added to odb
inline constexpr uint kSchemaOdbBusport = 86;

// Revision where constraint region was added to dbBTerm
inline constexpr uint kSchemaBtermConstraintRegion = 85;

// Revision where GRT layer adjustment was relocated to dbTechLayer
inline constexpr uint kSchemaLayerAdjustment = 84;

// Revision where scan structs are added
inline constexpr uint kSchemaAddScan = 83;

// Revision where _dbTechLayer::two_wires_forbidden_spc_rules_tbl_ was added
inline constexpr uint kSchemaLef58TwoWiresForbiddenSpacing = 82;
// Revision where hierarchy schema with modnets, modbterms, moditerms introduced
inline constexpr uint kSchemaUpdateHierarchy = 81;
// Revision where dbPowerSwitch changed from strings to structs
inline constexpr uint kSchemaUpdateDbPowerSwitch = 80;

// Revision where dbGCellGrid::GCellData moved to uint8_t
inline constexpr uint kSchemaSmalerGcelldata = 79;

// Revision where _dbBox / flags.mask was added
inline constexpr uint kSchemaDbBoxMask = 78;

inline constexpr uint kSchemaLevelShifterCell = 77;

inline constexpr uint kSchemaPowerDomainVoltage = 76;

// Revision where _dbTechLayer::wrongdir_spacing_rules_tbl_ was added
inline constexpr uint kSchemaWrongdirSpacing = 75;

// Revision where _dbLevelShifter was added
inline constexpr uint kSchemaLevelShifter = 74;

// Revision where _dbSite::_row_pattern/_parent_lib/_parent_site were added
inline constexpr uint kSchemaSiteRowPattern = 73;

// Revision where _dbMaster::_lib_for_site was added
inline constexpr uint kSchemaDbmasterLibForSite = 72;

// Revision where _dbObstruction::_except_pg_nets was added
inline constexpr uint kSchemaExceptPgNetsObstruction = 71;

// Revision where _dbTechLayer::forbidden_spacing_rules_tbl_ was added
inline constexpr uint kSchemaLef58ForbiddenSpacing = 70;

// Revision where upf power switch mapping was added.
inline constexpr uint kSchemaUpfPowerSwitchMapping = 69;

// Revision where _component_shift_mask is added to _dbBlock.
inline constexpr uint kSchemaBlockComponentMaskShift = 68;

// Revision where _minExtModelIndex & _maxExtModelIndex removed from
// _dbBlock.
inline constexpr uint kSchemaBlockExtModelIndex = 67;

// Revision where _tech moved to _dbBlock & _dbLib from _dbDatabase.
// Added name to dbTech.
inline constexpr uint kSchemaBlockTech = 66;

// Revision where _dbGCellGrid switch to using dbMatrix
inline constexpr uint kSchemaGcellGridMatrix = 65;

// Revision where _dbBoxFlags shifted _mark bit to _layer_id
inline constexpr uint kSchemaBoxLayerBits = 64;

// Revision where _dbTechLayer::keepout_zone_rules_tbl_ was added
inline constexpr uint kSchemaKeepoutZone = 63;

// Revision where _dbBlock::_net_tracks_tbl was added
inline constexpr uint kSchemaNetTracks = 62;

// Revision where _dbTechLayer::_first_last_pitch was added
inline constexpr uint kSchemaLef58Pitch = 61;

// Revision where _dbTechLayer::wrong_way_width_ was added
inline constexpr uint kSchemaWrongwayWidth = 60;

// Revision where dbGlobalConnect was added
inline constexpr uint kSchemaAddGlobalConnect = 58;

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
  bool isSchema(uint rev) const { return schema_minor_ >= rev; }
  bool isLessThanSchema(uint rev) { return schema_minor_ < rev; }
  // User Code End Methods

  uint magic1_;
  uint magic2_;
  uint schema_major_;
  uint schema_minor_;
  uint master_id_;
  dbId<_dbChip> chip_;
  uint dbu_per_micron_;
  dbTable<_dbChip, 2>* chip_tbl_;
  dbHashTable<_dbChip, 2> chip_hash_;
  dbTable<_dbProperty>* prop_tbl_;
  dbTable<_dbChipInst>* chip_inst_tbl_;
  dbTable<_dbChipRegionInst>* chip_region_inst_tbl_;
  dbTable<_dbChipConn>* chip_conn_tbl_;
  dbTable<_dbChipBumpInst>* chip_bump_inst_tbl_;
  dbTable<_dbChipNet>* chip_net_tbl_;

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
  dbChipNetItr* chip_net_itr_;
  int unique_id_;

  utl::Logger* logger_;
  std::set<dbDatabaseObserver*> observers_;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbDatabase& obj);
dbOStream& operator<<(dbOStream& stream, const _dbDatabase& obj);
}  // namespace odb
// Generator Code End Header
