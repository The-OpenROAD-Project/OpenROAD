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
//  $State: xxx $
// *****************************************************************************
// *****************************************************************************

#define EXTERN extern "C"

#include "lefiLayer.h"
#include "lefiLayer.hpp"

// Wrappers definitions.
int lefiAntennaPWL_numPWL(const ::lefiAntennaPWL* obj)
{
  return ((LefParser::lefiAntennaPWL*) obj)->numPWL();
}

char* lefiLayerDensity_type(const ::lefiLayerDensity* obj)
{
  return ((LefParser::lefiLayerDensity*) obj)->type();
}

int lefiLayerDensity_hasOneEntry(const ::lefiLayerDensity* obj)
{
  return ((LefParser::lefiLayerDensity*) obj)->hasOneEntry();
}

double lefiLayerDensity_oneEntry(const ::lefiLayerDensity* obj)
{
  return ((LefParser::lefiLayerDensity*) obj)->oneEntry();
}

int lefiLayerDensity_numFrequency(const ::lefiLayerDensity* obj)
{
  return ((LefParser::lefiLayerDensity*) obj)->numFrequency();
}

double lefiLayerDensity_frequency(const ::lefiLayerDensity* obj, int index)
{
  return ((LefParser::lefiLayerDensity*) obj)->frequency(index);
}

int lefiLayerDensity_numWidths(const ::lefiLayerDensity* obj)
{
  return ((LefParser::lefiLayerDensity*) obj)->numWidths();
}

double lefiLayerDensity_width(const ::lefiLayerDensity* obj, int index)
{
  return ((LefParser::lefiLayerDensity*) obj)->width(index);
}

int lefiLayerDensity_numTableEntries(const ::lefiLayerDensity* obj)
{
  return ((LefParser::lefiLayerDensity*) obj)->numTableEntries();
}

double lefiLayerDensity_tableEntry(const ::lefiLayerDensity* obj, int index)
{
  return ((LefParser::lefiLayerDensity*) obj)->tableEntry(index);
}

int lefiLayerDensity_numCutareas(const ::lefiLayerDensity* obj)
{
  return ((LefParser::lefiLayerDensity*) obj)->numCutareas();
}

double lefiLayerDensity_cutArea(const ::lefiLayerDensity* obj, int index)
{
  return ((LefParser::lefiLayerDensity*) obj)->cutArea(index);
}

int lefiParallel_numLength(const ::lefiParallel* obj)
{
  return ((LefParser::lefiParallel*) obj)->numLength();
}

int lefiParallel_numWidth(const ::lefiParallel* obj)
{
  return ((LefParser::lefiParallel*) obj)->numWidth();
}

double lefiParallel_length(const ::lefiParallel* obj, int iLength)
{
  return ((LefParser::lefiParallel*) obj)->length(iLength);
}

double lefiParallel_width(const ::lefiParallel* obj, int iWidth)
{
  return ((LefParser::lefiParallel*) obj)->width(iWidth);
}

double lefiParallel_widthSpacing(const ::lefiParallel* obj,
                                 int iWidth,
                                 int iWidthSpacing)
{
  return ((LefParser::lefiParallel*) obj)->widthSpacing(iWidth, iWidthSpacing);
}

int lefiInfluence_numInfluenceEntry(const ::lefiInfluence* obj)
{
  return ((LefParser::lefiInfluence*) obj)->numInfluenceEntry();
}

double lefiInfluence_width(const ::lefiInfluence* obj, int index)
{
  return ((LefParser::lefiInfluence*) obj)->width(index);
}

double lefiInfluence_distance(const ::lefiInfluence* obj, int index)
{
  return ((LefParser::lefiInfluence*) obj)->distance(index);
}

double lefiInfluence_spacing(const ::lefiInfluence* obj, int index)
{
  return ((LefParser::lefiInfluence*) obj)->spacing(index);
}

int lefiTwoWidths_numWidth(const ::lefiTwoWidths* obj)
{
  return ((LefParser::lefiTwoWidths*) obj)->numWidth();
}

double lefiTwoWidths_width(const ::lefiTwoWidths* obj, int iWidth)
{
  return ((LefParser::lefiTwoWidths*) obj)->width(iWidth);
}

int lefiTwoWidths_hasWidthPRL(const ::lefiTwoWidths* obj, int iWidth)
{
  return ((LefParser::lefiTwoWidths*) obj)->hasWidthPRL(iWidth);
}

double lefiTwoWidths_widthPRL(const ::lefiTwoWidths* obj, int iWidth)
{
  return ((LefParser::lefiTwoWidths*) obj)->widthPRL(iWidth);
}

int lefiTwoWidths_numWidthSpacing(const ::lefiTwoWidths* obj, int iWidth)
{
  return ((LefParser::lefiTwoWidths*) obj)->numWidthSpacing(iWidth);
}

double lefiTwoWidths_widthSpacing(const ::lefiTwoWidths* obj,
                                  int iWidth,
                                  int iWidthSpacing)
{
  return ((LefParser::lefiTwoWidths*) obj)->widthSpacing(iWidth, iWidthSpacing);
}

int lefiSpacingTable_isInfluence(const ::lefiSpacingTable* obj)
{
  return ((LefParser::lefiSpacingTable*) obj)->isInfluence();
}

const ::lefiInfluence* lefiSpacingTable_influence(const ::lefiSpacingTable* obj)
{
  return (const ::lefiInfluence*) ((LefParser::lefiSpacingTable*) obj)
      ->influence();
}

int lefiSpacingTable_isParallel(const ::lefiSpacingTable* obj)
{
  return ((LefParser::lefiSpacingTable*) obj)->isParallel();
}

const ::lefiParallel* lefiSpacingTable_parallel(const ::lefiSpacingTable* obj)
{
  return (const ::lefiParallel*) ((LefParser::lefiSpacingTable*) obj)
      ->parallel();
}

const ::lefiTwoWidths* lefiSpacingTable_twoWidths(const ::lefiSpacingTable* obj)
{
  return (const ::lefiTwoWidths*) ((LefParser::lefiSpacingTable*) obj)
      ->twoWidths();
}

int lefiOrthogonal_numOrthogonal(const ::lefiOrthogonal* obj)
{
  return ((LefParser::lefiOrthogonal*) obj)->numOrthogonal();
}

double lefiOrthogonal_cutWithin(const ::lefiOrthogonal* obj, int index)
{
  return ((LefParser::lefiOrthogonal*) obj)->cutWithin(index);
}

double lefiOrthogonal_orthoSpacing(const ::lefiOrthogonal* obj, int index)
{
  return ((LefParser::lefiOrthogonal*) obj)->orthoSpacing(index);
}

int lefiAntennaModel_hasAntennaAreaRatio(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaAreaRatio();
}

int lefiAntennaModel_hasAntennaDiffAreaRatio(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaDiffAreaRatio();
}

int lefiAntennaModel_hasAntennaDiffAreaRatioPWL(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaDiffAreaRatioPWL();
}

int lefiAntennaModel_hasAntennaCumAreaRatio(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaCumAreaRatio();
}

int lefiAntennaModel_hasAntennaCumDiffAreaRatio(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaCumDiffAreaRatio();
}

int lefiAntennaModel_hasAntennaCumDiffAreaRatioPWL(
    const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaCumDiffAreaRatioPWL();
}

int lefiAntennaModel_hasAntennaAreaFactor(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaAreaFactor();
}

int lefiAntennaModel_hasAntennaAreaFactorDUO(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaAreaFactorDUO();
}

int lefiAntennaModel_hasAntennaSideAreaRatio(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaSideAreaRatio();
}

int lefiAntennaModel_hasAntennaDiffSideAreaRatio(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaDiffSideAreaRatio();
}

int lefiAntennaModel_hasAntennaDiffSideAreaRatioPWL(
    const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaDiffSideAreaRatioPWL();
}

int lefiAntennaModel_hasAntennaCumSideAreaRatio(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaCumSideAreaRatio();
}

int lefiAntennaModel_hasAntennaCumDiffSideAreaRatio(
    const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaCumDiffSideAreaRatio();
}

int lefiAntennaModel_hasAntennaCumDiffSideAreaRatioPWL(
    const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)
      ->hasAntennaCumDiffSideAreaRatioPWL();
}

int lefiAntennaModel_hasAntennaSideAreaFactor(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaSideAreaFactor();
}

int lefiAntennaModel_hasAntennaSideAreaFactorDUO(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaSideAreaFactorDUO();
}

int lefiAntennaModel_hasAntennaCumRoutingPlusCut(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaCumRoutingPlusCut();
}

int lefiAntennaModel_hasAntennaGatePlusDiff(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaGatePlusDiff();
}

int lefiAntennaModel_hasAntennaAreaMinusDiff(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaAreaMinusDiff();
}

int lefiAntennaModel_hasAntennaAreaDiffReducePWL(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->hasAntennaAreaDiffReducePWL();
}

char* lefiAntennaModel_antennaOxide(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->antennaOxide();
}

double lefiAntennaModel_antennaAreaRatio(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->antennaAreaRatio();
}

double lefiAntennaModel_antennaDiffAreaRatio(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->antennaDiffAreaRatio();
}

const ::lefiAntennaPWL* lefiAntennaModel_antennaDiffAreaRatioPWL(
    const ::lefiAntennaModel* obj)
{
  return (const ::lefiAntennaPWL*) ((LefParser::lefiAntennaModel*) obj)
      ->antennaDiffAreaRatioPWL();
}

double lefiAntennaModel_antennaCumAreaRatio(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->antennaCumAreaRatio();
}

double lefiAntennaModel_antennaCumDiffAreaRatio(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->antennaCumDiffAreaRatio();
}

const ::lefiAntennaPWL* lefiAntennaModel_antennaCumDiffAreaRatioPWL(
    const ::lefiAntennaModel* obj)
{
  return (const ::lefiAntennaPWL*) ((LefParser::lefiAntennaModel*) obj)
      ->antennaCumDiffAreaRatioPWL();
}

double lefiAntennaModel_antennaAreaFactor(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->antennaAreaFactor();
}

double lefiAntennaModel_antennaSideAreaRatio(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->antennaSideAreaRatio();
}

double lefiAntennaModel_antennaDiffSideAreaRatio(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->antennaDiffSideAreaRatio();
}

const ::lefiAntennaPWL* lefiAntennaModel_antennaDiffSideAreaRatioPWL(
    const ::lefiAntennaModel* obj)
{
  return (const ::lefiAntennaPWL*) ((LefParser::lefiAntennaModel*) obj)
      ->antennaDiffSideAreaRatioPWL();
}

double lefiAntennaModel_antennaCumSideAreaRatio(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->antennaCumSideAreaRatio();
}

double lefiAntennaModel_antennaCumDiffSideAreaRatio(
    const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->antennaCumDiffSideAreaRatio();
}

const ::lefiAntennaPWL* lefiAntennaModel_antennaCumDiffSideAreaRatioPWL(
    const ::lefiAntennaModel* obj)
{
  return (const ::lefiAntennaPWL*) ((LefParser::lefiAntennaModel*) obj)
      ->antennaCumDiffSideAreaRatioPWL();
}

double lefiAntennaModel_antennaSideAreaFactor(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->antennaSideAreaFactor();
}

double lefiAntennaModel_antennaGatePlusDiff(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->antennaGatePlusDiff();
}

double lefiAntennaModel_antennaAreaMinusDiff(const ::lefiAntennaModel* obj)
{
  return ((LefParser::lefiAntennaModel*) obj)->antennaAreaMinusDiff();
}

const ::lefiAntennaPWL* lefiAntennaModel_antennaAreaDiffReducePWL(
    const ::lefiAntennaModel* obj)
{
  return (const ::lefiAntennaPWL*) ((LefParser::lefiAntennaModel*) obj)
      ->antennaAreaDiffReducePWL();
}

int lefiLayer_hasType(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasType();
}

int lefiLayer_hasLayerType(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasLayerType();
}

int lefiLayer_hasMask(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasMask();
}

int lefiLayer_hasPitch(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasPitch();
}

int lefiLayer_hasXYPitch(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasXYPitch();
}

int lefiLayer_hasOffset(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasOffset();
}

int lefiLayer_hasXYOffset(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasXYOffset();
}

int lefiLayer_hasWidth(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasWidth();
}

int lefiLayer_hasArea(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasArea();
}

int lefiLayer_hasDiagPitch(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasDiagPitch();
}

int lefiLayer_hasXYDiagPitch(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasXYDiagPitch();
}

int lefiLayer_hasDiagWidth(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasDiagWidth();
}

int lefiLayer_hasDiagSpacing(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasDiagSpacing();
}

int lefiLayer_hasSpacingNumber(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingNumber();
}

int lefiLayer_hasSpacingName(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingName(index);
}

int lefiLayer_hasSpacingLayerStack(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingLayerStack(index);
}

int lefiLayer_hasSpacingAdjacent(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingAdjacent(index);
}

int lefiLayer_hasSpacingCenterToCenter(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingCenterToCenter(index);
}

int lefiLayer_hasSpacingRange(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingRange(index);
}

int lefiLayer_hasSpacingRangeUseLengthThreshold(const ::lefiLayer* obj,
                                                int index)
{
  return ((LefParser::lefiLayer*) obj)
      ->hasSpacingRangeUseLengthThreshold(index);
}

int lefiLayer_hasSpacingRangeInfluence(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingRangeInfluence(index);
}

int lefiLayer_hasSpacingRangeInfluenceRange(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingRangeInfluenceRange(index);
}

int lefiLayer_hasSpacingRangeRange(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingRangeRange(index);
}

int lefiLayer_hasSpacingLengthThreshold(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingLengthThreshold(index);
}

int lefiLayer_hasSpacingLengthThresholdRange(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingLengthThresholdRange(index);
}

int lefiLayer_hasSpacingParallelOverlap(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingParallelOverlap(index);
}

int lefiLayer_hasSpacingArea(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingArea(index);
}

int lefiLayer_hasSpacingEndOfLine(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingEndOfLine(index);
}

int lefiLayer_hasSpacingParellelEdge(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingParellelEdge(index);
}

int lefiLayer_hasSpacingTwoEdges(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingTwoEdges(index);
}

int lefiLayer_hasSpacingAdjacentExcept(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingAdjacentExcept(index);
}

int lefiLayer_hasSpacingSamenet(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingSamenet(index);
}

int lefiLayer_hasSpacingSamenetPGonly(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingSamenetPGonly(index);
}

int lefiLayer_hasSpacingNotchLength(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingNotchLength(index);
}

int lefiLayer_hasSpacingEndOfNotchWidth(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingEndOfNotchWidth(index);
}

int lefiLayer_hasDirection(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasDirection();
}

int lefiLayer_hasResistance(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasResistance();
}

int lefiLayer_hasResistanceArray(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasResistanceArray();
}

int lefiLayer_hasCapacitance(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasCapacitance();
}

int lefiLayer_hasCapacitanceArray(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasCapacitanceArray();
}

int lefiLayer_hasHeight(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasHeight();
}

int lefiLayer_hasThickness(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasThickness();
}

int lefiLayer_hasWireExtension(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasWireExtension();
}

int lefiLayer_hasShrinkage(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasShrinkage();
}

int lefiLayer_hasCapMultiplier(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasCapMultiplier();
}

int lefiLayer_hasEdgeCap(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasEdgeCap();
}

int lefiLayer_hasAntennaLength(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasAntennaLength();
}

int lefiLayer_hasAntennaArea(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasAntennaArea();
}

int lefiLayer_hasCurrentDensityPoint(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasCurrentDensityPoint();
}

int lefiLayer_hasCurrentDensityArray(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasCurrentDensityArray();
}

int lefiLayer_hasAccurrentDensity(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasAccurrentDensity();
}

int lefiLayer_hasDccurrentDensity(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasDccurrentDensity();
}

int lefiLayer_numProps(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->numProps();
}

const char* lefiLayer_propName(const ::lefiLayer* obj, int index)
{
  return ((const LefParser::lefiLayer*) obj)->propName(index);
}

const char* lefiLayer_propValue(const ::lefiLayer* obj, int index)
{
  return ((const LefParser::lefiLayer*) obj)->propValue(index);
}

double lefiLayer_propNumber(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->propNumber(index);
}

const char lefiLayer_propType(const ::lefiLayer* obj, int index)
{
  return ((const LefParser::lefiLayer*) obj)->propType(index);
}

int lefiLayer_propIsNumber(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->propIsNumber(index);
}

int lefiLayer_propIsString(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->propIsString(index);
}

int lefiLayer_numSpacing(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->numSpacing();
}

char* lefiLayer_name(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->name();
}

const char* lefiLayer_type(const ::lefiLayer* obj)
{
  return ((const LefParser::lefiLayer*) obj)->type();
}

const char* lefiLayer_layerType(const ::lefiLayer* obj)
{
  return ((const LefParser::lefiLayer*) obj)->layerType();
}

double lefiLayer_pitch(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->pitch();
}

int lefiLayer_mask(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->mask();
}

double lefiLayer_pitchX(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->pitchX();
}

double lefiLayer_pitchY(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->pitchY();
}

double lefiLayer_offset(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->offset();
}

double lefiLayer_offsetX(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->offsetX();
}

double lefiLayer_offsetY(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->offsetY();
}

double lefiLayer_width(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->width();
}

double lefiLayer_area(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->area();
}

double lefiLayer_diagPitch(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->diagPitch();
}

double lefiLayer_diagPitchX(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->diagPitchX();
}

double lefiLayer_diagPitchY(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->diagPitchY();
}

double lefiLayer_diagWidth(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->diagWidth();
}

double lefiLayer_diagSpacing(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->diagSpacing();
}

double lefiLayer_spacing(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacing(index);
}

char* lefiLayer_spacingName(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingName(index);
}

int lefiLayer_spacingAdjacentCuts(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingAdjacentCuts(index);
}

double lefiLayer_spacingAdjacentWithin(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingAdjacentWithin(index);
}

double lefiLayer_spacingArea(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingArea(index);
}

double lefiLayer_spacingRangeMin(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingRangeMin(index);
}

double lefiLayer_spacingRangeMax(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingRangeMax(index);
}

double lefiLayer_spacingRangeInfluence(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingRangeInfluence(index);
}

double lefiLayer_spacingRangeInfluenceMin(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingRangeInfluenceMin(index);
}

double lefiLayer_spacingRangeInfluenceMax(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingRangeInfluenceMax(index);
}

double lefiLayer_spacingRangeRangeMin(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingRangeRangeMin(index);
}

double lefiLayer_spacingRangeRangeMax(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingRangeRangeMax(index);
}

double lefiLayer_spacingLengthThreshold(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingLengthThreshold(index);
}

double lefiLayer_spacingLengthThresholdRangeMin(const ::lefiLayer* obj,
                                                int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingLengthThresholdRangeMin(index);
}

double lefiLayer_spacingLengthThresholdRangeMax(const ::lefiLayer* obj,
                                                int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingLengthThresholdRangeMax(index);
}

double lefiLayer_spacingEolWidth(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingEolWidth(index);
}

double lefiLayer_spacingEolWithin(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingEolWithin(index);
}

double lefiLayer_spacingParSpace(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingParSpace(index);
}

double lefiLayer_spacingParWithin(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingParWithin(index);
}

double lefiLayer_spacingNotchLength(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingNotchLength(index);
}

double lefiLayer_spacingEndOfNotchWidth(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingEndOfNotchWidth(index);
}

double lefiLayer_spacingEndOfNotchSpacing(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingEndOfNotchSpacing(index);
}

double lefiLayer_spacingEndOfNotchLength(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->spacingEndOfNotchLength(index);
}

int lefiLayer_numMinimumcut(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->numMinimumcut();
}

int lefiLayer_minimumcut(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->minimumcut(index);
}

double lefiLayer_minimumcutWidth(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->minimumcutWidth(index);
}

int lefiLayer_hasMinimumcutWithin(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasMinimumcutWithin(index);
}

double lefiLayer_minimumcutWithin(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->minimumcutWithin(index);
}

int lefiLayer_hasMinimumcutConnection(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasMinimumcutConnection(index);
}

const char* lefiLayer_minimumcutConnection(const ::lefiLayer* obj, int index)
{
  return ((const LefParser::lefiLayer*) obj)->minimumcutConnection(index);
}

int lefiLayer_hasMinimumcutNumCuts(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasMinimumcutNumCuts(index);
}

double lefiLayer_minimumcutLength(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->minimumcutLength(index);
}

double lefiLayer_minimumcutDistance(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->minimumcutDistance(index);
}

const char* lefiLayer_direction(const ::lefiLayer* obj)
{
  return ((const LefParser::lefiLayer*) obj)->direction();
}

double lefiLayer_resistance(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->resistance();
}

double lefiLayer_capacitance(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->capacitance();
}

double lefiLayer_height(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->height();
}

double lefiLayer_wireExtension(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->wireExtension();
}

double lefiLayer_thickness(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->thickness();
}

double lefiLayer_shrinkage(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->shrinkage();
}

double lefiLayer_capMultiplier(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->capMultiplier();
}

double lefiLayer_edgeCap(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->edgeCap();
}

double lefiLayer_antennaLength(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->antennaLength();
}

double lefiLayer_antennaArea(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->antennaArea();
}

double lefiLayer_currentDensityPoint(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->currentDensityPoint();
}

void lefiLayer_currentDensityArray(const ::lefiLayer* obj,
                                   int* numPoints,
                                   double** widths,
                                   double** current)
{
  ((LefParser::lefiLayer*) obj)
      ->currentDensityArray(numPoints, widths, current);
}

void lefiLayer_capacitanceArray(const ::lefiLayer* obj,
                                int* numPoints,
                                double** widths,
                                double** resValues)
{
  ((LefParser::lefiLayer*) obj)->capacitanceArray(numPoints, widths, resValues);
}

void lefiLayer_resistanceArray(const ::lefiLayer* obj,
                               int* numPoints,
                               double** widths,
                               double** capValues)
{
  ((LefParser::lefiLayer*) obj)->resistanceArray(numPoints, widths, capValues);
}

int lefiLayer_numAccurrentDensity(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->numAccurrentDensity();
}

const ::lefiLayerDensity* lefiLayer_accurrent(const ::lefiLayer* obj, int index)
{
  return (const ::lefiLayerDensity*) ((LefParser::lefiLayer*) obj)
      ->accurrent(index);
}

int lefiLayer_numDccurrentDensity(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->numDccurrentDensity();
}

const ::lefiLayerDensity* lefiLayer_dccurrent(const ::lefiLayer* obj, int index)
{
  return (const ::lefiLayerDensity*) ((LefParser::lefiLayer*) obj)
      ->dccurrent(index);
}

int lefiLayer_numAntennaModel(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->numAntennaModel();
}

const ::lefiAntennaModel* lefiLayer_antennaModel(const ::lefiLayer* obj,
                                                 int index)
{
  return (const ::lefiAntennaModel*) ((LefParser::lefiLayer*) obj)
      ->antennaModel(index);
}

int lefiLayer_hasSlotWireWidth(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasSlotWireWidth();
}

int lefiLayer_hasSlotWireLength(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasSlotWireLength();
}

int lefiLayer_hasSlotWidth(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasSlotWidth();
}

int lefiLayer_hasSlotLength(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasSlotLength();
}

int lefiLayer_hasMaxAdjacentSlotSpacing(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasMaxAdjacentSlotSpacing();
}

int lefiLayer_hasMaxCoaxialSlotSpacing(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasMaxCoaxialSlotSpacing();
}

int lefiLayer_hasMaxEdgeSlotSpacing(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasMaxEdgeSlotSpacing();
}

int lefiLayer_hasSplitWireWidth(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasSplitWireWidth();
}

int lefiLayer_hasMinimumDensity(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasMinimumDensity();
}

int lefiLayer_hasMaximumDensity(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasMaximumDensity();
}

int lefiLayer_hasDensityCheckWindow(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasDensityCheckWindow();
}

int lefiLayer_hasDensityCheckStep(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasDensityCheckStep();
}

int lefiLayer_hasFillActiveSpacing(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasFillActiveSpacing();
}

int lefiLayer_hasMaxwidth(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasMaxwidth();
}

int lefiLayer_hasMinwidth(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasMinwidth();
}

int lefiLayer_hasMinstep(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasMinstep();
}

int lefiLayer_hasProtrusion(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasProtrusion();
}

double lefiLayer_slotWireWidth(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->slotWireWidth();
}

double lefiLayer_slotWireLength(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->slotWireLength();
}

double lefiLayer_slotWidth(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->slotWidth();
}

double lefiLayer_slotLength(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->slotLength();
}

double lefiLayer_maxAdjacentSlotSpacing(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->maxAdjacentSlotSpacing();
}

double lefiLayer_maxCoaxialSlotSpacing(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->maxCoaxialSlotSpacing();
}

double lefiLayer_maxEdgeSlotSpacing(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->maxEdgeSlotSpacing();
}

double lefiLayer_splitWireWidth(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->splitWireWidth();
}

double lefiLayer_minimumDensity(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->minimumDensity();
}

double lefiLayer_maximumDensity(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->maximumDensity();
}

double lefiLayer_densityCheckWindowLength(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->densityCheckWindowLength();
}

double lefiLayer_densityCheckWindowWidth(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->densityCheckWindowWidth();
}

double lefiLayer_densityCheckStep(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->densityCheckStep();
}

double lefiLayer_fillActiveSpacing(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->fillActiveSpacing();
}

double lefiLayer_maxwidth(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->maxwidth();
}

double lefiLayer_minwidth(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->minwidth();
}

double lefiLayer_protrusionWidth1(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->protrusionWidth1();
}

double lefiLayer_protrusionLength(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->protrusionLength();
}

double lefiLayer_protrusionWidth2(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->protrusionWidth2();
}

int lefiLayer_numMinstep(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->numMinstep();
}

double lefiLayer_minstep(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->minstep(index);
}

int lefiLayer_hasMinstepType(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasMinstepType(index);
}

char* lefiLayer_minstepType(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->minstepType(index);
}

int lefiLayer_hasMinstepLengthsum(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasMinstepLengthsum(index);
}

double lefiLayer_minstepLengthsum(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->minstepLengthsum(index);
}

int lefiLayer_hasMinstepMaxedges(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasMinstepMaxedges(index);
}

int lefiLayer_minstepMaxedges(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->minstepMaxedges(index);
}

int lefiLayer_hasMinstepMinAdjLength(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasMinstepMinAdjLength(index);
}

double lefiLayer_minstepMinAdjLength(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->minstepMinAdjLength(index);
}

int lefiLayer_hasMinstepMinBetLength(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasMinstepMinBetLength(index);
}

double lefiLayer_minstepMinBetLength(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->minstepMinBetLength(index);
}

int lefiLayer_hasMinstepXSameCorners(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasMinstepXSameCorners(index);
}

int lefiLayer_numMinenclosedarea(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->numMinenclosedarea();
}

double lefiLayer_minenclosedarea(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->minenclosedarea(index);
}

int lefiLayer_hasMinenclosedareaWidth(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasMinenclosedareaWidth(index);
}

double lefiLayer_minenclosedareaWidth(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->minenclosedareaWidth(index);
}

int lefiLayer_numEnclosure(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->numEnclosure();
}

int lefiLayer_hasEnclosureRule(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasEnclosureRule(index);
}

double lefiLayer_enclosureOverhang1(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->enclosureOverhang1(index);
}

double lefiLayer_enclosureOverhang2(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->enclosureOverhang2(index);
}

int lefiLayer_hasEnclosureWidth(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasEnclosureWidth(index);
}

double lefiLayer_enclosureMinWidth(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->enclosureMinWidth(index);
}

int lefiLayer_hasEnclosureExceptExtraCut(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasEnclosureExceptExtraCut(index);
}

double lefiLayer_enclosureExceptExtraCut(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->enclosureExceptExtraCut(index);
}

int lefiLayer_hasEnclosureMinLength(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasEnclosureMinLength(index);
}

double lefiLayer_enclosureMinLength(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->enclosureMinLength(index);
}

int lefiLayer_numPreferEnclosure(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->numPreferEnclosure();
}

int lefiLayer_hasPreferEnclosureRule(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasPreferEnclosureRule(index);
}

double lefiLayer_preferEnclosureOverhang1(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->preferEnclosureOverhang1(index);
}

double lefiLayer_preferEnclosureOverhang2(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->preferEnclosureOverhang2(index);
}

int lefiLayer_hasPreferEnclosureWidth(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->hasPreferEnclosureWidth(index);
}

double lefiLayer_preferEnclosureMinWidth(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->preferEnclosureMinWidth(index);
}

int lefiLayer_hasResistancePerCut(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasResistancePerCut();
}

double lefiLayer_resistancePerCut(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->resistancePerCut();
}

int lefiLayer_hasDiagMinEdgeLength(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasDiagMinEdgeLength();
}

double lefiLayer_diagMinEdgeLength(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->diagMinEdgeLength();
}

int lefiLayer_numMinSize(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->numMinSize();
}

double lefiLayer_minSizeWidth(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->minSizeWidth(index);
}

double lefiLayer_minSizeLength(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->minSizeLength(index);
}

int lefiLayer_hasMaxFloatingArea(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasMaxFloatingArea();
}

double lefiLayer_maxFloatingArea(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->maxFloatingArea();
}

int lefiLayer_hasArraySpacing(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasArraySpacing();
}

int lefiLayer_hasLongArray(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasLongArray();
}

int lefiLayer_hasViaWidth(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasViaWidth();
}

double lefiLayer_viaWidth(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->viaWidth();
}

double lefiLayer_cutSpacing(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->cutSpacing();
}

int lefiLayer_numArrayCuts(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->numArrayCuts();
}

int lefiLayer_arrayCuts(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->arrayCuts(index);
}

double lefiLayer_arraySpacing(const ::lefiLayer* obj, int index)
{
  return ((LefParser::lefiLayer*) obj)->arraySpacing(index);
}

int lefiLayer_hasSpacingTableOrtho(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->hasSpacingTableOrtho();
}

const ::lefiOrthogonal* lefiLayer_orthogonal(const ::lefiLayer* obj)
{
  return (const ::lefiOrthogonal*) ((LefParser::lefiLayer*) obj)->orthogonal();
}

int lefiLayer_need58PropsProcessing(const ::lefiLayer* obj)
{
  return ((LefParser::lefiLayer*) obj)->need58PropsProcessing();
}

void lefiLayer_print(const ::lefiLayer* obj, FILE* f)
{
  ((LefParser::lefiLayer*) obj)->print(f);
}
