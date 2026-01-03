// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2013, Cadence Design Systems
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
// *****************************************************************************
// *****************************************************************************

// This file contains code for implementing the lefwriter 5.3
// It has functions to set the user callback functions.  If functions
// are set, the lefwriter will call the user callback functions when
// it comes to the section.  If the section is required, but user
// does not set any callback functions, a warning will be printed
// both on stderr and on the output file if there is one.
// The lef writer does not provide any default callback functions for
// the required sections.
//
// Author: Wanda da Rosa
// Date:   05/06/99
//
// Revisions:

#include "lefwWriterCalls.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "lefiDebug.hpp"
#include "lefiKRDefs.hpp"

BEGIN_LEF_PARSER_NAMESPACE

static constexpr int kMaxCBs = 30;

enum Callback
{
  kVersionCbk = 0,
  kCaseSensitiveCbk = 1,
  kNoWireExtensionCbk = 2,
  kBusBitCharsCbk = 3,
  kDividerCharCbk = 4,
  kManufacturingGridCbk = 5,
  kUseMinSpacingCbk = 6,
  kClearanceMeasureCbk = 7,
  kUnitsCbk = 8,
  kAntennaInputGateAreaCbk = 9,
  kAntennaInOutDiffAreaCbk = 10,
  kAntennaOutputDiffAreaCbk = 11,
  kPropDefCbk = 12,
  kLayerCbk = 13,
  kViaCbk = 14,
  kViaRuleCbk = 15,
  kNonDefaultCbk = 16,
  kCrossTalkCbk = 17,
  kNoiseTableCbk = 18,
  kCorrectionTableCbk = 19,
  kSpacingCbk = 20,
  kMinFeatureCbk = 21,
  kDielectricCbk = 22,
  kIRDropCbk = 23,
  kSiteCbk = 24,
  kArrayCbk = 25,
  kMacroCbk = 26,
  kAntennaCbk = 27,
  kExtCbk = 28,
  kEndLibCbk = 29,
};

// NEW CALLBACK - then place it here.

int lefWRetVal;

#define WRITER_CALLBACK(func, type)                                 \
  if (func) {                                                       \
    lefWRetVal = (*func)(type, lefwUserData);                       \
    if (lefWRetVal != 0) {                                          \
      lefiError(1, 0, "User callback routine returned bad status"); \
      return lefWRetVal;                                            \
    }                                                               \
  }

// *****************************************************************************
//   Global variables
// *****************************************************************************

lefiUserData lefwUserData = nullptr;
static char* lefwFileName = nullptr;
static int lefwRegisterUnused = 0;

extern FILE* lefwFile;

// *****************************************************************************
//       List of call back routines
//  These are filled in by the user.  See the
//   "set" routines at the end of the file
// *****************************************************************************
// The callback routines
lefwVoidCbkFnType lefwCallbacksSeq[kMaxCBs] = {
    nullptr,  // lefwVersionCbk
    nullptr,  // lefwCaseSensitiveCbk
    nullptr,  // lefwNoWireExtensionCbk
    nullptr,  // lefwBusBitCharsCbk
    nullptr,  // lefwDividerCharCbk
    nullptr,  // lefwManufacturingGridCbk
    nullptr,  // lefwUseMinSpacingCbk
    nullptr,  // lefwClearanceMeasureCbk
    nullptr,  // lefwUnitsCbk
    nullptr,  // lefwAntennaInputGateAreaCbk
    nullptr,  // lefwAntennaInOutDiffAreaCbk
    nullptr,  // lefwAntennaOutputDiffAreaCbk
    nullptr,  // lefwPropDefCbk
    nullptr,  // lefwLayerCbk
    nullptr,  // lefwViaCbk
    nullptr,  // lefwViaRuleCbk
    nullptr,  // lefwNonDefaultCbk
    nullptr,  // lefwCrossTalkCbk
    nullptr,  // lefwNoiseTableCbk
    nullptr,  // lefwCorrectionTableCbk
    nullptr,  // lefwSpacingCbk
    nullptr,  // lefwMinFeatureCbk
    nullptr,  // lefwDielectricCbk
    nullptr,  // lefwIRDropCbk
    nullptr,  // lefwSiteCbk
    nullptr,  // lefwArrayCbk
    nullptr,  // lefwMacroCbk
    nullptr,  // lefwAntennaCbk
    nullptr,  // lefwExtCbk
    nullptr,  // lefwEndLibCbk
              // Add NEW CALLBACK here
};

// the optional and required callbacks
int lefwCallbacksReq[kMaxCBs] = {
    0,  // Version
    0,  // CaseSensitive
    0,  // NoWireExtension
    0,  // BusBitChars
    0,  // Divider
    0,  // ManufacturingGrid
    0,  // UseMinSpacing
    0,  // ClearanceMeasure
    0,  // Units
    0,  // AntennaInputGateArea
    0,  // AntennaInOutDiffArea
    0,  // AntennaOutputDiffArea
    0,  // PropDefinition
    0,  // Layer
    0,  // Via
    0,  // ViaRule
    0,  // NonDefault
    0,  // CrossTalk
    0,  // NoiseTable
    0,  // CorrectionTable
    0,  // Spacing
    0,  // MinFeature
    0,  // Dielectric
    0,  // IRDrop
    0,  // Site
    0,  // Array
    0,  // Macro
    0,  // Antenna
    0,  // Extension
    0,  // End Library
        // Add NEW CALLBACK here
};

// The section names
char lefwSectionNames[kMaxCBs][80] = {
    "Version",
    "CaseSensitive",
    "NoWireExtension",
    "BusBitChars",
    "DividerChar",
    "ManufacturingGrid",
    "UseMinSpacing",
    "ClearanceMeasure",
    "Units",
    "AntennaInputGateArea",
    "AntennaInOutDiffArea",
    "AntennaOutputDiffArea",
    "PropertyDefinition",
    "Layer",
    "Via",
    "ViaRule",
    "NonDefault",
    "CrossTalk",
    "NoiseTable",
    "CorrectionTable",
    "Spacing",
    "MinFeature",
    "Dielectric",
    "IRDrop",
    "Site",
    "Array",
    "Macro",
    "Antenna",
    "Ext",
    "End Library",
    // Add NEW CALLBACK here
};

// the call back types from the lefwCallbackType_e
lefwCallbackType_e lefwCallbacksType[kMaxCBs] = {
    lefwVersionCbkType,
    lefwCaseSensitiveCbkType,
    lefwNoWireExtensionCbkType,
    lefwBusBitCharsCbkType,
    lefwDividerCharCbkType,
    lefwManufacturingGridCbkType,
    lefwUseMinSpacingCbkType,
    lefwClearanceMeasureCbkType,
    lefwUnitsCbkType,
    lefwAntennaInputGateAreaCbkType,
    lefwAntennaInOutDiffAreaCbkType,
    lefwAntennaOutputDiffAreaCbkType,
    lefwPropDefCbkType,
    lefwLayerCbkType,
    lefwViaCbkType,
    lefwViaRuleCbkType,
    lefwNonDefaultCbkType,
    lefwCrossTalkCbkType,
    lefwNoiseTableCbkType,
    lefwCorrectionTableCbkType,
    lefwSpacingCbkType,
    lefwMinFeatureCbkType,
    lefwDielectricCbkType,
    lefwIRDropCbkType,
    lefwSiteCbkType,
    lefwArrayCbkType,
    lefwMacroCbkType,
    lefwAntennaCbkType,
    lefwExtCbkType,
    lefwEndLibCbkType,
    // Add NEW TYPES here
};

// *****************************************************************************
//   Routines for the callbacks
// *****************************************************************************
const char* lefwFName()
{
  return lefwFileName;
}

int lefwWrite(FILE* f, const char* fName, lefiUserData uData)
{
  int i;

  if (lefwHasInitCbk == 0 && lefwHasInit == 0) {
    fprintf(stderr,
            "ERROR (LEFWRIT-4100): lefwWrite called before lefwInitCbk\n");
    return -1;
  }

  lefwFileName = (char*) fName;
  lefwFile = f;
  lefwUserData = uData;

  // Loop through the list of callbacks and call the user define
  // callback routines if any are set

  for (i = 0; i < kMaxCBs; i++) {
    if (lefwCallbacksSeq[i] != nullptr) {  // user has set a callback function
      WRITER_CALLBACK(lefwCallbacksSeq[i], lefwCallbacksType[i]);
    } else if (lefwCallbacksReq[i]) {  // it is required but user hasn't set up
      fprintf(f,
              "# WARNING (LEFWRIT-4500): Callback for %s is required, but is "
              "not defined\n\n",
              lefwSectionNames[i]);
      fprintf(stderr,
              "WARNING (LEFWRIT-4500): Callback for %s is required, but is not "
              "defined\n\n",
              lefwSectionNames[i]);
    }
  }
  return 0;
}

void lefwSetUnusedCallbacks(lefwVoidCbkFnType func)
{
  // Set all of the callbacks that have not been set yet to
  // the given function.
  int i;

  for (i = 0; i < kMaxCBs; i++) {
    if (lefwCallbacksSeq[i] == nullptr) {
      lefwCallbacksSeq[i] = func;
    }
  }
}

// These count up the number of times an unset callback is called...
static int lefwUnusedCount[100];

int lefwCountFunc(lefwCallbackType_e e, lefiUserData d)
{
  int i = (int) e;
  if (lefiDebug(23)) {
    printf("count %d 0x%p\n", (int) e, d);
  }
  if (i >= 0 && i < 100) {
    lefwUnusedCount[i] += 1;
    return 0;
  }
  return 1;
}

void lefwSetRegisterUnusedCallbacks()
{
  int i;
  lefwRegisterUnused = 1;
  lefwSetUnusedCallbacks(lefwCountFunc);
  for (i = 0; i < 100; i++) {
    lefwUnusedCount[i] = 0;
  }
}

void lefwPrintUnusedCallbacks(FILE* f)
{
  int i;
  int first = 1;

  if (lefwRegisterUnused == 0) {
    fprintf(f,
            "ERROR (LEFWRIT-4101): lefwSetRegisterUnusedCallbacks was not "
            "called to setup this data.\n");
    return;
  }

  for (i = 0; i < 100; i++) {
    if (lefwUnusedCount[i]) {
      if (first) {
        fprintf(f,
                "INFO (LEFWRIT-4700): LEF items that were present but ignored "
                "because of no callback were set.\n");
      }
      first = 0;
      switch ((lefwCallbackType_e) i) {
        case lefwVersionCbkType:
          fprintf(f, "Version");
          break;
        case lefwCaseSensitiveCbkType:
          fprintf(f, "CaseSensitive");
          break;
        case lefwNoWireExtensionCbkType:
          fprintf(f, "NoWireExtensionAtPins");
          break;
        case lefwBusBitCharsCbkType:
          fprintf(f, "BusBitChars");
          break;
        case lefwDividerCharCbkType:
          fprintf(f, "DividerChar");
          break;
        case lefwManufacturingGridCbkType:
          fprintf(f, "ManufacturingGrid");
          break;
        case lefwUseMinSpacingCbkType:
          fprintf(f, "UseMinSpacing");
          break;
        case lefwClearanceMeasureCbkType:
          fprintf(f, "ClearanceMeasure");
          break;
        case lefwUnitsCbkType:
          fprintf(f, "Units");
          break;
        case lefwAntennaInputGateAreaCbkType:
          fprintf(f, "AntennaInputGateArea");
          break;
        case lefwAntennaInOutDiffAreaCbkType:
          fprintf(f, "AntennaInOutDiffArea");
          break;
        case lefwAntennaOutputDiffAreaCbkType:
          fprintf(f, "AntennaOutputDiffArea");
          break;
        case lefwPropDefCbkType:
          fprintf(f, "PropertyDefintion");
          break;
        case lefwLayerCbkType:
          fprintf(f, "Layer");
          break;
        case lefwViaCbkType:
          fprintf(f, "Via");
          break;
        case lefwViaRuleCbkType:
          fprintf(f, "ViaRule");
          break;
        case lefwNonDefaultCbkType:
          fprintf(f, "NonDefault");
          break;
        case lefwCrossTalkCbkType:
          fprintf(f, "CrossTalk");
          break;
        case lefwNoiseTableCbkType:
          fprintf(f, "NoiseTable");
          break;
        case lefwCorrectionTableCbkType:
          fprintf(f, "CorrectionTable");
          break;
        case lefwSpacingCbkType:
          fprintf(f, "Spacing");
          break;
        case lefwMinFeatureCbkType:
          fprintf(f, "MinFeature");
          break;
        case lefwDielectricCbkType:
          fprintf(f, "Dielectric");
          break;
        case lefwIRDropCbkType:
          fprintf(f, "IRDrop");
          break;
        case lefwSiteCbkType:
          fprintf(f, "Site");
          break;
        case lefwArrayCbkType:
          fprintf(f, "Array");
          break;
        case lefwMacroCbkType:
          fprintf(f, "Macro");
          break;
        case lefwAntennaCbkType:
          fprintf(f, "OutputAntenna");
          break;
        case lefwExtCbkType:
          fprintf(f, "Extension");
          break;
        case lefwEndLibCbkType:
          fprintf(f, "End Library");
          break;
          // NEW CALLBACK  add the print here
        default:
          fprintf(f, "BOGUS ENTRY");
          break;
      }
      fprintf(f, " %d\n", lefwUnusedCount[i]);
    }
  }
}

void lefwSetUserData(lefiUserData d)
{
  lefwUserData = d;
}

lefiUserData lefwGetUserData()
{
  return lefwUserData;
}

void lefwSetUnitsCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kUnitsCbk] = f;
}

void lefwSetDividerCharCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kDividerCharCbk] = f;
}

void lefwSetManufacturingGridCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kManufacturingGridCbk] = f;
}

void lefwSetUseMinSpacingCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kUseMinSpacingCbk] = f;
}

void lefwSetClearanceMeasureCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kClearanceMeasureCbk] = f;
}

void lefwSetNoWireExtensionCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kNoWireExtensionCbk] = f;
}

void lefwSetBusBitCharsCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kBusBitCharsCbk] = f;
}

void lefwSetCaseSensitiveCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kCaseSensitiveCbk] = f;
}

void lefwSetVersionCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kVersionCbk] = f;
}

void lefwSetLayerCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kLayerCbk] = f;
}

void lefwSetViaCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kViaCbk] = f;
}

void lefwSetViaRuleCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kViaRuleCbk] = f;
}

void lefwSetSpacingCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kSpacingCbk] = f;
}

void lefwSetIRDropCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kIRDropCbk] = f;
}

void lefwSetDielectricCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kDielectricCbk] = f;
}

void lefwSetMinFeatureCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kMinFeatureCbk] = f;
}

void lefwSetNonDefaultCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kNonDefaultCbk] = f;
}

void lefwSetSiteCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kSiteCbk] = f;
}

void lefwSetMacroCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kMacroCbk] = f;
}

void lefwSetArrayCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kArrayCbk] = f;
}

void lefwSetPropDefCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kPropDefCbk] = f;
}

void lefwSetCrossTalkCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kCrossTalkCbk] = f;
}

void lefwSetNoiseTableCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kNoiseTableCbk] = f;
}

void lefwSetCorrectionTableCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kCorrectionTableCbk] = f;
}

void lefwSetAntennaCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kAntennaCbk] = f;
}

void lefwSetExtCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kExtCbk] = f;
}

void lefwSetEndLibCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kEndLibCbk] = f;
}

/* NEW CALLBACK - Each callback routine must have a routine that allows
 * the user to set it.  The set routines go here. */

void lefwSetAntennaInputGateAreaCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kAntennaInputGateAreaCbk] = f;
}

void lefwSetAntennaInOutDiffAreaCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kAntennaInOutDiffAreaCbk] = f;
}

void lefwSetAntennaOutputDiffAreaCbk(lefwVoidCbkFnType f)
{
  lefwCallbacksSeq[kAntennaOutputDiffAreaCbk] = f;
}

extern LEFI_LOG_FUNCTION lefwErrorLogFunction;

void lefwSetLogFunction(LEFI_LOG_FUNCTION f)
{
  lefwErrorLogFunction = f;
}
extern LEFI_WARNING_LOG_FUNCTION lefwWarningLogFunction;

void lefwSetWarningLogFunction(LEFI_WARNING_LOG_FUNCTION f)
{
  lefwWarningLogFunction = f;
}
END_LEF_PARSER_NAMESPACE
