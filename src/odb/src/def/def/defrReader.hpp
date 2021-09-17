// *****************************************************************************
// *****************************************************************************
// Copyright 2013-2016, Cadence Design Systems
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

#ifndef DEFRREADER_H
#define DEFRREADER_H

#include <stdarg.h>

#include "defiKRDefs.hpp"
#include "defiDefs.hpp"
#include "defiUser.hpp"

#define DEF_MSGS 4013
#define CBMAX 150    // Number of callbacks.

BEGIN_LEFDEF_PARSER_NAMESPACE

// An enum describing all of the types of reader callbacks.
typedef enum {
  defrUnspecifiedCbkType = 0,
  defrDesignStartCbkType,
  defrTechNameCbkType,
  defrPropCbkType,
  defrPropDefEndCbkType,
  defrPropDefStartCbkType,
  defrFloorPlanNameCbkType,
  defrArrayNameCbkType,
  defrUnitsCbkType,
  defrDividerCbkType,
  defrBusBitCbkType,
  defrSiteCbkType,
  defrComponentStartCbkType,
  defrComponentCbkType,
  defrComponentEndCbkType,
  defrNetStartCbkType,
  defrNetCbkType,
  defrNetNameCbkType,
  defrNetNonDefaultRuleCbkType,
  defrNetSubnetNameCbkType,
  defrNetEndCbkType,
  defrPathCbkType,
  defrVersionCbkType,
  defrVersionStrCbkType,
  defrComponentExtCbkType,
  defrPinExtCbkType,
  defrViaExtCbkType,
  defrNetConnectionExtCbkType,
  defrNetExtCbkType,
  defrGroupExtCbkType,
  defrScanChainExtCbkType,
  defrIoTimingsExtCbkType,
  defrPartitionsExtCbkType,
  defrHistoryCbkType,
  defrDieAreaCbkType,
  defrCanplaceCbkType,
  defrCannotOccupyCbkType,
  defrPinCapCbkType,
  defrDefaultCapCbkType,
  defrStartPinsCbkType,
  defrPinCbkType,
  defrPinEndCbkType,
  defrRowCbkType,
  defrTrackCbkType,
  defrGcellGridCbkType,
  defrViaStartCbkType,
  defrViaCbkType,
  defrViaEndCbkType,
  defrRegionStartCbkType,
  defrRegionCbkType,
  defrRegionEndCbkType,
  defrSNetStartCbkType,
  defrSNetCbkType,
  defrSNetPartialPathCbkType,
  defrSNetWireCbkType,
  defrSNetEndCbkType,
  defrGroupsStartCbkType,
  defrGroupNameCbkType,
  defrGroupMemberCbkType,
  defrGroupCbkType,
  defrGroupsEndCbkType,
  defrAssertionsStartCbkType,
  defrAssertionCbkType,
  defrAssertionsEndCbkType,
  defrConstraintsStartCbkType,
  defrConstraintCbkType,
  defrConstraintsEndCbkType,
  defrScanchainsStartCbkType,
  defrScanchainCbkType,
  defrScanchainsEndCbkType,
  defrIOTimingsStartCbkType,
  defrIOTimingCbkType,
  defrIOTimingsEndCbkType,
  defrFPCStartCbkType,
  defrFPCCbkType,
  defrFPCEndCbkType,
  defrTimingDisablesStartCbkType,
  defrTimingDisableCbkType,
  defrTimingDisablesEndCbkType,
  defrPartitionsStartCbkType,
  defrPartitionCbkType,
  defrPartitionsEndCbkType,
  defrPinPropStartCbkType,
  defrPinPropCbkType,
  defrPinPropEndCbkType,
  defrBlockageStartCbkType,
  defrBlockageCbkType,
  defrBlockageEndCbkType,
  defrSlotStartCbkType,
  defrSlotCbkType,
  defrSlotEndCbkType,
  defrFillStartCbkType,
  defrFillCbkType,
  defrFillEndCbkType,
  defrCaseSensitiveCbkType,
  defrNonDefaultStartCbkType,
  defrNonDefaultCbkType,
  defrNonDefaultEndCbkType,
  defrStylesStartCbkType,
  defrStylesCbkType,
  defrStylesEndCbkType,
  defrExtensionCbkType,

  // NEW CALLBACK - If you are creating a new callback, you must add
  // a unique item to this enum for each callback routine. When the
  // callback is called in def.y you have to supply this enum item
  // as an argument in the call.

  defrComponentMaskShiftLayerCbkType,
  defrDesignEndCbkType
} defrCallbackType_e;
 
 
// Declarations of function signatures for each type of callback.
// These declarations are type-safe when compiling with ANSI C
// or C++; you will only be able to register a function pointer
// with the correct signature for a given type of callback.
//
// Each callback function is expected to return 0 if successful.
// A non-zero return code will cause the reader to abort.
//
// The defrDesignStart and defrDesignEnd callback is only called once.
// Other callbacks may be called multiple times, each time with a different
// set of data.
//
// For each callback, the Def API will make the callback to the
// function supplied by the client, which should either make a copy
// of the Def object, or store the data in the client's own data structures.
// The Def API will delete or reuse each object after making the callback,
// so the client should not keep a pointer to it.
//
// All callbacks pass the user data pointer provided in defrRead()
// or defrSetUserData() back to the client; this can be used by the
// client to obtain access to the rest of the client's data structures.
//
// The user data pointer is obtained using defrGetUserData() immediately
// prior to making each callback, so the client is free to change the
// user data on the fly if necessary.
//
// Callbacks with the same signature are passed a callback type
// parameter, which allows an application to write a single callback
// function, register that function for multiple callbacks, then
// switch based on the callback type to handle the appropriate type of
// data.
 

// A declaration of the signature of all callbacks that return nothing.     
typedef int (*defrVoidCbkFnType) (defrCallbackType_e, void* v, defiUserData);

// A declaration of the signature of all callbacks that return a string.     
typedef int (*defrStringCbkFnType) (defrCallbackType_e, const char *string, defiUserData);
 
// A declaration of the signature of all callbacks that return a integer.     
typedef int (*defrIntegerCbkFnType) (defrCallbackType_e, int number, defiUserData);
 
// A declaration of the signature of all callbacks that return a double.     
typedef int (*defrDoubleCbkFnType) (defrCallbackType_e, double number, defiUserData);
 
// A declaration of the signature of all callbacks that return a defiProp.     
typedef int (*defrPropCbkFnType) (defrCallbackType_e, defiProp *prop, defiUserData);
 
// A declaration of the signature of all callbacks that return a defiSite.     
typedef int (*defrSiteCbkFnType) (defrCallbackType_e, defiSite *site, defiUserData);
 
// A declaration of the signature of all callbacks that return a defComponent.     
typedef int (*defrComponentCbkFnType) (defrCallbackType_e, defiComponent *comp, defiUserData);

// A declaration of the signature of all callbacks that return a defComponentMaskShiftLayer.     
typedef int (*defrComponentMaskShiftLayerCbkFnType) (defrCallbackType_e, defiComponentMaskShiftLayer *comp, defiUserData);
 
// A declaration of the signature of all callbacks that return a defNet.     
typedef int (*defrNetCbkFnType) (defrCallbackType_e, defiNet *net, defiUserData);

// A declaration of the signature of all callbacks that return a defPath.     
typedef int (*defrPathCbkFnType) (defrCallbackType_e, defiPath *path, defiUserData);
 
// A declaration of the signature of all callbacks that return a defiBox.     
typedef int (*defrBoxCbkFnType) (defrCallbackType_e, defiBox *box, defiUserData);
 

// A declaration of the signature of all callbacks that return a defiPinCap.     
typedef int (*defrPinCapCbkFnType) (defrCallbackType_e, defiPinCap *pincap, defiUserData);
 

// A declaration of the signature of all callbacks that return a defiPin.     
typedef int (*defrPinCbkFnType) (defrCallbackType_e, defiPin *pin, defiUserData);
 

// A declaration of the signature of all callbacks that return a defiRow.     
typedef int (*defrRowCbkFnType) (defrCallbackType_e, defiRow *row, defiUserData);
 

// A declaration of the signature of all callbacks that return a defiTrack.     
typedef int (*defrTrackCbkFnType) (defrCallbackType_e, defiTrack *track, defiUserData);
 

// A declaration of the signature of all callbacks that return a defiGcellGrid.     
typedef int (*defrGcellGridCbkFnType) (defrCallbackType_e, defiGcellGrid *grid, defiUserData);
 

// A declaration of the signature of all callbacks that return a defiVia.     
typedef int (*defrViaCbkFnType) (defrCallbackType_e, defiVia *, defiUserData);
 

// A declaration of the signature of all callbacks that return a defiRegion.     
typedef int (*defrRegionCbkFnType) (defrCallbackType_e, defiRegion *, defiUserData);
 

// A declaration of the signature of all callbacks that return a defiGroup.     
typedef int (*defrGroupCbkFnType) (defrCallbackType_e, defiGroup *, defiUserData);
 

// A declaration of the signature of all callbacks that return a defiAssertion. 
typedef int (*defrAssertionCbkFnType) (defrCallbackType_e, defiAssertion *, defiUserData);
 

// A declaration of the signature of all callbacks that return a defiScanChain. 
typedef int (*defrScanchainCbkFnType) (defrCallbackType_e, defiScanchain *, defiUserData);
 

// A declaration of the signature of all callbacks that return a defiIOTiming. 
typedef int (*defrIOTimingCbkFnType) (defrCallbackType_e, defiIOTiming *, defiUserData);
 

// A declaration of the signature of all callbacks that return a defiFPC. 
typedef int (*defrFPCCbkFnType) (defrCallbackType_e, defiFPC *, defiUserData);
 

// A declaration of the signature of all callbacks that return a defiTimingDisable. 
typedef int (*defrTimingDisableCbkFnType) (defrCallbackType_e, defiTimingDisable *, defiUserData);
 

// A declaration of the signature of all callbacks that return a defiPartition. 
typedef int (*defrPartitionCbkFnType) (defrCallbackType_e, defiPartition *, defiUserData);
 
// A declaration of the signature of all callbacks that return a defiPinProp. 
typedef int (*defrPinPropCbkFnType) (defrCallbackType_e, defiPinProp *, defiUserData);

// A declaration of the signature of all callbacks that return a defiBlockage. 
typedef int (*defrBlockageCbkFnType) (defrCallbackType_e, defiBlockage *, defiUserData);

// A declaration of the signature of all callbacks that return a defiSlot. 
typedef int (*defrSlotCbkFnType) (defrCallbackType_e, defiSlot *, defiUserData);

// A declaration of the signature of all callbacks that return a defiFill. 
typedef int (*defrFillCbkFnType) (defrCallbackType_e, defiFill *, defiUserData);

// A declaration of the signature of all callbacks that return a defiNonDefault.
typedef int (*defrNonDefaultCbkFnType) (defrCallbackType_e, defiNonDefault *, defiUserData);

// A declaration of the signature of all callbacks that return a defiStyles. 
typedef int (*defrStylesCbkFnType) (defrCallbackType_e, defiStyles *, defiUserData);

// NEW CALLBACK - Each callback must return user data, enum, and
//   OUR-DATA item.  We must define a callback function type for
//   each type of OUR-DATA.  Some routines return a string, some
//   return an integer, and some return a pointer to a class.
//   If you create a new class, then you must create a new function
//   type here to return that class to the user. 

// The reader initialization.  Must be called before defrRead().
extern int defrInit ();
extern int defrInitSession (int startSession = 1);

// obsoleted now.
extern int defrReset ();

//Sets all parser memory into init state.
extern int defrClear();

// Change the comment character in the DEF file.  The default
// is '#' 
extern void defrSetCommentChar (char c);

// Functions to call to set specific actions in the parser.
extern void defrSetAddPathToNet ();
extern void defrSetAllowComponentNets ();
extern int defrGetAllowComponentNets ();
extern void defrSetCaseSensitivity (int caseSense);

// Functions to keep track of callbacks that the user did not
// supply.  Normally all parts of the DEF file that the user
// does not supply a callback for will be ignored.  These
// routines tell the parser count the DEF constructs that are
// present in the input file, but did not trigger a callback.
// This should help you find any "important" DEF constructs that
// you are ignoring.
extern void defrSetRegisterUnusedCallbacks ();
extern void defrPrintUnusedCallbacks (FILE* log);
// Obsoleted now.
extern int  defrReleaseNResetMemory ();

// This function clear session data.
extern void defrClearSession();

// The main reader function.
// The file should already be opened.  This requirement allows
// the reader to be used with stdin or a pipe.  The file name
// is only used for error messages.
extern int defrRead (FILE *file,
                    const char *fileName,
                    defiUserData userData,
                    int case_sensitive);

// Set/get the client-provided user data.  defi doesn't look at
// this data at all, it simply passes the opaque defiUserData pointer
// back to the application with each callback.  The client can
// change the data at any time, and it will take effect on the
// next callback.  The defi reader and writer maintain separate
// user data pointers.
extern void defrSetUserData (defiUserData);
extern defiUserData defrGetUserData ();

 
// Functions to call to register a callback function or get the function
//pointer after it has been registered.
//

// Register one function for all callbacks with the same signature 
extern void defrSetArrayNameCbk (defrStringCbkFnType);
extern void defrSetAssertionCbk (defrAssertionCbkFnType);
extern void defrSetAssertionsStartCbk (defrIntegerCbkFnType);
extern void defrSetAssertionsEndCbk (defrVoidCbkFnType);
extern void defrSetBlockageCbk (defrBlockageCbkFnType);
extern void defrSetBlockageStartCbk (defrIntegerCbkFnType);
extern void defrSetBlockageEndCbk (defrVoidCbkFnType);
extern void defrSetBusBitCbk (defrStringCbkFnType);
extern void defrSetCannotOccupyCbk (defrSiteCbkFnType);
extern void defrSetCanplaceCbk (defrSiteCbkFnType);
extern void defrSetCaseSensitiveCbk (defrIntegerCbkFnType);
extern void defrSetComponentCbk (defrComponentCbkFnType);
extern void defrSetComponentExtCbk (defrStringCbkFnType);
extern void defrSetComponentStartCbk (defrIntegerCbkFnType);
extern void defrSetComponentEndCbk (defrVoidCbkFnType);
extern void defrSetConstraintCbk (defrAssertionCbkFnType);
extern void defrSetConstraintsStartCbk (defrIntegerCbkFnType);
extern void defrSetConstraintsEndCbk (defrVoidCbkFnType);
extern void defrSetDefaultCapCbk (defrIntegerCbkFnType);
extern void defrSetDesignCbk (defrStringCbkFnType);
extern void defrSetDesignEndCbk (defrVoidCbkFnType);
extern void defrSetDieAreaCbk (defrBoxCbkFnType);
extern void defrSetDividerCbk (defrStringCbkFnType);
extern void defrSetExtensionCbk (defrStringCbkFnType);
extern void defrSetFillCbk (defrFillCbkFnType);
extern void defrSetFillStartCbk (defrIntegerCbkFnType);
extern void defrSetFillEndCbk (defrVoidCbkFnType);
extern void defrSetFPCCbk (defrFPCCbkFnType);
extern void defrSetFPCStartCbk (defrIntegerCbkFnType);
extern void defrSetFPCEndCbk (defrVoidCbkFnType);
extern void defrSetFloorPlanNameCbk (defrStringCbkFnType);
extern void defrSetGcellGridCbk (defrGcellGridCbkFnType);
extern void defrSetGroupNameCbk (defrStringCbkFnType);
extern void defrSetGroupMemberCbk (defrStringCbkFnType);
extern void defrSetComponentMaskShiftLayerCbk (defrComponentMaskShiftLayerCbkFnType);
extern void defrSetGroupCbk (defrGroupCbkFnType);
extern void defrSetGroupExtCbk (defrStringCbkFnType);
extern void defrSetGroupsStartCbk (defrIntegerCbkFnType);
extern void defrSetGroupsEndCbk (defrVoidCbkFnType);
extern void defrSetHistoryCbk (defrStringCbkFnType);
extern void defrSetIOTimingCbk (defrIOTimingCbkFnType);
extern void defrSetIOTimingsStartCbk (defrIntegerCbkFnType);
extern void defrSetIOTimingsEndCbk (defrVoidCbkFnType);
extern void defrSetIoTimingsExtCbk (defrStringCbkFnType);
extern void defrSetNetCbk (defrNetCbkFnType);
extern void defrSetNetNameCbk (defrStringCbkFnType);
extern void defrSetNetNonDefaultRuleCbk (defrStringCbkFnType);
extern void defrSetNetConnectionExtCbk (defrStringCbkFnType);
extern void defrSetNetExtCbk (defrStringCbkFnType);
extern void defrSetNetPartialPathCbk (defrNetCbkFnType);
extern void defrSetNetSubnetNameCbk (defrStringCbkFnType);
extern void defrSetNetStartCbk (defrIntegerCbkFnType);
extern void defrSetNetEndCbk (defrVoidCbkFnType);
extern void defrSetNonDefaultCbk (defrNonDefaultCbkFnType);
extern void defrSetNonDefaultStartCbk (defrIntegerCbkFnType);
extern void defrSetNonDefaultEndCbk (defrVoidCbkFnType);
extern void defrSetPartitionCbk (defrPartitionCbkFnType);
extern void defrSetPartitionsExtCbk (defrStringCbkFnType);
extern void defrSetPartitionsStartCbk (defrIntegerCbkFnType);
extern void defrSetPartitionsEndCbk (defrVoidCbkFnType);
extern void defrSetPathCbk (defrPathCbkFnType);
extern void defrSetPinCapCbk (defrPinCapCbkFnType);
extern void defrSetPinCbk (defrPinCbkFnType);
extern void defrSetPinExtCbk (defrStringCbkFnType);
extern void defrSetPinPropCbk (defrPinPropCbkFnType);
extern void defrSetPinPropStartCbk (defrIntegerCbkFnType);
extern void defrSetPinPropEndCbk (defrVoidCbkFnType);
extern void defrSetPropCbk (defrPropCbkFnType);
extern void defrSetPropDefEndCbk (defrVoidCbkFnType);
extern void defrSetPropDefStartCbk (defrVoidCbkFnType);
extern void defrSetRegionCbk (defrRegionCbkFnType);
extern void defrSetRegionStartCbk (defrIntegerCbkFnType);
extern void defrSetRegionEndCbk (defrVoidCbkFnType);
extern void defrSetRowCbk (defrRowCbkFnType);
extern void defrSetSNetCbk (defrNetCbkFnType);
extern void defrSetSNetStartCbk (defrIntegerCbkFnType);
extern void defrSetSNetEndCbk (defrVoidCbkFnType);
extern void defrSetSNetPartialPathCbk (defrNetCbkFnType);
extern void defrSetSNetWireCbk (defrNetCbkFnType);
extern void defrSetScanChainExtCbk (defrStringCbkFnType);
extern void defrSetScanchainCbk (defrScanchainCbkFnType);
extern void defrSetScanchainsStartCbk (defrIntegerCbkFnType);
extern void defrSetScanchainsEndCbk (defrVoidCbkFnType);
extern void defrSetSiteCbk (defrSiteCbkFnType);
extern void defrSetSlotCbk (defrSlotCbkFnType);
extern void defrSetSlotStartCbk (defrIntegerCbkFnType);
extern void defrSetSlotEndCbk (defrVoidCbkFnType);
extern void defrSetStartPinsCbk (defrIntegerCbkFnType);
extern void defrSetStylesCbk (defrStylesCbkFnType);
extern void defrSetStylesStartCbk (defrIntegerCbkFnType);
extern void defrSetStylesEndCbk (defrVoidCbkFnType);
extern void defrSetPinEndCbk (defrVoidCbkFnType);
extern void defrSetTechnologyCbk (defrStringCbkFnType);
extern void defrSetTimingDisableCbk (defrTimingDisableCbkFnType);
extern void defrSetTimingDisablesStartCbk (defrIntegerCbkFnType);
extern void defrSetTimingDisablesEndCbk (defrVoidCbkFnType);
extern void defrSetTrackCbk (defrTrackCbkFnType);
extern void defrSetUnitsCbk (defrDoubleCbkFnType);
extern void defrSetVersionCbk (defrDoubleCbkFnType);
extern void defrSetVersionStrCbk (defrStringCbkFnType);
extern void defrSetViaCbk (defrViaCbkFnType);
extern void defrSetViaExtCbk (defrStringCbkFnType);
extern void defrSetViaStartCbk (defrIntegerCbkFnType);
extern void defrSetViaEndCbk (defrVoidCbkFnType);

// NEW CALLBACK - For each new callback you create, you must
// create a routine that allows the user to set it.  Add the
// setting routines here. 

//Set all of the callbacks that have not yet been set to the following
//function.  This is especially useful if you want to check to see
//if you forgot anything.
extern void defrUnsetCallbacks ();

// Functions to call to unregister a callback function.
extern void defrUnsetArrayNameCbk ();
extern void defrUnsetAssertionCbk ();
extern void defrUnsetAssertionsStartCbk ();
extern void defrUnsetAssertionsEndCbk ();
extern void defrUnsetBlockageCbk ();
extern void defrUnsetBlockageStartCbk ();
extern void defrUnsetBlockageEndCbk ();
extern void defrUnsetBusBitCbk ();
extern void defrUnsetCannotOccupyCbk ();
extern void defrUnsetCanplaceCbk ();
extern void defrUnsetCaseSensitiveCbk ();
extern void defrUnsetComponentCbk ();
extern void defrUnsetComponentExtCbk ();
extern void defrUnsetComponentStartCbk ();
extern void defrUnsetComponentEndCbk ();
extern void defrUnsetConstraintCbk ();
extern void defrUnsetConstraintsStartCbk ();
extern void defrUnsetConstraintsEndCbk ();
extern void defrUnsetDefaultCapCbk ();
extern void defrUnsetDesignCbk ();
extern void defrUnsetDesignEndCbk ();
extern void defrUnsetDieAreaCbk ();
extern void defrUnsetDividerCbk ();
extern void defrUnsetExtensionCbk ();
extern void defrUnsetFillCbk ();
extern void defrUnsetFillStartCbk ();
extern void defrUnsetFillEndCbk ();
extern void defrUnsetFPCCbk ();
extern void defrUnsetFPCStartCbk ();
extern void defrUnsetFPCEndCbk ();
extern void defrUnsetFloorPlanNameCbk ();
extern void defrUnsetGcellGridCbk ();
extern void defrUnsetGroupCbk ();
extern void defrUnsetGroupExtCbk ();
extern void defrUnsetGroupMemberCbk ();
extern void defrUnsetComponentMaskShiftLayerCbk ();
extern void defrUnsetGroupNameCbk ();
extern void defrUnsetGroupsStartCbk ();
extern void defrUnsetGroupsEndCbk ();
extern void defrUnsetHistoryCbk ();
extern void defrUnsetIOTimingCbk ();
extern void defrUnsetIOTimingsStartCbk ();
extern void defrUnsetIOTimingsEndCbk ();
extern void defrUnsetIOTimingsExtCbk ();
extern void defrUnsetNetCbk ();
extern void defrUnsetNetNameCbk ();
extern void defrUnsetNetNonDefaultRuleCbk ();
extern void defrUnsetNetConnectionExtCbk ();
extern void defrUnsetNetExtCbk ();
extern void defrUnsetNetPartialPathCbk ();
extern void defrUnsetNetSubnetNameCbk ();
extern void defrUnsetNetStartCbk ();
extern void defrUnsetNetEndCbk ();
extern void defrUnsetNonDefaultCbk ();
extern void defrUnsetNonDefaultStartCbk ();
extern void defrUnsetNonDefaultEndCbk ();
extern void defrUnsetPartitionCbk ();
extern void defrUnsetPartitionsExtCbk ();
extern void defrUnsetPartitionsStartCbk ();
extern void defrUnsetPartitionsEndCbk ();
extern void defrUnsetPathCbk ();
extern void defrUnsetPinCapCbk ();
extern void defrUnsetPinCbk ();
extern void defrUnsetPinEndCbk ();
extern void defrUnsetPinExtCbk ();
extern void defrUnsetPinPropCbk ();
extern void defrUnsetPinPropStartCbk ();
extern void defrUnsetPinPropEndCbk ();
extern void defrUnsetPropCbk ();
extern void defrUnsetPropDefEndCbk ();
extern void defrUnsetPropDefStartCbk ();
extern void defrUnsetRegionCbk ();
extern void defrUnsetRegionStartCbk ();
extern void defrUnsetRegionEndCbk ();
extern void defrUnsetRowCbk ();
extern void defrUnsetScanChainExtCbk ();
extern void defrUnsetScanchainCbk ();
extern void defrUnsetScanchainsStartCbk ();
extern void defrUnsetScanchainsEndCbk ();
extern void defrUnsetSiteCbk ();
extern void defrUnsetSlotCbk ();
extern void defrUnsetSlotStartCbk ();
extern void defrUnsetSlotEndCbk ();
extern void defrUnsetSNetWireCbk ();
extern void defrUnsetSNetCbk ();
extern void defrUnsetSNetStartCbk ();
extern void defrUnsetSNetEndCbk ();
extern void defrUnsetSNetPartialPathCbk ();
extern void defrUnsetStartPinsCbk ();
extern void defrUnsetStylesCbk ();
extern void defrUnsetStylesStartCbk ();
extern void defrUnsetStylesEndCbk ();
extern void defrUnsetTechnologyCbk ();
extern void defrUnsetTimingDisableCbk ();
extern void defrUnsetTimingDisablesStartCbk ();
extern void defrUnsetTimingDisablesEndCbk ();
extern void defrUnsetTrackCbk ();
extern void defrUnsetUnitsCbk ();
extern void defrUnsetVersionCbk ();
extern void defrUnsetVersionStrCbk ();
extern void defrUnsetViaCbk ();
extern void defrUnsetViaExtCbk ();
extern void defrUnsetViaStartCbk ();
extern void defrUnsetViaEndCbk ();

// Routine to set all unused callbacks. This is useful for checking
//to see if you missed something. 
extern void defrSetUnusedCallbacks (defrVoidCbkFnType func);

// Return the current line number in the input file. 
extern int defrLineNumber ();
extern long long defrLongLineNumber ();

// Routine to set the message logging routine for errors 
#ifndef DEFI_LOG_FUNCTION
    typedef void (*DEFI_LOG_FUNCTION) (const char*);
#endif
extern void defrSetLogFunction(DEFI_LOG_FUNCTION);

// Routine to set the message logging routine for warnings 
#ifndef DEFI_WARNING_LOG_FUNCTION
    typedef void (*DEFI_WARNING_LOG_FUNCTION) (const char*);
#endif
extern void defrSetWarningLogFunction(DEFI_WARNING_LOG_FUNCTION);

// Routine to set the message logging routine for errors 
// Used in re-enterable environment.
#ifndef DEFI_LOG_FUNCTION
    typedef void (*DEFI_CONTEXT_LOG_FUNCTION) (defiUserData userData, const char*);
#endif
extern void defrSetContextLogFunction(DEFI_CONTEXT_LOG_FUNCTION);

// Routine to set the message logging routine for warnings 
// Used in re-enterable environment.
#ifndef DEFI_WARNING_LOG_FUNCTION
    typedef void (*DEFI_CONTEXT_WARNING_LOG_FUNCTION) (defiUserData userData, const char*);
#endif
extern void defrSetContextWarningLogFunction(DEFI_CONTEXT_WARNING_LOG_FUNCTION);

// Routine to set the user defined malloc routine 
typedef void* (*DEFI_MALLOC_FUNCTION) (size_t);
extern void defrSetMallocFunction(DEFI_MALLOC_FUNCTION);

// Routine to set the user defined realloc routine 
typedef void* (*DEFI_REALLOC_FUNCTION) (void*, size_t);
extern void defrSetReallocFunction(DEFI_REALLOC_FUNCTION);

// Routine to set the user defined free routine 
typedef void (*DEFI_FREE_FUNCTION) (void *);
extern void defrSetFreeFunction(DEFI_FREE_FUNCTION);

// Routine to set the line number of the file that is parsing routine (takes int)
typedef void (*DEFI_LINE_NUMBER_FUNCTION)  (int);
extern void defrSetLineNumberFunction(DEFI_LINE_NUMBER_FUNCTION);

// Routine to set the line number of the file that is parsing routine (takes long long)
typedef void (*DEFI_LONG_LINE_NUMBER_FUNCTION)  (long long);
extern void defrSetLongLineNumberFunction(DEFI_LONG_LINE_NUMBER_FUNCTION);

// Routine to set the line number of the file that is parsing routine (takes int)
// Used in re-enterable environment.
typedef void (*DEFI_CONTEXT_LINE_NUMBER_FUNCTION)  (defiUserData userData, int);
extern void defrSetContextLineNumberFunction(DEFI_CONTEXT_LINE_NUMBER_FUNCTION);

// Routine to set the line number of the file that is parsing routine (takes long long
// Used in re-enterable environment.
typedef void (*DEFI_CONTEXT_LONG_LINE_NUMBER_FUNCTION)  (defiUserData userData, long long);
extern void defrSetContextLongLineNumberFunction(DEFI_CONTEXT_LONG_LINE_NUMBER_FUNCTION);

// Set the number of lines before calling the line function callback routine 
// Default is 10000 
extern void defrSetDeltaNumberLines  (int);

// Routine to set the read function 
typedef size_t (*DEFI_READ_FUNCTION)  (FILE*, char*, size_t);
extern void defrSetReadFunction(DEFI_READ_FUNCTION);
extern void defrUnsetReadFunction ();

// Routine to set the defrWarning.log to open as append instead for write 
// New in 5.7 
extern void defrSetOpenLogFileAppend ();
extern void defrUnsetOpenLogFileAppend ();

// Routine to set the magic comment found routine 
typedef void (*DEFI_MAGIC_COMMENT_FOUND_FUNCTION) ();
extern void defrSetMagicCommentFoundFunction(DEFI_MAGIC_COMMENT_FOUND_FUNCTION);

// Routine to set the magic comment string 
extern void defrSetMagicCommentString(char *);

// Routine to disable string property value process, default it will process 
// the value string 
extern void defrDisablePropStrProcess ();

// Testing purposes only 
extern void defrSetNLines(long long n);

// Routine to set the max number of warnings for a perticular section 

extern void defrSetAssertionWarnings(int warn);
extern void defrSetBlockageWarnings(int warn);
extern void defrSetCaseSensitiveWarnings(int warn);
extern void defrSetComponentWarnings(int warn);
extern void defrSetConstraintWarnings(int warn);
extern void defrSetDefaultCapWarnings(int warn);
extern void defrSetGcellGridWarnings(int warn);
extern void defrSetIOTimingWarnings(int warn);
extern void defrSetNetWarnings(int warn);
extern void defrSetNonDefaultWarnings(int warn);
extern void defrSetPinExtWarnings(int warn);
extern void defrSetPinWarnings(int warn);
extern void defrSetRegionWarnings(int warn);
extern void defrSetRowWarnings(int warn);
extern void defrSetScanchainWarnings(int warn);
extern void defrSetSNetWarnings(int warn);
extern void defrSetStylesWarnings(int warn);
extern void defrSetTrackWarnings(int warn);
extern void defrSetUnitsWarnings(int warn);
extern void defrSetVersionWarnings(int warn);
extern void defrSetViaWarnings(int warn);

// Handling output messages 
extern void defrDisableParserMsgs(int nMsg, int* msgs);
extern void defrEnableParserMsgs(int nMsg, int* msgs);
extern void defrEnableAllMsgs();
extern void defrSetTotalMsgLimit(int totNumMsgs);
extern void defrSetLimitPerMsg(int msgId, int numMsg);

// Return codes for the user callbacks.
//The user should return one of these values. 
#define PARSE_OK 0      // continue parsing 
#define STOP_PARSE 1    // stop parsing with no error message 
#define PARSE_ERROR 2   // stop parsing, print an error message 

// Add this alias to the list for the parser     
extern void defrAddAlias (const char* key, 
                          const char* value,
                          int marked);

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
