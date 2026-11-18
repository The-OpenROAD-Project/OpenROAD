/* *****************************************************************************
 */
/* *****************************************************************************
 */
/* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT! */
/* *****************************************************************************
 */
/* *****************************************************************************
 */
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
/*  $Date: 2020/09/29 $ */
/*  $State:  $   */
/* *****************************************************************************
 */
/* *****************************************************************************
 */

#ifndef CLEFITYPEDEFS_H
#define CLEFITYPEDEFS_H

#ifndef EXTERN
#define EXTERN extern
#endif

#define bool int
#define lefiUserData void*
#define lefiUserDataHandle void**

/* Typedefs */
using lefiNum = struct lefiPoints;

/* Pointers to C++ classes */
using lefiArrayFloorPlan = void*;
using lefiLayer = void*;
using lefiNoiseEdge = void*;
using lefiAntennaModel = void*;
using lefiCorrectionEdge = void*;
using lefiTrackPattern = void*;
using lefiIRDrop = void*;
using lefiGeometries = void*;
using lefiSpacingTable = void*;
using lefiMinFeature = void*;
using lefiGcellPattern = void*;
using lefiTiming = void*;
using lefiNoiseTable = void*;
using lefiCorrectionVictim = void*;
using lefiPinAntennaModel = void*;
using lefiProp = void*;
using lefiUseMinSpacing = void*;
using lefiCorrectionResistance = void*;
using lefiAntennaPWL = void*;
using lefiArray = void*;
using lefiNonDefault = void*;
using lefiLayerDensity = void*;
using lefiSitePattern = void*;
using lefiMacro = void*;
using lefiCorrectionTable = void*;
using lefiOrthogonal = void*;
using lefiVia = void*;
using lefiSite = void*;
using lefiViaLayer = void*;
using lefiParallel = void*;
using lefiNoiseVictim = void*;
using lefiSpacing = void*;
using lefiViaRule = void*;
using lefiViaRuleLayer = void*;
using lefiTwoWidths = void*;
using lefiDensity = void*;
using lefiPin = void*;
using lefiMaxStackVia = void*;
using lefiInfluence = void*;
using lefiUnits = void*;
using lefiPropType = void*;
using lefiObstruction = void*;
using lefiNoiseResistance = void*;

/* Data structures definitions */
struct lefiNoiseMargin
{
  double high;
  double low;
};

struct lefiGeomRect
{
  double xl;
  double yl;
  double xh;
  double yh;
  int colorMask;
};

struct lefiGeomRectIter
{
  double xl;
  double yl;
  double xh;
  double yh;
  double xStart;
  double yStart;
  double xStep;
  double yStep;
  int colorMask;
};

struct lefiGeomPath
{
  int numPoints;
  double* x;
  double* y;
  int colorMask;
};

struct lefiGeomPathIter
{
  int numPoints;
  double* x;
  double* y;
  double xStart;
  double yStart;
  double xStep;
  double yStep;
  int colorMask;
};

struct lefiGeomPolygon
{
  int numPoints;
  double* x;
  double* y;
  int colorMask;
};

struct lefiGeomPolygonIter
{
  int numPoints;
  double* x;
  double* y;
  double xStart;
  double yStart;
  double xStep;
  double yStep;
  int colorMask;
};

struct lefiGeomVia
{
  char* name;
  double x;
  double y;
  int topMaskNum;
  int cutMaskNum;
  int bottomMaskNum;
};

struct lefiGeomViaIter
{
  char* name;
  double x;
  double y;
  double xStart;
  double yStart;
  double xStep;
  double yStep;
  int topMaskNum;
  int cutMaskNum;
  int bottomMaskNum;
};

struct lefiPoints
{
  double x;
  double y;
};

#endif
