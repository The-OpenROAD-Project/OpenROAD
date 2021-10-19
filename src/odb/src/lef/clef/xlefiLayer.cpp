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
//  $Date: 2017/06/06 $
//  $State: xxx $
// *****************************************************************************
// *****************************************************************************

#define EXTERN extern "C"

#include "lefiLayer.h"
#include "lefiLayer.hpp"

// Wrappers definitions.
int lefiAntennaPWL_numPWL (const ::lefiAntennaPWL* obj) {
    return ((LefDefParser::lefiAntennaPWL*)obj)->numPWL();
}

char* lefiLayerDensity_type (const ::lefiLayerDensity* obj) {
    return ((LefDefParser::lefiLayerDensity*)obj)->type();
}

int lefiLayerDensity_hasOneEntry (const ::lefiLayerDensity* obj) {
    return ((LefDefParser::lefiLayerDensity*)obj)->hasOneEntry();
}

double lefiLayerDensity_oneEntry (const ::lefiLayerDensity* obj) {
    return ((LefDefParser::lefiLayerDensity*)obj)->oneEntry();
}

int lefiLayerDensity_numFrequency (const ::lefiLayerDensity* obj) {
    return ((LefDefParser::lefiLayerDensity*)obj)->numFrequency();
}

double lefiLayerDensity_frequency (const ::lefiLayerDensity* obj, int  index) {
    return ((LefDefParser::lefiLayerDensity*)obj)->frequency(index);
}

int lefiLayerDensity_numWidths (const ::lefiLayerDensity* obj) {
    return ((LefDefParser::lefiLayerDensity*)obj)->numWidths();
}

double lefiLayerDensity_width (const ::lefiLayerDensity* obj, int  index) {
    return ((LefDefParser::lefiLayerDensity*)obj)->width(index);
}

int lefiLayerDensity_numTableEntries (const ::lefiLayerDensity* obj) {
    return ((LefDefParser::lefiLayerDensity*)obj)->numTableEntries();
}

double lefiLayerDensity_tableEntry (const ::lefiLayerDensity* obj, int  index) {
    return ((LefDefParser::lefiLayerDensity*)obj)->tableEntry(index);
}

int lefiLayerDensity_numCutareas (const ::lefiLayerDensity* obj) {
    return ((LefDefParser::lefiLayerDensity*)obj)->numCutareas();
}

double lefiLayerDensity_cutArea (const ::lefiLayerDensity* obj, int  index) {
    return ((LefDefParser::lefiLayerDensity*)obj)->cutArea(index);
}

int lefiParallel_numLength (const ::lefiParallel* obj) {
    return ((LefDefParser::lefiParallel*)obj)->numLength();
}

int lefiParallel_numWidth (const ::lefiParallel* obj) {
    return ((LefDefParser::lefiParallel*)obj)->numWidth();
}

double lefiParallel_length (const ::lefiParallel* obj, int  iLength) {
    return ((LefDefParser::lefiParallel*)obj)->length(iLength);
}

double lefiParallel_width (const ::lefiParallel* obj, int  iWidth) {
    return ((LefDefParser::lefiParallel*)obj)->width(iWidth);
}

double lefiParallel_widthSpacing (const ::lefiParallel* obj, int  iWidth, int  iWidthSpacing) {
    return ((LefDefParser::lefiParallel*)obj)->widthSpacing(iWidth, iWidthSpacing);
}

int lefiInfluence_numInfluenceEntry (const ::lefiInfluence* obj) {
    return ((LefDefParser::lefiInfluence*)obj)->numInfluenceEntry();
}

double lefiInfluence_width (const ::lefiInfluence* obj, int  index) {
    return ((LefDefParser::lefiInfluence*)obj)->width(index);
}

double lefiInfluence_distance (const ::lefiInfluence* obj, int  index) {
    return ((LefDefParser::lefiInfluence*)obj)->distance(index);
}

double lefiInfluence_spacing (const ::lefiInfluence* obj, int  index) {
    return ((LefDefParser::lefiInfluence*)obj)->spacing(index);
}

int lefiTwoWidths_numWidth (const ::lefiTwoWidths* obj) {
    return ((LefDefParser::lefiTwoWidths*)obj)->numWidth();
}

double lefiTwoWidths_width (const ::lefiTwoWidths* obj, int  iWidth) {
    return ((LefDefParser::lefiTwoWidths*)obj)->width(iWidth);
}

int lefiTwoWidths_hasWidthPRL (const ::lefiTwoWidths* obj, int  iWidth) {
    return ((LefDefParser::lefiTwoWidths*)obj)->hasWidthPRL(iWidth);
}

double lefiTwoWidths_widthPRL (const ::lefiTwoWidths* obj, int  iWidth) {
    return ((LefDefParser::lefiTwoWidths*)obj)->widthPRL(iWidth);
}

int lefiTwoWidths_numWidthSpacing (const ::lefiTwoWidths* obj, int  iWidth) {
    return ((LefDefParser::lefiTwoWidths*)obj)->numWidthSpacing(iWidth);
}

double lefiTwoWidths_widthSpacing (const ::lefiTwoWidths* obj, int  iWidth, int  iWidthSpacing) {
    return ((LefDefParser::lefiTwoWidths*)obj)->widthSpacing(iWidth, iWidthSpacing);
}

int lefiSpacingTable_isInfluence (const ::lefiSpacingTable* obj) {
    return ((LefDefParser::lefiSpacingTable*)obj)->isInfluence();
}

const ::lefiInfluence* lefiSpacingTable_influence (const ::lefiSpacingTable* obj) {
    return (const ::lefiInfluence*) ((LefDefParser::lefiSpacingTable*)obj)->influence();
}

int lefiSpacingTable_isParallel (const ::lefiSpacingTable* obj) {
    return ((LefDefParser::lefiSpacingTable*)obj)->isParallel();
}

const ::lefiParallel* lefiSpacingTable_parallel (const ::lefiSpacingTable* obj) {
    return (const ::lefiParallel*) ((LefDefParser::lefiSpacingTable*)obj)->parallel();
}

const ::lefiTwoWidths* lefiSpacingTable_twoWidths (const ::lefiSpacingTable* obj) {
    return (const ::lefiTwoWidths*) ((LefDefParser::lefiSpacingTable*)obj)->twoWidths();
}

int lefiOrthogonal_numOrthogonal (const ::lefiOrthogonal* obj) {
    return ((LefDefParser::lefiOrthogonal*)obj)->numOrthogonal();
}

double lefiOrthogonal_cutWithin (const ::lefiOrthogonal* obj, int  index) {
    return ((LefDefParser::lefiOrthogonal*)obj)->cutWithin(index);
}

double lefiOrthogonal_orthoSpacing (const ::lefiOrthogonal* obj, int  index) {
    return ((LefDefParser::lefiOrthogonal*)obj)->orthoSpacing(index);
}

int lefiAntennaModel_hasAntennaAreaRatio (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaAreaRatio();
}

int lefiAntennaModel_hasAntennaDiffAreaRatio (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaDiffAreaRatio();
}

int lefiAntennaModel_hasAntennaDiffAreaRatioPWL (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaDiffAreaRatioPWL();
}

int lefiAntennaModel_hasAntennaCumAreaRatio (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaCumAreaRatio();
}

int lefiAntennaModel_hasAntennaCumDiffAreaRatio (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaCumDiffAreaRatio();
}

int lefiAntennaModel_hasAntennaCumDiffAreaRatioPWL (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaCumDiffAreaRatioPWL();
}

int lefiAntennaModel_hasAntennaAreaFactor (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaAreaFactor();
}

int lefiAntennaModel_hasAntennaAreaFactorDUO (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaAreaFactorDUO();
}

int lefiAntennaModel_hasAntennaSideAreaRatio (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaSideAreaRatio();
}

int lefiAntennaModel_hasAntennaDiffSideAreaRatio (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaDiffSideAreaRatio();
}

int lefiAntennaModel_hasAntennaDiffSideAreaRatioPWL (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaDiffSideAreaRatioPWL();
}

int lefiAntennaModel_hasAntennaCumSideAreaRatio (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaCumSideAreaRatio();
}

int lefiAntennaModel_hasAntennaCumDiffSideAreaRatio (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaCumDiffSideAreaRatio();
}

int lefiAntennaModel_hasAntennaCumDiffSideAreaRatioPWL (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaCumDiffSideAreaRatioPWL();
}

int lefiAntennaModel_hasAntennaSideAreaFactor (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaSideAreaFactor();
}

int lefiAntennaModel_hasAntennaSideAreaFactorDUO (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaSideAreaFactorDUO();
}

int lefiAntennaModel_hasAntennaCumRoutingPlusCut (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaCumRoutingPlusCut();
}

int lefiAntennaModel_hasAntennaGatePlusDiff (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaGatePlusDiff();
}

int lefiAntennaModel_hasAntennaAreaMinusDiff (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaAreaMinusDiff();
}

int lefiAntennaModel_hasAntennaAreaDiffReducePWL (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->hasAntennaAreaDiffReducePWL();
}

char* lefiAntennaModel_antennaOxide (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->antennaOxide();
}

double lefiAntennaModel_antennaAreaRatio (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->antennaAreaRatio();
}

double lefiAntennaModel_antennaDiffAreaRatio (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->antennaDiffAreaRatio();
}

const ::lefiAntennaPWL* lefiAntennaModel_antennaDiffAreaRatioPWL (const ::lefiAntennaModel* obj) {
    return (const ::lefiAntennaPWL*) ((LefDefParser::lefiAntennaModel*)obj)->antennaDiffAreaRatioPWL();
}

double lefiAntennaModel_antennaCumAreaRatio (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->antennaCumAreaRatio();
}

double lefiAntennaModel_antennaCumDiffAreaRatio (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->antennaCumDiffAreaRatio();
}

const ::lefiAntennaPWL* lefiAntennaModel_antennaCumDiffAreaRatioPWL (const ::lefiAntennaModel* obj) {
    return (const ::lefiAntennaPWL*) ((LefDefParser::lefiAntennaModel*)obj)->antennaCumDiffAreaRatioPWL();
}

double lefiAntennaModel_antennaAreaFactor (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->antennaAreaFactor();
}

double lefiAntennaModel_antennaSideAreaRatio (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->antennaSideAreaRatio();
}

double lefiAntennaModel_antennaDiffSideAreaRatio (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->antennaDiffSideAreaRatio();
}

const ::lefiAntennaPWL* lefiAntennaModel_antennaDiffSideAreaRatioPWL (const ::lefiAntennaModel* obj) {
    return (const ::lefiAntennaPWL*) ((LefDefParser::lefiAntennaModel*)obj)->antennaDiffSideAreaRatioPWL();
}

double lefiAntennaModel_antennaCumSideAreaRatio (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->antennaCumSideAreaRatio();
}

double lefiAntennaModel_antennaCumDiffSideAreaRatio (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->antennaCumDiffSideAreaRatio();
}

const ::lefiAntennaPWL* lefiAntennaModel_antennaCumDiffSideAreaRatioPWL (const ::lefiAntennaModel* obj) {
    return (const ::lefiAntennaPWL*) ((LefDefParser::lefiAntennaModel*)obj)->antennaCumDiffSideAreaRatioPWL();
}

double lefiAntennaModel_antennaSideAreaFactor (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->antennaSideAreaFactor();
}

double lefiAntennaModel_antennaGatePlusDiff (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->antennaGatePlusDiff();
}

double lefiAntennaModel_antennaAreaMinusDiff (const ::lefiAntennaModel* obj) {
    return ((LefDefParser::lefiAntennaModel*)obj)->antennaAreaMinusDiff();
}

const ::lefiAntennaPWL* lefiAntennaModel_antennaAreaDiffReducePWL (const ::lefiAntennaModel* obj) {
    return (const ::lefiAntennaPWL*) ((LefDefParser::lefiAntennaModel*)obj)->antennaAreaDiffReducePWL();
}

int lefiLayer_hasType (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasType();
}

int lefiLayer_hasLayerType (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasLayerType();
}

int lefiLayer_hasMask (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasMask();
}

int lefiLayer_hasPitch (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasPitch();
}

int lefiLayer_hasXYPitch (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasXYPitch();
}

int lefiLayer_hasOffset (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasOffset();
}

int lefiLayer_hasXYOffset (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasXYOffset();
}

int lefiLayer_hasWidth (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasWidth();
}

int lefiLayer_hasArea (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasArea();
}

int lefiLayer_hasDiagPitch (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasDiagPitch();
}

int lefiLayer_hasXYDiagPitch (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasXYDiagPitch();
}

int lefiLayer_hasDiagWidth (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasDiagWidth();
}

int lefiLayer_hasDiagSpacing (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasDiagSpacing();
}

int lefiLayer_hasSpacingNumber (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingNumber();
}

int lefiLayer_hasSpacingName (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingName(index);
}

int lefiLayer_hasSpacingLayerStack (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingLayerStack(index);
}

int lefiLayer_hasSpacingAdjacent (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingAdjacent(index);
}

int lefiLayer_hasSpacingCenterToCenter (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingCenterToCenter(index);
}

int lefiLayer_hasSpacingRange (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingRange(index);
}

int lefiLayer_hasSpacingRangeUseLengthThreshold (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingRangeUseLengthThreshold(index);
}

int lefiLayer_hasSpacingRangeInfluence (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingRangeInfluence(index);
}

int lefiLayer_hasSpacingRangeInfluenceRange (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingRangeInfluenceRange(index);
}

int lefiLayer_hasSpacingRangeRange (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingRangeRange(index);
}

int lefiLayer_hasSpacingLengthThreshold (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingLengthThreshold(index);
}

int lefiLayer_hasSpacingLengthThresholdRange (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingLengthThresholdRange(index);
}

int lefiLayer_hasSpacingParallelOverlap (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingParallelOverlap(index);
}

int lefiLayer_hasSpacingArea (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingArea(index);
}

int lefiLayer_hasSpacingEndOfLine (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingEndOfLine(index);
}

int lefiLayer_hasSpacingParellelEdge (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingParellelEdge(index);
}

int lefiLayer_hasSpacingTwoEdges (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingTwoEdges(index);
}

int lefiLayer_hasSpacingAdjacentExcept (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingAdjacentExcept(index);
}

int lefiLayer_hasSpacingSamenet (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingSamenet(index);
}

int lefiLayer_hasSpacingSamenetPGonly (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingSamenetPGonly(index);
}

int lefiLayer_hasSpacingNotchLength (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingNotchLength(index);
}

int lefiLayer_hasSpacingEndOfNotchWidth (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingEndOfNotchWidth(index);
}

int lefiLayer_hasDirection (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasDirection();
}

int lefiLayer_hasResistance (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasResistance();
}

int lefiLayer_hasResistanceArray (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasResistanceArray();
}

int lefiLayer_hasCapacitance (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasCapacitance();
}

int lefiLayer_hasCapacitanceArray (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasCapacitanceArray();
}

int lefiLayer_hasHeight (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasHeight();
}

int lefiLayer_hasThickness (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasThickness();
}

int lefiLayer_hasWireExtension (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasWireExtension();
}

int lefiLayer_hasShrinkage (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasShrinkage();
}

int lefiLayer_hasCapMultiplier (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasCapMultiplier();
}

int lefiLayer_hasEdgeCap (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasEdgeCap();
}

int lefiLayer_hasAntennaLength (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasAntennaLength();
}

int lefiLayer_hasAntennaArea (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasAntennaArea();
}

int lefiLayer_hasCurrentDensityPoint (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasCurrentDensityPoint();
}

int lefiLayer_hasCurrentDensityArray (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasCurrentDensityArray();
}

int lefiLayer_hasAccurrentDensity (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasAccurrentDensity();
}

int lefiLayer_hasDccurrentDensity (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasDccurrentDensity();
}

int lefiLayer_numProps (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->numProps();
}

const char* lefiLayer_propName (const ::lefiLayer* obj, int  index) {
    return ((const LefDefParser::lefiLayer*)obj)->propName(index);
}

const char* lefiLayer_propValue (const ::lefiLayer* obj, int  index) {
    return ((const LefDefParser::lefiLayer*)obj)->propValue(index);
}

double lefiLayer_propNumber (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->propNumber(index);
}

const char lefiLayer_propType (const ::lefiLayer* obj, int  index) {
    return ((const LefDefParser::lefiLayer*)obj)->propType(index);
}

int lefiLayer_propIsNumber (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->propIsNumber(index);
}

int lefiLayer_propIsString (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->propIsString(index);
}

int lefiLayer_numSpacing (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->numSpacing();
}

char* lefiLayer_name (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->name();
}

const char* lefiLayer_type (const ::lefiLayer* obj) {
    return ((const LefDefParser::lefiLayer*)obj)->type();
}

const char* lefiLayer_layerType (const ::lefiLayer* obj) {
    return ((const LefDefParser::lefiLayer*)obj)->layerType();
}

double lefiLayer_pitch (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->pitch();
}

int lefiLayer_mask (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->mask();
}

double lefiLayer_pitchX (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->pitchX();
}

double lefiLayer_pitchY (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->pitchY();
}

double lefiLayer_offset (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->offset();
}

double lefiLayer_offsetX (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->offsetX();
}

double lefiLayer_offsetY (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->offsetY();
}

double lefiLayer_width (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->width();
}

double lefiLayer_area (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->area();
}

double lefiLayer_diagPitch (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->diagPitch();
}

double lefiLayer_diagPitchX (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->diagPitchX();
}

double lefiLayer_diagPitchY (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->diagPitchY();
}

double lefiLayer_diagWidth (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->diagWidth();
}

double lefiLayer_diagSpacing (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->diagSpacing();
}

double lefiLayer_spacing (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacing(index);
}

char* lefiLayer_spacingName (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingName(index);
}

int lefiLayer_spacingAdjacentCuts (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingAdjacentCuts(index);
}

double lefiLayer_spacingAdjacentWithin (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingAdjacentWithin(index);
}

double lefiLayer_spacingArea (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingArea(index);
}

double lefiLayer_spacingRangeMin (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingRangeMin(index);
}

double lefiLayer_spacingRangeMax (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingRangeMax(index);
}

double lefiLayer_spacingRangeInfluence (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingRangeInfluence(index);
}

double lefiLayer_spacingRangeInfluenceMin (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingRangeInfluenceMin(index);
}

double lefiLayer_spacingRangeInfluenceMax (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingRangeInfluenceMax(index);
}

double lefiLayer_spacingRangeRangeMin (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingRangeRangeMin(index);
}

double lefiLayer_spacingRangeRangeMax (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingRangeRangeMax(index);
}

double lefiLayer_spacingLengthThreshold (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingLengthThreshold(index);
}

double lefiLayer_spacingLengthThresholdRangeMin (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingLengthThresholdRangeMin(index);
}

double lefiLayer_spacingLengthThresholdRangeMax (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingLengthThresholdRangeMax(index);
}

double lefiLayer_spacingEolWidth (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingEolWidth(index);
}

double lefiLayer_spacingEolWithin (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingEolWithin(index);
}

double lefiLayer_spacingParSpace (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingParSpace(index);
}

double lefiLayer_spacingParWithin (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingParWithin(index);
}

double lefiLayer_spacingNotchLength (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingNotchLength(index);
}

double lefiLayer_spacingEndOfNotchWidth (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingEndOfNotchWidth(index);
}

double lefiLayer_spacingEndOfNotchSpacing (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingEndOfNotchSpacing(index);
}

double lefiLayer_spacingEndOfNotchLength (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->spacingEndOfNotchLength(index);
}

int lefiLayer_numMinimumcut (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->numMinimumcut();
}

int lefiLayer_minimumcut (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->minimumcut(index);
}

double lefiLayer_minimumcutWidth (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->minimumcutWidth(index);
}

int lefiLayer_hasMinimumcutWithin (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasMinimumcutWithin(index);
}

double lefiLayer_minimumcutWithin (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->minimumcutWithin(index);
}

int lefiLayer_hasMinimumcutConnection (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasMinimumcutConnection(index);
}

const char* lefiLayer_minimumcutConnection (const ::lefiLayer* obj, int  index) {
    return ((const LefDefParser::lefiLayer*)obj)->minimumcutConnection(index);
}

int lefiLayer_hasMinimumcutNumCuts (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasMinimumcutNumCuts(index);
}

double lefiLayer_minimumcutLength (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->minimumcutLength(index);
}

double lefiLayer_minimumcutDistance (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->minimumcutDistance(index);
}

const char* lefiLayer_direction (const ::lefiLayer* obj) {
    return ((const LefDefParser::lefiLayer*)obj)->direction();
}

double lefiLayer_resistance (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->resistance();
}

double lefiLayer_capacitance (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->capacitance();
}

double lefiLayer_height (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->height();
}

double lefiLayer_wireExtension (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->wireExtension();
}

double lefiLayer_thickness (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->thickness();
}

double lefiLayer_shrinkage (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->shrinkage();
}

double lefiLayer_capMultiplier (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->capMultiplier();
}

double lefiLayer_edgeCap (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->edgeCap();
}

double lefiLayer_antennaLength (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->antennaLength();
}

double lefiLayer_antennaArea (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->antennaArea();
}

double lefiLayer_currentDensityPoint (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->currentDensityPoint();
}

void lefiLayer_currentDensityArray (const ::lefiLayer* obj, int*  numPoints, double**  widths, double**  current) {
    ((LefDefParser::lefiLayer*)obj)->currentDensityArray(numPoints, widths, current);
}

void lefiLayer_capacitanceArray (const ::lefiLayer* obj, int*  numPoints, double**  widths, double**  resValues) {
    ((LefDefParser::lefiLayer*)obj)->capacitanceArray(numPoints, widths, resValues);
}

void lefiLayer_resistanceArray (const ::lefiLayer* obj, int*  numPoints, double**  widths, double**  capValues) {
    ((LefDefParser::lefiLayer*)obj)->resistanceArray(numPoints, widths, capValues);
}

int lefiLayer_numAccurrentDensity (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->numAccurrentDensity();
}

const ::lefiLayerDensity* lefiLayer_accurrent (const ::lefiLayer* obj, int  index) {
    return (const ::lefiLayerDensity*) ((LefDefParser::lefiLayer*)obj)->accurrent(index);
}

int lefiLayer_numDccurrentDensity (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->numDccurrentDensity();
}

const ::lefiLayerDensity* lefiLayer_dccurrent (const ::lefiLayer* obj, int  index) {
    return (const ::lefiLayerDensity*) ((LefDefParser::lefiLayer*)obj)->dccurrent(index);
}

int lefiLayer_numAntennaModel (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->numAntennaModel();
}

const ::lefiAntennaModel* lefiLayer_antennaModel (const ::lefiLayer* obj, int  index) {
    return (const ::lefiAntennaModel*) ((LefDefParser::lefiLayer*)obj)->antennaModel(index);
}

int lefiLayer_hasSlotWireWidth (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasSlotWireWidth();
}

int lefiLayer_hasSlotWireLength (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasSlotWireLength();
}

int lefiLayer_hasSlotWidth (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasSlotWidth();
}

int lefiLayer_hasSlotLength (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasSlotLength();
}

int lefiLayer_hasMaxAdjacentSlotSpacing (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasMaxAdjacentSlotSpacing();
}

int lefiLayer_hasMaxCoaxialSlotSpacing (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasMaxCoaxialSlotSpacing();
}

int lefiLayer_hasMaxEdgeSlotSpacing (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasMaxEdgeSlotSpacing();
}

int lefiLayer_hasSplitWireWidth (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasSplitWireWidth();
}

int lefiLayer_hasMinimumDensity (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasMinimumDensity();
}

int lefiLayer_hasMaximumDensity (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasMaximumDensity();
}

int lefiLayer_hasDensityCheckWindow (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasDensityCheckWindow();
}

int lefiLayer_hasDensityCheckStep (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasDensityCheckStep();
}

int lefiLayer_hasFillActiveSpacing (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasFillActiveSpacing();
}

int lefiLayer_hasMaxwidth (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasMaxwidth();
}

int lefiLayer_hasMinwidth (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasMinwidth();
}

int lefiLayer_hasMinstep (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasMinstep();
}

int lefiLayer_hasProtrusion (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasProtrusion();
}

double lefiLayer_slotWireWidth (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->slotWireWidth();
}

double lefiLayer_slotWireLength (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->slotWireLength();
}

double lefiLayer_slotWidth (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->slotWidth();
}

double lefiLayer_slotLength (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->slotLength();
}

double lefiLayer_maxAdjacentSlotSpacing (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->maxAdjacentSlotSpacing();
}

double lefiLayer_maxCoaxialSlotSpacing (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->maxCoaxialSlotSpacing();
}

double lefiLayer_maxEdgeSlotSpacing (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->maxEdgeSlotSpacing();
}

double lefiLayer_splitWireWidth (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->splitWireWidth();
}

double lefiLayer_minimumDensity (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->minimumDensity();
}

double lefiLayer_maximumDensity (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->maximumDensity();
}

double lefiLayer_densityCheckWindowLength (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->densityCheckWindowLength();
}

double lefiLayer_densityCheckWindowWidth (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->densityCheckWindowWidth();
}

double lefiLayer_densityCheckStep (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->densityCheckStep();
}

double lefiLayer_fillActiveSpacing (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->fillActiveSpacing();
}

double lefiLayer_maxwidth (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->maxwidth();
}

double lefiLayer_minwidth (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->minwidth();
}

double lefiLayer_protrusionWidth1 (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->protrusionWidth1();
}

double lefiLayer_protrusionLength (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->protrusionLength();
}

double lefiLayer_protrusionWidth2 (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->protrusionWidth2();
}

int lefiLayer_numMinstep (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->numMinstep();
}

double lefiLayer_minstep (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->minstep(index);
}

int lefiLayer_hasMinstepType (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasMinstepType(index);
}

char* lefiLayer_minstepType (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->minstepType(index);
}

int lefiLayer_hasMinstepLengthsum (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasMinstepLengthsum(index);
}

double lefiLayer_minstepLengthsum (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->minstepLengthsum(index);
}

int lefiLayer_hasMinstepMaxedges (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasMinstepMaxedges(index);
}

int lefiLayer_minstepMaxedges (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->minstepMaxedges(index);
}

int lefiLayer_hasMinstepMinAdjLength (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasMinstepMinAdjLength(index);
}

double lefiLayer_minstepMinAdjLength (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->minstepMinAdjLength(index);
}

int lefiLayer_hasMinstepMinBetLength (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasMinstepMinBetLength(index);
}

double lefiLayer_minstepMinBetLength (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->minstepMinBetLength(index);
}

int lefiLayer_hasMinstepXSameCorners (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasMinstepXSameCorners(index);
}

int lefiLayer_numMinenclosedarea (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->numMinenclosedarea();
}

double lefiLayer_minenclosedarea (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->minenclosedarea(index);
}

int lefiLayer_hasMinenclosedareaWidth (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasMinenclosedareaWidth(index);
}

double lefiLayer_minenclosedareaWidth (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->minenclosedareaWidth(index);
}

int lefiLayer_numEnclosure (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->numEnclosure();
}

int lefiLayer_hasEnclosureRule (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasEnclosureRule(index);
}

double lefiLayer_enclosureOverhang1 (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->enclosureOverhang1(index);
}

double lefiLayer_enclosureOverhang2 (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->enclosureOverhang2(index);
}

int lefiLayer_hasEnclosureWidth (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasEnclosureWidth(index);
}

double lefiLayer_enclosureMinWidth (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->enclosureMinWidth(index);
}

int lefiLayer_hasEnclosureExceptExtraCut (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasEnclosureExceptExtraCut(index);
}

double lefiLayer_enclosureExceptExtraCut (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->enclosureExceptExtraCut(index);
}

int lefiLayer_hasEnclosureMinLength (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasEnclosureMinLength(index);
}

double lefiLayer_enclosureMinLength (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->enclosureMinLength(index);
}

int lefiLayer_numPreferEnclosure (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->numPreferEnclosure();
}

int lefiLayer_hasPreferEnclosureRule (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasPreferEnclosureRule(index);
}

double lefiLayer_preferEnclosureOverhang1 (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->preferEnclosureOverhang1(index);
}

double lefiLayer_preferEnclosureOverhang2 (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->preferEnclosureOverhang2(index);
}

int lefiLayer_hasPreferEnclosureWidth (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->hasPreferEnclosureWidth(index);
}

double lefiLayer_preferEnclosureMinWidth (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->preferEnclosureMinWidth(index);
}

int lefiLayer_hasResistancePerCut (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasResistancePerCut();
}

double lefiLayer_resistancePerCut (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->resistancePerCut();
}

int lefiLayer_hasDiagMinEdgeLength (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasDiagMinEdgeLength();
}

double lefiLayer_diagMinEdgeLength (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->diagMinEdgeLength();
}

int lefiLayer_numMinSize (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->numMinSize();
}

double lefiLayer_minSizeWidth (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->minSizeWidth(index);
}

double lefiLayer_minSizeLength (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->minSizeLength(index);
}

int lefiLayer_hasMaxFloatingArea (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasMaxFloatingArea();
}

double lefiLayer_maxFloatingArea (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->maxFloatingArea();
}

int lefiLayer_hasArraySpacing (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasArraySpacing();
}

int lefiLayer_hasLongArray (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasLongArray();
}

int lefiLayer_hasViaWidth (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasViaWidth();
}

double lefiLayer_viaWidth (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->viaWidth();
}

double lefiLayer_cutSpacing (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->cutSpacing();
}

int lefiLayer_numArrayCuts (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->numArrayCuts();
}

int lefiLayer_arrayCuts (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->arrayCuts(index);
}

double lefiLayer_arraySpacing (const ::lefiLayer* obj, int  index) {
    return ((LefDefParser::lefiLayer*)obj)->arraySpacing(index);
}

int lefiLayer_hasSpacingTableOrtho (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->hasSpacingTableOrtho();
}

const ::lefiOrthogonal* lefiLayer_orthogonal (const ::lefiLayer* obj) {
    return (const ::lefiOrthogonal*) ((LefDefParser::lefiLayer*)obj)->orthogonal();
}

int lefiLayer_need58PropsProcessing (const ::lefiLayer* obj) {
    return ((LefDefParser::lefiLayer*)obj)->need58PropsProcessing();
}

void lefiLayer_print (const ::lefiLayer* obj, FILE*  f) {
    ((LefDefParser::lefiLayer*)obj)->print(f);
}

