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
//  $Date: 2020/09/29 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#ifndef DEFRCALLBACKS_H
#define DEFRCALLBACKS_H 1

#include "defiKRDefs.hpp"
#include "defrReader.hpp"

BEGIN_DEF_PARSER_NAMESPACE

class defrCallbacks
{
 public:
  void SetUnusedCallbacks(defrVoidCbkFnType f);

  defrStringCbkFnType DesignCbk{nullptr};
  defrStringCbkFnType TechnologyCbk{nullptr};
  defrVoidCbkFnType DesignEndCbk{nullptr};
  defrPropCbkFnType PropCbk{nullptr};
  defrVoidCbkFnType PropDefEndCbk{nullptr};
  defrVoidCbkFnType PropDefStartCbk{nullptr};
  defrStringCbkFnType ArrayNameCbk{nullptr};
  defrStringCbkFnType FloorPlanNameCbk{nullptr};
  defrDoubleCbkFnType UnitsCbk{nullptr};
  defrStringCbkFnType DividerCbk{nullptr};
  defrStringCbkFnType BusBitCbk{nullptr};
  defrSiteCbkFnType SiteCbk{nullptr};
  defrSiteCbkFnType CanplaceCbk{nullptr};
  defrSiteCbkFnType CannotOccupyCbk{nullptr};
  defrIntegerCbkFnType ComponentStartCbk{nullptr};
  defrVoidCbkFnType ComponentEndCbk{nullptr};
  defrComponentCbkFnType ComponentCbk{nullptr};
  defrIntegerCbkFnType NetStartCbk{nullptr};
  defrVoidCbkFnType NetEndCbk{nullptr};
  defrNetCbkFnType NetCbk{nullptr};
  defrStringCbkFnType NetNameCbk{nullptr};
  defrStringCbkFnType NetSubnetNameCbk{nullptr};
  defrStringCbkFnType NetNonDefaultRuleCbk{nullptr};
  defrNetCbkFnType NetPartialPathCbk{nullptr};
  defrPathCbkFnType PathCbk{nullptr};
  defrDoubleCbkFnType VersionCbk{nullptr};
  defrStringCbkFnType VersionStrCbk{nullptr};
  defrStringCbkFnType PinExtCbk{nullptr};
  defrStringCbkFnType ComponentExtCbk{nullptr};
  defrStringCbkFnType ViaExtCbk{nullptr};
  defrStringCbkFnType NetConnectionExtCbk{nullptr};
  defrStringCbkFnType NetExtCbk{nullptr};
  defrStringCbkFnType GroupExtCbk{nullptr};
  defrStringCbkFnType ScanChainExtCbk{nullptr};
  defrStringCbkFnType IoTimingsExtCbk{nullptr};
  defrStringCbkFnType PartitionsExtCbk{nullptr};
  defrStringCbkFnType HistoryCbk{nullptr};
  defrBoxCbkFnType DieAreaCbk{nullptr};
  defrPinCapCbkFnType PinCapCbk{nullptr};
  defrPinCbkFnType PinCbk{nullptr};
  defrIntegerCbkFnType StartPinsCbk{nullptr};
  defrVoidCbkFnType PinEndCbk{nullptr};
  defrIntegerCbkFnType DefaultCapCbk{nullptr};
  defrRowCbkFnType RowCbk{nullptr};
  defrTrackCbkFnType TrackCbk{nullptr};
  defrGcellGridCbkFnType GcellGridCbk{nullptr};
  defrIntegerCbkFnType ViaStartCbk{nullptr};
  defrVoidCbkFnType ViaEndCbk{nullptr};
  defrViaCbkFnType ViaCbk{nullptr};
  defrIntegerCbkFnType RegionStartCbk{nullptr};
  defrVoidCbkFnType RegionEndCbk{nullptr};
  defrRegionCbkFnType RegionCbk{nullptr};
  defrIntegerCbkFnType SNetStartCbk{nullptr};
  defrVoidCbkFnType SNetEndCbk{nullptr};
  defrNetCbkFnType SNetCbk{nullptr};
  defrNetCbkFnType SNetPartialPathCbk{nullptr};
  defrNetCbkFnType SNetWireCbk{nullptr};
  defrIntegerCbkFnType GroupsStartCbk{nullptr};
  defrVoidCbkFnType GroupsEndCbk{nullptr};
  defrStringCbkFnType GroupNameCbk{nullptr};
  defrStringCbkFnType GroupMemberCbk{nullptr};
  defrGroupCbkFnType GroupCbk{nullptr};
  defrIntegerCbkFnType AssertionsStartCbk{nullptr};
  defrVoidCbkFnType AssertionsEndCbk{nullptr};
  defrAssertionCbkFnType AssertionCbk{nullptr};
  defrIntegerCbkFnType ConstraintsStartCbk{nullptr};
  defrVoidCbkFnType ConstraintsEndCbk{nullptr};
  defrAssertionCbkFnType ConstraintCbk{nullptr};
  defrIntegerCbkFnType ScanchainsStartCbk{nullptr};
  defrVoidCbkFnType ScanchainsEndCbk{nullptr};
  defrScanchainCbkFnType ScanchainCbk{nullptr};
  defrIntegerCbkFnType IOTimingsStartCbk{nullptr};
  defrVoidCbkFnType IOTimingsEndCbk{nullptr};
  defrIOTimingCbkFnType IOTimingCbk{nullptr};
  defrIntegerCbkFnType FPCStartCbk{nullptr};
  defrVoidCbkFnType FPCEndCbk{nullptr};
  defrFPCCbkFnType FPCCbk{nullptr};
  defrIntegerCbkFnType TimingDisablesStartCbk{nullptr};
  defrVoidCbkFnType TimingDisablesEndCbk{nullptr};
  defrTimingDisableCbkFnType TimingDisableCbk{nullptr};
  defrIntegerCbkFnType PartitionsStartCbk{nullptr};
  defrVoidCbkFnType PartitionsEndCbk{nullptr};
  defrPartitionCbkFnType PartitionCbk{nullptr};
  defrIntegerCbkFnType PinPropStartCbk{nullptr};
  defrVoidCbkFnType PinPropEndCbk{nullptr};
  defrPinPropCbkFnType PinPropCbk{nullptr};
  defrIntegerCbkFnType CaseSensitiveCbk{nullptr};
  defrIntegerCbkFnType BlockageStartCbk{nullptr};
  defrVoidCbkFnType BlockageEndCbk{nullptr};
  defrBlockageCbkFnType BlockageCbk{nullptr};
  defrIntegerCbkFnType SlotStartCbk{nullptr};
  defrVoidCbkFnType SlotEndCbk{nullptr};
  defrSlotCbkFnType SlotCbk{nullptr};
  defrIntegerCbkFnType FillStartCbk{nullptr};
  defrVoidCbkFnType FillEndCbk{nullptr};
  defrFillCbkFnType FillCbk{nullptr};
  defrIntegerCbkFnType NonDefaultStartCbk{nullptr};
  defrVoidCbkFnType NonDefaultEndCbk{nullptr};
  defrNonDefaultCbkFnType NonDefaultCbk{nullptr};
  defrIntegerCbkFnType StylesStartCbk{nullptr};
  defrVoidCbkFnType StylesEndCbk{nullptr};
  defrStylesCbkFnType StylesCbk{nullptr};
  defrStringCbkFnType ExtensionCbk{nullptr};
  defrComponentMaskShiftLayerCbkFnType ComponentMaskShiftLayerCbk{nullptr};
};

END_DEF_PARSER_NAMESPACE

#endif
