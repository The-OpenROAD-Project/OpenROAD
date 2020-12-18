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
//  $State:  $  
// *****************************************************************************
// *****************************************************************************

#define EXTERN extern "C"

#include "lefwWriter.h"
#include "lefwWriter.hpp"

// Wrappers definitions.
int lefwInit (FILE*  f) {
    return LefDefParser::lefwInit(f);
}

int lefwInitCbk (FILE*  f) {
    return LefDefParser::lefwInitCbk(f);
}

int lefwEncrypt () {
    return LefDefParser::lefwEncrypt();
}

int lefwCloseEncrypt () {
    return LefDefParser::lefwCloseEncrypt();
}

int lefwNewLine () {
    return LefDefParser::lefwNewLine();
}

int lefwVersion (int  vers1, int  vers2) {
    return LefDefParser::lefwVersion(vers1, vers2);
}

int lefwCaseSensitive (const char*  caseSensitive) {
    return LefDefParser::lefwCaseSensitive(caseSensitive);
}

int lefwNoWireExtensionAtPin (const char*  noWireExt) {
    return LefDefParser::lefwNoWireExtensionAtPin(noWireExt);
}

int lefwMinfeature (double  minFeatureX, double  minFeatureY) {
    return LefDefParser::lefwMinfeature(minFeatureX, minFeatureY);
}

int lefwDielectric (double  dielectric) {
    return LefDefParser::lefwDielectric(dielectric);
}

int lefwBusBitChars (const char*  busBitChars) {
    return LefDefParser::lefwBusBitChars(busBitChars);
}

int lefwDividerChar (const char*  dividerChar) {
    return LefDefParser::lefwDividerChar(dividerChar);
}

int lefwManufacturingGrid (double  grid) {
    return LefDefParser::lefwManufacturingGrid(grid);
}

int lefwFixedMask () {
    return LefDefParser::lefwFixedMask();
}

int lefwUseMinSpacing (const char*  type, const char*  onOff) {
    return LefDefParser::lefwUseMinSpacing(type, onOff);
}

int lefwClearanceMeasure (const char*  type) {
    return LefDefParser::lefwClearanceMeasure(type);
}

int lefwAntennaInputGateArea (double  inputGateArea) {
    return LefDefParser::lefwAntennaInputGateArea(inputGateArea);
}

int lefwAntennaInOutDiffArea (double  inOutDiffArea) {
    return LefDefParser::lefwAntennaInOutDiffArea(inOutDiffArea);
}

int lefwAntennaOutputDiffArea (double  outputDiffArea) {
    return LefDefParser::lefwAntennaOutputDiffArea(outputDiffArea);
}

int lefwStartUnits () {
    return LefDefParser::lefwStartUnits();
}

int lefwUnits (double  time, double  capacitance, double  resistance, double  power, double  current, double  voltage, double  database) {
    return LefDefParser::lefwUnits(time, capacitance, resistance, power, current, voltage, database);
}

int lefwUnitsFrequency (double  frequency) {
    return LefDefParser::lefwUnitsFrequency(frequency);
}

int lefwEndUnits () {
    return LefDefParser::lefwEndUnits();
}

int lefwStartLayer (const char*  layerName, const char*  type) {
    return LefDefParser::lefwStartLayer(layerName, type);
}

int lefwLayerMask (int  maskColor) {
    return LefDefParser::lefwLayerMask(maskColor);
}

int lefwLayerWidth (double  minWidth) {
    return LefDefParser::lefwLayerWidth(minWidth);
}

int lefwLayerCutSpacing (double  spacing) {
    return LefDefParser::lefwLayerCutSpacing(spacing);
}

int lefwLayerCutSpacingCenterToCenter () {
    return LefDefParser::lefwLayerCutSpacingCenterToCenter();
}

int lefwLayerCutSpacingSameNet () {
    return LefDefParser::lefwLayerCutSpacingSameNet();
}

int lefwLayerCutSpacingLayer (const char*  name2, int  stack) {
    return LefDefParser::lefwLayerCutSpacingLayer(name2, stack);
}

int lefwLayerCutSpacingAdjacent (int  viaCuts, double  distance, int  stack) {
    return LefDefParser::lefwLayerCutSpacingAdjacent(viaCuts, distance, stack);
}

int lefwLayerCutSpacingParallel () {
    return LefDefParser::lefwLayerCutSpacingParallel();
}

int lefwLayerCutSpacingArea (double  cutArea) {
    return LefDefParser::lefwLayerCutSpacingArea(cutArea);
}

int lefwLayerCutSpacingEnd () {
    return LefDefParser::lefwLayerCutSpacingEnd();
}

int lefwLayerCutSpacingTableOrtho (int  numSpacing, double*  cutWithins, double*  orthoSpacings) {
    return LefDefParser::lefwLayerCutSpacingTableOrtho(numSpacing, cutWithins, orthoSpacings);
}

int lefwLayerArraySpacing (int  longArray, double  viaWidth, double  cutSpacing, int  numArrayCut, int*  arrayCuts, double*  arraySpacings) {
    return LefDefParser::lefwLayerArraySpacing(longArray, viaWidth, cutSpacing, numArrayCut, arrayCuts, arraySpacings);
}

int lefwLayerEnclosure (const char*  location, double  overhang1, double  overhang2, double  width) {
    return LefDefParser::lefwLayerEnclosure(location, overhang1, overhang2, width);
}

int lefwLayerEnclosureWidth (const char*  location, double  overhang1, double  overhang2, double  width, double  cutWithin) {
    return LefDefParser::lefwLayerEnclosureWidth(location, overhang1, overhang2, width, cutWithin);
}

int lefwLayerEnclosureLength (const char*  location, double  overhang1, double  overhang2, double  minLength) {
    return LefDefParser::lefwLayerEnclosureLength(location, overhang1, overhang2, minLength);
}

int lefwLayerPreferEnclosure (const char*  location, double  overhang1, double  overhang2, double  width) {
    return LefDefParser::lefwLayerPreferEnclosure(location, overhang1, overhang2, width);
}

int lefwLayerResistancePerCut (double  resistance) {
    return LefDefParser::lefwLayerResistancePerCut(resistance);
}

int lefwEndLayer (const char*  layerName) {
    return LefDefParser::lefwEndLayer(layerName);
}

int lefwStartLayerRouting (const char*  layerName) {
    return LefDefParser::lefwStartLayerRouting(layerName);
}

int lefwLayerRouting (const char*  direction, double  width) {
    return LefDefParser::lefwLayerRouting(direction, width);
}

int lefwLayerRoutingPitch (double  pitch) {
    return LefDefParser::lefwLayerRoutingPitch(pitch);
}

int lefwLayerRoutingPitchXYDistance (double  xDistance, double  yDistance) {
    return LefDefParser::lefwLayerRoutingPitchXYDistance(xDistance, yDistance);
}

int lefwLayerRoutingDiagPitch (double  distance) {
    return LefDefParser::lefwLayerRoutingDiagPitch(distance);
}

int lefwLayerRoutingDiagPitchXYDistance (double  diag45Distance, double  diag135Distance) {
    return LefDefParser::lefwLayerRoutingDiagPitchXYDistance(diag45Distance, diag135Distance);
}

int lefwLayerRoutingDiagWidth (double  diagWidth) {
    return LefDefParser::lefwLayerRoutingDiagWidth(diagWidth);
}

int lefwLayerRoutingDiagSpacing (double  diagSpacing) {
    return LefDefParser::lefwLayerRoutingDiagSpacing(diagSpacing);
}

int lefwLayerRoutingDiagMinEdgeLength (double  diagLength) {
    return LefDefParser::lefwLayerRoutingDiagMinEdgeLength(diagLength);
}

int lefwLayerRoutingOffset (double  offset) {
    return LefDefParser::lefwLayerRoutingOffset(offset);
}

int lefwLayerRoutingOffsetXYDistance (double  xDistance, double  yDistance) {
    return LefDefParser::lefwLayerRoutingOffsetXYDistance(xDistance, yDistance);
}

int lefwLayerRoutingArea (double  area) {
    return LefDefParser::lefwLayerRoutingArea(area);
}

int lefwLayerRoutingMinsize (int  numRect, double*  minWidth, double*  minLength) {
    return LefDefParser::lefwLayerRoutingMinsize(numRect, minWidth, minLength);
}

int lefwLayerRoutingMinimumcut (double  numCuts, double  minWidth) {
    return LefDefParser::lefwLayerRoutingMinimumcut(numCuts, minWidth);
}

int lefwLayerRoutingMinimumcutWithin (double  numCuts, double  minWidth, double  cutDistance) {
    return LefDefParser::lefwLayerRoutingMinimumcutWithin(numCuts, minWidth, cutDistance);
}

int lefwLayerRoutingMinimumcutConnections (const char*  direction) {
    return LefDefParser::lefwLayerRoutingMinimumcutConnections(direction);
}

int lefwLayerRoutingMinimumcutLengthWithin (double  length, double  distance) {
    return LefDefParser::lefwLayerRoutingMinimumcutLengthWithin(length, distance);
}

int lefwLayerRoutingSpacing (double  spacing) {
    return LefDefParser::lefwLayerRoutingSpacing(spacing);
}

int lefwLayerRoutingSpacingRange (double  minWidth, double  maxWidth) {
    return LefDefParser::lefwLayerRoutingSpacingRange(minWidth, maxWidth);
}

int lefwLayerRoutingSpacingRangeUseLengthThreshold () {
    return LefDefParser::lefwLayerRoutingSpacingRangeUseLengthThreshold();
}

int lefwLayerRoutingSpacingRangeInfluence (double  infValue, double  subMinWidth, double  subMaxWidth) {
    return LefDefParser::lefwLayerRoutingSpacingRangeInfluence(infValue, subMinWidth, subMaxWidth);
}

int lefwLayerRoutingSpacingRangeRange (double  minWidth, double  maxWidth) {
    return LefDefParser::lefwLayerRoutingSpacingRangeRange(minWidth, maxWidth);
}

int lefwLayerRoutingSpacingLengthThreshold (double  lengthValue, double  minWidth, double  maxWidth) {
    return LefDefParser::lefwLayerRoutingSpacingLengthThreshold(lengthValue, minWidth, maxWidth);
}

int lefwLayerRoutingSpacingSameNet (int  PGOnly) {
    return LefDefParser::lefwLayerRoutingSpacingSameNet(PGOnly);
}

int lefwLayerRoutingSpacingEndOfLine (double  eolWidth, double  eolWithin) {
    return LefDefParser::lefwLayerRoutingSpacingEndOfLine(eolWidth, eolWithin);
}

int lefwLayerRoutingSpacingEOLParallel (double  parSpace, double  parWithin, int  twoEdges) {
    return LefDefParser::lefwLayerRoutingSpacingEOLParallel(parSpace, parWithin, twoEdges);
}

int lefwLayerRoutingSpacingNotchLength (double  minNLength) {
    return LefDefParser::lefwLayerRoutingSpacingNotchLength(minNLength);
}

int lefwLayerRoutingSpacingEndOfNotchWidth (double  eonWidth, double  minNSpacing, double  minNLength) {
    return LefDefParser::lefwLayerRoutingSpacingEndOfNotchWidth(eonWidth, minNSpacing, minNLength);
}

int lefwLayerRoutingWireExtension (double  wireExtension) {
    return LefDefParser::lefwLayerRoutingWireExtension(wireExtension);
}

int lefwLayerRoutingResistance (const char*  resistance) {
    return LefDefParser::lefwLayerRoutingResistance(resistance);
}

int lefwLayerRoutingCapacitance (const char*  capacitance) {
    return LefDefParser::lefwLayerRoutingCapacitance(capacitance);
}

int lefwLayerRoutingHeight (double  height) {
    return LefDefParser::lefwLayerRoutingHeight(height);
}

int lefwLayerRoutingThickness (double  thickness) {
    return LefDefParser::lefwLayerRoutingThickness(thickness);
}

int lefwLayerRoutingShrinkage (double  shrinkage) {
    return LefDefParser::lefwLayerRoutingShrinkage(shrinkage);
}

int lefwLayerRoutingCapMultiplier (double  capMultiplier) {
    return LefDefParser::lefwLayerRoutingCapMultiplier(capMultiplier);
}

int lefwLayerRoutingEdgeCap (double  edgeCap) {
    return LefDefParser::lefwLayerRoutingEdgeCap(edgeCap);
}

int lefwLayerRoutingAntennaArea (double  antennaArea) {
    return LefDefParser::lefwLayerRoutingAntennaArea(antennaArea);
}

int lefwLayerRoutingAntennaLength (double  antennaLength) {
    return LefDefParser::lefwLayerRoutingAntennaLength(antennaLength);
}

int lefwLayerRoutingMaxwidth (double  width) {
    return LefDefParser::lefwLayerRoutingMaxwidth(width);
}

int lefwLayerRoutingMinwidth (double  width) {
    return LefDefParser::lefwLayerRoutingMinwidth(width);
}

int lefwLayerRoutingMinenclosedarea (int  numMinenclosed, double*  area, double*  width) {
    return LefDefParser::lefwLayerRoutingMinenclosedarea(numMinenclosed, area, width);
}

int lefwLayerRoutingMinstep (double  distance) {
    return LefDefParser::lefwLayerRoutingMinstep(distance);
}

int lefwLayerRoutingMinstepWithOptions (double  distance, const char*  rule, double  maxLength) {
    return LefDefParser::lefwLayerRoutingMinstepWithOptions(distance, rule, maxLength);
}

int lefwLayerRoutingMinstepMaxEdges (double  distance, double  maxEdges) {
    return LefDefParser::lefwLayerRoutingMinstepMaxEdges(distance, maxEdges);
}

int lefwLayerRoutingProtrusion (double  width1, double  length, double  width2) {
    return LefDefParser::lefwLayerRoutingProtrusion(width1, length, width2);
}

int lefwLayerRoutingStartSpacingtableParallel (int  numLength, double*  length) {
    return LefDefParser::lefwLayerRoutingStartSpacingtableParallel(numLength, length);
}

int lefwLayerRoutingSpacingtableParallelWidth (double  width, int  numSpacing, double*  spacing) {
    return LefDefParser::lefwLayerRoutingSpacingtableParallelWidth(width, numSpacing, spacing);
}

int lefwLayerRoutingStartSpacingtableInfluence () {
    return LefDefParser::lefwLayerRoutingStartSpacingtableInfluence();
}

int lefwLayerRoutingSpacingInfluenceWidth (double  width, double  distance, double  spacing) {
    return LefDefParser::lefwLayerRoutingSpacingInfluenceWidth(width, distance, spacing);
}

int lefwLayerRoutingStartSpacingtableTwoWidths () {
    return LefDefParser::lefwLayerRoutingStartSpacingtableTwoWidths();
}

int lefwLayerRoutingSpacingtableTwoWidthsWidth (double  width, double  runLength, int  numSpacing, double*  spacing) {
    return LefDefParser::lefwLayerRoutingSpacingtableTwoWidthsWidth(width, runLength, numSpacing, spacing);
}

int lefwLayerRoutineEndSpacingtable () {
    return LefDefParser::lefwLayerRoutineEndSpacingtable();
}

int lefwEndLayerRouting (const char*  layerName) {
    return LefDefParser::lefwEndLayerRouting(layerName);
}

int lefwLayerACCurrentDensity (const char*  type, double  value) {
    return LefDefParser::lefwLayerACCurrentDensity(type, value);
}

int lefwLayerACFrequency (int  numFrequency, double*  frequency) {
    return LefDefParser::lefwLayerACFrequency(numFrequency, frequency);
}

int lefwLayerACWidth (int  numWidths, double*  widths) {
    return LefDefParser::lefwLayerACWidth(numWidths, widths);
}

int lefwLayerACCutarea (int  numCutareas, double*  cutareas) {
    return LefDefParser::lefwLayerACCutarea(numCutareas, cutareas);
}

int lefwLayerACTableEntries (int  numEntries, double*  entries) {
    return LefDefParser::lefwLayerACTableEntries(numEntries, entries);
}

int lefwLayerDCCurrentDensity (const char*  type, double  value) {
    return LefDefParser::lefwLayerDCCurrentDensity(type, value);
}

int lefwLayerDCWidth (int  numWidths, double*  widths) {
    return LefDefParser::lefwLayerDCWidth(numWidths, widths);
}

int lefwLayerDCCutarea (int  numCutareas, double*  cutareas) {
    return LefDefParser::lefwLayerDCCutarea(numCutareas, cutareas);
}

int lefwLayerDCTableEntries (int  numEntries, double*  entries) {
    return LefDefParser::lefwLayerDCTableEntries(numEntries, entries);
}

int lefwLayerAntennaModel (const char*  oxide) {
    return LefDefParser::lefwLayerAntennaModel(oxide);
}

int lefwLayerAntennaAreaRatio (double  value) {
    return LefDefParser::lefwLayerAntennaAreaRatio(value);
}

int lefwLayerAntennaDiffAreaRatio (double  value) {
    return LefDefParser::lefwLayerAntennaDiffAreaRatio(value);
}

int lefwLayerAntennaDiffAreaRatioPwl (int  numPwls, double*  diffusions, double*  ratios) {
    return LefDefParser::lefwLayerAntennaDiffAreaRatioPwl(numPwls, diffusions, ratios);
}

int lefwLayerAntennaCumAreaRatio (double  value) {
    return LefDefParser::lefwLayerAntennaCumAreaRatio(value);
}

int lefwLayerAntennaCumDiffAreaRatio (double  value) {
    return LefDefParser::lefwLayerAntennaCumDiffAreaRatio(value);
}

int lefwLayerAntennaCumDiffAreaRatioPwl (int  numPwls, double*  diffusions, double*  ratios) {
    return LefDefParser::lefwLayerAntennaCumDiffAreaRatioPwl(numPwls, diffusions, ratios);
}

int lefwLayerAntennaAreaFactor (double  value, const char*  diffUseOnly) {
    return LefDefParser::lefwLayerAntennaAreaFactor(value, diffUseOnly);
}

int lefwLayerAntennaSideAreaRatio (double  value) {
    return LefDefParser::lefwLayerAntennaSideAreaRatio(value);
}

int lefwLayerAntennaDiffSideAreaRatio (double  value) {
    return LefDefParser::lefwLayerAntennaDiffSideAreaRatio(value);
}

int lefwLayerAntennaDiffSideAreaRatioPwl (int  numPwls, double*  diffusions, double*  ratios) {
    return LefDefParser::lefwLayerAntennaDiffSideAreaRatioPwl(numPwls, diffusions, ratios);
}

int lefwLayerAntennaCumSideAreaRatio (double  value) {
    return LefDefParser::lefwLayerAntennaCumSideAreaRatio(value);
}

int lefwLayerAntennaCumDiffSideAreaRatio (double  value) {
    return LefDefParser::lefwLayerAntennaCumDiffSideAreaRatio(value);
}

int lefwLayerAntennaCumDiffSideAreaRatioPwl (int  numPwls, double*  diffusions, double*  ratios) {
    return LefDefParser::lefwLayerAntennaCumDiffSideAreaRatioPwl(numPwls, diffusions, ratios);
}

int lefwLayerAntennaSideAreaFactor (double  value, const char*  diffUseOnly) {
    return LefDefParser::lefwLayerAntennaSideAreaFactor(value, diffUseOnly);
}

int lefwLayerAntennaCumRoutingPlusCut () {
    return LefDefParser::lefwLayerAntennaCumRoutingPlusCut();
}

int lefwLayerAntennaGatePlusDiff (double  plusDiffFactor) {
    return LefDefParser::lefwLayerAntennaGatePlusDiff(plusDiffFactor);
}

int lefwLayerAntennaAreaMinusDiff (double  minusDiffFactor) {
    return LefDefParser::lefwLayerAntennaAreaMinusDiff(minusDiffFactor);
}

int lefwLayerAntennaAreaDiffReducePwl (int  numPwls, double*  diffAreas, double*  metalDiffFactors) {
    return LefDefParser::lefwLayerAntennaAreaDiffReducePwl(numPwls, diffAreas, metalDiffFactors);
}

int lefwMinimumDensity (double  minDensity) {
    return LefDefParser::lefwMinimumDensity(minDensity);
}

int lefwMaximumDensity (double  maxDensity) {
    return LefDefParser::lefwMaximumDensity(maxDensity);
}

int lefwDensityCheckWindow (double  checkWindowLength, double  checkWindowWidth) {
    return LefDefParser::lefwDensityCheckWindow(checkWindowLength, checkWindowWidth);
}

int lefwDensityCheckStep (double  checkStepValue) {
    return LefDefParser::lefwDensityCheckStep(checkStepValue);
}

int lefwFillActiveSpacing (double  fillToActiveSpacing) {
    return LefDefParser::lefwFillActiveSpacing(fillToActiveSpacing);
}

int lefwMaxviastack (int  value, const char*  bottomLayer, const char*  topLayer) {
    return LefDefParser::lefwMaxviastack(value, bottomLayer, topLayer);
}

int lefwStartPropDef () {
    return LefDefParser::lefwStartPropDef();
}

int lefwIntPropDef (const char*  objType, const char*  propName, double  leftRange, double  rightRange, int     propValue) {
    return LefDefParser::lefwIntPropDef(objType, propName, leftRange, rightRange, propValue);
}

int lefwRealPropDef (const char*  objType, const char*  propName, double  leftRange, double  rightRange, double  propValue) {
    return LefDefParser::lefwRealPropDef(objType, propName, leftRange, rightRange, propValue);
}

int lefwStringPropDef (const char*  objType, const char*  propName, double  leftRange, double  rightRange, const char*  propValue) {
    return LefDefParser::lefwStringPropDef(objType, propName, leftRange, rightRange, propValue);
}

int lefwEndPropDef () {
    return LefDefParser::lefwEndPropDef();
}

int lefwStartVia (const char*  viaName, const char*  isDefault) {
    return LefDefParser::lefwStartVia(viaName, isDefault);
}

int lefwViaTopofstackonly () {
    return LefDefParser::lefwViaTopofstackonly();
}

int lefwViaForeign (const char*  foreignName, double  xl, double  yl, int  orient) {
    return LefDefParser::lefwViaForeign(foreignName, xl, yl, orient);
}

int lefwViaForeignStr (const char*  foreignName, double  xl, double  yl, const char*  orient) {
    return LefDefParser::lefwViaForeignStr(foreignName, xl, yl, orient);
}

int lefwViaResistance (double  resistance) {
    return LefDefParser::lefwViaResistance(resistance);
}

int lefwViaLayer (const char*  layerName) {
    return LefDefParser::lefwViaLayer(layerName);
}

int lefwViaLayerRect (double  x1l, double  y1l, double  x2l, double  y2l, int  mask) {
    return LefDefParser::lefwViaLayerRect(x1l, y1l, x2l, y2l, mask);
}

int lefwViaLayerPolygon (int  num_polys, double*  xl, double*  yl, int  mask) {
    return LefDefParser::lefwViaLayerPolygon(num_polys, xl, yl, mask);
}

int lefwViaViarule (const char*  viaRuleName, double  xCutSize, double  yCutSize, const char*  botMetalLayer, const char*  cutLayer, const char*  topMetalLayer, double  xCutSpacing, double  yCutSpacing, double  xBotEnc, double  yBotEnc, double  xTopEnc, double  yTopEnc) {
    return LefDefParser::lefwViaViarule(viaRuleName, xCutSize, yCutSize, botMetalLayer, cutLayer, topMetalLayer, xCutSpacing, yCutSpacing, xBotEnc, yBotEnc, xTopEnc, yTopEnc);
}

int lefwViaViaruleRowCol (int  numCutRows, int  numCutCols) {
    return LefDefParser::lefwViaViaruleRowCol(numCutRows, numCutCols);
}

int lefwViaViaruleOrigin (double  xOffset, double  yOffset) {
    return LefDefParser::lefwViaViaruleOrigin(xOffset, yOffset);
}

int lefwViaViaruleOffset (double  xBotOffset, double  yBotOffset, double  xTopOffset, double  yTopOffset) {
    return LefDefParser::lefwViaViaruleOffset(xBotOffset, yBotOffset, xTopOffset, yTopOffset);
}

int lefwViaViarulePattern (const char*  cutPattern) {
    return LefDefParser::lefwViaViarulePattern(cutPattern);
}

int lefwStringProperty (const char*  propName, const char*  propValue) {
    return LefDefParser::lefwStringProperty(propName, propValue);
}

int lefwRealProperty (const char*  propName, double  propValue) {
    return LefDefParser::lefwRealProperty(propName, propValue);
}

int lefwIntProperty (const char*  propName, int  propValue) {
    return LefDefParser::lefwIntProperty(propName, propValue);
}

int lefwEndVia (const char*  viaName) {
    return LefDefParser::lefwEndVia(viaName);
}

int lefwStartViaRule (const char*  viaRuleName) {
    return LefDefParser::lefwStartViaRule(viaRuleName);
}

int lefwViaRuleLayer (const char*  layerName, const char*  direction, double  minWidth, double  maxWidth, double  overhang, double  metalOverhang) {
    return LefDefParser::lefwViaRuleLayer(layerName, direction, minWidth, maxWidth, overhang, metalOverhang);
}

int lefwViaRuleVia (const char*  viaName) {
    return LefDefParser::lefwViaRuleVia(viaName);
}

int lefwEndViaRule (const char*  viaRuleName) {
    return LefDefParser::lefwEndViaRule(viaRuleName);
}

int lefwStartViaRuleGen (const char*  viaRuleName) {
    return LefDefParser::lefwStartViaRuleGen(viaRuleName);
}

int lefwViaRuleGenDefault () {
    return LefDefParser::lefwViaRuleGenDefault();
}

int lefwViaRuleGenLayer (const char*  layerName, const char*  direction, double  minWidth, double  maxWidth, double  overhang, double  metalOverhang) {
    return LefDefParser::lefwViaRuleGenLayer(layerName, direction, minWidth, maxWidth, overhang, metalOverhang);
}

int lefwViaRuleGenLayerEnclosure (const char*  layerName, double  overhang1, double  overhang2, double  minWidth, double  maxWidth) {
    return LefDefParser::lefwViaRuleGenLayerEnclosure(layerName, overhang1, overhang2, minWidth, maxWidth);
}

int lefwViaRuleGenLayer3 (const char*  layerName, double  xl, double  yl, double  xh, double  yh, double  xSpacing, double  ySpacing, double  resistance) {
    return LefDefParser::lefwViaRuleGenLayer3(layerName, xl, yl, xh, yh, xSpacing, ySpacing, resistance);
}

int lefwEndViaRuleGen (const char*  viaRuleName) {
    return LefDefParser::lefwEndViaRuleGen(viaRuleName);
}

int lefwStartNonDefaultRule (const char*  ruleName) {
    return LefDefParser::lefwStartNonDefaultRule(ruleName);
}

int lefwNonDefaultRuleLayer (const char*  routingLayerName, double  width, double  minSpacing, double  wireExtension, double  resistance, double  capacitance, double  edgeCap) {
    return LefDefParser::lefwNonDefaultRuleLayer(routingLayerName, width, minSpacing, wireExtension, resistance, capacitance, edgeCap);
}

int lefwNonDefaultRuleHardspacing () {
    return LefDefParser::lefwNonDefaultRuleHardspacing();
}

int lefwNonDefaultRuleStartVia (const char*  viaName, const char*  isDefault) {
    return LefDefParser::lefwNonDefaultRuleStartVia(viaName, isDefault);
}

int lefwNonDefaultRuleEndVia (const char*  viaName) {
    return LefDefParser::lefwNonDefaultRuleEndVia(viaName);
}

int lefwNonDefaultRuleUseVia (const char*  viaName) {
    return LefDefParser::lefwNonDefaultRuleUseVia(viaName);
}

int lefwNonDefaultRuleUseViaRule (const char*  viaRuleName) {
    return LefDefParser::lefwNonDefaultRuleUseViaRule(viaRuleName);
}

int lefwNonDefaultRuleMinCuts (const char*  layerName, int  numCuts) {
    return LefDefParser::lefwNonDefaultRuleMinCuts(layerName, numCuts);
}

int lefwEndNonDefaultRule (const char*  ruleName) {
    return LefDefParser::lefwEndNonDefaultRule(ruleName);
}

int lefwStartSpacing () {
    return LefDefParser::lefwStartSpacing();
}

int lefwSpacing (const char*  layerName1, const char*  layerName2, double  minSpace, const char*  stack) {
    return LefDefParser::lefwSpacing(layerName1, layerName2, minSpace, stack);
}

int lefwEndSpacing () {
    return LefDefParser::lefwEndSpacing();
}

int lefwUniversalNoiseMargin (double  high, double  low) {
    return LefDefParser::lefwUniversalNoiseMargin(high, low);
}

int lefwEdgeRateThreshold1 (double  num) {
    return LefDefParser::lefwEdgeRateThreshold1(num);
}

int lefwEdgeRateThreshold2 (double  num) {
    return LefDefParser::lefwEdgeRateThreshold2(num);
}

int lefwEdgeRateScaleFactor (double  num) {
    return LefDefParser::lefwEdgeRateScaleFactor(num);
}

int lefwStartNoiseTable (int  num) {
    return LefDefParser::lefwStartNoiseTable(num);
}

int lefwEdgeRate (double  num) {
    return LefDefParser::lefwEdgeRate(num);
}

int lefwOutputResistance (int  numResists, double*  resistance) {
    return LefDefParser::lefwOutputResistance(numResists, resistance);
}

int lefwVictims (int  length, int  numNoises, double*  noises) {
    return LefDefParser::lefwVictims(length, numNoises, noises);
}

int lefwEndNoiseTable () {
    return LefDefParser::lefwEndNoiseTable();
}

int lefwStartCorrectTable (int  num) {
    return LefDefParser::lefwStartCorrectTable(num);
}

int lefwEndCorrectTable () {
    return LefDefParser::lefwEndCorrectTable();
}

int lefwMinFeature (double  x, double  y) {
    return LefDefParser::lefwMinFeature(x, y);
}

int lefwStartIrdrop () {
    return LefDefParser::lefwStartIrdrop();
}

int lefwIrdropTable (const char*  tableName, const char*  currentsNvolts) {
    return LefDefParser::lefwIrdropTable(tableName, currentsNvolts);
}

int lefwEndIrdrop () {
    return LefDefParser::lefwEndIrdrop();
}

int lefwSite (const char*  siteName, const char*  classType, const char*  symmetry, double  width, double  height) {
    return LefDefParser::lefwSite(siteName, classType, symmetry, width, height);
}

int lefwSiteRowPattern (const char*  siteName, int  orient) {
    return LefDefParser::lefwSiteRowPattern(siteName, orient);
}

int lefwSiteRowPatternStr (const char*  siteName, const char * orient) {
    return LefDefParser::lefwSiteRowPatternStr(siteName, orient);
}

int lefwEndSite (const char*  siteName) {
    return LefDefParser::lefwEndSite(siteName);
}

int lefwStartArray (const char*  arrayName) {
    return LefDefParser::lefwStartArray(arrayName);
}

int lefwArraySite (const char*  name, double  origX, double  origY, int  orient, double  numX, double  numY, double  spaceX, double  spaceY) {
    return LefDefParser::lefwArraySite(name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwArraySiteStr (const char*  name, double  origX, double  origY, const char * orient, double  numX, double  numY, double  spaceX, double  spaceY) {
    return LefDefParser::lefwArraySiteStr(name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwArrayCanplace (const char*  name, double  origX, double  origY, int  orient, double  numX, double  numY, double  spaceX, double  spaceY) {
    return LefDefParser::lefwArrayCanplace(name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwArrayCanplaceStr (const char*  name, double  origX, double  origY, const char * orient, double  numX, double  numY, double  spaceX, double  spaceY) {
    return LefDefParser::lefwArrayCanplaceStr(name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwArrayCannotoccupy (const char*  name, double  origX, double  origY, int  orient, double  numX, double  numY, double  spaceX, double  spaceY) {
    return LefDefParser::lefwArrayCannotoccupy(name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwArrayCannotoccupyStr (const char*  name, double  origX, double  origY, const char * orient, double  numX, double  numY, double  spaceX, double  spaceY) {
    return LefDefParser::lefwArrayCannotoccupyStr(name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwArrayTracks (const char*  xy, double  start, int  numTracks, double  space, const char*  layers) {
    return LefDefParser::lefwArrayTracks(xy, start, numTracks, space, layers);
}

int lefwStartArrayFloorplan (const char*  name) {
    return LefDefParser::lefwStartArrayFloorplan(name);
}

int lefwArrayFloorplan (const char*  site, const char*  name, double  origX, double  origY, int  orient, int  numX, int  numY, double  spaceX, double  spaceY) {
    return LefDefParser::lefwArrayFloorplan(site, name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwArrayFloorplanStr (const char*  site, const char*  name, double  origX, double  origY, const char * orient, int  numX, int  numY, double  spaceX, double  spaceY) {
    return LefDefParser::lefwArrayFloorplanStr(site, name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwEndArrayFloorplan (const char*  name) {
    return LefDefParser::lefwEndArrayFloorplan(name);
}

int lefwArrayGcellgrid (const char*  xy, double  startXY, int  colRows, double  spaceXY) {
    return LefDefParser::lefwArrayGcellgrid(xy, startXY, colRows, spaceXY);
}

int lefwStartArrayDefaultCap (int  size) {
    return LefDefParser::lefwStartArrayDefaultCap(size);
}

int lefwArrayDefaultCap (double  numPins, double  cap) {
    return LefDefParser::lefwArrayDefaultCap(numPins, cap);
}

int lefwEndArrayDefaultCap () {
    return LefDefParser::lefwEndArrayDefaultCap();
}

int lefwEndArray (const char*  arrayName) {
    return LefDefParser::lefwEndArray(arrayName);
}

int lefwStartMacro (const char*  macroName) {
    return LefDefParser::lefwStartMacro(macroName);
}

int lefwMacroClass (const char*  value1, const char*  value2) {
    return LefDefParser::lefwMacroClass(value1, value2);
}

int lefwMacroFixedMask () {
    return LefDefParser::lefwMacroFixedMask();
}

int lefwMacroSource (const char*  value1) {
    return LefDefParser::lefwMacroSource(value1);
}

int lefwMacroForeign (const char*  name, double  xl, double  yl, int  orient) {
    return LefDefParser::lefwMacroForeign(name, xl, yl, orient);
}

int lefwMacroForeignStr (const char*  name, double  xl, double  yl, const char * orient) {
    return LefDefParser::lefwMacroForeignStr(name, xl, yl, orient);
}

int lefwMacroOrigin (double  xl, double  yl) {
    return LefDefParser::lefwMacroOrigin(xl, yl);
}

int lefwMacroEEQ (const char*  macroName) {
    return LefDefParser::lefwMacroEEQ(macroName);
}

int lefwMacroLEQ (const char*  macroName) {
    return LefDefParser::lefwMacroLEQ(macroName);
}

int lefwMacroSize (double  width, double  height) {
    return LefDefParser::lefwMacroSize(width, height);
}

int lefwMacroSymmetry (const char*  symmetry) {
    return LefDefParser::lefwMacroSymmetry(symmetry);
}

int lefwMacroSite (const char*  siteName) {
    return LefDefParser::lefwMacroSite(siteName);
}

int lefwMacroSitePattern (const char*  name, double  origX, double  origY, int  orient, int  numX, int  numY, double  spaceX, double  spaceY) {
    return LefDefParser::lefwMacroSitePattern(name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwMacroSitePatternStr (const char*  name, double  origX, double  origY, const char * orient, int  numX, int  numY, double  spaceX, double  spaceY) {
    return LefDefParser::lefwMacroSitePatternStr(name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwMacroPower (double  power) {
    return LefDefParser::lefwMacroPower(power);
}

int lefwEndMacro (const char*  macroName) {
    return LefDefParser::lefwEndMacro(macroName);
}

int lefwStartMacroDensity (const char*  layerName) {
    return LefDefParser::lefwStartMacroDensity(layerName);
}

int lefwMacroDensityLayerRect (double  x1, double  y1, double  x2, double  y2, double  densityValue) {
    return LefDefParser::lefwMacroDensityLayerRect(x1, y1, x2, y2, densityValue);
}

int lefwEndMacroDensity () {
    return LefDefParser::lefwEndMacroDensity();
}

int lefwStartMacroPin (const char*  pinName) {
    return LefDefParser::lefwStartMacroPin(pinName);
}

int lefwMacroPinTaperRule (const char*  ruleName) {
    return LefDefParser::lefwMacroPinTaperRule(ruleName);
}

int lefwMacroPinForeign (const char*  name, double  xl, double  yl, int  orient) {
    return LefDefParser::lefwMacroPinForeign(name, xl, yl, orient);
}

int lefwMacroPinForeignStr (const char*  name, double  xl, double  yl, const char*  orient) {
    return LefDefParser::lefwMacroPinForeignStr(name, xl, yl, orient);
}

int lefwMacroPinLEQ (const char*  pinName) {
    return LefDefParser::lefwMacroPinLEQ(pinName);
}

int lefwMacroPinDirection (const char*  direction) {
    return LefDefParser::lefwMacroPinDirection(direction);
}

int lefwMacroPinUse (const char*  use) {
    return LefDefParser::lefwMacroPinUse(use);
}

int lefwMacroPinShape (const char*  name) {
    return LefDefParser::lefwMacroPinShape(name);
}

int lefwMacroPinMustjoin (const char*  name) {
    return LefDefParser::lefwMacroPinMustjoin(name);
}

int lefwMacroPinNetExpr (const char*  name) {
    return LefDefParser::lefwMacroPinNetExpr(name);
}

int lefwMacroPinSupplySensitivity (const char*  pinName) {
    return LefDefParser::lefwMacroPinSupplySensitivity(pinName);
}

int lefwMacroPinGroundSensitivity (const char*  pinName) {
    return LefDefParser::lefwMacroPinGroundSensitivity(pinName);
}

int lefwMacroPinOutputnoisemargin (int  high, int  low) {
    return LefDefParser::lefwMacroPinOutputnoisemargin(high, low);
}

int lefwMacroPinOutputresistance (int  high, int  low) {
    return LefDefParser::lefwMacroPinOutputresistance(high, low);
}

int lefwMacroPinInputnoisemargin (int  high, int  low) {
    return LefDefParser::lefwMacroPinInputnoisemargin(high, low);
}

int lefwMacroPinPower (double  power) {
    return LefDefParser::lefwMacroPinPower(power);
}

int lefwMacroPinLeakage (double  leakage) {
    return LefDefParser::lefwMacroPinLeakage(leakage);
}

int lefwMacroPinCapacitance (double  capacitance) {
    return LefDefParser::lefwMacroPinCapacitance(capacitance);
}

int lefwMacroPinResistance (double  resistance) {
    return LefDefParser::lefwMacroPinResistance(resistance);
}

int lefwMacroPinPulldownres (double  resistance) {
    return LefDefParser::lefwMacroPinPulldownres(resistance);
}

int lefwMacroPinTieoffr (double  resistance) {
    return LefDefParser::lefwMacroPinTieoffr(resistance);
}

int lefwMacroPinVHI (double  voltage) {
    return LefDefParser::lefwMacroPinVHI(voltage);
}

int lefwMacroPinVLO (double  voltage) {
    return LefDefParser::lefwMacroPinVLO(voltage);
}

int lefwMacroPinRisevoltagethreshold (double  voltage) {
    return LefDefParser::lefwMacroPinRisevoltagethreshold(voltage);
}

int lefwMacroPinFallvoltagethreshold (double  voltage) {
    return LefDefParser::lefwMacroPinFallvoltagethreshold(voltage);
}

int lefwMacroPinRisethresh (double  capacitance) {
    return LefDefParser::lefwMacroPinRisethresh(capacitance);
}

int lefwMacroPinFallthresh (double  capacitance) {
    return LefDefParser::lefwMacroPinFallthresh(capacitance);
}

int lefwMacroPinRisesatcur (double  current) {
    return LefDefParser::lefwMacroPinRisesatcur(current);
}

int lefwMacroPinFallsatcur (double  current) {
    return LefDefParser::lefwMacroPinFallsatcur(current);
}

int lefwMacroPinCurrentsource (const char*  name) {
    return LefDefParser::lefwMacroPinCurrentsource(name);
}

int lefwMacroPinIV_Tables (const char*  lowName, const char*  highName) {
    return LefDefParser::lefwMacroPinIV_Tables(lowName, highName);
}

int lefwMacroPinAntennasize (double  value, const char*  layerName) {
    return LefDefParser::lefwMacroPinAntennasize(value, layerName);
}

int lefwMacroPinAntennaMetalArea (double  value, const char*  layerName) {
    return LefDefParser::lefwMacroPinAntennaMetalArea(value, layerName);
}

int lefwMacroPinAntennaMetalLength (double  value, const char*  layerName) {
    return LefDefParser::lefwMacroPinAntennaMetalLength(value, layerName);
}

int lefwMacroPinAntennaPartialMetalArea (double  value, const char*  layerName) {
    return LefDefParser::lefwMacroPinAntennaPartialMetalArea(value, layerName);
}

int lefwMacroPinAntennaPartialMetalSideArea (double  value, const char*  layerName) {
    return LefDefParser::lefwMacroPinAntennaPartialMetalSideArea(value, layerName);
}

int lefwMacroPinAntennaPartialCutArea (double  value, const char*  layerName) {
    return LefDefParser::lefwMacroPinAntennaPartialCutArea(value, layerName);
}

int lefwMacroPinAntennaDiffArea (double  value, const char*  layerName) {
    return LefDefParser::lefwMacroPinAntennaDiffArea(value, layerName);
}

int lefwMacroPinAntennaModel (const char*  oxide) {
    return LefDefParser::lefwMacroPinAntennaModel(oxide);
}

int lefwMacroPinAntennaGateArea (double  value, const char*  layerName) {
    return LefDefParser::lefwMacroPinAntennaGateArea(value, layerName);
}

int lefwMacroPinAntennaMaxAreaCar (double  value, const char*  layerName) {
    return LefDefParser::lefwMacroPinAntennaMaxAreaCar(value, layerName);
}

int lefwMacroPinAntennaMaxSideAreaCar (double  value, const char*  layerName) {
    return LefDefParser::lefwMacroPinAntennaMaxSideAreaCar(value, layerName);
}

int lefwMacroPinAntennaMaxCutCar (double  value, const char*  layerName) {
    return LefDefParser::lefwMacroPinAntennaMaxCutCar(value, layerName);
}

int lefwEndMacroPin (const char*  pinName) {
    return LefDefParser::lefwEndMacroPin(pinName);
}

int lefwStartMacroPinPort (const char*  classType) {
    return LefDefParser::lefwStartMacroPinPort(classType);
}

int lefwMacroPinPortLayer (const char*  layerName, double  spacing) {
    return LefDefParser::lefwMacroPinPortLayer(layerName, spacing);
}

int lefwMacroPinPortDesignRuleWidth (const char*  layerName, double  width) {
    return LefDefParser::lefwMacroPinPortDesignRuleWidth(layerName, width);
}

int lefwMacroPinPortLayerWidth (double  width) {
    return LefDefParser::lefwMacroPinPortLayerWidth(width);
}

int lefwMacroPinPortLayerPath (int  num_paths, double*  xl, double*  yl, int  numX, int  numY, double  spaceX, double  spaceY, int     mask) {
    return LefDefParser::lefwMacroPinPortLayerPath(num_paths, xl, yl, numX, numY, spaceX, spaceY, mask);
}

int lefwMacroPinPortLayerRect (double  xl1, double  yl1, double  xl2, double  yl2, int  numX, int  numY, double  spaceX, double  spaceY, int  mask) {
    return LefDefParser::lefwMacroPinPortLayerRect(xl1, yl1, xl2, yl2, numX, numY, spaceX, spaceY, mask);
}

int lefwMacroPinPortLayerPolygon (int  num_polys, double*  xl, double*  yl, int  numX, int  numY, double  spaceX, double  spaceY, int  mask) {
    return LefDefParser::lefwMacroPinPortLayerPolygon(num_polys, xl, yl, numX, numY, spaceX, spaceY, mask);
}

int lefwMacroPinPortVia (double  xl, double  yl, const char*  viaName, int  numX, int  numY, double  spaceX, double  spaceY, int  mask) {
    return LefDefParser::lefwMacroPinPortVia(xl, yl, viaName, numX, numY, spaceX, spaceY, mask);
}

int lefwEndMacroPinPort () {
    return LefDefParser::lefwEndMacroPinPort();
}

int lefwStartMacroObs () {
    return LefDefParser::lefwStartMacroObs();
}

int lefwMacroObsLayer (const char*  layerName, double  spacing) {
    return LefDefParser::lefwMacroObsLayer(layerName, spacing);
}

int lefwMacroObsDesignRuleWidth (const char*  layerName, double  width) {
    return LefDefParser::lefwMacroObsDesignRuleWidth(layerName, width);
}

int lefwMacroExceptPGNet (const char*  layerName) {
    return LefDefParser::lefwMacroExceptPGNet(layerName);
}

int lefwMacroObsLayerWidth (double  width) {
    return LefDefParser::lefwMacroObsLayerWidth(width);
}

int lefwMacroObsLayerPath (int  num_paths, double*  xl, double*  yl, int  numX, int  numY, double  spaceX, double  spaceY, int  mask) {
    return LefDefParser::lefwMacroObsLayerPath(num_paths, xl, yl, numX, numY, spaceX, spaceY, mask);
}

int lefwMacroObsLayerRect (double  xl1, double  yl1, double  xl2, double  yl2, int  numX, int  numY, double  spaceX, double  spaceY, int  mask) {
    return LefDefParser::lefwMacroObsLayerRect(xl1, yl1, xl2, yl2, numX, numY, spaceX, spaceY, mask);
}

int lefwMacroObsLayerPolygon (int  num_polys, double*  xl, double*  yl, int  numX, int  numY, double  spaceX, double  spaceY, int  mask) {
    return LefDefParser::lefwMacroObsLayerPolygon(num_polys, xl, yl, numX, numY, spaceX, spaceY, mask);
}

int lefwMacroObsVia (double  xl, double  yl, const char*  viaName, int  numX, int  numY, double  spaceX, double  spaceY, int  mask) {
    return LefDefParser::lefwMacroObsVia(xl, yl, viaName, numX, numY, spaceX, spaceY, mask);
}

int lefwEndMacroObs () {
    return LefDefParser::lefwEndMacroObs();
}

int lefwStartMacroTiming () {
    return LefDefParser::lefwStartMacroTiming();
}

int lefwMacroTimingPin (const char*  fromPin, const char*  toPin) {
    return LefDefParser::lefwMacroTimingPin(fromPin, toPin);
}

int lefwMacroTimingIntrinsic (const char*  riseFall, double  min, double  max, double  slewT1, double  slewT1Min, double  slewT1Max, double  slewT2, double  slewT2Min, double  slewT2Max, double  slewT3, double  varMin, double  varMax) {
    return LefDefParser::lefwMacroTimingIntrinsic(riseFall, min, max, slewT1, slewT1Min, slewT1Max, slewT2, slewT2Min, slewT2Max, slewT3, varMin, varMax);
}

int lefwMacroTimingRisers (double  min, double  max) {
    return LefDefParser::lefwMacroTimingRisers(min, max);
}

int lefwMacroTimingFallrs (double  min, double  max) {
    return LefDefParser::lefwMacroTimingFallrs(min, max);
}

int lefwMacroTimingRisecs (double  min, double  max) {
    return LefDefParser::lefwMacroTimingRisecs(min, max);
}

int lefwMacroTimingFallcs (double  min, double  max) {
    return LefDefParser::lefwMacroTimingFallcs(min, max);
}

int lefwMacroTimingRisesatt1 (double  min, double  max) {
    return LefDefParser::lefwMacroTimingRisesatt1(min, max);
}

int lefwMacroTimingFallsatt1 (double  min, double  max) {
    return LefDefParser::lefwMacroTimingFallsatt1(min, max);
}

int lefwMacroTimingRiset0 (double  min, double  max) {
    return LefDefParser::lefwMacroTimingRiset0(min, max);
}

int lefwMacroTimingFallt0 (double  min, double  max) {
    return LefDefParser::lefwMacroTimingFallt0(min, max);
}

int lefwMacroTimingUnateness (const char*  unateness) {
    return LefDefParser::lefwMacroTimingUnateness(unateness);
}

int lefwEndMacroTiming () {
    return LefDefParser::lefwEndMacroTiming();
}

int lefwAntenna (const char*  type, double  value) {
    return LefDefParser::lefwAntenna(type, value);
}

int lefwStartBeginext (const char*  name) {
    return LefDefParser::lefwStartBeginext(name);
}

int lefwBeginextCreator (const char*  creatorName) {
    return LefDefParser::lefwBeginextCreator(creatorName);
}

int lefwBeginextDate () {
    return LefDefParser::lefwBeginextDate();
}

int lefwBeginextRevision (int  vers1, int  vers2) {
    return LefDefParser::lefwBeginextRevision(vers1, vers2);
}

int lefwBeginextSyntax (const char*  title, const char*  string) {
    return LefDefParser::lefwBeginextSyntax(title, string);
}

int lefwEndBeginext () {
    return LefDefParser::lefwEndBeginext();
}

int lefwCurrentLineNumber () {
    return LefDefParser::lefwCurrentLineNumber();
}

int lefwEnd () {
    return LefDefParser::lefwEnd();
}

void lefwPrintError (int  status) {
    LefDefParser::lefwPrintError(status);
}

void lefwAddComment (const char*  comment) {
    LefDefParser::lefwAddComment(comment);
}

void lefwAddIndent () {
    LefDefParser::lefwAddIndent();
}

