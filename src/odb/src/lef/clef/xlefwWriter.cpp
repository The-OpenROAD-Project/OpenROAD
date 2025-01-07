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

#include "lefwWriter.h"
#include "lefwWriter.hpp"

// Wrappers definitions.
int lefwInit(FILE* f)
{
  return LefParser::lefwInit(f);
}

int lefwInitCbk(FILE* f)
{
  return LefParser::lefwInitCbk(f);
}

int lefwEncrypt()
{
  return LefParser::lefwEncrypt();
}

int lefwCloseEncrypt()
{
  return LefParser::lefwCloseEncrypt();
}

int lefwNewLine()
{
  return LefParser::lefwNewLine();
}

int lefwVersion(int vers1, int vers2)
{
  return LefParser::lefwVersion(vers1, vers2);
}

int lefwCaseSensitive(const char* caseSensitive)
{
  return LefParser::lefwCaseSensitive(caseSensitive);
}

int lefwNoWireExtensionAtPin(const char* noWireExt)
{
  return LefParser::lefwNoWireExtensionAtPin(noWireExt);
}

int lefwMinfeature(double minFeatureX, double minFeatureY)
{
  return LefParser::lefwMinfeature(minFeatureX, minFeatureY);
}

int lefwDielectric(double dielectric)
{
  return LefParser::lefwDielectric(dielectric);
}

int lefwBusBitChars(const char* busBitChars)
{
  return LefParser::lefwBusBitChars(busBitChars);
}

int lefwDividerChar(const char* dividerChar)
{
  return LefParser::lefwDividerChar(dividerChar);
}

int lefwManufacturingGrid(double grid)
{
  return LefParser::lefwManufacturingGrid(grid);
}

int lefwFixedMask()
{
  return LefParser::lefwFixedMask();
}

int lefwUseMinSpacing(const char* type, const char* onOff)
{
  return LefParser::lefwUseMinSpacing(type, onOff);
}

int lefwClearanceMeasure(const char* type)
{
  return LefParser::lefwClearanceMeasure(type);
}

int lefwAntennaInputGateArea(double inputGateArea)
{
  return LefParser::lefwAntennaInputGateArea(inputGateArea);
}

int lefwAntennaInOutDiffArea(double inOutDiffArea)
{
  return LefParser::lefwAntennaInOutDiffArea(inOutDiffArea);
}

int lefwAntennaOutputDiffArea(double outputDiffArea)
{
  return LefParser::lefwAntennaOutputDiffArea(outputDiffArea);
}

int lefwStartUnits()
{
  return LefParser::lefwStartUnits();
}

int lefwUnits(double time,
              double capacitance,
              double resistance,
              double power,
              double current,
              double voltage,
              double database)
{
  return LefParser::lefwUnits(
      time, capacitance, resistance, power, current, voltage, database);
}

int lefwUnitsFrequency(double frequency)
{
  return LefParser::lefwUnitsFrequency(frequency);
}

int lefwEndUnits()
{
  return LefParser::lefwEndUnits();
}

int lefwStartLayer(const char* layerName, const char* type)
{
  return LefParser::lefwStartLayer(layerName, type);
}

int lefwLayerMask(int maskColor)
{
  return LefParser::lefwLayerMask(maskColor);
}

int lefwLayerWidth(double minWidth)
{
  return LefParser::lefwLayerWidth(minWidth);
}

int lefwLayerCutSpacing(double spacing)
{
  return LefParser::lefwLayerCutSpacing(spacing);
}

int lefwLayerCutSpacingCenterToCenter()
{
  return LefParser::lefwLayerCutSpacingCenterToCenter();
}

int lefwLayerCutSpacingSameNet()
{
  return LefParser::lefwLayerCutSpacingSameNet();
}

int lefwLayerCutSpacingLayer(const char* name2, int stack)
{
  return LefParser::lefwLayerCutSpacingLayer(name2, stack);
}

int lefwLayerCutSpacingAdjacent(int viaCuts, double distance, int stack)
{
  return LefParser::lefwLayerCutSpacingAdjacent(viaCuts, distance, stack);
}

int lefwLayerCutSpacingParallel()
{
  return LefParser::lefwLayerCutSpacingParallel();
}

int lefwLayerCutSpacingArea(double cutArea)
{
  return LefParser::lefwLayerCutSpacingArea(cutArea);
}

int lefwLayerCutSpacingEnd()
{
  return LefParser::lefwLayerCutSpacingEnd();
}

int lefwLayerCutSpacingTableOrtho(int numSpacing,
                                  double* cutWithins,
                                  double* orthoSpacings)
{
  return LefParser::lefwLayerCutSpacingTableOrtho(
      numSpacing, cutWithins, orthoSpacings);
}

int lefwLayerArraySpacing(int longArray,
                          double viaWidth,
                          double cutSpacing,
                          int numArrayCut,
                          int* arrayCuts,
                          double* arraySpacings)
{
  return LefParser::lefwLayerArraySpacing(
      longArray, viaWidth, cutSpacing, numArrayCut, arrayCuts, arraySpacings);
}

int lefwLayerEnclosure(const char* location,
                       double overhang1,
                       double overhang2,
                       double width)
{
  return LefParser::lefwLayerEnclosure(location, overhang1, overhang2, width);
}

int lefwLayerEnclosureWidth(const char* location,
                            double overhang1,
                            double overhang2,
                            double width,
                            double cutWithin)
{
  return LefParser::lefwLayerEnclosureWidth(
      location, overhang1, overhang2, width, cutWithin);
}

int lefwLayerEnclosureLength(const char* location,
                             double overhang1,
                             double overhang2,
                             double minLength)
{
  return LefParser::lefwLayerEnclosureLength(
      location, overhang1, overhang2, minLength);
}

int lefwLayerPreferEnclosure(const char* location,
                             double overhang1,
                             double overhang2,
                             double width)
{
  return LefParser::lefwLayerPreferEnclosure(
      location, overhang1, overhang2, width);
}

int lefwLayerResistancePerCut(double resistance)
{
  return LefParser::lefwLayerResistancePerCut(resistance);
}

int lefwEndLayer(const char* layerName)
{
  return LefParser::lefwEndLayer(layerName);
}

int lefwStartLayerRouting(const char* layerName)
{
  return LefParser::lefwStartLayerRouting(layerName);
}

int lefwLayerRouting(const char* direction, double width)
{
  return LefParser::lefwLayerRouting(direction, width);
}

int lefwLayerRoutingPitch(double pitch)
{
  return LefParser::lefwLayerRoutingPitch(pitch);
}

int lefwLayerRoutingPitchXYDistance(double xDistance, double yDistance)
{
  return LefParser::lefwLayerRoutingPitchXYDistance(xDistance, yDistance);
}

int lefwLayerRoutingDiagPitch(double distance)
{
  return LefParser::lefwLayerRoutingDiagPitch(distance);
}

int lefwLayerRoutingDiagPitchXYDistance(double diag45Distance,
                                        double diag135Distance)
{
  return LefParser::lefwLayerRoutingDiagPitchXYDistance(diag45Distance,
                                                        diag135Distance);
}

int lefwLayerRoutingDiagWidth(double diagWidth)
{
  return LefParser::lefwLayerRoutingDiagWidth(diagWidth);
}

int lefwLayerRoutingDiagSpacing(double diagSpacing)
{
  return LefParser::lefwLayerRoutingDiagSpacing(diagSpacing);
}

int lefwLayerRoutingDiagMinEdgeLength(double diagLength)
{
  return LefParser::lefwLayerRoutingDiagMinEdgeLength(diagLength);
}

int lefwLayerRoutingOffset(double offset)
{
  return LefParser::lefwLayerRoutingOffset(offset);
}

int lefwLayerRoutingOffsetXYDistance(double xDistance, double yDistance)
{
  return LefParser::lefwLayerRoutingOffsetXYDistance(xDistance, yDistance);
}

int lefwLayerRoutingArea(double area)
{
  return LefParser::lefwLayerRoutingArea(area);
}

int lefwLayerRoutingMinsize(int numRect, double* minWidth, double* minLength)
{
  return LefParser::lefwLayerRoutingMinsize(numRect, minWidth, minLength);
}

int lefwLayerRoutingMinimumcut(double numCuts, double minWidth)
{
  return LefParser::lefwLayerRoutingMinimumcut(numCuts, minWidth);
}

int lefwLayerRoutingMinimumcutWithin(double numCuts,
                                     double minWidth,
                                     double cutDistance)
{
  return LefParser::lefwLayerRoutingMinimumcutWithin(
      numCuts, minWidth, cutDistance);
}

int lefwLayerRoutingMinimumcutConnections(const char* direction)
{
  return LefParser::lefwLayerRoutingMinimumcutConnections(direction);
}

int lefwLayerRoutingMinimumcutLengthWithin(double length, double distance)
{
  return LefParser::lefwLayerRoutingMinimumcutLengthWithin(length, distance);
}

int lefwLayerRoutingSpacing(double spacing)
{
  return LefParser::lefwLayerRoutingSpacing(spacing);
}

int lefwLayerRoutingSpacingRange(double minWidth, double maxWidth)
{
  return LefParser::lefwLayerRoutingSpacingRange(minWidth, maxWidth);
}

int lefwLayerRoutingSpacingRangeUseLengthThreshold()
{
  return LefParser::lefwLayerRoutingSpacingRangeUseLengthThreshold();
}

int lefwLayerRoutingSpacingRangeInfluence(double infValue,
                                          double subMinWidth,
                                          double subMaxWidth)
{
  return LefParser::lefwLayerRoutingSpacingRangeInfluence(
      infValue, subMinWidth, subMaxWidth);
}

int lefwLayerRoutingSpacingRangeRange(double minWidth, double maxWidth)
{
  return LefParser::lefwLayerRoutingSpacingRangeRange(minWidth, maxWidth);
}

int lefwLayerRoutingSpacingLengthThreshold(double lengthValue,
                                           double minWidth,
                                           double maxWidth)
{
  return LefParser::lefwLayerRoutingSpacingLengthThreshold(
      lengthValue, minWidth, maxWidth);
}

int lefwLayerRoutingSpacingSameNet(int PGOnly)
{
  return LefParser::lefwLayerRoutingSpacingSameNet(PGOnly);
}

int lefwLayerRoutingSpacingEndOfLine(double eolWidth, double eolWithin)
{
  return LefParser::lefwLayerRoutingSpacingEndOfLine(eolWidth, eolWithin);
}

int lefwLayerRoutingSpacingEOLParallel(double parSpace,
                                       double parWithin,
                                       int twoEdges)
{
  return LefParser::lefwLayerRoutingSpacingEOLParallel(
      parSpace, parWithin, twoEdges);
}

int lefwLayerRoutingSpacingNotchLength(double minNLength)
{
  return LefParser::lefwLayerRoutingSpacingNotchLength(minNLength);
}

int lefwLayerRoutingSpacingEndOfNotchWidth(double eonWidth,
                                           double minNSpacing,
                                           double minNLength)
{
  return LefParser::lefwLayerRoutingSpacingEndOfNotchWidth(
      eonWidth, minNSpacing, minNLength);
}

int lefwLayerRoutingWireExtension(double wireExtension)
{
  return LefParser::lefwLayerRoutingWireExtension(wireExtension);
}

int lefwLayerRoutingResistance(const char* resistance)
{
  return LefParser::lefwLayerRoutingResistance(resistance);
}

int lefwLayerRoutingCapacitance(const char* capacitance)
{
  return LefParser::lefwLayerRoutingCapacitance(capacitance);
}

int lefwLayerRoutingHeight(double height)
{
  return LefParser::lefwLayerRoutingHeight(height);
}

int lefwLayerRoutingThickness(double thickness)
{
  return LefParser::lefwLayerRoutingThickness(thickness);
}

int lefwLayerRoutingShrinkage(double shrinkage)
{
  return LefParser::lefwLayerRoutingShrinkage(shrinkage);
}

int lefwLayerRoutingCapMultiplier(double capMultiplier)
{
  return LefParser::lefwLayerRoutingCapMultiplier(capMultiplier);
}

int lefwLayerRoutingEdgeCap(double edgeCap)
{
  return LefParser::lefwLayerRoutingEdgeCap(edgeCap);
}

int lefwLayerRoutingAntennaArea(double antennaArea)
{
  return LefParser::lefwLayerRoutingAntennaArea(antennaArea);
}

int lefwLayerRoutingAntennaLength(double antennaLength)
{
  return LefParser::lefwLayerRoutingAntennaLength(antennaLength);
}

int lefwLayerRoutingMaxwidth(double width)
{
  return LefParser::lefwLayerRoutingMaxwidth(width);
}

int lefwLayerRoutingMinwidth(double width)
{
  return LefParser::lefwLayerRoutingMinwidth(width);
}

int lefwLayerRoutingMinenclosedarea(int numMinenclosed,
                                    double* area,
                                    double* width)
{
  return LefParser::lefwLayerRoutingMinenclosedarea(
      numMinenclosed, area, width);
}

int lefwLayerRoutingMinstep(double distance)
{
  return LefParser::lefwLayerRoutingMinstep(distance);
}

int lefwLayerRoutingMinstepWithOptions(double distance,
                                       const char* rule,
                                       double maxLength)
{
  return LefParser::lefwLayerRoutingMinstepWithOptions(
      distance, rule, maxLength);
}

int lefwLayerRoutingMinstepMaxEdges(double distance, double maxEdges)
{
  return LefParser::lefwLayerRoutingMinstepMaxEdges(distance, maxEdges);
}

int lefwLayerRoutingProtrusion(double width1, double length, double width2)
{
  return LefParser::lefwLayerRoutingProtrusion(width1, length, width2);
}

int lefwLayerRoutingStartSpacingtableParallel(int numLength, double* length)
{
  return LefParser::lefwLayerRoutingStartSpacingtableParallel(numLength,
                                                              length);
}

int lefwLayerRoutingSpacingtableParallelWidth(double width,
                                              int numSpacing,
                                              double* spacing)
{
  return LefParser::lefwLayerRoutingSpacingtableParallelWidth(
      width, numSpacing, spacing);
}

int lefwLayerRoutingStartSpacingtableInfluence()
{
  return LefParser::lefwLayerRoutingStartSpacingtableInfluence();
}

int lefwLayerRoutingSpacingInfluenceWidth(double width,
                                          double distance,
                                          double spacing)
{
  return LefParser::lefwLayerRoutingSpacingInfluenceWidth(
      width, distance, spacing);
}

int lefwLayerRoutingStartSpacingtableTwoWidths()
{
  return LefParser::lefwLayerRoutingStartSpacingtableTwoWidths();
}

int lefwLayerRoutingSpacingtableTwoWidthsWidth(double width,
                                               double runLength,
                                               int numSpacing,
                                               double* spacing)
{
  return LefParser::lefwLayerRoutingSpacingtableTwoWidthsWidth(
      width, runLength, numSpacing, spacing);
}

int lefwLayerRoutineEndSpacingtable()
{
  return LefParser::lefwLayerRoutineEndSpacingtable();
}

int lefwEndLayerRouting(const char* layerName)
{
  return LefParser::lefwEndLayerRouting(layerName);
}

int lefwLayerACCurrentDensity(const char* type, double value)
{
  return LefParser::lefwLayerACCurrentDensity(type, value);
}

int lefwLayerACFrequency(int numFrequency, double* frequency)
{
  return LefParser::lefwLayerACFrequency(numFrequency, frequency);
}

int lefwLayerACWidth(int numWidths, double* widths)
{
  return LefParser::lefwLayerACWidth(numWidths, widths);
}

int lefwLayerACCutarea(int numCutareas, double* cutareas)
{
  return LefParser::lefwLayerACCutarea(numCutareas, cutareas);
}

int lefwLayerACTableEntries(int numEntries, double* entries)
{
  return LefParser::lefwLayerACTableEntries(numEntries, entries);
}

int lefwLayerDCCurrentDensity(const char* type, double value)
{
  return LefParser::lefwLayerDCCurrentDensity(type, value);
}

int lefwLayerDCWidth(int numWidths, double* widths)
{
  return LefParser::lefwLayerDCWidth(numWidths, widths);
}

int lefwLayerDCCutarea(int numCutareas, double* cutareas)
{
  return LefParser::lefwLayerDCCutarea(numCutareas, cutareas);
}

int lefwLayerDCTableEntries(int numEntries, double* entries)
{
  return LefParser::lefwLayerDCTableEntries(numEntries, entries);
}

int lefwLayerAntennaModel(const char* oxide)
{
  return LefParser::lefwLayerAntennaModel(oxide);
}

int lefwLayerAntennaAreaRatio(double value)
{
  return LefParser::lefwLayerAntennaAreaRatio(value);
}

int lefwLayerAntennaDiffAreaRatio(double value)
{
  return LefParser::lefwLayerAntennaDiffAreaRatio(value);
}

int lefwLayerAntennaDiffAreaRatioPwl(int numPwls,
                                     double* diffusions,
                                     double* ratios)
{
  return LefParser::lefwLayerAntennaDiffAreaRatioPwl(
      numPwls, diffusions, ratios);
}

int lefwLayerAntennaCumAreaRatio(double value)
{
  return LefParser::lefwLayerAntennaCumAreaRatio(value);
}

int lefwLayerAntennaCumDiffAreaRatio(double value)
{
  return LefParser::lefwLayerAntennaCumDiffAreaRatio(value);
}

int lefwLayerAntennaCumDiffAreaRatioPwl(int numPwls,
                                        double* diffusions,
                                        double* ratios)
{
  return LefParser::lefwLayerAntennaCumDiffAreaRatioPwl(
      numPwls, diffusions, ratios);
}

int lefwLayerAntennaAreaFactor(double value, const char* diffUseOnly)
{
  return LefParser::lefwLayerAntennaAreaFactor(value, diffUseOnly);
}

int lefwLayerAntennaSideAreaRatio(double value)
{
  return LefParser::lefwLayerAntennaSideAreaRatio(value);
}

int lefwLayerAntennaDiffSideAreaRatio(double value)
{
  return LefParser::lefwLayerAntennaDiffSideAreaRatio(value);
}

int lefwLayerAntennaDiffSideAreaRatioPwl(int numPwls,
                                         double* diffusions,
                                         double* ratios)
{
  return LefParser::lefwLayerAntennaDiffSideAreaRatioPwl(
      numPwls, diffusions, ratios);
}

int lefwLayerAntennaCumSideAreaRatio(double value)
{
  return LefParser::lefwLayerAntennaCumSideAreaRatio(value);
}

int lefwLayerAntennaCumDiffSideAreaRatio(double value)
{
  return LefParser::lefwLayerAntennaCumDiffSideAreaRatio(value);
}

int lefwLayerAntennaCumDiffSideAreaRatioPwl(int numPwls,
                                            double* diffusions,
                                            double* ratios)
{
  return LefParser::lefwLayerAntennaCumDiffSideAreaRatioPwl(
      numPwls, diffusions, ratios);
}

int lefwLayerAntennaSideAreaFactor(double value, const char* diffUseOnly)
{
  return LefParser::lefwLayerAntennaSideAreaFactor(value, diffUseOnly);
}

int lefwLayerAntennaCumRoutingPlusCut()
{
  return LefParser::lefwLayerAntennaCumRoutingPlusCut();
}

int lefwLayerAntennaGatePlusDiff(double plusDiffFactor)
{
  return LefParser::lefwLayerAntennaGatePlusDiff(plusDiffFactor);
}

int lefwLayerAntennaAreaMinusDiff(double minusDiffFactor)
{
  return LefParser::lefwLayerAntennaAreaMinusDiff(minusDiffFactor);
}

int lefwLayerAntennaAreaDiffReducePwl(int numPwls,
                                      double* diffAreas,
                                      double* metalDiffFactors)
{
  return LefParser::lefwLayerAntennaAreaDiffReducePwl(
      numPwls, diffAreas, metalDiffFactors);
}

int lefwMinimumDensity(double minDensity)
{
  return LefParser::lefwMinimumDensity(minDensity);
}

int lefwMaximumDensity(double maxDensity)
{
  return LefParser::lefwMaximumDensity(maxDensity);
}

int lefwDensityCheckWindow(double checkWindowLength, double checkWindowWidth)
{
  return LefParser::lefwDensityCheckWindow(checkWindowLength, checkWindowWidth);
}

int lefwDensityCheckStep(double checkStepValue)
{
  return LefParser::lefwDensityCheckStep(checkStepValue);
}

int lefwFillActiveSpacing(double fillToActiveSpacing)
{
  return LefParser::lefwFillActiveSpacing(fillToActiveSpacing);
}

int lefwMaxviastack(int value, const char* bottomLayer, const char* topLayer)
{
  return LefParser::lefwMaxviastack(value, bottomLayer, topLayer);
}

int lefwStartPropDef()
{
  return LefParser::lefwStartPropDef();
}

int lefwIntPropDef(const char* objType,
                   const char* propName,
                   double leftRange,
                   double rightRange,
                   int propValue)
{
  return LefParser::lefwIntPropDef(
      objType, propName, leftRange, rightRange, propValue);
}

int lefwRealPropDef(const char* objType,
                    const char* propName,
                    double leftRange,
                    double rightRange,
                    double propValue)
{
  return LefParser::lefwRealPropDef(
      objType, propName, leftRange, rightRange, propValue);
}

int lefwStringPropDef(const char* objType,
                      const char* propName,
                      double leftRange,
                      double rightRange,
                      const char* propValue)
{
  return LefParser::lefwStringPropDef(
      objType, propName, leftRange, rightRange, propValue);
}

int lefwEndPropDef()
{
  return LefParser::lefwEndPropDef();
}

int lefwStartVia(const char* viaName, const char* isDefault)
{
  return LefParser::lefwStartVia(viaName, isDefault);
}

int lefwViaTopofstackonly()
{
  return LefParser::lefwViaTopofstackonly();
}

int lefwViaForeign(const char* foreignName, double xl, double yl, int orient)
{
  return LefParser::lefwViaForeign(foreignName, xl, yl, orient);
}

int lefwViaForeignStr(const char* foreignName,
                      double xl,
                      double yl,
                      const char* orient)
{
  return LefParser::lefwViaForeignStr(foreignName, xl, yl, orient);
}

int lefwViaResistance(double resistance)
{
  return LefParser::lefwViaResistance(resistance);
}

int lefwViaLayer(const char* layerName)
{
  return LefParser::lefwViaLayer(layerName);
}

int lefwViaLayerRect(double x1l, double y1l, double x2l, double y2l, int mask)
{
  return LefParser::lefwViaLayerRect(x1l, y1l, x2l, y2l, mask);
}

int lefwViaLayerPolygon(int num_polys, double* xl, double* yl, int mask)
{
  return LefParser::lefwViaLayerPolygon(num_polys, xl, yl, mask);
}

int lefwViaViarule(const char* viaRuleName,
                   double xCutSize,
                   double yCutSize,
                   const char* botMetalLayer,
                   const char* cutLayer,
                   const char* topMetalLayer,
                   double xCutSpacing,
                   double yCutSpacing,
                   double xBotEnc,
                   double yBotEnc,
                   double xTopEnc,
                   double yTopEnc)
{
  return LefParser::lefwViaViarule(viaRuleName,
                                   xCutSize,
                                   yCutSize,
                                   botMetalLayer,
                                   cutLayer,
                                   topMetalLayer,
                                   xCutSpacing,
                                   yCutSpacing,
                                   xBotEnc,
                                   yBotEnc,
                                   xTopEnc,
                                   yTopEnc);
}

int lefwViaViaruleRowCol(int numCutRows, int numCutCols)
{
  return LefParser::lefwViaViaruleRowCol(numCutRows, numCutCols);
}

int lefwViaViaruleOrigin(double xOffset, double yOffset)
{
  return LefParser::lefwViaViaruleOrigin(xOffset, yOffset);
}

int lefwViaViaruleOffset(double xBotOffset,
                         double yBotOffset,
                         double xTopOffset,
                         double yTopOffset)
{
  return LefParser::lefwViaViaruleOffset(
      xBotOffset, yBotOffset, xTopOffset, yTopOffset);
}

int lefwViaViarulePattern(const char* cutPattern)
{
  return LefParser::lefwViaViarulePattern(cutPattern);
}

int lefwStringProperty(const char* propName, const char* propValue)
{
  return LefParser::lefwStringProperty(propName, propValue);
}

int lefwRealProperty(const char* propName, double propValue)
{
  return LefParser::lefwRealProperty(propName, propValue);
}

int lefwIntProperty(const char* propName, int propValue)
{
  return LefParser::lefwIntProperty(propName, propValue);
}

int lefwEndVia(const char* viaName)
{
  return LefParser::lefwEndVia(viaName);
}

int lefwStartViaRule(const char* viaRuleName)
{
  return LefParser::lefwStartViaRule(viaRuleName);
}

int lefwViaRuleLayer(const char* layerName,
                     const char* direction,
                     double minWidth,
                     double maxWidth,
                     double overhang,
                     double metalOverhang)
{
  return LefParser::lefwViaRuleLayer(
      layerName, direction, minWidth, maxWidth, overhang, metalOverhang);
}

int lefwViaRuleVia(const char* viaName)
{
  return LefParser::lefwViaRuleVia(viaName);
}

int lefwEndViaRule(const char* viaRuleName)
{
  return LefParser::lefwEndViaRule(viaRuleName);
}

int lefwStartViaRuleGen(const char* viaRuleName)
{
  return LefParser::lefwStartViaRuleGen(viaRuleName);
}

int lefwViaRuleGenDefault()
{
  return LefParser::lefwViaRuleGenDefault();
}

int lefwViaRuleGenLayer(const char* layerName,
                        const char* direction,
                        double minWidth,
                        double maxWidth,
                        double overhang,
                        double metalOverhang)
{
  return LefParser::lefwViaRuleGenLayer(
      layerName, direction, minWidth, maxWidth, overhang, metalOverhang);
}

int lefwViaRuleGenLayerEnclosure(const char* layerName,
                                 double overhang1,
                                 double overhang2,
                                 double minWidth,
                                 double maxWidth)
{
  return LefParser::lefwViaRuleGenLayerEnclosure(
      layerName, overhang1, overhang2, minWidth, maxWidth);
}

int lefwViaRuleGenLayer3(const char* layerName,
                         double xl,
                         double yl,
                         double xh,
                         double yh,
                         double xSpacing,
                         double ySpacing,
                         double resistance)
{
  return LefParser::lefwViaRuleGenLayer3(
      layerName, xl, yl, xh, yh, xSpacing, ySpacing, resistance);
}

int lefwEndViaRuleGen(const char* viaRuleName)
{
  return LefParser::lefwEndViaRuleGen(viaRuleName);
}

int lefwStartNonDefaultRule(const char* ruleName)
{
  return LefParser::lefwStartNonDefaultRule(ruleName);
}

int lefwNonDefaultRuleLayer(const char* routingLayerName,
                            double width,
                            double minSpacing,
                            double wireExtension,
                            double resistance,
                            double capacitance,
                            double edgeCap)
{
  return LefParser::lefwNonDefaultRuleLayer(routingLayerName,
                                            width,
                                            minSpacing,
                                            wireExtension,
                                            resistance,
                                            capacitance,
                                            edgeCap);
}

int lefwNonDefaultRuleHardspacing()
{
  return LefParser::lefwNonDefaultRuleHardspacing();
}

int lefwNonDefaultRuleStartVia(const char* viaName, const char* isDefault)
{
  return LefParser::lefwNonDefaultRuleStartVia(viaName, isDefault);
}

int lefwNonDefaultRuleEndVia(const char* viaName)
{
  return LefParser::lefwNonDefaultRuleEndVia(viaName);
}

int lefwNonDefaultRuleUseVia(const char* viaName)
{
  return LefParser::lefwNonDefaultRuleUseVia(viaName);
}

int lefwNonDefaultRuleUseViaRule(const char* viaRuleName)
{
  return LefParser::lefwNonDefaultRuleUseViaRule(viaRuleName);
}

int lefwNonDefaultRuleMinCuts(const char* layerName, int numCuts)
{
  return LefParser::lefwNonDefaultRuleMinCuts(layerName, numCuts);
}

int lefwEndNonDefaultRule(const char* ruleName)
{
  return LefParser::lefwEndNonDefaultRule(ruleName);
}

int lefwStartSpacing()
{
  return LefParser::lefwStartSpacing();
}

int lefwSpacing(const char* layerName1,
                const char* layerName2,
                double minSpace,
                const char* stack)
{
  return LefParser::lefwSpacing(layerName1, layerName2, minSpace, stack);
}

int lefwEndSpacing()
{
  return LefParser::lefwEndSpacing();
}

int lefwUniversalNoiseMargin(double high, double low)
{
  return LefParser::lefwUniversalNoiseMargin(high, low);
}

int lefwEdgeRateThreshold1(double num)
{
  return LefParser::lefwEdgeRateThreshold1(num);
}

int lefwEdgeRateThreshold2(double num)
{
  return LefParser::lefwEdgeRateThreshold2(num);
}

int lefwEdgeRateScaleFactor(double num)
{
  return LefParser::lefwEdgeRateScaleFactor(num);
}

int lefwStartNoiseTable(int num)
{
  return LefParser::lefwStartNoiseTable(num);
}

int lefwEdgeRate(double num)
{
  return LefParser::lefwEdgeRate(num);
}

int lefwOutputResistance(int numResists, double* resistance)
{
  return LefParser::lefwOutputResistance(numResists, resistance);
}

int lefwVictims(int length, int numNoises, double* noises)
{
  return LefParser::lefwVictims(length, numNoises, noises);
}

int lefwEndNoiseTable()
{
  return LefParser::lefwEndNoiseTable();
}

int lefwStartCorrectTable(int num)
{
  return LefParser::lefwStartCorrectTable(num);
}

int lefwEndCorrectTable()
{
  return LefParser::lefwEndCorrectTable();
}

int lefwMinFeature(double x, double y)
{
  return LefParser::lefwMinFeature(x, y);
}

int lefwStartIrdrop()
{
  return LefParser::lefwStartIrdrop();
}

int lefwIrdropTable(const char* tableName, const char* currentsNvolts)
{
  return LefParser::lefwIrdropTable(tableName, currentsNvolts);
}

int lefwEndIrdrop()
{
  return LefParser::lefwEndIrdrop();
}

int lefwSite(const char* siteName,
             const char* classType,
             const char* symmetry,
             double width,
             double height)
{
  return LefParser::lefwSite(siteName, classType, symmetry, width, height);
}

int lefwSiteRowPattern(const char* siteName, int orient)
{
  return LefParser::lefwSiteRowPattern(siteName, orient);
}

int lefwSiteRowPatternStr(const char* siteName, const char* orient)
{
  return LefParser::lefwSiteRowPatternStr(siteName, orient);
}

int lefwEndSite(const char* siteName)
{
  return LefParser::lefwEndSite(siteName);
}

int lefwStartArray(const char* arrayName)
{
  return LefParser::lefwStartArray(arrayName);
}

int lefwArraySite(const char* name,
                  double origX,
                  double origY,
                  int orient,
                  double numX,
                  double numY,
                  double spaceX,
                  double spaceY)
{
  return LefParser::lefwArraySite(
      name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwArraySiteStr(const char* name,
                     double origX,
                     double origY,
                     const char* orient,
                     double numX,
                     double numY,
                     double spaceX,
                     double spaceY)
{
  return LefParser::lefwArraySiteStr(
      name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwArrayCanplace(const char* name,
                      double origX,
                      double origY,
                      int orient,
                      double numX,
                      double numY,
                      double spaceX,
                      double spaceY)
{
  return LefParser::lefwArrayCanplace(
      name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwArrayCanplaceStr(const char* name,
                         double origX,
                         double origY,
                         const char* orient,
                         double numX,
                         double numY,
                         double spaceX,
                         double spaceY)
{
  return LefParser::lefwArrayCanplaceStr(
      name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwArrayCannotoccupy(const char* name,
                          double origX,
                          double origY,
                          int orient,
                          double numX,
                          double numY,
                          double spaceX,
                          double spaceY)
{
  return LefParser::lefwArrayCannotoccupy(
      name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwArrayCannotoccupyStr(const char* name,
                             double origX,
                             double origY,
                             const char* orient,
                             double numX,
                             double numY,
                             double spaceX,
                             double spaceY)
{
  return LefParser::lefwArrayCannotoccupyStr(
      name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwArrayTracks(const char* xy,
                    double start,
                    int numTracks,
                    double space,
                    const char* layers)
{
  return LefParser::lefwArrayTracks(xy, start, numTracks, space, layers);
}

int lefwStartArrayFloorplan(const char* name)
{
  return LefParser::lefwStartArrayFloorplan(name);
}

int lefwArrayFloorplan(const char* site,
                       const char* name,
                       double origX,
                       double origY,
                       int orient,
                       int numX,
                       int numY,
                       double spaceX,
                       double spaceY)
{
  return LefParser::lefwArrayFloorplan(
      site, name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwArrayFloorplanStr(const char* site,
                          const char* name,
                          double origX,
                          double origY,
                          const char* orient,
                          int numX,
                          int numY,
                          double spaceX,
                          double spaceY)
{
  return LefParser::lefwArrayFloorplanStr(
      site, name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwEndArrayFloorplan(const char* name)
{
  return LefParser::lefwEndArrayFloorplan(name);
}

int lefwArrayGcellgrid(const char* xy,
                       double startXY,
                       int colRows,
                       double spaceXY)
{
  return LefParser::lefwArrayGcellgrid(xy, startXY, colRows, spaceXY);
}

int lefwStartArrayDefaultCap(int size)
{
  return LefParser::lefwStartArrayDefaultCap(size);
}

int lefwArrayDefaultCap(double numPins, double cap)
{
  return LefParser::lefwArrayDefaultCap(numPins, cap);
}

int lefwEndArrayDefaultCap()
{
  return LefParser::lefwEndArrayDefaultCap();
}

int lefwEndArray(const char* arrayName)
{
  return LefParser::lefwEndArray(arrayName);
}

int lefwStartMacro(const char* macroName)
{
  return LefParser::lefwStartMacro(macroName);
}

int lefwMacroClass(const char* value1, const char* value2)
{
  return LefParser::lefwMacroClass(value1, value2);
}

int lefwMacroFixedMask()
{
  return LefParser::lefwMacroFixedMask();
}

int lefwMacroSource(const char* value1)
{
  return LefParser::lefwMacroSource(value1);
}

int lefwMacroForeign(const char* name, double xl, double yl, int orient)
{
  return LefParser::lefwMacroForeign(name, xl, yl, orient);
}

int lefwMacroForeignStr(const char* name,
                        double xl,
                        double yl,
                        const char* orient)
{
  return LefParser::lefwMacroForeignStr(name, xl, yl, orient);
}

int lefwMacroOrigin(double xl, double yl)
{
  return LefParser::lefwMacroOrigin(xl, yl);
}

int lefwMacroEEQ(const char* macroName)
{
  return LefParser::lefwMacroEEQ(macroName);
}

int lefwMacroLEQ(const char* macroName)
{
  return LefParser::lefwMacroLEQ(macroName);
}

int lefwMacroSize(double width, double height)
{
  return LefParser::lefwMacroSize(width, height);
}

int lefwMacroSymmetry(const char* symmetry)
{
  return LefParser::lefwMacroSymmetry(symmetry);
}

int lefwMacroSite(const char* siteName)
{
  return LefParser::lefwMacroSite(siteName);
}

int lefwMacroSitePattern(const char* name,
                         double origX,
                         double origY,
                         int orient,
                         int numX,
                         int numY,
                         double spaceX,
                         double spaceY)
{
  return LefParser::lefwMacroSitePattern(
      name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwMacroSitePatternStr(const char* name,
                            double origX,
                            double origY,
                            const char* orient,
                            int numX,
                            int numY,
                            double spaceX,
                            double spaceY)
{
  return LefParser::lefwMacroSitePatternStr(
      name, origX, origY, orient, numX, numY, spaceX, spaceY);
}

int lefwMacroPower(double power)
{
  return LefParser::lefwMacroPower(power);
}

int lefwEndMacro(const char* macroName)
{
  return LefParser::lefwEndMacro(macroName);
}

int lefwStartMacroDensity(const char* layerName)
{
  return LefParser::lefwStartMacroDensity(layerName);
}

int lefwMacroDensityLayerRect(double x1,
                              double y1,
                              double x2,
                              double y2,
                              double densityValue)
{
  return LefParser::lefwMacroDensityLayerRect(x1, y1, x2, y2, densityValue);
}

int lefwEndMacroDensity()
{
  return LefParser::lefwEndMacroDensity();
}

int lefwStartMacroPin(const char* pinName)
{
  return LefParser::lefwStartMacroPin(pinName);
}

int lefwMacroPinTaperRule(const char* ruleName)
{
  return LefParser::lefwMacroPinTaperRule(ruleName);
}

int lefwMacroPinForeign(const char* name, double xl, double yl, int orient)
{
  return LefParser::lefwMacroPinForeign(name, xl, yl, orient);
}

int lefwMacroPinForeignStr(const char* name,
                           double xl,
                           double yl,
                           const char* orient)
{
  return LefParser::lefwMacroPinForeignStr(name, xl, yl, orient);
}

int lefwMacroPinLEQ(const char* pinName)
{
  return LefParser::lefwMacroPinLEQ(pinName);
}

int lefwMacroPinDirection(const char* direction)
{
  return LefParser::lefwMacroPinDirection(direction);
}

int lefwMacroPinUse(const char* use)
{
  return LefParser::lefwMacroPinUse(use);
}

int lefwMacroPinShape(const char* name)
{
  return LefParser::lefwMacroPinShape(name);
}

int lefwMacroPinMustjoin(const char* name)
{
  return LefParser::lefwMacroPinMustjoin(name);
}

int lefwMacroPinNetExpr(const char* name)
{
  return LefParser::lefwMacroPinNetExpr(name);
}

int lefwMacroPinSupplySensitivity(const char* pinName)
{
  return LefParser::lefwMacroPinSupplySensitivity(pinName);
}

int lefwMacroPinGroundSensitivity(const char* pinName)
{
  return LefParser::lefwMacroPinGroundSensitivity(pinName);
}

int lefwMacroPinOutputnoisemargin(int high, int low)
{
  return LefParser::lefwMacroPinOutputnoisemargin(high, low);
}

int lefwMacroPinOutputresistance(int high, int low)
{
  return LefParser::lefwMacroPinOutputresistance(high, low);
}

int lefwMacroPinInputnoisemargin(int high, int low)
{
  return LefParser::lefwMacroPinInputnoisemargin(high, low);
}

int lefwMacroPinPower(double power)
{
  return LefParser::lefwMacroPinPower(power);
}

int lefwMacroPinLeakage(double leakage)
{
  return LefParser::lefwMacroPinLeakage(leakage);
}

int lefwMacroPinCapacitance(double capacitance)
{
  return LefParser::lefwMacroPinCapacitance(capacitance);
}

int lefwMacroPinResistance(double resistance)
{
  return LefParser::lefwMacroPinResistance(resistance);
}

int lefwMacroPinPulldownres(double resistance)
{
  return LefParser::lefwMacroPinPulldownres(resistance);
}

int lefwMacroPinTieoffr(double resistance)
{
  return LefParser::lefwMacroPinTieoffr(resistance);
}

int lefwMacroPinVHI(double voltage)
{
  return LefParser::lefwMacroPinVHI(voltage);
}

int lefwMacroPinVLO(double voltage)
{
  return LefParser::lefwMacroPinVLO(voltage);
}

int lefwMacroPinRisevoltagethreshold(double voltage)
{
  return LefParser::lefwMacroPinRisevoltagethreshold(voltage);
}

int lefwMacroPinFallvoltagethreshold(double voltage)
{
  return LefParser::lefwMacroPinFallvoltagethreshold(voltage);
}

int lefwMacroPinRisethresh(double capacitance)
{
  return LefParser::lefwMacroPinRisethresh(capacitance);
}

int lefwMacroPinFallthresh(double capacitance)
{
  return LefParser::lefwMacroPinFallthresh(capacitance);
}

int lefwMacroPinRisesatcur(double current)
{
  return LefParser::lefwMacroPinRisesatcur(current);
}

int lefwMacroPinFallsatcur(double current)
{
  return LefParser::lefwMacroPinFallsatcur(current);
}

int lefwMacroPinCurrentsource(const char* name)
{
  return LefParser::lefwMacroPinCurrentsource(name);
}

int lefwMacroPinIV_Tables(const char* lowName, const char* highName)
{
  return LefParser::lefwMacroPinIV_Tables(lowName, highName);
}

int lefwMacroPinAntennasize(double value, const char* layerName)
{
  return LefParser::lefwMacroPinAntennasize(value, layerName);
}

int lefwMacroPinAntennaMetalArea(double value, const char* layerName)
{
  return LefParser::lefwMacroPinAntennaMetalArea(value, layerName);
}

int lefwMacroPinAntennaMetalLength(double value, const char* layerName)
{
  return LefParser::lefwMacroPinAntennaMetalLength(value, layerName);
}

int lefwMacroPinAntennaPartialMetalArea(double value, const char* layerName)
{
  return LefParser::lefwMacroPinAntennaPartialMetalArea(value, layerName);
}

int lefwMacroPinAntennaPartialMetalSideArea(double value, const char* layerName)
{
  return LefParser::lefwMacroPinAntennaPartialMetalSideArea(value, layerName);
}

int lefwMacroPinAntennaPartialCutArea(double value, const char* layerName)
{
  return LefParser::lefwMacroPinAntennaPartialCutArea(value, layerName);
}

int lefwMacroPinAntennaDiffArea(double value, const char* layerName)
{
  return LefParser::lefwMacroPinAntennaDiffArea(value, layerName);
}

int lefwMacroPinAntennaModel(const char* oxide)
{
  return LefParser::lefwMacroPinAntennaModel(oxide);
}

int lefwMacroPinAntennaGateArea(double value, const char* layerName)
{
  return LefParser::lefwMacroPinAntennaGateArea(value, layerName);
}

int lefwMacroPinAntennaMaxAreaCar(double value, const char* layerName)
{
  return LefParser::lefwMacroPinAntennaMaxAreaCar(value, layerName);
}

int lefwMacroPinAntennaMaxSideAreaCar(double value, const char* layerName)
{
  return LefParser::lefwMacroPinAntennaMaxSideAreaCar(value, layerName);
}

int lefwMacroPinAntennaMaxCutCar(double value, const char* layerName)
{
  return LefParser::lefwMacroPinAntennaMaxCutCar(value, layerName);
}

int lefwEndMacroPin(const char* pinName)
{
  return LefParser::lefwEndMacroPin(pinName);
}

int lefwStartMacroPinPort(const char* classType)
{
  return LefParser::lefwStartMacroPinPort(classType);
}

int lefwMacroPinPortLayer(const char* layerName, double spacing)
{
  return LefParser::lefwMacroPinPortLayer(layerName, spacing);
}

int lefwMacroPinPortDesignRuleWidth(const char* layerName, double width)
{
  return LefParser::lefwMacroPinPortDesignRuleWidth(layerName, width);
}

int lefwMacroPinPortLayerWidth(double width)
{
  return LefParser::lefwMacroPinPortLayerWidth(width);
}

int lefwMacroPinPortLayerPath(int num_paths,
                              double* xl,
                              double* yl,
                              int numX,
                              int numY,
                              double spaceX,
                              double spaceY,
                              int mask)
{
  return LefParser::lefwMacroPinPortLayerPath(
      num_paths, xl, yl, numX, numY, spaceX, spaceY, mask);
}

int lefwMacroPinPortLayerRect(double xl1,
                              double yl1,
                              double xl2,
                              double yl2,
                              int numX,
                              int numY,
                              double spaceX,
                              double spaceY,
                              int mask)
{
  return LefParser::lefwMacroPinPortLayerRect(
      xl1, yl1, xl2, yl2, numX, numY, spaceX, spaceY, mask);
}

int lefwMacroPinPortLayerPolygon(int num_polys,
                                 double* xl,
                                 double* yl,
                                 int numX,
                                 int numY,
                                 double spaceX,
                                 double spaceY,
                                 int mask)
{
  return LefParser::lefwMacroPinPortLayerPolygon(
      num_polys, xl, yl, numX, numY, spaceX, spaceY, mask);
}

int lefwMacroPinPortVia(double xl,
                        double yl,
                        const char* viaName,
                        int numX,
                        int numY,
                        double spaceX,
                        double spaceY,
                        int mask)
{
  return LefParser::lefwMacroPinPortVia(
      xl, yl, viaName, numX, numY, spaceX, spaceY, mask);
}

int lefwEndMacroPinPort()
{
  return LefParser::lefwEndMacroPinPort();
}

int lefwStartMacroObs()
{
  return LefParser::lefwStartMacroObs();
}

int lefwMacroObsLayer(const char* layerName, double spacing)
{
  return LefParser::lefwMacroObsLayer(layerName, spacing);
}

int lefwMacroObsDesignRuleWidth(const char* layerName, double width)
{
  return LefParser::lefwMacroObsDesignRuleWidth(layerName, width);
}

int lefwMacroExceptPGNet(const char* layerName)
{
  return LefParser::lefwMacroExceptPGNet(layerName);
}

int lefwMacroObsLayerWidth(double width)
{
  return LefParser::lefwMacroObsLayerWidth(width);
}

int lefwMacroObsLayerPath(int num_paths,
                          double* xl,
                          double* yl,
                          int numX,
                          int numY,
                          double spaceX,
                          double spaceY,
                          int mask)
{
  return LefParser::lefwMacroObsLayerPath(
      num_paths, xl, yl, numX, numY, spaceX, spaceY, mask);
}

int lefwMacroObsLayerRect(double xl1,
                          double yl1,
                          double xl2,
                          double yl2,
                          int numX,
                          int numY,
                          double spaceX,
                          double spaceY,
                          int mask)
{
  return LefParser::lefwMacroObsLayerRect(
      xl1, yl1, xl2, yl2, numX, numY, spaceX, spaceY, mask);
}

int lefwMacroObsLayerPolygon(int num_polys,
                             double* xl,
                             double* yl,
                             int numX,
                             int numY,
                             double spaceX,
                             double spaceY,
                             int mask)
{
  return LefParser::lefwMacroObsLayerPolygon(
      num_polys, xl, yl, numX, numY, spaceX, spaceY, mask);
}

int lefwMacroObsVia(double xl,
                    double yl,
                    const char* viaName,
                    int numX,
                    int numY,
                    double spaceX,
                    double spaceY,
                    int mask)
{
  return LefParser::lefwMacroObsVia(
      xl, yl, viaName, numX, numY, spaceX, spaceY, mask);
}

int lefwEndMacroObs()
{
  return LefParser::lefwEndMacroObs();
}

int lefwStartMacroTiming()
{
  return LefParser::lefwStartMacroTiming();
}

int lefwMacroTimingPin(const char* fromPin, const char* toPin)
{
  return LefParser::lefwMacroTimingPin(fromPin, toPin);
}

int lefwMacroTimingIntrinsic(const char* riseFall,
                             double min,
                             double max,
                             double slewT1,
                             double slewT1Min,
                             double slewT1Max,
                             double slewT2,
                             double slewT2Min,
                             double slewT2Max,
                             double slewT3,
                             double varMin,
                             double varMax)
{
  return LefParser::lefwMacroTimingIntrinsic(riseFall,
                                             min,
                                             max,
                                             slewT1,
                                             slewT1Min,
                                             slewT1Max,
                                             slewT2,
                                             slewT2Min,
                                             slewT2Max,
                                             slewT3,
                                             varMin,
                                             varMax);
}

int lefwMacroTimingRisers(double min, double max)
{
  return LefParser::lefwMacroTimingRisers(min, max);
}

int lefwMacroTimingFallrs(double min, double max)
{
  return LefParser::lefwMacroTimingFallrs(min, max);
}

int lefwMacroTimingRisecs(double min, double max)
{
  return LefParser::lefwMacroTimingRisecs(min, max);
}

int lefwMacroTimingFallcs(double min, double max)
{
  return LefParser::lefwMacroTimingFallcs(min, max);
}

int lefwMacroTimingRisesatt1(double min, double max)
{
  return LefParser::lefwMacroTimingRisesatt1(min, max);
}

int lefwMacroTimingFallsatt1(double min, double max)
{
  return LefParser::lefwMacroTimingFallsatt1(min, max);
}

int lefwMacroTimingRiset0(double min, double max)
{
  return LefParser::lefwMacroTimingRiset0(min, max);
}

int lefwMacroTimingFallt0(double min, double max)
{
  return LefParser::lefwMacroTimingFallt0(min, max);
}

int lefwMacroTimingUnateness(const char* unateness)
{
  return LefParser::lefwMacroTimingUnateness(unateness);
}

int lefwEndMacroTiming()
{
  return LefParser::lefwEndMacroTiming();
}

int lefwAntenna(const char* type, double value)
{
  return LefParser::lefwAntenna(type, value);
}

int lefwStartBeginext(const char* name)
{
  return LefParser::lefwStartBeginext(name);
}

int lefwBeginextCreator(const char* creatorName)
{
  return LefParser::lefwBeginextCreator(creatorName);
}

int lefwBeginextDate()
{
  return LefParser::lefwBeginextDate();
}

int lefwBeginextRevision(int vers1, int vers2)
{
  return LefParser::lefwBeginextRevision(vers1, vers2);
}

int lefwBeginextSyntax(const char* title, const char* string)
{
  return LefParser::lefwBeginextSyntax(title, string);
}

int lefwEndBeginext()
{
  return LefParser::lefwEndBeginext();
}

int lefwCurrentLineNumber()
{
  return LefParser::lefwCurrentLineNumber();
}

int lefwEnd()
{
  return LefParser::lefwEnd();
}

void lefwPrintError(int status)
{
  LefParser::lefwPrintError(status);
}

void lefwAddComment(const char* comment)
{
  LefParser::lefwAddComment(comment);
}

void lefwAddIndent()
{
  LefParser::lefwAddIndent();
}
