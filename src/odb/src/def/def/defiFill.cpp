// *****************************************************************************
// *****************************************************************************
// Copyright 2013 - 2015, Cadence Design Systems
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "lex.h"
#include "defiFill.hpp"
#include "defiDebug.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

////////////////////////////////////////////////////
////////////////////////////////////////////////////
//
//    defiFill
//
////////////////////////////////////////////////////
////////////////////////////////////////////////////

defiFill::defiFill(defrData *data) 
: defData(data) 
{
  Init();
}


void defiFill::Init() {
  numPolys_ = 0;
  numPts_ = 0;
  clear();
  layerNameLength_ = 0;
  xl_ = (int*)malloc(sizeof(int)*1);
  yl_ = (int*)malloc(sizeof(int)*1);
  xh_ = (int*)malloc(sizeof(int)*1);
  yh_ = (int*)malloc(sizeof(int)*1);
  rectsAllocated_ = 1;      // At least 1 rectangle will define
  polysAllocated_ = 0;
  polygons_ = 0;
  layerName_ = 0;
  viaName_ = 0;
  viaNameLength_ = 0;
  viaPts_ = 0;
  ptsAllocated_ = 0;
  viaPts_ = 0;

}

defiFill::~defiFill() {
  Destroy();
}

void defiFill::clear() {
  hasLayer_ = 0;
  layerOpc_ = 0;
  numRectangles_ = 0;
  hasVia_ = 0;
  viaOpc_ = 0;
  mask_ = 0;
}

void defiFill::clearPoly() {
  struct defiPoints* p;
  int i;

  for (i = 0; i < numPolys_; i++) {
    p = polygons_[i];
    free((char*)(p->x));
    free((char*)(p->y));
    free((char*)(polygons_[i]));
  }
  numPolys_ = 0;
}

void defiFill::clearPts() {
  struct defiPoints* p;
  int i;

  for (i = 0; i < numPts_; i++) {
    p = viaPts_[i];
    free((char*)(p->x));
    free((char*)(p->y));
    free((char*)(viaPts_[i]));
  }
  numPts_ = 0;
}

void defiFill::Destroy() {
  if (layerName_) free(layerName_);
  if (viaName_) free(viaName_);
  free((char*)(xl_));
  free((char*)(yl_));
  free((char*)(xh_));
  free((char*)(yh_));
  rectsAllocated_ = 0;
  xl_ = 0;
  yl_ = 0;
  xh_ = 0;
  yh_ = 0;
  clearPoly();
  if (polygons_) free((char*)(polygons_));
  polygons_ = 0;
  clearPts();
  if (viaPts_) free((char*)(viaPts_));
  viaPts_ = 0;
  clear();
}


void defiFill::setLayer(const char* name) {
  int len = strlen(name) + 1;
  if (layerNameLength_ < len) {
    if (layerName_) free(layerName_);
    layerName_ = (char*)malloc(len);
    layerNameLength_ = len;
  }
  strcpy(layerName_, defData->DEFCASE(name));
  hasLayer_ = 1;
}

// 5.7
void defiFill::setLayerOpc() {
  layerOpc_ = 1;
}

void defiFill::addRect(int xl, int yl, int xh, int yh) {
  if (numRectangles_ == rectsAllocated_) {
    int i;
    int max = rectsAllocated_ = rectsAllocated_ * 2;
    int* newxl = (int*)malloc(sizeof(int)*max);
    int* newyl = (int*)malloc(sizeof(int)*max);
    int* newxh = (int*)malloc(sizeof(int)*max);
    int* newyh = (int*)malloc(sizeof(int)*max);
    for (i = 0; i < numRectangles_; i++) {
      newxl[i] = xl_[i];
      newyl[i] = yl_[i];
      newxh[i] = xh_[i];
      newyh[i] = yh_[i];
    }
    free((char*)(xl_));
    free((char*)(yl_));
    free((char*)(xh_));
    free((char*)(yh_));
    xl_ = newxl;
    yl_ = newyl;
    xh_ = newxh;
    yh_ = newyh;
  }
  xl_[numRectangles_] = xl;
  yl_[numRectangles_] = yl;
  xh_[numRectangles_] = xh;
  yh_[numRectangles_] = yh;
  numRectangles_ += 1;
}

// 5.6
void defiFill::addPolygon(defiGeometries* geom) {
  struct defiPoints* p;
  int x, y;
  int i;

  if (numPolys_ == polysAllocated_) {
    struct defiPoints** poly;
    polysAllocated_ = (polysAllocated_ == 0) ?
          2 : polysAllocated_ * 2;
    poly = (struct defiPoints**)malloc(sizeof(struct defiPoints*) *
            polysAllocated_);
    for (i = 0; i < numPolys_; i++)
      poly[i] = polygons_[i];
    if (polygons_)
      free((char*)(polygons_));
    polygons_ = poly;
  }
  p = (struct defiPoints*)malloc(sizeof(struct defiPoints));
  p->numPoints = geom->numPoints();
  p->x = (int*)malloc(sizeof(int)*p->numPoints);
  p->y = (int*)malloc(sizeof(int)*p->numPoints);
  for (i = 0; i < p->numPoints; i++) {
    geom->points(i, &x, &y);
    p->x[i] = x;
    p->y[i] = y;
  }
  polygons_[numPolys_] = p;
  numPolys_ += 1;
}

int defiFill::hasLayer() const {
  return hasLayer_;
}

const char* defiFill::layerName() const {
  return layerName_;
}

// 5.7
int defiFill::hasLayerOpc() const {
  return layerOpc_;
}

int defiFill::numRectangles() const {
  return numRectangles_;
}


int defiFill::xl(int index) const {
  if (index < 0 || index >= numRectangles_) {
    defiError(1, 0, "bad index for Fill xl", defData);
    return 0;
  }
  return xl_[index];
}


int defiFill::yl(int index) const {
  if (index < 0 || index >= numRectangles_) {
    defiError(1, 0, "bad index for Fill yl", defData);
    return 0;
  }
  return yl_[index];
}


int defiFill::xh(int index) const {
  if (index < 0 || index >= numRectangles_) {
    defiError(1, 0, "bad index for Fill xh", defData);
    return 0;
  }
  return xh_[index];
}


int defiFill::yh(int index) const {
  if (index < 0 || index >= numRectangles_) {
    defiError(1, 0, "bad index for Fill yh", defData);
    return 0;
  }
  return yh_[index];
}

// 5.6
int defiFill::numPolygons() const {
  return numPolys_;
}


// 5.6
struct defiPoints defiFill::getPolygon(int index) const {
  return *(polygons_[index]);
}

// 5.7
void defiFill::setVia(const char* name) {
  int len = strlen(name) + 1;
  if (viaNameLength_ < len) {
    if (viaName_) free(viaName_);
    viaName_ = (char*)malloc(len);
    viaNameLength_ = len;
  }
  strcpy(viaName_, defData->DEFCASE(name));
  hasVia_ = 1;
}

// 5.7
void defiFill::setViaOpc() {
  viaOpc_ = 1;
}

// 5.8
void defiFill::setMask(int colorMask) {
    mask_ = colorMask;
}


// 5.7
void defiFill::addPts(defiGeometries* geom) {
  struct defiPoints* p;
  int x, y;
  int i;

  if (numPts_ == ptsAllocated_) {
    struct defiPoints** pts;
    ptsAllocated_ = (ptsAllocated_ == 0) ?
          2 : ptsAllocated_ * 2;
    pts= (struct defiPoints**)malloc(sizeof(struct defiPoints*) *
            ptsAllocated_);
    for (i = 0; i < numPts_; i++)
      pts[i] = viaPts_[i];
    if (viaPts_)
      free((char*)(viaPts_));
    viaPts_ = pts;
  }
  p = (struct defiPoints*)malloc(sizeof(struct defiPoints));
  p->numPoints = geom->numPoints();
  p->x = (int*)malloc(sizeof(int)*p->numPoints);
  p->y = (int*)malloc(sizeof(int)*p->numPoints);
  for (i = 0; i < p->numPoints; i++) {
    geom->points(i, &x, &y);
    p->x[i] = x;
    p->y[i] = y;
  }
  viaPts_[numPts_] = p;
  numPts_ += 1;
}

// 5.7
int defiFill::hasVia() const {
  return hasVia_;
}

// 5.7
const char* defiFill::viaName() const {
  return viaName_;
}

// 5.7
int defiFill::hasViaOpc() const {
  return viaOpc_;
}

// 5.7
int defiFill::numViaPts() const {
  return numPts_;
}

// 5.8
int defiFill::layerMask() const {
    return mask_;
}

// 5.8
int defiFill::viaTopMask() const {
    return mask_ / 100;
}

// 5.8
int defiFill::viaCutMask() const {
    return mask_ / 10 % 10;
}

// 5.8
int defiFill::viaBottomMask() const {
    return mask_ % 10;
}

// 5.7
struct defiPoints defiFill::getViaPts(int index) const {
  return *(viaPts_[index]);
}

void defiFill::print(FILE* f) const {
  int i, j;
  struct defiPoints points;

  if (hasLayer())
    fprintf(f, "- LAYER %s", layerName());

  if (layerMask())
      fprintf(f, " + Mask %d", layerMask());

  if (hasLayerOpc())
    fprintf(f, " + OPC");
  fprintf(f, "\n");

  for (i = 0; i < numRectangles(); i++) {
    fprintf(f, "   RECT %d %d %d %d\n", xl(i),
            yl(i), xh(i),
            yh(i));
  }

  for (i = 0; i < numPolygons(); i++) {
    fprintf(f, "   POLYGON ");
    points = getPolygon(i);
    for (j = 0; j < points.numPoints; j++)
      fprintf(f, "%d %d ", points.x[j], points.y[j]);
    fprintf(f, "\n");
  }
  fprintf(f,"\n");

  if (hasVia())
    fprintf(f, "- VIA %s", viaName());

  if (mask_) {
      fprintf(f, " + MASK %d%d%d", viaTopMask(),
          viaCutMask(),
          viaBottomMask());
  }

  if (hasViaOpc())
    fprintf(f, " + OPC");
  fprintf(f, "\n");

  for (i = 0; i < numViaPts(); i++) {
    fprintf(f, "   ");
    points = getViaPts(i);
    for (j = 0; j < points.numPoints; j++)
      fprintf(f, "%d %d ", points.x[j], points.y[j]);
    fprintf(f, "\n");
  }
  fprintf(f,"\n");
}
END_LEFDEF_PARSER_NAMESPACE

