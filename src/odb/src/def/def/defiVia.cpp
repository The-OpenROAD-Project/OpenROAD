// *****************************************************************************
// *****************************************************************************
// Copyright 2013 - 2016, Cadence Design Systems
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

#include "defiVia.hpp"

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "defiDebug.hpp"
#include "defiKRDefs.hpp"
#include "defiMisc.hpp"
#include "defrData.hpp"

BEGIN_DEF_PARSER_NAMESPACE

//////////////////////////////////////////////
//////////////////////////////////////////////
//
//   defiVia
//
//////////////////////////////////////////////
//////////////////////////////////////////////

defiVia::defiVia(defrData* data) : defData(data)
{
  Init();
}

void defiVia::Init()
{
  name_ = nullptr;
  nameLength_ = 0;
  pattern_ = nullptr;
  patternLength_ = 0;
  xl_ = nullptr;
  yl_ = nullptr;
  xh_ = nullptr;
  yh_ = nullptr;
  layersLength_ = 0;
  layers_ = nullptr;
  viaRule_ = nullptr;
  viaRuleLength_ = 0;
  xSize_ = 0;
  ySize_ = 0;
  botLayer_ = nullptr;
  cutLayer_ = nullptr;
  topLayer_ = nullptr;
  botLayerLength_ = 0;
  cutLayerLength_ = 0;
  topLayerLength_ = 0;
  xCutSpacing_ = 0;
  yCutSpacing_ = 0;
  xBotEnc_ = 0;
  yBotEnc_ = 0;
  xTopEnc_ = 0;
  yTopEnc_ = 0;
  cutPattern_ = nullptr;
  cutPatternLength_ = 0;
  numLayers_ = 0;
  numPolys_ = 0;
  polygons_ = nullptr;
  polysAllocated_ = 0;
  polygonNames_ = nullptr;
  rectMask_ = nullptr;
  polyMask_ = nullptr;
  clear();
}

void defiVia::clear()
{
  int i;

  hasPattern_ = 0;
  hasViaRule_ = 0;
  rows_ = 0;
  cols_ = 0;
  xOffset_ = 0;
  yOffset_ = 0;
  xBotOffset_ = 0;
  yBotOffset_ = 0;
  xTopOffset_ = 0;
  yTopOffset_ = 0;
  hasCutPattern_ = 0;

  if (polygonNames_) {
    struct defiPoints* p;
    for (i = 0; i < numPolys_; i++) {
      free(polygonNames_[i]);

      p = polygons_[i];
      free(p->x);
      free(p->y);
      free(p);
    }

    free(polygonNames_);
    free(polygons_);
    free(polyMask_);

    polygonNames_ = nullptr;
    polygons_ = nullptr;
    polyMask_ = nullptr;
  }

  numPolys_ = 0;
  polysAllocated_ = 0;
}

defiVia::~defiVia()
{
  Destroy();
}

void defiVia::Destroy()
{
  int i;

  free(name_);
  name_ = nullptr;

  free(pattern_);
  pattern_ = nullptr;

  if (layers_) {
    for (i = 0; i < numLayers_; i++) {
      free(layers_[i]);
    }

    free(layers_);
    layers_ = nullptr;

    free(xl_);
    xl_ = nullptr;

    free(yl_);
    yl_ = nullptr;

    free(xh_);
    xh_ = nullptr;

    free(yh_);
    yh_ = nullptr;

    free(rectMask_);
    rectMask_ = nullptr;

    free(polyMask_);
    polyMask_ = nullptr;
  }

  free(viaRule_);
  viaRule_ = nullptr;

  free(botLayer_);
  botLayer_ = nullptr;

  free(cutLayer_);
  cutLayer_ = nullptr;

  free(topLayer_);
  topLayer_ = nullptr;

  free(cutPattern_);
  cutPattern_ = nullptr;

  clear();
}

void defiVia::setup(const char* name)
{
  int i;
  int len = strlen(name) + 1;
  if (len > nameLength_) {
    nameLength_ = len;
    name_ = (char*) realloc(name_, len);
  }
  strcpy(name_, defData->DEFCASE(name));
  if (pattern_) {
    *(pattern_) = 0;
  }
  if (layers_) {
    for (i = 0; i < numLayers_; i++) {
      free(layers_[i]);
      layers_[i] = nullptr;
    }
  }

  numLayers_ = 0;
}

void defiVia::addPattern(const char* pattern)
{
  int len = strlen(pattern) + 1;
  if (len > patternLength_) {
    patternLength_ = len;
    pattern_ = (char*) realloc(pattern_, len);
  }
  strcpy(pattern_, defData->DEFCASE(pattern));
  hasPattern_ = 1;
}

void defiVia::addLayer(const char* layer,
                       int xl,
                       int yl,
                       int xh,
                       int yh,
                       int colorMask)
{
  char* l;
  int len;

  if (numLayers_ >= layersLength_) {
    int i;
    char** newl;
    int* ints;
    layersLength_ = layersLength_ ? 2 * layersLength_ : 8;

    newl = (char**) malloc(layersLength_ * sizeof(char*));
    for (i = 0; i < numLayers_; i++) {
      newl[i] = layers_[i];
    }
    if (layers_) {
      free((char*) (layers_));
    }
    layers_ = newl;

    ints = (int*) malloc(layersLength_ * sizeof(int));
    for (i = 0; i < numLayers_; i++) {
      ints[i] = xl_[i];
    }
    if (xl_) {
      free((char*) (xl_));
    }
    xl_ = ints;

    ints = (int*) malloc(layersLength_ * sizeof(int));
    for (i = 0; i < numLayers_; i++) {
      ints[i] = yl_[i];
    }
    if (yl_) {
      free((char*) (yl_));
    }
    yl_ = ints;

    ints = (int*) malloc(layersLength_ * sizeof(int));
    for (i = 0; i < numLayers_; i++) {
      ints[i] = xh_[i];
    }
    if (xh_) {
      free((char*) (xh_));
    }
    xh_ = ints;

    ints = (int*) malloc(layersLength_ * sizeof(int));
    for (i = 0; i < numLayers_; i++) {
      ints[i] = yh_[i];
    }
    if (yh_) {
      free((char*) (yh_));
    }
    yh_ = ints;

    ints = (int*) malloc(layersLength_ * sizeof(int));
    for (i = 0; i < numLayers_; i++) {
      ints[i] = rectMask_[i];
    }
    if (rectMask_) {
      free((char*) (rectMask_));
    }
    rectMask_ = ints;
  }

  len = strlen(layer) + 1;
  l = (char*) malloc(len);
  strcpy(l, defData->DEFCASE(layer));
  layers_[numLayers_] = l;
  xl_[numLayers_] = xl;
  yl_[numLayers_] = yl;
  xh_[numLayers_] = xh;
  yh_[numLayers_] = yh;
  rectMask_[numLayers_] = colorMask;
  numLayers_++;
}

// 5.6
void defiVia::addPolygon(const char* layer, defiGeometries* geom, int colorMask)
{
  struct defiPoints* p;
  int x, y;
  int i;

  if (numPolys_ == polysAllocated_) {
    char** newn;
    int* masks;
    struct defiPoints** poly;
    polysAllocated_ = (polysAllocated_ == 0) ? 2 : polysAllocated_ * 2;
    newn = (char**) malloc(sizeof(char*) * polysAllocated_);
    poly = (struct defiPoints**) malloc(sizeof(struct defiPoints*)
                                        * polysAllocated_);
    masks = (int*) malloc(polysAllocated_ * sizeof(int));
    for (i = 0; i < numPolys_; i++) {
      newn[i] = polygonNames_[i];
      poly[i] = polygons_[i];
      masks[i] = polyMask_[i];
    }
    if (polygons_) {
      free((char*) (polygons_));
    }
    if (polygonNames_) {
      free((char*) (polygonNames_));
    }
    if (polyMask_) {
      free((char*) (polyMask_));
    }
    polygonNames_ = newn;
    polygons_ = poly;
    polyMask_ = masks;
  }
  polygonNames_[numPolys_] = strdup(layer);
  p = (struct defiPoints*) malloc(sizeof(struct defiPoints));
  p->numPoints = geom->numPoints();
  p->x = (int*) malloc(sizeof(int) * p->numPoints);
  p->y = (int*) malloc(sizeof(int) * p->numPoints);
  for (i = 0; i < p->numPoints; i++) {
    geom->points(i, &x, &y);
    p->x[i] = x;
    p->y[i] = y;
  }
  polygons_[numPolys_] = p;
  polyMask_[numPolys_] = colorMask;
  numPolys_ += 1;
}

void defiVia::addViaRule(char* viaRuleName,
                         int xSize,
                         int ySize,
                         char* botLayer,
                         char* cutLayer,
                         char* topLayer,
                         int xSpacing,
                         int ySpacing,
                         int xBotEnc,
                         int yBotEnc,
                         int xTopEnc,
                         int yTopEnc)
{
  int len;

  len = strlen(viaRuleName) + 1;
  if (len > viaRuleLength_) {
    if (viaRule_) {
      free(viaRule_);
    }
    viaRule_ = (char*) malloc(strlen(viaRuleName) + 1);
  }
  strcpy(viaRule_, defData->DEFCASE(viaRuleName));
  xSize_ = xSize;
  ySize_ = ySize;
  len = strlen(botLayer) + 1;
  if (len > botLayerLength_) {
    if (botLayer_) {
      free(botLayer_);
    }
    botLayer_ = (char*) malloc(strlen(botLayer) + 1);
    botLayerLength_ = len;
  }
  strcpy(botLayer_, defData->DEFCASE(botLayer));
  len = strlen(cutLayer) + 1;
  if (len > cutLayerLength_) {
    if (cutLayer_) {
      free(cutLayer_);
    }
    cutLayer_ = (char*) malloc(strlen(cutLayer) + 1);
    cutLayerLength_ = len;
  }
  strcpy(cutLayer_, defData->DEFCASE(cutLayer));
  len = strlen(topLayer) + 1;
  if (len > topLayerLength_) {
    if (topLayer_) {
      free(topLayer_);
    }
    topLayer_ = (char*) malloc(strlen(topLayer) + 1);
    topLayerLength_ = len;
  }
  strcpy(topLayer_, defData->DEFCASE(topLayer));
  xCutSpacing_ = xSpacing;
  yCutSpacing_ = ySpacing;
  xBotEnc_ = xBotEnc;
  yBotEnc_ = yBotEnc;
  xTopEnc_ = xTopEnc;
  yTopEnc_ = yTopEnc;
  hasViaRule_ = 1;
}

void defiVia::addRowCol(int numCutRows, int numCutCols)
{
  rows_ = numCutRows;
  cols_ = numCutCols;
}

void defiVia::addOrigin(int xOffset, int yOffset)
{
  xOffset_ = xOffset;
  yOffset_ = yOffset;
}

void defiVia::addOffset(int xBotOs, int yBotOs, int xTopOs, int yTopOs)
{
  xBotOffset_ = xBotOs;
  yBotOffset_ = yBotOs;
  xTopOffset_ = xTopOs;
  yTopOffset_ = yTopOs;
}

void defiVia::addCutPattern(char* cutPattern)
{
  int len;

  len = strlen(cutPattern) + 1;
  if (len > cutPatternLength_) {
    if (cutPattern_) {
      free(cutPattern_);
    }
    cutPattern_ = (char*) malloc(strlen(cutPattern) + 1);
    cutPatternLength_ = len;
  }
  strcpy(cutPattern_, defData->DEFCASE(cutPattern));
  hasCutPattern_ = 1;
}

int defiVia::hasPattern() const
{
  return hasPattern_;
}

const char* defiVia::pattern() const
{
  return pattern_;
}

const char* defiVia::name() const
{
  return name_;
}

int defiVia::numLayers() const
{
  return numLayers_;
}

int defiVia::rectMask(int index) const
{
  if (index >= 0 && index < numLayers_) {
    return rectMask_[index];
  }

  return 0;
}

int defiVia::polyMask(int index) const
{
  if (index >= 0 && index < numPolys_) {
    return polyMask_[index];
  }

  return 0;
}

void defiVia::layer(int index, char** layer, int* xl, int* yl, int* xh, int* yh)
    const
{
  if (index >= 0 && index < numLayers_) {
    if (layer) {
      *layer = layers_[index];
    }
    if (xl) {
      *xl = xl_[index];
    }
    if (yl) {
      *yl = yl_[index];
    }
    if (xh) {
      *xh = xh_[index];
    }
    if (yh) {
      *yh = yh_[index];
    }
  }
}

// The following code is for 5.6

int defiVia::numPolygons() const
{
  return numPolys_;
}

const char* defiVia::polygonName(int index) const
{
  char msg[160];
  if (index < 0 || index > numPolys_) {
    sprintf(msg,
            "ERROR (DEFPARS-6180): The index number %d specified for the VIA "
            "POLYGON is invalid.\nValid index is from 0 to %d. Specify a valid "
            "index number and then try again",
            index,
            numPolys_);
    defiError(0, 6180, msg, defData);
    return nullptr;
  }
  return polygonNames_[index];
}

struct defiPoints defiVia::getPolygon(int index) const
{
  return *(polygons_[index]);
}

int defiVia::hasViaRule() const
{
  return hasViaRule_;
}

void defiVia::viaRule(char** viaRuleName,
                      int* xSize,
                      int* ySize,
                      char** botLayer,
                      char** cutLayer,
                      char** topLayer,
                      int* xCutSpacing,
                      int* yCutSpacing,
                      int* xBotEnc,
                      int* yBotEnc,
                      int* xTopEnc,
                      int* yTopEnc) const
{
  *viaRuleName = viaRule_;
  *xSize = xSize_;
  *ySize = ySize_;
  *botLayer = botLayer_;
  *cutLayer = cutLayer_;
  *topLayer = topLayer_;
  *xCutSpacing = xCutSpacing_;
  *yCutSpacing = yCutSpacing_;
  *xBotEnc = xBotEnc_;
  *yBotEnc = yBotEnc_;
  *xTopEnc = xTopEnc_;
  *yTopEnc = yTopEnc_;
}

int defiVia::hasRowCol() const
{
  if (rows_) {
    return rows_;
  }

  return cols_;
}

void defiVia::rowCol(int* numCutRows, int* numCutCols) const
{
  *numCutRows = rows_;
  *numCutCols = cols_;
}

int defiVia::hasOrigin() const
{
  if (xOffset_) {
    return xOffset_;
  }

  return yOffset_;
}

void defiVia::origin(int* xOffset, int* yOffset) const
{
  *xOffset = xOffset_;
  *yOffset = yOffset_;
}

int defiVia::hasOffset() const
{
  if (xBotOffset_) {
    return xBotOffset_;
  }
  if (yBotOffset_) {
    return yBotOffset_;
  }
  if (xTopOffset_) {
    return xTopOffset_;
  }
  return yTopOffset_;
}

void defiVia::offset(int* xBotOffset,
                     int* yBotOffset,
                     int* xTopOffset,
                     int* yTopOffset) const
{
  *xBotOffset = xBotOffset_;
  *yBotOffset = yBotOffset_;
  *xTopOffset = xTopOffset_;
  *yTopOffset = yTopOffset_;
}

int defiVia::hasCutPattern() const
{
  return hasCutPattern_;
}

int defiVia::hasRectMask(int index) const
{
  if (index > 0 || index < numLayers_) {
    return rectMask_[index];
  }

  return 0;
}

int defiVia::hasPolyMask(int index) const
{
  if (index > 0 || index < numPolys_) {
    return polyMask_[index];
  }

  return 0;
}

const char* defiVia::cutPattern() const
{
  return cutPattern_;
}

void defiVia::print(FILE* f) const
{
  int i;
  int xl, yl, xh, yh;
  char* c;
  char* vrn;
  char *bl, *cl, *tl;
  int xs, ys, xcs, ycs, xbe, ybe, xte, yte;
  int cr, cc, xo, yo, xbo, ybo, xto, yto;

  fprintf(f, "via '%s'\n", name());

  if (hasPattern()) {
    fprintf(f, "  pattern '%s'\n", pattern());
  }

  for (i = 0; i < numLayers(); i++) {
    layer(i, &c, &xl, &yl, &xh, &yh);
    fprintf(f, "  layer '%s' %d,%d %d,%d\n", c, xl, yl, xh, yh);
  }

  if (hasViaRule()) {
    viaRule(&vrn, &xs, &ys, &bl, &cl, &tl, &xcs, &ycs, &xbe, &ybe, &xte, &yte);
    fprintf(f, "  viarule '%s'\n", vrn);
    fprintf(f, "    cutsize %d %d\n", xs, ys);
    fprintf(f, "    layers %s %s %s\n", bl, cl, tl);
    fprintf(f, "    cutspacing %d %d\n", xcs, ycs);
    fprintf(f, "    enclosure %d %d %d %d\n", xbe, ybe, xte, yte);
    if (hasRowCol()) {
      rowCol(&cr, &cc);
      fprintf(f, "    rowcol %d %d\n", cr, cc);
    }
    if (hasOrigin()) {
      origin(&xo, &yo);
      fprintf(f, "    origin %d %d\n", xo, yo);
    }
    if (hasOffset()) {
      offset(&xbo, &ybo, &xto, &yto);
      fprintf(f, "    offset %d %d %d %d\n", xbo, ybo, xto, yto);
    }
    if (hasCutPattern()) {
      fprintf(f, "    pattern '%s'\n", cutPattern());
    }
  }
}
END_DEF_PARSER_NAMESPACE
