// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2014, Cadence Design Systems
// 
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8. 
// 
// Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
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

#ifndef lefiMisc_h
#define lefiMisc_h

#include <stdio.h>
#include "lefiKRDefs.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

// The different types of items in a geometry list.

struct lefiGeomRect {
      double xl;
      double yl;
      double xh;
      double yh;
      int    colorMask;
};

struct lefiGeomRectIter {
  double xl;
  double yl;
  double xh;
  double yh;
  double xStart;
  double yStart;
  double xStep;
  double yStep;
  int    colorMask;
};

struct lefiGeomPath {
  int     numPoints;
  double* x;
  double* y;
  int     colorMask;
};

struct lefiGeomPathIter {
  int     numPoints;
  double* x;
  double* y;
  double xStart;
  double yStart;
  double xStep;
  double yStep;
  int    colorMask;
};

struct lefiGeomPolygon {
  int     numPoints;
  double* x;
  double* y;
  int     colorMask;
  lefiGeomPolygon();
  LEF_COPY_CONSTRUCTOR_H(lefiGeomPolygon);
};

struct lefiGeomPolygonIter {
  int numPoints;
  double* x;
  double* y;
  double xStart;
  double yStart;
  double xStep;
  double yStep;
  int    colorMask;
};

enum lefiGeomEnum {
  lefiGeomUnknown = 0,
  lefiGeomLayerE,
  lefiGeomLayerExceptPgNetE,
  lefiGeomLayerMinSpacingE,
  lefiGeomLayerRuleWidthE,
  lefiGeomWidthE,
  lefiGeomPathE,
  lefiGeomPathIterE,
  lefiGeomRectE,
  lefiGeomRectIterE,
  lefiGeomPolygonE,
  lefiGeomPolygonIterE,
  lefiGeomViaE,
  lefiGeomViaIterE,
  lefiGeomClassE,
  lefiGeomEnd
};

struct lefiGeomVia {
  char*  name;
  double x;
  double y;
  int    topMaskNum;
  int    cutMaskNum;
  int    bottomMaskNum;
};

struct lefiGeomViaIter {
  char*  name;
  double x;
  double y;
  double xStart;
  double yStart;
  double xStep;
  double yStep;
  int    topMaskNum;
  int    cutMaskNum;
  int    bottomMaskNum;
};

class lefiGeometries {
public:
  lefiGeometries();
  void Init();

  LEF_COPY_CONSTRUCTOR_H(lefiGeometries);
  void Destroy();
  ~lefiGeometries();

  void clear();
  void clearPolyItems();
  void add(void* v, lefiGeomEnum e);
  void addLayer(const char* name);
  void addLayerExceptPgNet();                     // 5.7
  void addLayerMinSpacing(double spacing);
  void addLayerRuleWidth(double width);
  void addClass(const char* name);
  void addWidth(double w);
  void addPath(int colorMask);
  void addPathIter(int colorMask);
/*  pcr 481783 & 560504
*/
  void addRect(int colorMask, double xl, double yl, double xh, double yh);
  void addRectIter(int colorMask, double xl, double yl, double xh, double yh);
  void addPolygon(int colorMask = 0);
  void addPolygonIter(int colorMask);
  void addVia(int viaMasks,
              double x, double y, const char* name);
  void addViaIter(int viaMasks,
                  double x, double y, const char* name);
  void addStepPattern(double xStart, double yStart,
                      double xStep, double yStep);
  void startList(double x, double y);
  void addToList(double x, double y);

  int numItems() const;
  lefiGeomEnum itemType(int index) const;
  lefiGeomRect* getRect(int index) const;
  lefiGeomRectIter* getRectIter(int index) const;
  lefiGeomPath* getPath(int index) const;
  lefiGeomPathIter* getPathIter(int index) const;
  int    hasLayerExceptPgNet(int index) const ;     // 5.7
  char*  getLayer(int index) const;
  double getLayerMinSpacing(int index) const;
  double getLayerRuleWidth(int index) const;
  double getWidth(int index) const;
  lefiGeomPolygon* getPolygon(int index) const;
  lefiGeomPolygonIter* getPolygonIter(int index) const;
  char*  getClass(int index) const;
  lefiGeomVia* getVia(int index) const;
  lefiGeomViaIter* getViaIter(int index) const;

  void print(FILE* f) const;

protected:

  int numItems_;
  int itemsAllocated_;
  lefiGeomEnum* itemType_;
  void** items_;

  int numPoints_;
  int pointsAllocated_;
  double* x_;
  double* y_;

  double xStart_;
  double yStart_;
  double xStep_;
  double yStep_;
};

class lefiSpacing {
public:
  lefiSpacing();
  void Init();

  void Destroy();
  ~lefiSpacing();

  lefiSpacing* clone();

  void set(const char* name1, const char* name2, double num, int hasStack);

  int hasStack() const;

  const char* name1() const;
  const char* name2() const;
  double distance() const;

  // Debug print
  void print(FILE* f) const;

protected:
  int    name1Size_;
  int    name2Size_;
  char*  name1_;
  char*  name2_;
  double distance_;
  int    hasStack_;
};

class lefiIRDrop {
public:
  lefiIRDrop();
  void Init();

  void Destroy();
  ~lefiIRDrop();

  void clear();
  void setTableName(const char* name);
  void setValues(double name1, double name2);

  const char* name() const;
  double value1(int index) const;
  double value2(int index) const;

  int numValues() const;

  // Debug print
  void print(FILE* f) const;

protected:
  int     nameSize_;
  int     value1Size_;
  int     value2Size_;
  int     numValues_;
  int     valuesAllocated_;
  char*   name_;
  double* value1_;
  double* value2_;
};

class lefiMinFeature {
public:
  lefiMinFeature();
  void Init();

  void Destroy();
  ~lefiMinFeature();

  void set(double one, double two);

  double one() const;
  double two() const;

  // Debug print
  void print(FILE* f) const;

protected:
  double one_;
  double two_;
};

class lefiSite {
public:
  lefiSite();
  void Init();

  LEF_COPY_CONSTRUCTOR_H( lefiSite );
  LEF_ASSIGN_OPERATOR_H( lefiSite );

  void Destroy();
  ~lefiSite();

  void setName(const char* name);
  void setClass(const char* cls);
  void setSize(double x, double y);
  void setXSymmetry();
  void setYSymmetry();
  void set90Symmetry();
  void addRowPattern(const char* name, int orient);

  const char* name() const;
  int hasClass() const;
  const char* siteClass() const;
  double sizeX() const;
  double sizeY() const;
  int hasSize() const;
  int hasXSymmetry() const;
  int hasYSymmetry() const;
  int has90Symmetry() const;
  int hasRowPattern() const;                 // 5.6
  int numSites() const;                      // 5.6
  char* siteName(int index) const;           // 5.6
  int   siteOrient(int index) const;         // 5.6
  char* siteOrientStr(int index) const;      // 5.6

  // Debug print
  void print(FILE* f) const;

protected:
  int    nameSize_;
  char*  name_;
  int    hasClass_;
  char   siteClass_[8];
  double sizeX_;
  double sizeY_;
  int    hasSize_;
  int    symmetry_;   // bit 0-x   bit 1-y   bit 2-90

  int    numRowPattern_;         // 5.6 ROWPATTERN
  int    rowPatternAllocated_;
  char** siteNames_;
  int*   siteOrients_;
};

class lefiSitePattern {
public:
  lefiSitePattern();
  void Init();

  LEF_COPY_CONSTRUCTOR_H(lefiSitePattern);
  void Destroy();
  ~lefiSitePattern();

  void set(const char* name, double x, double y, int orient,
       double xStart, double yStart, double xStep, double yStep);

  const  char* name() const;
  int    orient() const;
  const  char* orientStr() const;
  double x() const;
  double y() const;
  int    hasStepPattern() const;    // 5.6
  double xStart() const;
  double yStart() const;
  double xStep() const;
  double yStep() const;

  // Debug print
  void print(FILE* f) const;

protected:
  int    nameSize_;
  char*  name_;
  int    orient_;
  double x_;
  double y_;
  double xStart_;
  double yStart_;
  double xStep_;
  double yStep_;
};

class lefiTrackPattern {
public:
  lefiTrackPattern();
  void Init();

  void Destroy();
  ~lefiTrackPattern();

  void clear();
  void set(const char* name, double start, int numTracks, double space);
  void addLayer(const char* name);

  const char* name() const;
  double start() const;
  int numTracks() const;
  double space() const;

  int numLayers() const;
  const char* layerName(int index) const;

  // Debug print
  void print(FILE* f) const;

protected:
  int    nameSize_;
  char*  name_;
  double start_;
  int    numTracks_;
  double space_;

  int    numLayers_;
  int    layerAllocated_;
  char** layerNames_;
};

class lefiGcellPattern {
public:
  lefiGcellPattern();
  void Init();

  void Destroy();
  ~lefiGcellPattern();

  void set(const char* name, double start, int numCRs, double space);

  const char* name() const;
  double start() const;
  int numCRs() const;
  double space() const;

  // Debug print
  void print(FILE* f) const;

protected:
  int    nameSize_;
  char*  name_;
  double start_;
  int    numCRs_;
  double space_;
};

class lefiUseMinSpacing {
public:
  lefiUseMinSpacing();
  void Init();

  void Destroy();
  ~lefiUseMinSpacing();

  void set(const char* name, int value);

  const char* name() const;
  int   value() const;

  // Debug print
  void print(FILE* f) const;

protected:
  char* name_;
  int   value_;
};

// 5.5 for Maximum Stacked-via rule
class lefiMaxStackVia {
public:
  lefiMaxStackVia();
  void Init();

  void Destroy();
  ~lefiMaxStackVia();

  void clear();
  void setMaxStackVia(int value);
  void setMaxStackViaRange(const char* bottomLayer, const char* topLayer);

  int maxStackVia() const;
  int hasMaxStackViaRange() const;
  const char* maxStackViaBottomLayer() const;
  const char* maxStackViaTopLayer() const;

  // Debug print
  void print(FILE* f) const;

protected:
  int   value_;
  int   hasRange_;
  char* bottomLayer_;
  char* topLayer_;
};

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif

