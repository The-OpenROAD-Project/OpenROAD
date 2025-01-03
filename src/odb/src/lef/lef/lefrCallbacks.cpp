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

#include <cstring>

BEGIN_LEFDEF_PARSER_NAMESPACE

lefrCallbacks::lefrCallbacks()
    : AntennaInoutCbk(nullptr),
      AntennaInputCbk(nullptr),
      AntennaOutputCbk(nullptr),
      ArrayBeginCbk(nullptr),
      ArrayCbk(nullptr),
      ArrayEndCbk(nullptr),
      BusBitCharsCbk(nullptr),
      CaseSensitiveCbk(nullptr),
      ClearanceMeasureCbk(nullptr),
      CorrectionTableCbk(nullptr),
      DensityCbk(nullptr),
      DielectricCbk(nullptr),
      DividerCharCbk(nullptr),
      EdgeRateScaleFactorCbk(nullptr),
      EdgeRateThreshold1Cbk(nullptr),
      EdgeRateThreshold2Cbk(nullptr),
      ExtensionCbk(nullptr),
      FixedMaskCbk(nullptr),
      IRDropBeginCbk(nullptr),
      IRDropCbk(nullptr),
      IRDropEndCbk(nullptr),
      InoutAntennaCbk(nullptr),
      InputAntennaCbk(nullptr),
      LayerCbk(nullptr),
      LibraryEndCbk(nullptr),
      MacroBeginCbk(nullptr),
      MacroCbk(nullptr),
      MacroClassTypeCbk(nullptr),
      MacroEndCbk(nullptr),
      MacroFixedMaskCbk(nullptr),
      MacroOriginCbk(nullptr),
      MacroSiteCbk(nullptr),
      MacroForeignCbk(nullptr),
      MacroSizeCbk(nullptr),
      ManufacturingCbk(nullptr),
      MaxStackViaCbk(nullptr),
      MinFeatureCbk(nullptr),
      NoWireExtensionCbk(nullptr),
      NoiseMarginCbk(nullptr),
      NoiseTableCbk(nullptr),
      NonDefaultCbk(nullptr),
      ObstructionCbk(nullptr),
      OutputAntennaCbk(nullptr),
      PinCbk(nullptr),
      PropBeginCbk(nullptr),
      PropCbk(nullptr),
      PropEndCbk(nullptr),
      SiteCbk(nullptr),
      SpacingBeginCbk(nullptr),
      SpacingCbk(nullptr),
      SpacingEndCbk(nullptr),
      TimingCbk(nullptr),
      UnitsCbk(nullptr),
      UseMinSpacingCbk(nullptr),
      VersionCbk(nullptr),
      VersionStrCbk(nullptr),
      ViaCbk(nullptr),
      ViaRuleCbk(nullptr)
{
}

lefrCallbacks* lefCallbacks = nullptr;

void lefrCallbacks::reset()
{
  if (lefCallbacks) {
    delete lefCallbacks;
  }

  lefCallbacks = new lefrCallbacks();
}

END_LEFDEF_PARSER_NAMESPACE
