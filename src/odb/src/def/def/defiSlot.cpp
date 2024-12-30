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
//  $Date: 2020/09/29 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#include "defiSlot.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "defiDebug.hpp"
#include "lex.h"

BEGIN_LEFDEF_PARSER_NAMESPACE

namespace {

void defiError6160(int index, int numRectangles, defrData* defData)
{
  std::stringstream msg;
  msg << "ERROR (DEFPARS-6160): The index number " << index
      << " specified for the SLOT ";
  msg << "RECTANGLE is invalid.\nValid index number is from 0 to "
      << numRectangles << ". ";
  msg << "Specify a valid index number and then try again.";
  defiError(0, 6160, msg.str().c_str(), defData);
}

}  // namespace

////////////////////////////////////////////////////
////////////////////////////////////////////////////
//
//    defiSlot
//
////////////////////////////////////////////////////
////////////////////////////////////////////////////

defiSlot::defiSlot(defrData* data) : defData(data)
{
  Init();
}

void defiSlot::Init()
{
  numPolys_ = 0;
  clear();
  layerNameLength_ = 0;
  xl_ = (int*) malloc(sizeof(int) * 1);
  yl_ = (int*) malloc(sizeof(int) * 1);
  xh_ = (int*) malloc(sizeof(int) * 1);
  yh_ = (int*) malloc(sizeof(int) * 1);
  rectsAllocated_ = 1;  // At least 1 rectangle will define
  polysAllocated_ = 0;
  polygons_ = nullptr;
  layerName_ = nullptr;
}

defiSlot::~defiSlot()
{
  Destroy();
}

void defiSlot::clear()
{
  hasLayer_ = 0;
  numRectangles_ = 0;
}

void defiSlot::clearPoly()
{
  struct defiPoints* p;
  int i;

  for (i = 0; i < numPolys_; i++) {
    p = polygons_[i];
    free((char*) (p->x));
    free((char*) (p->y));
    free((char*) (polygons_[i]));
  }
  numPolys_ = 0;
}

void defiSlot::Destroy()
{
  if (layerName_)
    free(layerName_);
  free((char*) (xl_));
  free((char*) (yl_));
  free((char*) (xh_));
  free((char*) (yh_));
  rectsAllocated_ = 0;
  xl_ = nullptr;
  yl_ = nullptr;
  xh_ = nullptr;
  yh_ = nullptr;
  clearPoly();
  if (polygons_)
    free((char*) (polygons_));
  polygons_ = nullptr;
  clear();
}

void defiSlot::setLayer(const char* name)
{
  int len = strlen(name) + 1;
  if (layerNameLength_ < len) {
    if (layerName_)
      free(layerName_);
    layerName_ = (char*) malloc(len);
    layerNameLength_ = len;
  }
  strcpy(layerName_, defData->DEFCASE(name));
  hasLayer_ = 1;
}

void defiSlot::addRect(int xl, int yl, int xh, int yh)
{
  if (numRectangles_ == rectsAllocated_) {
    int i;
    int max = rectsAllocated_ = rectsAllocated_ * 2;
    int* newxl = (int*) malloc(sizeof(int) * max);
    int* newyl = (int*) malloc(sizeof(int) * max);
    int* newxh = (int*) malloc(sizeof(int) * max);
    int* newyh = (int*) malloc(sizeof(int) * max);
    for (i = 0; i < numRectangles_; i++) {
      newxl[i] = xl_[i];
      newyl[i] = yl_[i];
      newxh[i] = xh_[i];
      newyh[i] = yh_[i];
    }
    free((char*) (xl_));
    free((char*) (yl_));
    free((char*) (xh_));
    free((char*) (yh_));
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
void defiSlot::addPolygon(defiGeometries* geom)
{
  struct defiPoints* p;
  int x, y;
  int i;

  if (numPolys_ == polysAllocated_) {
    struct defiPoints** poly;
    polysAllocated_ = (polysAllocated_ == 0) ? 2 : polysAllocated_ * 2;
    poly = (struct defiPoints**) malloc(sizeof(struct defiPoints*)
                                        * polysAllocated_);
    for (i = 0; i < numPolys_; i++)
      poly[i] = polygons_[i];
    if (polygons_)
      free((char*) (polygons_));
    polygons_ = poly;
  }
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
  numPolys_ += 1;
}

int defiSlot::hasLayer() const
{
  return hasLayer_;
}

const char* defiSlot::layerName() const
{
  return layerName_;
}

int defiSlot::numRectangles() const
{
  return numRectangles_;
}

int defiSlot::xl(int index) const
{
  if (index < 0 || index >= numRectangles_) {
    defiError6160(index, numRectangles_, defData);
    return 0;
  }
  return xl_[index];
}

int defiSlot::yl(int index) const
{
  if (index < 0 || index >= numRectangles_) {
    defiError6160(index, numRectangles_, defData);
    return 0;
  }
  return yl_[index];
}

int defiSlot::xh(int index) const
{
  if (index < 0 || index >= numRectangles_) {
    defiError6160(index, numRectangles_, defData);
    return 0;
  }
  return xh_[index];
}

int defiSlot::yh(int index) const
{
  if (index < 0 || index >= numRectangles_) {
    defiError6160(index, numRectangles_, defData);
    return 0;
  }
  return yh_[index];
}

// 5.6
int defiSlot::numPolygons() const
{
  return numPolys_;
}

// 5.6
struct defiPoints defiSlot::getPolygon(int index) const
{
  return *(polygons_[index]);
}

void defiSlot::print(FILE* f) const
{
  int i, j;
  struct defiPoints points;

  if (hasLayer())
    fprintf(f, "- LAYER %s\n", layerName());

  for (i = 0; i < numRectangles(); i++) {
    fprintf(f, "   RECT %d %d %d %d\n", xl(i), yl(i), xh(i), yh(i));
  }

  for (i = 0; i < numPolygons(); i++) {
    fprintf(f, "   POLYGON ");
    points = getPolygon(i);
    for (j = 0; j < points.numPoints; j++)
      fprintf(f, "%d %d ", points.x[j], points.y[j]);
    fprintf(f, "\n");
  }
  fprintf(f, "\n");
}
END_LEFDEF_PARSER_NAMESPACE
