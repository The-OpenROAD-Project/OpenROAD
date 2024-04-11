///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2023, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "placerObjects.h"
#include <odb/db.h>
#include "utl/Logger.h"
#include "util.h"
#include <iostream>
#include <cmath>
#include <memory>
#include <numeric>
#include <chrono>
#include <cuda.h>
#include <cuda_runtime.h>
// basic vectors
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include <thrust/device_malloc.h>
#include <thrust/device_free.h>
#include <thrust/sequence.h>
#include <thrust/reduce.h>
// memory related
#include <thrust/copy.h>
#include <thrust/fill.h>
// algorithm related
#include <thrust/transform.h>
#include <thrust/replace.h>
#include <thrust/functional.h>
#include <thrust/for_each.h>
#include <thrust/execution_policy.h>

namespace gpl2 {

using namespace std;
using utl::GPL2;

///////////////////////////////////////////////////////////////
// Instance
Instance::Instance()
  : instId_(-1),
    inst_(nullptr),
    cx_(0.0),
    cy_(0.0),
    dx_(0.0),
    dy_(0.0),
    dDx_(0.0),
    dDy_(0.0),
    densityScale_(0.0),
    type_(InstanceType::FILLER),
    isFixed_(false)
{
}


Instance::Instance(odb::dbInst* inst,
    int padLeft,
    int padRight,
    int siteHeight,
    int rowLimit,
    utl::Logger* logger) : Instance()
{
  // get the instance id  
  instId_ = odb::dbIntProperty::find(inst, "instId")->getValue();  
  inst_ = inst;
  // get the bounding box
  odb::dbBox* bbox = inst->getBBox();
  int lx = 0.0;
  int ly = 0.0;
  inst->getLocation(lx, ly);
  int ux = lx + floor(bbox->getDX() / 2) * 2;
  int uy = ly + floor(bbox->getDY() / 2) * 2;

  

  isFixed_ = isFixedOdbInst(inst);  
  if (isPlaceInstance()) {
    lx -= padLeft;
    ux += padRight;
  }

  dx_ = ux - lx;
  dy_ = uy - ly;
  cx_ = lx + dx_ / 2;
  cy_ = ly + dy_ / 2;

  dDx_ = dx_;
  dDy_ = dy_;

  // Masters more than row_limit rows tall are treated as macros
  if (inst->getMaster()->getType().isBlock()) {
    setMacro();
  } else if (bbox->getDY() > rowLimit * siteHeight) {
    setMacro();
    std::string msg = std::string("Master ") + std::string(inst->getMaster()->getName());
    msg += std::string(" is not marked as a BLOCK in LEF but is more than ");
    msg += std::to_string(rowLimit) + " rows tall.  It will be treated as a macro.";
    logger->report(msg);
  } else {
    setStdInstance();
  }
}


// for dummy instances or filler instance
Instance::Instance(int cx, int cy, int width, int height, bool isDummy) 
  : Instance()
{
  instId_ = -1; // This is a dummy instance
  cx_ = cx;
  cy_ = cy;

  // round to even number
  dx_ = width / 2 * 2;
  dy_ = height / 2 * 2;

  dDx_ = dx_;
  dDy_ = dy_;

  if (isDummy == true) {
    setDummy();
    isFixed_ = true; // dummy instance is always fixed
  } else {
    setFiller();
    isFixed_ = false; // filler instance is not fixed
  }
}

Instance::~Instance()
{
  inst_ = nullptr;
  instId_ = -1;
  pins_.clear();
}


void Instance::snapOutward(const odb::Point& origin, 
                           int step_x, 
                           int step_y)
{
  int lx = cx_ - dx_ / 2;
  int ly = cy_ - dy_ / 2;
  int ux = cx_ + dx_ / 2;
  int uy = cy_ + dy_ / 2;

  lx = snapDown(lx, origin.x(), step_x);
  ly = snapDown(ly, origin.y(), step_y);
  ux = snapUp(ux, origin.x(), step_x);
  uy = snapUp(uy, origin.y(), step_y);

  dx_ = ux - lx;
  dy_ = uy - ly;
  cx_ = lx + dx_ / 2;
  cy_ = ly + dy_ / 2;

  dDx_ = dx_;
  dDy_ = dy_;
}


void Instance::dbSetPlaced()
{
  if (inst_ != nullptr) {
    inst_->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }
}


void Instance::dbSetPlacementStatus(odb::dbPlacementStatus ps)
{
  if (inst_ != nullptr) {
    inst_->setPlacementStatus(ps);
  }
}

void Instance::dbSetLocation()
{ 
  if (inst_ != nullptr) {
    int lx = cx_ - dx_ / 2 - haloWidth_;
    int ly = cy_ - dy_ / 2 - haloWidth_;
    inst_->setLocation(lx, ly);
  }
}

void Instance::setCenterLocation(int cx, int cy)
{
  cx_ = cx;
  cy_ = cy;

  for (auto& pin : pins_) {
    pin->updateLocation(cx_, cy_);
  }
}


void Instance::setDensitySize(int dDx, int dDy, float densityScale)
{
  dDx_ = dDx / 2 * 2;
  dDy_ = dDy / 2 * 2;
  densityScale_ = densityScale;
}


////////////////////////////////////////////////////////
// Pin
Pin::Pin()
  : pinId_(-1),
    pin_(nullptr),
    inst_(nullptr),
    net_(nullptr),
    cx_(0),
    cy_(0),
    offsetCx_(0),
    offsetCy_(0),
    iTermField_(false),
    bTermField_(false),
    minPinXField_(false),
    minPinYField_(false),
    maxPinXField_(false),
    maxPinYField_(false)
{
}


Pin::Pin(int pinId) : Pin()
{
  pinId_ = pinId;
  iTermField_ = true;
}

Pin::Pin(odb::dbITerm* iTerm, utl::Logger* logger) : Pin()
{
  iTermField_ = true;
  pinId_ = odb::dbIntProperty::find(iTerm, "pinId")->getValue();  
  pin_ = (void*) iTerm;
  updateCoordi(iTerm, logger);
}


Pin::Pin(odb::dbBTerm* bTerm, utl::Logger* logger) : Pin()
{
  bTermField_ = true;
  pinId_ = odb::dbIntProperty::find(bTerm, "pinId")->getValue();  
  pin_ = (void*) bTerm;
  updateCoordi(bTerm, logger);
}

std::string Pin::name() const
{
  if (pinId_ == -1) {
    return "DUMMY";
  }
  if (isITerm()) {
    return dbITerm()->getInst()->getName() + '/'
           + dbITerm()->getMTerm()->getName();
  } else {
    return dbBTerm()->getName();
  }
}

odb::dbITerm* Pin::dbITerm() const
{
  return (isITerm()) ? (odb::dbITerm*) pin_ : nullptr;
}

odb::dbBTerm* Pin::dbBTerm() const
{
  return (isBTerm()) ? (odb::dbBTerm*) pin_ : nullptr;
}

void Pin::updateCoordi(odb::dbITerm* iTerm, utl::Logger* logger)
{
  int offsetLx = INT_MAX;
  int offsetLy = INT_MAX;
  int offsetUx = INT_MIN;
  int offsetUy = INT_MIN;

  for (odb::dbMPin* mPin : iTerm->getMTerm()->getMPins()) {
    for (odb::dbBox* box : mPin->getGeometry()) {
      offsetLx = std::min(box->xMin(), offsetLx);
      offsetLy = std::min(box->yMin(), offsetLy);
      offsetUx = std::max(box->xMax(), offsetUx);
      offsetUy = std::max(box->yMax(), offsetUy);
    }
  }

  int lx = iTerm->getInst()->getBBox()->xMin();
  int ly = iTerm->getInst()->getBBox()->yMin();

  int instCenterX = iTerm->getInst()->getMaster()->getWidth() / 2;
  int instCenterY = iTerm->getInst()->getMaster()->getHeight() / 2;

  // Pin SHAPE is NOT FOUND;
  // (may happen on OpenDB bug case)
  if (offsetLx == INT_MAX || offsetLy == INT_MAX || offsetUx == INT_MIN
      || offsetUy == INT_MIN) {
    // offset is center of instances
    offsetCx_ = 0;
    offsetCy_ = 0;
  } else { // usual case
    // offset is Pin BBoxs' center, so
    // subtract the Origin coordinates (e.g. instCenterX, instCenterY)
    //
    // Transform coordinates
    // from (origin: 0,0)
    // to (origin: instCenterX, instCenterY)
    //
    offsetCx_ = (offsetLx + offsetUx) / 2 - instCenterX;
    offsetCy_ = (offsetLy + offsetUy) / 2 - instCenterY;
  }

  cx_ = lx + instCenterX + offsetCx_;
  cy_ = ly + instCenterY + offsetCy_;
}


//
// for BTerm, offset* will hold bbox info.
//
void Pin::updateCoordi(odb::dbBTerm* bTerm, utl::Logger* logger)
{
  int lx = INT_MAX;
  int ly = INT_MAX;
  int ux = INT_MIN;
  int uy = INT_MIN;

  for (odb::dbBPin* bPin : bTerm->getBPins()) {
    odb::Rect bbox = bPin->getBBox();
    lx = std::min(bbox.xMin(), lx);
    ly = std::min(bbox.yMin(), ly);
    ux = std::max(bbox.xMax(), ux);
    uy = std::max(bbox.yMax(), uy);
  }

  if (lx == INT_MAX || ly == INT_MAX || ux == INT_MIN || uy == INT_MIN) {
    std::string msg
        = string(bTerm->getConstName()) + " toplevel port is not placed!\n";
    msg += "Replace will regard " + string(bTerm->getConstName());
    msg += " is placed in (0, 0)";
    logger->report(msg);
  }

  // Just center
  offsetCx_ = 0;
  offsetCy_ = 0;

  cx_ = (lx + ux) / 2;
  cy_ = (ly + uy) / 2;
}

Pin::~Pin()
{
  pinId_ = -1;
  pin_ = nullptr;
  inst_ = nullptr;
  net_ = nullptr;
}

void Pin::updateLocation(int instCx, int instCy)
{
  cx_ = instCx + offsetCx_;
  cy_ = instCy + offsetCy_;
}

void Pin::updateLocation(Instance* inst)
{
  cx_ = inst->cx() + offsetCx_;
  cy_ = inst->cy() + offsetCy_;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// Net
Net::Net() 
  : netId_(-1), 
    net_(nullptr),
    lx_(0), 
    ly_(0), 
    ux_(0), 
    uy_(0),
    isDontCare_(false),
    weight_(1.0),
    virtualWeight_(0.0)
{
}

Net::Net(int netId) : Net()
{
  netId_ = netId;
  weight_ = 0.0;
  virtualWeight_ = 1.0;
}

Net::Net(odb::dbNet* net) : Net()
{
  netId_ = odb::dbIntProperty::find(net, "netId")->getValue();  
  net_ = net;
 
  weight_ = 1.0;
  virtualWeight_ = 0.0;

  lx_ = ly_ = INT_MAX;
  ux_ = uy_ = INT_MIN;
  for (odb::dbITerm* iTerm : net_->getITerms()) {
    odb::dbBox* box = iTerm->getInst()->getBBox();
    lx_ = std::min(box->xMin(), lx_);
    ly_ = std::min(box->yMin(), ly_);
    ux_ = std::max(box->xMax(), ux_);
    uy_ = std::max(box->yMax(), uy_);
  }
 
  for (odb::dbBTerm* bTerm : net_->getBTerms()) {
    for (odb::dbBPin* bPin : bTerm->getBPins()) {
      odb::Rect bbox = bPin->getBBox();
      lx_ = std::min(bbox.xMin(), lx_);
      ly_ = std::min(bbox.yMin(), ly_);
      ux_ = std::max(bbox.xMax(), ux_);
      uy_ = std::max(bbox.yMax(), uy_);
    }
  }
}


Net::~Net()
{
  net_ = nullptr;
  netId_ = -1;
  pins_.clear();
}


void Net::updateBBox()
{
  lx_ = ly_ = INT_MAX;
  ux_ = uy_ = INT_MIN;
  for (Pin* pin : pins_) {
    lx_ = std::min(pin->cx(), lx_);
    ly_ = std::min(pin->cy(), ly_);
    ux_ = std::max(pin->cx(), ux_);
    uy_ = std::max(pin->cy(), uy_);
  }
}

////////////////////////////////////////////////
// Bin
Bin::Bin()
  : x_(0),
    y_(0),
    lx_(0),
    ly_(0),
    ux_(0),
    uy_(0),
    nonPlaceArea_(0),
    instPlacedArea_(0),
    fillerArea_(0),
    density_(0),
    targetDensity_(0),
    electroPhi_(0),
    electroForceX_(0),
    electroForceY_(0)
{
}


Bin::Bin(int x, int y, int lx, int ly, int ux, int uy, float targetDensity) : Bin()
{
  x_ = x;
  y_ = y;
  lx_ = lx;
  ly_ = ly;
  ux_ = ux;
  uy_ = uy;
  targetDensity_ = targetDensity;
}


////////////////////////////////////////////////////////
// Die
Die::Die()
  : dieLx_(0),
    dieLy_(0),
    dieUx_(0),
    dieUy_(0),
    coreLx_(0),
    coreLy_(0),
    coreUx_(0),
    coreUy_(0)
{
}

Die::Die(const odb::Rect& dieRect, 
         const odb::Rect& coreRect) : Die()
{
  setDieBox(dieRect);
  setCoreBox(coreRect);
}

void Die::setDieBox(const odb::Rect& dieRect)
{
  dieLx_ = dieRect.xMin();
  dieLy_ = dieRect.yMin();
  dieUx_ = dieRect.xMax();
  dieUy_ = dieRect.yMax();
}

void Die::setCoreBox(const odb::Rect& coreRect)
{
  coreLx_ = coreRect.xMin();
  coreLy_ = coreRect.yMin();
  coreUx_ = coreRect.xMax();
  coreUy_ = coreRect.yMax();
}


/////////////////////////////////////////////////////////////////
// Utilites functions
bool isCoreAreaOverlap(Die& die, const Instance* inst)
{
  int rectLx = std::max(die.coreLx(), inst->lx());
  int rectLy = std::max(die.coreLy(), inst->ly());
  int rectUx = std::min(die.coreUx(), inst->ux());
  int rectUy = std::min(die.coreUy(), inst->uy());
  return !(rectLx >= rectUx || rectLy >= rectUy);
}

int64_t getOverlapWithCoreArea(Die& die, const Instance* inst)
{
  int rectLx = max(die.coreLx(), inst->lx());
  int rectLy = max(die.coreLy(), inst->ly());
  int rectUx = min(die.coreUx(), inst->ux());
  int rectUy = min(die.coreUy(), inst->uy());
  if (rectLx >= rectUx || rectLy >= rectUy) {
    return 0;
  } else {
    return static_cast<int64_t>(rectUx - rectLx)
        * static_cast<int64_t>(rectUy - rectLy);
  }
}

// check if the odb::dbInst is fixed
bool isFixedOdbInst(odb::dbInst* inst) 
{
  bool isFixed = false;
  if (inst == nullptr) {
    return isFixed;
  } else {
    switch (inst->getPlacementStatus()) {
      case odb::dbPlacementStatus::NONE:
      case odb::dbPlacementStatus::UNPLACED:
      case odb::dbPlacementStatus::SUGGESTED:
      case odb::dbPlacementStatus::PLACED:
        isFixed = false;  
        break;
      case odb::dbPlacementStatus::LOCKED:
      case odb::dbPlacementStatus::FIRM:
      case odb::dbPlacementStatus::COVER:
        isFixed = true;  
        break;
    }
  }
  return isFixed;  
}

// utility functions for snaping the inst into sites
int snapDown(int value, int origin, int step)
{
  return ((value - origin) / step) * step + origin;
}

int snapUp(int value, int origin, int step)
{
  return ((value + step - 1 - origin) / step) * step + origin;
}


std::string intToStringWithPrecision(int value, int precision) 
{
  std::ostringstream out;
  out << std::fixed << std::setprecision(precision) << value;
  return out.str();
}


std::string floatToStringWithPrecision(float value, int precision) 
{
  std::ostringstream out;
  out << std::fixed << std::setprecision(precision) << value;
  return out.str();
}




// utility functions for both host and device
// A function that does 2D integration to the density function of a
// bivariate normal distribution with 0 correlation.
// Essentially, the function being integrated is the product
// of 2 1D probability density functions (for x and y). The means and standard
// deviation of the probablity density functions are parametarized. In this
// function, I am using the closed-form solution of the integration. The limits
// of integration are lx->ux and ly->uy For reference: the equation that is
// being integrated is:
//      (1/(2*pi*sigmaX*sigmaY))*e^(-(y-meanY)^2/(2*sigmaY*sigmaY))*e^(-(x-meanX)^2/(2*sigmaX*sigmaX))
int calculateBiVariateNormalCDF(biNormalParameters i)
{
  const int x1 = (i.meanX - i.lx) / (sqrt(2) * i.sigmaX);
  const int x2 = (i.meanX - i.ux) / (sqrt(2) * i.sigmaX);

  const int y1 = (i.meanY - i.ly) / (sqrt(2) * i.sigmaY);
  const int y2 = (i.meanY - i.uy) / (sqrt(2) * i.sigmaY);

  return 0.25
         * (erf(x1) * erf(y1) + erf(x2) * erf(y2) - erf(x1) * erf(y2)
            - erf(x2) * erf(y1));
}




// https://stackoverflow.com/questions/33333363/built-in-mod-vs-custom-mod-function-improve-the-performance-of-modulus-op
__host__ __device__
int fastModulo(const int input, const int ceil)
{
  return input >= ceil ? input % ceil : input;
}



}
