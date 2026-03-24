// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2013, Cadence Design Systems
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
//  $Date: 2020/09/29 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#include "lefiVia.hpp"

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "lefiDebug.hpp"
#include "lefiKRDefs.hpp"
#include "lefiUtil.hpp"
#include "lex.h"

BEGIN_LEF_PARSER_NAMESPACE

// *****************************************************************************
// lefiViaLayer
// *****************************************************************************

lefiViaLayer::lefiViaLayer()
{
  Init();
}

void lefiViaLayer::Init()
{
  name_ = nullptr;
  rectsAllocated_ = 2;
  numRects_ = 0;
  rectColorMask_ = (int*) lefMalloc(sizeof(int) * 2);
  polyColorMask_ = (int*) lefMalloc(sizeof(int) * 2);
  xl_ = (double*) lefMalloc(sizeof(double) * 2);
  yl_ = (double*) lefMalloc(sizeof(double) * 2);
  xh_ = (double*) lefMalloc(sizeof(double) * 2);
  yh_ = (double*) lefMalloc(sizeof(double) * 2);
  polysAllocated_ = 2;
  numPolys_ = 0;
  polygons_ = ((lefiGeomPolygon**) lefMalloc(sizeof(lefiGeomPolygon*) * 2));
}

void lefiViaLayer::Destroy()
{
  if (xl_) {
    lefFree(xl_);
    lefFree(yl_);
    lefFree(xh_);
    lefFree(yh_);
  }
  if (polygons_) {
    lefiGeomPolygon* geom;
    for (int i = 0; i < numPolys_; i++) {
      geom = polygons_[i];
      lefFree(geom->x);
      lefFree(geom->y);
      lefFree(polygons_[i]);
    }
    lefFree(polygons_);

    polygons_ = nullptr;
    numPolys_ = 0;
  }

  lefFree(name_);
  lefFree(rectColorMask_);
  lefFree(polyColorMask_);
}

lefiViaLayer::~lefiViaLayer()
{
  Destroy();
}

void lefiViaLayer::setName(const char* name)
{
  int len = strlen(name) + 1;
  name_ = (char*) lefMalloc(len);
  strcpy(name_, CASE(name));
}

void lefiViaLayer::addRect(int colorMask,
                           double xl,
                           double yl,
                           double xh,
                           double yh)
{
  if (numRects_ == rectsAllocated_) {
    int i;
    double* newxl;
    double* newyl;
    double* newxh;
    double* newyh;
    int* rectMask;

    rectsAllocated_ = (rectsAllocated_ == 0) ? 2 : rectsAllocated_ * 2;
    newxl = (double*) lefMalloc(sizeof(double) * rectsAllocated_);
    newyl = (double*) lefMalloc(sizeof(double) * rectsAllocated_);
    newxh = (double*) lefMalloc(sizeof(double) * rectsAllocated_);
    newyh = (double*) lefMalloc(sizeof(double) * rectsAllocated_);
    rectMask = (int*) lefMalloc(sizeof(int) * rectsAllocated_);

    for (i = 0; i < numRects_; i++) {
      newxl[i] = xl_[i];
      newyl[i] = yl_[i];
      newxh[i] = xh_[i];
      newyh[i] = yh_[i];
      rectMask[i] = rectColorMask_[i];
    }

    if (xl_) {
      lefFree(xl_);
      lefFree(yl_);
      lefFree(xh_);
      lefFree(yh_);
      lefFree(rectColorMask_);
    }

    xl_ = newxl;
    yl_ = newyl;
    xh_ = newxh;
    yh_ = newyh;
    rectColorMask_ = rectMask;
  }

  xl_[numRects_] = xl;
  yl_[numRects_] = yl;
  xh_[numRects_] = xh;
  yh_[numRects_] = yh;
  rectColorMask_[numRects_] = colorMask;

  numRects_ += 1;
}

void lefiViaLayer::addPoly(int colorMask, lefiGeometries* geom)
{
  if (numPolys_ == polysAllocated_) {
    int i;
    lefiGeomPolygon** poly;
    int* polyMask;

    polysAllocated_ = (polysAllocated_ == 0) ? 2 : polysAllocated_ * 2;
    poly = (lefiGeomPolygon**) lefMalloc(sizeof(lefiGeomPolygon*)
                                         * polysAllocated_);
    polyMask = (int*) lefMalloc(sizeof(int) * polysAllocated_);

    for (i = 0; i < numPolys_; i++) {
      poly[i] = polygons_[i];
      polyMask[i] = polyColorMask_[i];
    }

    if (polygons_) {
      lefFree(polygons_);
      lefFree(polyColorMask_);
    }

    polygons_ = poly;
    polyColorMask_ = polyMask;
  }

  polygons_[numPolys_] = geom->getPolygon(0);
  polyColorMask_[numPolys_] = colorMask;

  numPolys_ += 1;
}

int lefiViaLayer::numRects()
{
  return numRects_;
}

int lefiViaLayer::numPolygons()
{
  return numPolys_;
}

lefiViaLayer* lefiViaLayer::clone()
{
  lefiViaLayer* vl = (lefiViaLayer*) lefMalloc(sizeof(lefiViaLayer));
  int i, j;
  if (numRects_ > 0) {
    vl->xl_ = (double*) lefMalloc(sizeof(double) * numRects_);
    vl->yl_ = (double*) lefMalloc(sizeof(double) * numRects_);
    vl->xh_ = (double*) lefMalloc(sizeof(double) * numRects_);
    vl->yh_ = (double*) lefMalloc(sizeof(double) * numRects_);
    vl->rectColorMask_ = (int*) lefMalloc(sizeof(int) * numRects_);
    vl->numRects_ = numRects_;
    vl->rectsAllocated_ = rectsAllocated_;

    for (i = 0; i < numRects_; i++) {
      vl->xl_[i] = xl_[i];
      vl->yl_[i] = yl_[i];
      vl->xh_[i] = xh_[i];
      vl->yh_[i] = yh_[i];
      vl->rectColorMask_[i] = rectColorMask_[i];
    }
  } else {
    vl->xl_ = nullptr;
    vl->yl_ = nullptr;
    vl->xh_ = nullptr;
    vl->yh_ = nullptr;
    vl->rectColorMask_ = nullptr;
    vl->numRects_ = numRects_;
    vl->rectsAllocated_ = rectsAllocated_;
  }

  vl->numPolys_ = numPolys_;
  vl->polysAllocated_ = polysAllocated_;
  vl->polygons_ = (lefiGeomPolygon**) lefMalloc(sizeof(lefiGeomPolygon*)
                                                * polysAllocated_);

  if (numPolys_ > 0) {
    vl->polyColorMask_ = (int*) lefMalloc(sizeof(int) * numPolys_);
  } else {
    vl->polyColorMask_ = nullptr;
  }

  for (i = 0; i < numPolys_; i++) {
    vl->polygons_[i] = (lefiGeomPolygon*) lefMalloc(sizeof(lefiGeomPolygon));
    vl->polygons_[i]->numPoints = polygons_[i]->numPoints;
    vl->polygons_[i]->x
        = (double*) lefMalloc(sizeof(double) * polygons_[i]->numPoints);
    vl->polygons_[i]->y
        = (double*) lefMalloc(sizeof(double) * polygons_[i]->numPoints);
    vl->polygons_[i]->colorMask = polygons_[i]->colorMask;
    vl->polyColorMask_[i] = polyColorMask_[i];

    for (j = 0; j < polygons_[i]->numPoints; j++) {
      vl->polygons_[i]->x[j] = polygons_[i]->x[j];
      vl->polygons_[i]->y[j] = polygons_[i]->y[j];
    }
    /*
        vl->polygons_[i] =  polygons_[i];
    */
  }
  vl->name_ = (char*) lefMalloc(strlen(name_) + 1);
  strcpy(vl->name_, name_);
  return vl;
}

char* lefiViaLayer::name()
{
  return name_;
}

int lefiViaLayer::rectColorMask(int index)
{
  char msg[160];

  if (index < 0 || index >= numRects_) {
    sprintf(msg,
            "ERROR (LEFPARS-1420): The index number %d given for the VIA LAYER "
            "RECTANGLE is invalid.\nValid index is from 0 to %d",
            index,
            numRects_);
    lefiError(0, 1420, msg);
    return 0;
  }

  return rectColorMask_[index];
}

int lefiViaLayer::polyColorMask(int index)
{
  char msg[160];

  if (index < 0 || index >= numPolys_) {
    sprintf(msg,
            "ERROR (LEFPARS-1420): The index number %d given for the VIA LAYER "
            "POLYGON is invalid.\nValid index is from 0 to %d",
            index,
            numPolys_);
    lefiError(0, 1420, msg);
    return 0;
  }

  return polyColorMask_[index];
}

double lefiViaLayer::xl(int index)
{
  char msg[160];
  if (index < 0 || index >= numRects_) {
    sprintf(msg,
            "ERROR (LEFPARS-1420): The index number %d given for the VIA LAYER "
            "RECTANGLE is invalid.\nValid index is from 0 to %d",
            index,
            numRects_);
    lefiError(0, 1420, msg);
    return 0;
  }
  return xl_[index];
}

double lefiViaLayer::yl(int index)
{
  char msg[160];
  if (index < 0 || index >= numRects_) {
    sprintf(msg,
            "ERROR (LEFPARS-1420): The index number %d given for the VIA LAYER "
            "RECTANGLE is invalid.\nValid index is from 0 to %d",
            index,
            numRects_);
    lefiError(0, 1420, msg);
    return 0;
  }
  return yl_[index];
}

double lefiViaLayer::xh(int index)
{
  char msg[160];
  if (index < 0 || index >= numRects_) {
    sprintf(msg,
            "ERROR (LEFPARS-1420): The index number %d given for the VIA LAYER "
            "RECTANGLE is invalid.\nValid index is from 0 to %d",
            index,
            numRects_);
    lefiError(0, 1420, msg);
    return 0;
  }
  return xh_[index];
}

double lefiViaLayer::yh(int index)
{
  char msg[160];
  if (index < 0 || index >= numRects_) {
    sprintf(msg,
            "ERROR (LEFPARS-1420): The index number %d given for the VIA LAYER "
            "RECTANGLE is invalid.\nValid index is from 0 to %d",
            index,
            numRects_);
    lefiError(0, 1420, msg);
    return 0;
  }
  return yh_[index];
}

lefiGeomPolygon* lefiViaLayer::getPolygon(int index) const
{
  return polygons_[index];
}

// *****************************************************************************
// lefiVia
// *****************************************************************************

lefiVia::lefiVia()
{
  Init();
}

void lefiVia::Init()
{
  nameSize_ = 16;
  name_ = (char*) lefMalloc(16);
  foreign_ = nullptr;
  numProps_ = 0;
  propsAllocated_ = 0;
  layersAllocated_ = 3;
  layers_ = (lefiViaLayer**) lefMalloc(sizeof(lefiViaLayer*) * 3);
  numLayers_ = 0;
  clear();
  viaRuleName_ = nullptr;
}

void lefiVia::Destroy()
{
  clear();
  lefFree(name_);
  if (layers_) {
    lefFree(layers_);
  }
  layers_ = nullptr;
  if (propName_) {
    lefFree(propName_);
  }
  if (propValue_) {
    lefFree(propValue_);
  }
  if (propDValue_) {
    lefFree(propDValue_);
  }
  if (propType_) {
    lefFree(propType_);
  }
  if (viaRuleName_) {
    lefFree(viaRuleName_);
  }
  if (botLayer_) {
    lefFree(botLayer_);
  }
  if (cutLayer_) {
    lefFree(cutLayer_);
  }
  if (topLayer_) {
    lefFree(topLayer_);
  }
  if (cutPattern_) {
    lefFree(cutPattern_);
  }
  propName_ = nullptr;
  propValue_ = nullptr;
  propDValue_ = nullptr;
  propType_ = nullptr;
  viaRuleName_ = nullptr;
  botLayer_ = nullptr;
  cutLayer_ = nullptr;
  topLayer_ = nullptr;
  cutPattern_ = nullptr;
}

lefiVia::~lefiVia()
{
  Destroy();
}

lefiVia* lefiVia::clone()
{
  int i;
  lefiViaLayer* l;
  lefiVia* v = (lefiVia*) lefMalloc(sizeof(lefiVia));
  v->nameSize_ = strlen(name_) + 1;
  v->name_ = (char*) lefMalloc(v->nameSize_);
  strcpy(v->name_, name_);
  v->foreign_ = nullptr;
  if (hasForeign()) {
    v->setForeign(
        foreign_, hasForeignPnt(), foreignX_, foreignY_, foreignOrient_);
  }
  v->hasDefault_ = hasDefault_;
  v->hasGenerated_ = hasGenerated_;
  v->hasResistance_ = hasResistance_;
  v->hasForeignPnt_ = hasForeignPnt_;
  v->hasTopOfStack_ = hasTopOfStack_;
  v->numProps_ = numProps_;
  v->propsAllocated_ = numProps_;
  if (numProps_ > 0) {
    v->propName_ = (char**) lefMalloc(sizeof(char*) * numProps_);
    v->propValue_ = (char**) lefMalloc(sizeof(char*) * numProps_);
    v->propDValue_ = (double*) lefMalloc(sizeof(double) * numProps_);
    v->propType_ = (char*) lefMalloc(sizeof(char) * numProps_);
    for (i = 0; i < numProps_; i++) {
      v->propName_[i] = (char*) lefMalloc(strlen(propName_[i]) + 1);
      strcpy(v->propName_[i], propName_[i]);
      // Modified 8/27/99 - Wanda da Rosa for pcr 274891
      // propValue_[i] can be null, if propValue was a number
      if (propValue_[i]) {
        v->propValue_[i] = (char*) lefMalloc(strlen(propValue_[i]) + 1);
        strcpy(v->propValue_[i], propValue_[i]);
      } else {
        v->propValue_[i] = nullptr;
      }
      v->propDValue_[i] = propDValue_[i];
      v->propType_[i] = propType_[i];
    }
  } else {
    v->propName_ = nullptr;
    v->propValue_ = nullptr;
    v->propDValue_ = nullptr;
    v->propType_ = nullptr;
  }
  v->layersAllocated_ = layersAllocated_;
  v->numLayers_ = numLayers_;
  if (numLayers_ > 0) {
    v->layers_ = (lefiViaLayer**) lefMalloc(sizeof(lefiViaLayer*) * numLayers_);
  } else {  // still malloc the memory because lefiVia::Init does
    v->layers_ = (lefiViaLayer**) lefMalloc(sizeof(lefiViaLayer*) * 2);
  }
  for (i = 0; i < numLayers_; i++) {
    l = layers_[i];
    v->layers_[i] = l->clone();
  }
  v->resistance_ = resistance_;
  if (foreignOrient_ == 0) {
    v->foreignOrient_ = -1;
  } else {
    v->foreignOrient_ = foreignOrient_;
  }

  v->viaRuleName_ = nullptr;
  v->botLayer_ = nullptr;
  v->cutLayer_ = nullptr;
  v->topLayer_ = nullptr;
  v->cutPattern_ = nullptr;
  if (viaRuleName_) {
    v->viaRuleName_ = strdup(viaRuleName_);
  }
  v->xSize_ = xSize_;
  v->ySize_ = ySize_;
  if (botLayer_) {
    v->botLayer_ = strdup(botLayer_);
  }
  if (cutLayer_) {
    v->cutLayer_ = strdup(cutLayer_);
  }
  if (topLayer_) {
    v->topLayer_ = strdup(topLayer_);
  }
  v->xSpacing_ = xSpacing_;
  v->ySpacing_ = ySpacing_;
  v->xBotEnc_ = xBotEnc_;
  v->yBotEnc_ = yBotEnc_;
  v->xTopEnc_ = xTopEnc_;
  v->yTopEnc_ = yTopEnc_;
  v->numRows_ = numRows_;
  v->numCols_ = numCols_;
  v->xOffset_ = xOffset_;
  v->yOffset_ = yOffset_;
  v->xBotOs_ = xBotOs_;
  v->yBotOs_ = yBotOs_;
  v->xTopOs_ = xTopOs_;
  v->yTopOs_ = yTopOs_;
  if (cutPattern_) {
    v->cutPattern_ = strdup(cutPattern_);
  }

  return v;
}

void lefiVia::clear()
{
  int i;

  if (name_) {
    *(name_) = '\0';
  }
  if (foreign_) {
    lefFree(foreign_);
  }
  foreign_ = nullptr;
  hasDefault_ = 0;
  hasGenerated_ = 0;
  hasResistance_ = 0;
  hasForeignPnt_ = 0;
  hasTopOfStack_ = 0;
  foreignOrient_ = -1;

  for (i = 0; i < numProps_; i++) {
    lefFree(propName_[i]);
    propName_[i] = nullptr;
    if (propValue_[i]) {
      lefFree(propValue_[i]);
    }
    propValue_[i] = nullptr;
    propType_[i] = ' ';
  }
  numProps_ = 0;

  for (i = 0; i < numLayers_; i++) {
    layers_[i]->Destroy();
    lefFree(layers_[i]);
    layers_[i] = nullptr;
  }
  numLayers_ = 0;

  if (viaRuleName_) {
    lefFree(viaRuleName_);
  }
  viaRuleName_ = nullptr;
  xSize_ = 0;
  ySize_ = 0;
  if (botLayer_) {
    lefFree(botLayer_);
  }
  if (cutLayer_) {
    lefFree(cutLayer_);
  }
  if (topLayer_) {
    lefFree(topLayer_);
  }
  botLayer_ = nullptr;
  cutLayer_ = nullptr;
  topLayer_ = nullptr;
  xSpacing_ = 0;
  ySpacing_ = 0;
  xBotEnc_ = 0;
  yBotEnc_ = 0;
  xTopEnc_ = 0;
  yTopEnc_ = 0;
  numRows_ = 0;
  numCols_ = 0;
  xOffset_ = 0;
  yOffset_ = 0;
  xBotOs_ = 0;
  yBotOs_ = 0;
  xTopOs_ = 0;
  yTopOs_ = 0;
  if (cutPattern_) {
    lefFree(cutPattern_);
  }
  cutPattern_ = nullptr;
}

void lefiVia::setName(const char* name, int viaType)
{
  int len;
  // setName calls clear to init
  // default=0 no default specified
  // default=1 default specified in lef file
  clear();
  switch (viaType) {
    case 1:
      hasDefault_ = 1;
      break;
    case 2:
      hasGenerated_ = 1;
      break;
  }

  len = strlen(name) + 1;
  if (len > nameSize_) {
    lefFree(name_);
    name_ = (char*) lefMalloc(len);
    nameSize_ = len;
  }
  strcpy(name_, CASE(name));
}

void lefiVia::setResistance(double num)
{
  hasResistance_ = 1;
  resistance_ = num;
}

void lefiVia::bumpProps()
{
  int i;
  double* d;
  char** n;
  char** v;
  char* t;

  if (propsAllocated_ == 0) {
    propsAllocated_ = 2;
  } else {
    propsAllocated_ *= 2;
  }

  d = (double*) lefMalloc(sizeof(double) * propsAllocated_);
  n = (char**) lefMalloc(sizeof(char*) * propsAllocated_);
  v = (char**) lefMalloc(sizeof(char*) * propsAllocated_);
  t = (char*) lefMalloc(sizeof(char) * propsAllocated_);

  for (i = 0; i < numProps_; i++) {
    d[i] = propDValue_[i];
    n[i] = propName_[i];
    v[i] = propValue_[i];
    t[i] = propType_[i];
  }

  if (numProps_ > 0) {
    lefFree(propDValue_);
    lefFree(propName_);
    lefFree(propValue_);
    lefFree(propType_);
  }

  propDValue_ = d;
  propName_ = n;
  propValue_ = v;
  propType_ = t;
}

void lefiVia::addProp(const char* name, const char* value, const char type)
{
  int len = strlen(name) + 1;

  if (numProps_ == propsAllocated_) {
    bumpProps();
  }

  propName_[numProps_] = (char*) lefMalloc(len);
  strcpy(propName_[numProps_], CASE(name));

  len = strlen(value) + 1;
  propValue_[numProps_] = (char*) lefMalloc(len);
  strcpy(propValue_[numProps_], CASE(value));

  propDValue_[numProps_] = 0.0;

  propType_[numProps_] = type;

  numProps_ += 1;
}

void lefiVia::addNumProp(const char* name,
                         double d,
                         const char* value,
                         const char type)
{
  int len = strlen(name) + 1;

  if (numProps_ == propsAllocated_) {
    bumpProps();
  }

  propName_[numProps_] = (char*) lefMalloc(len);
  strcpy(propName_[numProps_], CASE(name));

  len = strlen(value) + 1;
  propValue_[numProps_] = (char*) lefMalloc(len);
  strcpy(propValue_[numProps_], CASE(value));

  propDValue_[numProps_] = d;

  propType_[numProps_] = type;

  numProps_ += 1;
}

void lefiVia::setForeign(const char* name,
                         int hasPnt,
                         double x,
                         double y,
                         int orient)
{
  // orient=-1 means no orient was specified.
  int len = strlen(name) + 1;

  hasForeignPnt_ = hasPnt;
  foreignOrient_ = orient;
  foreignX_ = x;
  foreignY_ = y;

  foreign_ = (char*) lefMalloc(len);
  strcpy(foreign_, CASE(name));
}

void lefiVia::setTopOfStack()
{
  hasTopOfStack_ = 1;
}

void lefiVia::addLayer(const char* name)
{
  lefiViaLayer* newl;
  if (numLayers_ == layersAllocated_) {
    int i;
    lefiViaLayer** l;
    if (layersAllocated_ == 0) {
      layersAllocated_ = 2;
    } else {
      layersAllocated_ *= 2;
    }
    l = (lefiViaLayer**) lefMalloc(sizeof(lefiViaLayer*) * layersAllocated_);
    for (i = 0; i < numLayers_; i++) {
      l[i] = layers_[i];
    }
    lefFree(layers_);
    layers_ = l;
  }
  newl = (lefiViaLayer*) lefMalloc(sizeof(lefiViaLayer));
  newl->Init();
  layers_[numLayers_] = newl;
  newl->setName(name);
  numLayers_ += 1;
}

void lefiVia::addRectToLayer(int mask,
                             double xl,
                             double yl,
                             double xh,
                             double yh)
{
  layers_[numLayers_ - 1]->addRect(mask, xl, yl, xh, yh);
}

void lefiVia::addPolyToLayer(int mask, lefiGeometries* geom)
{
  layers_[numLayers_ - 1]->addPoly(mask, geom);
}

void lefiVia::setViaRule(const char* viaRuleName,
                         double xSize,
                         double ySize,
                         const char* botLayer,
                         const char* cutLayer,
                         const char* topLayer,
                         double xCut,
                         double yCut,
                         double xBotEnc,
                         double yBotEnc,
                         double xTopEnc,
                         double yTopEnc)
{
  viaRuleName_ = strdup(viaRuleName);
  xSize_ = xSize;
  ySize_ = ySize;
  botLayer_ = strdup(botLayer);
  cutLayer_ = strdup(cutLayer);
  topLayer_ = strdup(topLayer);
  xSpacing_ = xCut;
  ySpacing_ = yCut;
  xBotEnc_ = xBotEnc;
  yBotEnc_ = yBotEnc;
  xTopEnc_ = xTopEnc;
  yTopEnc_ = yTopEnc;
}

void lefiVia::setRowCol(int numRows, int numCols)
{
  numRows_ = numRows;
  numCols_ = numCols;
}

void lefiVia::setOrigin(double xOffset, double yOffset)
{
  xOffset_ = xOffset;
  yOffset_ = yOffset;
}

void lefiVia::setOffset(double xBot, double yBot, double xTop, double yTop)
{
  xBotOs_ = xBot;
  yBotOs_ = yBot;
  xTopOs_ = xTop;
  yTopOs_ = yTop;
}

void lefiVia::setPattern(const char* cutPattern)
{
  cutPattern_ = strdup(cutPattern);
}

int lefiVia::hasDefault() const
{
  return hasDefault_;
}

int lefiVia::hasGenerated() const
{
  return hasGenerated_;
}

int lefiVia::hasForeign() const
{
  return foreign_ ? 1 : 0;
}

int lefiVia::hasForeignPnt() const
{
  return hasForeignPnt_;
}

int lefiVia::hasForeignOrient() const
{
  return foreignOrient_ == -1 ? 0 : 1;
}

int lefiVia::hasProperties() const
{
  return numProps_ ? 1 : 0;
}

int lefiVia::hasResistance() const
{
  return hasResistance_;
}

int lefiVia::hasTopOfStack() const
{
  return hasTopOfStack_;
}

// 5.6
int lefiVia::hasViaRule() const
{
  return viaRuleName_ ? 1 : 0;
}

// 5.6
int lefiVia::hasRowCol() const
{
  if (numRows_ != 0 || numCols_ != 0) {
    return 1;
  }
  return 0;
}

// 5.6
int lefiVia::hasOrigin() const
{
  if (xOffset_ != 0 || yOffset_ != 0) {
    return 1;
  }
  return 0;
}

// 5.6
int lefiVia::hasOffset() const
{
  if (xBotOs_ != 0 || yBotOs_ != 0 || xTopOs_ != 0 || yTopOs_ != 0) {
    return 1;
  }
  return 0;
}

// 5.6
int lefiVia::hasCutPattern() const
{
  return cutPattern_ ? 1 : 0;
}

int lefiVia::numLayers() const
{
  return numLayers_;
}

char* lefiVia::layerName(int layerNum) const
{
  lefiViaLayer* vl;
  char msg[160];
  if (layerNum < 0 || layerNum >= numLayers_) {
    sprintf(msg,
            "ERROR (LEFPARS-1421): The layer number %d given for the VIA LAYER "
            "is invalid.\nValid number is from 0 to %d",
            layerNum,
            numLayers_);
    lefiError(0, 1421, msg);
    return nullptr;
  }
  vl = layers_[layerNum];
  return vl->name();
}

int lefiVia::numRects(int layerNum) const
{
  lefiViaLayer* vl;
  char msg[160];
  if (layerNum < 0 || layerNum >= numLayers_) {
    sprintf(msg,
            "ERROR (LEFPARS-1421): The layer number %d given for the VIA LAYER "
            "is invalid.\nValid number is from 0 to %d",
            layerNum,
            numLayers_);
    lefiError(0, 1421, msg);
    return 0;
  }
  vl = layers_[layerNum];
  return vl->numRects();
}

double lefiVia::xl(int layerNum, int rectNum) const
{
  lefiViaLayer* vl;
  char msg[160];
  if (layerNum < 0 || layerNum >= numLayers_) {
    sprintf(msg,
            "ERROR (LEFPARS-1421): The layer number %d given for the VIA LAYER "
            "is invalid.\nValid number is from 0 to %d",
            layerNum,
            numLayers_);
    lefiError(0, 1421, msg);
    return 0;
  }
  vl = layers_[layerNum];
  return vl->xl(rectNum);
}

double lefiVia::yl(int layerNum, int rectNum) const
{
  lefiViaLayer* vl;
  char msg[160];
  if (layerNum < 0 || layerNum >= numLayers_) {
    sprintf(msg,
            "ERROR (LEFPARS-1421): The layer number %d given for the VIA LAYER "
            "is invalid.\nValid number is from 0 to %d",
            layerNum,
            numLayers_);
    lefiError(0, 1421, msg);
    return 0;
  }
  vl = layers_[layerNum];
  return vl->yl(rectNum);
}

double lefiVia::xh(int layerNum, int rectNum) const
{
  lefiViaLayer* vl;
  char msg[160];
  if (layerNum < 0 || layerNum >= numLayers_) {
    sprintf(msg,
            "ERROR (LEFPARS-1421): The layer number %d given for the VIA LAYER "
            "is invalid.\nValid number is from 0 to %d",
            layerNum,
            numLayers_);
    lefiError(0, 1421, msg);
    return 0;
  }
  vl = layers_[layerNum];
  return vl->xh(rectNum);
}

double lefiVia::yh(int layerNum, int rectNum) const
{
  lefiViaLayer* vl;
  char msg[160];
  if (layerNum < 0 || layerNum >= numLayers_) {
    sprintf(msg,
            "ERROR (LEFPARS-1421): The layer number %d given for the VIA LAYER "
            "is invalid.\nValid number is from 0 to %d",
            layerNum,
            numLayers_);
    lefiError(0, 1421, msg);
    return 0;
  }
  vl = layers_[layerNum];
  return vl->yh(rectNum);
}

int lefiVia::rectColorMask(int layerNum, int rectNum) const
{
  lefiViaLayer* vl;
  char msg[160];

  if (layerNum < 0 || layerNum >= numLayers_) {
    sprintf(msg,
            "ERROR (LEFPARS-1421): The layer number %d given for the VIA LAYER "
            "is invalid.\nValid number is from 0 to %d",
            layerNum,
            numLayers_);
    lefiError(0, 1421, msg);
    return 0;
  }

  vl = layers_[layerNum];

  return vl->rectColorMask(rectNum);
}

int lefiVia::polyColorMask(int layerNum, int polyNum) const
{
  lefiViaLayer* vl;
  char msg[160];

  if (layerNum < 0 || layerNum >= numLayers_) {
    sprintf(msg,
            "ERROR (LEFPARS-1421): The layer number %d given for the VIA LAYER "
            "is invalid.\nValid number is from 0 to %d",
            layerNum,
            numLayers_);
    lefiError(0, 1421, msg);
    return 0;
  }

  vl = layers_[layerNum];

  return vl->polyColorMask(polyNum);
}

int lefiVia::numPolygons(int layerNum) const
{
  lefiViaLayer* vl;
  char msg[160];
  if (layerNum < 0 || layerNum >= numLayers_) {
    sprintf(msg,
            "ERROR (LEFPARS-1421): The layer number %d given for the VIA LAYER "
            "is invalid.\nValid number is from 0 to %d",
            layerNum,
            numLayers_);
    lefiError(0, 1421, msg);
    return 0;
  }
  vl = layers_[layerNum];
  return vl->numPolygons();
}

lefiGeomPolygon lefiVia::getPolygon(int layerNum, int polyNum) const
{
  lefiViaLayer* vl;
  char msg[160];
  lefiGeomPolygon tempPoly;

  tempPoly.numPoints = 0;
  tempPoly.x = nullptr;
  tempPoly.y = nullptr;
  tempPoly.colorMask = 0;

  if (layerNum < 0 || layerNum >= numLayers_) {
    sprintf(msg,
            "ERROR (LEFPARS-1421): The layer number %d given for the VIA LAYER "
            "is invalid.\nValid number is from 0 to %d",
            layerNum,
            numLayers_);
    lefiError(0, 1421, msg);
    return tempPoly;
  }

  vl = layers_[layerNum];

  return *(vl->getPolygon(polyNum));
}

char* lefiVia::name() const
{
  return name_;
}

double lefiVia::resistance() const
{
  return resistance_;
}

// Given an index from 0 to numProperties()-1 return
// information about that property.
int lefiVia::numProperties() const
{
  return numProps_;
}

char* lefiVia::propName(int index) const
{
  char msg[160];
  if (index < 0 || index >= numProps_) {
    sprintf(msg,
            "ERROR (LEFPARS-1422): The layer number %d given for the VIA "
            "PROPERTY is invalid.\nValid number is from 0 to %d",
            index,
            numLayers_);
    lefiError(0, 1422, msg);
    return nullptr;
  }
  return propName_[index];
}

char* lefiVia::propValue(int index) const
{
  char msg[160];
  if (index < 0 || index >= numProps_) {
    sprintf(msg,
            "ERROR (LEFPARS-1422): The layer number %d given for the VIA "
            "PROPERTY is invalid.\nValid number is from 0 to %d",
            index,
            numLayers_);
    lefiError(0, 1422, msg);
    return nullptr;
  }
  return propValue_[index];
}

double lefiVia::propNumber(int index) const
{
  char msg[160];
  if (index < 0 || index >= numProps_) {
    sprintf(msg,
            "ERROR (LEFPARS-1422): The layer number %d given for the VIA "
            "PROPERTY is invalid.\nValid number is from 0 to %d",
            index,
            numLayers_);
    lefiError(0, 1422, msg);
    return 0;
  }
  return propDValue_[index];
}

char lefiVia::propType(int index) const
{
  char msg[160];
  if (index < 0 || index >= numProps_) {
    sprintf(msg,
            "ERROR (LEFPARS-1422): The layer number %d given for the VIA "
            "PROPERTY is invalid.\nValid number is from 0 to %d",
            index,
            numLayers_);
    lefiError(0, 1422, msg);
    return 0;
  }
  return propType_[index];
}

int lefiVia::propIsNumber(int index) const
{
  char msg[160];
  if (index < 0 || index >= numProps_) {
    sprintf(msg,
            "ERROR (LEFPARS-1422): The layer number %d given for the VIA "
            "PROPERTY is invalid.\nValid number is from 0 to %d",
            index,
            numLayers_);
    lefiError(0, 1422, msg);
    return 0;
  }
  return propDValue_[index] ? 1 : 0;
}

int lefiVia::propIsString(int index) const
{
  char msg[160];
  if (index < 0 || index >= numProps_) {
    sprintf(msg,
            "ERROR (LEFPARS-1422): The layer number %d given for the VIA "
            "PROPERTY is invalid.\nValid number is from 0 to %d",
            index,
            numLayers_);
    lefiError(0, 1422, msg);
    return 0;
  }
  return propDValue_[index] ? 0 : 1;
}

char* lefiVia::foreign() const
{
  return foreign_;
}

double lefiVia::foreignX() const
{
  return foreignX_;
}

double lefiVia::foreignY() const
{
  return foreignY_;
}

int lefiVia::foreignOrient() const
{
  return foreignOrient_;
}

char* lefiVia::foreignOrientStr() const
{
  return (lefiOrientStr(foreignOrient_));
}

// 5.6
const char* lefiVia::viaRuleName() const
{
  return viaRuleName_;
}

double lefiVia::xCutSize() const
{
  return xSize_;
}

double lefiVia::yCutSize() const
{
  return ySize_;
}

const char* lefiVia::botMetalLayer() const
{
  return botLayer_;
}

const char* lefiVia::cutLayer() const
{
  return cutLayer_;
}

const char* lefiVia::topMetalLayer() const
{
  return topLayer_;
}

double lefiVia::xCutSpacing() const
{
  return xSpacing_;
}

double lefiVia::yCutSpacing() const
{
  return ySpacing_;
}

double lefiVia::xBotEnc() const
{
  return xBotEnc_;
}

double lefiVia::yBotEnc() const
{
  return yBotEnc_;
}

double lefiVia::xTopEnc() const
{
  return xTopEnc_;
}

double lefiVia::yTopEnc() const
{
  return yTopEnc_;
}

int lefiVia::numCutRows() const
{
  return numRows_;
}

int lefiVia::numCutCols() const
{
  return numCols_;
}

double lefiVia::xOffset() const
{
  return xOffset_;
}

double lefiVia::yOffset() const
{
  return yOffset_;
}

double lefiVia::xBotOffset() const
{
  return xBotOs_;
}

double lefiVia::yBotOffset() const
{
  return yBotOs_;
}

double lefiVia::xTopOffset() const
{
  return xTopOs_;
}

double lefiVia::yTopOffset() const
{
  return yTopOs_;
}

const char* lefiVia::cutPattern() const
{
  return cutPattern_;
}

// Debug print
void lefiVia::print(FILE* f) const
{
  int i;
  int h;

  fprintf(f, "Via %s:\n", name());

  if (hasDefault()) {
    fprintf(f, "  DEFAULT\n");
  }

  if (hasForeign()) {
    fprintf(f, "  foreign %s", foreign());
    if (hasForeignPnt()) {
      fprintf(f, " %g,%g", foreignX(), foreignY());
    }
    if (hasForeignOrient()) {
      fprintf(f, " orient %s", foreignOrientStr());
    }
    fprintf(f, "\n");
  }

  if (hasResistance()) {
    fprintf(f, "  RESISTANCE %g\n", resistance());
  }

  if (hasProperties()) {
    for (i = 0; i < numProperties(); i++) {
      if (propIsString(i)) {
        fprintf(f, "  PROP %s %s\n", propName(i), propValue(i));
      } else {
        fprintf(f, "  PROP %s %g\n", propName(i), propNumber(i));
      }
    }
  }

  for (i = 0; i < numLayers(); i++) {
    fprintf(f, "  LAYER %s\n", layerName(i));
    for (h = 0; h < numRects(i); h++) {
      if (rectColorMask(i, h) != 0) {
        fprintf(f,
                "    RECT MASK %d %g,%g %g,%g\n",
                rectColorMask(i, h),
                xl(i, h),
                yl(i, h),
                xh(i, h),
                yh(i, h));
      } else {
        fprintf(f,
                "    RECT %g,%g %g,%g\n",
                xl(i, h),
                yl(i, h),
                xh(i, h),
                yh(i, h));
      }
    }
  }
}
END_LEF_PARSER_NAMESPACE
