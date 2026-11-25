// *****************************************************************************
// *****************************************************************************
// Copyright 2013-2017, Cadence Design Systems
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

#include "defiNet.hpp"

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#include "defiDebug.hpp"
#include "defiKRDefs.hpp"
#include "defiMisc.hpp"
#include "defiPath.hpp"
#include "defiUtil.hpp"
#include "defrData.hpp"

BEGIN_DEF_PARSER_NAMESPACE

namespace {

void defiError6084(int index, int numPins, defrData* defData)
{
  std::stringstream errMsg;
  errMsg << "ERROR (DEFPARS-6084): The index number " << index
         << " specified for the NET ";
  errMsg << "PIN is invalid.\n";
  errMsg << "Valid index is from 0 to " << numPins << ". Specify a valid ";
  errMsg << "index number and then try again.";
  defiError(0, 6084, errMsg.str().c_str(), defData);
}

void defiError6085(int index, int numPolys, defrData* defData)
{
  std::stringstream errMsg;
  errMsg << "ERROR (DEFPARS-6085): The index number " << index
         << " specified for the NET ";
  errMsg << "POLYGON is invalid.\n";
  errMsg << "Valid index is from 0 to " << numPolys << ". Specify a valid ";
  errMsg << "index number and then try again.";
  defiError(0, 6085, errMsg.str().c_str(), defData);
}

void defiError6086(int index, int numRects, defrData* defData)
{
  std::stringstream errMsg;
  errMsg << "ERROR (DEFPARS-6086): The index number " << index
         << " specified for the NET ";
  errMsg << "RECTANGLE is invalid.\n"
         << "Valid index is from 0 to " << numRects << ". Specify a ";
  errMsg << "valid index number and then try again.";
  defiError(0, 6086, errMsg.str().c_str(), defData);
}

}  // namespace

static constexpr int maxLimit = 65536;

////////////////////////////////////////////////////
////////////////////////////////////////////////////
//
//    defiSubnet
//
////////////////////////////////////////////////////
////////////////////////////////////////////////////

defiSubnet::defiSubnet(defrData* data) : defData(data)
{
  Init();
}

void defiSubnet::Init()
{
  name_ = nullptr;
  bumpName(16);

  instances_ = nullptr;
  pins_ = nullptr;
  musts_ = nullptr;
  synthesized_ = nullptr;
  numPins_ = 0;
  bumpPins(16);

  // WMD -- this will be removed by the next release
  paths_ = nullptr;
  numPaths_ = 0;
  pathsAllocated_ = 0;

  numWires_ = 0;
  wiresAllocated_ = 0;
  wires_ = nullptr;
  nonDefaultRule_ = nullptr;

  clear();
}

void defiSubnet::Destroy()
{
  clear();
  free(name_);
  free((char*) (instances_));
  free((char*) (pins_));
  free(musts_);
  free(synthesized_);
}

defiSubnet::~defiSubnet()
{
  Destroy();
}

void defiSubnet::setName(const char* name)
{
  int len = strlen(name) + 1;
  if (len > nameSize_) {
    bumpName(len);
  }
  strcpy(name_, defData->DEFCASE(name));
}

void defiSubnet::setNonDefault(const char* name)
{
  int len = strlen(name) + 1;
  nonDefaultRule_ = (char*) malloc(len);
  strcpy(nonDefaultRule_, defData->DEFCASE(name));
}

void defiSubnet::addMustPin(const char* instance, const char* pin, int syn)
{
  addPin(instance, pin, syn);
  musts_[numPins_ - 1] = 1;
}

void defiSubnet::addPin(const char* instance, const char* pin, int syn)
{
  int len;

  if (numPins_ == pinsAllocated_) {
    bumpPins(pinsAllocated_ * 2);
  }

  len = strlen(instance) + 1;
  instances_[numPins_] = (char*) malloc(len);
  strcpy(instances_[numPins_], defData->DEFCASE(instance));

  len = strlen(pin) + 1;
  pins_[numPins_] = (char*) malloc(len);
  strcpy(pins_[numPins_], defData->DEFCASE(pin));

  musts_[numPins_] = 0;
  synthesized_[numPins_] = syn;

  (numPins_)++;
}

// WMD -- this will be removed by the next release
void defiSubnet::setType(const char* typ)
{
  if (*typ == 'F') {
    isFixed_ = 1;
  } else if (*typ == 'C') {
    isCover_ = 1;
  } else if (*typ == 'R') {
    isRouted_ = 1;
  } else {
    // Silently do nothing with bad input.
  }
}

// WMD -- this will be removed by the next release
void defiSubnet::addPath(defiPath* p, int reset, int netOsnet, int* needCbk)
{
  int i;
  size_t incNumber;

  if (reset) {
    for (i = 0; i < numPaths_; i++) {
      delete paths_[i];
    }
    numPaths_ = 0;
  }

  if (numPaths_ >= pathsAllocated_) {
    // 6/17/2003 - don't want to allocate too large memory just in case
    // a net has many wires with only 1 or 2 paths
    if (pathsAllocated_ <= maxLimit) {
      incNumber = pathsAllocated_ * 2;
      if (incNumber > maxLimit) {
        incNumber = pathsAllocated_ + maxLimit;
      }
    } else {
      incNumber = pathsAllocated_ + maxLimit;
    }

    switch (netOsnet) {
      case 2:
        bumpPaths(pathsAllocated_ ? incNumber : 1000);
        break;
      default:
        bumpPaths(pathsAllocated_ ? incNumber : 8);
        break;
    }
  }

  paths_[numPaths_++] = new defiPath(p);

  if (numPaths_ == pathsAllocated_) {
    *needCbk = 1;  // pre-warn the parser it needs to realloc next time
  }
}

void defiSubnet::addWire(const char* type)
{
  defiWire* wire;
  if (numWires_ == wiresAllocated_) {
    defiWire** array;
    int i;
    wiresAllocated_ = wiresAllocated_ ? wiresAllocated_ * 2 : 2;
    array = (defiWire**) malloc(sizeof(defiWire*) * wiresAllocated_);
    for (i = 0; i < numWires_; i++) {
      array[i] = wires_[i];
    }
    if (wires_) {
      free((char*) (wires_));
    }
    wires_ = array;
  }
  wire = wires_[numWires_] = new defiWire(defData);
  numWires_ += 1;
  wire->Init(type, nullptr);
}

void defiSubnet::addWirePath(defiPath* p, int reset, int netOsnet, int* needCbk)
{
  if (numWires_ > 0) {
    wires_[numWires_ - 1]->addPath(p, reset, netOsnet, needCbk);
  } else {
    // Something screw up, can't be both be zero.
    defiError(
        0,
        6080,
        "ERROR (DEFPARS-6080): An internal error has occurred. The index "
        "number for the SUBNET wires array is less then or equal to "
        "0.\nContact Cadence Customer Support with this error information.",
        defData);
  }
}

const char* defiSubnet::name() const
{
  return name_;
}

int defiSubnet::hasNonDefaultRule() const
{
  return nonDefaultRule_ ? 1 : 0;
}

const char* defiSubnet::nonDefaultRule() const
{
  return nonDefaultRule_;
}

int defiSubnet::numConnections() const
{
  return numPins_;
}

const char* defiSubnet::instance(int index) const
{
  if (index >= 0 && index < numPins_) {
    return instances_[index];
  }
  return nullptr;
}

const char* defiSubnet::pin(int index) const
{
  if (index >= 0 && index < numPins_) {
    return pins_[index];
  }
  return nullptr;
}

int defiSubnet::pinIsMustJoin(int index) const
{
  if (index >= 0 && index < numPins_) {
    return (int) (musts_[index]);
  }
  return 0;
}

int defiSubnet::pinIsSynthesized(int index) const
{
  if (index >= 0 && index < numPins_) {
    return (int) (synthesized_[index]);
  }
  return 0;
}

// WMD -- this will be removed by the next release
int defiSubnet::isFixed() const
{
  return (int) (isFixed_);
}

// WMD -- this will be removed by the next release
int defiSubnet::isRouted() const
{
  return (int) (isRouted_);
}

// WMD -- this will be removed by the next release
int defiSubnet::isCover() const
{
  return (int) (isCover_);
}

void defiSubnet::bumpName(int64_t size)
{
  if (name_) {
    free(name_);
  }
  name_ = (char*) malloc(size);
  nameSize_ = size;
  name_[0] = '\0';
}

void defiSubnet::bumpPins(int64_t size)
{
  char** newInstances = (char**) malloc(sizeof(char*) * size);
  char** newPins = (char**) malloc(sizeof(char*) * size);
  char* newMusts = (char*) malloc(size);
  char* newSyn = (char*) malloc(size);
  int64_t i;

  if (instances_) {
    for (i = 0; i < pinsAllocated_; i++) {
      newInstances[i] = instances_[i];
      newPins[i] = pins_[i];
      newMusts[i] = musts_[i];
      newSyn[i] = synthesized_[i];
    }
    free((char*) (instances_));
    free((char*) (pins_));
    free(musts_);
    free(synthesized_);
  }

  instances_ = newInstances;
  pins_ = newPins;
  musts_ = newMusts;
  synthesized_ = newSyn;
  pinsAllocated_ = size;
}

void defiSubnet::clear()
{
  int i;

  // WMD -- this will be removed by the next release
  isFixed_ = 0;
  isRouted_ = 0;
  isCover_ = 0;
  name_[0] = '\0';

  for (i = 0; i < numPins_; i++) {
    free(instances_[i]);
    free(pins_[i]);
    instances_[i] = nullptr;
    pins_[i] = nullptr;
    musts_[i] = 0;
    synthesized_[i] = 0;
  }
  numPins_ = 0;

  // WMD -- this will be removed by the next release
  if (paths_) {
    for (i = 0; i < numPaths_; i++) {
      delete paths_[i];
    }
    delete[] paths_;
    paths_ = nullptr;
    numPaths_ = 0;
    pathsAllocated_ = 0;
  }

  if (nonDefaultRule_) {
    free(nonDefaultRule_);
    nonDefaultRule_ = nullptr;
  }

  if (numWires_) {
    for (i = 0; i < numWires_; i++) {
      delete wires_[i];
      wires_[i] = nullptr;
    }
    free((char*) (wires_));
    wires_ = nullptr;
    numWires_ = 0;
    wiresAllocated_ = 0;
  }
}

void defiSubnet::print(FILE* f) const
{
  int i, j;
  const defiPath* p;
  const defiWire* w;

  fprintf(f, " subnet '%s'", name_);
  fprintf(f, "\n");

  if (hasNonDefaultRule()) {
    fprintf(f, "  nondefault rule %s\n", nonDefaultRule());
  }

  if (numConnections()) {
    fprintf(f, "  Pins:\n");
    for (i = 0; i < numConnections(); i++) {
      fprintf(f,
              "   '%s' '%s'%s%s\n",
              instance(i),
              pin(i),
              pinIsMustJoin(i) ? " MUSTJOIN" : "",
              pinIsSynthesized(i) ? " SYNTHESIZED" : "");
    }
  }

  if (numWires()) {
    fprintf(f, "  Paths:\n");
    for (i = 0; i < numWires(); i++) {
      w = wire(i);
      for (j = 0; j < w->numPaths(); j++) {
        p = w->path(j);
        p->print(f);
      }
    }
  }
}

int defiSubnet::numWires() const
{
  return numWires_;
}

defiWire* defiSubnet::wire(int index)
{
  if (index >= 0 && index < numWires_) {
    return wires_[index];
  }
  return nullptr;
}

const defiWire* defiSubnet::wire(int index) const
{
  if (index >= 0 && index < numWires_) {
    return wires_[index];
  }
  return nullptr;
}

// WMD -- this will be removed after the next release
defiPath* defiSubnet::path(int index)
{
  if (index >= 0 && index < numPaths_) {
    return paths_[index];
  }
  return nullptr;
}

// WMD -- this will be removed after the next release
const defiPath* defiSubnet::path(int index) const
{
  if (index >= 0 && index < numPaths_) {
    return paths_[index];
  }
  return nullptr;
}

// WMD -- this will be removed after the next release
int defiSubnet::numPaths() const
{
  return numPaths_;
}

// WMD -- this will be removed after the next release
void defiSubnet::bumpPaths(int64_t size)
{
  int64_t i;
  defiPath** newPaths = new defiPath*[size];

  for (i = 0; i < numPaths_; i++) {
    newPaths[i] = paths_[i];
  }

  pathsAllocated_ = size;

  delete[] paths_;
  paths_ = newPaths;
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////
//
//    defiVpin
//
////////////////////////////////////////////////////
////////////////////////////////////////////////////

defiVpin::defiVpin(defrData* data) : defData(data)
{
}

void defiVpin::Init(const char* name)
{
  int len = strlen(name) + 1;
  name_ = (char*) malloc(len);
  strcpy(name_, defData->DEFCASE(name));
  orient_ = -1;
  status_ = ' ';
  layer_ = nullptr;
}

defiVpin::~defiVpin()
{
  Destroy();
}

void defiVpin::Destroy()
{
  free(name_);
  if (layer_) {
    free(layer_);
  }
}

void defiVpin::setBounds(int xl, int yl, int xh, int yh)
{
  xl_ = xl;
  yl_ = yl;
  xh_ = xh;
  yh_ = yh;
}

void defiVpin::setLayer(const char* lay)
{
  int len = strlen(lay) + 1;
  layer_ = (char*) malloc(len);
  strcpy(layer_, lay);
}

void defiVpin::setOrient(int orient)
{
  orient_ = orient;
}

void defiVpin::setLoc(int x, int y)
{
  xLoc_ = x;
  yLoc_ = y;
}

void defiVpin::setStatus(char st)
{
  status_ = st;
}

int defiVpin::xl() const
{
  return xl_;
}

int defiVpin::yl() const
{
  return yl_;
}

int defiVpin::xh() const
{
  return xh_;
}

int defiVpin::yh() const
{
  return yh_;
}

char defiVpin::status() const
{
  return status_;
}

int defiVpin::orient() const
{
  return orient_;
}

const char* defiVpin::orientStr() const
{
  return (defiOrientStr(orient_));
}

int defiVpin::xLoc() const
{
  return xLoc_;
}

int defiVpin::yLoc() const
{
  return yLoc_;
}

const char* defiVpin::name() const
{
  return name_;
}

const char* defiVpin::layer() const
{
  return layer_;
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////
//
//    defiShield
//
////////////////////////////////////////////////////
////////////////////////////////////////////////////

defiShield::defiShield(defrData* data) : defData(data)
{
}

void defiShield::Init(const char* name)
{
  int len = strlen(name) + 1;
  name_ = (char*) malloc(len);
  strcpy(name_, defData->DEFCASE(name));
  numPaths_ = 0;
  pathsAllocated_ = 0;
  paths_ = nullptr;
}

void defiShield::Destroy()
{
  clear();
}

defiShield::~defiShield()
{
  Destroy();
}

void defiShield::addPath(defiPath* p, int reset, int netOsnet, int* needCbk)
{
  int i;
  size_t incNumber;

  if (reset) {
    for (i = 0; i < numPaths_; i++) {
      delete paths_[i];
    }
    numPaths_ = 0;
  }
  if (numPaths_ >= pathsAllocated_) {
    // 6/17/2003 - don't want to allocate too large memory just in case
    // a net has many wires with only 1 or 2 paths

    if (pathsAllocated_ <= maxLimit) {
      incNumber = pathsAllocated_ * 2;
      if (incNumber > maxLimit) {
        incNumber = pathsAllocated_ + maxLimit;
      }
    } else {
      incNumber = pathsAllocated_ + maxLimit;
    }

    switch (netOsnet) {
      case 2:
        bumpPaths(pathsAllocated_ ? incNumber : 1000);
        break;
      default:
        bumpPaths(pathsAllocated_ ? incNumber : 8);
        break;
    }
  }
  paths_[numPaths_++] = new defiPath(p);
  if (numPaths_ == pathsAllocated_) {
    *needCbk = 1;  // pre-warn the parser it needs to realloc next time
  }
}

void defiShield::clear()
{
  int i;

  if (name_) {
    free(name_);
    name_ = nullptr;
  }

  if (paths_) {
    for (i = 0; i < numPaths_; i++) {
      delete paths_[i];
    }

    delete[] paths_;

    paths_ = nullptr;
    numPaths_ = 0;
    pathsAllocated_ = 0;
  }
}

void defiShield::bumpPaths(int64_t size)
{
  int64_t i;

  defiPath** newPaths = new defiPath*[size];

  for (i = 0; i < numPaths_; i++) {
    newPaths[i] = paths_[i];
  }

  pathsAllocated_ = size;

  delete[] paths_;

  paths_ = newPaths;
}

int defiShield::numPaths() const
{
  return numPaths_;
}

const char* defiShield::shieldName() const
{
  return name_;
}

defiPath* defiShield::path(int index)
{
  if (index >= 0 && index < numPaths_) {
    return paths_[index];
  }
  return nullptr;
}

const defiPath* defiShield::path(int index) const
{
  if (index >= 0 && index < numPaths_) {
    return paths_[index];
  }
  return nullptr;
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////
//
//    defiWire
//
////////////////////////////////////////////////////
////////////////////////////////////////////////////

defiWire::defiWire(defrData* data) : defData(data)
{
}

void defiWire::Init(const char* type, const char* wireShieldName)
{
  int len = strlen(type) + 1;
  type_ = (char*) malloc(len);
  strcpy(type_, defData->DEFCASE(type));
  if (wireShieldName) {
    wireShieldName_ = (char*) malloc(strlen(wireShieldName) + 1);
    strcpy(wireShieldName_, wireShieldName);
  } else {
    wireShieldName_ = nullptr;
  }
  numPaths_ = 0;
  pathsAllocated_ = 0;
  paths_ = nullptr;
}

void defiWire::Destroy()
{
  clear();
}

defiWire::~defiWire()
{
  Destroy();
}

void defiWire::addPath(defiPath* p, int reset, int netOsnet, int* needCbk)
{
  int i;
  size_t incNumber;

  if (reset) {
    for (i = 0; i < numPaths_; i++) {
      delete paths_[i];
    }
    numPaths_ = 0;
  }
  if (numPaths_ >= pathsAllocated_) {
    // 6/17/2003 - don't want to allocate too large memory just in case
    // a net has many wires with only 1 or 2 paths

    if (pathsAllocated_ <= maxLimit) {
      incNumber = pathsAllocated_ * 2;
      if (incNumber > maxLimit) {
        incNumber = pathsAllocated_ + maxLimit;
      }
    } else {
      incNumber = pathsAllocated_ + maxLimit;
    }

    switch (netOsnet) {
      case 2:
        bumpPaths(pathsAllocated_ ? incNumber : 1000);
        break;
      default:
        bumpPaths(pathsAllocated_ ? incNumber : 8);
        break;
    }
  }

  paths_[numPaths_++] = new defiPath(p);

  if (numPaths_ == pathsAllocated_) {
    *needCbk = 1;  // pre-warn the parser it needs to realloc next time
  }
}

void defiWire::clear()
{
  int i;

  if (type_) {
    free(type_);
    type_ = nullptr;
  }

  if (wireShieldName_) {
    free(wireShieldName_);
    wireShieldName_ = nullptr;
  }

  if (paths_) {
    for (i = 0; i < numPaths_; i++) {
      delete paths_[i];
    }

    delete[] paths_;
    paths_ = nullptr;
    numPaths_ = 0;
    pathsAllocated_ = 0;
  }
}

void defiWire::bumpPaths(int64_t size)
{
  int64_t i;
  defiPath** newPaths = new defiPath*[size];

  for (i = 0; i < numPaths_; i++) {
    newPaths[i] = paths_[i];
  }

  pathsAllocated_ = size;
  delete[] paths_;
  paths_ = newPaths;
}

int defiWire::numPaths() const
{
  return numPaths_;
}

const char* defiWire::wireType() const
{
  return type_;
}

const char* defiWire::wireShieldNetName() const
{
  return wireShieldName_;
}

defiPath* defiWire::path(int index)
{
  if (index >= 0 && index < numPaths_) {
    return paths_[index];
  }
  return nullptr;
}

const defiPath* defiWire::path(int index) const
{
  if (index >= 0 && index < numPaths_) {
    return paths_[index];
  }
  return nullptr;
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////
//
//    defiNet
//
////////////////////////////////////////////////////
////////////////////////////////////////////////////

defiNet::defiNet(defrData* data) : defData(data)
{
  Init();
}

void defiNet::Init()
{
  name_ = nullptr;
  instances_ = nullptr;
  numPins_ = 0;
  numProps_ = 0;
  propNames_ = nullptr;
  subnets_ = nullptr;
  source_ = nullptr;
  pattern_ = nullptr;
  style_ = 0;
  shieldNet_ = nullptr;
  original_ = nullptr;
  use_ = nullptr;
  nonDefaultRule_ = nullptr;
  numWires_ = 0;
  wiresAllocated_ = 0;
  wires_ = nullptr;

  numWidths_ = 0;
  widthsAllocated_ = 0;
  wlayers_ = nullptr;
  wdist_ = nullptr;

  numSpacing_ = 0;
  spacingAllocated_ = 0;
  slayers_ = nullptr;
  sdist_ = nullptr;
  sleft_ = nullptr;
  sright_ = nullptr;

  vpins_ = nullptr;
  numVpins_ = 0;
  vpinsAllocated_ = 0;

  shields_ = nullptr;
  numShields_ = 0;
  numNoShields_ = 0;
  shieldsAllocated_ = 0;
  numShieldNet_ = 0;
  shieldNetsAllocated_ = 0;

  bumpProps(2);
  bumpName(16);
  bumpPins(16);
  bumpSubnets(2);

  rectNames_ = nullptr;
  rectRouteStatus_ = nullptr;
  rectRouteStatusShieldNames_ = nullptr;
  rectShapeTypes_ = nullptr;
  rectMasks_ = nullptr;
  polygonNames_ = nullptr;
  polyRouteStatus_ = nullptr;
  polyShapeTypes_ = nullptr;
  polyRouteStatusShieldNames_ = nullptr;
  numPolys_ = 0;
  polysAllocated_ = 0;
  polygons_ = nullptr;
  polyMasks_ = nullptr;

  numSubnets_ = 0;
  paths_ = nullptr;
  numPaths_ = 0;

  numPts_ = 0;
  viaNames_ = nullptr;
  viaPts_ = nullptr;
  ptsAllocated_ = 0;
  viaMasks_ = nullptr;
  viaOrients_ = nullptr;
  viaRouteStatus_ = nullptr;
  viaShapeTypes_ = nullptr;
  viaRouteStatusShieldNames_ = nullptr;

  clear();
}

void defiNet::Destroy()
{
  clear();
  free(name_);
  free((char*) (instances_));
  free((char*) (pins_));
  free(musts_);
  free(synthesized_);
  free((char*) (propNames_));
  free((char*) (propValues_));
  free((char*) (propDValues_));
  free(propTypes_);
  free((char*) (subnets_));
  if (source_) {
    free(source_);
  }
  if (pattern_) {
    free(pattern_);
  }
  if (shieldNet_) {
    free(shieldNet_);
  }
  if (original_) {
    free(original_);
  }
  if (use_) {
    free(use_);
  }
  if (nonDefaultRule_) {
    free(nonDefaultRule_);
  }
  if (wlayers_) {
    free((char*) (wlayers_));
  }
  if (slayers_) {
    free((char*) (slayers_));
  }
  if (sdist_) {
    free((char*) (sdist_));
  }
  if (wdist_) {
    free((char*) (wdist_));
  }
  if (sleft_) {
    free((char*) (sleft_));
  }
  if (sright_) {
    free((char*) (sright_));
  }
}

defiNet::~defiNet()
{
  Destroy();
}

void defiNet::setName(const char* name)
{
  int len = strlen(name) + 1;
  clear();
  if (len > nameSize_) {
    bumpName(len);
  }
  strcpy(name_, defData->DEFCASE(name));
}

void defiNet::addMustPin(const char* instance, const char* pin, int syn)
{
  clear();
  addPin(instance, pin, syn);
  musts_[numPins_ - 1] = 1;
}

void defiNet::addPin(const char* instance, const char* pin, int syn)
{
  int len;

  if (numPins_ == pinsAllocated_) {
    bumpPins(pinsAllocated_ * 2);
  }

  len = strlen(instance) + 1;
  instances_[numPins_] = (char*) malloc(len);
  strcpy(instances_[numPins_], defData->DEFCASE(instance));

  len = strlen(pin) + 1;
  pins_[numPins_] = (char*) malloc(len);
  strcpy(pins_[numPins_], defData->DEFCASE(pin));

  musts_[numPins_] = 0;
  synthesized_[numPins_] = syn;

  (numPins_)++;
}

void defiNet::setWeight(int w)
{
  hasWeight_ = 1;
  weight_ = w;
}

void defiNet::addProp(const char* name, const char* value, const char type)
{
  int len;

  if (numProps_ == propsAllocated_) {
    bumpProps(propsAllocated_ * 2);
  }

  len = strlen(name) + 1;
  propNames_[numProps_] = (char*) malloc(len);
  strcpy(propNames_[numProps_], defData->DEFCASE(name));

  len = strlen(value) + 1;
  propValues_[numProps_] = (char*) malloc(len);
  strcpy(propValues_[numProps_], defData->DEFCASE(value));

  propDValues_[numProps_] = 0;
  propTypes_[numProps_] = type;

  (numProps_)++;
}

void defiNet::addNumProp(const char* name,
                         const double d,
                         const char* value,
                         const char type)
{
  int len;

  if (numProps_ == propsAllocated_) {
    bumpProps(propsAllocated_ * 2);
  }

  len = strlen(name) + 1;
  propNames_[numProps_] = (char*) malloc(len);
  strcpy(propNames_[numProps_], defData->DEFCASE(name));

  len = strlen(value) + 1;
  propValues_[numProps_] = (char*) malloc(len);
  strcpy(propValues_[numProps_], defData->DEFCASE(value));

  propDValues_[numProps_] = d;
  propTypes_[numProps_] = type;

  (numProps_)++;
}

void defiNet::addSubnet(defiSubnet* subnet)
{
  if (numSubnets_ >= subnetsAllocated_) {
    bumpSubnets(subnetsAllocated_ * 2);
  }

  subnets_[numSubnets_++] = subnet;
}

// WMD -- will be removed after the next release
void defiNet::setType(const char* typ)
{
  if (*typ == 'F') {
    isFixed_ = 1;
  } else if (*typ == 'C') {
    isCover_ = 1;
  } else if (*typ == 'R') {
    isRouted_ = 1;
  } else {
    // Silently do nothing with bad input.
  }
}

void defiNet::addWire(const char* type, const char* wireShieldName)
{
  defiWire* wire;
  if (numWires_ == wiresAllocated_) {
    defiWire** array;
    int i;
    wiresAllocated_ = wiresAllocated_ ? wiresAllocated_ * 2 : 2;
    array = (defiWire**) malloc(sizeof(defiWire*) * wiresAllocated_);
    for (i = 0; i < numWires_; i++) {
      array[i] = wires_[i];
    }
    if (wires_) {
      free((char*) (wires_));
    }
    wires_ = array;
  }
  wire = wires_[numWires_] = new defiWire(defData);
  numWires_ += 1;
  wire->Init(type, wireShieldName);
}

void defiNet::addWirePath(defiPath* p, int reset, int netOsnet, int* needCbk)
{
  if (numWires_ > 0) {
    wires_[numWires_ - 1]->addPath(p, reset, netOsnet, needCbk);
  } else {
    // Something screw up, can't be both be zero.
    defiError(
        0,
        6081,
        "ERROR (DEFPARS-6081): An internal error has occurred. The index "
        "number for the NET PATH wires array is less then or equal to "
        "0.\nContact Cadence Customer Support with this error information.",
        defData);
  }
}

void defiNet::addShield(const char* name)
{
  defiShield* shield;
  if (numShields_ == shieldsAllocated_) {
    defiShield** array;
    int i;
    shieldsAllocated_ = shieldsAllocated_ ? shieldsAllocated_ * 2 : 2;
    array = (defiShield**) malloc(sizeof(defiShield*) * shieldsAllocated_);
    for (i = 0; i < numShields_; i++) {
      array[i] = shields_[i];
    }
    if (shields_) {
      free((char*) (shields_));
    }
    shields_ = array;
  }
  shield = shields_[numShields_] = new defiShield(defData);
  numShields_ += 1;
  shield->Init(name);
}

void defiNet::addShieldPath(defiPath* p, int reset, int netOsnet, int* needCbk)
{
  // Since shield and noshield share the list shields_, the
  // only way to tell whether the list is currently contained
  // data for shields_ or noshields_ is from the variables
  // numShields_ and numNoShields_.
  // Since shield and noshield are mutual exclusive, only one
  // numShields_ or numNoShields will be non-zero
  // in this method.  Whichever is non-zero will be the current
  // working list
  if (numShields_ > 0) {
    shields_[numShields_ - 1]->addPath(p, reset, netOsnet, needCbk);
  } else if (numNoShields_ > 0) {
    shields_[numNoShields_ - 1]->addPath(p, reset, netOsnet, needCbk);
  } else {
    // Something screw up, can't be both be zero.
    defiError(
        0,
        6082,
        "ERROR (DEFPARS-6082): An internal error has occurred. The index "
        "number for the NET SHIELDPATH wires array is less then or equal to "
        "0.\nContact Cadence Customer Support with this error information.",
        defData);
  }
}

void defiNet::addNoShield(const char* name)
{
  defiShield* shield;
  if (numNoShields_ == shieldsAllocated_) {
    defiShield** array;
    int i;
    shieldsAllocated_ = shieldsAllocated_ ? shieldsAllocated_ * 2 : 2;
    array = (defiShield**) malloc(sizeof(defiShield*) * shieldsAllocated_);
    for (i = 0; i < numNoShields_; i++) {
      array[i] = shields_[i];
    }
    if (shields_) {
      free((char*) (shields_));
    }
    shields_ = array;
  }
  shield = shields_[numNoShields_] = new defiShield(defData);
  numNoShields_ += 1;
  shield->Init(name);
}

void defiNet::addShieldNet(const char* name)
{
  int len;

  if (numShieldNet_ == shieldNetsAllocated_) {
    if (shieldNetsAllocated_ == 0) {
      bumpShieldNets(2);
    } else {
      bumpShieldNets(shieldNetsAllocated_ * 2);
    }
  }

  len = strlen(name) + 1;
  shieldNet_[numShieldNet_] = (char*) malloc(len);
  strcpy(shieldNet_[numShieldNet_], defData->DEFCASE(name));
  (numShieldNet_)++;
}

void defiNet::changeNetName(const char* name)
{
  int len = strlen(name) + 1;
  if (len > nameSize_) {
    bumpName(len);
  }
  strcpy(name_, defData->DEFCASE(name));
}

void defiNet::changeInstance(const char* instance, int index)
{
  int len;

  if ((index < 0) || (index > numPins_)) {
    std::stringstream errMsg;
    errMsg << "ERROR (DEFPARS-6083): The index number " << index;
    errMsg << "specified for the NET INSTANCE is invalid.\n";
    errMsg << "Valid index is from 0 to " << numPins_ << ".";
    errMsg << "Specify a valid index number and then try again.";
    defiError(0, 6083, errMsg.str().c_str(), defData);
  }

  len = strlen(instance) + 1;
  if (instances_[index]) {
    free(instances_[index]);
  }
  instances_[index] = (char*) malloc(len);
  strcpy(instances_[index], defData->DEFCASE(instance));
}

void defiNet::changePin(const char* pin, int index)
{
  int len;

  if ((index < 0) || (index > numPins_)) {
    defiError6084(index, numPins_, defData);
  }

  len = strlen(pin) + 1;
  if (pins_[index]) {
    free(pins_[index]);
  }
  pins_[index] = (char*) malloc(len);
  strcpy(pins_[index], defData->DEFCASE(pin));
}

const char* defiNet::name() const
{
  return name_;
}

int defiNet::weight() const
{
  return weight_;
}

int defiNet::numProps() const
{
  return numProps_;
}

int defiNet::hasProps() const
{
  return numProps_ ? 1 : 0;
}

int defiNet::hasWeight() const
{
  return (int) (hasWeight_);
}

const char* defiNet::propName(int index) const
{
  if (index >= 0 && index < numProps_) {
    return propNames_[index];
  }
  return nullptr;
}

const char* defiNet::propValue(int index) const
{
  if (index >= 0 && index < numProps_) {
    return propValues_[index];
  }
  return nullptr;
}

double defiNet::propNumber(int index) const
{
  if (index >= 0 && index < numProps_) {
    return propDValues_[index];
  }
  return 0;
}

char defiNet::propType(int index) const
{
  if (index >= 0 && index < numProps_) {
    return propTypes_[index];
  }
  return 0;
}

int defiNet::propIsNumber(int index) const
{
  if (index >= 0 && index < numProps_) {
    return propDValues_[index] ? 1 : 0;
  }
  return 0;
}

int defiNet::propIsString(int index) const
{
  if (index >= 0 && index < numProps_) {
    return propDValues_[index] ? 0 : 1;
  }
  return 0;
}

int defiNet::numConnections() const
{
  return numPins_;
}

int defiNet::numShieldNets() const
{
  return numShieldNet_;
}

const char* defiNet::instance(int index) const
{
  if (index >= 0 && index < numPins_) {
    return instances_[index];
  }
  return nullptr;
}

const char* defiNet::pin(int index) const
{
  if (index >= 0 && index < numPins_) {
    return pins_[index];
  }
  return nullptr;
}

int defiNet::pinIsMustJoin(int index) const
{
  if (index >= 0 && index < numPins_) {
    return (int) (musts_[index]);
  }
  return 0;
}

int defiNet::pinIsSynthesized(int index) const
{
  if (index >= 0 && index < numPins_) {
    return (int) (synthesized_[index]);
  }
  return 0;
}

int defiNet::hasSubnets() const
{
  return numSubnets_ ? 1 : 0;
}

int defiNet::numSubnets() const
{
  return numSubnets_;
}

defiSubnet* defiNet::subnet(int index)
{
  if (index >= 0 && index < numSubnets_) {
    return subnets_[index];
  }
  return nullptr;
}

const defiSubnet* defiNet::subnet(int index) const
{
  if (index >= 0 && index < numSubnets_) {
    return subnets_[index];
  }
  return nullptr;
}

int defiNet::isFixed() const
{
  return (int) (isFixed_);
}

int defiNet::isRouted() const
{
  return (int) (isRouted_);
}

int defiNet::isCover() const
{
  return (int) (isCover_);
}

// this method will only call if the callback defrSNetWireCbk is set
// which will callback every wire.  Therefore, only one wire should be here
void defiNet::freeWire()
{
  int i;

  if (numWires_) {
    for (i = 0; i < numWires_; i++) {
      wires_[i]->Destroy();
      delete wires_[i];
      wires_[i] = nullptr;
    }
    free((char*) (wires_));
    wires_ = nullptr;
    numWires_ = 0;
    wiresAllocated_ = 0;
  }

  clearRectPoly();
  clearVia();
}

void defiNet::freeShield()
{
  int i;

  if (numShields_) {
    for (i = 0; i < numShields_; i++) {
      shields_[i]->Destroy();
      free((char*) (shields_[i]));
      shields_[i] = nullptr;
    }
    numShields_ = 0;
    shieldsAllocated_ = 0;
  }
}

void defiNet::print(FILE* f) const
{
  int i, j, x, y, newLayer;
  int numX, numY, stepX, stepY;
  const defiPath* p;
  const defiSubnet* s;
  const defiVpin* vp;
  const defiWire* w;
  int path;

  fprintf(f, "Net '%s'", name_);
  fprintf(f, "\n");

  if (hasWeight()) {
    fprintf(f, "  weight=%d\n", weight());
  }

  if (hasFixedbump()) {
    fprintf(f, "  fixedbump\n");
  }

  if (hasFrequency()) {
    fprintf(f, "  frequency=%f\n", frequency());
  }

  if (hasCap()) {
    fprintf(f, "  cap=%f\n", cap());
  }

  if (hasSource()) {
    fprintf(f, "  source='%s'\n", source());
  }

  if (hasPattern()) {
    fprintf(f, "  pattern='%s'\n", pattern());
  }

  if (hasOriginal()) {
    fprintf(f, "  original='%s'\n", original());
  }

  if (hasUse()) {
    fprintf(f, "  use='%s'\n", use());
  }

  if (hasNonDefaultRule()) {
    fprintf(f, "  nonDefaultRule='%s'\n", nonDefaultRule());
  }

  if (hasXTalk()) {
    fprintf(f, "  xtalk=%d\n", XTalk());
  }

  if (hasStyle()) {
    fprintf(f, "  style='%d'\n", style());
  }

  if (hasProps()) {
    fprintf(f, " Props:\n");
    for (i = 0; i < numProps(); i++) {
      fprintf(f, "  '%s' '%s'\n", propName(i), propValue(i));
    }
  }

  if (numConnections()) {
    fprintf(f, " Pins:\n");
    for (i = 0; i < numConnections(); i++) {
      fprintf(f,
              "  '%s' '%s'%s%s\n",
              instance(i),
              pin(i),
              pinIsMustJoin(i) ? " MUSTJOIN" : "",
              pinIsSynthesized(i) ? " SYNTHESIZED" : "");
    }
  }

  for (i = 0; i < numVpins_; i++) {
    vp = vpin(i);
    fprintf(f,
            "  VPIN %s status '%c' layer %s %d,%d orient %s bounds %d,%d to "
            "%d,%d\n",
            vp->name(),
            vp->status(),
            vp->layer() ? vp->layer() : "",
            vp->xLoc(),
            vp->yLoc(),
            vp->orientStr(),
            vp->xl(),
            vp->yl(),
            vp->xh(),
            vp->yh());
  }

  for (i = 0; i < numWires_; i++) {
    newLayer = 0;
    w = wire(i);
    fprintf(f, "+ %s ", w->wireType());
    for (j = 0; j < w->numPaths(); j++) {
      p = w->path(j);
      p->initTraverse();
      while ((path = p->next()) != DEFIPATH_DONE) {
        switch (path) {
          case DEFIPATH_LAYER:
            if (newLayer == 0) {
              fprintf(f, "%s ", p->getLayer());
              newLayer = 1;
            } else {
              fprintf(f, "NEW %s ", p->getLayer());
            }
            break;
          case DEFIPATH_VIA:
            fprintf(f, "%s\n", p->getVia());
            break;
          case DEFIPATH_VIAROTATION:
            fprintf(f, "%d\n", p->getViaRotation());
            break;
          case DEFIPATH_VIADATA:
            p->getViaData(&numX, &numY, &stepX, &stepY);
            fprintf(f, "%d %d %d %d\n", numX, numY, stepX, stepY);
            break;
          case DEFIPATH_WIDTH:
            fprintf(f, "%d\n", p->getWidth());
            break;
          case DEFIPATH_POINT:
            p->getPoint(&x, &y);
            fprintf(f, "( %d %d )\n", x, y);
            break;
          case DEFIPATH_TAPER:
            fprintf(f, "TAPER\n");
            break;
        }
      }
    }
  }

  if (hasSubnets()) {
    fprintf(f, " Subnets:\n");
    for (i = 0; i < numSubnets(); i++) {
      s = subnet(i);
      s->print(f);
    }
  }
}

void defiNet::bumpName(int64_t size)
{
  if (name_) {
    free(name_);
  }
  name_ = (char*) malloc(size);
  nameSize_ = size;
  name_[0] = '\0';
}

void defiNet::bumpPins(int64_t size)
{
  char** newInstances = (char**) malloc(sizeof(char*) * size);
  char** newPins = (char**) malloc(sizeof(char*) * size);
  char* newMusts = (char*) malloc(size);
  char* newSyn = (char*) malloc(size);
  int64_t i;

  if (instances_) {
    for (i = 0; i < pinsAllocated_; i++) {
      newInstances[i] = instances_[i];
      newPins[i] = pins_[i];
      newMusts[i] = musts_[i];
      newSyn[i] = synthesized_[i];
    }
    free((char*) (instances_));
    free((char*) (pins_));
    free(musts_);
    free(synthesized_);
  }

  instances_ = newInstances;
  pins_ = newPins;
  musts_ = newMusts;
  synthesized_ = newSyn;
  pinsAllocated_ = size;
}

void defiNet::bumpProps(int64_t size)
{
  char** newNames = (char**) malloc(sizeof(char*) * size);
  char** newValues = (char**) malloc(sizeof(char*) * size);
  double* newDValues = (double*) malloc(sizeof(double) * size);
  char* newTypes = (char*) malloc(sizeof(char) * size);
  int64_t i;

  if (propNames_) {
    for (i = 0; i < numProps_; i++) {
      newNames[i] = propNames_[i];
      newValues[i] = propValues_[i];
      newDValues[i] = propDValues_[i];
      newTypes[i] = propTypes_[i];
    }
    free((char*) (propNames_));
    free((char*) (propValues_));
    free((char*) (propDValues_));
    free(propTypes_);
  }

  propNames_ = newNames;
  propValues_ = newValues;
  propDValues_ = newDValues;
  propTypes_ = newTypes;
  propsAllocated_ = size;
}

void defiNet::bumpSubnets(int64_t size)
{
  defiSubnet** newSubnets = (defiSubnet**) malloc(sizeof(defiSubnet*) * size);
  int i;
  if (subnets_) {
    for (i = 0; i < numSubnets_; i++) {
      newSubnets[i] = subnets_[i];
    }
    free((char*) (subnets_));
  }

  subnets_ = newSubnets;
  subnetsAllocated_ = size;
}

void defiNet::clear()
{
  int i;

  // WMD -- this will be removed by the next release
  isFixed_ = 0;
  isRouted_ = 0;
  isCover_ = 0;

  hasWeight_ = 0;
  hasCap_ = 0;
  hasFrequency_ = 0;
  hasVoltage_ = 0;
  xTalk_ = -1;

  if (vpins_) {
    for (i = 0; i < numVpins_; i++) {
      delete vpins_[i];
    }
    free((char*) vpins_);
    vpins_ = nullptr;
    numVpins_ = 0;
    vpinsAllocated_ = 0;
  }

  for (i = 0; i < numProps_; i++) {
    free(propNames_[i]);
    free(propValues_[i]);
    propNames_[i] = nullptr;
    propValues_[i] = nullptr;
    propDValues_[i] = 0;
  }
  numProps_ = 0;

  for (i = 0; i < numPins_; i++) {
    free(instances_[i]);
    free(pins_[i]);
    instances_[i] = nullptr;
    pins_[i] = nullptr;
    musts_[i] = 0;
    synthesized_[i] = 0;
  }
  numPins_ = 0;

  for (i = 0; i < numSubnets_; i++) {
    delete subnets_[i];
    subnets_[i] = nullptr;
  }
  numSubnets_ = 0;

  if (name_) {
    name_[0] = '\0';
  }

  // WMD -- this will be removed by the next release
  if (paths_) {
    for (i = 0; i < numPaths_; i++) {
      delete paths_[i];
    }

    delete[] paths_;
    paths_ = nullptr;
    numPaths_ = 0;
    pathsAllocated_ = 0;
  }

  // 5.4.1
  fixedbump_ = 0;

  if (source_) {
    free(source_);
    source_ = nullptr;
  }
  if (pattern_) {
    free(pattern_);
    pattern_ = nullptr;
  }
  if (original_) {
    free(original_);
    original_ = nullptr;
  }
  if (use_) {
    free(use_);
    use_ = nullptr;
  }
  if (nonDefaultRule_) {
    free(nonDefaultRule_);
    nonDefaultRule_ = nullptr;
  }
  style_ = 0;

  if (numWires_) {
    for (i = 0; i < numWires_; i++) {
      delete wires_[i];
      wires_[i] = nullptr;
    }
    free((char*) (wires_));
    wires_ = nullptr;
    numWires_ = 0;
    wiresAllocated_ = 0;
  }

  if (numShields_) {
    for (i = 0; i < numShields_; i++) {
      delete shields_[i];
      shields_[i] = nullptr;
    }
    numShields_ = 0;
    shieldsAllocated_ = 0;
  }

  if (numNoShields_) {
    for (i = 0; i < numNoShields_; i++) {
      delete shields_[i];
      shields_[i] = nullptr;
    }
    numNoShields_ = 0;
    shieldsAllocated_ = 0;
  }
  if (shields_) {
    free((char*) (shields_));
  }

  shields_ = nullptr;

  if (numWidths_) {
    for (i = 0; i < numWidths_; i++) {
      free(wlayers_[i]);
    }
    numWidths_ = 0;
  }

  if (numSpacing_) {
    for (i = 0; i < numSpacing_; i++) {
      free(slayers_[i]);
    }
    numSpacing_ = 0;
  }

  if (numShieldNet_) {
    for (i = 0; i < numShieldNet_; i++) {
      free(shieldNet_[i]);
    }
    numShieldNet_ = 0;
  }

  if (polygonNames_) {
    struct defiPoints* p;
    for (i = 0; i < numPolys_; i++) {
      if (polygonNames_[i]) {
        free(polygonNames_[i]);
      }
      if (polyRouteStatus_[i]) {
        free(polyRouteStatus_[i]);
      }
      if (polyShapeTypes_[i]) {
        free(polyShapeTypes_[i]);
      }
      if (polyRouteStatusShieldNames_[i]) {
        free(polyRouteStatusShieldNames_[i]);
      }
      p = polygons_[i];
      free((char*) (p->x));
      free((char*) (p->y));
      free((char*) (polygons_[i]));
    }
    free((char*) (polygonNames_));
    free((char*) (polygons_));
    free((char*) (polyMasks_));
    free((char*) (polyRouteStatus_));
    free((char*) (polyShapeTypes_));
    free((char*) (polyRouteStatusShieldNames_));
    polygonNames_ = nullptr;
    polygons_ = nullptr;
    polyMasks_ = nullptr;
    polyRouteStatus_ = nullptr;
    polyShapeTypes_ = nullptr;
    polyRouteStatusShieldNames_ = nullptr;
  }
  numPolys_ = 0;
  polysAllocated_ = 0;

  if (rectNames_) {
    for (i = 0; i < numRects_; i++) {
      if (rectNames_[i]) {
        free(rectNames_[i]);
      }
      if (rectRouteStatus_[i]) {
        free(rectRouteStatus_[i]);
      }
      if (rectRouteStatusShieldNames_[i]) {
        free(rectRouteStatusShieldNames_[i]);
      }
      if (rectShapeTypes_[i]) {
        free(rectShapeTypes_[i]);
      }
    }
    free((char*) (rectNames_));
    free((char*) (xl_));
    free((char*) (yl_));
    free((char*) (xh_));
    free((char*) (yh_));
    free((char*) (rectMasks_));
    free((char*) (rectRouteStatus_));
    free((char*) (rectRouteStatusShieldNames_));
    free((char*) (rectShapeTypes_));
  }
  rectNames_ = nullptr;
  rectRouteStatus_ = nullptr;
  rectShapeTypes_ = nullptr;
  rectRouteStatusShieldNames_ = nullptr;
  numRects_ = 0;
  rectsAllocated_ = 0;
  xl_ = nullptr;
  yl_ = nullptr;
  xh_ = nullptr;
  yh_ = nullptr;
  rectMasks_ = nullptr;

  if (viaNames_) {
    struct defiPoints* p;

    for (i = 0; i < numPts_; i++) {
      p = viaPts_[i];
      free((char*) (p->x));
      free((char*) (p->y));
      free((char*) (viaPts_[i]));
      if (viaNames_[i]) {
        free(viaNames_[i]);
      }
      if (viaRouteStatus_[i]) {
        free(viaRouteStatus_[i]);
      }
      if (viaShapeTypes_[i]) {
        free(viaShapeTypes_[i]);
      }
      if (viaRouteStatusShieldNames_[i]) {
        free(viaRouteStatusShieldNames_[i]);
      }
    }
    free((char*) (viaNames_));
    free((char*) (viaPts_));
    free((char*) (viaMasks_));
    free((char*) (viaOrients_));
    free((char*) (viaRouteStatus_));
    free((char*) (viaShapeTypes_));
    free((char*) (viaRouteStatusShieldNames_));
    viaNames_ = nullptr;
    viaPts_ = nullptr;
    viaRouteStatus_ = nullptr;
    viaShapeTypes_ = nullptr;
    viaRouteStatusShieldNames_ = nullptr;
  }
  numPts_ = 0;
  ptsAllocated_ = 0;
  viaOrients_ = nullptr;
  viaMasks_ = nullptr;
}

void defiNet::clearRectPolyNPath()
{
  int i;

  if (paths_) {
    for (i = 0; i < numPaths_; i++) {
      delete paths_[i];
    }
    numPaths_ = 0;
  }

  clearRectPoly();
}

void defiNet::clearRectPoly()
{
  int i;

  if (polygonNames_) {
    struct defiPoints* p;
    for (i = 0; i < numPolys_; i++) {
      if (polygonNames_[i]) {
        free(polygonNames_[i]);
      }
      if (polyRouteStatus_[i]) {
        free(polyRouteStatus_[i]);
      }
      if (polyShapeTypes_[i]) {
        free(polyShapeTypes_[i]);
      }
      if (polyRouteStatusShieldNames_[i]) {
        free(polyRouteStatusShieldNames_[i]);
      }
      p = polygons_[i];
      free((char*) (p->x));
      free((char*) (p->y));
      free((char*) (polygons_[i]));
    }
    free((char*) (polyMasks_));
    free((char*) (polygonNames_));
    free((char*) (polygons_));
    free((char*) (polyRouteStatus_));
    free((char*) (polyShapeTypes_));
    free((char*) (polyRouteStatusShieldNames_));
  }
  numPolys_ = 0;
  polysAllocated_ = 0;
  polyMasks_ = nullptr;
  polygonNames_ = nullptr;
  polyRouteStatus_ = nullptr;
  polyShapeTypes_ = nullptr;
  polyRouteStatusShieldNames_ = nullptr;
  polygons_ = nullptr;

  if (rectNames_) {
    for (i = 0; i < numRects_; i++) {
      if (rectNames_[i]) {
        free(rectNames_[i]);
      }
      if (rectRouteStatus_[i]) {
        free(rectRouteStatus_[i]);
      }
      if (rectShapeTypes_[i]) {
        free(rectShapeTypes_[i]);
      }
      if (rectRouteStatusShieldNames_[i]) {
        free(rectRouteStatusShieldNames_[i]);
      }
    }
    free((char*) (rectMasks_));
    free((char*) (rectNames_));
    free((char*) (xl_));
    free((char*) (yl_));
    free((char*) (xh_));
    free((char*) (yh_));
    free((char*) (rectShapeTypes_));
    free((char*) (rectRouteStatus_));
    free((char*) (rectRouteStatusShieldNames_));
  }
  rectNames_ = nullptr;
  rectsAllocated_ = 0;
  xl_ = nullptr;
  yl_ = nullptr;
  xh_ = nullptr;
  yh_ = nullptr;
  numRects_ = 0;
  rectMasks_ = nullptr;
  rectRouteStatus_ = nullptr;
  rectShapeTypes_ = nullptr;
  rectRouteStatusShieldNames_ = nullptr;
}

int defiNet::hasSource() const
{
  return source_ ? 1 : 0;
}

int defiNet::hasFixedbump() const
{
  return fixedbump_ ? 1 : 0;
}

int defiNet::hasFrequency() const
{
  return (int) (hasFrequency_);
}

int defiNet::hasPattern() const
{
  return pattern_ ? 1 : 0;
}

int defiNet::hasOriginal() const
{
  return original_ ? 1 : 0;
}

int defiNet::hasCap() const
{
  return (int) (hasCap_);
}

int defiNet::hasUse() const
{
  return use_ ? 1 : 0;
}

int defiNet::hasStyle() const
{
  return style_ ? 1 : 0;
}

int defiNet::hasXTalk() const
{
  return (xTalk_ != -1) ? 1 : 0;
}

int defiNet::hasNonDefaultRule() const
{
  return nonDefaultRule_ ? 1 : 0;
}

void defiNet::setSource(const char* typ)
{
  int len;
  if (source_) {
    free(source_);
  }
  len = strlen(typ) + 1;
  source_ = (char*) malloc(len);
  strcpy(source_, defData->DEFCASE(typ));
}

void defiNet::setFixedbump()
{
  fixedbump_ = 1;
}

void defiNet::setFrequency(double frequency)
{
  frequency_ = frequency;
  hasFrequency_ = 1;
}

void defiNet::setOriginal(const char* typ)
{
  int len;
  if (original_) {
    free(original_);
  }
  len = strlen(typ) + 1;
  original_ = (char*) malloc(len);
  strcpy(original_, defData->DEFCASE(typ));
}

void defiNet::setPattern(const char* typ)
{
  int len;
  if (pattern_) {
    free(pattern_);
  }
  len = strlen(typ) + 1;
  pattern_ = (char*) malloc(len);
  strcpy(pattern_, defData->DEFCASE(typ));
}

void defiNet::setCap(double w)
{
  cap_ = w;
  hasCap_ = 1;
}

void defiNet::setUse(const char* typ)
{
  int len;
  if (use_) {
    free(use_);
  }
  len = strlen(typ) + 1;
  use_ = (char*) malloc(len);
  strcpy(use_, defData->DEFCASE(typ));
}

void defiNet::setStyle(int style)
{
  style_ = style;
}

void defiNet::setNonDefaultRule(const char* typ)
{
  int len;
  if (nonDefaultRule_) {
    free(nonDefaultRule_);
  }
  len = strlen(typ) + 1;
  nonDefaultRule_ = (char*) malloc(len);
  strcpy(nonDefaultRule_, defData->DEFCASE(typ));
}

const char* defiNet::source() const
{
  return source_;
}

const char* defiNet::original() const
{
  return original_;
}

const char* defiNet::pattern() const
{
  return pattern_;
}

double defiNet::cap() const
{
  return (hasCap_ ? cap_ : 0.0);
}

double defiNet::frequency() const
{
  return (hasFrequency_ ? frequency_ : 0.0);
}

const char* defiNet::use() const
{
  return use_;
}

int defiNet::style() const
{
  return style_;
}

const char* defiNet::shieldNet(int index) const
{
  return shieldNet_[index];
}

const char* defiNet::nonDefaultRule() const
{
  return nonDefaultRule_;
}

// WMD -- this will be removed by the next release
void defiNet::bumpPaths(int64_t size)
{
  int64_t i;

  defiPath** newPaths = new defiPath*[size];

  for (i = 0; i < numPaths_; i++) {
    newPaths[i] = paths_[i];
  }

  delete[] paths_;
  pathsAllocated_ = size;
  paths_ = newPaths;
}

// WMD -- this will be removed by the next release
int defiNet::numPaths() const
{
  return numPaths_;
}

// WMD -- this will be removed by the next release
defiPath* defiNet::path(int index)
{
  if (index >= 0 && index < numPaths_) {
    return paths_[index];
  }
  return nullptr;
}

const defiPath* defiNet::path(int index) const
{
  if (index >= 0 && index < numPaths_) {
    return paths_[index];
  }
  return nullptr;
}

int defiNet::numWires() const
{
  return numWires_;
}

defiWire* defiNet::wire(int index)
{
  if (index >= 0 && index < numWires_) {
    return wires_[index];
  }
  return nullptr;
}

const defiWire* defiNet::wire(int index) const
{
  if (index >= 0 && index < numWires_) {
    return wires_[index];
  }
  return nullptr;
}

void defiNet::bumpShieldNets(int64_t size)
{
  char** newShieldNets = (char**) malloc(sizeof(char*) * size);
  int64_t i;

  if (shieldNet_) {
    for (i = 0; i < shieldNetsAllocated_; i++) {
      newShieldNets[i] = shieldNet_[i];
    }
    free((char*) (shieldNet_));
  }

  shieldNet_ = newShieldNets;
  shieldNetsAllocated_ = size;
}

int defiNet::numShields() const
{
  return numShields_;
}

defiShield* defiNet::shield(int index)
{
  if (index >= 0 && index < numShields_) {
    return shields_[index];
  }
  return nullptr;
}

const defiShield* defiNet::shield(int index) const
{
  if (index >= 0 && index < numShields_) {
    return shields_[index];
  }
  return nullptr;
}

int defiNet::numNoShields() const
{
  return numNoShields_;
}

defiShield* defiNet::noShield(int index)
{
  if (index >= 0 && index < numNoShields_) {
    return shields_[index];
  }
  return nullptr;
}

const defiShield* defiNet::noShield(int index) const
{
  if (index >= 0 && index < numNoShields_) {
    return shields_[index];
  }
  return nullptr;
}

int defiNet::hasVoltage() const
{
  return (int) (hasVoltage_);
}

double defiNet::voltage() const
{
  return voltage_;
}

int defiNet::numWidthRules() const
{
  return numWidths_;
}

int defiNet::numSpacingRules() const
{
  return numSpacing_;
}

int defiNet::hasWidthRules() const
{
  return numWidths_;
}

int defiNet::hasSpacingRules() const
{
  return numSpacing_;
}

void defiNet::setXTalk(int i)
{
  xTalk_ = i;
}

int defiNet::XTalk() const
{
  return xTalk_;
}

void defiNet::addVpin(const char* name)
{
  defiVpin* vp;
  if (numVpins_ == vpinsAllocated_) {
    defiVpin** array;
    int i;
    vpinsAllocated_ = vpinsAllocated_ ? vpinsAllocated_ * 2 : 2;
    array = (defiVpin**) malloc(sizeof(defiVpin*) * vpinsAllocated_);
    for (i = 0; i < numVpins_; i++) {
      array[i] = vpins_[i];
    }
    if (vpins_) {
      free((char*) (vpins_));
    }
    vpins_ = array;
  }
  vp = vpins_[numVpins_] = new defiVpin(defData);
  numVpins_ += 1;
  vp->Init(name);
}

void defiNet::addVpinLayer(const char* name)
{
  defiVpin* vp = vpins_[numVpins_ - 1];
  vp->setLayer(name);
}

void defiNet::addVpinLoc(const char* status, int x, int y, int orient)
{
  defiVpin* vp = vpins_[numVpins_ - 1];
  vp->setStatus(*status);
  vp->setLoc(x, y);
  vp->setOrient(orient);
}

void defiNet::addVpinBounds(int xl, int yl, int xh, int yh)
{
  defiVpin* vp = vpins_[numVpins_ - 1];
  vp->setBounds(xl, yl, xh, yh);
}

int defiNet::numVpins() const
{
  return numVpins_;
}

defiVpin* defiNet::vpin(int index)
{
  if (index < 0 || index >= numVpins_) {
    return nullptr;
  }
  return vpins_[index];
}

const defiVpin* defiNet::vpin(int index) const
{
  if (index < 0 || index >= numVpins_) {
    return nullptr;
  }
  return vpins_[index];
}

void defiNet::spacingRule(int index,
                          char** layer,
                          double* dist,
                          double* left,
                          double* right) const
{
  if (index >= 0 && index < numSpacing_) {
    if (layer) {
      *layer = slayers_[index];
    }
    if (dist) {
      *dist = sdist_[index];
    }
    if (left) {
      *left = sleft_[index];
    }
    if (right) {
      *right = sright_[index];
    }
  }
}

void defiNet::widthRule(int index, char** layer, double* dist) const
{
  if (index >= 0 && index < numWidths_) {
    if (layer) {
      *layer = wlayers_[index];
    }
    if (dist) {
      *dist = wdist_[index];
    }
  }
}

void defiNet::setVoltage(double v)
{
  voltage_ = v;
  hasVoltage_ = 1;
}

void defiNet::setWidth(const char* layer, double d)
{
  int len = strlen(layer) + 1;
  char* l = (char*) malloc(len);
  strcpy(l, defData->DEFCASE(layer));

  if (numWidths_ >= widthsAllocated_) {
    int i;
    char** nl;
    double* nd;
    widthsAllocated_ = widthsAllocated_ ? widthsAllocated_ * 2 : 4;
    nl = (char**) malloc(sizeof(char*) * widthsAllocated_);
    nd = (double*) malloc(sizeof(double) * widthsAllocated_);
    for (i = 0; i < numWidths_; i++) {
      nl[i] = wlayers_[i];
      nd[i] = wdist_[i];
    }
    free((char*) (wlayers_));
    free((char*) (wdist_));
    wlayers_ = nl;
    wdist_ = nd;
  }

  wlayers_[numWidths_] = l;
  wdist_[numWidths_] = d;
  (numWidths_)++;
}

void defiNet::setSpacing(const char* layer, double d)
{
  const int len = strlen(layer) + 1;
  char* l = (char*) malloc(len);
  strcpy(l, defData->DEFCASE(layer));

  if (numSpacing_ >= spacingAllocated_) {
    spacingAllocated_ = spacingAllocated_ ? spacingAllocated_ * 2 : 4;
    char** layers = (char**) malloc(sizeof(char*) * spacingAllocated_);
    double* dist = (double*) malloc(sizeof(double) * spacingAllocated_);
    double* left = (double*) malloc(sizeof(double) * spacingAllocated_);
    double* right = (double*) malloc(sizeof(double) * spacingAllocated_);
    for (int i = 0; i < numSpacing_; i++) {
      layers[i] = slayers_[i];
      dist[i] = sdist_[i];
      left[i] = sleft_[i];
      right[i] = sright_[i];
    }
    free((char*) (slayers_));
    free((char*) (sdist_));
    free((char*) (sleft_));
    free((char*) (sright_));
    slayers_ = layers;
    sdist_ = dist;
    sleft_ = left;
    sright_ = right;
  }

  slayers_[numSpacing_] = l;
  sdist_[numSpacing_] = d;
  sleft_[numSpacing_] = d;
  sright_[numSpacing_] = d;
  (numSpacing_)++;
}

void defiNet::setRange(double left, double right)
{
  // This is always called right after setSpacing.
  sleft_[numSpacing_ - 1] = left;
  sright_[numSpacing_ - 1] = right;
}

// 5.6
void defiNet::addPolygon(const char* layerName,
                         defiGeometries* geom,
                         int* needCbk,
                         int colorMask,
                         const char* routeStatus,
                         const char* shapeType,
                         const char* routeStatusShieldName)
{
  struct defiPoints* p;
  int x, y;
  int i;

  // This method will only call by specialnet, need to change if net also
  // calls it.
  *needCbk = 0;
  if (numPolys_ == polysAllocated_) {
    char** newn;
    char** newRS;
    char** newST;
    char** newRSN;
    int* maskn;
    struct defiPoints** poly;
    polysAllocated_ = (polysAllocated_ == 0) ? 1000 : polysAllocated_ * 2;
    newn = (char**) malloc(sizeof(char*) * polysAllocated_);
    newRS = (char**) malloc(sizeof(char*) * polysAllocated_);
    newST = (char**) malloc(sizeof(char*) * polysAllocated_);
    newRSN = (char**) malloc(sizeof(char*) * polysAllocated_);
    maskn = (int*) malloc(sizeof(int) * polysAllocated_);
    poly = (struct defiPoints**) malloc(sizeof(struct defiPoints*)
                                        * polysAllocated_);
    for (i = 0; i < numPolys_; i++) {
      newn[i] = polygonNames_[i];
      poly[i] = polygons_[i];
      maskn[i] = polyMasks_[i];
      newRS[i] = polyRouteStatus_[i];
      newST[i] = polyShapeTypes_[i];
      newRSN[i] = polyRouteStatusShieldNames_[i];
    }
    if (polygons_) {
      free((char*) (polygons_));
    }
    if (polygonNames_) {
      free((char*) (polygonNames_));
    }
    if (polyMasks_) {
      free((char*) (polyMasks_));
    }
    if (polyRouteStatus_) {
      free((char*) (polyRouteStatus_));
    }
    if (polyShapeTypes_) {
      free((char*) (polyShapeTypes_));
    }
    if (polyRouteStatusShieldNames_) {
      free((char*) (polyRouteStatusShieldNames_));
    }
    polygonNames_ = newn;
    polygons_ = poly;
    polyMasks_ = maskn;
    polyShapeTypes_ = newST;
    polyRouteStatus_ = newRS;
    polyRouteStatusShieldNames_ = newRSN;
  }
  polygonNames_[numPolys_] = strdup(layerName);
  polyRouteStatus_[numPolys_] = strdup(routeStatus);
  polyShapeTypes_[numPolys_] = strdup(shapeType);
  polyRouteStatusShieldNames_[numPolys_] = strdup(routeStatusShieldName);
  p = (struct defiPoints*) malloc(sizeof(struct defiPoints));
  p->numPoints = geom->numPoints();
  p->x = (int*) malloc(sizeof(int) * p->numPoints);
  p->y = (int*) malloc(sizeof(int) * p->numPoints);
  for (i = 0; i < p->numPoints; i++) {
    geom->points(i, &x, &y);
    p->x[i] = x;
    p->y[i] = y;
  }
  polyMasks_[numPolys_] = colorMask;
  polygons_[numPolys_] = p;
  numPolys_ += 1;
  if (numPolys_ == 1000) {  // Want to invoke the partial callback if set
    *needCbk = 1;
  }
}

// 5.6
int defiNet::numPolygons() const
{
  return numPolys_;
}

// 5.6
const char* defiNet::polygonName(int index) const
{
  if (index < 0 || index > numPolys_) {
    defiError6085(index, numPolys_, defData);
    return nullptr;
  }
  return polygonNames_[index];
}

const char* defiNet::polyRouteStatus(int index) const
{
  if (index < 0 || index > numPolys_) {
    defiError6085(index, numPolys_, defData);
    return nullptr;
  }
  return polyRouteStatus_[index];
}

const char* defiNet::polyRouteStatusShieldName(int index) const
{
  if (index < 0 || index > numPolys_) {
    defiError6085(index, numPolys_, defData);
    return nullptr;
  }
  return polyRouteStatusShieldNames_[index];
}

const char* defiNet::polyShapeType(int index) const
{
  if (index < 0 || index > numPolys_) {
    defiError6085(index, numPolys_, defData);
    return nullptr;
  }
  return polyShapeTypes_[index];
}

int defiNet::polyMask(int index) const
{
  if (index < 0 || index > numPolys_) {
    defiError6085(index, numPolys_, defData);
    return 0;
  }
  return polyMasks_[index];
}

// 5.6
struct defiPoints defiNet::getPolygon(int index) const
{
  return *(polygons_[index]);
}

// 5.6
void defiNet::addRect(const char* layerName,
                      int xl,
                      int yl,
                      int xh,
                      int yh,
                      int* needCbk,
                      int colorMask,
                      const char* routeStatus,
                      const char* shapeType,
                      const char* routeStatusName)
{
  // This method will only call by specialnet, need to change if net also
  // calls it.
  *needCbk = 0;
  if (numRects_ == rectsAllocated_) {
    int i;
    int max;
    char** newn;
    int* newxl;
    int* newyl;
    int* newxh;
    int* newyh;
    int* newMask;
    char** newRS;
    char** newST;
    char** newRSN;

    max = rectsAllocated_ = (rectsAllocated_ == 0) ? 1000 : rectsAllocated_ * 2;
    newn = (char**) malloc(sizeof(char*) * max);
    newRS = (char**) malloc(sizeof(char*) * max);
    newST = (char**) malloc(sizeof(char*) * max);
    newRSN = (char**) malloc(sizeof(char*) * max);
    newxl = (int*) malloc(sizeof(int) * max);
    newyl = (int*) malloc(sizeof(int) * max);
    newxh = (int*) malloc(sizeof(int) * max);
    newyh = (int*) malloc(sizeof(int) * max);
    newMask = (int*) malloc(sizeof(int) * max);
    for (i = 0; i < numRects_; i++) {
      newn[i] = rectNames_[i];
      newxl[i] = xl_[i];
      newyl[i] = yl_[i];
      newxh[i] = xh_[i];
      newyh[i] = yh_[i];
      newMask[i] = rectMasks_[i];
      newRS[i] = rectRouteStatus_[i];
      newST[i] = rectShapeTypes_[i];
      newRSN[i] = rectRouteStatusShieldNames_[i];
    }
    if (rectNames_) {
      free((char*) (rectNames_));
    }
    if (rectRouteStatus_) {
      free((char*) (rectRouteStatus_));
    }
    if (rectShapeTypes_) {
      free((char*) (rectShapeTypes_));
    }
    if (rectRouteStatusShieldNames_) {
      free((char*) (rectRouteStatusShieldNames_));
    }
    if (xl_) {
      free((char*) (xl_));
      free((char*) (yl_));
      free((char*) (xh_));
      free((char*) (yh_));
      free((char*) (rectMasks_));
    }
    rectNames_ = newn;
    xl_ = newxl;
    yl_ = newyl;
    xh_ = newxh;
    yh_ = newyh;
    rectMasks_ = newMask;
    rectRouteStatus_ = newRS;
    rectShapeTypes_ = newST;
    rectRouteStatusShieldNames_ = newRSN;
  }
  rectNames_[numRects_] = strdup(layerName);
  xl_[numRects_] = xl;
  yl_[numRects_] = yl;
  xh_[numRects_] = xh;
  yh_[numRects_] = yh;
  rectMasks_[numRects_] = colorMask;
  rectRouteStatus_[numRects_] = strdup(routeStatus);
  rectShapeTypes_[numRects_] = strdup(shapeType);
  rectRouteStatusShieldNames_[numRects_] = strdup(routeStatusName);
  numRects_ += 1;
  if (numRects_ == 1000) {  // Want to invoke the partial callback if set
    *needCbk = 1;
  }
}

// 5.6
int defiNet::numRectangles() const
{
  return numRects_;
}

// 5.6
const char* defiNet::rectName(int index) const
{
  if (index < 0 || index > numRects_) {
    return nullptr;
  }
  return rectNames_[index];
}

const char* defiNet::rectRouteStatus(int index) const
{
  if (index < 0 || index > numRects_) {
    defiError6086(index, numRects_, defData);
    return nullptr;
  }
  return rectRouteStatus_[index];
}

const char* defiNet::rectRouteStatusShieldName(int index) const
{
  if (index < 0 || index > numRects_) {
    defiError6086(index, numRects_, defData);
    return nullptr;
  }
  return rectRouteStatusShieldNames_[index];
}

const char* defiNet::rectShapeType(int index) const
{
  if (index < 0 || index > numRects_) {
    defiError6086(index, numRects_, defData);
    return nullptr;
  }
  return rectShapeTypes_[index];
}

// 5.6
int defiNet::xl(int index) const
{
  if (index < 0 || index >= numRects_) {
    defiError6086(index, numRects_, defData);
    return 0;
  }
  return xl_[index];
}

// 5.6
int defiNet::yl(int index) const
{
  if (index < 0 || index >= numRects_) {
    defiError6086(index, numRects_, defData);
    return 0;
  }
  return yl_[index];
}

// 5.6
int defiNet::xh(int index) const
{
  if (index < 0 || index >= numRects_) {
    defiError6086(index, numRects_, defData);
    return 0;
  }
  return xh_[index];
}

// 5.6
int defiNet::yh(int index) const
{
  if (index < 0 || index >= numRects_) {
    defiError6086(index, numRects_, defData);
    return 0;
  }
  return yh_[index];
}

int defiNet::rectMask(int index) const
{
  if (index < 0 || index >= numRects_) {
    defiError6086(index, numRects_, defData);
    return 0;
  }
  return rectMasks_[index];
}

void defiNet::addPts(const char* viaName,
                     int o,
                     defiGeometries* geom,
                     int* needCbk,
                     int colorMask,
                     const char* routeStatus,
                     const char* shapeType,
                     const char* routeStatusShieldName)
{
  struct defiPoints* p;
  int x, y;
  int i;

  *needCbk = 0;
  if (numPts_ == ptsAllocated_) {
    struct defiPoints** pts;
    char** newn;
    char** newRS;
    char** newST;
    char** newRSN;
    int* orientn;
    int* maskn;

    ptsAllocated_ = (ptsAllocated_ == 0) ? 1000 : ptsAllocated_ * 2;
    newn = (char**) malloc(sizeof(char*) * ptsAllocated_);
    newRS = (char**) malloc(sizeof(char*) * ptsAllocated_);
    newST = (char**) malloc(sizeof(char*) * ptsAllocated_);
    newRSN = (char**) malloc(sizeof(char*) * ptsAllocated_);
    orientn = (int*) malloc(sizeof(int) * ptsAllocated_);
    pts = (struct defiPoints**) malloc(sizeof(struct defiPoints*)
                                       * ptsAllocated_);
    maskn = (int*) malloc(sizeof(int) * ptsAllocated_);
    for (i = 0; i < numPts_; i++) {
      pts[i] = viaPts_[i];
      newn[i] = viaNames_[i];
      newRS[i] = viaRouteStatus_[i];
      newST[i] = viaShapeTypes_[i];
      newRSN[i] = viaRouteStatusShieldNames_[i];
      orientn[i] = viaOrients_[i];
      maskn[i] = viaMasks_[i];
    }
    if (viaPts_) {
      free((char*) (viaPts_));
    }
    if (viaNames_) {
      free((char*) (viaNames_));
    }
    if (viaOrients_) {
      free((char*) (viaOrients_));
    }
    if (viaMasks_) {
      free((char*) (viaMasks_));
    }
    if (viaRouteStatus_) {
      free((char*) (viaRouteStatus_));
    }
    if (viaShapeTypes_) {
      free((char*) (viaShapeTypes_));
    }
    if (viaRouteStatusShieldNames_) {
      free((char*) (viaRouteStatusShieldNames_));
    }

    viaPts_ = pts;
    viaNames_ = newn;
    viaOrients_ = orientn;
    viaMasks_ = maskn;
    viaShapeTypes_ = newST;
    viaRouteStatus_ = newRS;
    viaRouteStatusShieldNames_ = newRSN;
  }
  viaNames_[numPts_] = strdup(viaName);
  viaShapeTypes_[numPts_] = strdup(shapeType);
  viaRouteStatus_[numPts_] = strdup(routeStatus);
  viaRouteStatusShieldNames_[numPts_] = strdup(routeStatusShieldName);
  viaOrients_[numPts_] = o;
  viaMasks_[numPts_] = colorMask;
  p = (struct defiPoints*) malloc(sizeof(struct defiPoints));
  p->numPoints = geom->numPoints();
  p->x = (int*) malloc(sizeof(int) * p->numPoints);
  p->y = (int*) malloc(sizeof(int) * p->numPoints);
  for (i = 0; i < p->numPoints; i++) {
    geom->points(i, &x, &y);
    p->x[i] = x;
    p->y[i] = y;
  }
  viaPts_[numPts_] = p;
  numPts_ += 1;
  if (numPts_ == 1000) {  // Want to invoke the partial callback if set
    *needCbk = 1;
  }
}

int defiNet::numViaSpecs() const
{
  return numPts_;
}

const char* defiNet::viaName(int index) const
{
  if (index < 0 || index > numPts_) {
    defiError6085(index, numPolys_, defData);
    return nullptr;
  }
  return viaNames_[index];
}

const char* defiNet::viaRouteStatus(int index) const
{
  if (index < 0 || index > numPts_) {
    defiError6085(index, numPolys_, defData);
    return nullptr;
  }
  return viaRouteStatus_[index];
}

const char* defiNet::viaRouteStatusShieldName(int index) const
{
  if (index < 0 || index > numPts_) {
    defiError6085(index, numPolys_, defData);
    return nullptr;
  }
  return viaRouteStatusShieldNames_[index];
}

const char* defiNet::viaShapeType(int index) const
{
  if (index < 0 || index > numPts_) {
    defiError6085(index, numPolys_, defData);
    return nullptr;
  }
  return viaShapeTypes_[index];
}

int defiNet::viaOrient(int index) const
{
  if (index < 0 || index > numPts_) {
    defiError6085(index, numPolys_, defData);
    return 0;
  }
  return viaOrients_[index];
}

const char* defiNet::viaOrientStr(int index) const
{
  if (index < 0 || index > numPts_) {
    defiError6085(index, numPolys_, defData);
    return nullptr;
  }
  return (defiOrientStr(viaOrients_[index]));
}

int defiNet::topMaskNum(int index) const
{
  if (index < 0 || index > numPts_) {
    defiError6085(index, numPolys_, defData);
    return 0;
  }

  return viaMasks_[index] / 100;
}

int defiNet::cutMaskNum(int index) const
{
  if (index < 0 || index > numPts_) {
    defiError6085(index, numPolys_, defData);
    return 0;
  }

  return viaMasks_[index] / 10 % 10;
}

int defiNet::bottomMaskNum(int index) const
{
  if (index < 0 || index > numPts_) {
    defiError6085(index, numPolys_, defData);
    return 0;
  }

  return viaMasks_[index] % 10;
}

struct defiPoints defiNet::getViaPts(int index) const
{
  return *(viaPts_[index]);
}

void defiNet::clearVia()
{
  if (viaNames_) {
    struct defiPoints* p;
    for (int i = 0; i < numPts_; i++) {
      if (viaNames_[i]) {
        free(viaNames_[i]);
      }
      if (viaRouteStatus_[i]) {
        free(viaRouteStatus_[i]);
      }
      if (viaShapeTypes_[i]) {
        free(viaShapeTypes_[i]);
      }
      if (viaRouteStatusShieldNames_[i]) {
        free(viaRouteStatusShieldNames_[i]);
      }
      p = viaPts_[i];
      free((char*) (p->x));
      free((char*) (p->y));
      free((char*) (viaPts_[i]));
    }
    if (viaMasks_) {
      free((char*) (viaMasks_));
    }
    if (viaOrients_) {
      free((char*) (viaOrients_));
    }
    if (viaNames_) {
      free((char*) (viaNames_));
    }
    if (viaRouteStatus_) {
      free((char*) (viaRouteStatus_));
    }
    if (viaShapeTypes_) {
      free((char*) (viaShapeTypes_));
    }
    if (viaRouteStatusShieldNames_) {
      free((char*) (viaRouteStatusShieldNames_));
    }
    if (viaPts_) {
      free((char*) (viaPts_));
    }
  }

  viaMasks_ = nullptr;
  viaOrients_ = nullptr;
  numPts_ = 0;
  ptsAllocated_ = 0;
  viaPts_ = nullptr;
  viaRouteStatus_ = nullptr;
  viaShapeTypes_ = nullptr;
  viaRouteStatusShieldNames_ = nullptr;
  viaNames_ = nullptr;
}

END_DEF_PARSER_NAMESPACE
