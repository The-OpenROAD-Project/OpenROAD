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

#include "gpl/Replace.h"

#include <iostream>

#include "initialPlace.h"
#include "nesterovBase.h"
#include "nesterovPlace.h"
#include "odb/db.h"
#include "placerBase.h"
#include "routeBase.h"
#include "rsz/Resizer.hh"
#include "timingBase.h"
#include "utl/Logger.h"

namespace gpl {

using namespace std;
using utl::GPL;

Replace::Replace()
    : db_(nullptr),
      rs_(nullptr),
      fr_(nullptr),
      log_(nullptr),
      pb_(nullptr),
      nb_(nullptr),
      rb_(nullptr),
      tb_(nullptr),
      ip_(nullptr),
      np_(nullptr),
      initialPlaceMaxIter_(20),
      initialPlaceMinDiffLength_(1500),
      initialPlaceMaxSolverIter_(100),
      initialPlaceMaxFanout_(200),
      initialPlaceNetWeightScale_(800),
      forceCPU_(false),
      nesterovPlaceMaxIter_(5000),
      binGridCntX_(0),
      binGridCntY_(0),
      overflow_(0.1),
      density_(1.0),
      initDensityPenalityFactor_(0.00008),
      initWireLengthCoef_(0.25),
      minPhiCoef_(0.95),
      maxPhiCoef_(1.05),
      referenceHpwl_(446000000),
      routabilityCheckOverflow_(0.20),
      routabilityMaxDensity_(0.99),
      routabilityTargetRcMetric_(1.25),
      routabilityInflationRatioCoef_(2.5),
      routabilityMaxInflationRatio_(2.5),
      routabilityRcK1_(1.0),
      routabilityRcK2_(1.0),
      routabilityRcK3_(0.0),
      routabilityRcK4_(0.0),
      routabilityMaxBloatIter_(1),
      routabilityMaxInflationIter_(4),
      timingNetWeightMax_(1.9),
      timingDrivenMode_(true),
      routabilityDrivenMode_(true),
      uniformTargetDensityMode_(false),
      skipIoMode_(false),
      padLeft_(0),
      padRight_(0),
      gui_debug_(false),
      gui_debug_pause_iterations_(10),
      gui_debug_update_iterations_(10),
      gui_debug_draw_bins_(false),
      gui_debug_initial_(false),
      gui_debug_inst_(nullptr){};

Replace::~Replace()
{
}

void Replace::init(odb::dbDatabase* odb,
                   rsz::Resizer* resizer,
                   grt::GlobalRouter* router,
                   utl::Logger* logger)
{
  db_ = odb;
  rs_ = resizer;
  fr_ = router;
  log_ = logger;
}

void Replace::reset()
{
  ip_.reset();
  np_.reset();

  pb_.reset();
  nb_.reset();
  tb_.reset();
  rb_.reset();

  initialPlaceMaxIter_ = 20;
  initialPlaceMinDiffLength_ = 1500;
  initialPlaceMaxSolverIter_ = 100;
  initialPlaceMaxFanout_ = 200;
  initialPlaceNetWeightScale_ = 800;
  forceCPU_ = false;

  nesterovPlaceMaxIter_ = 5000;
  binGridCntX_ = binGridCntY_ = 0;
  overflow_ = 0.1;
  density_ = 1.0;
  initDensityPenalityFactor_ = 0.00008;
  initWireLengthCoef_ = 0.25;
  minPhiCoef_ = 0.95;
  maxPhiCoef_ = 1.05;
  referenceHpwl_ = 446000000;

  routabilityCheckOverflow_ = 0.20;
  routabilityMaxDensity_ = 0.99;
  routabilityTargetRcMetric_ = 1.25;
  routabilityInflationRatioCoef_ = 2.5;
  routabilityMaxInflationRatio_ = 2.5;
  routabilityRcK1_ = routabilityRcK2_ = 1.0;
  routabilityRcK3_ = routabilityRcK4_ = 0.0;
  routabilityMaxBloatIter_ = 1;
  routabilityMaxInflationIter_ = 4;

  timingDrivenMode_ = true;
  routabilityDrivenMode_ = true;
  uniformTargetDensityMode_ = false;
  skipIoMode_ = false;

  padLeft_ = padRight_ = 0;

  timingNetWeightOverflows_.clear();
  timingNetWeightOverflows_.shrink_to_fit();
  timingNetWeightMax_ = 1.9;

  gui_debug_ = false;
  gui_debug_pause_iterations_ = 10;
  gui_debug_update_iterations_ = 10;
  gui_debug_draw_bins_ = false;
  gui_debug_initial_ = false;
}

void Replace::doIncrementalPlace()
{
  PlacerBaseVars pbVars;
  pbVars.padLeft = padLeft_;
  pbVars.padRight = padRight_;
  pbVars.skipIoMode = skipIoMode_;

  pb_ = std::make_shared<PlacerBase>(db_, pbVars, log_);

  // Lock down already placed objects
  int locked_cnt = 0;
  int unplaced_cnt = 0;
  auto block = db_->getChip()->getBlock();
  for (auto inst : block->getInsts()) {
    auto status = inst->getPlacementStatus();
    if (status == odb::dbPlacementStatus::PLACED) {
      pb_->dbToPb(inst)->lock();
      ++locked_cnt;
    } else if (!status.isPlaced()) {
      ++unplaced_cnt;
    }
  }

  if (unplaced_cnt == 0) {
    // Everything was already placed so we do the old incremental mode
    // which just skips initial placement and runs nesterov.
    pb_->unlockAll();
    doNesterovPlace();
    return;
  }

  log_->info(GPL, 132, "Locked {} instances", locked_cnt);

  // Roughly place the unplaced objects (allow more overflow).
  // Limit iterations to prevent objects drifting too far or
  // non-convergence.
  constexpr float rough_oveflow = 0.2f;
  float previous_overflow = overflow_;
  setTargetOverflow(std::max(rough_oveflow, overflow_));
  doInitialPlace();

  int previous_max_iter = nesterovPlaceMaxIter_;
  initNesterovPlace();
  setNesterovPlaceMaxIter(300);
  int iter = doNesterovPlace();
  setNesterovPlaceMaxIter(previous_max_iter);

  // Finish the overflow resolution from the rough placement
  log_->info(GPL, 133, "Unlocked instances");
  pb_->unlockAll();

  setTargetOverflow(previous_overflow);
  if (previous_overflow < rough_oveflow) {
    doNesterovPlace(iter + 1);
  }
}

void Replace::doInitialPlace()
{
  if (pb_ == nullptr) {
    PlacerBaseVars pbVars;
    pbVars.padLeft = padLeft_;
    pbVars.padRight = padRight_;
    pbVars.skipIoMode = skipIoMode_;

    pb_ = std::make_shared<PlacerBase>(db_, pbVars, log_);
  }

  InitialPlaceVars ipVars;
  ipVars.maxIter = initialPlaceMaxIter_;
  ipVars.minDiffLength = initialPlaceMinDiffLength_;
  ipVars.maxSolverIter = initialPlaceMaxSolverIter_;
  ipVars.maxFanout = initialPlaceMaxFanout_;
  ipVars.netWeightScale = initialPlaceNetWeightScale_;
  ipVars.debug = gui_debug_initial_;
  ipVars.forceCPU = forceCPU_;

  std::unique_ptr<InitialPlace> ip(new InitialPlace(ipVars, pb_, log_));
  ip_ = std::move(ip);
  ip_->doBicgstabPlace();
}

bool Replace::initNesterovPlace()
{
  if (!pb_) {
    PlacerBaseVars pbVars;
    pbVars.padLeft = padLeft_;
    pbVars.padRight = padRight_;
    pbVars.skipIoMode = skipIoMode_;

    pb_ = std::make_shared<PlacerBase>(db_, pbVars, log_);
  }

  if (pb_->placeInsts().size() == 0) {
    log_->warn(GPL, 136, "No placeable instances - skipping placement.");
    return false;
  }

  if (!nb_) {
    NesterovBaseVars nbVars;
    nbVars.targetDensity = density_;

    if (binGridCntX_ != 0 && binGridCntY_ != 0) {
      nbVars.isSetBinCnt = 1;
      nbVars.binCntX = binGridCntX_;
      nbVars.binCntY = binGridCntY_;
    }

    nbVars.useUniformTargetDensity = uniformTargetDensityMode_;

    nb_ = std::make_shared<NesterovBase>(nbVars, pb_, log_);
  }

  if (!rb_) {
    RouteBaseVars rbVars;
    rbVars.maxDensity = routabilityMaxDensity_;
    rbVars.maxBloatIter = routabilityMaxBloatIter_;
    rbVars.maxInflationIter = routabilityMaxInflationIter_;
    rbVars.targetRC = routabilityTargetRcMetric_;
    rbVars.inflationRatioCoef = routabilityInflationRatioCoef_;
    rbVars.maxInflationRatio = routabilityMaxInflationRatio_;
    rbVars.rcK1 = routabilityRcK1_;
    rbVars.rcK2 = routabilityRcK2_;
    rbVars.rcK3 = routabilityRcK3_;
    rbVars.rcK4 = routabilityRcK4_;

    rb_ = std::make_shared<RouteBase>(rbVars, db_, fr_, nb_, log_);
  }

  if (!tb_) {
    tb_ = std::make_shared<TimingBase>(nb_, rs_, log_);
    tb_->setTimingNetWeightOverflows(timingNetWeightOverflows_);
    tb_->setTimingNetWeightMax(timingNetWeightMax_);
  }

  if (!np_) {
    NesterovPlaceVars npVars;

    npVars.minPhiCoef = minPhiCoef_;
    npVars.maxPhiCoef = maxPhiCoef_;
    npVars.referenceHpwl = referenceHpwl_;
    npVars.routabilityCheckOverflow = routabilityCheckOverflow_;
    npVars.initDensityPenalty = initDensityPenalityFactor_;
    npVars.initWireLengthCoef = initWireLengthCoef_;
    npVars.targetOverflow = overflow_;
    npVars.maxNesterovIter = nesterovPlaceMaxIter_;
    npVars.timingDrivenMode = timingDrivenMode_;
    npVars.routabilityDrivenMode = routabilityDrivenMode_;
    npVars.debug = gui_debug_;
    npVars.debug_pause_iterations = gui_debug_pause_iterations_;
    npVars.debug_update_iterations = gui_debug_update_iterations_;
    npVars.debug_draw_bins = gui_debug_draw_bins_;
    npVars.debug_inst = gui_debug_inst_;

    std::unique_ptr<NesterovPlace> np(
        new NesterovPlace(npVars, pb_, nb_, rb_, tb_, log_));
    np_ = std::move(np);
  }
  return true;
}

int Replace::doNesterovPlace(int start_iter)
{
  if (!initNesterovPlace()) {
    return 0;
  }
  if (timingDrivenMode_)
    rs_->resizeSlackPreamble();
  return np_->doNesterovPlace(start_iter);
}

void Replace::setInitialPlaceMaxIter(int iter)
{
  initialPlaceMaxIter_ = iter;
}

void Replace::setInitialPlaceMinDiffLength(int length)
{
  initialPlaceMinDiffLength_ = length;
}

void Replace::setInitialPlaceMaxSolverIter(int iter)
{
  initialPlaceMaxSolverIter_ = iter;
}

void Replace::setInitialPlaceMaxFanout(int fanout)
{
  initialPlaceMaxFanout_ = fanout;
}

void Replace::setInitialPlaceNetWeightScale(float scale)
{
  initialPlaceNetWeightScale_ = scale;
}

void Replace::setNesterovPlaceMaxIter(int iter)
{
  nesterovPlaceMaxIter_ = iter;
  if (np_) {
    np_->setMaxIters(iter);
  }
}

void Replace::setBinGridCnt(int binGridCntX, int binGridCntY)
{
  binGridCntX_ = binGridCntX;
  binGridCntY_ = binGridCntY;
}

void Replace::setTargetOverflow(float overflow)
{
  overflow_ = overflow;
  if (np_) {
    np_->setTargetOverflow(overflow);
  }
}

void Replace::setTargetDensity(float density)
{
  density_ = density;
}

void Replace::setUniformTargetDensityMode(bool mode)
{
  uniformTargetDensityMode_ = mode;
}

float Replace::getUniformTargetDensity()
{
  initNesterovPlace();
  return nb_->uniformTargetDensity();
}

void Replace::setInitDensityPenalityFactor(float penaltyFactor)
{
  initDensityPenalityFactor_ = penaltyFactor;
}

void Replace::setInitWireLengthCoef(float coef)
{
  initWireLengthCoef_ = coef;
}

void Replace::setMinPhiCoef(float minPhiCoef)
{
  minPhiCoef_ = minPhiCoef;
}

void Replace::setMaxPhiCoef(float maxPhiCoef)
{
  maxPhiCoef_ = maxPhiCoef;
}

void Replace::setReferenceHpwl(float refHpwl)
{
  referenceHpwl_ = refHpwl;
}

void Replace::setDebug(int pause_iterations,
                       int update_iterations,
                       bool draw_bins,
                       bool initial,
                       odb::dbInst* inst)
{
  gui_debug_ = true;
  gui_debug_pause_iterations_ = pause_iterations;
  gui_debug_update_iterations_ = update_iterations;
  gui_debug_draw_bins_ = draw_bins;
  gui_debug_initial_ = initial;
  gui_debug_inst_ = inst;
}

void Replace::setSkipIoMode(bool mode)
{
  skipIoMode_ = mode;
}

void Replace::setForceCPU(bool force_cpu)
{
  forceCPU_ = force_cpu;
}

void Replace::setTimingDrivenMode(bool mode)
{
  timingDrivenMode_ = mode;
}

void Replace::setRoutabilityDrivenMode(bool mode)
{
  routabilityDrivenMode_ = mode;
}

void Replace::setRoutabilityCheckOverflow(float overflow)
{
  routabilityCheckOverflow_ = overflow;
}

void Replace::setRoutabilityMaxDensity(float density)
{
  routabilityMaxDensity_ = density;
}

void Replace::setRoutabilityMaxBloatIter(int iter)
{
  routabilityMaxBloatIter_ = iter;
}

void Replace::setRoutabilityMaxInflationIter(int iter)
{
  routabilityMaxInflationIter_ = iter;
}

void Replace::setRoutabilityTargetRcMetric(float rc)
{
  routabilityTargetRcMetric_ = rc;
}

void Replace::setRoutabilityInflationRatioCoef(float coef)
{
  routabilityInflationRatioCoef_ = coef;
}

void Replace::setRoutabilityMaxInflationRatio(float ratio)
{
  routabilityMaxInflationRatio_ = ratio;
}

void Replace::setRoutabilityRcCoefficients(float k1,
                                           float k2,
                                           float k3,
                                           float k4)
{
  routabilityRcK1_ = k1;
  routabilityRcK2_ = k2;
  routabilityRcK3_ = k3;
  routabilityRcK4_ = k4;
}

void Replace::setPadLeft(int pad)
{
  padLeft_ = pad;
}

void Replace::setPadRight(int pad)
{
  padRight_ = pad;
}

void Replace::addTimingNetWeightOverflow(int overflow)
{
  timingNetWeightOverflows_.push_back(overflow);
}

void Replace::setTimingNetWeightMax(float max)
{
  timingNetWeightMax_ = max;
}

}  // namespace gpl
