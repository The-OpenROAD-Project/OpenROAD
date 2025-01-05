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
int defwWrite(FILE* file, const char* fileName, defiUserData userData)
{
  return DefParser::defwWrite(file, fileName, userData);
}

void defwSetRegisterUnusedCallbacks()
{
  DefParser::defwSetRegisterUnusedCallbacks();
}

void defwPrintUnusedCallbacks(FILE* log)
{
  DefParser::defwPrintUnusedCallbacks(log);
}

void defwSetUserData(defiUserData p0)
{
  DefParser::defwSetUserData(p0);
}

defiUserData defwGetUserData()
{
  return DefParser::defwGetUserData();
}

void defwSetArrayCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetArrayCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetAssertionCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetAssertionCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetBlockageCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetBlockageCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetBusBitCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetBusBitCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetCannotOccupyCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetCannotOccupyCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetCanplaceCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetCanplaceCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetCaseSensitiveCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetCaseSensitiveCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetComponentCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetComponentCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetConstraintCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetConstraintCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetDefaultCapCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetDefaultCapCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetDesignCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetDesignCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetDesignEndCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetDesignEndCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetDieAreaCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetDieAreaCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetDividerCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetDividerCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetExtCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetExtCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetFloorPlanCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetFloorPlanCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetGcellGridCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetGcellGridCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetGroupCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetGroupCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetHistoryCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetHistoryCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetIOTimingCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetIOTimingCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetNetCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetNetCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetPinCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetPinCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetPinPropCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetPinPropCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetPropDefCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetPropDefCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetRegionCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetRegionCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetRowCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetRowCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetSNetCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetSNetCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetScanchainCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetScanchainCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetTechnologyCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetTechnologyCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetTrackCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetTrackCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetUnitsCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetUnitsCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetVersionCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetVersionCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetViaCbk(::defwVoidCbkFnType p0)
{
  DefParser::defwSetViaCbk((DefParser::defwVoidCbkFnType) p0);
}

void defwSetUnusedCallbacks(::defwVoidCbkFnType func)
{
  DefParser::defwSetUnusedCallbacks((DefParser::defwVoidCbkFnType) func);
}

void defwSetLogFunction(::DEFI_LOG_FUNCTION p0)
{
  DefParser::defwSetLogFunction(p0);
}

void defwSetWarningLogFunction(::DEFI_WARNING_LOG_FUNCTION p0)
{
  DefParser::defwSetWarningLogFunction(p0);
}
