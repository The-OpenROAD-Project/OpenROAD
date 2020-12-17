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
//  $Date: 2017/06/06 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#ifndef lefrCallbacks_h
#define lefrCallbacks_h

#include "lefiKRDefs.hpp"
#include "lefrReader.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

class lefrCallbacks {
public:
    lefrCallbacks();
    static void reset();

    // List of call back routines
    // These are filled in by the user.  See the
    // "set" routines at the end of the file
    lefrDoubleCbkFnType AntennaInoutCbk;
    lefrDoubleCbkFnType AntennaInputCbk;
    lefrDoubleCbkFnType AntennaOutputCbk;
    lefrStringCbkFnType ArrayBeginCbk;
    lefrArrayCbkFnType ArrayCbk;
    lefrStringCbkFnType ArrayEndCbk;
    lefrStringCbkFnType BusBitCharsCbk;
    lefrIntegerCbkFnType CaseSensitiveCbk;
    lefrStringCbkFnType ClearanceMeasureCbk;
    lefrCorrectionTableCbkFnType CorrectionTableCbk;
    lefrDensityCbkFnType DensityCbk;
    lefrDoubleCbkFnType DielectricCbk;
    lefrStringCbkFnType DividerCharCbk;
    lefrDoubleCbkFnType EdgeRateScaleFactorCbk;
    lefrDoubleCbkFnType EdgeRateThreshold1Cbk;
    lefrDoubleCbkFnType EdgeRateThreshold2Cbk;
    lefrStringCbkFnType ExtensionCbk;
    lefrIntegerCbkFnType FixedMaskCbk;
    lefrVoidCbkFnType IRDropBeginCbk;
    lefrIRDropCbkFnType IRDropCbk;
    lefrVoidCbkFnType IRDropEndCbk;
    lefrDoubleCbkFnType InoutAntennaCbk;
    lefrDoubleCbkFnType InputAntennaCbk;
    lefrLayerCbkFnType LayerCbk;
    lefrVoidCbkFnType LibraryEndCbk;
    lefrStringCbkFnType MacroBeginCbk;
    lefrMacroCbkFnType MacroCbk;
    lefrStringCbkFnType MacroClassTypeCbk;
    lefrStringCbkFnType MacroEndCbk;
    lefrIntegerCbkFnType MacroFixedMaskCbk;
    lefrMacroNumCbkFnType MacroOriginCbk;
    lefrMacroSiteCbkFnType MacroSiteCbk;
    lefrMacroForeignCbkFnType MacroForeignCbk;
    lefrMacroNumCbkFnType MacroSizeCbk;
    lefrDoubleCbkFnType ManufacturingCbk;
    lefrMaxStackViaCbkFnType MaxStackViaCbk;
    lefrMinFeatureCbkFnType MinFeatureCbk;
    lefrStringCbkFnType NoWireExtensionCbk;
    lefrNoiseMarginCbkFnType NoiseMarginCbk;
    lefrNoiseTableCbkFnType NoiseTableCbk;
    lefrNonDefaultCbkFnType NonDefaultCbk;
    lefrObstructionCbkFnType ObstructionCbk;
    lefrDoubleCbkFnType OutputAntennaCbk;
    lefrPinCbkFnType PinCbk;
    lefrVoidCbkFnType PropBeginCbk;
    lefrPropCbkFnType PropCbk;
    lefrVoidCbkFnType PropEndCbk;
    lefrSiteCbkFnType SiteCbk;
    lefrVoidCbkFnType SpacingBeginCbk;
    lefrSpacingCbkFnType SpacingCbk;
    lefrVoidCbkFnType SpacingEndCbk;
    lefrTimingCbkFnType TimingCbk;
    lefrUnitsCbkFnType UnitsCbk;
    lefrUseMinSpacingCbkFnType UseMinSpacingCbk;
    lefrDoubleCbkFnType VersionCbk;
    lefrStringCbkFnType VersionStrCbk;
    lefrViaCbkFnType ViaCbk;
    lefrViaRuleCbkFnType ViaRuleCbk;
};

extern lefrCallbacks *lefCallbacks;

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
