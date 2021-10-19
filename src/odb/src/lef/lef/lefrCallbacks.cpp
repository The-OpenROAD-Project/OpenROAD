// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2017, Cadence Design Systems
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
//  $Revision: #11 $
//  $Date: 2013/04/23 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#include "lefrCallBacks.hpp"
#include "string.h"

BEGIN_LEFDEF_PARSER_NAMESPACE

lefrCallbacks::lefrCallbacks()
: AntennaInoutCbk(0),
  AntennaInputCbk(0),
  AntennaOutputCbk(0),
  ArrayBeginCbk(0),
  ArrayCbk(0),
  ArrayEndCbk(0),
  BusBitCharsCbk(0),
  CaseSensitiveCbk(0),
  ClearanceMeasureCbk(0),
  CorrectionTableCbk(0),
  DensityCbk(0),
  DielectricCbk(0),
  DividerCharCbk(0),
  EdgeRateScaleFactorCbk(0),
  EdgeRateThreshold1Cbk(0),
  EdgeRateThreshold2Cbk(0),
  ExtensionCbk(0),
  FixedMaskCbk(0),
  IRDropBeginCbk(0),
  IRDropCbk(0),
  IRDropEndCbk(0),
  InoutAntennaCbk(0),
  InputAntennaCbk(0),
  LayerCbk(0),
  LibraryEndCbk(0),
  MacroBeginCbk(0),
  MacroCbk(0),
  MacroClassTypeCbk(0),
  MacroEndCbk(0),
  MacroFixedMaskCbk(0),
  MacroOriginCbk(0),
  MacroSiteCbk(0),
  MacroForeignCbk(0),
  MacroSizeCbk(0),
  ManufacturingCbk(0),
  MaxStackViaCbk(0),
  MinFeatureCbk(0),
  NoWireExtensionCbk(0),
  NoiseMarginCbk(0),
  NoiseTableCbk(0),
  NonDefaultCbk(0),
  ObstructionCbk(0),
  OutputAntennaCbk(0),
  PinCbk(0),
  PropBeginCbk(0),
  PropCbk(0),
  PropEndCbk(0),
  SiteCbk(0),
  SpacingBeginCbk(0),
  SpacingCbk(0),
  SpacingEndCbk(0),
  TimingCbk(0),
  UnitsCbk(0),
  UseMinSpacingCbk(0),
  VersionCbk(0),
  VersionStrCbk(0),
  ViaCbk(0),
  ViaRuleCbk(0)
{
}

lefrCallbacks *lefCallbacks = NULL;

void
lefrCallbacks::reset()
{
    if (lefCallbacks) {
        delete lefCallbacks;
    }

    lefCallbacks = new lefrCallbacks();
}

END_LEFDEF_PARSER_NAMESPACE
