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

#include <opendb/db.h>

#include <iostream>

#include "nesterovBase.h"
#include "utl/Logger.h"

namespace gpl {

using namespace odb;
using namespace std;
using utl::GPL;

static int fastModulo(const int input, const int ceil);

static std::pair<int, int> getMinMaxIdx(int ll,
                                        int uu,
                                        int coreLL,
                                        int siteSize,
                                        int minIdx,
                                        int maxIdx);

static utl::Logger* slog_;

static bool isCoreAreaOverlap(Die& die, Instance& inst);

static int64_t getOverlapWithCoreArea(Die& die, Instance& inst);

static bool isPlacementInst(dbInst* inst);

////////////////////////////////////////////////////////
// Instance

Instance::Instance()
    : inst_(nullptr), lx_(0), ly_(0), ux_(0), uy_(0), extId_(INT_MIN)
{
}

// for movable real instances
Instance::Instance(odb::dbInst* inst, int padLeft, int padRight) : Instance()
{
  inst_ = inst;
  int lx = 0, ly = 0;
  inst_->getLocation(lx, ly);

  // padding apply on placeInstance
  if (isPlaceInstance()) {
    lx_ = lx - padLeft;
    ly_ = ly;
    ux_ = lx + inst_->getBBox()->getDX() + padRight;
    uy_ = ly + inst_->getBBox()->getDY();
  } else {
    lx_ = lx;
    ly_ = ly;
    ux_ = lx + inst_->getBBox()->getDX();
    uy_ = ly + inst_->getBBox()->getDY();
  }

  //
  // TODO
  // need additional adjustment
  // if instance (macro) is fixed and
  // its coordi is not multiple of rows' integer.
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

  switch (inst_->getPlacementStatus()) {
    case dbPlacementStatus::NONE:
    case dbPlacementStatus::UNPLACED:
    case dbPlacementStatus::SUGGESTED:
    case dbPlacementStatus::PLACED:
      return false;
      break;
    case dbPlacementStatus::LOCKED:
    case dbPlacementStatus::FIRM:
    case dbPlacementStatus::COVER:
      return true;
      break;
  }
  return false;
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

void Instance::dbSetPlacementStatus(dbPlacementStatus ps)
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

void Instance::addPin(Pin* pin)
{
  pins_.push_back(pin);
}

void Instance::setExtId(int extId)
{
  extId_ = extId;
}

////////////////////////////////////////////////////////
// Pin

Pin::Pin()
    : term_(nullptr),
      inst_(nullptr),
      net_(nullptr),
      cx_(0),
      cy_(0),
      offsetCx_(0),
      offsetCy_(0),
      iTermField_(0),
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

Pin::Pin(odb::dbBTerm* bTerm) : Pin()
{
  setBTerm();
  term_ = (void*) bTerm;
  updateCoordi(bTerm);
}

std::string Pin::name() const
{
  if (!term_) {
    return "DUMMY";
  }
  if (isITerm()) {
    return dbITerm()->getInst()->getName() + '/'
           + dbITerm()->getMTerm()->getName();
  } else {
    return dbBTerm()->getName();
  }
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
  int offsetLx = INT_MAX;
  int offsetLy = INT_MAX;
  int offsetUx = INT_MIN;
  int offsetUy = INT_MIN;

  for (dbMPin* mPin : iTerm->getMTerm()->getMPins()) {
    for (dbBox* box : mPin->getGeometry()) {
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
    offsetCx_ = offsetCy_ = 0;
  }
  // usual case
  else {
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
void Pin::updateCoordi(odb::dbBTerm* bTerm)
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
    string msg
        = string(bTerm->getConstName()) + " toplevel port is not placed!\n";
    msg += "       Replace will regard " + string(bTerm->getConstName())
           + " is placed in (0, 0)";
    slog_->warn(GPL, 1, msg);
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

Net::Net() : net_(nullptr), lx_(0), ly_(0), ux_(0), uy_(0)
{
}
Net::Net(odb::dbNet* net) : Net()
{
  net_ = net;
  updateBox();
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
  return static_cast<int64_t>((ux_ - lx_) + (uy_ - ly_));
}

void Net::updateBox()
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
}

////////////////////////////////////////////////////////
// PlacerBase

PlacerBase::PlacerBase()
    : db_(nullptr),
      log_(nullptr),
      pbVars_(),
      siteSizeX_(0),
      siteSizeY_(0),
      placeInstsArea_(0),
      nonPlaceInstsArea_(0),
      macroInstsArea_(0),
      stdInstsArea_(0)
{
}

PlacerBase::PlacerBase(odb::dbDatabase* db,
                       PlacerBaseVars pbVars,
                       utl::Logger* log)
    : PlacerBase()
{
  db_ = db;
  log_ = log;
  pbVars_ = pbVars;
  init();
}

PlacerBase::~PlacerBase()
{
  reset();
}

void PlacerBase::init()
{
  slog_ = log_;

  log_->info(GPL, 2, "DBU: {}", db_->getTech()->getDbUnitsPerMicron());

  dbBlock* block = db_->getChip()->getBlock();
  dbSet<dbInst> insts = block->getInsts();

  // die-core area update
  dbSet<dbRow> rows = block->getRows();
  odb::Rect coreRect;
  block->getCoreArea(coreRect);
  odb::Rect dieRect;
  block->getDieArea(dieRect);

  if (!dieRect.contains(coreRect))
    log_->error(GPL, 118, "core area outside of die.");

  die_ = Die(dieRect, coreRect);

  // siteSize update
  dbRow* firstRow = *(rows.begin());
  siteSizeX_ = firstRow->getSite()->getWidth();
  siteSizeY_ = firstRow->getSite()->getHeight();

  log_->info(GPL, 3, "SiteSize: {} {}", siteSizeX_, siteSizeY_);
  log_->info(GPL, 4, "CoreAreaLxLy: {} {}", die_.coreLx(), die_.coreLy());
  log_->info(GPL, 5, "CoreAreaUxUy: {} {}", die_.coreUx(), die_.coreUy());

  // insts fill with real instances
  instStor_.reserve(insts.size());
  for (dbInst* inst : insts) {
    if (!isPlacementInst(inst)) {
      continue;
    }
    Instance myInst(
        inst, pbVars_.padLeft * siteSizeX_, pbVars_.padRight * siteSizeX_);
    instStor_.push_back(myInst);

    dbBox* bbox = inst->getBBox();
    if (bbox->getDY() > die_.coreDy())
      log_->error(
          GPL, 119, "instance {} height is larger than core.", inst->getName());
    if (bbox->getDX() > die_.coreDx())
      log_->error(
          GPL, 120, "instance {} width is larger than core.", inst->getName());
  }

  // insts fill with fake instances (fragmented row or blockage)
  initInstsForUnusableSites();

  // init inst ptrs and areas
  insts_.reserve(instStor_.size());
  for (auto& inst : instStor_) {
    if (inst.isInstance()) {
      if (inst.isFixed()) {
        // Check whether fixed instance is
        // within the corearea
        //
        // outside of corearea is none of RePlAce's business
        if (isCoreAreaOverlap(die_, inst)) {
          fixedInsts_.push_back(&inst);
          nonPlaceInsts_.push_back(&inst);
          nonPlaceInstsArea_ += getOverlapWithCoreArea(die_, inst);
        }
      } else {
        placeInsts_.push_back(&inst);
        int64_t instArea
            = static_cast<int64_t>(inst.dx()) * static_cast<int64_t>(inst.dy());
        placeInstsArea_ += instArea;
        // macro cells should be
        // macroInstsArea_
        if (inst.dy() > siteSizeY_ * 6) {
          macroInstsArea_ += instArea;
        }
        // smaller or equal height cells should be
        // stdInstArea_
        else {
          stdInstsArea_ += instArea;
        }
      }
      instMap_[inst.dbInst()] = &inst;
    } else if (inst.isDummy()) {
      dummyInsts_.push_back(&inst);
      nonPlaceInsts_.push_back(&inst);
      nonPlaceInstsArea_
          += static_cast<int64_t>(inst.dx()) * static_cast<int64_t>(inst.dy());
    }
    insts_.push_back(&inst);
  }

  // nets fill
  dbSet<dbNet> nets = block->getNets();
  netStor_.reserve(nets.size());
  for (dbNet* net : nets) {
    dbSigType netType = net->getSigType();

    // escape nets with VDD/VSS/reset nets
    if (netType == dbSigType::GROUND || netType == dbSigType::POWER
        || netType == dbSigType::RESET) {
      continue;
    }

    Net myNet(net);
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

    for (dbBTerm* bTerm : net->getBTerms()) {
      Pin myPin(bTerm);
      myPin.setNet(myNetPtr);
      pinStor_.push_back(myPin);
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
    for (dbBTerm* bTerm : net.dbNet()->getBTerms()) {
      net.addPin(dbToPb(bTerm));
    }
    nets_.push_back(&net);
  }

  printInfo();
}

// Use dummy instance to fill unusable sites.  Sites are unusable
// due to fragmented rows or placement blockages.
void PlacerBase::initInstsForUnusableSites()
{
  dbSet<dbRow> rows = db_->getChip()->getBlock()->getRows();

  int siteCountX = (die_.coreUx() - die_.coreLx()) / siteSizeX_;
  int siteCountY = (die_.coreUy() - die_.coreLy()) / siteSizeY_;

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

  // fill in rows' bbox
  for (dbRow* row : rows) {
    Rect rect;
    row->getBBox(rect);

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

  // Mark blockage areas as empty so that their sites will be blocked.
  for (dbBlockage* blockage : db_->getChip()->getBlock()->getBlockages()) {
    dbInst* inst = blockage->getInstance();
    if (inst && !inst->isFixed()) {
      string msg
          = "Blockages associated with moveable instances "
            " are unsupported and ignored [inst: "
            + inst->getName() + "]\n";
      slog_->error(GPL, 3, msg);
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
  for (auto& inst : instStor_) {
    if (!inst.isFixed()) {
      continue;
    }
    std::pair<int, int> pairX = getMinMaxIdx(
        inst.lx(), inst.ux(), die_.coreLx(), siteSizeX_, 0, siteCountX);
    std::pair<int, int> pairY = getMinMaxIdx(
        inst.ly(), inst.uy(), die_.coreLy(), siteSizeY_, 0, siteCountY);

    for (int i = pairX.first; i < pairX.second; i++) {
      for (int j = pairY.first; j < pairY.second; j++) {
        siteGrid[j * siteCountX + i] = FixedInst;
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

  placeInsts_.clear();
  fixedInsts_.clear();
  nonPlaceInsts_.clear();
}

int64_t PlacerBase::hpwl() const
{
  int64_t hpwl = 0;
  for (auto& net : nets_) {
    net->updateBox();
    hpwl += net->hpwl();
  }
  return hpwl;
}

Instance* PlacerBase::dbToPb(odb::dbInst* inst) const
{
  auto instPtr = instMap_.find(inst);
  return (instPtr == instMap_.end()) ? nullptr : instPtr->second;
}

Pin* PlacerBase::dbToPb(odb::dbITerm* term) const
{
  auto pinPtr = pinMap_.find((void*) term);
  return (pinPtr == pinMap_.end()) ? nullptr : pinPtr->second;
}

Pin* PlacerBase::dbToPb(odb::dbBTerm* term) const
{
  auto pinPtr = pinMap_.find((void*) term);
  return (pinPtr == pinMap_.end()) ? nullptr : pinPtr->second;
}

Net* PlacerBase::dbToPb(odb::dbNet* net) const
{
  auto netPtr = netMap_.find(net);
  return (netPtr == netMap_.end()) ? nullptr : netPtr->second;
}

void PlacerBase::printInfo() const
{
  log_->info(GPL, 6, "NumInstances: {}", instStor_.size());
  log_->info(GPL, 7, "NumPlaceInstances: {}", placeInsts_.size());
  log_->info(GPL, 8, "NumFixedInstances: {}", fixedInsts_.size());
  log_->info(GPL, 9, "NumDummyInstances: {}", dummyInsts_.size());
  log_->info(GPL, 10, "NumNets: {}", nets_.size());
  log_->info(GPL, 11, "NumPins: {}", pins_.size());

  int maxFanout = INT_MIN;
  int sumFanout = 0;
  for (auto& net : nets_) {
    if (maxFanout < (int) net->pins().size()) {
      maxFanout = (int) net->pins().size();
    }
    sumFanout += (int) net->pins().size();
  }

  log_->info(GPL, 12, "DieAreaLxLy: {} {}", die_.dieLx(), die_.dieLy());
  log_->info(GPL, 13, "DieAreaUxUy: {} {}", die_.dieUx(), die_.dieUy());
  log_->info(GPL, 14, "CoreAreaLxLy: {} {}", die_.coreLx(), die_.coreLy());
  log_->info(GPL, 15, "CoreAreaUxUy: {} {}", die_.coreUx(), die_.coreUy());

  const int64_t coreArea = die_.coreArea();
  float util = static_cast<float>(placeInstsArea_)
               / (coreArea - nonPlaceInstsArea_) * 100;

  log_->info(GPL, 16, "CoreArea: {}", coreArea);
  log_->info(GPL, 17, "NonPlaceInstsArea: {}", nonPlaceInstsArea_);

  log_->info(GPL, 18, "PlaceInstsArea: {}", placeInstsArea_);
  log_->info(GPL, 19, "Util(%): {:.2f}", util);

  log_->info(GPL, 20, "StdInstsArea: {}", stdInstsArea_);
  log_->info(GPL, 21, "MacroInstsArea: {}", macroInstsArea_);

  if (util >= 100.1) {
    log_->error(GPL, 301, "Utilization exceeds 100%.");
  }
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
  int rectLx = max(die.coreLx(), inst.lx()),
      rectLy = max(die.coreLy(), inst.ly()),
      rectUx = min(die.coreUx(), inst.ux()),
      rectUy = min(die.coreUy(), inst.uy());
  return !(rectLx >= rectUx || rectLy >= rectUy);
}

static int64_t getOverlapWithCoreArea(Die& die, Instance& inst)
{
  int rectLx = max(die.coreLx(), inst.lx()),
      rectLy = max(die.coreLy(), inst.ly()),
      rectUx = min(die.coreUx(), inst.ux()),
      rectUy = min(die.coreUy(), inst.uy());
  return static_cast<int64_t>(rectUx - rectLx)
         * static_cast<int64_t>(rectUy - rectLy);
}

static bool isPlacementInst(dbInst* inst)
{
  switch (inst->getMaster()->getType()) {
    case dbMasterType::BLOCK:
    case dbMasterType::BLOCK_BLACKBOX:
    case dbMasterType::BLOCK_SOFT:
    case dbMasterType::CORE:
    case dbMasterType::CORE_FEEDTHRU:
    case dbMasterType::CORE_TIEHIGH:
    case dbMasterType::CORE_TIELOW:
    case dbMasterType::CORE_SPACER:
    case dbMasterType::CORE_ANTENNACELL:
    case dbMasterType::CORE_WELLTAP:
      return true;
    default:
      return false;
  }
  return false;
}

}  // namespace gpl
