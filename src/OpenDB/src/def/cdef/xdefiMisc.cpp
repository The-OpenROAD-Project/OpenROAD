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

#include "defiMisc.h"
#include "defiMisc.hpp"

// Wrappers definitions.
int defiGeometries_numPoints (const ::defiGeometries* obj) {
    return ((LefDefParser::defiGeometries*)obj)->numPoints();
}

void defiGeometries_points (const ::defiGeometries* obj, int  index, int*  x, int*  y) {
    ((LefDefParser::defiGeometries*)obj)->points(index, x, y);
}

int defiStyles_style (const ::defiStyles* obj) {
    return ((LefDefParser::defiStyles*)obj)->style();
}

::defiPoints defiStyles_getPolygon (const ::defiStyles* obj) {
    LefDefParser::defiPoints tmp;
    tmp = ((LefDefParser::defiStyles*)obj)->getPolygon();
    return *((::defiPoints*)&tmp);
}

