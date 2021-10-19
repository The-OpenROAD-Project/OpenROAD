// *****************************************************************************
// *****************************************************************************
// ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!
// *****************************************************************************
// *****************************************************************************
// Copyright 2012, Cadence Design Systems
// 
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8. 
// 
// Licensed under the Apache License, Version 2.0 (the \"License\");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an \"AS IS\" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
// 
// 
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
// 
//  $Author: xxx $
//  $Revision: xxx $
//  $Date: xxx $
//  $State: xxx $  
// *****************************************************************************
// *****************************************************************************

#define EXTERN extern "C"

#include "defiNet.h"
#include "defiNet.hpp"

// Wrappers definitions.
const char* defiWire_wireType (const ::defiWire* obj) {
    return ((const LefDefParser::defiWire*)obj)->wireType();
}

const char* defiWire_wireShieldNetName (const ::defiWire* obj) {
    return ((const LefDefParser::defiWire*)obj)->wireShieldNetName();
}

int defiWire_numPaths (const ::defiWire* obj) {
    return ((LefDefParser::defiWire*)obj)->numPaths();
}

const ::defiPath* defiWire_path (const ::defiWire* obj, int  index) {
    return (const ::defiPath*) ((const LefDefParser::defiWire*)obj)->path(index);
}

void defiSubnet_print (const ::defiSubnet* obj, FILE*  f) {
    ((LefDefParser::defiSubnet*)obj)->print(f);
}

const char* defiSubnet_name (const ::defiSubnet* obj) {
    return ((const LefDefParser::defiSubnet*)obj)->name();
}

int defiSubnet_numConnections (const ::defiSubnet* obj) {
    return ((LefDefParser::defiSubnet*)obj)->numConnections();
}

const char* defiSubnet_instance (const ::defiSubnet* obj, int  index) {
    return ((const LefDefParser::defiSubnet*)obj)->instance(index);
}

const char* defiSubnet_pin (const ::defiSubnet* obj, int  index) {
    return ((const LefDefParser::defiSubnet*)obj)->pin(index);
}

int defiSubnet_pinIsSynthesized (const ::defiSubnet* obj, int  index) {
    return ((LefDefParser::defiSubnet*)obj)->pinIsSynthesized(index);
}

int defiSubnet_pinIsMustJoin (const ::defiSubnet* obj, int  index) {
    return ((LefDefParser::defiSubnet*)obj)->pinIsMustJoin(index);
}

int defiSubnet_isFixed (const ::defiSubnet* obj) {
    return ((LefDefParser::defiSubnet*)obj)->isFixed();
}

int defiSubnet_isRouted (const ::defiSubnet* obj) {
    return ((LefDefParser::defiSubnet*)obj)->isRouted();
}

int defiSubnet_isCover (const ::defiSubnet* obj) {
    return ((LefDefParser::defiSubnet*)obj)->isCover();
}

int defiSubnet_hasNonDefaultRule (const ::defiSubnet* obj) {
    return ((LefDefParser::defiSubnet*)obj)->hasNonDefaultRule();
}

int defiSubnet_numPaths (const ::defiSubnet* obj) {
    return ((LefDefParser::defiSubnet*)obj)->numPaths();
}

const ::defiPath* defiSubnet_path (const ::defiSubnet* obj, int  index) {
    return (const ::defiPath*) ((const LefDefParser::defiSubnet*)obj)->path(index);
}

const char* defiSubnet_nonDefaultRule (const ::defiSubnet* obj) {
    return ((const LefDefParser::defiSubnet*)obj)->nonDefaultRule();
}

int defiSubnet_numWires (const ::defiSubnet* obj) {
    return ((LefDefParser::defiSubnet*)obj)->numWires();
}

const ::defiWire* defiSubnet_wire (const ::defiSubnet* obj, int  index) {
    return (const ::defiWire*) ((const LefDefParser::defiSubnet*)obj)->wire(index);
}

int defiVpin_xl (const ::defiVpin* obj) {
    return ((LefDefParser::defiVpin*)obj)->xl();
}

int defiVpin_yl (const ::defiVpin* obj) {
    return ((LefDefParser::defiVpin*)obj)->yl();
}

int defiVpin_xh (const ::defiVpin* obj) {
    return ((LefDefParser::defiVpin*)obj)->xh();
}

int defiVpin_yh (const ::defiVpin* obj) {
    return ((LefDefParser::defiVpin*)obj)->yh();
}

char defiVpin_status (const ::defiVpin* obj) {
    return ((LefDefParser::defiVpin*)obj)->status();
}

int defiVpin_orient (const ::defiVpin* obj) {
    return ((LefDefParser::defiVpin*)obj)->orient();
}

const char* defiVpin_orientStr (const ::defiVpin* obj) {
    return ((const LefDefParser::defiVpin*)obj)->orientStr();
}

int defiVpin_xLoc (const ::defiVpin* obj) {
    return ((LefDefParser::defiVpin*)obj)->xLoc();
}

int defiVpin_yLoc (const ::defiVpin* obj) {
    return ((LefDefParser::defiVpin*)obj)->yLoc();
}

const char* defiVpin_name (const ::defiVpin* obj) {
    return ((const LefDefParser::defiVpin*)obj)->name();
}

const char* defiVpin_layer (const ::defiVpin* obj) {
    return ((const LefDefParser::defiVpin*)obj)->layer();
}

const char* defiShield_shieldName (const ::defiShield* obj) {
    return ((const LefDefParser::defiShield*)obj)->shieldName();
}

int defiShield_numPaths (const ::defiShield* obj) {
    return ((LefDefParser::defiShield*)obj)->numPaths();
}

const ::defiPath* defiShield_path (const ::defiShield* obj, int  index) {
    return (const ::defiPath*) ((const LefDefParser::defiShield*)obj)->path(index);
}

const char* defiNet_name (const ::defiNet* obj) {
    return ((const LefDefParser::defiNet*)obj)->name();
}

int defiNet_weight (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->weight();
}

int defiNet_numProps (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->numProps();
}

const char* defiNet_propName (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->propName(index);
}

const char* defiNet_propValue (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->propValue(index);
}

double defiNet_propNumber (const ::defiNet* obj, int  index) {
    return ((LefDefParser::defiNet*)obj)->propNumber(index);
}

char defiNet_propType (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->propType(index);
}

int defiNet_propIsNumber (const ::defiNet* obj, int  index) {
    return ((LefDefParser::defiNet*)obj)->propIsNumber(index);
}

int defiNet_propIsString (const ::defiNet* obj, int  index) {
    return ((LefDefParser::defiNet*)obj)->propIsString(index);
}

int defiNet_numConnections (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->numConnections();
}

const char* defiNet_instance (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->instance(index);
}

const char* defiNet_pin (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->pin(index);
}

int defiNet_pinIsMustJoin (const ::defiNet* obj, int  index) {
    return ((LefDefParser::defiNet*)obj)->pinIsMustJoin(index);
}

int defiNet_pinIsSynthesized (const ::defiNet* obj, int  index) {
    return ((LefDefParser::defiNet*)obj)->pinIsSynthesized(index);
}

int defiNet_numSubnets (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->numSubnets();
}

const ::defiSubnet* defiNet_subnet (const ::defiNet* obj, int  index) {
    return (const ::defiSubnet*) ((const LefDefParser::defiNet*)obj)->subnet(index);
}

int defiNet_isFixed (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->isFixed();
}

int defiNet_isRouted (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->isRouted();
}

int defiNet_isCover (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->isCover();
}

int defiNet_numWires (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->numWires();
}

const ::defiWire* defiNet_wire (const ::defiNet* obj, int  index) {
    return (const ::defiWire*) ((const LefDefParser::defiNet*)obj)->wire(index);
}

int defiNet_numVpins (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->numVpins();
}

const ::defiVpin* defiNet_vpin (const ::defiNet* obj, int  index) {
    return (const ::defiVpin*) ((const LefDefParser::defiNet*)obj)->vpin(index);
}

int defiNet_hasProps (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->hasProps();
}

int defiNet_hasWeight (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->hasWeight();
}

int defiNet_hasSubnets (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->hasSubnets();
}

int defiNet_hasSource (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->hasSource();
}

int defiNet_hasFixedbump (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->hasFixedbump();
}

int defiNet_hasFrequency (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->hasFrequency();
}

int defiNet_hasPattern (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->hasPattern();
}

int defiNet_hasOriginal (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->hasOriginal();
}

int defiNet_hasCap (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->hasCap();
}

int defiNet_hasUse (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->hasUse();
}

int defiNet_hasStyle (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->hasStyle();
}

int defiNet_hasNonDefaultRule (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->hasNonDefaultRule();
}

int defiNet_hasVoltage (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->hasVoltage();
}

int defiNet_hasSpacingRules (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->hasSpacingRules();
}

int defiNet_hasWidthRules (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->hasWidthRules();
}

int defiNet_hasXTalk (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->hasXTalk();
}

int defiNet_numSpacingRules (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->numSpacingRules();
}

void defiNet_spacingRule (const ::defiNet* obj, int  index, char**  layer, double*  dist, double*  left, double*  right) {
    ((LefDefParser::defiNet*)obj)->spacingRule(index, layer, dist, left, right);
}

int defiNet_numWidthRules (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->numWidthRules();
}

void defiNet_widthRule (const ::defiNet* obj, int  index, char**  layer, double*  dist) {
    ((LefDefParser::defiNet*)obj)->widthRule(index, layer, dist);
}

double defiNet_voltage (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->voltage();
}

int defiNet_XTalk (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->XTalk();
}

const char* defiNet_source (const ::defiNet* obj) {
    return ((const LefDefParser::defiNet*)obj)->source();
}

double defiNet_frequency (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->frequency();
}

const char* defiNet_original (const ::defiNet* obj) {
    return ((const LefDefParser::defiNet*)obj)->original();
}

const char* defiNet_pattern (const ::defiNet* obj) {
    return ((const LefDefParser::defiNet*)obj)->pattern();
}

double defiNet_cap (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->cap();
}

const char* defiNet_use (const ::defiNet* obj) {
    return ((const LefDefParser::defiNet*)obj)->use();
}

int defiNet_style (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->style();
}

const char* defiNet_nonDefaultRule (const ::defiNet* obj) {
    return ((const LefDefParser::defiNet*)obj)->nonDefaultRule();
}

int defiNet_numPaths (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->numPaths();
}

const ::defiPath* defiNet_path (const ::defiNet* obj, int  index) {
    return (const ::defiPath*) ((const LefDefParser::defiNet*)obj)->path(index);
}

int defiNet_numShields (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->numShields();
}

const ::defiShield* defiNet_shield (const ::defiNet* obj, int  index) {
    return (const ::defiShield*) ((const LefDefParser::defiNet*)obj)->shield(index);
}

int defiNet_numShieldNets (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->numShieldNets();
}

const char* defiNet_shieldNet (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->shieldNet(index);
}

int defiNet_numNoShields (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->numNoShields();
}

const ::defiShield* defiNet_noShield (const ::defiNet* obj, int  index) {
    return (const ::defiShield*) ((const LefDefParser::defiNet*)obj)->noShield(index);
}

int defiNet_numPolygons (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->numPolygons();
}

const char* defiNet_polygonName (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->polygonName(index);
}

::defiPoints defiNet_getPolygon (const ::defiNet* obj, int  index) {
    LefDefParser::defiPoints tmp;
    tmp = ((LefDefParser::defiNet*)obj)->getPolygon(index);
    return *((::defiPoints*)&tmp);
}

int defiNet_polyMask (const ::defiNet* obj, int  index) {
    return ((LefDefParser::defiNet*)obj)->polyMask(index);
}

const char* defiNet_polyRouteStatus (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->polyRouteStatus(index);
}

const char* defiNet_polyRouteStatusShieldName (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->polyRouteStatusShieldName(index);
}

const char* defiNet_polyShapeType (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->polyShapeType(index);
}

int defiNet_numRectangles (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->numRectangles();
}

const char* defiNet_rectName (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->rectName(index);
}

int defiNet_xl (const ::defiNet* obj, int  index) {
    return ((LefDefParser::defiNet*)obj)->xl(index);
}

int defiNet_yl (const ::defiNet* obj, int  index) {
    return ((LefDefParser::defiNet*)obj)->yl(index);
}

int defiNet_xh (const ::defiNet* obj, int  index) {
    return ((LefDefParser::defiNet*)obj)->xh(index);
}

int defiNet_yh (const ::defiNet* obj, int  index) {
    return ((LefDefParser::defiNet*)obj)->yh(index);
}

int defiNet_rectMask (const ::defiNet* obj, int  index) {
    return ((LefDefParser::defiNet*)obj)->rectMask(index);
}

const char* defiNet_rectRouteStatus (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->rectRouteStatus(index);
}

const char* defiNet_rectRouteStatusShieldName (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->rectRouteStatusShieldName(index);
}

const char* defiNet_rectShapeType (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->rectShapeType(index);
}

int defiNet_numViaSpecs (const ::defiNet* obj) {
    return ((LefDefParser::defiNet*)obj)->numViaSpecs();
}

::defiPoints defiNet_getViaPts (const ::defiNet* obj, int  index) {
    LefDefParser::defiPoints tmp;
    tmp = ((LefDefParser::defiNet*)obj)->getViaPts(index);
    return *((::defiPoints*)&tmp);
}

const char* defiNet_viaName (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->viaName(index);
}

int defiNet_viaOrient (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->viaOrient(index);
}

const char* defiNet_viaOrientStr (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->viaOrientStr(index);
}

int defiNet_topMaskNum (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->topMaskNum(index);
}

int defiNet_cutMaskNum (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->cutMaskNum(index);
}

int defiNet_bottomMaskNum (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->bottomMaskNum(index);
}

const char* defiNet_viaRouteStatus (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->viaRouteStatus(index);
}

const char* defiNet_viaRouteStatusShieldName (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->viaRouteStatusShieldName(index);
}

const char* defiNet_viaShapeType (const ::defiNet* obj, int  index) {
    return ((const LefDefParser::defiNet*)obj)->viaShapeType(index);
}

void defiNet_print (const ::defiNet* obj, FILE*  f) {
    ((LefDefParser::defiNet*)obj)->print(f);
}

