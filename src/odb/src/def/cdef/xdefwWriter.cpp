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

#include "defwWriter.h"
#include "defwWriter.hpp"

// Wrappers definitions.
int defwNewLine () {
    return LefDefParser::defwNewLine();
}

int defwInit (FILE*  f, int  vers1, int  version2, const char*  caseSensitive, const char*  dividerChar, const char*  busBitChars, const char*  designName, const char*  technology, const char*  array, const char*  floorplan, double  units) {
    return LefDefParser::defwInit(f, vers1, version2, caseSensitive, dividerChar, busBitChars, designName, technology, array, floorplan, units);
}

int defwInitCbk (FILE*  f) {
    return LefDefParser::defwInitCbk(f);
}

int defwVersion (int  vers1, int  vers2) {
    return LefDefParser::defwVersion(vers1, vers2);
}

int defwCaseSensitive (const char*  caseSensitive) {
    return LefDefParser::defwCaseSensitive(caseSensitive);
}

int defwBusBitChars (const char*  busBitChars) {
    return LefDefParser::defwBusBitChars(busBitChars);
}

int defwDividerChar (const char*  dividerChar) {
    return LefDefParser::defwDividerChar(dividerChar);
}

int defwDesignName (const char*  name) {
    return LefDefParser::defwDesignName(name);
}

int defwTechnology (const char*  technology) {
    return LefDefParser::defwTechnology(technology);
}

int defwArray (const char*  array) {
    return LefDefParser::defwArray(array);
}

int defwFloorplan (const char*  floorplan) {
    return LefDefParser::defwFloorplan(floorplan);
}

int defwUnits (int  units) {
    return LefDefParser::defwUnits(units);
}

int defwHistory (const char*  string) {
    return LefDefParser::defwHistory(string);
}

int defwStartPropDef () {
    return LefDefParser::defwStartPropDef();
}

int defwIntPropDef (const char*  objType, const char*  propName, double  leftRange, double  rightRange, int     propValue) {
    return LefDefParser::defwIntPropDef(objType, propName, leftRange, rightRange, propValue);
}

int defwRealPropDef (const char*  objType, const char*  propName, double  leftRange, double  rightRange, double  propValue) {
    return LefDefParser::defwRealPropDef(objType, propName, leftRange, rightRange, propValue);
}

int defwStringPropDef (const char*  objType, const char*  propName, double  leftRange, double  rightRange, const char*  propValue) {
    return LefDefParser::defwStringPropDef(objType, propName, leftRange, rightRange, propValue);
}

int defwEndPropDef () {
    return LefDefParser::defwEndPropDef();
}

int defwStringProperty (const char*  propName, const char*  propValue) {
    return LefDefParser::defwStringProperty(propName, propValue);
}

int defwRealProperty (const char*  propName, double  propValue) {
    return LefDefParser::defwRealProperty(propName, propValue);
}

int defwIntProperty (const char*  propName, int  propValue) {
    return LefDefParser::defwIntProperty(propName, propValue);
}

int defwDieArea (int  xl, int  yl, int  xh, int  yh) {
    return LefDefParser::defwDieArea(xl, yl, xh, yh);
}

int defwDieAreaList (int  num_points, int*  xl, int*  yh) {
    return LefDefParser::defwDieAreaList(num_points, xl, yh);
}

int defwRow (const char*  rowName, const char*  rowType, int  x_orig, int  y_orig, int  orient, int  do_count, int  do_increment, int  xstep, int  ystep) {
    return LefDefParser::defwRow(rowName, rowType, x_orig, y_orig, orient, do_count, do_increment, xstep, ystep);
}

int defwRowStr (const char*  rowName, const char*  rowType, int  x_orig, int  y_orig, const char*  orient, int  do_count, int  do_increment, int  xstep, int  ystep) {
    return LefDefParser::defwRowStr(rowName, rowType, x_orig, y_orig, orient, do_count, do_increment, xstep, ystep);
}

int defwTracks (const char*  master, int  doStart, int  doCount, int  doStep, int  numLayers, const char**  layers, int  mask, int  sameMask) {
    return LefDefParser::defwTracks(master, doStart, doCount, doStep, numLayers, layers, mask, sameMask);
}

int defwGcellGrid (const char*  master, int  doStart, int  doCount, int  doStep) {
    return LefDefParser::defwGcellGrid(master, doStart, doCount, doStep);
}

int defwStartDefaultCap (int  count) {
    return LefDefParser::defwStartDefaultCap(count);
}

int defwDefaultCap (int  pins, double  cap) {
    return LefDefParser::defwDefaultCap(pins, cap);
}

int defwEndDefaultCap () {
    return LefDefParser::defwEndDefaultCap();
}

int defwCanPlace (const char*  master, int  xOrig, int  yOrig, int  orient, int  doCnt, int  doInc, int  xStep, int  yStep) {
    return LefDefParser::defwCanPlace(master, xOrig, yOrig, orient, doCnt, doInc, xStep, yStep);
}

int defwCanPlaceStr (const char*  master, int  xOrig, int  yOrig, const char*  orient, int  doCnt, int  doInc, int  xStep, int  yStep) {
    return LefDefParser::defwCanPlaceStr(master, xOrig, yOrig, orient, doCnt, doInc, xStep, yStep);
}

int defwCannotOccupy (const char*  master, int  xOrig, int  yOrig, int  orient, int  doCnt, int  doInc, int  xStep, int  yStep) {
    return LefDefParser::defwCannotOccupy(master, xOrig, yOrig, orient, doCnt, doInc, xStep, yStep);
}

int defwCannotOccupyStr (const char*  master, int  xOrig, int  yOrig, const char*  orient, int  doCnt, int  doInc, int  xStep, int  yStep) {
    return LefDefParser::defwCannotOccupyStr(master, xOrig, yOrig, orient, doCnt, doInc, xStep, yStep);
}

int defwStartVias (int  count) {
    return LefDefParser::defwStartVias(count);
}

int defwViaName (const char*  name) {
    return LefDefParser::defwViaName(name);
}

int defwViaPattern (const char*  patternName) {
    return LefDefParser::defwViaPattern(patternName);
}

int defwViaRect (const char*  layerName, int  xl, int  yl, int  xh, int  yh, int  mask) {
    return LefDefParser::defwViaRect(layerName, xl, yl, xh, yh, mask);
}

int defwViaPolygon (const char*  layerName, int  num_polys, double*  xl, double*  yl, int  mask) {
    return LefDefParser::defwViaPolygon(layerName, num_polys, xl, yl, mask);
}

int defwViaViarule (const char*  viaRuleName, double  xCutSize, double  yCutSize, const char*  botMetalLayer, const char*  cutLayer, const char*  topMetalLayer, double  xCutSpacing, double  yCutSpacing, double  xBotEnc, double  yBotEnc, double  xTopEnc, double  yTopEnc) {
    return LefDefParser::defwViaViarule(viaRuleName, xCutSize, yCutSize, botMetalLayer, cutLayer, topMetalLayer, xCutSpacing, yCutSpacing, xBotEnc, yBotEnc, xTopEnc, yTopEnc);
}

int defwViaViaruleRowCol (int  numCutRows, int  numCutCols) {
    return LefDefParser::defwViaViaruleRowCol(numCutRows, numCutCols);
}

int defwViaViaruleOrigin (int  xOffset, int  yOffset) {
    return LefDefParser::defwViaViaruleOrigin(xOffset, yOffset);
}

int defwViaViaruleOffset (int  xBotOffset, int  yBotOffset, int  xTopOffset, int  yTopOffset) {
    return LefDefParser::defwViaViaruleOffset(xBotOffset, yBotOffset, xTopOffset, yTopOffset);
}

int defwViaViarulePattern (const char*  cutPattern) {
    return LefDefParser::defwViaViarulePattern(cutPattern);
}

int defwOneViaEnd () {
    return LefDefParser::defwOneViaEnd();
}

int defwEndVias () {
    return LefDefParser::defwEndVias();
}

int defwStartRegions (int  count) {
    return LefDefParser::defwStartRegions(count);
}

int defwRegionName (const char*  name) {
    return LefDefParser::defwRegionName(name);
}

int defwRegionPoints (int  xl, int  yl, int  xh, int  yh) {
    return LefDefParser::defwRegionPoints(xl, yl, xh, yh);
}

int defwRegionType (const char*  type) {
    return LefDefParser::defwRegionType(type);
}

int defwEndRegions () {
    return LefDefParser::defwEndRegions();
}

int defwComponentMaskShiftLayers (const char**  layerNames, int           numLayerName) {
    return LefDefParser::defwComponentMaskShiftLayers(layerNames, numLayerName);
}

int defwStartComponents (int  count) {
    return LefDefParser::defwStartComponents(count);
}

int defwComponent (const char*  instance, const char*  master, int    numNetName, const char**  netNames, const char*  eeq, const char*  genName, const char*  genParemeters, const char*  source, int  numForeign, const char**  foreigns, int*  foreignX, int*  foreignY, int*  foreignOrients, const char*  status, int  statusX, int  statusY, int  statusOrient, double  weight, const char*  region, int  xl, int  yl, int  xh, int  yh) {
    return LefDefParser::defwComponent(instance, master, numNetName, netNames, eeq, genName, genParemeters, source, numForeign, foreigns, foreignX, foreignY, foreignOrients, status, statusX, statusY, statusOrient, weight, region, xl, yl, xh, yh);
}

int defwComponentStr (const char*  instance, const char*  master, int    numNetName, const char**  netNames, const char*  eeq, const char*  genName, const char*  genParemeters, const char*  source, int  numForeign, const char**  foreigns, int*  foreignX, int*  foreignY, const char**  foreignOrients, const char*  status, int  statusX, int  statusY, const char*  statusOrient, double  weight, const char*  region, int  xl, int  yl, int  xh, int  yh) {
    return LefDefParser::defwComponentStr(instance, master, numNetName, netNames, eeq, genName, genParemeters, source, numForeign, foreigns, foreignX, foreignY, foreignOrients, status, statusX, statusY, statusOrient, weight, region, xl, yl, xh, yh);
}

int defwComponentMaskShift (int  shiftLayerMasks) {
    return LefDefParser::defwComponentMaskShift(shiftLayerMasks);
}

int defwComponentHalo (int  left, int  bottom, int  right, int  top) {
    return LefDefParser::defwComponentHalo(left, bottom, right, top);
}

int defwComponentHaloSoft (int  left, int  bottom, int  right, int  top) {
    return LefDefParser::defwComponentHaloSoft(left, bottom, right, top);
}

int defwComponentRouteHalo (int  haloDist, const char*  minLayer, const char*  maxLayer) {
    return LefDefParser::defwComponentRouteHalo(haloDist, minLayer, maxLayer);
}

int defwEndComponents () {
    return LefDefParser::defwEndComponents();
}

int defwStartPins (int  count) {
    return LefDefParser::defwStartPins(count);
}

int defwPin (const char*  name, const char*  net, int  special, const char*  direction, const char*  use, const char*  status, int  statusX, int  statusY, int  orient, const char*  layer, int  xl, int  yl, int  xh, int  yh) {
    return LefDefParser::defwPin(name, net, special, direction, use, status, statusX, statusY, orient, layer, xl, yl, xh, yh);
}

int defwPinStr (const char*  name, const char*  net, int  special, const char*  direction, const char*  use, const char*  status, int  statusX, int  statusY, const char*  orient, const char*  layer, int  xl, int  yl, int  xh, int  yh) {
    return LefDefParser::defwPinStr(name, net, special, direction, use, status, statusX, statusY, orient, layer, xl, yl, xh, yh);
}

int defwPinLayer (const char*  layerName, int  spacing, int  designRuleWidth, int  xl, int  yl, int  xh, int  yh, int  mask) {
    return LefDefParser::defwPinLayer(layerName, spacing, designRuleWidth, xl, yl, xh, yh, mask);
}

int defwPinPolygon (const char*  layerName, int  spacing, int  designRuleWidth, int  num_polys, double*  xl, double*  yl, int  mask) {
    return LefDefParser::defwPinPolygon(layerName, spacing, designRuleWidth, num_polys, xl, yl, mask);
}

int defwPinVia (const char*  viaName, int  xl, int  yl, int  mask) {
    return LefDefParser::defwPinVia(viaName, xl, yl, mask);
}

int defwPinPort () {
    return LefDefParser::defwPinPort();
}

int defwPinPortLayer (const char*  layerName, int  spacing, int  designRuleWidth, int  xl, int  yl, int  xh, int  yh, int  mask) {
    return LefDefParser::defwPinPortLayer(layerName, spacing, designRuleWidth, xl, yl, xh, yh, mask);
}

int defwPinPortPolygon (const char*  layerName, int  spacing, int  designRuleWidth, int  num_polys, double*  xl, double*  yl, int  mask) {
    return LefDefParser::defwPinPortPolygon(layerName, spacing, designRuleWidth, num_polys, xl, yl, mask);
}

int defwPinPortVia (const char*  viaName, int  xl, int  yl, int  mask) {
    return LefDefParser::defwPinPortVia(viaName, xl, yl, mask);
}

int defwPinPortLocation (const char*  status, int  statusX, int  statusY, const char*  orient) {
    return LefDefParser::defwPinPortLocation(status, statusX, statusY, orient);
}

int defwPinNetExpr (const char*  pinExpr) {
    return LefDefParser::defwPinNetExpr(pinExpr);
}

int defwPinSupplySensitivity (const char*  pinName) {
    return LefDefParser::defwPinSupplySensitivity(pinName);
}

int defwPinGroundSensitivity (const char*  pinName) {
    return LefDefParser::defwPinGroundSensitivity(pinName);
}

int defwPinAntennaPinPartialMetalArea (int  value, const char*  layerName) {
    return LefDefParser::defwPinAntennaPinPartialMetalArea(value, layerName);
}

int defwPinAntennaPinPartialMetalSideArea (int  value, const char*  layerName) {
    return LefDefParser::defwPinAntennaPinPartialMetalSideArea(value, layerName);
}

int defwPinAntennaPinPartialCutArea (int  value, const char*  layerName) {
    return LefDefParser::defwPinAntennaPinPartialCutArea(value, layerName);
}

int defwPinAntennaPinDiffArea (int  value, const char*  layerName) {
    return LefDefParser::defwPinAntennaPinDiffArea(value, layerName);
}

int defwPinAntennaModel (const char*  oxide) {
    return LefDefParser::defwPinAntennaModel(oxide);
}

int defwPinAntennaPinGateArea (int  value, const char*  layerName) {
    return LefDefParser::defwPinAntennaPinGateArea(value, layerName);
}

int defwPinAntennaPinMaxAreaCar (int  value, const char*  layerName) {
    return LefDefParser::defwPinAntennaPinMaxAreaCar(value, layerName);
}

int defwPinAntennaPinMaxSideAreaCar (int  value, const char*  layerName) {
    return LefDefParser::defwPinAntennaPinMaxSideAreaCar(value, layerName);
}

int defwPinAntennaPinMaxCutCar (int  value, const char*  layerName) {
    return LefDefParser::defwPinAntennaPinMaxCutCar(value, layerName);
}

int defwEndPins () {
    return LefDefParser::defwEndPins();
}

int defwStartPinProperties (int  count) {
    return LefDefParser::defwStartPinProperties(count);
}

int defwPinProperty (const char*  name, const char*  pinName) {
    return LefDefParser::defwPinProperty(name, pinName);
}

int defwEndPinProperties () {
    return LefDefParser::defwEndPinProperties();
}

int defwStartSpecialNets (int  count) {
    return LefDefParser::defwStartSpecialNets(count);
}

int defwSpecialNet (const char*  name) {
    return LefDefParser::defwSpecialNet(name);
}

int defwSpecialNetConnection (const char*  inst, const char*  pin, int  synthesized) {
    return LefDefParser::defwSpecialNetConnection(inst, pin, synthesized);
}

int defwSpecialNetFixedbump () {
    return LefDefParser::defwSpecialNetFixedbump();
}

int defwSpecialNetVoltage (double  v) {
    return LefDefParser::defwSpecialNetVoltage(v);
}

int defwSpecialNetSpacing (const char*  layer, int  spacing, double  minwidth, double  maxwidth) {
    return LefDefParser::defwSpecialNetSpacing(layer, spacing, minwidth, maxwidth);
}

int defwSpecialNetWidth (const char*  layer, int  width) {
    return LefDefParser::defwSpecialNetWidth(layer, width);
}

int defwSpecialNetSource (const char*  name) {
    return LefDefParser::defwSpecialNetSource(name);
}

int defwSpecialNetOriginal (const char*  name) {
    return LefDefParser::defwSpecialNetOriginal(name);
}

int defwSpecialNetPattern (const char*  name) {
    return LefDefParser::defwSpecialNetPattern(name);
}

int defwSpecialNetUse (const char*  name) {
    return LefDefParser::defwSpecialNetUse(name);
}

int defwSpecialNetWeight (double  value) {
    return LefDefParser::defwSpecialNetWeight(value);
}

int defwSpecialNetEstCap (double  value) {
    return LefDefParser::defwSpecialNetEstCap(value);
}

int defwSpecialNetPathStart (const char*  typ) {
    return LefDefParser::defwSpecialNetPathStart(typ);
}

int defwSpecialNetShieldNetName (const char*  name) {
    return LefDefParser::defwSpecialNetShieldNetName(name);
}

int defwSpecialNetPathLayer (const char*  name) {
    return LefDefParser::defwSpecialNetPathLayer(name);
}

int defwSpecialNetPathWidth (int  width) {
    return LefDefParser::defwSpecialNetPathWidth(width);
}

int defwSpecialNetPathStyle (int  styleNum) {
    return LefDefParser::defwSpecialNetPathStyle(styleNum);
}

int defwSpecialNetPathShape (const char*  shapeType) {
    return LefDefParser::defwSpecialNetPathShape(shapeType);
}

int defwSpecialNetPathMask (int  colorMask) {
    return LefDefParser::defwSpecialNetPathMask(colorMask);
}

int defwSpecialNetPathPoint (int  numPts, double*  pointx, double*  pointy) {
    return LefDefParser::defwSpecialNetPathPoint(numPts, pointx, pointy);
}

int defwSpecialNetPathVia (const char*  name) {
    return LefDefParser::defwSpecialNetPathVia(name);
}

int defwSpecialNetPathViaData (int  numX, int  numY, int  stepX, int  stepY) {
    return LefDefParser::defwSpecialNetPathViaData(numX, numY, stepX, stepY);
}

int defwSpecialNetPathPointWithWireExt (int  numPts, double*  pointx, double*  pointy, double*  optValue) {
    return LefDefParser::defwSpecialNetPathPointWithWireExt(numPts, pointx, pointy, optValue);
}

int defwSpecialNetPathEnd () {
    return LefDefParser::defwSpecialNetPathEnd();
}

int defwSpecialNetPolygon (const char*  layerName, int  num_polys, double*  xl, double*  yl) {
    return LefDefParser::defwSpecialNetPolygon(layerName, num_polys, xl, yl);
}

int defwSpecialNetRect (const char*  layerName, int  xl, int  yl, int  xh, int  yh) {
    return LefDefParser::defwSpecialNetRect(layerName, xl, yl, xh, yh);
}

int defwSpecialNetVia (const char*  layerName) {
    return LefDefParser::defwSpecialNetVia(layerName);
}

int defwSpecialNetViaWithOrient (const char*  layerName, int  orient) {
    return LefDefParser::defwSpecialNetViaWithOrient(layerName, orient);
}

int defwSpecialNetViaPoints (int  num_points, double*  xl, double*  yl) {
    return LefDefParser::defwSpecialNetViaPoints(num_points, xl, yl);
}

int defwSpecialNetEndOneNet () {
    return LefDefParser::defwSpecialNetEndOneNet();
}

int defwSpecialNetShieldStart (const char*  name) {
    return LefDefParser::defwSpecialNetShieldStart(name);
}

int defwSpecialNetShieldLayer (const char*  name) {
    return LefDefParser::defwSpecialNetShieldLayer(name);
}

int defwSpecialNetShieldWidth (int  width) {
    return LefDefParser::defwSpecialNetShieldWidth(width);
}

int defwSpecialNetShieldShape (const char*  shapeType) {
    return LefDefParser::defwSpecialNetShieldShape(shapeType);
}

int defwSpecialNetShieldPoint (int  numPts, double*  pointx, double*  pointy) {
    return LefDefParser::defwSpecialNetShieldPoint(numPts, pointx, pointy);
}

int defwSpecialNetShieldVia (const char*  name) {
    return LefDefParser::defwSpecialNetShieldVia(name);
}

int defwSpecialNetShieldViaData (int  numX, int  numY, int  stepX, int  stepY) {
    return LefDefParser::defwSpecialNetShieldViaData(numX, numY, stepX, stepY);
}

int defwSpecialNetShieldEnd () {
    return LefDefParser::defwSpecialNetShieldEnd();
}

int defwEndSpecialNets () {
    return LefDefParser::defwEndSpecialNets();
}

int defwStartNets (int  count) {
    return LefDefParser::defwStartNets(count);
}

int defwNet (const char*  name) {
    return LefDefParser::defwNet(name);
}

int defwNetConnection (const char*  inst, const char*  pin, int  synthesized) {
    return LefDefParser::defwNetConnection(inst, pin, synthesized);
}

int defwNetMustjoinConnection (const char*  inst, const char*  pin) {
    return LefDefParser::defwNetMustjoinConnection(inst, pin);
}

int defwNetVpin (const char*  vpinName, const char*  layerName, int  layerXl, int  layerYl, int  layerXh, int  layerYh, const char*  status, int  statusX, int  statusY, int  orient) {
    return LefDefParser::defwNetVpin(vpinName, layerName, layerXl, layerYl, layerXh, layerYh, status, statusX, statusY, orient);
}

int defwNetVpinStr (const char*  vpinName, const char*  layerName, int  layerXl, int  layerYl, int  layerXh, int  layerYh, const char*  status, int  statusX, int  statusY, const char*  orient) {
    return LefDefParser::defwNetVpinStr(vpinName, layerName, layerXl, layerYl, layerXh, layerYh, status, statusX, statusY, orient);
}

int defwNetNondefaultRule (const char*  name) {
    return LefDefParser::defwNetNondefaultRule(name);
}

int defwNetXtalk (int  xtalk) {
    return LefDefParser::defwNetXtalk(xtalk);
}

int defwNetFixedbump () {
    return LefDefParser::defwNetFixedbump();
}

int defwNetFrequency (double  frequency) {
    return LefDefParser::defwNetFrequency(frequency);
}

int defwNetSource (const char*  name) {
    return LefDefParser::defwNetSource(name);
}

int defwNetOriginal (const char*  name) {
    return LefDefParser::defwNetOriginal(name);
}

int defwNetUse (const char*  name) {
    return LefDefParser::defwNetUse(name);
}

int defwNetPattern (const char*  name) {
    return LefDefParser::defwNetPattern(name);
}

int defwNetEstCap (double  value) {
    return LefDefParser::defwNetEstCap(value);
}

int defwNetWeight (double  value) {
    return LefDefParser::defwNetWeight(value);
}

int defwNetShieldnet (const char*  name) {
    return LefDefParser::defwNetShieldnet(name);
}

int defwNetNoshieldStart (const char*  name) {
    return LefDefParser::defwNetNoshieldStart(name);
}

int defwNetNoshieldPoint (int  numPts, const char**  pointx, const char**  pointy) {
    return LefDefParser::defwNetNoshieldPoint(numPts, pointx, pointy);
}

int defwNetNoshieldVia (const char*  name) {
    return LefDefParser::defwNetNoshieldVia(name);
}

int defwNetNoshieldEnd () {
    return LefDefParser::defwNetNoshieldEnd();
}

int defwNetSubnetStart (const char*  name) {
    return LefDefParser::defwNetSubnetStart(name);
}

int defwNetSubnetPin (const char*  compName, const char*  pinName) {
    return LefDefParser::defwNetSubnetPin(compName, pinName);
}

int defwNetSubnetEnd () {
    return LefDefParser::defwNetSubnetEnd();
}

int defwNetPathStart (const char*  typ) {
    return LefDefParser::defwNetPathStart(typ);
}

int defwNetPathWidth (int  w) {
    return LefDefParser::defwNetPathWidth(w);
}

int defwNetPathLayer (const char*  name, int  isTaper, const char*  rulename) {
    return LefDefParser::defwNetPathLayer(name, isTaper, rulename);
}

int defwNetPathStyle (int  styleNum) {
    return LefDefParser::defwNetPathStyle(styleNum);
}

int defwNetPathMask (int  maskNum) {
    return LefDefParser::defwNetPathMask(maskNum);
}

int defwNetPathRect (int  deltaX1, int  deltaY1, int  deltaX2, int  deltaY2) {
    return LefDefParser::defwNetPathRect(deltaX1, deltaY1, deltaX2, deltaY2);
}

int defwNetPathVirtual (int  x, int  y) {
    return LefDefParser::defwNetPathVirtual(x, y);
}

int defwNetPathPoint (int  numPts, double*  pointx, double*  pointy) {
    return LefDefParser::defwNetPathPoint(numPts, pointx, pointy);
}

int defwNetPathPointWithExt (int  numPts, double*  pointx, double*  pointy, double*  optValue) {
    return LefDefParser::defwNetPathPointWithExt(numPts, pointx, pointy, optValue);
}

int defwNetPathVia (const char*  name) {
    return LefDefParser::defwNetPathVia(name);
}

int defwNetPathViaWithOrient (const char*  name, int  orient) {
    return LefDefParser::defwNetPathViaWithOrient(name, orient);
}

int defwNetPathViaWithOrientStr (const char*  name, const char*  orient) {
    return LefDefParser::defwNetPathViaWithOrientStr(name, orient);
}

int defwNetPathEnd () {
    return LefDefParser::defwNetPathEnd();
}

int defwNetEndOneNet () {
    return LefDefParser::defwNetEndOneNet();
}

int defwEndNets () {
    return LefDefParser::defwEndNets();
}

int defwStartIOTimings (int  count) {
    return LefDefParser::defwStartIOTimings(count);
}

int defwIOTiming (const char*  inst, const char*  pin) {
    return LefDefParser::defwIOTiming(inst, pin);
}

int defwIOTimingVariable (const char*  riseFall, int  num1, int  num2) {
    return LefDefParser::defwIOTimingVariable(riseFall, num1, num2);
}

int defwIOTimingSlewrate (const char*  riseFall, int  num1, int  num2) {
    return LefDefParser::defwIOTimingSlewrate(riseFall, num1, num2);
}

int defwIOTimingDrivecell (const char*  name, const char*  fromPin, const char*  toPin, int  numDrivers) {
    return LefDefParser::defwIOTimingDrivecell(name, fromPin, toPin, numDrivers);
}

int defwIOTimingCapacitance (double  num) {
    return LefDefParser::defwIOTimingCapacitance(num);
}

int defwEndIOTimings () {
    return LefDefParser::defwEndIOTimings();
}

int defwStartScanchains (int  count) {
    return LefDefParser::defwStartScanchains(count);
}

int defwScanchain (const char*  name) {
    return LefDefParser::defwScanchain(name);
}

int defwScanchainCommonscanpins (const char*  inst1, const char*  pin1, const char*  inst2, const char*  pin2) {
    return LefDefParser::defwScanchainCommonscanpins(inst1, pin1, inst2, pin2);
}

int defwScanchainPartition (const char*  name, int  maxBits) {
    return LefDefParser::defwScanchainPartition(name, maxBits);
}

int defwScanchainStart (const char*  inst, const char*  pin) {
    return LefDefParser::defwScanchainStart(inst, pin);
}

int defwScanchainStop (const char*  inst, const char*  pin) {
    return LefDefParser::defwScanchainStop(inst, pin);
}

int defwScanchainFloating (const char*  name, const char*  inst1, const char*  pin1, const char*  inst2, const char*  pin2) {
    return LefDefParser::defwScanchainFloating(name, inst1, pin1, inst2, pin2);
}

int defwScanchainFloatingBits (const char*  name, const char*  inst1, const char*  pin1, const char*  inst2, const char*  pin2, int    bits) {
    return LefDefParser::defwScanchainFloatingBits(name, inst1, pin1, inst2, pin2, bits);
}

int defwScanchainOrdered (const char*  name1, const char*  inst1, const char*  pin1, const char*  inst2, const char*  pin2, const char*  name2, const char*  inst3, const char*  pin3, const char*  inst4, const char*  pin4) {
    return LefDefParser::defwScanchainOrdered(name1, inst1, pin1, inst2, pin2, name2, inst3, pin3, inst4, pin4);
}

int defwScanchainOrderedBits (const char*  name1, const char*  inst1, const char*  pin1, const char*  inst2, const char*  pin2, int    bits1, const char*  name2, const char*  inst3, const char*  pin3, const char*  inst4, const char*  pin4, int    bits2) {
    return LefDefParser::defwScanchainOrderedBits(name1, inst1, pin1, inst2, pin2, bits1, name2, inst3, pin3, inst4, pin4, bits2);
}

int defwEndScanchain () {
    return LefDefParser::defwEndScanchain();
}

int defwStartConstraints (int  count) {
    return LefDefParser::defwStartConstraints(count);
}

int defwConstraintOperand () {
    return LefDefParser::defwConstraintOperand();
}

int defwConstraintOperandNet (const char*  netName) {
    return LefDefParser::defwConstraintOperandNet(netName);
}

int defwConstraintOperandPath (const char*  comp1, const char*  fromPin, const char*  comp2, const char*  toPin) {
    return LefDefParser::defwConstraintOperandPath(comp1, fromPin, comp2, toPin);
}

int defwConstraintOperandSum () {
    return LefDefParser::defwConstraintOperandSum();
}

int defwConstraintOperandSumEnd () {
    return LefDefParser::defwConstraintOperandSumEnd();
}

int defwConstraintOperandTime (const char*  timeType, int  time) {
    return LefDefParser::defwConstraintOperandTime(timeType, time);
}

int defwConstraintOperandEnd () {
    return LefDefParser::defwConstraintOperandEnd();
}

int defwConstraintWiredlogic (const char*  netName, int  distance) {
    return LefDefParser::defwConstraintWiredlogic(netName, distance);
}

int defwEndConstraints () {
    return LefDefParser::defwEndConstraints();
}

int defwStartGroups (int  count) {
    return LefDefParser::defwStartGroups(count);
}

int defwGroup (const char*  groupName, int  numExpr, const char**  groupExpr) {
    return LefDefParser::defwGroup(groupName, numExpr, groupExpr);
}

int defwGroupSoft (const char*  type1, double  value1, const char*  type2, double  value2, const char*  type3, double  value3) {
    return LefDefParser::defwGroupSoft(type1, value1, type2, value2, type3, value3);
}

int defwGroupRegion (int  xl, int  yl, int  xh, int  yh, const char*  regionName) {
    return LefDefParser::defwGroupRegion(xl, yl, xh, yh, regionName);
}

int defwEndGroups () {
    return LefDefParser::defwEndGroups();
}

int defwStartBlockages (int  count) {
    return LefDefParser::defwStartBlockages(count);
}

int defwBlockagesLayer (const char*  layerName) {
    return LefDefParser::defwBlockagesLayer(layerName);
}

int defwBlockagesLayerSlots () {
    return LefDefParser::defwBlockagesLayerSlots();
}

int defwBlockagesLayerFills () {
    return LefDefParser::defwBlockagesLayerFills();
}

int defwBlockagesLayerPushdown () {
    return LefDefParser::defwBlockagesLayerPushdown();
}

int defwBlockagesLayerExceptpgnet () {
    return LefDefParser::defwBlockagesLayerExceptpgnet();
}

int defwBlockagesLayerComponent (const char*  compName) {
    return LefDefParser::defwBlockagesLayerComponent(compName);
}

int defwBlockagesLayerSpacing (int  minSpacing) {
    return LefDefParser::defwBlockagesLayerSpacing(minSpacing);
}

int defwBlockagesLayerDesignRuleWidth (int  effectiveWidth) {
    return LefDefParser::defwBlockagesLayerDesignRuleWidth(effectiveWidth);
}

int defwBlockagesLayerMask (int  maskColor) {
    return LefDefParser::defwBlockagesLayerMask(maskColor);
}

int defwBlockageLayer (const char*  layerName, const char*  compName) {
    return LefDefParser::defwBlockageLayer(layerName, compName);
}

int defwBlockageLayerSlots (const char*  layerName) {
    return LefDefParser::defwBlockageLayerSlots(layerName);
}

int defwBlockageLayerFills (const char*  layerName) {
    return LefDefParser::defwBlockageLayerFills(layerName);
}

int defwBlockageLayerPushdown (const char*  layerName) {
    return LefDefParser::defwBlockageLayerPushdown(layerName);
}

int defwBlockageLayerExceptpgnet (const char*  layerName) {
    return LefDefParser::defwBlockageLayerExceptpgnet(layerName);
}

int defwBlockageSpacing (int  minSpacing) {
    return LefDefParser::defwBlockageSpacing(minSpacing);
}

int defwBlockageDesignRuleWidth (int  effectiveWidth) {
    return LefDefParser::defwBlockageDesignRuleWidth(effectiveWidth);
}

int defwBlockagesPlacement () {
    return LefDefParser::defwBlockagesPlacement();
}

int defwBlockagesPlacementComponent (const char*  compName) {
    return LefDefParser::defwBlockagesPlacementComponent(compName);
}

int defwBlockagesPlacementPushdown () {
    return LefDefParser::defwBlockagesPlacementPushdown();
}

int defwBlockagesPlacementSoft () {
    return LefDefParser::defwBlockagesPlacementSoft();
}

int defwBlockagesPlacementPartial (double  maxDensity) {
    return LefDefParser::defwBlockagesPlacementPartial(maxDensity);
}

int defwBlockagesRect (int  xl, int  yl, int  xh, int  yh) {
    return LefDefParser::defwBlockagesRect(xl, yl, xh, yh);
}

int defwBlockagesPolygon (int  num_polys, int*  xl, int*  yl) {
    return LefDefParser::defwBlockagesPolygon(num_polys, xl, yl);
}

int defwBlockagePlacement () {
    return LefDefParser::defwBlockagePlacement();
}

int defwBlockagePlacementComponent (const char*  compName) {
    return LefDefParser::defwBlockagePlacementComponent(compName);
}

int defwBlockagePlacementPushdown () {
    return LefDefParser::defwBlockagePlacementPushdown();
}

int defwBlockagePlacementSoft () {
    return LefDefParser::defwBlockagePlacementSoft();
}

int defwBlockagePlacementPartial (double  maxDensity) {
    return LefDefParser::defwBlockagePlacementPartial(maxDensity);
}

int defwBlockageMask (int  maskColor) {
    return LefDefParser::defwBlockageMask(maskColor);
}

int defwBlockageRect (int  xl, int  yl, int  xh, int  yh) {
    return LefDefParser::defwBlockageRect(xl, yl, xh, yh);
}

int defwBlockagePolygon (int  num_polys, int*  xl, int*  yl) {
    return LefDefParser::defwBlockagePolygon(num_polys, xl, yl);
}

int defwEndBlockages () {
    return LefDefParser::defwEndBlockages();
}

int defwStartSlots (int  count) {
    return LefDefParser::defwStartSlots(count);
}

int defwSlotLayer (const char*  layerName) {
    return LefDefParser::defwSlotLayer(layerName);
}

int defwSlotRect (int  xl, int  yl, int  xh, int  yh) {
    return LefDefParser::defwSlotRect(xl, yl, xh, yh);
}

int defwSlotPolygon (int  num_polys, double*  xl, double*  yl) {
    return LefDefParser::defwSlotPolygon(num_polys, xl, yl);
}

int defwEndSlots () {
    return LefDefParser::defwEndSlots();
}

int defwStartFills (int  count) {
    return LefDefParser::defwStartFills(count);
}

int defwFillLayer (const char*  layerName) {
    return LefDefParser::defwFillLayer(layerName);
}

int defwFillLayerMask (int  maskColor) {
    return LefDefParser::defwFillLayerMask(maskColor);
}

int defwFillLayerOPC () {
    return LefDefParser::defwFillLayerOPC();
}

int defwFillRect (int  xl, int  yl, int  xh, int  yh) {
    return LefDefParser::defwFillRect(xl, yl, xh, yh);
}

int defwFillPolygon (int  num_polys, double*  xl, double*  yl) {
    return LefDefParser::defwFillPolygon(num_polys, xl, yl);
}

int defwFillVia (const char*  viaName) {
    return LefDefParser::defwFillVia(viaName);
}

int defwFillViaMask (int  colorMask) {
    return LefDefParser::defwFillViaMask(colorMask);
}

int defwFillViaOPC () {
    return LefDefParser::defwFillViaOPC();
}

int defwFillPoints (int  num_points, double*  xl, double*  yl) {
    return LefDefParser::defwFillPoints(num_points, xl, yl);
}

int defwEndFills () {
    return LefDefParser::defwEndFills();
}

int defwStartNonDefaultRules (int  count) {
    return LefDefParser::defwStartNonDefaultRules(count);
}

int defwNonDefaultRule (const char*  ruleName, int  hardSpacing) {
    return LefDefParser::defwNonDefaultRule(ruleName, hardSpacing);
}

int defwNonDefaultRuleLayer (const char*  layerName, int  width, int  diagWidth, int  spacing, int  wireExt) {
    return LefDefParser::defwNonDefaultRuleLayer(layerName, width, diagWidth, spacing, wireExt);
}

int defwNonDefaultRuleVia (const char*  viaName) {
    return LefDefParser::defwNonDefaultRuleVia(viaName);
}

int defwNonDefaultRuleViaRule (const char*  viaRuleName) {
    return LefDefParser::defwNonDefaultRuleViaRule(viaRuleName);
}

int defwNonDefaultRuleMinCuts (const char*  cutLayerName, int  numCutS) {
    return LefDefParser::defwNonDefaultRuleMinCuts(cutLayerName, numCutS);
}

int defwEndNonDefaultRules () {
    return LefDefParser::defwEndNonDefaultRules();
}

int defwStartStyles (int  count) {
    return LefDefParser::defwStartStyles(count);
}

int defwStyles (int  styleNums, int  num_points, double*  xp, double*  yp) {
    return LefDefParser::defwStyles(styleNums, num_points, xp, yp);
}

int defwEndStyles () {
    return LefDefParser::defwEndStyles();
}

int defwStartBeginext (const char*  name) {
    return LefDefParser::defwStartBeginext(name);
}

int defwBeginextCreator (const char*  creatorName) {
    return LefDefParser::defwBeginextCreator(creatorName);
}

int defwBeginextDate () {
    return LefDefParser::defwBeginextDate();
}

int defwBeginextRevision (int  vers1, int  vers2) {
    return LefDefParser::defwBeginextRevision(vers1, vers2);
}

int defwBeginextSyntax (const char*  title, const char*  string) {
    return LefDefParser::defwBeginextSyntax(title, string);
}

int defwEndBeginext () {
    return LefDefParser::defwEndBeginext();
}

int defwEnd () {
    return LefDefParser::defwEnd();
}

int defwCurrentLineNumber () {
    return LefDefParser::defwCurrentLineNumber();
}

void defwPrintError (int  status) {
    LefDefParser::defwPrintError(status);
}

void defwAddComment (const char*  comment) {
    LefDefParser::defwAddComment(comment);
}

void defwAddIndent () {
    LefDefParser::defwAddIndent();
}

