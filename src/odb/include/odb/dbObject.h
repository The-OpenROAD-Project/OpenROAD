// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb.h"

namespace utl {
class Logger;
}

namespace odb {

///
/// When adding a new database object, you must add a dbObjectType enumerator
/// and edit dbObject.cpp and assign an unique "character" code for its
/// database-name.
///
class _dbDatabase;
class dbOStream;
class dbIStream;
class dbObjectPage;
class dbObjectTable;
class _dbObject;

///
/// Steps to add new objects -
///      - add the "name_tbl" entry in dbObject.cpp
///      - add the table entry to the getObjectTable method of the container
///      object.
///
enum dbObjectType
{
  dbDatabaseObj,

  // Design Objects
  dbChipObj,
  dbGdsLibObj,
  dbBlockObj,
  dbInstHdrObj,
  dbInstObj,
  dbNetObj,
  dbBTermObj,
  dbITermObj,
  dbBoxObj,
  dbViaObj,
  dbTrackGridObj,
  dbObstructionObj,
  dbBlockageObj,
  dbWireObj,
  dbSWireObj,
  dbSBoxObj,
  dbCapNodeObj,
  dbRSegObj,
  dbCCSegObj,
  dbRowObj,
  dbFillObj,
  dbRegionObj,
  dbHierObj,
  dbBPinObj,
  // Generator Code Begin DbObjectType
  dbAccessPointObj,
  dbBusPortObj,
  dbCellEdgeSpacingObj,
  dbDftObj,
  dbGCellGridObj,
  dbGDSARefObj,
  dbGDSBoundaryObj,
  dbGDSBoxObj,
  dbGDSPathObj,
  dbGDSSRefObj,
  dbGDSStructureObj,
  dbGDSTextObj,
  dbGlobalConnectObj,
  dbGroupObj,
  dbGuideObj,
  dbIsolationObj,
  dbLevelShifterObj,
  dbLogicPortObj,
  dbMarkerObj,
  dbMarkerCategoryObj,
  dbMasterEdgeTypeObj,
  dbMetalWidthViaMapObj,
  dbModBTermObj,
  dbModInstObj,
  dbModITermObj,
  dbModNetObj,
  dbModuleObj,
  dbNetTrackObj,
  dbPolygonObj,
  dbPowerDomainObj,
  dbPowerSwitchObj,
  dbScanChainObj,
  dbScanInstObj,
  dbScanListObj,
  dbScanPartitionObj,
  dbScanPinObj,
  dbTechLayerObj,
  dbTechLayerAreaRuleObj,
  dbTechLayerArraySpacingRuleObj,
  dbTechLayerCornerSpacingRuleObj,
  dbTechLayerCutClassRuleObj,
  dbTechLayerCutEnclosureRuleObj,
  dbTechLayerCutSpacingRuleObj,
  dbTechLayerCutSpacingTableDefRuleObj,
  dbTechLayerCutSpacingTableOrthRuleObj,
  dbTechLayerEolExtensionRuleObj,
  dbTechLayerEolKeepOutRuleObj,
  dbTechLayerForbiddenSpacingRuleObj,
  dbTechLayerKeepOutZoneRuleObj,
  dbTechLayerMaxSpacingRuleObj,
  dbTechLayerMinCutRuleObj,
  dbTechLayerMinStepRuleObj,
  dbTechLayerSpacingEolRuleObj,
  dbTechLayerSpacingTablePrlRuleObj,
  dbTechLayerTwoWiresForbiddenSpcRuleObj,
  dbTechLayerWidthTableRuleObj,
  dbTechLayerWrongDirSpacingRuleObj,
  // Generator Code End DbObjectType

  // Lib Objects
  dbLibObj,
  dbGDSLibObj,
  dbSiteObj,
  dbMasterObj,
  dbMPinObj,
  dbMTermObj,
  dbTechAntennaPinModelObj,

  // Tech Objects
  dbTechObj,
  dbTechViaObj,
  dbTechNonDefaultRuleObj,  // also a design object
  dbTechLayerRuleObj,       // also a design object
  dbTechSameNetRuleObj,
  dbTechLayerSpacingRuleObj,
  dbTechMinCutRuleObj,
  dbTechMinEncRuleObj,
  dbTechV55InfluenceEntryObj,
  dbTechLayerAntennaRuleObj,
  dbTechViaRuleObj,
  dbTechViaGenerateRuleObj,
  dbTechViaLayerRuleObj,

  // Property
  dbPropertyObj,
  dbNameObj
};

class dbDatabase;

class dbObject
{
 public:
  dbObjectType getObjectType() const;
  dbDatabase* getDb() const;
  uint getId() const;
  const char* getTypeName() const;

  static const char* getTypeName(dbObjectType type);
  static dbObjectType getType(const char* name, utl::Logger* logger);
  // These are not intended for client use as the returned class is
  // not exported.  They are for internal db convenience.
  _dbObject* getImpl();
  const _dbObject* getImpl() const;

 protected:
  dbObject() = default;
  ~dbObject() = default;
};

}  // namespace odb
