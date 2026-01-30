// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// IWYU pragma: private, include "db.h"

// Provide overloads of std::less for all instantiable, dbObject-derived types.
// This avoids pointer comparison which is a frequent source of non-determinism.
//
// Unfortunately this can't be done once on dbObject* as the gcc provides an
// overload for T* which specializes to the dervied class and wins overload
// resolution.

#include <functional>

#include "odb/dbObject.h"

namespace odb {

bool compare_by_id(const dbObject* lhs, const dbObject* rhs);

}  // namespace odb

namespace std {

template <>
struct less<odb::dbObject*>
{
  bool operator()(const odb::dbObject* lhs, const odb::dbObject* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbBoolProperty*>
{
  bool operator()(const odb::dbBoolProperty* lhs,
                  const odb::dbBoolProperty* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbStringProperty*>
{
  bool operator()(const odb::dbStringProperty* lhs,
                  const odb::dbStringProperty* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbIntProperty*>
{
  bool operator()(const odb::dbIntProperty* lhs,
                  const odb::dbIntProperty* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbDoubleProperty*>
{
  bool operator()(const odb::dbDoubleProperty* lhs,
                  const odb::dbDoubleProperty* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbBox*>
{
  bool operator()(const odb::dbBox* lhs, const odb::dbBox* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbSBox*>
{
  bool operator()(const odb::dbSBox* lhs, const odb::dbSBox* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbBlock*>
{
  bool operator()(const odb::dbBlock* lhs, const odb::dbBlock* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbBTerm*>
{
  bool operator()(const odb::dbBTerm* lhs, const odb::dbBTerm* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbBPin*>
{
  bool operator()(const odb::dbBPin* lhs, const odb::dbBPin* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbNet*>
{
  bool operator()(const odb::dbNet* lhs, const odb::dbNet* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbInst*>
{
  bool operator()(const odb::dbInst* lhs, const odb::dbInst* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbITerm*>
{
  bool operator()(const odb::dbITerm* lhs, const odb::dbITerm* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbVia*>
{
  bool operator()(const odb::dbVia* lhs, const odb::dbVia* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbWire*>
{
  bool operator()(const odb::dbWire* lhs, const odb::dbWire* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbSWire*>
{
  bool operator()(const odb::dbSWire* lhs, const odb::dbSWire* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTrackGrid*>
{
  bool operator()(const odb::dbTrackGrid* lhs,
                  const odb::dbTrackGrid* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbObstruction*>
{
  bool operator()(const odb::dbObstruction* lhs,
                  const odb::dbObstruction* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbBlockage*>
{
  bool operator()(const odb::dbBlockage* lhs, const odb::dbBlockage* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbCapNode*>
{
  bool operator()(const odb::dbCapNode* lhs, const odb::dbCapNode* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbRSeg*>
{
  bool operator()(const odb::dbRSeg* lhs, const odb::dbRSeg* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbCCSeg*>
{
  bool operator()(const odb::dbCCSeg* lhs, const odb::dbCCSeg* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbRow*>
{
  bool operator()(const odb::dbRow* lhs, const odb::dbRow* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbFill*>
{
  bool operator()(const odb::dbFill* lhs, const odb::dbFill* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbRegion*>
{
  bool operator()(const odb::dbRegion* lhs, const odb::dbRegion* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbLib*>
{
  bool operator()(const odb::dbLib* lhs, const odb::dbLib* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbSite*>
{
  bool operator()(const odb::dbSite* lhs, const odb::dbSite* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbMaster*>
{
  bool operator()(const odb::dbMaster* lhs, const odb::dbMaster* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbMTerm*>
{
  bool operator()(const odb::dbMTerm* lhs, const odb::dbMTerm* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbMPin*>
{
  bool operator()(const odb::dbMPin* lhs, const odb::dbMPin* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTech*>
{
  bool operator()(const odb::dbTech* lhs, const odb::dbTech* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechVia*>
{
  bool operator()(const odb::dbTechVia* lhs, const odb::dbTechVia* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechViaRule*>
{
  bool operator()(const odb::dbTechViaRule* lhs,
                  const odb::dbTechViaRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechViaLayerRule*>
{
  bool operator()(const odb::dbTechViaLayerRule* lhs,
                  const odb::dbTechViaLayerRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechViaGenerateRule*>
{
  bool operator()(const odb::dbTechViaGenerateRule* lhs,
                  const odb::dbTechViaGenerateRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerSpacingRule*>
{
  bool operator()(const odb::dbTechLayerSpacingRule* lhs,
                  const odb::dbTechLayerSpacingRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechMinCutRule*>
{
  bool operator()(const odb::dbTechMinCutRule* lhs,
                  const odb::dbTechMinCutRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechMinEncRule*>
{
  bool operator()(const odb::dbTechMinEncRule* lhs,
                  const odb::dbTechMinEncRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechV55InfluenceEntry*>
{
  bool operator()(const odb::dbTechV55InfluenceEntry* lhs,
                  const odb::dbTechV55InfluenceEntry* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerAntennaRule*>
{
  bool operator()(const odb::dbTechLayerAntennaRule* lhs,
                  const odb::dbTechLayerAntennaRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechAntennaPinModel*>
{
  bool operator()(const odb::dbTechAntennaPinModel* lhs,
                  const odb::dbTechAntennaPinModel* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechNonDefaultRule*>
{
  bool operator()(const odb::dbTechNonDefaultRule* lhs,
                  const odb::dbTechNonDefaultRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerRule*>
{
  bool operator()(const odb::dbTechLayerRule* lhs,
                  const odb::dbTechLayerRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechSameNetRule*>
{
  bool operator()(const odb::dbTechSameNetRule* lhs,
                  const odb::dbTechSameNetRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

// Generator Code Begin Less
template <>
struct less<odb::dbAccessPoint*>
{
  bool operator()(const odb::dbAccessPoint* lhs,
                  const odb::dbAccessPoint* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbBusPort*>
{
  bool operator()(const odb::dbBusPort* lhs, const odb::dbBusPort* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbCellEdgeSpacing*>
{
  bool operator()(const odb::dbCellEdgeSpacing* lhs,
                  const odb::dbCellEdgeSpacing* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbChip*>
{
  bool operator()(const odb::dbChip* lhs, const odb::dbChip* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbChipBump*>
{
  bool operator()(const odb::dbChipBump* lhs, const odb::dbChipBump* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbChipBumpInst*>
{
  bool operator()(const odb::dbChipBumpInst* lhs,
                  const odb::dbChipBumpInst* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbChipConn*>
{
  bool operator()(const odb::dbChipConn* lhs, const odb::dbChipConn* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbChipInst*>
{
  bool operator()(const odb::dbChipInst* lhs, const odb::dbChipInst* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbChipNet*>
{
  bool operator()(const odb::dbChipNet* lhs, const odb::dbChipNet* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbChipRegion*>
{
  bool operator()(const odb::dbChipRegion* lhs,
                  const odb::dbChipRegion* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbChipRegionInst*>
{
  bool operator()(const odb::dbChipRegionInst* lhs,
                  const odb::dbChipRegionInst* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbDatabase*>
{
  bool operator()(const odb::dbDatabase* lhs, const odb::dbDatabase* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbDft*>
{
  bool operator()(const odb::dbDft* lhs, const odb::dbDft* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbGCellGrid*>
{
  bool operator()(const odb::dbGCellGrid* lhs,
                  const odb::dbGCellGrid* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbGDSARef*>
{
  bool operator()(const odb::dbGDSARef* lhs, const odb::dbGDSARef* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbGDSBoundary*>
{
  bool operator()(const odb::dbGDSBoundary* lhs,
                  const odb::dbGDSBoundary* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbGDSBox*>
{
  bool operator()(const odb::dbGDSBox* lhs, const odb::dbGDSBox* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbGDSPath*>
{
  bool operator()(const odb::dbGDSPath* lhs, const odb::dbGDSPath* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbGDSSRef*>
{
  bool operator()(const odb::dbGDSSRef* lhs, const odb::dbGDSSRef* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbGDSStructure*>
{
  bool operator()(const odb::dbGDSStructure* lhs,
                  const odb::dbGDSStructure* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbGDSText*>
{
  bool operator()(const odb::dbGDSText* lhs, const odb::dbGDSText* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbGlobalConnect*>
{
  bool operator()(const odb::dbGlobalConnect* lhs,
                  const odb::dbGlobalConnect* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbGroup*>
{
  bool operator()(const odb::dbGroup* lhs, const odb::dbGroup* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbGuide*>
{
  bool operator()(const odb::dbGuide* lhs, const odb::dbGuide* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbIsolation*>
{
  bool operator()(const odb::dbIsolation* lhs,
                  const odb::dbIsolation* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbLevelShifter*>
{
  bool operator()(const odb::dbLevelShifter* lhs,
                  const odb::dbLevelShifter* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbLogicPort*>
{
  bool operator()(const odb::dbLogicPort* lhs,
                  const odb::dbLogicPort* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbMarker*>
{
  bool operator()(const odb::dbMarker* lhs, const odb::dbMarker* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbMarkerCategory*>
{
  bool operator()(const odb::dbMarkerCategory* lhs,
                  const odb::dbMarkerCategory* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbMasterEdgeType*>
{
  bool operator()(const odb::dbMasterEdgeType* lhs,
                  const odb::dbMasterEdgeType* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbMetalWidthViaMap*>
{
  bool operator()(const odb::dbMetalWidthViaMap* lhs,
                  const odb::dbMetalWidthViaMap* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbModBTerm*>
{
  bool operator()(const odb::dbModBTerm* lhs, const odb::dbModBTerm* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbModInst*>
{
  bool operator()(const odb::dbModInst* lhs, const odb::dbModInst* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbModITerm*>
{
  bool operator()(const odb::dbModITerm* lhs, const odb::dbModITerm* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbModNet*>
{
  bool operator()(const odb::dbModNet* lhs, const odb::dbModNet* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbModule*>
{
  bool operator()(const odb::dbModule* lhs, const odb::dbModule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbNetTrack*>
{
  bool operator()(const odb::dbNetTrack* lhs, const odb::dbNetTrack* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbPolygon*>
{
  bool operator()(const odb::dbPolygon* lhs, const odb::dbPolygon* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbPowerDomain*>
{
  bool operator()(const odb::dbPowerDomain* lhs,
                  const odb::dbPowerDomain* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbPowerSwitch*>
{
  bool operator()(const odb::dbPowerSwitch* lhs,
                  const odb::dbPowerSwitch* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbScanChain*>
{
  bool operator()(const odb::dbScanChain* lhs,
                  const odb::dbScanChain* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbScanInst*>
{
  bool operator()(const odb::dbScanInst* lhs, const odb::dbScanInst* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbScanList*>
{
  bool operator()(const odb::dbScanList* lhs, const odb::dbScanList* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbScanPartition*>
{
  bool operator()(const odb::dbScanPartition* lhs,
                  const odb::dbScanPartition* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbScanPin*>
{
  bool operator()(const odb::dbScanPin* lhs, const odb::dbScanPin* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayer*>
{
  bool operator()(const odb::dbTechLayer* lhs,
                  const odb::dbTechLayer* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerAreaRule*>
{
  bool operator()(const odb::dbTechLayerAreaRule* lhs,
                  const odb::dbTechLayerAreaRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerArraySpacingRule*>
{
  bool operator()(const odb::dbTechLayerArraySpacingRule* lhs,
                  const odb::dbTechLayerArraySpacingRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerCornerSpacingRule*>
{
  bool operator()(const odb::dbTechLayerCornerSpacingRule* lhs,
                  const odb::dbTechLayerCornerSpacingRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerCutClassRule*>
{
  bool operator()(const odb::dbTechLayerCutClassRule* lhs,
                  const odb::dbTechLayerCutClassRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerCutEnclosureRule*>
{
  bool operator()(const odb::dbTechLayerCutEnclosureRule* lhs,
                  const odb::dbTechLayerCutEnclosureRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerCutSpacingRule*>
{
  bool operator()(const odb::dbTechLayerCutSpacingRule* lhs,
                  const odb::dbTechLayerCutSpacingRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerCutSpacingTableDefRule*>
{
  bool operator()(const odb::dbTechLayerCutSpacingTableDefRule* lhs,
                  const odb::dbTechLayerCutSpacingTableDefRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerCutSpacingTableOrthRule*>
{
  bool operator()(const odb::dbTechLayerCutSpacingTableOrthRule* lhs,
                  const odb::dbTechLayerCutSpacingTableOrthRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerEolExtensionRule*>
{
  bool operator()(const odb::dbTechLayerEolExtensionRule* lhs,
                  const odb::dbTechLayerEolExtensionRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerEolKeepOutRule*>
{
  bool operator()(const odb::dbTechLayerEolKeepOutRule* lhs,
                  const odb::dbTechLayerEolKeepOutRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerForbiddenSpacingRule*>
{
  bool operator()(const odb::dbTechLayerForbiddenSpacingRule* lhs,
                  const odb::dbTechLayerForbiddenSpacingRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerKeepOutZoneRule*>
{
  bool operator()(const odb::dbTechLayerKeepOutZoneRule* lhs,
                  const odb::dbTechLayerKeepOutZoneRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerMaxSpacingRule*>
{
  bool operator()(const odb::dbTechLayerMaxSpacingRule* lhs,
                  const odb::dbTechLayerMaxSpacingRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerMinCutRule*>
{
  bool operator()(const odb::dbTechLayerMinCutRule* lhs,
                  const odb::dbTechLayerMinCutRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerMinStepRule*>
{
  bool operator()(const odb::dbTechLayerMinStepRule* lhs,
                  const odb::dbTechLayerMinStepRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerSpacingEolRule*>
{
  bool operator()(const odb::dbTechLayerSpacingEolRule* lhs,
                  const odb::dbTechLayerSpacingEolRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerSpacingTablePrlRule*>
{
  bool operator()(const odb::dbTechLayerSpacingTablePrlRule* lhs,
                  const odb::dbTechLayerSpacingTablePrlRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerTwoWiresForbiddenSpcRule*>
{
  bool operator()(const odb::dbTechLayerTwoWiresForbiddenSpcRule* lhs,
                  const odb::dbTechLayerTwoWiresForbiddenSpcRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerVoltageSpacing*>
{
  bool operator()(const odb::dbTechLayerVoltageSpacing* lhs,
                  const odb::dbTechLayerVoltageSpacing* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerWidthTableRule*>
{
  bool operator()(const odb::dbTechLayerWidthTableRule* lhs,
                  const odb::dbTechLayerWidthTableRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

template <>
struct less<odb::dbTechLayerWrongDirSpacingRule*>
{
  bool operator()(const odb::dbTechLayerWrongDirSpacingRule* lhs,
                  const odb::dbTechLayerWrongDirSpacingRule* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

// Generator Code End Less

}  // namespace std
