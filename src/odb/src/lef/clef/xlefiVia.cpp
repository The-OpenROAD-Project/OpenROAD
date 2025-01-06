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
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2020/09/29 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#define EXTERN extern "C"

#include "lefiVia.h"
#include "lefiVia.hpp"

union ulefiGeomPolygon
{
  LefParser::lefiGeomPolygon cpp;
  ::lefiGeomPolygon c;
};

// Wrappers definitions.
struct ::lefiGeomPolygon* lefiViaLayer_getPolygon(const ::lefiViaLayer* obj,
                                                  int index)
{
  return (::lefiGeomPolygon*) ((LefParser::lefiViaLayer*) obj)
      ->getPolygon(index);
}

int lefiVia_hasDefault(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->hasDefault();
}

int lefiVia_hasGenerated(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->hasGenerated();
}

int lefiVia_hasForeign(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->hasForeign();
}

int lefiVia_hasForeignPnt(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->hasForeignPnt();
}

int lefiVia_hasForeignOrient(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->hasForeignOrient();
}

int lefiVia_hasProperties(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->hasProperties();
}

int lefiVia_hasResistance(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->hasResistance();
}

int lefiVia_hasTopOfStack(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->hasTopOfStack();
}

int lefiVia_numLayers(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->numLayers();
}

char* lefiVia_layerName(const ::lefiVia* obj, int layerNum)
{
  return ((LefParser::lefiVia*) obj)->layerName(layerNum);
}

int lefiVia_numRects(const ::lefiVia* obj, int layerNum)
{
  return ((LefParser::lefiVia*) obj)->numRects(layerNum);
}

double lefiVia_xl(const ::lefiVia* obj, int layerNum, int rectNum)
{
  return ((LefParser::lefiVia*) obj)->xl(layerNum, rectNum);
}

double lefiVia_yl(const ::lefiVia* obj, int layerNum, int rectNum)
{
  return ((LefParser::lefiVia*) obj)->yl(layerNum, rectNum);
}

double lefiVia_xh(const ::lefiVia* obj, int layerNum, int rectNum)
{
  return ((LefParser::lefiVia*) obj)->xh(layerNum, rectNum);
}

double lefiVia_yh(const ::lefiVia* obj, int layerNum, int rectNum)
{
  return ((LefParser::lefiVia*) obj)->yh(layerNum, rectNum);
}

int lefiVia_rectColorMask(const ::lefiVia* obj, int layerNum, int rectNum)
{
  return ((LefParser::lefiVia*) obj)->rectColorMask(layerNum, rectNum);
}

int lefiVia_polyColorMask(const ::lefiVia* obj, int layerNum, int polyNum)
{
  return ((LefParser::lefiVia*) obj)->polyColorMask(layerNum, polyNum);
}

int lefiVia_numPolygons(const ::lefiVia* obj, int layerNum)
{
  return ((LefParser::lefiVia*) obj)->numPolygons(layerNum);
}

::lefiGeomPolygon lefiVia_getPolygon(const ::lefiVia* obj,
                                     int layerNum,
                                     int polyNum)
{
  ulefiGeomPolygon tmp;
  tmp.cpp = ((LefParser::lefiVia*) obj)->getPolygon(layerNum, polyNum);
  return tmp.c;
}

char* lefiVia_name(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->name();
}

double lefiVia_resistance(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->resistance();
}

int lefiVia_numProperties(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->numProperties();
}

char* lefiVia_propName(const ::lefiVia* obj, int index)
{
  return ((LefParser::lefiVia*) obj)->propName(index);
}

char* lefiVia_propValue(const ::lefiVia* obj, int index)
{
  return ((LefParser::lefiVia*) obj)->propValue(index);
}

double lefiVia_propNumber(const ::lefiVia* obj, int index)
{
  return ((LefParser::lefiVia*) obj)->propNumber(index);
}

char lefiVia_propType(const ::lefiVia* obj, int index)
{
  return ((LefParser::lefiVia*) obj)->propType(index);
}

int lefiVia_propIsNumber(const ::lefiVia* obj, int index)
{
  return ((LefParser::lefiVia*) obj)->propIsNumber(index);
}

int lefiVia_propIsString(const ::lefiVia* obj, int index)
{
  return ((LefParser::lefiVia*) obj)->propIsString(index);
}

char* lefiVia_foreign(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->foreign();
}

double lefiVia_foreignX(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->foreignX();
}

double lefiVia_foreignY(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->foreignY();
}

int lefiVia_foreignOrient(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->foreignOrient();
}

char* lefiVia_foreignOrientStr(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->foreignOrientStr();
}

int lefiVia_hasViaRule(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->hasViaRule();
}

const char* lefiVia_viaRuleName(const ::lefiVia* obj)
{
  return ((const LefParser::lefiVia*) obj)->viaRuleName();
}

double lefiVia_xCutSize(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->xCutSize();
}

double lefiVia_yCutSize(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->yCutSize();
}

const char* lefiVia_botMetalLayer(const ::lefiVia* obj)
{
  return ((const LefParser::lefiVia*) obj)->botMetalLayer();
}

const char* lefiVia_cutLayer(const ::lefiVia* obj)
{
  return ((const LefParser::lefiVia*) obj)->cutLayer();
}

const char* lefiVia_topMetalLayer(const ::lefiVia* obj)
{
  return ((const LefParser::lefiVia*) obj)->topMetalLayer();
}

double lefiVia_xCutSpacing(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->xCutSpacing();
}

double lefiVia_yCutSpacing(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->yCutSpacing();
}

double lefiVia_xBotEnc(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->xBotEnc();
}

double lefiVia_yBotEnc(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->yBotEnc();
}

double lefiVia_xTopEnc(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->xTopEnc();
}

double lefiVia_yTopEnc(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->yTopEnc();
}

int lefiVia_hasRowCol(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->hasRowCol();
}

int lefiVia_numCutRows(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->numCutRows();
}

int lefiVia_numCutCols(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->numCutCols();
}

int lefiVia_hasOrigin(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->hasOrigin();
}

double lefiVia_xOffset(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->xOffset();
}

double lefiVia_yOffset(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->yOffset();
}

int lefiVia_hasOffset(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->hasOffset();
}

double lefiVia_xBotOffset(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->xBotOffset();
}

double lefiVia_yBotOffset(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->yBotOffset();
}

double lefiVia_xTopOffset(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->xTopOffset();
}

double lefiVia_yTopOffset(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->yTopOffset();
}

int lefiVia_hasCutPattern(const ::lefiVia* obj)
{
  return ((LefParser::lefiVia*) obj)->hasCutPattern();
}

const char* lefiVia_cutPattern(const ::lefiVia* obj)
{
  return ((const LefParser::lefiVia*) obj)->cutPattern();
}

void lefiVia_print(const ::lefiVia* obj, FILE* f)
{
  ((LefParser::lefiVia*) obj)->print(f);
}
