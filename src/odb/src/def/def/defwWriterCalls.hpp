// *****************************************************************************
// *****************************************************************************
// Copyright 2013-2014, Cadence Design Systems
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

#ifndef DEFI_WRITER_H
#define DEFI_WRITER_H

#include <stdarg.h>
#include <stdio.h>
 
#include "defiKRDefs.hpp"
#include "defiDefs.hpp"
#include "defiUser.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

/*
 * The main writer function.
 * The file should already be opened.  This requirement allows
 * the writer to be used with stdin or a pipe.  The file name
 * is only used for error messages.  The includeSearchPath is
 * a colon-delimited list of directories in which to find
 * include files.
 */
extern int defwWrite (  FILE *file,
                        const char *fileName,
                        defiUserData userData );

/*
 * Set all of the callbacks that have not yet been set to a function
 * that will add up how many times a given def data type was ignored
 * (ie no callback was done).  The statistics can later be printed out.
 */
extern void defwSetRegisterUnusedCallbacks (void);
extern void defwPrintUnusedCallbacks (FILE* log);

/*
 * Set/get the client-provided user data.  defi doesn't look at
 * this data at all, it simply passes the opaque defiUserData pointer
 * back to the application with each callback.  The client can
 * change the data at any time, and it will take effect on the
 * next callback.  The defi writer and writer maintain separate
 * user data pointers.
 */
extern void defwSetUserData ( defiUserData );
extern defiUserData defwGetUserData ( void );
 
/*
 * An enum describing all of the types of writer callbacks.
 */
typedef enum {
  defwUnspecifiedCbkType = 0,
  defwVersionCbkType,
  defwCaseSensitiveCbkType,
  defwBusBitCbkType,
  defwDividerCbkType,
  defwDesignCbkType,
  defwTechCbkType,
  defwArrayCbkType,
  defwFloorPlanCbkType,
  defwUnitsCbkType,
  defwHistoryCbkType,
  defwPropDefCbkType,
  defwDieAreaCbkType,
  defwRowCbkType,
  defwTrackCbkType,
  defwGcellGridCbkType,
  defwDefaultCapCbkType,
  defwCanplaceCbkType,
  defwCannotOccupyCbkType,
  defwViaCbkType,
  defwRegionCbkType,
  defwComponentCbkType,
  defwPinCbkType,
  defwPinPropCbkType,
  defwSNetCbkType,
  defwNetCbkType,
  defwIOTimingCbkType,
  defwScanchainCbkType,
  defwConstraintCbkType,
  defwAssertionCbkType,    // pre 5.2
  defwGroupCbkType,
  defwBlockageCbkType,     // 5.4
  defwExtCbkType,
  defwDesignEndCbkType

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
extern void defwSetArrayCbk (defwVoidCbkFnType);
extern void defwSetAssertionCbk (defwVoidCbkFnType);
extern void defwSetBlockageCbk (defwVoidCbkFnType);
extern void defwSetBusBitCbk (defwVoidCbkFnType);
extern void defwSetCannotOccupyCbk (defwVoidCbkFnType);
extern void defwSetCanplaceCbk (defwVoidCbkFnType);
extern void defwSetCaseSensitiveCbk (defwVoidCbkFnType);
extern void defwSetComponentCbk (defwVoidCbkFnType);
extern void defwSetConstraintCbk (defwVoidCbkFnType);
extern void defwSetDefaultCapCbk (defwVoidCbkFnType);
extern void defwSetDesignCbk (defwVoidCbkFnType);
extern void defwSetDesignEndCbk (defwVoidCbkFnType);
extern void defwSetDieAreaCbk (defwVoidCbkFnType);
extern void defwSetDividerCbk (defwVoidCbkFnType);
extern void defwSetExtCbk (defwVoidCbkFnType);
extern void defwSetFloorPlanCbk (defwVoidCbkFnType);
extern void defwSetGcellGridCbk (defwVoidCbkFnType);
extern void defwSetGroupCbk (defwVoidCbkFnType);
extern void defwSetHistoryCbk (defwVoidCbkFnType);
extern void defwSetIOTimingCbk (defwVoidCbkFnType);
extern void defwSetNetCbk (defwVoidCbkFnType);
extern void defwSetPinCbk (defwVoidCbkFnType);
extern void defwSetPinPropCbk (defwVoidCbkFnType);
extern void defwSetPropDefCbk (defwVoidCbkFnType);
extern void defwSetRegionCbk (defwVoidCbkFnType);
extern void defwSetRowCbk (defwVoidCbkFnType);
extern void defwSetSNetCbk (defwVoidCbkFnType);
extern void defwSetScanchainCbk (defwVoidCbkFnType);
extern void defwSetTechnologyCbk (defwVoidCbkFnType);
extern void defwSetTrackCbk (defwVoidCbkFnType);
extern void defwSetUnitsCbk (defwVoidCbkFnType);
extern void defwSetVersionCbk (defwVoidCbkFnType);
extern void defwSetViaCbk (defwVoidCbkFnType);

/* NEW CALLBACK - each callback must have a function to allow the user
 * to set it.  Add the function here. */


/*
 * Set all of the callbacks that have not yet been set to the following
 * function.  This is especially useful if you want to check to see
 * if you forgot anything.
 */
extern void defwSetUnusedCallbacks (defwVoidCbkFnType func);

/* Routine to set the message logging routine for errors */
#ifndef DEFI_LOG_FUNCTION
    typedef void (*DEFI_LOG_FUNCTION) (const char*);
#endif

extern void defwSetLogFunction ( DEFI_LOG_FUNCTION );

/* Routine to set the message logging routine for warnings */
#ifndef DEFI_WARNING_LOG_FUNCTION
    typedef void (*DEFI_WARNING_LOG_FUNCTION)(const char*);
#endif

extern void defwSetWarningLogFunction( DEFI_WARNING_LOG_FUNCTION );

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
