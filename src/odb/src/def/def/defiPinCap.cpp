// *****************************************************************************
// *****************************************************************************
// Copyright 2013 - 2019, Cadence Design Systems
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

#include "defiPinCap.hpp"

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "defiComponent.hpp"
#include "defiDebug.hpp"
#include "defiKRDefs.hpp"
#include "defiMisc.hpp"
#include "defiUtil.hpp"
#include "defrData.hpp"
#include "defrSettings.hpp"

BEGIN_DEF_PARSER_NAMESPACE

///////////////////////////////////////////////
///////////////////////////////////////////////
//
//     defiPinCap
//
///////////////////////////////////////////////
///////////////////////////////////////////////

void defiPinCap::setPin(int p)
{
  pin_ = p;
}

void defiPinCap::setCap(double d)
{
  cap_ = d;
}

int defiPinCap::pin() const
{
  return pin_;
}

double defiPinCap::cap() const
{
  return cap_;
}

void defiPinCap::print(FILE* f) const
{
  fprintf(f, "PinCap  %d %5.2f\n", pin_, cap_);
}

///////////////////////////////////////////////
///////////////////////////////////////////////
//
//     defiPinAntennaModel
//
///////////////////////////////////////////////
///////////////////////////////////////////////

defiPinAntennaModel::defiPinAntennaModel(defrData* data) : defData(data)
{
  Init();
}

void defiPinAntennaModel::Init()
{
  numAPinGateArea_ = 0;        // 5.4
  APinGateAreaAllocated_ = 0;  // 5.4
  APinGateArea_ = nullptr;
  APinGateAreaLayer_ = nullptr;
  numAPinMaxAreaCar_ = 0;        // 5.4
  APinMaxAreaCarAllocated_ = 0;  // 5.4
  APinMaxAreaCar_ = nullptr;
  APinMaxAreaCarLayer_ = nullptr;
  numAPinMaxSideAreaCar_ = 0;        // 5.4
  APinMaxSideAreaCarAllocated_ = 0;  // 5.4
  APinMaxSideAreaCar_ = nullptr;
  APinMaxSideAreaCarLayer_ = nullptr;
  numAPinMaxCutCar_ = 0;        // 5.4
  APinMaxCutCarAllocated_ = 0;  // 5.4
  APinMaxCutCar_ = nullptr;
  APinMaxCutCarLayer_ = nullptr;
}

defiPinAntennaModel::~defiPinAntennaModel()
{
  Destroy();
}

void defiPinAntennaModel::clear()
{
  int i;

  if (oxide_) {
    free(oxide_);
  }
  oxide_ = nullptr;

  for (i = 0; i < numAPinGateArea_; i++) {
    if (APinGateAreaLayer_[i]) {
      free(APinGateAreaLayer_[i]);
    }
  }
  numAPinGateArea_ = 0;

  for (i = 0; i < numAPinMaxAreaCar_; i++) {
    if (APinMaxAreaCarLayer_[i]) {
      free(APinMaxAreaCarLayer_[i]);
    }
  }
  numAPinMaxAreaCar_ = 0;

  for (i = 0; i < numAPinMaxSideAreaCar_; i++) {
    if (APinMaxSideAreaCarLayer_[i]) {
      free(APinMaxSideAreaCarLayer_[i]);
    }
  }
  numAPinMaxSideAreaCar_ = 0;

  for (i = 0; i < numAPinMaxCutCar_; i++) {
    if (APinMaxCutCarLayer_[i]) {
      free(APinMaxCutCarLayer_[i]);
    }
  }
  numAPinMaxCutCar_ = 0;
}

void defiPinAntennaModel::Destroy()
{
  clear();
  if (APinGateArea_) {
    free((char*) (APinGateArea_));
  }
  if (APinGateAreaLayer_) {
    free((char*) (APinGateAreaLayer_));
  }
  if (APinMaxAreaCar_) {
    free((char*) (APinMaxAreaCar_));
  }
  if (APinMaxAreaCarLayer_) {
    free((char*) (APinMaxAreaCarLayer_));
  }
  if (APinMaxSideAreaCar_) {
    free((char*) (APinMaxSideAreaCar_));
  }
  if (APinMaxSideAreaCarLayer_) {
    free((char*) (APinMaxSideAreaCarLayer_));
  }
  if (APinMaxCutCar_) {
    free((char*) (APinMaxCutCar_));
  }
  if (APinMaxCutCarLayer_) {
    free((char*) (APinMaxCutCarLayer_));
  }
}

// 5.5
void defiPinAntennaModel::setAntennaModel(int aOxide)
{
  if (oxide_) {
    free(oxide_);
  }

  if (aOxide < 1 || aOxide > defMaxOxides) {
    aOxide = 1;
  }

  oxide_ = strdup(defrSettings::defOxides[aOxide - 1]);
}

void defiPinAntennaModel::addAPinGateArea(int value, const char* layer)
{
  if (numAPinGateArea_ == APinGateAreaAllocated_) {
    int i;
    int max;
    int lim = numAPinGateArea_;
    int* nd;
    char** nl;

    if (APinGateAreaAllocated_ == 0) {
      max = APinGateAreaAllocated_ = 2;
    } else {
      max = APinGateAreaAllocated_ *= 2;
    }
    nd = (int*) malloc(sizeof(int) * max);
    nl = (char**) malloc(sizeof(char*) * max);
    for (i = 0; i < lim; i++) {
      nd[i] = APinGateArea_[i];
      nl[i] = APinGateAreaLayer_[i];
    }
    free((char*) (APinGateArea_));
    free((char*) (APinGateAreaLayer_));
    APinGateArea_ = nd;
    APinGateAreaLayer_ = nl;
  }
  APinGateArea_[numAPinGateArea_] = value;
  if (layer) {
    APinGateAreaLayer_[numAPinGateArea_] = (char*) malloc(strlen(layer) + 1);
    strcpy(APinGateAreaLayer_[numAPinGateArea_], defData->DEFCASE(layer));
  } else {
    APinGateAreaLayer_[numAPinGateArea_] = nullptr;
  }
  numAPinGateArea_ += 1;
}

void defiPinAntennaModel::addAPinMaxAreaCar(int value, const char* layer)
{
  if (numAPinMaxAreaCar_ == APinMaxAreaCarAllocated_) {
    int i;
    int max;
    int lim = numAPinMaxAreaCar_;
    int* nd;
    char** nl;

    if (APinMaxAreaCarAllocated_ == 0) {
      max = APinMaxAreaCarAllocated_ = 2;
    } else {
      max = APinMaxAreaCarAllocated_ *= 2;
    }
    nd = (int*) malloc(sizeof(int) * max);
    nl = (char**) malloc(sizeof(char*) * max);
    for (i = 0; i < lim; i++) {
      nd[i] = APinMaxAreaCar_[i];
      nl[i] = APinMaxAreaCarLayer_[i];
    }
    free((char*) (APinMaxAreaCar_));
    free((char*) (APinMaxAreaCarLayer_));
    APinMaxAreaCar_ = nd;
    APinMaxAreaCarLayer_ = nl;
  }
  APinMaxAreaCar_[numAPinMaxAreaCar_] = value;
  if (layer) {
    APinMaxAreaCarLayer_[numAPinMaxAreaCar_]
        = (char*) malloc(strlen(layer) + 1);
    strcpy(APinMaxAreaCarLayer_[numAPinMaxAreaCar_], defData->DEFCASE(layer));
  } else {
    APinMaxAreaCarLayer_[numAPinMaxAreaCar_] = nullptr;
  }
  numAPinMaxAreaCar_ += 1;
}

void defiPinAntennaModel::addAPinMaxSideAreaCar(int value, const char* layer)
{
  if (numAPinMaxSideAreaCar_ == APinMaxSideAreaCarAllocated_) {
    int i;
    int max;
    int lim = numAPinMaxSideAreaCar_;
    int* nd;
    char** nl;

    if (APinMaxSideAreaCarAllocated_ == 0) {
      max = APinMaxSideAreaCarAllocated_ = 2;
    } else {
      max = APinMaxSideAreaCarAllocated_ *= 2;
    }
    nd = (int*) malloc(sizeof(int) * max);
    nl = (char**) malloc(sizeof(char*) * max);
    for (i = 0; i < lim; i++) {
      nd[i] = APinMaxSideAreaCar_[i];
      nl[i] = APinMaxSideAreaCarLayer_[i];
    }
    free((char*) (APinMaxSideAreaCar_));
    free((char*) (APinMaxSideAreaCarLayer_));
    APinMaxSideAreaCar_ = nd;
    APinMaxSideAreaCarLayer_ = nl;
  }
  APinMaxSideAreaCar_[numAPinMaxSideAreaCar_] = value;
  if (layer) {
    APinMaxSideAreaCarLayer_[numAPinMaxSideAreaCar_]
        = (char*) malloc(strlen(layer) + 1);
    strcpy(APinMaxSideAreaCarLayer_[numAPinMaxSideAreaCar_],
           defData->DEFCASE(layer));
  } else {
    APinMaxSideAreaCarLayer_[numAPinMaxSideAreaCar_] = nullptr;
  }
  numAPinMaxSideAreaCar_ += 1;
}

void defiPinAntennaModel::addAPinMaxCutCar(int value, const char* layer)
{
  if (numAPinMaxCutCar_ == APinMaxCutCarAllocated_) {
    int i;
    int max;
    int lim = numAPinMaxCutCar_;
    int* nd;
    char** nl;

    if (APinMaxCutCarAllocated_ == 0) {
      max = APinMaxCutCarAllocated_ = 2;
    } else {
      max = APinMaxCutCarAllocated_ *= 2;
    }
    nd = (int*) malloc(sizeof(int) * max);
    nl = (char**) malloc(sizeof(char*) * max);
    for (i = 0; i < lim; i++) {
      nd[i] = APinMaxCutCar_[i];
      nl[i] = APinMaxCutCarLayer_[i];
    }
    free((char*) (APinMaxCutCar_));
    free((char*) (APinMaxCutCarLayer_));
    APinMaxCutCar_ = nd;
    APinMaxCutCarLayer_ = nl;
  }
  APinMaxCutCar_[numAPinMaxCutCar_] = value;
  if (layer) {
    APinMaxCutCarLayer_[numAPinMaxCutCar_] = (char*) malloc(strlen(layer) + 1);
    strcpy(APinMaxCutCarLayer_[numAPinMaxCutCar_], defData->DEFCASE(layer));
  } else {
    APinMaxCutCarLayer_[numAPinMaxCutCar_] = nullptr;
  }
  numAPinMaxCutCar_ += 1;
}

// 5.5
char* defiPinAntennaModel::antennaOxide() const
{
  return oxide_;
}

int defiPinAntennaModel::hasAPinGateArea() const
{
  return numAPinGateArea_ ? 1 : 0;
}

int defiPinAntennaModel::hasAPinMaxAreaCar() const
{
  return numAPinMaxAreaCar_ ? 1 : 0;
}

int defiPinAntennaModel::hasAPinMaxSideAreaCar() const
{
  return numAPinMaxSideAreaCar_ ? 1 : 0;
}

int defiPinAntennaModel::hasAPinMaxCutCar() const
{
  return numAPinMaxCutCar_ ? 1 : 0;
}

int defiPinAntennaModel::numAPinGateArea() const
{
  return numAPinGateArea_;
}

int defiPinAntennaModel::numAPinMaxAreaCar() const
{
  return numAPinMaxAreaCar_;
}

int defiPinAntennaModel::numAPinMaxSideAreaCar() const
{
  return numAPinMaxSideAreaCar_;
}

int defiPinAntennaModel::numAPinMaxCutCar() const
{
  return numAPinMaxCutCar_;
}

int defiPinAntennaModel::APinGateArea(int i) const
{
  return APinGateArea_[i];
}

int defiPinAntennaModel::hasAPinGateAreaLayer(int i) const
{
  return (APinGateAreaLayer_[i] && *(APinGateAreaLayer_[i])) ? 1 : 0;
}

const char* defiPinAntennaModel::APinGateAreaLayer(int i) const
{
  return APinGateAreaLayer_[i];
}

int defiPinAntennaModel::APinMaxAreaCar(int i) const
{
  return APinMaxAreaCar_[i];
}

int defiPinAntennaModel::hasAPinMaxAreaCarLayer(int i) const
{
  return (APinMaxAreaCarLayer_[i] && *(APinMaxAreaCarLayer_[i])) ? 1 : 0;
}

const char* defiPinAntennaModel::APinMaxAreaCarLayer(int i) const
{
  return APinMaxAreaCarLayer_[i];
}

int defiPinAntennaModel::APinMaxSideAreaCar(int i) const
{
  return APinMaxSideAreaCar_[i];
}

int defiPinAntennaModel::hasAPinMaxSideAreaCarLayer(int i) const
{
  return (APinMaxSideAreaCarLayer_[i] && *(APinMaxSideAreaCarLayer_[i])) ? 1
                                                                         : 0;
}

const char* defiPinAntennaModel::APinMaxSideAreaCarLayer(int i) const
{
  return APinMaxSideAreaCarLayer_[i];
}

int defiPinAntennaModel::APinMaxCutCar(int i) const
{
  return APinMaxCutCar_[i];
}

int defiPinAntennaModel::hasAPinMaxCutCarLayer(int i) const
{
  return (APinMaxCutCarLayer_[i] && *(APinMaxCutCarLayer_[i])) ? 1 : 0;
}

const char* defiPinAntennaModel::APinMaxCutCarLayer(int i) const
{
  return APinMaxCutCarLayer_[i];
}

///////////////////////////////////////////////
///////////////////////////////////////////////
//
//     defiPinPort
//
///////////////////////////////////////////////
///////////////////////////////////////////////

defiPinPort::defiPinPort(defrData* data) : defData(data)
{
  Init();
}

void defiPinPort::Init()
{
  layersAllocated_ = 0;
  numLayers_ = 0;
  layers_ = nullptr;
  layerMinSpacing_ = nullptr;
  layerMask_ = nullptr;
  layerEffectiveWidth_ = nullptr;
  xl_ = nullptr;
  yl_ = nullptr;
  xh_ = nullptr;
  yh_ = nullptr;
  polysAllocated_ = 0;
  numPolys_ = 0;
  polygonNames_ = nullptr;
  polyMinSpacing_ = nullptr;
  polyMask_ = nullptr;
  polyEffectiveWidth_ = nullptr;
  polygons_ = nullptr;
  viasAllocated_ = 0;
  numVias_ = 0;
  viaNames_ = nullptr;
  viaX_ = nullptr;
  viaY_ = nullptr;
  viaMask_ = nullptr;
  placeType_ = 0;
  x_ = 0;
  y_ = 0;
  orient_ = 0;
}

defiPinPort::~defiPinPort()
{
  clear();
}

void defiPinPort::clear()
{
  int i;

  placeType_ = 0;
  orient_ = 0;
  x_ = 0;
  y_ = 0;

  if (layers_) {
    for (i = 0; i < numLayers_; i++) {
      if (layers_[i]) {
        free(layers_[i]);
      }
    }
    free((char*) (layers_));
    free((char*) (xl_));
    free((char*) (yl_));
    free((char*) (xh_));
    free((char*) (yh_));
    free((char*) (layerMinSpacing_));
    free((char*) (layerMask_));
    free((char*) (layerEffectiveWidth_));
  }
  layers_ = nullptr;
  layerMinSpacing_ = nullptr;
  layerEffectiveWidth_ = nullptr;
  layerMask_ = nullptr;
  numLayers_ = 0;
  layersAllocated_ = 0;
  if (polygonNames_) {
    struct defiPoints* p;
    for (i = 0; i < numPolys_; i++) {
      if (polygonNames_[i]) {
        free(polygonNames_[i]);
      }
      p = polygons_[i];
      free((char*) (p->x));
      free((char*) (p->y));
      free((char*) (polygons_[i]));
    }
    free((char*) (polygonNames_));
    free((char*) (polygons_));
    free((char*) (polyMinSpacing_));
    free((char*) (polyMask_));
    free((char*) (polyEffectiveWidth_));
    polygonNames_ = nullptr;
    polygons_ = nullptr;
    polyMinSpacing_ = nullptr;
    polyEffectiveWidth_ = nullptr;
    polyMask_ = nullptr;
  }
  numPolys_ = 0;
  polysAllocated_ = 0;
  if (viaNames_) {
    for (i = 0; i < numVias_; i++) {
      if (viaNames_[i]) {
        free(viaNames_[i]);
      }
    }
    free((char*) (viaNames_));
    free((char*) (viaX_));
    free((char*) (viaY_));
    free((char*) (viaMask_));
  }
  viaNames_ = nullptr;
  numVias_ = 0;
  viasAllocated_ = 0;
  viaMask_ = nullptr;
}

void defiPinPort::addLayer(const char* layer)
{
  if (numLayers_ >= layersAllocated_) {
    int i;
    char** newl;
    int *nxl, *nyl, *nxh, *nyh;
    int *lms, *lew, *lm;

    layersAllocated_ = layersAllocated_ ? layersAllocated_ * 2 : 8;
    newl = (char**) malloc(layersAllocated_ * sizeof(char*));
    nxl = (int*) malloc(layersAllocated_ * sizeof(int));
    nyl = (int*) malloc(layersAllocated_ * sizeof(int));
    nxh = (int*) malloc(layersAllocated_ * sizeof(int));
    nyh = (int*) malloc(layersAllocated_ * sizeof(int));
    lms = (int*) malloc(layersAllocated_ * sizeof(int));
    lew = (int*) malloc(layersAllocated_ * sizeof(int));
    lm = (int*) malloc(layersAllocated_ * sizeof(int));

    for (i = 0; i < numLayers_; i++) {
      newl[i] = layers_[i];
      nxl[i] = xl_[i];
      nyl[i] = yl_[i];
      nxh[i] = xh_[i];
      nyh[i] = yh_[i];
      lms[i] = layerMinSpacing_[i];
      lew[i] = layerEffectiveWidth_[i];
      lm[i] = layerMask_[i];
    }
    if (numLayers_ > 0) {
      free((char*) layers_);
      free((char*) xl_);
      free((char*) yl_);
      free((char*) xh_);
      free((char*) yh_);
      free((char*) layerMinSpacing_);
      free((char*) layerEffectiveWidth_);
      free((char*) layerMask_);
    }
    layers_ = newl;
    xl_ = nxl;
    yl_ = nyl;
    xh_ = nxh;
    yh_ = nyh;
    layerMinSpacing_ = lms;
    layerEffectiveWidth_ = lew;
    layerMask_ = lm;
  }
  layers_[numLayers_] = (char*) malloc(strlen(layer) + 1);
  strcpy(layers_[numLayers_], defData->DEFCASE(layer));
  xl_[numLayers_] = 0;
  yl_[numLayers_] = 0;
  xh_[numLayers_] = 0;
  yh_[numLayers_] = 0;
  layerMinSpacing_[numLayers_] = -1;
  layerEffectiveWidth_[numLayers_] = -1;
  layerMask_[numLayers_] = 0;
  numLayers_ += 1;
}

void defiPinPort::addLayerSpacing(int minSpacing)
{
  layerMinSpacing_[numLayers_ - 1] = minSpacing;
}

void defiPinPort::addLayerMask(int mask)
{
  layerMask_[numLayers_ - 1] = mask;
}

void defiPinPort::addLayerDesignRuleWidth(int effectiveWidth)
{
  layerEffectiveWidth_[numLayers_ - 1] = effectiveWidth;
}

void defiPinPort::addLayerPts(int xl, int yl, int xh, int yh)
{
  xl_[numLayers_ - 1] = xl;
  yl_[numLayers_ - 1] = yl;
  xh_[numLayers_ - 1] = xh;
  yh_[numLayers_ - 1] = yh;
}

void defiPinPort::addPolygon(const char* layerName)
{
  int *pms, *pdw, *pm;
  int i;

  if (numPolys_ == polysAllocated_) {
    char** newn;
    struct defiPoints** poly;
    polysAllocated_ = (polysAllocated_ == 0) ? 2 : polysAllocated_ * 2;
    newn = (char**) malloc(sizeof(char*) * polysAllocated_);
    poly = (struct defiPoints**) malloc(sizeof(struct defiPoints*)
                                        * polysAllocated_);
    pms = (int*) malloc(polysAllocated_ * sizeof(int));
    pdw = (int*) malloc(polysAllocated_ * sizeof(int));
    pm = (int*) malloc(polysAllocated_ * sizeof(int));

    for (i = 0; i < numPolys_; i++) {
      newn[i] = polygonNames_[i];
      poly[i] = polygons_[i];
      pms[i] = polyMinSpacing_[i];
      pdw[i] = polyEffectiveWidth_[i];
      pm[i] = polyMask_[i];
    }
    if (numPolys_ > 0) {
      free((char*) (polygons_));
      free((char*) (polygonNames_));
      free((char*) (polyMinSpacing_));
      free((char*) (polyEffectiveWidth_));
      free((char*) (polyMask_));
    }
    polygonNames_ = newn;
    polygons_ = poly;
    polyMinSpacing_ = pms;
    polyEffectiveWidth_ = pdw;
    polyMask_ = pm;
  }
  polygonNames_[numPolys_] = strdup(layerName);
  polygons_[numPolys_] = nullptr;
  polyMinSpacing_[numPolys_] = -1;
  polyEffectiveWidth_[numPolys_] = -1;
  polyMask_[numPolys_] = 0;
  numPolys_ += 1;
}

void defiPinPort::addPolySpacing(int minSpacing)
{
  polyMinSpacing_[numPolys_ - 1] = minSpacing;
}

void defiPinPort::addPolyMask(int color)
{
  polyMask_[numPolys_ - 1] = color;
}

void defiPinPort::addPolyDesignRuleWidth(int effectiveWidth)
{
  polyEffectiveWidth_[numPolys_ - 1] = effectiveWidth;
}

void defiPinPort::addPolygonPts(defiGeometries* geom)
{
  struct defiPoints* p;
  int x, y;
  int i;

  p = (struct defiPoints*) malloc(sizeof(struct defiPoints));
  p->numPoints = geom->numPoints();
  p->x = (int*) malloc(sizeof(int) * p->numPoints);
  p->y = (int*) malloc(sizeof(int) * p->numPoints);
  for (i = 0; i < p->numPoints; i++) {
    geom->points(i, &x, &y);
    p->x[i] = x;
    p->y[i] = y;
  }
  polygons_[numPolys_ - 1] = p;
}

void defiPinPort::addVia(const char* viaName, int ptX, int ptY, int color)
{
  if (numVias_ >= viasAllocated_) {
    int i;
    char** newl;
    int *nx, *ny, *nm;

    viasAllocated_ = viasAllocated_ ? viasAllocated_ * 2 : 8;
    newl = (char**) malloc(viasAllocated_ * sizeof(char*));
    nx = (int*) malloc(viasAllocated_ * sizeof(int));
    ny = (int*) malloc(viasAllocated_ * sizeof(int));
    nm = (int*) malloc(viasAllocated_ * sizeof(int));
    for (i = 0; i < numVias_; i++) {
      newl[i] = viaNames_[i];
      nx[i] = viaX_[i];
      ny[i] = viaY_[i];
      nm[i] = viaMask_[i];
    }
    if (numVias_ > 0) {
      free((char*) viaNames_);
      free((char*) viaX_);
      free((char*) viaY_);
      free((char*) viaMask_);
    }
    viaNames_ = newl;
    viaX_ = nx;
    viaY_ = ny;
    viaMask_ = nm;
  }
  viaNames_[numVias_] = (char*) malloc(strlen(viaName) + 1);
  strcpy(viaNames_[numVias_], defData->DEFCASE(viaName));
  viaX_[numVias_] = ptX;
  viaY_[numVias_] = ptY;
  viaMask_[numVias_] = color;
  numVias_ += 1;
}

void defiPinPort::setPlacement(int typ, int x, int y, int orient)
{
  x_ = x;
  y_ = y;
  orient_ = orient;
  placeType_ = typ;
}

int defiPinPort::numLayer() const
{
  return numLayers_;
}

const char* defiPinPort::layer(int index) const
{
  return layers_[index];
}

void defiPinPort::bounds(int index, int* xl, int* yl, int* xh, int* yh) const
{
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

int defiPinPort::hasLayerSpacing(int index) const
{
  if (layerMinSpacing_[index] == -1) {
    return 0;
  }
  return 1;
}

int defiPinPort::hasLayerDesignRuleWidth(int index) const
{
  if (layerEffectiveWidth_[index] == -1) {
    return 0;
  }
  return 1;
}

int defiPinPort::layerSpacing(int index) const
{
  return layerMinSpacing_[index];
}

int defiPinPort::layerMask(int index) const
{
  return layerMask_[index];
}

int defiPinPort::layerDesignRuleWidth(int index) const
{
  return layerEffectiveWidth_[index];
}

int defiPinPort::numPolygons() const
{
  return numPolys_;
}

const char* defiPinPort::polygonName(int index) const
{
  if (index < 0 || index > numPolys_) {
    defiError(1, 0, "index out of bounds", defData);
    return nullptr;
  }
  return polygonNames_[index];
}

struct defiPoints defiPinPort::getPolygon(int index) const
{
  return *(polygons_[index]);
}

int defiPinPort::hasPolygonSpacing(int index) const
{
  if (polyMinSpacing_[index] == -1) {
    return 0;
  }
  return 1;
}

int defiPinPort::hasPolygonDesignRuleWidth(int index) const
{
  if (polyEffectiveWidth_[index] == -1) {
    return 0;
  }
  return 1;
}

int defiPinPort::polygonSpacing(int index) const
{
  return polyMinSpacing_[index];
}

int defiPinPort::polygonMask(int index) const
{
  return polyMask_[index];
}

int defiPinPort::polygonDesignRuleWidth(int index) const
{
  return polyEffectiveWidth_[index];
}

int defiPinPort::numVias() const
{
  return numVias_;
}

const char* defiPinPort::viaName(int index) const
{
  if (index < 0 || index > numVias_) {
    defiError(1, 0, "index out of bounds", defData);
    return nullptr;
  }
  return viaNames_[index];
}

int defiPinPort::viaPtX(int index) const
{
  return viaX_[index];
}

int defiPinPort::viaPtY(int index) const
{
  return viaY_[index];
}

int defiPinPort::viaBottomMask(int index) const
{
  return viaMask_[index] % 10;
}

int defiPinPort::viaTopMask(int index) const
{
  return viaMask_[index] / 100;
}

int defiPinPort::viaCutMask(int index) const
{
  return viaMask_[index] / 10 % 10;
}

int defiPinPort::hasPlacement() const
{
  return placeType_ == 0 ? 0 : 1;
}

int defiPinPort::isPlaced() const
{
  return placeType_ == DEFI_COMPONENT_PLACED ? 1 : 0;
}

int defiPinPort::isCover() const
{
  return placeType_ == DEFI_COMPONENT_COVER ? 1 : 0;
}

int defiPinPort::isFixed() const
{
  return placeType_ == DEFI_COMPONENT_FIXED ? 1 : 0;
}

int defiPinPort::placementX() const
{
  return x_;
}

int defiPinPort::placementY() const
{
  return y_;
}

int defiPinPort::orient() const
{
  return orient_;
}

const char* defiPinPort::orientStr() const
{
  return (defiOrientStr(orient_));
}

///////////////////////////////////////////////
///////////////////////////////////////////////
//
//     defiPin
//
///////////////////////////////////////////////
///////////////////////////////////////////////

defiPin::defiPin(defrData* data) : defData(data)
{
  Init();
}

void defiPin::Init()
{
  pinNameLength_ = 0;
  pinName_ = nullptr;
  netNameLength_ = 0;
  netName_ = nullptr;
  useLength_ = 0;
  use_ = nullptr;
  directionLength_ = 0;
  direction_ = nullptr;
  hasDirection_ = 0;
  hasUse_ = 0;
  placeType_ = 0;
  orient_ = 0;
  xl_ = nullptr;
  yl_ = nullptr;
  xh_ = nullptr;
  yh_ = nullptr;
  x_ = 0;
  y_ = 0;
  netExprLength_ = 0;                  // 5.6
  netExpr_ = nullptr;                  // 5.6
  hasNetExpr_ = 0;                     // 5.6
  supplySensLength_ = 0;               // 5.6
  supplySens_ = nullptr;               // 5.6
  hasSupplySens_ = 0;                  // 5.6
  groundSensLength_ = 0;               // 5.6
  groundSens_ = nullptr;               // 5.6
  hasGroundSens_ = 0;                  // 5.6
  layers_ = nullptr;                   // 5.6
  layersAllocated_ = 0;                // 5.6
  numLayers_ = 0;                      // 5.6
  polygonNames_ = nullptr;             // 5.6
  numPolys_ = 0;                       // 5.6
  polysAllocated_ = 0;                 // 5.6
  polygons_ = nullptr;                 // 5.6
  numAPinPartialMetalArea_ = 0;        // 5.4
  APinPartialMetalAreaAllocated_ = 0;  // 5.4
  APinPartialMetalArea_ = nullptr;
  APinPartialMetalAreaLayer_ = nullptr;
  numAPinPartialMetalSideArea_ = 0;        // 5.4
  APinPartialMetalSideAreaAllocated_ = 0;  // 5.4
  APinPartialMetalSideArea_ = nullptr;
  APinPartialMetalSideAreaLayer_ = nullptr;
  numAPinDiffArea_ = 0;        // 5.4
  APinDiffAreaAllocated_ = 0;  // 5.4
  APinDiffArea_ = nullptr;
  APinDiffAreaLayer_ = nullptr;
  numAPinPartialCutArea_ = 0;        // 5.4
  APinPartialCutAreaAllocated_ = 0;  // 5.4
  APinPartialCutArea_ = nullptr;
  APinPartialCutAreaLayer_ = nullptr;
  antennaModel_ = nullptr;
  viaNames_ = nullptr;  // 5.7
  viasAllocated_ = 0;   // 5.7
  numVias_ = 0;         // 5.7
  viaX_ = nullptr;      // 5.7
  viaY_ = nullptr;      // 5.7
  numPorts_ = 0;        // 5.7
  pinPort_ = nullptr;   // 5.7
  numAntennaModel_ = 0;
  antennaModelAllocated_ = 0;
}

defiPin::~defiPin()
{
  Destroy();
}

void defiPin::clear()
{
  int i;

  hasDirection_ = 0;
  hasNetExpr_ = 0;
  hasSupplySens_ = 0;
  hasGroundSens_ = 0;
  hasUse_ = 0;
  hasSpecial_ = 0;
  placeType_ = 0;
  orient_ = 0;
  x_ = 0;
  y_ = 0;

  if (layers_) {
    for (i = 0; i < numLayers_; i++) {
      if (layers_[i]) {
        free(layers_[i]);
      }
    }
    free((char*) (layers_));
    free((char*) (xl_));
    free((char*) (yl_));
    free((char*) (xh_));
    free((char*) (yh_));
    free((char*) (layerMinSpacing_));
    free((char*) (layerMask_));
    free((char*) (layerEffectiveWidth_));
  }
  layers_ = nullptr;
  layerMinSpacing_ = nullptr;
  layerMask_ = nullptr;
  layerEffectiveWidth_ = nullptr;
  numLayers_ = 0;
  layersAllocated_ = 0;
  // 5.6
  if (polygonNames_) {
    struct defiPoints* p;
    for (i = 0; i < numPolys_; i++) {
      if (polygonNames_[i]) {
        free(polygonNames_[i]);
      }
      p = polygons_[i];
      free((char*) (p->x));
      free((char*) (p->y));
      free((char*) (polygons_[i]));
    }
    free((char*) (polygonNames_));
    free((char*) (polygons_));
    free((char*) (polyMinSpacing_));
    free((char*) (polyMask_));
    free((char*) (polyEffectiveWidth_));
    polygonNames_ = nullptr;
    polygons_ = nullptr;
    polyMinSpacing_ = nullptr;
    polyMask_ = nullptr;
    polyEffectiveWidth_ = nullptr;
  }
  numPolys_ = 0;
  polysAllocated_ = 0;
  // 5.7
  if (viaNames_) {
    for (i = 0; i < numVias_; i++) {
      if (viaNames_[i]) {
        free(viaNames_[i]);
      }
    }
    free((char*) (viaNames_));
    free((char*) (viaX_));
    free((char*) (viaY_));
    free((char*) (viaMask_));
  }
  viaNames_ = nullptr;
  numVias_ = 0;
  viaMask_ = nullptr;
  viasAllocated_ = 0;
  // 5.7
  if (pinPort_) {
    for (i = 0; i < numPorts_; i++) {
      if (pinPort_[i]) {
        pinPort_[i]->clear();
        delete pinPort_[i];
      }
    }
    free(pinPort_);
  }
  pinPort_ = nullptr;
  numPorts_ = 0;
  portsAllocated_ = 0;

  for (i = 0; i < numAPinPartialMetalArea_; i++) {
    if (APinPartialMetalAreaLayer_[i]) {
      free(APinPartialMetalAreaLayer_[i]);
    }
  }
  numAPinPartialMetalArea_ = 0;

  for (i = 0; i < numAPinPartialMetalSideArea_; i++) {
    if (APinPartialMetalSideAreaLayer_[i]) {
      free(APinPartialMetalSideAreaLayer_[i]);
    }
  }
  numAPinPartialMetalSideArea_ = 0;

  for (i = 0; i < numAPinDiffArea_; i++) {
    if (APinDiffAreaLayer_[i]) {
      free(APinDiffAreaLayer_[i]);
    }
  }
  numAPinDiffArea_ = 0;

  for (i = 0; i < numAPinPartialCutArea_; i++) {
    if (APinPartialCutAreaLayer_[i]) {
      free(APinPartialCutAreaLayer_[i]);
    }
  }
  numAPinPartialCutArea_ = 0;

  for (i = 0; i < antennaModelAllocated_; i++) {  // 5.5
    delete antennaModel_[i];
  }

  numAntennaModel_ = 0;
  antennaModelAllocated_ = 0;
}

void defiPin::Destroy()
{
  if (pinName_) {
    free(pinName_);
  }
  if (netName_) {
    free(netName_);
  }
  if (use_) {
    free(use_);
  }
  if (direction_) {
    free(direction_);
  }
  if (netExpr_) {
    free(netExpr_);
  }
  if (supplySens_) {
    free(supplySens_);
  }
  if (groundSens_) {
    free(groundSens_);
  }
  pinName_ = nullptr;
  netName_ = nullptr;
  use_ = nullptr;
  direction_ = nullptr;
  netExpr_ = nullptr;
  supplySens_ = nullptr;
  groundSens_ = nullptr;
  pinNameLength_ = 0;
  netNameLength_ = 0;
  useLength_ = 0;
  directionLength_ = 0;
  netExprLength_ = 0;
  supplySensLength_ = 0;
  groundSensLength_ = 0;
  layersAllocated_ = 0;
  clear();

  // 5.4
  if (APinPartialMetalArea_) {
    free((char*) (APinPartialMetalArea_));
  }
  if (APinPartialMetalAreaLayer_) {
    free((char*) (APinPartialMetalAreaLayer_));
  }
  if (APinPartialMetalSideArea_) {
    free((char*) (APinPartialMetalSideArea_));
  }
  if (APinPartialMetalSideAreaLayer_) {
    free((char*) (APinPartialMetalSideAreaLayer_));
  }
  if (APinDiffArea_) {
    free((char*) (APinDiffArea_));
  }
  if (APinDiffAreaLayer_) {
    free((char*) (APinDiffAreaLayer_));
  }
  if (APinPartialCutArea_) {
    free((char*) (APinPartialCutArea_));
  }
  if (APinPartialCutAreaLayer_) {
    free((char*) (APinPartialCutAreaLayer_));
  }
  if (antennaModel_) {
    free((char*) (antennaModel_));
  }
}

void defiPin::Setup(const char* pinName, const char* netName)
{
  int len = strlen(pinName) + 1;
  if (pinNameLength_ < len) {
    if (pinName_) {
      free(pinName_);
    }
    pinName_ = (char*) malloc(len);
    pinNameLength_ = len;
  }
  strcpy(pinName_, defData->DEFCASE(pinName));

  len = strlen(netName) + 1;
  if (netNameLength_ < len) {
    if (netName_) {
      free(netName_);
    }
    netName_ = (char*) malloc(len);
    netNameLength_ = len;
  }
  strcpy(netName_, defData->DEFCASE(netName));

  clear();
}

void defiPin::setDirection(const char* dir)
{
  int len = strlen(dir) + 1;
  if (directionLength_ < len) {
    if (direction_) {
      free(direction_);
    }
    direction_ = (char*) malloc(len);
    directionLength_ = len;
  }
  strcpy(direction_, defData->DEFCASE(dir));
  hasDirection_ = 1;
}

void defiPin::setNetExpr(const char* name)
{
  int len = strlen(name) + 1;
  if (netExprLength_ < len) {
    if (netExpr_) {
      free(netExpr_);
    }
    netExpr_ = (char*) malloc(len);
    netExprLength_ = len;
  }
  strcpy(netExpr_, defData->DEFCASE(name));
  hasNetExpr_ = 1;
}

void defiPin::setSupplySens(const char* name)
{
  int len = strlen(name) + 1;
  if (supplySensLength_ < len) {
    if (supplySens_) {
      free(supplySens_);
    }
    supplySens_ = (char*) malloc(len);
    supplySensLength_ = len;
  }
  strcpy(supplySens_, defData->DEFCASE(name));
  hasSupplySens_ = 1;
}

void defiPin::setGroundSens(const char* name)
{
  int len = strlen(name) + 1;
  if (groundSensLength_ < len) {
    if (groundSens_) {
      free(groundSens_);
    }
    groundSens_ = (char*) malloc(len);
    groundSensLength_ = len;
  }
  strcpy(groundSens_, defData->DEFCASE(name));
  hasGroundSens_ = 1;
}

void defiPin::setUse(const char* use)
{
  int len = strlen(use) + 1;
  if (useLength_ < len) {
    if (use_) {
      free(use_);
    }
    use_ = (char*) malloc(len);
    useLength_ = len;
  }
  strcpy(use_, defData->DEFCASE(use));
  hasUse_ = 1;
}

// 5.6, renamed from setLayer to addLayer for multiple layers allowed
void defiPin::addLayer(const char* layer)
{
  if (numLayers_ >= layersAllocated_) {
    int i;
    char** newl;
    int *nxl, *nyl, *nxh, *nyh;
    int *lms, *lew, *lm;

    layersAllocated_ = layersAllocated_ ? layersAllocated_ * 2 : 8;
    newl = (char**) malloc(layersAllocated_ * sizeof(char*));
    nxl = (int*) malloc(layersAllocated_ * sizeof(int));
    nyl = (int*) malloc(layersAllocated_ * sizeof(int));
    nxh = (int*) malloc(layersAllocated_ * sizeof(int));
    nyh = (int*) malloc(layersAllocated_ * sizeof(int));
    lms = (int*) malloc(layersAllocated_ * sizeof(int));
    lew = (int*) malloc(layersAllocated_ * sizeof(int));
    lm = (int*) malloc(layersAllocated_ * sizeof(int));

    for (i = 0; i < numLayers_; i++) {
      newl[i] = layers_[i];
      nxl[i] = xl_[i];
      nyl[i] = yl_[i];
      nxh[i] = xh_[i];
      nyh[i] = yh_[i];
      lms[i] = layerMinSpacing_[i];
      lew[i] = layerEffectiveWidth_[i];
      lm[i] = layerMask_[i];
    }
    if (numLayers_ > 0) {
      free((char*) layers_);
      free((char*) xl_);
      free((char*) yl_);
      free((char*) xh_);
      free((char*) yh_);
      free((char*) layerMinSpacing_);
      free((char*) layerMask_);
      free((char*) layerEffectiveWidth_);
    }
    layers_ = newl;
    xl_ = nxl;
    yl_ = nyl;
    xh_ = nxh;
    yh_ = nyh;
    layerMinSpacing_ = lms;
    layerEffectiveWidth_ = lew;
    layerMask_ = lm;
  }
  layers_[numLayers_] = (char*) malloc(strlen(layer) + 1);
  strcpy(layers_[numLayers_], defData->DEFCASE(layer));
  xl_[numLayers_] = 0;
  yl_[numLayers_] = 0;
  xh_[numLayers_] = 0;
  yh_[numLayers_] = 0;
  layerMinSpacing_[numLayers_] = -1;
  layerMask_[numLayers_] = 0;
  layerEffectiveWidth_[numLayers_] = -1;
  numLayers_ += 1;
}

// 5.6
void defiPin::addLayerPts(int xl, int yl, int xh, int yh)
{
  xl_[numLayers_ - 1] = xl;
  yl_[numLayers_ - 1] = yl;
  xh_[numLayers_ - 1] = xh;
  yh_[numLayers_ - 1] = yh;
}

// 5.6
void defiPin::addLayerSpacing(int minSpacing)
{
  layerMinSpacing_[numLayers_ - 1] = minSpacing;
}

void defiPin::addLayerMask(int mask)
{
  layerMask_[numLayers_ - 1] = mask;
}

// 5.6
void defiPin::addLayerDesignRuleWidth(int effectiveWidth)
{
  layerEffectiveWidth_[numLayers_ - 1] = effectiveWidth;
}

void defiPin::setPlacement(int typ, int x, int y, int orient)
{
  x_ = x;
  y_ = y;
  orient_ = orient;
  placeType_ = typ;
}

const char* defiPin::pinName() const
{
  return pinName_;
}

const char* defiPin::netName() const
{
  return netName_;
}

void defiPin::changePinName(const char* pinName)
{
  int len = strlen(pinName) + 1;
  if (pinNameLength_ < len) {
    if (pinName_) {
      free(pinName_);
    }
    pinName_ = (char*) malloc(len);
    pinNameLength_ = len;
  }
  strcpy(pinName_, defData->DEFCASE(pinName));
}

int defiPin::hasDirection() const
{
  return (int) (hasDirection_);
}

int defiPin::hasUse() const
{
  return (int) (hasUse_);
}

int defiPin::hasLayer() const
{
  return numLayers_ || numPolys_;  // 5.6, either layer or polygon is
}

int defiPin::hasPlacement() const
{
  return placeType_ == 0 ? 0 : 1;
}

int defiPin::isUnplaced() const
{
  return placeType_ == DEFI_COMPONENT_UNPLACED ? 1 : 0;
}

int defiPin::isPlaced() const
{
  return placeType_ == DEFI_COMPONENT_PLACED ? 1 : 0;
}

int defiPin::isCover() const
{
  return placeType_ == DEFI_COMPONENT_COVER ? 1 : 0;
}

int defiPin::isFixed() const
{
  return placeType_ == DEFI_COMPONENT_FIXED ? 1 : 0;
}

int defiPin::placementX() const
{
  return x_;
}

int defiPin::placementY() const
{
  return y_;
}

const char* defiPin::direction() const
{
  return direction_;
}

const char* defiPin::use() const
{
  return use_;
}

int defiPin::numLayer() const
{
  return numLayers_;
}

const char* defiPin::layer(int index) const
{
  return layers_[index];
}

void defiPin::bounds(int index, int* xl, int* yl, int* xh, int* yh) const
{
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

// 5.6
int defiPin::hasLayerSpacing(int index) const
{
  if (layerMinSpacing_[index] == -1) {
    return 0;
  }
  return 1;
}

// 5.6
int defiPin::hasLayerDesignRuleWidth(int index) const
{
  if (layerEffectiveWidth_[index] == -1) {
    return 0;
  }
  return 1;
}

// 5.6
int defiPin::layerSpacing(int index) const
{
  return layerMinSpacing_[index];
}

int defiPin::layerMask(int index) const
{
  return layerMask_[index];
}

// 5.6
int defiPin::layerDesignRuleWidth(int index) const
{
  return layerEffectiveWidth_[index];
}

int defiPin::orient() const
{
  return orient_;
}

const char* defiPin::orientStr() const
{
  return (defiOrientStr(orient_));
}

void defiPin::setSpecial()
{
  hasSpecial_ = 1;
}

// 5.5
void defiPin::addAntennaModel(int oxide)
{
  // For version 5.5 only OXIDE1, OXIDE2, OXIDE3, OXIDE4 ...
  // are defined within a pin
  defiPinAntennaModel* amo;
  int i;

  if (numAntennaModel_ == 0) {  // does not have antennaModel
    if (!antennaModel_) {       // only need to malloc if it is nill
      antennaModel_ = (defiPinAntennaModel**) malloc(
          sizeof(defiPinAntennaModel*) * defMaxOxides);
    }
    antennaModelAllocated_ = defMaxOxides;
    for (i = 0; i < defMaxOxides; i++) {
      antennaModel_[i] = new defiPinAntennaModel(defData);
    }
    numAntennaModel_++;
    antennaModelAllocated_ = defMaxOxides;
    amo = antennaModel_[0];
  } else {
    amo = antennaModel_[numAntennaModel_];
    numAntennaModel_++;
  }
  amo->Init();
  amo->setAntennaModel(oxide);
}

// 5.5
int defiPin::numAntennaModel() const
{
  return numAntennaModel_;
}

// 5.5
defiPinAntennaModel* defiPin::antennaModel(int index) const
{
  return antennaModel_[index];
}

void defiPin::addAPinPartialMetalArea(int value, const char* layer)
{
  if (numAPinPartialMetalArea_ == APinPartialMetalAreaAllocated_) {
    int i;
    int max;
    int lim = numAPinPartialMetalArea_;
    int* nd;
    char** nl;

    if (APinPartialMetalAreaAllocated_ == 0) {
      max = APinPartialMetalAreaAllocated_ = 2;
    } else {
      max = APinPartialMetalAreaAllocated_ *= 2;
    }
    nd = (int*) malloc(sizeof(int) * max);
    nl = (char**) malloc(sizeof(char*) * max);
    for (i = 0; i < lim; i++) {
      nd[i] = APinPartialMetalArea_[i];
      nl[i] = APinPartialMetalAreaLayer_[i];
    }
    free((char*) (APinPartialMetalArea_));
    free((char*) (APinPartialMetalAreaLayer_));
    APinPartialMetalArea_ = nd;
    APinPartialMetalAreaLayer_ = nl;
  }
  APinPartialMetalArea_[numAPinPartialMetalArea_] = value;
  if (layer) {
    APinPartialMetalAreaLayer_[numAPinPartialMetalArea_]
        = (char*) malloc(strlen(layer) + 1);
    strcpy(APinPartialMetalAreaLayer_[numAPinPartialMetalArea_],
           defData->DEFCASE(layer));
  } else {
    APinPartialMetalAreaLayer_[numAPinPartialMetalArea_] = nullptr;
  }
  numAPinPartialMetalArea_ += 1;
}

void defiPin::addAPinPartialMetalSideArea(int value, const char* layer)
{
  if (numAPinPartialMetalSideArea_ == APinPartialMetalSideAreaAllocated_) {
    int i;
    int max;
    int lim = numAPinPartialMetalSideArea_;
    int* nd;
    char** nl;

    if (APinPartialMetalSideAreaAllocated_ == 0) {
      max = APinPartialMetalSideAreaAllocated_ = 2;
    } else {
      max = APinPartialMetalSideAreaAllocated_ *= 2;
    }
    nd = (int*) malloc(sizeof(int) * max);
    nl = (char**) malloc(sizeof(char*) * max);
    for (i = 0; i < lim; i++) {
      nd[i] = APinPartialMetalSideArea_[i];
      nl[i] = APinPartialMetalSideAreaLayer_[i];
    }
    free((char*) (APinPartialMetalSideArea_));
    free((char*) (APinPartialMetalSideAreaLayer_));
    APinPartialMetalSideArea_ = nd;
    APinPartialMetalSideAreaLayer_ = nl;
  }
  APinPartialMetalSideArea_[numAPinPartialMetalSideArea_] = value;
  if (layer) {
    APinPartialMetalSideAreaLayer_[numAPinPartialMetalSideArea_]
        = (char*) malloc(strlen(layer) + 1);
    strcpy(APinPartialMetalSideAreaLayer_[numAPinPartialMetalSideArea_],
           defData->DEFCASE(layer));
  } else {
    APinPartialMetalSideAreaLayer_[numAPinPartialMetalSideArea_] = nullptr;
  }
  numAPinPartialMetalSideArea_ += 1;
}

void defiPin::addAPinGateArea(int value, const char* layer)
{
  if (numAntennaModel_ == 0) {  // haven't created any antennaModel yet
    addAntennaModel(1);
  }
  antennaModel_[numAntennaModel_ - 1]->addAPinGateArea(value, layer);
}

void defiPin::addAPinDiffArea(int value, const char* layer)
{
  if (numAPinDiffArea_ == APinDiffAreaAllocated_) {
    int i;
    int max;
    int lim = numAPinDiffArea_;
    int* nd;
    char** nl;

    if (APinDiffAreaAllocated_ == 0) {
      max = APinDiffAreaAllocated_ = 2;
    } else {
      max = APinDiffAreaAllocated_ *= 2;
    }
    nd = (int*) malloc(sizeof(int) * max);
    nl = (char**) malloc(sizeof(char*) * max);
    for (i = 0; i < lim; i++) {
      nd[i] = APinDiffArea_[i];
      nl[i] = APinDiffAreaLayer_[i];
    }
    free((char*) (APinDiffArea_));
    free((char*) (APinDiffAreaLayer_));
    APinDiffArea_ = nd;
    APinDiffAreaLayer_ = nl;
  }
  APinDiffArea_[numAPinDiffArea_] = value;
  if (layer) {
    APinDiffAreaLayer_[numAPinDiffArea_] = (char*) malloc(strlen(layer) + 1);
    strcpy(APinDiffAreaLayer_[numAPinDiffArea_], defData->DEFCASE(layer));
  } else {
    APinDiffAreaLayer_[numAPinDiffArea_] = nullptr;
  }
  numAPinDiffArea_ += 1;
}

void defiPin::addAPinMaxAreaCar(int value, const char* layer)
{
  if (numAntennaModel_ == 0) {  // haven't created any antennaModel yet
    addAntennaModel(1);
  }
  antennaModel_[numAntennaModel_ - 1]->addAPinMaxAreaCar(value, layer);
}

void defiPin::addAPinMaxSideAreaCar(int value, const char* layer)
{
  if (numAntennaModel_ == 0) {  // haven't created any antennaModel yet
    addAntennaModel(1);
  }
  antennaModel_[numAntennaModel_ - 1]->addAPinMaxSideAreaCar(value, layer);
}

void defiPin::addAPinPartialCutArea(int value, const char* layer)
{
  if (numAPinPartialCutArea_ == APinPartialCutAreaAllocated_) {
    int i;
    int max;
    int lim = numAPinPartialCutArea_;
    int* nd;
    char** nl;

    if (APinPartialCutAreaAllocated_ == 0) {
      max = APinPartialCutAreaAllocated_ = 2;
    } else {
      max = APinPartialCutAreaAllocated_ *= 2;
    }
    nd = (int*) malloc(sizeof(int) * max);
    nl = (char**) malloc(sizeof(char*) * max);
    for (i = 0; i < lim; i++) {
      nd[i] = APinPartialCutArea_[i];
      nl[i] = APinPartialCutAreaLayer_[i];
    }
    free((char*) (APinPartialCutArea_));
    free((char*) (APinPartialCutAreaLayer_));
    APinPartialCutArea_ = nd;
    APinPartialCutAreaLayer_ = nl;
  }
  APinPartialCutArea_[numAPinPartialCutArea_] = value;
  if (layer) {
    APinPartialCutAreaLayer_[numAPinPartialCutArea_]
        = (char*) malloc(strlen(layer) + 1);
    strcpy(APinPartialCutAreaLayer_[numAPinPartialCutArea_],
           defData->DEFCASE(layer));
  } else {
    APinPartialCutAreaLayer_[numAPinPartialCutArea_] = nullptr;
  }
  numAPinPartialCutArea_ += 1;
}

void defiPin::addAPinMaxCutCar(int value, const char* layer)
{
  if (numAntennaModel_ == 0) {  // haven't created any antennaModel yet
    addAntennaModel(1);
  }
  antennaModel_[numAntennaModel_ - 1]->addAPinMaxCutCar(value, layer);
}

int defiPin::hasSpecial() const
{
  return (int) hasSpecial_;
}

int defiPin::hasAPinPartialMetalArea() const
{
  return numAPinPartialMetalArea_ ? 1 : 0;
}

int defiPin::hasAPinPartialMetalSideArea() const
{
  return numAPinPartialMetalSideArea_ ? 1 : 0;
}

int defiPin::hasAPinDiffArea() const
{
  return numAPinDiffArea_ ? 1 : 0;
}

int defiPin::hasAPinPartialCutArea() const
{
  return numAPinPartialCutArea_ ? 1 : 0;
}

int defiPin::numAPinPartialMetalArea() const
{
  return numAPinPartialMetalArea_;
}

int defiPin::numAPinPartialMetalSideArea() const
{
  return numAPinPartialMetalSideArea_;
}

int defiPin::numAPinDiffArea() const
{
  return numAPinDiffArea_;
}

int defiPin::numAPinPartialCutArea() const
{
  return numAPinPartialCutArea_;
}

int defiPin::APinPartialMetalArea(int i) const
{
  return APinPartialMetalArea_[i];
}

int defiPin::hasAPinPartialMetalAreaLayer(int i) const
{
  return (APinPartialMetalAreaLayer_[i] && *(APinPartialMetalAreaLayer_[i]))
             ? 1
             : 0;
}

const char* defiPin::APinPartialMetalAreaLayer(int i) const
{
  return APinPartialMetalAreaLayer_[i];
}

int defiPin::APinPartialMetalSideArea(int i) const
{
  return APinPartialMetalSideArea_[i];
}

int defiPin::hasAPinPartialMetalSideAreaLayer(int i) const
{
  return (APinPartialMetalSideAreaLayer_[i]
          && *(APinPartialMetalSideAreaLayer_[i]))
             ? 1
             : 0;
}

const char* defiPin::APinPartialMetalSideAreaLayer(int i) const
{
  return APinPartialMetalSideAreaLayer_[i];
}

int defiPin::APinDiffArea(int i) const
{
  return APinDiffArea_[i];
}

int defiPin::hasAPinDiffAreaLayer(int i) const
{
  return (APinDiffAreaLayer_[i] && *(APinDiffAreaLayer_[i])) ? 1 : 0;
}

const char* defiPin::APinDiffAreaLayer(int i) const
{
  return APinDiffAreaLayer_[i];
}

int defiPin::APinPartialCutArea(int i) const
{
  return APinPartialCutArea_[i];
}

int defiPin::hasAPinPartialCutAreaLayer(int i) const
{
  return (APinPartialCutAreaLayer_[i] && *(APinPartialCutAreaLayer_[i])) ? 1
                                                                         : 0;
}

const char* defiPin::APinPartialCutAreaLayer(int i) const
{
  return APinPartialCutAreaLayer_[i];
}

// 5.6
void defiPin::addPolygon(const char* layerName)
{
  int *pms, *pdw, *pm;
  int i;

  if (numPolys_ == polysAllocated_) {
    char** newn;
    struct defiPoints** poly;
    polysAllocated_ = (polysAllocated_ == 0) ? 2 : polysAllocated_ * 2;
    newn = (char**) malloc(sizeof(char*) * polysAllocated_);
    poly = (struct defiPoints**) malloc(sizeof(struct defiPoints*)
                                        * polysAllocated_);
    pms = (int*) malloc(polysAllocated_ * sizeof(int));
    pdw = (int*) malloc(polysAllocated_ * sizeof(int));
    pm = (int*) malloc(polysAllocated_ * sizeof(int));

    for (i = 0; i < numPolys_; i++) {
      newn[i] = polygonNames_[i];
      poly[i] = polygons_[i];
      pms[i] = polyMinSpacing_[i];
      pdw[i] = polyEffectiveWidth_[i];
      pm[i] = polyMask_[i];
    }
    if (numPolys_ > 0) {
      free((char*) (polygons_));
      free((char*) (polygonNames_));
      free((char*) (polyMinSpacing_));
      free((char*) (polyEffectiveWidth_));
      free((char*) (polyMask_));
    }
    polygonNames_ = newn;
    polygons_ = poly;
    polyMinSpacing_ = pms;
    polyEffectiveWidth_ = pdw;
    polyMask_ = pm;
  }
  polygonNames_[numPolys_] = strdup(layerName);
  polygons_[numPolys_] = nullptr;
  polyMinSpacing_[numPolys_] = -1;
  polyEffectiveWidth_[numPolys_] = -1;
  polyMask_[numPolys_] = 0;
  numPolys_ += 1;
}

// 5.6
void defiPin::addPolygonPts(defiGeometries* geom)
{
  struct defiPoints* p;
  int x, y;
  int i;

  p = (struct defiPoints*) malloc(sizeof(struct defiPoints));
  p->numPoints = geom->numPoints();
  p->x = (int*) malloc(sizeof(int) * p->numPoints);
  p->y = (int*) malloc(sizeof(int) * p->numPoints);
  for (i = 0; i < p->numPoints; i++) {
    geom->points(i, &x, &y);
    p->x[i] = x;
    p->y[i] = y;
  }
  polygons_[numPolys_ - 1] = p;
}

// 5.6
void defiPin::addPolySpacing(int minSpacing)
{
  polyMinSpacing_[numPolys_ - 1] = minSpacing;
}

void defiPin::addPolyMask(int color)
{
  polyMask_[numPolys_ - 1] = color;
}

// 5.6
void defiPin::addPolyDesignRuleWidth(int effectiveWidth)
{
  polyEffectiveWidth_[numPolys_ - 1] = effectiveWidth;
}

// 5.6
int defiPin::numPolygons() const
{
  return numPolys_;
}

// 5.6
const char* defiPin::polygonName(int index) const
{
  if (index < 0 || index > numPolys_) {
    defiError(1, 0, "index out of bounds", defData);
    return nullptr;
  }
  return polygonNames_[index];
}

// 5.6
struct defiPoints defiPin::getPolygon(int index) const
{
  return *(polygons_[index]);
}

// 5.6
int defiPin::hasPolygonSpacing(int index) const
{
  if (polyMinSpacing_[index] == -1) {
    return 0;
  }
  return 1;
}

// 5.6
int defiPin::hasPolygonDesignRuleWidth(int index) const
{
  if (polyEffectiveWidth_[index] == -1) {
    return 0;
  }
  return 1;
}

// 5.6
int defiPin::polygonSpacing(int index) const
{
  return polyMinSpacing_[index];
}

int defiPin::polygonMask(int index) const
{
  return polyMask_[index];
}

// 5.6
int defiPin::polygonDesignRuleWidth(int index) const
{
  return polyEffectiveWidth_[index];
}

// 5.6
int defiPin::hasNetExpr() const
{
  return (int) (hasNetExpr_);
}

// 5.6
const char* defiPin::netExpr() const
{
  return netExpr_;
}

// 5.6
int defiPin::hasSupplySensitivity() const
{
  return (int) (hasSupplySens_);
}

// 5.6
const char* defiPin::supplySensitivity() const
{
  return supplySens_;
}

// 5.6
int defiPin::hasGroundSensitivity() const
{
  return (int) (hasGroundSens_);
}

// 5.6
const char* defiPin::groundSensitivity() const
{
  return groundSens_;
}

// 5.7
void defiPin::addVia(const char* viaName, int ptX, int ptY, int color)
{
  if (numVias_ >= viasAllocated_) {
    int i;
    char** newl;
    int *nx, *ny, *nm;

    viasAllocated_ = viasAllocated_ ? viasAllocated_ * 2 : 8;
    newl = (char**) malloc(viasAllocated_ * sizeof(char*));
    nx = (int*) malloc(viasAllocated_ * sizeof(int));
    ny = (int*) malloc(viasAllocated_ * sizeof(int));
    nm = (int*) malloc(viasAllocated_ * sizeof(int));

    for (i = 0; i < numVias_; i++) {
      newl[i] = viaNames_[i];
      nx[i] = viaX_[i];
      ny[i] = viaY_[i];
      nm[i] = viaMask_[i];
    }
    if (numVias_ > 0) {
      free((char*) viaNames_);
      free((char*) viaX_);
      free((char*) viaY_);
      free((char*) viaMask_);
    }
    viaNames_ = newl;
    viaX_ = nx;
    viaY_ = ny;
    viaMask_ = nm;
  }
  viaNames_[numVias_] = (char*) malloc(strlen(viaName) + 1);
  strcpy(viaNames_[numVias_], defData->DEFCASE(viaName));
  viaX_[numVias_] = ptX;
  viaY_[numVias_] = ptY;
  viaMask_[numVias_] = color;
  numVias_ += 1;
}

// 5.7
int defiPin::numVias() const
{
  return numVias_;
}

// 5.7
const char* defiPin::viaName(int index) const
{
  if (index < 0 || index > numVias_) {
    defiError(1, 0, "index out of bounds", defData);
    return nullptr;
  }
  return viaNames_[index];
}

// 5.7
int defiPin::viaPtX(int index) const
{
  return viaX_[index];
}

// 5.7
int defiPin::viaPtY(int index) const
{
  return viaY_[index];
}

int defiPin::viaTopMask(int index) const
{
  int cutMaskNum = viaMask_[index] / 10;

  if (cutMaskNum) {
    return cutMaskNum /= 10;
  }
  return 0;
}

int defiPin::viaCutMask(int index) const
{
  int cutMaskNum = viaMask_[index] / 10;

  if (cutMaskNum) {
    return cutMaskNum % 10;
  }
  return 0;
}

int defiPin::viaBottomMask(int index) const
{
  return viaMask_[index] % 10;
}
// 5.7
void defiPin::addPort()
{
  defiPinPort** pp;
  defiPinPort* pv;
  int i;

  if (numPorts_ >= portsAllocated_) {
    if (portsAllocated_ == 0) {
      pinPort_ = (defiPinPort**) malloc(sizeof(defiPinPort*) * 4);
      portsAllocated_ = 4;
    } else {
      portsAllocated_ = portsAllocated_ * 2;
      pp = (defiPinPort**) malloc(sizeof(defiPinPort*) * portsAllocated_);
      for (i = 0; i < numPorts_; i++) {
        pp[i] = pinPort_[i];
      }
      free((char*) (pinPort_));
      pinPort_ = pp;
    }
  }
  pv = new defiPinPort(defData);
  pv->Init();
  pinPort_[numPorts_] = pv;
  numPorts_ += 1;
}

// 5.7
void defiPin::addPortLayer(const char* layer)
{
  int i = numPorts_ - 1;
  pinPort_[i]->addLayer(layer);
}

// 5.7
void defiPin::addPortLayerSpacing(int minSpacing)
{
  int i = numPorts_ - 1;
  pinPort_[i]->addLayerSpacing(minSpacing);
}

void defiPin::addPortLayerMask(int color)
{
  int i = numPorts_ - 1;
  pinPort_[i]->addLayerMask(color);
}

// 5.7
void defiPin::addPortLayerDesignRuleWidth(int effectiveWidth)
{
  int i = numPorts_ - 1;
  pinPort_[i]->addLayerDesignRuleWidth(effectiveWidth);
}

// 5.7
void defiPin::addPortLayerPts(int xl, int yl, int xh, int yh)
{
  int i = numPorts_ - 1;
  pinPort_[i]->addLayerPts(xl, yl, xh, yh);
}

// 5.7
void defiPin::addPortPolygon(const char* layerName)
{
  int i = numPorts_ - 1;
  pinPort_[i]->addPolygon(layerName);
}

// 5.7
void defiPin::addPortPolySpacing(int minSpacing)
{
  int i = numPorts_ - 1;
  pinPort_[i]->addPolySpacing(minSpacing);
}

void defiPin::addPortPolyMask(int color)
{
  int i = numPorts_ - 1;
  pinPort_[i]->addPolyMask(color);
}

// 5.7
void defiPin::addPortPolyDesignRuleWidth(int effectiveWidth)
{
  int i = numPorts_ - 1;
  pinPort_[i]->addPolyDesignRuleWidth(effectiveWidth);
}

// 5.7
void defiPin::addPortPolygonPts(defiGeometries* geom)
{
  int i = numPorts_ - 1;
  pinPort_[i]->addPolygonPts(geom);
}

// 5.7
void defiPin::addPortVia(const char* via, int viaX, int viaY, int color)
{
  int i = numPorts_ - 1;
  pinPort_[i]->addVia(via, viaX, viaY, color);
}

// 5.7
void defiPin::setPortPlacement(int typ, int x, int y, int orient)
{
  int i = numPorts_ - 1;
  pinPort_[i]->setPlacement(typ, x, y, orient);
}

// 5.7
int defiPin::hasPort() const
{
  return numPorts_;
}

// 5.7
int defiPin::numPorts() const
{
  return numPorts_;
}

// 5.7
defiPinPort* defiPin::pinPort(int index) const
{
  if (index < 0 || index > numPorts_) {
    defiError(1, 0, "index out of bounds", defData);
    return nullptr;
  }
  return pinPort_[index];
}

void defiPin::print(FILE* f) const
{
  int xl, yl, xh, yh;
  int i;

  fprintf(f, "PINS '%s' on net '%s'\n", pinName(), netName());
  if (hasDirection()) {
    fprintf(f, "+ DIRECTION '%s'\n", direction());
  }
  if (hasNetExpr()) {
    fprintf(f, "+ NETEXPR '%s'\n", netExpr());
  }
  if (hasSupplySensitivity()) {
    fprintf(f, "+ SUPPLYSENSITIVITY '%s'\n", supplySensitivity());
  }
  if (hasGroundSensitivity()) {
    fprintf(f, "+ GROUNDSENSITIVITY '%s'\n", groundSensitivity());
  }
  if (hasUse()) {
    fprintf(f, "+ USE '%s'\n", use());
  }
  if (hasLayer()) {
    for (i = 0; i < numLayer(); i++) {
      bounds(i, &xl, &yl, &xh, &yh);
      fprintf(f, "+ LAYER '%s' %d %d %d %d\n", layer(i), xl, yl, xh, yh);
    }
  }
  for (i = 0; i < numPolygons(); i++) {
    fprintf(f, "+ POLYGON %s", polygonName(i));
    if (hasPolygonSpacing(i)) {
      fprintf(f, " SPACING %d", polygonSpacing(i));
    }
    if (hasPolygonDesignRuleWidth(i)) {
      fprintf(f, " DESIGNRULEWIDTH %d", polygonDesignRuleWidth(i));
    }
    fprintf(f, "\n");
  }
  for (i = 0; i < numVias(); i++) {
    fprintf(f, "+ VIA %s %d %d\n", viaName(i), viaPtX(i), viaPtY(i));
  }
  if (hasPlacement()) {
    fprintf(f,
            "  PLACED %s%s%d %d\n",
            isFixed() ? " FIXED" : "",
            isCover() ? " COVER" : "",
            placementX(),
            placementY());
  }
  if (hasSpecial()) {
    fprintf(f, "+ SPECIAL\n");
  }
}
END_DEF_PARSER_NAMESPACE
