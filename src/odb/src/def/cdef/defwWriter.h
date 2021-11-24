/* ************************************************************************** */
/* ************************************************************************** */
/* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!               */
/* ************************************************************************** */
/* ************************************************************************** */
/* Copyright 2013, Cadence Design Systems                                     */
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


#ifndef CDEFWWRITER_H
#define CDEFWWRITER_H

#include <stdio.h>
#include "defiTypedefs.h"

/* Return codes for writing functions: */
#define DEFW_OK                0
#define DEFW_UNINITIALIZED     1
#define DEFW_BAD_ORDER         2
#define DEFW_BAD_DATA          3
#define DEFW_ALREADY_DEFINED   4
#define DEFW_WRONG_VERSION     5
#define DEFW_OBSOLETE          6
#define DEFW_TOO_MANY_STMS     7  
                                  /* section is smaller than the actual numbe */
                                  /* of statements defined in that section    */

/* orient
   0 = N
   1 = W
   2 = S
   3 = E
   4 = FN
   5 = FW
   6 = FS
   7 = FE
*/

/* This routine will write a new line */
EXTERN int defwNewLine ();

/* The DEF writer initialization.  Must be called first.
 * Either this routine or defwInitCbk should be call only.
 * Can't call both routines in one program.
 * This routine is for user who does not want to use the callback machanism.
 * Returns 0 if successful. */
EXTERN int defwInit (FILE*  f, int  vers1, int  version2, const char*  caseSensitive, const char*  dividerChar, const char*  busBitChars, const char*  designName, const char*  technology, const char*  array, const char*  floorplan, double  units);

/* The DEF writer initialization.  Must be called first.
 * Either this routine or defwInit should be call only.
 * Can't call both routines in one program.
 * This routine is for user who choose to use the callback machanism.
 * If user uses the callback for the writer, they need to provide
 * callbacks for Version, NamesCaseSensitive, BusBitChars and DividerChar.
 * These sections are required by the def.  If any of these callbacks
 * are missing, defaults will be used.
 * Returns 0 if successful. */
EXTERN int defwInitCbk (FILE*  f);

/* This routine must be called after the defwInit.
 * This routine is required.
 * Returns 0 if successful.
 * The routine can be called only once. */
EXTERN int defwVersion (int  vers1, int  vers2);

/* This routine must be called after the defwInit.
 * This routine is required.
 * Returns 0 if successful.
 * The routine can be called only once. */
EXTERN int defwCaseSensitive (const char*  caseSensitive);

/* This routine must be called after the defwInit.
 * This routine is required.
 * Returns 0 if successful.
 * The routine can be called only once. */
EXTERN int defwBusBitChars (const char*  busBitChars);

/* This routine must be called after the defwInit.
 * This routine is required.
 * Returns 0 if successful.
 * The routine can be called only once. */
EXTERN int defwDividerChar (const char*  dividerChar);

/* This routine must be called after the defwInit.
 * This routine is required.
 * Returns 0 if successful.
 * The routine can be called only once. */
EXTERN int defwDesignName (const char*  name);

/* This routine must be called after the defwInit.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called only once. */
EXTERN int defwTechnology (const char*  technology);

/* This routine must be called after the defwInit.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called only once. */
EXTERN int defwArray (const char*  array);

/* This routine must be called after the defwInit.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called only once. */
EXTERN int defwFloorplan (const char*  floorplan);

/* This routine must be called after the defwInit.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called only once. */
EXTERN int defwUnits (int  units);

/* This routine must be called after the defwInit.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called 0 to many times. */
EXTERN int defwHistory (const char*  string);

/* This routine must be called after the history routines (if any).
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called only once. */
EXTERN int defwStartPropDef ();

/* This routine must be called after defwStartPropDef.
 * This routine can be called multiple times.
 * It adds integer property definition to the statement.
 * Returns 0 if successfull.
 * The objType can be LIBRARY or VIA or MACRO or PIN. */
                                      /* NONDEFAULTRULE | MACRO | PIN         */
EXTERN int defwIntPropDef (const char*  objType, const char*  propName, double  leftRange, double  rightRange, int     propValue);

/* This routine must be called after defwStartPropDef.
 * This routine can be called multiple times.
 * It adds real property definition to the statement.
 * Returns 0 if successfull.
 * The objType can be LIBRARY or VIA or MACRO or PIN. */
                                      /* NONDEFAULTRULE | MACRO | PIN         */
EXTERN int defwRealPropDef (const char*  objType, const char*  propName, double  leftRange, double  rightRange, double  propValue);

/* This routine must be called after defwStartPropDef.
 * This routine can be called multiple times.
 * It adds string property definition to the statement.
 * Returns 0 if successfull.
 * The objType can be LIBRARY or VIA or MACRO or PIN. */
                                       /* NONDEFAULTRULE | MACRO | PIN        */
EXTERN int defwStringPropDef (const char*  objType, const char*  propName, double  leftRange, double  rightRange, const char*  propValue);

/* This routine must be called after all the properties have been
 * added to the file.
 * If you called defwPropertyDefinitions then this routine is NOT optional.
 * Returns 0 if successful.
 * The routine can be called only once. */
EXTERN int defwEndPropDef ();

/* This routine can be called after defwRow, defwRegion, defwComponent,
 * defwPin, defwSpecialNet, defwNet, and defwGroup
 * This routine is optional, it adds string property to the statement.
 * Returns 0 if successful.
 * This routine can be called 0 to many times */
EXTERN int defwStringProperty (const char*  propName, const char*  propValue);

/* This routine can be called after defwRow, defwRegion, defwComponent,
 * defwPin, defwSpecialNet, defwNet, and defwGroup
 * This routine is optional, it adds real property to the statement.
 * Returns 0 if successful.
 * This routine can be called 0 to many times */
EXTERN int defwRealProperty (const char*  propName, double  propValue);

/* This routine can be called after defwRow, defwRegion, defwComponent,
 * defwPin, defwSpecialNet, defwNet, and defwGroup
 * This routine is optional, it adds int property to the statement.
 * Returns 0 if successful.
 * This routine can be called 0 to many times */
EXTERN int defwIntProperty (const char*  propName, int  propValue);

/* This routine must be called after the property definitions (if any).
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called only once. */
EXTERN int defwDieArea (int  xl, int  yl, int  xh, int  yh);

/* This routine must be called after the property definitions (if any).
 * This routine is optional.
 * This routine is the same as defwDieArea, but accept more than 2 points
 * This is a 5.6 syntax
 * Returns 0 if successful.
 * The routine can be called only once. */
EXTERN int defwDieAreaList (int  num_points, int*  xl, int*  yh);

/* This routine must be called after the Die Area (if any).
 * This routine is optional.
 * Returns 0 if successful.
 * The integer "orient" and operation of the do is explained in
 * the documentation.
 * In 5.6, the DO syntax is optional and the STEP syntax is optional in DO */
EXTERN int defwRow (const char*  rowName, const char*  rowType, int  x_orig, int  y_orig, int  orient, int  do_count, int  do_increment, int  xstep, int  ystep);

/* This routine must be called after the Die Area (if any).
 * This routine is optional.
 * Returns 0 if successful.
 * This routine is the same as defwRow, excpet orient is a char* */
EXTERN int defwRowStr (const char*  rowName, const char*  rowType, int  x_orig, int  y_orig, const char*  orient, int  do_count, int  do_increment, int  xstep, int  ystep);

/* This routine must be called after the defwRow (if any).
 * This routine is optional.
 * Returns 0 if successful.
 * The operation of the do is explained in the documentation. */
EXTERN int defwTracks (const char*  master, int  doStart, int  doCount, int  doStep, int  numLayers, const char**  layers, int  mask, int  sameMask);

/* This routine must be called after the defwTracks (if any).
 * This routine is optional.
 * Returns 0 if successful.
 * The operation of the do is explained in the documentation. */
EXTERN int defwGcellGrid (const char*  master, int  doStart, int  doCount, int  doStep);

/* This routine must be called after the defwTracks (if any).
 * This section of routines is optional.
 * Returns 0 if successful.
 * The routine starts the default capacitance section.   All of the
 * capacitances must follow.
 * The count is the number of defwDefaultCap calls to follow.
 * The routine can be called only once.
 * This api is obsolete in 5.4. */
EXTERN int defwStartDefaultCap (int  count);

/* This routine is called once for each default cap.  The calls must
 * be preceeded by a call to defwStartDefaultCap and must be
 * terminated by a call to defwEndDefaultCap.
 * Returns 0 if successful.
 * This api is obsolete in 5.4. */
EXTERN int defwDefaultCap (int  pins, double  cap);

/* This routine must be called after the defwDefaultCap calls (if any).
 * Returns 0 if successful.
 * If the count in StartDefaultCap is not the same as the number of
 * calls to DefaultCap then DEFW_BAD_DATA will return returned.
 * The routine can be called only once.
 * This api is obsolete in 5.4. */
EXTERN int defwEndDefaultCap ();

/* This routine must be called after the defwDefaultCap calls (if any).
 * The operation of the do is explained in the documentation.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called many times. */
EXTERN int defwCanPlace (const char*  master, int  xOrig, int  yOrig, int  orient, int  doCnt, int  doInc, int  xStep, int  yStep);

/* This routine must be called after the defwDefaultCap calls (if any).
 * The operation of the do is explained in the documentation.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called many times.
 * This routine is the same as defwCanPlace, except orient is a char* */
EXTERN int defwCanPlaceStr (const char*  master, int  xOrig, int  yOrig, const char*  orient, int  doCnt, int  doInc, int  xStep, int  yStep);

/* This routine must be called after the defwCanPlace calls (if any).
 * The operation of the do is explained in the documentation.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called many times. */
EXTERN int defwCannotOccupy (const char*  master, int  xOrig, int  yOrig, int  orient, int  doCnt, int  doInc, int  xStep, int  yStep);

/* This routine must be called after the defwCanPlace calls (if any).
 * The operation of the do is explained in the documentation.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called many times.
 * This routine is the same as defwCannotOccupy, except orient is a char* */
EXTERN int defwCannotOccupyStr (const char*  master, int  xOrig, int  yOrig, const char*  orient, int  doCnt, int  doInc, int  xStep, int  yStep);

/* This routine must be called after defwCannotOccupy (if any).
 * This section of routines is optional.
 * Returns 0 if successful.
 * The routine starts the via section.   All of the vias must follow.
 * The count is the number of defwVia calls to follow.
 * The routine can be called only once. */
EXTERN int defwStartVias (int  count);

/* These routines enter each via into the file.
 * These routines must be called after the defwStartVias call.
 * defwViaName should be called first, follow either by defwViaPattern or
 * defwViaLayer.  At the end of each via, defwOneViaEnd should be called
 * These routines are for [- viaName [+ PATTERNNAME patternName + RECT layerName
 * pt pt]...;]...
 * Returns 0 if successful.
 * The routines can be called many times. */
EXTERN int defwViaName (const char*  name);

EXTERN int defwViaPattern (const char*  patternName);

/* This routine can be called multiple times. */
/* mask is 5.8 syntax */
EXTERN int defwViaRect (const char*  layerName, int  xl, int  yl, int  xh, int  yh, int  mask);

/* This is a 5.6 syntax
 * This routine can be called multiple times. */
/* mask is 5.8 syntax */
EXTERN int defwViaPolygon (const char*  layerName, int  num_polys, double*  xl, double*  yl, int  mask);

/* These routine must be called after defwViaName.
 * Either this routine or defwViaPattern can be called after each
 * defwViaName is called.
 * This is a 5.6 syntax
 * Returns 0 if successful
 * The routine can be called only once per defwViaName called. */
EXTERN int defwViaViarule (const char*  viaRuleName, double  xCutSize, double  yCutSize, const char*  botMetalLayer, const char*  cutLayer, const char*  topMetalLayer, double  xCutSpacing, double  yCutSpacing, double  xBotEnc, double  yBotEnc, double  xTopEnc, double  yTopEnc);

/* This routine can call only after defwViaViarule.
 * It can only be called once.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
EXTERN int defwViaViaruleRowCol (int  numCutRows, int  numCutCols);

/* This routine can call only after defwViaViarule.
 * It can only be called once.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
EXTERN int defwViaViaruleOrigin (int  xOffset, int  yOffset);

/* This routine can call only after defwViaViarule.
 * It can only be called once.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
EXTERN int defwViaViaruleOffset (int  xBotOffset, int  yBotOffset, int  xTopOffset, int  yTopOffset);

/* This routine can call only after defwViaViarule.
 * It can only be called once.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
EXTERN int defwViaViarulePattern (const char*  cutPattern);

EXTERN int defwOneViaEnd ();

/* This routine must be called after the defwVia calls.
 * Returns 0 if successful.
 * If the count in StartVias is not the same as the number of
 * calls to Via or ViaPattern then DEFW_BAD_DATA will return returned.
 * The routine can be called only once. */
EXTERN int defwEndVias ();

/* This routine must be called after via section (if any).
 * This section of routines is optional.
 * Returns 0 if successful.
 * The routine starts the region section.   All of the regions must follow.
 * The count is the number of defwRegion calls to follow.
 * The routine can be called only once. */
EXTERN int defwStartRegions (int  count);

/* This routine enter each region into the file.
 * This routine must be called after the defwStartRegions call.
 * Returns 0 if successful.
 * The routine can be called many times. */
EXTERN int defwRegionName (const char*  name);

/* This routine enter the region point to the region name.
 * This routine must be called after the defwRegionName call.
 * Returns 0 if successful.
 * The routine can be called many times. */
EXTERN int defwRegionPoints (int  xl, int  yl, int  xh, int  yh);

/* This routine enter the region type, FENCE | GUIDE.
 * This routine must be called after the defwRegionName call.
 * This is a 5.4.1 syntax.
 * Returns 0 if successful.
 * The routine can be called only once. */
EXTERN int defwRegionType (const char*  type);

/* This routine must be called after the defwRegion calls.
 * Returns 0 if successful.
 * If the count in StartRegions is not the same as the number of
 * calls to Region or RegionPattern then DEFW_BAD_DATA will return returned.
 * The routine can be called only once. */
EXTERN int defwEndRegions ();

/* This is a 5.8 syntax.
 * Returns 0 if successful.
 * The routine can be called only once. */
EXTERN int defwComponentMaskShiftLayers (const char**  layerNames, int           numLayerName);

/* This routine must be called after the regions section (if any).
 * This section of routines is NOT optional.
 * Returns 0 if successful.
 * The routine starts the components section. All of the components
 * must follow.
 * The count is the number of defwComponent calls to follow.
 * The routine can be called only once. */
EXTERN int defwStartComponents (int  count);

/* This routine enter each component into the file.
 * This routine must be called after the defwStartComponents call.
 * The optional fields will be ignored if they are set to zero
 * (except for weight which must be set to -1.0).
 * Returns 0 if successful.
 * The routine can be called many times. */

                                            /* USER | TIMING                  */
                                            /* foreignx, foreigny & orients   */
                                            /*  PLACED | UNPLACED             */
                                            /* xh, yh or region               */
EXTERN int defwComponent (const char*  instance, const char*  master, int    numNetName, const char**  netNames, const char*  eeq, const char*  genName, const char*  genParemeters, const char*  source, int  numForeign, const char**  foreigns, int*  foreignX, int*  foreignY, int*  foreignOrients, const char*  status, int  statusX, int  statusY, int  statusOrient, double  weight, const char*  region, int  xl, int  yl, int  xh, int  yh);

/* This routine enter each component into the file.
 * This routine must be called after the defwStartComponents call.
 * The optional fields will be ignored if they are set to zero
 * (except for weight which must be set to -1.0).
 * Returns 0 if successful.
 * The routine can be called many times. 
 * This routine is the same as defwComponent, except orient is a char** */
                                            /*                                */
                                            /* USER | TIMING                  */
                                            /* foreignx, foreigny & orients   */
                                            /* PLACED | UNPLACED              */
                                            /* xh, yh or region               */
EXTERN int defwComponentStr (const char*  instance, const char*  master, int    numNetName, const char**  netNames, const char*  eeq, const char*  genName, const char*  genParemeters, const char*  source, int  numForeign, const char**  foreigns, int*  foreignX, int*  foreignY, const char**  foreignOrients, const char*  status, int  statusX, int  statusY, const char*  statusOrient, double  weight, const char*  region, int  xl, int  yl, int  xh, int  yh);

/* This is a 5.8 syntax.
 * Returns 0 if successful.
 * The routine can be called only once. */
EXTERN int defwComponentMaskShift (int  shiftLayerMasks);

/* This routine must be called after either the defwComponent or
 * defwComponentStr.
 * This routine can only called once per component.
 * Either this routine or defwComponentHaloSoft can be called, but not both
 * This routine is optional.
 * This is a 5.6 syntax.
 * Returns 0 if successful.  */
EXTERN int defwComponentHalo (int  left, int  bottom, int  right, int  top);

/* This routine must be called after either the defwComponent or
 * defwComponentStr.
 * This routine can only called once per component.
 * This routine is just like defwComponentHalo, except it writes the option SOFT
 * Either this routine or defwComponentHalo can be called, but not both
 * This routine is optional.
 * This is a 5.7 syntax.
 * Returns 0 if successful.  */
EXTERN int defwComponentHaloSoft (int  left, int  bottom, int  right, int  top);

/* This routine must be called after either the defwComponent or
 * defwComponentStr.
 * This routine can only called once per component.
 * This routine is optional.
 * This is a 5.7 syntax.
 * Returns 0 if successful.  */
EXTERN int defwComponentRouteHalo (int  haloDist, const char*  minLayer, const char*  maxLayer);

/* This routine must be called after the defwComponent calls.
 * Returns 0 if successful.
 * If the count in StartComponents is not the same as the number of
 * calls to Component then DEFW_BAD_DATA will return returned.
 * The routine can be called only once. */
EXTERN int defwEndComponents ();

/* This routine must be called after the components section (if any).
 * This section of routines is optional.
 * Returns 0 if successful.
 * The routine starts the pins section. All of the pins must follow.
 * The count is the number of defwPin calls to follow.
 * The routine can be called only once. */
EXTERN int defwStartPins (int  count);

/* This routine enter each pin into the file.
 * This routine must be called after the defwStartPins call.
 * The optional fields will be ignored if they are set to zero.
 * Returns 0 if successful.
 * The routine can be called many times.
 * NOTE: Use defwPinLayer to write out layer with SPACING or DESIGNRULEWIDTH */
                                         /* INOUT | FEEDTHRU                  */
                                         /* GROUND | CLOCK | TIEOFF | ANALOG  */
                                         /* COVER                             */
EXTERN int defwPin (const char*  name, const char*  net, int  special, const char*  direction, const char*  use, const char*  status, int  statusX, int  statusY, int  orient, const char*  layer, int  xl, int  yl, int  xh, int  yh);

/* This routine enter each pin into the file.
 * This routine must be called after the defwStartPins call.
 * The optional fields will be ignored if they are set to zero.
 * Returns 0 if successful.
 * The routine can be called many times.
 * This routine is the same as defwPin, except orient is a char*
 * NOTE: Use defwPinLayer to write out layer with SPACING or DESIGNRULEWIDTH */
                                         /* INOUT | FEEDTHRU                  */
                                         /* GROUND | CLOCK | TIEOFF | ANALOG  */
                                         /* COVER                             */
EXTERN int defwPinStr (const char*  name, const char*  net, int  special, const char*  direction, const char*  use, const char*  status, int  statusX, int  statusY, const char*  orient, const char*  layer, int  xl, int  yl, int  xh, int  yh);

/* This routine should be called if the layer has either SPACING or
 * DESIGNRULEWIDTH.  If this routine is used and the pin has only one
 * layer, the layer in defwPin or defwPinStr has to be null, otherwise
 * the layer will be written out twice.
 * This routine must be called after defwPin or defwPinStr.
 * This is a 5.6 syntax.
 * This routine is optional.
 * Returns 0 if successful.
 * This routine can be called multiple times within a pin. */
EXTERN int defwPinLayer (const char*  layerName, int  spacing, int  designRuleWidth, int  xl, int  yl, int  xh, int  yh, int  mask);

/* This routine must be called after defwPin or defwPinStr.
 * This routine is to write out layer with polygon.
 * This is a 5.6 syntax.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called multiple times within a pin. */
EXTERN int defwPinPolygon (const char*  layerName, int  spacing, int  designRuleWidth, int  num_polys, double*  xl, double*  yl, int  mask);

/* This routine must be called after defwPin or defwPinStr.
 * This routine is to write out layer with via.
 * This is a 5.7 syntax.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called multiple times within a pin. */
EXTERN int defwPinVia (const char*  viaName, int  xl, int  yl, int  mask);

/* This routine must be called after defwPin or defwPinStr.
 * This routine is to write out pin with port.
 * This is a 5.7 syntax.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called multiple times within a pin. */
EXTERN int defwPinPort ();

/* This routine is called after defwPinPort. 
 * This is a 5.7 syntax.
 * This routine is optional.
 * Returns 0 if successful.
 * This routine can be called multiple times within a pin. */
EXTERN int defwPinPortLayer (const char*  layerName, int  spacing, int  designRuleWidth, int  xl, int  yl, int  xh, int  yh, int  mask);

/* This routine must be called after defwPinPort.
 * This is a 5.7 syntax.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called multiple times within a pin. */
EXTERN int defwPinPortPolygon (const char*  layerName, int  spacing, int  designRuleWidth, int  num_polys, double*  xl, double*  yl, int  mask);

/* This routine must be called after defwPinPort.
 * This is a 5.7 syntax.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called multiple times within a pin. */
EXTERN int defwPinPortVia (const char*  viaName, int  xl, int  yl, int  mask);

/* This routine must be called after defwPinPort.
 * This is a 5.7 syntax.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called many times.
 * NOTE: Use defwPinLayer to write out layer with SPACING or DESIGNRULEWIDTH */
EXTERN int defwPinPortLocation (const char*  status, int  statusX, int  statusY, const char*  orient);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.6 syntax.
 * The routine can be called only once per pin. */
EXTERN int defwPinNetExpr (const char*  pinExpr);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.6 syntax.
 * The routine can be called only once per pin. */
EXTERN int defwPinSupplySensitivity (const char*  pinName);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.6 syntax.
 * The routine can be called only once per pin. */
EXTERN int defwPinGroundSensitivity (const char*  pinName);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.4 syntax.
 * The routine can be called multiple times. */
EXTERN int defwPinAntennaPinPartialMetalArea (int  value, const char*  layerName);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.4 syntax.
 * The routine can be called multiple times. */
EXTERN int defwPinAntennaPinPartialMetalSideArea (int  value, const char*  layerName);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.4 syntax.
 * The routine can be called multiple times. */
EXTERN int defwPinAntennaPinPartialCutArea (int  value, const char*  layerName);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.4 syntax.
 * The routine can be called multiple times. */
EXTERN int defwPinAntennaPinDiffArea (int  value, const char*  layerName);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.5 syntax.
 * The oxide can be either OXIDE1, OXIDE2, OXIDE3, or OXIDE4.
 * Each oxide value can be called only once after defwPin. */
EXTERN int defwPinAntennaModel (const char*  oxide);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.4 syntax.
 * The routine can be called multiple times. */
EXTERN int defwPinAntennaPinGateArea (int  value, const char*  layerName);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.4 syntax.
 * The routine can be called multiple times. */
EXTERN int defwPinAntennaPinMaxAreaCar (int  value, const char*  layerName);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.4 syntax.
 * The routine can be called multiple times. */
EXTERN int defwPinAntennaPinMaxSideAreaCar (int  value, const char*  layerName);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.4 syntax.
 * The routine can be called multiple times. */
EXTERN int defwPinAntennaPinMaxCutCar (int  value, const char*  layerName);

/* This routine must be called after the defwPin calls.
 * Returns 0 if successful.
 * If the count in StartPins is not the same as the number of
 * calls to Pin then DEFW_BAD_DATA will return returned.
 * The routine can be called only once. */
EXTERN int defwEndPins ();

/* This routine must be called after the pin section (if any).
 * This section of routines is optional.
 * Returns 0 if successful.
 * The routine starts the pinproperties section. All of the pinproperties
 * must follow.
 * The count is the number of defwPinProp calls to follow.
 * The routine can be called only once. */
EXTERN int defwStartPinProperties (int  count);

/* This routine enter each pinproperty into the file.
 * This routine must be called after the defwStartPinProperties call.
 * The optional fields will be ignored if they are set to zero.
 * Returns 0 if successful.
 * The routine can be called many times. */
EXTERN int defwPinProperty (const char*  name, const char*  pinName);

/* This routine must be called after the defwPinProperty calls.
 * Returns 0 if successful.
 * If the count in StartPins is not the same as the number of
 * calls to Pin then DEFW_BAD_DATA will return returned.
 * The routine can be called only once. */
EXTERN int defwEndPinProperties ();

/* Routines to enter a special net or nets into the file.
 * You must first call defwStartSpecialNets with the number of
 * nets. This section is required, even if you do not have any nets.
 * For each net you should call defwSpecialNet followed by
 * one or more defwSpecialNetConnection calls.
 * After the connections come the options.  Options are
 * NOT required.
 * Each net is completed by calling defwSpecialNetEndOneNet().
 * The nets section is finished by calling defwEndNets(). */
EXTERN int defwStartSpecialNets (int  count);

/* This routine must be called after the defwStartSpecialNets it is for
 * - netName */
EXTERN int defwSpecialNet (const char*  name);

/* This routine is for compNameRegExpr, pinName, and SYNTHESIZED */
/* It can be called multiple times */
EXTERN int defwSpecialNetConnection (const char*  inst, const char*  pin, int  synthesized);

/* This routine is for + FIXEDBUMP
 * This is a 5.4.1 syntax */
EXTERN int defwSpecialNetFixedbump ();

/* This routine is for + VOLTAGE volts */
EXTERN int defwSpecialNetVoltage (double  v);

/* This routine is for + SPACING layerName spacing [RANGE minwidth maxwidth */
EXTERN int defwSpecialNetSpacing (const char*  layer, int  spacing, double  minwidth, double  maxwidth);

/* This routine is for + WIDTH layerName width */
EXTERN int defwSpecialNetWidth (const char*  layer, int  width);

/* This routine is for + SOURCE {NETLIST | DIST | USER | TIMING} */
EXTERN int defwSpecialNetSource (const char*  name);

/* This routine is for + ORIGINAL netName */
EXTERN int defwSpecialNetOriginal (const char*  name);

/* This routine is for + PATTERN {STEINER | BALANCED | WIREDLOGIC | TRUNK} */
EXTERN int defwSpecialNetPattern (const char*  name);

/* This routine is for + USE {SIGNAL | POWER | GROUND | CLOCK | TIEOFF |
   ANALOG | SCAN | RESET} */
EXTERN int defwSpecialNetUse (const char*  name);

/* This routine is for + WEIGHT weight */
EXTERN int defwSpecialNetWeight (double  value);

/* This routine is for + ESTCAP wireCapacitance */
EXTERN int defwSpecialNetEstCap (double  value);

/* Paths are a special type of option.  A path must begin
 * with a defwSpecialNetPathStart and end with a defwSpecialNetPathEnd().
 * The individual parts of the path can be entered in
 * any order. */
EXTERN int defwSpecialNetPathStart (const char*  typ);
                                                     /* SHIELD | NEW          */
EXTERN int defwSpecialNetShieldNetName (const char*  name);

EXTERN int defwSpecialNetPathLayer (const char*  name);

EXTERN int defwSpecialNetPathWidth (int  width);

/* This routine is optional.
 * This is a 5.6 syntax. */
EXTERN int defwSpecialNetPathStyle (int  styleNum);

EXTERN int defwSpecialNetPathShape (const char*  shapeType);
        /* FOLLOWPIN | IOWIRE | COREWIRE | BLOCKWIRE | FILLWIRE | BLOCKAGEWIR */

/* This routine is optional.
   This is a 5.8 syntax.
 * Returns 0 if successful. */
EXTERN int defwSpecialNetPathMask (int  colorMask);

/* x and y location of the path */
EXTERN int defwSpecialNetPathPoint (int  numPts, double*  pointx, double*  pointy);
EXTERN int defwSpecialNetPathVia (const char*  name);

/* This routine is called after defwSpecialNetPath
 * This is a 5.4.1 syntax */
EXTERN int defwSpecialNetPathViaData (int  numX, int  numY, int  stepX, int  stepY);

/* x and y location of the path */
EXTERN int defwSpecialNetPathPointWithWireExt (int  numPts, double*  pointx, double*  pointy, double*  optValue);

EXTERN int defwSpecialNetPathEnd ();

/* This is a 5.6 syntax
 * This routine can be called multiple times. */
EXTERN int defwSpecialNetPolygon (const char*  layerName, int  num_polys, double*  xl, double*  yl);

/* This is a 5.6 syntax
 * This routine can be called multiple times. */
EXTERN int defwSpecialNetRect (const char*  layerName, int  xl, int  yl, int  xh, int  yh);

EXTERN int defwSpecialNetVia (const char*  layerName);

EXTERN int defwSpecialNetViaWithOrient (const char*  layerName, int  orient);

EXTERN int defwSpecialNetViaPoints (int  num_points, double*  xl, double*  yl);

/* This routine is called at the end of each net */
EXTERN int defwSpecialNetEndOneNet ();

/* 5.3 for special net */
/* Shields are a special type of option.  A shield must begin
 * with a defwSpecialNetShieldStart and end with a defwSpecialNetShieldEnd().
 * The individual parts of the shield can be entered in
 * any order. */
EXTERN int defwSpecialNetShieldStart (const char*  name);

EXTERN int defwSpecialNetShieldLayer (const char*  name);
EXTERN int defwSpecialNetShieldWidth (int  width);
EXTERN int defwSpecialNetShieldShape (const char*  shapeType);
        /* FOLLOWPIN | IOWIRE | COREWIRE | BLOCKWIRE | FILLWIRE | BLOCKAGEWIR */

/* x and y location of the path */
EXTERN int defwSpecialNetShieldPoint (int  numPts, double*  pointx, double*  pointy);
EXTERN int defwSpecialNetShieldVia (const char*  name);

/* A 5.4.1 syntax */
EXTERN int defwSpecialNetShieldViaData (int  numX, int  numY, int  stepX, int  stepY);
EXTERN int defwSpecialNetShieldEnd ();
/* end 5.3 */

/* This routine is called at the end of the special net section */
EXTERN int defwEndSpecialNets ();

/* Routines to enter a net or nets into the file.
 * You must first call defwNets with the number of nets.
 * This section is required, even if you do not have any nets.
 * For each net you should call defwNet followed by one or
 * more defwNetConnection calls.
 * After the connections come the options.  Options are
 * NOT required.
 * Each net is completed by calling defwNetEndOneNet().
 * The nets section is finished by calling defwEndNets(). */
EXTERN int defwStartNets (int  count);

/* This routine must be called after the defwStartNets, it is for - netName */
EXTERN int defwNet (const char*  name);

/* This routine is for { compName | PIN } pinName [+ SYNTHESIZED] */
/* It can be called multiple times */
EXTERN int defwNetConnection (const char*  inst, const char*  pin, int  synthesized);

/* This routine is for MUSTJOIN, compName, pinName */
EXTERN int defwNetMustjoinConnection (const char*  inst, const char*  pin);

/* This routine is for + VPIN vpinName [LAYER layerName pt pt
 * [{ PLACED | FIXED | COVER } pt orient] */
EXTERN int defwNetVpin (const char*  vpinName, const char*  layerName, int  layerXl, int  layerYl, int  layerXh, int  layerYh, const char*  status, int  statusX, int  statusY, int  orient);

/* This routine is for + VPIN vpinName [LAYER layerName pt pt
 * [{ PLACED | FIXED | COVER } pt orient]
 * This routine is the same as defwNetVpin, except orient is a char* */
EXTERN int defwNetVpinStr (const char*  vpinName, const char*  layerName, int  layerXl, int  layerYl, int  layerXh, int  layerYh, const char*  status, int  statusX, int  statusY, const char*  orient);

/* This routine can be called either within net or subnet.
 * it is for NONDEFAULTRULE rulename */
EXTERN int defwNetNondefaultRule (const char*  name);

/* This routine is for + XTALK num */
EXTERN int defwNetXtalk (int  xtalk);

/* This routine is for + FIXEDBUMP
 * This is a 5.4.1 syntax */
EXTERN int defwNetFixedbump ();

/* This routine is for + FREQUENCY
 * This is a 5.4.1 syntax */
EXTERN int defwNetFrequency (double  frequency);

/* This routine is for + SOURCE {NETLIST | DIST | USER | TEST | TIMING} */
EXTERN int defwNetSource (const char*  name);

/* This routine is for + ORIGINAL netname */
EXTERN int defwNetOriginal (const char*  name);

/* This routine is for + USE {SIGNAL | POWER | GROUND | CLOCK | TIEOFF | 
 * ANALOG} */
EXTERN int defwNetUse (const char*  name);

/* This routine is for + PATTERN {STEINER | BALANCED | WIREDLOGIC} */
EXTERN int defwNetPattern (const char*  name);

/* This routine is for + ESTCAP wireCapacitance */
EXTERN int defwNetEstCap (double  value);

/* This routine is for + WEIGHT weight */
EXTERN int defwNetWeight (double  value);

/* 5.3 for net */
/* This routine is for + SHIELDNET weight */
EXTERN int defwNetShieldnet (const char*  name);

/* Noshield are a special type of option.  A noshield must begin
 * with a defwNetNoshieldStart and end with a defwNetNoshieldEnd().
 * The individual parts of the noshield can be entered in
 * any order. */
EXTERN int defwNetNoshieldStart (const char*  name);

/* x and y location of the path */
EXTERN int defwNetNoshieldPoint (int  numPts, const char**  pointx, const char**  pointy);
EXTERN int defwNetNoshieldVia (const char*  name);
EXTERN int defwNetNoshieldEnd ();
/* end 5.3 */

/* Subnet are a special type of option. A subnet must begin
 * with a defwNetSubnetStart and end with a defwNetSubnetEnd().
 * Routines to call within the subnet are: defwNetSubnetPin,
 * defwNetNondefaultRule and defwNetPathStart... */
EXTERN int defwNetSubnetStart (const char*  name);

/* This routine is called after the defwNetSubnet, it is for
 * [({compName | PIN} pinName) | (VPIN vpinName)]... */
EXTERN int defwNetSubnetPin (const char*  compName, const char*  pinName);

EXTERN int defwNetSubnetEnd ();

/* Paths are a special type of option.  A path must begin
 * with a defwNetPathStart and end with a defwPathEnd().
 * The individual parts of the path can be entered in
 * any order. */
EXTERN int defwNetPathStart (const char*  typ);
                                              /* NOSHIELD | NEW               */
EXTERN int defwNetPathWidth (int  w);
EXTERN int defwNetPathLayer (const char*  name, int  isTaper, const char*  rulename);
                                              /*rulename can be assigned */
/* This routine is optional.
 * This is a 5.6 syntax. */
EXTERN int defwNetPathStyle (int  styleNum);

/* This routine is optional.
 * This is a 5.8 syntax. */
EXTERN int defwNetPathMask (int  maskNum);

EXTERN int defwNetPathRect (int  deltaX1, int  deltaY1, int  deltaX2, int  deltaY2);

EXTERN int defwNetPathVirtual (int  x, int  y);

/* x and y location of the path */
EXTERN int defwNetPathPoint (int  numPts, double*  pointx, double*  pointy);

EXTERN int defwNetPathPointWithExt (int  numPts, double*  pointx, double*  pointy, double*  optValue);

EXTERN int defwNetPathVia (const char*  name);

EXTERN int defwNetPathViaWithOrient (const char*  name, int  orient);

EXTERN int defwNetPathViaWithOrientStr (const char*  name, const char*  orient);
EXTERN int defwNetPathEnd ();

/* This routine is called at the end of each net */
EXTERN int defwNetEndOneNet ();

/* This routine is called at the end of the net section */
EXTERN int defwEndNets ();

/* This section of routines is optional.
 * Returns 0 if successful.
 * The routine starts the I/O Timing section. All of the iotimings options
 * must follow.
 * The count is the number of defwIOTiming calls to follow.
 * The routine can be called only once.
 * This api is obsolete in 5.4. */
EXTERN int defwStartIOTimings (int  count);

/* This routine can be called after defwStaratIOTiming
 * It is for - - {(comp pin) | (PIN name)}
 * This api is obsolete in 5.4. */
EXTERN int defwIOTiming (const char*  inst, const char*  pin);

/* This routine is for + { RISE | FALL } VARIABLE min max
 * This api is obsolete in 5.4. */
EXTERN int defwIOTimingVariable (const char*  riseFall, int  num1, int  num2);

/* This routine is for + { RISE | FALL } SLEWRATE min max
 * This api is obsolete in 5.4. */
EXTERN int defwIOTimingSlewrate (const char*  riseFall, int  num1, int  num2);

/* This routine is for + DRIVECELL macroName [[FROMPIN pinName] TOPIN pinName]
 * [PARALLEL numDrivers]
 * This api is obsolete in 5.4. */
EXTERN int defwIOTimingDrivecell (const char*  name, const char*  fromPin, const char*  toPin, int  numDrivers);

/* This routine is for + CAPACITANCE capacitance
 * This api is obsolete in 5.4. */
EXTERN int defwIOTimingCapacitance (double  num);

/* This api is obsolete in 5.4. */
EXTERN int defwEndIOTimings ();

/* Routines to enter scan chains.  This section is optional
 * The section must start with a defwStartScanchains() call and
 * end with a defwEndScanchain() call.
 * Each scan chain begins with a defwScanchain() call.
 * The rest of the calls follow.  */
EXTERN int defwStartScanchains (int  count);

/* This routine can be called after defwStartScanchains
 * It is for - chainName */
EXTERN int defwScanchain (const char*  name);

/* This routine is for + COMMONSCANPINS [IN pin] [OUT pin] */
EXTERN int defwScanchainCommonscanpins (const char*  inst1, const char*  pin1, const char*  inst2, const char*  pin2);

/* This routine is for + PARTITION paratitionName [MAXBITS maxBits] */
/* This is 5.4.1 syntax */
EXTERN int defwScanchainPartition (const char*  name, int  maxBits);

/* This routine is for + START {fixedInComp | PIN } [outPin] */
EXTERN int defwScanchainStart (const char*  inst, const char*  pin);

/* This routine is for + STOP {fixedOutComp | PIN } [inPin] */
EXTERN int defwScanchainStop (const char*  inst, const char*  pin);

/* This routine is for + FLOATING {floatingComp [IN pin] [OUT pin]}
 * This is a 5.4.1 syntax */
EXTERN int defwScanchainFloating (const char*  name, const char*  inst1, const char*  pin1, const char*  inst2, const char*  pin2);

/* This routine is for + FLOATING {floatingComp [IN pin] [OUT pin]}
 * This is a 5.4.1 syntax.
 * This routine is the same as defwScanchainFloating.  But also added
 * the option BITS. */
EXTERN int defwScanchainFloatingBits (const char*  name, const char*  inst1, const char*  pin1, const char*  inst2, const char*  pin2, int    bits);

/* This routine is for + ORDERED {fixedComp [IN pin] [OUT pin]
 * fixedComp [IN pin] [OUT pin].
 * When this routine is called for the 1st time within a scanchain, 
 * both name1 and name2 are required.  Only name1 is required is the
 * routine is called more than once. */
EXTERN int defwScanchainOrdered (const char*  name1, const char*  inst1, const char*  pin1, const char*  inst2, const char*  pin2, const char*  name2, const char*  inst3, const char*  pin3, const char*  inst4, const char*  pin4);

/* This routine is for + ORDERED {fixedComp [IN pin] [OUT pin]
 * fixedComp [IN pin] [OUT pin].
 * When this routine is called for the 1st time within a scanchain, 
 * both name1 and name2 are required.  Only name1 is required is the
 * routine is called more than once.
 * This is a 5.4.1 syntax.
 * This routine is the same as defwScanchainOrdered.  But also added
 * the option BITS */
EXTERN int defwScanchainOrderedBits (const char*  name1, const char*  inst1, const char*  pin1, const char*  inst2, const char*  pin2, int    bits1, const char*  name2, const char*  inst3, const char*  pin3, const char*  inst4, const char*  pin4, int    bits2);

EXTERN int defwEndScanchain ();

/* Routines to enter constraints.  This section is optional
 * The section must start with a defwStartConstrains() call and
 * end with a defwEndConstraints() call.
 * Each contraint will call the defwConstraint...().
 * This api is obsolete in 5.4. */
EXTERN int defwStartConstraints (int  count);

/* The following routines are for - {operand [+ RISEMAX time] [+ FALLMAX time]
 * [+ RISEMIN time] [+ FALLMIN time] | WIREDLOGIC netName MAXDIST distance };}
 * operand - NET netName | PATH comp fromPin comp toPin | SUM (operand, ...)
 * The following apis are obsolete in 5.4. */ 
EXTERN int defwConstraintOperand ();
EXTERN int defwConstraintOperandNet (const char*  netName);
EXTERN int defwConstraintOperandPath (const char*  comp1, const char*  fromPin, const char*  comp2, const char*  toPin);
EXTERN int defwConstraintOperandSum ();
EXTERN int defwConstraintOperandSumEnd ();
EXTERN int defwConstraintOperandTime (const char*  timeType, int  time);
EXTERN int defwConstraintOperandEnd ();

/* This routine is for - WIRELOGIC netName MAXDIST distance */
EXTERN int defwConstraintWiredlogic (const char*  netName, int  distance);

EXTERN int defwEndConstraints ();

/* Routines to enter groups.  This section is optional
 * The section must start with a defwStartGroups() call and
 * end with a defwEndGroups() call.
 * Each group will call the defwGroup...(). */
EXTERN int defwStartGroups (int  count);

/* This routine is for - groupName compNameRegExpr ... */
EXTERN int defwGroup (const char*  groupName, int  numExpr, const char**  groupExpr);

/* This routine is for + SOFT [MAXHALFPERIMETER value] [MAXX value]
 * [MAXY value] */
EXTERN int defwGroupSoft (const char*  type1, double  value1, const char*  type2, double  value2, const char*  type3, double  value3);

/* This routine is for + REGION {pt pt | regionName} */
EXTERN int defwGroupRegion (int  xl, int  yl, int  xh, int  yh, const char*  regionName);

EXTERN int defwEndGroups ();

/* Routines to enter Blockages.  This section is optional
 * The section must start with a defwStartBlockages() call and
 * end with a defwEndBlockages() call.
 * Each blockage will call the defwBlockages...().
 * This is a 5.4 syntax. */
EXTERN int defwStartBlockages (int  count);

/* This routine is for - layerName
* This routine is called per entry within a blockage for layer.
* This is a 5.4 syntax. */
EXTERN int defwBlockagesLayer (const char*  layerName);

/* This routine is for - slots 
* This routine is called per entry within a blockage layer, can't be more then one.
* This is a 5.4 syntax. */
EXTERN int defwBlockagesLayerSlots ();

/* This routine is for - fills
* This routine is called per entry within a blockage layer, can't be more then one.
* This is a 5.4 syntax. */
EXTERN int defwBlockagesLayerFills ();

/* This routine is for - pushdown
* This routine is called per entry within a blockage layer, can't be more then one.
* This is a 5.4 syntax. */
EXTERN int defwBlockagesLayerPushdown ();

/* This routine is for - exceptpgnet
* This routine is called per entry within a blockage layer, can't be more then one.
* This is a 5.7 syntax. */
EXTERN int defwBlockagesLayerExceptpgnet ();

/* This routine is for - component
* This routine called per entry within a blockage layer, can't be more than one.
* This is a 5.6 syntax. */
EXTERN int defwBlockagesLayerComponent (const char*  compName);

/* This routine is for - spacing
* Either this routine or defwBlockagesDesignRuleWidth is called per entry
* within a blockage layer, can't be more than one.
* This is a 5.6 syntax. */
EXTERN int defwBlockagesLayerSpacing (int  minSpacing);

/* This routine is for - designrulewidth
* Either this routine or defwBlockagesSpacing is called per entry
* within a blockage layer, can't be more than one.
* This is a 5.6 syntax. */
EXTERN int defwBlockagesLayerDesignRuleWidth (int  effectiveWidth);

/* This routine is for - mask.
* This routine called per entry within a blockage layer, can't be more than one.
* This is a 5.8 syntax. */
EXTERN int defwBlockagesLayerMask (int  maskColor);

/* This routine is for - layerName & compName
 * Either this routine, defBlockageLayerSlots, defBlockageLayerFills,
 * or defwBlockagePlacement is called per entry within
 * a blockage, can't be more then one.
 * This is a 5.4 syntax. */
EXTERN int defwBlockageLayer (const char*  layerName, const char*  compName);

/* This routine is for - layerName & slots 
 * Either this routine, defBlockageLayer, defBlockageLayerFills,
 * defwBlockagePlacement, or defwBlockagePushdown is called per entry within
 * a blockage, can't be more then one.
 * This is a 5.4 syntax. */
EXTERN int defwBlockageLayerSlots (const char*  layerName);

/* This routine is for - layerName & fills
 * Either this routine, defBlockageLayer, defBlockageLayerSlots,
 * defwBlockagePlacement, or defwBlockagePushdown is called per entry within
 * a blockage, can't be more then one.
 * This is a 5.4 syntax. */
EXTERN int defwBlockageLayerFills (const char*  layerName);

/* This routine is for - layerName & pushdown
 * Either this routine, defBlockageLayer, defBlockageLayerSlots,
 * defwBlockagePlacement, or defwBlockageFills is called per entry within
 * a blockage, can't be more then one.
 * This is a 5.4 syntax. */
EXTERN int defwBlockageLayerPushdown (const char*  layerName);

/* This routine is for - exceptpgnet
 * Either this routine, defBlockageLayer, defBlockageLayerSlots,
 * defwBlockagePlacement, or defwBlockageFills is called per entry within
 * a blockage, can't be more then one.
 * This is a 5.7 syntax. */
EXTERN int defwBlockageLayerExceptpgnet (const char*  layerName);

/* This routine is for - spacing
 * Either this routine or defwBlockageDesignRuleWidth is called per entry
 * within a blockage, can't be more than one.
 * This is a 5.6 syntax. */
EXTERN int defwBlockageSpacing (int  minSpacing);

/* This routine is for - designrulewidth
 * Either this routine or defwBlockageSpacing is called per entry
 * within a blockage, can't be more than one.
 * This is a 5.6 syntax. */
EXTERN int defwBlockageDesignRuleWidth (int  effectiveWidth);

/* This routine is for - placement
 * This routine is called per entry within blockage for placement.
 * This is a 5.4 syntax.
 * 11/25/2002 - bug fix: submitted by Craig Files (cfiles@ftc.agilent.com)
 * this routine allows to call blockage without a component. */
EXTERN int defwBlockagesPlacement ();

/* This routine is for - component
* This routine is called per entry within blockage placement, can't be more then one.
* This is a 5.4 syntax. */
EXTERN int defwBlockagesPlacementComponent (const char*  compName);

/* This routine is for - Pushdown
* This routine is called per entry within blockage placement, can't be more then one.
* This is a 5.4 syntax. */
EXTERN int defwBlockagesPlacementPushdown ();

/* This routine is for - soft
* Either this routine or defwBlockagesPlacementPartial
* is called per entry within blockage placement, can't be more then one.
* This is a 5.7 syntax. */
EXTERN int defwBlockagesPlacementSoft ();

/* This routine is for - Partial
* Either this routine or defwBlockagesPlacementSoft
* is called per entry within blockage placement, can't be more then one.
* This is a 5.7 syntax. */
EXTERN int defwBlockagesPlacementPartial (double  maxDensity);

/* This routine is for rectangle.
* This routine is optional and can be called multiple time. 
* This is a 5.4 syntax. */
EXTERN int defwBlockagesRect (int  xl, int  yl, int  xh, int  yh);

/* This routine is for polygon.
* This routine is optional and can be called multiple time. 
* This is a 5.6 syntax. */
EXTERN int defwBlockagesPolygon (int  num_polys, int*  xl, int*  yl);

/* This routine is for - placement
* Either this routine or defBlockageLayer
* is called per entry within blockage, can't be more then one.
* This is a 5.4 syntax.
* 11/25/2002 - bug fix: submitted by Craig Files (cfiles@ftc.agilent.com)
* this routine allows to call blockage without a component. */
EXTERN int defwBlockagePlacement ();

/* This routine is for - placement & component
 * Either this routine or defwBlockagePlacementPushdown
 * is called per entry within blockage, can't be more then one.
 * This is a 5.4 syntax. */
EXTERN int defwBlockagePlacementComponent (const char*  compName);

/* This routine is for - placement & Pushdown
 * Either this routine or defwBlockagePlacementComponent
 * is called per entry within blockage, can't be more then one.
 * This is a 5.4 syntax. */
EXTERN int defwBlockagePlacementPushdown ();

/* This routine is for - placement & soft
 * Either this routine or defwBlockagePlacementPushdown
 * is called per entry within blockage, can't be more then one.
 * This is a 5.7 syntax. */
EXTERN int defwBlockagePlacementSoft ();

/* This routine is for - placement & Partial
 * Either this routine or defwBlockagePlacementComponent
 * is called per entry within blockage, can't be more then one.
 * This is a 5.7 syntax. */
EXTERN int defwBlockagePlacementPartial (double  maxDensity);

/* This routine is optional.
 * This is a 5.8 syntax. */
EXTERN int defwBlockageMask (int  maskColor);

/* This routine is for rectangle.
 * This is a 5.4 syntax. */
EXTERN int defwBlockageRect (int  xl, int  yl, int  xh, int  yh);

/* This routine is for polygon.
 * This routine is optinal and can be called multiple time. 
 * This is a 5.6 syntax. */
EXTERN int defwBlockagePolygon (int  num_polys, int*  xl, int*  yl);

/* This is a 5.4 syntax. */
EXTERN int defwEndBlockages ();

/* Routines to enter Slots.  This section is optional
 * The section must start with a defwStartSlots() call and
 * end with a defwEndSlots() call.
 * Each slots will call the defwSlots...().
 * This is a 5.4 syntax. */
EXTERN int defwStartSlots (int  count);

/* This routine is for - layerName & compName
 * Either this routine, defSlots, defSlotsLayerFills,
 * or defwSlotsPlacement is called per entry within
 * a slot, can't be more then one.
 * This is a 5.4 syntax. */
EXTERN int defwSlotLayer (const char*  layerName);

/* This routine is for rectangle
 * This is a 5.4 syntax. */
EXTERN int defwSlotRect (int  xl, int  yl, int  xh, int  yh);

/* This routine is for rectangle
 * This is a 5.6 syntax and can be called multiple time. */
EXTERN int defwSlotPolygon (int  num_polys, double*  xl, double*  yl);

/* This is a 5.4 syntax. */
EXTERN int defwEndSlots ();

/* Routines to enter Fills.  This section is optional
 * The section must start with a defwStartFills() call and
 * end with a defwEndFills() call.
 * Each fills will call the defwFills...().
 * This is a 5.4 syntax. */
EXTERN int defwStartFills (int  count);

/* This routine is for - layerName & compName
 * Either this routine, defFills, defFillsLayerFills,
 * or defwFillsPlacement is called per entry within
 * a fill, can't be more then one.
 * This is a 5.4 syntax. */
EXTERN int defwFillLayer (const char*  layerName);

/* This routine is optional.
 * This is a 5.8 syntax. */
EXTERN int defwFillLayerMask (int  maskColor);

/* This routine has to be called after defwFillLayer
 * This routine is optional.
 * This is a 5.7 syntax. */
EXTERN int defwFillLayerOPC ();

/* This routine is for rectangle.
 * This is a 5.4 syntax. */
EXTERN int defwFillRect (int  xl, int  yl, int  xh, int  yh);

/* This routine is for polygon.
 * This is a 5.6 syntax and can be called multiple time. */
EXTERN int defwFillPolygon (int  num_polys, double*  xl, double*  yl);

/* This routine is for via.
 * This routine is optional.
 * This is a 5.7 syntax and can be called multiple time. */
EXTERN int defwFillVia (const char*  viaName);

/* This routine is optional.
 * This is a 5.8 syntax. */
EXTERN int defwFillViaMask (int  colorMask);

/* This routine is for via OPC.
 * This routine can only be called after defwFillVia.
 * This routine is optional.
 * This is a 5.7 syntax and can be called multiple time. */
EXTERN int defwFillViaOPC ();

/* This routine is for via OPC.
 * This routine can only be called after defwFillVia.
 * This routine is required following defwFillVia.
 * This is a 5.7 syntax and can be called multiple time. */
EXTERN int defwFillPoints (int  num_points, double*  xl, double*  yl);

/* This is a 5.4 syntax. */
EXTERN int defwEndFills ();

/* Routines to enter NONDEFAULTRULES.  This section is required
 * The section must start with a defwStartNonDefaultRules() and
 * end with defwEndNonDefaultRules() call.
 * This is a 5.6 syntax. */
EXTERN int defwStartNonDefaultRules (int  count);

/* This routine is for Layer within the NONDEFAULTRULES
 * This routine can be called multiple times.  It is required.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
EXTERN int defwNonDefaultRule (const char*  ruleName, int  hardSpacing);

/* Routines to enter NONDEFAULTRULES.  This section is required
 * This routine must be called after the defwNonDefaultRule. 
 * This routine can be called multiple times.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
EXTERN int defwNonDefaultRuleLayer (const char*  layerName, int  width, int  diagWidth, int  spacing, int  wireExt);

/* Routines to enter NONDEFAULTRULES.  This section is optional.
 * This routine must be called after the defwNonDefaultRule. 
 * This routine can be called multiple times.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
EXTERN int defwNonDefaultRuleVia (const char*  viaName);

/* Routines to enter NONDEFAULTRULES.  This section is optional.
 * This routine must be called after the defwNonDefaultRule. 
 * This routine can be called multiple times.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
EXTERN int defwNonDefaultRuleViaRule (const char*  viaRuleName);

/* Routines to enter NONDEFAULTRULES.  This section is optional.
 * This routine must be called after the defwNonDefaultRule. 
 * This routine can be called multiple times.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
EXTERN int defwNonDefaultRuleMinCuts (const char*  cutLayerName, int  numCutS);

/* This is a 5.4 syntax. */
EXTERN int defwEndNonDefaultRules ();

/* Routines to enter STYLES.  This section is required
 * The section must start with a defwStartStyles() and
 * end with defwEndStyles() call.
 * This is a 5.6 syntax. */
EXTERN int defwStartStyles (int  count);

/* This routine is for Layer within the NONDEFAULTRULES
 * This routine can be called multiple times.  It is required.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
EXTERN int defwStyles (int  styleNums, int  num_points, double*  xp, double*  yp);

/* This is a 5.4 syntax. */
EXTERN int defwEndStyles ();

/* This routine is called after defwInit.
 * This routine is optional and it can be called only once.
 * Returns 0 if successful. */
EXTERN int defwStartBeginext (const char*  name);

/* This routine is called after defwBeginext.
 * This routine is optional, it can be called only once.
 * Returns 0 if successful. */
EXTERN int defwBeginextCreator (const char*  creatorName);

/* This routine is called after defwBeginext.
 * This routine is optional, it can be called only once.
 * It gets the current system time and date.
 * Returns 0 if successful. */
EXTERN int defwBeginextDate ();

/* This routine is called after defwBeginext.
 * This routine is optional, it can be called only once.
 * Returns 0 if successful. */
EXTERN int defwBeginextRevision (int  vers1, int  vers2);

/* This routine is called after defwBeginext.
 * This routine is optional, it can be called many times.
 * It allows user to customize their own syntax.
 * Returns 0 if successful. */
EXTERN int defwBeginextSyntax (const char*  title, const char*  string);

/* This routine is called after defwInit.
 * This routine is optional and it can be called only once.
 * Returns 0 if successful. */
EXTERN int defwEndBeginext ();

/* End the DEF file.
 * This routine IS NOT OPTIONAL.
 * The routine must be called LAST. */
EXTERN int defwEnd ();

/* General routines that can be called anytime after the Init is called.
 */
EXTERN int defwCurrentLineNumber ();

/*
 * extern void defwError ( const char *, ... );
 * extern void defwWarning ( const char *, ... );
 * extern void defwVError ( const char *, va_list );
 * extern void defwVWarning ( const char *, va_list );
 * extern int  defwGetCurrentLineNumber (void);
 * extern const char *defwGetCurrentFileName (void);
 */

/* This routine will print the error message. */
EXTERN void defwPrintError (int  status);

/* This routine will allow user to write their own comemnt.  It will
 * automactically add a # infront of the line.
 */
EXTERN void defwAddComment (const char*  comment);

/* This routine will indent 3 blank spaces */
EXTERN void defwAddIndent ();

#endif
