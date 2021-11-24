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

#include "defiFill.h"
#include "defiFill.hpp"

// Wrappers definitions.
int defiFill_hasLayer (const ::defiFill* obj) {
    return ((LefDefParser::defiFill*)obj)->hasLayer();
}

const char* defiFill_layerName (const ::defiFill* obj) {
    return ((const LefDefParser::defiFill*)obj)->layerName();
}

int defiFill_hasLayerOpc (const ::defiFill* obj) {
    return ((LefDefParser::defiFill*)obj)->hasLayerOpc();
}

int defiFill_layerMask (const ::defiFill* obj) {
    return ((LefDefParser::defiFill*)obj)->layerMask();
}

int defiFill_viaTopMask (const ::defiFill* obj) {
    return ((LefDefParser::defiFill*)obj)->viaTopMask();
}

int defiFill_viaCutMask (const ::defiFill* obj) {
    return ((LefDefParser::defiFill*)obj)->viaCutMask();
}

int defiFill_viaBottomMask (const ::defiFill* obj) {
    return ((LefDefParser::defiFill*)obj)->viaBottomMask();
}

int defiFill_numRectangles (const ::defiFill* obj) {
    return ((LefDefParser::defiFill*)obj)->numRectangles();
}

int defiFill_xl (const ::defiFill* obj, int  index) {
    return ((LefDefParser::defiFill*)obj)->xl(index);
}

int defiFill_yl (const ::defiFill* obj, int  index) {
    return ((LefDefParser::defiFill*)obj)->yl(index);
}

int defiFill_xh (const ::defiFill* obj, int  index) {
    return ((LefDefParser::defiFill*)obj)->xh(index);
}

int defiFill_yh (const ::defiFill* obj, int  index) {
    return ((LefDefParser::defiFill*)obj)->yh(index);
}

int defiFill_numPolygons (const ::defiFill* obj) {
    return ((LefDefParser::defiFill*)obj)->numPolygons();
}

::defiPoints defiFill_getPolygon (const ::defiFill* obj, int  index) {
    LefDefParser::defiPoints tmp;
    tmp = ((LefDefParser::defiFill*)obj)->getPolygon(index);
    return *((::defiPoints*)&tmp);
}

int defiFill_hasVia (const ::defiFill* obj) {
    return ((LefDefParser::defiFill*)obj)->hasVia();
}

const char* defiFill_viaName (const ::defiFill* obj) {
    return ((const LefDefParser::defiFill*)obj)->viaName();
}

int defiFill_hasViaOpc (const ::defiFill* obj) {
    return ((LefDefParser::defiFill*)obj)->hasViaOpc();
}

int defiFill_numViaPts (const ::defiFill* obj) {
    return ((LefDefParser::defiFill*)obj)->numViaPts();
}

::defiPoints defiFill_getViaPts (const ::defiFill* obj, int  index) {
    LefDefParser::defiPoints tmp;
    tmp = ((LefDefParser::defiFill*)obj)->getViaPts(index);
    return *((::defiPoints*)&tmp);
}

void defiFill_print (const ::defiFill* obj, FILE*  f) {
    ((LefDefParser::defiFill*)obj)->print(f);
}

