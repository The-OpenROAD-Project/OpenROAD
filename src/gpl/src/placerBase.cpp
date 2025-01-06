///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2020, The Regents of the University of California
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

#include "placerBase.h"

#include <iostream>
#include <utility>

#include "nesterovBase.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "utl/Logger.h"

namespace gpl {

using odb::dbBlock;
using odb::dbBlockage;
using odb::dbBox;
using odb::dbBPin;
using odb::dbBTerm;
using odb::dbInst;
using odb::dbITerm;
using odb::dbMPin;
using odb::dbNet;
using odb::dbPlacementStatus;
using odb::dbPowerDomain;
using odb::dbRow;
using odb::dbSet;
using odb::dbSigType;
using odb::Rect;
using utl::GPL;

static int fastModulo(int input, int ceil);

static std::pair<int, int> getMinMaxIdx(int ll,
                                        int uu,
                                        int coreLL,
                                        int siteSize,
                                        int minIdx,
                                        int maxIdx);

static bool isCoreAreaOverlap(Die& die, Instance& inst);

static int64_t getOverlapWithCoreArea(Die& die, Instance& inst);

////////////////////////////////////////////////////////
// Instance

Instance::Instance() = default;

// for movable real instances
Instance::Instance(odb::dbInst* inst,
                   int padLeft,
                   int padRight,
                   int site_height,
                   utl::Logger* logger)
    : Instance()
{
  inst_ = inst;
  dbBox* bbox = inst->getBBox();
  inst->getLocation(lx_, ly_);
  ux_ = lx_ + bbox->getDX();
  uy_ = ly_ + bbox->getDY();

  if (isPlaceInstance()) {
    lx_ -= padLeft;
    ux_ += padRight;
  }

  // Masters more than row_limit rows tall are treated as macros
  constexpr int row_limit = 6;

  if (inst->getMaster()->getType().isBlock()) {
    is_macro_ = true;
  } else if (bbox->getDY() > 6 * site_height) {
    is_macro_ = true;
    logger->warn(GPL,
                 134,
                 "Master {} is not marked as a BLOCK in LEF but is more "
                 "than {} rows tall.  It will be treated as a macro.",
                 inst->getMaster()->getName(),
                 row_limit);
  }
}

// for dummy instances
Instance::Instance(int lx, int ly, int ux, int uy) : Instance()
{
  inst_ = nullptr;
  lx_ = lx;
  ly_ = ly;
  ux_ = ux;
  uy_ = uy;
}

Instance::~Instance()
{
  inst_ = nullptr;
  lx_ = ly_ = 0;
  ux_ = uy_ = 0;
  pins_.clear();
}

bool Instance::isFixed() const
{
  // dummy instance is always fixed
  if (isDummy()) {
    return true;
  }

  return inst_->getPlacementStatus().isFixed();
}

bool Instance::isInstance() const
{
  return (inst_ != nullptr);
}

bool Instance::isPlaceInstance() const
{
  return (isInstance() && !isFixed());
}

bool Instance::isDummy() const
{
  return (inst_ == nullptr);
}

void Instance::setLocation(int x, int y)
{
  ux_ = x + (ux_ - lx_);
  uy_ = y + (uy_ - ly_);

  lx_ = x;
  ly_ = y;

  // pins update
  for (auto& pin : pins_) {
    pin->updateLocation(this);
  }
}

void Instance::setCenterLocation(int x, int y)
{
  const int halfX = (ux_ - lx_) / 2;
  const int halfY = (uy_ - ly_) / 2;
  lx_ = x - halfX;
  ly_ = y - halfY;
  ux_ = x + halfX;
  uy_ = y + halfY;

  // pins update
  for (auto& pin : pins_) {
    pin->updateLocation(this);
  }
}

void Instance::dbSetPlaced()
{
  inst_->setPlacementStatus(dbPlacementStatus::PLACED);
}

void Instance::dbSetPlacementStatus(const dbPlacementStatus& ps)
{
  inst_->setPlacementStatus(ps);
}

void Instance::dbSetLocation()
{
  inst_->setLocation(lx_, ly_);
}

void Instance::dbSetLocation(int x, int y)
{
  setLocation(x, y);
  dbSetLocation();
}

void Instance::dbSetCenterLocation(int x, int y)
{
  setCenterLocation(x, y);
  dbSetLocation();
}

int Instance::lx() const
{
  return lx_;
}

int Instance::ly() const
{
  return ly_;
}

int Instance::ux() const
{
  return ux_;
}

int Instance::uy() const
{
  return uy_;
}

int Instance::cx() const
{
  return (lx_ + ux_) / 2;
}

int Instance::cy() const
{
  return (ly_ + uy_) / 2;
}

int Instance::dx() const
{
  return (ux_ - lx_);
}

int Instance::dy() const
{
  return (uy_ - ly_);
}

int64_t Instance::area() const
{
  return static_cast<int64_t>(dx()) * dy();
}

void Instance::addPin(Pin* pin)
{
  pins_.push_back(pin);
}

void Instance::setExtId(int extId)
{
  extId_ = extId;
}

bool Instance::isMacro() const
{
  return is_macro_;
}

bool Instance::isLocked() const
{
  return is_locked_;
}

void Instance::lock()
{
  is_locked_ = true;
}

void Instance::unlock()
{
  is_locked_ = false;
}

static int snapDown(int value, int origin, int step)
{
  return ((value - origin) / step) * step + origin;
}

static int snapUp(int value, int origin, int step)
{
  return ((value + step - 1 - origin) / step) * step + origin;
}

void Instance::snapOutward(const odb::Point& origin, int step_x, int step_y)
{
  lx_ = snapDown(lx_, origin.x(), step_x);
  ly_ = snapDown(ly_, origin.y(), step_y);
  ux_ = snapUp(ux_, origin.x(), step_x);
  uy_ = snapUp(uy_, origin.y(), step_y);
}

////////////////////////////////////////////////////////
// Pin

Pin::Pin()
    : iTermField_(0),
      bTermField_(0),
      minPinXField_(0),
      minPinYField_(0),
      maxPinXField_(0),
      maxPinYField_(0)
{
}

Pin::Pin(odb::dbITerm* iTerm) : Pin()
{
  setITerm();
  term_ = (void*) iTerm;
  updateCoordi(iTerm);
}

Pin::Pin(odb::dbBTerm* bTerm, utl::Logger* logger) : Pin()
{
  setBTerm();
  term_ = (void*) bTerm;
  updateCoordi(bTerm, logger);
}

std::string Pin::name() const
{
  if (!term_) {
    return "DUMMY";
  }
  if (isITerm()) {
    return dbITerm()->getName();
  }
  return dbBTerm()->getName();
}

void Pin::setITerm()
{
  iTermField_ = 1;
}

void Pin::setBTerm()
{
  bTermField_ = 1;
}

void Pin::setMinPinX()
{
  minPinXField_ = 1;
}

void Pin::setMinPinY()
{
  minPinYField_ = 1;
}

void Pin::setMaxPinX()
{
  maxPinXField_ = 1;
}

void Pin::setMaxPinY()
{
  maxPinYField_ = 1;
}

void Pin::unsetMinPinX()
{
  minPinXField_ = 0;
}

void Pin::unsetMinPinY()
{
  minPinYField_ = 0;
}

void Pin::unsetMaxPinX()
{
  maxPinXField_ = 0;
}

void Pin::unsetMaxPinY()
{
  maxPinYField_ = 0;
}

bool Pin::isITerm() const
{
  return (iTermField_ == 1);
}

bool Pin::isBTerm() const
{
  return (bTermField_ == 1);
}

bool Pin::isMinPinX() const
{
  return (minPinXField_ == 1);
}

bool Pin::isMinPinY() const
{
  return (minPinYField_ == 1);
}

bool Pin::isMaxPinX() const
{
  return (maxPinXField_ == 1);
}

bool Pin::isMaxPinY() const
{
  return (maxPinYField_ == 1);
}

int Pin::cx() const
{
  return cx_;
}

int Pin::cy() const
{
  return cy_;
}

int Pin::offsetCx() const
{
  return offsetCx_;
}

int Pin::offsetCy() const
{
  return offsetCy_;
}

odb::dbITerm* Pin::dbITerm() const
{
  return (isITerm()) ? (odb::dbITerm*) term_ : nullptr;
}
odb::dbBTerm* Pin::dbBTerm() const
{
  return (isBTerm()) ? (odb::dbBTerm*) term_ : nullptr;
}

void Pin::updateCoordi(odb::dbITerm* iTerm)
{
  Rect pin_bbox;
  pin_bbox.mergeInit();

  for (dbMPin* mPin : iTerm->getMTerm()->getMPins()) {
    for (dbBox* box : mPin->getGeometry()) {
      pin_bbox.merge(box->getBox());
    }
  }

  odb::dbInst* inst = iTerm->getInst();
  const int lx = inst->getBBox()->xMin();
  const int ly = inst->getBBox()->yMin();

  odb::dbMaster* master = iTerm->getInst()->getMaster();
  const int instCenterX = master->getWidth() / 2;
  const int instCenterY = master->getHeight() / 2;

  // Pin has no shapes (rare/odd)
  if (pin_bbox.isInverted()) {
    offsetCx_ = offsetCy_ = 0;  // offset is center of the instance
  } else {
    // Rotate the pin's bbox in correspondence to the instance's orientation.
    // Rotation consists of (1) shift the instance center to the origin
    // (2) rotate (3) shift back.
    const auto inst_orient = inst->getTransform().getOrient();
    odb::dbTransform xfm({-instCenterX, -instCenterY});
    xfm.concat(inst_orient);
    xfm.concat(odb::dbTransform({instCenterX, instCenterY}));
    xfm.apply(pin_bbox);
    // offset is Pin BBoxs' center, so
    // subtract the Origin coordinates (e.g. instCenterX, instCenterY)
    //
    // Transform coordinates
    // from (origin: 0,0)
    // to (origin: instCenterX, instCenterY)
    //
    offsetCx_ = pin_bbox.xCenter() - instCenterX;
    offsetCy_ = pin_bbox.yCenter() - instCenterY;
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

  for (dbBPin* bPin : bTerm->getBPins()) {
    Rect bbox = bPin->getBBox();
    lx = std::min(bbox.xMin(), lx);
    ly = std::min(bbox.yMin(), ly);
    ux = std::max(bbox.xMax(), ux);
    uy = std::max(bbox.yMax(), uy);
  }

  if (lx == INT_MAX || ly == INT_MAX || ux == INT_MIN || uy == INT_MIN) {
    logger->warn(GPL,
                 1,
                 "{} toplevel port is not placed!\n"
                 "       Replace will regard {} is placed in (0, 0)",
                 bTerm->getConstName(),
                 bTerm->getConstName());
  }

  // Just center
  offsetCx_ = offsetCy_ = 0;

  cx_ = (lx + ux) / 2;
  cy_ = (ly + uy) / 2;
}

void Pin::updateLocation(const Instance* inst)
{
  cx_ = inst->cx() + offsetCx_;
  cy_ = inst->cy() + offsetCy_;
}

void Pin::setInstance(Instance* inst)
{
  inst_ = inst;
}

void Pin::setNet(Net* net)
{
  net_ = net;
}

bool Pin::isPlaceInstConnected() const
{
  if (!inst_) {
    return false;
  }
  return inst_->isPlaceInstance();
}

Pin::~Pin()
{
  term_ = nullptr;
  inst_ = nullptr;
  net_ = nullptr;
}

////////////////////////////////////////////////////////
// Net

Net::Net() = default;

Net::Net(odb::dbNet* net, bool skipIoMode) : Net()
{
  net_ = net;
  updateBox(skipIoMode);
}

Net::~Net()
{
  net_ = nullptr;
  lx_ = ly_ = ux_ = uy_ = 0;
}

int Net::lx() const
{
  return lx_;
}

int Net::ly() const
{
  return ly_;
}

int Net::ux() const
{
  return ux_;
}

int Net::uy() const
{
  return uy_;
}

int Net::cx() const
{
  return (lx_ + ux_) / 2;
}

int Net::cy() const
{
  return (ly_ + uy_) / 2;
}

int64_t Net::hpwl() const
{
  if (ux_ < lx_) {  // dangling net
    return 0;
  }
  return static_cast<int64_t>(ux_ - lx_) + (uy_ - ly_);
}

void Net::updateBox(bool skipIoMode)
{
  lx_ = INT_MAX;
  ly_ = INT_MAX;
  ux_ = INT_MIN;
  uy_ = INT_MIN;
  for (dbITerm* iTerm : net_->getITerms()) {
    dbBox* box = iTerm->getInst()->getBBox();
    lx_ = std::min(box->xMin(), lx_);
    ly_ = std::min(box->yMin(), ly_);
    ux_ = std::max(box->xMax(), ux_);
    uy_ = std::max(box->yMax(), uy_);
  }

  if (skipIoMode == false) {
    for (dbBTerm* bTerm : net_->getBTerms()) {
      for (dbBPin* bPin : bTerm->getBPins()) {
        Rect bbox = bPin->getBBox();
        lx_ = std::min(bbox.xMin(), lx_);
        ly_ = std::min(bbox.yMin(), ly_);
        ux_ = std::max(bbox.xMax(), ux_);
        uy_ = std::max(bbox.yMax(), uy_);
      }
    }
  }
}

void Net::addPin(Pin* pin)
{
  pins_.push_back(pin);
}

odb::dbSigType Net::getSigType() const
{
  return net_->getSigType();
}

////////////////////////////////////////////////////////
// Die

Die::Die() = default;

Die::Die(const odb::Rect& dieRect, const odb::Rect& coreRect) : Die()
{
  setDieBox(dieRect);
  setCoreBox(coreRect);
}

Die::~Die()
{
  dieLx_ = dieLy_ = dieUx_ = dieUy_ = 0;
  coreLx_ = coreLy_ = coreUx_ = coreUy_ = 0;
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

int Die::dieCx() const
{
  return (dieLx_ + dieUx_) / 2;
}

int Die::dieCy() const
{
  return (dieLy_ + dieUy_) / 2;
}

int Die::dieDx() const
{
  return dieUx_ - dieLx_;
}

int Die::dieDy() const
{
  return dieUy_ - dieLy_;
}

int Die::coreCx() const
{
  return (coreLx_ + coreUx_) / 2;
}

int Die::coreCy() const
{
  return (coreLy_ + coreUy_) / 2;
}

int Die::coreDx() const
{
  return coreUx_ - coreLx_;
}

int Die::coreDy() const
{
  return coreUy_ - coreLy_;
}

int64_t Die::dieArea() const
{
  return static_cast<int64_t>(dieDx()) * static_cast<int64_t>(dieDy());
}

int64_t Die::coreArea() const
{
  return static_cast<int64_t>(coreDx()) * static_cast<int64_t>(coreDy());
}

PlacerBaseVars::PlacerBaseVars()
{
  reset();
}

void PlacerBaseVars::reset()
{
  padLeft = padRight = 0;
  skipIoMode = false;
}

////////////////////////////////////////////////////////
// PlacerBaseCommon

PlacerBaseCommon::PlacerBaseCommon() = default;

PlacerBaseCommon::PlacerBaseCommon(odb::dbDatabase* db,
                                   PlacerBaseVars pbVars,
                                   utl::Logger* log)
    : PlacerBaseCommon()
{
  db_ = db;
  log_ = log;
  pbVars_ = pbVars;
  init();
}

PlacerBaseCommon::~PlacerBaseCommon()
{
  reset();
}

void PlacerBaseCommon::init()
{
  log_->info(GPL, 2, "DBU: {}", db_->getTech()->getDbUnitsPerMicron());

  dbBlock* block = db_->getChip()->getBlock();

  // die-core area update
  odb::dbSite* site = nullptr;
  for (auto* row : block->getRows()) {
    if (row->getSite()->getClass() != odb::dbSiteClass::PAD) {
      site = row->getSite();
      break;
    }
  }
  if (site == nullptr) {
    log_->error(GPL, 305, "Unable to find a site");
  }
  odb::Rect coreRect = block->getCoreArea();
  odb::Rect dieRect = block->getDieArea();

  if (!dieRect.contains(coreRect)) {
    log_->error(GPL, 118, "core area outside of die.");
  }

  die_ = Die(dieRect, coreRect);

  // siteSize update
  siteSizeX_ = site->getWidth();
  siteSizeY_ = site->getHeight();

  log_->info(GPL,
             3,
             "{:9} ( {:6.3f} {:6.3f} ) um",
             "SiteSize:",
             block->dbuToMicrons(siteSizeX_),
             block->dbuToMicrons(siteSizeY_));
  log_->info(GPL,
             4,
             "{:9} ( {:6.3f} {:6.3f} ) ( {:6.3f} {:6.3f} ) um",
             "CoreBBox:",
             block->dbuToMicrons(die_.coreLx()),
             block->dbuToMicrons(die_.coreLy()),
             block->dbuToMicrons(die_.coreUx()),
             block->dbuToMicrons(die_.coreUy()));

  // insts fill with real instances
  dbSet<dbInst> insts = block->getInsts();
  instStor_.reserve(insts.size());
  insts_.reserve(instStor_.size());
  for (dbInst* inst : insts) {
    auto type = inst->getMaster()->getType();
    if (!type.isCore() && !type.isBlock()) {
      continue;
    }
    Instance myInst(inst,
                    pbVars_.padLeft * siteSizeX_,
                    pbVars_.padRight * siteSizeX_,
                    siteSizeY_,
                    log_);

    // Fixed instaces need to be snapped outwards to the nearest site
    // boundary.  A partially overlapped site is unusable and this
    // is the simplest way to ensure it is counted as fully used.
    if (myInst.isFixed()) {
      myInst.snapOutward(coreRect.ll(), siteSizeX_, siteSizeY_);
    }

    instStor_.push_back(myInst);

    if (myInst.dy() > siteSizeY_ * 6) {
      macroInstsArea_ += myInst.area();
    }

    dbBox* bbox = inst->getBBox();
    if (bbox->getDY() > die_.coreDy()) {
      log_->error(
          GPL, 119, "instance {} height is larger than core.", inst->getName());
    }
    if (bbox->getDX() > die_.coreDx()) {
      log_->error(
          GPL, 120, "instance {} width is larger than core.", inst->getName());
    }
  }

  for (auto& inst : instStor_) {
    instMap_[inst.dbInst()] = &inst;
    insts_.push_back(&inst);

    if (!inst.isFixed()) {
      placeInsts_.push_back(&inst);
    }
  }

  // nets fill
  dbSet<dbNet> nets = block->getNets();
  netStor_.reserve(nets.size());
  for (dbNet* net : nets) {
    dbSigType netType = net->getSigType();

    // escape nets with VDD/VSS/reset nets
    if (netType == dbSigType::SIGNAL || netType == dbSigType::CLOCK) {
      Net myNet(net, pbVars_.skipIoMode);
      netStor_.push_back(myNet);

      // this is safe because of "reserve"
      Net* myNetPtr = &netStor_[netStor_.size() - 1];
      netMap_[net] = myNetPtr;

      for (dbITerm* iTerm : net->getITerms()) {
        Pin myPin(iTerm);
        myPin.setNet(myNetPtr);
        myPin.setInstance(dbToPb(iTerm->getInst()));
        pinStor_.push_back(myPin);
      }

      if (pbVars_.skipIoMode == false) {
        for (dbBTerm* bTerm : net->getBTerms()) {
          Pin myPin(bTerm, log_);
          myPin.setNet(myNetPtr);
          pinStor_.push_back(myPin);
        }
      }
    }
  }

  // pinMap_ and pins_ update
  pins_.reserve(pinStor_.size());
  for (auto& pin : pinStor_) {
    if (pin.isITerm()) {
      pinMap_[(void*) pin.dbITerm()] = &pin;
    } else if (pin.isBTerm()) {
      pinMap_[(void*) pin.dbBTerm()] = &pin;
    }
    pins_.push_back(&pin);
  }

  // instStor_'s pins_ fill
  for (auto& inst : instStor_) {
    if (!inst.isInstance()) {
      continue;
    }
    for (dbITerm* iTerm : inst.dbInst()->getITerms()) {
      // note that, DB's ITerm can have
      // VDD/VSS pins.
      //
      // Escape those pins
      Pin* curPin = dbToPb(iTerm);
      if (curPin) {
        inst.addPin(curPin);
      }
    }
  }

  // nets' pin update
  nets_.reserve(netStor_.size());
  for (auto& net : netStor_) {
    for (dbITerm* iTerm : net.dbNet()->getITerms()) {
      net.addPin(dbToPb(iTerm));
    }
    if (pbVars_.skipIoMode == false) {
      for (dbBTerm* bTerm : net.dbNet()->getBTerms()) {
        net.addPin(dbToPb(bTerm));
      }
    }
    nets_.push_back(&net);
  }
}

void PlacerBaseCommon::reset()
{
  db_ = nullptr;
  pbVars_.reset();

  instStor_.clear();
  pinStor_.clear();
  netStor_.clear();

  pins_.clear();
  nets_.clear();
  insts_.clear();

  instMap_.clear();
  pinMap_.clear();
  netMap_.clear();
}

int64_t PlacerBaseCommon::hpwl() const
{
  int64_t hpwl = 0;
  for (auto& net : nets_) {
    net->updateBox(pbVars_.skipIoMode);
    hpwl += net->hpwl();
  }
  return hpwl;
}

Instance* PlacerBaseCommon::dbToPb(odb::dbInst* inst) const
{
  auto instPtr = instMap_.find(inst);
  return (instPtr == instMap_.end()) ? nullptr : instPtr->second;
}

Pin* PlacerBaseCommon::dbToPb(odb::dbITerm* term) const
{
  auto pinPtr = pinMap_.find((void*) term);
  return (pinPtr == pinMap_.end()) ? nullptr : pinPtr->second;
}

Pin* PlacerBaseCommon::dbToPb(odb::dbBTerm* term) const
{
  auto pinPtr = pinMap_.find((void*) term);
  return (pinPtr == pinMap_.end()) ? nullptr : pinPtr->second;
}

Net* PlacerBaseCommon::dbToPb(odb::dbNet* net) const
{
  auto netPtr = netMap_.find(net);
  return (netPtr == netMap_.end()) ? nullptr : netPtr->second;
}

void PlacerBaseCommon::printInfo() const
{
}

void PlacerBaseCommon::unlockAll()
{
  for (auto inst : insts_) {
    inst->unlock();
  }
}

////////////////////////////////////////////////////////
// PlacerBase

PlacerBase::PlacerBase() = default;

PlacerBase::PlacerBase(odb::dbDatabase* db,
                       std::shared_ptr<PlacerBaseCommon> pbCommon,
                       utl::Logger* log,
                       odb::dbGroup* group)
    : PlacerBase()
{
  db_ = db;
  log_ = log;
  pbCommon_ = std::move(pbCommon);
  group_ = group;
  init();
}

PlacerBase::~PlacerBase()
{
  reset();
}

void PlacerBase::init()
{
  die_ = pbCommon_->die();

  // siteSize update
  siteSizeX_ = pbCommon_->siteSizeX();
  siteSizeY_ = pbCommon_->siteSizeY();

  for (auto& inst : pbCommon_->insts()) {
    if (!inst->isInstance()) {
      continue;
    }

    if (inst->dbInst() && inst->dbInst()->getGroup() != group_) {
      continue;
    }

    if (inst->isFixed()) {
      // Check whether fixed instance is
      // within the corearea
      //
      // outside of corearea is none of RePlAce's business
      if (isCoreAreaOverlap(die_, *inst)) {
        fixedInsts_.push_back(inst);
        nonPlaceInsts_.push_back(inst);
        nonPlaceInstsArea_ += getOverlapWithCoreArea(die_, *inst);
      }
    } else {
      placeInsts_.push_back(inst);
      int64_t instArea = inst->area();
      placeInstsArea_ += instArea;
      // macro cells should be
      // macroInstsArea_
      if (inst->dy() > siteSizeY_ * 6) {
        macroInstsArea_ += instArea;
      }
      // smaller or equal height cells should be
      // stdInstArea_
      else {
        stdInstsArea_ += instArea;
      }
    }

    insts_.push_back(inst);
  }

  // insts fill with fake instances (fragmented row or blockage)
  initInstsForUnusableSites();

  // handle newly added dummy instances
  for (auto& inst : instStor_) {
    if (inst.isDummy()) {
      dummyInsts_.push_back(&inst);
      nonPlaceInsts_.push_back(&inst);
      nonPlaceInstsArea_ += inst.area();
    }
    insts_.push_back(&inst);
  }

  printInfo();
}

// Use dummy instance to fill unusable sites.  Sites are unusable
// due to fragmented rows or placement blockages.
void PlacerBase::initInstsForUnusableSites()
{
  dbSet<dbRow> rows = db_->getChip()->getBlock()->getRows();
  dbSet<dbPowerDomain> pds = db_->getChip()->getBlock()->getPowerDomains();

  int64_t siteCountX = (die_.coreUx() - die_.coreLx()) / siteSizeX_;
  int64_t siteCountY = (die_.coreUy() - die_.coreLy()) / siteSizeY_;

  enum PlaceInfo
  {
    Empty,
    Row,
    FixedInst
  };

  //
  // Initialize siteGrid as empty
  //
  std::vector<PlaceInfo> siteGrid(siteCountX * siteCountY, PlaceInfo::Empty);

  // check if this belongs to a group
  // if there is a group, only mark the sites that belong to the group as Row
  // if there is no group, then mark all as Row, and then for each power
  // domain, mark the sites that belong to the power domain as Empty

  if (group_ != nullptr) {
    for (auto boundary : group_->getRegion()->getBoundaries()) {
      Rect rect = boundary->getBox();

      std::pair<int, int> pairX = getMinMaxIdx(
          rect.xMin(), rect.xMax(), die_.coreLx(), siteSizeX_, 0, siteCountX);

      std::pair<int, int> pairY = getMinMaxIdx(
          rect.yMin(), rect.yMax(), die_.coreLy(), siteSizeY_, 0, siteCountY);

      for (int i = pairX.first; i < pairX.second; i++) {
        for (int j = pairY.first; j < pairY.second; j++) {
          siteGrid[j * siteCountX + i] = Row;
        }
      }
    }
  } else {
    // fill in rows' bbox
    for (dbRow* row : rows) {
      Rect rect = row->getBBox();

      std::pair<int, int> pairX = getMinMaxIdx(
          rect.xMin(), rect.xMax(), die_.coreLx(), siteSizeX_, 0, siteCountX);

      std::pair<int, int> pairY = getMinMaxIdx(
          rect.yMin(), rect.yMax(), die_.coreLy(), siteSizeY_, 0, siteCountY);

      for (int i = pairX.first; i < pairX.second; i++) {
        for (int j = pairY.first; j < pairY.second; j++) {
          siteGrid[j * siteCountX + i] = Row;
        }
      }
    }
  }

  // Mark blockage areas as empty so that their sites will be blocked.
  for (dbBlockage* blockage : db_->getChip()->getBlock()->getBlockages()) {
    dbInst* inst = blockage->getInstance();
    if (inst && !inst->isFixed()) {
      std::string msg
          = "Blockages associated with moveable instances "
            " are unsupported and ignored [inst: "
            + inst->getName() + "]\n";
      log_->error(GPL, 3, msg);
      continue;
    }
    dbBox* bbox = blockage->getBBox();
    std::pair<int, int> pairX = getMinMaxIdx(
        bbox->xMin(), bbox->xMax(), die_.coreLx(), siteSizeX_, 0, siteCountX);

    std::pair<int, int> pairY = getMinMaxIdx(
        bbox->yMin(), bbox->yMax(), die_.coreLy(), siteSizeY_, 0, siteCountY);

    float filler_density = (100 - blockage->getMaxDensity()) / 100;
    int cells = 0;
    int filled = 0;

    for (int j = pairY.first; j < pairY.second; j++) {
      for (int i = pairX.first; i < pairX.second; i++) {
        if (cells == 0 || filled / (float) cells <= filler_density) {
          siteGrid[j * siteCountX + i] = Empty;
          ++filled;
        }
        ++cells;
      }
    }
  }

  // fill fixed instances' bbox
  for (auto& inst : pbCommon_->insts()) {
    if (!inst->isFixed()) {
      continue;
    }
    std::pair<int, int> pairX = getMinMaxIdx(
        inst->lx(), inst->ux(), die_.coreLx(), siteSizeX_, 0, siteCountX);
    std::pair<int, int> pairY = getMinMaxIdx(
        inst->ly(), inst->uy(), die_.coreLy(), siteSizeY_, 0, siteCountY);

    for (int i = pairX.first; i < pairX.second; i++) {
      for (int j = pairY.first; j < pairY.second; j++) {
        siteGrid[j * siteCountX + i] = FixedInst;
      }
    }
  }

  // In the case of top level power domain i.e no group,
  // mark all other power domains as empty
  if (group_ == nullptr) {
    for (dbPowerDomain* pd : pds) {
      if (pd->getGroup() != nullptr) {
        for (auto boundary : pd->getGroup()->getRegion()->getBoundaries()) {
          Rect rect = boundary->getBox();

          std::pair<int, int> pairX = getMinMaxIdx(rect.xMin(),
                                                   rect.xMax(),
                                                   die_.coreLx(),
                                                   siteSizeX_,
                                                   0,
                                                   siteCountX);

          std::pair<int, int> pairY = getMinMaxIdx(rect.yMin(),
                                                   rect.yMax(),
                                                   die_.coreLy(),
                                                   siteSizeY_,
                                                   0,
                                                   siteCountY);

          for (int i = pairX.first; i < pairX.second; i++) {
            for (int j = pairY.first; j < pairY.second; j++) {
              siteGrid[j * siteCountX + i] = Empty;
            }
          }
        }
      }
    }
  }

  //
  // Search the "Empty" coordinates on site-grid
  // --> These sites need to be dummyInstance
  //
  for (int j = 0; j < siteCountY; j++) {
    for (int i = 0; i < siteCountX; i++) {
      // if empty spot found
      if (siteGrid[j * siteCountX + i] == Empty) {
        int startX = i;
        // find end points
        while (i < siteCountX && siteGrid[j * siteCountX + i] == Empty) {
          i++;
        }
        int endX = i;
        Instance myInst(die_.coreLx() + siteSizeX_ * startX,
                        die_.coreLy() + siteSizeY_ * j,
                        die_.coreLx() + siteSizeX_ * endX,
                        die_.coreLy() + siteSizeY_ * (j + 1));
        instStor_.push_back(myInst);
      }
    }
  }
}

void PlacerBase::reset()
{
  db_ = nullptr;
  instStor_.clear();
  insts_.clear();
  placeInsts_.clear();
  fixedInsts_.clear();
  nonPlaceInsts_.clear();
}

void PlacerBase::printInfo() const
{
  dbBlock* block = db_->getChip()->getBlock();
  log_->info(GPL,
             6,
             "{:20} {:10}",
             "NumInstances:",
             placeInsts_.size() + fixedInsts_.size() + dummyInsts_.size());
  log_->info(GPL, 7, "{:20} {:10}", "NumPlaceInstances:", placeInsts_.size());
  log_->info(GPL, 8, "{:20} {:10}", "NumFixedInstances:", fixedInsts_.size());
  log_->info(GPL, 9, "{:20} {:10}", "NumDummyInstances:", dummyInsts_.size());
  log_->info(GPL, 10, "{:20} {:10}", "NumNets:", pbCommon_->nets().size());
  log_->info(GPL, 11, "{:20} {:10}", "NumPins:", pbCommon_->pins().size());

  log_->info(GPL,
             12,
             "{:9} ( {:6.3f} {:6.3f} ) ( {:6.3f} {:6.3f} ) um",
             "DieBBox:",
             block->dbuToMicrons(die_.dieLx()),
             block->dbuToMicrons(die_.dieLy()),
             block->dbuToMicrons(die_.dieUx()),
             block->dbuToMicrons(die_.dieUy()));
  log_->info(GPL,
             13,
             "{:9} ( {:6.3f} {:6.3f} ) ( {:6.3f} {:6.3f} ) um",
             "CoreBBox:",
             block->dbuToMicrons(die_.coreLx()),
             block->dbuToMicrons(die_.coreLy()),
             block->dbuToMicrons(die_.coreUx()),
             block->dbuToMicrons(die_.coreUy()));

  const int64_t coreArea = die_.coreArea();
  float util = static_cast<float>(placeInstsArea_)
               / (coreArea - nonPlaceInstsArea_) * 100;

  log_->info(GPL,
             16,
             "{:20} {:10.3f} um^2",
             "CoreArea:",
             block->dbuAreaToMicrons(coreArea));
  log_->info(GPL,
             17,
             "{:20} {:10.3f} um^2",
             "NonPlaceInstsArea:",
             block->dbuAreaToMicrons(nonPlaceInstsArea_));

  log_->info(GPL,
             18,
             "{:20} {:10.3f} um^2",
             "PlaceInstsArea:",
             block->dbuAreaToMicrons(placeInstsArea_));
  log_->info(GPL, 19, "{:20} {:10.3f} %", "Util:", util);

  log_->info(GPL,
             20,
             "{:20} {:10.3f} um^2",
             "StdInstsArea:",
             block->dbuAreaToMicrons(stdInstsArea_));

  log_->info(GPL,
             21,
             "{:20} {:10.3f} um^2",
             "MacroInstsArea:",
             block->dbuAreaToMicrons(macroInstsArea_));

  if (util >= 100.1) {
    log_->error(GPL, 301, "Utilization {:.3f} % exceeds 100%.", util);
  }
}

void PlacerBase::unlockAll()
{
  for (auto inst : insts_) {
    inst->unlock();
  }
}

int64_t PlacerBase::macroInstsArea() const
{
  return macroInstsArea_;
}

// https://stackoverflow.com/questions/33333363/built-in-mod-vs-custom-mod-function-improve-the-performance-of-modulus-op
static int fastModulo(const int input, const int ceil)
{
  return input >= ceil ? input % ceil : input;
}

static std::pair<int, int> getMinMaxIdx(int ll,
                                        int uu,
                                        int coreLL,
                                        int siteSize,
                                        int minIdx,
                                        int maxIdx)
{
  int lowerIdx = (ll - coreLL) / siteSize;
  int upperIdx = (fastModulo((uu - coreLL), siteSize) == 0)
                     ? (uu - coreLL) / siteSize
                     : (uu - coreLL) / siteSize + 1;
  return std::make_pair(std::max(minIdx, lowerIdx), std::min(maxIdx, upperIdx));
}

static bool isCoreAreaOverlap(Die& die, Instance& inst)
{
  int rectLx = std::max(die.coreLx(), inst.lx()),
      rectLy = std::max(die.coreLy(), inst.ly()),
      rectUx = std::min(die.coreUx(), inst.ux()),
      rectUy = std::min(die.coreUy(), inst.uy());
  return !(rectLx >= rectUx || rectLy >= rectUy);
}

static int64_t getOverlapWithCoreArea(Die& die, Instance& inst)
{
  int rectLx = std::max(die.coreLx(), inst.lx()),
      rectLy = std::max(die.coreLy(), inst.ly()),
      rectUx = std::min(die.coreUx(), inst.ux()),
      rectUy = std::min(die.coreUy(), inst.uy());
  return static_cast<int64_t>(rectUx - rectLx)
         * static_cast<int64_t>(rectUy - rectLy);
}

}  // namespace gpl
