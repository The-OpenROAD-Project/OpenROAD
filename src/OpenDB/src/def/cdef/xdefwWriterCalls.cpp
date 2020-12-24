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
//  $Author: xxx $
//  $Revision: xxx $
//  $Date: xxx $
//  $State: xxx $  
// *****************************************************************************
// *****************************************************************************

#define EXTERN extern "C"

#include "defwWriterCalls.h"
#include "defwWriterCalls.hpp"

// Wrappers definitions.
int defwWrite (FILE * file, const char * fileName, defiUserData  userData) {
    return LefDefParser::defwWrite(file, fileName, userData);
}

void defwSetRegisterUnusedCallbacks () {
    LefDefParser::defwSetRegisterUnusedCallbacks();
}

void defwPrintUnusedCallbacks (FILE*  log) {
    LefDefParser::defwPrintUnusedCallbacks(log);
}

void defwSetUserData (defiUserData  p0) {
    LefDefParser::defwSetUserData(p0);
}

defiUserData defwGetUserData () {
    return LefDefParser::defwGetUserData();
}

void defwSetArrayCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetArrayCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetAssertionCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetAssertionCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetBlockageCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetBlockageCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetBusBitCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetBusBitCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetCannotOccupyCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetCannotOccupyCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetCanplaceCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetCanplaceCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetCaseSensitiveCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetCaseSensitiveCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetComponentCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetComponentCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetConstraintCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetConstraintCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetDefaultCapCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetDefaultCapCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetDesignCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetDesignCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetDesignEndCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetDesignEndCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetDieAreaCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetDieAreaCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetDividerCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetDividerCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetExtCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetExtCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetFloorPlanCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetFloorPlanCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetGcellGridCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetGcellGridCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetGroupCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetGroupCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetHistoryCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetHistoryCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetIOTimingCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetIOTimingCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetNetCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetNetCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetPinCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetPinCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetPinPropCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetPinPropCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetPropDefCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetPropDefCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetRegionCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetRegionCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetRowCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetRowCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetSNetCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetSNetCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetScanchainCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetScanchainCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetTechnologyCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetTechnologyCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetTrackCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetTrackCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetUnitsCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetUnitsCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetVersionCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetVersionCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetViaCbk (::defwVoidCbkFnType p0) {
    LefDefParser::defwSetViaCbk((LefDefParser::defwVoidCbkFnType) p0);
}

void defwSetUnusedCallbacks (::defwVoidCbkFnType  func) {
    LefDefParser::defwSetUnusedCallbacks((LefDefParser::defwVoidCbkFnType ) func);
}

void defwSetLogFunction (::DEFI_LOG_FUNCTION  p0) {
    LefDefParser::defwSetLogFunction(p0);
}

void defwSetWarningLogFunction (::DEFI_WARNING_LOG_FUNCTION  p0) {
    LefDefParser::defwSetWarningLogFunction(p0);
}

