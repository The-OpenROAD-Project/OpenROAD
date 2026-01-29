// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "placerBase.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "nesterovBase.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
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
Instance::Instance(odb::dbInst* db_inst,
                   PlacerBaseCommon* pbc,
                   utl::Logger* logger)
    : Instance()
{
  inst_ = db_inst;
  copyDbLocation(pbc);

  // Masters more than row_limit rows tall are treated as macros
  constexpr int row_limit = 6;
  dbBox* bbox = db_inst->getBBox();

  if (db_inst->getMaster()->getType().isBlock()) {
    is_macro_ = true;
  } else if (bbox->getDY() > row_limit * pbc->siteSizeY()) {
    is_macro_ = true;
    logger->warn(GPL,
                 134,
                 "Master {} is not marked as a BLOCK in LEF but is more "
                 "than {} rows tall.  It will be treated as a macro.",
                 db_inst->getMaster()->getName(),
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

void Instance::copyDbLocation(PlacerBaseCommon* pbc)
{
  dbBox* bbox = inst_->getBBox();
  inst_->getLocation(lx_, ly_);
  ux_ = lx_ + bbox->getDX();
  uy_ = ly_ + bbox->getDY();

  if (isPlaceInstance()) {
    lx_ -= pbc->getPadLeft() * pbc->siteSizeX();
    ux_ += pbc->getPadRight() * pbc->siteSizeX();
  }
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

int64_t Instance::getArea() const
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

int64_t Instance::extendSizeByScale(double scale, utl::Logger* logger)
{
  int64_t old_area = getArea();
  if (old_area == 0) {
    return 0;
  }

  int center_x = cx();
  int center_y = cy();
  int new_dx = static_cast<int>((ux_ - lx_) * scale);
  int new_dy = static_cast<int>((uy_ - ly_) * scale);

  lx_ = center_x - new_dx / 2;
  ux_ = center_x + new_dx / 2;
  ly_ = center_y - new_dy / 2;
  uy_ = center_y + new_dy / 2;

  return getArea() - old_area;
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
  term_ = iTerm;
  updateCoordi(iTerm);
}

Pin::Pin(odb::dbBTerm* bTerm, utl::Logger* logger) : Pin()
{
  setBTerm();
  term_ = bTerm;
  updateCoordi(bTerm, logger);
}

std::string Pin::getName() const
{
  if (!term_) {
    return "DUMMY";
  }
  if (isITerm()) {
    return getDbITerm()->getName();
  }
  return getDbBTerm()->getName();
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

int Pin::getOffsetCx() const
{
  return offsetCx_;
}

int Pin::getOffsetCy() const
{
  return offsetCy_;
}

odb::dbITerm* Pin::getDbITerm() const
{
  return (isITerm()) ? (odb::dbITerm*) term_ : nullptr;
}
odb::dbBTerm* Pin::getDbBTerm() const
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
  Rect bbox = bTerm->getBBox();
  if (bbox.isInverted()) {
    logger->error(
        GPL, 326, "{} toplevel port is not placed.", bTerm->getConstName());
  }

  // Just center
  offsetCx_ = offsetCy_ = 0;

  cx_ = bbox.xCenter();
  cy_ = bbox.yCenter();
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

int64_t Net::getHpwl() const
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

PlacerBaseVars::PlacerBaseVars(const PlaceOptions& options)
    : padLeft(options.padLeft),
      padRight(options.padRight),
      skipIoMode(options.skipIoMode),
      disablePinDensityAdjust(options.disablePinDensityAdjust)
{
}

////////////////////////////////////////////////////////
// PlacerBaseCommon

PlacerBaseCommon::PlacerBaseCommon(odb::dbDatabase* db,
                                   PlacerBaseVars pbVars,
                                   utl::Logger* log)
    : pbVars_(pbVars)
{
  db_ = db;
  log_ = log;
  init();
}

PlacerBaseCommon::~PlacerBaseCommon()
{
  reset();
}

void PlacerBaseCommon::init()
{
  log_->info(GPL, 1, "---- Initialize GPL Main Data Structures");
  log_->info(GPL, 2, "DBU: {}", db_->getTech()->getDbUnitsPerMicron());

  dbBlock* block = db_->getChip()->getBlock();

  // die-core area update
  odb::dbSite* db_site = nullptr;
  for (auto* row : block->getRows()) {
    if (row->getSite()->getClass() != odb::dbSiteClass::PAD) {
      db_site = row->getSite();
      break;
    }
  }
  if (db_site == nullptr) {
    log_->error(GPL, 305, "Unable to find a site");
  }
  odb::Rect core_rect = block->getCoreArea();
  odb::Rect die_rect = block->getDieArea();

  if (!die_rect.contains(core_rect)) {
    log_->error(GPL, 118, "core area outside of die.");
  }

  die_ = Die(die_rect, core_rect);

  // siteSize update
  siteSizeX_ = db_site->getWidth();
  siteSizeY_ = db_site->getHeight();

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
  dbSet<dbInst> db_insts = block->getInsts();
  instStor_.reserve(db_insts.size());
  insts_.reserve(instStor_.size());
  for (dbInst* db_inst : db_insts) {
    auto type = db_inst->getMaster()->getType();
    if (!type.isCore() && !type.isBlock()) {
      continue;
    }

    Instance temp_inst(db_inst, this, log_);
    odb::dbBox* inst_bbox = db_inst->getBBox();
    if (inst_bbox->getDY() > die_.coreDy()) {
      log_->error(GPL,
                  119,
                  "instance {} height is larger than core.",
                  db_inst->getName());
    }
    if (inst_bbox->getDX() > die_.coreDx()) {
      log_->error(GPL,
                  120,
                  "instance {} width is larger than core.",
                  db_inst->getName());
    }

    // Fixed instaces need to be snapped outwards to the nearest site
    // boundary.  A partially overlapped site is unusable and this
    // is the simplest way to ensure it is counted as fully used.
    if (temp_inst.isFixed()) {
      temp_inst.snapOutward(core_rect.ll(), siteSizeX_, siteSizeY_);
    }

    instStor_.push_back(temp_inst);

    if (temp_inst.dy() > siteSizeY_ * 6) {
      macroInstsArea_ += temp_inst.getArea();
    }
  }

  // Extending instances by average pin density.
  auto count_signal_pins = [](const Instance& inst) -> int {
    return std::ranges::count_if(
        inst.dbInst()->getITerms(),
        [](odb::dbITerm* iterm) { return !iterm->getSigType().isSupply(); });
  };

  int total_signal_pins = 0;
  int64_t movable_area = 0;
  int64_t total_area = 0;
  for (const auto& inst : instStor_) {
    if (inst.isInstance()) {
      total_area += inst.getArea();
      if (!inst.isFixed()) {
        total_signal_pins += count_signal_pins(inst);
        movable_area += inst.getArea();
      }
    }
  }

  log_->info(GPL,
             36,
             format_label_um2,
             "Movable instances area:",
             block->dbuAreaToMicrons(movable_area));
  log_->info(GPL,
             37,
             format_label_um2,
             "Total instances area:",
             block->dbuAreaToMicrons(total_area));

  double avg_density
      = (movable_area > 0)
            ? static_cast<double>(total_signal_pins) / movable_area
            : 0.0;
  if (log_->debugCheck(GPL, "extendPinDensity", 1)) {
    double avg_density_micron = block->dbuToMicrons(avg_density);
    double avg_area_per_pin_dbu
        = (total_signal_pins > 0)
              ? static_cast<double>(movable_area) / total_signal_pins
              : 0.0;
    double avg_area_per_pin_micron
        = block->dbuAreaToMicrons(avg_area_per_pin_dbu);
    log_->report("Average pin density: {:.6f} pins per DBU^2", avg_density);
    log_->report("Average pin density: {:.6f} pins per micron^2",
                 avg_density_micron);
    log_->report("Average area per pin: {:.2f} DBU^2 ({:.6f} micron^2)",
                 avg_area_per_pin_dbu,
                 avg_area_per_pin_micron);
  }

  // Adjust each movable instance to match the average density
  int64_t total_adjustment_area = 0;
  if (!pbVars_.disablePinDensityAdjust) {
    for (auto& inst : instStor_) {
      if (!inst.isFixed() && inst.isInstance()) {
        int pin_count = count_signal_pins(inst);
        if (pin_count > 2 && avg_density > 0.0) {
          double target_area = static_cast<double>(pin_count) / avg_density;
          double scale
              = std::sqrt(target_area / static_cast<double>(inst.getArea()));
          // Cap scaling, avoid later excessive routability inflation
          if (scale > 1.2) {
            scale = 1.2;
          } else if (scale < 0.95) {
            scale = 0.95;
          }
          total_adjustment_area += inst.extendSizeByScale(scale, log_);
        }
      }
    }
  }

  log_->info(GPL,
             35,
             format_label_um2,
             "Pin density area adjust:",
             block->dbuAreaToMicrons(total_adjustment_area));

  instMap_.reserve(instStor_.size());
  insts_.reserve(instStor_.size());
  for (auto& pb_inst : instStor_) {
    instMap_[pb_inst.dbInst()] = &pb_inst;
    insts_.push_back(&pb_inst);

    if (!pb_inst.isFixed()) {
      placeInsts_.push_back(&pb_inst);
    }
  }

  // nets fill
  dbSet<dbNet> db_nets = block->getNets();
  netStor_.reserve(db_nets.size());
  for (dbNet* db_net : db_nets) {
    dbSigType net_type = db_net->getSigType();

    // escape nets with VDD/VSS/reset nets
    if (net_type == dbSigType::SIGNAL || net_type == dbSigType::CLOCK) {
      Net temp_net(db_net, pbVars_.skipIoMode);
      netStor_.push_back(temp_net);

      // this is safe because of "reserve"
      Net* temp_net_ptr = &netStor_[netStor_.size() - 1];
      netMap_[db_net] = temp_net_ptr;

      for (dbITerm* iTerm : db_net->getITerms()) {
        Pin temp_pin(iTerm);
        temp_pin.setNet(temp_net_ptr);
        temp_pin.setInstance(dbToPb(iTerm->getInst()));
        pinStor_.push_back(temp_pin);
      }

      if (pbVars_.skipIoMode == false) {
        for (dbBTerm* bTerm : db_net->getBTerms()) {
          Pin temp_pin(bTerm, log_);
          temp_pin.setNet(temp_net_ptr);
          pinStor_.push_back(temp_pin);
        }
      }
    }
  }

  // pinMap_ and pins_ update
  pins_.reserve(pinStor_.size());
  pinMap_.reserve(pinStor_.size());
  for (auto& pb_pin : pinStor_) {
    if (pb_pin.isITerm()) {
      pinMap_[pb_pin.getDbITerm()] = &pb_pin;
    } else if (pb_pin.isBTerm()) {
      pinMap_[pb_pin.getDbBTerm()] = &pb_pin;
    }
    pins_.push_back(&pb_pin);
  }

  // instStor_'s pins_ fill
  for (auto& pb_inst : instStor_) {
    if (!pb_inst.isInstance()) {
      continue;
    }
    for (dbITerm* iTerm : pb_inst.dbInst()->getITerms()) {
      // note that, DB's ITerm can have
      // VDD/VSS pins.
      //
      // Escape those pins
      Pin* cur_pin = dbToPb(iTerm);
      if (cur_pin) {
        pb_inst.addPin(cur_pin);
      }
    }
  }

  // nets' pin update
  nets_.reserve(netStor_.size());
  for (auto& pb_net : netStor_) {
    for (dbITerm* iTerm : pb_net.getDbNet()->getITerms()) {
      pb_net.addPin(dbToPb(iTerm));
    }
    if (pbVars_.skipIoMode == false) {
      for (dbBTerm* bTerm : pb_net.getDbNet()->getBTerms()) {
        pb_net.addPin(dbToPb(bTerm));
      }
    }
    nets_.push_back(&pb_net);
  }
}

void PlacerBaseCommon::reset()
{
  db_ = nullptr;

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

int64_t PlacerBaseCommon::getHpwl() const
{
  int64_t hpwl = 0;
  for (auto& net : nets_) {
    net->updateBox(pbVars_.skipIoMode);
    hpwl += net->getHpwl();
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
  auto pinPtr = pinMap_.find(term);
  return (pinPtr == pinMap_.end()) ? nullptr : pinPtr->second;
}

Pin* PlacerBaseCommon::dbToPb(odb::dbBTerm* term) const
{
  auto pinPtr = pinMap_.find(term);
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
  log_->info(GPL,
             32,
             "---- Initialize Region: {}",
             (group_ == nullptr) ? "Top-level" : group_->getName());
  init();
}

PlacerBase::~PlacerBase()
{
  reset();
}

void PlacerBase::init()
{
  die_ = pbCommon_->getDie();
  if (group_ != nullptr) {
    auto boundaries = group_->getRegion()->getBoundaries();

    if (!boundaries.empty()) {
      region_bbox_.mergeInit();
      for (auto boundary : boundaries) {
        region_bbox_.merge(boundary->getBox());
      }
      region_area_ = region_bbox_.area();
    } else {
      region_bbox_ = odb::Rect(
          die_.coreLx(), die_.coreLy(), die_.coreUx(), die_.coreUy());
      region_area_ = die_.coreArea();
    }
  } else {
    region_bbox_
        = odb::Rect(die_.coreLx(), die_.coreLy(), die_.coreUx(), die_.coreUy());
    region_area_ = die_.coreArea();
  }

  // siteSize update
  siteSizeX_ = pbCommon_->siteSizeX();
  siteSizeY_ = pbCommon_->siteSizeY();

  for (auto& inst : pbCommon_->getInsts()) {
    if (!inst->isInstance()) {
      continue;
    }

    odb::dbInst* db_inst = inst->dbInst();
    if (!db_inst) {
      continue;
    }

    odb::dbGroup* db_inst_group = db_inst->getGroup();
    if (group_ == nullptr) {
      if (db_inst_group
          && db_inst_group->getType() != odb::dbGroupType::VISUAL_DEBUG) {
        continue;
      }
    } else {
      if (!db_inst_group || db_inst_group != group_
          || db_inst_group->getType() == odb::dbGroupType::VISUAL_DEBUG) {
        continue;
      }
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
      int64_t instArea = inst->getArea();
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

    pb_insts_.push_back(inst);
  }

  // insts fill with fake instances (fragmented row or blockage)
  initInstsForUnusableSites();

  // handle newly added dummy instances
  for (auto& inst : instStor_) {
    if (inst.isDummy()) {
      dummyInsts_.push_back(&inst);
      nonPlaceInsts_.push_back(&inst);
      nonPlaceInstsArea_ += inst.getArea();
    }
    pb_insts_.push_back(&inst);
  }

  printInfo();
}

// Use dummy instance to fill unusable sites.  Sites are unusable
// due to fragmented rows or placement blockages.
void PlacerBase::initInstsForUnusableSites()
{
  dbSet<dbRow> rows = db_->getChip()->getBlock()->getRows();

  int64_t siteCountX = (die_.coreUx() - die_.coreLx()) / siteSizeX_;
  int64_t siteCountY = (die_.coreUy() - die_.coreLy()) / siteSizeY_;

  enum SiteInfo
  {
    Blocked,   // site representation of dummy instances
    Row,       // placable site
    FixedInst  // site taken by fixed instance
  };

  //
  // Initialize siteGrid as Row.
  // All sites placeable so we avoid dummies creation due to regions
  //
  std::vector<SiteInfo> siteGrid(siteCountX * siteCountY, SiteInfo::Row);

  // If we have no group (top-level), check which sites are NOT covered by
  // actual rows Create dummy instances for unusable sites (test simple02.tcl
  // has an example on bottom right)
  if (group_ == nullptr) {
    std::ranges::fill(siteGrid, SiteInfo::Blocked);

    // Mark only sites covered by actual rows as Row
    for (dbRow* row : rows) {
      Rect rect = row->getBBox();

      std::pair<int, int> pairX = getMinMaxIdx(
          rect.xMin(), rect.xMax(), die_.coreLx(), siteSizeX_, 0, siteCountX);

      std::pair<int, int> pairY = getMinMaxIdx(
          rect.yMin(), rect.yMax(), die_.coreLy(), siteSizeY_, 0, siteCountY);

      for (int i = pairX.first; i < pairX.second; i++) {
        for (int j = pairY.first; j < pairY.second; j++) {
          siteGrid[(j * siteCountX) + i] = Row;
        }
      }
    }

    // Mark sites intersecting with any region as Blocked
    for (auto* group : db_->getChip()->getBlock()->getGroups()) {
      if (group->getRegion()) {
        auto boundaries = group->getRegion()->getBoundaries();
        for (auto boundary : boundaries) {
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
              siteGrid[(j * siteCountX) + i] = Blocked;
            }
          }
        }
      }
    }
  }

  // Mark blockage areas as Blocked so that their sites will be blocked.
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
          siteGrid[(j * siteCountX) + i] = Blocked;
          debugPrint(log_,
                     GPL,
                     "dummies",
                     1,
                     "Blocking site at ({}, {}) due to blockage.",
                     i,
                     j);
          ++filled;
        }
        ++cells;
      }
    }
  }

  // fill fixed instances' bbox
  for (auto& inst : pbCommon_->getInsts()) {
    if (!inst->isFixed()) {
      continue;
    }
    odb::dbInst* db_inst = inst->dbInst();
    if (!db_inst) {
      continue;
    }

    odb::dbGroup* db_inst_group = db_inst->getGroup();
    if (group_ == nullptr) {
      if (db_inst_group
          && db_inst_group->getType() != odb::dbGroupType::VISUAL_DEBUG) {
        continue;
      }
    } else {
      if (!db_inst_group || db_inst_group != group_
          || db_inst_group->getType() == odb::dbGroupType::VISUAL_DEBUG) {
        continue;
      }
    }

    std::pair<int, int> pairX = getMinMaxIdx(
        inst->lx(), inst->ux(), die_.coreLx(), siteSizeX_, 0, siteCountX);
    std::pair<int, int> pairY = getMinMaxIdx(
        inst->ly(), inst->uy(), die_.coreLy(), siteSizeY_, 0, siteCountY);
    for (int i = pairX.first; i < pairX.second; i++) {
      for (int j = pairY.first; j < pairY.second; j++) {
        siteGrid[(j * siteCountX) + i] = FixedInst;
        debugPrint(log_,
                   GPL,
                   "dummies",
                   1,
                   "Blocking site at ({}, {}) due to fixed instance {}.",
                   i,
                   j,
                   db_inst->getName());
      }
    }
  }

  //
  // Search the "Blocked" coordinates on site-grid
  // --> These sites need to be dummyInstance
  //
  for (int j = 0; j < siteCountY; j++) {
    for (int i = 0; i < siteCountX; i++) {
      if (siteGrid[(j * siteCountX) + i] == Blocked) {
        int startX = i;
        // find end points
        while (i < siteCountX && siteGrid[(j * siteCountX) + i] == Blocked) {
          i++;
        }
        int endX = i;
        Instance dummy_gcell(die_.coreLx() + (siteSizeX_ * startX),
                             die_.coreLy() + (siteSizeY_ * j),
                             die_.coreLx() + (siteSizeX_ * endX),
                             die_.coreLy() + (siteSizeY_ * (j + 1)));
        instStor_.push_back(dummy_gcell);
      }
    }
  }
}

void PlacerBase::reset()
{
  db_ = nullptr;
  instStor_.clear();
  pb_insts_.clear();
  placeInsts_.clear();
  fixedInsts_.clear();
  nonPlaceInsts_.clear();
}

void PlacerBase::printInfo() const
{
  dbBlock* block = db_->getChip()->getBlock();
  log_->info(GPL,
             6,
             format_label_int,
             "Number of instances:",
             placeInsts_.size() + fixedInsts_.size() + dummyInsts_.size());
  log_->info(
      GPL, 7, format_label_int, "Movable instances:", placeInsts_.size());
  log_->info(GPL, 8, format_label_int, "Fixed instances:", fixedInsts_.size());
  log_->info(GPL, 9, format_label_int, "Dummy instances:", dummyInsts_.size());
  log_->info(GPL,
             10,
             format_label_int,
             "Number of nets:",
             pbCommon_->getNets().size());
  log_->info(GPL,
             11,
             format_label_int,
             "Number of pins:",
             pbCommon_->getPins().size());

  log_->info(GPL,
             12,
             "{:10} ( {:6.3f} {:6.3f} ) ( {:6.3f} {:6.3f} ) um",
             "Die BBox:",
             block->dbuToMicrons(die_.dieLx()),
             block->dbuToMicrons(die_.dieLy()),
             block->dbuToMicrons(die_.dieUx()),
             block->dbuToMicrons(die_.dieUy()));
  log_->info(GPL,
             13,
             "{:10} ( {:6.3f} {:6.3f} ) ( {:6.3f} {:6.3f} ) um",
             "Core BBox:",
             block->dbuToMicrons(die_.coreLx()),
             block->dbuToMicrons(die_.coreLy()),
             block->dbuToMicrons(die_.coreUx()),
             block->dbuToMicrons(die_.coreUy()));

  float util = static_cast<float>(placeInstsArea_)
               / (region_area_ - nonPlaceInstsArea_) * 100;

  log_->info(GPL,
             16,
             format_label_um2,
             "Core area:",
             block->dbuAreaToMicrons(die_.coreArea()));
  log_->info(GPL,
             14,
             "Region name: {}.",
             (group_ != nullptr) ? group_->getName() : "top-level");
  log_->info(GPL,
             15,
             format_label_um2,
             "Region area:",
             block->dbuAreaToMicrons(region_area_));
  log_->info(GPL,
             17,
             format_label_um2,
             "Fixed instances area:",
             block->dbuAreaToMicrons(nonPlaceInstsArea_));

  log_->info(GPL,
             18,
             format_label_um2,
             "Movable instances area:",
             block->dbuAreaToMicrons(placeInstsArea_));
  log_->info(GPL, 19, "{:27} {:10.3f} %", "Utilization:", util);

  log_->info(GPL,
             20,
             format_label_um2,
             "Standard cells area:",
             block->dbuAreaToMicrons(stdInstsArea_));

  log_->info(GPL,
             21,
             format_label_um2,
             "Large instances area:",
             block->dbuAreaToMicrons(macroInstsArea_));

  if (util >= 100.1) {
    log_->error(GPL, 301, "Utilization {:.3f} % exceeds 100%.", util);
  }
}

void PlacerBase::unlockAll()
{
  for (auto inst : pb_insts_) {
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
