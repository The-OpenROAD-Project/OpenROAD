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

#include "lefiMacro.h"
#include "lefiMacro.hpp"

union ulefiGeomRect
{
  LefParser::lefiGeomRect cpp;
  ::lefiGeomRect c;
};

// Wrappers definitions.
const ::lefiGeometries* lefiObstruction_geometries(const ::lefiObstruction* obj)
{
  return (const ::lefiGeometries*) ((LefParser::lefiObstruction*) obj)
      ->geometries();
}

void lefiObstruction_print(const ::lefiObstruction* obj, FILE* f)
{
  ((LefParser::lefiObstruction*) obj)->print(f);
}

int lefiPinAntennaModel_hasAntennaGateArea(const ::lefiPinAntennaModel* obj)
{
  return ((LefParser::lefiPinAntennaModel*) obj)->hasAntennaGateArea();
}

int lefiPinAntennaModel_hasAntennaMaxAreaCar(const ::lefiPinAntennaModel* obj)
{
  return ((LefParser::lefiPinAntennaModel*) obj)->hasAntennaMaxAreaCar();
}

int lefiPinAntennaModel_hasAntennaMaxSideAreaCar(
    const ::lefiPinAntennaModel* obj)
{
  return ((LefParser::lefiPinAntennaModel*) obj)->hasAntennaMaxSideAreaCar();
}

int lefiPinAntennaModel_hasAntennaMaxCutCar(const ::lefiPinAntennaModel* obj)
{
  return ((LefParser::lefiPinAntennaModel*) obj)->hasAntennaMaxCutCar();
}

char* lefiPinAntennaModel_antennaOxide(const ::lefiPinAntennaModel* obj)
{
  return ((LefParser::lefiPinAntennaModel*) obj)->antennaOxide();
}

int lefiPinAntennaModel_numAntennaGateArea(const ::lefiPinAntennaModel* obj)
{
  return ((LefParser::lefiPinAntennaModel*) obj)->numAntennaGateArea();
}

double lefiPinAntennaModel_antennaGateArea(const ::lefiPinAntennaModel* obj,
                                           int index)
{
  return ((LefParser::lefiPinAntennaModel*) obj)->antennaGateArea(index);
}

const char* lefiPinAntennaModel_antennaGateAreaLayer(
    const ::lefiPinAntennaModel* obj,
    int index)
{
  return ((const LefParser::lefiPinAntennaModel*) obj)
      ->antennaGateAreaLayer(index);
}

int lefiPinAntennaModel_numAntennaMaxAreaCar(const ::lefiPinAntennaModel* obj)
{
  return ((LefParser::lefiPinAntennaModel*) obj)->numAntennaMaxAreaCar();
}

double lefiPinAntennaModel_antennaMaxAreaCar(const ::lefiPinAntennaModel* obj,
                                             int index)
{
  return ((LefParser::lefiPinAntennaModel*) obj)->antennaMaxAreaCar(index);
}

const char* lefiPinAntennaModel_antennaMaxAreaCarLayer(
    const ::lefiPinAntennaModel* obj,
    int index)
{
  return ((const LefParser::lefiPinAntennaModel*) obj)
      ->antennaMaxAreaCarLayer(index);
}

int lefiPinAntennaModel_numAntennaMaxSideAreaCar(
    const ::lefiPinAntennaModel* obj)
{
  return ((LefParser::lefiPinAntennaModel*) obj)->numAntennaMaxSideAreaCar();
}

double lefiPinAntennaModel_antennaMaxSideAreaCar(
    const ::lefiPinAntennaModel* obj,
    int index)
{
  return ((LefParser::lefiPinAntennaModel*) obj)->antennaMaxSideAreaCar(index);
}

const char* lefiPinAntennaModel_antennaMaxSideAreaCarLayer(
    const ::lefiPinAntennaModel* obj,
    int index)
{
  return ((const LefParser::lefiPinAntennaModel*) obj)
      ->antennaMaxSideAreaCarLayer(index);
}

int lefiPinAntennaModel_numAntennaMaxCutCar(const ::lefiPinAntennaModel* obj)
{
  return ((LefParser::lefiPinAntennaModel*) obj)->numAntennaMaxCutCar();
}

double lefiPinAntennaModel_antennaMaxCutCar(const ::lefiPinAntennaModel* obj,
                                            int index)
{
  return ((LefParser::lefiPinAntennaModel*) obj)->antennaMaxCutCar(index);
}

const char* lefiPinAntennaModel_antennaMaxCutCarLayer(
    const ::lefiPinAntennaModel* obj,
    int index)
{
  return ((const LefParser::lefiPinAntennaModel*) obj)
      ->antennaMaxCutCarLayer(index);
}

int lefiPinAntennaModel_hasReturn(const ::lefiPinAntennaModel* obj)
{
  return ((LefParser::lefiPinAntennaModel*) obj)->hasReturn();
}

int lefiPin_hasForeign(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasForeign();
}

int lefiPin_hasForeignOrient(const ::lefiPin* obj, int index)
{
  return ((LefParser::lefiPin*) obj)->hasForeignOrient(index);
}

int lefiPin_hasForeignPoint(const ::lefiPin* obj, int index)
{
  return ((LefParser::lefiPin*) obj)->hasForeignPoint(index);
}

int lefiPin_hasLEQ(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasLEQ();
}

int lefiPin_hasDirection(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasDirection();
}

int lefiPin_hasUse(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasUse();
}

int lefiPin_hasShape(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasShape();
}

int lefiPin_hasMustjoin(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasMustjoin();
}

int lefiPin_hasOutMargin(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasOutMargin();
}

int lefiPin_hasOutResistance(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasOutResistance();
}

int lefiPin_hasInMargin(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasInMargin();
}

int lefiPin_hasPower(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasPower();
}

int lefiPin_hasLeakage(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasLeakage();
}

int lefiPin_hasMaxload(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasMaxload();
}

int lefiPin_hasMaxdelay(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasMaxdelay();
}

int lefiPin_hasCapacitance(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasCapacitance();
}

int lefiPin_hasResistance(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasResistance();
}

int lefiPin_hasPulldownres(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasPulldownres();
}

int lefiPin_hasTieoffr(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasTieoffr();
}

int lefiPin_hasVHI(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasVHI();
}

int lefiPin_hasVLO(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasVLO();
}

int lefiPin_hasRiseVoltage(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasRiseVoltage();
}

int lefiPin_hasFallVoltage(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasFallVoltage();
}

int lefiPin_hasRiseThresh(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasRiseThresh();
}

int lefiPin_hasFallThresh(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasFallThresh();
}

int lefiPin_hasRiseSatcur(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasRiseSatcur();
}

int lefiPin_hasFallSatcur(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasFallSatcur();
}

int lefiPin_hasCurrentSource(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasCurrentSource();
}

int lefiPin_hasTables(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasTables();
}

int lefiPin_hasAntennaSize(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasAntennaSize();
}

int lefiPin_hasAntennaMetalArea(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasAntennaMetalArea();
}

int lefiPin_hasAntennaMetalLength(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasAntennaMetalLength();
}

int lefiPin_hasAntennaPartialMetalArea(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasAntennaPartialMetalArea();
}

int lefiPin_hasAntennaPartialMetalSideArea(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasAntennaPartialMetalSideArea();
}

int lefiPin_hasAntennaPartialCutArea(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasAntennaPartialCutArea();
}

int lefiPin_hasAntennaDiffArea(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasAntennaDiffArea();
}

int lefiPin_hasAntennaModel(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasAntennaModel();
}

int lefiPin_hasTaperRule(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasTaperRule();
}

int lefiPin_hasRiseSlewLimit(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasRiseSlewLimit();
}

int lefiPin_hasFallSlewLimit(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasFallSlewLimit();
}

int lefiPin_hasNetExpr(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasNetExpr();
}

int lefiPin_hasSupplySensitivity(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasSupplySensitivity();
}

int lefiPin_hasGroundSensitivity(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->hasGroundSensitivity();
}

const char* lefiPin_name(const ::lefiPin* obj)
{
  return ((const LefParser::lefiPin*) obj)->name();
}

int lefiPin_numPorts(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->numPorts();
}

const ::lefiGeometries* lefiPin_port(const ::lefiPin* obj, int index)
{
  return (const ::lefiGeometries*) ((LefParser::lefiPin*) obj)->port(index);
}

int lefiPin_numForeigns(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->numForeigns();
}

const char* lefiPin_foreignName(const ::lefiPin* obj, int index)
{
  return ((const LefParser::lefiPin*) obj)->foreignName(index);
}

const char* lefiPin_taperRule(const ::lefiPin* obj)
{
  return ((const LefParser::lefiPin*) obj)->taperRule();
}

int lefiPin_foreignOrient(const ::lefiPin* obj, int index)
{
  return ((LefParser::lefiPin*) obj)->foreignOrient(index);
}

const char* lefiPin_foreignOrientStr(const ::lefiPin* obj, int index)
{
  return ((const LefParser::lefiPin*) obj)->foreignOrientStr(index);
}

double lefiPin_foreignX(const ::lefiPin* obj, int index)
{
  return ((LefParser::lefiPin*) obj)->foreignX(index);
}

double lefiPin_foreignY(const ::lefiPin* obj, int index)
{
  return ((LefParser::lefiPin*) obj)->foreignY(index);
}

const char* lefiPin_LEQ(const ::lefiPin* obj)
{
  return ((const LefParser::lefiPin*) obj)->LEQ();
}

const char* lefiPin_direction(const ::lefiPin* obj)
{
  return ((const LefParser::lefiPin*) obj)->direction();
}

const char* lefiPin_use(const ::lefiPin* obj)
{
  return ((const LefParser::lefiPin*) obj)->use();
}

const char* lefiPin_shape(const ::lefiPin* obj)
{
  return ((const LefParser::lefiPin*) obj)->shape();
}

const char* lefiPin_mustjoin(const ::lefiPin* obj)
{
  return ((const LefParser::lefiPin*) obj)->mustjoin();
}

double lefiPin_outMarginHigh(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->outMarginHigh();
}

double lefiPin_outMarginLow(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->outMarginLow();
}

double lefiPin_outResistanceHigh(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->outResistanceHigh();
}

double lefiPin_outResistanceLow(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->outResistanceLow();
}

double lefiPin_inMarginHigh(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->inMarginHigh();
}

double lefiPin_inMarginLow(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->inMarginLow();
}

double lefiPin_power(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->power();
}

double lefiPin_leakage(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->leakage();
}

double lefiPin_maxload(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->maxload();
}

double lefiPin_maxdelay(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->maxdelay();
}

double lefiPin_capacitance(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->capacitance();
}

double lefiPin_resistance(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->resistance();
}

double lefiPin_pulldownres(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->pulldownres();
}

double lefiPin_tieoffr(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->tieoffr();
}

double lefiPin_VHI(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->VHI();
}

double lefiPin_VLO(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->VLO();
}

double lefiPin_riseVoltage(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->riseVoltage();
}

double lefiPin_fallVoltage(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->fallVoltage();
}

double lefiPin_riseThresh(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->riseThresh();
}

double lefiPin_fallThresh(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->fallThresh();
}

double lefiPin_riseSatcur(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->riseSatcur();
}

double lefiPin_fallSatcur(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->fallSatcur();
}

double lefiPin_riseSlewLimit(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->riseSlewLimit();
}

double lefiPin_fallSlewLimit(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->fallSlewLimit();
}

const char* lefiPin_currentSource(const ::lefiPin* obj)
{
  return ((const LefParser::lefiPin*) obj)->currentSource();
}

const char* lefiPin_tableHighName(const ::lefiPin* obj)
{
  return ((const LefParser::lefiPin*) obj)->tableHighName();
}

const char* lefiPin_tableLowName(const ::lefiPin* obj)
{
  return ((const LefParser::lefiPin*) obj)->tableLowName();
}

int lefiPin_numAntennaSize(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->numAntennaSize();
}

double lefiPin_antennaSize(const ::lefiPin* obj, int index)
{
  return ((LefParser::lefiPin*) obj)->antennaSize(index);
}

const char* lefiPin_antennaSizeLayer(const ::lefiPin* obj, int index)
{
  return ((const LefParser::lefiPin*) obj)->antennaSizeLayer(index);
}

int lefiPin_numAntennaMetalArea(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->numAntennaMetalArea();
}

double lefiPin_antennaMetalArea(const ::lefiPin* obj, int index)
{
  return ((LefParser::lefiPin*) obj)->antennaMetalArea(index);
}

const char* lefiPin_antennaMetalAreaLayer(const ::lefiPin* obj, int index)
{
  return ((const LefParser::lefiPin*) obj)->antennaMetalAreaLayer(index);
}

int lefiPin_numAntennaMetalLength(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->numAntennaMetalLength();
}

double lefiPin_antennaMetalLength(const ::lefiPin* obj, int index)
{
  return ((LefParser::lefiPin*) obj)->antennaMetalLength(index);
}

const char* lefiPin_antennaMetalLengthLayer(const ::lefiPin* obj, int index)
{
  return ((const LefParser::lefiPin*) obj)->antennaMetalLengthLayer(index);
}

int lefiPin_numAntennaPartialMetalArea(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->numAntennaPartialMetalArea();
}

double lefiPin_antennaPartialMetalArea(const ::lefiPin* obj, int index)
{
  return ((LefParser::lefiPin*) obj)->antennaPartialMetalArea(index);
}

const char* lefiPin_antennaPartialMetalAreaLayer(const ::lefiPin* obj,
                                                 int index)
{
  return ((const LefParser::lefiPin*) obj)->antennaPartialMetalAreaLayer(index);
}

int lefiPin_numAntennaPartialMetalSideArea(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->numAntennaPartialMetalSideArea();
}

double lefiPin_antennaPartialMetalSideArea(const ::lefiPin* obj, int index)
{
  return ((LefParser::lefiPin*) obj)->antennaPartialMetalSideArea(index);
}

const char* lefiPin_antennaPartialMetalSideAreaLayer(const ::lefiPin* obj,
                                                     int index)
{
  return ((const LefParser::lefiPin*) obj)
      ->antennaPartialMetalSideAreaLayer(index);
}

int lefiPin_numAntennaPartialCutArea(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->numAntennaPartialCutArea();
}

double lefiPin_antennaPartialCutArea(const ::lefiPin* obj, int index)
{
  return ((LefParser::lefiPin*) obj)->antennaPartialCutArea(index);
}

const char* lefiPin_antennaPartialCutAreaLayer(const ::lefiPin* obj, int index)
{
  return ((const LefParser::lefiPin*) obj)->antennaPartialCutAreaLayer(index);
}

int lefiPin_numAntennaDiffArea(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->numAntennaDiffArea();
}

double lefiPin_antennaDiffArea(const ::lefiPin* obj, int index)
{
  return ((LefParser::lefiPin*) obj)->antennaDiffArea(index);
}

const char* lefiPin_antennaDiffAreaLayer(const ::lefiPin* obj, int index)
{
  return ((const LefParser::lefiPin*) obj)->antennaDiffAreaLayer(index);
}

const char* lefiPin_netExpr(const ::lefiPin* obj)
{
  return ((const LefParser::lefiPin*) obj)->netExpr();
}

const char* lefiPin_supplySensitivity(const ::lefiPin* obj)
{
  return ((const LefParser::lefiPin*) obj)->supplySensitivity();
}

const char* lefiPin_groundSensitivity(const ::lefiPin* obj)
{
  return ((const LefParser::lefiPin*) obj)->groundSensitivity();
}

int lefiPin_numAntennaModel(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->numAntennaModel();
}

const ::lefiPinAntennaModel* lefiPin_antennaModel(const ::lefiPin* obj,
                                                  int index)
{
  return (const ::lefiPinAntennaModel*) ((LefParser::lefiPin*) obj)
      ->antennaModel(index);
}

int lefiPin_numProperties(const ::lefiPin* obj)
{
  return ((LefParser::lefiPin*) obj)->numProperties();
}

const char* lefiPin_propName(const ::lefiPin* obj, int index)
{
  return ((const LefParser::lefiPin*) obj)->propName(index);
}

const char* lefiPin_propValue(const ::lefiPin* obj, int index)
{
  return ((const LefParser::lefiPin*) obj)->propValue(index);
}

double lefiPin_propNum(const ::lefiPin* obj, int index)
{
  return ((LefParser::lefiPin*) obj)->propNum(index);
}

const char lefiPin_propType(const ::lefiPin* obj, int index)
{
  return ((const LefParser::lefiPin*) obj)->propType(index);
}

int lefiPin_propIsNumber(const ::lefiPin* obj, int index)
{
  return ((LefParser::lefiPin*) obj)->propIsNumber(index);
}

int lefiPin_propIsString(const ::lefiPin* obj, int index)
{
  return ((LefParser::lefiPin*) obj)->propIsString(index);
}

void lefiPin_print(const ::lefiPin* obj, FILE* f)
{
  ((LefParser::lefiPin*) obj)->print(f);
}

int lefiDensity_numLayer(const ::lefiDensity* obj)
{
  return ((LefParser::lefiDensity*) obj)->numLayer();
}

char* lefiDensity_layerName(const ::lefiDensity* obj, int index)
{
  return ((LefParser::lefiDensity*) obj)->layerName(index);
}

int lefiDensity_numRects(const ::lefiDensity* obj, int index)
{
  return ((LefParser::lefiDensity*) obj)->numRects(index);
}

::lefiGeomRect lefiDensity_getRect(const ::lefiDensity* obj,
                                   int index,
                                   int rectIndex)
{
  ulefiGeomRect tmp;
  tmp.cpp = ((LefParser::lefiDensity*) obj)->getRect(index, rectIndex);
  return tmp.c;
}

double lefiDensity_densityValue(const ::lefiDensity* obj,
                                int index,
                                int rectIndex)
{
  return ((LefParser::lefiDensity*) obj)->densityValue(index, rectIndex);
}

void lefiDensity_print(const ::lefiDensity* obj, FILE* f)
{
  ((LefParser::lefiDensity*) obj)->print(f);
}

int lefiMacro_hasClass(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->hasClass();
}

int lefiMacro_hasGenerator(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->hasGenerator();
}

int lefiMacro_hasGenerate(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->hasGenerate();
}

int lefiMacro_hasPower(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->hasPower();
}

int lefiMacro_hasOrigin(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->hasOrigin();
}

int lefiMacro_hasEEQ(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->hasEEQ();
}

int lefiMacro_hasLEQ(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->hasLEQ();
}

int lefiMacro_hasSource(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->hasSource();
}

int lefiMacro_hasXSymmetry(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->hasXSymmetry();
}

int lefiMacro_hasYSymmetry(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->hasYSymmetry();
}

int lefiMacro_has90Symmetry(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->has90Symmetry();
}

int lefiMacro_hasSiteName(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->hasSiteName();
}

int lefiMacro_hasSitePattern(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->hasSitePattern();
}

int lefiMacro_hasSize(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->hasSize();
}

int lefiMacro_hasForeign(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->hasForeign();
}

int lefiMacro_hasForeignOrigin(const ::lefiMacro* obj, int index)
{
  return ((LefParser::lefiMacro*) obj)->hasForeignOrigin(index);
}

int lefiMacro_hasForeignOrient(const ::lefiMacro* obj, int index)
{
  return ((LefParser::lefiMacro*) obj)->hasForeignOrient(index);
}

int lefiMacro_hasForeignPoint(const ::lefiMacro* obj, int index)
{
  return ((LefParser::lefiMacro*) obj)->hasForeignPoint(index);
}

int lefiMacro_hasClockType(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->hasClockType();
}

int lefiMacro_isBuffer(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->isBuffer();
}

int lefiMacro_isInverter(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->isInverter();
}

int lefiMacro_isFixedMask(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->isFixedMask();
}

int lefiMacro_numSitePattern(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->numSitePattern();
}

int lefiMacro_numProperties(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->numProperties();
}

const char* lefiMacro_propName(const ::lefiMacro* obj, int index)
{
  return ((const LefParser::lefiMacro*) obj)->propName(index);
}

const char* lefiMacro_propValue(const ::lefiMacro* obj, int index)
{
  return ((const LefParser::lefiMacro*) obj)->propValue(index);
}

double lefiMacro_propNum(const ::lefiMacro* obj, int index)
{
  return ((LefParser::lefiMacro*) obj)->propNum(index);
}

const char lefiMacro_propType(const ::lefiMacro* obj, int index)
{
  return ((const LefParser::lefiMacro*) obj)->propType(index);
}

int lefiMacro_propIsNumber(const ::lefiMacro* obj, int index)
{
  return ((LefParser::lefiMacro*) obj)->propIsNumber(index);
}

int lefiMacro_propIsString(const ::lefiMacro* obj, int index)
{
  return ((LefParser::lefiMacro*) obj)->propIsString(index);
}

const char* lefiMacro_name(const ::lefiMacro* obj)
{
  return ((const LefParser::lefiMacro*) obj)->name();
}

const char* lefiMacro_macroClass(const ::lefiMacro* obj)
{
  return ((const LefParser::lefiMacro*) obj)->macroClass();
}

const char* lefiMacro_generator(const ::lefiMacro* obj)
{
  return ((const LefParser::lefiMacro*) obj)->generator();
}

const char* lefiMacro_EEQ(const ::lefiMacro* obj)
{
  return ((const LefParser::lefiMacro*) obj)->EEQ();
}

const char* lefiMacro_LEQ(const ::lefiMacro* obj)
{
  return ((const LefParser::lefiMacro*) obj)->LEQ();
}

const char* lefiMacro_source(const ::lefiMacro* obj)
{
  return ((const LefParser::lefiMacro*) obj)->source();
}

const char* lefiMacro_clockType(const ::lefiMacro* obj)
{
  return ((const LefParser::lefiMacro*) obj)->clockType();
}

double lefiMacro_originX(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->originX();
}

double lefiMacro_originY(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->originY();
}

double lefiMacro_power(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->power();
}

void lefiMacro_generate(const ::lefiMacro* obj, char** name1, char** name2)
{
  ((LefParser::lefiMacro*) obj)->generate(name1, name2);
}

const ::lefiSitePattern* lefiMacro_sitePattern(const ::lefiMacro* obj,
                                               int index)
{
  return (const ::lefiSitePattern*) ((LefParser::lefiMacro*) obj)
      ->sitePattern(index);
}

const char* lefiMacro_siteName(const ::lefiMacro* obj)
{
  return ((const LefParser::lefiMacro*) obj)->siteName();
}

double lefiMacro_sizeX(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->sizeX();
}

double lefiMacro_sizeY(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->sizeY();
}

int lefiMacro_numForeigns(const ::lefiMacro* obj)
{
  return ((LefParser::lefiMacro*) obj)->numForeigns();
}

int lefiMacro_foreignOrient(const ::lefiMacro* obj, int index)
{
  return ((LefParser::lefiMacro*) obj)->foreignOrient(index);
}

const char* lefiMacro_foreignOrientStr(const ::lefiMacro* obj, int index)
{
  return ((const LefParser::lefiMacro*) obj)->foreignOrientStr(index);
}

double lefiMacro_foreignX(const ::lefiMacro* obj, int index)
{
  return ((LefParser::lefiMacro*) obj)->foreignX(index);
}

double lefiMacro_foreignY(const ::lefiMacro* obj, int index)
{
  return ((LefParser::lefiMacro*) obj)->foreignY(index);
}

const char* lefiMacro_foreignName(const ::lefiMacro* obj, int index)
{
  return ((const LefParser::lefiMacro*) obj)->foreignName(index);
}

void lefiMacro_print(const ::lefiMacro* obj, FILE* f)
{
  ((LefParser::lefiMacro*) obj)->print(f);
}
