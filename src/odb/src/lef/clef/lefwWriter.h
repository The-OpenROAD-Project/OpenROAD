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


#ifndef CLEFWWRITER_H
#define CLEFWWRITER_H

#include <stdio.h>
#include "lefiTypedefs.h"

/* Return codes for writing functions:                                        */
#define LEFW_OK               0
#define LEFW_UNINITIALIZED    1
#define LEFW_BAD_ORDER        2
#define LEFW_BAD_DATA         3
#define LEFW_ALREADY_DEFINED  4
#define LEFW_WRONG_VERSION    5
#define LEFW_MIX_VERSION_DATA 6
#define LEFW_OBSOLETE         7

/* orient                                                                     */
/*   0 = N                                                                    */
/*   1 = W                                                                    */
/*   2 = S                                                                    */
/*   3 = E                                                                    */
/*   4 = FN                                                                   */
/*   5 = FW                                                                   */
/*   6 = FS                                                                   */
/*   7 = FE                                                                   */

/* The LEF writer initialization.  Must be called first.                      */
/* Either this routine or lefwInitCbk should be call only,                    */
/* Can't call both routines.                                                  */
/* This routine must be called only once.                                     */
/* Returns 0 if successful.                                                   */
EXTERN int lefwInit (FILE*  f);

/* The LEF writer initialization.  Must be called first.                      */
/* Either this routine or lefwInit should be call only,                       */
/* Can't call both routines.                                                  */
/* This routine must be called only once.                                     */
/* Returns 0 if successful.                                                   */
EXTERN int lefwInitCbk (FILE*  f);

/* This routine will set the writer to write out an encrypted                 */
/* lef file.                                                                  */
/* This routine must be called only once and has to be called after           */
/* lefwInit or lefwInitCbk                                                    */
/* Need to call lefwCloseEncrypt to do some cleaning if this routine          */
/* has called                                                                 */
EXTERN int lefwEncrypt ();

/* This routine needs to be called if lefwEncrypt has called.                 */
/* It should be called before the fclose.                                     */
/* It does some house cleaning.                                               */
EXTERN int lefwCloseEncrypt ();

/* This routine will write a blank line                                       */

EXTERN int lefwNewLine ();

/* This routine is called after lefwInit.                                     */
/* This routine can be called only once.                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwVersion (int  vers1, int  vers2);

/* This routine is called after lefwInit.                                     */
/* This routine can be called only once.                                      */
/* Returns 0 if successful.                                                   */
/* The caseSensitive can be ON or OFF.                                        */
EXTERN int lefwCaseSensitive (const char*  caseSensitive);

/* This routine is called after lefwInit.                                     */
/* This routine can be called only once.                                      */
/* Returns 0 if successful.                                                   */
/* The noWireExtensionAtPin can be ON or OFF.                                 */
EXTERN int lefwNoWireExtensionAtPin (const char*  noWireExt);

/* This routine is called after lefwInit.                                     */
/* This routine can be called only once.                                      */
/* Returns 0 if successful.                                                   */
/*     already have one                                                       */
EXTERN int lefwMinfeature (double  minFeatureX, double  minFeatureY);

/* This routine is called after lefwInit.                                     */
/* This routine can be called only once.                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwDielectric (double  dielectric);

/* This routine is called after lefwInit.                                     */
/* This routine can be called only once.                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwBusBitChars (const char*  busBitChars);

/* This routine is called after lefwInit.                                     */
/* This routine can be called only once.                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwDividerChar (const char*  dividerChar);

/* This routine is called after lefwInit.                                     */
/* This routine can be called only once.                                      */
/* This is a 5.4 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwManufacturingGrid (double  grid);

/* This routine is called after lefwInit.                                     */
/* This routine can be called only once.                                      */
/* This is a 5.8 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwFixedMask ();

/* This routine is called after lefwInit.                                     */
/* This routine can be called multiple times.                                 */
/* This is a 5.4 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwUseMinSpacing (const char*  type, const char*  onOff);

/* This routine is called after lefwInit.                                     */
/* This routine can be called only once.                                      */
/* This is a 5.4 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwClearanceMeasure (const char*  type);

/* This routine is called after lefwInit.                                     */
/* This routine can be called only once.                                      */
/* This is a 5.4 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwAntennaInputGateArea (double  inputGateArea);

/* This routine is called after lefwInit.                                     */
/* This routine can be called only once.                                      */
/* This is a 5.4 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwAntennaInOutDiffArea (double  inOutDiffArea);

/* This routine is called after lefwInit.                                     */
/* This routine can be called only once.                                      */
/* This is a 5.4 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwAntennaOutputDiffArea (double  outputDiffArea);

/* This routine is called after lefwInit.                                     */
/* This routine can be called only once.                                      */
/* Returns 0 if successful.                                                   */
/* This section of routines is optional.                                      */
/* The routine starts the units section. All of the units must follow.        */
EXTERN int lefwStartUnits ();

/* This routine is called once for each unit.  The call must                  */
/* be preceeded by a call to lefwStartUnits and must be                       */
/* terminated by a call to lefwEndUnits.                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwUnits (double  time, double  capacitance, double  resistance, double  power, double  current, double  voltage, double  database);

/* This routine is called once for each unit.  It is separated from           */
/* lefwUnits due to backwards compatible for pre 5.3.                         */
/* Returns 0 if successful.                                                   */
EXTERN int lefwUnitsFrequency (double  frequency);

/* This routine must be called after the lefwUnits call (if any).             */
/* The routine can be called only once.                                       */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndUnits ();

/* This routine is called after lefwInit.                                     */
/* Either this routine or lefwStartLayerRouting must be called.               */
/* Multiple sections of lefwStartLayer can be called.                         */
/* This routine can be called only once per section.                          */
/* Returns 0 if successful.                                                   */
/* The routine starts the layer section. All of the layers must follow.       */
/* The type can be either CUT for Cut Layer, MASTERSLICE or OVERLAP for       */
/* Masterslice or Overlay Layer.                                              */
EXTERN int lefwStartLayer (const char*  layerName, const char*  type);

/* This routine must be called after lefwStartLayer.                          */
/* This section of routines is optional.                                      */
/* Returns 0 if successful.                                                   */
/* This is a 5.8 syntax.                                                      */
EXTERN int lefwLayerMask (int  maskColor);

/* This routine must be called after lefwStartLayer.                          */
/* This section of routines is optional.                                      */
/* Returns 0 if successful.                                                   */
/* This is a 5.5 syntax.                                                      */
/* This routine is called if the layer type is IMPLANT when                   */
/* lefwStartLayer is called.                                                  */
EXTERN int lefwLayerWidth (double  minWidth);

/* The following APIs are for lefwrite 5.7 Layer, Spacing with type CUT       */
/* Due to adding new constructs, it is impossible to use the previous         */
/* APIs.                                                                      */
/* The following APIs are obsoleted in 5.7:                                   */
/*    lefwLayer                                                               */
/*    lefwLayerStack                                                          */
/*    lefwLayerSpacingAdjacent                                                */
/*    lefwLayerSpacingCenterToCenter                                          */

/* This routine must be called after lefwStartLayer.                          */
/* This routine starts the Layer Type Cut Spacing                             */
/* Returns 0 if successful.                                                   */
/* The routing lefwLayerCutSpacingEnd has to call at the end of each spacing. */
/* This is a 5.7 syntax.                                                      */
EXTERN int lefwLayerCutSpacing (double  spacing);

/* This routine must be called after lefwLayerSpacing                         */
/* This routine is optional.                                                  */
/* Returns 0 if successful.                                                   */
/* This is a 5.7 syntax.                                                      */
EXTERN int lefwLayerCutSpacingCenterToCenter ();

/* This routine must be called after lefwLayerSpacing                         */
/* This routine is optional.                                                  */
/* Returns 0 if successful.                                                   */
/* This is a 5.7 syntax.                                                      */
EXTERN int lefwLayerCutSpacingSameNet ();

/* This routine must be called after lefwLayerSpacing                         */
/* This routine is optional.                                                  */
/* Either this routine, lefwLayerCutSpacingAdjacent,                          */
/* lefwLayerCutSpacingParallel or lefwLayerCutSpacingArea is called per Spaci */
/* Returns 0 if successful.                                                   */
/* This is a 5.7 syntax.                                                      */
EXTERN int lefwLayerCutSpacingLayer (const char*  name2, int  stack);

/* This routine must be called after lefwLayerSpacing                         */
/* This routine is optional.                                                  */
/* Either this routine, lefwLayerCutSpacingLayer,                             */
/* lefwLayerCutSpacingParallel or lefwLayerCutSpacingArea is called per Spaci */
/* Returns 0 if successful.                                                   */
/* This is a 5.7 syntax.                                                      */
EXTERN int lefwLayerCutSpacingAdjacent (int  viaCuts, double  distance, int  stack);

/* This routine must be called after lefwLayerSpacing                         */
/* This routine is optional.                                                  */
/* Either this routine, lefwLayerCutSpacingLayer,                             */
/* lefwLayerCutSpacingAdjacent or lefwLayerCutSpacingArea is called per Spaci */
/* Returns 0 if successful.                                                   */
/* This is a 5.7 syntax.                                                      */
EXTERN int lefwLayerCutSpacingParallel ();

/* This routine must be called after lefwLayerSpacing                         */
/* This routine is optional.                                                  */
/* Either this routine, lefwLayerCutSpacingLayer, * lefwLayerCutSpacingAdjace */
/* or lefwLayerCutSpacingParallel is called per Spacing.                      */
/* Returns 0 if successful.                                                   */
/* This is a 5.7 syntax.                                                      */
EXTERN int lefwLayerCutSpacingArea (double  cutArea);

/* This routine must be called after lefwLayerSpacing                         */
/* This routine marks the end of a Layer Type CUT Spacing                     */
/* Returns 0 if successful.                                                   */
/* This is a 5.7 syntax.                                                      */
EXTERN int lefwLayerCutSpacingEnd ();

/* This routine must be called after lefwStartLayer.                          */
/* This routine can be called only once.                                      */
/* This section of routines is optional.                                      */
/* This is a 5.7 syntax                                                       */
/* Returns 0 if successful.                                                   */
/* This routine is called if the layer type is CUT when                       */
/* lefwStartLayer is called.                                                  */
EXTERN int lefwLayerCutSpacingTableOrtho (int  numSpacing, double*  cutWithins, double*  orthoSpacings);

/* This routine must be called after lefwStartLayer.                          */
/* This routine can be called only once.                                      */
/* This section of routines is optional.                                      */
/* This is a 5.7 syntax                                                       */
/* Returns 0 if successful.                                                   */
/* This routine is called if the layer type is CUT when                       */
/* lefwStartLayer is called.                                                  */
EXTERN int lefwLayerArraySpacing (int  longArray, double  viaWidth, double  cutSpacing, int  numArrayCut, int*  arrayCuts, double*  arraySpacings);

/* This routine must be called after lefwStartLayer.                          */
/* This routine can be called multiple times.                                 */
/* This section of routines is optional.                                      */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
/* This routine is called if the layer type is CUT when                       */
/* lefwStartLayer is called.                                                  */
EXTERN int lefwLayerEnclosure (const char*  location, double  overhang1, double  overhang2, double  width);

/* This routine must be called after lefwStartLayer.                          */
/* This routine can be called multiple times.                                 */
/* This routine is similar as lefwLayerEnclosure, but allow user to add       */
/* EXCEPTEXTRACUT option                                                      */
/* This section of routines is optional.                                      */
/* This is a 5.7 syntax.                                                      */
/* Returns 0 if successful.                                                   */
/* This routine is called if the layer type is CUT when                       */
/* lefwStartLayer is called.                                                  */
EXTERN int lefwLayerEnclosureWidth (const char*  location, double  overhang1, double  overhang2, double  width, double  cutWithin);

/* This routine must be called after lefwStartLayer.                          */
/* This routine can be called multiple times.                                 */
/* This routine is to write out minLength inside ENCLOSURE instead of WIDTH   */
/* as in the routine lefwLayerEnclosure & lefwLayerEnclosureWidth.            */
/* This section of routines is optional.                                      */
/* This is a 5.7 syntax.                                                      */
/* Returns 0 if successful.                                                   */
/* This routine is called if the layer type is CUT when                       */
/* lefwStartLayer is called.                                                  */
EXTERN int lefwLayerEnclosureLength (const char*  location, double  overhang1, double  overhang2, double  minLength);

/* This routine must be called after lefwStartLayer.                          */
/* This routine can be called multiple times.                                 */
/* This section of routines is optional.                                      */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
/* This routine is called if the layer type is CUT when                       */
/* lefwStartLayer is called.                                                  */
EXTERN int lefwLayerPreferEnclosure (const char*  location, double  overhang1, double  overhang2, double  width);

/* This routine must be called after lefwStartLayer.                          */
/* This routine can be called only once per Layer.                            */
/* This section of routines is optional.                                      */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
/* This routine is called if the layer type is CUT when                       */
/* lefwStartLayer is called.                                                  */
EXTERN int lefwLayerResistancePerCut (double  resistance);

/* This routine must be called after the lefwStartLayer call (if any).        */
/* The routine can be called only once per section.                           */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndLayer (const char*  layerName);

/* This routine is called after lefwInit.                                     */
/* Either this routine or lefwStartLayer must be called.                      */
/* Multiple section of lefwStartLayer can be called.                          */
/* The routine can be called only once per section.                           */
/* Returns 0 if successful.                                                   */
/* The routine starts the layer routing section.                              */
/* All of the layers must follow.                                             */
EXTERN int lefwStartLayerRouting (const char*  layerName);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is required for LayerRouting.                                 */
/* Returns 0 if successful.                                                   */
                                                   /* DIAG45 | DIAG135        */
EXTERN int lefwLayerRouting (const char*  direction, double  width);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* Either this routine or lefwLayerRoutingPitchXYDistance can be called       */
/* but not both                                                               */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingPitch (double  pitch);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* Either this routine or lefwLayerRoutingPitch can be called but not both    */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingPitchXYDistance (double  xDistance, double  yDistance);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* Either this routine or lefwLayerRoutingDiagPitchXYDistance can be called   */
/* but not both                                                               */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingDiagPitch (double  distance);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* Either this routine or lefwLayerRoutingDiagPitch can be called             */
/* but not both                                                               */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingDiagPitchXYDistance (double  diag45Distance, double  diag135Distance);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingDiagWidth (double  diagWidth);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingDiagSpacing (double  diagSpacing);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingDiagMinEdgeLength (double  diagLength);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* Either this routine or lefwLayerRoutingOffsetXYDistance can be called      */
/* but not both                                                               */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingOffset (double  offset);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* Either this routine or lefwLayerRoutingOffset can be called but not both   */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingOffsetXYDistance (double  xDistance, double  yDistance);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingArea (double  area);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingMinsize (int  numRect, double*  minWidth, double*  minLength);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingMinimumcut (double  numCuts, double  minWidth);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is similar as lefwLayerRoutingMinimumcut, but allow user      */
/* to specify value for WITHIN                                                */
/* This is a 5.7 syntax.                                                      */
/* This routine is optional                                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingMinimumcutWithin (double  numCuts, double  minWidth, double  cutDistance);

/* This routine must be called only once after lefwLayerRoutingMinimumcut.    */
/* This routine is optional.                                                  */
/* Direction can be either FROMABOVE or FROMBELOW                             */
/* This is a 5.5 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingMinimumcutConnections (const char*  direction);

/* This routine must be called only once after lefwLayerRoutingMinimumcut.    */
/* This routine is optional.                                                  */
/* This is a 5.5 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingMinimumcutLengthWithin (double  length, double  distance);

/* This routine must be called multiple time after lefwStartLayerRouting.     */
/* This routine is optional                                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingSpacing (double  spacing);

/* Either this routine, lefwLayerRoutingSpacingLengthThreshold or             */
/* lefwLayerRoutingSpacingSameNet can be                                      */
/* called only once after lefwLayerRoutingSpacing.                            */
/* This routine is optional.                                                  */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingSpacingRange (double  minWidth, double  maxWidth);

/* Either this routine or lefwLayerRoutingSpacingRangeInfluence or            */
/* lefwLayerRoutingSpacingRangeRange can                                      */
/* be called once after llefwLayerRoutingSpacingRange.                        */
/* This routine is valid only if either or both leftRange and rightRange      */
/* in lefwLayerRoutingSpacing are non zero                                    */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingSpacingRangeUseLengthThreshold ();

/* Either this routine or lefwLayerRoutingSpacingRangeUseLengthThreshold or   */
/* lefwLayerRoutingSpacingRangeRange can be called once after                 */
/* lefwLayerRoutingSpacingRange.                                              */
/* subMinWidth & subMaxWidth are optional.                                    */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingSpacingRangeInfluence (double  infValue, double  subMinWidth, double  subMaxWidth);

/* Either this routine or lefwLayerRoutingSpacingRangeUseLengthThreshold or   */
/* lefwLayerRoutingSpacingRangeInfluence can be called once after             */
/* lefwLayerRoutingSpacingRange.                                              */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingSpacingRangeRange (double  minWidth, double  maxWidth);

/* Either this routine, lefwLayerRoutingSpacingRange or                       */
/* lefwLayerRoutingSpacingSameNet can be                                      */
/* be called once after lefwLayerRoutingSpacing.                              */
/* minWidth & maxWidth are optional.                                          */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingSpacingLengthThreshold (double  lengthValue, double  minWidth, double  maxWidth);

/* Either this routine, lefwLayerRoutingSpacingRange or                       */
/* lefwLayerRoutingSpacingRange can be                                        */
/* be called once after lefwLayerRoutingSpacing.                              */
/* This is a 5.7 routine.                                                     */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingSpacingSameNet (int  PGOnly);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* This is a 5.7 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingSpacingEndOfLine (double  eolWidth, double  eolWithin);

/* This routine must be called only once after lefwLayerRoutingSpacingEndOfLi */
/* This routine is optional                                                   */
/* This is a 5.7 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingSpacingEOLParallel (double  parSpace, double  parWithin, int  twoEdges);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* This is a 5.7 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingSpacingNotchLength (double  minNLength);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* This is a 5.7 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingSpacingEndOfNotchWidth (double  eonWidth, double  minNSpacing, double  minNLength);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingWireExtension (double  wireExtension);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingResistance (const char*  resistance);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingCapacitance (const char*  capacitance);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingHeight (double  height);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingThickness (double  thickness);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingShrinkage (double  shrinkage);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingCapMultiplier (double  capMultiplier);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingEdgeCap (double  edgeCap);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingAntennaArea (double  antennaArea);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional                                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingAntennaLength (double  antennaLength);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional and can be called once.                           */
/* This is a 5.5 syntax.                                                      */
/* width is the maximum width.                                                */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingMaxwidth (double  width);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional and can be called once.                           */
/* This is a 5.5 syntax.                                                      */
/* width is the maximum width.                                                */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingMinwidth (double  width);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional.                                                  */
/* This is a 5.5 syntax.                                                      */
/* Area is the minimum area size limit for metal that encloses an empty area. */
/* Width is optional, it says the rule only applies when a donut is careted   */
/* from a wire of width <= width.  The parameter width is required for the    */
/* routine lefwLayerRoutineMinenclosedarea.  If width is optional, a "0"      */
/* value is assigned for that index slot of the array.                        */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingMinenclosedarea (int  numMinenclosed, double*  area, double*  width);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional and can be called once.                           */
/* This is a 5.5 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingMinstep (double  distance);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional and can be called once.                           */
/* This routine is equivalent to lefwLayerRoutingMinstep, except it also      */
/* takes the options for type & Lengthsum.                                    */
/* This is a 5.5 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingMinstepWithOptions (double  distance, const char*  rule, double  maxLength);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional and can be called once.                           */
/* This routine is equivalent to lefwLayerRoutingMinstep, except it also      */
/* takes the option for MaxEdges.                                             */
/* This is a 5.7 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingMinstepMaxEdges (double  distance, double  maxEdges);

/* This routine must be called only once after lefwStartLayerRouting.         */
/* This routine is optional and can be called once.                           */
/* This is a 5.5 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingProtrusion (double  width1, double  length, double  width2);

/* This routine must be called only after lefwStartLayerRouting.              */
/* This routine is optional and can be called multiple times.                 */
/* This is a 5.5 syntax.                                                      */
/* numLength has the size of the array length.                                */
/* length is an array of length values.                                       */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingStartSpacingtableParallel (int  numLength, double*  length);

/* This routine must be called only after                                     */
/* lefwLayerRoutingStartSpacingtableParallel.                                 */
/* This routine is required after lefwLayerRoutingStartSpacingtableParallel.  */
/* This routine can be called multiple times.                                 */
/* This is a 5.5 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingSpacingtableParallelWidth (double  width, int  numSpacing, double*  spacing);

/* This routine must be called only after lefwStartLayerRouting.              */
/* This routine is optional and can be called multiple times.                 */
/* This is a 5.5 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingStartSpacingtableInfluence ();

/* This routine must be called only after                                     */
/* lefwLayerRoutingStartSpacingtableInfluence.                                */
/* This routine is required after lefwLayerRoutingStartSpacingtableInfluence. */
/* This routine can be called multiple times.                                 */
/* This is a 5.5 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingSpacingInfluenceWidth (double  width, double  distance, double  spacing);

/* This routine must be called only after lefwStartLayerRouting.              */
/* This routine is optional and can be called multiple times.                 */
/* This is a 5.7 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingStartSpacingtableTwoWidths ();

/* This routine must be called only after                                     */
/* lefwLayerRoutingStartSpacingtableInfluence.                                */
/* This routine is required after lefwLayerRoutingStartSpacingtableTwoWidths. */
/* This routine can be called multiple times.                                 */
/* This is a 5.7 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutingSpacingtableTwoWidthsWidth (double  width, double  runLength, int  numSpacing, double*  spacing);

/* This routine can be called after lefwLayerRoutingStartSpacingtableParallel */
/* or lefwLayerRoutingStartSpacingtableInfluence.                             */
/* It can only be called once.                                                */
/* This is a 5.5 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerRoutineEndSpacingtable ();

/* This routine must be called after the lefwStartLayer call (if any).        */
/* The routine can be called only once per section.                           */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndLayerRouting (const char*  layerName);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called multiple times.                 */
/* if the value for the variable value is not 0, one of the following         */
/* routines have to be called: lefwLayerACFrequency,                          */
/* lefwLayerACWidth, lefwLayerACCutarea, or                                   */
/* lefwLayerACTableEntries                                                    */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerACCurrentDensity (const char*  type, double  value);

/* This routine must be called after lefwLayerACCurrentDensity.               */
/* This routine is required and can be called only once after each            */
/* lefwLayerACCurrentDensity.                                                 */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerACFrequency (int  numFrequency, double*  frequency);

/* This routine must be called after lefwLayerACCurrentDensity.               */
/* This routine is optional and can be called only once after each            */
/* lefwLayerACCurrentDensity.                                                 */
/* This routine can only be called in Layer Routing                           */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerACWidth (int  numWidths, double*  widths);

/* This routine must be called after lefwLayerACCurrentDensity.               */
/* This routine is optional and can be called only once after each            */
/* lefwLayerACCurrentDensity.                                                 */
/* This routine can only be called in Layer                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerACCutarea (int  numCutareas, double*  cutareas);

/* This routine must be called after lefwLayerACCurrentDensity.               */
/* This routine is required and can be called only once after each            */
/* lefwLayerACCurrentDensity.                                                 */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerACTableEntries (int  numEntries, double*  entries);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called multiple times.                 */
/* if the value for the variable value is not 0, one of the following         */
/* routines have to be called: lefwLayerDCWidth, lefwLayerDCCutarea, or       */
/* lefwLayerDCTableEntries                                                    */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerDCCurrentDensity (const char*  type, double  value);

/* This routine must be called after lefwLayerDCCurrentDensity.               */
/* This routine is optional and can be called only once after each            */
/* lefwLayerDCCurrentDensity.                                                 */
/* This routine can only be called in Layer Routing                           */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerDCWidth (int  numWidths, double*  widths);

/* This routine must be called after lefwLayerDCCurrentDensity.               */
/* This routine is optional and can be called only once after each            */
/* lefwLayerDCCurrentDensity.                                                 */
/* This routine can only be called in Layer                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerDCCutarea (int  numCutareas, double*  cutareas);

/* This routine must be called after lefwLayerDCCurrentDensity.               */
/* This routine is required and can be called only once after each            */
/* lefwLayerDCCurrentDensity.                                                 */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerDCTableEntries (int  numEntries, double*  entries);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional.  The oxide value, can be either OXIDE1, OXIDE2,  */
/* OXIDE3, or OXIDE4.  Each can only be called once within a layer.           */
/* This routine is valid only if the layer type is either ROUTING or CUT.     */
/* This is a 5.5 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaModel (const char*  oxide);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING or CUT.     */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or lefwLayerRoutingAntennaLength is allowed, not both. */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaAreaRatio (double  value);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING or CUT.     */
/* Either this routine or lefwLayerAntennaDiffAreaRatioPwl can be called, but */
/* not both                                                                   */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or lefwLayerRoutingAntennaLength is allowed, not both. */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaDiffAreaRatio (double  value);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING or CUT.     */
/* Either this routine or lefwLayerAntennaDiffAreaRatio can be called, but    */
/* not both                                                                   */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or lefwLayerRoutingAntennaLength is allowed, not both. */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaDiffAreaRatioPwl (int  numPwls, double*  diffusions, double*  ratios);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING or CUT.     */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or lefwLayerRoutingAntennaLength is allowed, not both. */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaCumAreaRatio (double  value);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING or CUT.     */
/* Either this routine or lefwLayerAntennaCumDiffAreaRatioPwl can be called,  */
/* but not both                                                               */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or lefwLayerRoutingAntennaLength is allowed, not both. */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaCumDiffAreaRatio (double  value);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING or CUT.     */
/* Either this routine or lefwLayerAntennaCumDiffAreaRatio can be called, but */
/* not both                                                                   */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or lefwLayerRoutingAntennaLength is allowed, not both. */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaCumDiffAreaRatioPwl (int  numPwls, double*  diffusions, double*  ratios);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING or CUT.     */
/* The option DIFFUSEONLY is a 5.4 syntax.                                    */
/* If DIFFUSEONLY, lefwLayerRoutingAntennaLength is not allowed.              */
/* This function is similar to lefwLayerRoutingAntennaArea                    */
/* diffUseOnly has to be DIFFUSEONLY.                                         */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaAreaFactor (double  value, const char*  diffUseOnly);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING.            */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or lefwLayerRoutingAntennaLength is allowed, not both. */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaSideAreaRatio (double  value);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING.            */
/* Either this routine or lefwLayerAntennaDiffSideAreaRatioPwl can be         */
/* called, but not both                                                       */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or lefwLayerRoutingAntennaLength is allowed, not both. */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaDiffSideAreaRatio (double  value);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING or CUT.     */
/* Either this routine or lefwLayerAntennaDiffSideAreaRatio can be called,    */
/* but not both                                                               */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or lefwLayerRoutingAntennaLength is allowed, not both. */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaDiffSideAreaRatioPwl (int  numPwls, double*  diffusions, double*  ratios);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING.            */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or lefwLayerRoutingAntennaLength is allowed, not both. */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaCumSideAreaRatio (double  value);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING.            */
/* Either this routine or lefwLayerAntennaCumDiffSideAreaRatioPwl can be      */
/* called, but not both                                                       */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or lefwLayerRoutingAntennaLength is allowed, not both. */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaCumDiffSideAreaRatio (double  value);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING or CUT.     */
/* Either this routine or lefwLayerAntennaCumDiffSideAreaRatio can be called, */
/* but not both                                                               */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or lefwLayerRoutingAntennaLength is allowed, not both. */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaCumDiffSideAreaRatioPwl (int  numPwls, double*  diffusions, double*  ratios);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING.            */
/* The option DIFFUSEONLY is a 5.4 syntax.                                    */
/* diffUseOnly has to be DIFFUSEONLY.                                         */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaSideAreaFactor (double  value, const char*  diffUseOnly);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING or CUT.     */
/* This is a 5.7 routine.                                                     */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaCumRoutingPlusCut ();

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING or CUT.     */
/* This is a 5.7 routine.                                                     */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaGatePlusDiff (double  plusDiffFactor);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING or CUT.     */
/* This is a 5.7 routine.                                                     */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaAreaMinusDiff (double  minusDiffFactor);

/* This routine must be called after lefwStartLayerRouting or lefwStartLayer. */
/* This routine is optional and can be called only once within a layer        */
/* after each lefwLayerAntennaModel function.                                 */
/* This routine is valid only if the layer type is either ROUTING or CUT.     */
/* This is a 5.7 routine.                                                     */
/* Returns 0 if successful.                                                   */
EXTERN int lefwLayerAntennaAreaDiffReducePwl (int  numPwls, double*  diffAreas, double*  metalDiffFactors);

/* This routine must be called only once after lefwStartLayer.                */
/* The option MINIMUMDENSITY is a 5.4 syntax.                                 */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMinimumDensity (double  minDensity);

/* This routine must be called only once after lefwStartLayer.                */
/* The option MAXIMUMDENSITY is a 5.4 syntax.                                 */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMaximumDensity (double  maxDensity);

/* This routine must be called only once after lefwStartLayer.                */
/* The option DENSITYCHECKWINDOW is a 5.4 syntax.                             */
/* Returns 0 if successful.                                                   */
EXTERN int lefwDensityCheckWindow (double  checkWindowLength, double  checkWindowWidth);

/* This routine must be called only once after lefwStartLayer.                */
/* The option DENSITYCHECKSTEP is a 5.4 syntax.                               */
/* Returns 0 if successful.                                                   */
EXTERN int lefwDensityCheckStep (double  checkStepValue);

/* This routine must be called only once after lefwStartLayer.                */
/* The option FILLACTIVESPACING is a 5.4 syntax.                              */
/* Returns 0 if successful.                                                   */
EXTERN int lefwFillActiveSpacing (double  fillToActiveSpacing);

/* This routine must be called only once after all the layers.                */
/* This routine is optional.                                                  */
/* This is a 5.5 syntax.                                                      */
/* Returns 0 if succesful.                                                    */
EXTERN int lefwMaxviastack (int  value, const char*  bottomLayer, const char*  topLayer);

/* This routine is called after lefwInit.                                     */
/* This section of routines is optional.                                      */
/* The routine can be called only once.                                       */
/* Returns 0 if successful.                                                   */
/* The routine starts propertydefinitions section.                            */
EXTERN int lefwStartPropDef ();

/* This routine must be called after lefwStartPropDef.                        */
/* This routine can be called multiple times.                                 */
/* It adds integer property definition to the statement.                      */
/* Returns 0 if successful.                                                   */
/* The objType can be LIBRARY or VIA or MACRO or PIN.                         */
                                      /* NONDEFAULTRULE | MACRO | PIN         */
EXTERN int lefwIntPropDef (const char*  objType, const char*  propName, double  leftRange, double  rightRange, int     propValue);

/* This routine must be called after lefwStartPropDef.                        */
/* This routine can be called multiple times.                                 */
/* It adds real property definition to the statement.                         */
/* Returns 0 if successful.                                                   */
/* The objType can be LIBRARY or VIA or MACRO or PIN.                         */
                                      /* NONDEFAULTRULE | MACRO | PIN         */
EXTERN int lefwRealPropDef (const char*  objType, const char*  propName, double  leftRange, double  rightRange, double  propValue);

/* This routine must be called after lefwStartPropDef.                        */
/* This routine can be called multiple times.                                 */
/* It adds string property definition to the statement.                       */
/* Returns 0 if successful.                                                   */
/* The objType can be LIBRARY or VIA or MACRO or PIN.                         */
                                       /*  NONDEFAULTRULE | MACRO | PIN       */
EXTERN int lefwStringPropDef (const char*  objType, const char*  propName, double  leftRange, double  rightRange, const char*  propValue);

/* This routine must be called after the lefwStartPropDef call (if any).      */
/* The routine can be called only once.                                       */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndPropDef ();

/* This routine is called after lefwInit.                                     */
/* This routine must be called only once.                                     */
/* Returns 0 if successful.                                                   */
/* The routine starts the via section. All of the vias must follow.           */
EXTERN int lefwStartVia (const char*  viaName, const char*  isDefault);

/* This routine is optional, it call only be called after lefwStartVia.       */
/* It can only be called once.                                                */
/* Returns 0 if successful.                                                   */
EXTERN int lefwViaTopofstackonly ();

/* This routine is optional, it call only be called after lefwStartVia.       */
/* It can only be called once.                                                */
/* Returns 0 if successful.                                                   */
EXTERN int lefwViaForeign (const char*  foreignName, double  xl, double  yl, int  orient);

/* This routine is optional, it call only be called after lefwStartVia.       */
/* It can only be called once.                                                */
/* Returns 0 if successful.                                                   */
/* This routine is the same as lefwViaForeign, except orient is a char*       */
EXTERN int lefwViaForeignStr (const char*  foreignName, double  xl, double  yl, const char*  orient);

/* This routine is optional, it call only be called after lefwStartVia.       */
/* Either this routine or lefwViaViarule can be called within a via.          */
/* It can only be called once in a via.                                       */
/* Returns 0 if successful.                                                   */
EXTERN int lefwViaResistance (double  resistance);

/* This routine must be called after lefwStartVia.                            */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
EXTERN int lefwViaLayer (const char*  layerName);

/* This routine can call only after lefwViaLayer.                             */
/* Either this routine or lefwViaLayerPolygon can be called within a layer    */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* mask is 5.8 syntax                                                         */
EXTERN int lefwViaLayerRect (double  x1l, double  y1l, double  x2l, double  y2l, int  mask);

/* This routine can call only after lefwViaLayer.                             */
/* Either this routine or lefwViaLayerRect can be called within a layer       */
/* It can be called multiple times.                                           */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
/* mask is 5.8 syntax                                                         */
EXTERN int lefwViaLayerPolygon (int  num_polys, double*  xl, double*  yl, int  mask);

/* This routine can call only after lefwStartVia.                             */
/* Either this routine or lefwViaResistance can be called within a via.       */
/* It can only be called once in a via.                                       */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwViaViarule (const char*  viaRuleName, double  xCutSize, double  yCutSize, const char*  botMetalLayer, const char*  cutLayer, const char*  topMetalLayer, double  xCutSpacing, double  yCutSpacing, double  xBotEnc, double  yBotEnc, double  xTopEnc, double  yTopEnc);

/* This routine can call only after lefwViaViarule.                           */
/* It can only be called once.                                                */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwViaViaruleRowCol (int  numCutRows, int  numCutCols);

/* This routine can call only after lefwViaViarule.                           */
/* It can only be called once.                                                */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwViaViaruleOrigin (double  xOffset, double  yOffset);

/* This routine can call only after lefwViaViarule.                           */
/* It can only be called once.                                                */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwViaViaruleOffset (double  xBotOffset, double  yBotOffset, double  xTopOffset, double  yTopOffset);

/* This routine can call only after lefwViaViarule.                           */
/* It can only be called once.                                                */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwViaViarulePattern (const char*  cutPattern);

/* This routine is called after lefwVia.                                      */
/* This routine is optional, it adds string property to the statement.        */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
EXTERN int lefwStringProperty (const char*  propName, const char*  propValue);

/* This routine is called after lefwVia.                                      */
/* This routine is optional, it adds real property to the statement.          */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
EXTERN int lefwRealProperty (const char*  propName, double  propValue);

/* This routine is called after lefwVia.                                      */
/* This routine is optional, it adds int property to the statement.           */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
EXTERN int lefwIntProperty (const char*  propName, int  propValue);

/* This routine must be called after the lefwStartVia call (if any).          */
/* The routine can be called only once.                                       */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndVia (const char*  viaName);

/* This routine is called after lefwInit.                                     */
/* This routine must be called as a set with lefwViaRuleLayer, lefwViaRuleVia */
/* and lefwEndViaRule                                                         */
/* multiple times.                                                            */
/* Returns 0 if successful.                                                   */
/* The routine starts the via rule section. The via rule data and its propert */
/* must follow                                                                */
EXTERN int lefwStartViaRule (const char*  viaRuleName);

/* This routine is called after lefwStartViaRule.                             */
/* This routine has to be called 2 times exact.                               */
/* Returns 0 if successful.                                                   */
/* The direction1 or direction2 can be HORIZONTAL or VERTICAL.                */
EXTERN int lefwViaRuleLayer (const char*  layerName, const char*  direction, double  minWidth, double  maxWidth, double  overhang, double  metalOverhang);

/* This routine is called after lefwViaRuleLayer is called twice.             */
/* This routine is required in a viarule and can be called more than once.    */
/* Returns 0 if successful.                                                   */
/* The direction1 or direction2 can be HORIZONTAL or VERTICAL.                */
EXTERN int lefwViaRuleVia (const char*  viaName);

/* This routine must be called after the lefwStartViaRule call (if any).      */
/* The routine can be called only once per lefwStartViaRule.                  */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndViaRule (const char*  viaRuleName);

/* This routine is called after lefwInit.                                     */
/* This routine must be called as a set with lefwViaRuleGenLayer,             */
/* lefwViaRuleGenLayer3, and lefwEndViaRuleGen multiple times.                */
/* Returns 0 if successful.                                                   */
/* The routine starts the via rule section. The via rule data and its propert */
/* must follow                                                                */
EXTERN int lefwStartViaRuleGen (const char*  viaRuleName);

/* This routine is called after lefwSartViaRuleGen.                           */
/* This routine is optional                                                   */
/* Returns 0 if successful                                                    */
EXTERN int lefwViaRuleGenDefault ();

/* This routine is called after lefwStartViaRuleGen.                          */
/* This routine has to be called 2 times exact.                               */
/* Returns 0 if successful.                                                   */
/* Either this routine or lefwViaRuleGenLayerEnclosure, not both.             */
/* The direction1 or direction2 can be HORIZONTAL or VERTICAL.                */
EXTERN int lefwViaRuleGenLayer (const char*  layerName, const char*  direction, double  minWidth, double  maxWidth, double  overhang, double  metalOverhang);

/* This routine is called after lefwStartViaRuleGen.                          */
/* This routine has to be called 2 times exact.                               */
/* This is 5.5 syntax                                                         */
/* Returns 0 if successful.                                                   */
/* Either this routine or lefwViaRuleGenLayer, not both.                      */
EXTERN int lefwViaRuleGenLayerEnclosure (const char*  layerName, double  overhang1, double  overhang2, double  minWidth, double  maxWidth);

/* This routine is called after lefwViaRuleLayerGen is called twice.          */
/* This routine is optional in a viarule generate and can be called once.     */
/* Returns 0 if successful.                                                   */
/* The direction1 or direction2 can be HORIZONTAL or VERTICAL.                */
EXTERN int lefwViaRuleGenLayer3 (const char*  layerName, double  xl, double  yl, double  xh, double  yh, double  xSpacing, double  ySpacing, double  resistance);

/* This routine must be called after the lefwStartViaRuleGen call (if any).   */
/* The routine can be called only once per lefwStartViaRuleGen.               */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndViaRuleGen (const char*  viaRuleName);

/* This routine is called after lefwInit.                                     */
/* This routine must be called only once.                                     */
/* Returns 0 if successful.                                                   */
/* The routine starts the nonDefaultRule section. The nonDefaultRule layers   */
/* must follow                                                                */
EXTERN int lefwStartNonDefaultRule (const char*  ruleName);

/* This routine is called after lefwInit.                                     */
/* This routine must be called after lefwStartNonDefaultRule                  */
/* This routine can be called multiple times.                                 */
/* Returns 0 if successful.                                                   */
EXTERN int lefwNonDefaultRuleLayer (const char*  routingLayerName, double  width, double  minSpacing, double  wireExtension, double  resistance, double  capacitance, double  edgeCap);

/* This routine is called after lefwInit.                                     */
/* This routine must be called after lefwStartNonDefaultRule                  */
/* This routine is optional and it can be called only once.                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwNonDefaultRuleHardspacing ();

/* This routine is called after lefwStartNonDefaultRule.                      */
/* This routine must be called only once for each via section.                */
/* Returns 0 if successful.                                                   */
/* The routine starts the nondefaultrule via section.                         */
/* Call the following via functions for the rest of NONDEFAULTRULE VIA:       */
/*   lefwViaTopofstackonly                                                    */
/*   lefwViaForeign                                                           */
/*   lefwViaForeignStr                                                        */
/*   lefwViaResistance                                                        */
/*   lefwViaLayer                                                             */
/*   lefwViaLayerRect                                                         */
/*   lefwNonDefaultRuleEndVia                                                 */
EXTERN int lefwNonDefaultRuleStartVia (const char*  viaName, const char*  isDefault);

/* This routine must be called after the lefwNonDefaultRuleStartVia call.     */
/* The routine can be called only once per via section.                       */
/* Returns 0 if successful.                                                   */
EXTERN int lefwNonDefaultRuleEndVia (const char*  viaName);

/* This routine is called after lefwStartNonDefaultRule.                      */
/* This routine can be called multiple times.                                 */
/* Returns 0 if successful.                                                   */
EXTERN int lefwNonDefaultRuleUseVia (const char*  viaName);

/* This routine is called after lefwStartNonDefaultRule.                      */
/* This routine can be called multiple times.                                 */
/* Returns 0 if successful.                                                   */
EXTERN int lefwNonDefaultRuleUseViaRule (const char*  viaRuleName);

/* This routine is called after lefwStartNonDefaultRule.                      */
/* This routine can be called multiple times.                                 */
/* Returns 0 if successful.                                                   */
EXTERN int lefwNonDefaultRuleMinCuts (const char*  layerName, int  numCuts);

/* This routine must be called after the lefwStartNonDefaultRule call (if any */
/* The routine can be called only once.                                       */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndNonDefaultRule (const char*  ruleName);

/* This routine is called after lefwInit.                                     */
/* This section of routines is optional.                                      */
/* This routine can be called only once.                                      */
/* Returns 0 if successful.                                                   */
/* It starts the spacing section.                                             */
EXTERN int lefwStartSpacing ();

/* This routine must be called after lefwStartSpacing.                        */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* The stack has to be STACK.                                                 */
EXTERN int lefwSpacing (const char*  layerName1, const char*  layerName2, double  minSpace, const char*  stack);

/* This routine must be called after the lefwStartSpacing call (if any).      */
/* The routine can be called only once.                                       */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndSpacing ();

/* This routine is called after lefwInit.                                     */
/* This routine is optional and it can be called only once.                   */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwUniversalNoiseMargin (double  high, double  low);

/* This routine is called after lefwInit.                                     */
/* This routine is optional and it can be called only once.                   */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwEdgeRateThreshold1 (double  num);

/* This routine is called after lefwInit.                                     */
/* This routine is optional and it can be called only once.                   */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwEdgeRateThreshold2 (double  num);

/* This routine is called after lefwInit.                                     */
/* This routine is optional and it can be called only once.                   */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwEdgeRateScaleFactor (double  num);

/* This routine is called after lefwInit.                                     */
/* This routine is optional and it can be called only once.                   */
/* Returns 0 if successful.                                                   */
/* This routine starts the noisetable section.                                */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwStartNoiseTable (int  num);

/* This routine is called after lefwStartNoiseTable or lefwStartCorrectTable. */
/* This routine is optional and it can be called multiple times               */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwEdgeRate (double  num);

/* This routine is called after lefwEdgeRate.                                 */
/* This routine is optional and it can be called only once inside lefwEdgeRat */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwOutputResistance (int  numResists, double*  resistance);

/* This routine is called after lefwOutputResistance.                         */
/* This routine is optional and it can be called multiple times inside        */
/* lefwOutputResistance                                                       */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwVictims (int  length, int  numNoises, double*  noises);

/* This routine must be called after the lefwStartNoiseTable call (if any).   */
/* The routine can be called only once per section.                           */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwEndNoiseTable ();

/* This routine is called after lefwInit.                                     */
/* This routine is optional and it can be called only once.                   */
/* Returns 0 if successful.                                                   */
/* This routine starts the correctTable section.                              */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwStartCorrectTable (int  num);

/* This routine must be called after the lefwStartCorrectTable call (if any). */
/* The routine can be called only once per section.                           */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwEndCorrectTable ();

/* This routine is called after lefwInit.                                     */
/* This routine is optional and it can be called only once.                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMinFeature (double  x, double  y);

/* This routine is called after lefwInit.                                     */
/* This routine is optional and it can be called only once.                   */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
/* extern int lefwDielectric (float dielectric);                              */

/* This routine is called after lefwInit.                                     */
/* This routine is optional and it can be called only once.                   */
/* Returns 0 if successful.                                                   */
/* This routine starts the irdrop section.                                    */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwStartIrdrop ();

/* This routine is must be called after lefwStartIrdrop.                      */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* The currentsNvolts is a list of current and volts.                         */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwIrdropTable (const char*  tableName, const char*  currentsNvolts);

/* This routine must be called after the lefwStartIrdrop call (if any).       */
/* The routine can be called only once.                                       */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwEndIrdrop ();

/* This routine is must be called after lefwInit.                             */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* The classType can be PAD or CORE.                                          */
/* The symmetry can be a list of X or Y or R90.                               */
EXTERN int lefwSite (const char*  siteName, const char*  classType, const char*  symmetry, double  width, double  height);

/* This routine is must be called after lefwSite.                             */
/* It can be called multiple times.                                           */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwSiteRowPattern (const char*  siteName, int  orient);

/* This routine is must be called after lefwSite.                             */
/* It can be called multiple times.                                           */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
/* This routine is the same as lefwSiteRowPattern, except orient is a char*   */
EXTERN int lefwSiteRowPatternStr (const char*  siteName, const char * orient);

/* This routine must be called after the lefwSite call (if any).              */
/* The routine can be called only once after lefwSite.                        */
/* This is a 5.4 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndSite (const char*  siteName);

/* This routine is called after lefwInit.                                     */
/* This routine is optional and it can be called multiple times.              */
/* Returns 0 if successful.                                                   */
/* This routine starts the array section.                                     */
EXTERN int lefwStartArray (const char*  arrayName);

/* This routine is called after lefwStartArray.                               */
/* Either this routine, lefwArrayCanplace, lefwArrayCannotoccupy,             */
/* lefwArrayTracks, or lefwStartArrayFloorplan must be called at least once.  */
/* It can be call multiple times.                                             */
/* Returns 0 if successful.                                                   */
EXTERN int lefwArraySite (const char*  name, double  origX, double  origY, int  orient, double  numX, double  numY, double  spaceX, double  spaceY);

/* This routine is called after lefwStartArray.                               */
/* Either this routine, lefwArrayCanplace, lefwArrayCannotoccupy,             */
/* lefwArrayTracks, or lefwStartArrayFloorplan must be called at least once.  */
/* It can be call multiple times.                                             */
/* Returns 0 if successful.                                                   */
/* This routine is the same as lefwArraySite, except orient is a char*        */
EXTERN int lefwArraySiteStr (const char*  name, double  origX, double  origY, const char * orient, double  numX, double  numY, double  spaceX, double  spaceY);

/* This routine is called after lefwStartArray.                               */
/* Either this routine, lefwArraySite, lefwArrayCannotoccupy,                 */
/* lefwArrayTracks, or lefwStartArrayFloorplan must be called at least once.  */
/* It can be call multiple times.                                             */
/* Returns 0 if successful.                                                   */
EXTERN int lefwArrayCanplace (const char*  name, double  origX, double  origY, int  orient, double  numX, double  numY, double  spaceX, double  spaceY);

/* This routine is called after lefwStartArray.                               */
/* Either this routine, lefwArraySite, lefwArrayCannotoccupy,                 */
/* lefwArrayTracks, or lefwStartArrayFloorplan must be called at least once.  */
/* It can be call multiple times.                                             */
/* Returns 0 if successful.                                                   */
/* This routine is the same as lefwArrayCanplace, except orient is a char*    */
EXTERN int lefwArrayCanplaceStr (const char*  name, double  origX, double  origY, const char * orient, double  numX, double  numY, double  spaceX, double  spaceY);

/* This routine is called after lefwStartArray.                               */
/* Either this routine, lefwArraySite, lefwArrayCanplace,                     */
/* lefwArrayTracks, or lefwStartArrayFloorplan must be called at least once.  */
/* It can be call multiple times.                                             */
/* Returns 0 if successful.                                                   */
EXTERN int lefwArrayCannotoccupy (const char*  name, double  origX, double  origY, int  orient, double  numX, double  numY, double  spaceX, double  spaceY);

/* This routine is called after lefwStartArray.                               */
/* Either this routine, lefwArraySite, lefwArrayCanplace,                     */
/* lefwArrayTracks, or lefwStartArrayFloorplan must be called at least once.  */
/* It can be call multiple times.                                             */
/* Returns 0 if successful.                                                   */
/* This routine is the same as lefwArrayTracks, except orient is a char*      */
EXTERN int lefwArrayCannotoccupyStr (const char*  name, double  origX, double  origY, const char * orient, double  numX, double  numY, double  spaceX, double  spaceY);

/* This routine is called after lefwStartArray.                               */
/* Either this routine, lefwArraySite, lefwArrayCanplace, lefwArrayCannotoccu */
/* or lefwStartArrayFloorplan must be called at least once.                   */
/* It can be call multiple times.                                             */
/* Returns 0 if successful.                                                   */
EXTERN int lefwArrayTracks (const char*  xy, double  start, int  numTracks, double  space, const char*  layers);

/* This routine is called after lefwStartArray.                               */
/* Either this routine, lefwArraySite, lefwArrayCanplace, lefwArrayCannotoccu */
/* or lefwArrayTracks must be called at least once.                           */
/* It can be call multiple times.                                             */
/* Returns 0 if successful.                                                   */
/* The routine starts the array floorplan section                             */
EXTERN int lefwStartArrayFloorplan (const char*  name);

/* This routine must be called after lefwStartArrayFloorplan.                 */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* The site can be CANPLACE or CANNOTOCCUPY                                   */
EXTERN int lefwArrayFloorplan (const char*  site, const char*  name, double  origX, double  origY, int  orient, int  numX, int  numY, double  spaceX, double  spaceY);

/* This routine must be called after lefwStartArrayFloorplan.                 */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* The site can be CANPLACE or CANNOTOCCUPY                                   */
/* This routine is the same as lefwArrayFloorplan, except orient is a char*   */
EXTERN int lefwArrayFloorplanStr (const char*  site, const char*  name, double  origX, double  origY, const char * orient, int  numX, int  numY, double  spaceX, double  spaceY);

/* This routine must be called after the lefwStartArrayFloorplan call (if any */
/* The routine can be called only once per section.                           */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndArrayFloorplan (const char*  name);

/* This routine is called after lefwStartArray.                               */
/* This routine is optional.                                                  */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* The xy can be X or Y.                                                      */
EXTERN int lefwArrayGcellgrid (const char*  xy, double  startXY, int  colRows, double  spaceXY);

/* This routine is called after lefwStartArray.                               */
/* This section of routines is optional and can be call only once.            */
/* Returns 0 if successful.                                                   */
/* The routine starts the array defaultcap section                            */
EXTERN int lefwStartArrayDefaultCap (int  size);

/* This routine must be called after lefwStartArrayDefaultCap.                */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
EXTERN int lefwArrayDefaultCap (double  numPins, double  cap);

/* This routine must be called after the lefwStartArrayDefaultCap call (if an */
/* The routine can be called only once.                                       */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndArrayDefaultCap ();

/* This routine must be called after the lefwStartArray call (if any).        */
/* The routine can be called only once per section.                           */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndArray (const char*  arrayName);

/* This routine is must be called after lefwInit.                             */
/* This routine can be called multiple times.                                 */
/* Returns 0 if successful.                                                   */
/* This routine starts the macro section.                                     */
EXTERN int lefwStartMacro (const char*  macroName);

/* This routine is called after lefwStartMacro.                               */
/* This routine is optional and can be called only once per macro section.    */
/* Returns 0 if successful.                                                   */
/* The value1 can be COVER, RING, BLOCK, PAD, CORE, or ENCAP.                 */
/* The value2 can be BUMP if value1 is COVER,                                 */
/* or BLACKBOX, or SOFT if value1 is BLOCK,                                   */
/* or INPUT, OUTPUT, INOUT, POWER, or SPACER if value1 is PAD,                */
/* or FEEDTHRU, TIEHIGH, TIELOW, SPACER, ANTENNACELL, or WELLTAP              */
/* if value1 is CORE,                                                         */
/* or PRE, POST, TOPLEFT, TOPRIGHT, BOOTOMLEFT, or BOTTOMRIGHT if value1 is   */
/* ENCAP.                                                                     */
EXTERN int lefwMacroClass (const char*  value1, const char*  value2);

/* This routine is must be called after lefwInit.                             */
/* This routine can be called multiple times.                                 */
/* Returns 0 if successful.                                                   */
/* This is a 5.8 syntax.                                                      */
EXTERN int lefwMacroFixedMask ();

/* This routine is called after lefwStartMacro.                               */
/* This routine is optional and can be called only once per macro section.    */
/* Returns 0 if successful.                                                   */
/* The value1 can be USER, GENERATE, or BLOCK.                                */
EXTERN int lefwMacroSource (const char*  value1);

/* This routine is called after lefwStartMacro.                               */
/* This routine is optional and can be called multiple times per macro sectio */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroForeign (const char*  name, double  xl, double  yl, int  orient);

/* This routine is called after lefwStartMacro.                               */
/* This routine is optional and can be called multiple times per macro sectio */
/* Returns 0 if successful.                                                   */
/* This routine is the same as lefwMacroForeign, except orient is a char*     */
EXTERN int lefwMacroForeignStr (const char*  name, double  xl, double  yl, const char * orient);

/* This routine is called after lefwStartMacro.                               */
/* This routine is optional and can be called only once per macro section.    */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroOrigin (double  xl, double  yl);

/* This routine is called after lefwStartMacro.                               */
/* This routine is optional and can be called only once per macro section.    */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroEEQ (const char*  macroName);

/* This routine is called after lefwStartMacro.                               */
/* This routine is optional and can be called only once per macro section.    */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroLEQ (const char*  macroName);

/* This routine is called after lefwStartMacro.                               */
/* This routine must be called only once per macro section.                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroSize (double  width, double  height);

/* This routine is called after lefwStartMacro.                               */
/* This routine is optional and can be called only once per macro section.    */
/* Returns 0 if successful.                                                   */
/* The symmetry can be a list of X, Y, or R90                                 */
EXTERN int lefwMacroSymmetry (const char*  symmetry);

/* This routine is called after lefwStartMacro.                               */
/* This routine must be called only once per macro section.                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroSite (const char*  siteName);

/* This routine is called after lefwStartMacro.                               */
/* This routine must be called at least once per macro section.               */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroSitePattern (const char*  name, double  origX, double  origY, int  orient, int  numX, int  numY, double  spaceX, double  spaceY);

/* This routine is called after lefwStartMacro.                               */
/* This routine must be called at least once per macro section.               */
/* Returns 0 if successful.                                                   */
/* This routine is the same as lefwMacroSitePattern, except orient is a char* */
EXTERN int lefwMacroSitePatternStr (const char*  name, double  origX, double  origY, const char * orient, int  numX, int  numY, double  spaceX, double  spaceY);

/* This routine is called after lefwStartMacro.                               */
/* This routine is optional and can be called only once per macro section.    */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPower (double  power);

/* This routine must be called after the lefwStartMacro call (if any).        */
/* The routine can be called only once per macro section.                     */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndMacro (const char*  macroName);

/* This routine is called after lefwStartMacro.                               */
/* This routine is optional and can be called only once per macro section.    */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwStartMacroDensity (const char*  layerName);

/* This routine is called after lefwStartMacroDensity.                        */
/* This routine can be called multiple times.                                 */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroDensityLayerRect (double  x1, double  y1, double  x2, double  y2, double  densityValue);

/* This routine must be called after the lefwStartMacroPin call (if any).     */
/* The routine can be called only once per macro section.                     */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndMacroDensity ();

/* This routine must be called after the lefwStartMacro call (if any).        */
/* The routine can be called multiple time.                                   */
/* Returns 0 if successful.                                                   */
/* It starts the macro pin section within macro.                              */
EXTERN int lefwStartMacroPin (const char*  pinName);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinTaperRule (const char*  ruleName);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinForeign (const char*  name, double  xl, double  yl, int  orient);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* Ths routine is the same as lefwMacroPinForeign, except orient is a char*   */
EXTERN int lefwMacroPinForeignStr (const char*  name, double  xl, double  yl, const char*  orient);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinLEQ (const char*  pinName);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* The direction can be INPUT, OUTPUT, OUTPUT TRISTATE, INOUT, or FEEDTHRU.   */
EXTERN int lefwMacroPinDirection (const char*  direction);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* The use can be SIGNAL, ANALOG, POWER, GROUND, or CLOCK.                    */
EXTERN int lefwMacroPinUse (const char*  use);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* The namce can be ABUTMENT, RING, or FEEDTHRU.                              */
EXTERN int lefwMacroPinShape (const char*  name);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinMustjoin (const char*  name);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinNetExpr (const char*  name);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinSupplySensitivity (const char*  pinName);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* This is a 5.6 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinGroundSensitivity (const char*  pinName);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinOutputnoisemargin (int  high, int  low);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinOutputresistance (int  high, int  low);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinInputnoisemargin (int  high, int  low);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinPower (double  power);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinLeakage (double  leakage);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinCapacitance (double  capacitance);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinResistance (double  resistance);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinPulldownres (double  resistance);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinTieoffr (double  resistance);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinVHI (double  voltage);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinVLO (double  voltage);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinRisevoltagethreshold (double  voltage);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinFallvoltagethreshold (double  voltage);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinRisethresh (double  capacitance);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinFallthresh (double  capacitance);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinRisesatcur (double  current);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinFallsatcur (double  current);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* The name can be ACTIVE or RESISTIVE.                                       */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinCurrentsource (const char*  name);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroPinIV_Tables (const char*  lowName, const char*  highName);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called multiple times.                 */
/* Either this routine or 5.4 Antenna syntax, cannot be both                  */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinAntennasize (double  value, const char*  layerName);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called multiple times.                 */
/* Either this routine or 5.4 Antenna syntax, cannot be both                  */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinAntennaMetalArea (double  value, const char*  layerName);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called multiple times.                 */
/* Either this routine or 5.4 Antenna syntax, cannot be both                  */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinAntennaMetalLength (double  value, const char*  layerName);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called multiple times.                 */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or 5.3 Antenna syntax, cannot be both                  */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinAntennaPartialMetalArea (double  value, const char*  layerName);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called multiple times.                 */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or 5.3 Antenna syntax, cannot be both                  */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinAntennaPartialMetalSideArea (double  value, const char*  layerName);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called multiple times.                 */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or 5.3 Antenna syntax, cannot be both                  */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinAntennaPartialCutArea (double  value, const char*  layerName);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called multiple times.                 */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or 5.3 Antenna syntax, cannot be both                  */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinAntennaDiffArea (double  value, const char*  layerName);

/* This routine is called after lefwStartMacroPin.                            */
/* The oxide value, can be either OXIDE1, OXIDE2, OXIDE3, or OXIDE4.          */
/* This routine is optional. Each oxide value can be called only once         */
/* after the lefwStartMacroPin.                                               */
/* This is a 5.5 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinAntennaModel (const char*  oxide);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called multiple times.                 */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or 5.3 Antenna syntax, cannot be both                  */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinAntennaGateArea (double  value, const char*  layerName);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or 5.3 Antenna syntax, cannot be both                  */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinAntennaMaxAreaCar (double  value, const char*  layerName);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or 5.3 Antenna syntax, cannot be both                  */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinAntennaMaxSideAreaCar (double  value, const char*  layerName);

/* This routine is called after lefwStartMacroPin.                            */
/* This routine is optional and can be called only once.                      */
/* This is a 5.4 syntax.                                                      */
/* Either this routine or 5.3 Antenna syntax, cannot be both                  */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinAntennaMaxCutCar (double  value, const char*  layerName);

/* This routine must be called after the lefwStartMacroPin call (if any).     */
/* The routine can be called only once per macro section.                     */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndMacroPin (const char*  pinName);

/* This routine must be called after the lefwStartMacroPin call (if any).     */
/* The routine can be called multiple time.                                   */
/* Returns 0 if successful.                                                   */
/* It starts the macro pin port section within macro pin.                     */
/* The classType can be NONE or CORE.                                         */
EXTERN int lefwStartMacroPinPort (const char*  classType);

/* This routine is called after lefwStartMacroPinPort.                        */
/* Either this routine or lefwMacroPinPortDesignRuleWidth must be called,     */
/* but can't be both.                                                         */
/* Spacing is optional for minimum spacing.                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinPortLayer (const char*  layerName, double  spacing);

/* This routine is called after lefwStartMacroPinPort.                        */
/* Either this routine or lefwMacroPinPortLayer must be called, but can't     */
/* be both.                                                                   */
/* width is optional for DesignRuleWidth                                      */
/* This is a 5.4 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinPortDesignRuleWidth (const char*  layerName, double  width);

/* This routine is called after lefwMacroPinPortLayer.                        */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinPortLayerWidth (double  width);

/* This routine is called after lefwStartMacroPinPortLayer.                   */
/* Either this routine, lefwStartMacroPinPortLayerRect, or                    */
/* lefwStartMacroPinPortLayerPolygon must be called.                          */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinPortLayerPath (int  num_paths, double*  xl, double*  yl, int  numX, int  numY, double  spaceX, double  spaceY, int     mask);

/* This routine is called after lefwStartMacroPinPortLayer.                   */
/* Either this routine, lefwStartMacroPinPortLayerPath, or                    */
/* lefwStartMacroPinPortLayerPolygon must be called.                          */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinPortLayerRect (double  xl1, double  yl1, double  xl2, double  yl2, int  numX, int  numY, double  spaceX, double  spaceY, int  mask);

/* This routine is called after lefwStartMacroPinPortLayer.                   */
/* Either this routine, lefwStartMacroPinPortLayerPath, or                    */
/* lefwStartMacroPinPortLayerRect must be called.                             */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinPortLayerPolygon (int  num_polys, double*  xl, double*  yl, int  numX, int  numY, double  spaceX, double  spaceY, int  mask);

/* This routine is called after lefwStartMacroPinPort.                        */
/* Either this routine or lefwStartMacroPinPortLayer must be called.          */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroPinPortVia (double  xl, double  yl, const char*  viaName, int  numX, int  numY, double  spaceX, double  spaceY, int  mask);

/* This routine must be called after the lefwStartMacroPinPort call (if any). */
/* The routine can be called only once per macro section.                     */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndMacroPinPort ();

/* This routine is called after the lefwStartMacro call (if any).             */
/* The routine is optional and can be called multiple times.                  */
/* Returns 0 if successful.                                                   */
/* It starts the macro obs section within macro.                              */
EXTERN int lefwStartMacroObs ();

/* This routine is called after lefwStartMacroObs.                            */
/* Either this routine, lefwMacroObsDesignRuleWidth, lefwMacroObsVia or       */
/* lefwMacroExceptPGNet must be called.                                       */
/* Spacing is optional for minimum spacing.                                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroObsLayer (const char*  layerName, double  spacing);

/* This routine is called after lefwStartMacroObs.                            */
/* Either this routine, lefwMacroObsLayer, lefwMacroObsVia or                 */
/* lefwMacroExceptPGNet must be called.                                       */
/* Spacing is optional for minimum spacing.                                   */
/* This is a 5.4 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroObsDesignRuleWidth (const char*  layerName, double  width);

/* This routine is called after lefwStartMacroObs.                            */
/* Either this routine, lefwMacroObsLayer, lefwMacroObsVia or                 */
/* lefwMacroObsDesignRuleWidth must be called.                                */
/* Spacing is optional for minimum spacing.                                   */
/* This is a 5.4 syntax.                                                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroExceptPGNet (const char*  layerName);

/* This routine is called after lefwStartMacroObs.                            */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroObsLayerWidth (double  width);

/* This routine is called after lefwStartMacroObsLayer.                       */
/* Either this routine, lefwMacroObsLayerRect, or                             */
/* lefwStartMacroObsLayerPolygon must be called.                              */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroObsLayerPath (int  num_paths, double*  xl, double*  yl, int  numX, int  numY, double  spaceX, double  spaceY, int  mask);

/* This routine is called after lefwStartMacroObsLayer.                       */
/* Either this routine, lefwMacroObsLayerPath, or                             */
/* lefwStartMacroObsLayerPolygon must be called.                              */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroObsLayerRect (double  xl1, double  yl1, double  xl2, double  yl2, int  numX, int  numY, double  spaceX, double  spaceY, int  mask);

/* This routine is called after lefwStartMacroObsLayer.                       */
/* Either this routine, lefwMacroObsLayerPath, or                             */
/* lefwStartMacroObsLayerPath must be called.                                 */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroObsLayerPolygon (int  num_polys, double*  xl, double*  yl, int  numX, int  numY, double  spaceX, double  spaceY, int  mask);

/* This routine is called after lefwStartMacroObs.                            */
/* Either this routine or lefwMacroObsLayer|lefwMacroObsDesignRuleWidth       */
/* must be called.                                                            */
/* Returns 0 if successful.                                                   */
EXTERN int lefwMacroObsVia (double  xl, double  yl, const char*  viaName, int  numX, int  numY, double  spaceX, double  spaceY, int  mask);

/* This routine must be called after the lefwStartMacroObs call (if any).     */
/* The routine can be called only once per macro section.                     */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndMacroObs ();

/* This routine is called after the lefwStartMacro call (if any).             */
/* The routine is optional and can be called only once.                       */
/* Returns 0 if successful.                                                   */
/* It starts the macro timing section within macro.                           */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwStartMacroTiming ();

/* This routine must be called after the lefwStartMacroTiming.                */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* This routine is for {FROMPIN pinName...; [TOPIN pinName...;] | FROMPIN     */
/* pinName...;}                                                               */
/* This routince can be called multiple times.                                */
/* The num_frPinNames contains the number of object in the array fromPins.    */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroTimingPin (const char*  fromPin, const char*  toPin);

/* This routine must be called after the lefwStartMacroTiming.                */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* The riseFall can be RISE or FALL.                                          */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroTimingIntrinsic (const char*  riseFall, double  min, double  max, double  slewT1, double  slewT1Min, double  slewT1Max, double  slewT2, double  slewT2Min, double  slewT2Max, double  slewT3, double  varMin, double  varMax);

/* This routine must be called after the lefwStartMacroTiming.                */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroTimingRisers (double  min, double  max);

/* This routine must be called after the lefwStartMacroTiming.                */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroTimingFallrs (double  min, double  max);

/* This routine must be called after the lefwStartMacroTiming.                */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroTimingRisecs (double  min, double  max);

/* This routine must be called after the lefwStartMacroTiming.                */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroTimingFallcs (double  min, double  max);

/* This routine must be called after the lefwStartMacroTiming.                */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroTimingRisesatt1 (double  min, double  max);

/* This routine must be called after the lefwStartMacroTiming.                */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroTimingFallsatt1 (double  min, double  max);

/* This routine must be called after the lefwStartMacroTiming.                */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroTimingRiset0 (double  min, double  max);

/* This routine must be called after the lefwStartMacroTiming.                */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroTimingFallt0 (double  min, double  max);

/* This routine must be called after the lefwStartMacroTiming.                */
/* It can be called multiple times.                                           */
/* Returns 0 if successful.                                                   */
/* The unateness can be INVERT, NONINVERT or NONUNATE.                        */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwMacroTimingUnateness (const char*  unateness);

/* This routine must be called after the lefwStartMacroTiming call (if any).  */
/* The routine can be called only once.                                       */
/* Returns 0 if successful.                                                   */
/* This api is obsolete in 5.4.                                               */
EXTERN int lefwEndMacroTiming ();

/* This routine is called after lefwInit.                                     */
/* This routine is optional and it can be called only once.                   */
/* Returns 0 if successful.                                                   */
                                           /* OUTPUTPINANTENNASIZE |          */
                                           /* INOUTPINANTENNASIZE             */
EXTERN int lefwAntenna (const char*  type, double  value);

/* This routine is called after lefwInit.                                     */
/* This routine is optional and it can be called only once.                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwStartBeginext (const char*  name);

/* This routine is called after lefwBeginext.                                 */
/* This routine is optional, it can be called only once.                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwBeginextCreator (const char*  creatorName);

/* This routine is called after lefwBeginext.                                 */
/* This routine is optional, it can be called only once.                      */
/* It gets the current system time and date.                                  */
/* Returns 0 if successful.                                                   */
EXTERN int lefwBeginextDate ();

/* This routine is called after lefwBeginext.                                 */
/* This routine is optional, it can be called only once.                      */
/* Returns 0 if successful.                                                   */
EXTERN int lefwBeginextRevision (int  vers1, int  vers2);

/* This routine is called after lefwBeginext.                                 */
/* This routine is optional, it can be called many times.                     */
/* It allows user to customize their own syntax.                              */
/* Returns 0 if successful.                                                   */
EXTERN int lefwBeginextSyntax (const char*  title, const char*  string);

/* This routine is called after lefwInit.                                     */
/* This routine is optional and it can be called only once.                   */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEndBeginext ();

/* General routines that can be called anytime after the Init is called.      */
EXTERN int lefwCurrentLineNumber ();

/* This routine must call last, it ends the Lef library.                      */
/* It must be called only once.                                               */
/* Returns 0 if successful.                                                   */
EXTERN int lefwEnd ();

/* This routine will print the error message.                                 */
EXTERN void lefwPrintError (int  status);

/* This routine will allow user to write their own comemnt.  It will          */
/* automactically add a # infront of the line.                                */
EXTERN void lefwAddComment (const char*  comment);

/* This routine will indent 3 blank spaces                                    */
EXTERN void lefwAddIndent ();

#endif
