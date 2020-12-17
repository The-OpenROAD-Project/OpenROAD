/* ************************************************************************** */
/* ************************************************************************** */
/* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!               */
/* ************************************************************************** */
/* ************************************************************************** */
/* Copyright 2012 - 2013, Cadence Design Systems                              */
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
/*  $Author: dell $                                                                  */
/*  $Revision: #1 $                                                                */
/*  $Date: 2017/06/06 $                                                                    */
/*  $State:  $                                                                */
/* ************************************************************************** */
/* ************************************************************************** */


#ifndef CLEFWWRITERCALLS_H
#define CLEFWWRITERCALLS_H

#include <stdio.h>
#include "lefiTypedefs.h"

/*
 * The main writer function.
 * The file should already be opened.  This requirement allows
 * the writer to be used with stdin or a pipe.  The file name
 * is only used for error messages.  The includeSearchPath is
 * a colon-delimited list of directories in which to find
 * include files.
 */
EXTERN int lefwWrite (FILE * file, const char * fileName, lefiUserData  userData);

/*
 * Set all of the callbacks that have not yet been set to a function
 * that will add up how many times a given lef data type was ignored
 * (ie no callback was done).  The statistics can later be printed out.
 */
EXTERN void lefwSetRegisterUnusedCallbacks ();
EXTERN void lefwPrintUnusedCallbacks (FILE*  f);

/*
 * Set/get the client-provided user data.  lefi doesn't look at
 * this data at all, it simply passes the opaque lefiUserData pointer
 * back to the application with each callback.  The client can
 * change the data at any time, and it will take effect on the
 * next callback.  The lefi writer and writer maintain separate
 * user data pointers.
 */
EXTERN void lefwSetUserData (lefiUserData  p0);
EXTERN lefiUserData lefwGetUserData ();

/*
 * An enum describing all of the types of writer callbacks.
 */
typedef enum {
  lefwUnspecifiedCbkType = 0,
  lefwVersionCbkType = 1,
  lefwCaseSensitiveCbkType = 2,
  lefwNoWireExtensionCbkType = 3,
  lefwBusBitCharsCbkType = 4,
  lefwDividerCharCbkType = 5,
  lefwManufacturingGridCbkType = 6,
  lefwUseMinSpacingCbkType = 7,
  lefwClearanceMeasureCbkType = 8,
  lefwUnitsCbkType = 9,
  lefwAntennaInputGateAreaCbkType = 10,
  lefwAntennaInOutDiffAreaCbkType = 11,
  lefwAntennaOutputDiffAreaCbkType = 12,
  lefwPropDefCbkType = 13,
  lefwLayerCbkType = 14,
  lefwViaCbkType = 15,
  lefwViaRuleCbkType = 16,
  lefwNonDefaultCbkType = 17,
  lefwCrossTalkCbkType = 18,
  lefwNoiseTableCbkType = 19,
  lefwCorrectionTableCbkType = 20,
  lefwSpacingCbkType = 21,
  lefwMinFeatureCbkType = 22,
  lefwDielectricCbkType = 23,
  lefwIRDropCbkType = 24,
  lefwSiteCbkType = 25,
  lefwArrayCbkType = 26,
  lefwMacroCbkType = 27,
  lefwAntennaCbkType = 28,
  lefwExtCbkType = 29,
  lefwEndLibCbkType = 30

  /* NEW CALLBACKS - each callback has its own type.  For each callback
   * that you add, you must add an item to this enum. */

} lefwCallbackType_e;

/* Declarations of function signatures for each type of callback.
 * These declarations are type-safe when compiling with ANSI C
 * or C++; you will only be able to register a function pointer
 * with the correct signature for a given type of callback.
 *
 * Each callback function is expected to return 0 if successful.
 * A non-zero return code will cause the writer to abort.
 *
 * The lefwDesignStart and lefwDesignEnd callback is only called once.
 * Other callbacks may be called multiple times, each time with a different
 * set of data.
 *
 * For each callback, the Lef API will make the callback to the
 * function supplied by the client, which should either make a copy
 * of the Lef object, or store the data in the client's own data structures.
 * The Lef API will delete or reuse each object after making the callback,
 * so the client should not keep a pointer to it.
 *
 * All callbacks pass the user data pointer provided in lefwRead()
 * or lefwSetUserData() back to the client; this can be used by the
 * client to obtain access to the rest of the client's data structures.
 *
 * The user data pointer is obtained using lefwGetUserData() immediately
 * prior to making each callback, so the client is free to change the
 * user data on the fly if necessary.
 *
 * Callbacks with the same signature are passed a callback type
 * parameter, which allows an application to write a single callback
 * function, register that function for multiple callbacks, then
 * switch based on the callback type to handle the appropriate type of
 * data.
 */

/* A declaration of the signature of all callbacks that return nothing.       */
typedef int (*lefwVoidCbkFnType) ( lefwCallbackType_e, lefiUserData );

 /* NEW CALLBACK - If your callback returns a pointer to a new class then
  * you must add a type function here. */

/* Functions to call to register a callback function.
 */
EXTERN void lefwSetVersionCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetCaseSensitiveCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetNoWireExtensionCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetBusBitCharsCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetDividerCharCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetManufacturingGridCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetUseMinSpacingCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetClearanceMeasureCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetUnitsCbk (lefwVoidCbkFnType p0);
EXTERN void lefwAntennaInputGateAreaCbk (lefwVoidCbkFnType p0);
EXTERN void lefwAntennaInOutDiffAreaCbk (lefwVoidCbkFnType p0);
EXTERN void lefwAntennaOutputDiffAreaCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetPropDefCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetLayerCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetViaCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetViaRuleCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetNonDefaultCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetCrossTalkCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetNoiseTableCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetCorrectionTableCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetSpacingCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetMinFeatureCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetDielectricCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetIRDropCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetSiteCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetArrayCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetMacroCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetAntennaCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetExtCbk (lefwVoidCbkFnType p0);
EXTERN void lefwSetEndLibCbk (lefwVoidCbkFnType p0);

/* NEW CALLBACK - each callback must have a function to allow the user
 * to set it.  Add the function here. */

/*
 * Set all of the callbacks that have not yet been set to the following
 * function.  This is especially useful if you want to check to see
 * if you forgot anything.
 */
EXTERN void lefwSetUnusedCallbacks (lefwVoidCbkFnType  func);

/* Routine to set the message logging routine for errors                      */
typedef void (*LEFI_LOG_FUNCTION)(const char*);
EXTERN void lefwSetLogFunction (LEFI_LOG_FUNCTION  p0);

/* Routine to set the message logging routine for warnings                    */
typedef void (*LEFI_WARNING_LOG_FUNCTION)(const char*);
EXTERN void lefwSetWarningLogFunction (LEFI_WARNING_LOG_FUNCTION  p0);

#endif
