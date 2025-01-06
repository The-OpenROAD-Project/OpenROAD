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

#include "defiPinCap.h"
#include "defiPinCap.hpp"

union udefiPoints
{
  DefParser::defiPoints cpp;
  ::defiPoints c;
};

// Wrappers definitions.
int defiPinCap_pin(const ::defiPinCap* obj)
{
  return ((DefParser::defiPinCap*) obj)->pin();
}

double defiPinCap_cap(const ::defiPinCap* obj)
{
  return ((DefParser::defiPinCap*) obj)->cap();
}

void defiPinCap_print(const ::defiPinCap* obj, FILE* f)
{
  ((DefParser::defiPinCap*) obj)->print(f);
}

char* defiPinAntennaModel_antennaOxide(const ::defiPinAntennaModel* obj)
{
  return ((DefParser::defiPinAntennaModel*) obj)->antennaOxide();
}

int defiPinAntennaModel_hasAPinGateArea(const ::defiPinAntennaModel* obj)
{
  return ((DefParser::defiPinAntennaModel*) obj)->hasAPinGateArea();
}

int defiPinAntennaModel_numAPinGateArea(const ::defiPinAntennaModel* obj)
{
  return ((DefParser::defiPinAntennaModel*) obj)->numAPinGateArea();
}

int defiPinAntennaModel_APinGateArea(const ::defiPinAntennaModel* obj,
                                     int index)
{
  return ((DefParser::defiPinAntennaModel*) obj)->APinGateArea(index);
}

int defiPinAntennaModel_hasAPinGateAreaLayer(const ::defiPinAntennaModel* obj,
                                             int index)
{
  return ((DefParser::defiPinAntennaModel*) obj)->hasAPinGateAreaLayer(index);
}

const char* defiPinAntennaModel_APinGateAreaLayer(
    const ::defiPinAntennaModel* obj,
    int index)
{
  return ((const DefParser::defiPinAntennaModel*) obj)
      ->APinGateAreaLayer(index);
}

int defiPinAntennaModel_hasAPinMaxAreaCar(const ::defiPinAntennaModel* obj)
{
  return ((DefParser::defiPinAntennaModel*) obj)->hasAPinMaxAreaCar();
}

int defiPinAntennaModel_numAPinMaxAreaCar(const ::defiPinAntennaModel* obj)
{
  return ((DefParser::defiPinAntennaModel*) obj)->numAPinMaxAreaCar();
}

int defiPinAntennaModel_APinMaxAreaCar(const ::defiPinAntennaModel* obj,
                                       int index)
{
  return ((DefParser::defiPinAntennaModel*) obj)->APinMaxAreaCar(index);
}

int defiPinAntennaModel_hasAPinMaxAreaCarLayer(const ::defiPinAntennaModel* obj,
                                               int index)
{
  return ((DefParser::defiPinAntennaModel*) obj)->hasAPinMaxAreaCarLayer(index);
}

const char* defiPinAntennaModel_APinMaxAreaCarLayer(
    const ::defiPinAntennaModel* obj,
    int index)
{
  return ((const DefParser::defiPinAntennaModel*) obj)
      ->APinMaxAreaCarLayer(index);
}

int defiPinAntennaModel_hasAPinMaxSideAreaCar(const ::defiPinAntennaModel* obj)
{
  return ((DefParser::defiPinAntennaModel*) obj)->hasAPinMaxSideAreaCar();
}

int defiPinAntennaModel_numAPinMaxSideAreaCar(const ::defiPinAntennaModel* obj)
{
  return ((DefParser::defiPinAntennaModel*) obj)->numAPinMaxSideAreaCar();
}

int defiPinAntennaModel_APinMaxSideAreaCar(const ::defiPinAntennaModel* obj,
                                           int index)
{
  return ((DefParser::defiPinAntennaModel*) obj)->APinMaxSideAreaCar(index);
}

int defiPinAntennaModel_hasAPinMaxSideAreaCarLayer(
    const ::defiPinAntennaModel* obj,
    int index)
{
  return ((DefParser::defiPinAntennaModel*) obj)
      ->hasAPinMaxSideAreaCarLayer(index);
}

const char* defiPinAntennaModel_APinMaxSideAreaCarLayer(
    const ::defiPinAntennaModel* obj,
    int index)
{
  return ((const DefParser::defiPinAntennaModel*) obj)
      ->APinMaxSideAreaCarLayer(index);
}

int defiPinAntennaModel_hasAPinMaxCutCar(const ::defiPinAntennaModel* obj)
{
  return ((DefParser::defiPinAntennaModel*) obj)->hasAPinMaxCutCar();
}

int defiPinAntennaModel_numAPinMaxCutCar(const ::defiPinAntennaModel* obj)
{
  return ((DefParser::defiPinAntennaModel*) obj)->numAPinMaxCutCar();
}

int defiPinAntennaModel_APinMaxCutCar(const ::defiPinAntennaModel* obj,
                                      int index)
{
  return ((DefParser::defiPinAntennaModel*) obj)->APinMaxCutCar(index);
}

int defiPinAntennaModel_hasAPinMaxCutCarLayer(const ::defiPinAntennaModel* obj,
                                              int index)
{
  return ((DefParser::defiPinAntennaModel*) obj)->hasAPinMaxCutCarLayer(index);
}

const char* defiPinAntennaModel_APinMaxCutCarLayer(
    const ::defiPinAntennaModel* obj,
    int index)
{
  return ((const DefParser::defiPinAntennaModel*) obj)
      ->APinMaxCutCarLayer(index);
}

int defiPinPort_numLayer(const ::defiPinPort* obj)
{
  return ((DefParser::defiPinPort*) obj)->numLayer();
}

const char* defiPinPort_layer(const ::defiPinPort* obj, int index)
{
  return ((const DefParser::defiPinPort*) obj)->layer(index);
}

void defiPinPort_bounds(const ::defiPinPort* obj,
                        int index,
                        int* xl,
                        int* yl,
                        int* xh,
                        int* yh)
{
  ((DefParser::defiPinPort*) obj)->bounds(index, xl, yl, xh, yh);
}

int defiPinPort_hasLayerSpacing(const ::defiPinPort* obj, int index)
{
  return ((DefParser::defiPinPort*) obj)->hasLayerSpacing(index);
}

int defiPinPort_hasLayerDesignRuleWidth(const ::defiPinPort* obj, int index)
{
  return ((DefParser::defiPinPort*) obj)->hasLayerDesignRuleWidth(index);
}

int defiPinPort_layerSpacing(const ::defiPinPort* obj, int index)
{
  return ((DefParser::defiPinPort*) obj)->layerSpacing(index);
}

int defiPinPort_layerMask(const ::defiPinPort* obj, int index)
{
  return ((DefParser::defiPinPort*) obj)->layerMask(index);
}

int defiPinPort_layerDesignRuleWidth(const ::defiPinPort* obj, int index)
{
  return ((DefParser::defiPinPort*) obj)->layerDesignRuleWidth(index);
}

int defiPinPort_numPolygons(const ::defiPinPort* obj)
{
  return ((DefParser::defiPinPort*) obj)->numPolygons();
}

const char* defiPinPort_polygonName(const ::defiPinPort* obj, int index)
{
  return ((const DefParser::defiPinPort*) obj)->polygonName(index);
}

::defiPoints defiPinPort_getPolygon(const ::defiPinPort* obj, int index)
{
  udefiPoints tmp;
  tmp.cpp = ((DefParser::defiPinPort*) obj)->getPolygon(index);
  return tmp.c;
}

int defiPinPort_hasPolygonSpacing(const ::defiPinPort* obj, int index)
{
  return ((DefParser::defiPinPort*) obj)->hasPolygonSpacing(index);
}

int defiPinPort_hasPolygonDesignRuleWidth(const ::defiPinPort* obj, int index)
{
  return ((DefParser::defiPinPort*) obj)->hasPolygonDesignRuleWidth(index);
}

int defiPinPort_polygonSpacing(const ::defiPinPort* obj, int index)
{
  return ((DefParser::defiPinPort*) obj)->polygonSpacing(index);
}

int defiPinPort_polygonDesignRuleWidth(const ::defiPinPort* obj, int index)
{
  return ((DefParser::defiPinPort*) obj)->polygonDesignRuleWidth(index);
}

int defiPinPort_polygonMask(const ::defiPinPort* obj, int index)
{
  return ((DefParser::defiPinPort*) obj)->polygonMask(index);
}

int defiPinPort_numVias(const ::defiPinPort* obj)
{
  return ((DefParser::defiPinPort*) obj)->numVias();
}

const char* defiPinPort_viaName(const ::defiPinPort* obj, int index)
{
  return ((const DefParser::defiPinPort*) obj)->viaName(index);
}

int defiPinPort_viaPtX(const ::defiPinPort* obj, int index)
{
  return ((DefParser::defiPinPort*) obj)->viaPtX(index);
}

int defiPinPort_viaPtY(const ::defiPinPort* obj, int index)
{
  return ((DefParser::defiPinPort*) obj)->viaPtY(index);
}

int defiPinPort_viaTopMask(const ::defiPinPort* obj, int index)
{
  return ((DefParser::defiPinPort*) obj)->viaTopMask(index);
}

int defiPinPort_viaCutMask(const ::defiPinPort* obj, int index)
{
  return ((DefParser::defiPinPort*) obj)->viaCutMask(index);
}

int defiPinPort_viaBottomMask(const ::defiPinPort* obj, int index)
{
  return ((DefParser::defiPinPort*) obj)->viaBottomMask(index);
}

int defiPinPort_hasPlacement(const ::defiPinPort* obj)
{
  return ((DefParser::defiPinPort*) obj)->hasPlacement();
}

int defiPinPort_isPlaced(const ::defiPinPort* obj)
{
  return ((DefParser::defiPinPort*) obj)->isPlaced();
}

int defiPinPort_isCover(const ::defiPinPort* obj)
{
  return ((DefParser::defiPinPort*) obj)->isCover();
}

int defiPinPort_isFixed(const ::defiPinPort* obj)
{
  return ((DefParser::defiPinPort*) obj)->isFixed();
}

int defiPinPort_placementX(const ::defiPinPort* obj)
{
  return ((DefParser::defiPinPort*) obj)->placementX();
}

int defiPinPort_placementY(const ::defiPinPort* obj)
{
  return ((DefParser::defiPinPort*) obj)->placementY();
}

int defiPinPort_orient(const ::defiPinPort* obj)
{
  return ((DefParser::defiPinPort*) obj)->orient();
}

const char* defiPinPort_orientStr(const ::defiPinPort* obj)
{
  return ((const DefParser::defiPinPort*) obj)->orientStr();
}

const char* defiPin_pinName(const ::defiPin* obj)
{
  return ((const DefParser::defiPin*) obj)->pinName();
}

const char* defiPin_netName(const ::defiPin* obj)
{
  return ((const DefParser::defiPin*) obj)->netName();
}

int defiPin_hasDirection(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->hasDirection();
}

int defiPin_hasUse(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->hasUse();
}

int defiPin_hasLayer(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->hasLayer();
}

int defiPin_hasPlacement(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->hasPlacement();
}

int defiPin_isUnplaced(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->isUnplaced();
}

int defiPin_isPlaced(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->isPlaced();
}

int defiPin_isCover(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->isCover();
}

int defiPin_isFixed(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->isFixed();
}

int defiPin_placementX(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->placementX();
}

int defiPin_placementY(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->placementY();
}

const char* defiPin_direction(const ::defiPin* obj)
{
  return ((const DefParser::defiPin*) obj)->direction();
}

const char* defiPin_use(const ::defiPin* obj)
{
  return ((const DefParser::defiPin*) obj)->use();
}

int defiPin_numLayer(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->numLayer();
}

const char* defiPin_layer(const ::defiPin* obj, int index)
{
  return ((const DefParser::defiPin*) obj)->layer(index);
}

void defiPin_bounds(const ::defiPin* obj,
                    int index,
                    int* xl,
                    int* yl,
                    int* xh,
                    int* yh)
{
  ((DefParser::defiPin*) obj)->bounds(index, xl, yl, xh, yh);
}

int defiPin_layerMask(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->layerMask(index);
}

int defiPin_hasLayerSpacing(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->hasLayerSpacing(index);
}

int defiPin_hasLayerDesignRuleWidth(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->hasLayerDesignRuleWidth(index);
}

int defiPin_layerSpacing(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->layerSpacing(index);
}

int defiPin_layerDesignRuleWidth(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->layerDesignRuleWidth(index);
}

int defiPin_numPolygons(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->numPolygons();
}

const char* defiPin_polygonName(const ::defiPin* obj, int index)
{
  return ((const DefParser::defiPin*) obj)->polygonName(index);
}

::defiPoints defiPin_getPolygon(const ::defiPin* obj, int index)
{
  udefiPoints tmp;
  tmp.cpp = ((DefParser::defiPin*) obj)->getPolygon(index);
  return tmp.c;
}

int defiPin_polygonMask(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->polygonMask(index);
}

int defiPin_hasPolygonSpacing(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->hasPolygonSpacing(index);
}

int defiPin_hasPolygonDesignRuleWidth(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->hasPolygonDesignRuleWidth(index);
}

int defiPin_polygonSpacing(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->polygonSpacing(index);
}

int defiPin_polygonDesignRuleWidth(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->polygonDesignRuleWidth(index);
}

int defiPin_hasNetExpr(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->hasNetExpr();
}

int defiPin_hasSupplySensitivity(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->hasSupplySensitivity();
}

int defiPin_hasGroundSensitivity(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->hasGroundSensitivity();
}

const char* defiPin_netExpr(const ::defiPin* obj)
{
  return ((const DefParser::defiPin*) obj)->netExpr();
}

const char* defiPin_supplySensitivity(const ::defiPin* obj)
{
  return ((const DefParser::defiPin*) obj)->supplySensitivity();
}

const char* defiPin_groundSensitivity(const ::defiPin* obj)
{
  return ((const DefParser::defiPin*) obj)->groundSensitivity();
}

int defiPin_orient(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->orient();
}

const char* defiPin_orientStr(const ::defiPin* obj)
{
  return ((const DefParser::defiPin*) obj)->orientStr();
}

int defiPin_hasSpecial(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->hasSpecial();
}

int defiPin_numVias(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->numVias();
}

const char* defiPin_viaName(const ::defiPin* obj, int index)
{
  return ((const DefParser::defiPin*) obj)->viaName(index);
}

int defiPin_viaTopMask(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->viaTopMask(index);
}

int defiPin_viaCutMask(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->viaCutMask(index);
}

int defiPin_viaBottomMask(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->viaBottomMask(index);
}

int defiPin_viaPtX(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->viaPtX(index);
}

int defiPin_viaPtY(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->viaPtY(index);
}

int defiPin_hasAPinPartialMetalArea(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->hasAPinPartialMetalArea();
}

int defiPin_numAPinPartialMetalArea(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->numAPinPartialMetalArea();
}

int defiPin_APinPartialMetalArea(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->APinPartialMetalArea(index);
}

int defiPin_hasAPinPartialMetalAreaLayer(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->hasAPinPartialMetalAreaLayer(index);
}

const char* defiPin_APinPartialMetalAreaLayer(const ::defiPin* obj, int index)
{
  return ((const DefParser::defiPin*) obj)->APinPartialMetalAreaLayer(index);
}

int defiPin_hasAPinPartialMetalSideArea(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->hasAPinPartialMetalSideArea();
}

int defiPin_numAPinPartialMetalSideArea(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->numAPinPartialMetalSideArea();
}

int defiPin_APinPartialMetalSideArea(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->APinPartialMetalSideArea(index);
}

int defiPin_hasAPinPartialMetalSideAreaLayer(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->hasAPinPartialMetalSideAreaLayer(index);
}

const char* defiPin_APinPartialMetalSideAreaLayer(const ::defiPin* obj,
                                                  int index)
{
  return ((const DefParser::defiPin*) obj)
      ->APinPartialMetalSideAreaLayer(index);
}

int defiPin_hasAPinDiffArea(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->hasAPinDiffArea();
}

int defiPin_numAPinDiffArea(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->numAPinDiffArea();
}

int defiPin_APinDiffArea(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->APinDiffArea(index);
}

int defiPin_hasAPinDiffAreaLayer(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->hasAPinDiffAreaLayer(index);
}

const char* defiPin_APinDiffAreaLayer(const ::defiPin* obj, int index)
{
  return ((const DefParser::defiPin*) obj)->APinDiffAreaLayer(index);
}

int defiPin_hasAPinPartialCutArea(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->hasAPinPartialCutArea();
}

int defiPin_numAPinPartialCutArea(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->numAPinPartialCutArea();
}

int defiPin_APinPartialCutArea(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->APinPartialCutArea(index);
}

int defiPin_hasAPinPartialCutAreaLayer(const ::defiPin* obj, int index)
{
  return ((DefParser::defiPin*) obj)->hasAPinPartialCutAreaLayer(index);
}

const char* defiPin_APinPartialCutAreaLayer(const ::defiPin* obj, int index)
{
  return ((const DefParser::defiPin*) obj)->APinPartialCutAreaLayer(index);
}

int defiPin_numAntennaModel(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->numAntennaModel();
}

const ::defiPinAntennaModel* defiPin_antennaModel(const ::defiPin* obj,
                                                  int index)
{
  return (const ::defiPinAntennaModel*) ((DefParser::defiPin*) obj)
      ->antennaModel(index);
}

int defiPin_hasPort(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->hasPort();
}

int defiPin_numPorts(const ::defiPin* obj)
{
  return ((DefParser::defiPin*) obj)->numPorts();
}

const ::defiPinPort* defiPin_pinPort(const ::defiPin* obj, int index)
{
  return (const ::defiPinPort*) ((DefParser::defiPin*) obj)->pinPort(index);
}

void defiPin_print(const ::defiPin* obj, FILE* f)
{
  ((DefParser::defiPin*) obj)->print(f);
}
