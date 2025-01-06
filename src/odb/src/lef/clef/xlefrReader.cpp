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
//  $Date: 2020/09/29 $
//  $State: xxx $
// *****************************************************************************
// *****************************************************************************

#define EXTERN extern "C"

#include "lefrReader.h"
#include "lefrReader.hpp"

// Wrappers definitions.
int lefrInit()
{
  return LefParser::lefrInit();
}

int lefrInitSession(int startSession)
{
  return LefParser::lefrInitSession(startSession);
}

int lefrReset()
{
  return LefParser::lefrReset();
}

int lefrClear()
{
  return LefParser::lefrClear();
}

int lefrReleaseNResetMemory()
{
  return LefParser::lefrReleaseNResetMemory();
}

void lefrSetCommentChar(char c)
{
  LefParser::lefrSetCommentChar(c);
}

void lefrSetShiftCase()
{
  LefParser::lefrSetShiftCase();
}

void lefrSetCaseSensitivity(int caseSense)
{
  LefParser::lefrSetCaseSensitivity(caseSense);
}

const char* lefrFName()
{
  return LefParser::lefrFName();
}

int lefrRead(FILE* file, const char* fileName, lefiUserData userData)
{
  return LefParser::lefrRead(file, fileName, userData);
}

void lefrSetRegisterUnusedCallbacks()
{
  LefParser::lefrSetRegisterUnusedCallbacks();
}

void lefrPrintUnusedCallbacks(FILE* f)
{
  LefParser::lefrPrintUnusedCallbacks(f);
}

void lefrSetUserData(lefiUserData p0)
{
  LefParser::lefrSetUserData(p0);
}

lefiUserData lefrGetUserData()
{
  return LefParser::lefrGetUserData();
}

void lefrSetUnitsCbk(::lefrUnitsCbkFnType p0)
{
  LefParser::lefrSetUnitsCbk((LefParser::lefrUnitsCbkFnType) p0);
}

void lefrSetVersionCbk(::lefrDoubleCbkFnType p0)
{
  LefParser::lefrSetVersionCbk((LefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetVersionStrCbk(::lefrStringCbkFnType p0)
{
  LefParser::lefrSetVersionStrCbk((LefParser::lefrStringCbkFnType) p0);
}

void lefrSetDividerCharCbk(::lefrStringCbkFnType p0)
{
  LefParser::lefrSetDividerCharCbk((LefParser::lefrStringCbkFnType) p0);
}

void lefrSetBusBitCharsCbk(::lefrStringCbkFnType p0)
{
  LefParser::lefrSetBusBitCharsCbk((LefParser::lefrStringCbkFnType) p0);
}

void lefrSetNoWireExtensionCbk(::lefrStringCbkFnType p0)
{
  LefParser::lefrSetNoWireExtensionCbk((LefParser::lefrStringCbkFnType) p0);
}

void lefrSetCaseSensitiveCbk(::lefrIntegerCbkFnType p0)
{
  LefParser::lefrSetCaseSensitiveCbk((LefParser::lefrIntegerCbkFnType) p0);
}

void lefrSetPropBeginCbk(::lefrVoidCbkFnType p0)
{
  LefParser::lefrSetPropBeginCbk((LefParser::lefrVoidCbkFnType) p0);
}

void lefrSetPropCbk(::lefrPropCbkFnType p0)
{
  LefParser::lefrSetPropCbk((LefParser::lefrPropCbkFnType) p0);
}

void lefrSetPropEndCbk(::lefrVoidCbkFnType p0)
{
  LefParser::lefrSetPropEndCbk((LefParser::lefrVoidCbkFnType) p0);
}

void lefrSetLayerCbk(::lefrLayerCbkFnType p0)
{
  LefParser::lefrSetLayerCbk((LefParser::lefrLayerCbkFnType) p0);
}

void lefrSetViaCbk(::lefrViaCbkFnType p0)
{
  LefParser::lefrSetViaCbk((LefParser::lefrViaCbkFnType) p0);
}

void lefrSetViaRuleCbk(::lefrViaRuleCbkFnType p0)
{
  LefParser::lefrSetViaRuleCbk((LefParser::lefrViaRuleCbkFnType) p0);
}

void lefrSetSpacingCbk(::lefrSpacingCbkFnType p0)
{
  LefParser::lefrSetSpacingCbk((LefParser::lefrSpacingCbkFnType) p0);
}

void lefrSetIRDropCbk(::lefrIRDropCbkFnType p0)
{
  LefParser::lefrSetIRDropCbk((LefParser::lefrIRDropCbkFnType) p0);
}

void lefrSetDielectricCbk(::lefrDoubleCbkFnType p0)
{
  LefParser::lefrSetDielectricCbk((LefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetMinFeatureCbk(::lefrMinFeatureCbkFnType p0)
{
  LefParser::lefrSetMinFeatureCbk((LefParser::lefrMinFeatureCbkFnType) p0);
}

void lefrSetNonDefaultCbk(::lefrNonDefaultCbkFnType p0)
{
  LefParser::lefrSetNonDefaultCbk((LefParser::lefrNonDefaultCbkFnType) p0);
}

void lefrSetSiteCbk(::lefrSiteCbkFnType p0)
{
  LefParser::lefrSetSiteCbk((LefParser::lefrSiteCbkFnType) p0);
}

void lefrSetMacroBeginCbk(::lefrStringCbkFnType p0)
{
  LefParser::lefrSetMacroBeginCbk((LefParser::lefrStringCbkFnType) p0);
}

void lefrSetPinCbk(::lefrPinCbkFnType p0)
{
  LefParser::lefrSetPinCbk((LefParser::lefrPinCbkFnType) p0);
}

void lefrSetObstructionCbk(::lefrObstructionCbkFnType p0)
{
  LefParser::lefrSetObstructionCbk((LefParser::lefrObstructionCbkFnType) p0);
}

void lefrSetArrayCbk(::lefrArrayCbkFnType p0)
{
  LefParser::lefrSetArrayCbk((LefParser::lefrArrayCbkFnType) p0);
}

void lefrSetMacroCbk(::lefrMacroCbkFnType p0)
{
  LefParser::lefrSetMacroCbk((LefParser::lefrMacroCbkFnType) p0);
}

void lefrSetLibraryEndCbk(::lefrVoidCbkFnType p0)
{
  LefParser::lefrSetLibraryEndCbk((LefParser::lefrVoidCbkFnType) p0);
}

void lefrSetTimingCbk(::lefrTimingCbkFnType p0)
{
  LefParser::lefrSetTimingCbk((LefParser::lefrTimingCbkFnType) p0);
}

void lefrSetSpacingBeginCbk(::lefrVoidCbkFnType p0)
{
  LefParser::lefrSetSpacingBeginCbk((LefParser::lefrVoidCbkFnType) p0);
}

void lefrSetSpacingEndCbk(::lefrVoidCbkFnType p0)
{
  LefParser::lefrSetSpacingEndCbk((LefParser::lefrVoidCbkFnType) p0);
}

void lefrSetArrayBeginCbk(::lefrStringCbkFnType p0)
{
  LefParser::lefrSetArrayBeginCbk((LefParser::lefrStringCbkFnType) p0);
}

void lefrSetArrayEndCbk(::lefrStringCbkFnType p0)
{
  LefParser::lefrSetArrayEndCbk((LefParser::lefrStringCbkFnType) p0);
}

void lefrSetIRDropBeginCbk(::lefrVoidCbkFnType p0)
{
  LefParser::lefrSetIRDropBeginCbk((LefParser::lefrVoidCbkFnType) p0);
}

void lefrSetIRDropEndCbk(::lefrVoidCbkFnType p0)
{
  LefParser::lefrSetIRDropEndCbk((LefParser::lefrVoidCbkFnType) p0);
}

void lefrSetNoiseMarginCbk(::lefrNoiseMarginCbkFnType p0)
{
  LefParser::lefrSetNoiseMarginCbk((LefParser::lefrNoiseMarginCbkFnType) p0);
}

void lefrSetEdgeRateThreshold1Cbk(::lefrDoubleCbkFnType p0)
{
  LefParser::lefrSetEdgeRateThreshold1Cbk((LefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetEdgeRateThreshold2Cbk(::lefrDoubleCbkFnType p0)
{
  LefParser::lefrSetEdgeRateThreshold2Cbk((LefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetEdgeRateScaleFactorCbk(::lefrDoubleCbkFnType p0)
{
  LefParser::lefrSetEdgeRateScaleFactorCbk((LefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetNoiseTableCbk(::lefrNoiseTableCbkFnType p0)
{
  LefParser::lefrSetNoiseTableCbk((LefParser::lefrNoiseTableCbkFnType) p0);
}

void lefrSetCorrectionTableCbk(::lefrCorrectionTableCbkFnType p0)
{
  LefParser::lefrSetCorrectionTableCbk(
      (LefParser::lefrCorrectionTableCbkFnType) p0);
}

void lefrSetInputAntennaCbk(::lefrDoubleCbkFnType p0)
{
  LefParser::lefrSetInputAntennaCbk((LefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetOutputAntennaCbk(::lefrDoubleCbkFnType p0)
{
  LefParser::lefrSetOutputAntennaCbk((LefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetInoutAntennaCbk(::lefrDoubleCbkFnType p0)
{
  LefParser::lefrSetInoutAntennaCbk((LefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetAntennaInputCbk(::lefrDoubleCbkFnType p0)
{
  LefParser::lefrSetAntennaInputCbk((LefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetAntennaInoutCbk(::lefrDoubleCbkFnType p0)
{
  LefParser::lefrSetAntennaInoutCbk((LefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetAntennaOutputCbk(::lefrDoubleCbkFnType p0)
{
  LefParser::lefrSetAntennaOutputCbk((LefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetClearanceMeasureCbk(::lefrStringCbkFnType p0)
{
  LefParser::lefrSetClearanceMeasureCbk((LefParser::lefrStringCbkFnType) p0);
}

void lefrSetManufacturingCbk(::lefrDoubleCbkFnType p0)
{
  LefParser::lefrSetManufacturingCbk((LefParser::lefrDoubleCbkFnType) p0);
}

void lefrSetUseMinSpacingCbk(::lefrUseMinSpacingCbkFnType p0)
{
  LefParser::lefrSetUseMinSpacingCbk(
      (LefParser::lefrUseMinSpacingCbkFnType) p0);
}

void lefrSetMacroClassTypeCbk(::lefrStringCbkFnType p0)
{
  LefParser::lefrSetMacroClassTypeCbk((LefParser::lefrStringCbkFnType) p0);
}

void lefrSetMacroOriginCbk(::lefrMacroNumCbkFnType p0)
{
  LefParser::lefrSetMacroOriginCbk((LefParser::lefrMacroNumCbkFnType) p0);
}

void lefrSetMacroSizeCbk(::lefrMacroNumCbkFnType p0)
{
  LefParser::lefrSetMacroSizeCbk((LefParser::lefrMacroNumCbkFnType) p0);
}

void lefrSetMacroFixedMaskCbk(::lefrIntegerCbkFnType p0)
{
  LefParser::lefrSetMacroFixedMaskCbk((LefParser::lefrIntegerCbkFnType) p0);
}

void lefrSetMacroEndCbk(::lefrStringCbkFnType p0)
{
  LefParser::lefrSetMacroEndCbk((LefParser::lefrStringCbkFnType) p0);
}

void lefrSetMaxStackViaCbk(::lefrMaxStackViaCbkFnType p0)
{
  LefParser::lefrSetMaxStackViaCbk((LefParser::lefrMaxStackViaCbkFnType) p0);
}

void lefrSetExtensionCbk(::lefrStringCbkFnType p0)
{
  LefParser::lefrSetExtensionCbk((LefParser::lefrStringCbkFnType) p0);
}

void lefrSetDensityCbk(::lefrDensityCbkFnType p0)
{
  LefParser::lefrSetDensityCbk((LefParser::lefrDensityCbkFnType) p0);
}

void lefrSetFixedMaskCbk(::lefrIntegerCbkFnType p0)
{
  LefParser::lefrSetFixedMaskCbk((LefParser::lefrIntegerCbkFnType) p0);
}

void lefrSetUnusedCallbacks(::lefrVoidCbkFnType func)
{
  LefParser::lefrSetUnusedCallbacks((LefParser::lefrVoidCbkFnType) func);
}

void lefrUnsetCallbacks()
{
  LefParser::lefrUnsetCallbacks();
}

void lefrUnsetAntennaInputCbk()
{
  LefParser::lefrUnsetAntennaInputCbk();
}

void lefrUnsetAntennaInoutCbk()
{
  LefParser::lefrUnsetAntennaInoutCbk();
}

void lefrUnsetAntennaOutputCbk()
{
  LefParser::lefrUnsetAntennaOutputCbk();
}

void lefrUnsetArrayBeginCbk()
{
  LefParser::lefrUnsetArrayBeginCbk();
}

void lefrUnsetArrayCbk()
{
  LefParser::lefrUnsetArrayCbk();
}

void lefrUnsetArrayEndCbk()
{
  LefParser::lefrUnsetArrayEndCbk();
}

void lefrUnsetBusBitCharsCbk()
{
  LefParser::lefrUnsetBusBitCharsCbk();
}

void lefrUnsetCaseSensitiveCbk()
{
  LefParser::lefrUnsetCaseSensitiveCbk();
}

void lefrUnsetClearanceMeasureCbk()
{
  LefParser::lefrUnsetClearanceMeasureCbk();
}

void lefrUnsetCorrectionTableCbk()
{
  LefParser::lefrUnsetCorrectionTableCbk();
}

void lefrUnsetDensityCbk()
{
  LefParser::lefrUnsetDensityCbk();
}

void lefrUnsetDielectricCbk()
{
  LefParser::lefrUnsetDielectricCbk();
}

void lefrUnsetDividerCharCbk()
{
  LefParser::lefrUnsetDividerCharCbk();
}

void lefrUnsetEdgeRateScaleFactorCbk()
{
  LefParser::lefrUnsetEdgeRateScaleFactorCbk();
}

void lefrUnsetEdgeRateThreshold1Cbk()
{
  LefParser::lefrUnsetEdgeRateThreshold1Cbk();
}

void lefrUnsetEdgeRateThreshold2Cbk()
{
  LefParser::lefrUnsetEdgeRateThreshold2Cbk();
}

void lefrUnsetExtensionCbk()
{
  LefParser::lefrUnsetExtensionCbk();
}

void lefrUnsetInoutAntennaCbk()
{
  LefParser::lefrUnsetInoutAntennaCbk();
}

void lefrUnsetInputAntennaCbk()
{
  LefParser::lefrUnsetInputAntennaCbk();
}

void lefrUnsetIRDropBeginCbk()
{
  LefParser::lefrUnsetIRDropBeginCbk();
}

void lefrUnsetIRDropCbk()
{
  LefParser::lefrUnsetIRDropCbk();
}

void lefrUnsetIRDropEndCbk()
{
  LefParser::lefrUnsetIRDropEndCbk();
}

void lefrUnsetLayerCbk()
{
  LefParser::lefrUnsetLayerCbk();
}

void lefrUnsetLibraryEndCbk()
{
  LefParser::lefrUnsetLibraryEndCbk();
}

void lefrUnsetMacroBeginCbk()
{
  LefParser::lefrUnsetMacroBeginCbk();
}

void lefrUnsetMacroCbk()
{
  LefParser::lefrUnsetMacroCbk();
}

void lefrUnsetMacroClassTypeCbk()
{
  LefParser::lefrUnsetMacroClassTypeCbk();
}

void lefrUnsetMacroEndCbk()
{
  LefParser::lefrUnsetMacroEndCbk();
}

void lefrUnsetMacroOriginCbk()
{
  LefParser::lefrUnsetMacroOriginCbk();
}

void lefrUnsetMacroSizeCbk()
{
  LefParser::lefrUnsetMacroSizeCbk();
}

void lefrUnsetManufacturingCbk()
{
  LefParser::lefrUnsetManufacturingCbk();
}

void lefrUnsetMaxStackViaCbk()
{
  LefParser::lefrUnsetMaxStackViaCbk();
}

void lefrUnsetMinFeatureCbk()
{
  LefParser::lefrUnsetMinFeatureCbk();
}

void lefrUnsetNoiseMarginCbk()
{
  LefParser::lefrUnsetNoiseMarginCbk();
}

void lefrUnsetNoiseTableCbk()
{
  LefParser::lefrUnsetNoiseTableCbk();
}

void lefrUnsetNonDefaultCbk()
{
  LefParser::lefrUnsetNonDefaultCbk();
}

void lefrUnsetNoWireExtensionCbk()
{
  LefParser::lefrUnsetNoWireExtensionCbk();
}

void lefrUnsetObstructionCbk()
{
  LefParser::lefrUnsetObstructionCbk();
}

void lefrUnsetOutputAntennaCbk()
{
  LefParser::lefrUnsetOutputAntennaCbk();
}

void lefrUnsetPinCbk()
{
  LefParser::lefrUnsetPinCbk();
}

void lefrUnsetPropBeginCbk()
{
  LefParser::lefrUnsetPropBeginCbk();
}

void lefrUnsetPropCbk()
{
  LefParser::lefrUnsetPropCbk();
}

void lefrUnsetPropEndCbk()
{
  LefParser::lefrUnsetPropEndCbk();
}

void lefrUnsetSiteCbk()
{
  LefParser::lefrUnsetSiteCbk();
}

void lefrUnsetSpacingBeginCbk()
{
  LefParser::lefrUnsetSpacingBeginCbk();
}

void lefrUnsetSpacingCbk()
{
  LefParser::lefrUnsetSpacingCbk();
}

void lefrUnsetSpacingEndCbk()
{
  LefParser::lefrUnsetSpacingEndCbk();
}

void lefrUnsetTimingCbk()
{
  LefParser::lefrUnsetTimingCbk();
}

void lefrUnsetUseMinSpacingCbk()
{
  LefParser::lefrUnsetUseMinSpacingCbk();
}

void lefrUnsetUnitsCbk()
{
  LefParser::lefrUnsetUnitsCbk();
}

void lefrUnsetVersionCbk()
{
  LefParser::lefrUnsetVersionCbk();
}

void lefrUnsetVersionStrCbk()
{
  LefParser::lefrUnsetVersionStrCbk();
}

void lefrUnsetViaCbk()
{
  LefParser::lefrUnsetViaCbk();
}

void lefrUnsetViaRuleCbk()
{
  LefParser::lefrUnsetViaRuleCbk();
}

int lefrLineNumber()
{
  return LefParser::lefrLineNumber();
}

void lefrSetLogFunction(::LEFI_LOG_FUNCTION p0)
{
  LefParser::lefrSetLogFunction(p0);
}

void lefrSetWarningLogFunction(::LEFI_WARNING_LOG_FUNCTION p0)
{
  LefParser::lefrSetWarningLogFunction(p0);
}

void lefrSetMallocFunction(::LEFI_MALLOC_FUNCTION p0)
{
  LefParser::lefrSetMallocFunction(p0);
}

void lefrSetReallocFunction(::LEFI_REALLOC_FUNCTION p0)
{
  LefParser::lefrSetReallocFunction(p0);
}

void lefrSetFreeFunction(::LEFI_FREE_FUNCTION p0)
{
  LefParser::lefrSetFreeFunction(p0);
}

void lefrSetLineNumberFunction(::LEFI_LINE_NUMBER_FUNCTION p0)
{
  LefParser::lefrSetLineNumberFunction(p0);
}

void lefrSetDeltaNumberLines(int p0)
{
  LefParser::lefrSetDeltaNumberLines(p0);
}

void lefrSetRelaxMode()
{
  LefParser::lefrSetRelaxMode();
}

void lefrUnsetRelaxMode()
{
  LefParser::lefrUnsetRelaxMode();
}

void lefrSetVersionValue(const char* version)
{
  LefParser::lefrSetVersionValue(version);
}

void lefrSetReadFunction(::LEFI_READ_FUNCTION p0)
{
  LefParser::lefrSetReadFunction(p0);
}

void lefrUnsetReadFunction()
{
  LefParser::lefrUnsetReadFunction();
}

void lefrSetOpenLogFileAppend()
{
  LefParser::lefrSetOpenLogFileAppend();
}

void lefrUnsetOpenLogFileAppend()
{
  LefParser::lefrUnsetOpenLogFileAppend();
}

void lefrDisablePropStrProcess()
{
  LefParser::lefrDisablePropStrProcess();
}

void lefrSetAntennaInoutWarnings(int warn)
{
  LefParser::lefrSetAntennaInoutWarnings(warn);
}

void lefrSetAntennaInputWarnings(int warn)
{
  LefParser::lefrSetAntennaInputWarnings(warn);
}

void lefrSetAntennaOutputWarnings(int warn)
{
  LefParser::lefrSetAntennaOutputWarnings(warn);
}

void lefrSetArrayWarnings(int warn)
{
  LefParser::lefrSetArrayWarnings(warn);
}

void lefrSetCaseSensitiveWarnings(int warn)
{
  LefParser::lefrSetCaseSensitiveWarnings(warn);
}

void lefrSetCorrectionTableWarnings(int warn)
{
  LefParser::lefrSetCorrectionTableWarnings(warn);
}

void lefrSetDielectricWarnings(int warn)
{
  LefParser::lefrSetDielectricWarnings(warn);
}

void lefrSetEdgeRateThreshold1Warnings(int warn)
{
  LefParser::lefrSetEdgeRateThreshold1Warnings(warn);
}

void lefrSetEdgeRateThreshold2Warnings(int warn)
{
  LefParser::lefrSetEdgeRateThreshold2Warnings(warn);
}

void lefrSetEdgeRateScaleFactorWarnings(int warn)
{
  LefParser::lefrSetEdgeRateScaleFactorWarnings(warn);
}

void lefrSetInoutAntennaWarnings(int warn)
{
  LefParser::lefrSetInoutAntennaWarnings(warn);
}

void lefrSetInputAntennaWarnings(int warn)
{
  LefParser::lefrSetInputAntennaWarnings(warn);
}

void lefrSetIRDropWarnings(int warn)
{
  LefParser::lefrSetIRDropWarnings(warn);
}

void lefrSetLayerWarnings(int warn)
{
  LefParser::lefrSetLayerWarnings(warn);
}

void lefrSetMacroWarnings(int warn)
{
  LefParser::lefrSetMacroWarnings(warn);
}

void lefrSetMaxStackViaWarnings(int warn)
{
  LefParser::lefrSetMaxStackViaWarnings(warn);
}

void lefrSetMinFeatureWarnings(int warn)
{
  LefParser::lefrSetMinFeatureWarnings(warn);
}

void lefrSetNoiseMarginWarnings(int warn)
{
  LefParser::lefrSetNoiseMarginWarnings(warn);
}

void lefrSetNoiseTableWarnings(int warn)
{
  LefParser::lefrSetNoiseTableWarnings(warn);
}

void lefrSetNonDefaultWarnings(int warn)
{
  LefParser::lefrSetNonDefaultWarnings(warn);
}

void lefrSetNoWireExtensionWarnings(int warn)
{
  LefParser::lefrSetNoWireExtensionWarnings(warn);
}

void lefrSetOutputAntennaWarnings(int warn)
{
  LefParser::lefrSetOutputAntennaWarnings(warn);
}

void lefrSetPinWarnings(int warn)
{
  LefParser::lefrSetPinWarnings(warn);
}

void lefrSetSiteWarnings(int warn)
{
  LefParser::lefrSetSiteWarnings(warn);
}

void lefrSetSpacingWarnings(int warn)
{
  LefParser::lefrSetSpacingWarnings(warn);
}

void lefrSetTimingWarnings(int warn)
{
  LefParser::lefrSetTimingWarnings(warn);
}

void lefrSetUnitsWarnings(int warn)
{
  LefParser::lefrSetUnitsWarnings(warn);
}

void lefrSetUseMinSpacingWarnings(int warn)
{
  LefParser::lefrSetUseMinSpacingWarnings(warn);
}

void lefrSetViaRuleWarnings(int warn)
{
  LefParser::lefrSetViaRuleWarnings(warn);
}

void lefrSetViaWarnings(int warn)
{
  LefParser::lefrSetViaWarnings(warn);
}

void lefrDisableParserMsgs(int nMsg, int* msgs)
{
  LefParser::lefrDisableParserMsgs(nMsg, msgs);
}

void lefrEnableParserMsgs(int nMsg, int* msgs)
{
  LefParser::lefrEnableParserMsgs(nMsg, msgs);
}

void lefrEnableAllMsgs()
{
  LefParser::lefrEnableAllMsgs();
}

void lefrDisableAllMsgs()
{
  LefParser::lefrDisableAllMsgs();
}

void lefrSetTotalMsgLimit(int totNumMsgs)
{
  LefParser::lefrSetTotalMsgLimit(totNumMsgs);
}

void lefrSetLimitPerMsg(int msgId, int numMsg)
{
  LefParser::lefrSetLimitPerMsg(msgId, numMsg);
}
