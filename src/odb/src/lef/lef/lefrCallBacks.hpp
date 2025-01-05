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
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2020/09/29 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#ifndef lefrCallbacks_h
#define lefrCallbacks_h

#include "lefiKRDefs.hpp"
#include "lefrReader.hpp"

BEGIN_LEF_PARSER_NAMESPACE

class lefrCallbacks
{
 public:
  static void reset();

  // List of call back routines
  // These are filled in by the user.  See the
  // "set" routines at the end of the file
  lefrDoubleCbkFnType AntennaInoutCbk{nullptr};
  lefrDoubleCbkFnType AntennaInputCbk{nullptr};
  lefrDoubleCbkFnType AntennaOutputCbk{nullptr};
  lefrStringCbkFnType ArrayBeginCbk{nullptr};
  lefrArrayCbkFnType ArrayCbk{nullptr};
  lefrStringCbkFnType ArrayEndCbk{nullptr};
  lefrStringCbkFnType BusBitCharsCbk{nullptr};
  lefrIntegerCbkFnType CaseSensitiveCbk{nullptr};
  lefrStringCbkFnType ClearanceMeasureCbk{nullptr};
  lefrCorrectionTableCbkFnType CorrectionTableCbk{nullptr};
  lefrDensityCbkFnType DensityCbk{nullptr};
  lefrDoubleCbkFnType DielectricCbk{nullptr};
  lefrStringCbkFnType DividerCharCbk{nullptr};
  lefrDoubleCbkFnType EdgeRateScaleFactorCbk{nullptr};
  lefrDoubleCbkFnType EdgeRateThreshold1Cbk{nullptr};
  lefrDoubleCbkFnType EdgeRateThreshold2Cbk{nullptr};
  lefrStringCbkFnType ExtensionCbk{nullptr};
  lefrIntegerCbkFnType FixedMaskCbk{nullptr};
  lefrVoidCbkFnType IRDropBeginCbk{nullptr};
  lefrIRDropCbkFnType IRDropCbk{nullptr};
  lefrVoidCbkFnType IRDropEndCbk{nullptr};
  lefrDoubleCbkFnType InoutAntennaCbk{nullptr};
  lefrDoubleCbkFnType InputAntennaCbk{nullptr};
  lefrLayerCbkFnType LayerCbk{nullptr};
  lefrVoidCbkFnType LibraryEndCbk{nullptr};
  lefrStringCbkFnType MacroBeginCbk{nullptr};
  lefrMacroCbkFnType MacroCbk{nullptr};
  lefrStringCbkFnType MacroClassTypeCbk{nullptr};
  lefrStringCbkFnType MacroEndCbk{nullptr};
  lefrIntegerCbkFnType MacroFixedMaskCbk{nullptr};
  lefrMacroNumCbkFnType MacroOriginCbk{nullptr};
  lefrMacroSiteCbkFnType MacroSiteCbk{nullptr};
  lefrMacroForeignCbkFnType MacroForeignCbk{nullptr};
  lefrMacroNumCbkFnType MacroSizeCbk{nullptr};
  lefrDoubleCbkFnType ManufacturingCbk{nullptr};
  lefrMaxStackViaCbkFnType MaxStackViaCbk{nullptr};
  lefrMinFeatureCbkFnType MinFeatureCbk{nullptr};
  lefrStringCbkFnType NoWireExtensionCbk{nullptr};
  lefrNoiseMarginCbkFnType NoiseMarginCbk{nullptr};
  lefrNoiseTableCbkFnType NoiseTableCbk{nullptr};
  lefrNonDefaultCbkFnType NonDefaultCbk{nullptr};
  lefrObstructionCbkFnType ObstructionCbk{nullptr};
  lefrDoubleCbkFnType OutputAntennaCbk{nullptr};
  lefrPinCbkFnType PinCbk{nullptr};
  lefrVoidCbkFnType PropBeginCbk{nullptr};
  lefrPropCbkFnType PropCbk{nullptr};
  lefrVoidCbkFnType PropEndCbk{nullptr};
  lefrSiteCbkFnType SiteCbk{nullptr};
  lefrVoidCbkFnType SpacingBeginCbk{nullptr};
  lefrSpacingCbkFnType SpacingCbk{nullptr};
  lefrVoidCbkFnType SpacingEndCbk{nullptr};
  lefrTimingCbkFnType TimingCbk{nullptr};
  lefrUnitsCbkFnType UnitsCbk{nullptr};
  lefrUseMinSpacingCbkFnType UseMinSpacingCbk{nullptr};
  lefrDoubleCbkFnType VersionCbk{nullptr};
  lefrStringCbkFnType VersionStrCbk{nullptr};
  lefrViaCbkFnType ViaCbk{nullptr};
  lefrViaRuleCbkFnType ViaRuleCbk{nullptr};
};

extern lefrCallbacks* lefCallbacks;

END_LEF_PARSER_NAMESPACE

#endif
