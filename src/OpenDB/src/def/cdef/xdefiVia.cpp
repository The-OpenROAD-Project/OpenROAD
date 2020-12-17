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

#include "defiVia.h"
#include "defiVia.hpp"

// Wrappers definitions.
const char* defiVia_name (const ::defiVia* obj) {
    return ((const LefDefParser::defiVia*)obj)->name();
}

const char* defiVia_pattern (const ::defiVia* obj) {
    return ((const LefDefParser::defiVia*)obj)->pattern();
}

int defiVia_hasPattern (const ::defiVia* obj) {
    return ((LefDefParser::defiVia*)obj)->hasPattern();
}

int defiVia_numLayers (const ::defiVia* obj) {
    return ((LefDefParser::defiVia*)obj)->numLayers();
}

void defiVia_layer (const ::defiVia* obj, int  index, char**  layer, int*  xl, int*  yl, int*  xh, int*  yh) {
    ((LefDefParser::defiVia*)obj)->layer(index, layer, xl, yl, xh, yh);
}

int defiVia_numPolygons (const ::defiVia* obj) {
    return ((LefDefParser::defiVia*)obj)->numPolygons();
}

const char* defiVia_polygonName (const ::defiVia* obj, int  index) {
    return ((const LefDefParser::defiVia*)obj)->polygonName(index);
}

::defiPoints defiVia_getPolygon (const ::defiVia* obj, int  index) {
    LefDefParser::defiPoints tmp;
    tmp = ((LefDefParser::defiVia*)obj)->getPolygon(index);
    return *((::defiPoints*)&tmp);
}

int defiVia_hasViaRule (const ::defiVia* obj) {
    return ((LefDefParser::defiVia*)obj)->hasViaRule();
}

void defiVia_viaRule (const ::defiVia* obj, char**  viaRuleName, int*  xSize, int*  ySize, char**  botLayer, char**  cutLayer, char**  topLayer, int*  xCutSpacing, int*  yCutSpacing, int*  xBotEnc, int*  yBotEnc, int*  xTopEnc, int*  yTopEnc) {
    ((LefDefParser::defiVia*)obj)->viaRule(viaRuleName, xSize, ySize, botLayer, cutLayer, topLayer, xCutSpacing, yCutSpacing, xBotEnc, yBotEnc, xTopEnc, yTopEnc);
}

int defiVia_hasRowCol (const ::defiVia* obj) {
    return ((LefDefParser::defiVia*)obj)->hasRowCol();
}

void defiVia_rowCol (const ::defiVia* obj, int*  numCutRows, int*  numCutCols) {
    ((LefDefParser::defiVia*)obj)->rowCol(numCutRows, numCutCols);
}

int defiVia_hasOrigin (const ::defiVia* obj) {
    return ((LefDefParser::defiVia*)obj)->hasOrigin();
}

void defiVia_origin (const ::defiVia* obj, int*  xOffset, int*  yOffset) {
    ((LefDefParser::defiVia*)obj)->origin(xOffset, yOffset);
}

int defiVia_hasOffset (const ::defiVia* obj) {
    return ((LefDefParser::defiVia*)obj)->hasOffset();
}

void defiVia_offset (const ::defiVia* obj, int*  xBotOffset, int*  yBotOffset, int*  xTopOffset, int*  yTopOffset) {
    ((LefDefParser::defiVia*)obj)->offset(xBotOffset, yBotOffset, xTopOffset, yTopOffset);
}

int defiVia_hasCutPattern (const ::defiVia* obj) {
    return ((LefDefParser::defiVia*)obj)->hasCutPattern();
}

const char* defiVia_cutPattern (const ::defiVia* obj) {
    return ((const LefDefParser::defiVia*)obj)->cutPattern();
}

int defiVia_hasRectMask (const ::defiVia* obj, int  index) {
    return ((LefDefParser::defiVia*)obj)->hasRectMask(index);
}

int defiVia_rectMask (const ::defiVia* obj, int  index) {
    return ((LefDefParser::defiVia*)obj)->rectMask(index);
}

int defiVia_hasPolyMask (const ::defiVia* obj, int  index) {
    return ((LefDefParser::defiVia*)obj)->hasPolyMask(index);
}

int defiVia_polyMask (const ::defiVia* obj, int  index) {
    return ((LefDefParser::defiVia*)obj)->polyMask(index);
}

void defiVia_print (const ::defiVia* obj, FILE*  f) {
    ((LefDefParser::defiVia*)obj)->print(f);
}

