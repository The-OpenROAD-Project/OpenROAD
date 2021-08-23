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

#include "defiSlot.h"
#include "defiSlot.hpp"

// Wrappers definitions.
int defiSlot_hasLayer (const ::defiSlot* obj) {
    return ((LefDefParser::defiSlot*)obj)->hasLayer();
}

const char* defiSlot_layerName (const ::defiSlot* obj) {
    return ((const LefDefParser::defiSlot*)obj)->layerName();
}

int defiSlot_numRectangles (const ::defiSlot* obj) {
    return ((LefDefParser::defiSlot*)obj)->numRectangles();
}

int defiSlot_xl (const ::defiSlot* obj, int  index) {
    return ((LefDefParser::defiSlot*)obj)->xl(index);
}

int defiSlot_yl (const ::defiSlot* obj, int  index) {
    return ((LefDefParser::defiSlot*)obj)->yl(index);
}

int defiSlot_xh (const ::defiSlot* obj, int  index) {
    return ((LefDefParser::defiSlot*)obj)->xh(index);
}

int defiSlot_yh (const ::defiSlot* obj, int  index) {
    return ((LefDefParser::defiSlot*)obj)->yh(index);
}

int defiSlot_numPolygons (const ::defiSlot* obj) {
    return ((LefDefParser::defiSlot*)obj)->numPolygons();
}

::defiPoints defiSlot_getPolygon (const ::defiSlot* obj, int  index) {
    LefDefParser::defiPoints tmp;
    tmp = ((LefDefParser::defiSlot*)obj)->getPolygon(index);
    return *((::defiPoints*)&tmp);
}

void defiSlot_print (const ::defiSlot* obj, FILE*  f) {
    ((LefDefParser::defiSlot*)obj)->print(f);
}

