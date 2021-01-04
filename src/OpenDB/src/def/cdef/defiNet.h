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


#ifndef CDEFINET_H
#define CDEFINET_H

#include <stdio.h>
#include "defiTypedefs.h"

/* Return codes for defiNet::viaOrient 
    DEF_ORIENT_N  0
    DEF_ORIENT_W  1
    DEF_ORIENT_S  2
    DEF_ORIENT_E  3
    DEF_ORIENT_FN 4
    DEF_ORIENT_FW 5
    DEF_ORIENT_FS 6
    DEF_ORIENT_FE 7
*/

EXTERN const char* defiWire_wireType (const defiWire* obj);
EXTERN const char* defiWire_wireShieldNetName (const defiWire* obj);
EXTERN int defiWire_numPaths (const defiWire* obj);

EXTERN const defiPath* defiWire_path (const defiWire* obj, int  index);

  /* WMD -- the following will be removed by the next release                 */

  /* NEW: a net can have more than 1 wire                                     */

  /* Debug printing                                                           */
EXTERN void defiSubnet_print (const defiSubnet* obj, FILE*  f);

EXTERN const char* defiSubnet_name (const defiSubnet* obj);
EXTERN int defiSubnet_numConnections (const defiSubnet* obj);
EXTERN const char* defiSubnet_instance (const defiSubnet* obj, int  index);
EXTERN const char* defiSubnet_pin (const defiSubnet* obj, int  index);
EXTERN int defiSubnet_pinIsSynthesized (const defiSubnet* obj, int  index);
EXTERN int defiSubnet_pinIsMustJoin (const defiSubnet* obj, int  index);

  /* WMD -- the following will be removed by the next release                 */
EXTERN int defiSubnet_isFixed (const defiSubnet* obj);
EXTERN int defiSubnet_isRouted (const defiSubnet* obj);
EXTERN int defiSubnet_isCover (const defiSubnet* obj);

EXTERN int defiSubnet_hasNonDefaultRule (const defiSubnet* obj);

  /* WMD -- the following will be removed by the next release                 */
EXTERN int defiSubnet_numPaths (const defiSubnet* obj);
EXTERN const defiPath* defiSubnet_path (const defiSubnet* obj, int  index);

EXTERN const char* defiSubnet_nonDefaultRule (const defiSubnet* obj);

EXTERN int defiSubnet_numWires (const defiSubnet* obj);
EXTERN const defiWire* defiSubnet_wire (const defiSubnet* obj, int  index);

  /* WMD -- the following will be removed by the next release                 */

EXTERN int defiVpin_xl (const defiVpin* obj);
EXTERN int defiVpin_yl (const defiVpin* obj);
EXTERN int defiVpin_xh (const defiVpin* obj);
EXTERN int defiVpin_yh (const defiVpin* obj);
EXTERN char defiVpin_status (const defiVpin* obj);
EXTERN int defiVpin_orient (const defiVpin* obj);
EXTERN const char* defiVpin_orientStr (const defiVpin* obj);
EXTERN int defiVpin_xLoc (const defiVpin* obj);
EXTERN int defiVpin_yLoc (const defiVpin* obj);
EXTERN const char* defiVpin_name (const defiVpin* obj);
EXTERN const char* defiVpin_layer (const defiVpin* obj);

/* Pre 5.4                                                                    */

EXTERN const char* defiShield_shieldName (const defiShield* obj);
EXTERN int defiShield_numPaths (const defiShield* obj);

EXTERN const defiPath* defiShield_path (const defiShield* obj, int  index);

/* Struct holds the data for one component.                                   */

  /* Routines used by YACC to set the fields in the net.                      */

  /* WMD -- the following will be removed by the next release                 */

  /* NEW: a net can have more than 1 wire                                     */

  /* 5.6                                                                      */

  /* For OA to modify the netName, id & pinName                               */

  /* Routines to return the value of net data.                                */
EXTERN const char* defiNet_name (const defiNet* obj);
EXTERN int defiNet_weight (const defiNet* obj);
EXTERN int defiNet_numProps (const defiNet* obj);
EXTERN const char* defiNet_propName (const defiNet* obj, int  index);
EXTERN const char* defiNet_propValue (const defiNet* obj, int  index);
EXTERN double defiNet_propNumber (const defiNet* obj, int  index);
EXTERN char defiNet_propType (const defiNet* obj, int  index);
EXTERN int defiNet_propIsNumber (const defiNet* obj, int  index);
EXTERN int defiNet_propIsString (const defiNet* obj, int  index);
EXTERN int defiNet_numConnections (const defiNet* obj);
EXTERN const char* defiNet_instance (const defiNet* obj, int  index);
EXTERN const char* defiNet_pin (const defiNet* obj, int  index);
EXTERN int defiNet_pinIsMustJoin (const defiNet* obj, int  index);
EXTERN int defiNet_pinIsSynthesized (const defiNet* obj, int  index);
EXTERN int defiNet_numSubnets (const defiNet* obj);

EXTERN const defiSubnet* defiNet_subnet (const defiNet* obj, int  index);

  /* WMD -- the following will be removed by the next release                 */
EXTERN int defiNet_isFixed (const defiNet* obj);
EXTERN int defiNet_isRouted (const defiNet* obj);
EXTERN int defiNet_isCover (const defiNet* obj);

  /* The following routines are for wiring */
EXTERN int defiNet_numWires (const defiNet* obj);

EXTERN const defiWire* defiNet_wire (const defiNet* obj, int  index);

  /* Routines to get the information about Virtual Pins. */
EXTERN int defiNet_numVpins (const defiNet* obj);

EXTERN const defiVpin* defiNet_vpin (const defiNet* obj, int  index);

EXTERN int defiNet_hasProps (const defiNet* obj);
EXTERN int defiNet_hasWeight (const defiNet* obj);
EXTERN int defiNet_hasSubnets (const defiNet* obj);
EXTERN int defiNet_hasSource (const defiNet* obj);
EXTERN int defiNet_hasFixedbump (const defiNet* obj);
EXTERN int defiNet_hasFrequency (const defiNet* obj);
EXTERN int defiNet_hasPattern (const defiNet* obj);
EXTERN int defiNet_hasOriginal (const defiNet* obj);
EXTERN int defiNet_hasCap (const defiNet* obj);
EXTERN int defiNet_hasUse (const defiNet* obj);
EXTERN int defiNet_hasStyle (const defiNet* obj);
EXTERN int defiNet_hasNonDefaultRule (const defiNet* obj);
EXTERN int defiNet_hasVoltage (const defiNet* obj);
EXTERN int defiNet_hasSpacingRules (const defiNet* obj);
EXTERN int defiNet_hasWidthRules (const defiNet* obj);
EXTERN int defiNet_hasXTalk (const defiNet* obj);

EXTERN int defiNet_numSpacingRules (const defiNet* obj);
EXTERN void defiNet_spacingRule (const defiNet* obj, int  index, char**  layer, double*  dist, double*  left, double*  right);
EXTERN int defiNet_numWidthRules (const defiNet* obj);
EXTERN void defiNet_widthRule (const defiNet* obj, int  index, char**  layer, double*  dist);
EXTERN double defiNet_voltage (const defiNet* obj);

EXTERN int defiNet_XTalk (const defiNet* obj);
EXTERN const char* defiNet_source (const defiNet* obj);
EXTERN double defiNet_frequency (const defiNet* obj);
EXTERN const char* defiNet_original (const defiNet* obj);
EXTERN const char* defiNet_pattern (const defiNet* obj);
EXTERN double defiNet_cap (const defiNet* obj);
EXTERN const char* defiNet_use (const defiNet* obj);
EXTERN int defiNet_style (const defiNet* obj);
EXTERN const char* defiNet_nonDefaultRule (const defiNet* obj);

  /* WMD -- the following will be removed by the next release                 */
EXTERN int defiNet_numPaths (const defiNet* obj);

EXTERN const defiPath* defiNet_path (const defiNet* obj, int  index);

EXTERN int defiNet_numShields (const defiNet* obj);

EXTERN const defiShield* defiNet_shield (const defiNet* obj, int  index);

EXTERN int defiNet_numShieldNets (const defiNet* obj);
EXTERN const char* defiNet_shieldNet (const defiNet* obj, int  index);
EXTERN int defiNet_numNoShields (const defiNet* obj);

EXTERN const defiShield* defiNet_noShield (const defiNet* obj, int  index);

  /* 5.6                                                                      */
EXTERN int defiNet_numPolygons (const defiNet* obj);
EXTERN const char* defiNet_polygonName (const defiNet* obj, int  index);
EXTERN struct defiPoints defiNet_getPolygon (const defiNet* obj, int  index);
EXTERN int defiNet_polyMask (const defiNet* obj, int  index);
EXTERN const char* defiNet_polyRouteStatus (const defiNet* obj, int  index);
EXTERN const char* defiNet_polyRouteStatusShieldName (const defiNet* obj, int  index);
EXTERN const char* defiNet_polyShapeType (const defiNet* obj, int  index);

EXTERN int defiNet_numRectangles (const defiNet* obj);
EXTERN const char* defiNet_rectName (const defiNet* obj, int  index);
EXTERN int defiNet_xl (const defiNet* obj, int  index);
EXTERN int defiNet_yl (const defiNet* obj, int  index);
EXTERN int defiNet_xh (const defiNet* obj, int  index);
EXTERN int defiNet_yh (const defiNet* obj, int  index);
EXTERN int defiNet_rectMask (const defiNet* obj, int  index);
EXTERN const char* defiNet_rectRouteStatus (const defiNet* obj, int  index);
EXTERN const char* defiNet_rectRouteStatusShieldName (const defiNet* obj, int  index);
EXTERN const char* defiNet_rectShapeType (const defiNet* obj, int  index);

  /* 5.8                                                                      */
EXTERN int defiNet_numViaSpecs (const defiNet* obj);
EXTERN struct defiPoints defiNet_getViaPts (const defiNet* obj, int  index);
EXTERN const char* defiNet_viaName (const defiNet* obj, int  index);
EXTERN int defiNet_viaOrient (const defiNet* obj, int  index);
EXTERN const char* defiNet_viaOrientStr (const defiNet* obj, int  index);
EXTERN int defiNet_topMaskNum (const defiNet* obj, int  index);
EXTERN int defiNet_cutMaskNum (const defiNet* obj, int  index);
EXTERN int defiNet_bottomMaskNum (const defiNet* obj, int  index);
EXTERN const char* defiNet_viaRouteStatus (const defiNet* obj, int  index);
EXTERN const char* defiNet_viaRouteStatusShieldName (const defiNet* obj, int  index);
EXTERN const char* defiNet_viaShapeType (const defiNet* obj, int  index);

  /* Debug printing                                                           */
EXTERN void defiNet_print (const defiNet* obj, FILE*  f);

  /* The method freeWire() is added is user select to have a callback         */
  /* per wire within a net This is an internal method and is not public       */

  /* Clear the rectangles & polygons data if partial path callback is set     */

  /* WMD -- the following will be removed by the nex release                  */

  /* WMD -- the following will be removed by the nex release                  */

#endif
