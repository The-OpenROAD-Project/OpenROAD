/* ************************************************************************** */
/* ************************************************************************** */
/* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!               */
/* ************************************************************************** */
/* ************************************************************************** */
/* Copyright 2012 - 2017, Cadence Design Systems                              */
/*                                                                            */
/* This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source             */
/* Distribution,  Product Version 5.8.                                        */
/*                                                                            */
/* Licensed under the Apache License, Version 2.0 (the "License");            */
/*    you may not use this file except in compliance with the License.        */
/*    You may obtain a copy of the License at                                 */
/*                                                                            */
/*        http://www.apache.org/licenses/LICENSE-2.0                          */
/*                                                                            */
/*    Unless required by applicable law or agreed to in writing, software     */
/*    distributed under the License is distributed on an "AS IS" BASIS,       */
/*    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or         */
/*    implied. See the License for the specific language governing            */
/*    permissions and limitations under the License.                          */
/*                                                                            */
/* For updates, support, or to become part of the LEF/DEF Community,          */
/* check www.openeda.org for details.                                         */
/*                                                                            */
/*  $Author: dell $                                                           */
/*  $Revision: #1 $                                                           */
/*  $Date: 2017/06/06 $                                                       */
/*  $State:  $                                                                */
/* ************************************************************************** */
/* ************************************************************************** */


#ifndef CLEFRREADER_H
#define CLEFRREADER_H

#include <stdio.h>
#include "lefiTypedefs.h"

#define MAX_LEF_MSGS 4701

/* The reader initialization.  Must be called before lefrRead().              */
EXTERN int lefrInit ();
EXTERN int lefrInitSession (int  startSession);

/* obsolted                                                                   */
EXTERN int lefrReset ();

/* Clears parser configuration and return it in inial state.                  */
EXTERN int lefrClear ();

/* obsoleted                                                                  */
EXTERN int lefrReleaseNResetMemory ();

/* Change the comment character in LEF.  The normal character is              */
/* '#'.   You can change it to anything you want, but be careful.             */
EXTERN void lefrSetCommentChar (char  c);

/* Allow the parser to upshift all names if the LEF                           */
/* file is case insensitive.  The default is no shift, so the user            */
/* must do case insensitive matching.                                         */
EXTERN void lefrSetShiftCase ();

/* Allow the user to change the casesensitivity anytime during                */
/* parsing.                                                                   */
/* caseSen = 0, will turn off the casesensitivity                             */
/* caseSen != 0, will turn on the casesensitivity                             */

EXTERN void lefrSetCaseSensitivity (int  caseSense);

/* The reader request the file name they are parsing                          */

EXTERN const char * lefrFName ();

/*  The main reader function.                                                 */
/*  The file should already be opened.  This requirement allows               */
/*  the reader to be used with stdin or a pipe.  The file name                */
/*  is only used for error messages.  The includeSearchPath is                */
/*  a colon-delimited list of directories in which to find                    */
/*  include files.                                                            */

EXTERN int lefrRead (FILE * file, const char * fileName, lefiUserData  userData);

/*  Set all of the callbacks that have not yet been set to a function         */
/*  that will add up how many times a given lef data type was ignored         */
/*  (ie no callback was done).  The statistics can later be printed out.      */
EXTERN void lefrSetRegisterUnusedCallbacks ();
EXTERN void lefrPrintUnusedCallbacks (FILE*  f);

/* Set/get the client-provided user data.  lefi doesn't look at               */
/* this data at all, it simply passes the opaque lefiUserData pointer         */
/* back to the application with each callback.  The client can                */
/* change the data at any time, and it will take effect on the                */
/* next callback.  The lefi reader and writer maintain separate               */
/* user data pointers.                                                        */
EXTERN void lefrSetUserData (lefiUserData p0);
EXTERN lefiUserData lefrGetUserData ();

/* An enum describing all of the types of reader callbacks.                   */
typedef enum {
  lefrUnspecifiedCbkType = 0,
  lefrVersionCbkType = 1,
  lefrVersionStrCbkType = 2,
  lefrDividerCharCbkType = 3,
  lefrBusBitCharsCbkType = 4,
  lefrUnitsCbkType = 5,
  lefrCaseSensitiveCbkType = 6,
  lefrNoWireExtensionCbkType = 7,
  lefrPropBeginCbkType = 8,
  lefrPropCbkType = 9,
  lefrPropEndCbkType = 10,
  lefrLayerCbkType = 11,
  lefrViaCbkType = 12,
  lefrViaRuleCbkType = 13,
  lefrSpacingCbkType = 14,
  lefrIRDropCbkType = 15,
  lefrDielectricCbkType = 16,
  lefrMinFeatureCbkType = 17,
  lefrNonDefaultCbkType = 18,
  lefrSiteCbkType = 19,
  lefrMacroBeginCbkType = 20,
  lefrPinCbkType = 21,
  lefrMacroCbkType = 22,
  lefrObstructionCbkType = 23,
  lefrArrayCbkType = 24,

  /* NEW CALLBACKS - each callback has its own type.  For each callback       */
  /* that you add, you must add an item to this enum.                         */

  lefrSpacingBeginCbkType = 25,
  lefrSpacingEndCbkType = 26,
  lefrArrayBeginCbkType = 27,
  lefrArrayEndCbkType = 28,
  lefrIRDropBeginCbkType = 29,
  lefrIRDropEndCbkType = 30,
  lefrNoiseMarginCbkType = 31,
  lefrEdgeRateThreshold1CbkType = 32,
  lefrEdgeRateThreshold2CbkType = 33,
  lefrEdgeRateScaleFactorCbkType = 34,
  lefrNoiseTableCbkType = 35,
  lefrCorrectionTableCbkType = 36,
  lefrInputAntennaCbkType = 37,
  lefrOutputAntennaCbkType = 38,
  lefrInoutAntennaCbkType = 39,
  lefrAntennaInputCbkType = 40,
  lefrAntennaInoutCbkType = 41,
  lefrAntennaOutputCbkType = 42,
  lefrManufacturingCbkType = 43,
  lefrUseMinSpacingCbkType = 44,
  lefrClearanceMeasureCbkType = 45,
  lefrTimingCbkType = 46,
  lefrMacroClassTypeCbkType = 47,
  lefrMacroOriginCbkType = 48,
  lefrMacroSizeCbkType = 49,
  lefrMacroFixedMaskCbkType = 50,
  lefrMacroEndCbkType = 51,
  lefrMaxStackViaCbkType = 52,
  lefrExtensionCbkType = 53,
  lefrDensityCbkType = 54,
  lefrFixedMaskCbkType = 55,
  lefrMacroSiteCbkType = 56,
  lefrMacroForeignCbkType = 57,

  lefrLibraryEndCbkType = 58
} lefrCallbackType_e;

/* Declarations of function signatures for each type of callback.             */
/* These declarations are type-safe when compiling with ANSI C                */
/* or C++; you will only be able to register a function pointer               */
/* with the correct signature for a given type of callback.                   */
/*                                                                            */
/* Each callback function is expected to return 0 if successful.              */
/* A non-zero return code will cause the reader to abort.                     */
/*                                                                            */
/* The lefrDesignStart and lefrDesignEnd callback is only called once.        */
/* Other callbacks may be called multiple times, each time with a different   */
/* set of data.                                                               */
/*                                                                            */
/* For each callback, the Def API will make the callback to the               */
/* function supplied by the client, which should either make a copy           */
/* of the Def object, or store the data in the client's own data structures.  */
/* The Def API will delete or reuse each object after making the callback,    */
/* so the client should not keep a pointer to it.                             */
/*                                                                            */
/* All callbacks pass the user data pointer provided in lefrRead()            */
/* or lefrSetUserData() back to the client; this can be used by the           */
/* client to obtain access to the rest of the client's data structures.       */
/*                                                                            */
/* The user data pointer is obtained using lefrGetUserData() immediately      */
/* prior to making each callback, so the client is free to change the         */
/* user data on the fly if necessary.                                         */
/*                                                                            */
/* Callbacks with the same signature are passed a callback type               */
/* parameter, which allows an application to write a single callback          */
/* function, register that function for multiple callbacks, then              */
/* switch based on the callback type to handle the appropriate type of        */
/* data.                                                                      */

/* A declaration of the signature of all callbacks that return nothing.       */
typedef int (*lefrVoidCbkFnType) (lefrCallbackType_e, void* num, lefiUserData);

/* A declaration of the signature of all callbacks that return a string.      */
typedef int (*lefrStringCbkFnType) (lefrCallbackType_e, const char *string, lefiUserData);

/* A declaration of the signature of all callbacks that return a integer.     */
typedef int (*lefrIntegerCbkFnType) (lefrCallbackType_e, int number, lefiUserData);

/* A declaration of the signature of all callbacks that return a double.      */
typedef int (*lefrDoubleCbkFnType) (lefrCallbackType_e, double number, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiUnits.   */
typedef int (*lefrUnitsCbkFnType) (lefrCallbackType_e, lefiUnits* units, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiLayer.   */
typedef int (*lefrLayerCbkFnType) (lefrCallbackType_e, lefiLayer* l, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiVia.     */
typedef int (*lefrViaCbkFnType) (lefrCallbackType_e, lefiVia* l, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiViaRule. */
typedef int (*lefrViaRuleCbkFnType) (lefrCallbackType_e, lefiViaRule* l, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiSpacing. */
typedef int (*lefrSpacingCbkFnType) (lefrCallbackType_e, lefiSpacing* l, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiIRDrop.  */
typedef int (*lefrIRDropCbkFnType) (lefrCallbackType_e, lefiIRDrop* l, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiMinFeatu */
typedef int (*lefrMinFeatureCbkFnType) (lefrCallbackType_e, lefiMinFeature* l, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiNonDefau */
typedef int (*lefrNonDefaultCbkFnType) (lefrCallbackType_e, lefiNonDefault* l, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiSite.    */
typedef int (*lefrSiteCbkFnType) (lefrCallbackType_e, lefiSite* l, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiMacro.   */
typedef int (*lefrMacroCbkFnType) (lefrCallbackType_e, lefiMacro* l, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiPin.     */
typedef int (*lefrPinCbkFnType) (lefrCallbackType_e, lefiPin* l, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiObstruct */
typedef int (*lefrObstructionCbkFnType) (lefrCallbackType_e, lefiObstruction* l, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiArray.   */
typedef int (*lefrArrayCbkFnType) (lefrCallbackType_e, lefiArray* l, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiProp.    */
typedef int (*lefrPropCbkFnType) (lefrCallbackType_e, lefiProp* p, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiNoiseMar */
typedef int (*lefrNoiseMarginCbkFnType) (lefrCallbackType_e, struct lefiNoiseMargin* p, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiNoiseTab */
typedef int (*lefrNoiseTableCbkFnType) (lefrCallbackType_e, lefiNoiseTable* p, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiCorrecti */
typedef int (*lefrCorrectionTableCbkFnType) (lefrCallbackType_e, lefiCorrectionTable* p, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiTiming.  */
typedef int (*lefrTimingCbkFnType) (lefrCallbackType_e, lefiTiming* p, lefiUserData);

/* A declaration of the signature of all callbacks that return a lefiUseMinSp */
typedef int (*lefrUseMinSpacingCbkFnType) (lefrCallbackType_e, lefiUseMinSpacing* l, lefiUserData);

  /* NEW CALLBACK - If your callback returns a pointer to a new class then    */
  /* you must add a type function here.                                       */

/* A declaration of the signature of all callbacks that return a lefiMaxStack */
typedef int (*lefrMaxStackViaCbkFnType) (lefrCallbackType_e, lefiMaxStackVia* l, lefiUserData);

typedef int (*lefrMacroNumCbkFnType) (lefrCallbackType_e, lefiNum l, lefiUserData);

/* 5.6                                                                        */
/* A declaration of the signature of all callbacks that return a lefiDensity. */
typedef int (*lefrDensityCbkFnType) (lefrCallbackType_e, lefiDensity* l, lefiUserData);

/* Functions to call to register a callback function.                         */
EXTERN void lefrSetUnitsCbk (lefrUnitsCbkFnType p0);
EXTERN void lefrSetVersionCbk (lefrDoubleCbkFnType p0);
EXTERN void lefrSetVersionStrCbk (lefrStringCbkFnType p0);
EXTERN void lefrSetDividerCharCbk (lefrStringCbkFnType p0);
EXTERN void lefrSetBusBitCharsCbk (lefrStringCbkFnType p0);
EXTERN void lefrSetNoWireExtensionCbk (lefrStringCbkFnType p0);
EXTERN void lefrSetCaseSensitiveCbk (lefrIntegerCbkFnType p0);
EXTERN void lefrSetPropBeginCbk (lefrVoidCbkFnType p0);
EXTERN void lefrSetPropCbk (lefrPropCbkFnType p0);
EXTERN void lefrSetPropEndCbk (lefrVoidCbkFnType p0);
EXTERN void lefrSetLayerCbk (lefrLayerCbkFnType p0);
EXTERN void lefrSetViaCbk (lefrViaCbkFnType p0);
EXTERN void lefrSetViaRuleCbk (lefrViaRuleCbkFnType p0);
EXTERN void lefrSetSpacingCbk (lefrSpacingCbkFnType p0);
EXTERN void lefrSetIRDropCbk (lefrIRDropCbkFnType p0);
EXTERN void lefrSetDielectricCbk (lefrDoubleCbkFnType p0);
EXTERN void lefrSetMinFeatureCbk (lefrMinFeatureCbkFnType p0);
EXTERN void lefrSetNonDefaultCbk (lefrNonDefaultCbkFnType p0);
EXTERN void lefrSetSiteCbk (lefrSiteCbkFnType p0);
EXTERN void lefrSetMacroBeginCbk (lefrStringCbkFnType p0);
EXTERN void lefrSetPinCbk (lefrPinCbkFnType p0);
EXTERN void lefrSetObstructionCbk (lefrObstructionCbkFnType p0);
EXTERN void lefrSetArrayCbk (lefrArrayCbkFnType p0);
EXTERN void lefrSetMacroCbk (lefrMacroCbkFnType p0);
EXTERN void lefrSetLibraryEndCbk (lefrVoidCbkFnType p0);

/* NEW CALLBACK - each callback must have a function to allow the user        */
/* to set it.  Add the function here.                                         */

EXTERN void lefrSetTimingCbk (lefrTimingCbkFnType p0);
EXTERN void lefrSetSpacingBeginCbk (lefrVoidCbkFnType p0);
EXTERN void lefrSetSpacingEndCbk (lefrVoidCbkFnType p0);
EXTERN void lefrSetArrayBeginCbk (lefrStringCbkFnType p0);
EXTERN void lefrSetArrayEndCbk (lefrStringCbkFnType p0);
EXTERN void lefrSetIRDropBeginCbk (lefrVoidCbkFnType p0);
EXTERN void lefrSetIRDropEndCbk (lefrVoidCbkFnType p0);
EXTERN void lefrSetNoiseMarginCbk (lefrNoiseMarginCbkFnType p0);
EXTERN void lefrSetEdgeRateThreshold1Cbk (lefrDoubleCbkFnType p0);
EXTERN void lefrSetEdgeRateThreshold2Cbk (lefrDoubleCbkFnType p0);
EXTERN void lefrSetEdgeRateScaleFactorCbk (lefrDoubleCbkFnType p0);
EXTERN void lefrSetNoiseTableCbk (lefrNoiseTableCbkFnType p0);
EXTERN void lefrSetCorrectionTableCbk (lefrCorrectionTableCbkFnType p0);
EXTERN void lefrSetInputAntennaCbk (lefrDoubleCbkFnType p0);
EXTERN void lefrSetOutputAntennaCbk (lefrDoubleCbkFnType p0);
EXTERN void lefrSetInoutAntennaCbk (lefrDoubleCbkFnType p0);
EXTERN void lefrSetAntennaInputCbk (lefrDoubleCbkFnType p0);
EXTERN void lefrSetAntennaInoutCbk (lefrDoubleCbkFnType p0);
EXTERN void lefrSetAntennaOutputCbk (lefrDoubleCbkFnType p0);
EXTERN void lefrSetClearanceMeasureCbk (lefrStringCbkFnType p0);
EXTERN void lefrSetManufacturingCbk (lefrDoubleCbkFnType p0);
EXTERN void lefrSetUseMinSpacingCbk (lefrUseMinSpacingCbkFnType p0);
EXTERN void lefrSetMacroClassTypeCbk (lefrStringCbkFnType p0);
EXTERN void lefrSetMacroOriginCbk (lefrMacroNumCbkFnType p0);
EXTERN void lefrSetMacroSizeCbk (lefrMacroNumCbkFnType p0);
EXTERN void lefrSetMacroFixedMaskCbk (lefrIntegerCbkFnType p0);
EXTERN void lefrSetMacroEndCbk (lefrStringCbkFnType p0);
EXTERN void lefrSetMaxStackViaCbk (lefrMaxStackViaCbkFnType p0);
EXTERN void lefrSetExtensionCbk (lefrStringCbkFnType p0);
EXTERN void lefrSetDensityCbk (lefrDensityCbkFnType p0);
EXTERN void lefrSetFixedMaskCbk (lefrIntegerCbkFnType p0);

/* Set all of the callbacks that have not yet been set to the following       */
/* function.  This is especially useful if you want to check to see           */
/* if you forgot anything.                                                    */
EXTERN void lefrSetUnusedCallbacks (lefrVoidCbkFnType  func);

/* Reset all the callback functions to nil                                    */
EXTERN void lefrUnsetCallbacks ();

/* Functions to call to unregister a callback function.                       */
EXTERN void lefrUnsetAntennaInputCbk ();
EXTERN void lefrUnsetAntennaInoutCbk ();
EXTERN void lefrUnsetAntennaOutputCbk ();
EXTERN void lefrUnsetArrayBeginCbk ();
EXTERN void lefrUnsetArrayCbk ();
EXTERN void lefrUnsetArrayEndCbk ();
EXTERN void lefrUnsetBusBitCharsCbk ();
EXTERN void lefrUnsetCaseSensitiveCbk ();
EXTERN void lefrUnsetClearanceMeasureCbk ();
EXTERN void lefrUnsetCorrectionTableCbk ();
EXTERN void lefrUnsetDensityCbk ();
EXTERN void lefrUnsetDielectricCbk ();
EXTERN void lefrUnsetDividerCharCbk ();
EXTERN void lefrUnsetEdgeRateScaleFactorCbk ();
EXTERN void lefrUnsetEdgeRateThreshold1Cbk ();
EXTERN void lefrUnsetEdgeRateThreshold2Cbk ();
EXTERN void lefrUnsetExtensionCbk ();
EXTERN void lefrUnsetInoutAntennaCbk ();
EXTERN void lefrUnsetInputAntennaCbk ();
EXTERN void lefrUnsetIRDropBeginCbk ();
EXTERN void lefrUnsetIRDropCbk ();
EXTERN void lefrUnsetIRDropEndCbk ();
EXTERN void lefrUnsetLayerCbk ();
EXTERN void lefrUnsetLibraryEndCbk ();
EXTERN void lefrUnsetMacroBeginCbk ();
EXTERN void lefrUnsetMacroCbk ();
EXTERN void lefrUnsetMacroClassTypeCbk ();
EXTERN void lefrUnsetMacroEndCbk ();
EXTERN void lefrUnsetMacroOriginCbk ();
EXTERN void lefrUnsetMacroSizeCbk ();
EXTERN void lefrUnsetManufacturingCbk ();
EXTERN void lefrUnsetMaxStackViaCbk ();
EXTERN void lefrUnsetMinFeatureCbk ();
EXTERN void lefrUnsetNoiseMarginCbk ();
EXTERN void lefrUnsetNoiseTableCbk ();
EXTERN void lefrUnsetNonDefaultCbk ();
EXTERN void lefrUnsetNoWireExtensionCbk ();
EXTERN void lefrUnsetObstructionCbk ();
EXTERN void lefrUnsetOutputAntennaCbk ();
EXTERN void lefrUnsetPinCbk ();
EXTERN void lefrUnsetPropBeginCbk ();
EXTERN void lefrUnsetPropCbk ();
EXTERN void lefrUnsetPropEndCbk ();
EXTERN void lefrUnsetSiteCbk ();
EXTERN void lefrUnsetSpacingBeginCbk ();
EXTERN void lefrUnsetSpacingCbk ();
EXTERN void lefrUnsetSpacingEndCbk ();
EXTERN void lefrUnsetTimingCbk ();
EXTERN void lefrUnsetUseMinSpacingCbk ();
EXTERN void lefrUnsetUnitsCbk ();
EXTERN void lefrUnsetVersionCbk ();
EXTERN void lefrUnsetVersionStrCbk ();
EXTERN void lefrUnsetViaCbk ();
EXTERN void lefrUnsetViaRuleCbk ();

/* Return the current line number in the parser.                              */
EXTERN int lefrLineNumber ();

/* Routine to set the message logging routine for errors                      */
typedef void (*LEFI_LOG_FUNCTION) (const char*);
EXTERN void lefrSetLogFunction (LEFI_LOG_FUNCTION p0);

/* Routine to set the message logging routine for warnings                    */
typedef void (*LEFI_WARNING_LOG_FUNCTION) (const char*);
EXTERN void lefrSetWarningLogFunction (LEFI_WARNING_LOG_FUNCTION p0);

/* Routine to set the user defined malloc routine                             */
typedef void* (*LEFI_MALLOC_FUNCTION) (int);
EXTERN void lefrSetMallocFunction (LEFI_MALLOC_FUNCTION p0);

/* Routine to set the user defined realloc routine                            */
typedef void* (*LEFI_REALLOC_FUNCTION) (void *, int);
EXTERN void lefrSetReallocFunction (LEFI_REALLOC_FUNCTION p0);

/* Routine to set the user defined free routine                               */
typedef void (*LEFI_FREE_FUNCTION) (void *);
EXTERN void lefrSetFreeFunction (LEFI_FREE_FUNCTION p0);

/* Routine to set the line number callback routine                            */
typedef void (*LEFI_LINE_NUMBER_FUNCTION) (int);
EXTERN void lefrSetLineNumberFunction (LEFI_LINE_NUMBER_FUNCTION p0);

/* Set the number of lines before calling the line function callback routine  */
/* Default is 10000                                                           */
EXTERN void lefrSetDeltaNumberLines (int p0);

/* PCR 551229 - Set the parser to be more relax                               */
/* This api is specific for PKS.                                              */
/* When in relax mode, the parser will not require width, pitch, & direction  */
/* in routing layers. Also vias in nondefault rules                           */
EXTERN void lefrSetRelaxMode ();
EXTERN void lefrUnsetRelaxMode ();

/* PCR 565274 - LEF/DEF API should have the API call to overwrite default     */
/*              version                                                       */
EXTERN void lefrSetVersionValue (const  char*   version);

/* Routine to set the read function                                           */
typedef size_t (*LEFI_READ_FUNCTION) (FILE*, char*, size_t);
EXTERN void lefrSetReadFunction (LEFI_READ_FUNCTION p0);
EXTERN void lefrUnsetReadFunction ();

/* Routine to set the lefrWarning.log to open as append instead for write     */
/* New in 5.7                                                                 */
EXTERN void lefrSetOpenLogFileAppend ();
EXTERN void lefrUnsetOpenLogFileAppend ();

/* Routine to disable string property value process, default it will process  */
/* the value string                                                           */
EXTERN void lefrDisablePropStrProcess ();

/* Routine to set the max number of warnings for a perticular section         */

EXTERN void lefrSetAntennaInoutWarnings (int  warn);
EXTERN void lefrSetAntennaInputWarnings (int  warn);
EXTERN void lefrSetAntennaOutputWarnings (int  warn);
EXTERN void lefrSetArrayWarnings (int  warn);
EXTERN void lefrSetCaseSensitiveWarnings (int  warn);
EXTERN void lefrSetCorrectionTableWarnings (int  warn);
EXTERN void lefrSetDielectricWarnings (int  warn);
EXTERN void lefrSetEdgeRateThreshold1Warnings (int  warn);
EXTERN void lefrSetEdgeRateThreshold2Warnings (int  warn);
EXTERN void lefrSetEdgeRateScaleFactorWarnings (int  warn);
EXTERN void lefrSetInoutAntennaWarnings (int  warn);
EXTERN void lefrSetInputAntennaWarnings (int  warn);
EXTERN void lefrSetIRDropWarnings (int  warn);
EXTERN void lefrSetLayerWarnings (int  warn);
EXTERN void lefrSetMacroWarnings (int  warn);
EXTERN void lefrSetMaxStackViaWarnings (int  warn);
EXTERN void lefrSetMinFeatureWarnings (int  warn);
EXTERN void lefrSetNoiseMarginWarnings (int  warn);
EXTERN void lefrSetNoiseTableWarnings (int  warn);
EXTERN void lefrSetNonDefaultWarnings (int  warn);
EXTERN void lefrSetNoWireExtensionWarnings (int  warn);
EXTERN void lefrSetOutputAntennaWarnings (int  warn);
EXTERN void lefrSetPinWarnings (int  warn);
EXTERN void lefrSetSiteWarnings (int  warn);
EXTERN void lefrSetSpacingWarnings (int  warn);
EXTERN void lefrSetTimingWarnings (int  warn);
EXTERN void lefrSetUnitsWarnings (int  warn);
EXTERN void lefrSetUseMinSpacingWarnings (int  warn);
EXTERN void lefrSetViaRuleWarnings (int  warn);
EXTERN void lefrSetViaWarnings (int  warn);

/* Handling output messages                                                   */
EXTERN void lefrDisableParserMsgs (int  nMsg, int*  msgs);
EXTERN void lefrEnableParserMsgs (int  nMsg, int*  msgs);
EXTERN void lefrEnableAllMsgs ();
EXTERN void lefrDisableAllMsgs ();
EXTERN void lefrSetTotalMsgLimit (int  totNumMsgs);
EXTERN void lefrSetLimitPerMsg (int  msgId, int  numMsg);

/* Return codes for the user callbacks.                                       */
/* The user should return one of these values.                                */
#define PARSE_OK 0      
#define STOP_PARSE 1    
#define PARSE_ERROR 2   

#endif
