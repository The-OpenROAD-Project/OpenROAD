// *****************************************************************************
// *****************************************************************************
// Copyright 2013, Cadence Design Systems
// 
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8. 
// 
// Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
// 
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
// 
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2017/06/06 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#ifndef DEFRCALLBACKS_H
#define DEFRCALLBACKS_H 1

#include "defiKRDefs.hpp"

#include "defrReader.hpp"

#include "defrReader.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

class defrCallbacks {
public:
    defrCallbacks();

    void SetUnusedCallbacks(defrVoidCbkFnType f);

    defrStringCbkFnType DesignCbk;
    defrStringCbkFnType TechnologyCbk;
    defrVoidCbkFnType DesignEndCbk;
    defrPropCbkFnType PropCbk;
    defrVoidCbkFnType PropDefEndCbk;
    defrVoidCbkFnType PropDefStartCbk;
    defrStringCbkFnType ArrayNameCbk;
    defrStringCbkFnType FloorPlanNameCbk;
    defrDoubleCbkFnType UnitsCbk;
    defrStringCbkFnType DividerCbk;
    defrStringCbkFnType BusBitCbk;
    defrSiteCbkFnType SiteCbk;
    defrSiteCbkFnType CanplaceCbk;
    defrSiteCbkFnType CannotOccupyCbk;
    defrIntegerCbkFnType ComponentStartCbk;
    defrVoidCbkFnType ComponentEndCbk;
    defrComponentCbkFnType ComponentCbk;
    defrComponentMaskShiftLayerCbkFnType ComponentMaskShiftLayerCbk;
    defrIntegerCbkFnType NetStartCbk;
    defrVoidCbkFnType NetEndCbk;
    defrNetCbkFnType NetCbk;
    defrStringCbkFnType NetNameCbk;
    defrStringCbkFnType NetSubnetNameCbk;
    defrStringCbkFnType NetNonDefaultRuleCbk;
    defrNetCbkFnType NetPartialPathCbk;
    defrPathCbkFnType PathCbk;
    defrDoubleCbkFnType VersionCbk;
    defrStringCbkFnType VersionStrCbk;
    defrStringCbkFnType PinExtCbk;
    defrStringCbkFnType ComponentExtCbk;
    defrStringCbkFnType ViaExtCbk;
    defrStringCbkFnType NetConnectionExtCbk;
    defrStringCbkFnType NetExtCbk;
    defrStringCbkFnType GroupExtCbk;
    defrStringCbkFnType ScanChainExtCbk;
    defrStringCbkFnType IoTimingsExtCbk;
    defrStringCbkFnType PartitionsExtCbk;
    defrStringCbkFnType HistoryCbk;
    defrBoxCbkFnType DieAreaCbk;
    defrPinCapCbkFnType PinCapCbk;
    defrPinCbkFnType PinCbk;
    defrIntegerCbkFnType StartPinsCbk;
    defrVoidCbkFnType PinEndCbk;
    defrIntegerCbkFnType DefaultCapCbk;
    defrRowCbkFnType RowCbk;
    defrTrackCbkFnType TrackCbk;
    defrGcellGridCbkFnType GcellGridCbk;
    defrIntegerCbkFnType ViaStartCbk;
    defrVoidCbkFnType ViaEndCbk;
    defrViaCbkFnType ViaCbk;
    defrIntegerCbkFnType RegionStartCbk;
    defrVoidCbkFnType RegionEndCbk;
    defrRegionCbkFnType RegionCbk;
    defrIntegerCbkFnType SNetStartCbk;
    defrVoidCbkFnType SNetEndCbk;
    defrNetCbkFnType SNetCbk;
    defrNetCbkFnType SNetPartialPathCbk;
    defrNetCbkFnType SNetWireCbk;
    defrIntegerCbkFnType GroupsStartCbk;
    defrVoidCbkFnType GroupsEndCbk;
    defrStringCbkFnType GroupNameCbk;
    defrStringCbkFnType GroupMemberCbk;
    defrGroupCbkFnType GroupCbk;
    defrIntegerCbkFnType AssertionsStartCbk;
    defrVoidCbkFnType AssertionsEndCbk;
    defrAssertionCbkFnType AssertionCbk;
    defrIntegerCbkFnType ConstraintsStartCbk;
    defrVoidCbkFnType ConstraintsEndCbk;
    defrAssertionCbkFnType ConstraintCbk;
    defrIntegerCbkFnType ScanchainsStartCbk;
    defrVoidCbkFnType ScanchainsEndCbk;
    defrScanchainCbkFnType ScanchainCbk;
    defrIntegerCbkFnType IOTimingsStartCbk;
    defrVoidCbkFnType IOTimingsEndCbk;
    defrIOTimingCbkFnType IOTimingCbk;
    defrIntegerCbkFnType FPCStartCbk;
    defrVoidCbkFnType FPCEndCbk;
    defrFPCCbkFnType FPCCbk;
    defrIntegerCbkFnType TimingDisablesStartCbk;
    defrVoidCbkFnType TimingDisablesEndCbk;
    defrTimingDisableCbkFnType TimingDisableCbk;
    defrIntegerCbkFnType PartitionsStartCbk;
    defrVoidCbkFnType PartitionsEndCbk;
    defrPartitionCbkFnType PartitionCbk;
    defrIntegerCbkFnType PinPropStartCbk;
    defrVoidCbkFnType PinPropEndCbk;
    defrPinPropCbkFnType PinPropCbk;
    defrIntegerCbkFnType CaseSensitiveCbk;
    defrIntegerCbkFnType BlockageStartCbk;
    defrVoidCbkFnType BlockageEndCbk;
    defrBlockageCbkFnType BlockageCbk;
    defrIntegerCbkFnType SlotStartCbk;
    defrVoidCbkFnType SlotEndCbk;
    defrSlotCbkFnType SlotCbk;
    defrIntegerCbkFnType FillStartCbk;
    defrVoidCbkFnType FillEndCbk;
    defrFillCbkFnType FillCbk;
    defrIntegerCbkFnType NonDefaultStartCbk;
    defrVoidCbkFnType NonDefaultEndCbk;
    defrNonDefaultCbkFnType NonDefaultCbk;
    defrIntegerCbkFnType StylesStartCbk;
    defrVoidCbkFnType StylesEndCbk;
    defrStylesCbkFnType StylesCbk;
    defrStringCbkFnType ExtensionCbk;


};

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
