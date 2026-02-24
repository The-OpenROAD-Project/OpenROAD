// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "nesterovBase.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "fft.h"
#include "gpl/Replace.h"
#include "nesterovPlace.h"
#include "odb/db.h"
#include "omp.h"
#include "placerBase.h"
#include "point.h"
#include "utl/Logger.h"

#define REPLACE_SQRT2 1.414213562373095048801L

namespace gpl {

using odb::dbBlock;
using utl::GPL;

static float calculateBiVariateNormalCDF(biNormalParameters i);

static int64_t getOverlapArea(const Bin* bin,
                              const Instance* inst,
                              int dbu_per_micron);

static int64_t getOverlapAreaUnscaled(const Bin* bin, const Instance* inst);

static float getDistance(const std::vector<FloatPoint>& a,
                         const std::vector<FloatPoint>& b);

static float getSecondNorm(const std::vector<FloatPoint>& a);

// Note that
// int64_t is ideal in the following function, but
// runtime is doubled compared with float.
//
// Choose to use "float" only in the following functions
static float getOverlapDensityArea(const Bin& bin, const GCell* cell);

static float fastExp(float exp);

////////////////////////////////////////////////
// GCell

GCell::GCell(Instance* inst) : GCell(std::vector<Instance*>{inst})
{
}

GCell::GCell(const std::vector<Instance*>& insts)
{
  insts_ = insts;
  updateLocations();
}

GCell::GCell(const int cx, const int cy, const int dx, const int dy)
{
  dLx_ = lx_ = cx - dx / 2;
  dLy_ = ly_ = cy - dy / 2;
  dUx_ = ux_ = cx + dx / 2;
  dUy_ = uy_ = cy + dy / 2;
}

bool GCell::isLocked() const
{
  return std::any_of(insts_.begin(), insts_.end(), [](Instance* inst) {
    return inst->isLocked();
  });
}

void GCell::lock()
{
  for (Instance* inst : insts_) {
    inst->lock();
  }
}

std::string GCell::getName() const
{
  if (insts_.empty()) {
    return "fill";
  }
  std::string name = insts_[0]->dbInst()->getConstName();
  if (insts_.size() > 1) {
    name += "-(Cluster)";
  }
  return name;
}

void GCell::setAllLocations(int lx, int ly, int ux, int uy)
{
  dLx_ = lx_ = lx;
  dLy_ = ly_ = ly;
  dUx_ = ux_ = ux;
  dUy_ = uy_ = uy;
}

void GCell::addGPin(GPin* gPin)
{
  gPins_.push_back(gPin);
}

void GCell::updateLocations()
{
  odb::Rect bbox;
  if (insts_.size() == 1) {
    Instance* inst = insts_[0];
    bbox.init(inst->lx(), inst->ly(), inst->ux(), inst->uy());
  } else {
    bbox.mergeInit();
    int64_t inst_area = 0;
    for (Instance* inst : insts_) {
      inst_area += inst->getArea();
      bbox.merge({inst->lx(), inst->ly(), inst->ux(), inst->uy()});
    }
    odb::Rect core_area = insts_[0]->dbInst()->getBlock()->getCoreArea();
    const int center_x = bbox.xCenter();
    const int center_y = bbox.yCenter();
    const double aspect_ratio = core_area.dx() / (double) core_area.dy();
    const double height = std::sqrt(inst_area / aspect_ratio);
    const double width = height * aspect_ratio;
    bbox.init(center_x - (width / 2),
              center_y - (height / 2),
              center_x + (width / 2),
              center_y + (height / 2));
  }
  // density coordi has the same center points.
  dLx_ = lx_ = bbox.xMin();
  dLy_ = ly_ = bbox.yMin();
  dUx_ = ux_ = bbox.xMax();
  dUy_ = uy_ = bbox.yMax();
}

void GCell::setCenterLocation(int cx, int cy)
{
  const int halfDx = dx() / 2;
  const int halfDy = dy() / 2;

  lx_ = cx - halfDx;
  ly_ = cy - halfDy;
  ux_ = cx + halfDx;
  uy_ = cy + halfDy;

  for (auto& gPin : gPins_) {
    gPin->updateLocation(this);
  }
}

// changing size and preserve center coordinates
void GCell::setSize(int dx, int dy, GCellChange change)
{
  const int centerX = cx();
  const int centerY = cy();

  lx_ = centerX - dx / 2;
  ly_ = centerY - dy / 2;
  ux_ = centerX + dx / 2;
  uy_ = centerY + dy / 2;

  change_ = change;
}

// Used for initialization
void GCell::setDensityLocation(int dLx, int dLy)
{
  dUx_ = dLx + (dUx_ - dLx_);
  dUy_ = dLy + (dUy_ - dLy_);
  dLx_ = dLx;
  dLy_ = dLy;

  // assume that density Center change the gPin coordi
  for (auto& gPin : gPins_) {
    gPin->updateDensityLocation(this);
  }
}

// Used for updating density locations
void GCell::setDensityCenterLocation(int dCx, int dCy)
{
  const int halfDDx = dDx() / 2;
  const int halfDDy = dDy() / 2;

  dLx_ = dCx - halfDDx;
  dLy_ = dCy - halfDDy;
  dUx_ = dCx + halfDDx;
  dUy_ = dCy + halfDDy;

  // assume that density Center change the gPin coordi
  for (auto& gPin : gPins_) {
    gPin->updateDensityLocation(this);
  }
}

// changing size and preserve center coordinates
void GCell::setDensitySize(int dDx, int dDy)
{
  const int dCenterX = dCx();
  const int dCenterY = dCy();

  dLx_ = dCenterX - dDx / 2;
  dLy_ = dCenterY - dDy / 2;
  dUx_ = dCenterX + dDx / 2;
  dUy_ = dCenterY + dDy / 2;
}

void GCell::setDensityScale(float densityScale)
{
  densityScale_ = densityScale;
}

void GCell::setGradientX(float gradientX)
{
  gradientX_ = gradientX;
}

void GCell::setGradientY(float gradientY)
{
  gradientY_ = gradientY;
}

bool GCell::contains(odb::dbInst* db_inst) const
{
  return std::any_of(insts_.begin(), insts_.end(), [=](Instance* inst) {
    return inst->dbInst() == db_inst;
  });
}

bool GCell::isInstance() const
{
  return !insts_.empty();
}

bool GCell::isFiller() const
{
  return insts_.empty();
}

bool GCell::isMacroInstance() const
{
  if (!isInstance()) {
    return false;
  }
  return insts_[0]->isMacro();
}

bool GCell::isStdInstance() const
{
  if (!isInstance()) {
    return false;
  }
  return !insts_[0]->isMacro();
}

void GCell::print(utl::Logger* logger, bool print_only_name = true) const
{
  if (!insts_.empty()) {
    logger->report("print gcell:{}", insts_[0]->dbInst()->getName());
  } else {
    logger->report("print gcell insts_ empty! (filler cell)");
  }

  if (!print_only_name) {
    logger->report(
        "insts_ size: {}, gPins_ size: {}", insts_.size(), gPins_.size());
    logger->report("lx_: {} ly_: {} ux_: {} uy_: {}", lx_, ly_, ux_, uy_);
    logger->report(
        "dLx_: {} dLy_: {} dUx_: {} dUy_: {}", dLx_, dLy_, dUx_, dUy_);
    logger->report("densityScale_: {} gradientX_: {} gradientY_: {}",
                   densityScale_,
                   gradientX_,
                   gradientY_);
  }
}

void GCell::writeAttributesToCSV(std::ostream& out) const
{
  out << "," << insts_.size() << "," << gPins_.size();
  out << "," << lx_ << "," << ly_ << "," << ux_ << "," << uy_;
  out << "," << dLx_ << "," << dLy_ << "," << dUx_ << "," << dUy_;
  out << "," << densityScale_ << "," << gradientX_ << "," << gradientY_;
}

////////////////////////////////////////////////
// GNet

GNet::GNet(Net* net)
{
  nets_.push_back(net);
}

GNet::GNet(const std::vector<Net*>& nets)
{
  nets_ = nets;
}

Net* GNet::getPbNet() const
{
  return *nets_.begin();
}

void GNet::setTimingWeight(float timingWeight)
{
  timingWeight_ = timingWeight;
}

void GNet::setCustomWeight(float customWeight)
{
  customWeight_ = customWeight;
}

void GNet::addGPin(GPin* gPin)
{
  gPins_.push_back(gPin);
}

void GNet::updateBox()
{
  lx_ = ly_ = INT_MAX;
  ux_ = uy_ = INT_MIN;

  for (auto& gPin : gPins_) {
    lx_ = std::min(gPin->cx(), lx_);
    ly_ = std::min(gPin->cy(), ly_);
    ux_ = std::max(gPin->cx(), ux_);
    uy_ = std::max(gPin->cy(), uy_);
  }
}

int64_t GNet::getHpwl() const
{
  if (ux_ < lx_) {  // dangling net
    return 0;
  }
  int64_t lx = lx_;
  int64_t ly = ly_;
  int64_t ux = ux_;
  int64_t uy = uy_;
  return (ux - lx) + (uy - ly);
}

void GNet::clearWaVars()
{
  waExpMinSumX_ = 0;
  waXExpMinSumX_ = 0;

  waExpMaxSumX_ = 0;
  waXExpMaxSumX_ = 0;

  waExpMinSumY_ = 0;
  waYExpMinSumY_ = 0;

  waExpMaxSumY_ = 0;
  waYExpMaxSumY_ = 0;
}

void GNet::setDontCare()
{
  isDontCare_ = true;
}

bool GNet::isDontCare() const
{
  return gPins_.empty() || isDontCare_;
}

void GNet::print(utl::Logger* log) const
{
  log->report("print net: {}", nets_[0]->getDbNet()->getName());
  log->report("gPins_ size: {}", gPins_.size());
  log->report("nets_ size: {}", nets_.size());
  // log->report("gpl_net_: {}", pb_net->);
  log->report("lx_: {}, ly_: {}, ux_: {}, uy_: {}", lx_, ly_, ux_, uy_);
  log->report("timingWeight_: {}", timingWeight_);
  log->report("customWeight_: {}", customWeight_);
  log->report(
      "waExpMinSumX_: {}, waXExpMinSumX_: {}", waExpMinSumX_, waXExpMinSumX_);
  log->report(
      "waExpMaxSumX_: {}, waXExpMaxSumX_: {}", waExpMaxSumX_, waXExpMaxSumX_);
  log->report(
      "waExpMinSumY_: {}, waYExpMinSumY_: {}", waExpMinSumY_, waYExpMinSumY_);
  log->report(
      "waExpMaxSumY_: {}, waYExpMaxSumY_: {}", waExpMaxSumY_, waYExpMaxSumY_);
  log->report("isDontCare_: {}", isDontCare_ ? "true" : "false");
}

////////////////////////////////////////////////
// GPin

GPin::GPin(Pin* pin)
{
  pins_.push_back(pin);
  cx_ = pin->cx();
  cy_ = pin->cy();
  offsetCx_ = pin->getOffsetCx();
  offsetCy_ = pin->getOffsetCy();
}

GPin::GPin(const std::vector<Pin*>& pins)
{
  pins_ = pins;
}

Pin* GPin::getPbPin() const
{
  return *pins_.begin();
}

void GPin::setGCell(GCell* gCell)
{
  gCell_ = gCell;
}

void GPin::setGNet(GNet* gNet)
{
  gNet_ = gNet;
}

void GPin::setCenterLocation(int cx, int cy)
{
  cx_ = cx;
  cy_ = cy;
}

void GPin::clearWaVars()
{
  hasMaxExpSumX_ = false;
  hasMaxExpSumY_ = false;
  hasMinExpSumX_ = false;
  hasMinExpSumY_ = false;

  maxExpSumX_ = maxExpSumY_ = 0;
  minExpSumX_ = minExpSumY_ = 0;
}

void GPin::setMaxExpSumX(float maxExpSumX)
{
  hasMaxExpSumX_ = true;
  maxExpSumX_ = maxExpSumX;
}

void GPin::setMaxExpSumY(float maxExpSumY)
{
  hasMaxExpSumY_ = true;
  maxExpSumY_ = maxExpSumY;
}

void GPin::setMinExpSumX(float minExpSumX)
{
  hasMinExpSumX_ = true;
  minExpSumX_ = minExpSumX;
}

void GPin::setMinExpSumY(float minExpSumY)
{
  hasMinExpSumY_ = true;
  minExpSumY_ = minExpSumY;
}

void GPin::updateLocation(const GCell* gCell)
{
  cx_ = gCell->cx() + offsetCx_;
  cy_ = gCell->cy() + offsetCy_;
}

void GPin::updateDensityLocation(const GCell* gCell)
{
  cx_ = gCell->dCx() + offsetCx_;
  cy_ = gCell->dCy() + offsetCy_;
}

void GPin::updateCoordi()
{
  Pin* pb_pin = pins_[0];
  cx_ = pb_pin->cx();
  cy_ = pb_pin->cy();
  offsetCx_ = pb_pin->getOffsetCx();
  offsetCy_ = pb_pin->getOffsetCy();
}

void GPin::print(utl::Logger* log) const
{
  if (getPbPin()->getDbITerm() != nullptr) {
    log->report("--> print pin: {}", getPbPin()->getDbITerm()->getName());
  } else {
    log->report("pin()->dbIterm() is nullptr!");
  }
  if (gCell_) {
    if (gCell_->isInstance()) {
      log->report("GCell*: {}", gCell_->getName());
    } else {
      log->report("GCell of gpin is filler!");
    }
  } else {
    log->report("gcell of gpin is null");
  }
  log->report("GNet: {}", gNet_->getPbNet()->getDbNet()->getName());
  log->report("pins_.size(): {}", pins_.size());
  log->report("offsetCx_: {}", offsetCx_);
  log->report("offsetCy_: {}", offsetCy_);
  log->report("cx_: {}", cx_);
  log->report("cy_: {}", cy_);
  log->report("maxExpSumX_: {}", maxExpSumX_);
  log->report("maxExpSumY_: {}", maxExpSumY_);
  log->report("minExpSumX_: {}", minExpSumX_);
  log->report("minExpSumY_: {}", minExpSumY_);
  log->report("hasMaxExpSumX_: {}", hasMaxExpSumX_);
  log->report("hasMaxExpSumY_: {}", hasMaxExpSumY_);
  log->report("hasMinExpSumX_: {}", hasMinExpSumX_);
  log->report("hasMinExpSumY_: {}", hasMinExpSumY_);
}

////////////////////////////////////////////////////////
// Bin

Bin::Bin(int x, int y, int lx, int ly, int ux, int uy, float targetDensity)
{
  x_ = x;
  y_ = y;
  lx_ = lx;
  ly_ = ly;
  ux_ = ux;
  uy_ = uy;
  targetDensity_ = targetDensity;
}

int64_t Bin::getBinArea() const
{
  return static_cast<int64_t>(dx()) * static_cast<int64_t>(dy());
}

float Bin::getDensity() const
{
  return density_;
}

float Bin::getTargetDensity() const
{
  return targetDensity_;
}

float Bin::electroForceX() const
{
  return electroForceX_;
}

float Bin::electroForceY() const
{
  return electroForceY_;
}

float Bin::electroPhi() const
{
  return electroPhi_;
}

void Bin::setDensity(float density)
{
  density_ = density;
}

void Bin::setBinTargetDensity(float density)
{
  targetDensity_ = density;
}

void Bin::setElectroForce(float electroForceX, float electroForceY)
{
  electroForceX_ = electroForceX;
  electroForceY_ = electroForceY;
}

void Bin::setElectroPhi(float phi)
{
  electroPhi_ = phi;
}

////////////////////////////////////////////////
// BinGrid

BinGrid::BinGrid(int lx, int ly, int ux, int uy)
{
  setRegionPoints(lx, ly, ux, uy);
}

void BinGrid::setRegionPoints(int lx, int ly, int ux, int uy)
{
  lx_ = lx;
  ly_ = ly;
  ux_ = ux;
  uy_ = uy;
}

// NOLINTNEXTLINE(performance-unnecessary-value-param)
void BinGrid::setPlacerBase(std::shared_ptr<PlacerBase> pb)
{
  pb_ = std::move(pb);
}

void BinGrid::setLogger(utl::Logger* log)
{
  log_ = log;
}

void BinGrid::setBinTargetDensity(float density)
{
  targetDensity_ = density;
}

void BinGrid::setBinCnt(int binCntX, int binCntY)
{
  isSetBinCnt_ = true;
  binCntX_ = binCntX;
  binCntY_ = binCntY;
}

int BinGrid::lx() const
{
  return lx_;
}
int BinGrid::ly() const
{
  return ly_;
}

int BinGrid::ux() const
{
  return ux_;
}

int BinGrid::uy() const
{
  return uy_;
}

int BinGrid::cx() const
{
  return (ux_ + lx_) / 2;
}

int BinGrid::cy() const
{
  return (uy_ + ly_) / 2;
}

int BinGrid::dx() const
{
  return (ux_ - lx_);
}
int BinGrid::dy() const
{
  return (uy_ - ly_);
}
int BinGrid::getBinCntX() const
{
  return binCntX_;
}

int BinGrid::getBinCntY() const
{
  return binCntY_;
}

double BinGrid::getBinSizeX() const
{
  return binSizeX_;
}

double BinGrid::getBinSizeY() const
{
  return binSizeY_;
}

int64_t BinGrid::getOverflowArea() const
{
  return sumOverflowArea_;
}

int64_t BinGrid::getOverflowAreaUnscaled() const
{
  return sumOverflowAreaUnscaled_;
}

static unsigned int roundDownToPowerOfTwo(unsigned int x)
{
  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);
  return x ^ (x >> 1);
}

void BinGrid::initBins()
{
  assert(omp_get_thread_num() == 0);
  int64_t totalBinArea
      = static_cast<int64_t>(ux_ - lx_) * static_cast<int64_t>(uy_ - ly_);

  int64_t averagePlaceInstArea = 0;
  if (!pb_->placeInsts().empty()) {
    averagePlaceInstArea = pb_->placeInstsArea() / pb_->placeInsts().size();
  } else {
    log_->warn(GPL, 306, "GPL component has no placed instances.");
  }

  int64_t idealBinArea = 0;
  if (targetDensity_ != 0) {
    idealBinArea
        = std::round(static_cast<float>(averagePlaceInstArea) / targetDensity_);
  }

  int idealBinCnt = 0;
  if (idealBinArea != 0) {
    idealBinCnt = totalBinArea / idealBinArea;
  }
  idealBinCnt = std::max(idealBinCnt, 4);

  dbBlock* block = pb_->db()->getChip()->getBlock();
  log_->info(
      GPL, 23, format_label_float, "Placement target density:", targetDensity_);
  log_->info(GPL,
             24,
             format_label_um2,
             "Movable insts average area:",
             block->dbuAreaToMicrons(averagePlaceInstArea));
  log_->info(GPL,
             25,
             format_label_um2,
             "Ideal bin area:",
             block->dbuAreaToMicrons(idealBinArea));
  log_->info(GPL, 26, format_label_int, "Ideal bin count:", idealBinCnt);
  log_->info(GPL,
             27,
             format_label_um2,
             "Total bin area:",
             block->dbuAreaToMicrons(totalBinArea));

  if (!isSetBinCnt_) {
    // Consider the apect ratio of the block when computing the number
    // of bins so that the bins remain relatively square.
    const int width = ux_ - lx_;
    const int height = uy_ - ly_;
    const int ratio = roundDownToPowerOfTwo(std::max(width, height)
                                            / std::min(width, height));

    int foundBinCnt = 2;
    // find binCnt: 2, 4, 8, 16, 32, 64, ...
    // s.t. #bins(binCnt) <= idealBinCnt <= #bins(binCnt*2).
    for (foundBinCnt = 2; foundBinCnt <= 1024; foundBinCnt *= 2) {
      if ((foundBinCnt == 2
           || foundBinCnt * (foundBinCnt * ratio) <= idealBinCnt)
          && 4 * foundBinCnt * (foundBinCnt * ratio) > idealBinCnt) {
        break;
      }
    }

    if (width > height) {
      binCntX_ = foundBinCnt * ratio;
      binCntY_ = foundBinCnt;
    } else {
      binCntX_ = foundBinCnt;
      binCntY_ = foundBinCnt * ratio;
    }
  }

  log_->info(
      GPL, 28, "{:21} {:7d} , {:6d}", "Bin count (X, Y):", binCntX_, binCntY_);

  binSizeX_ = static_cast<double>((ux_ - lx_)) / binCntX_;
  binSizeY_ = static_cast<double>((uy_ - ly_)) / binCntY_;

  log_->info(GPL,
             29,
             "{:21} {:7.3f} * {:6.3f} um",
             "Bin size (W * H):",
             block->dbuToMicrons(binSizeX_),
             block->dbuToMicrons(binSizeY_));

  // initialize bins_ vector
  bins_.resize(binCntX_ * (size_t) binCntY_);
#pragma omp parallel for num_threads(num_threads_)
  for (int idxY = 0; idxY < binCntY_; ++idxY) {
    for (int idxX = 0; idxX < binCntX_; ++idxX) {
      const int bin_lx = lx_ + std::lround(idxX * binSizeX_);
      const int bin_ly = ly_ + std::lround(idxY * binSizeY_);
      const int bin_ux = lx_ + std::lround((idxX + 1) * binSizeX_);
      const int bin_uy = ly_ + std::lround((idxY + 1) * binSizeY_);
      const int bin_index = (idxY * binCntX_) + idxX;
      bins_[bin_index]
          = Bin(idxX, idxY, bin_lx, bin_ly, bin_ux, bin_uy, targetDensity_);
      auto& bin = bins_[bin_index];
      if (bin.dx() < 0 || bin.dy() < 0) {
        log_->warn(GPL,
                   34,
                   "Bin (center: {},{}, index: {}) has negative size: {}, {}",
                   bin.cx(),
                   bin.cy(),
                   bin_index,
                   bin.dx(),
                   bin.dy());
      }
    }
  }

  log_->info(GPL, 30, format_label_int, "Number of bins:", bins_.size());

  // only initialized once
  updateBinsNonPlaceArea();
}

void BinGrid::updateBinsNonPlaceArea()
{
  for (auto& bin : bins_) {
    bin.setNonPlaceArea(0);
    bin.setNonPlaceAreaUnscaled(0);
  }

  for (auto& inst : pb_->nonPlaceInsts()) {
    std::pair<int, int> pairX = getMinMaxIdxX(inst);
    std::pair<int, int> pairY = getMinMaxIdxY(inst);
    for (int y = pairY.first; y < pairY.second; y++) {
      for (int x = pairX.first; x < pairX.second; x++) {
        Bin& bin = bins_[y * binCntX_ + x];

        // Note that nonPlaceArea should have scale-down with
        // target density.
        // See MS-replace paper
        //
        bin.addNonPlaceArea(
            getOverlapArea(
                &bin,
                inst,
                pb_->db()->getChip()->getBlock()->getDbUnitsPerMicron())
            * bin.getTargetDensity());
        bin.addNonPlaceAreaUnscaled(getOverlapAreaUnscaled(&bin, inst)
                                    * bin.getTargetDensity());
      }
    }
  }
}

// Core Part
void BinGrid::updateBinsGCellDensityArea(const std::vector<GCellHandle>& cells)
{
  // clear the Bin-area info
  for (Bin& bin : bins_) {
    bin.setInstPlacedAreaUnscaled(0);
    bin.setFillerArea(0);
  }

  for (auto& cell : cells) {
    std::pair<int, int> pairX = getDensityMinMaxIdxX(cell);
    std::pair<int, int> pairY = getDensityMinMaxIdxY(cell);

    // The following function is critical runtime hotspot
    // for global placer.
    //
    if (cell->isInstance()) {
      // macro should have
      // scale-down with target-density
      if (cell->isMacroInstance()) {
        for (int y = pairY.first; y < pairY.second; y++) {
          for (int x = pairX.first; x < pairX.second; x++) {
            Bin& bin = bins_[y * binCntX_ + x];

            const float scaledAvea = getOverlapDensityArea(bin, cell)
                                     * cell->getDensityScale()
                                     * bin.getTargetDensity();
            bin.addInstPlacedAreaUnscaled(scaledAvea);
          }
        }
      }
      // normal cells
      else if (cell->isStdInstance()) {
        for (int y = pairY.first; y < pairY.second; y++) {
          for (int x = pairX.first; x < pairX.second; x++) {
            Bin& bin = bins_[y * binCntX_ + x];
            const float scaledArea
                = getOverlapDensityArea(bin, cell) * cell->getDensityScale();
            bin.addInstPlacedAreaUnscaled(scaledArea);
          }
        }
      }
    } else if (cell->isFiller()) {
      for (int y = pairY.first; y < pairY.second; y++) {
        for (int x = pairX.first; x < pairX.second; x++) {
          Bin& bin = bins_[y * binCntX_ + x];
          bin.addFillerArea(getOverlapDensityArea(bin, cell)
                            * cell->getDensityScale());
        }
      }
    }
  }

  odb::dbBlock* block = pb_->db()->getChip()->getBlock();
  sumOverflowArea_ = 0;
  sumOverflowAreaUnscaled_ = 0;
  // update density and overflowArea
  // for nesterov use and FFT library
#pragma omp parallel for num_threads(num_threads_) \
    reduction(+ : sumOverflowArea_, sumOverflowAreaUnscaled_)
  for (auto it = bins_.begin(); it < bins_.end(); ++it) {
    Bin& bin = *it;  // old-style loop for old OpenMP

    // Copy unscaled to scaled
    bin.setInstPlacedArea(bin.getInstPlacedAreaUnscaled());

    int64_t binArea = bin.getBinArea();
    const float scaledBinArea
        = static_cast<float>(binArea * bin.getTargetDensity());
    bin.setDensity((static_cast<float>(bin.instPlacedArea())
                    + static_cast<float>(bin.getFillerArea())
                    + static_cast<float>(bin.getNonPlaceArea()))
                   / scaledBinArea);

    const float overflowArea = std::max(
        0.0f,
        static_cast<float>(bin.instPlacedArea())
            + static_cast<float>(bin.getNonPlaceArea()) - scaledBinArea);
    sumOverflowArea_ += overflowArea;  // NOLINT

    const float overflowAreaUnscaled
        = std::max(0.0f,
                   static_cast<float>(bin.getInstPlacedAreaUnscaled())
                       + static_cast<float>(bin.getNonPlaceAreaUnscaled())
                       - scaledBinArea);
    sumOverflowAreaUnscaled_ += overflowAreaUnscaled;
    if (overflowAreaUnscaled > 0) {
      debugPrint(log_,
                 GPL,
                 "overflow",
                 1,
                 "overflow:{}, bin:{},{}",
                 block->dbuAreaToMicrons(overflowAreaUnscaled),
                 block->dbuToMicrons(bin.lx()),
                 block->dbuToMicrons(bin.ly()));
      debugPrint(log_,
                 GPL,
                 "overflow",
                 1,
                 "binArea:{}, scaledBinArea:{}",
                 block->dbuAreaToMicrons(binArea),
                 block->dbuAreaToMicrons(scaledBinArea));
      debugPrint(
          log_,
          GPL,
          "overflow",
          1,
          "bin.instPlacedAreaUnscaled():{}, bin.nonPlaceAreaUnscaled():{}",
          block->dbuAreaToMicrons(bin.getInstPlacedAreaUnscaled()),
          block->dbuAreaToMicrons(bin.getNonPlaceAreaUnscaled()));
    }
  }
}

std::pair<int, int> BinGrid::getDensityMinMaxIdxX(const GCell* gcell) const
{
  int lowerIdx = (gcell->dLx() - lx()) / binSizeX_;
  int upperIdx = std::ceil((gcell->dUx() - lx()) / binSizeX_);

  lowerIdx = std::max(lowerIdx, 0);
  upperIdx = std::min(upperIdx, binCntX_);
  return std::make_pair(lowerIdx, upperIdx);
}

std::pair<int, int> BinGrid::getDensityMinMaxIdxY(const GCell* gcell) const
{
  int lowerIdx = (gcell->dLy() - ly()) / binSizeY_;
  int upperIdx = std::ceil((gcell->dUy() - ly()) / binSizeY_);

  lowerIdx = std::max(lowerIdx, 0);
  upperIdx = std::min(upperIdx, binCntY_);
  return std::make_pair(lowerIdx, upperIdx);
}

std::pair<int, int> BinGrid::getMinMaxIdxX(const Instance* inst) const
{
  int lowerIdx = (inst->lx() - lx()) / binSizeX_;
  int upperIdx = std::ceil((inst->ux() - lx()) / binSizeX_);

  return std::make_pair(std::max(lowerIdx, 0), std::min(upperIdx, binCntX_));
}

std::pair<int, int> BinGrid::getMinMaxIdxY(const Instance* inst) const
{
  int lowerIdx = (inst->ly() - ly()) / binSizeY_;
  int upperIdx = std::ceil((inst->uy() - ly()) / binSizeY_);

  return std::make_pair(std::max(lowerIdx, 0), std::min(upperIdx, binCntY_));
}

////////////////////////////////////////////////
// NesterovBaseVars
NesterovBaseVars::NesterovBaseVars(const PlaceOptions& options)
    : isSetBinCnt(options.binGridCntX != 0 && options.binGridCntY != 0),
      useUniformTargetDensity(options.uniformTargetDensityMode),
      targetDensity(options.density),
      binCntX(isSetBinCnt ? options.binGridCntX : 0),
      binCntY(isSetBinCnt ? options.binGridCntY : 0),
      minPhiCoef(options.minPhiCoef),
      maxPhiCoef(options.maxPhiCoef)
{
}

////////////////////////////////////////////////
// NesterovPlaceVars
NesterovPlaceVars::NesterovPlaceVars(const PlaceOptions& options)
    : maxNesterovIter(options.nesterovPlaceMaxIter),
      initDensityPenalty(options.initDensityPenaltyFactor),
      initWireLengthCoef(options.initWireLengthCoef),
      targetOverflow(options.overflow),
      referenceHpwl(options.referenceHpwl),
      routability_end_overflow(options.routabilityCheckOverflow),
      routability_snapshot_overflow(options.routabilitySnapshotOverflow),
      keepResizeBelowOverflow(options.keepResizeBelowOverflow),
      timingDrivenMode(options.timingDrivenMode),
      routability_driven_mode(options.routabilityDrivenMode),
      disableRevertIfDiverge(options.disableRevertIfDiverge)
{
}

////////////////////////////////////////////////
// NesterovBaseCommon
///////////////////////////////////////////////

NesterovBaseCommon::NesterovBaseCommon(
    NesterovBaseVars nbVars,
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    std::shared_ptr<PlacerBaseCommon> pbc,
    utl::Logger* log,
    int num_threads,
    const Clusters& clusters)
    : nbVars_(nbVars), num_threads_{num_threads}
{
  assert(omp_get_thread_num() == 0);
  pbc_ = std::move(pbc);
  log_ = log;
  delta_area_ = 0;
  new_gcells_count_ = 0;
  deleted_gcells_count_ = 0;

  // gCellStor init
  gCellStor_.reserve(pbc_->placeInsts().size());

  std::unordered_set<Instance*> in_cluster;
  for (const Cluster& cluster : clusters) {
    std::vector<Instance*> insts;
    for (odb::dbInst* db_inst : cluster) {
      Instance* inst = pbc_->dbToPb(db_inst);
      in_cluster.insert(inst);
      insts.emplace_back(inst);
    }
    gCellStor_.emplace_back(insts);
  }

  for (Instance* inst : pbc_->placeInsts()) {
    if (in_cluster.find(inst) == in_cluster.end()) {
      gCellStor_.emplace_back(inst);
    }
  }

  // Instance extension from pin density done in placerBase construction
  if (log_->debugCheck(GPL, "extendPinDensity", 1)) {
    reportInstanceExtensionByPinDensity();
  }

  // TODO:
  // at this moment, GNet and GPin is equal to
  // Net and Pin

  // gPinStor init
  gPinStor_.reserve(pbc_->getPins().size());
  for (auto& pin : pbc_->getPins()) {
    GPin myGPin(pin);
    gPinStor_.push_back(myGPin);
  }

  // gNetStor init
  gNetStor_.reserve(pbc_->getNets().size());
  for (auto& net : pbc_->getNets()) {
    GNet myGNet(net);
    gNetStor_.push_back(myGNet);
  }

  // gCell ptr init
  nbc_gcells_.reserve(gCellStor_.size());
  for (auto& gCell : gCellStor_) {
    if (!gCell.isInstance()) {
      continue;
    }
    nbc_gcells_.push_back(&gCell);
    for (Instance* inst : gCell.insts()) {
      gCellMap_[inst] = &gCell;
      db_inst_to_nbc_index_map_[inst->dbInst()] = &gCell - &gCellStor_[0];
    }
  }

  // gPin ptr init
  gPinMap_.reserve(gPinStor_.size());
  db_iterm_to_index_map_.reserve(gPinStor_.size());
  gPins_.reserve(gPinStor_.size());
  for (size_t i = 0; i < gPinStor_.size(); ++i) {
    GPin& gPin = gPinStor_[i];
    gPins_.push_back(&gPin);
    gPinMap_[gPin.getPbPin()] = &gPin;
    if (gPin.getPbPin()->isITerm()) {
      db_iterm_to_index_map_[gPin.getPbPin()->getDbITerm()] = i;
    } else if (gPin.getPbPin()->isBTerm()) {
      db_bterm_to_index_map_[gPin.getPbPin()->getDbBTerm()] = i;
    } else {
      debugPrint(log_, GPL, "callbacks", 1, "gPin neither bterm or iterm!");
    }
  }

  // gNet ptr init
  gNets_.reserve(gNetStor_.size());
  gNetMap_.reserve(gNetStor_.size());
  db_net_to_index_map_.reserve(gNetStor_.size());
  for (size_t i = 0; i < gNetStor_.size(); ++i) {
    GNet& gNet = gNetStor_[i];
    gNets_.push_back(&gNet);
    gNetMap_[gNet.getPbNet()] = &gNet;
    db_net_to_index_map_[gNet.getPbNet()->getDbNet()] = i;
  }

  // gCellStor_'s pins_ fill
#pragma omp parallel for num_threads(num_threads_)
  for (auto it = gCellStor_.begin(); it < gCellStor_.end(); ++it) {
    auto& gCell = *it;  // old-style loop for old OpenMP

    if (gCell.isFiller()) {
      continue;
    }

    for (Instance* inst : gCell.insts()) {
      for (auto& pin : inst->getPins()) {
        gCell.addGPin(pbToNb(pin));
      }
    }
  }

  // gPinStor_' GNet and GCell fill
#pragma omp parallel for num_threads(num_threads_)
  for (auto it = gPinStor_.begin(); it < gPinStor_.end(); ++it) {
    auto& gPin = *it;  // old-style loop for old OpenMP

    gPin.setGCell(pbToNb(gPin.getPbPin()->getInstance()));
    gPin.setGNet(pbToNb(gPin.getPbPin()->getNet()));
  }

  // gNetStor_'s GPin fill
#pragma omp parallel for num_threads(num_threads_)
  for (auto it = gNetStor_.begin(); it < gNetStor_.end(); ++it) {
    auto& gNet = *it;  // old-style loop for old OpenMP

    for (auto& pin : gNet.getPbNet()->getPins()) {
      gNet.addGPin(pbToNb(pin));
    }
  }
}

GCell* NesterovBaseCommon::pbToNb(Instance* inst) const
{
  auto gcPtr = gCellMap_.find(inst);
  return (gcPtr == gCellMap_.end()) ? nullptr : gcPtr->second;
}

GPin* NesterovBaseCommon::pbToNb(Pin* pin) const
{
  auto gpPtr = gPinMap_.find(pin);
  return (gpPtr == gPinMap_.end()) ? nullptr : gpPtr->second;
}

GNet* NesterovBaseCommon::pbToNb(Net* net) const
{
  auto gnPtr = gNetMap_.find(net);
  return (gnPtr == gNetMap_.end()) ? nullptr : gnPtr->second;
}

GCell* NesterovBaseCommon::dbToNb(odb::dbInst* inst) const
{
  Instance* pbInst = pbc_->dbToPb(inst);
  return pbToNb(pbInst);
}

GPin* NesterovBaseCommon::dbToNb(odb::dbITerm* pin) const
{
  Pin* pbPin = pbc_->dbToPb(pin);
  return pbToNb(pbPin);
}

GPin* NesterovBaseCommon::dbToNb(odb::dbBTerm* pin) const
{
  Pin* pbPin = pbc_->dbToPb(pin);
  return pbToNb(pbPin);
}

GNet* NesterovBaseCommon::dbToNb(odb::dbNet* net) const
{
  Net* pbNet = pbc_->dbToPb(net);
  return pbToNb(pbNet);
}

//
// WA force cals - wlCoeffX / wlCoeffY
//
// * Note that wlCoeffX and wlCoeffY is 1/gamma
// in ePlace paper.
void NesterovBaseCommon::updateWireLengthForceWA(float wlCoeffX, float wlCoeffY)
{
  assert(omp_get_thread_num() == 0);
  // clear all WA variables.
#pragma omp parallel for num_threads(num_threads_)
  for (auto gPin = gPinStor_.begin(); gPin < gPinStor_.end(); ++gPin) {
    // old-style loop for old OpenMP
    gPin->clearWaVars();
  }

  // If checks are very expensive, so short circuit them if debug is not enabled
  bool debug_enabled = log_->debugCheck(GPL, "wlUpdateWA", 1);
#pragma omp parallel for num_threads(num_threads_)
  for (auto gNet = gNetStor_.begin(); gNet < gNetStor_.end(); ++gNet) {
    // old-style loop for old OpenMP

    gNet->clearWaVars();
    gNet->updateBox();

    for (auto& gPin : gNet->getGPins()) {
      // The WA terms are shift invariant:
      //
      //   Sum(x_i * exp(x_i))    Sum(x_i * exp(x_i - C))
      //   -----------------    = -----------------
      //   Sum(exp(x_i))          Sum(exp(x_i - C))
      //
      // So we shift to keep the exponential from overflowing
      float expMinX = (gNet->lx() - gPin->cx()) * wlCoeffX;
      float expMaxX = (gPin->cx() - gNet->ux()) * wlCoeffX;
      float expMinY = (gNet->ly() - gPin->cy()) * wlCoeffY;
      float expMaxY = (gPin->cy() - gNet->uy()) * wlCoeffY;

      // min x
      if (expMinX > nbVars_.minWireLengthForceBar) {
        gPin->setMinExpSumX(fastExp(expMinX));
        gNet->addWaExpMinSumX(gPin->minExpSumX());
        gNet->addWaXExpMinSumX(gPin->cx() * gPin->minExpSumX());
        if (debug_enabled && gPin->getGCell()
            && gPin->getGCell()->isInstance()) {
          debugPrint(log_,
                     GPL,
                     "wlUpdateWA",
                     1,
                     "MinX updated: {} {:g}",
                     gPin->getGCell()->getName(),
                     gPin->minExpSumX());
        }
      }

      // max x
      if (expMaxX > nbVars_.minWireLengthForceBar) {
        gPin->setMaxExpSumX(fastExp(expMaxX));
        gNet->addWaExpMaxSumX(gPin->maxExpSumX());
        gNet->addWaXExpMaxSumX(gPin->cx() * gPin->maxExpSumX());
        if (debug_enabled && gPin->getGCell()
            && gPin->getGCell()->isInstance()) {
          debugPrint(log_,
                     GPL,
                     "wlUpdateWA",
                     1,
                     "MaxX updated: {} {:g}",
                     gPin->getGCell()->getName(),
                     gPin->maxExpSumX());
        }
      }

      // min y
      if (expMinY > nbVars_.minWireLengthForceBar) {
        gPin->setMinExpSumY(fastExp(expMinY));
        gNet->addWaExpMinSumY(gPin->minExpSumY());
        gNet->addWaYExpMinSumY(gPin->cy() * gPin->minExpSumY());
        if (debug_enabled && gPin->getGCell()
            && gPin->getGCell()->isInstance()) {
          debugPrint(log_,
                     GPL,
                     "wlUpdateWA",
                     1,
                     "MinY updated: {} {:g}",
                     gPin->getGCell()->getName(),
                     gPin->minExpSumY());
        }
      }

      // max y
      if (expMaxY > nbVars_.minWireLengthForceBar) {
        gPin->setMaxExpSumY(fastExp(expMaxY));
        gNet->addWaExpMaxSumY(gPin->maxExpSumY());
        gNet->addWaYExpMaxSumY(gPin->cy() * gPin->maxExpSumY());
        if (debug_enabled && gPin->getGCell()
            && gPin->getGCell()->isInstance()) {
          debugPrint(log_,
                     GPL,
                     "wlUpdateWA",
                     1,
                     "MaxY updated: {} {:g}",
                     gPin->getGCell()->getName(),
                     gPin->maxExpSumY());
        }
      }
    }
  }
}

GCell& NesterovBaseCommon::getGCell(size_t index)
{
  if (index >= gCellStor_.size()) {
    log_->error(utl::GPL,
                316,
                "getGCell: index {} out of bounds (gCellStor_.size() = {}).",
                index,
                gCellStor_.size());
  }
  return gCellStor_[index];
}

size_t NesterovBaseCommon::getGCellIndex(const GCell* gCell) const
{
  return std::distance(gCellStor_.data(), gCell);
}

// get x,y WA Gradient values with given GCell
FloatPoint NesterovBaseCommon::getWireLengthGradientWA(const GCell* gCell,
                                                       float wlCoeffX,
                                                       float wlCoeffY) const
{
  FloatPoint gradientPair;

  for (auto& gPin : gCell->gPins()) {
    auto tmpPair = getWireLengthGradientPinWA(gPin, wlCoeffX, wlCoeffY);

    debugPrint(log_,
               GPL,
               "getGradientWA",
               1,
               "wlPair: {:g} {:g}",
               tmpPair.x,
               tmpPair.y);

    // apply timing/custom net weight
    tmpPair.x *= gPin->getGNet()->getTotalWeight();
    tmpPair.y *= gPin->getGNet()->getTotalWeight();

    gradientPair.x += tmpPair.x;
    gradientPair.y += tmpPair.y;
  }

  if (gCell->isInstance()) {
    debugPrint(log_,
               GPL,
               "getGradientWA",
               1,
               "{}, gradient: {:g} {:g}",
               gCell->getName(),
               gradientPair.x,
               gradientPair.y);
  }

  // return sum
  return gradientPair;
}

// get x,y WA Gradient values from GPin
// Please check the JingWei's Ph.D. thesis full paper,
// Equation (4.13)
//
// You can't understand the following function
// unless you read the (4.13) formula
FloatPoint NesterovBaseCommon::getWireLengthGradientPinWA(const GPin* gPin,
                                                          float wlCoeffX,
                                                          float wlCoeffY) const
{
  float gradientMinX = 0, gradientMinY = 0;
  float gradientMaxX = 0, gradientMaxY = 0;

  // min x
  if (gPin->hasMinExpSumX()) {
    // from Net.
    float waExpMinSumX = gPin->getGNet()->waExpMinSumX();
    float waXExpMinSumX = gPin->getGNet()->waXExpMinSumX();

    gradientMinX
        = (waExpMinSumX * (gPin->minExpSumX() * (1.0 - wlCoeffX * gPin->cx()))
           + wlCoeffX * gPin->minExpSumX() * waXExpMinSumX)
          / (waExpMinSumX * waExpMinSumX);
  }

  // max x
  if (gPin->hasMaxExpSumX()) {
    float waExpMaxSumX = gPin->getGNet()->waExpMaxSumX();
    float waXExpMaxSumX = gPin->getGNet()->waXExpMaxSumX();

    gradientMaxX
        = (waExpMaxSumX * (gPin->maxExpSumX() * (1.0 + wlCoeffX * gPin->cx()))
           - wlCoeffX * gPin->maxExpSumX() * waXExpMaxSumX)
          / (waExpMaxSumX * waExpMaxSumX);
  }

  // min y
  if (gPin->hasMinExpSumY()) {
    float waExpMinSumY = gPin->getGNet()->waExpMinSumY();
    float waYExpMinSumY = gPin->getGNet()->waYExpMinSumY();

    gradientMinY
        = (waExpMinSumY * (gPin->minExpSumY() * (1.0 - wlCoeffY * gPin->cy()))
           + wlCoeffY * gPin->minExpSumY() * waYExpMinSumY)
          / (waExpMinSumY * waExpMinSumY);
  }

  // max y
  if (gPin->hasMaxExpSumY()) {
    float waExpMaxSumY = gPin->getGNet()->waExpMaxSumY();
    float waYExpMaxSumY = gPin->getGNet()->waYExpMaxSumY();

    gradientMaxY
        = (waExpMaxSumY * (gPin->maxExpSumY() * (1.0 + wlCoeffY * gPin->cy()))
           - wlCoeffY * gPin->maxExpSumY() * waYExpMaxSumY)
          / (waExpMaxSumY * waExpMaxSumY);
  }

  debugPrint(log_,
             GPL,
             "getGradientWAPin",
             1,
             "{}, X[{:g} {:g}]  Y[{:g} {:g}]",
             gPin->getGCell()->getName(),
             gradientMinX,
             gradientMaxX,
             gradientMinY,
             gradientMaxY);

  return FloatPoint(gradientMinX - gradientMaxX, gradientMinY - gradientMaxY);
}

FloatPoint NesterovBaseCommon::getWireLengthPreconditioner(
    const GCell* gCell) const
{
  return FloatPoint(gCell->gPins().size(), gCell->gPins().size());
}

void NesterovBaseCommon::updateDbGCells()
{
  if (db_cbk_) {
    db_cbk_->removeOwner();
  }

  for (auto& gCell : getGCells()) {
    if (gCell->isInstance()) {
      for (Instance* inst : gCell->insts()) {
        odb::dbInst* db_inst = inst->dbInst();
        db_inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);

        // pad awareness on X coordinates
        db_inst->setLocation(gCell->dCx() - inst->dx() / 2
                                 + pbc_->siteSizeX() * pbc_->getPadLeft(),
                             gCell->dCy() - inst->dy() / 2);
      }
    }
  }

  if (db_cbk_) {
    db_cbk_->addOwner(pbc_->db()->getChip()->getBlock());
  }
}

int64_t NesterovBaseCommon::getHpwl()
{
  assert(omp_get_thread_num() == 0);
  int64_t hpwl = 0;
#pragma omp parallel for num_threads(num_threads_) reduction(+ : hpwl)
  for (auto gNet = gNetStor_.begin(); gNet < gNetStor_.end(); ++gNet) {
    // old-style loop for old OpenMP
    gNet->updateBox();
    hpwl += gNet->getHpwl();
  }
  return hpwl;
}

void NesterovBaseCommon::resetMinRcCellSize()
{
  minRcCellSize_.clear();
  minRcCellSize_.shrink_to_fit();
}

void NesterovBaseCommon::resizeMinRcCellSize()
{
  minRcCellSize_.resize(nbc_gcells_.size(), odb::Rect(0, 0, 0, 0));
}

void NesterovBaseCommon::updateMinRcCellSize()
{
  for (auto& gCell : nbc_gcells_) {
    if (!gCell->isStdInstance()) {
      continue;
    }

    int idx = &gCell - nbc_gcells_.data();
    minRcCellSize_[idx] = odb::Rect(0, 0, gCell->dx(), gCell->dy());
  }
}

void NesterovBaseCommon::revertGCellSizeToMinRc()
{
  for (auto& gCell : nbc_gcells_) {
    if (!gCell->isStdInstance()) {
      continue;
    }

    int idx = &gCell - nbc_gcells_.data();
    const odb::Rect& rect = minRcCellSize_[idx];
    int dx = rect.dx();
    int dy = rect.dy();

    if (rect.area() > gCell->insts()[0]->getArea()) {
      gCell->setSize(dx, dy, GCell::GCellChange::kRoutability);
    } else {
      gCell->setSize(dx, dy, GCell::GCellChange::kNone);
    }
  }
}

GCell* NesterovBaseCommon::getGCellByIndex(size_t idx)
{
  if (idx >= gCellStor_.size()) {
    log_->error(GPL,
                315,
                "getGCellByIndex out of bounds: idx = {}, size = {}",
                idx,
                gCellStor_.size());
  }
  return &gCellStor_[idx];
}

// fixPointers() member functions assumes there was push_backs to storage
// vectors, invalidating them. This function resets the pointers and maintain
// consistency among parallel vectors. Most of the code here is based on
// nesterovBaseCommon constructor.
//
void NesterovBaseCommon::fixPointers()
{
  nbc_gcells_.clear();
  gCellMap_.clear();
  db_inst_to_nbc_index_map_.clear();
  nbc_gcells_.reserve(gCellStor_.size());
  for (auto& gCell : gCellStor_) {
    if (!gCell.isInstance()) {
      continue;
    }
    nbc_gcells_.push_back(&gCell);
    for (Instance* inst : gCell.insts()) {
      gCellMap_[inst] = &gCell;
      db_inst_to_nbc_index_map_[inst->dbInst()] = &gCell - &gCellStor_[0];
    }
  }

  gPins_.clear();
  gPinMap_.clear();
  db_iterm_to_index_map_.clear();
  db_bterm_to_index_map_.clear();
  gPins_.reserve(gPinStor_.size());
  for (size_t i = 0; i < gPinStor_.size(); ++i) {
    GPin& gPin = gPinStor_[i];
    gPins_.push_back(&gPin);
    gPinMap_[gPin.getPbPin()] = &gPin;
    if (gPin.getPbPin()->isITerm()) {
      db_iterm_to_index_map_[gPin.getPbPin()->getDbITerm()] = i;
    } else if (gPin.getPbPin()->isBTerm()) {
      db_bterm_to_index_map_[gPin.getPbPin()->getDbBTerm()] = i;
    } else {
      debugPrint(log_, GPL, "callbacks", 1, "gPin neither bterm or iterm!");
    }
  }

  gNets_.clear();
  gNetMap_.clear();
  db_net_to_index_map_.clear();
  gNets_.reserve(gNetStor_.size());
  for (size_t i = 0; i < gNetStor_.size(); ++i) {
    GNet& gNet = gNetStor_[i];
    gNets_.push_back(&gNet);
    gNetMap_[gNet.getPbNet()] = &gNet;
    db_net_to_index_map_[gNet.getPbNet()->getDbNet()] = i;
  }

  for (auto& gCell : gCellStor_) {
    if (gCell.isFiller()) {
      continue;
    }

    gCell.clearGPins();
    for (Instance* inst : gCell.insts()) {
      for (odb::dbITerm* iterm : inst->dbInst()->getITerms()) {
        if (isValidSigType(iterm->getSigType())) {
          auto it = db_iterm_to_index_map_.find(iterm);
          if (it != db_iterm_to_index_map_.end()) {
            size_t gpin_index = it->second;
            gCell.addGPin(&gPinStor_[gpin_index]);
          } else {
            debugPrint(log_,
                       GPL,
                       "callbacks",
                       1,
                       "warning: gpin nullptr (from iterm:{}) in gcell:{}",
                       iterm->getName(),
                       inst->dbInst()->getName());
          }
        }
      }
    }
  }

  for (auto& gPin : gPinStor_) {
    auto iterm = gPin.getPbPin()->getDbITerm();
    if (iterm != nullptr) {
      if (isValidSigType(iterm->getSigType())) {
        auto inst_it = db_inst_to_nbc_index_map_.find(iterm->getInst());
        auto net_it = db_net_to_index_map_.find(iterm->getNet());

        if (inst_it != db_inst_to_nbc_index_map_.end()) {
          gPin.setGCell(&gCellStor_[inst_it->second]);
        }

        if (net_it != db_net_to_index_map_.end()) {
          gPin.setGNet(&gNetStor_[net_it->second]);
        } else {
          debugPrint(
              log_,
              GPL,
              "callbacks",
              1,
              "warning: Net not found in db_net_map_ for ITerm: {} -> {}",
              iterm->getNet()->getName(),
              iterm->getName());
        }
      } else {
        debugPrint(log_,
                   GPL,
                   "callbacks",
                   1,
                   "warning: invalid type itermType: {}",
                   iterm->getSigType().getString());
      }
    }
  }

  for (auto& gNet : gNetStor_) {
    gNet.clearGPins();
    for (odb::dbITerm* iterm : gNet.getPbNet()->getDbNet()->getITerms()) {
      if (isValidSigType(iterm->getSigType())) {
        auto it = db_iterm_to_index_map_.find(iterm);
        if (it != db_iterm_to_index_map_.end()) {
          size_t gpin_index = it->second;
          gNet.addGPin(&gPinStor_[gpin_index]);
        }
      }
    }

    for (odb::dbBTerm* bterm : gNet.getPbNet()->getDbNet()->getBTerms()) {
      if (isValidSigType(bterm->getSigType())) {
        auto it = db_bterm_to_index_map_.find(bterm);
        if (it != db_bterm_to_index_map_.end()) {
          size_t gpin_index = it->second;
          gNet.addGPin(&gPinStor_[gpin_index]);
          if (gPinStor_[gpin_index].getGCell()) {
            gPinStor_[gpin_index].getGCell()->addGPin(&gPinStor_[gpin_index]);
          }
        } else {
          debugPrint(log_,
                     GPL,
                     "callbacks",
                     1,
                     "warning: gpin not found for BTerm: {}",
                     bterm->getName());
        }
      }
    }
  }
}

void NesterovBaseCommon::reportInstanceExtensionByPinDensity() const
{
  int64_t total_original_area = 0;
  int64_t total_extended_area = 0;
  int64_t total_area_diff = 0;
  int increased_instance_count = 0;
  int64_t increased_area = 0;
  int decreased_instance_count = 0;
  int64_t decreased_area = 0;
  int unchanged_instance_count = 0;
  int total_instance_count = 0;
  struct MasterStats
  {
    int instance_count = 0;
    int pin_count = 0;
    double total_original_area = 0;
    double total_extended_area = 0;
    float original_area_per_pin = 0.0;
    float extended_area_per_pin = 0.0;
    float area_diff = 0.0;
  };
  static std::unordered_map<std::string, struct MasterStats> master_stats_map;

  odb::dbBlock* block = pbc_->db()->getChip()->getBlock();

  for (const GCell& gcell : gCellStor_) {
    if (!gcell.isInstance()) {
      continue;
    }
    odb::dbInst* db_inst = gcell.insts()[0]->dbInst();
    odb::dbBox* bbox = db_inst->getBBox();
    if (!bbox) {
      continue;
    }
    ++total_instance_count;
    int orig_dx = bbox->getDX();
    int orig_dy = bbox->getDY();
    int64_t orig_area
        = static_cast<int64_t>(orig_dx) * static_cast<int64_t>(orig_dy);

    int ext_dx = gcell.ux() - gcell.lx();
    int ext_dy = gcell.uy() - gcell.ly();
    int64_t ext_area
        = static_cast<int64_t>(ext_dx) * static_cast<int64_t>(ext_dy);

    total_original_area += orig_area;
    total_extended_area += ext_area;
    int64_t area_diff = ext_area - orig_area;
    total_area_diff += area_diff;

    if (area_diff > 0) {
      ++increased_instance_count;
      increased_area += area_diff;
    } else if (area_diff < 0) {
      ++decreased_instance_count;
      decreased_area += -area_diff;
    } else {
      ++unchanged_instance_count;
    }

    // Collect per-master statistics
    odb::dbMaster* master = db_inst->getMaster();
    std::string master_name = master->getName();
    auto& stats = master_stats_map[master_name];
    stats.instance_count += 1;
    if (stats.pin_count == 0) {
      stats.pin_count = db_inst->getITerms().size();
    }
    stats.total_original_area = block->dbuAreaToMicrons(orig_area);
    stats.total_extended_area = block->dbuAreaToMicrons(ext_area);

    // Save area per pin
    int pin_count = db_inst->getITerms().size();
    if (pin_count > 0) {
      stats.original_area_per_pin
          = block->dbuAreaToMicrons(orig_area) / pin_count;
      stats.extended_area_per_pin
          = block->dbuAreaToMicrons(ext_area) / pin_count;
    }
    // Populate area_diff as the percentage difference between extended and
    // original area
    if (orig_area != 0) {
      stats.area_diff = 100.0f
                        * (static_cast<float>(ext_area - orig_area)
                           / static_cast<float>(orig_area));
    } else {
      stats.area_diff = 0.0f;
    }
  }

  // Log per-master statistics
  log_->report("NB Per-master statistics:");
  for (const auto& entry : master_stats_map) {
    const std::string& master_name = entry.first;
    const MasterStats& stats = entry.second;
    log_->report(
        "  Master: {} | Instances: {} | Pins: {} | Total original area: {} "
        "um^2 | Total extended area: {} um^2 | Area diff: {:.2f}% | Original "
        "area/pin: {:.4f} um^2 | Extended area/pin: {:.4f} um^2",
        master_name,
        stats.instance_count,
        stats.pin_count,
        stats.total_original_area,
        stats.total_extended_area,
        stats.area_diff,
        stats.original_area_per_pin,
        stats.extended_area_per_pin);
  }

  // Write per-master statistics to CSV
  const std::string csv_filename = "inflation_stats.csv";
  std::ofstream csv_file(csv_filename, std::ios::out);
  if (csv_file.is_open()) {
    csv_file << "master_name,instance_count,pin_count,total_original_area_um2,"
                "total_extended_area_um2,area_diff_percent,original_area_per_"
                "pin_um2,extended_area_per_pin_um2\n";
    for (const auto& entry : master_stats_map) {
      const std::string& master_name = entry.first;
      const MasterStats& stats = entry.second;
      csv_file << master_name << "," << stats.instance_count << ","
               << stats.pin_count << "," << stats.total_original_area << ","
               << stats.total_extended_area << "," << stats.area_diff << ","
               << stats.original_area_per_pin << ","
               << stats.extended_area_per_pin << "\n";
    }
    csv_file.close();
  }

  log_->report("NB Total original area: {} um^2",
               block->dbuAreaToMicrons(total_original_area));
  log_->report("NB Total extended area: {} um^2",
               block->dbuAreaToMicrons(total_extended_area));
  log_->report("NB Total area difference (extended - original): {} um^2",
               block->dbuAreaToMicrons(total_area_diff));
  log_->report("NB Total area increased: {} um^2 ({} instances)",
               block->dbuAreaToMicrons(increased_area),
               increased_instance_count);
  log_->report("NB Total area decreased: {} um^2 ({} instances)",
               block->dbuAreaToMicrons(decreased_area),
               decreased_instance_count);
  log_->report(
      "NB Total area modified (sum of increases and decreases): {} um^2",
      block->dbuAreaToMicrons(increased_area + decreased_area));
  if (total_original_area != 0) {
    double rel_diff = static_cast<double>(total_area_diff)
                      / static_cast<double>(total_original_area);
    log_->report("NB Relative area difference: {:.2f}%%", rel_diff * 100.0);
  }
  log_->report("NB Number of instances with increased area: {}",
               increased_instance_count);
  log_->report("NB Number of instances with decreased area: {}",
               decreased_instance_count);
  log_->report("NB Number of instances with unchanged area: {}",
               unchanged_instance_count);
  if (total_instance_count != 0) {
    double percent_increased = static_cast<double>(increased_instance_count)
                               / static_cast<double>(total_instance_count)
                               * 100.0;
    double percent_decreased = static_cast<double>(decreased_instance_count)
                               / static_cast<double>(total_instance_count)
                               * 100.0;
    double percent_unchanged = static_cast<double>(unchanged_instance_count)
                               / static_cast<double>(total_instance_count)
                               * 100.0;
    log_->report("NB Percentage of instances with increased area: {:.2f}%%",
                 percent_increased);
    log_->report("NB Percentage of instances with decreased area: {:.2f}%%",
                 percent_decreased);
    log_->report("NB Percentage of instances with unchanged area: {:.2f}%%",
                 percent_unchanged);
  }
}

////////////////////////////////////////////////
// NesterovBase

NesterovBase::NesterovBase(
    NesterovBaseVars nbVars,
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    std::shared_ptr<PlacerBase> pb,
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    std::shared_ptr<NesterovBaseCommon> nbc,
    utl::Logger* log)
    : nbVars_(nbVars)
{
  pb_ = std::move(pb);
  nbc_ = std::move(nbc);
  log_ = log;
  log_->info(GPL,
             33,
             "---- Initialize Nesterov Region: {}",
             pb_->getGroup() ? pb_->getGroup()->getName() : "Top-level");

  // Set a fixed seed
  srand(42);
  // area update from pb
  stdInstsArea_ = pb_->stdInstsArea();
  macroInstsArea_ = pb_->macroInstsArea();

  int dbu_per_micron = pb_->db()->getChip()->getBlock()->getDbUnitsPerMicron();

  // update gFillerCells
  initFillerGCells();

  nb_gcells_.reserve(pb_->getInsts().size() + fillerStor_.size());

  // add place instances
  for (auto& pb_inst : pb_->placeInsts()) {
    int x_offset = rand() % (2 * dbu_per_micron) - dbu_per_micron;
    int y_offset = rand() % (2 * dbu_per_micron) - dbu_per_micron;

    GCell* gCell = nbc_->pbToNb(pb_inst);
    if (pb_inst != gCell->insts()[0]) {
      // Only process the first cluster once
      continue;
    }

    for (Instance* inst : gCell->insts()) {
      inst->setLocation(pb_inst->lx() + x_offset, pb_inst->ly() + y_offset);
    }
    gCell->updateLocations();
    nb_gcells_.emplace_back(nbc_.get(), nbc_->getGCellIndex(gCell));
    size_t gcells_index = nb_gcells_.size() - 1;
    db_inst_to_nb_index_[pb_inst->dbInst()] = gcells_index;
  }

  // add filler cells to gCells_
  for (size_t i = 0; i < fillerStor_.size(); ++i) {
    nb_gcells_.emplace_back(this, i);
    filler_stor_index_to_nb_index_[i] = nb_gcells_.size() - 1;
  }

  debugPrint(log_,
             GPL,
             "FillerInit",
             1,
             format_label_int,
             "FillerInit:NumGCells:",
             nb_gcells_.size());
  debugPrint(log_,
             GPL,
             "FillerInit",
             1,
             format_label_int,
             "FillerInit:NumGNets:",
             nbc_->getGNets().size());
  debugPrint(log_,
             GPL,
             "FillerInit",
             1,
             format_label_int,
             "FillerInit:NumGPins:",
             nbc_->getGPins().size());

  // initialize bin grid structure
  // send param into binGrid structure
  if (nbVars_.isSetBinCnt) {
    bg_.setBinCnt(nbVars_.binCntX, nbVars_.binCntY);
  }

  bg_.setPlacerBase(pb_);
  bg_.setLogger(log_);
  const odb::Rect& region_bbox = pb_->getRegionBBox();
  bg_.setRegionPoints(region_bbox.xMin(),
                      region_bbox.yMin(),
                      region_bbox.xMax(),
                      region_bbox.yMax());
  bg_.setBinTargetDensity(targetDensity_);

  // update binGrid info
  bg_.initBins();

  // initialize fft structrue based on bins
  std::unique_ptr<FFT> fft(new FFT(bg_.getBinCntX(),
                                   bg_.getBinCntY(),
                                   bg_.getBinSizeX(),
                                   bg_.getBinSizeY()));

  fft_ = std::move(fft);

  // update densitySize and densityScale in each gCell
  updateDensitySize();

  checkConsistency();
}

// virtual filler GCells
void NesterovBase::initFillerGCells()
{
  dbBlock* block = pb_->db()->getChip()->getBlock();
  // extract average dx/dy in range (10%, 90%)
  std::vector<int> dxStor;
  std::vector<int> dyStor;

  dxStor.reserve(pb_->placeInsts().size());
  dyStor.reserve(pb_->placeInsts().size());
  for (auto& placeInst : pb_->placeInsts()) {
    dxStor.push_back(placeInst->dx());
    dyStor.push_back(placeInst->dy());
  }

  // sort
  std::sort(dxStor.begin(), dxStor.end());
  std::sort(dyStor.begin(), dyStor.end());

  // average from (10 - 90%) .
  int64_t dxSum = 0, dySum = 0;

  int minIdx = dxStor.size() * 0.05;
  int maxIdx = dxStor.size() * 0.95;

  // when #instances are too small,
  // extracts average values in whole ranges.
  if (minIdx == maxIdx) {
    minIdx = 0;
    maxIdx = dxStor.size();
  }

  // This should never happen (implies no placeable insts) but it
  // quiets clang-tidy.
  if (maxIdx == minIdx) {
    return;
  }

  for (int i = minIdx; i < maxIdx; i++) {
    dxSum += dxStor[i];
    dySum += dyStor[i];
  }

  // the avgDx and avgDy will be used as filler cells'
  // width and height
  fillerDx_ = static_cast<int>(dxSum / (maxIdx - minIdx));
  fillerDy_ = static_cast<int>(dySum / (maxIdx - minIdx));

  int64_t region_area = pb_->getRegionArea();
  whiteSpaceArea_ = region_area - pb_->nonPlaceInstsArea();

  // if(pb_->group() == nullptr) {
  //   // nonPlaceInstsArea should not have density downscaling!!!
  //   whiteSpaceArea_ = coreArea - pb_->nonPlaceInstsArea();
  // } else {
  //   int64_t domainArea = 0;
  //   for(auto boundary: pb_->group()->getRegion()->getBoundaries()) {
  //     domainArea += boundary->getBox().area();
  //   }
  //   whiteSpaceArea_ = domainArea - pb_->nonPlaceInstsArea();
  // }

  float tmp_targetDensity
      = static_cast<float>(stdInstsArea_)
            / static_cast<float>(whiteSpaceArea_ - macroInstsArea_)
        + 0.01;
  // targetDensity initialize
  if (nbVars_.useUniformTargetDensity) {
    targetDensity_ = tmp_targetDensity;
  } else {
    targetDensity_ = nbVars_.targetDensity;
  }

  const int64_t nesterovInstanceArea = getNesterovInstsArea();

  // TODO density screening
  movableArea_ = whiteSpaceArea_ * targetDensity_;

  totalFillerArea_ = movableArea_ - nesterovInstanceArea;
  uniformTargetDensity_ = static_cast<float>(nesterovInstanceArea)
                          / static_cast<float>(whiteSpaceArea_);
  uniformTargetDensity_ = ceilf(uniformTargetDensity_ * 100) / 100;

  if (totalFillerArea_ < 0) {
    log_->warn(GPL,
               302,
               "Target density {:.4f} is too low for the available free area.\n"
               "Automatically adjusting to uniform density {:.4f}.",
               targetDensity_,
               uniformTargetDensity_);
    targetDensity_ = uniformTargetDensity_;
    movableArea_ = whiteSpaceArea_ * targetDensity_;
    totalFillerArea_ = movableArea_ - nesterovInstanceArea;
  }

  // limit filler cells
  const double limit_filler_ratio = 10;
  const double filler_scale_factor = std::sqrt(
      totalFillerArea_ / (limit_filler_ratio * nesterovInstanceArea));
  if (filler_scale_factor > 1.0) {
    debugPrint(log_,
               GPL,
               "FillerInit",
               1,
               "InitialFillerCellSize {} {}",
               fillerDx_,
               fillerDy_);

    // TODO reference region area, not die here
    const double max_edge_fillers = 1024;
    const int max_filler_x = std::max(
        static_cast<int>(pb_->getDie().coreDx() / max_edge_fillers), fillerDx_);
    const int max_filler_y = std::max(
        static_cast<int>(pb_->getDie().coreDy() / max_edge_fillers), fillerDy_);
    debugPrint(log_,
               GPL,
               "FillerInit",
               1,
               "FillerCellMaxSize {} {}",
               max_filler_x,
               max_filler_y);

    debugPrint(log_,
               GPL,
               "FillerInit",
               1,
               "FillerCellScaleFactor {:.4f}",
               filler_scale_factor);

    fillerDx_ *= filler_scale_factor;
    fillerDy_ *= filler_scale_factor;

    fillerDx_ = std::min(fillerDx_, max_filler_x);
    fillerDy_ = std::min(fillerDy_, max_filler_y);
  }

  const int fillerCnt = static_cast<int>(
      totalFillerArea_ / static_cast<int64_t>(fillerDx_ * fillerDy_));

  debugPrint(log_,
             GPL,
             "FillerInit",
             1,
             "Region Area {}",
             block->dbuAreaToMicrons(region_area));
  debugPrint(log_,
             GPL,
             "FillerInit",
             1,
             "nesterovInstsArea {}",
             block->dbuAreaToMicrons(nesterovInstanceArea));
  debugPrint(log_,
             GPL,
             "FillerInit",
             1,
             "WhiteSpaceArea {}",
             block->dbuAreaToMicrons(whiteSpaceArea_));
  debugPrint(log_,
             GPL,
             "FillerInit",
             1,
             "MovableArea {}",
             block->dbuAreaToMicrons(movableArea_));
  debugPrint(log_,
             GPL,
             "FillerInit",
             1,
             "TotalFillerArea {}",
             block->dbuAreaToMicrons(totalFillerArea_));
  debugPrint(log_, GPL, "FillerInit", 1, "NumFillerCells {}", fillerCnt);
  debugPrint(log_,
             GPL,
             "FillerInit",
             1,
             "FillerCellArea {}",
             block->dbuAreaToMicrons(getFillerCellArea()));
  debugPrint(log_,
             GPL,
             "FillerInit",
             1,
             "FillerCellSize {} {}",
             block->dbuToMicrons(fillerDx_),
             block->dbuToMicrons(fillerDy_));

  //
  // mt19937 supports huge range of random values.
  // rand()'s RAND_MAX is only 32767.
  //
  std::mt19937 randVal(0);
  for (int i = 0; i < fillerCnt; i++) {
    // instability problem between g++ and clang++!
    auto randX = randVal();
    auto randY = randVal();

    // Use group region bounding box
    const odb::Rect& region_bbox = pb_->getRegionBBox();
    int region_dx = region_bbox.dx();
    int region_dy = region_bbox.dy();
    int region_lx = region_bbox.xMin();
    int region_ly = region_bbox.yMin();

    // place filler cells on random coordi and
    // set size as avgDx and avgDy
    GCell filler_gcell(((randX % region_dx) + region_lx),
                       ((randY % region_dy) + region_ly),
                       fillerDx_,
                       fillerDy_);

    fillerStor_.push_back(filler_gcell);
  }
  // totalFillerArea_ = fillerStor_.size() * getFillerCellArea();
  initial_filler_area_ = totalFillerArea_;
}

NesterovBase::~NesterovBase() = default;

// gcell update
void NesterovBase::updateGCellCenterLocation(
    const std::vector<FloatPoint>& coordis)
{
  for (int idx = 0; idx < coordis.size(); ++idx) {
    nb_gcells_[idx]->setCenterLocation(coordis[idx].x, coordis[idx].y);
  }
}

void NesterovBase::updateGCellDensityCenterLocation(
    const std::vector<FloatPoint>& coordis)
{
  for (int idx = 0; idx < coordis.size(); ++idx) {
    nb_gcells_[idx]->setDensityCenterLocation(coordis[idx].x, coordis[idx].y);
  }
  bg_.updateBinsGCellDensityArea(nb_gcells_);
}

void NesterovBase::setTargetDensity(float density)
{
  assert(omp_get_thread_num() == 0);
  targetDensity_ = density;
  bg_.setBinTargetDensity(density);
#pragma omp parallel for num_threads(nbc_->getNumThreads())
  for (auto bin = getBins().begin(); bin < getBins().end(); ++bin) {
    // old-style loop for old OpenMP
    bin->setBinTargetDensity(density);
  }
  // update nonPlaceArea's target denstiy
  bg_.updateBinsNonPlaceArea();
}

void NesterovBase::checkConsistency()
{
  if (!log_->debugCheck(GPL, "checkConsistency", 1)) {
    return;
  }
  const auto block = pb_->db()->getChip()->getBlock();
  const int64_t tolerance = 10000;

  const int64_t expected_white_space
      = pb_->getDie().coreArea() - pb_->nonPlaceInstsArea();
  if (std::abs(whiteSpaceArea_ - expected_white_space) > tolerance) {
    log_->warn(utl::GPL, 319, "Inconsistent white space area");
    log_->report(
        "whiteSpaceArea_: {} (expected:{}) | coreArea: {}, "
        "nonPlaceInstsArea: {}",
        block->dbuAreaToMicrons(whiteSpaceArea_),
        block->dbuAreaToMicrons(expected_white_space),
        block->dbuAreaToMicrons(pb_->getDie().coreArea()),
        block->dbuAreaToMicrons(pb_->nonPlaceInstsArea()));
  }

  const int64_t expected_movable_area = whiteSpaceArea_ * targetDensity_;
  if (std::abs(movableArea_ - expected_movable_area) > tolerance) {
    log_->warn(utl::GPL, 320, "Inconsistent movable area 1");
    log_->report(
        "movableArea_: {} (expected:{}) | whiteSpaceArea_: {}, "
        "targetDensity_: {}",
        block->dbuAreaToMicrons(movableArea_),
        block->dbuAreaToMicrons(expected_movable_area),
        block->dbuAreaToMicrons(whiteSpaceArea_),
        targetDensity_);
  }

  const int64_t expected_filler_area = movableArea_ - getNesterovInstsArea();
  if (std::abs(totalFillerArea_ - expected_filler_area) > tolerance) {
    log_->warn(utl::GPL, 321, "Inconsistent filler area");
    log_->report(
        "totalFillerArea_: {} (expected:{}) | movableArea_: {}, "
        "getNesterovInstsArea_: {}",
        block->dbuAreaToMicrons(totalFillerArea_),
        block->dbuAreaToMicrons(expected_filler_area),
        block->dbuAreaToMicrons(movableArea_),
        block->dbuAreaToMicrons(getNesterovInstsArea()));
  }

  float expected_density = movableArea_ * 1.0 / whiteSpaceArea_;
  float density_diff = std::abs(targetDensity_ - expected_density);
  if (density_diff > 1e-6) {
    log_->warn(utl::GPL, 322, "Inconsistent target density");
    log_->report(
        "targetDensity_: {} (expected:{}) | movableArea_: {}, "
        "whiteSpaceArea_: {}",
        targetDensity_,
        expected_density,
        block->dbuAreaToMicrons(movableArea_),
        block->dbuAreaToMicrons(whiteSpaceArea_));
  }
}

int NesterovBase::getBinCntX() const
{
  return bg_.getBinCntX();
}

int NesterovBase::getBinCntY() const
{
  return bg_.getBinCntY();
}

double NesterovBase::getBinSizeX() const
{
  return bg_.getBinSizeX();
}

double NesterovBase::getBinSizeY() const
{
  return bg_.getBinSizeY();
}

int64_t NesterovBase::getOverflowArea() const
{
  return bg_.getOverflowArea();
}

int64_t NesterovBase::getOverflowAreaUnscaled() const
{
  return bg_.getOverflowAreaUnscaled();
}

int NesterovBase::getFillerDx() const
{
  return fillerDx_;
}

int NesterovBase::getFillerDy() const
{
  return fillerDy_;
}

int NesterovBase::getFillerCnt() const
{
  return static_cast<int>(fillerStor_.size());
}

int64_t NesterovBase::getFillerCellArea() const
{
  return static_cast<int64_t>(fillerDx_) * static_cast<int64_t>(fillerDy_);
}

GCell& NesterovBase::getFillerGCell(size_t index)
{
  if (index >= fillerStor_.size()) {
    log_->error(
        utl::GPL,
        314,
        "getFillerGCell: index {} out of bounds (fillerStor_.size() = {}).",
        index,
        fillerStor_.size());
  }
  return fillerStor_[index];
}

int64_t NesterovBase::getWhiteSpaceArea() const
{
  return whiteSpaceArea_;
}

int64_t NesterovBase::getMovableArea() const
{
  return movableArea_;
}

int64_t NesterovBase::getTotalFillerArea() const
{
  return totalFillerArea_;
}

int64_t NesterovBase::getNesterovInstsArea() const
{
  return stdInstsArea_
         + static_cast<int64_t>(
             std::round(pb_->macroInstsArea() * targetDensity_));
}

float NesterovBase::getSumPhi() const
{
  return sumPhi_;
}

float NesterovBase::getUniformTargetDensity() const
{
  return uniformTargetDensity_;
}

float NesterovBase::initTargetDensity() const
{
  return nbVars_.targetDensity;
}

float NesterovBase::getTargetDensity() const
{
  return targetDensity_;
}

// update densitySize and densityScale in each gCell
void NesterovBase::updateDensitySize()
{
  assert(omp_get_thread_num() == 0);
#pragma omp parallel for num_threads(nbc_->getNumThreads())
  for (auto it = nb_gcells_.begin(); it < nb_gcells_.end(); ++it) {
    auto& gCell = *it;  // old-style loop for old OpenMP
    float scaleX = 0, scaleY = 0;
    float densitySizeX = 0, densitySizeY = 0;
    if (gCell->dx() < REPLACE_SQRT2 * bg_.getBinSizeX()) {
      scaleX = static_cast<float>(gCell->dx())
               / static_cast<float>(REPLACE_SQRT2 * bg_.getBinSizeX());
      densitySizeX = REPLACE_SQRT2 * static_cast<float>(bg_.getBinSizeX());
    } else {
      scaleX = 1.0;
      densitySizeX = gCell->dx();
    }

    if (gCell->dy() < REPLACE_SQRT2 * bg_.getBinSizeY()) {
      scaleY = static_cast<float>(gCell->dy())
               / static_cast<float>(REPLACE_SQRT2 * bg_.getBinSizeY());
      densitySizeY = REPLACE_SQRT2 * static_cast<float>(bg_.getBinSizeY());
    } else {
      scaleY = 1.0;
      densitySizeY = gCell->dy();
    }

    gCell->setDensitySize(densitySizeX, densitySizeY);
    gCell->setDensityScale(scaleX * scaleY);
  }
}

void NesterovBase::updateAreas()
{
  // bloating can change the following :
  // stdInstsArea and macroInstsArea
  stdInstsArea_ = macroInstsArea_ = 0;
  for (auto it = nb_gcells_.begin(); it < nb_gcells_.end(); ++it) {
    auto& gCell = *it;  // old-style loop for old OpenMP
    if (!gCell) {
      continue;
    }
    if (gCell->isMacroInstance()) {
      macroInstsArea_ += static_cast<int64_t>(gCell->dx())
                         * static_cast<int64_t>(gCell->dy());
    } else if (gCell->isStdInstance()) {
      stdInstsArea_ += static_cast<int64_t>(gCell->dx())
                       * static_cast<int64_t>(gCell->dy());
    }
  }
}

void NesterovBase::updateDensityCoordiLayoutInside(GCell* gCell)
{
  float targetLx = gCell->dLx();
  float targetLy = gCell->dLy();

  targetLx = std::max<float>(targetLx, bg_.lx());
  targetLy = std::max<float>(targetLy, bg_.ly());

  if (targetLx + gCell->dDx() > bg_.ux()) {
    targetLx = bg_.ux() - gCell->dDx();
  }

  if (targetLy + gCell->dDy() > bg_.uy()) {
    targetLy = bg_.uy() - gCell->dDy();
  }
  gCell->setDensityLocation(targetLx, targetLy);
}

float NesterovBase::getDensityCoordiLayoutInsideX(const GCell* gCell,
                                                  float cx) const
{
  float adjVal = cx;
  // TODO will change base on each assigned binGrids.
  //
  if (cx - gCell->dDx() / 2.0f < bg_.lx()) {
    adjVal = bg_.lx() + gCell->dDx() / 2.0f;
  }
  if (cx + gCell->dDx() / 2.0f > bg_.ux()) {
    adjVal = bg_.ux() - gCell->dDx() / 2.0f;
  }
  return adjVal;
}

float NesterovBase::getDensityCoordiLayoutInsideY(const GCell* gCell,
                                                  float cy) const
{
  float adjVal = cy;
  // TODO will change base on each assigned binGrids.
  //
  if (cy - gCell->dDy() / 2.0f < bg_.ly()) {
    adjVal = bg_.ly() + gCell->dDy() / 2.0f;
  }
  if (cy + gCell->dDy() / 2.0f > bg_.uy()) {
    adjVal = bg_.uy() - gCell->dDy() / 2.0f;
  }

  return adjVal;
}

FloatPoint NesterovBase::getDensityPreconditioner(const GCell* gCell) const
{
  float areaVal
      = static_cast<float>(gCell->dx()) * static_cast<float>(gCell->dy());

  return FloatPoint(areaVal, areaVal);
}

// get GCells' electroForcePair
// i.e. get DensityGradient with given GCell
FloatPoint NesterovBase::getDensityGradient(const GCell* gCell) const
{
  std::pair<int, int> pairX = bg_.getDensityMinMaxIdxX(gCell);
  std::pair<int, int> pairY = bg_.getDensityMinMaxIdxY(gCell);

  FloatPoint electroForce;

  for (int i = pairX.first; i < pairX.second; i++) {
    for (int j = pairY.first; j < pairY.second; j++) {
      const Bin& bin = bg_.getBinsConst()[j * getBinCntX() + i];
      float overlapArea
          = getOverlapDensityArea(bin, gCell) * gCell->getDensityScale();

      electroForce.x += overlapArea * bin.electroForceX();
      electroForce.y += overlapArea * bin.electroForceY();
    }
  }

  return electroForce;
}

// Density force cals
void NesterovBase::updateDensityForceBin()
{
  assert(omp_get_thread_num() == 0);
  // copy density to utilize FFT
#pragma omp parallel for num_threads(nbc_->getNumThreads())
  for (auto it = getBins().begin(); it < getBins().end(); ++it) {
    auto& bin = *it;  // old-style loop for old OpenMP
    fft_->updateDensity(bin.x(), bin.y(), bin.getDensity());
  }

  // do FFT
  fft_->doFFT();

  // update electroPhi and electroForce
  // update sumPhi_ for nesterov loop
  sumPhi_ = 0;
#pragma omp parallel for num_threads(nbc_->getNumThreads()) \
    reduction(+ : sumPhi_)
  for (auto it = getBins().begin(); it < getBins().end(); ++it) {
    auto& bin = *it;  // old-style loop for old OpenMP
    auto eForcePair = fft_->getElectroForce(bin.x(), bin.y());
    bin.setElectroForce(eForcePair.first, eForcePair.second);

    float electroPhi = fft_->getElectroPhi(bin.x(), bin.y());
    bin.setElectroPhi(electroPhi);

    sumPhi_ += electroPhi
               * static_cast<float>(bin.getNonPlaceArea() + bin.instPlacedArea()
                                    + bin.getFillerArea());
  }
}

void NesterovBase::initDensity1()
{
  assert(omp_get_thread_num() == 0);
  const int gCellSize = nb_gcells_.size();
  curSLPCoordi_.resize(gCellSize, FloatPoint());
  curSLPWireLengthGrads_.resize(gCellSize, FloatPoint());
  curSLPDensityGrads_.resize(gCellSize, FloatPoint());
  curSLPSumGrads_.resize(gCellSize, FloatPoint());

  nextSLPCoordi_.resize(gCellSize, FloatPoint());
  nextSLPWireLengthGrads_.resize(gCellSize, FloatPoint());
  nextSLPDensityGrads_.resize(gCellSize, FloatPoint());
  nextSLPSumGrads_.resize(gCellSize, FloatPoint());

  prevSLPCoordi_.resize(gCellSize, FloatPoint());
  prevSLPWireLengthGrads_.resize(gCellSize, FloatPoint());
  prevSLPDensityGrads_.resize(gCellSize, FloatPoint());
  prevSLPSumGrads_.resize(gCellSize, FloatPoint());

  curCoordi_.resize(gCellSize, FloatPoint());
  nextCoordi_.resize(gCellSize, FloatPoint());

  initCoordi_.resize(gCellSize, FloatPoint());

  snapshotCoordi_.resize(gCellSize, FloatPoint());
  snapshotSLPCoordi_.resize(gCellSize, FloatPoint());
  snapshotSLPSumGrads_.resize(gCellSize, FloatPoint());

#pragma omp parallel for num_threads(nbc_->getNumThreads())
  for (auto it = nb_gcells_.begin(); it < nb_gcells_.end(); ++it) {
    GCell* gCell = *it;  // old-style loop for old OpenMP
    updateDensityCoordiLayoutInside(gCell);
    int idx = it - nb_gcells_.begin();
    curSLPCoordi_[idx] = prevSLPCoordi_[idx] = curCoordi_[idx]
        = initCoordi_[idx] = FloatPoint(gCell->dCx(), gCell->dCy());

    std::string type = "Uknown";
    if (gCell->isInstance()) {
      type = "StdCell";
    } else if (gCell->isMacroInstance()) {
      type = "Macro";
    } else if (gCell->isFiller()) {
      type = "Filler";
    }
  }

  // bin
  updateGCellDensityCenterLocation(curSLPCoordi_);

  prev_hpwl_ = nbc_->getHpwl();

  // FFT update
  updateDensityForceBin();

  baseWireLengthCoef_
      = npVars_->initWireLengthCoef
        / (static_cast<float>(getBinSizeX() + getBinSizeY()) * 0.5);

  sum_overflow_ = static_cast<float>(getOverflowArea())
                  / static_cast<float>(getNesterovInstsArea());

  sum_overflow_unscaled_ = static_cast<float>(getOverflowAreaUnscaled())
                           / static_cast<float>(getNesterovInstsArea());
}

float NesterovBase::initDensity2(float wlCoeffX, float wlCoeffY)
{
  if (wireLengthGradSum_ == 0) {
    densityPenalty_ = npVars_->initDensityPenalty;
    nbUpdatePrevGradient(wlCoeffX, wlCoeffY);
  }

  if (wireLengthGradSum_ != 0) {
    densityPenalty_
        = (wireLengthGradSum_ / densityGradSum_) * npVars_->initDensityPenalty;
  }

  sum_overflow_ = static_cast<float>(getOverflowArea())
                  / static_cast<float>(getNesterovInstsArea());

  sum_overflow_unscaled_ = static_cast<float>(getOverflowAreaUnscaled())
                           / static_cast<float>(getNesterovInstsArea());

  stepLength_ = getStepLength(
      prevSLPCoordi_, prevSLPSumGrads_, curSLPCoordi_, curSLPSumGrads_);

  return stepLength_;
}

float NesterovBase::getStepLength(
    const std::vector<FloatPoint>& prevSLPCoordi_,
    const std::vector<FloatPoint>& prevSLPSumGrads_,
    const std::vector<FloatPoint>& curSLPCoordi_,
    const std::vector<FloatPoint>& curSLPSumGrads_)
{
  coordiDistance_ = getDistance(prevSLPCoordi_, curSLPCoordi_);
  gradDistance_ = getDistance(prevSLPSumGrads_, curSLPSumGrads_);
  debugPrint(log_,
             GPL,
             "getStepLength",
             1,
             "CoordinateDis {:g}, GradientDist {:g}, StepLength: {:g}",
             coordiDistance_,
             gradDistance_,
             stepLength_);

  return coordiDistance_ / gradDistance_;
}

// to execute following function,
//
// nb_->updateGCellDensityCenterLocation(coordi); // bin update
// nb_->updateDensityForceBin(); // bin Force update
//
// nb_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_); // WL
// update
//
void NesterovBase::updateGradients(std::vector<FloatPoint>& sumGrads,
                                   std::vector<FloatPoint>& wireLengthGrads,
                                   std::vector<FloatPoint>& densityGrads,
                                   float wlCoeffX,
                                   float wlCoeffY)
{
  assert(omp_get_thread_num() == 0);
  if (isConverged_) {
    return;
  }

  wireLengthGradSum_ = 0;
  densityGradSum_ = 0;

  float gradSum = 0;

  debugPrint(
      log_, GPL, "updateGrad", 1, "DensityPenalty: {:g}", densityPenalty_);

  // TODO: This OpenMP parallel section is causing non-determinism. Consider
  // revisiting this in the future to restore determinism.
  // #pragma omp parallel for num_threads(nbc_->getNumThreads()) reduction(+ :
  // wireLengthGradSum_, densityGradSum_, gradSum)
  for (size_t i = 0; i < nb_gcells_.size(); i++) {
    GCell* gCell = nb_gcells_.at(i);
    wireLengthGrads[i]
        = nbc_->getWireLengthGradientWA(gCell, wlCoeffX, wlCoeffY);
    densityGrads[i] = getDensityGradient(gCell);

    // Different compiler has different results on the following formula.
    // e.g. wireLengthGradSum_ += fabs(~~.x) + fabs(~~.y);
    //
    // To prevent instability problem,
    // I partitioned the fabs(~~.x) + fabs(~~.y) as two terms.
    //
    wireLengthGradSum_ += std::fabs(wireLengthGrads[i].x);
    wireLengthGradSum_ += std::fabs(wireLengthGrads[i].y);

    densityGradSum_ += std::fabs(densityGrads[i].x);
    densityGradSum_ += std::fabs(densityGrads[i].y);

    sumGrads[i].x = wireLengthGrads[i].x + densityPenalty_ * densityGrads[i].x;
    sumGrads[i].y = wireLengthGrads[i].y + densityPenalty_ * densityGrads[i].y;

    FloatPoint wireLengthPreCondi = nbc_->getWireLengthPreconditioner(gCell);
    FloatPoint densityPrecondi = getDensityPreconditioner(gCell);

    FloatPoint sumPrecondi(
        wireLengthPreCondi.x + (densityPenalty_ * densityPrecondi.x),
        wireLengthPreCondi.y + (densityPenalty_ * densityPrecondi.y));

    sumPrecondi.x
        = std::max(sumPrecondi.x, NesterovPlaceVars::minPreconditioner);
    sumPrecondi.y
        = std::max(sumPrecondi.y, NesterovPlaceVars::minPreconditioner);

    sumGrads[i].x /= sumPrecondi.x;
    sumGrads[i].y /= sumPrecondi.y;

    gradSum += std::fabs(sumGrads[i].x) + std::fabs(sumGrads[i].y);
  }

  debugPrint(log_,
             GPL,
             "updateGrad",
             1,
             "WireLengthGradSum: {:g}",
             wireLengthGradSum_);
  debugPrint(
      log_, GPL, "updateGrad", 1, "DensityGradSum: {:g}", densityGradSum_);
  debugPrint(log_, GPL, "updateGrad", 1, "GradSum: {:g}", gradSum);
}

void NesterovBase::nbUpdatePrevGradient(float wlCoeffX, float wlCoeffY)
{
  updateGradients(prevSLPSumGrads_,
                  prevSLPWireLengthGrads_,
                  prevSLPDensityGrads_,
                  wlCoeffX,
                  wlCoeffY);
}

void NesterovBase::nbUpdateCurGradient(float wlCoeffX, float wlCoeffY)
{
  updateGradients(curSLPSumGrads_,
                  curSLPWireLengthGrads_,
                  curSLPDensityGrads_,
                  wlCoeffX,
                  wlCoeffY);
}

void NesterovBase::nbUpdateNextGradient(float wlCoeffX, float wlCoeffY)
{
  updateGradients(nextSLPSumGrads_,
                  nextSLPWireLengthGrads_,
                  nextSLPDensityGrads_,
                  wlCoeffX,
                  wlCoeffY);
}

void NesterovBase::updateSinglePrevGradient(size_t gCellIndex,
                                            float wlCoeffX,
                                            float wlCoeffY)
{
  updateSingleGradient(gCellIndex,
                       prevSLPSumGrads_,
                       prevSLPWireLengthGrads_,
                       prevSLPDensityGrads_,
                       wlCoeffX,
                       wlCoeffY);
}

void NesterovBase::updateSingleCurGradient(size_t gCellIndex,
                                           float wlCoeffX,
                                           float wlCoeffY)
{
  updateSingleGradient(gCellIndex,
                       curSLPSumGrads_,
                       curSLPWireLengthGrads_,
                       curSLPDensityGrads_,
                       wlCoeffX,
                       wlCoeffY);
}

void NesterovBase::updateSingleGradient(
    size_t gCellIndex,
    std::vector<FloatPoint>& sumGrads,
    std::vector<FloatPoint>& wireLengthGrads,
    std::vector<FloatPoint>& densityGrads,
    float wlCoeffX,
    float wlCoeffY)
{
  if (gCellIndex >= nb_gcells_.size()) {
    return;
  }

  GCell* gCell = nb_gcells_.at(gCellIndex);

  wireLengthGrads[gCellIndex]
      = nbc_->getWireLengthGradientWA(gCell, wlCoeffX, wlCoeffY);
  densityGrads[gCellIndex] = getDensityGradient(gCell);

  sumGrads[gCellIndex].x = wireLengthGrads[gCellIndex].x
                           + densityPenalty_ * densityGrads[gCellIndex].x;
  sumGrads[gCellIndex].y = wireLengthGrads[gCellIndex].y
                           + densityPenalty_ * densityGrads[gCellIndex].y;

  FloatPoint wireLengthPreCondi = nbc_->getWireLengthPreconditioner(gCell);
  FloatPoint densityPrecondi = getDensityPreconditioner(gCell);

  FloatPoint sumPrecondi(
      wireLengthPreCondi.x + (densityPenalty_ * densityPrecondi.x),
      wireLengthPreCondi.y + (densityPenalty_ * densityPrecondi.y));

  sumPrecondi.x = std::max(sumPrecondi.x, NesterovPlaceVars::minPreconditioner);
  sumPrecondi.y = std::max(sumPrecondi.y, NesterovPlaceVars::minPreconditioner);

  sumGrads[gCellIndex].x /= sumPrecondi.x;
  sumGrads[gCellIndex].y /= sumPrecondi.y;
}

void NesterovBase::updateInitialPrevSLPCoordi()
{
  assert(omp_get_thread_num() == 0);
#pragma omp parallel for num_threads(nbc_->getNumThreads())
  for (size_t i = 0; i < nb_gcells_.size(); i++) {
    GCell* curGCell = nb_gcells_[i];

    float prevCoordiX
        = curSLPCoordi_[i].x
          - (npVars_->initialPrevCoordiUpdateCoef * curSLPSumGrads_[i].x);

    float prevCoordiY
        = curSLPCoordi_[i].y
          - (npVars_->initialPrevCoordiUpdateCoef * curSLPSumGrads_[i].y);

    FloatPoint newCoordi(getDensityCoordiLayoutInsideX(curGCell, prevCoordiX),
                         getDensityCoordiLayoutInsideY(curGCell, prevCoordiY));

    prevSLPCoordi_[i] = newCoordi;
  }
}

void NesterovBase::updateDensityCenterCur()
{
  updateGCellDensityCenterLocation(curCoordi_);
}
void NesterovBase::updateDensityCenterCurSLP()
{
  updateGCellDensityCenterLocation(curSLPCoordi_);
}
void NesterovBase::updateDensityCenterPrevSLP()
{
  updateGCellDensityCenterLocation(prevSLPCoordi_);
}
void NesterovBase::updateDensityCenterNextSLP()
{
  updateGCellDensityCenterLocation(nextSLPCoordi_);
}

void NesterovBase::resetMinSumOverflow()
{
  // reset the divergence detect conditions
  minSumOverflow_ = 1e30;
  hpwlWithMinSumOverflow_ = 1e30;
}

float NesterovBase::getPhiCoef(float scaledDiffHpwl) const
{
  debugPrint(
      log_, GPL, "getPhiCoef", 1, "InputScaleDiffHPWL: {:g}", scaledDiffHpwl);

  float retCoef = (scaledDiffHpwl < 0)
                      ? nbVars_.maxPhiCoef
                      : nbVars_.maxPhiCoef
                            * pow(nbVars_.maxPhiCoef, scaledDiffHpwl * -1.0);
  retCoef = std::max(nbVars_.minPhiCoef, retCoef);
  return retCoef;
}

void NesterovBase::updateNextIter(const int iter)
{
  assert(omp_get_thread_num() == 0);
  if (isConverged_) {
    return;
  }

  // swap vector pointers
  std::swap(prevSLPCoordi_, curSLPCoordi_);
  std::swap(prevSLPWireLengthGrads_, curSLPWireLengthGrads_);
  std::swap(prevSLPDensityGrads_, curSLPDensityGrads_);
  std::swap(prevSLPSumGrads_, curSLPSumGrads_);

  // Prevent locked instances from moving
#pragma omp parallel for num_threads(nbc_->getNumThreads())
  for (size_t k = 0; k < nb_gcells_.size(); ++k) {
    if (nb_gcells_[k]->isInstance() && nb_gcells_[k]->isLocked()) {
      nextSLPCoordi_[k] = curSLPCoordi_[k];
      nextSLPWireLengthGrads_[k] = curSLPWireLengthGrads_[k];
      nextSLPDensityGrads_[k] = curSLPDensityGrads_[k];
      nextSLPSumGrads_[k] = curSLPSumGrads_[k];
      nextCoordi_[k] = curCoordi_[k];
    }
  }

  std::swap(curSLPCoordi_, nextSLPCoordi_);
  std::swap(curSLPWireLengthGrads_, nextSLPWireLengthGrads_);
  std::swap(curSLPDensityGrads_, nextSLPDensityGrads_);
  std::swap(curSLPSumGrads_, nextSLPSumGrads_);

  std::swap(curCoordi_, nextCoordi_);

  // In a macro dominated design like mock-array you may be placing
  // very few std cells in a sea of fixed macros.  The overflow denominator
  // may be quite small and prevent convergence.  This is mostly due
  // to our limited ability to move instances off macros cleanly.  As that
  // improves this should no longer be needed.
  const float fractionOfMaxIters
      = static_cast<float>(iter) / npVars_->maxNesterovIter;
  const float overflowDenominator
      = std::max(static_cast<float>(getNesterovInstsArea()),
                 fractionOfMaxIters * pb_->nonPlaceInstsArea() * 0.05f);

  sum_overflow_ = getOverflowArea() / overflowDenominator;
  sum_overflow_unscaled_ = getOverflowAreaUnscaled() / overflowDenominator;

  int64_t hpwl = nbc_->getHpwl();

  float hpwl_percent_change = 0.0;
  if (iter == 0 || (iter) % 10 == 0) {
    if (prev_reported_hpwl_ != 0) {
      hpwl_percent_change = (static_cast<double>(hpwl - prev_reported_hpwl_)
                             / static_cast<double>(prev_reported_hpwl_))
                            * 100.0;
    }
    prev_reported_hpwl_ = hpwl;
    prev_reported_overflow_unscaled_ = sum_overflow_unscaled_;

    std::string group_name;
    if (pb_->getGroup()) {
      group_name = fmt::format(" ({})", pb_->getGroup()->getName());
    }

    if ((iter == 0 || reprint_iter_header_) && !pb_->getGroup()) {
      if (iter == 0) {
        log_->info(GPL, 31, "HPWL: Half-Perimeter Wirelength");
      }

      const std::string nesterov_header
          = fmt::format("{:>9} | {:>8} | {:>13} | {:>8} | {:>9} | {:>5}",
                        "Iteration",
                        "Overflow",
                        "HPWL (um)",
                        "HPWL(%)",
                        "Penalty",
                        "Group");

      log_->report(nesterov_header);
      log_->report(
          "---------------------------------------------------------------");

      reprint_iter_header_ = false;
    }

    dbBlock* block = pb_->db()->getChip()->getBlock();
    log_->report("{:9d} | {:8.4f} | {:13.6e} | {:+7.2f}% | {:9.2e} | {:>5}",
                 iter,
                 sum_overflow_unscaled_,
                 block->dbuToMicrons(hpwl),
                 hpwl_percent_change,
                 densityPenalty_,
                 group_name);
  }

  float phiCoef = getPhiCoef(static_cast<float>(hpwl - prev_hpwl_)
                             / npVars_->referenceHpwl);
  phiCoef_ = phiCoef;
  debugPrint(log_, GPL, "updateNextIter", 1, "PreviousHPWL: {}", prev_hpwl_);
  debugPrint(log_, GPL, "updateNextIter", 1, "NewHPWL: {}", hpwl);
  debugPrint(log_, GPL, "updateNextIter", 1, "PhiCoef: {:g}", phiCoef);
  debugPrint(log_,
             GPL,
             "updateNextIter",
             1,
             "Gradient: {:g}",
             getSecondNorm(curSLPSumGrads_));
  debugPrint(log_, GPL, "updateNextIter", 1, "Phi: {:g}", getSumPhi());
  debugPrint(
      log_, GPL, "updateNextIter", 1, "Overflow: {:g}", sum_overflow_unscaled_);

  densityPenalty_ *= phiCoef;
  prev_hpwl_ = hpwl;

  if (iter > 50 && minSumOverflow_ > sum_overflow_unscaled_) {
    minSumOverflow_ = sum_overflow_unscaled_;
    hpwlWithMinSumOverflow_ = prev_hpwl_;
  }
}

bool NesterovBase::nesterovUpdateStepLength()
{
  if (isConverged_) {
    return true;
  }

  float newStepLength = getStepLength(
      curSLPCoordi_, curSLPSumGrads_, nextSLPCoordi_, nextSLPSumGrads_);

  debugPrint(log_, GPL, "np", 1, "NewStepLength: {:g}", newStepLength);

  if (std::isnan(newStepLength) || std::isinf(newStepLength)) {
    isDiverged_ = true;
    return false;
  }

  if (newStepLength > stepLength_ * 0.95) {
    stepLength_ = newStepLength;
    return false;
  }
  if (newStepLength < 0.01) {
    stepLength_ = 0.01;
    return false;
  }

  stepLength_ = newStepLength;

  return true;
}

void NesterovBase::nesterovUpdateCoordinates(float coeff)
{
  if (isConverged_) {
    return;
  }

  // fill in nextCoordinates with given stepLength_
  for (size_t k = 0; k < nb_gcells_.size(); k++) {
    FloatPoint nextCoordi(
        curSLPCoordi_[k].x + stepLength_ * curSLPSumGrads_[k].x,
        curSLPCoordi_[k].y + stepLength_ * curSLPSumGrads_[k].y);

    FloatPoint nextSLPCoordi(
        nextCoordi.x + coeff * (nextCoordi.x - curCoordi_[k].x),
        nextCoordi.y + coeff * (nextCoordi.y - curCoordi_[k].y));

    GCell* curGCell = nb_gcells_[k];

    nextCoordi_[k]
        = FloatPoint(getDensityCoordiLayoutInsideX(curGCell, nextCoordi.x),
                     getDensityCoordiLayoutInsideY(curGCell, nextCoordi.y));

    nextSLPCoordi_[k]
        = FloatPoint(getDensityCoordiLayoutInsideX(curGCell, nextSLPCoordi.x),
                     getDensityCoordiLayoutInsideY(curGCell, nextSLPCoordi.y));
  }

  // Update Density
  updateGCellDensityCenterLocation(nextSLPCoordi_);
  updateDensityForceBin();
}

void NesterovBase::nesterovAdjustPhi()
{
  if (isConverged_) {
    return;
  }

  // dynamic adjustment for
  // better convergence with
  // large designs
  if (!nbVars_.isMaxPhiCoefChanged && sum_overflow_unscaled_ < 0.35f) {
    nbVars_.isMaxPhiCoefChanged = true;
    nbVars_.maxPhiCoef *= 0.99;
  }
  // keep maxPhiCoef > 1.0, avoid decreasing densityPenalty
  if (nbVars_.maxPhiCoef <= 1.0f) {
    nbVars_.maxPhiCoef = 1.01f;
  }
}

void NesterovBase::saveSnapshot()
{
  if (isConverged_) {
    return;
  }
  // save snapshots for routability-driven
  snapshotCoordi_ = curCoordi_;
  snapshotSLPCoordi_ = curSLPCoordi_;
  snapshotSLPSumGrads_ = curSLPSumGrads_;
  snapshotDensityPenalty_ = densityPenalty_;
  snapshotStepLength_ = stepLength_;
}

bool NesterovBase::checkConvergence(int gpl_iter_count,
                                    int routability_gpl_iter_count,
                                    RouteBase* rb)
{
  assert(omp_get_thread_num() == 0);
  if (isConverged_) {
    return true;
  }
  if (sum_overflow_unscaled_ <= npVars_->targetOverflow) {
    const bool has_group = pb_->getGroup();
    const std::string group_name = has_group ? pb_->getGroup()->getName() : "";
    const int final_iter = gpl_iter_count;
    dbBlock* block = pb_->db()->getChip()->getBlock();

    log_->report("{:9d} | {:8.4f} | {:13.6e} | {:>8} | {:9.2e} | {:>5}",
                 final_iter,
                 sum_overflow_unscaled_,
                 block->dbuToMicrons(nbc_->getHpwl()),
                 "",  // No % delta
                 densityPenalty_,
                 group_name);
    log_->report(
        "---------------------------------------------------------------");

    if (has_group) {
      log_->info(GPL,
                 1016,
                 "Region '{}' placement finished at iteration {}",
                 group_name,
                 final_iter);
    } else {
      log_->info(
          GPL, 1001, "Global placement finished at iteration {}", final_iter);
      if (npVars_->routability_driven_mode) {
        log_->info(GPL,
                   1003,
                   "Routability mode iteration count: {}",
                   routability_gpl_iter_count);
      }
    }

    if (npVars_->routability_driven_mode) {
      rb->calculateRudyTiles();
      rb->updateRudyAverage(false);
      log_->info(GPL,
                 1005,
                 "Routability final weighted congestion: {:.4f}",
                 rb->getRudyAverage());
    }

    log_->info(GPL,
               1002,
               format_label_float,
               "Placed Cell Area",
               block->dbuAreaToMicrons(getNesterovInstsArea()));

    log_->info(GPL,
               1003,
               format_label_float,
               "Available Free Area",
               block->dbuAreaToMicrons(whiteSpaceArea_));

    log_->info(GPL,
               1004,
               "Minimum Feasible Density        {:.4f} (cell_area / free_area)",
               uniformTargetDensity_);

    // The target density should not fall below the uniform density,
    // which is the lower bound: instance_area / whitespace_area.
    // Values below this lead to negative filler area (physically invalid).
    //
    // While the theoretical upper bound is 1.0 (fully using all whitespace),
    // a practical way to define the target density may be based on desired
    // whitespace usage: instance_area / (whitespace_area * usage).
    log_->info(GPL, 1006, "  Suggested Target Densities:");
    log_->info(
        GPL,
        1007,
        "    - For 90% usage of free space: {:.4f}",
        static_cast<double>(getNesterovInstsArea()) / (whiteSpaceArea_ * 0.90));

    log_->info(
        GPL,
        1008,
        "    - For 80% usage of free space: {:.4f}",
        static_cast<double>(getNesterovInstsArea()) / (whiteSpaceArea_ * 0.80));

    if (static_cast<double>(getNesterovInstsArea()) / (whiteSpaceArea_ * 0.50)
        <= 1.0) {
      log_->info(GPL,
                 1009,
                 "    - For 50% usage of free space: {:.4f}",
                 static_cast<double>(getNesterovInstsArea())
                     / (whiteSpaceArea_ * 0.50));
    }

    if (uniformTargetDensity_ > 0.95f) {
      log_->warn(GPL,
                 1015,
                 "High uniform density (>{:.2f}) may cause congestion or "
                 "legalization issues.",
                 uniformTargetDensity_);
    }

#pragma omp parallel for num_threads(nbc_->getNumThreads())
    for (auto it = nb_gcells_.begin(); it < nb_gcells_.end(); ++it) {
      auto& gCell = *it;  // old-style loop for old OpenMP
      if (!gCell->isInstance()) {
        continue;
      }
      gCell->lock();
    }

    isConverged_ = true;
    return true;
  }

  return false;
}

bool NesterovBase::checkDivergence()
{
  if (sum_overflow_unscaled_ < 0.2f
      && sum_overflow_unscaled_ - minSumOverflow_ >= 0.02f
      && hpwlWithMinSumOverflow_ * 1.2f < prev_hpwl_) {
    isDiverged_ = true;
    log_->warn(GPL, 323, "Divergence detected between consecutive iterations");
  }

  // Check if both overflow and HPWL increase
  if (minSumOverflow_ < 0.2f && prev_reported_overflow_unscaled_ > 0
      && prev_reported_hpwl_ > 0) {
    float overflow_change
        = sum_overflow_unscaled_ - prev_reported_overflow_unscaled_;
    float hpwl_increase = (static_cast<float>(prev_hpwl_ - prev_reported_hpwl_))
                          / static_cast<float>(prev_reported_hpwl_);

    const float overflow_acceptance = 0.05f;
    const float hpwl_acceptance = 0.25f;
    if (overflow_change >= overflow_acceptance
        && hpwl_increase >= hpwl_acceptance) {
      isDiverged_ = true;
      log_->warn(GPL,
                 324,
                 "Divergence detected between reported values. Overflow "
                 "change: {:g}, HPWL increase: {:g}%.",
                 overflow_change,
                 hpwl_increase * 100.0f);
    }
  }

  return isDiverged_;
}

bool NesterovBase::revertToSnapshot()
{
  if (isConverged_) {
    return true;
  }
  // revert back the current density penality
  curCoordi_ = snapshotCoordi_;
  curSLPCoordi_ = snapshotSLPCoordi_;
  curSLPSumGrads_ = snapshotSLPSumGrads_;
  densityPenalty_ = snapshotDensityPenalty_;
  stepLength_ = snapshotStepLength_;

  updateGCellDensityCenterLocation(curCoordi_);
  updateDensityForceBin();

  isDiverged_ = false;

  return true;
}

void NesterovBaseCommon::moveGCell(odb::dbInst* db_inst)
{
  auto it = db_inst_to_nbc_index_map_.find(db_inst);
  if (it == db_inst_to_nbc_index_map_.end()) {
    debugPrint(log_,
               GPL,
               "callbacks",
               1,
               "warning: db_inst {} not found in db_inst_to_nbc_index_map_",
               db_inst->getName());
    return;
  }

  GCell* gcell = getGCellByIndex(it->second);
  odb::dbBox* bbox = db_inst->getBBox();
  gcell->setAllLocations(
      bbox->xMin(), bbox->yMin(), bbox->xMax(), bbox->yMax());
}

void NesterovBaseCommon::resizeGCell(odb::dbInst* db_inst)
{
  auto it = db_inst_to_nbc_index_map_.find(db_inst);
  if (it == db_inst_to_nbc_index_map_.end()) {
    debugPrint(log_,
               GPL,
               "callbacks",
               1,
               "warning: db_inst {} not found in db_inst_to_nbc_index_map_",
               db_inst->getName());
    return;
  }

  GCell* gcell = getGCellByIndex(it->second);
  if (!gcell->contains(db_inst)) {
    debugPrint(log_,
               GPL,
               "callbacks",
               1,
               "warning: gcell {} found in db_inst_map_ as {}",
               gcell->getName(),
               db_inst->getName());
  }

  int64_t prevCellArea
      = static_cast<int64_t>(gcell->dx()) * static_cast<int64_t>(gcell->dy());

  // pull new instance dimensions from DB
  for (Instance* inst : gcell->insts()) {
    inst->copyDbLocation(pbc_.get());
  }
  // update gcell
  gcell->updateLocations();
  gcell->setAreaChangeType(GCell::GCellChange::kTimingDriven);

  int64_t newCellArea
      = static_cast<int64_t>(gcell->dx()) * static_cast<int64_t>(gcell->dy());
  int64_t area_change = newCellArea - prevCellArea;
  delta_area_ += area_change;
  if (area_change > 0) {
    gcell->setAreaChangeType(GCell::GCellChange::kUpsize);
  } else if (area_change < 0) {
    gcell->setAreaChangeType(GCell::GCellChange::kDownsize);
  } else {
    gcell->setAreaChangeType(GCell::GCellChange::kResizeNoChange);
  }
}

void NesterovBase::updateGCellState(float wlCoeffX, float wlCoeffY)
{
  for (auto& db_inst : new_instances_) {
    auto db_it = db_inst_to_nb_index_.find(db_inst);
    if (db_it != db_inst_to_nb_index_.end()) {
      size_t gcells_index = db_it->second;
      GCellHandle& handle = nb_gcells_[gcells_index];
      GCell* gcell = handle;

      for (auto& gpin : gcell->gPins()) {
        gpin->getPbPin()->updateCoordi(gpin->getPbPin()->getDbITerm());
        gpin->updateCoordi();
      }

      // analogous to NesterovBase::updateDensitySize()
      float scaleX = 0, scaleY = 0;
      float densitySizeX = 0, densitySizeY = 0;
      if (gcell->dx() < REPLACE_SQRT2 * bg_.getBinSizeX()) {
        scaleX = static_cast<float>(gcell->dx())
                 / static_cast<float>(REPLACE_SQRT2 * bg_.getBinSizeX());
        densitySizeX = REPLACE_SQRT2 * static_cast<float>(bg_.getBinSizeX());
      } else {
        scaleX = 1.0;
        densitySizeX = gcell->dx();
      }

      if (gcell->dy() < REPLACE_SQRT2 * bg_.getBinSizeY()) {
        scaleY = static_cast<float>(gcell->dy())
                 / static_cast<float>(REPLACE_SQRT2 * bg_.getBinSizeY());
        densitySizeY = REPLACE_SQRT2 * static_cast<float>(bg_.getBinSizeY());
      } else {
        scaleY = 1.0;
        densitySizeY = gcell->dy();
      }

      gcell->setDensitySize(densitySizeX, densitySizeY);
      gcell->setDensityScale(scaleX * scaleY);

      // analogous to NesterovBase::initDensity1()
      updateDensityCoordiLayoutInside(gcell);
      curSLPCoordi_[gcells_index] = prevSLPCoordi_[gcells_index]
          = curCoordi_[gcells_index] = initCoordi_[gcells_index]
          = FloatPoint(gcell->dCx(), gcell->dCy());

      // analogous to updateCurGradient()
      updateSingleCurGradient(gcells_index, wlCoeffX, wlCoeffY);

      // analogous to NesterovBase::updateInitialPrevSLPCoordi()
      GCell* curGCell = nb_gcells_[gcells_index];
      float prevCoordiX = curSLPCoordi_[gcells_index].x
                          - npVars_->initialPrevCoordiUpdateCoef
                                * curSLPSumGrads_[gcells_index].x;
      float prevCoordiY = curSLPCoordi_[gcells_index].y
                          - npVars_->initialPrevCoordiUpdateCoef
                                * curSLPSumGrads_[gcells_index].y;
      FloatPoint newCoordi(
          getDensityCoordiLayoutInsideX(curGCell, prevCoordiX),
          getDensityCoordiLayoutInsideY(curGCell, prevCoordiY));
      prevSLPCoordi_[gcells_index] = newCoordi;

      // analogous to
      // NesterovBase::updateGCellDensityCenterLocation(prevSLPCoordi_)
      nb_gcells_[gcells_index]->setDensityCenterLocation(
          prevSLPCoordi_[gcells_index].x, prevSLPCoordi_[gcells_index].y);

      // analogous to updatePrevGradient()
      updateSinglePrevGradient(gcells_index, wlCoeffX, wlCoeffY);
    } else {
      // Not finding a db_inst in the map should not be a problem. Just ignore
      // Occurs when instance created and destroyed in same iteration.
      debugPrint(log_,
                 GPL,
                 "callbacks",
                 1,
                 "warning: updateGCellState, db_inst not found in "
                 "db_inst_to_nb_index_");
    }
  }
  new_instances_.clear();
}

void NesterovBase::createCbkGCell(odb::dbInst* db_inst, size_t stor_index)
{
  debugPrint(log_,
             GPL,
             "callbacks",
             2,
             "NesterovBase: createGCell {}",
             db_inst->getName());
  auto gcell = nbc_->getGCellByIndex(stor_index);
  if (gcell != nullptr) {
    new_instances_.push_back(db_inst);
    nb_gcells_.emplace_back(nbc_.get(), stor_index);
    size_t gcells_index = nb_gcells_.size() - 1;
    debugPrint(log_,
               GPL,
               "callbacks",
               1,
               "NesterovBase: creatGCell {}, index: {}",
               db_inst->getName(),
               gcells_index);
    db_inst_to_nb_index_[db_inst] = gcells_index;
    appendParallelVectors();

  } else {
    debugPrint(log_,
               GPL,
               "callbacks",
               1,
               "Error. Trying to create gCell but it is nullptr!");
  }
}

size_t NesterovBaseCommon::createCbkGCell(odb::dbInst* db_inst)
{
  debugPrint(log_, GPL, "callbacks", 2, "NBC createCbkGCell");
  Instance pb_inst(db_inst, pbc_.get(), log_);

  pb_insts_stor_.push_back(pb_inst);
  GCell gcell(&pb_insts_stor_.back());
  gCellStor_.push_back(gcell);
  minRcCellSize_.emplace_back(gcell.lx(), gcell.ly(), gcell.ux(), gcell.uy());
  GCell* gcell_ptr = &gCellStor_.back();
  gCellMap_[gcell_ptr->insts()[0]] = gcell_ptr;
  db_inst_to_nbc_index_map_[db_inst] = gCellStor_.size() - 1;

  int64_t area_change = static_cast<int64_t>(gcell_ptr->dx())
                        * static_cast<int64_t>(gcell_ptr->dy());
  delta_area_ += area_change;
  new_gcells_count_++;
  gcell_ptr->setAreaChangeType(GCell::GCellChange::kNewInstance);
  return gCellStor_.size() - 1;
}

void NesterovBaseCommon::createCbkGNet(odb::dbNet* db_net, bool skip_io_mode)
{
  debugPrint(log_, GPL, "callbacks", 3, "NBC createGNet");
  Net gpl_net(db_net, skip_io_mode);
  pb_nets_stor_.push_back(gpl_net);
  GNet gnet(&pb_nets_stor_.back());
  gNetStor_.push_back(gnet);
  GNet* gnet_ptr = &gNetStor_.back();
  gNetMap_[gnet_ptr->getPbNet()] = gnet_ptr;
  db_net_to_index_map_[db_net] = gNetStor_.size() - 1;
}

void NesterovBaseCommon::createCbkITerm(odb::dbITerm* iTerm)
{
  debugPrint(log_, GPL, "callbacks", 3, "NBC createITerm");
  Pin gpl_pin(iTerm);
  pb_pins_stor_.push_back(gpl_pin);
  GPin gpin(&pb_pins_stor_.back());
  gPinStor_.push_back(gpin);
  GPin* gpin_ptr = &gPinStor_.back();
  gPinMap_[gpin_ptr->getPbPin()] = gpin_ptr;
  db_iterm_to_index_map_[iTerm] = gPinStor_.size() - 1;
}

// assuming fixpointers will be called later
//  maintaining consistency in NBC::gcellStor_ and NB::gCells_
void NesterovBase::destroyCbkGCell(odb::dbInst* db_inst)
{
  debugPrint(log_,
             GPL,
             "callbacks",
             2,
             "NesterovBase: destroyCbkGCell {}",
             db_inst->getName());
  auto db_it = db_inst_to_nb_index_.find(db_inst);
  if (db_it != db_inst_to_nb_index_.end()) {
    size_t last_index = nb_gcells_.size() - 1;
    size_t gcell_index = db_it->second;

    GCellHandle& handle = nb_gcells_[gcell_index];

    if (handle->isFiller()) {
      debugPrint(log_,
                 GPL,
                 "callbacks",
                 1,
                 "error: trying to destroy filler gcell during callback!");
      return;
    }

    if (gcell_index != last_index) {
      std::swap(nb_gcells_[gcell_index], nb_gcells_[last_index]);
    }
    swapAndPopParallelVectors(gcell_index, last_index);
    nb_gcells_.pop_back();
    db_inst_to_nb_index_.erase(db_it);

    // From now on gcell_index is the index for the replacement (previous last
    // element)
    size_t replacer_index = gcell_index;
    if (replacer_index != last_index) {
      if (!nb_gcells_[replacer_index]->isFiller()) {
        odb::dbInst* replacer_inst
            = nb_gcells_[replacer_index]->insts()[0]->dbInst();
        db_inst_to_nb_index_[replacer_inst] = replacer_index;
      } else {
        size_t filler_stor_index = nb_gcells_[replacer_index].getStorageIndex();
        filler_stor_index_to_nb_index_[filler_stor_index] = replacer_index;
      }
    }

    std::pair<odb::dbInst*, size_t> replacer = nbc_->destroyCbkGCell(db_inst);

    if (replacer.first != nullptr) {
      auto it = db_inst_to_nb_index_.find(replacer.first);
      if (it != db_inst_to_nb_index_.end()) {
        nb_gcells_[it->second].updateHandle(nbc_.get(), replacer.second);
      } else {
        debugPrint(log_,
                   GPL,
                   "callbacks",
                   1,
                   "warn replacer dbInst {} not found in NB map!",
                   replacer.first->getName());
      }
    }

  } else {
    debugPrint(
        log_,
        GPL,
        "callbacks",
        1,
        "warning: db_inst not found in db_inst_to_nb_index_ for instance: {}",
        db_inst->getName());
  }
}

std::pair<odb::dbInst*, size_t> NesterovBaseCommon::destroyCbkGCell(
    odb::dbInst* db_inst)
{
  auto it = db_inst_to_nbc_index_map_.find(db_inst);
  if (it == db_inst_to_nbc_index_map_.end()) {
    log_->error(GPL,
                307,
                "db_inst not found in db_inst_to_NBC_index_map_ when trying to "
                "destroy GCell on NBC");
  }

  size_t index_remove = it->second;
  db_inst_to_nbc_index_map_.erase(it);

  std::pair<odb::dbInst*, size_t> replacement;
  size_t last_index = gCellStor_.size() - 1;

  if (index_remove != last_index) {
    std::swap(gCellStor_[index_remove], gCellStor_[last_index]);
    std::swap(minRcCellSize_[index_remove], minRcCellSize_[last_index]);

    odb::dbInst* swapped_inst = gCellStor_[index_remove].insts()[0]->dbInst();
    db_inst_to_nbc_index_map_[swapped_inst] = index_remove;
    replacement = {swapped_inst, index_remove};
  }

  int64_t area_change = static_cast<int64_t>(gCellStor_.back().dx())
                        * static_cast<int64_t>(gCellStor_.back().dy());
  delta_area_ -= area_change;
  deleted_gcells_count_++;

  gCellStor_.pop_back();
  minRcCellSize_.pop_back();
  return replacement;
}

void NesterovBase::cutFillerCells(int64_t inflation_area)
{
  dbBlock* block = pb_->db()->getChip()->getBlock();
  if (inflation_area < 0) {
    log_->warn(GPL,
               313,
               "Negative area provided to remove fillers: {}. Expected "
               "positive value, ignoring.",
               block->dbuAreaToMicrons(inflation_area));
    return;
  }

  int removed_count = 0;
  const int64_t single_filler_area = getFillerCellArea();
  const int64_t max_fllers_to_remove
      = std::min(inflation_area / single_filler_area,
                 static_cast<int64_t>(fillerStor_.size()));

  int64_t filler_area_before_removal = totalFillerArea_;
  size_t num_filler_before_removal = fillerStor_.size();
  int64_t availableFillerArea = single_filler_area * fillerStor_.size();
  int64_t originalInflationArea = inflation_area;

  for (int i = nb_gcells_.size() - 1;
       i >= 0 && removed_count < max_fllers_to_remove;
       --i) {
    if (nb_gcells_[i]->isFiller()) {
      const GCell& removed = fillerStor_[nb_gcells_[i].getStorageIndex()];
      removed_fillers_.push_back(RemovedFillerState{
          .gcell = removed,
          .curSLPCoordi = curSLPCoordi_[i],
          .curSLPWireLengthGrads = curSLPWireLengthGrads_[i],
          .curSLPDensityGrads = curSLPDensityGrads_[i],
          .curSLPSumGrads = curSLPSumGrads_[i],

          .nextSLPCoordi = nextSLPCoordi_[i],
          .nextSLPWireLengthGrads = nextSLPWireLengthGrads_[i],
          .nextSLPDensityGrads = nextSLPDensityGrads_[i],
          .nextSLPSumGrads = nextSLPSumGrads_[i],

          .prevSLPCoordi = prevSLPCoordi_[i],
          .prevSLPWireLengthGrads = prevSLPWireLengthGrads_[i],
          .prevSLPDensityGrads = prevSLPDensityGrads_[i],
          .prevSLPSumGrads = prevSLPSumGrads_[i],

          .curCoordi = curCoordi_[i],
          .nextCoordi = nextCoordi_[i],
          .initCoordi = initCoordi_[i],

          .snapshotCoordi = snapshotCoordi_[i],
          .snapshotSLPCoordi = snapshotSLPCoordi_[i],
          .snapshotSLPSumGrads = snapshotSLPSumGrads_[i]});

      destroyFillerGCell(i);
      availableFillerArea -= single_filler_area;
      inflation_area -= single_filler_area;
      ++removed_count;
    }
  }

  totalFillerArea_ = availableFillerArea;

  if (single_filler_area * fillerStor_.size() != totalFillerArea_) {
    log_->warn(GPL,
               312,
               "Unexpected filler area! The value {}, should be equal to "
               "totalFillerArea_ {}.",
               block->dbuAreaToMicrons(single_filler_area * fillerStor_.size()),
               block->dbuAreaToMicrons(totalFillerArea_));
  }

  log_->info(GPL,
             76,
             "Removing fillers, count: Before: {}, After: {} ({:+.2f}%)",
             num_filler_before_removal,
             fillerStor_.size(),
             (num_filler_before_removal != 0)
                 ? (static_cast<double>(
                        static_cast<int64_t>(fillerStor_.size())
                        - static_cast<int64_t>(num_filler_before_removal))
                    / num_filler_before_removal * 100.0)
                 : 0.0);

  log_->info(
      GPL,
      77,
      "Filler area (um^2)     : Before: {:.3f}, After: {:.3f} ({:+.2f}%)",
      block->dbuAreaToMicrons(filler_area_before_removal),
      block->dbuAreaToMicrons(totalFillerArea_),
      (filler_area_before_removal != 0)
          ? (static_cast<double>(totalFillerArea_ - filler_area_before_removal)
             / filler_area_before_removal * 100.0)
          : 0.0);

  int64_t removedFillerArea = single_filler_area * removed_count;
  int64_t remainingInflationArea = originalInflationArea - removedFillerArea;

  log_->info(GPL,
             78,
             "Removed fillers count: {}, area removed: {:.3f} um^2. Remaining "
             "area to be "
             "compensated by modifying density: {:.3f} um^2",
             removed_count,
             block->dbuAreaToMicrons(removedFillerArea),
             block->dbuAreaToMicrons(remainingInflationArea));

  if (remainingInflationArea > single_filler_area) {
    int64_t totalGCellArea = getNesterovInstsArea() + removedFillerArea
                             + totalFillerArea_ + remainingInflationArea;
    setTargetDensity(static_cast<float>(totalGCellArea)
                     / static_cast<float>(getWhiteSpaceArea()));
    movableArea_ = whiteSpaceArea_ * targetDensity_;
    log_->info(GPL, 79, "New target density: {}", targetDensity_);
  }
}

void NesterovBase::destroyFillerGCell(size_t nb_index_remove)
{
  debugPrint(log_,
             GPL,
             "callbacks",
             2,
             "destroy filler nb index: {}, nb_gcells_ size: {}",
             nb_index_remove,
             nb_gcells_.size());
  size_t stor_last_index = fillerStor_.size() - 1;
  GCellHandle& gcell_remove = nb_gcells_[nb_index_remove];
  size_t stor_index_remove = gcell_remove.getStorageIndex();
  if (!gcell_remove->isFiller()) {
    debugPrint(log_,
               GPL,
               "callbacks",
               1,
               "trying to destroy filler, but gcell ({}) is not filler!",
               gcell_remove->getName());
    return;
  }
  if (stor_index_remove > stor_last_index) {
    debugPrint(
        log_,
        GPL,
        "callbacks",
        1,
        "destroy filler: index {} out of bounds for fillerStor_ (max:{})",
        stor_index_remove,
        stor_last_index);
    return;
  }

  size_t nb_last_index = nb_gcells_.size() - 1;
  if (nb_index_remove != nb_last_index) {
    GCellHandle& gcell_replace = nb_gcells_[nb_last_index];
    if (!gcell_replace->isFiller()) {
      odb::dbInst* db_inst = gcell_replace->insts()[0]->dbInst();
      auto it = db_inst_to_nb_index_.find(db_inst);
      if (it != db_inst_to_nb_index_.end()) {
        it->second = nb_index_remove;
      } else {
        debugPrint(log_,
                   GPL,
                   "callbacks",
                   1,
                   "Warning: gcell_replace dbInst {} not found in "
                   "db_inst_to_nb_index_ map",
                   db_inst->getName());
      }
    }
    std::swap(nb_gcells_[nb_index_remove], nb_gcells_[nb_last_index]);
  }
  swapAndPopParallelVectors(nb_index_remove, nb_last_index);
  nb_gcells_.pop_back();
  filler_stor_index_to_nb_index_.erase(stor_index_remove);

  if (stor_index_remove != stor_last_index) {
    size_t replacer_index
        = filler_stor_index_to_nb_index_.find(stor_last_index)->second;
    std::swap(fillerStor_[stor_index_remove], fillerStor_[stor_last_index]);
    nb_gcells_[replacer_index].updateHandle(this, stor_index_remove);
    filler_stor_index_to_nb_index_[stor_index_remove] = replacer_index;
  }
  fillerStor_.pop_back();
}

void NesterovBase::restoreRemovedFillers()
{
  log_->info(GPL,
             80,
             "Restoring {} previously removed fillers.",
             removed_fillers_.size());

  if (removed_fillers_.empty()) {
    return;
  }

  size_t num_fill_before = fillerStor_.size();
  int64_t area_before = totalFillerArea_;

  for (const auto& filler : removed_fillers_) {
    fillerStor_.push_back(filler.gcell);
    size_t new_index = fillerStor_.size() - 1;
    nb_gcells_.emplace_back(this, new_index);
    filler_stor_index_to_nb_index_[new_index] = nb_gcells_.size() - 1;

    appendParallelVectors();
    size_t idx = nb_gcells_.size() - 1;
    debugPrint(log_, GPL, "callbacks", 2, "restore filler nb index:  {}", idx);
    // Restore parallel vector data
    curSLPCoordi_[idx] = filler.curSLPCoordi;
    curSLPWireLengthGrads_[idx] = filler.curSLPWireLengthGrads;
    curSLPDensityGrads_[idx] = filler.curSLPDensityGrads;
    curSLPSumGrads_[idx] = filler.curSLPSumGrads;

    nextSLPCoordi_[idx] = filler.nextSLPCoordi;
    nextSLPWireLengthGrads_[idx] = filler.nextSLPWireLengthGrads;
    nextSLPDensityGrads_[idx] = filler.nextSLPDensityGrads;
    nextSLPSumGrads_[idx] = filler.nextSLPSumGrads;

    prevSLPCoordi_[idx] = filler.prevSLPCoordi;
    prevSLPWireLengthGrads_[idx] = filler.prevSLPWireLengthGrads;
    prevSLPDensityGrads_[idx] = filler.prevSLPDensityGrads;
    prevSLPSumGrads_[idx] = filler.prevSLPSumGrads;

    curCoordi_[idx] = filler.curCoordi;
    nextCoordi_[idx] = filler.nextCoordi;
    initCoordi_[idx] = filler.initCoordi;

    snapshotCoordi_[idx] = filler.snapshotCoordi;
    snapshotSLPCoordi_[idx] = filler.snapshotSLPCoordi;
    snapshotSLPSumGrads_[idx] = filler.snapshotSLPSumGrads;

    totalFillerArea_ += getFillerCellArea();
  }

  size_t num_fill_after = fillerStor_.size();
  int64_t area_after = totalFillerArea_;

  double rel_count_change
      = (num_fill_before > 0)
            ? (static_cast<double>(num_fill_after - num_fill_before)
               / num_fill_before)
                  * 100.0
            : 0.0;

  double rel_area_change = (area_before > 0)
                               ? (static_cast<double>(area_after - area_before)
                                  / static_cast<double>(area_before))
                                     * 100.0
                               : 0.0;

  dbBlock* block = pb_->db()->getChip()->getBlock();
  double area_before_um = block->dbuAreaToMicrons(area_before);
  double area_after_um = block->dbuAreaToMicrons(area_after);

  log_->info(GPL,
             81,
             "Number of fillers before restoration {} and after {} . Relative "
             "change: {:+.2f}%%",
             num_fill_before,
             num_fill_after,
             rel_count_change);

  log_->info(GPL,
             82,
             "Total filler area before restoration {:.2f} and after {:.2f} "
             "(um^2). Relative change: {:+.2f}%%",
             area_before_um,
             area_after_um,
             rel_area_change);

  removed_fillers_.clear();
}

void NesterovBaseCommon::destroyCbkGNet(odb::dbNet* db_net)
{
  debugPrint(log_, GPL, "callbacks", 3, "NBC destroyGNet");
  auto db_it = db_net_to_index_map_.find(db_net);
  if (db_it == db_net_to_index_map_.end()) {
    log_->error(GPL,
                308,
                "db_net not found in db_net_to_NBC_index_map_ for net: {}",
                db_net->getName());
    return;
  }

  size_t index_remove = db_it->second;
  size_t last_index = gNetStor_.size() - 1;

  if (index_remove > last_index) {
    log_->error(GPL,
                309,
                "index {} out of bounds for gNetStor_ (max: {})",
                index_remove,
                last_index);
  }

  if (index_remove != last_index) {
    std::swap(gNetStor_[index_remove], gNetStor_[last_index]);

    // Update index map for the swapped net
    odb::dbNet* swapped_net
        = gNetStor_[index_remove].getPbNets()[0]->getDbNet();
    db_net_to_index_map_[swapped_net] = index_remove;
  }

  gNetStor_.pop_back();
  db_net_to_index_map_.erase(db_it);
}

void NesterovBaseCommon::destroyCbkITerm(odb::dbITerm* db_iterm)
{
  debugPrint(log_, GPL, "callbacks", 3, "NBC destroyITerm");
  auto db_it = db_iterm_to_index_map_.find(db_iterm);
  if (db_it != db_iterm_to_index_map_.end()) {
    size_t last_index = gPinStor_.size() - 1;
    size_t index_remove = db_it->second;

    if (index_remove > last_index) {
      log_->error(GPL,
                  310,
                  "index {} out of bounds for gPinStor_ (max:{})",
                  index_remove,
                  last_index);
    }
    if (index_remove != last_index) {
      std::swap(gPinStor_[index_remove], gPinStor_[last_index]);
      odb::dbITerm* swapped_iterm
          = gPinStor_[index_remove].getPbPin()->getDbITerm();
      db_iterm_to_index_map_[swapped_iterm] = index_remove;
    }
    gPinStor_.pop_back();
    db_iterm_to_index_map_.erase(db_it);

  } else {
    log_->error(GPL,
                311,
                "db_iterm not found in db_iterm_map_ for iterm: {}",
                db_iterm->getMTerm()->getName());
  }
}

void NesterovBase::swapAndPop(std::vector<FloatPoint>& vec,
                              size_t remove_index,
                              size_t last_index)
{
  if (vec.empty()) {
    debugPrint(
        log_, GPL, "callbacks", 1, "Warn Attempted to pop from empty vector.");
    return;
  }

  if (remove_index >= vec.size()) {
    debugPrint(log_,
               GPL,
               "callbacks",
               1,
               "remove_index {} out of bounds for vector size {}.",
               remove_index,
               vec.size());
    return;
  }

  if (remove_index != last_index) {
    std::swap(vec[remove_index], vec[last_index]);
  }
  vec.pop_back();
}

void NesterovBase::swapAndPopParallelVectors(size_t remove_index,
                                             size_t last_index)
{
  debugPrint(log_,
             GPL,
             "callbacks",
             3,
             "Swapping and popping parallel vectors with remove_index {} and "
             "last_index {}",
             remove_index,
             last_index);

  // Avoid modifying this if snapshot has not been saved yet.
  if (curSLPCoordi_.size() == snapshotCoordi_.size()) {
    swapAndPop(snapshotCoordi_, remove_index, last_index);
    swapAndPop(snapshotSLPCoordi_, remove_index, last_index);
    swapAndPop(snapshotSLPSumGrads_, remove_index, last_index);
  }
  swapAndPop(curSLPCoordi_, remove_index, last_index);
  swapAndPop(curSLPWireLengthGrads_, remove_index, last_index);
  swapAndPop(curSLPDensityGrads_, remove_index, last_index);
  swapAndPop(curSLPSumGrads_, remove_index, last_index);
  swapAndPop(nextSLPCoordi_, remove_index, last_index);
  swapAndPop(nextSLPWireLengthGrads_, remove_index, last_index);
  swapAndPop(nextSLPDensityGrads_, remove_index, last_index);
  swapAndPop(nextSLPSumGrads_, remove_index, last_index);
  swapAndPop(prevSLPCoordi_, remove_index, last_index);
  swapAndPop(prevSLPWireLengthGrads_, remove_index, last_index);
  swapAndPop(prevSLPDensityGrads_, remove_index, last_index);
  swapAndPop(prevSLPSumGrads_, remove_index, last_index);
  swapAndPop(curCoordi_, remove_index, last_index);
  swapAndPop(nextCoordi_, remove_index, last_index);
  swapAndPop(initCoordi_, remove_index, last_index);
}

void NesterovBase::appendParallelVectors()
{
  if (curSLPCoordi_.size() == snapshotCoordi_.size()) {
    snapshotCoordi_.emplace_back();
    snapshotSLPCoordi_.emplace_back();
    snapshotSLPSumGrads_.emplace_back();
  }
  curSLPCoordi_.emplace_back();
  curSLPWireLengthGrads_.emplace_back();
  curSLPDensityGrads_.emplace_back();
  curSLPSumGrads_.emplace_back();
  nextSLPCoordi_.emplace_back();
  nextSLPWireLengthGrads_.emplace_back();
  nextSLPDensityGrads_.emplace_back();
  nextSLPSumGrads_.emplace_back();
  prevSLPCoordi_.emplace_back();
  prevSLPWireLengthGrads_.emplace_back();
  prevSLPDensityGrads_.emplace_back();
  prevSLPSumGrads_.emplace_back();
  curCoordi_.emplace_back();
  nextCoordi_.emplace_back();
  initCoordi_.emplace_back();
}

void NesterovBaseCommon::printGCells()
{
  log_->report("gCellStor_.size():{}", gCellStor_.size());
  for (size_t i = 0; i < gCellStor_.size(); ++i) {
    log_->reportLiteral(fmt::format("idx:{}", i));
    gCellStor_[i].print(log_);
  }
}

void NesterovBaseCommon::printGPins()
{
  for (auto& gpin : gPinStor_) {
    gpin.print(log_);
  }
}

void NesterovBase::appendGCellCSVNote(const std::string& filename,
                                      int iteration,
                                      const std::string& message) const
{
  std::ofstream file(filename, std::ios::app);
  if (!file.is_open()) {
    log_->report("Could not open CSV file for appending message: {}", filename);
    return;
  }

  file << "# NOTE @ iteration " << iteration << ": " << message << "\n";
  file.close();
}

void NesterovBase::writeGCellVectorsToCSV(const std::string& filename,
                                          int iteration,
                                          int start_iteration,
                                          int iteration_stride,
                                          int gcell_index_stride) const
{
  if (iteration != 0
      && (iteration < start_iteration || iteration % iteration_stride != 0)) {
    return;
  }

  bool file_exists = std::ifstream(filename).good();
  std::ofstream file(filename, std::ios::app);
  if (!file.is_open()) {
    file.open(filename, std::ios::out | std::ios::app);
    if (!file.is_open()) {
      log_->report("Could not create or open file: {}", filename);
      return;
    }
  }

  // Write header only if file didn't exist before
  if (!file_exists) {
    file << "iteration,index,name";
    file << ",insts_size,gPins_size";
    file << ",lx,ly,ux,uy";
    file << ",dLx,dLy,dUx,dUy";
    file << ",densityScale,gradientX,gradientY";

    auto add_header = [&](const std::string& name) {
      file << "," << name << "_x" << "," << name << "_y";
    };

    add_header("curSLPCoordi");
    add_header("curSLPWireLengthGrads");
    add_header("curSLPDensityGrads");
    add_header("curSLPSumGrads");

    add_header("nextSLPCoordi");
    add_header("nextSLPWireLengthGrads");
    add_header("nextSLPDensityGrads");
    add_header("nextSLPSumGrads");

    add_header("prevSLPCoordi");
    add_header("prevSLPWireLengthGrads");
    add_header("prevSLPDensityGrads");
    add_header("prevSLPSumGrads");

    add_header("curCoordi");
    add_header("nextCoordi");
    add_header("initCoordi");

    add_header("snapshotCoordi");
    add_header("snapshotSLPCoordi");
    add_header("snapshotSLPSumGrads");

    file << "\n";
  }

  size_t num_rows = curSLPCoordi_.size();

  for (size_t i = 0; i < num_rows; i += gcell_index_stride) {
    file << iteration << "," << i;
    file << "," << nb_gcells_[i]->getName();
    nb_gcells_[i]->writeAttributesToCSV(file);
    // file << "," << nb_gcells_[i]->insts().size() << "," <<
    // nb_gcells_[i]->gPins().size();

    auto add_value = [&](const std::vector<FloatPoint>& vec) {
      file << "," << vec[i].x << "," << vec[i].y;
    };

    add_value(curSLPCoordi_);
    add_value(curSLPWireLengthGrads_);
    add_value(curSLPDensityGrads_);
    add_value(curSLPSumGrads_);

    add_value(nextSLPCoordi_);
    add_value(nextSLPWireLengthGrads_);
    add_value(nextSLPDensityGrads_);
    add_value(nextSLPSumGrads_);

    add_value(prevSLPCoordi_);
    add_value(prevSLPWireLengthGrads_);
    add_value(prevSLPDensityGrads_);
    add_value(prevSLPSumGrads_);

    add_value(curCoordi_);
    add_value(nextCoordi_);
    add_value(initCoordi_);

    if (snapshotCoordi_.size() == curSLPCoordi_.size()) {
      add_value(snapshotCoordi_);
      add_value(snapshotSLPCoordi_);
      add_value(snapshotSLPSumGrads_);
    }

    file << "\n";
  }

  file.close();
}

static float getOverlapDensityArea(const Bin& bin, const GCell* cell)
{
  const int rectLx = std::max(bin.lx(), cell->dLx());
  const int rectLy = std::max(bin.ly(), cell->dLy());
  const int rectUx = std::min(bin.ux(), cell->dUx());
  const int rectUy = std::min(bin.uy(), cell->dUy());

  if (rectLx >= rectUx || rectLy >= rectUy) {
    return 0;
  }
  return static_cast<float>(rectUx - rectLx)
         * static_cast<float>(rectUy - rectLy);
}

static int64_t getOverlapArea(const Bin* bin,
                              const Instance* inst,
                              int dbu_per_micron)
{
  int rectLx = std::max(bin->lx(), inst->lx()),
      rectLy = std::max(bin->ly(), inst->ly()),
      rectUx = std::min(bin->ux(), inst->ux()),
      rectUy = std::min(bin->uy(), inst->uy());

  if (rectLx >= rectUx || rectLy >= rectUy) {
    return 0;
  }

  if (inst->isMacro()) {
    const float meanX = (inst->cx() - inst->lx()) / (float) dbu_per_micron;
    const float meanY = (inst->cy() - inst->ly()) / (float) dbu_per_micron;

    // For the bivariate normal distribution, we are using
    // the shifted means of X and Y.
    // Sigma is used as the mean/4 for both dimensions
    const biNormalParameters i
        = {meanX,
           meanY,
           meanX / 6,
           meanY / 6,
           (rectLx - inst->lx()) / (float) dbu_per_micron,
           (rectLy - inst->ly()) / (float) dbu_per_micron,
           (rectUx - inst->lx()) / (float) dbu_per_micron,
           (rectUy - inst->ly()) / (float) dbu_per_micron};

    const float original = static_cast<float>(rectUx - rectLx)
                           * static_cast<float>(rectUy - rectLy);
    const float scaled = calculateBiVariateNormalCDF(i)
                         * static_cast<float>(inst->ux() - inst->lx())
                         * static_cast<float>(inst->uy() - inst->ly());

    // For heavily dense regions towards the center of the macro,
    // we are using an upper limit of 1.10*(overlap) between the macro
    // and the bin.
    if (scaled >= original) {
      return std::min<float>(scaled, original * 1.10);
    }
    // If the scaled value is smaller than the actual overlap
    // then use the original overlap value instead.
    // This is implemented to prevent cells from being placed
    // at the outer sides of the macro.
    return original;
  }
  return static_cast<float>(rectUx - rectLx)
         * static_cast<float>(rectUy - rectLy);
}

static int64_t getOverlapAreaUnscaled(const Bin* bin, const Instance* inst)
{
  const int rectLx = std::max(bin->lx(), inst->lx());
  const int rectLy = std::max(bin->ly(), inst->ly());
  const int rectUx = std::min(bin->ux(), inst->ux());
  const int rectUy = std::min(bin->uy(), inst->uy());

  if (rectLx >= rectUx || rectLy >= rectUy) {
    return 0;
  }
  return static_cast<int64_t>(rectUx - rectLx)
         * static_cast<int64_t>(rectUy - rectLy);
}

// A function that does 2D integration to the density function of a
// bivariate normal distribution with 0 correlation.
// Essentially, the function being integrated is the product
// of 2 1D probability density functions (for x and y). The means and standard
// deviation of the probablity density functions are parametarized. In this
// function, I am using the closed-form solution of the integration. The limits
// of integration are lx->ux and ly->uy For reference: the equation that is
// being integrated is:
//      (1/(2*pi*sigmaX*sigmaY))*e^(-(y-meanY)^2/(2*sigmaY*sigmaY))*e^(-(x-meanX)^2/(2*sigmaX*sigmaX))
static float calculateBiVariateNormalCDF(biNormalParameters i)
{
  const float x1 = (i.meanX - i.lx) / (std::sqrt(2) * i.sigmaX);
  const float x2 = (i.meanX - i.ux) / (std::sqrt(2) * i.sigmaX);

  const float y1 = (i.meanY - i.ly) / (std::sqrt(2) * i.sigmaY);
  const float y2 = (i.meanY - i.uy) / (std::sqrt(2) * i.sigmaY);

  return 0.25
         * (std::erf(x1) * std::erf(y1) + std::erf(x2) * std::erf(y2)
            - std::erf(x1) * std::erf(y2) - std::erf(x2) * std::erf(y1));
}
//
// https://codingforspeed.com/using-faster-exponential-approximation/
static float fastExp(float exp)
{
  exp = 1.0f + exp / 1024.0f;
  exp *= exp;
  exp *= exp;
  exp *= exp;
  exp *= exp;
  exp *= exp;
  exp *= exp;
  exp *= exp;
  exp *= exp;
  exp *= exp;
  exp *= exp;
  return exp;
}

static float getDistance(const std::vector<FloatPoint>& a,
                         const std::vector<FloatPoint>& b)
{
  float sumDistance = 0.0f;
  for (size_t i = 0; i < a.size(); i++) {
    sumDistance += (a[i].x - b[i].x) * (a[i].x - b[i].x);
    sumDistance += (a[i].y - b[i].y) * (a[i].y - b[i].y);
  }

  return std::sqrt(sumDistance / (2.0 * a.size()));
}

static float getSecondNorm(const std::vector<FloatPoint>& a)
{
  float norm = 0;
  for (auto& coordi : a) {
    norm += coordi.x * coordi.x + coordi.y * coordi.y;
  }
  return std::sqrt(norm / (2.0 * a.size()));
}
}  // namespace gpl
