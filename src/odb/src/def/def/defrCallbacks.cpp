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
//  $Author: arakhman $
//  $Revision: #5 $
//  $Date: 2013/03/13 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#include "defrCallBacks.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

defrCallbacks::defrCallbacks()
: DesignCbk(NULL),
  TechnologyCbk(NULL),
  DesignEndCbk(NULL),
  PropCbk(NULL),
  PropDefEndCbk(NULL),
  PropDefStartCbk(NULL),
  ArrayNameCbk(NULL),
  FloorPlanNameCbk(NULL),
  UnitsCbk(NULL),
  DividerCbk(NULL),
  BusBitCbk(NULL),
  SiteCbk(NULL),
  CanplaceCbk(NULL),
  CannotOccupyCbk(NULL),
  ComponentStartCbk(NULL),
  ComponentEndCbk(NULL),
  ComponentCbk(NULL),
  NetStartCbk(NULL),
  NetEndCbk(NULL),
  NetCbk(NULL),
  NetNameCbk(NULL),
  NetSubnetNameCbk(NULL),
  NetNonDefaultRuleCbk(NULL),
  NetPartialPathCbk(NULL),
  PathCbk(NULL),
  VersionCbk(NULL),
  VersionStrCbk(NULL),
  PinExtCbk(NULL),
  ComponentExtCbk(NULL),
  ViaExtCbk(NULL),
  NetConnectionExtCbk(NULL),
  NetExtCbk(NULL),
  GroupExtCbk(NULL),
  ScanChainExtCbk(NULL),
  IoTimingsExtCbk(NULL),
  PartitionsExtCbk(NULL),
  HistoryCbk(NULL),
  DieAreaCbk(NULL),
  PinCapCbk(NULL),
  PinCbk(NULL),
  StartPinsCbk(NULL),
  PinEndCbk(NULL),
  DefaultCapCbk(NULL),
  RowCbk(NULL),
  TrackCbk(NULL),
  GcellGridCbk(NULL),
  ViaStartCbk(NULL),
  ViaEndCbk(NULL),
  ViaCbk(NULL),
  RegionStartCbk(NULL),
  RegionEndCbk(NULL),
  RegionCbk(NULL),
  SNetStartCbk(NULL),
  SNetEndCbk(NULL),
  SNetCbk(NULL),
  SNetPartialPathCbk(NULL),
  SNetWireCbk(NULL),
  GroupsStartCbk(NULL),
  GroupsEndCbk(NULL),
  GroupNameCbk(NULL),
  GroupMemberCbk(NULL),
  ComponentMaskShiftLayerCbk(NULL),
  GroupCbk(NULL),
  AssertionsStartCbk(NULL),
  AssertionsEndCbk(NULL),
  AssertionCbk(NULL),
  ConstraintsStartCbk(NULL),
  ConstraintsEndCbk(NULL),
  ConstraintCbk(NULL),
  ScanchainsStartCbk(NULL),
  ScanchainsEndCbk(NULL),
  ScanchainCbk(NULL),
  IOTimingsStartCbk(NULL),
  IOTimingsEndCbk(NULL),
  IOTimingCbk(NULL),
  FPCStartCbk(NULL),
  FPCEndCbk(NULL),
  FPCCbk(NULL),
  TimingDisablesStartCbk(NULL),
  TimingDisablesEndCbk(NULL),
  TimingDisableCbk(NULL),
  PartitionsStartCbk(NULL),
  PartitionsEndCbk(NULL),
  PartitionCbk(NULL),
  PinPropStartCbk(NULL),
  PinPropEndCbk(NULL),
  PinPropCbk(NULL),
  CaseSensitiveCbk(NULL),
  BlockageStartCbk(NULL),
  BlockageEndCbk(NULL),
  BlockageCbk(NULL),
  SlotStartCbk(NULL),
  SlotEndCbk(NULL),
  SlotCbk(NULL),
  FillStartCbk(NULL),
  FillEndCbk(NULL),
  FillCbk(NULL),
  NonDefaultStartCbk(NULL),
  NonDefaultEndCbk(NULL),
  NonDefaultCbk(NULL),
  StylesStartCbk(NULL),
  StylesEndCbk(NULL),
  StylesCbk(NULL),
  ExtensionCbk(NULL)
{
}


void
defrCallbacks::SetUnusedCallbacks(defrVoidCbkFnType f)
{
    if (!DesignCbk)
        DesignCbk = (defrStringCbkFnType) f;
    if (!TechnologyCbk)
        TechnologyCbk = (defrStringCbkFnType) f;
    if (!DesignEndCbk)
        DesignEndCbk = (defrVoidCbkFnType) f;
    if (!PropCbk)
        PropCbk = (defrPropCbkFnType) f;
    if (!PropDefEndCbk)
        PropDefEndCbk = (defrVoidCbkFnType) f;
    if (!PropDefStartCbk)
        PropDefStartCbk = (defrVoidCbkFnType) f;
    if (!ArrayNameCbk)
        ArrayNameCbk = (defrStringCbkFnType) f;
    if (!FloorPlanNameCbk)
        FloorPlanNameCbk = (defrStringCbkFnType) f;
    if (!UnitsCbk)
        UnitsCbk = (defrDoubleCbkFnType) f;
    if (!DividerCbk)
        DividerCbk = (defrStringCbkFnType) f;
    if (!BusBitCbk)
        BusBitCbk = (defrStringCbkFnType) f;
    if (!SiteCbk)
        SiteCbk = (defrSiteCbkFnType) f;
    if (!CanplaceCbk)
        CanplaceCbk = (defrSiteCbkFnType) f;
    if (!CannotOccupyCbk)
        CannotOccupyCbk = (defrSiteCbkFnType) f;
    if (!ComponentStartCbk)
        ComponentStartCbk = (defrIntegerCbkFnType) f;
    if (!ComponentEndCbk)
        ComponentEndCbk = (defrVoidCbkFnType) f;
    if (!ComponentCbk)
        ComponentCbk = (defrComponentCbkFnType) f;
    if (!NetStartCbk)
        NetStartCbk = (defrIntegerCbkFnType) f;
    if (!NetEndCbk)
        NetEndCbk = (defrVoidCbkFnType) f;
    if (!NetCbk)
        NetCbk = (defrNetCbkFnType) f;
    //  if (! defrNetPartialPathCbk) defrNetPartialPathCbk = (defrNetCbkFnType)f;
    if (!PathCbk)
        PathCbk = (defrPathCbkFnType) f;
    if ((!VersionCbk) && (!VersionStrCbk)) {
        // both version callbacks weren't set, if either one is set, it is ok
        VersionCbk = (defrDoubleCbkFnType) f;
        VersionStrCbk = (defrStringCbkFnType) f;
    }
    if (!PinExtCbk)
        PinExtCbk = (defrStringCbkFnType) f;
    if (!ComponentExtCbk)
        ComponentExtCbk = (defrStringCbkFnType) f;
    if (!ViaExtCbk)
        ViaExtCbk = (defrStringCbkFnType) f;
    if (!NetConnectionExtCbk)
        NetConnectionExtCbk = (defrStringCbkFnType) f;
    if (!NetExtCbk)
        NetExtCbk = (defrStringCbkFnType) f;
    if (!GroupExtCbk)
        GroupExtCbk = (defrStringCbkFnType) f;
    if (!ScanChainExtCbk)
        ScanChainExtCbk = (defrStringCbkFnType) f;
    if (!IoTimingsExtCbk)
        IoTimingsExtCbk = (defrStringCbkFnType) f;
    if (!PartitionsExtCbk)
        PartitionsExtCbk = (defrStringCbkFnType) f;
    if (!HistoryCbk)
        HistoryCbk = (defrStringCbkFnType) f;
    if (!DieAreaCbk)
        DieAreaCbk = (defrBoxCbkFnType) f;
    if (!PinCapCbk)
        PinCapCbk = (defrPinCapCbkFnType) f;
    if (!PinCbk)
        PinCbk = (defrPinCbkFnType) f;
    if (!StartPinsCbk)
        StartPinsCbk = (defrIntegerCbkFnType) f;
    if (!PinEndCbk)
        PinEndCbk = (defrVoidCbkFnType) f;
    if (!DefaultCapCbk)
        DefaultCapCbk = (defrIntegerCbkFnType) f;
    if (!RowCbk)
        RowCbk = (defrRowCbkFnType) f;
    if (!TrackCbk)
        TrackCbk = (defrTrackCbkFnType) f;
    if (!GcellGridCbk)
        GcellGridCbk = (defrGcellGridCbkFnType) f;
    if (!ViaStartCbk)
        ViaStartCbk = (defrIntegerCbkFnType) f;
    if (!ViaEndCbk)
        ViaEndCbk = (defrVoidCbkFnType) f;
    if (!ViaCbk)
        ViaCbk = (defrViaCbkFnType) f;
    if (!RegionStartCbk)
        RegionStartCbk = (defrIntegerCbkFnType) f;
    if (!RegionEndCbk)
        RegionEndCbk = (defrVoidCbkFnType) f;
    if (!RegionCbk)
        RegionCbk = (defrRegionCbkFnType) f;
    if (!SNetStartCbk)
        SNetStartCbk = (defrIntegerCbkFnType) f;
    if (!SNetEndCbk)
        SNetEndCbk = (defrVoidCbkFnType) f;
    if (!SNetCbk)
        SNetCbk = (defrNetCbkFnType) f;
    //  if(! defrSNetPartialPathCbk) defrSNetPartialPathCbk = (defrNetCbkFnType)f;
    //  if(! defrSNetWireCbk) defrSNetWireCbk = (defrNetCbkFnType)f;
    if (!GroupsStartCbk)
        GroupsStartCbk = (defrIntegerCbkFnType) f;
    if (!GroupsEndCbk)
        GroupsEndCbk = (defrVoidCbkFnType) f;
    if (!GroupNameCbk)
        GroupNameCbk = (defrStringCbkFnType) f;
    if (!GroupMemberCbk)
        GroupMemberCbk = (defrStringCbkFnType) f;
    if (!ComponentMaskShiftLayerCbk)
        ComponentMaskShiftLayerCbk = (defrComponentMaskShiftLayerCbkFnType) f;
    if (!GroupCbk)
        GroupCbk = (defrGroupCbkFnType) f;
    if (!AssertionsStartCbk)
        AssertionsStartCbk = (defrIntegerCbkFnType) f;
    if (!AssertionsEndCbk)
        AssertionsEndCbk = (defrVoidCbkFnType) f;
    if (!AssertionCbk)
        AssertionCbk = (defrAssertionCbkFnType) f;
    if (!ConstraintsStartCbk)
        ConstraintsStartCbk = (defrIntegerCbkFnType) f;
    if (!ConstraintsEndCbk)
        ConstraintsEndCbk = (defrVoidCbkFnType) f;
    if (!ConstraintCbk)
        ConstraintCbk = (defrAssertionCbkFnType) f;
    if (!ScanchainsStartCbk)
        ScanchainsStartCbk = (defrIntegerCbkFnType) f;
    if (!ScanchainsEndCbk)
        ScanchainsEndCbk = (defrVoidCbkFnType) f;
    if (!ScanchainCbk)
        ScanchainCbk = (defrScanchainCbkFnType) f;
    if (!IOTimingsStartCbk)
        IOTimingsStartCbk = (defrIntegerCbkFnType) f;
    if (!IOTimingsEndCbk)
        IOTimingsEndCbk = (defrVoidCbkFnType) f;
    if (!IOTimingCbk)
        IOTimingCbk = (defrIOTimingCbkFnType) f;
    if (!FPCStartCbk)
        FPCStartCbk = (defrIntegerCbkFnType) f;
    if (!FPCEndCbk)
        FPCEndCbk = (defrVoidCbkFnType) f;
    if (!FPCCbk)
        FPCCbk = (defrFPCCbkFnType) f;
    if (!TimingDisablesStartCbk)
        TimingDisablesStartCbk = (defrIntegerCbkFnType) f;
    if (!TimingDisablesEndCbk)
        TimingDisablesEndCbk = (defrVoidCbkFnType) f;
    if (!TimingDisableCbk)
        TimingDisableCbk = (defrTimingDisableCbkFnType) f;
    if (!PartitionsStartCbk)
        PartitionsStartCbk = (defrIntegerCbkFnType) f;
    if (!PartitionsEndCbk)
        PartitionsEndCbk = (defrVoidCbkFnType) f;
    if (!PartitionCbk)
        PartitionCbk = (defrPartitionCbkFnType) f;
    if (!PinPropStartCbk)
        PinPropStartCbk = (defrIntegerCbkFnType) f;
    if (!PinPropEndCbk)
        PinPropEndCbk = (defrVoidCbkFnType) f;
    if (!PinPropCbk)
        PinPropCbk = (defrPinPropCbkFnType) f;
    if (!CaseSensitiveCbk)
        CaseSensitiveCbk = (defrIntegerCbkFnType) f;
    if (!BlockageStartCbk)
        BlockageStartCbk = (defrIntegerCbkFnType) f;
    if (!BlockageEndCbk)
        BlockageEndCbk = (defrVoidCbkFnType) f;
    if (!BlockageCbk)
        BlockageCbk = (defrBlockageCbkFnType) f;
    if (!SlotStartCbk)
        SlotStartCbk = (defrIntegerCbkFnType) f;
    if (!SlotEndCbk)
        SlotEndCbk = (defrVoidCbkFnType) f;
    if (!SlotCbk)
        SlotCbk = (defrSlotCbkFnType) f;
    if (!FillStartCbk)
        FillStartCbk = (defrIntegerCbkFnType) f;
    if (!FillEndCbk)
        FillEndCbk = (defrVoidCbkFnType) f;
    if (!FillCbk)
        FillCbk = (defrFillCbkFnType) f;
    if (!NonDefaultStartCbk)
        NonDefaultStartCbk = (defrIntegerCbkFnType) f;
    if (!NonDefaultEndCbk)
        NonDefaultEndCbk = (defrVoidCbkFnType) f;
    if (!NonDefaultCbk)
        NonDefaultCbk = (defrNonDefaultCbkFnType) f;
    if (!StylesStartCbk)
        StylesStartCbk = (defrIntegerCbkFnType) f;
    if (!StylesEndCbk)
        StylesEndCbk = (defrVoidCbkFnType) f;
    if (!StylesCbk)
        StylesCbk = (defrStylesCbkFnType) f;
    if (!ExtensionCbk)
        ExtensionCbk = (defrStringCbkFnType) f;

    /* NEW CALLBACK - Each new callback must have an entry here. */
}


END_LEFDEF_PARSER_NAMESPACE
