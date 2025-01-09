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
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#define EXTERN extern "C"

#include "lefwWriterCalls.h"
#include "lefwWriterCalls.hpp"

// Wrappers definitions.
int lefwWrite(FILE* file, const char* fileName, lefiUserData userData)
{
  return LefParser::lefwWrite(file, fileName, userData);
}

void lefwSetRegisterUnusedCallbacks()
{
  LefParser::lefwSetRegisterUnusedCallbacks();
}

void lefwPrintUnusedCallbacks(FILE* f)
{
  LefParser::lefwPrintUnusedCallbacks(f);
}

void lefwSetUserData(lefiUserData p0)
{
  LefParser::lefwSetUserData(p0);
}

lefiUserData lefwGetUserData()
{
  return LefParser::lefwGetUserData();
}

void lefwSetVersionCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetVersionCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetCaseSensitiveCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetCaseSensitiveCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetNoWireExtensionCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetNoWireExtensionCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetBusBitCharsCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetBusBitCharsCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetDividerCharCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetDividerCharCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetManufacturingGridCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetManufacturingGridCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetUseMinSpacingCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetUseMinSpacingCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetClearanceMeasureCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetClearanceMeasureCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetUnitsCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetUnitsCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwAntennaInputGateAreaCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwAntennaInputGateAreaCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwAntennaInOutDiffAreaCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwAntennaInOutDiffAreaCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwAntennaOutputDiffAreaCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwAntennaOutputDiffAreaCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetPropDefCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetPropDefCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetLayerCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetLayerCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetViaCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetViaCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetViaRuleCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetViaRuleCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetNonDefaultCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetNonDefaultCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetCrossTalkCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetCrossTalkCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetNoiseTableCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetNoiseTableCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetCorrectionTableCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetCorrectionTableCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetSpacingCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetSpacingCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetMinFeatureCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetMinFeatureCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetDielectricCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetDielectricCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetIRDropCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetIRDropCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetSiteCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetSiteCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetArrayCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetArrayCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetMacroCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetMacroCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetAntennaCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetAntennaCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetExtCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetExtCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetEndLibCbk(::lefwVoidCbkFnType p0)
{
  LefParser::lefwSetEndLibCbk((LefParser::lefwVoidCbkFnType) p0);
}

void lefwSetUnusedCallbacks(::lefwVoidCbkFnType func)
{
  LefParser::lefwSetUnusedCallbacks((LefParser::lefwVoidCbkFnType) func);
}

void lefwSetLogFunction(::LEFI_LOG_FUNCTION p0)
{
  LefParser::lefwSetLogFunction(p0);
}

void lefwSetWarningLogFunction(::LEFI_WARNING_LOG_FUNCTION p0)
{
  LefParser::lefwSetWarningLogFunction(p0);
}
