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
int defwNewLine()
{
  return DefParser::defwNewLine();
}

int defwInit(FILE* f,
             int vers1,
             int version2,
             const char* caseSensitive,
             const char* dividerChar,
             const char* busBitChars,
             const char* designName,
             const char* technology,
             const char* array,
             const char* floorplan,
             double units)
{
  return DefParser::defwInit(f,
                             vers1,
                             version2,
                             caseSensitive,
                             dividerChar,
                             busBitChars,
                             designName,
                             technology,
                             array,
                             floorplan,
                             units);
}

int defwInitCbk(FILE* f)
{
  return DefParser::defwInitCbk(f);
}

int defwVersion(int vers1, int vers2)
{
  return DefParser::defwVersion(vers1, vers2);
}

int defwCaseSensitive(const char* caseSensitive)
{
  return DefParser::defwCaseSensitive(caseSensitive);
}

int defwBusBitChars(const char* busBitChars)
{
  return DefParser::defwBusBitChars(busBitChars);
}

int defwDividerChar(const char* dividerChar)
{
  return DefParser::defwDividerChar(dividerChar);
}

int defwDesignName(const char* name)
{
  return DefParser::defwDesignName(name);
}

int defwTechnology(const char* technology)
{
  return DefParser::defwTechnology(technology);
}

int defwArray(const char* array)
{
  return DefParser::defwArray(array);
}

int defwFloorplan(const char* floorplan)
{
  return DefParser::defwFloorplan(floorplan);
}

int defwUnits(int units)
{
  return DefParser::defwUnits(units);
}

int defwHistory(const char* string)
{
  return DefParser::defwHistory(string);
}

int defwStartPropDef()
{
  return DefParser::defwStartPropDef();
}

int defwIntPropDef(const char* objType,
                   const char* propName,
                   double leftRange,
                   double rightRange,
                   int propValue)
{
  return DefParser::defwIntPropDef(
      objType, propName, leftRange, rightRange, propValue);
}

int defwRealPropDef(const char* objType,
                    const char* propName,
                    double leftRange,
                    double rightRange,
                    double propValue)
{
  return DefParser::defwRealPropDef(
      objType, propName, leftRange, rightRange, propValue);
}

int defwStringPropDef(const char* objType,
                      const char* propName,
                      double leftRange,
                      double rightRange,
                      const char* propValue)
{
  return DefParser::defwStringPropDef(
      objType, propName, leftRange, rightRange, propValue);
}

int defwEndPropDef()
{
  return DefParser::defwEndPropDef();
}

int defwStringProperty(const char* propName, const char* propValue)
{
  return DefParser::defwStringProperty(propName, propValue);
}

int defwRealProperty(const char* propName, double propValue)
{
  return DefParser::defwRealProperty(propName, propValue);
}

int defwIntProperty(const char* propName, int propValue)
{
  return DefParser::defwIntProperty(propName, propValue);
}

int defwDieArea(int xl, int yl, int xh, int yh)
{
  return DefParser::defwDieArea(xl, yl, xh, yh);
}

int defwDieAreaList(int num_points, int* xl, int* yh)
{
  return DefParser::defwDieAreaList(num_points, xl, yh);
}

int defwRow(const char* rowName,
            const char* rowType,
            int x_orig,
            int y_orig,
            int orient,
            int do_count,
            int do_increment,
            int xstep,
            int ystep)
{
  return DefParser::defwRow(rowName,
                            rowType,
                            x_orig,
                            y_orig,
                            orient,
                            do_count,
                            do_increment,
                            xstep,
                            ystep);
}

int defwRowStr(const char* rowName,
               const char* rowType,
               int x_orig,
               int y_orig,
               const char* orient,
               int do_count,
               int do_increment,
               int xstep,
               int ystep)
{
  return DefParser::defwRowStr(rowName,
                               rowType,
                               x_orig,
                               y_orig,
                               orient,
                               do_count,
                               do_increment,
                               xstep,
                               ystep);
}

int defwTracks(const char* master,
               int doStart,
               int doCount,
               int doStep,
               int numLayers,
               const char** layers,
               int mask,
               int sameMask)
{
  return DefParser::defwTracks(
      master, doStart, doCount, doStep, numLayers, layers, mask, sameMask);
}

int defwGcellGrid(const char* master, int doStart, int doCount, int doStep)
{
  return DefParser::defwGcellGrid(master, doStart, doCount, doStep);
}

int defwStartDefaultCap(int count)
{
  return DefParser::defwStartDefaultCap(count);
}

int defwDefaultCap(int pins, double cap)
{
  return DefParser::defwDefaultCap(pins, cap);
}

int defwEndDefaultCap()
{
  return DefParser::defwEndDefaultCap();
}

int defwCanPlace(const char* master,
                 int xOrig,
                 int yOrig,
                 int orient,
                 int doCnt,
                 int doInc,
                 int xStep,
                 int yStep)
{
  return DefParser::defwCanPlace(
      master, xOrig, yOrig, orient, doCnt, doInc, xStep, yStep);
}

int defwCanPlaceStr(const char* master,
                    int xOrig,
                    int yOrig,
                    const char* orient,
                    int doCnt,
                    int doInc,
                    int xStep,
                    int yStep)
{
  return DefParser::defwCanPlaceStr(
      master, xOrig, yOrig, orient, doCnt, doInc, xStep, yStep);
}

int defwCannotOccupy(const char* master,
                     int xOrig,
                     int yOrig,
                     int orient,
                     int doCnt,
                     int doInc,
                     int xStep,
                     int yStep)
{
  return DefParser::defwCannotOccupy(
      master, xOrig, yOrig, orient, doCnt, doInc, xStep, yStep);
}

int defwCannotOccupyStr(const char* master,
                        int xOrig,
                        int yOrig,
                        const char* orient,
                        int doCnt,
                        int doInc,
                        int xStep,
                        int yStep)
{
  return DefParser::defwCannotOccupyStr(
      master, xOrig, yOrig, orient, doCnt, doInc, xStep, yStep);
}

int defwStartVias(int count)
{
  return DefParser::defwStartVias(count);
}

int defwViaName(const char* name)
{
  return DefParser::defwViaName(name);
}

int defwViaPattern(const char* patternName)
{
  return DefParser::defwViaPattern(patternName);
}

int defwViaRect(const char* layerName, int xl, int yl, int xh, int yh, int mask)
{
  return DefParser::defwViaRect(layerName, xl, yl, xh, yh, mask);
}

int defwViaPolygon(const char* layerName,
                   int num_polys,
                   double* xl,
                   double* yl,
                   int mask)
{
  return DefParser::defwViaPolygon(layerName, num_polys, xl, yl, mask);
}

int defwViaViarule(const char* viaRuleName,
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
  return DefParser::defwViaViarule(viaRuleName,
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

int defwViaViaruleRowCol(int numCutRows, int numCutCols)
{
  return DefParser::defwViaViaruleRowCol(numCutRows, numCutCols);
}

int defwViaViaruleOrigin(int xOffset, int yOffset)
{
  return DefParser::defwViaViaruleOrigin(xOffset, yOffset);
}

int defwViaViaruleOffset(int xBotOffset,
                         int yBotOffset,
                         int xTopOffset,
                         int yTopOffset)
{
  return DefParser::defwViaViaruleOffset(
      xBotOffset, yBotOffset, xTopOffset, yTopOffset);
}

int defwViaViarulePattern(const char* cutPattern)
{
  return DefParser::defwViaViarulePattern(cutPattern);
}

int defwOneViaEnd()
{
  return DefParser::defwOneViaEnd();
}

int defwEndVias()
{
  return DefParser::defwEndVias();
}

int defwStartRegions(int count)
{
  return DefParser::defwStartRegions(count);
}

int defwRegionName(const char* name)
{
  return DefParser::defwRegionName(name);
}

int defwRegionPoints(int xl, int yl, int xh, int yh)
{
  return DefParser::defwRegionPoints(xl, yl, xh, yh);
}

int defwRegionType(const char* type)
{
  return DefParser::defwRegionType(type);
}

int defwEndRegions()
{
  return DefParser::defwEndRegions();
}

int defwComponentMaskShiftLayers(const char** layerNames, int numLayerName)
{
  return DefParser::defwComponentMaskShiftLayers(layerNames, numLayerName);
}

int defwStartComponents(int count)
{
  return DefParser::defwStartComponents(count);
}

int defwComponent(const char* instance,
                  const char* master,
                  int numNetName,
                  const char** netNames,
                  const char* eeq,
                  const char* genName,
                  const char* genParemeters,
                  const char* source,
                  int numForeign,
                  const char** foreigns,
                  int* foreignX,
                  int* foreignY,
                  int* foreignOrients,
                  const char* status,
                  int statusX,
                  int statusY,
                  int statusOrient,
                  double weight,
                  const char* region,
                  int xl,
                  int yl,
                  int xh,
                  int yh)
{
  return DefParser::defwComponent(instance,
                                  master,
                                  numNetName,
                                  netNames,
                                  eeq,
                                  genName,
                                  genParemeters,
                                  source,
                                  numForeign,
                                  foreigns,
                                  foreignX,
                                  foreignY,
                                  foreignOrients,
                                  status,
                                  statusX,
                                  statusY,
                                  statusOrient,
                                  weight,
                                  region,
                                  xl,
                                  yl,
                                  xh,
                                  yh);
}

int defwComponentStr(const char* instance,
                     const char* master,
                     int numNetName,
                     const char** netNames,
                     const char* eeq,
                     const char* genName,
                     const char* genParemeters,
                     const char* source,
                     int numForeign,
                     const char** foreigns,
                     int* foreignX,
                     int* foreignY,
                     const char** foreignOrients,
                     const char* status,
                     int statusX,
                     int statusY,
                     const char* statusOrient,
                     double weight,
                     const char* region,
                     int xl,
                     int yl,
                     int xh,
                     int yh)
{
  return DefParser::defwComponentStr(instance,
                                     master,
                                     numNetName,
                                     netNames,
                                     eeq,
                                     genName,
                                     genParemeters,
                                     source,
                                     numForeign,
                                     foreigns,
                                     foreignX,
                                     foreignY,
                                     foreignOrients,
                                     status,
                                     statusX,
                                     statusY,
                                     statusOrient,
                                     weight,
                                     region,
                                     xl,
                                     yl,
                                     xh,
                                     yh);
}

int defwComponentMaskShift(int shiftLayerMasks)
{
  return DefParser::defwComponentMaskShift(shiftLayerMasks);
}

int defwComponentHalo(int left, int bottom, int right, int top)
{
  return DefParser::defwComponentHalo(left, bottom, right, top);
}

int defwComponentHaloSoft(int left, int bottom, int right, int top)
{
  return DefParser::defwComponentHaloSoft(left, bottom, right, top);
}

int defwComponentRouteHalo(int haloDist,
                           const char* minLayer,
                           const char* maxLayer)
{
  return DefParser::defwComponentRouteHalo(haloDist, minLayer, maxLayer);
}

int defwEndComponents()
{
  return DefParser::defwEndComponents();
}

int defwStartPins(int count)
{
  return DefParser::defwStartPins(count);
}

int defwPin(const char* name,
            const char* net,
            int special,
            const char* direction,
            const char* use,
            const char* status,
            int statusX,
            int statusY,
            int orient,
            const char* layer,
            int xl,
            int yl,
            int xh,
            int yh)
{
  return DefParser::defwPin(name,
                            net,
                            special,
                            direction,
                            use,
                            status,
                            statusX,
                            statusY,
                            orient,
                            layer,
                            xl,
                            yl,
                            xh,
                            yh);
}

int defwPinStr(const char* name,
               const char* net,
               int special,
               const char* direction,
               const char* use,
               const char* status,
               int statusX,
               int statusY,
               const char* orient,
               const char* layer,
               int xl,
               int yl,
               int xh,
               int yh)
{
  return DefParser::defwPinStr(name,
                               net,
                               special,
                               direction,
                               use,
                               status,
                               statusX,
                               statusY,
                               orient,
                               layer,
                               xl,
                               yl,
                               xh,
                               yh);
}

int defwPinLayer(const char* layerName,
                 int spacing,
                 int designRuleWidth,
                 int xl,
                 int yl,
                 int xh,
                 int yh,
                 int mask)
{
  return DefParser::defwPinLayer(
      layerName, spacing, designRuleWidth, xl, yl, xh, yh, mask);
}

int defwPinPolygon(const char* layerName,
                   int spacing,
                   int designRuleWidth,
                   int num_polys,
                   double* xl,
                   double* yl,
                   int mask)
{
  return DefParser::defwPinPolygon(
      layerName, spacing, designRuleWidth, num_polys, xl, yl, mask);
}

int defwPinVia(const char* viaName, int xl, int yl, int mask)
{
  return DefParser::defwPinVia(viaName, xl, yl, mask);
}

int defwPinPort()
{
  return DefParser::defwPinPort();
}

int defwPinPortLayer(const char* layerName,
                     int spacing,
                     int designRuleWidth,
                     int xl,
                     int yl,
                     int xh,
                     int yh,
                     int mask)
{
  return DefParser::defwPinPortLayer(
      layerName, spacing, designRuleWidth, xl, yl, xh, yh, mask);
}

int defwPinPortPolygon(const char* layerName,
                       int spacing,
                       int designRuleWidth,
                       int num_polys,
                       double* xl,
                       double* yl,
                       int mask)
{
  return DefParser::defwPinPortPolygon(
      layerName, spacing, designRuleWidth, num_polys, xl, yl, mask);
}

int defwPinPortVia(const char* viaName, int xl, int yl, int mask)
{
  return DefParser::defwPinPortVia(viaName, xl, yl, mask);
}

int defwPinPortLocation(const char* status,
                        int statusX,
                        int statusY,
                        const char* orient)
{
  return DefParser::defwPinPortLocation(status, statusX, statusY, orient);
}

int defwPinNetExpr(const char* pinExpr)
{
  return DefParser::defwPinNetExpr(pinExpr);
}

int defwPinSupplySensitivity(const char* pinName)
{
  return DefParser::defwPinSupplySensitivity(pinName);
}

int defwPinGroundSensitivity(const char* pinName)
{
  return DefParser::defwPinGroundSensitivity(pinName);
}

int defwPinAntennaPinPartialMetalArea(int value, const char* layerName)
{
  return DefParser::defwPinAntennaPinPartialMetalArea(value, layerName);
}

int defwPinAntennaPinPartialMetalSideArea(int value, const char* layerName)
{
  return DefParser::defwPinAntennaPinPartialMetalSideArea(value, layerName);
}

int defwPinAntennaPinPartialCutArea(int value, const char* layerName)
{
  return DefParser::defwPinAntennaPinPartialCutArea(value, layerName);
}

int defwPinAntennaPinDiffArea(int value, const char* layerName)
{
  return DefParser::defwPinAntennaPinDiffArea(value, layerName);
}

int defwPinAntennaModel(const char* oxide)
{
  return DefParser::defwPinAntennaModel(oxide);
}

int defwPinAntennaPinGateArea(int value, const char* layerName)
{
  return DefParser::defwPinAntennaPinGateArea(value, layerName);
}

int defwPinAntennaPinMaxAreaCar(int value, const char* layerName)
{
  return DefParser::defwPinAntennaPinMaxAreaCar(value, layerName);
}

int defwPinAntennaPinMaxSideAreaCar(int value, const char* layerName)
{
  return DefParser::defwPinAntennaPinMaxSideAreaCar(value, layerName);
}

int defwPinAntennaPinMaxCutCar(int value, const char* layerName)
{
  return DefParser::defwPinAntennaPinMaxCutCar(value, layerName);
}

int defwEndPins()
{
  return DefParser::defwEndPins();
}

int defwStartPinProperties(int count)
{
  return DefParser::defwStartPinProperties(count);
}

int defwPinProperty(const char* name, const char* pinName)
{
  return DefParser::defwPinProperty(name, pinName);
}

int defwEndPinProperties()
{
  return DefParser::defwEndPinProperties();
}

int defwStartSpecialNets(int count)
{
  return DefParser::defwStartSpecialNets(count);
}

int defwSpecialNet(const char* name)
{
  return DefParser::defwSpecialNet(name);
}

int defwSpecialNetConnection(const char* inst, const char* pin, int synthesized)
{
  return DefParser::defwSpecialNetConnection(inst, pin, synthesized);
}

int defwSpecialNetFixedbump()
{
  return DefParser::defwSpecialNetFixedbump();
}

int defwSpecialNetVoltage(double v)
{
  return DefParser::defwSpecialNetVoltage(v);
}

int defwSpecialNetSpacing(const char* layer,
                          int spacing,
                          double minwidth,
                          double maxwidth)
{
  return DefParser::defwSpecialNetSpacing(layer, spacing, minwidth, maxwidth);
}

int defwSpecialNetWidth(const char* layer, int width)
{
  return DefParser::defwSpecialNetWidth(layer, width);
}

int defwSpecialNetSource(const char* name)
{
  return DefParser::defwSpecialNetSource(name);
}

int defwSpecialNetOriginal(const char* name)
{
  return DefParser::defwSpecialNetOriginal(name);
}

int defwSpecialNetPattern(const char* name)
{
  return DefParser::defwSpecialNetPattern(name);
}

int defwSpecialNetUse(const char* name)
{
  return DefParser::defwSpecialNetUse(name);
}

int defwSpecialNetWeight(double value)
{
  return DefParser::defwSpecialNetWeight(value);
}

int defwSpecialNetEstCap(double value)
{
  return DefParser::defwSpecialNetEstCap(value);
}

int defwSpecialNetPathStart(const char* typ)
{
  return DefParser::defwSpecialNetPathStart(typ);
}

int defwSpecialNetShieldNetName(const char* name)
{
  return DefParser::defwSpecialNetShieldNetName(name);
}

int defwSpecialNetPathLayer(const char* name)
{
  return DefParser::defwSpecialNetPathLayer(name);
}

int defwSpecialNetPathWidth(int width)
{
  return DefParser::defwSpecialNetPathWidth(width);
}

int defwSpecialNetPathStyle(int styleNum)
{
  return DefParser::defwSpecialNetPathStyle(styleNum);
}

int defwSpecialNetPathShape(const char* shapeType)
{
  return DefParser::defwSpecialNetPathShape(shapeType);
}

int defwSpecialNetPathMask(int colorMask)
{
  return DefParser::defwSpecialNetPathMask(colorMask);
}

int defwSpecialNetPathPoint(int numPts, double* pointx, double* pointy)
{
  return DefParser::defwSpecialNetPathPoint(numPts, pointx, pointy);
}

int defwSpecialNetPathVia(const char* name)
{
  return DefParser::defwSpecialNetPathVia(name);
}

int defwSpecialNetPathViaData(int numX, int numY, int stepX, int stepY)
{
  return DefParser::defwSpecialNetPathViaData(numX, numY, stepX, stepY);
}

int defwSpecialNetPathPointWithWireExt(int numPts,
                                       double* pointx,
                                       double* pointy,
                                       double* optValue)
{
  return DefParser::defwSpecialNetPathPointWithWireExt(
      numPts, pointx, pointy, optValue);
}

int defwSpecialNetPathEnd()
{
  return DefParser::defwSpecialNetPathEnd();
}

int defwSpecialNetPolygon(const char* layerName,
                          int num_polys,
                          double* xl,
                          double* yl)
{
  return DefParser::defwSpecialNetPolygon(layerName, num_polys, xl, yl);
}

int defwSpecialNetRect(const char* layerName, int xl, int yl, int xh, int yh)
{
  return DefParser::defwSpecialNetRect(layerName, xl, yl, xh, yh);
}

int defwSpecialNetVia(const char* layerName)
{
  return DefParser::defwSpecialNetVia(layerName);
}

int defwSpecialNetViaWithOrient(const char* layerName, int orient)
{
  return DefParser::defwSpecialNetViaWithOrient(layerName, orient);
}

int defwSpecialNetViaPoints(int num_points, double* xl, double* yl)
{
  return DefParser::defwSpecialNetViaPoints(num_points, xl, yl);
}

int defwSpecialNetEndOneNet()
{
  return DefParser::defwSpecialNetEndOneNet();
}

int defwSpecialNetShieldStart(const char* name)
{
  return DefParser::defwSpecialNetShieldStart(name);
}

int defwSpecialNetShieldLayer(const char* name)
{
  return DefParser::defwSpecialNetShieldLayer(name);
}

int defwSpecialNetShieldWidth(int width)
{
  return DefParser::defwSpecialNetShieldWidth(width);
}

int defwSpecialNetShieldShape(const char* shapeType)
{
  return DefParser::defwSpecialNetShieldShape(shapeType);
}

int defwSpecialNetShieldPoint(int numPts, double* pointx, double* pointy)
{
  return DefParser::defwSpecialNetShieldPoint(numPts, pointx, pointy);
}

int defwSpecialNetShieldVia(const char* name)
{
  return DefParser::defwSpecialNetShieldVia(name);
}

int defwSpecialNetShieldViaData(int numX, int numY, int stepX, int stepY)
{
  return DefParser::defwSpecialNetShieldViaData(numX, numY, stepX, stepY);
}

int defwSpecialNetShieldEnd()
{
  return DefParser::defwSpecialNetShieldEnd();
}

int defwEndSpecialNets()
{
  return DefParser::defwEndSpecialNets();
}

int defwStartNets(int count)
{
  return DefParser::defwStartNets(count);
}

int defwNet(const char* name)
{
  return DefParser::defwNet(name);
}

int defwNetConnection(const char* inst, const char* pin, int synthesized)
{
  return DefParser::defwNetConnection(inst, pin, synthesized);
}

int defwNetMustjoinConnection(const char* inst, const char* pin)
{
  return DefParser::defwNetMustjoinConnection(inst, pin);
}

int defwNetVpin(const char* vpinName,
                const char* layerName,
                int layerXl,
                int layerYl,
                int layerXh,
                int layerYh,
                const char* status,
                int statusX,
                int statusY,
                int orient)
{
  return DefParser::defwNetVpin(vpinName,
                                layerName,
                                layerXl,
                                layerYl,
                                layerXh,
                                layerYh,
                                status,
                                statusX,
                                statusY,
                                orient);
}

int defwNetVpinStr(const char* vpinName,
                   const char* layerName,
                   int layerXl,
                   int layerYl,
                   int layerXh,
                   int layerYh,
                   const char* status,
                   int statusX,
                   int statusY,
                   const char* orient)
{
  return DefParser::defwNetVpinStr(vpinName,
                                   layerName,
                                   layerXl,
                                   layerYl,
                                   layerXh,
                                   layerYh,
                                   status,
                                   statusX,
                                   statusY,
                                   orient);
}

int defwNetNondefaultRule(const char* name)
{
  return DefParser::defwNetNondefaultRule(name);
}

int defwNetXtalk(int xtalk)
{
  return DefParser::defwNetXtalk(xtalk);
}

int defwNetFixedbump()
{
  return DefParser::defwNetFixedbump();
}

int defwNetFrequency(double frequency)
{
  return DefParser::defwNetFrequency(frequency);
}

int defwNetSource(const char* name)
{
  return DefParser::defwNetSource(name);
}

int defwNetOriginal(const char* name)
{
  return DefParser::defwNetOriginal(name);
}

int defwNetUse(const char* name)
{
  return DefParser::defwNetUse(name);
}

int defwNetPattern(const char* name)
{
  return DefParser::defwNetPattern(name);
}

int defwNetEstCap(double value)
{
  return DefParser::defwNetEstCap(value);
}

int defwNetWeight(double value)
{
  return DefParser::defwNetWeight(value);
}

int defwNetShieldnet(const char* name)
{
  return DefParser::defwNetShieldnet(name);
}

int defwNetNoshieldStart(const char* name)
{
  return DefParser::defwNetNoshieldStart(name);
}

int defwNetNoshieldPoint(int numPts, const char** pointx, const char** pointy)
{
  return DefParser::defwNetNoshieldPoint(numPts, pointx, pointy);
}

int defwNetNoshieldVia(const char* name)
{
  return DefParser::defwNetNoshieldVia(name);
}

int defwNetNoshieldEnd()
{
  return DefParser::defwNetNoshieldEnd();
}

int defwNetSubnetStart(const char* name)
{
  return DefParser::defwNetSubnetStart(name);
}

int defwNetSubnetPin(const char* compName, const char* pinName)
{
  return DefParser::defwNetSubnetPin(compName, pinName);
}

int defwNetSubnetEnd()
{
  return DefParser::defwNetSubnetEnd();
}

int defwNetPathStart(const char* typ)
{
  return DefParser::defwNetPathStart(typ);
}

int defwNetPathWidth(int w)
{
  return DefParser::defwNetPathWidth(w);
}

int defwNetPathLayer(const char* name, int isTaper, const char* rulename)
{
  return DefParser::defwNetPathLayer(name, isTaper, rulename);
}

int defwNetPathStyle(int styleNum)
{
  return DefParser::defwNetPathStyle(styleNum);
}

int defwNetPathMask(int maskNum)
{
  return DefParser::defwNetPathMask(maskNum);
}

int defwNetPathRect(int deltaX1, int deltaY1, int deltaX2, int deltaY2)
{
  return DefParser::defwNetPathRect(deltaX1, deltaY1, deltaX2, deltaY2);
}

int defwNetPathVirtual(int x, int y)
{
  return DefParser::defwNetPathVirtual(x, y);
}

int defwNetPathPoint(int numPts, double* pointx, double* pointy)
{
  return DefParser::defwNetPathPoint(numPts, pointx, pointy);
}

int defwNetPathPointWithExt(int numPts,
                            double* pointx,
                            double* pointy,
                            double* optValue)
{
  return DefParser::defwNetPathPointWithExt(numPts, pointx, pointy, optValue);
}

int defwNetPathVia(const char* name)
{
  return DefParser::defwNetPathVia(name);
}

int defwNetPathViaWithOrient(const char* name, int orient)
{
  return DefParser::defwNetPathViaWithOrient(name, orient);
}

int defwNetPathViaWithOrientStr(const char* name, const char* orient)
{
  return DefParser::defwNetPathViaWithOrientStr(name, orient);
}

int defwNetPathEnd()
{
  return DefParser::defwNetPathEnd();
}

int defwNetEndOneNet()
{
  return DefParser::defwNetEndOneNet();
}

int defwEndNets()
{
  return DefParser::defwEndNets();
}

int defwStartIOTimings(int count)
{
  return DefParser::defwStartIOTimings(count);
}

int defwIOTiming(const char* inst, const char* pin)
{
  return DefParser::defwIOTiming(inst, pin);
}

int defwIOTimingVariable(const char* riseFall, int num1, int num2)
{
  return DefParser::defwIOTimingVariable(riseFall, num1, num2);
}

int defwIOTimingSlewrate(const char* riseFall, int num1, int num2)
{
  return DefParser::defwIOTimingSlewrate(riseFall, num1, num2);
}

int defwIOTimingDrivecell(const char* name,
                          const char* fromPin,
                          const char* toPin,
                          int numDrivers)
{
  return DefParser::defwIOTimingDrivecell(name, fromPin, toPin, numDrivers);
}

int defwIOTimingCapacitance(double num)
{
  return DefParser::defwIOTimingCapacitance(num);
}

int defwEndIOTimings()
{
  return DefParser::defwEndIOTimings();
}

int defwStartScanchains(int count)
{
  return DefParser::defwStartScanchains(count);
}

int defwScanchain(const char* name)
{
  return DefParser::defwScanchain(name);
}

int defwScanchainCommonscanpins(const char* inst1,
                                const char* pin1,
                                const char* inst2,
                                const char* pin2)
{
  return DefParser::defwScanchainCommonscanpins(inst1, pin1, inst2, pin2);
}

int defwScanchainPartition(const char* name, int maxBits)
{
  return DefParser::defwScanchainPartition(name, maxBits);
}

int defwScanchainStart(const char* inst, const char* pin)
{
  return DefParser::defwScanchainStart(inst, pin);
}

int defwScanchainStop(const char* inst, const char* pin)
{
  return DefParser::defwScanchainStop(inst, pin);
}

int defwScanchainFloating(const char* name,
                          const char* inst1,
                          const char* pin1,
                          const char* inst2,
                          const char* pin2)
{
  return DefParser::defwScanchainFloating(name, inst1, pin1, inst2, pin2);
}

int defwScanchainFloatingBits(const char* name,
                              const char* inst1,
                              const char* pin1,
                              const char* inst2,
                              const char* pin2,
                              int bits)
{
  return DefParser::defwScanchainFloatingBits(
      name, inst1, pin1, inst2, pin2, bits);
}

int defwScanchainOrdered(const char* name1,
                         const char* inst1,
                         const char* pin1,
                         const char* inst2,
                         const char* pin2,
                         const char* name2,
                         const char* inst3,
                         const char* pin3,
                         const char* inst4,
                         const char* pin4)
{
  return DefParser::defwScanchainOrdered(
      name1, inst1, pin1, inst2, pin2, name2, inst3, pin3, inst4, pin4);
}

int defwScanchainOrderedBits(const char* name1,
                             const char* inst1,
                             const char* pin1,
                             const char* inst2,
                             const char* pin2,
                             int bits1,
                             const char* name2,
                             const char* inst3,
                             const char* pin3,
                             const char* inst4,
                             const char* pin4,
                             int bits2)
{
  return DefParser::defwScanchainOrderedBits(name1,
                                             inst1,
                                             pin1,
                                             inst2,
                                             pin2,
                                             bits1,
                                             name2,
                                             inst3,
                                             pin3,
                                             inst4,
                                             pin4,
                                             bits2);
}

int defwEndScanchain()
{
  return DefParser::defwEndScanchain();
}

int defwStartConstraints(int count)
{
  return DefParser::defwStartConstraints(count);
}

int defwConstraintOperand()
{
  return DefParser::defwConstraintOperand();
}

int defwConstraintOperandNet(const char* netName)
{
  return DefParser::defwConstraintOperandNet(netName);
}

int defwConstraintOperandPath(const char* comp1,
                              const char* fromPin,
                              const char* comp2,
                              const char* toPin)
{
  return DefParser::defwConstraintOperandPath(comp1, fromPin, comp2, toPin);
}

int defwConstraintOperandSum()
{
  return DefParser::defwConstraintOperandSum();
}

int defwConstraintOperandSumEnd()
{
  return DefParser::defwConstraintOperandSumEnd();
}

int defwConstraintOperandTime(const char* timeType, int time)
{
  return DefParser::defwConstraintOperandTime(timeType, time);
}

int defwConstraintOperandEnd()
{
  return DefParser::defwConstraintOperandEnd();
}

int defwConstraintWiredlogic(const char* netName, int distance)
{
  return DefParser::defwConstraintWiredlogic(netName, distance);
}

int defwEndConstraints()
{
  return DefParser::defwEndConstraints();
}

int defwStartGroups(int count)
{
  return DefParser::defwStartGroups(count);
}

int defwGroup(const char* groupName, int numExpr, const char** groupExpr)
{
  return DefParser::defwGroup(groupName, numExpr, groupExpr);
}

int defwGroupSoft(const char* type1,
                  double value1,
                  const char* type2,
                  double value2,
                  const char* type3,
                  double value3)
{
  return DefParser::defwGroupSoft(type1, value1, type2, value2, type3, value3);
}

int defwGroupRegion(int xl, int yl, int xh, int yh, const char* regionName)
{
  return DefParser::defwGroupRegion(xl, yl, xh, yh, regionName);
}

int defwEndGroups()
{
  return DefParser::defwEndGroups();
}

int defwStartBlockages(int count)
{
  return DefParser::defwStartBlockages(count);
}

int defwBlockagesLayer(const char* layerName)
{
  return DefParser::defwBlockagesLayer(layerName);
}

int defwBlockagesLayerSlots()
{
  return DefParser::defwBlockagesLayerSlots();
}

int defwBlockagesLayerFills()
{
  return DefParser::defwBlockagesLayerFills();
}

int defwBlockagesLayerPushdown()
{
  return DefParser::defwBlockagesLayerPushdown();
}

int defwBlockagesLayerExceptpgnet()
{
  return DefParser::defwBlockagesLayerExceptpgnet();
}

int defwBlockagesLayerComponent(const char* compName)
{
  return DefParser::defwBlockagesLayerComponent(compName);
}

int defwBlockagesLayerSpacing(int minSpacing)
{
  return DefParser::defwBlockagesLayerSpacing(minSpacing);
}

int defwBlockagesLayerDesignRuleWidth(int effectiveWidth)
{
  return DefParser::defwBlockagesLayerDesignRuleWidth(effectiveWidth);
}

int defwBlockagesLayerMask(int maskColor)
{
  return DefParser::defwBlockagesLayerMask(maskColor);
}

int defwBlockageLayer(const char* layerName, const char* compName)
{
  return DefParser::defwBlockageLayer(layerName, compName);
}

int defwBlockageLayerSlots(const char* layerName)
{
  return DefParser::defwBlockageLayerSlots(layerName);
}

int defwBlockageLayerFills(const char* layerName)
{
  return DefParser::defwBlockageLayerFills(layerName);
}

int defwBlockageLayerPushdown(const char* layerName)
{
  return DefParser::defwBlockageLayerPushdown(layerName);
}

int defwBlockageLayerExceptpgnet(const char* layerName)
{
  return DefParser::defwBlockageLayerExceptpgnet(layerName);
}

int defwBlockageSpacing(int minSpacing)
{
  return DefParser::defwBlockageSpacing(minSpacing);
}

int defwBlockageDesignRuleWidth(int effectiveWidth)
{
  return DefParser::defwBlockageDesignRuleWidth(effectiveWidth);
}

int defwBlockagesPlacement()
{
  return DefParser::defwBlockagesPlacement();
}

int defwBlockagesPlacementComponent(const char* compName)
{
  return DefParser::defwBlockagesPlacementComponent(compName);
}

int defwBlockagesPlacementPushdown()
{
  return DefParser::defwBlockagesPlacementPushdown();
}

int defwBlockagesPlacementSoft()
{
  return DefParser::defwBlockagesPlacementSoft();
}

int defwBlockagesPlacementPartial(double maxDensity)
{
  return DefParser::defwBlockagesPlacementPartial(maxDensity);
}

int defwBlockagesRect(int xl, int yl, int xh, int yh)
{
  return DefParser::defwBlockagesRect(xl, yl, xh, yh);
}

int defwBlockagesPolygon(int num_polys, int* xl, int* yl)
{
  return DefParser::defwBlockagesPolygon(num_polys, xl, yl);
}

int defwBlockagePlacement()
{
  return DefParser::defwBlockagePlacement();
}

int defwBlockagePlacementComponent(const char* compName)
{
  return DefParser::defwBlockagePlacementComponent(compName);
}

int defwBlockagePlacementPushdown()
{
  return DefParser::defwBlockagePlacementPushdown();
}

int defwBlockagePlacementSoft()
{
  return DefParser::defwBlockagePlacementSoft();
}

int defwBlockagePlacementPartial(double maxDensity)
{
  return DefParser::defwBlockagePlacementPartial(maxDensity);
}

int defwBlockageMask(int maskColor)
{
  return DefParser::defwBlockageMask(maskColor);
}

int defwBlockageRect(int xl, int yl, int xh, int yh)
{
  return DefParser::defwBlockageRect(xl, yl, xh, yh);
}

int defwBlockagePolygon(int num_polys, int* xl, int* yl)
{
  return DefParser::defwBlockagePolygon(num_polys, xl, yl);
}

int defwEndBlockages()
{
  return DefParser::defwEndBlockages();
}

int defwStartSlots(int count)
{
  return DefParser::defwStartSlots(count);
}

int defwSlotLayer(const char* layerName)
{
  return DefParser::defwSlotLayer(layerName);
}

int defwSlotRect(int xl, int yl, int xh, int yh)
{
  return DefParser::defwSlotRect(xl, yl, xh, yh);
}

int defwSlotPolygon(int num_polys, double* xl, double* yl)
{
  return DefParser::defwSlotPolygon(num_polys, xl, yl);
}

int defwEndSlots()
{
  return DefParser::defwEndSlots();
}

int defwStartFills(int count)
{
  return DefParser::defwStartFills(count);
}

int defwFillLayer(const char* layerName)
{
  return DefParser::defwFillLayer(layerName);
}

int defwFillLayerMask(int maskColor)
{
  return DefParser::defwFillLayerMask(maskColor);
}

int defwFillLayerOPC()
{
  return DefParser::defwFillLayerOPC();
}

int defwFillRect(int xl, int yl, int xh, int yh)
{
  return DefParser::defwFillRect(xl, yl, xh, yh);
}

int defwFillPolygon(int num_polys, double* xl, double* yl)
{
  return DefParser::defwFillPolygon(num_polys, xl, yl);
}

int defwFillVia(const char* viaName)
{
  return DefParser::defwFillVia(viaName);
}

int defwFillViaMask(int colorMask)
{
  return DefParser::defwFillViaMask(colorMask);
}

int defwFillViaOPC()
{
  return DefParser::defwFillViaOPC();
}

int defwFillPoints(int num_points, double* xl, double* yl)
{
  return DefParser::defwFillPoints(num_points, xl, yl);
}

int defwEndFills()
{
  return DefParser::defwEndFills();
}

int defwStartNonDefaultRules(int count)
{
  return DefParser::defwStartNonDefaultRules(count);
}

int defwNonDefaultRule(const char* ruleName, int hardSpacing)
{
  return DefParser::defwNonDefaultRule(ruleName, hardSpacing);
}

int defwNonDefaultRuleLayer(const char* layerName,
                            int width,
                            int diagWidth,
                            int spacing,
                            int wireExt)
{
  return DefParser::defwNonDefaultRuleLayer(
      layerName, width, diagWidth, spacing, wireExt);
}

int defwNonDefaultRuleVia(const char* viaName)
{
  return DefParser::defwNonDefaultRuleVia(viaName);
}

int defwNonDefaultRuleViaRule(const char* viaRuleName)
{
  return DefParser::defwNonDefaultRuleViaRule(viaRuleName);
}

int defwNonDefaultRuleMinCuts(const char* cutLayerName, int numCutS)
{
  return DefParser::defwNonDefaultRuleMinCuts(cutLayerName, numCutS);
}

int defwEndNonDefaultRules()
{
  return DefParser::defwEndNonDefaultRules();
}

int defwStartStyles(int count)
{
  return DefParser::defwStartStyles(count);
}

int defwStyles(int styleNums, int num_points, double* xp, double* yp)
{
  return DefParser::defwStyles(styleNums, num_points, xp, yp);
}

int defwEndStyles()
{
  return DefParser::defwEndStyles();
}

int defwStartBeginext(const char* name)
{
  return DefParser::defwStartBeginext(name);
}

int defwBeginextCreator(const char* creatorName)
{
  return DefParser::defwBeginextCreator(creatorName);
}

int defwBeginextDate()
{
  return DefParser::defwBeginextDate();
}

int defwBeginextRevision(int vers1, int vers2)
{
  return DefParser::defwBeginextRevision(vers1, vers2);
}

int defwBeginextSyntax(const char* title, const char* string)
{
  return DefParser::defwBeginextSyntax(title, string);
}

int defwEndBeginext()
{
  return DefParser::defwEndBeginext();
}

int defwEnd()
{
  return DefParser::defwEnd();
}

int defwCurrentLineNumber()
{
  return DefParser::defwCurrentLineNumber();
}

void defwPrintError(int status)
{
  DefParser::defwPrintError(status);
}

void defwAddComment(const char* comment)
{
  DefParser::defwAddComment(comment);
}

void defwAddIndent()
{
  DefParser::defwAddIndent();
}
