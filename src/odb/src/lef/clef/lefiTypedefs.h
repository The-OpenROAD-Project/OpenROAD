 /* ***************************************************************************** */
 /* ***************************************************************************** */
 /* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT! */
 /* ***************************************************************************** */
 /* ***************************************************************************** */
 /* Copyright 2012, Cadence Design Systems */
 /*  */
 /* This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source */
 /* Distribution,  Product Version 5.8.  */
 /*  */
 /* Licensed under the Apache License, Version 2.0 (the \"License\"); */
 /*    you may not use this file except in compliance with the License. */
 /*    You may obtain a copy of the License at */
 /*  */
 /*        http://www.apache.org/licenses/LICENSE-2.0 */
 /*  */
 /*    Unless required by applicable law or agreed to in writing, software */
 /*    distributed under the License is distributed on an \"AS IS\" BASIS, */
 /*    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or */
 /*    implied. See the License for the specific language governing */
 /*    permissions and limitations under the License. */
 /*  */
 /*  */
 /* For updates, support, or to become part of the LEF/DEF Community, */
 /* check www.openeda.org for details. */
 /*  */
 /*  $Author: dell $ */
 /*  $Revision: #1 $ */
 /*  $Date: 2017/06/06 $ */
 /*  $State:  $   */
 /* ***************************************************************************** */
 /* ***************************************************************************** */

#ifndef CLEFITYPEDEFS_H
#define CLEFITYPEDEFS_H

#ifndef EXTERN
#define EXTERN extern
#endif

#define bool int
#define lefiUserData void *
#define lefiUserDataHandle void **

/* Typedefs */
typedef struct lefiPoints lefiNum;

/* Pointers to C++ classes */
typedef void *lefiArrayFloorPlan;
typedef void *lefiLayer;
typedef void *lefiNoiseEdge;
typedef void *lefiAntennaModel;
typedef void *lefiCorrectionEdge;
typedef void *lefiTrackPattern;
typedef void *lefiIRDrop;
typedef void *lefiGeometries;
typedef void *lefiSpacingTable;
typedef void *lefiMinFeature;
typedef void *lefiGcellPattern;
typedef void *lefiTiming;
typedef void *lefiNoiseTable;
typedef void *lefiCorrectionVictim;
typedef void *lefiPinAntennaModel;
typedef void *lefiProp;
typedef void *lefiUseMinSpacing;
typedef void *lefiCorrectionResistance;
typedef void *lefiAntennaPWL;
typedef void *lefiArray;
typedef void *lefiNonDefault;
typedef void *lefiLayerDensity;
typedef void *lefiSitePattern;
typedef void *lefiMacro;
typedef void *lefiCorrectionTable;
typedef void *lefiOrthogonal;
typedef void *lefiVia;
typedef void *lefiSite;
typedef void *lefiViaLayer;
typedef void *lefiParallel;
typedef void *lefiNoiseVictim;
typedef void *lefiSpacing;
typedef void *lefiViaRule;
typedef void *lefiViaRuleLayer;
typedef void *lefiTwoWidths;
typedef void *lefiDensity;
typedef void *lefiPin;
typedef void *lefiMaxStackVia;
typedef void *lefiInfluence;
typedef void *lefiUnits;
typedef void *lefiPropType;
typedef void *lefiObstruction;
typedef void *lefiNoiseResistance;

/* Data structures definitions */
struct lefiNoiseMargin {
  double high;
  double low;
};

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

struct lefiPoints {
  double x;
  double y;
};


#endif
