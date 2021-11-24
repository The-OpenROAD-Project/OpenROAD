// *****************************************************************************
// *****************************************************************************
// ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!
// *****************************************************************************
// *****************************************************************************
// Copyright 2012, Cadence Design Systems
// 
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8. 
// 
// Licensed under the Apache License, Version 2.0 (the \"License\");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an \"AS IS\" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
// 
// 
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
// 
//  $Author: xxx $
//  $Revision: xxx $
//  $Date: xxx $
//  $State: xxx $
// *****************************************************************************
// *****************************************************************************

#define EXTERN extern "C"

#include "defrReader.h"
#include "defrReader.hpp"

// Wrappers definitions.
int defrInit () {
    return LefDefParser::defrInit();
}

int defrInitSession (int  startSession) {
    return LefDefParser::defrInitSession(startSession);
}

int defrReset () {
    return LefDefParser::defrReset();
}

int defrClear () {
    return LefDefParser::defrClear();
}

void defrSetCommentChar (char  c) {
    LefDefParser::defrSetCommentChar(c);
}

void defrSetAddPathToNet () {
    LefDefParser::defrSetAddPathToNet();
}

void defrSetAllowComponentNets () {
    LefDefParser::defrSetAllowComponentNets();
}

int defrGetAllowComponentNets () {
    return LefDefParser::defrGetAllowComponentNets();
}

void defrSetCaseSensitivity (int  caseSense) {
    LefDefParser::defrSetCaseSensitivity(caseSense);
}

void defrSetRegisterUnusedCallbacks () {
    LefDefParser::defrSetRegisterUnusedCallbacks();
}

void defrPrintUnusedCallbacks (FILE*  log) {
    LefDefParser::defrPrintUnusedCallbacks(log);
}

int  defrReleaseNResetMemory () {
    return LefDefParser::defrReleaseNResetMemory();
}

int defrRead (FILE * file, const char * fileName, defiUserData  userData, int  case_sensitive) {
    return LefDefParser::defrRead(file, fileName, userData, case_sensitive);
}

void defrSetUserData (defiUserData p0) {
    LefDefParser::defrSetUserData(p0);
}

defiUserData defrGetUserData () {
    return LefDefParser::defrGetUserData();
}

void defrSetArrayNameCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetArrayNameCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetAssertionCbk (::defrAssertionCbkFnType p0) {
    LefDefParser::defrSetAssertionCbk((LefDefParser::defrAssertionCbkFnType) p0);
}

void defrSetAssertionsStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetAssertionsStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetAssertionsEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetAssertionsEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetBlockageCbk (::defrBlockageCbkFnType p0) {
    LefDefParser::defrSetBlockageCbk((LefDefParser::defrBlockageCbkFnType) p0);
}

void defrSetBlockageStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetBlockageStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetBlockageEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetBlockageEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetBusBitCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetBusBitCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetCannotOccupyCbk (::defrSiteCbkFnType p0) {
    LefDefParser::defrSetCannotOccupyCbk((LefDefParser::defrSiteCbkFnType) p0);
}

void defrSetCanplaceCbk (::defrSiteCbkFnType p0) {
    LefDefParser::defrSetCanplaceCbk((LefDefParser::defrSiteCbkFnType) p0);
}

void defrSetCaseSensitiveCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetCaseSensitiveCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetComponentCbk (::defrComponentCbkFnType p0) {
    LefDefParser::defrSetComponentCbk((LefDefParser::defrComponentCbkFnType) p0);
}

void defrSetComponentExtCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetComponentExtCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetComponentStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetComponentStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetComponentEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetComponentEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetConstraintCbk (::defrAssertionCbkFnType p0) {
    LefDefParser::defrSetConstraintCbk((LefDefParser::defrAssertionCbkFnType) p0);
}

void defrSetConstraintsStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetConstraintsStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetConstraintsEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetConstraintsEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetDefaultCapCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetDefaultCapCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetDesignCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetDesignCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetDesignEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetDesignEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetDieAreaCbk (::defrBoxCbkFnType p0) {
    LefDefParser::defrSetDieAreaCbk((LefDefParser::defrBoxCbkFnType) p0);
}

void defrSetDividerCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetDividerCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetExtensionCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetExtensionCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetFillCbk (::defrFillCbkFnType p0) {
    LefDefParser::defrSetFillCbk((LefDefParser::defrFillCbkFnType) p0);
}

void defrSetFillStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetFillStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetFillEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetFillEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetFPCCbk (::defrFPCCbkFnType p0) {
    LefDefParser::defrSetFPCCbk((LefDefParser::defrFPCCbkFnType) p0);
}

void defrSetFPCStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetFPCStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetFPCEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetFPCEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetFloorPlanNameCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetFloorPlanNameCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetGcellGridCbk (::defrGcellGridCbkFnType p0) {
    LefDefParser::defrSetGcellGridCbk((LefDefParser::defrGcellGridCbkFnType) p0);
}

void defrSetGroupNameCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetGroupNameCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetGroupMemberCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetGroupMemberCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetComponentMaskShiftLayerCbk (::defrComponentMaskShiftLayerCbkFnType p0) {
    LefDefParser::defrSetComponentMaskShiftLayerCbk((LefDefParser::defrComponentMaskShiftLayerCbkFnType) p0);
}

void defrSetGroupCbk (::defrGroupCbkFnType p0) {
    LefDefParser::defrSetGroupCbk((LefDefParser::defrGroupCbkFnType) p0);
}

void defrSetGroupExtCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetGroupExtCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetGroupsStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetGroupsStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetGroupsEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetGroupsEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetHistoryCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetHistoryCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetIOTimingCbk (::defrIOTimingCbkFnType p0) {
    LefDefParser::defrSetIOTimingCbk((LefDefParser::defrIOTimingCbkFnType) p0);
}

void defrSetIOTimingsStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetIOTimingsStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetIOTimingsEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetIOTimingsEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetIoTimingsExtCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetIoTimingsExtCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetNetCbk (::defrNetCbkFnType p0) {
    LefDefParser::defrSetNetCbk((LefDefParser::defrNetCbkFnType) p0);
}

void defrSetNetNameCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetNetNameCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetNetNonDefaultRuleCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetNetNonDefaultRuleCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetNetConnectionExtCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetNetConnectionExtCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetNetExtCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetNetExtCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetNetPartialPathCbk (::defrNetCbkFnType p0) {
    LefDefParser::defrSetNetPartialPathCbk((LefDefParser::defrNetCbkFnType) p0);
}

void defrSetNetSubnetNameCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetNetSubnetNameCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetNetStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetNetStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetNetEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetNetEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetNonDefaultCbk (::defrNonDefaultCbkFnType p0) {
    LefDefParser::defrSetNonDefaultCbk((LefDefParser::defrNonDefaultCbkFnType) p0);
}

void defrSetNonDefaultStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetNonDefaultStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetNonDefaultEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetNonDefaultEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetPartitionCbk (::defrPartitionCbkFnType p0) {
    LefDefParser::defrSetPartitionCbk((LefDefParser::defrPartitionCbkFnType) p0);
}

void defrSetPartitionsExtCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetPartitionsExtCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetPartitionsStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetPartitionsStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetPartitionsEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetPartitionsEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetPathCbk (::defrPathCbkFnType p0) {
    LefDefParser::defrSetPathCbk((LefDefParser::defrPathCbkFnType) p0);
}

void defrSetPinCapCbk (::defrPinCapCbkFnType p0) {
    LefDefParser::defrSetPinCapCbk((LefDefParser::defrPinCapCbkFnType) p0);
}

void defrSetPinCbk (::defrPinCbkFnType p0) {
    LefDefParser::defrSetPinCbk((LefDefParser::defrPinCbkFnType) p0);
}

void defrSetPinExtCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetPinExtCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetPinPropCbk (::defrPinPropCbkFnType p0) {
    LefDefParser::defrSetPinPropCbk((LefDefParser::defrPinPropCbkFnType) p0);
}

void defrSetPinPropStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetPinPropStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetPinPropEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetPinPropEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetPropCbk (::defrPropCbkFnType p0) {
    LefDefParser::defrSetPropCbk((LefDefParser::defrPropCbkFnType) p0);
}

void defrSetPropDefEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetPropDefEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetPropDefStartCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetPropDefStartCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetRegionCbk (::defrRegionCbkFnType p0) {
    LefDefParser::defrSetRegionCbk((LefDefParser::defrRegionCbkFnType) p0);
}

void defrSetRegionStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetRegionStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetRegionEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetRegionEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetRowCbk (::defrRowCbkFnType p0) {
    LefDefParser::defrSetRowCbk((LefDefParser::defrRowCbkFnType) p0);
}

void defrSetSNetCbk (::defrNetCbkFnType p0) {
    LefDefParser::defrSetSNetCbk((LefDefParser::defrNetCbkFnType) p0);
}

void defrSetSNetStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetSNetStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetSNetEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetSNetEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetSNetPartialPathCbk (::defrNetCbkFnType p0) {
    LefDefParser::defrSetSNetPartialPathCbk((LefDefParser::defrNetCbkFnType) p0);
}

void defrSetSNetWireCbk (::defrNetCbkFnType p0) {
    LefDefParser::defrSetSNetWireCbk((LefDefParser::defrNetCbkFnType) p0);
}

void defrSetScanChainExtCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetScanChainExtCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetScanchainCbk (::defrScanchainCbkFnType p0) {
    LefDefParser::defrSetScanchainCbk((LefDefParser::defrScanchainCbkFnType) p0);
}

void defrSetScanchainsStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetScanchainsStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetScanchainsEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetScanchainsEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetSiteCbk (::defrSiteCbkFnType p0) {
    LefDefParser::defrSetSiteCbk((LefDefParser::defrSiteCbkFnType) p0);
}

void defrSetSlotCbk (::defrSlotCbkFnType p0) {
    LefDefParser::defrSetSlotCbk((LefDefParser::defrSlotCbkFnType) p0);
}

void defrSetSlotStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetSlotStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetSlotEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetSlotEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetStartPinsCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetStartPinsCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetStylesCbk (::defrStylesCbkFnType p0) {
    LefDefParser::defrSetStylesCbk((LefDefParser::defrStylesCbkFnType) p0);
}

void defrSetStylesStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetStylesStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetStylesEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetStylesEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetPinEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetPinEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetTechnologyCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetTechnologyCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetTimingDisableCbk (::defrTimingDisableCbkFnType p0) {
    LefDefParser::defrSetTimingDisableCbk((LefDefParser::defrTimingDisableCbkFnType) p0);
}

void defrSetTimingDisablesStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetTimingDisablesStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetTimingDisablesEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetTimingDisablesEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrSetTrackCbk (::defrTrackCbkFnType p0) {
    LefDefParser::defrSetTrackCbk((LefDefParser::defrTrackCbkFnType) p0);
}

void defrSetUnitsCbk (::defrDoubleCbkFnType p0) {
    LefDefParser::defrSetUnitsCbk((LefDefParser::defrDoubleCbkFnType) p0);
}

void defrSetVersionCbk (::defrDoubleCbkFnType p0) {
    LefDefParser::defrSetVersionCbk((LefDefParser::defrDoubleCbkFnType) p0);
}

void defrSetVersionStrCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetVersionStrCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetViaCbk (::defrViaCbkFnType p0) {
    LefDefParser::defrSetViaCbk((LefDefParser::defrViaCbkFnType) p0);
}

void defrSetViaExtCbk (::defrStringCbkFnType p0) {
    LefDefParser::defrSetViaExtCbk((LefDefParser::defrStringCbkFnType) p0);
}

void defrSetViaStartCbk (::defrIntegerCbkFnType p0) {
    LefDefParser::defrSetViaStartCbk((LefDefParser::defrIntegerCbkFnType) p0);
}

void defrSetViaEndCbk (::defrVoidCbkFnType p0) {
    LefDefParser::defrSetViaEndCbk((LefDefParser::defrVoidCbkFnType) p0);
}

void defrUnsetCallbacks () {
    LefDefParser::defrUnsetCallbacks();
}

void defrUnsetArrayNameCbk () {
    LefDefParser::defrUnsetArrayNameCbk();
}

void defrUnsetAssertionCbk () {
    LefDefParser::defrUnsetAssertionCbk();
}

void defrUnsetAssertionsStartCbk () {
    LefDefParser::defrUnsetAssertionsStartCbk();
}

void defrUnsetAssertionsEndCbk () {
    LefDefParser::defrUnsetAssertionsEndCbk();
}

void defrUnsetBlockageCbk () {
    LefDefParser::defrUnsetBlockageCbk();
}

void defrUnsetBlockageStartCbk () {
    LefDefParser::defrUnsetBlockageStartCbk();
}

void defrUnsetBlockageEndCbk () {
    LefDefParser::defrUnsetBlockageEndCbk();
}

void defrUnsetBusBitCbk () {
    LefDefParser::defrUnsetBusBitCbk();
}

void defrUnsetCannotOccupyCbk () {
    LefDefParser::defrUnsetCannotOccupyCbk();
}

void defrUnsetCanplaceCbk () {
    LefDefParser::defrUnsetCanplaceCbk();
}

void defrUnsetCaseSensitiveCbk () {
    LefDefParser::defrUnsetCaseSensitiveCbk();
}

void defrUnsetComponentCbk () {
    LefDefParser::defrUnsetComponentCbk();
}

void defrUnsetComponentExtCbk () {
    LefDefParser::defrUnsetComponentExtCbk();
}

void defrUnsetComponentStartCbk () {
    LefDefParser::defrUnsetComponentStartCbk();
}

void defrUnsetComponentEndCbk () {
    LefDefParser::defrUnsetComponentEndCbk();
}

void defrUnsetConstraintCbk () {
    LefDefParser::defrUnsetConstraintCbk();
}

void defrUnsetConstraintsStartCbk () {
    LefDefParser::defrUnsetConstraintsStartCbk();
}

void defrUnsetConstraintsEndCbk () {
    LefDefParser::defrUnsetConstraintsEndCbk();
}

void defrUnsetDefaultCapCbk () {
    LefDefParser::defrUnsetDefaultCapCbk();
}

void defrUnsetDesignCbk () {
    LefDefParser::defrUnsetDesignCbk();
}

void defrUnsetDesignEndCbk () {
    LefDefParser::defrUnsetDesignEndCbk();
}

void defrUnsetDieAreaCbk () {
    LefDefParser::defrUnsetDieAreaCbk();
}

void defrUnsetDividerCbk () {
    LefDefParser::defrUnsetDividerCbk();
}

void defrUnsetExtensionCbk () {
    LefDefParser::defrUnsetExtensionCbk();
}

void defrUnsetFillCbk () {
    LefDefParser::defrUnsetFillCbk();
}

void defrUnsetFillStartCbk () {
    LefDefParser::defrUnsetFillStartCbk();
}

void defrUnsetFillEndCbk () {
    LefDefParser::defrUnsetFillEndCbk();
}

void defrUnsetFPCCbk () {
    LefDefParser::defrUnsetFPCCbk();
}

void defrUnsetFPCStartCbk () {
    LefDefParser::defrUnsetFPCStartCbk();
}

void defrUnsetFPCEndCbk () {
    LefDefParser::defrUnsetFPCEndCbk();
}

void defrUnsetFloorPlanNameCbk () {
    LefDefParser::defrUnsetFloorPlanNameCbk();
}

void defrUnsetGcellGridCbk () {
    LefDefParser::defrUnsetGcellGridCbk();
}

void defrUnsetGroupCbk () {
    LefDefParser::defrUnsetGroupCbk();
}

void defrUnsetGroupExtCbk () {
    LefDefParser::defrUnsetGroupExtCbk();
}

void defrUnsetGroupMemberCbk () {
    LefDefParser::defrUnsetGroupMemberCbk();
}

void defrUnsetComponentMaskShiftLayerCbk () {
    LefDefParser::defrUnsetComponentMaskShiftLayerCbk();
}

void defrUnsetGroupNameCbk () {
    LefDefParser::defrUnsetGroupNameCbk();
}

void defrUnsetGroupsStartCbk () {
    LefDefParser::defrUnsetGroupsStartCbk();
}

void defrUnsetGroupsEndCbk () {
    LefDefParser::defrUnsetGroupsEndCbk();
}

void defrUnsetHistoryCbk () {
    LefDefParser::defrUnsetHistoryCbk();
}

void defrUnsetIOTimingCbk () {
    LefDefParser::defrUnsetIOTimingCbk();
}

void defrUnsetIOTimingsStartCbk () {
    LefDefParser::defrUnsetIOTimingsStartCbk();
}

void defrUnsetIOTimingsEndCbk () {
    LefDefParser::defrUnsetIOTimingsEndCbk();
}

void defrUnsetIOTimingsExtCbk () {
    LefDefParser::defrUnsetIOTimingsExtCbk();
}

void defrUnsetNetCbk () {
    LefDefParser::defrUnsetNetCbk();
}

void defrUnsetNetNameCbk () {
    LefDefParser::defrUnsetNetNameCbk();
}

void defrUnsetNetNonDefaultRuleCbk () {
    LefDefParser::defrUnsetNetNonDefaultRuleCbk();
}

void defrUnsetNetConnectionExtCbk () {
    LefDefParser::defrUnsetNetConnectionExtCbk();
}

void defrUnsetNetExtCbk () {
    LefDefParser::defrUnsetNetExtCbk();
}

void defrUnsetNetPartialPathCbk () {
    LefDefParser::defrUnsetNetPartialPathCbk();
}

void defrUnsetNetSubnetNameCbk () {
    LefDefParser::defrUnsetNetSubnetNameCbk();
}

void defrUnsetNetStartCbk () {
    LefDefParser::defrUnsetNetStartCbk();
}

void defrUnsetNetEndCbk () {
    LefDefParser::defrUnsetNetEndCbk();
}

void defrUnsetNonDefaultCbk () {
    LefDefParser::defrUnsetNonDefaultCbk();
}

void defrUnsetNonDefaultStartCbk () {
    LefDefParser::defrUnsetNonDefaultStartCbk();
}

void defrUnsetNonDefaultEndCbk () {
    LefDefParser::defrUnsetNonDefaultEndCbk();
}

void defrUnsetPartitionCbk () {
    LefDefParser::defrUnsetPartitionCbk();
}

void defrUnsetPartitionsExtCbk () {
    LefDefParser::defrUnsetPartitionsExtCbk();
}

void defrUnsetPartitionsStartCbk () {
    LefDefParser::defrUnsetPartitionsStartCbk();
}

void defrUnsetPartitionsEndCbk () {
    LefDefParser::defrUnsetPartitionsEndCbk();
}

void defrUnsetPathCbk () {
    LefDefParser::defrUnsetPathCbk();
}

void defrUnsetPinCapCbk () {
    LefDefParser::defrUnsetPinCapCbk();
}

void defrUnsetPinCbk () {
    LefDefParser::defrUnsetPinCbk();
}

void defrUnsetPinEndCbk () {
    LefDefParser::defrUnsetPinEndCbk();
}

void defrUnsetPinExtCbk () {
    LefDefParser::defrUnsetPinExtCbk();
}

void defrUnsetPinPropCbk () {
    LefDefParser::defrUnsetPinPropCbk();
}

void defrUnsetPinPropStartCbk () {
    LefDefParser::defrUnsetPinPropStartCbk();
}

void defrUnsetPinPropEndCbk () {
    LefDefParser::defrUnsetPinPropEndCbk();
}

void defrUnsetPropCbk () {
    LefDefParser::defrUnsetPropCbk();
}

void defrUnsetPropDefEndCbk () {
    LefDefParser::defrUnsetPropDefEndCbk();
}

void defrUnsetPropDefStartCbk () {
    LefDefParser::defrUnsetPropDefStartCbk();
}

void defrUnsetRegionCbk () {
    LefDefParser::defrUnsetRegionCbk();
}

void defrUnsetRegionStartCbk () {
    LefDefParser::defrUnsetRegionStartCbk();
}

void defrUnsetRegionEndCbk () {
    LefDefParser::defrUnsetRegionEndCbk();
}

void defrUnsetRowCbk () {
    LefDefParser::defrUnsetRowCbk();
}

void defrUnsetScanChainExtCbk () {
    LefDefParser::defrUnsetScanChainExtCbk();
}

void defrUnsetScanchainCbk () {
    LefDefParser::defrUnsetScanchainCbk();
}

void defrUnsetScanchainsStartCbk () {
    LefDefParser::defrUnsetScanchainsStartCbk();
}

void defrUnsetScanchainsEndCbk () {
    LefDefParser::defrUnsetScanchainsEndCbk();
}

void defrUnsetSiteCbk () {
    LefDefParser::defrUnsetSiteCbk();
}

void defrUnsetSlotCbk () {
    LefDefParser::defrUnsetSlotCbk();
}

void defrUnsetSlotStartCbk () {
    LefDefParser::defrUnsetSlotStartCbk();
}

void defrUnsetSlotEndCbk () {
    LefDefParser::defrUnsetSlotEndCbk();
}

void defrUnsetSNetWireCbk () {
    LefDefParser::defrUnsetSNetWireCbk();
}

void defrUnsetSNetCbk () {
    LefDefParser::defrUnsetSNetCbk();
}

void defrUnsetSNetStartCbk () {
    LefDefParser::defrUnsetSNetStartCbk();
}

void defrUnsetSNetEndCbk () {
    LefDefParser::defrUnsetSNetEndCbk();
}

void defrUnsetSNetPartialPathCbk () {
    LefDefParser::defrUnsetSNetPartialPathCbk();
}

void defrUnsetStartPinsCbk () {
    LefDefParser::defrUnsetStartPinsCbk();
}

void defrUnsetStylesCbk () {
    LefDefParser::defrUnsetStylesCbk();
}

void defrUnsetStylesStartCbk () {
    LefDefParser::defrUnsetStylesStartCbk();
}

void defrUnsetStylesEndCbk () {
    LefDefParser::defrUnsetStylesEndCbk();
}

void defrUnsetTechnologyCbk () {
    LefDefParser::defrUnsetTechnologyCbk();
}

void defrUnsetTimingDisableCbk () {
    LefDefParser::defrUnsetTimingDisableCbk();
}

void defrUnsetTimingDisablesStartCbk () {
    LefDefParser::defrUnsetTimingDisablesStartCbk();
}

void defrUnsetTimingDisablesEndCbk () {
    LefDefParser::defrUnsetTimingDisablesEndCbk();
}

void defrUnsetTrackCbk () {
    LefDefParser::defrUnsetTrackCbk();
}

void defrUnsetUnitsCbk () {
    LefDefParser::defrUnsetUnitsCbk();
}

void defrUnsetVersionCbk () {
    LefDefParser::defrUnsetVersionCbk();
}

void defrUnsetVersionStrCbk () {
    LefDefParser::defrUnsetVersionStrCbk();
}

void defrUnsetViaCbk () {
    LefDefParser::defrUnsetViaCbk();
}

void defrUnsetViaExtCbk () {
    LefDefParser::defrUnsetViaExtCbk();
}

void defrUnsetViaStartCbk () {
    LefDefParser::defrUnsetViaStartCbk();
}

void defrUnsetViaEndCbk () {
    LefDefParser::defrUnsetViaEndCbk();
}

void defrSetUnusedCallbacks (::defrVoidCbkFnType  func) {
    LefDefParser::defrSetUnusedCallbacks((LefDefParser::defrVoidCbkFnType ) func);
}

int defrLineNumber () {
    return LefDefParser::defrLineNumber();
}

long long defrLongLineNumber () {
    return LefDefParser::defrLongLineNumber();
}

void defrSetLogFunction (::DEFI_LOG_FUNCTION p0) {
    LefDefParser::defrSetLogFunction(p0);
}

void defrSetWarningLogFunction (::DEFI_WARNING_LOG_FUNCTION p0) {
    LefDefParser::defrSetWarningLogFunction(p0);
}

void defrSetMallocFunction (::DEFI_MALLOC_FUNCTION p0) {
    LefDefParser::defrSetMallocFunction(p0);
}

void defrSetReallocFunction (::DEFI_REALLOC_FUNCTION p0) {
    LefDefParser::defrSetReallocFunction(p0);
}

void defrSetFreeFunction (::DEFI_FREE_FUNCTION p0) {
    LefDefParser::defrSetFreeFunction(p0);
}

void defrSetLineNumberFunction (::DEFI_LINE_NUMBER_FUNCTION p0) {
    LefDefParser::defrSetLineNumberFunction(p0);
}

void defrSetLongLineNumberFunction (::DEFI_LONG_LINE_NUMBER_FUNCTION p0) {
    LefDefParser::defrSetLongLineNumberFunction(p0);
}

void defrSetDeltaNumberLines (int p0) {
    LefDefParser::defrSetDeltaNumberLines(p0);
}

void defrSetReadFunction (::DEFI_READ_FUNCTION p0) {
    LefDefParser::defrSetReadFunction(p0);
}

void defrUnsetReadFunction () {
    LefDefParser::defrUnsetReadFunction();
}

void defrSetOpenLogFileAppend () {
    LefDefParser::defrSetOpenLogFileAppend();
}

void defrUnsetOpenLogFileAppend () {
    LefDefParser::defrUnsetOpenLogFileAppend();
}

void defrSetMagicCommentFoundFunction (::DEFI_MAGIC_COMMENT_FOUND_FUNCTION p0) {
    LefDefParser::defrSetMagicCommentFoundFunction(p0);
}

void defrSetMagicCommentString (char * p0) {
    LefDefParser::defrSetMagicCommentString(p0);
}

void defrDisablePropStrProcess () {
    LefDefParser::defrDisablePropStrProcess();
}

void defrSetNLines (long long  n) {
    LefDefParser::defrSetNLines(n);
}

void defrSetAssertionWarnings (int  warn) {
    LefDefParser::defrSetAssertionWarnings(warn);
}

void defrSetBlockageWarnings (int  warn) {
    LefDefParser::defrSetBlockageWarnings(warn);
}

void defrSetCaseSensitiveWarnings (int  warn) {
    LefDefParser::defrSetCaseSensitiveWarnings(warn);
}

void defrSetComponentWarnings (int  warn) {
    LefDefParser::defrSetComponentWarnings(warn);
}

void defrSetConstraintWarnings (int  warn) {
    LefDefParser::defrSetConstraintWarnings(warn);
}

void defrSetDefaultCapWarnings (int  warn) {
    LefDefParser::defrSetDefaultCapWarnings(warn);
}

void defrSetGcellGridWarnings (int  warn) {
    LefDefParser::defrSetGcellGridWarnings(warn);
}

void defrSetIOTimingWarnings (int  warn) {
    LefDefParser::defrSetIOTimingWarnings(warn);
}

void defrSetNetWarnings (int  warn) {
    LefDefParser::defrSetNetWarnings(warn);
}

void defrSetNonDefaultWarnings (int  warn) {
    LefDefParser::defrSetNonDefaultWarnings(warn);
}

void defrSetPinExtWarnings (int  warn) {
    LefDefParser::defrSetPinExtWarnings(warn);
}

void defrSetPinWarnings (int  warn) {
    LefDefParser::defrSetPinWarnings(warn);
}

void defrSetRegionWarnings (int  warn) {
    LefDefParser::defrSetRegionWarnings(warn);
}

void defrSetRowWarnings (int  warn) {
    LefDefParser::defrSetRowWarnings(warn);
}

void defrSetScanchainWarnings (int  warn) {
    LefDefParser::defrSetScanchainWarnings(warn);
}

void defrSetSNetWarnings (int  warn) {
    LefDefParser::defrSetSNetWarnings(warn);
}

void defrSetStylesWarnings (int  warn) {
    LefDefParser::defrSetStylesWarnings(warn);
}

void defrSetTrackWarnings (int  warn) {
    LefDefParser::defrSetTrackWarnings(warn);
}

void defrSetUnitsWarnings (int  warn) {
    LefDefParser::defrSetUnitsWarnings(warn);
}

void defrSetVersionWarnings (int  warn) {
    LefDefParser::defrSetVersionWarnings(warn);
}

void defrSetViaWarnings (int  warn) {
    LefDefParser::defrSetViaWarnings(warn);
}

void defrDisableParserMsgs (int  nMsg, int*  msgs) {
    LefDefParser::defrDisableParserMsgs(nMsg, msgs);
}

void defrEnableParserMsgs (int  nMsg, int*  msgs) {
    LefDefParser::defrEnableParserMsgs(nMsg, msgs);
}

void defrEnableAllMsgs () {
    LefDefParser::defrEnableAllMsgs();
}

void defrSetTotalMsgLimit (int  totNumMsgs) {
    LefDefParser::defrSetTotalMsgLimit(totNumMsgs);
}

void defrSetLimitPerMsg (int  msgId, int  numMsg) {
    LefDefParser::defrSetLimitPerMsg(msgId, numMsg);
}

void defrAddAlias (const char*  key, const char*  value, int  marked) {
    LefDefParser::defrAddAlias(key, value, marked);
}

