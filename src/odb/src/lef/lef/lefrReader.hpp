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

#ifndef LEFRREADER_H
#define LEFRREADER_H

#include <stdarg.h>
#include <stdio.h>

#include "lefiKRDefs.hpp"
#include "lefiDefs.hpp"
#include "lefiUser.hpp"
#include "lefiUtil.hpp"

#define MAX_LEF_MSGS 4701

BEGIN_LEFDEF_PARSER_NAMESPACE

// The reader initialization.  Must be called before lefrRead(). 
extern int lefrInit ();
extern int lefrInitSession (int startSession = 1);

// obsolted
extern int lefrReset ();

// Clears parser configuration and return it in inial state.
extern int lefrClear ();

// obsoleted
extern int lefrReleaseNResetMemory ();

// Change the comment character in LEF.  The normal character is
// '#'.   You can change it to anything you want, but be careful.
extern void lefrSetCommentChar (char c);

// Allow the parser to upshift all names if the LEF
// file is case insensitive.  The default is no shift, so the user
// must do case insensitive matching.
extern void lefrSetShiftCase ();

// Allow the user to change the casesensitivity anytime during
// parsing.
// caseSen = 0, will turn off the casesensitivity
// caseSen != 0, will turn on the casesensitivity
 
extern void lefrSetCaseSensitivity (int caseSense);

// The reader request the file name they are parsing

extern const char * lefrFName ();


//  The main reader function.
//  The file should already be opened.  This requirement allows
//  the reader to be used with stdin or a pipe.  The file name
//  is only used for error messages.  The includeSearchPath is
//  a colon-delimited list of directories in which to find
//  include files.

extern int lefrRead (FILE *file, const char *fileName, lefiUserData userData);

//  Set all of the callbacks that have not yet been set to a function
//  that will add up how many times a given lef data type was ignored
//  (ie no callback was done).  The statistics can later be printed out.
extern void lefrSetRegisterUnusedCallbacks ();
extern void lefrPrintUnusedCallbacks (FILE* f);

// Set/get the client-provided user data.  lefi doesn't look at
// this data at all, it simply passes the opaque lefiUserData pointer
// back to the application with each callback.  The client can
// change the data at any time, and it will take effect on the
// next callback.  The lefi reader and writer maintain separate
// user data pointers.
extern void lefrSetUserData (lefiUserData);
extern lefiUserData lefrGetUserData ();

// An enum describing all of the types of reader callbacks.
typedef enum {
  lefrUnspecifiedCbkType = 0,
  lefrVersionCbkType,
  lefrVersionStrCbkType,
  lefrDividerCharCbkType,
  lefrBusBitCharsCbkType,
  lefrUnitsCbkType,
  lefrCaseSensitiveCbkType,
  lefrNoWireExtensionCbkType,
  lefrPropBeginCbkType,
  lefrPropCbkType,
  lefrPropEndCbkType,
  lefrLayerCbkType,
  lefrViaCbkType,
  lefrViaRuleCbkType,
  lefrSpacingCbkType,
  lefrIRDropCbkType,
  lefrDielectricCbkType,
  lefrMinFeatureCbkType,
  lefrNonDefaultCbkType,
  lefrSiteCbkType,
  lefrMacroBeginCbkType,
  lefrPinCbkType,
  lefrMacroCbkType,
  lefrObstructionCbkType,
  lefrArrayCbkType,

  // NEW CALLBACKS - each callback has its own type.  For each callback
  // that you add, you must add an item to this enum. 

  lefrSpacingBeginCbkType,
  lefrSpacingEndCbkType,
  lefrArrayBeginCbkType,
  lefrArrayEndCbkType,
  lefrIRDropBeginCbkType,
  lefrIRDropEndCbkType,
  lefrNoiseMarginCbkType,
  lefrEdgeRateThreshold1CbkType,
  lefrEdgeRateThreshold2CbkType,
  lefrEdgeRateScaleFactorCbkType,
  lefrNoiseTableCbkType,
  lefrCorrectionTableCbkType,
  lefrInputAntennaCbkType,
  lefrOutputAntennaCbkType,
  lefrInoutAntennaCbkType,
  lefrAntennaInputCbkType,
  lefrAntennaInoutCbkType,
  lefrAntennaOutputCbkType,
  lefrManufacturingCbkType,
  lefrUseMinSpacingCbkType,
  lefrClearanceMeasureCbkType,
  lefrTimingCbkType,
  lefrMacroClassTypeCbkType,
  lefrMacroOriginCbkType,
  lefrMacroSizeCbkType,
  lefrMacroFixedMaskCbkType,
  lefrMacroEndCbkType,
  lefrMaxStackViaCbkType,
  lefrExtensionCbkType,
  lefrDensityCbkType,
  lefrFixedMaskCbkType,
  lefrMacroSiteCbkType,
  lefrMacroForeignCbkType,

  lefrLibraryEndCbkType
} lefrCallbackType_e;
 
// Declarations of function signatures for each type of callback.
// These declarations are type-safe when compiling with ANSI C
// or C++; you will only be able to register a function pointer
// with the correct signature for a given type of callback.
//
// Each callback function is expected to return 0 if successful.
// A non-zero return code will cause the reader to abort.
//
// The lefrDesignStart and lefrDesignEnd callback is only called once.
// Other callbacks may be called multiple times, each time with a different
// set of data.
//
// For each callback, the Def API will make the callback to the
// function supplied by the client, which should either make a copy
// of the Def object, or store the data in the client's own data structures.
// The Def API will delete or reuse each object after making the callback,
// so the client should not keep a pointer to it.
//
// All callbacks pass the user data pointer provided in lefrRead()
// or lefrSetUserData() back to the client; this can be used by the
// client to obtain access to the rest of the client's data structures.
//
// The user data pointer is obtained using lefrGetUserData() immediately
// prior to making each callback, so the client is free to change the
// user data on the fly if necessary.
//
// Callbacks with the same signature are passed a callback type
// parameter, which allows an application to write a single callback
// function, register that function for multiple callbacks, then
// switch based on the callback type to handle the appropriate type of
// data.
 

// A declaration of the signature of all callbacks that return nothing. 
typedef int (*lefrVoidCbkFnType) (lefrCallbackType_e, 
                                  void* num, 
                                  lefiUserData);

// A declaration of the signature of all callbacks that return a string. 
typedef int (*lefrStringCbkFnType) (lefrCallbackType_e, 
                                    const char *string, 
                                    lefiUserData);
 
// A declaration of the signature of all callbacks that return a integer. 
typedef int (*lefrIntegerCbkFnType) (lefrCallbackType_e, 
                                     int number, 
                                     lefiUserData);
 
// A declaration of the signature of all callbacks that return a double. 
typedef int (*lefrDoubleCbkFnType) (lefrCallbackType_e, 
                                    double number, 
                                    lefiUserData);

// A declaration of the signature of all callbacks that return a lefiUnits. 
typedef int (*lefrUnitsCbkFnType) (lefrCallbackType_e, 
                                   lefiUnits* units, 
                                   lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiLayer. 
typedef int (*lefrLayerCbkFnType) (lefrCallbackType_e, 
                                   lefiLayer* l, 
                                   lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiVia. 
typedef int (*lefrViaCbkFnType) (lefrCallbackType_e, 
                                 lefiVia* l, 
                                 lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiViaRule. 
typedef int (*lefrViaRuleCbkFnType) (lefrCallbackType_e, 
                                     lefiViaRule* l, 
                                     lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiSpacing. 
typedef int (*lefrSpacingCbkFnType) (lefrCallbackType_e, 
                                     lefiSpacing* l, 
                                     lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiIRDrop. 
typedef int (*lefrIRDropCbkFnType) (lefrCallbackType_e, 
                                    lefiIRDrop* l, 
                                    lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiMinFeature. 
typedef int (*lefrMinFeatureCbkFnType) (lefrCallbackType_e, 
                                        lefiMinFeature* l, 
                                        lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiNonDefault. 
typedef int (*lefrNonDefaultCbkFnType) (lefrCallbackType_e, 
                                        lefiNonDefault* l, 
                                        lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiSite. 
typedef int (*lefrSiteCbkFnType) (lefrCallbackType_e, 
                                  lefiSite* l, 
                                  lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiMacro. 
typedef int (*lefrMacroCbkFnType) (lefrCallbackType_e, 
                                   lefiMacro* l, 
                                   lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiPin. 
typedef int (*lefrPinCbkFnType) (lefrCallbackType_e, 
                                 lefiPin* l, 
                                 lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiObstruction. 
typedef int (*lefrObstructionCbkFnType) (lefrCallbackType_e, 
                                         lefiObstruction* l, 
                                         lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiArray. 
typedef int (*lefrArrayCbkFnType) (lefrCallbackType_e, 
                                   lefiArray* l, 
                                   lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiProp. 
typedef int (*lefrPropCbkFnType) (lefrCallbackType_e, 
                                  lefiProp* p, 
                                  lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiNoiseMargin. 
typedef int (*lefrNoiseMarginCbkFnType) (lefrCallbackType_e, 
                                         struct lefiNoiseMargin* p, 
                                         lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiNoiseTable. 
typedef int (*lefrNoiseTableCbkFnType) (lefrCallbackType_e, 
                                        lefiNoiseTable* p, 
                                        lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiCorrectionTable. 
typedef int (*lefrCorrectionTableCbkFnType) (lefrCallbackType_e, 
                                             lefiCorrectionTable* p, 
                                             lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiTiming. 
typedef int (*lefrTimingCbkFnType) (lefrCallbackType_e, 
                                    lefiTiming* p, 
                                    lefiUserData);
 
// A declaration of the signature of all callbacks that return a lefiUseMinSpacing. 
typedef int (*lefrUseMinSpacingCbkFnType) (lefrCallbackType_e, 
                                           lefiUseMinSpacing* l, 
                                           lefiUserData);
 
  // NEW CALLBACK - If your callback returns a pointer to a new class then
  // you must add a type function here.

// A declaration of the signature of all callbacks that return a lefiMaxStackVia. 
typedef int (*lefrMaxStackViaCbkFnType) (lefrCallbackType_e, 
                                         lefiMaxStackVia* l, 
                                         lefiUserData);

typedef int (*lefrMacroNumCbkFnType) (lefrCallbackType_e, 
                                      lefiNum l, 
                                      lefiUserData);

typedef int (*lefrMacroSiteCbkFnType) (lefrCallbackType_e, 
                                      const lefiMacroSite *site,
                                      lefiUserData);

typedef int (*lefrMacroForeignCbkFnType) (lefrCallbackType_e, 
                                          const lefiMacroForeign *foreign,
                                          lefiUserData);

// 5.6 
// A declaration of the signature of all callbacks that return a lefiDensity. 
typedef int (*lefrDensityCbkFnType) (lefrCallbackType_e, 
                                     lefiDensity* l, 
                                     lefiUserData);
 
// Functions to call to register a callback function.
extern void lefrSetUnitsCbk(lefrUnitsCbkFnType);
extern void lefrSetVersionCbk(lefrDoubleCbkFnType);
extern void lefrSetVersionStrCbk(lefrStringCbkFnType);
extern void lefrSetDividerCharCbk(lefrStringCbkFnType);
extern void lefrSetBusBitCharsCbk(lefrStringCbkFnType);
extern void lefrSetNoWireExtensionCbk(lefrStringCbkFnType);
extern void lefrSetCaseSensitiveCbk(lefrIntegerCbkFnType);
extern void lefrSetPropBeginCbk(lefrVoidCbkFnType);
extern void lefrSetPropCbk(lefrPropCbkFnType);
extern void lefrSetPropEndCbk(lefrVoidCbkFnType);
extern void lefrSetLayerCbk(lefrLayerCbkFnType);
extern void lefrSetViaCbk(lefrViaCbkFnType);
extern void lefrSetViaRuleCbk(lefrViaRuleCbkFnType);
extern void lefrSetSpacingCbk(lefrSpacingCbkFnType);
extern void lefrSetIRDropCbk(lefrIRDropCbkFnType);
extern void lefrSetDielectricCbk(lefrDoubleCbkFnType);
extern void lefrSetMinFeatureCbk(lefrMinFeatureCbkFnType);
extern void lefrSetNonDefaultCbk(lefrNonDefaultCbkFnType);
extern void lefrSetSiteCbk(lefrSiteCbkFnType);
extern void lefrSetMacroBeginCbk(lefrStringCbkFnType);
extern void lefrSetPinCbk(lefrPinCbkFnType);
extern void lefrSetObstructionCbk(lefrObstructionCbkFnType);
extern void lefrSetArrayCbk(lefrArrayCbkFnType);
extern void lefrSetMacroCbk(lefrMacroCbkFnType);
extern void lefrSetLibraryEndCbk(lefrVoidCbkFnType);

// NEW CALLBACK - each callback must have a function to allow the user
// to set it.  Add the function here.

extern void lefrSetTimingCbk(lefrTimingCbkFnType);
extern void lefrSetSpacingBeginCbk(lefrVoidCbkFnType);
extern void lefrSetSpacingEndCbk(lefrVoidCbkFnType);
extern void lefrSetArrayBeginCbk(lefrStringCbkFnType);
extern void lefrSetArrayEndCbk(lefrStringCbkFnType);
extern void lefrSetIRDropBeginCbk(lefrVoidCbkFnType);
extern void lefrSetIRDropEndCbk(lefrVoidCbkFnType);
extern void lefrSetNoiseMarginCbk(lefrNoiseMarginCbkFnType);
extern void lefrSetEdgeRateThreshold1Cbk(lefrDoubleCbkFnType);
extern void lefrSetEdgeRateThreshold2Cbk(lefrDoubleCbkFnType);
extern void lefrSetEdgeRateScaleFactorCbk(lefrDoubleCbkFnType);
extern void lefrSetNoiseTableCbk(lefrNoiseTableCbkFnType);
extern void lefrSetCorrectionTableCbk(lefrCorrectionTableCbkFnType);
extern void lefrSetInputAntennaCbk(lefrDoubleCbkFnType);
extern void lefrSetOutputAntennaCbk(lefrDoubleCbkFnType);
extern void lefrSetInoutAntennaCbk(lefrDoubleCbkFnType);
extern void lefrSetAntennaInputCbk(lefrDoubleCbkFnType);
extern void lefrSetAntennaInoutCbk(lefrDoubleCbkFnType);
extern void lefrSetAntennaOutputCbk(lefrDoubleCbkFnType);
extern void lefrSetClearanceMeasureCbk(lefrStringCbkFnType);
extern void lefrSetManufacturingCbk(lefrDoubleCbkFnType);
extern void lefrSetUseMinSpacingCbk(lefrUseMinSpacingCbkFnType);
extern void lefrSetMacroClassTypeCbk(lefrStringCbkFnType);
extern void lefrSetMacroOriginCbk(lefrMacroNumCbkFnType);
extern void lefrSetMacroSiteCbk(lefrMacroSiteCbkFnType);
extern void lefrSetMacroForeignCbk(lefrMacroForeignCbkFnType);
extern void lefrSetMacroSizeCbk(lefrMacroNumCbkFnType);
extern void lefrSetMacroFixedMaskCbk(lefrIntegerCbkFnType);
extern void lefrSetMacroEndCbk(lefrStringCbkFnType);
extern void lefrSetMaxStackViaCbk(lefrMaxStackViaCbkFnType);
extern void lefrSetExtensionCbk(lefrStringCbkFnType);
extern void lefrSetDensityCbk(lefrDensityCbkFnType);
extern void lefrSetFixedMaskCbk(lefrIntegerCbkFnType);

// Set all of the callbacks that have not yet been set to the following
// function.  This is especially useful if you want to check to see
// if you forgot anything.
extern void lefrSetUnusedCallbacks (lefrVoidCbkFnType func);

// Reset all the callback functions to nil
extern void lefrUnsetCallbacks();

// Functions to call to unregister a callback function.
extern void lefrUnsetAntennaInputCbk();
extern void lefrUnsetAntennaInoutCbk();
extern void lefrUnsetAntennaOutputCbk();
extern void lefrUnsetArrayBeginCbk();
extern void lefrUnsetArrayCbk();
extern void lefrUnsetArrayEndCbk();
extern void lefrUnsetBusBitCharsCbk();
extern void lefrUnsetCaseSensitiveCbk();
extern void lefrUnsetClearanceMeasureCbk();
extern void lefrUnsetCorrectionTableCbk();
extern void lefrUnsetDensityCbk();
extern void lefrUnsetDielectricCbk();
extern void lefrUnsetDividerCharCbk();
extern void lefrUnsetEdgeRateScaleFactorCbk();
extern void lefrUnsetEdgeRateThreshold1Cbk();
extern void lefrUnsetEdgeRateThreshold2Cbk();
extern void lefrUnsetExtensionCbk();
extern void lefrUnsetInoutAntennaCbk();
extern void lefrUnsetInputAntennaCbk();
extern void lefrUnsetIRDropBeginCbk();
extern void lefrUnsetIRDropCbk();
extern void lefrUnsetIRDropEndCbk();
extern void lefrUnsetLayerCbk();
extern void lefrUnsetLibraryEndCbk();
extern void lefrUnsetMacroBeginCbk();
extern void lefrUnsetMacroCbk();
extern void lefrUnsetMacroClassTypeCbk();
extern void lefrUnsetMacroEndCbk();
extern void lefrUnsetMacroOriginCbk();
extern void lefrUnsetMacroSiteCbk();
extern void lefrUnsetMacroForeignCbk();
extern void lefrUnsetMacroSizeCbk();
extern void lefrUnsetManufacturingCbk();
extern void lefrUnsetMaxStackViaCbk();
extern void lefrUnsetMinFeatureCbk();
extern void lefrUnsetNoiseMarginCbk();
extern void lefrUnsetNoiseTableCbk();
extern void lefrUnsetNonDefaultCbk();
extern void lefrUnsetNoWireExtensionCbk();
extern void lefrUnsetObstructionCbk();
extern void lefrUnsetOutputAntennaCbk();
extern void lefrUnsetPinCbk();
extern void lefrUnsetPropBeginCbk();
extern void lefrUnsetPropCbk();
extern void lefrUnsetPropEndCbk();
extern void lefrUnsetSiteCbk();
extern void lefrUnsetSpacingBeginCbk();
extern void lefrUnsetSpacingCbk();
extern void lefrUnsetSpacingEndCbk();
extern void lefrUnsetTimingCbk();
extern void lefrUnsetUseMinSpacingCbk();
extern void lefrUnsetUnitsCbk();
extern void lefrUnsetVersionCbk();
extern void lefrUnsetVersionStrCbk();
extern void lefrUnsetViaCbk();
extern void lefrUnsetViaRuleCbk();

// Return the current line number in the parser.
extern int lefrLineNumber ();

// Routine to set the message logging routine for errors 
typedef void (*LEFI_LOG_FUNCTION) (const char*);
extern void lefrSetLogFunction(LEFI_LOG_FUNCTION);

// Routine to set the message logging routine for warnings 
typedef void (*LEFI_WARNING_LOG_FUNCTION) (const char*);
extern void lefrSetWarningLogFunction(LEFI_WARNING_LOG_FUNCTION);

// Routine to set the user defined malloc routine 
typedef void* (*LEFI_MALLOC_FUNCTION) (int);
extern void lefrSetMallocFunction(LEFI_MALLOC_FUNCTION);

// Routine to set the user defined realloc routine 
typedef void* (*LEFI_REALLOC_FUNCTION) (void *, int);
extern void lefrSetReallocFunction(LEFI_REALLOC_FUNCTION);

// Routine to set the user defined free routine 
typedef void (*LEFI_FREE_FUNCTION) (void *);
extern void lefrSetFreeFunction(LEFI_FREE_FUNCTION);

// Routine to set the line number callback routine 
typedef void (*LEFI_LINE_NUMBER_FUNCTION) (int);
extern void lefrSetLineNumberFunction( LEFI_LINE_NUMBER_FUNCTION);

// Set the number of lines before calling the line function callback routine 
// Default is 10000 
extern void lefrSetDeltaNumberLines  (int);

// PCR 551229 - Set the parser to be more relax 
// This api is specific for PKS. 
// When in relax mode, the parser will not require width, pitch, & direction 
// in routing layers. Also vias in nondefault rules 
extern void lefrSetRelaxMode ();
extern void lefrUnsetRelaxMode ();

// PCR 565274 - LEF/DEF API should have the API call to overwrite default 
//              version 
extern void lefrSetVersionValue(const  char*  version);

// Routine to set the read function 
typedef size_t (*LEFI_READ_FUNCTION) (FILE*, char*, size_t);
extern void lefrSetReadFunction(LEFI_READ_FUNCTION);
extern void lefrUnsetReadFunction();

// Routine to set the lefrWarning.log to open as append instead for write 
// New in 5.7 
extern void lefrSetOpenLogFileAppend();
extern void lefrUnsetOpenLogFileAppend();

// Routine to disable string property value process, default it will process 
// the value string 
extern void lefrDisablePropStrProcess();

// Routine to set the max number of warnings for a perticular section 

extern void lefrSetAntennaInoutWarnings(int warn);
extern void lefrSetAntennaInputWarnings(int warn);
extern void lefrSetAntennaOutputWarnings(int warn);
extern void lefrSetArrayWarnings(int warn);
extern void lefrSetCaseSensitiveWarnings(int warn);
extern void lefrSetCorrectionTableWarnings(int warn);
extern void lefrSetDielectricWarnings(int warn);
extern void lefrSetEdgeRateThreshold1Warnings(int warn);
extern void lefrSetEdgeRateThreshold2Warnings(int warn);
extern void lefrSetEdgeRateScaleFactorWarnings(int warn);
extern void lefrSetInoutAntennaWarnings(int warn);
extern void lefrSetInputAntennaWarnings(int warn);
extern void lefrSetIRDropWarnings(int warn);
extern void lefrSetLayerWarnings(int warn);
extern void lefrSetMacroWarnings(int warn);
extern void lefrSetMaxStackViaWarnings(int warn);
extern void lefrSetMinFeatureWarnings(int warn);
extern void lefrSetNoiseMarginWarnings(int warn);
extern void lefrSetNoiseTableWarnings(int warn);
extern void lefrSetNonDefaultWarnings(int warn);
extern void lefrSetNoWireExtensionWarnings(int warn);
extern void lefrSetOutputAntennaWarnings(int warn);
extern void lefrSetPinWarnings(int warn);
extern void lefrSetSiteWarnings(int warn);
extern void lefrSetSpacingWarnings(int warn);
extern void lefrSetTimingWarnings(int warn);
extern void lefrSetUnitsWarnings(int warn);
extern void lefrSetUseMinSpacingWarnings(int warn);
extern void lefrSetViaRuleWarnings(int warn);
extern void lefrSetViaWarnings(int warn);

// Handling output messages 
extern void lefrDisableParserMsgs(int nMsg, int* msgs);
extern void lefrEnableParserMsgs(int nMsg, int* msgs);
extern void lefrEnableAllMsgs();
extern void lefrDisableAllMsgs();
extern void lefrSetTotalMsgLimit(int totNumMsgs);
extern void lefrSetLimitPerMsg(int msgId, int numMsg);

// Register lef58Type-layerType pair. 
extern void lefrRegisterLef58Type(const char *lef58Type, 
                                  const char *layerType);

// Return codes for the user callbacks.
// The user should return one of these values.
#define PARSE_OK 0      // continue parsing 
#define STOP_PARSE 1    // stop parsing with no error message 
#define PARSE_ERROR 2   // stop parsing, print an error message

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
