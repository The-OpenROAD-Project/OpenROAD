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
//  $Date: 2020/09/29 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#include "lefiMisc.hpp"

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "lefiDebug.hpp"
#include "lefiKRDefs.hpp"
#include "lefiUtil.hpp"
#include "lex.h"

BEGIN_LEF_PARSER_NAMESPACE

// *****************************************************************************
// lefiGeometries
// *****************************************************************************

lefiGeometries::lefiGeometries()
{
  Init();
}

void lefiGeometries::Init()
{
  itemsAllocated_ = 2;
  numItems_ = 0;
  itemType_ = (lefiGeomEnum*) lefMalloc(sizeof(lefiGeomEnum) * 2);
  items_ = (void**) lefMalloc(sizeof(void*) * 2);
  numPoints_ = 0;
  pointsAllocated_ = 0;
  x_ = nullptr;
  y_ = nullptr;
  xStart_ = -1;
  yStart_ = -1;
  xStep_ = -1;
  yStep_ = -1;
}

void lefiGeometries::Destroy()
{
  clear();
  lefFree(items_);
  lefFree(itemType_);
  if (x_) {
    lefFree(x_);
    lefFree(y_);
  }
  pointsAllocated_ = 0;
}

lefiGeometries::~lefiGeometries()
{
  Destroy();
}

void lefiGeometries::clear()
{
  for (int i = 0; i < numItems_; i++) {
    if (itemType_[i] == lefiGeomViaE) {
      lefFree(((lefiGeomVia*) items_[i])->name);
    }
    if (itemType_[i] == lefiGeomViaIterE) {
      lefFree(((lefiGeomViaIter*) items_[i])->name);
    }
    if (itemType_[i] == lefiGeomPathE) {
      ((lefiGeomPath*) (items_[i]))->numPoints = 0;
      lefFree(((lefiGeomPath*) items_[i])->x);
      lefFree(((lefiGeomPath*) items_[i])->y);
    }
    if (itemType_[i] == lefiGeomPathIterE) {
      ((lefiGeomPathIter*) (items_[i]))->numPoints = 0;
      lefFree(((lefiGeomPathIter*) items_[i])->x);
      lefFree(((lefiGeomPathIter*) items_[i])->y);
    }
    if (itemType_[i] == lefiGeomPolygonE) {
      ((lefiGeomPolygon*) (items_[i]))->numPoints = 0;
      lefFree(((lefiGeomPolygon*) items_[i])->x);
      lefFree(((lefiGeomPolygon*) items_[i])->y);
    }
    if (itemType_[i] == lefiGeomPolygonIterE) {
      ((lefiGeomPolygonIter*) (items_[i]))->numPoints = 0;
      lefFree(((lefiGeomPolygonIter*) items_[i])->x);
      lefFree(((lefiGeomPolygonIter*) items_[i])->y);
    }
    lefFree(items_[i]);
  }
  numItems_ = 0;
}

void lefiGeometries::clearPolyItems()
{
  lefFree(items_);
  lefFree(itemType_);
  if (x_) {
    lefFree(x_);
    lefFree(y_);
  }
  numPoints_ = 0;
  pointsAllocated_ = 0;
  numItems_ = 0;
}

void lefiGeometries::add(void* v, lefiGeomEnum e)
{
  if (numItems_ == itemsAllocated_) {
    int i;
    void** newi;
    lefiGeomEnum* newe;
    if (itemsAllocated_ == 0) {  // 9/12/2002 - for C version
      itemsAllocated_ = 2;
    } else {
      itemsAllocated_ *= 2;
    }
    newe = (lefiGeomEnum*) lefMalloc(sizeof(lefiGeomEnum) * itemsAllocated_);
    newi = (void**) lefMalloc(sizeof(void*) * itemsAllocated_);
    for (i = 0; i < numItems_; i++) {
      newe[i] = itemType_[i];
      newi[i] = items_[i];
    }
    lefFree(items_);
    lefFree(itemType_);
    items_ = newi;
    itemType_ = newe;
  }
  items_[numItems_] = v;
  itemType_[numItems_] = e;
  numItems_ += 1;
}

void lefiGeometries::addLayer(const char* name)
{
  char* c = (char*) lefMalloc(strlen(name) + 1);
  strcpy(c, CASE(name));
  add((void*) c, lefiGeomLayerE);
}

// 5.7
void lefiGeometries::addLayerExceptPgNet()
{
  int* d = (int*) lefMalloc(sizeof(int));
  *d = 1;
  add((void*) d, lefiGeomLayerExceptPgNetE);
}

void lefiGeometries::addLayerMinSpacing(double spacing)
{
  double* d = (double*) lefMalloc(sizeof(double));
  *d = spacing;
  add((void*) d, lefiGeomLayerMinSpacingE);
}

void lefiGeometries::addLayerRuleWidth(double width)
{
  double* d = (double*) lefMalloc(sizeof(double));
  *d = width;
  add((void*) d, lefiGeomLayerRuleWidthE);
}

void lefiGeometries::addClass(const char* name)
{
  char* c = (char*) lefMalloc(strlen(name) + 1);
  strcpy(c, CASE(name));
  add((void*) c, lefiGeomClassE);
}

void lefiGeometries::addWidth(double w)
{
  double* d = (double*) lefMalloc(sizeof(double));
  *d = w;
  add((void*) d, lefiGeomWidthE);
}

void lefiGeometries::addPath(int colorMask)
{
  int i;
  int lim;
  lefiGeomPath* p = (lefiGeomPath*) lefMalloc(sizeof(lefiGeomPath));

  lim = p->numPoints = numPoints_;

  if (lim > 0) {
    p->x = (double*) lefMalloc(sizeof(double) * lim);
    p->y = (double*) lefMalloc(sizeof(double) * lim);
    for (i = 0; i < lim; i++) {
      p->x[i] = x_[i];
      p->y[i] = y_[i];
    }
  } else {
    p->x = nullptr;
    p->y = nullptr;
  }

  p->colorMask = colorMask;

  add((void*) p, lefiGeomPathE);
}

void lefiGeometries::addPathIter(int colorMask)
{
  int i;
  int lim;
  lefiGeomPathIter* p = (lefiGeomPathIter*) lefMalloc(sizeof(lefiGeomPathIter));

  lim = p->numPoints = numPoints_;

  if (lim > 0) {
    p->x = (double*) lefMalloc(sizeof(double) * lim);
    p->y = (double*) lefMalloc(sizeof(double) * lim);
    for (i = 0; i < lim; i++) {
      p->x[i] = x_[i];
      p->y[i] = y_[i];
    }
  } else {
    p->x = nullptr;
    p->y = nullptr;
  }

  p->colorMask = colorMask;
  p->xStart = xStart_;
  p->yStart = yStart_;
  p->xStep = xStep_;
  p->yStep = yStep_;

  add((void*) p, lefiGeomPathIterE);
}

// pcr 481783 & 560504
void lefiGeometries::addRect(int colorMask,
                             double xl,
                             double yl,
                             double xh,
                             double yh)
{
  lefiGeomRect* p = (lefiGeomRect*) lefMalloc(sizeof(lefiGeomRect));
  p->xl = xl;
  p->yl = yl;
  p->xh = xh;
  p->yh = yh;
  p->colorMask = colorMask;

  add((void*) p, lefiGeomRectE);
}

void lefiGeometries::addRectIter(int colorMask,
                                 double xl,
                                 double yl,
                                 double xh,
                                 double yh)
{
  lefiGeomRectIter* p = (lefiGeomRectIter*) lefMalloc(sizeof(lefiGeomRectIter));

  p->xl = xl;
  p->yl = yl;
  p->xh = xh;
  p->yh = yh;
  p->xStart = xStart_;
  p->yStart = yStart_;
  p->xStep = xStep_;
  p->yStep = yStep_;
  p->colorMask = colorMask;

  add((void*) p, lefiGeomRectIterE);
}

void lefiGeometries::addPolygon(int colorMask)
{
  int i;
  int lim;
  lefiGeomPolygon* p = (lefiGeomPolygon*) lefMalloc(sizeof(lefiGeomPolygon));

  lim = p->numPoints = numPoints_;

  if (lim > 0) {
    p->x = (double*) lefMalloc(sizeof(double) * lim);
    p->y = (double*) lefMalloc(sizeof(double) * lim);
    for (i = 0; i < lim; i++) {
      p->x[i] = x_[i];
      p->y[i] = y_[i];
    }
  } else {
    p->x = nullptr;
    p->y = nullptr;
  }

  p->colorMask = colorMask;

  add((void*) p, lefiGeomPolygonE);
}

void lefiGeometries::addPolygonIter(int colorMask)
{
  int i;
  int lim;
  lefiGeomPolygonIter* p
      = (lefiGeomPolygonIter*) lefMalloc(sizeof(lefiGeomPolygonIter));

  lim = p->numPoints = numPoints_;

  if (lim > 0) {
    p->x = (double*) lefMalloc(sizeof(double) * lim);
    p->y = (double*) lefMalloc(sizeof(double) * lim);
    for (i = 0; i < lim; i++) {
      p->x[i] = x_[i];
      p->y[i] = y_[i];
    }
  } else {
    p->x = nullptr;
    p->y = nullptr;
  }

  p->xStart = xStart_;
  p->yStart = yStart_;
  p->xStep = xStep_;
  p->yStep = yStep_;

  p->colorMask = colorMask;

  add((void*) p, lefiGeomPolygonIterE);
}

void lefiGeometries::addVia(int viaMask, double x, double y, const char* name)
{
  lefiGeomVia* p = (lefiGeomVia*) lefMalloc(sizeof(lefiGeomVia));
  char* c = (char*) lefMalloc(strlen(name) + 1);

  strcpy(c, CASE(name));
  p->x = x;
  p->y = y;
  p->name = c;
  p->bottomMaskNum = viaMask % 10;
  p->cutMaskNum = viaMask / 10 % 10;
  p->topMaskNum = viaMask / 100;

  add((void*) p, lefiGeomViaE);
}

void lefiGeometries::addViaIter(int viaMask,
                                double x,
                                double y,
                                const char* name)
{
  lefiGeomViaIter* p = (lefiGeomViaIter*) lefMalloc(sizeof(lefiGeomViaIter));
  char* c = (char*) lefMalloc(strlen(name) + 1);

  strcpy(c, CASE(name));
  p->bottomMaskNum = viaMask % 10;
  p->cutMaskNum = viaMask / 10 % 10;
  p->topMaskNum = viaMask / 100;
  p->x = x;
  p->y = y;
  p->name = c;
  p->xStart = xStart_;
  p->yStart = yStart_;
  p->xStep = xStep_;
  p->yStep = yStep_;

  add((void*) p, lefiGeomViaIterE);
}

void lefiGeometries::addStepPattern(double xStart,
                                    double yStart,
                                    double xStep,
                                    double yStep)
{
  xStart_ = xStart;
  yStart_ = yStart;
  xStep_ = xStep;
  yStep_ = yStep;
}

void lefiGeometries::startList(double x, double y)
{
  if (!x_) {
    numPoints_ = 0;
    pointsAllocated_ = 16;
    x_ = (double*) lefMalloc(sizeof(double) * 16);
    y_ = (double*) lefMalloc(sizeof(double) * 16);
  } else {  // reset the numPoits to 0
    numPoints_ = 0;
  }
  addToList(x, y);
}

void lefiGeometries::addToList(double x, double y)
{
  if (numPoints_ == pointsAllocated_) {
    int i;
    double* nx;
    double* ny;
    if (pointsAllocated_ == 0) {
      pointsAllocated_ = 2;
    } else {
      pointsAllocated_ *= 2;
    }
    nx = (double*) lefMalloc(sizeof(double) * pointsAllocated_);
    ny = (double*) lefMalloc(sizeof(double) * pointsAllocated_);
    for (i = 0; i < numPoints_; i++) {
      nx[i] = x_[i];
      ny[i] = y_[i];
    }
    lefFree(x_);
    lefFree(y_);
    x_ = nx;
    y_ = ny;
  }
  x_[numPoints_] = x;
  y_[numPoints_] = y;
  numPoints_ += 1;
}

int lefiGeometries::numItems() const
{
  return numItems_;
}

lefiGeomEnum lefiGeometries::itemType(int index) const
{
  char msg[160];
  if (index < 0 || index >= numItems_) {
    sprintf(msg,
            "ERROR (LEFPARS-1360): The index number %d given for the geometry "
            "item is invalid.\nValid index is from 0 to %d",
            index,
            numItems_);
    lefiError(0, 1360, msg);
    return lefiGeomUnknown;
  }
  return itemType_[index];
}

lefiGeomRect* lefiGeometries::getRect(int index) const
{
  char msg[160];
  if (index < 0 || index >= numItems_) {
    sprintf(msg,
            "ERROR (LEFPARS-1361): The index number %d given for the geometry "
            "RECTANGLE is invalid.\nValid index is from 0 to %d",
            index,
            numItems_);
    lefiError(0, 1361, msg);
    return nullptr;
  }
  return (lefiGeomRect*) (items_[index]);
}

lefiGeomRectIter* lefiGeometries::getRectIter(int index) const
{
  char msg[160];
  if (index < 0 || index >= numItems_) {
    sprintf(msg,
            "ERROR (LEFPARS-1362): The index number %d given for the geometry "
            "RECTANGLE ITERATE is invalid.\nValid index is from 0 to %d",
            index,
            numItems_);
    lefiError(0, 1362, msg);
    return nullptr;
  }
  return (lefiGeomRectIter*) (items_[index]);
}

lefiGeomPath* lefiGeometries::getPath(int index) const
{
  char msg[160];
  if (index < 0 || index >= numItems_) {
    sprintf(msg,
            "ERROR (LEFPARS-1363): The index number %d given for the geometry "
            "PATH is invalid.\nValid index is from 0 to %d",
            index,
            numItems_);
    lefiError(0, 1363, msg);
    return nullptr;
  }
  return (lefiGeomPath*) (items_[index]);
}

lefiGeomPathIter* lefiGeometries::getPathIter(int index) const
{
  char msg[160];
  if (index < 0 || index >= numItems_) {
    sprintf(msg,
            "ERROR (LEFPARS-1364): The index number %d given for the geometry "
            "PATH ITERATE is invalid.\nValid index is from 0 to %d",
            index,
            numItems_);
    lefiError(0, 1364, msg);
    return nullptr;
  }
  return (lefiGeomPathIter*) (items_[index]);
}

char* lefiGeometries::getLayer(int index) const
{
  char msg[160];
  if (index < 0 || index >= numItems_) {
    sprintf(msg,
            "ERROR (LEFPARS-1365): The index number %d given for the geometry "
            "LAYER is invalid.\nValid index is from 0 to %d",
            index,
            numItems_);
    lefiError(0, 1365, msg);
    return nullptr;
  }
  return (char*) (items_[index]);
}

// 5.7
int lefiGeometries::hasLayerExceptPgNet(int index) const
{
  char msg[160];
  if (index < 0 || index >= numItems_) {
    sprintf(msg,
            "ERROR (LEFPARS-1366): The index number %d given for the geometry "
            "LAYER EXCEPT PG NET is invalid.\nValid index is from 0 to %d",
            index,
            numItems_);
    lefiError(0, 1366, msg);
    return 0;
  }
  return *((int*) (items_[index]));
}

double lefiGeometries::getLayerMinSpacing(int index) const
{
  char msg[160];
  if (index < 0 || index >= numItems_) {
    sprintf(msg,
            "ERROR (LEFPARS-1367): The index number %d given for the geometry "
            "LAYER MINSPACING is invalid.\nValid index is from 0 to %d",
            index,
            numItems_);
    lefiError(0, 1367, msg);
    return 0;
  }
  return *((double*) (items_[index]));
}

double lefiGeometries::getLayerRuleWidth(int index) const
{
  char msg[160];
  if (index < 0 || index >= numItems_) {
    sprintf(msg,
            "ERROR (LEFPARS-1368): The index number %d given for the geometry "
            "LAYER RULE WIDTH is invalid.\nValid index is from 0 to %d",
            index,
            numItems_);
    lefiError(0, 1368, msg);
    return 0;
  }
  return *((double*) (items_[index]));
}

double lefiGeometries::getWidth(int index) const
{
  char msg[160];
  if (index < 0 || index >= numItems_) {
    sprintf(msg,
            "ERROR (LEFPARS-1369): The index number %d given for the geometry "
            "WIDTH is invalid.\nValid index is from 0 to %d",
            index,
            numItems_);
    lefiError(0, 1369, msg);
    return 0;
  }
  return *((double*) (items_[index]));
}

lefiGeomPolygon* lefiGeometries::getPolygon(int index) const
{
  char msg[160];
  if (index < 0 || index >= numItems_) {
    sprintf(msg,
            "ERROR (LEFPARS-1370): The index number %d given for the geometry "
            "POLYGON is invalid.\nValid index is from 0 to %d",
            index,
            numItems_);
    lefiError(0, 1370, msg);
    return nullptr;
  }
  return (lefiGeomPolygon*) (items_[index]);
}

lefiGeomPolygonIter* lefiGeometries::getPolygonIter(int index) const
{
  char msg[160];
  if (index < 0 || index >= numItems_) {
    sprintf(msg,
            "ERROR (LEFPARS-1371): The index number %d given for the geometry "
            "POLYGON ITERATE is invalid.\nValid index is from 0 to %d",
            index,
            numItems_);
    lefiError(0, 1371, msg);
    return nullptr;
  }
  return (lefiGeomPolygonIter*) (items_[index]);
}

char* lefiGeometries::getClass(int index) const
{
  char msg[160];
  if (index < 0 || index >= numItems_) {
    sprintf(msg,
            "ERROR (LEFPARS-1372): The index number %d given for the geometry "
            "CLASS is invalid.\nValid index is from 0 to %d",
            index,
            numItems_);
    lefiError(0, 1372, msg);
    return nullptr;
  }
  return (char*) (items_[index]);
}

lefiGeomVia* lefiGeometries::getVia(int index) const
{
  char msg[160];
  if (index < 0 || index >= numItems_) {
    sprintf(msg,
            "ERROR (LEFPARS-1373): The index number %d given for the geometry "
            "VIA is invalid.\nValid index is from 0 to %d",
            index,
            numItems_);
    lefiError(0, 1373, msg);
    return nullptr;
  }
  return (lefiGeomVia*) (items_[index]);
}

lefiGeomViaIter* lefiGeometries::getViaIter(int index) const
{
  char msg[160];
  if (index < 0 || index >= numItems_) {
    sprintf(msg,
            "ERROR (LEFPARS-1374): The index number %d given for the geometry "
            "VIA ITERATE is invalid.\nValid index is from 0 to %d",
            index,
            numItems_);
    lefiError(0, 1374, msg);
    return nullptr;
  }
  return (lefiGeomViaIter*) (items_[index]);
}

void lefiGeometries::print(FILE* f) const
{
  int i;
  int l;
  lefiGeomRect* rect;
  lefiGeomRectIter* rectiter;
  lefiGeomPath* path;
  lefiGeomPathIter* pathiter;
  lefiGeomPolygon* polygon;
  lefiGeomPolygonIter* polygoniter;

  for (i = 0; i < numItems_; i++) {
    switch (itemType(i)) {
      case lefiGeomLayerE:
        fprintf(f, "Layer %s\n", getLayer(i));
        break;

      case lefiGeomLayerExceptPgNetE:
        if (getLayerMinSpacing(i)) {
          fprintf(f, "EXCEPTPGNET \n");
        }
        break;

      case lefiGeomLayerMinSpacingE:
        fprintf(f, "Spacing %g\n", getLayerMinSpacing(i));
        break;

      case lefiGeomLayerRuleWidthE:
        fprintf(f, "DesignRuleWidth %g\n", getLayerRuleWidth(i));
        break;

      case lefiGeomWidthE:
        fprintf(f, "Width %g\n", getWidth(i));
        break;

      case lefiGeomPathE:
        path = getPath(i);
        fprintf(f, "Path");

        if (path->colorMask) {
          fprintf(f, " MASK %d", path->colorMask);
        }
        for (l = 0; l < path->numPoints; l++) {
          fprintf(f, " %g,%g", path->x[l], path->y[l]);
        }
        fprintf(f, "\n");
        break;

      case lefiGeomPathIterE:
        pathiter = getPathIter(i);

        if (pathiter->colorMask) {
          fprintf(f, "MASK %d", pathiter->colorMask);
        }

        fprintf(f,
                "Path iter  start %g,%g  step %g,%g\n",
                pathiter->xStart,
                pathiter->yStart,
                pathiter->xStep,
                pathiter->yStep);

        for (l = 0; l < pathiter->numPoints; l++) {
          fprintf(f, " %g,%g", pathiter->x[l], pathiter->y[l]);
        }

        fprintf(f, "\n");
        break;

      case lefiGeomRectE:
        rect = getRect(i);

        if (rect->colorMask) {
          fprintf(f,
                  "Rect MASK %d, %g,%g  %g,%g\n",
                  rect->colorMask,
                  rect->xl,
                  rect->yl,
                  rect->xh,
                  rect->yh);
        } else {
          fprintf(
              f, "Rect %g,%g  %g,%g\n", rect->xl, rect->yl, rect->xh, rect->yh);
        }
        break;

      case lefiGeomRectIterE:
        rectiter = getRectIter(i);

        if (rectiter->colorMask) {
          fprintf(f,
                  "Rect MASK %d iter  start %g,%g  step %g,%g\n",
                  rectiter->colorMask,
                  rectiter->xStart,
                  rectiter->yStart,
                  rectiter->xStep,
                  rectiter->yStep);
        } else {
          fprintf(f,
                  "Rect iter  start %g,%g  step %g,%g\n",
                  rectiter->xStart,
                  rectiter->yStart,
                  rectiter->xStep,
                  rectiter->yStep);
        }

        fprintf(f,
                "     %g,%g  %g,%g\n",
                rectiter->xl,
                rectiter->yl,
                rectiter->xh,
                rectiter->yh);
        break;

      case lefiGeomPolygonE:
        polygon = getPolygon(i);
        fprintf(f, "Polygon");

        if (polygon->colorMask) {
          fprintf(f, " MASK %d", polygon->colorMask);
        }

        for (l = 0; l < polygon->numPoints; l++) {
          fprintf(f, " %g,%g", polygon->x[l], polygon->y[l]);
        }

        fprintf(f, "\n");
        break;

      case lefiGeomPolygonIterE:
        polygoniter = getPolygonIter(i);

        if (polygoniter->colorMask) {
          fprintf(f,
                  "Polygon MASK %d iter  start %g,%g  step %g,%g\n",
                  polygoniter->colorMask,
                  polygoniter->xStart,
                  polygoniter->yStart,
                  polygoniter->xStep,
                  polygoniter->yStep);
        } else {
          fprintf(f,
                  "Polygon iter  start %g,%g  step %g,%g\n",
                  polygoniter->xStart,
                  polygoniter->yStart,
                  polygoniter->xStep,
                  polygoniter->yStep);
        }

        for (l = 0; l < polygoniter->numPoints; l++) {
          fprintf(f, " %g,%g", polygoniter->x[l], polygoniter->y[l]);
        }

        fprintf(f, "\n");
        break;

      case lefiGeomViaE:
        fprintf(f, "Via \n");
        break;

      case lefiGeomViaIterE:
        fprintf(f, "Via iter \n");
        break;

      case lefiGeomClassE:
        fprintf(f, "Classtype %s\n", (char*) items_[i]);
        break;

      default:
        lefiError(0, 1375, "ERROR (LEFPARS-1375): unknown geometry type");
        fprintf(f, "Unknown geometry type %d\n", (int) (itemType(i)));
        break;
    }
  }
}

// *****************************************************************************
// lefiSpacing
// *****************************************************************************

lefiSpacing::lefiSpacing()
{
  Init();
}

void lefiSpacing::Init()
{
  name1_ = (char*) lefMalloc(16);
  name2_ = (char*) lefMalloc(16);
  name1Size_ = 16;
  name2Size_ = 16;
  distance_ = 0;
  hasStack_ = 0;
}

void lefiSpacing::Destroy()
{
  if (name1_) {
    lefFree(name1_);
  }
  if (name2_) {
    lefFree(name2_);
  }
}

lefiSpacing::~lefiSpacing()
{
  Destroy();
}

lefiSpacing* lefiSpacing::clone()
{
  lefiSpacing* sp = (lefiSpacing*) lefMalloc(sizeof(lefiSpacing));
  sp->name1Size_ = strlen(name1_) + 1;
  sp->name1_ = (char*) lefMalloc(sp->name1Size_);
  strcpy(sp->name1_, name1_);
  sp->name2Size_ = strlen(name2_) + 1;
  sp->name2_ = (char*) lefMalloc(sp->name2Size_);
  strcpy(sp->name2_, name2_);
  sp->distance_ = distance_;
  sp->hasStack_ = hasStack_;
  return sp;
}

void lefiSpacing::set(const char* name1,
                      const char* name2,
                      double d,
                      int hasStack)
{
  int len = strlen(name1) + 1;
  if (len > name1Size_) {
    lefFree(name1_);
    name1_ = (char*) lefMalloc(len);
    name1Size_ = len;
  }
  len = strlen(name2) + 1;
  if (len > name2Size_) {
    lefFree(name2_);
    name2_ = (char*) lefMalloc(len);
    name2Size_ = len;
  }
  strcpy(name1_, CASE(name1));
  strcpy(name2_, CASE(name2));
  distance_ = d;
  hasStack_ = hasStack;
}

const char* lefiSpacing::name1() const
{
  return name1_;
}

const char* lefiSpacing::name2() const
{
  return name2_;
}

double lefiSpacing::distance() const
{
  return distance_;
}

int lefiSpacing::hasStack() const
{
  return hasStack_;
}
void lefiSpacing::print(FILE* f) const
{
  fprintf(f, "SPACING %s %s %g", name1(), name2(), distance());

  if (hasStack()) {
    fprintf(f, "  STACK");
  }

  fprintf(f, "\n");
}

// *****************************************************************************
// lefiIRDrop
// *****************************************************************************

lefiIRDrop::lefiIRDrop()
{
  Init();
}

void lefiIRDrop::Init()
{
  nameSize_ = 16;
  value1Size_ = 16;
  value2Size_ = 16;
  name_ = (char*) lefMalloc(16);
  numValues_ = 0;
  valuesAllocated_ = 2;
  value1_ = (double*) lefMalloc(sizeof(double) * 2);
  value2_ = (double*) lefMalloc(sizeof(double) * 2);
}

void lefiIRDrop::Destroy()
{
  lefFree(name_);
  clear();
  lefFree(value1_);
  lefFree(value2_);
}

lefiIRDrop::~lefiIRDrop()
{
  Destroy();
}

void lefiIRDrop::clear()
{
  numValues_ = 0;
}

void lefiIRDrop::setTableName(const char* name)
{
  int len = strlen(name) + 1;
  if (len > nameSize_) {
    lefFree(name_);
    name_ = (char*) lefMalloc(len);
    nameSize_ = len;
  }
  strcpy(name_, CASE(name));
  clear();
  /*
   *(value1_) = '\0';
   *(value2_) = '\0';
   */
}

void lefiIRDrop::setValues(double value1, double value2)
{
  if (numValues_ == valuesAllocated_) {
    int i;
    double* v1;
    double* v2;
    if (valuesAllocated_ == 0) {
      valuesAllocated_ = 2;
      v1 = (double*) lefMalloc(sizeof(double) * valuesAllocated_);
      v2 = (double*) lefMalloc(sizeof(double) * valuesAllocated_);
    } else {
      valuesAllocated_ *= 2;
      v1 = (double*) lefMalloc(sizeof(double) * valuesAllocated_);
      v2 = (double*) lefMalloc(sizeof(double) * valuesAllocated_);
      for (i = 0; i < numValues_; i++) {
        v1[i] = value1_[i];
        v2[i] = value2_[i];
      }
      lefFree(value1_);
      lefFree(value2_);
    }
    value1_ = v1;
    value2_ = v2;
  }
  value1_[numValues_] = value1;
  value2_[numValues_] = value2;
  numValues_ += 1;
}

const char* lefiIRDrop::name() const
{
  return name_;
}

int lefiIRDrop::numValues() const
{
  return numValues_;
}

double lefiIRDrop::value1(int index) const
{
  char msg[160];
  if (index < 0 || index >= numValues_) {
    sprintf(msg,
            "ERROR (LEFPARS-1376): The index number %d given for the IRDROP is "
            "invalid.\nValid index is from 0 to %d",
            index,
            numValues_);
    lefiError(0, 1376, msg);
    return 0;
  }
  return value1_[index];
}

double lefiIRDrop::value2(int index) const
{
  char msg[160];
  if (index < 0 || index >= numValues_) {
    sprintf(msg,
            "ERROR (LEFPARS-1376): The index number %d given for the IRDROP is "
            "invalid.\nValid index is from 0 to %d",
            index,
            numValues_);
    lefiError(0, 1376, msg);
    return 0;
  }
  return value2_[index];
}

void lefiIRDrop::print(FILE* f) const
{
  int i;
  fprintf(f, "IRDROP %s ", name());
  for (i = 0; i < numValues(); i++) {
    fprintf(f, "%g %g ", value1(i), value2(i));
  }
  fprintf(f, "\n");
  fprintf(f, "END IRDrop\n");
}

// *****************************************************************************
// lefitMinFeature
// *****************************************************************************
lefiMinFeature::lefiMinFeature()
{
  Init();
}

void lefiMinFeature::Init()
{
  // nothing to do
}

void lefiMinFeature::Destroy()
{
  // nothing to do
}

lefiMinFeature::~lefiMinFeature()
{
  Destroy();
}

void lefiMinFeature::set(double one, double two)
{
  one_ = one;
  two_ = two;
}

double lefiMinFeature::one() const
{
  return one_;
}

double lefiMinFeature::two() const
{
  return two_;
}

void lefiMinFeature::print(FILE* f) const
{
  fprintf(f, "MINfEATURE %g %g\n", one(), two());
}

// *****************************************************************************
// lefiSite
// *****************************************************************************

lefiSite::lefiSite()
{
  Init();
}

void lefiSite::Init()
{
  nameSize_ = 16;
  name_ = (char*) lefMalloc(16);
  numRowPattern_ = 0;
  rowPatternAllocated_ = 0;
  siteNames_ = nullptr;
  siteOrients_ = nullptr;
}

void lefiSite::Destroy()
{
  int i;

  lefFree(name_);
  if (numRowPattern_) {
    for (i = 0; i < numRowPattern_; i++) {
      lefFree(siteNames_[i]);
    }
    lefFree(siteNames_);
    lefFree(siteOrients_);
    numRowPattern_ = 0;
  }
}

lefiSite::~lefiSite()
{
  Destroy();
}

void lefiSite::setName(const char* name)
{
  int i;
  int len = strlen(name) + 1;
  if (len > nameSize_) {
    lefFree(name_);
    name_ = (char*) lefMalloc(len);
    nameSize_ = len;
  }
  strcpy(name_, CASE(name));
  hasClass_ = 0;
  *(siteClass_) = 0;
  hasSize_ = 0;
  symmetry_ = 0;
  if (numRowPattern_) {
    for (i = 0; i < numRowPattern_; i++) {
      lefFree(siteNames_[i]);
    }
    numRowPattern_ = 0;
  }
}

void lefiSite::setClass(const char* cls)
{
  strcpy(siteClass_, cls);
  hasClass_ = 1;
}

void lefiSite::setSize(double x, double y)
{
  hasSize_ = 1;
  sizeX_ = x;
  sizeY_ = y;
}

void lefiSite::setXSymmetry()
{
  symmetry_ |= 1;
}

void lefiSite::setYSymmetry()
{
  symmetry_ |= 2;
}

void lefiSite::set90Symmetry()
{
  symmetry_ |= 4;
}

void lefiSite::addRowPattern(const char* name, int orient)
{
  if (numRowPattern_ == rowPatternAllocated_) {
    int i;
    char** sn;
    int* so;

    rowPatternAllocated_
        = (rowPatternAllocated_ == 0) ? 2 : rowPatternAllocated_ * 2;
    sn = (char**) lefMalloc(sizeof(char*) * rowPatternAllocated_);
    so = (int*) lefMalloc(sizeof(int) * rowPatternAllocated_);
    for (i = 0; i < numRowPattern_; i++) {
      sn[i] = siteNames_[i];
      so[i] = siteOrients_[i];
    }
    if (siteNames_) {
      lefFree(siteNames_);
      lefFree(siteOrients_);
    }
    siteNames_ = sn;
    siteOrients_ = so;
  }
  siteNames_[numRowPattern_] = strdup(name);
  siteOrients_[numRowPattern_] = orient;
  numRowPattern_ += 1;
}

std::vector<std::pair<std::string, std::string>> lefiSite::getRowPatterns()
    const
{
  std::vector<std::pair<std::string, std::string>> patterns(numRowPattern_);
  for (int i = 0; i < numRowPattern_; i++) {
    patterns[i] = {siteName(i), siteOrientStr(i)};
  }
  return patterns;
}

const char* lefiSite::name() const
{
  return name_;
}

int lefiSite::hasClass() const
{
  return hasClass_;
}

const char* lefiSite::siteClass() const
{
  return siteClass_;
}

double lefiSite::sizeX() const
{
  return sizeX_;
}

double lefiSite::sizeY() const
{
  return sizeY_;
}

int lefiSite::hasSize() const
{
  return hasSize_;
}

int lefiSite::hasXSymmetry() const
{
  return (symmetry_ & 1) ? 1 : 0;
}

int lefiSite::hasYSymmetry() const
{
  return (symmetry_ & 2) ? 1 : 0;
}

int lefiSite::has90Symmetry() const
{
  return (symmetry_ & 4) ? 1 : 0;
}

int lefiSite::hasRowPattern() const
{
  return (numRowPattern_) ? 1 : 0;
}

int lefiSite::numSites() const
{
  return (numRowPattern_);
}

char* lefiSite::siteName(int index) const
{
  return (siteNames_[index]);
}

int lefiSite::siteOrient(int index) const
{
  return (siteOrients_[index]);
}

char* lefiSite::siteOrientStr(int index) const
{
  return (lefiOrientStr(siteOrients_[index]));
}

void lefiSite::print(FILE* f) const
{
  fprintf(f, "SITE %s", name());

  if (hasClass()) {
    fprintf(f, " CLASS %s", siteClass());
  }

  if (hasSize()) {
    fprintf(f, " SIZE %g %g", sizeX(), sizeY());
  }

  if (hasXSymmetry()) {
    fprintf(f, " SYMMETRY X");
  }

  if (hasYSymmetry()) {
    fprintf(f, " SYMMETRY Y");
  }

  if (has90Symmetry()) {
    fprintf(f, " SYMMETRY R90");
  }

  fprintf(f, "\n");
}

// *****************************************************************************
// lefiSitePattern
// *****************************************************************************

lefiSitePattern::lefiSitePattern()
{
  Init();
}

void lefiSitePattern::Init()
{
  nameSize_ = 16;
  name_ = (char*) lefMalloc(16);
}

void lefiSitePattern::Destroy()
{
  lefFree(name_);
}

lefiSitePattern::~lefiSitePattern()
{
  Destroy();
}

void lefiSitePattern::set(const char* name,
                          double x,
                          double y,
                          int orient,
                          double xStart,
                          double yStart,
                          double xStep,
                          double yStep)
{
  int len = strlen(name) + 1;
  if (len > nameSize_) {
    lefFree(name_);
    name_ = (char*) lefMalloc(len);
    nameSize_ = len;
  }
  strcpy(name_, CASE(name));

  x_ = x;
  y_ = y;
  xStep_ = xStep;
  yStep_ = yStep;
  xStart_ = xStart;
  yStart_ = yStart;
  orient_ = orient;
}

const char* lefiSitePattern::name() const
{
  return name_;
}

int lefiSitePattern::orient() const
{
  return orient_;
}

const char* lefiSitePattern::orientStr() const
{
  return (lefiOrientStr(orient_));
}

double lefiSitePattern::x() const
{
  return x_;
}

double lefiSitePattern::y() const
{
  return y_;
}

int lefiSitePattern::hasStepPattern() const
{
  if (xStart_ == -1 && yStart_ == -1 && xStep_ == -1 && yStep_ == -1) {
    return 0;
  }
  return 1;
}

double lefiSitePattern::xStart() const
{
  return xStart_;
}

double lefiSitePattern::yStart() const
{
  return yStart_;
}

double lefiSitePattern::xStep() const
{
  return xStep_;
}

double lefiSitePattern::yStep() const
{
  return yStep_;
}

void lefiSitePattern::print(FILE* f) const
{
  fprintf(f, "  SITE Pattern %s  %g,%g %s\n", name(), x(), y(), orientStr());
  fprintf(f, "    %g,%g step %g,%g\n", xStart(), yStart(), xStep(), yStep());
}

// *****************************************************************************
// lefiTrackPattern
// *****************************************************************************

lefiTrackPattern::lefiTrackPattern()
{
  Init();
}

void lefiTrackPattern::Init()
{
  nameSize_ = 16;
  name_ = (char*) lefMalloc(16);
  start_ = 0;
  numTracks_ = 0;
  space_ = 0;
  numLayers_ = 0;
  layerAllocated_ = 2;
  layerNames_ = (char**) lefMalloc(sizeof(char*) * 2);
  clear();
}

void lefiTrackPattern::Destroy()
{
  if (name_) {
    lefFree(name_);
  }
  clear();
  name_ = nullptr;
  start_ = 0;
  numTracks_ = 0;
  space_ = 0;
  lefFree(layerNames_);
}

void lefiTrackPattern::clear()
{
  int i;
  for (i = 0; i < numLayers_; i++) {
    lefFree(layerNames_[i]);
  }
}

lefiTrackPattern::~lefiTrackPattern()
{
  Destroy();
}

void lefiTrackPattern::set(const char* name,
                           double start,
                           int numTracks,
                           double space)
{
  int len = strlen(name) + 1;
  if (len > nameSize_) {
    lefFree(name_);
    name_ = (char*) lefMalloc(len);
    nameSize_ = len;
  }
  strcpy(name_, CASE(name));

  start_ = start;
  numTracks_ = numTracks;
  space_ = space;
}

void lefiTrackPattern::addLayer(const char* name)
{
  int len;
  if (numLayers_ == layerAllocated_) {
    int i;
    char** nn;

    if (layerAllocated_ == 0) {
      layerAllocated_ = 2;
    } else {
      layerAllocated_ *= 2;
    }
    nn = (char**) lefMalloc(sizeof(char*) * layerAllocated_);
    for (i = 0; i < numLayers_; i++) {
      nn[i] = layerNames_[i];
    }
    lefFree(layerNames_);
    layerNames_ = nn;
  }
  len = strlen(name) + 1;
  layerNames_[numLayers_] = (char*) lefMalloc(len);
  strcpy(layerNames_[numLayers_], CASE(name));
  numLayers_ += 1;
}

const char* lefiTrackPattern::name() const
{
  return name_;
}

double lefiTrackPattern::start() const
{
  return start_;
}

int lefiTrackPattern::numTracks() const
{
  return numTracks_;
}

double lefiTrackPattern::space() const
{
  return space_;
}

int lefiTrackPattern::numLayers() const
{
  return numLayers_;
}

const char* lefiTrackPattern::layerName(int index) const
{
  char msg[160];
  if (index < 0 || index >= numLayers_) {
    sprintf(msg,
            "ERROR (LEFPARS-1377): The index number %d given for the TRACK "
            "PATTERN  is invalid.\nValid index is from 0 to %d",
            index,
            numLayers_);
    lefiError(0, 1377, msg);
    return nullptr;
  }
  return layerNames_[index];
}

void lefiTrackPattern::print(FILE* f) const
{
  int i;
  fprintf(f,
          "  TRACK Pattern %s  %g DO %d STEP %g\n",
          name(),
          start(),
          numTracks(),
          space());
  if (numLayers() > 0) {
    fprintf(f, "    LAYER ");
    for (i = 0; i < numLayers(); i++) {
      fprintf(f, "%s ", layerName(i));
    }
    fprintf(f, "\n");
  }
}

// *****************************************************************************
// lefiGcellPattern
// *****************************************************************************

lefiGcellPattern::lefiGcellPattern()
{
  Init();
}

void lefiGcellPattern::Init()
{
  nameSize_ = 16;
  name_ = (char*) lefMalloc(16);
  start_ = 0;
  numCRs_ = 0;
  space_ = 0;
}

void lefiGcellPattern::Destroy()
{
  if (name_) {
    lefFree(name_);
  }
  name_ = nullptr;
  start_ = 0;
  numCRs_ = 0;
  space_ = 0;
}

lefiGcellPattern::~lefiGcellPattern()
{
  Destroy();
}

void lefiGcellPattern::set(const char* name,
                           double start,
                           int numCRs,
                           double space)
{
  int len = strlen(name) + 1;
  if (len > nameSize_) {
    lefFree(name_);
    name_ = (char*) lefMalloc(len);
    nameSize_ = len;
  }
  strcpy(name_, CASE(name));

  start_ = start;
  numCRs_ = numCRs;
  space_ = space;
}

const char* lefiGcellPattern::name() const
{
  return name_;
}

double lefiGcellPattern::start() const
{
  return start_;
}

int lefiGcellPattern::numCRs() const
{
  return numCRs_;
}

double lefiGcellPattern::space() const
{
  return space_;
}

void lefiGcellPattern::print(FILE* f) const
{
  fprintf(f,
          "  TRACK Pattern %s  %g DO %d STEP %g\n",
          name(),
          start(),
          numCRs(),
          space());
}

// *****************************************************************************
// lefiUseMinSpacing
// *****************************************************************************

lefiUseMinSpacing::lefiUseMinSpacing()
{
  Init();
}

void lefiUseMinSpacing::Init()
{
  name_ = nullptr;
  value_ = 0;
}

void lefiUseMinSpacing::Destroy()
{
  if (name_) {
    lefFree(name_);
  }
}

lefiUseMinSpacing::~lefiUseMinSpacing()
{
  Destroy();
}

void lefiUseMinSpacing::set(const char* name, int value)
{
  Destroy();  // lefFree previous name, if there is any
  name_ = (char*) lefMalloc(strlen(name) + 1);
  strcpy(name_, CASE(name));
  value_ = value;
}

const char* lefiUseMinSpacing::name() const
{
  return name_;
}

int lefiUseMinSpacing::value() const
{
  return value_;
}

void lefiUseMinSpacing::print(FILE* f) const
{
  fprintf(f, "USEMINSPACING %s %d\n", name(), value());
}

// *****************************************************************************
// lefiMaxStackVia
// *****************************************************************************

lefiMaxStackVia::lefiMaxStackVia()
{
  bottomLayer_ = nullptr;
  topLayer_ = nullptr;
  Init();
}

void lefiMaxStackVia::Init()
{
  value_ = 0;
  hasRange_ = 0;
  if (bottomLayer_) {       // This is for C version, since C will
    lefFree(bottomLayer_);  // call this function before calling
  }
  if (topLayer_) {       // setMaxStackViaRange when more than 1 lef
    lefFree(topLayer_);  // files are parse. C++ skips this function
  }
  bottomLayer_ = nullptr;
  topLayer_ = nullptr;
}

void lefiMaxStackVia::Destroy()
{
  if (bottomLayer_) {
    lefFree(bottomLayer_);
  }
  if (topLayer_) {
    lefFree(topLayer_);
  }
  bottomLayer_ = nullptr;
  topLayer_ = nullptr;
  hasRange_ = 0;
  value_ = 0;
}

lefiMaxStackVia::~lefiMaxStackVia()
{
  Destroy();
}

void lefiMaxStackVia::setMaxStackVia(int value)
{
  value_ = value;
}

void lefiMaxStackVia::setMaxStackViaRange(const char* bottomLayer,
                                          const char* topLayer)
{
  hasRange_ = 1;
  if (bottomLayer_) {       // May be lefrReset is called and
    lefFree(bottomLayer_);  // bottomLayer_ and/or topLayer_ have
  }
  if (topLayer_) {  // value malloc on them
    lefFree(topLayer_);
  }
  bottomLayer_ = (char*) lefMalloc(strlen(bottomLayer) + 1);
  strcpy(bottomLayer_, CASE(bottomLayer));
  topLayer_ = (char*) lefMalloc(strlen(topLayer) + 1);
  strcpy(topLayer_, CASE(topLayer));
  // bottomLayer_ = strdup(bottomLayer);
  // topLayer_    = strdup(topLayer);
}

int lefiMaxStackVia::maxStackVia() const
{
  return value_;
}

int lefiMaxStackVia::hasMaxStackViaRange() const
{
  return hasRange_ ? 1 : 0;
}

const char* lefiMaxStackVia::maxStackViaBottomLayer() const
{
  return bottomLayer_;
}

const char* lefiMaxStackVia::maxStackViaTopLayer() const
{
  return topLayer_;
}

void lefiMaxStackVia::print(FILE* f) const
{
  fprintf(f, "MAXVIASTACK %d", maxStackVia());
  if (hasMaxStackViaRange()) {
    fprintf(f, " RANGE %s %s", maxStackViaBottomLayer(), maxStackViaTopLayer());
  }
  fprintf(f, "\n");
}
END_LEF_PARSER_NAMESPACE
