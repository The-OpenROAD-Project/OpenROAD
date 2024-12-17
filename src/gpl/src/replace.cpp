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

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "initialPlace.h"
#include "mbff.h"
#include "nesterovBase.h"
#include "nesterovPlace.h"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "placerBase.h"
#include "routeBase.h"
#include "rsz/Resizer.hh"
#include "sta/StaMain.hh"
#include "timingBase.h"
#include "utl/Logger.h"

namespace gpl {

using utl::GPL;

Replace::Replace() = default;

Replace::~Replace() = default;

void Replace::init(odb::dbDatabase* odb,
                   sta::dbSta* sta,
                   rsz::Resizer* resizer,
                   grt::GlobalRouter* router,
                   utl::Logger* logger)
{
  db_ = odb;
  sta_ = sta;
  rs_ = resizer;
  fr_ = router;
  log_ = logger;
}

void Replace::reset()
{
  ip_.reset();
  np_.reset();

  pbc_.reset();
  nbc_.reset();

  pbVec_.clear();
  pbVec_.shrink_to_fit();
  nbVec_.clear();
  nbVec_.shrink_to_fit();

  tb_.reset();
  rb_.reset();

  initialPlaceMaxIter_ = 20;
  initialPlaceMinDiffLength_ = 1500;
  initialPlaceMaxSolverIter_ = 100;
  initialPlaceMaxFanout_ = 200;
  initialPlaceNetWeightScale_ = 800;

  nesterovPlaceMaxIter_ = 5000;
  binGridCntX_ = binGridCntY_ = 0;
  overflow_ = 0.1;
  density_ = 1.0;
  initDensityPenalityFactor_ = 0.00008;
  initWireLengthCoef_ = 0.25;
  minPhiCoef_ = 0.95;
  maxPhiCoef_ = 1.05;
  referenceHpwl_ = 446000000;

  routabilityCheckOverflow_ = 0.3;
  routabilityMaxDensity_ = 0.99;
  routabilityTargetRcMetric_ = 1.01;
  routabilityInflationRatioCoef_ = 5;
  routabilityMaxInflationRatio_ = 8;
  routabilityRcK1_ = routabilityRcK2_ = 1.0;
  routabilityRcK3_ = routabilityRcK4_ = 0.0;
  routabilityMaxInflationIter_ = 4;

  timingDrivenMode_ = true;
  keepResizeBelowOverflow_ = 0.3;
  routabilityDrivenMode_ = true;
  routabilityUseRudy_ = true;
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

void Replace::doIncrementalPlace(int threads)
{
  if (pbc_ == nullptr) {
    PlacerBaseVars pbVars;
    pbVars.padLeft = padLeft_;
    pbVars.padRight = padRight_;
    pbVars.skipIoMode = skipIoMode_;

    pbc_ = std::make_shared<PlacerBaseCommon>(db_, pbVars, log_);

    pbVec_.push_back(std::make_shared<PlacerBase>(db_, pbc_, log_));

    for (auto pd : db_->getChip()->getBlock()->getPowerDomains()) {
      if (pd->getGroup()) {
        pbVec_.push_back(
            std::make_shared<PlacerBase>(db_, pbc_, log_, pd->getGroup()));
      }
    }

    total_placeable_insts_ = 0;
    for (const auto& pb : pbVec_) {
      total_placeable_insts_ += pb->placeInsts().size();
    }
  }

  // Lock down already placed objects
  int locked_cnt = 0;
  int unplaced_cnt = 0;
  auto block = db_->getChip()->getBlock();
  for (auto inst : block->getInsts()) {
    auto status = inst->getPlacementStatus();
    if (status == odb::dbPlacementStatus::PLACED) {
      pbc_->dbToPb(inst)->lock();
      ++locked_cnt;
    } else if (!status.isPlaced()) {
      ++unplaced_cnt;
    }
  }

  if (unplaced_cnt == 0) {
    // Everything was already placed so we do the old incremental mode
    // which just skips initial placement and runs nesterov.
    for (auto& pb : pbVec_) {
      pb->unlockAll();
    }
    // pbc_->unlockAll();
    doNesterovPlace(threads);
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
  initNesterovPlace(threads);
  setNesterovPlaceMaxIter(300);
  int iter = doNesterovPlace(threads);
  setNesterovPlaceMaxIter(previous_max_iter);

  // Finish the overflow resolution from the rough placement
  log_->info(GPL, 133, "Unlocked instances");
  for (auto& pb : pbVec_) {
    pb->unlockAll();
  }

  setTargetOverflow(previous_overflow);
  if (previous_overflow < rough_oveflow) {
    doNesterovPlace(threads, iter + 1);
  }
}

void Replace::doInitialPlace()
{
  if (pbc_ == nullptr) {
    PlacerBaseVars pbVars;
    pbVars.padLeft = padLeft_;
    pbVars.padRight = padRight_;
    pbVars.skipIoMode = skipIoMode_;

    pbc_ = std::make_shared<PlacerBaseCommon>(db_, pbVars, log_);

    pbVec_.push_back(std::make_shared<PlacerBase>(db_, pbc_, log_));

    for (auto pd : db_->getChip()->getBlock()->getPowerDomains()) {
      if (pd->getGroup()) {
        pbVec_.push_back(
            std::make_shared<PlacerBase>(db_, pbc_, log_, pd->getGroup()));
      }
    }

    if (pbVec_.front()->placeInsts().empty()) {
      pbVec_.erase(pbVec_.begin());
    }

    total_placeable_insts_ = 0;
    for (const auto& pb : pbVec_) {
      total_placeable_insts_ += pb->placeInsts().size();
    }
  }

  InitialPlaceVars ipVars;
  ipVars.maxIter = initialPlaceMaxIter_;
  ipVars.minDiffLength = initialPlaceMinDiffLength_;
  ipVars.maxSolverIter = initialPlaceMaxSolverIter_;
  ipVars.maxFanout = initialPlaceMaxFanout_;
  ipVars.netWeightScale = initialPlaceNetWeightScale_;
  ipVars.debug = gui_debug_initial_;

  std::unique_ptr<InitialPlace> ip(
      new InitialPlace(ipVars, pbc_, pbVec_, log_));
  ip_ = std::move(ip);
  ip_->doBicgstabPlace();
}

void Replace::runMBFF(int max_sz,
                      float alpha,
                      float beta,
                      int threads,
                      int num_paths)
{
  MBFF pntset(db_, sta_, log_, threads, 20, num_paths, gui_debug_);
  pntset.Run(max_sz, alpha, beta);
}

bool Replace::initNesterovPlace(int threads)
{
  if (!pbc_) {
    PlacerBaseVars pbVars;
    pbVars.padLeft = padLeft_;
    pbVars.padRight = padRight_;
    pbVars.skipIoMode = skipIoMode_;

    pbc_ = std::make_shared<PlacerBaseCommon>(db_, pbVars, log_);

    pbVec_.push_back(std::make_shared<PlacerBase>(db_, pbc_, log_));

    for (auto pd : db_->getChip()->getBlock()->getPowerDomains()) {
      if (pd->getGroup()) {
        pbVec_.push_back(
            std::make_shared<PlacerBase>(db_, pbc_, log_, pd->getGroup()));
      }
    }

    total_placeable_insts_ = 0;
    for (const auto& pb : pbVec_) {
      total_placeable_insts_ += pb->placeInsts().size();
    }
  }

  if (total_placeable_insts_ == 0) {
    log_->warn(GPL, 136, "No placeable instances - skipping placement.");
    return false;
  }

  if (!nbc_) {
    NesterovBaseVars nbVars;
    nbVars.targetDensity = density_;

    if (binGridCntX_ != 0 && binGridCntY_ != 0) {
      nbVars.isSetBinCnt = true;
      nbVars.binCntX = binGridCntX_;
      nbVars.binCntY = binGridCntY_;
    }

    nbVars.useUniformTargetDensity = uniformTargetDensityMode_;

    nbc_ = std::make_shared<NesterovBaseCommon>(nbVars, pbc_, log_, threads);

    for (const auto& pb : pbVec_) {
      nbVec_.push_back(std::make_shared<NesterovBase>(nbVars, pb, nbc_, log_));
    }
  }

  if (!rb_) {
    RouteBaseVars rbVars;
    rbVars.useRudy = routabilityUseRudy_;
    rbVars.maxDensity = routabilityMaxDensity_;
    rbVars.maxInflationIter = routabilityMaxInflationIter_;
    rbVars.targetRC = routabilityTargetRcMetric_;
    rbVars.inflationRatioCoef = routabilityInflationRatioCoef_;
    rbVars.maxInflationRatio = routabilityMaxInflationRatio_;
    rbVars.rcK1 = routabilityRcK1_;
    rbVars.rcK2 = routabilityRcK2_;
    rbVars.rcK3 = routabilityRcK3_;
    rbVars.rcK4 = routabilityRcK4_;

    rb_ = std::make_shared<RouteBase>(rbVars, db_, fr_, nbc_, nbVec_, log_);
  }

  if (!tb_) {
    tb_ = std::make_shared<TimingBase>(nbc_, rs_, log_);
    tb_->setTimingNetWeightOverflows(timingNetWeightOverflows_);
    tb_->setTimingNetWeightMax(timingNetWeightMax_);
  }

  if (!np_) {
    NesterovPlaceVars npVars;

    npVars.minPhiCoef = minPhiCoef_;
    npVars.maxPhiCoef = maxPhiCoef_;
    npVars.referenceHpwl = referenceHpwl_;
    npVars.routabilityCheckOverflow = routabilityCheckOverflow_;
    npVars.keepResizeBelowOverflow = keepResizeBelowOverflow_;
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

    for (const auto& nb : nbVec_) {
      nb->setNpVars(&npVars);
    }

    std::unique_ptr<NesterovPlace> np(
        new NesterovPlace(npVars, pbc_, nbc_, pbVec_, nbVec_, rb_, tb_, log_));

    np_ = std::move(np);
  }
  return true;
}

int Replace::doNesterovPlace(int threads, int start_iter)
{
  if (!initNesterovPlace(threads)) {
    return 0;
  }
  if (timingDrivenMode_) {
    rs_->resizeSlackPreamble();
  }
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

float Replace::getUniformTargetDensity(int threads)
{
  // TODO: update to be compatible with multiple target densities
  if (initNesterovPlace(threads)) {
    return nbVec_[0]->uniformTargetDensity();
  }
  return 1;
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

void Replace::setTimingDrivenMode(bool mode)
{
  timingDrivenMode_ = mode;
}

void Replace::setRoutabilityDrivenMode(bool mode)
{
  routabilityDrivenMode_ = mode;
}

void Replace::setRoutabilityUseGrt(bool mode)
{
  routabilityUseRudy_ = !mode;
}

void Replace::setRoutabilityCheckOverflow(float overflow)
{
  routabilityCheckOverflow_ = overflow;
}

void Replace::setKeepResizeBelowOverflow(float overflow)
{
  keepResizeBelowOverflow_ = overflow;
}

void Replace::setRoutabilityMaxDensity(float density)
{
  routabilityMaxDensity_ = density;
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
