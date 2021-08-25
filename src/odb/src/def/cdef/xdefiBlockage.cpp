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

#include "defiBlockage.h"
#include "defiBlockage.hpp"

// Wrappers definitions.
int defiBlockage_hasLayer (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->hasLayer();
}

int defiBlockage_hasPlacement (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->hasPlacement();
}

int defiBlockage_hasComponent (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->hasComponent();
}

int defiBlockage_hasSlots (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->hasSlots();
}

int defiBlockage_hasFills (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->hasFills();
}

int defiBlockage_hasPushdown (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->hasPushdown();
}

int defiBlockage_hasExceptpgnet (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->hasExceptpgnet();
}

int defiBlockage_hasSoft (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->hasSoft();
}

int defiBlockage_hasPartial (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->hasPartial();
}

int defiBlockage_hasSpacing (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->hasSpacing();
}

int defiBlockage_hasDesignRuleWidth (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->hasDesignRuleWidth();
}

int defiBlockage_hasMask (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->hasMask();
}

int defiBlockage_mask (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->mask();
}

int defiBlockage_minSpacing (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->minSpacing();
}

int defiBlockage_designRuleWidth (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->designRuleWidth();
}

double defiBlockage_placementMaxDensity (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->placementMaxDensity();
}

const char* defiBlockage_layerName (const ::defiBlockage* obj) {
    return ((const LefDefParser::defiBlockage*)obj)->layerName();
}

const char* defiBlockage_layerComponentName (const ::defiBlockage* obj) {
    return ((const LefDefParser::defiBlockage*)obj)->layerComponentName();
}

const char* defiBlockage_placementComponentName (const ::defiBlockage* obj) {
    return ((const LefDefParser::defiBlockage*)obj)->placementComponentName();
}

int defiBlockage_numRectangles (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->numRectangles();
}

int defiBlockage_xl (const ::defiBlockage* obj, int  index) {
    return ((LefDefParser::defiBlockage*)obj)->xl(index);
}

int defiBlockage_yl (const ::defiBlockage* obj, int  index) {
    return ((LefDefParser::defiBlockage*)obj)->yl(index);
}

int defiBlockage_xh (const ::defiBlockage* obj, int  index) {
    return ((LefDefParser::defiBlockage*)obj)->xh(index);
}

int defiBlockage_yh (const ::defiBlockage* obj, int  index) {
    return ((LefDefParser::defiBlockage*)obj)->yh(index);
}

int defiBlockage_numPolygons (const ::defiBlockage* obj) {
    return ((LefDefParser::defiBlockage*)obj)->numPolygons();
}

::defiPoints defiBlockage_getPolygon (const ::defiBlockage* obj, int  index) {
    LefDefParser::defiPoints tmp;
    tmp = ((LefDefParser::defiBlockage*)obj)->getPolygon(index);
    return *((::defiPoints*)&tmp);
}

void defiBlockage_print (const ::defiBlockage* obj, FILE*  f) {
    ((LefDefParser::defiBlockage*)obj)->print(f);
}

