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
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2017/06/06 $
//  $State: xxx $
// *****************************************************************************
// *****************************************************************************

#define EXTERN extern "C"

#include "lefrReader.h"
#include "lefrReader.hpp"

// Wrappers definitions.
int lefrInit () {
    return LefDefParser::lefrInit();
}

int lefrInitSession (int  startSession) {
    return LefDefParser::lefrInitSession(startSession);
}

int lefrReset () {
    return LefDefParser::lefrReset();
}

int lefrClear () {
    return LefDefParser::lefrClear();
}

int lefrReleaseNResetMemory () {
    return LefDefParser::lefrReleaseNResetMemory();
}

void lefrSetCommentChar (char  c) {
    LefDefParser::lefrSetCommentChar(c);
}

void lefrSetShiftCase () {
    LefDefParser::lefrSetShiftCase();
}

void lefrSetCaseSensitivity (int  caseSense) {
    LefDefParser::lefrSetCaseSensitivity(caseSense);
}

const char * lefrFName () {
    return LefDefParser::lefrFName();
}

int lefrRead (FILE * file, const char * fileName, lefiUserData  userData) {
    return LefDefParser::lefrRead(file, fileName, userData);
}

void lefrSetRegisterUnusedCallbacks () {
    LefDefParser::lefrSetRegisterUnusedCallbacks();
}

void lefrPrintUnusedCallbacks (FILE*  f) {
    LefDefParser::lefrPrintUnusedCallbacks(f);
}

void lefrSetUserData (lefiUserData p0) {
    LefDefParser::lefrSetUserData(p0);
}

lefiUserData lefrGetUserData () {
    return LefDefParser::lefrGetUserData();
}

void lefrSetUnitsCbk (::lefrUnitsCbkFnType p0) {
    LefDefParser::lefrSetUnitsCbk((LefDefParser::lefrUnitsCbkFnType) p0);
}

void lefrSetVersionCbk (::lefrDoubleCbkFnType p0) {
    LefDefParser::lefrSetVersionCbk((LefDefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetVersionStrCbk (::lefrStringCbkFnType p0) {
    LefDefParser::lefrSetVersionStrCbk((LefDefParser::lefrStringCbkFnType) p0);
}

void lefrSetDividerCharCbk (::lefrStringCbkFnType p0) {
    LefDefParser::lefrSetDividerCharCbk((LefDefParser::lefrStringCbkFnType) p0);
}

void lefrSetBusBitCharsCbk (::lefrStringCbkFnType p0) {
    LefDefParser::lefrSetBusBitCharsCbk((LefDefParser::lefrStringCbkFnType) p0);
}

void lefrSetNoWireExtensionCbk (::lefrStringCbkFnType p0) {
    LefDefParser::lefrSetNoWireExtensionCbk((LefDefParser::lefrStringCbkFnType) p0);
}

void lefrSetCaseSensitiveCbk (::lefrIntegerCbkFnType p0) {
    LefDefParser::lefrSetCaseSensitiveCbk((LefDefParser::lefrIntegerCbkFnType) p0);
}

void lefrSetPropBeginCbk (::lefrVoidCbkFnType p0) {
    LefDefParser::lefrSetPropBeginCbk((LefDefParser::lefrVoidCbkFnType) p0);
}

void lefrSetPropCbk (::lefrPropCbkFnType p0) {
    LefDefParser::lefrSetPropCbk((LefDefParser::lefrPropCbkFnType) p0);
}

void lefrSetPropEndCbk (::lefrVoidCbkFnType p0) {
    LefDefParser::lefrSetPropEndCbk((LefDefParser::lefrVoidCbkFnType) p0);
}

void lefrSetLayerCbk (::lefrLayerCbkFnType p0) {
    LefDefParser::lefrSetLayerCbk((LefDefParser::lefrLayerCbkFnType) p0);
}

void lefrSetViaCbk (::lefrViaCbkFnType p0) {
    LefDefParser::lefrSetViaCbk((LefDefParser::lefrViaCbkFnType) p0);
}

void lefrSetViaRuleCbk (::lefrViaRuleCbkFnType p0) {
    LefDefParser::lefrSetViaRuleCbk((LefDefParser::lefrViaRuleCbkFnType) p0);
}

void lefrSetSpacingCbk (::lefrSpacingCbkFnType p0) {
    LefDefParser::lefrSetSpacingCbk((LefDefParser::lefrSpacingCbkFnType) p0);
}

void lefrSetIRDropCbk (::lefrIRDropCbkFnType p0) {
    LefDefParser::lefrSetIRDropCbk((LefDefParser::lefrIRDropCbkFnType) p0);
}

void lefrSetDielectricCbk (::lefrDoubleCbkFnType p0) {
    LefDefParser::lefrSetDielectricCbk((LefDefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetMinFeatureCbk (::lefrMinFeatureCbkFnType p0) {
    LefDefParser::lefrSetMinFeatureCbk((LefDefParser::lefrMinFeatureCbkFnType) p0);
}

void lefrSetNonDefaultCbk (::lefrNonDefaultCbkFnType p0) {
    LefDefParser::lefrSetNonDefaultCbk((LefDefParser::lefrNonDefaultCbkFnType) p0);
}

void lefrSetSiteCbk (::lefrSiteCbkFnType p0) {
    LefDefParser::lefrSetSiteCbk((LefDefParser::lefrSiteCbkFnType) p0);
}

void lefrSetMacroBeginCbk (::lefrStringCbkFnType p0) {
    LefDefParser::lefrSetMacroBeginCbk((LefDefParser::lefrStringCbkFnType) p0);
}

void lefrSetPinCbk (::lefrPinCbkFnType p0) {
    LefDefParser::lefrSetPinCbk((LefDefParser::lefrPinCbkFnType) p0);
}

void lefrSetObstructionCbk (::lefrObstructionCbkFnType p0) {
    LefDefParser::lefrSetObstructionCbk((LefDefParser::lefrObstructionCbkFnType) p0);
}

void lefrSetArrayCbk (::lefrArrayCbkFnType p0) {
    LefDefParser::lefrSetArrayCbk((LefDefParser::lefrArrayCbkFnType) p0);
}

void lefrSetMacroCbk (::lefrMacroCbkFnType p0) {
    LefDefParser::lefrSetMacroCbk((LefDefParser::lefrMacroCbkFnType) p0);
}

void lefrSetLibraryEndCbk (::lefrVoidCbkFnType p0) {
    LefDefParser::lefrSetLibraryEndCbk((LefDefParser::lefrVoidCbkFnType) p0);
}

void lefrSetTimingCbk (::lefrTimingCbkFnType p0) {
    LefDefParser::lefrSetTimingCbk((LefDefParser::lefrTimingCbkFnType) p0);
}

void lefrSetSpacingBeginCbk (::lefrVoidCbkFnType p0) {
    LefDefParser::lefrSetSpacingBeginCbk((LefDefParser::lefrVoidCbkFnType) p0);
}

void lefrSetSpacingEndCbk (::lefrVoidCbkFnType p0) {
    LefDefParser::lefrSetSpacingEndCbk((LefDefParser::lefrVoidCbkFnType) p0);
}

void lefrSetArrayBeginCbk (::lefrStringCbkFnType p0) {
    LefDefParser::lefrSetArrayBeginCbk((LefDefParser::lefrStringCbkFnType) p0);
}

void lefrSetArrayEndCbk (::lefrStringCbkFnType p0) {
    LefDefParser::lefrSetArrayEndCbk((LefDefParser::lefrStringCbkFnType) p0);
}

void lefrSetIRDropBeginCbk (::lefrVoidCbkFnType p0) {
    LefDefParser::lefrSetIRDropBeginCbk((LefDefParser::lefrVoidCbkFnType) p0);
}

void lefrSetIRDropEndCbk (::lefrVoidCbkFnType p0) {
    LefDefParser::lefrSetIRDropEndCbk((LefDefParser::lefrVoidCbkFnType) p0);
}

void lefrSetNoiseMarginCbk (::lefrNoiseMarginCbkFnType p0) {
    LefDefParser::lefrSetNoiseMarginCbk((LefDefParser::lefrNoiseMarginCbkFnType) p0);
}

void lefrSetEdgeRateThreshold1Cbk (::lefrDoubleCbkFnType p0) {
    LefDefParser::lefrSetEdgeRateThreshold1Cbk((LefDefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetEdgeRateThreshold2Cbk (::lefrDoubleCbkFnType p0) {
    LefDefParser::lefrSetEdgeRateThreshold2Cbk((LefDefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetEdgeRateScaleFactorCbk (::lefrDoubleCbkFnType p0) {
    LefDefParser::lefrSetEdgeRateScaleFactorCbk((LefDefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetNoiseTableCbk (::lefrNoiseTableCbkFnType p0) {
    LefDefParser::lefrSetNoiseTableCbk((LefDefParser::lefrNoiseTableCbkFnType) p0);
}

void lefrSetCorrectionTableCbk (::lefrCorrectionTableCbkFnType p0) {
    LefDefParser::lefrSetCorrectionTableCbk((LefDefParser::lefrCorrectionTableCbkFnType) p0);
}

void lefrSetInputAntennaCbk (::lefrDoubleCbkFnType p0) {
    LefDefParser::lefrSetInputAntennaCbk((LefDefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetOutputAntennaCbk (::lefrDoubleCbkFnType p0) {
    LefDefParser::lefrSetOutputAntennaCbk((LefDefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetInoutAntennaCbk (::lefrDoubleCbkFnType p0) {
    LefDefParser::lefrSetInoutAntennaCbk((LefDefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetAntennaInputCbk (::lefrDoubleCbkFnType p0) {
    LefDefParser::lefrSetAntennaInputCbk((LefDefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetAntennaInoutCbk (::lefrDoubleCbkFnType p0) {
    LefDefParser::lefrSetAntennaInoutCbk((LefDefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetAntennaOutputCbk (::lefrDoubleCbkFnType p0) {
    LefDefParser::lefrSetAntennaOutputCbk((LefDefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetClearanceMeasureCbk (::lefrStringCbkFnType p0) {
    LefDefParser::lefrSetClearanceMeasureCbk((LefDefParser::lefrStringCbkFnType) p0);
}

void lefrSetManufacturingCbk (::lefrDoubleCbkFnType p0) {
    LefDefParser::lefrSetManufacturingCbk((LefDefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetUseMinSpacingCbk (::lefrUseMinSpacingCbkFnType p0) {
    LefDefParser::lefrSetUseMinSpacingCbk((LefDefParser::lefrUseMinSpacingCbkFnType) p0);
}

void lefrSetMacroClassTypeCbk (::lefrStringCbkFnType p0) {
    LefDefParser::lefrSetMacroClassTypeCbk((LefDefParser::lefrStringCbkFnType) p0);
}

void lefrSetMacroOriginCbk (::lefrMacroNumCbkFnType p0) {
    LefDefParser::lefrSetMacroOriginCbk((LefDefParser::lefrMacroNumCbkFnType) p0);
}

void lefrSetMacroSizeCbk (::lefrMacroNumCbkFnType p0) {
    LefDefParser::lefrSetMacroSizeCbk((LefDefParser::lefrMacroNumCbkFnType) p0);
}

void lefrSetMacroFixedMaskCbk (::lefrIntegerCbkFnType p0) {
    LefDefParser::lefrSetMacroFixedMaskCbk((LefDefParser::lefrIntegerCbkFnType) p0);
}

void lefrSetMacroEndCbk (::lefrStringCbkFnType p0) {
    LefDefParser::lefrSetMacroEndCbk((LefDefParser::lefrStringCbkFnType) p0);
}

void lefrSetMaxStackViaCbk (::lefrMaxStackViaCbkFnType p0) {
    LefDefParser::lefrSetMaxStackViaCbk((LefDefParser::lefrMaxStackViaCbkFnType) p0);
}

void lefrSetExtensionCbk (::lefrStringCbkFnType p0) {
    LefDefParser::lefrSetExtensionCbk((LefDefParser::lefrStringCbkFnType) p0);
}

void lefrSetDensityCbk (::lefrDensityCbkFnType p0) {
    LefDefParser::lefrSetDensityCbk((LefDefParser::lefrDensityCbkFnType) p0);
}

void lefrSetFixedMaskCbk (::lefrIntegerCbkFnType p0) {
    LefDefParser::lefrSetFixedMaskCbk((LefDefParser::lefrIntegerCbkFnType) p0);
}

void lefrSetUnusedCallbacks (::lefrVoidCbkFnType  func) {
    LefDefParser::lefrSetUnusedCallbacks((LefDefParser::lefrVoidCbkFnType ) func);
}

void lefrUnsetCallbacks () {
    LefDefParser::lefrUnsetCallbacks();
}

void lefrUnsetAntennaInputCbk () {
    LefDefParser::lefrUnsetAntennaInputCbk();
}

void lefrUnsetAntennaInoutCbk () {
    LefDefParser::lefrUnsetAntennaInoutCbk();
}

void lefrUnsetAntennaOutputCbk () {
    LefDefParser::lefrUnsetAntennaOutputCbk();
}

void lefrUnsetArrayBeginCbk () {
    LefDefParser::lefrUnsetArrayBeginCbk();
}

void lefrUnsetArrayCbk () {
    LefDefParser::lefrUnsetArrayCbk();
}

void lefrUnsetArrayEndCbk () {
    LefDefParser::lefrUnsetArrayEndCbk();
}

void lefrUnsetBusBitCharsCbk () {
    LefDefParser::lefrUnsetBusBitCharsCbk();
}

void lefrUnsetCaseSensitiveCbk () {
    LefDefParser::lefrUnsetCaseSensitiveCbk();
}

void lefrUnsetClearanceMeasureCbk () {
    LefDefParser::lefrUnsetClearanceMeasureCbk();
}

void lefrUnsetCorrectionTableCbk () {
    LefDefParser::lefrUnsetCorrectionTableCbk();
}

void lefrUnsetDensityCbk () {
    LefDefParser::lefrUnsetDensityCbk();
}

void lefrUnsetDielectricCbk () {
    LefDefParser::lefrUnsetDielectricCbk();
}

void lefrUnsetDividerCharCbk () {
    LefDefParser::lefrUnsetDividerCharCbk();
}

void lefrUnsetEdgeRateScaleFactorCbk () {
    LefDefParser::lefrUnsetEdgeRateScaleFactorCbk();
}

void lefrUnsetEdgeRateThreshold1Cbk () {
    LefDefParser::lefrUnsetEdgeRateThreshold1Cbk();
}

void lefrUnsetEdgeRateThreshold2Cbk () {
    LefDefParser::lefrUnsetEdgeRateThreshold2Cbk();
}

void lefrUnsetExtensionCbk () {
    LefDefParser::lefrUnsetExtensionCbk();
}

void lefrUnsetInoutAntennaCbk () {
    LefDefParser::lefrUnsetInoutAntennaCbk();
}

void lefrUnsetInputAntennaCbk () {
    LefDefParser::lefrUnsetInputAntennaCbk();
}

void lefrUnsetIRDropBeginCbk () {
    LefDefParser::lefrUnsetIRDropBeginCbk();
}

void lefrUnsetIRDropCbk () {
    LefDefParser::lefrUnsetIRDropCbk();
}

void lefrUnsetIRDropEndCbk () {
    LefDefParser::lefrUnsetIRDropEndCbk();
}

void lefrUnsetLayerCbk () {
    LefDefParser::lefrUnsetLayerCbk();
}

void lefrUnsetLibraryEndCbk () {
    LefDefParser::lefrUnsetLibraryEndCbk();
}

void lefrUnsetMacroBeginCbk () {
    LefDefParser::lefrUnsetMacroBeginCbk();
}

void lefrUnsetMacroCbk () {
    LefDefParser::lefrUnsetMacroCbk();
}

void lefrUnsetMacroClassTypeCbk () {
    LefDefParser::lefrUnsetMacroClassTypeCbk();
}

void lefrUnsetMacroEndCbk () {
    LefDefParser::lefrUnsetMacroEndCbk();
}

void lefrUnsetMacroOriginCbk () {
    LefDefParser::lefrUnsetMacroOriginCbk();
}

void lefrUnsetMacroSizeCbk () {
    LefDefParser::lefrUnsetMacroSizeCbk();
}

void lefrUnsetManufacturingCbk () {
    LefDefParser::lefrUnsetManufacturingCbk();
}

void lefrUnsetMaxStackViaCbk () {
    LefDefParser::lefrUnsetMaxStackViaCbk();
}

void lefrUnsetMinFeatureCbk () {
    LefDefParser::lefrUnsetMinFeatureCbk();
}

void lefrUnsetNoiseMarginCbk () {
    LefDefParser::lefrUnsetNoiseMarginCbk();
}

void lefrUnsetNoiseTableCbk () {
    LefDefParser::lefrUnsetNoiseTableCbk();
}

void lefrUnsetNonDefaultCbk () {
    LefDefParser::lefrUnsetNonDefaultCbk();
}

void lefrUnsetNoWireExtensionCbk () {
    LefDefParser::lefrUnsetNoWireExtensionCbk();
}

void lefrUnsetObstructionCbk () {
    LefDefParser::lefrUnsetObstructionCbk();
}

void lefrUnsetOutputAntennaCbk () {
    LefDefParser::lefrUnsetOutputAntennaCbk();
}

void lefrUnsetPinCbk () {
    LefDefParser::lefrUnsetPinCbk();
}

void lefrUnsetPropBeginCbk () {
    LefDefParser::lefrUnsetPropBeginCbk();
}

void lefrUnsetPropCbk () {
    LefDefParser::lefrUnsetPropCbk();
}

void lefrUnsetPropEndCbk () {
    LefDefParser::lefrUnsetPropEndCbk();
}

void lefrUnsetSiteCbk () {
    LefDefParser::lefrUnsetSiteCbk();
}

void lefrUnsetSpacingBeginCbk () {
    LefDefParser::lefrUnsetSpacingBeginCbk();
}

void lefrUnsetSpacingCbk () {
    LefDefParser::lefrUnsetSpacingCbk();
}

void lefrUnsetSpacingEndCbk () {
    LefDefParser::lefrUnsetSpacingEndCbk();
}

void lefrUnsetTimingCbk () {
    LefDefParser::lefrUnsetTimingCbk();
}

void lefrUnsetUseMinSpacingCbk () {
    LefDefParser::lefrUnsetUseMinSpacingCbk();
}

void lefrUnsetUnitsCbk () {
    LefDefParser::lefrUnsetUnitsCbk();
}

void lefrUnsetVersionCbk () {
    LefDefParser::lefrUnsetVersionCbk();
}

void lefrUnsetVersionStrCbk () {
    LefDefParser::lefrUnsetVersionStrCbk();
}

void lefrUnsetViaCbk () {
    LefDefParser::lefrUnsetViaCbk();
}

void lefrUnsetViaRuleCbk () {
    LefDefParser::lefrUnsetViaRuleCbk();
}

int lefrLineNumber () {
    return LefDefParser::lefrLineNumber();
}

void lefrSetLogFunction (::LEFI_LOG_FUNCTION p0) {
    LefDefParser::lefrSetLogFunction(p0);
}

void lefrSetWarningLogFunction (::LEFI_WARNING_LOG_FUNCTION p0) {
    LefDefParser::lefrSetWarningLogFunction(p0);
}

void lefrSetMallocFunction (::LEFI_MALLOC_FUNCTION p0) {
    LefDefParser::lefrSetMallocFunction(p0);
}

void lefrSetReallocFunction (::LEFI_REALLOC_FUNCTION p0) {
    LefDefParser::lefrSetReallocFunction(p0);
}

void lefrSetFreeFunction (::LEFI_FREE_FUNCTION p0) {
    LefDefParser::lefrSetFreeFunction(p0);
}

void lefrSetLineNumberFunction (::LEFI_LINE_NUMBER_FUNCTION p0) {
    LefDefParser::lefrSetLineNumberFunction(p0);
}

void lefrSetDeltaNumberLines (int p0) {
    LefDefParser::lefrSetDeltaNumberLines(p0);
}

void lefrSetRelaxMode () {
    LefDefParser::lefrSetRelaxMode();
}

void lefrUnsetRelaxMode () {
    LefDefParser::lefrUnsetRelaxMode();
}

void lefrSetVersionValue (const  char*   version) {
    LefDefParser::lefrSetVersionValue(version);
}

void lefrSetReadFunction (::LEFI_READ_FUNCTION p0) {
    LefDefParser::lefrSetReadFunction(p0);
}

void lefrUnsetReadFunction () {
    LefDefParser::lefrUnsetReadFunction();
}

void lefrSetOpenLogFileAppend () {
    LefDefParser::lefrSetOpenLogFileAppend();
}

void lefrUnsetOpenLogFileAppend () {
    LefDefParser::lefrUnsetOpenLogFileAppend();
}

void lefrDisablePropStrProcess () {
    LefDefParser::lefrDisablePropStrProcess();
}

void lefrSetAntennaInoutWarnings (int  warn) {
    LefDefParser::lefrSetAntennaInoutWarnings(warn);
}

void lefrSetAntennaInputWarnings (int  warn) {
    LefDefParser::lefrSetAntennaInputWarnings(warn);
}

void lefrSetAntennaOutputWarnings (int  warn) {
    LefDefParser::lefrSetAntennaOutputWarnings(warn);
}

void lefrSetArrayWarnings (int  warn) {
    LefDefParser::lefrSetArrayWarnings(warn);
}

void lefrSetCaseSensitiveWarnings (int  warn) {
    LefDefParser::lefrSetCaseSensitiveWarnings(warn);
}

void lefrSetCorrectionTableWarnings (int  warn) {
    LefDefParser::lefrSetCorrectionTableWarnings(warn);
}

void lefrSetDielectricWarnings (int  warn) {
    LefDefParser::lefrSetDielectricWarnings(warn);
}

void lefrSetEdgeRateThreshold1Warnings (int  warn) {
    LefDefParser::lefrSetEdgeRateThreshold1Warnings(warn);
}

void lefrSetEdgeRateThreshold2Warnings (int  warn) {
    LefDefParser::lefrSetEdgeRateThreshold2Warnings(warn);
}

void lefrSetEdgeRateScaleFactorWarnings (int  warn) {
    LefDefParser::lefrSetEdgeRateScaleFactorWarnings(warn);
}

void lefrSetInoutAntennaWarnings (int  warn) {
    LefDefParser::lefrSetInoutAntennaWarnings(warn);
}

void lefrSetInputAntennaWarnings (int  warn) {
    LefDefParser::lefrSetInputAntennaWarnings(warn);
}

void lefrSetIRDropWarnings (int  warn) {
    LefDefParser::lefrSetIRDropWarnings(warn);
}

void lefrSetLayerWarnings (int  warn) {
    LefDefParser::lefrSetLayerWarnings(warn);
}

void lefrSetMacroWarnings (int  warn) {
    LefDefParser::lefrSetMacroWarnings(warn);
}

void lefrSetMaxStackViaWarnings (int  warn) {
    LefDefParser::lefrSetMaxStackViaWarnings(warn);
}

void lefrSetMinFeatureWarnings (int  warn) {
    LefDefParser::lefrSetMinFeatureWarnings(warn);
}

void lefrSetNoiseMarginWarnings (int  warn) {
    LefDefParser::lefrSetNoiseMarginWarnings(warn);
}

void lefrSetNoiseTableWarnings (int  warn) {
    LefDefParser::lefrSetNoiseTableWarnings(warn);
}

void lefrSetNonDefaultWarnings (int  warn) {
    LefDefParser::lefrSetNonDefaultWarnings(warn);
}

void lefrSetNoWireExtensionWarnings (int  warn) {
    LefDefParser::lefrSetNoWireExtensionWarnings(warn);
}

void lefrSetOutputAntennaWarnings (int  warn) {
    LefDefParser::lefrSetOutputAntennaWarnings(warn);
}

void lefrSetPinWarnings (int  warn) {
    LefDefParser::lefrSetPinWarnings(warn);
}

void lefrSetSiteWarnings (int  warn) {
    LefDefParser::lefrSetSiteWarnings(warn);
}

void lefrSetSpacingWarnings (int  warn) {
    LefDefParser::lefrSetSpacingWarnings(warn);
}

void lefrSetTimingWarnings (int  warn) {
    LefDefParser::lefrSetTimingWarnings(warn);
}

void lefrSetUnitsWarnings (int  warn) {
    LefDefParser::lefrSetUnitsWarnings(warn);
}

void lefrSetUseMinSpacingWarnings (int  warn) {
    LefDefParser::lefrSetUseMinSpacingWarnings(warn);
}

void lefrSetViaRuleWarnings (int  warn) {
    LefDefParser::lefrSetViaRuleWarnings(warn);
}

void lefrSetViaWarnings (int  warn) {
    LefDefParser::lefrSetViaWarnings(warn);
}

void lefrDisableParserMsgs (int  nMsg, int*  msgs) {
    LefDefParser::lefrDisableParserMsgs(nMsg, msgs);
}

void lefrEnableParserMsgs (int  nMsg, int*  msgs) {
    LefDefParser::lefrEnableParserMsgs(nMsg, msgs);
}

void lefrEnableAllMsgs () {
    LefDefParser::lefrEnableAllMsgs();
}

void lefrDisableAllMsgs () {
    LefDefParser::lefrDisableAllMsgs();
}

void lefrSetTotalMsgLimit (int  totNumMsgs) {
    LefDefParser::lefrSetTotalMsgLimit(totNumMsgs);
}

void lefrSetLimitPerMsg (int  msgId, int  numMsg) {
    LefDefParser::lefrSetLimitPerMsg(msgId, numMsg);
}

