/* ************************************************************************** */
/* ************************************************************************** */
/* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!               */
/* ************************************************************************** */
/* ************************************************************************** */
/* Copyright 2013-2014, Cadence Design Systems                                */
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


#ifndef CDEFRREADER_H
#define CDEFRREADER_H

#include <stdio.h>
#include "defiTypedefs.h"

#define DEF_MSGS 4013
#define CBMAX 150    

/* An enum describing all of the types of reader callbacks.                   */
typedef enum {
  defrUnspecifiedCbkType = 0,
  defrDesignStartCbkType = 1,
  defrTechNameCbkType = 2,
  defrPropCbkType = 3,
  defrPropDefEndCbkType = 4,
  defrPropDefStartCbkType = 5,
  defrFloorPlanNameCbkType = 6,
  defrArrayNameCbkType = 7,
  defrUnitsCbkType = 8,
  defrDividerCbkType = 9,
  defrBusBitCbkType = 10,
  defrSiteCbkType = 11,
  defrComponentStartCbkType = 12,
  defrComponentCbkType = 13,
  defrComponentEndCbkType = 14,
  defrNetStartCbkType = 15,
  defrNetCbkType = 16,
  defrNetNameCbkType = 17,
  defrNetNonDefaultRuleCbkType = 18,
  defrNetSubnetNameCbkType = 19,
  defrNetEndCbkType = 20,
  defrPathCbkType = 21,
  defrVersionCbkType = 22,
  defrVersionStrCbkType = 23,
  defrComponentExtCbkType = 24,
  defrPinExtCbkType = 25,
  defrViaExtCbkType = 26,
  defrNetConnectionExtCbkType = 27,
  defrNetExtCbkType = 28,
  defrGroupExtCbkType = 29,
  defrScanChainExtCbkType = 30,
  defrIoTimingsExtCbkType = 31,
  defrPartitionsExtCbkType = 32,
  defrHistoryCbkType = 33,
  defrDieAreaCbkType = 34,
  defrCanplaceCbkType = 35,
  defrCannotOccupyCbkType = 36,
  defrPinCapCbkType = 37,
  defrDefaultCapCbkType = 38,
  defrStartPinsCbkType = 39,
  defrPinCbkType = 40,
  defrPinEndCbkType = 41,
  defrRowCbkType = 42,
  defrTrackCbkType = 43,
  defrGcellGridCbkType = 44,
  defrViaStartCbkType = 45,
  defrViaCbkType = 46,
  defrViaEndCbkType = 47,
  defrRegionStartCbkType = 48,
  defrRegionCbkType = 49,
  defrRegionEndCbkType = 50,
  defrSNetStartCbkType = 51,
  defrSNetCbkType = 52,
  defrSNetPartialPathCbkType = 53,
  defrSNetWireCbkType = 54,
  defrSNetEndCbkType = 55,
  defrGroupsStartCbkType = 56,
  defrGroupNameCbkType = 57,
  defrGroupMemberCbkType = 58,
  defrGroupCbkType = 59,
  defrGroupsEndCbkType = 60,
  defrAssertionsStartCbkType = 61,
  defrAssertionCbkType = 62,
  defrAssertionsEndCbkType = 63,
  defrConstraintsStartCbkType = 64,
  defrConstraintCbkType = 65,
  defrConstraintsEndCbkType = 66,
  defrScanchainsStartCbkType = 67,
  defrScanchainCbkType = 68,
  defrScanchainsEndCbkType = 69,
  defrIOTimingsStartCbkType = 70,
  defrIOTimingCbkType = 71,
  defrIOTimingsEndCbkType = 72,
  defrFPCStartCbkType = 73,
  defrFPCCbkType = 74,
  defrFPCEndCbkType = 75,
  defrTimingDisablesStartCbkType = 76,
  defrTimingDisableCbkType = 77,
  defrTimingDisablesEndCbkType = 78,
  defrPartitionsStartCbkType = 79,
  defrPartitionCbkType = 80,
  defrPartitionsEndCbkType = 81,
  defrPinPropStartCbkType = 82,
  defrPinPropCbkType = 83,
  defrPinPropEndCbkType = 84,
  defrBlockageStartCbkType = 85,
  defrBlockageCbkType = 86,
  defrBlockageEndCbkType = 87,
  defrSlotStartCbkType = 88,
  defrSlotCbkType = 89,
  defrSlotEndCbkType = 90,
  defrFillStartCbkType = 91,
  defrFillCbkType = 92,
  defrFillEndCbkType = 93,
  defrCaseSensitiveCbkType = 94,
  defrNonDefaultStartCbkType = 95,
  defrNonDefaultCbkType = 96,
  defrNonDefaultEndCbkType = 97,
  defrStylesStartCbkType = 98,
  defrStylesCbkType = 99,
  defrStylesEndCbkType = 100,
  defrExtensionCbkType = 101,

  /* NEW CALLBACK - If you are creating a new callback, you must add          */
  /* a unique item to this enum for each callback routine. When the           */
  /* callback is called in def.y you have to supply this enum item            */
  /* as an argument in the call.                                              */

  defrComponentMaskShiftLayerCbkType = 102,
  defrDesignEndCbkType = 103
} defrCallbackType_e;

/* Declarations of function signatures for each type of callback.             */
/* These declarations are type-safe when compiling with ANSI C                */
/* or C++; you will only be able to register a function pointer               */
/* with the correct signature for a given type of callback.                   */
/*                                                                            */
/* Each callback function is expected to return 0 if successful.              */
/* A non-zero return code will cause the reader to abort.                     */
/*                                                                            */
/* The defrDesignStart and defrDesignEnd callback is only called once.        */
/* Other callbacks may be called multiple times, each time with a different   */
/* set of data.                                                               */
/*                                                                            */
/* For each callback, the Def API will make the callback to the               */
/* function supplied by the client, which should either make a copy           */
/* of the Def object, or store the data in the client's own data structures.  */
/* The Def API will delete or reuse each object after making the callback,    */
/* so the client should not keep a pointer to it.                             */
/*                                                                            */
/* All callbacks pass the user data pointer provided in defrRead()            */
/* or defrSetUserData() back to the client; this can be used by the           */
/* client to obtain access to the rest of the client's data structures.       */
/*                                                                            */
/* The user data pointer is obtained using defrGetUserData() immediately      */
/* prior to making each callback, so the client is free to change the         */
/* user data on the fly if necessary.                                         */
/*                                                                            */
/* Callbacks with the same signature are passed a callback type               */
/* parameter, which allows an application to write a single callback          */
/* function, register that function for multiple callbacks, then              */
/* switch based on the callback type to handle the appropriate type of        */
/* data.                                                                      */

/* A declaration of the signature of all callbacks that return nothing.       */
typedef int (*defrVoidCbkFnType) (defrCallbackType_e, void* v, defiUserData);

/* A declaration of the signature of all callbacks that return a string.      */
typedef int (*defrStringCbkFnType) (defrCallbackType_e, const char *string, defiUserData);

/* A declaration of the signature of all callbacks that return a integer.     */
typedef int (*defrIntegerCbkFnType) (defrCallbackType_e, int number, defiUserData);

/* A declaration of the signature of all callbacks that return a double.      */
typedef int (*defrDoubleCbkFnType) (defrCallbackType_e, double number, defiUserData);

/* A declaration of the signature of all callbacks that return a defiProp.    */
typedef int (*defrPropCbkFnType) (defrCallbackType_e, defiProp *prop, defiUserData);

/* A declaration of the signature of all callbacks that return a defiSite.    */
typedef int (*defrSiteCbkFnType) (defrCallbackType_e, defiSite *site, defiUserData);

/* A declaration of the signature of all callbacks that return a defComponent */
typedef int (*defrComponentCbkFnType) (defrCallbackType_e, defiComponent *comp, defiUserData);

/* A declaration of the signature of all callbacks that return a defComponent */
typedef int (*defrComponentMaskShiftLayerCbkFnType) (defrCallbackType_e, defiComponentMaskShiftLayer *comp, defiUserData);

/* A declaration of the signature of all callbacks that return a defNet.      */
typedef int (*defrNetCbkFnType) (defrCallbackType_e, defiNet *net, defiUserData);

/* A declaration of the signature of all callbacks that return a defPath.     */
typedef int (*defrPathCbkFnType) (defrCallbackType_e, defiPath *path, defiUserData);

/* A declaration of the signature of all callbacks that return a defiBox.     */
typedef int (*defrBoxCbkFnType) (defrCallbackType_e, defiBox *box, defiUserData);

/* A declaration of the signature of all callbacks that return a defiPinCap.  */
typedef int (*defrPinCapCbkFnType) (defrCallbackType_e, defiPinCap *pincap, defiUserData);

/* A declaration of the signature of all callbacks that return a defiPin.     */
typedef int (*defrPinCbkFnType) (defrCallbackType_e, defiPin *pin, defiUserData);

/* A declaration of the signature of all callbacks that return a defiRow.     */
typedef int (*defrRowCbkFnType) (defrCallbackType_e, defiRow *row, defiUserData);

/* A declaration of the signature of all callbacks that return a defiTrack.   */
typedef int (*defrTrackCbkFnType) (defrCallbackType_e, defiTrack *track, defiUserData);

/* A declaration of the signature of all callbacks that return a defiGcellGri */
typedef int (*defrGcellGridCbkFnType) (defrCallbackType_e, defiGcellGrid *grid, defiUserData);

/* A declaration of the signature of all callbacks that return a defiVia.     */
typedef int (*defrViaCbkFnType) (defrCallbackType_e, defiVia *, defiUserData);

/* A declaration of the signature of all callbacks that return a defiRegion.  */
typedef int (*defrRegionCbkFnType) (defrCallbackType_e, defiRegion *, defiUserData);

/* A declaration of the signature of all callbacks that return a defiGroup.   */
typedef int (*defrGroupCbkFnType) (defrCallbackType_e, defiGroup *, defiUserData);

/* A declaration of the signature of all callbacks that return a defiAssertio */
typedef int (*defrAssertionCbkFnType) (defrCallbackType_e, defiAssertion *, defiUserData);

/* A declaration of the signature of all callbacks that return a defiScanChai */
typedef int (*defrScanchainCbkFnType) (defrCallbackType_e, defiScanchain *, defiUserData);

/* A declaration of the signature of all callbacks that return a defiIOTiming */
typedef int (*defrIOTimingCbkFnType) (defrCallbackType_e, defiIOTiming *, defiUserData);

/* A declaration of the signature of all callbacks that return a defiFPC.     */
typedef int (*defrFPCCbkFnType) (defrCallbackType_e, defiFPC *, defiUserData);

/* A declaration of the signature of all callbacks that return a defiTimingDi */
typedef int (*defrTimingDisableCbkFnType) (defrCallbackType_e, defiTimingDisable *, defiUserData);

/* A declaration of the signature of all callbacks that return a defiPartitio */
typedef int (*defrPartitionCbkFnType) (defrCallbackType_e, defiPartition *, defiUserData);

/* A declaration of the signature of all callbacks that return a defiPinProp. */
typedef int (*defrPinPropCbkFnType) (defrCallbackType_e, defiPinProp *, defiUserData);

/* A declaration of the signature of all callbacks that return a defiBlockage */
typedef int (*defrBlockageCbkFnType) (defrCallbackType_e, defiBlockage *, defiUserData);

/* A declaration of the signature of all callbacks that return a defiSlot.    */
typedef int (*defrSlotCbkFnType) (defrCallbackType_e, defiSlot *, defiUserData);

/* A declaration of the signature of all callbacks that return a defiFill.    */
typedef int (*defrFillCbkFnType) (defrCallbackType_e, defiFill *, defiUserData);

/* A declaration of the signature of all callbacks that return a defiNonDefau */
typedef int (*defrNonDefaultCbkFnType) (defrCallbackType_e, defiNonDefault *, defiUserData);

/* A declaration of the signature of all callbacks that return a defiStyles.  */
typedef int (*defrStylesCbkFnType) (defrCallbackType_e, defiStyles *, defiUserData);

/* NEW CALLBACK - Each callback must return user data, enum, and              */
/*   OUR-DATA item.  We must define a callback function type for              */
/*   each type of OUR-DATA.  Some routines return a string, some              */
/*   return an integer, and some return a pointer to a class.                 */
/*   If you create a new class, then you must create a new function           */
/*   type here to return that class to the user.                              */

/* The reader initialization.  Must be called before defrRead().              */
EXTERN int defrInit ();
EXTERN int defrInitSession (int  startSession);

/* obsoleted now.                                                             */
EXTERN int defrReset ();

/*Sets all parser memory into init state.                                     */
EXTERN int defrClear ();

/* Change the comment character in the DEF file.  The default                 */
/* is '#'                                                                     */
EXTERN void defrSetCommentChar (char  c);

/* Functions to call to set specific actions in the parser.                   */
EXTERN void defrSetAddPathToNet ();
EXTERN void defrSetAllowComponentNets ();
EXTERN int defrGetAllowComponentNets ();
EXTERN void defrSetCaseSensitivity (int  caseSense);

/* Functions to keep track of callbacks that the user did not                 */
/* supply.  Normally all parts of the DEF file that the user                  */
/* does not supply a callback for will be ignored.  These                     */
/* routines tell the parser count the DEF constructs that are                 */
/* present in the input file, but did not trigger a callback.                 */
/* This should help you find any "important" DEF constructs that              */
/* you are ignoring.                                                          */
EXTERN void defrSetRegisterUnusedCallbacks ();
EXTERN void defrPrintUnusedCallbacks (FILE*  log);

/* Obsoleted now.                                                             */
EXTERN int  defrReleaseNResetMemory ();

/* The main reader function.                                                  */
/* The file should already be opened.  This requirement allows                */
/* the reader to be used with stdin or a pipe.  The file name                 */
/* is only used for error messages.                                           */
EXTERN int defrRead (FILE * file, const char * fileName, defiUserData  userData, int  case_sensitive);

/* Set/get the client-provided user data.  defi doesn't look at               */
/* this data at all, it simply passes the opaque defiUserData pointer         */
/* back to the application with each callback.  The client can                */
/* change the data at any time, and it will take effect on the                */
/* next callback.  The defi reader and writer maintain separate               */
/* user data pointers.                                                        */
EXTERN void defrSetUserData (defiUserData p0);
EXTERN defiUserData defrGetUserData ();

/* Functions to call to register a callback function or get the function      */
/*pointer after it has been registered.                                       */
/*                                                                            */

/* Register one function for all callbacks with the same signature            */
EXTERN void defrSetArrayNameCbk (defrStringCbkFnType p0);
EXTERN void defrSetAssertionCbk (defrAssertionCbkFnType p0);
EXTERN void defrSetAssertionsStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetAssertionsEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetBlockageCbk (defrBlockageCbkFnType p0);
EXTERN void defrSetBlockageStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetBlockageEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetBusBitCbk (defrStringCbkFnType p0);
EXTERN void defrSetCannotOccupyCbk (defrSiteCbkFnType p0);
EXTERN void defrSetCanplaceCbk (defrSiteCbkFnType p0);
EXTERN void defrSetCaseSensitiveCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetComponentCbk (defrComponentCbkFnType p0);
EXTERN void defrSetComponentExtCbk (defrStringCbkFnType p0);
EXTERN void defrSetComponentStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetComponentEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetConstraintCbk (defrAssertionCbkFnType p0);
EXTERN void defrSetConstraintsStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetConstraintsEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetDefaultCapCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetDesignCbk (defrStringCbkFnType p0);
EXTERN void defrSetDesignEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetDieAreaCbk (defrBoxCbkFnType p0);
EXTERN void defrSetDividerCbk (defrStringCbkFnType p0);
EXTERN void defrSetExtensionCbk (defrStringCbkFnType p0);
EXTERN void defrSetFillCbk (defrFillCbkFnType p0);
EXTERN void defrSetFillStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetFillEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetFPCCbk (defrFPCCbkFnType p0);
EXTERN void defrSetFPCStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetFPCEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetFloorPlanNameCbk (defrStringCbkFnType p0);
EXTERN void defrSetGcellGridCbk (defrGcellGridCbkFnType p0);
EXTERN void defrSetGroupNameCbk (defrStringCbkFnType p0);
EXTERN void defrSetGroupMemberCbk (defrStringCbkFnType p0);
EXTERN void defrSetComponentMaskShiftLayerCbk (defrComponentMaskShiftLayerCbkFnType p0);
EXTERN void defrSetGroupCbk (defrGroupCbkFnType p0);
EXTERN void defrSetGroupExtCbk (defrStringCbkFnType p0);
EXTERN void defrSetGroupsStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetGroupsEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetHistoryCbk (defrStringCbkFnType p0);
EXTERN void defrSetIOTimingCbk (defrIOTimingCbkFnType p0);
EXTERN void defrSetIOTimingsStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetIOTimingsEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetIoTimingsExtCbk (defrStringCbkFnType p0);
EXTERN void defrSetNetCbk (defrNetCbkFnType p0);
EXTERN void defrSetNetNameCbk (defrStringCbkFnType p0);
EXTERN void defrSetNetNonDefaultRuleCbk (defrStringCbkFnType p0);
EXTERN void defrSetNetConnectionExtCbk (defrStringCbkFnType p0);
EXTERN void defrSetNetExtCbk (defrStringCbkFnType p0);
EXTERN void defrSetNetPartialPathCbk (defrNetCbkFnType p0);
EXTERN void defrSetNetSubnetNameCbk (defrStringCbkFnType p0);
EXTERN void defrSetNetStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetNetEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetNonDefaultCbk (defrNonDefaultCbkFnType p0);
EXTERN void defrSetNonDefaultStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetNonDefaultEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetPartitionCbk (defrPartitionCbkFnType p0);
EXTERN void defrSetPartitionsExtCbk (defrStringCbkFnType p0);
EXTERN void defrSetPartitionsStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetPartitionsEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetPathCbk (defrPathCbkFnType p0);
EXTERN void defrSetPinCapCbk (defrPinCapCbkFnType p0);
EXTERN void defrSetPinCbk (defrPinCbkFnType p0);
EXTERN void defrSetPinExtCbk (defrStringCbkFnType p0);
EXTERN void defrSetPinPropCbk (defrPinPropCbkFnType p0);
EXTERN void defrSetPinPropStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetPinPropEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetPropCbk (defrPropCbkFnType p0);
EXTERN void defrSetPropDefEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetPropDefStartCbk (defrVoidCbkFnType p0);
EXTERN void defrSetRegionCbk (defrRegionCbkFnType p0);
EXTERN void defrSetRegionStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetRegionEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetRowCbk (defrRowCbkFnType p0);
EXTERN void defrSetSNetCbk (defrNetCbkFnType p0);
EXTERN void defrSetSNetStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetSNetEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetSNetPartialPathCbk (defrNetCbkFnType p0);
EXTERN void defrSetSNetWireCbk (defrNetCbkFnType p0);
EXTERN void defrSetScanChainExtCbk (defrStringCbkFnType p0);
EXTERN void defrSetScanchainCbk (defrScanchainCbkFnType p0);
EXTERN void defrSetScanchainsStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetScanchainsEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetSiteCbk (defrSiteCbkFnType p0);
EXTERN void defrSetSlotCbk (defrSlotCbkFnType p0);
EXTERN void defrSetSlotStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetSlotEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetStartPinsCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetStylesCbk (defrStylesCbkFnType p0);
EXTERN void defrSetStylesStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetStylesEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetPinEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetTechnologyCbk (defrStringCbkFnType p0);
EXTERN void defrSetTimingDisableCbk (defrTimingDisableCbkFnType p0);
EXTERN void defrSetTimingDisablesStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetTimingDisablesEndCbk (defrVoidCbkFnType p0);
EXTERN void defrSetTrackCbk (defrTrackCbkFnType p0);
EXTERN void defrSetUnitsCbk (defrDoubleCbkFnType p0);
EXTERN void defrSetVersionCbk (defrDoubleCbkFnType p0);
EXTERN void defrSetVersionStrCbk (defrStringCbkFnType p0);
EXTERN void defrSetViaCbk (defrViaCbkFnType p0);
EXTERN void defrSetViaExtCbk (defrStringCbkFnType p0);
EXTERN void defrSetViaStartCbk (defrIntegerCbkFnType p0);
EXTERN void defrSetViaEndCbk (defrVoidCbkFnType p0);

/* NEW CALLBACK - For each new callback you create, you must                  */
/* create a routine that allows the user to set it.  Add the                  */
/* setting routines here.                                                     */

/*Set all of the callbacks that have not yet been set to the following        */
/*function.  This is especially useful if you want to check to see            */
/*if you forgot anything.                                                     */
EXTERN void defrUnsetCallbacks ();

/* Functions to call to unregister a callback function.                       */
EXTERN void defrUnsetArrayNameCbk ();
EXTERN void defrUnsetAssertionCbk ();
EXTERN void defrUnsetAssertionsStartCbk ();
EXTERN void defrUnsetAssertionsEndCbk ();
EXTERN void defrUnsetBlockageCbk ();
EXTERN void defrUnsetBlockageStartCbk ();
EXTERN void defrUnsetBlockageEndCbk ();
EXTERN void defrUnsetBusBitCbk ();
EXTERN void defrUnsetCannotOccupyCbk ();
EXTERN void defrUnsetCanplaceCbk ();
EXTERN void defrUnsetCaseSensitiveCbk ();
EXTERN void defrUnsetComponentCbk ();
EXTERN void defrUnsetComponentExtCbk ();
EXTERN void defrUnsetComponentStartCbk ();
EXTERN void defrUnsetComponentEndCbk ();
EXTERN void defrUnsetConstraintCbk ();
EXTERN void defrUnsetConstraintsStartCbk ();
EXTERN void defrUnsetConstraintsEndCbk ();
EXTERN void defrUnsetDefaultCapCbk ();
EXTERN void defrUnsetDesignCbk ();
EXTERN void defrUnsetDesignEndCbk ();
EXTERN void defrUnsetDieAreaCbk ();
EXTERN void defrUnsetDividerCbk ();
EXTERN void defrUnsetExtensionCbk ();
EXTERN void defrUnsetFillCbk ();
EXTERN void defrUnsetFillStartCbk ();
EXTERN void defrUnsetFillEndCbk ();
EXTERN void defrUnsetFPCCbk ();
EXTERN void defrUnsetFPCStartCbk ();
EXTERN void defrUnsetFPCEndCbk ();
EXTERN void defrUnsetFloorPlanNameCbk ();
EXTERN void defrUnsetGcellGridCbk ();
EXTERN void defrUnsetGroupCbk ();
EXTERN void defrUnsetGroupExtCbk ();
EXTERN void defrUnsetGroupMemberCbk ();
EXTERN void defrUnsetComponentMaskShiftLayerCbk ();
EXTERN void defrUnsetGroupNameCbk ();
EXTERN void defrUnsetGroupsStartCbk ();
EXTERN void defrUnsetGroupsEndCbk ();
EXTERN void defrUnsetHistoryCbk ();
EXTERN void defrUnsetIOTimingCbk ();
EXTERN void defrUnsetIOTimingsStartCbk ();
EXTERN void defrUnsetIOTimingsEndCbk ();
EXTERN void defrUnsetIOTimingsExtCbk ();
EXTERN void defrUnsetNetCbk ();
EXTERN void defrUnsetNetNameCbk ();
EXTERN void defrUnsetNetNonDefaultRuleCbk ();
EXTERN void defrUnsetNetConnectionExtCbk ();
EXTERN void defrUnsetNetExtCbk ();
EXTERN void defrUnsetNetPartialPathCbk ();
EXTERN void defrUnsetNetSubnetNameCbk ();
EXTERN void defrUnsetNetStartCbk ();
EXTERN void defrUnsetNetEndCbk ();
EXTERN void defrUnsetNonDefaultCbk ();
EXTERN void defrUnsetNonDefaultStartCbk ();
EXTERN void defrUnsetNonDefaultEndCbk ();
EXTERN void defrUnsetPartitionCbk ();
EXTERN void defrUnsetPartitionsExtCbk ();
EXTERN void defrUnsetPartitionsStartCbk ();
EXTERN void defrUnsetPartitionsEndCbk ();
EXTERN void defrUnsetPathCbk ();
EXTERN void defrUnsetPinCapCbk ();
EXTERN void defrUnsetPinCbk ();
EXTERN void defrUnsetPinEndCbk ();
EXTERN void defrUnsetPinExtCbk ();
EXTERN void defrUnsetPinPropCbk ();
EXTERN void defrUnsetPinPropStartCbk ();
EXTERN void defrUnsetPinPropEndCbk ();
EXTERN void defrUnsetPropCbk ();
EXTERN void defrUnsetPropDefEndCbk ();
EXTERN void defrUnsetPropDefStartCbk ();
EXTERN void defrUnsetRegionCbk ();
EXTERN void defrUnsetRegionStartCbk ();
EXTERN void defrUnsetRegionEndCbk ();
EXTERN void defrUnsetRowCbk ();
EXTERN void defrUnsetScanChainExtCbk ();
EXTERN void defrUnsetScanchainCbk ();
EXTERN void defrUnsetScanchainsStartCbk ();
EXTERN void defrUnsetScanchainsEndCbk ();
EXTERN void defrUnsetSiteCbk ();
EXTERN void defrUnsetSlotCbk ();
EXTERN void defrUnsetSlotStartCbk ();
EXTERN void defrUnsetSlotEndCbk ();
EXTERN void defrUnsetSNetWireCbk ();
EXTERN void defrUnsetSNetCbk ();
EXTERN void defrUnsetSNetStartCbk ();
EXTERN void defrUnsetSNetEndCbk ();
EXTERN void defrUnsetSNetPartialPathCbk ();
EXTERN void defrUnsetStartPinsCbk ();
EXTERN void defrUnsetStylesCbk ();
EXTERN void defrUnsetStylesStartCbk ();
EXTERN void defrUnsetStylesEndCbk ();
EXTERN void defrUnsetTechnologyCbk ();
EXTERN void defrUnsetTimingDisableCbk ();
EXTERN void defrUnsetTimingDisablesStartCbk ();
EXTERN void defrUnsetTimingDisablesEndCbk ();
EXTERN void defrUnsetTrackCbk ();
EXTERN void defrUnsetUnitsCbk ();
EXTERN void defrUnsetVersionCbk ();
EXTERN void defrUnsetVersionStrCbk ();
EXTERN void defrUnsetViaCbk ();
EXTERN void defrUnsetViaExtCbk ();
EXTERN void defrUnsetViaStartCbk ();
EXTERN void defrUnsetViaEndCbk ();

/* Routine to set all unused callbacks. This is useful for checking           */
/*to see if you missed something.                                             */
EXTERN void defrSetUnusedCallbacks (defrVoidCbkFnType  func);

/* Return the current line number in the input file.                          */
EXTERN int defrLineNumber ();
EXTERN long long defrLongLineNumber ();

/* Routine to set the message logging routine for errors                      */
    typedef void (*DEFI_LOG_FUNCTION) (const char*);
EXTERN void defrSetLogFunction (DEFI_LOG_FUNCTION p0);

/* Routine to set the message logging routine for warnings                    */
#ifndef DEFI_WARNING_LOG_FUNCTION
    typedef void (*DEFI_WARNING_LOG_FUNCTION) (const char*);
#endif
EXTERN void defrSetWarningLogFunction (DEFI_WARNING_LOG_FUNCTION p0);

/* Routine to set the user defined malloc routine                             */
typedef void* (*DEFI_MALLOC_FUNCTION) (size_t);
EXTERN void defrSetMallocFunction (DEFI_MALLOC_FUNCTION p0);

/* Routine to set the user defined realloc routine                            */
typedef void* (*DEFI_REALLOC_FUNCTION) (void*, size_t);
EXTERN void defrSetReallocFunction (DEFI_REALLOC_FUNCTION p0);

/* Routine to set the user defined free routine                               */
typedef void (*DEFI_FREE_FUNCTION) (void *);
EXTERN void defrSetFreeFunction (DEFI_FREE_FUNCTION p0);

/* Routine to set the line number of the file that is parsing routine (takes  */
typedef void (*DEFI_LINE_NUMBER_FUNCTION)  (int);
EXTERN void defrSetLineNumberFunction (DEFI_LINE_NUMBER_FUNCTION p0);

/* Routine to set the line number of the file that is parsing routine (takes  */
typedef void (*DEFI_LONG_LINE_NUMBER_FUNCTION)  (long long);
EXTERN void defrSetLongLineNumberFunction (DEFI_LONG_LINE_NUMBER_FUNCTION p0);

/* Set the number of lines before calling the line function callback routine  */
/* Default is 10000                                                           */
EXTERN void defrSetDeltaNumberLines (int p0);

/* Routine to set the read function                                           */
typedef size_t (*DEFI_READ_FUNCTION)  (FILE*, char*, size_t);
EXTERN void defrSetReadFunction (DEFI_READ_FUNCTION p0);
EXTERN void defrUnsetReadFunction ();

/* Routine to set the defrWarning.log to open as append instead for write     */
/* New in 5.7                                                                 */
EXTERN void defrSetOpenLogFileAppend ();
EXTERN void defrUnsetOpenLogFileAppend ();

/* Routine to set the magic comment found routine                             */
typedef void (*DEFI_MAGIC_COMMENT_FOUND_FUNCTION) ();
EXTERN void defrSetMagicCommentFoundFunction (DEFI_MAGIC_COMMENT_FOUND_FUNCTION p0);

/* Routine to set the magic comment string                                    */
EXTERN void defrSetMagicCommentString (char * p0);

/* Routine to disable string property value process, default it will process  */
/* the value string                                                           */
EXTERN void defrDisablePropStrProcess ();

/* Testing purposes only                                                      */
EXTERN void defrSetNLines (long long  n);

/* Routine to set the max number of warnings for a perticular section         */

EXTERN void defrSetAssertionWarnings (int  warn);
EXTERN void defrSetBlockageWarnings (int  warn);
EXTERN void defrSetCaseSensitiveWarnings (int  warn);
EXTERN void defrSetComponentWarnings (int  warn);
EXTERN void defrSetConstraintWarnings (int  warn);
EXTERN void defrSetDefaultCapWarnings (int  warn);
EXTERN void defrSetGcellGridWarnings (int  warn);
EXTERN void defrSetIOTimingWarnings (int  warn);
EXTERN void defrSetNetWarnings (int  warn);
EXTERN void defrSetNonDefaultWarnings (int  warn);
EXTERN void defrSetPinExtWarnings (int  warn);
EXTERN void defrSetPinWarnings (int  warn);
EXTERN void defrSetRegionWarnings (int  warn);
EXTERN void defrSetRowWarnings (int  warn);
EXTERN void defrSetScanchainWarnings (int  warn);
EXTERN void defrSetSNetWarnings (int  warn);
EXTERN void defrSetStylesWarnings (int  warn);
EXTERN void defrSetTrackWarnings (int  warn);
EXTERN void defrSetUnitsWarnings (int  warn);
EXTERN void defrSetVersionWarnings (int  warn);
EXTERN void defrSetViaWarnings (int  warn);

/* Handling output messages                                                   */
EXTERN void defrDisableParserMsgs (int  nMsg, int*  msgs);
EXTERN void defrEnableParserMsgs (int  nMsg, int*  msgs);
EXTERN void defrEnableAllMsgs ();
EXTERN void defrSetTotalMsgLimit (int  totNumMsgs);
EXTERN void defrSetLimitPerMsg (int  msgId, int  numMsg);

/* Return codes for the user callbacks.                                       */
/*The user should return one of these values.                                 */
#define PARSE_OK 0      
#define STOP_PARSE 1    
#define PARSE_ERROR 2   

/* Add this alias to the list for the parser                                  */
EXTERN void defrAddAlias (const char*  key, const char*  value, int  marked, defrData * data);

#endif
