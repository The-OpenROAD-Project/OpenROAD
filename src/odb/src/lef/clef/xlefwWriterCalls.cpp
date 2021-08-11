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
//  $State:  $  
// *****************************************************************************
// *****************************************************************************

#define EXTERN extern "C"

#include "lefwWriterCalls.h"
#include "lefwWriterCalls.hpp"

// Wrappers definitions.
int lefwWrite (FILE * file, const char * fileName, lefiUserData  userData) {
    return LefDefParser::lefwWrite(file, fileName, userData);
}

void lefwSetRegisterUnusedCallbacks () {
    LefDefParser::lefwSetRegisterUnusedCallbacks();
}

void lefwPrintUnusedCallbacks (FILE*  f) {
    LefDefParser::lefwPrintUnusedCallbacks(f);
}

void lefwSetUserData (lefiUserData  p0) {
    LefDefParser::lefwSetUserData(p0);
}

lefiUserData lefwGetUserData () {
    return LefDefParser::lefwGetUserData();
}

void lefwSetVersionCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetVersionCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetCaseSensitiveCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetCaseSensitiveCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetNoWireExtensionCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetNoWireExtensionCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetBusBitCharsCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetBusBitCharsCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetDividerCharCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetDividerCharCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetManufacturingGridCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetManufacturingGridCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetUseMinSpacingCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetUseMinSpacingCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetClearanceMeasureCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetClearanceMeasureCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetUnitsCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetUnitsCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwAntennaInputGateAreaCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwAntennaInputGateAreaCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwAntennaInOutDiffAreaCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwAntennaInOutDiffAreaCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwAntennaOutputDiffAreaCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwAntennaOutputDiffAreaCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetPropDefCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetPropDefCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetLayerCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetLayerCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetViaCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetViaCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetViaRuleCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetViaRuleCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetNonDefaultCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetNonDefaultCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetCrossTalkCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetCrossTalkCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetNoiseTableCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetNoiseTableCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetCorrectionTableCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetCorrectionTableCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetSpacingCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetSpacingCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetMinFeatureCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetMinFeatureCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetDielectricCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetDielectricCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetIRDropCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetIRDropCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetSiteCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetSiteCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetArrayCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetArrayCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetMacroCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetMacroCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetAntennaCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetAntennaCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetExtCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetExtCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetEndLibCbk (::lefwVoidCbkFnType p0) {
    LefDefParser::lefwSetEndLibCbk((LefDefParser::lefwVoidCbkFnType) p0);
}

void lefwSetUnusedCallbacks (::lefwVoidCbkFnType  func) {
    LefDefParser::lefwSetUnusedCallbacks((LefDefParser::lefwVoidCbkFnType ) func);
}

void lefwSetLogFunction (::LEFI_LOG_FUNCTION  p0) {
    LefDefParser::lefwSetLogFunction(p0);
}

void lefwSetWarningLogFunction (::LEFI_WARNING_LOG_FUNCTION  p0) {
    LefDefParser::lefwSetWarningLogFunction(p0);
}

