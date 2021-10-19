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
#include <stdlib.h>
#include <string.h>
#include "lefiDebug.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

#define MAXCBS    30

#define lefwVersionCbk 0
#define lefwCaseSensitiveCbk 1
#define lefwNoWireExtensionCbk 2
#define lefwBusBitCharsCbk 3
#define lefwDividerCharCbk 4
#define lefwManufacturingGridCbk 5
#define lefwUseMinSpacingCbk 6
#define lefwClearanceMeasureCbk 7
#define lefwUnitsCbk 8
#define lefwAntennaInputGateAreaCbk 9
#define lefwAntennaInOutDiffAreaCbk 10
#define lefwAntennaOutputDiffAreaCbk 11
#define lefwPropDefCbk 12
#define lefwLayerCbk 13
#define lefwViaCbk 14
#define lefwViaRuleCbk 15
#define lefwNonDefaultCbk 16
#define lefwCrossTalkCbk 17
#define lefwNoiseTableCbk 18
#define lefwCorrectionTableCbk 19
#define lefwSpacingCbk 20
#define lefwMinFeatureCbk 21
#define lefwDielectricCbk 22
#define lefwIRDropCbk 23
#define lefwSiteCbk 24
#define lefwArrayCbk 25
#define lefwMacroCbk 26
#define lefwAntennaCbk 27
#define lefwExtCbk 28
#define lefwEndLibCbk 29

// NEW CALLBACK - then place it here. 

int lefWRetVal;
extern int lefwHasInit;
extern int lefwHasInitCbk;

#define WRITER_CALLBACK(func, type) \
    if (func) { \
       if ((lefWRetVal = (*func)(type, lefwUserData)) == 0) { \
       } else { \
          lefiError(1, 0, "User callback routine returned bad status"); \
          return lefWRetVal; \
       } \
    }

// *****************************************************************************
//   Global variables
// *****************************************************************************

lefiUserData lefwUserData = 0;
static char *lefwFileName = 0;
static int lefwRegisterUnused = 0;

extern FILE *lefwFile;

// *****************************************************************************
//       List of call back routines
//  These are filled in by the user.  See the
//   "set" routines at the end of the file
// *****************************************************************************
// The callback routines
lefwVoidCbkFnType lefwCallbacksSeq[MAXCBS] = {
    0, // lefwVersionCbk
    0, // lefwCaseSensitiveCbk
    0, // lefwNoWireExtensionCbk
    0, // lefwBusBitCharsCbk
    0, // lefwDividerCharCbk
    0, // lefwManufacturingGridCbk
    0, // lefwUseMinSpacingCbk
    0, // lefwClearanceMeasureCbk
    0, // lefwUnitsCbk 
    0, // lefwAntennaInputGateAreaCbk
    0, // lefwAntennaInOutDiffAreaCbk
    0, // lefwAntennaOutputDiffAreaCbk
    0, // lefwPropDefCbk
    0, // lefwLayerCbk
    0, // lefwViaCbk
    0, // lefwViaRuleCbk
    0, // lefwNonDefaultCbk
    0, // lefwCrossTalkCbk
    0, // lefwNoiseTableCbk
    0, // lefwCorrectionTableCbk
    0, // lefwSpacingCbk
    0, // lefwMinFeatureCbk
    0, // lefwDielectricCbk
    0, // lefwIRDropCbk
    0, // lefwSiteCbk
    0, // lefwArrayCbk
    0, // lefwMacroCbk
    0, // lefwAntennaCbk
    0, // lefwExtCbk
    0, // lefwEndLibCbk
    // Add NEW CALLBACK here
};

// the optional and required callbacks
int lefwCallbacksReq[MAXCBS] = {
    0,    // Version
    0,    // CaseSensitive
    0,    // NoWireExtension
    0,    // BusBitChars
    0,    // Divider
    0,    // ManufacturingGrid
    0,    // UseMinSpacing
    0,    // ClearanceMeasure
    0,    // Units
    0,    // AntennaInputGateArea
    0,    // AntennaInOutDiffArea
    0,    // AntennaOutputDiffArea
    0,    // PropDefinition
    0,    // Layer
    0,    // Via
    0,    // ViaRule
    0,    // NonDefault
    0,    // CrossTalk
    0,    // NoiseTable
    0,    // CorrectionTable
    0,    // Spacing
    0,    // MinFeature
    0,    // Dielectric
    0,    // IRDrop
    0,    // Site
    0,    // Array
    0,    // Macro
    0,    // Antenna
    0,    // Extension
    0,    // End Library
    // Add NEW CALLBACK here
};

// The section names
char lefwSectionNames[MAXCBS] [80] = {
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
lefwCallbackType_e lefwCallbacksType[MAXCBS] = {
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
const char *
lefwFName()
{
    return lefwFileName;
}


int
lefwWrite(FILE          *f,
          const char    *fName,
          lefiUserData  uData)
{
    int i;

    if (lefwHasInitCbk == 0 && lefwHasInit == 0) {
        fprintf(stderr, "ERROR (LEFWRIT-4100): lefwWrite called before lefwInitCbk\n");
        return -1;
    }

    lefwFileName = (char*) fName;
    lefwFile = f;
    lefwUserData = uData;

    // Loop through the list of callbacks and call the user define
    // callback routines if any are set

    for (i = 0; i < MAXCBS; i++) {
        if (lefwCallbacksSeq[i] != 0) {   // user has set a callback function
            WRITER_CALLBACK(lefwCallbacksSeq[i], lefwCallbacksType[i]);
        } else if (lefwCallbacksReq[i]) { // it is required but user hasn't set up
            fprintf(f,
                    "# WARNING (LEFWRIT-4500): Callback for %s is required, but is not defined\n\n",
                    lefwSectionNames[i]);
            fprintf(stderr,
                    "WARNING (LEFWRIT-4500): Callback for %s is required, but is not defined\n\n",
                    lefwSectionNames[i]);
        }
    }
    return 0;
}

void
lefwSetUnusedCallbacks(lefwVoidCbkFnType func)
{
    // Set all of the callbacks that have not been set yet to
    // the given function.
    int i;

    for (i = 0; i < MAXCBS; i++) {
        if (lefwCallbacksSeq[i] == 0)
            lefwCallbacksSeq[i] = (lefwVoidCbkFnType) func;
    }
}

// These count up the number of times an unset callback is called... 
static int lefwUnusedCount[100];

int
lefwCountFunc(lefwCallbackType_e    e,
              lefiUserData          d)
{
    int i = (int) e;
    if (lefiDebug(23))
        printf("count %d 0x%p\n", (int) e, d);
    if (i >= 0 && i < 100) {
        lefwUnusedCount[i] += 1;
        return 0;
    }
    return 1;
}

void
lefwSetRegisterUnusedCallbacks()
{
    int i;
    lefwRegisterUnused = 1;
    lefwSetUnusedCallbacks(lefwCountFunc);
    for (i = 0; i < 100; i++)
        lefwUnusedCount[i] = 0;
}

void
lefwPrintUnusedCallbacks(FILE *f)
{
    int i;
    int first = 1;

    if (lefwRegisterUnused == 0) {
        fprintf(f,
                "ERROR (LEFWRIT-4101): lefwSetRegisterUnusedCallbacks was not called to setup this data.\n");
        return;
    }

    for (i = 0; i < 100; i++) {
        if (lefwUnusedCount[i]) {
            if (first)
                fprintf(f,
                        "INFO (LEFWRIT-4700): LEF items that were present but ignored because of no callback were set.\n");
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

void
lefwSetUserData(lefiUserData d)
{
    lefwUserData = d;
}

lefiUserData
lefwGetUserData()
{
    return lefwUserData;
}

void
lefwSetUnitsCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwUnitsCbk] = f;
}

void
lefwSetDividerCharCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwDividerCharCbk] = f;
}

void
lefwSetManufacturingGridCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwManufacturingGridCbk] = f;
}

void
lefwSetUseMinSpacingCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwUseMinSpacingCbk] = f;
}

void
lefwSetClearanceMeasureCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwClearanceMeasureCbk] = f;
}

void
lefwSetNoWireExtensionCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwNoWireExtensionCbk] = f;
}

void
lefwSetBusBitCharsCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwBusBitCharsCbk] = f;
}

void
lefwSetCaseSensitiveCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwCaseSensitiveCbk] = f;
}

void
lefwSetVersionCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwVersionCbk] = f;
}

void
lefwSetLayerCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwLayerCbk] = f;
}

void
lefwSetViaCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwViaCbk] = f;
}

void
lefwSetViaRuleCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwViaRuleCbk] = f;
}

void
lefwSetSpacingCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwSpacingCbk] = f;
}

void
lefwSetIRDropCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwIRDropCbk] = f;
}

void
lefwSetDielectricCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwDielectricCbk] = f;
}

void
lefwSetMinFeatureCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwMinFeatureCbk] = f;
}

void
lefwSetNonDefaultCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwNonDefaultCbk] = f;
}

void
lefwSetSiteCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwSiteCbk] = f;
}

void
lefwSetMacroCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwMacroCbk] = f;
}

void
lefwSetArrayCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwArrayCbk] = f;
}

void
lefwSetPropDefCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwPropDefCbk] = f;
}

void
lefwSetCrossTalkCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwCrossTalkCbk] = f;
}

void
lefwSetNoiseTableCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwNoiseTableCbk] = f;
}


void
lefwSetCorrectionTableCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwCorrectionTableCbk] = f;
}


void
lefwSetAntennaCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwAntennaCbk] = f;
}

void
lefwSetExtCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwExtCbk] = f;
}

void
lefwSetEndLibCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwEndLibCbk] = f;
}

/* NEW CALLBACK - Each callback routine must have a routine that allows
 * the user to set it.  The set routines go here. */

void
lefwSetAntennaInputGateAreaCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwAntennaInputGateAreaCbk] = f;
}

void
lefwSetAntennaInOutDiffAreaCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwAntennaInOutDiffAreaCbk] = f;
}

void
lefwSetAntennaOutputDiffAreaCbk(lefwVoidCbkFnType f)
{
    lefwCallbacksSeq[lefwAntennaOutputDiffAreaCbk] = f;
}

extern LEFI_LOG_FUNCTION lefwErrorLogFunction;

void
lefwSetLogFunction(LEFI_LOG_FUNCTION f)
{
    lefwErrorLogFunction = f;
}
extern LEFI_WARNING_LOG_FUNCTION lefwWarningLogFunction;

void
lefwSetWarningLogFunction(LEFI_WARNING_LOG_FUNCTION f)
{
    lefwWarningLogFunction = f;
}
END_LEFDEF_PARSER_NAMESPACE

