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
/*  $Author: dell $                                                       */
/*  $Revision: #1 $                                                           */
/*  $Date: 2017/06/06 $                                                       */
/*  $State:  $                                                                */
/* ************************************************************************** */
/* ************************************************************************** */


#ifndef CDEFWWRITERCALLS_H
#define CDEFWWRITERCALLS_H

#include <stdio.h>
#include "defiTypedefs.h"

/*
 * The main writer function.
 * The file should already be opened.  This requirement allows
 * the writer to be used with stdin or a pipe.  The file name
 * is only used for error messages.  The includeSearchPath is
 * a colon-delimited list of directories in which to find
 * include files.
 */
EXTERN int defwWrite (FILE * file, const char * fileName, defiUserData  userData);

/*
 * Set all of the callbacks that have not yet been set to a function
 * that will add up how many times a given def data type was ignored
 * (ie no callback was done).  The statistics can later be printed out.
 */
EXTERN void defwSetRegisterUnusedCallbacks ();
EXTERN void defwPrintUnusedCallbacks (FILE*  log);

/*
 * Set/get the client-provided user data.  defi doesn't look at
 * this data at all, it simply passes the opaque defiUserData pointer
 * back to the application with each callback.  The client can
 * change the data at any time, and it will take effect on the
 * next callback.  The defi writer and writer maintain separate
 * user data pointers.
 */
EXTERN void defwSetUserData (defiUserData  p0);
EXTERN defiUserData defwGetUserData ();

/*
 * An enum describing all of the types of writer callbacks.
 */
typedef enum {
  defwUnspecifiedCbkType = 0,
  defwVersionCbkType = 1,
  defwCaseSensitiveCbkType = 2,
  defwBusBitCbkType = 3,
  defwDividerCbkType = 4,
  defwDesignCbkType = 5,
  defwTechCbkType = 6,
  defwArrayCbkType = 7,
  defwFloorPlanCbkType = 8,
  defwUnitsCbkType = 9,
  defwHistoryCbkType = 10,
  defwPropDefCbkType = 11,
  defwDieAreaCbkType = 12,
  defwRowCbkType = 13,
  defwTrackCbkType = 14,
  defwGcellGridCbkType = 15,
  defwDefaultCapCbkType = 16,
  defwCanplaceCbkType = 17,
  defwCannotOccupyCbkType = 18,
  defwViaCbkType = 19,
  defwRegionCbkType = 20,
  defwComponentCbkType = 21,
  defwPinCbkType = 22,
  defwPinPropCbkType = 23,
  defwSNetCbkType = 24,
  defwNetCbkType = 25,
  defwIOTimingCbkType = 26,
  defwScanchainCbkType = 27,
  defwConstraintCbkType = 28,
  defwAssertionCbkType = 29,
  defwGroupCbkType = 30,
  defwBlockageCbkType = 31,
  defwExtCbkType = 32,
  defwDesignEndCbkType = 33

  /* NEW CALLBACKS - each callback has its own type.  For each callback
   * that you add, you must add an item to this enum. */

} defwCallbackType_e;

/* Declarations of function signatures for each type of callback.
 * These declarations are type-safe when compiling with ANSI C
 * or C++; you will only be able to register a function pointer
 * with the correct signature for a given type of callback.
 *
 * Each callback function is expected to return 0 if successful.
 * A non-zero return code will cause the writer to abort.
 *
 * The defwDesignStart and defwDesignEnd callback is only called once.
 * Other callbacks may be called multiple times, each time with a different
 * set of data.
 *
 * For each callback, the Def API will make the callback to the
 * function supplied by the client, which should either make a copy
 * of the Def object, or store the data in the client's own data structures.
 * The Def API will delete or reuse each object after making the callback,
 * so the client should not keep a pointer to it.
 *
 * All callbacks pass the user data pointer provided in defwRead()
 * or defwSetUserData() back to the client; this can be used by the
 * client to obtain access to the rest of the client's data structures.
 *
 * The user data pointer is obtained using defwGetUserData() immediately
 * prior to making each callback, so the client is free to change the
 * user data on the fly if necessary.
 *
 * Callbacks with the same signature are passed a callback type
 * parameter, which allows an application to write a single callback
 * function, register that function for multiple callbacks, then
 * switch based on the callback type to handle the appropriate type of
 * data.
 */

/* A declaration of the signature of all callbacks that return nothing. */
typedef int (*defwVoidCbkFnType) ( defwCallbackType_e, defiUserData );

/* Functions to call to register a callback function.
 */
EXTERN void defwSetArrayCbk (defwVoidCbkFnType p0);
EXTERN void defwSetAssertionCbk (defwVoidCbkFnType p0);
EXTERN void defwSetBlockageCbk (defwVoidCbkFnType p0);
EXTERN void defwSetBusBitCbk (defwVoidCbkFnType p0);
EXTERN void defwSetCannotOccupyCbk (defwVoidCbkFnType p0);
EXTERN void defwSetCanplaceCbk (defwVoidCbkFnType p0);
EXTERN void defwSetCaseSensitiveCbk (defwVoidCbkFnType p0);
EXTERN void defwSetComponentCbk (defwVoidCbkFnType p0);
EXTERN void defwSetConstraintCbk (defwVoidCbkFnType p0);
EXTERN void defwSetDefaultCapCbk (defwVoidCbkFnType p0);
EXTERN void defwSetDesignCbk (defwVoidCbkFnType p0);
EXTERN void defwSetDesignEndCbk (defwVoidCbkFnType p0);
EXTERN void defwSetDieAreaCbk (defwVoidCbkFnType p0);
EXTERN void defwSetDividerCbk (defwVoidCbkFnType p0);
EXTERN void defwSetExtCbk (defwVoidCbkFnType p0);
EXTERN void defwSetFloorPlanCbk (defwVoidCbkFnType p0);
EXTERN void defwSetGcellGridCbk (defwVoidCbkFnType p0);
EXTERN void defwSetGroupCbk (defwVoidCbkFnType p0);
EXTERN void defwSetHistoryCbk (defwVoidCbkFnType p0);
EXTERN void defwSetIOTimingCbk (defwVoidCbkFnType p0);
EXTERN void defwSetNetCbk (defwVoidCbkFnType p0);
EXTERN void defwSetPinCbk (defwVoidCbkFnType p0);
EXTERN void defwSetPinPropCbk (defwVoidCbkFnType p0);
EXTERN void defwSetPropDefCbk (defwVoidCbkFnType p0);
EXTERN void defwSetRegionCbk (defwVoidCbkFnType p0);
EXTERN void defwSetRowCbk (defwVoidCbkFnType p0);
EXTERN void defwSetSNetCbk (defwVoidCbkFnType p0);
EXTERN void defwSetScanchainCbk (defwVoidCbkFnType p0);
EXTERN void defwSetTechnologyCbk (defwVoidCbkFnType p0);
EXTERN void defwSetTrackCbk (defwVoidCbkFnType p0);
EXTERN void defwSetUnitsCbk (defwVoidCbkFnType p0);
EXTERN void defwSetVersionCbk (defwVoidCbkFnType p0);
EXTERN void defwSetViaCbk (defwVoidCbkFnType p0);

/* NEW CALLBACK - each callback must have a function to allow the user
 * to set it.  Add the function here. */

/*
 * Set all of the callbacks that have not yet been set to the following
 * function.  This is especially useful if you want to check to see
 * if you forgot anything.
 */
EXTERN void defwSetUnusedCallbacks (defwVoidCbkFnType  func);

/* Routine to set the message logging routine for errors */
    typedef void (*DEFI_LOG_FUNCTION) (const char*);

EXTERN void defwSetLogFunction (DEFI_LOG_FUNCTION  p0);

/* Routine to set the message logging routine for warnings */
#ifndef DEFI_WARNING_LOG_FUNCTION
    typedef void (*DEFI_WARNING_LOG_FUNCTION)(const char*);
#endif

EXTERN void defwSetWarningLogFunction (DEFI_WARNING_LOG_FUNCTION  p0);

#endif
