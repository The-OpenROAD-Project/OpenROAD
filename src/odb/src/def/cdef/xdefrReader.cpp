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
int defrInit()
{
  return DefParser::defrInit();
}

int defrInitSession(int startSession)
{
  return DefParser::defrInitSession(startSession);
}

int defrReset()
{
  return DefParser::defrReset();
}

int defrClear()
{
  return DefParser::defrClear();
}

void defrSetCommentChar(char c)
{
  DefParser::defrSetCommentChar(c);
}

void defrSetAddPathToNet()
{
  DefParser::defrSetAddPathToNet();
}

void defrSetAllowComponentNets()
{
  DefParser::defrSetAllowComponentNets();
}

int defrGetAllowComponentNets()
{
  return DefParser::defrGetAllowComponentNets();
}

void defrSetCaseSensitivity(int caseSense)
{
  DefParser::defrSetCaseSensitivity(caseSense);
}

void defrSetRegisterUnusedCallbacks()
{
  DefParser::defrSetRegisterUnusedCallbacks();
}

void defrPrintUnusedCallbacks(FILE* log)
{
  DefParser::defrPrintUnusedCallbacks(log);
}

int defrReleaseNResetMemory()
{
  return DefParser::defrReleaseNResetMemory();
}

int defrRead(FILE* file,
             const char* fileName,
             defiUserData userData,
             int case_sensitive)
{
  return DefParser::defrRead(file, fileName, userData, case_sensitive);
}

void defrSetUserData(defiUserData p0)
{
  DefParser::defrSetUserData(p0);
}

defiUserData defrGetUserData()
{
  return DefParser::defrGetUserData();
}

void defrSetArrayNameCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetArrayNameCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetAssertionCbk(::defrAssertionCbkFnType p0)
{
  DefParser::defrSetAssertionCbk((DefParser::defrAssertionCbkFnType) p0);
}

void defrSetAssertionsStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetAssertionsStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetAssertionsEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetAssertionsEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetBlockageCbk(::defrBlockageCbkFnType p0)
{
  DefParser::defrSetBlockageCbk((DefParser::defrBlockageCbkFnType) p0);
}

void defrSetBlockageStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetBlockageStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetBlockageEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetBlockageEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetBusBitCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetBusBitCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetCannotOccupyCbk(::defrSiteCbkFnType p0)
{
  DefParser::defrSetCannotOccupyCbk((DefParser::defrSiteCbkFnType) p0);
}

void defrSetCanplaceCbk(::defrSiteCbkFnType p0)
{
  DefParser::defrSetCanplaceCbk((DefParser::defrSiteCbkFnType) p0);
}

void defrSetCaseSensitiveCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetCaseSensitiveCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetComponentCbk(::defrComponentCbkFnType p0)
{
  DefParser::defrSetComponentCbk((DefParser::defrComponentCbkFnType) p0);
}

void defrSetComponentExtCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetComponentExtCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetComponentStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetComponentStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetComponentEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetComponentEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetConstraintCbk(::defrAssertionCbkFnType p0)
{
  DefParser::defrSetConstraintCbk((DefParser::defrAssertionCbkFnType) p0);
}

void defrSetConstraintsStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetConstraintsStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetConstraintsEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetConstraintsEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetDefaultCapCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetDefaultCapCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetDesignCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetDesignCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetDesignEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetDesignEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetDieAreaCbk(::defrBoxCbkFnType p0)
{
  DefParser::defrSetDieAreaCbk((DefParser::defrBoxCbkFnType) p0);
}

void defrSetDividerCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetDividerCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetExtensionCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetExtensionCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetFillCbk(::defrFillCbkFnType p0)
{
  DefParser::defrSetFillCbk((DefParser::defrFillCbkFnType) p0);
}

void defrSetFillStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetFillStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetFillEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetFillEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetFPCCbk(::defrFPCCbkFnType p0)
{
  DefParser::defrSetFPCCbk((DefParser::defrFPCCbkFnType) p0);
}

void defrSetFPCStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetFPCStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetFPCEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetFPCEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetFloorPlanNameCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetFloorPlanNameCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetGcellGridCbk(::defrGcellGridCbkFnType p0)
{
  DefParser::defrSetGcellGridCbk((DefParser::defrGcellGridCbkFnType) p0);
}

void defrSetGroupNameCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetGroupNameCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetGroupMemberCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetGroupMemberCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetComponentMaskShiftLayerCbk(
    ::defrComponentMaskShiftLayerCbkFnType p0)
{
  DefParser::defrSetComponentMaskShiftLayerCbk(
      (DefParser::defrComponentMaskShiftLayerCbkFnType) p0);
}

void defrSetGroupCbk(::defrGroupCbkFnType p0)
{
  DefParser::defrSetGroupCbk((DefParser::defrGroupCbkFnType) p0);
}

void defrSetGroupExtCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetGroupExtCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetGroupsStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetGroupsStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetGroupsEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetGroupsEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetHistoryCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetHistoryCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetIOTimingCbk(::defrIOTimingCbkFnType p0)
{
  DefParser::defrSetIOTimingCbk((DefParser::defrIOTimingCbkFnType) p0);
}

void defrSetIOTimingsStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetIOTimingsStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetIOTimingsEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetIOTimingsEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetIoTimingsExtCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetIoTimingsExtCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetNetCbk(::defrNetCbkFnType p0)
{
  DefParser::defrSetNetCbk((DefParser::defrNetCbkFnType) p0);
}

void defrSetNetNameCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetNetNameCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetNetNonDefaultRuleCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetNetNonDefaultRuleCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetNetConnectionExtCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetNetConnectionExtCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetNetExtCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetNetExtCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetNetPartialPathCbk(::defrNetCbkFnType p0)
{
  DefParser::defrSetNetPartialPathCbk((DefParser::defrNetCbkFnType) p0);
}

void defrSetNetSubnetNameCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetNetSubnetNameCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetNetStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetNetStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetNetEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetNetEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetNonDefaultCbk(::defrNonDefaultCbkFnType p0)
{
  DefParser::defrSetNonDefaultCbk((DefParser::defrNonDefaultCbkFnType) p0);
}

void defrSetNonDefaultStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetNonDefaultStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetNonDefaultEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetNonDefaultEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetPartitionCbk(::defrPartitionCbkFnType p0)
{
  DefParser::defrSetPartitionCbk((DefParser::defrPartitionCbkFnType) p0);
}

void defrSetPartitionsExtCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetPartitionsExtCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetPartitionsStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetPartitionsStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetPartitionsEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetPartitionsEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetPathCbk(::defrPathCbkFnType p0)
{
  DefParser::defrSetPathCbk((DefParser::defrPathCbkFnType) p0);
}

void defrSetPinCapCbk(::defrPinCapCbkFnType p0)
{
  DefParser::defrSetPinCapCbk((DefParser::defrPinCapCbkFnType) p0);
}

void defrSetPinCbk(::defrPinCbkFnType p0)
{
  DefParser::defrSetPinCbk((DefParser::defrPinCbkFnType) p0);
}

void defrSetPinExtCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetPinExtCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetPinPropCbk(::defrPinPropCbkFnType p0)
{
  DefParser::defrSetPinPropCbk((DefParser::defrPinPropCbkFnType) p0);
}

void defrSetPinPropStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetPinPropStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetPinPropEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetPinPropEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetPropCbk(::defrPropCbkFnType p0)
{
  DefParser::defrSetPropCbk((DefParser::defrPropCbkFnType) p0);
}

void defrSetPropDefEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetPropDefEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetPropDefStartCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetPropDefStartCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetRegionCbk(::defrRegionCbkFnType p0)
{
  DefParser::defrSetRegionCbk((DefParser::defrRegionCbkFnType) p0);
}

void defrSetRegionStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetRegionStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetRegionEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetRegionEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetRowCbk(::defrRowCbkFnType p0)
{
  DefParser::defrSetRowCbk((DefParser::defrRowCbkFnType) p0);
}

void defrSetSNetCbk(::defrNetCbkFnType p0)
{
  DefParser::defrSetSNetCbk((DefParser::defrNetCbkFnType) p0);
}

void defrSetSNetStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetSNetStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetSNetEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetSNetEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetSNetPartialPathCbk(::defrNetCbkFnType p0)
{
  DefParser::defrSetSNetPartialPathCbk((DefParser::defrNetCbkFnType) p0);
}

void defrSetSNetWireCbk(::defrNetCbkFnType p0)
{
  DefParser::defrSetSNetWireCbk((DefParser::defrNetCbkFnType) p0);
}

void defrSetScanChainExtCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetScanChainExtCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetScanchainCbk(::defrScanchainCbkFnType p0)
{
  DefParser::defrSetScanchainCbk((DefParser::defrScanchainCbkFnType) p0);
}

void defrSetScanchainsStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetScanchainsStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetScanchainsEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetScanchainsEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetSiteCbk(::defrSiteCbkFnType p0)
{
  DefParser::defrSetSiteCbk((DefParser::defrSiteCbkFnType) p0);
}

void defrSetSlotCbk(::defrSlotCbkFnType p0)
{
  DefParser::defrSetSlotCbk((DefParser::defrSlotCbkFnType) p0);
}

void defrSetSlotStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetSlotStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetSlotEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetSlotEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetStartPinsCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetStartPinsCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetStylesCbk(::defrStylesCbkFnType p0)
{
  DefParser::defrSetStylesCbk((DefParser::defrStylesCbkFnType) p0);
}

void defrSetStylesStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetStylesStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetStylesEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetStylesEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetPinEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetPinEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetTechnologyCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetTechnologyCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetTimingDisableCbk(::defrTimingDisableCbkFnType p0)
{
  DefParser::defrSetTimingDisableCbk(
      (DefParser::defrTimingDisableCbkFnType) p0);
}

void defrSetTimingDisablesStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetTimingDisablesStartCbk(
      (DefParser::defrIntegerCbkFnType) p0);
}

void defrSetTimingDisablesEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetTimingDisablesEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrSetTrackCbk(::defrTrackCbkFnType p0)
{
  DefParser::defrSetTrackCbk((DefParser::defrTrackCbkFnType) p0);
}

void defrSetUnitsCbk(::defrDoubleCbkFnType p0)
{
  DefParser::defrSetUnitsCbk((DefParser::defrDoubleCbkFnType) p0);
}

void defrSetVersionCbk(::defrDoubleCbkFnType p0)
{
  DefParser::defrSetVersionCbk((DefParser::defrDoubleCbkFnType) p0);
}

void defrSetVersionStrCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetVersionStrCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetViaCbk(::defrViaCbkFnType p0)
{
  DefParser::defrSetViaCbk((DefParser::defrViaCbkFnType) p0);
}

void defrSetViaExtCbk(::defrStringCbkFnType p0)
{
  DefParser::defrSetViaExtCbk((DefParser::defrStringCbkFnType) p0);
}

void defrSetViaStartCbk(::defrIntegerCbkFnType p0)
{
  DefParser::defrSetViaStartCbk((DefParser::defrIntegerCbkFnType) p0);
}

void defrSetViaEndCbk(::defrVoidCbkFnType p0)
{
  DefParser::defrSetViaEndCbk((DefParser::defrVoidCbkFnType) p0);
}

void defrUnsetCallbacks()
{
  DefParser::defrUnsetCallbacks();
}

void defrUnsetArrayNameCbk()
{
  DefParser::defrUnsetArrayNameCbk();
}

void defrUnsetAssertionCbk()
{
  DefParser::defrUnsetAssertionCbk();
}

void defrUnsetAssertionsStartCbk()
{
  DefParser::defrUnsetAssertionsStartCbk();
}

void defrUnsetAssertionsEndCbk()
{
  DefParser::defrUnsetAssertionsEndCbk();
}

void defrUnsetBlockageCbk()
{
  DefParser::defrUnsetBlockageCbk();
}

void defrUnsetBlockageStartCbk()
{
  DefParser::defrUnsetBlockageStartCbk();
}

void defrUnsetBlockageEndCbk()
{
  DefParser::defrUnsetBlockageEndCbk();
}

void defrUnsetBusBitCbk()
{
  DefParser::defrUnsetBusBitCbk();
}

void defrUnsetCannotOccupyCbk()
{
  DefParser::defrUnsetCannotOccupyCbk();
}

void defrUnsetCanplaceCbk()
{
  DefParser::defrUnsetCanplaceCbk();
}

void defrUnsetCaseSensitiveCbk()
{
  DefParser::defrUnsetCaseSensitiveCbk();
}

void defrUnsetComponentCbk()
{
  DefParser::defrUnsetComponentCbk();
}

void defrUnsetComponentExtCbk()
{
  DefParser::defrUnsetComponentExtCbk();
}

void defrUnsetComponentStartCbk()
{
  DefParser::defrUnsetComponentStartCbk();
}

void defrUnsetComponentEndCbk()
{
  DefParser::defrUnsetComponentEndCbk();
}

void defrUnsetConstraintCbk()
{
  DefParser::defrUnsetConstraintCbk();
}

void defrUnsetConstraintsStartCbk()
{
  DefParser::defrUnsetConstraintsStartCbk();
}

void defrUnsetConstraintsEndCbk()
{
  DefParser::defrUnsetConstraintsEndCbk();
}

void defrUnsetDefaultCapCbk()
{
  DefParser::defrUnsetDefaultCapCbk();
}

void defrUnsetDesignCbk()
{
  DefParser::defrUnsetDesignCbk();
}

void defrUnsetDesignEndCbk()
{
  DefParser::defrUnsetDesignEndCbk();
}

void defrUnsetDieAreaCbk()
{
  DefParser::defrUnsetDieAreaCbk();
}

void defrUnsetDividerCbk()
{
  DefParser::defrUnsetDividerCbk();
}

void defrUnsetExtensionCbk()
{
  DefParser::defrUnsetExtensionCbk();
}

void defrUnsetFillCbk()
{
  DefParser::defrUnsetFillCbk();
}

void defrUnsetFillStartCbk()
{
  DefParser::defrUnsetFillStartCbk();
}

void defrUnsetFillEndCbk()
{
  DefParser::defrUnsetFillEndCbk();
}

void defrUnsetFPCCbk()
{
  DefParser::defrUnsetFPCCbk();
}

void defrUnsetFPCStartCbk()
{
  DefParser::defrUnsetFPCStartCbk();
}

void defrUnsetFPCEndCbk()
{
  DefParser::defrUnsetFPCEndCbk();
}

void defrUnsetFloorPlanNameCbk()
{
  DefParser::defrUnsetFloorPlanNameCbk();
}

void defrUnsetGcellGridCbk()
{
  DefParser::defrUnsetGcellGridCbk();
}

void defrUnsetGroupCbk()
{
  DefParser::defrUnsetGroupCbk();
}

void defrUnsetGroupExtCbk()
{
  DefParser::defrUnsetGroupExtCbk();
}

void defrUnsetGroupMemberCbk()
{
  DefParser::defrUnsetGroupMemberCbk();
}

void defrUnsetComponentMaskShiftLayerCbk()
{
  DefParser::defrUnsetComponentMaskShiftLayerCbk();
}

void defrUnsetGroupNameCbk()
{
  DefParser::defrUnsetGroupNameCbk();
}

void defrUnsetGroupsStartCbk()
{
  DefParser::defrUnsetGroupsStartCbk();
}

void defrUnsetGroupsEndCbk()
{
  DefParser::defrUnsetGroupsEndCbk();
}

void defrUnsetHistoryCbk()
{
  DefParser::defrUnsetHistoryCbk();
}

void defrUnsetIOTimingCbk()
{
  DefParser::defrUnsetIOTimingCbk();
}

void defrUnsetIOTimingsStartCbk()
{
  DefParser::defrUnsetIOTimingsStartCbk();
}

void defrUnsetIOTimingsEndCbk()
{
  DefParser::defrUnsetIOTimingsEndCbk();
}

void defrUnsetIOTimingsExtCbk()
{
  DefParser::defrUnsetIOTimingsExtCbk();
}

void defrUnsetNetCbk()
{
  DefParser::defrUnsetNetCbk();
}

void defrUnsetNetNameCbk()
{
  DefParser::defrUnsetNetNameCbk();
}

void defrUnsetNetNonDefaultRuleCbk()
{
  DefParser::defrUnsetNetNonDefaultRuleCbk();
}

void defrUnsetNetConnectionExtCbk()
{
  DefParser::defrUnsetNetConnectionExtCbk();
}

void defrUnsetNetExtCbk()
{
  DefParser::defrUnsetNetExtCbk();
}

void defrUnsetNetPartialPathCbk()
{
  DefParser::defrUnsetNetPartialPathCbk();
}

void defrUnsetNetSubnetNameCbk()
{
  DefParser::defrUnsetNetSubnetNameCbk();
}

void defrUnsetNetStartCbk()
{
  DefParser::defrUnsetNetStartCbk();
}

void defrUnsetNetEndCbk()
{
  DefParser::defrUnsetNetEndCbk();
}

void defrUnsetNonDefaultCbk()
{
  DefParser::defrUnsetNonDefaultCbk();
}

void defrUnsetNonDefaultStartCbk()
{
  DefParser::defrUnsetNonDefaultStartCbk();
}

void defrUnsetNonDefaultEndCbk()
{
  DefParser::defrUnsetNonDefaultEndCbk();
}

void defrUnsetPartitionCbk()
{
  DefParser::defrUnsetPartitionCbk();
}

void defrUnsetPartitionsExtCbk()
{
  DefParser::defrUnsetPartitionsExtCbk();
}

void defrUnsetPartitionsStartCbk()
{
  DefParser::defrUnsetPartitionsStartCbk();
}

void defrUnsetPartitionsEndCbk()
{
  DefParser::defrUnsetPartitionsEndCbk();
}

void defrUnsetPathCbk()
{
  DefParser::defrUnsetPathCbk();
}

void defrUnsetPinCapCbk()
{
  DefParser::defrUnsetPinCapCbk();
}

void defrUnsetPinCbk()
{
  DefParser::defrUnsetPinCbk();
}

void defrUnsetPinEndCbk()
{
  DefParser::defrUnsetPinEndCbk();
}

void defrUnsetPinExtCbk()
{
  DefParser::defrUnsetPinExtCbk();
}

void defrUnsetPinPropCbk()
{
  DefParser::defrUnsetPinPropCbk();
}

void defrUnsetPinPropStartCbk()
{
  DefParser::defrUnsetPinPropStartCbk();
}

void defrUnsetPinPropEndCbk()
{
  DefParser::defrUnsetPinPropEndCbk();
}

void defrUnsetPropCbk()
{
  DefParser::defrUnsetPropCbk();
}

void defrUnsetPropDefEndCbk()
{
  DefParser::defrUnsetPropDefEndCbk();
}

void defrUnsetPropDefStartCbk()
{
  DefParser::defrUnsetPropDefStartCbk();
}

void defrUnsetRegionCbk()
{
  DefParser::defrUnsetRegionCbk();
}

void defrUnsetRegionStartCbk()
{
  DefParser::defrUnsetRegionStartCbk();
}

void defrUnsetRegionEndCbk()
{
  DefParser::defrUnsetRegionEndCbk();
}

void defrUnsetRowCbk()
{
  DefParser::defrUnsetRowCbk();
}

void defrUnsetScanChainExtCbk()
{
  DefParser::defrUnsetScanChainExtCbk();
}

void defrUnsetScanchainCbk()
{
  DefParser::defrUnsetScanchainCbk();
}

void defrUnsetScanchainsStartCbk()
{
  DefParser::defrUnsetScanchainsStartCbk();
}

void defrUnsetScanchainsEndCbk()
{
  DefParser::defrUnsetScanchainsEndCbk();
}

void defrUnsetSiteCbk()
{
  DefParser::defrUnsetSiteCbk();
}

void defrUnsetSlotCbk()
{
  DefParser::defrUnsetSlotCbk();
}

void defrUnsetSlotStartCbk()
{
  DefParser::defrUnsetSlotStartCbk();
}

void defrUnsetSlotEndCbk()
{
  DefParser::defrUnsetSlotEndCbk();
}

void defrUnsetSNetWireCbk()
{
  DefParser::defrUnsetSNetWireCbk();
}

void defrUnsetSNetCbk()
{
  DefParser::defrUnsetSNetCbk();
}

void defrUnsetSNetStartCbk()
{
  DefParser::defrUnsetSNetStartCbk();
}

void defrUnsetSNetEndCbk()
{
  DefParser::defrUnsetSNetEndCbk();
}

void defrUnsetSNetPartialPathCbk()
{
  DefParser::defrUnsetSNetPartialPathCbk();
}

void defrUnsetStartPinsCbk()
{
  DefParser::defrUnsetStartPinsCbk();
}

void defrUnsetStylesCbk()
{
  DefParser::defrUnsetStylesCbk();
}

void defrUnsetStylesStartCbk()
{
  DefParser::defrUnsetStylesStartCbk();
}

void defrUnsetStylesEndCbk()
{
  DefParser::defrUnsetStylesEndCbk();
}

void defrUnsetTechnologyCbk()
{
  DefParser::defrUnsetTechnologyCbk();
}

void defrUnsetTimingDisableCbk()
{
  DefParser::defrUnsetTimingDisableCbk();
}

void defrUnsetTimingDisablesStartCbk()
{
  DefParser::defrUnsetTimingDisablesStartCbk();
}

void defrUnsetTimingDisablesEndCbk()
{
  DefParser::defrUnsetTimingDisablesEndCbk();
}

void defrUnsetTrackCbk()
{
  DefParser::defrUnsetTrackCbk();
}

void defrUnsetUnitsCbk()
{
  DefParser::defrUnsetUnitsCbk();
}

void defrUnsetVersionCbk()
{
  DefParser::defrUnsetVersionCbk();
}

void defrUnsetVersionStrCbk()
{
  DefParser::defrUnsetVersionStrCbk();
}

void defrUnsetViaCbk()
{
  DefParser::defrUnsetViaCbk();
}

void defrUnsetViaExtCbk()
{
  DefParser::defrUnsetViaExtCbk();
}

void defrUnsetViaStartCbk()
{
  DefParser::defrUnsetViaStartCbk();
}

void defrUnsetViaEndCbk()
{
  DefParser::defrUnsetViaEndCbk();
}

void defrSetUnusedCallbacks(::defrVoidCbkFnType func)
{
  DefParser::defrSetUnusedCallbacks((DefParser::defrVoidCbkFnType) func);
}

int defrLineNumber()
{
  return DefParser::defrLineNumber();
}

long long defrLongLineNumber()
{
  return DefParser::defrLongLineNumber();
}

void defrSetLogFunction(::DEFI_LOG_FUNCTION p0)
{
  DefParser::defrSetLogFunction(p0);
}

void defrSetWarningLogFunction(::DEFI_WARNING_LOG_FUNCTION p0)
{
  DefParser::defrSetWarningLogFunction(p0);
}

void defrSetMallocFunction(::DEFI_MALLOC_FUNCTION p0)
{
  DefParser::defrSetMallocFunction(p0);
}

void defrSetReallocFunction(::DEFI_REALLOC_FUNCTION p0)
{
  DefParser::defrSetReallocFunction(p0);
}

void defrSetFreeFunction(::DEFI_FREE_FUNCTION p0)
{
  DefParser::defrSetFreeFunction(p0);
}

void defrSetLineNumberFunction(::DEFI_LINE_NUMBER_FUNCTION p0)
{
  DefParser::defrSetLineNumberFunction(p0);
}

void defrSetLongLineNumberFunction(::DEFI_LONG_LINE_NUMBER_FUNCTION p0)
{
  DefParser::defrSetLongLineNumberFunction(p0);
}

void defrSetDeltaNumberLines(int p0)
{
  DefParser::defrSetDeltaNumberLines(p0);
}

void defrSetReadFunction(::DEFI_READ_FUNCTION p0)
{
  DefParser::defrSetReadFunction(p0);
}

void defrUnsetReadFunction()
{
  DefParser::defrUnsetReadFunction();
}

void defrSetOpenLogFileAppend()
{
  DefParser::defrSetOpenLogFileAppend();
}

void defrUnsetOpenLogFileAppend()
{
  DefParser::defrUnsetOpenLogFileAppend();
}

void defrSetMagicCommentFoundFunction(::DEFI_MAGIC_COMMENT_FOUND_FUNCTION p0)
{
  DefParser::defrSetMagicCommentFoundFunction(p0);
}

void defrSetMagicCommentString(char* p0)
{
  DefParser::defrSetMagicCommentString(p0);
}

void defrDisablePropStrProcess()
{
  DefParser::defrDisablePropStrProcess();
}

void defrSetNLines(long long n)
{
  DefParser::defrSetNLines(n);
}

void defrSetAssertionWarnings(int warn)
{
  DefParser::defrSetAssertionWarnings(warn);
}

void defrSetBlockageWarnings(int warn)
{
  DefParser::defrSetBlockageWarnings(warn);
}

void defrSetCaseSensitiveWarnings(int warn)
{
  DefParser::defrSetCaseSensitiveWarnings(warn);
}

void defrSetComponentWarnings(int warn)
{
  DefParser::defrSetComponentWarnings(warn);
}

void defrSetConstraintWarnings(int warn)
{
  DefParser::defrSetConstraintWarnings(warn);
}

void defrSetDefaultCapWarnings(int warn)
{
  DefParser::defrSetDefaultCapWarnings(warn);
}

void defrSetGcellGridWarnings(int warn)
{
  DefParser::defrSetGcellGridWarnings(warn);
}

void defrSetIOTimingWarnings(int warn)
{
  DefParser::defrSetIOTimingWarnings(warn);
}

void defrSetNetWarnings(int warn)
{
  DefParser::defrSetNetWarnings(warn);
}

void defrSetNonDefaultWarnings(int warn)
{
  DefParser::defrSetNonDefaultWarnings(warn);
}

void defrSetPinExtWarnings(int warn)
{
  DefParser::defrSetPinExtWarnings(warn);
}

void defrSetPinWarnings(int warn)
{
  DefParser::defrSetPinWarnings(warn);
}

void defrSetRegionWarnings(int warn)
{
  DefParser::defrSetRegionWarnings(warn);
}

void defrSetRowWarnings(int warn)
{
  DefParser::defrSetRowWarnings(warn);
}

void defrSetScanchainWarnings(int warn)
{
  DefParser::defrSetScanchainWarnings(warn);
}

void defrSetSNetWarnings(int warn)
{
  DefParser::defrSetSNetWarnings(warn);
}

void defrSetStylesWarnings(int warn)
{
  DefParser::defrSetStylesWarnings(warn);
}

void defrSetTrackWarnings(int warn)
{
  DefParser::defrSetTrackWarnings(warn);
}

void defrSetUnitsWarnings(int warn)
{
  DefParser::defrSetUnitsWarnings(warn);
}

void defrSetVersionWarnings(int warn)
{
  DefParser::defrSetVersionWarnings(warn);
}

void defrSetViaWarnings(int warn)
{
  DefParser::defrSetViaWarnings(warn);
}

void defrDisableParserMsgs(int nMsg, int* msgs)
{
  DefParser::defrDisableParserMsgs(nMsg, msgs);
}

void defrEnableParserMsgs(int nMsg, int* msgs)
{
  DefParser::defrEnableParserMsgs(nMsg, msgs);
}

void defrEnableAllMsgs()
{
  DefParser::defrEnableAllMsgs();
}

void defrSetTotalMsgLimit(int totNumMsgs)
{
  DefParser::defrSetTotalMsgLimit(totNumMsgs);
}

void defrSetLimitPerMsg(int msgId, int numMsg)
{
  DefParser::defrSetLimitPerMsg(msgId, numMsg);
}

void defrAddAlias(const char* key, const char* value, int marked)
{
  DefParser::defrAddAlias(key, value, marked);
}
