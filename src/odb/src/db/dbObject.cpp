// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/dbObject.h"

#include <cstdint>
#include <cstring>
#include <iterator>
#include <string>
#include <unordered_map>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbProperty.h"
#include "dbTable.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace odb {

uint32_t dbObject::getId() const
{
  return getImpl()->getOID();
}

dbObjectType dbObject::getObjectType() const
{
  return getImpl()->getType();
}

//
// NOTE: The name declaration order MUST match the enumeration order.
//
static const char* name_tbl[] = {"dbGDSLib",
                                 "dbBlock",
                                 "dbInstHdr",
                                 "dbInst",
                                 "dbNet",
                                 "dbBTerm",
                                 "dbITerm",
                                 "dbBox",
                                 "dbVia",
                                 "dbTrackGrid",
                                 "dbObstruction",
                                 "dbBlockage",
                                 "dbWire",
                                 "dbSWire",
                                 "dbSBox",
                                 "dbCapNode",
                                 "dbRSeg",
                                 "dbCCSeg",
                                 "dbRow",
                                 "dbFill",
                                 "dbRegion",
                                 "dbHier",
                                 "dbBPin",
                                 // Generator Code Begin ObjectNames
                                 "dbAccessPoint",
                                 "dbBusPort",
                                 "dbCellEdgeSpacing",
                                 "dbChip",
                                 "dbChipBump",
                                 "dbChipBumpInst",
                                 "dbChipConn",
                                 "dbChipInst",
                                 "dbChipNet",
                                 "dbChipRegion",
                                 "dbChipRegionInst",
                                 "dbDatabase",
                                 "dbDft",
                                 "dbGCellGrid",
                                 "dbGDSARef",
                                 "dbGDSBoundary",
                                 "dbGDSBox",
                                 "dbGDSPath",
                                 "dbGDSSRef",
                                 "dbGDSStructure",
                                 "dbGDSText",
                                 "dbGlobalConnect",
                                 "dbGroup",
                                 "dbGuide",
                                 "dbIsolation",
                                 "dbLevelShifter",
                                 "dbLogicPort",
                                 "dbMarker",
                                 "dbMarkerCategory",
                                 "dbMasterEdgeType",
                                 "dbMetalWidthViaMap",
                                 "dbModBTerm",
                                 "dbModInst",
                                 "dbModITerm",
                                 "dbModNet",
                                 "dbModule",
                                 "dbNetTrack",
                                 "dbPolygon",
                                 "dbPowerDomain",
                                 "dbPowerSwitch",
                                 "dbProperty",
                                 "dbScanChain",
                                 "dbScanInst",
                                 "dbScanList",
                                 "dbScanPartition",
                                 "dbScanPin",
                                 "dbTechLayer",
                                 "dbTechLayerAreaRule",
                                 "dbTechLayerArraySpacingRule",
                                 "dbTechLayerCornerSpacingRule",
                                 "dbTechLayerCutClassRule",
                                 "dbTechLayerCutEnclosureRule",
                                 "dbTechLayerCutSpacingRule",
                                 "dbTechLayerCutSpacingTableDefRule",
                                 "dbTechLayerCutSpacingTableOrthRule",
                                 "dbTechLayerEolExtensionRule",
                                 "dbTechLayerEolKeepOutRule",
                                 "dbTechLayerForbiddenSpacingRule",
                                 "dbTechLayerKeepOutZoneRule",
                                 "dbTechLayerMaxSpacingRule",
                                 "dbTechLayerMinCutRule",
                                 "dbTechLayerMinStepRule",
                                 "dbTechLayerSpacingEolRule",
                                 "dbTechLayerSpacingTablePrlRule",
                                 "dbTechLayerTwoWiresForbiddenSpcRule",
                                 "dbTechLayerVoltageSpacing",
                                 "dbTechLayerWidthTableRule",
                                 "dbTechLayerWrongDirSpacingRule",
                                 // Generator Code End ObjectNames

                                 // Lib Objects
                                 "dbLib",
                                 "dbSite",
                                 "dbMaster",
                                 "dbMPin",
                                 "dbMTerm",
                                 "dbTechAntennaPinModel",

                                 // Tech Objects
                                 "dbTech",
                                 "dbTechVia",
                                 "dbTechNonDefaultRule",
                                 "dbTechLayerRule",
                                 "dbTechSameNetRule",
                                 "dbTechLayerSpacingRule",
                                 "dbTechMinCutRule",
                                 "dbTechMinEncRule",
                                 "dbTechV55InfluenceEntry",
                                 "dbTechLayerAntennaRule",
                                 "dbTechViaRule",
                                 "dbTechViaGenerateRule",
                                 "dbTechViaLayerRule",

                                 "dbName"};

static const std::unordered_map<uint32_t, dbObjectType> hash_to_object_type
    = {{0x2, dbGdsLibObj},
       {0x3, dbBlockObj},
       {0x4, dbInstHdrObj},
       {0x5, dbInstObj},
       {0x6, dbNetObj},
       {0x7, dbBTermObj},
       {0x8, dbITermObj},
       {0x9, dbBoxObj},
       {0xA, dbViaObj},
       {0xB, dbTrackGridObj},
       {0xC, dbObstructionObj},
       {0xD, dbBlockageObj},
       {0xE, dbWireObj},
       {0xF, dbSWireObj},
       {0x10, dbSBoxObj},
       {0x11, dbCapNodeObj},
       {0x12, dbRSegObj},
       {0x13, dbCCSegObj},
       {0x14, dbRowObj},
       {0x15, dbFillObj},
       {0x16, dbRegionObj},
       {0x17, dbHierObj},
       {0x18, dbBPinObj},
       // Generator Code Begin HashToObjectType
       {0x663302D5, dbAccessPointObj},
       {0x12B22B2C, dbBusPortObj},
       {0xEE4BAB67, dbCellEdgeSpacingObj},
       {0x00000001, dbChipObj},
       {0xEC4D30D9, dbChipBumpObj},
       {0xC3302A8D, dbChipBumpInstObj},
       {0x9FEC713B, dbChipConnObj},
       {0x09A169FD, dbChipInstObj},
       {0x972E397C, dbChipNetObj},
       {0x0676E6F1, dbChipRegionObj},
       {0x457A83E5, dbChipRegionInstObj},
       {0x00000000, dbDatabaseObj},
       {0x7C713BD7, dbDftObj},
       {0x645E6090, dbGCellGridObj},
       {0xCC5EB4AD, dbGDSARefObj},
       {0xE4D136A3, dbGDSBoundaryObj},
       {0x2A7E2FFE, dbGDSBoxObj},
       {0xE38B6A0A, dbGDSPathObj},
       {0x85F207E7, dbGDSSRefObj},
       {0x99297302, dbGDSStructureObj},
       {0x90DFB402, dbGDSTextObj},
       {0xA0D52ACE, dbGlobalConnectObj},
       {0x7A16F67A, dbGroupObj},
       {0xE76AA1E9, dbGuideObj},
       {0xB6738B19, dbIsolationObj},
       {0x861F1D0A, dbLevelShifterObj},
       {0x2B46CE60, dbLogicPortObj},
       {0xEB83E7A1, dbMarkerObj},
       {0x4D3FFF1D, dbMarkerCategoryObj},
       {0x05DBC7F0, dbMasterEdgeTypeObj},
       {0x0FD3C974, dbMetalWidthViaMapObj},
       {0x7EBE8F33, dbModBTermObj},
       {0xAFA31C35, dbModInstObj},
       {0xFAC1E1FA, dbModITermObj},
       {0xDFD34354, dbModNetObj},
       {0x9F568C6F, dbModuleObj},
       {0x6D3EE435, dbNetTrackObj},
       {0x110F3173, dbPolygonObj},
       {0x7352FE40, dbPowerDomainObj},
       {0x7D280800, dbPowerSwitchObj},
       {0x00000066, dbPropertyObj},
       {0xBF9698BF, dbScanChainObj},
       {0x05CD9D4A, dbScanInstObj},
       {0xD398101E, dbScanListObj},
       {0x534639A0, dbScanPartitionObj},
       {0x2BA03C39, dbScanPinObj},
       {0x4ED3B51C, dbTechLayerObj},
       {0x2C0432EB, dbTechLayerAreaRuleObj},
       {0x1904C344, dbTechLayerArraySpacingRuleObj},
       {0x598158F4, dbTechLayerCornerSpacingRuleObj},
       {0xE1F72282, dbTechLayerCutClassRuleObj},
       {0xFFC5F736, dbTechLayerCutEnclosureRuleObj},
       {0xB78E788B, dbTechLayerCutSpacingRuleObj},
       {0x79DFAE70, dbTechLayerCutSpacingTableDefRuleObj},
       {0xA5373824, dbTechLayerCutSpacingTableOrthRuleObj},
       {0x49B1981B, dbTechLayerEolExtensionRuleObj},
       {0x88B0E445, dbTechLayerEolKeepOutRuleObj},
       {0x139CF020, dbTechLayerForbiddenSpacingRuleObj},
       {0x0AD77253, dbTechLayerKeepOutZoneRuleObj},
       {0xA65D53B9, dbTechLayerMaxSpacingRuleObj},
       {0x4BA55CDE, dbTechLayerMinCutRuleObj},
       {0x6BE58714, dbTechLayerMinStepRuleObj},
       {0x49E7A7BD, dbTechLayerSpacingEolRuleObj},
       {0xA223F41D, dbTechLayerSpacingTablePrlRuleObj},
       {0x7C5FB405, dbTechLayerTwoWiresForbiddenSpcRuleObj},
       {0x690396A7, dbTechLayerVoltageSpacingObj},
       {0x7BF3D392, dbTechLayerWidthTableRuleObj},
       {0xF73FA7DF, dbTechLayerWrongDirSpacingRuleObj},
       // Generator Code End HashToObjectType

       // Lib Objects
       {0x52, dbLibObj},
       {0x53, dbGdsLibObj},
       {0x54, dbSiteObj},
       {0x55, dbMasterObj},
       {0x56, dbMPinObj},
       {0x57, dbMTermObj},
       {0x58, dbTechAntennaPinModelObj},

       // Tech Objects
       {0x59, dbTechObj},
       {0x5A, dbTechViaObj},
       {0x5B, dbTechNonDefaultRuleObj},
       {0x5C, dbTechLayerRuleObj},
       {0x5D, dbTechSameNetRuleObj},
       {0x5E, dbTechLayerSpacingRuleObj},
       {0x5F, dbTechMinCutRuleObj},
       {0x60, dbTechMinEncRuleObj},
       {0x61, dbTechV55InfluenceEntryObj},
       {0x62, dbTechLayerAntennaRuleObj},
       {0x63, dbTechViaRuleObj},
       {0x64, dbTechViaGenerateRuleObj},
       {0x65, dbTechViaLayerRuleObj},

       {0x67, dbNameObj}};

static const std::unordered_map<dbObjectType, uint32_t> object_type_to_hash
    = {{dbGdsLibObj, 0x2},
       {dbBlockObj, 0x3},
       {dbInstHdrObj, 0x4},
       {dbInstObj, 0x5},
       {dbNetObj, 0x6},
       {dbBTermObj, 0x7},
       {dbITermObj, 0x8},
       {dbBoxObj, 0x9},
       {dbViaObj, 0xA},
       {dbTrackGridObj, 0xB},
       {dbObstructionObj, 0xC},
       {dbBlockageObj, 0xD},
       {dbWireObj, 0xE},
       {dbSWireObj, 0xF},
       {dbSBoxObj, 0x10},
       {dbCapNodeObj, 0x11},
       {dbRSegObj, 0x12},
       {dbCCSegObj, 0x13},
       {dbRowObj, 0x14},
       {dbFillObj, 0x15},
       {dbRegionObj, 0x16},
       {dbHierObj, 0x17},
       {dbBPinObj, 0x18},
       // Generator Code Begin ObjectTypeToHash
       {dbAccessPointObj, 0x663302D5},
       {dbBusPortObj, 0x12B22B2C},
       {dbCellEdgeSpacingObj, 0xEE4BAB67},
       {dbChipObj, 0x00000001},
       {dbChipBumpObj, 0xEC4D30D9},
       {dbChipBumpInstObj, 0xC3302A8D},
       {dbChipConnObj, 0x9FEC713B},
       {dbChipInstObj, 0x09A169FD},
       {dbChipNetObj, 0x972E397C},
       {dbChipRegionObj, 0x0676E6F1},
       {dbChipRegionInstObj, 0x457A83E5},
       {dbDatabaseObj, 0x00000000},
       {dbDftObj, 0x7C713BD7},
       {dbGCellGridObj, 0x645E6090},
       {dbGDSARefObj, 0xCC5EB4AD},
       {dbGDSBoundaryObj, 0xE4D136A3},
       {dbGDSBoxObj, 0x2A7E2FFE},
       {dbGDSPathObj, 0xE38B6A0A},
       {dbGDSSRefObj, 0x85F207E7},
       {dbGDSStructureObj, 0x99297302},
       {dbGDSTextObj, 0x90DFB402},
       {dbGlobalConnectObj, 0xA0D52ACE},
       {dbGroupObj, 0x7A16F67A},
       {dbGuideObj, 0xE76AA1E9},
       {dbIsolationObj, 0xB6738B19},
       {dbLevelShifterObj, 0x861F1D0A},
       {dbLogicPortObj, 0x2B46CE60},
       {dbMarkerObj, 0xEB83E7A1},
       {dbMarkerCategoryObj, 0x4D3FFF1D},
       {dbMasterEdgeTypeObj, 0x05DBC7F0},
       {dbMetalWidthViaMapObj, 0x0FD3C974},
       {dbModBTermObj, 0x7EBE8F33},
       {dbModInstObj, 0xAFA31C35},
       {dbModITermObj, 0xFAC1E1FA},
       {dbModNetObj, 0xDFD34354},
       {dbModuleObj, 0x9F568C6F},
       {dbNetTrackObj, 0x6D3EE435},
       {dbPolygonObj, 0x110F3173},
       {dbPowerDomainObj, 0x7352FE40},
       {dbPowerSwitchObj, 0x7D280800},
       {dbPropertyObj, 0x00000066},
       {dbScanChainObj, 0xBF9698BF},
       {dbScanInstObj, 0x05CD9D4A},
       {dbScanListObj, 0xD398101E},
       {dbScanPartitionObj, 0x534639A0},
       {dbScanPinObj, 0x2BA03C39},
       {dbTechLayerObj, 0x4ED3B51C},
       {dbTechLayerAreaRuleObj, 0x2C0432EB},
       {dbTechLayerArraySpacingRuleObj, 0x1904C344},
       {dbTechLayerCornerSpacingRuleObj, 0x598158F4},
       {dbTechLayerCutClassRuleObj, 0xE1F72282},
       {dbTechLayerCutEnclosureRuleObj, 0xFFC5F736},
       {dbTechLayerCutSpacingRuleObj, 0xB78E788B},
       {dbTechLayerCutSpacingTableDefRuleObj, 0x79DFAE70},
       {dbTechLayerCutSpacingTableOrthRuleObj, 0xA5373824},
       {dbTechLayerEolExtensionRuleObj, 0x49B1981B},
       {dbTechLayerEolKeepOutRuleObj, 0x88B0E445},
       {dbTechLayerForbiddenSpacingRuleObj, 0x139CF020},
       {dbTechLayerKeepOutZoneRuleObj, 0x0AD77253},
       {dbTechLayerMaxSpacingRuleObj, 0xA65D53B9},
       {dbTechLayerMinCutRuleObj, 0x4BA55CDE},
       {dbTechLayerMinStepRuleObj, 0x6BE58714},
       {dbTechLayerSpacingEolRuleObj, 0x49E7A7BD},
       {dbTechLayerSpacingTablePrlRuleObj, 0xA223F41D},
       {dbTechLayerTwoWiresForbiddenSpcRuleObj, 0x7C5FB405},
       {dbTechLayerVoltageSpacingObj, 0x690396A7},
       {dbTechLayerWidthTableRuleObj, 0x7BF3D392},
       {dbTechLayerWrongDirSpacingRuleObj, 0xF73FA7DF},
       // Generator Code End ObjectTypeToHash

       // Lib Objects
       {dbLibObj, 0x52},
       {dbSiteObj, 0x54},
       {dbMasterObj, 0x55},
       {dbMPinObj, 0x56},
       {dbMTermObj, 0x57},
       {dbTechAntennaPinModelObj, 0x58},

       // Tech Objects
       {dbTechObj, 0x59},
       {dbTechViaObj, 0x5A},
       {dbTechNonDefaultRuleObj, 0x5B},
       {dbTechLayerRuleObj, 0x5C},
       {dbTechSameNetRuleObj, 0x5D},
       {dbTechLayerSpacingRuleObj, 0x5E},
       {dbTechMinCutRuleObj, 0x5F},
       {dbTechMinEncRuleObj, 0x60},
       {dbTechV55InfluenceEntryObj, 0x61},
       {dbTechLayerAntennaRuleObj, 0x62},
       {dbTechViaRuleObj, 0x63},
       {dbTechViaGenerateRuleObj, 0x64},
       {dbTechViaLayerRuleObj, 0x65},

       {dbNameObj, 0x67}};

const char* dbObject::getTypeName() const
{
  return name_tbl[getImpl()->getType()];
}

const char* dbObject::getTypeName(dbObjectType type)
{
  return name_tbl[type];
}

dbObjectType dbObject::getType(const char* name, utl::Logger* logger)
{
  std::size_t i = 0;
  for (const char* type_name : name_tbl) {
    if (strcmp(type_name, name) == 0) {
      return (dbObjectType) i;
    }

    i++;
  }

  logger->error(utl::ODB, 267, "Unable to find {} object type", name);
}

// We have to compare the id not only of this object but all its
// owning objects to properly compare.  For example dbMTerm is owned
// by dbMaster so two mterms could have the same id within the scope
// of different masters.
bool compare_by_id(const dbObject* lhs, const dbObject* rhs)
{
  if (lhs == nullptr || rhs == nullptr) {
    return lhs < rhs;
  }
  const auto lhs_id = lhs->getId();
  const auto rhs_id = rhs->getId();
  if (lhs_id != rhs_id) {
    return lhs_id < rhs_id;
  }
  const auto lhs_owner = lhs->getImpl()->getOwner();
  const auto rhs_owner = rhs->getImpl()->getOwner();
  return compare_by_id(lhs_owner, rhs_owner);
}

std::string dbObject::getName() const
{
  switch (getObjectType()) {
    case dbNetObj:
      return static_cast<const dbNet*>(this)->getName();
    case dbModNetObj:
      return static_cast<const dbModNet*>(this)->getHierarchicalName();
    case dbITermObj:
      return static_cast<const dbITerm*>(this)->getName();
    case dbBTermObj:
      return static_cast<const dbBTerm*>(this)->getName();
    case dbModITermObj:
      return static_cast<const dbModITerm*>(this)->getHierarchicalName();
    case dbModBTermObj:
      return static_cast<const dbModBTerm*>(this)->getHierarchicalName();
    case dbInstObj:
      return static_cast<const dbInst*>(this)->getName();
    case dbModuleObj:
      return static_cast<const dbModule*>(this)->getHierarchicalName();
    case dbModInstObj:
      return static_cast<const dbModInst*>(this)->getHierarchicalName();
    default:
      return fmt::format("<{}:{}>", getTypeName(), getId());
  }
}

std::string dbObject::getDebugName() const
{
  fmt::memory_buffer buf;

  // dbObject type
  fmt::format_to(std::back_inserter(buf), "{}({}", getTypeName(), getId());

  // dbObject pointer address if needed
  if (getImpl()->getLogger()->debugCheck(utl::ODB, "dump_pointer", 1)) {
    fmt::format_to(
        std::back_inserter(buf), ", {}", static_cast<const void*>(this));
  }

  // dbObject name
  fmt::format_to(std::back_inserter(buf), ", '{}')", getName());
  return fmt::to_string(buf);
}

bool dbObject::isValid() const
{
  return getImpl()->isValid();
}

dbIStream& operator>>(dbIStream& stream, dbObjectType& type)
{
  uint32_t hash;
  stream >> hash;
  type = hash_to_object_type.at(hash);
  return stream;
}

dbOStream& operator<<(dbOStream& stream, dbObjectType type)
{
  stream << object_type_to_hash.at(type);
  return stream;
}

}  // namespace odb
