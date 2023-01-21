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
#include "utl/validation.h"

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
      np_(nullptr){};

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
}

void Replace::doIncrementalPlace()
{
  PlacerBaseVars pbVars;
  pbVars.padLeft = options_.getPadLeft();
  pbVars.padRight = options_.getPadRight();
  pbVars.skipIoMode = options_.getSkipIoMode();

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

  doInitialPlace();

  // Roughly place the unplaced objects (allow more overflow).
  // Limit iterations to prevent objects drifting too far or
  // non-convergence.
  constexpr float rough_oveflow = 0.2f;
  const float previous_overflow = options_.getTargetOverflow();
  const int previous_max_iter = options_.getNesterovPlaceMaxIter();
  initNesterovPlace();
  options_.setTargetOverflow(std::max(rough_oveflow, previous_overflow));
  np_->setTargetOverflow(std::max(rough_oveflow, previous_overflow));
  options_.setNesterovPlaceMaxIter(300);
  np_->setMaxIters(300);
  int iter = doNesterovPlace();
  options_.setNesterovPlaceMaxIter(previous_max_iter);
  np_->setMaxIters(previous_max_iter);

  // Finish the overflow resolution from the rough placement
  log_->info(GPL, 133, "Unlocked instances");
  pb_->unlockAll();

  options_.setTargetOverflow(previous_overflow);
  np_->setTargetOverflow(previous_overflow);
  if (previous_overflow < rough_oveflow) {
    doNesterovPlace(iter + 1);
  }
}

void Replace::doInitialPlace()
{
  if (pb_ == nullptr) {
    PlacerBaseVars pbVars;
    pbVars.padLeft = options_.getPadLeft();
    pbVars.padRight = options_.getPadRight();
    pbVars.skipIoMode = options_.getSkipIoMode();

    pb_ = std::make_shared<PlacerBase>(db_, pbVars, log_);
  }

  InitialPlaceVars ipVars;
  ipVars.maxIter = options_.getInitialPlaceMaxIter();
  ipVars.minDiffLength = options_.getInitialPlaceMinDiffLength();
  ipVars.maxSolverIter = options_.getInitialPlaceMaxSolverIter();
  ipVars.maxFanout = options_.getInitialPlaceMaxFanout();
  ipVars.netWeightScale = options_.getInitialPlaceNetWeightScale();
  ipVars.debug = options_.getGuiDebugInitial();
  ipVars.forceCPU = options_.getForceCpu();

  std::unique_ptr<InitialPlace> ip(new InitialPlace(ipVars, pb_, log_));
  ip_ = std::move(ip);
  ip_->doBicgstabPlace();
}

bool Replace::initNesterovPlace()
{
  if (!pb_) {
    PlacerBaseVars pbVars;
    pbVars.padLeft = options_.getPadLeft();
    pbVars.padRight = options_.getPadRight();
    pbVars.skipIoMode = options_.getSkipIoMode();

    pb_ = std::make_shared<PlacerBase>(db_, pbVars, log_);
  }

  if (pb_->placeInsts().size() == 0) {
    log_->warn(GPL, 136, "No placeable instances - skipping placement.");
    return false;
  }

  if (!nb_) {
    NesterovBaseVars nbVars;
    nbVars.targetDensity = options_.getTargetDensity();

    if (options_.getBinGridCntX() != 0) {
      nbVars.isSetBinCntX = 1;
      nbVars.binCntX = options_.getBinGridCntX();
    }

    if (options_.getBinGridCntY() != 0) {
      nbVars.isSetBinCntY = 1;
      nbVars.binCntY = options_.getBinGridCntY();
    }

    nbVars.useUniformTargetDensity = options_.getUniformTargetDensityMode();

    nb_ = std::make_shared<NesterovBase>(nbVars, pb_, log_);
  }

  if (!rb_) {
    RouteBaseVars rbVars;
    rbVars.maxDensity = options_.getRoutabilityMaxDensity();
    rbVars.maxBloatIter = options_.getRoutabilityMaxBloatIter();
    rbVars.maxInflationIter = options_.getRoutabilityMaxInflationIter();
    rbVars.targetRC = options_.getRoutabilityTargetRcMetric();
    rbVars.inflationRatioCoef = options_.getRoutabilityInflationRatioCoef();
    rbVars.maxInflationRatio = options_.getRoutabilityMaxInflationRatio();
    rbVars.rcK1 = options_.getRoutabilityRcK1();
    rbVars.rcK2 = options_.getRoutabilityRcK2();
    rbVars.rcK3 = options_.getRoutabilityRcK3();
    rbVars.rcK4 = options_.getRoutabilityRcK4();

    rb_ = std::make_shared<RouteBase>(rbVars, db_, fr_, nb_, log_);
  }

  if (!tb_) {
    tb_ = std::make_shared<TimingBase>(nb_, rs_, log_);
    tb_->setTimingNetWeightOverflows(options_.getTimingNetWeightOverflows());
    tb_->setTimingNetWeightMax(options_.getTimingNetWeightMax());
  }

  if (!np_) {
    NesterovPlaceVars npVars;

    npVars.minPhiCoef = options_.getMinPhiCoef();
    npVars.maxPhiCoef = options_.getMaxPhiCoef();
    npVars.referenceHpwl = options_.getReferenceHpwl();
    npVars.routabilityCheckOverflow = options_.getRoutabilityCheckOverflow();
    npVars.initDensityPenalty = options_.getInitDensityPenalityFactor();
    npVars.initWireLengthCoef = options_.getInitWireLengthCoef();
    npVars.targetOverflow = options_.getTargetOverflow();
    npVars.maxNesterovIter = options_.getNesterovPlaceMaxIter();
    npVars.timingDrivenMode = options_.getTimingDrivenMode();
    npVars.routabilityDrivenMode = options_.getRoutabilityDrivenMode();
    npVars.debug = options_.getGuiDebug();
    npVars.debug_pause_iterations = options_.getGuiDebugPauseIterations();
    npVars.debug_update_iterations = options_.getGuiDebugUpdateIterations();
    npVars.debug_draw_bins = options_.getGuiDebugDrawBins();
    npVars.debug_inst = options_.getGuiDebugInst();

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
  if (options_.getTimingDrivenMode())
    rs_->resizeSlackPreamble();
  return np_->doNesterovPlace(start_iter);
}

void Replace::place(const ReplaceOptions& options)
{
  validate(options);
  options_ = options;
  if (options.getIncremental()) {
    doIncrementalPlace();
  } else {
    doInitialPlace();
    if (options.getDoNesterovPlace()) {
      doNesterovPlace();
    }
  }
}

float Replace::getUniformTargetDensity(const ReplaceOptions& options)
{
  validate(options);
  options_ = options;
  options_.setTargetDensity(1.0);
  initNesterovPlace();
  return nb_->uniformTargetDensity();
}

void Replace::validate(const ReplaceOptions& options)
{
  utl::Validator validator(log_, GPL);
  validator.check_non_negative(
      "initialPlaceMaxIter", options.getInitialPlaceMaxIter(), 137);

  validator.check_non_negative(
      "initialPlaceMaxFanout", options.getInitialPlaceMaxFanout(), 138);

  validator.check_range(
      "targetDensity", options.getTargetDensity(), 0.0f, 1.0f, 139);

  validator.check_non_negative(
      "routabilityMaxDensity", options.getRoutabilityMaxDensity(), 138);

  validator.check_non_negative("minPhiCoef", options.getMinPhiCoef(), 140);

  validator.check_non_negative("maxPhiCoef", options.getMaxPhiCoef(), 141);

  validator.check_non_negative(
      "timingNetWeightMax", options.getTimingNetWeightMax(), 142);

  validator.check_non_negative(
      "timingNetWeightMax", options.getTimingNetWeightMax(), 143);
}

}  // namespace gpl
