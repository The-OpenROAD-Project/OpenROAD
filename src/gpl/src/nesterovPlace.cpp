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

// Debug controls: npinit, updateGrad, np, updateNextIter

#include "nesterovPlace.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "graphics.h"
#include "nesterovBase.h"
#include "odb/db.h"
#include "placerBase.h"
#include "routeBase.h"
#include "timingBase.h"
#include "utl/Logger.h"

namespace gpl {
using utl::GPL;

NesterovPlace::NesterovPlace() = default;

NesterovPlace::NesterovPlace(const NesterovPlaceVars& npVars,
                             const std::shared_ptr<PlacerBaseCommon>& pbc,
                             const std::shared_ptr<NesterovBaseCommon>& nbc,
                             std::vector<std::shared_ptr<PlacerBase>>& pbVec,
                             std::vector<std::shared_ptr<NesterovBase>>& nbVec,
                             std::shared_ptr<RouteBase> rb,
                             std::shared_ptr<TimingBase> tb,
                             utl::Logger* log)
    : NesterovPlace()
{
  npVars_ = npVars;
  pbc_ = pbc;
  nbc_ = nbc;
  pbVec_ = pbVec;
  nbVec_ = nbVec;
  rb_ = std::move(rb);
  tb_ = std::move(tb);
  log_ = log;

  db_cbk_ = std::make_unique<nesterovDbCbk>(this);
  nbc_->setCbk(db_cbk_.get());
  if (npVars_.timingDrivenMode) {
    db_cbk_->addOwner(pbc_->db()->getChip()->getBlock());
  }

  if (npVars.debug && Graphics::guiActive()) {
    graphics_ = std::make_unique<Graphics>(log_,
                                           this,
                                           pbc,
                                           nbc,
                                           pbVec,
                                           nbVec,
                                           npVars_.debug_draw_bins,
                                           npVars.debug_inst);
  }
  init();
}

NesterovPlace::~NesterovPlace()
{
  reset();
}

void NesterovPlace::updatePrevGradient(const std::shared_ptr<NesterovBase>& nb)
{
  nb->updatePrevGradient(wireLengthCoefX_, wireLengthCoefY_);
  auto wireLengthGradSum_ = nb->getWireLengthGradSum();
  auto densityGradSum_ = nb->getDensityGradSum();

  if (wireLengthGradSum_ == 0
      && recursionCntWlCoef_ < gpl::NesterovPlaceVars::maxRecursionWlCoef) {
    wireLengthCoefX_ *= 0.5;
    wireLengthCoefY_ *= 0.5;
    baseWireLengthCoef_ *= 0.5;
    debugPrint(
        log_,
        GPL,
        "updateGrad",
        1,
        "sum(WL gradient) = 0 detected, trying again with wlCoef: {:g} {:g}",
        wireLengthCoefX_,
        wireLengthCoefY_);

    // update WL forces
    nbc_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

    // recursive call again with smaller wirelength coef
    recursionCntWlCoef_++;
    updatePrevGradient(nb);
    return;
  }

  // divergence detection on
  // Wirelength / density gradient calculation
  if (std::isnan(wireLengthGradSum_) || std::isinf(wireLengthGradSum_)
      || std::isnan(densityGradSum_) || std::isinf(densityGradSum_)) {
    isDiverged_ = true;
    divergeMsg_ = "RePlAce diverged at wire/density gradient Sum.";
    divergeCode_ = 306;
  }
}

void NesterovPlace::updateCurGradient(const std::shared_ptr<NesterovBase>& nb)
{
  nb->updateCurGradient(wireLengthCoefX_, wireLengthCoefY_);
  auto wireLengthGradSum_ = nb->getWireLengthGradSum();
  auto densityGradSum_ = nb->getDensityGradSum();

  if (wireLengthGradSum_ == 0
      && recursionCntWlCoef_ < gpl::NesterovPlaceVars::maxRecursionWlCoef) {
    wireLengthCoefX_ *= 0.5;
    wireLengthCoefY_ *= 0.5;
    baseWireLengthCoef_ *= 0.5;
    debugPrint(
        log_,
        GPL,
        "updateGrad",
        1,
        "sum(WL gradient) = 0 detected, trying again with wlCoef: {:g} {:g}",
        wireLengthCoefX_,
        wireLengthCoefY_);

    // update WL forces
    nbc_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

    // recursive call again with smaller wirelength coef
    recursionCntWlCoef_++;
    updateCurGradient(nb);
    return;
  }

  // divergence detection on
  // Wirelength / density gradient calculation
  if (std::isnan(wireLengthGradSum_) || std::isinf(wireLengthGradSum_)
      || std::isnan(densityGradSum_) || std::isinf(densityGradSum_)) {
    isDiverged_ = true;
    divergeMsg_ = "RePlAce diverged at wire/density gradient Sum.";
    divergeCode_ = 306;
  }
}

void NesterovPlace::updateNextGradient(const std::shared_ptr<NesterovBase>& nb)
{
  nb->updateNextGradient(wireLengthCoefX_, wireLengthCoefY_);

  auto wireLengthGradSum_ = nb->getWireLengthGradSum();
  auto densityGradSum_ = nb->getDensityGradSum();

  if (wireLengthGradSum_ == 0
      && recursionCntWlCoef_ < gpl::NesterovPlaceVars::maxRecursionWlCoef) {
    wireLengthCoefX_ *= 0.5;
    wireLengthCoefY_ *= 0.5;
    baseWireLengthCoef_ *= 0.5;
    debugPrint(
        log_,
        GPL,
        "updateGrad",
        1,
        "sum(WL gradient) = 0 detected, trying again with wlCoef: {:g} {:g}",
        wireLengthCoefX_,
        wireLengthCoefY_);

    // update WL forces
    nbc_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

    // recursive call again with smaller wirelength coef
    recursionCntWlCoef_++;
    updateNextGradient(nb);
    return;
  }

  // divergence detection on
  // Wirelength / density gradient calculation
  if (std::isnan(wireLengthGradSum_) || std::isinf(wireLengthGradSum_)
      || std::isnan(densityGradSum_) || std::isinf(densityGradSum_)) {
    isDiverged_ = true;
    divergeMsg_ = "RePlAce diverged at wire/density gradient Sum.";
    divergeCode_ = 306;
  }
}

void NesterovPlace::init()
{
  // foreach nesterovbase call init
  total_sum_overflow_ = 0;
  float totalBaseWireLengthCoeff = 0;
  for (auto& nb : nbVec_) {
    nb->setNpVars(&npVars_);
    nb->initDensity1();
    total_sum_overflow_ += nb->getSumOverflow();
    totalBaseWireLengthCoeff += nb->getBaseWireLengthCoef();
  }

  average_overflow_ = total_sum_overflow_ / nbVec_.size();
  baseWireLengthCoef_ = totalBaseWireLengthCoeff / nbVec_.size();
  updateWireLengthCoef(average_overflow_);

  nbc_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

  for (auto& nb : nbVec_) {
    // fill in curSLPSumGrads_, curSLPWireLengthGrads_, curSLPDensityGrads_
    updateCurGradient(nb);

    // approximately fill in
    // prevSLPCoordi_ to calculate lc vars
    nb->updateInitialPrevSLPCoordi();

    // bin, FFT, wlen update with prevSLPCoordi.
    nb->updateDensityCenterPrevSLP();
    nb->updateDensityForceBin();
  }

  nbc_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

  for (auto& nb : nbVec_) {
    // update previSumGrads_, prevSLPWireLengthGrads_, prevSLPDensityGrads_
    updatePrevGradient(nb);
  }

  for (auto& nb : nbVec_) {
    auto stepL = nb->initDensity2(wireLengthCoefX_, wireLengthCoefY_);
    if ((std::isnan(stepL) || std::isinf(stepL))
        && recursionCntInitSLPCoef_
               < gpl::NesterovPlaceVars::maxRecursionInitSLPCoef) {
      npVars_.initialPrevCoordiUpdateCoef *= 10;
      debugPrint(log_,
                 GPL,
                 "npinit",
                 1,
                 "steplength = 0 detected. Rerunning Nesterov::init() "
                 "with initPrevSLPCoef {:g}",
                 npVars_.initialPrevCoordiUpdateCoef);
      recursionCntInitSLPCoef_++;
      init();
      break;
    }

    if (std::isnan(stepL) || std::isinf(stepL)) {
      log_->error(
          GPL,
          304,
          "RePlAce diverged at initial iteration with steplength being {}. "
          "Re-run with a smaller init_density_penalty value.",
          stepL);
    }
  }
}

// clear reset
void NesterovPlace::reset()
{
  npVars_.reset();
  log_ = nullptr;

  densityPenaltyStor_.clear();

  densityPenaltyStor_.shrink_to_fit();

  baseWireLengthCoef_ = 0;
  wireLengthCoefX_ = wireLengthCoefY_ = 0;
  prevHpwl_ = 0;
  isDiverged_ = false;
  isRoutabilityNeed_ = true;

  divergeMsg_ = "";
  divergeCode_ = 0;

  recursionCntWlCoef_ = 0;
  recursionCntInitSLPCoef_ = 0;
}

int NesterovPlace::doNesterovPlace(int start_iter)
{
  // if replace diverged in init() function,
  // replace must be skipped.
  if (isDiverged_) {
    log_->error(GPL, divergeCode_, divergeMsg_);
    return 0;
  }

  if (graphics_) {
    graphics_->cellPlot(true);
  }

  // snapshot saving detection
  bool isSnapshotSaved = false;

  // snapshot info
  float snapshotA = 0;
  float snapshotWlCoefX = 0, snapshotWlCoefY = 0;
  bool isDivergeTriedRevert = false;

  // backTracking variable.
  float curA = 1.0;

  for (auto& nb : nbVec_) {
    nb->setIter(start_iter);
    nb->setMaxPhiCoefChanged(false);
    nb->resetMinSumOverflow();
  }

  // Core Nesterov Loop
  int iter = start_iter;
  for (; iter < npVars_.maxNesterovIter; iter++) {
    float prevA = curA;

    // here, prevA is a_(k), curA is a_(k+1)
    // See, the ePlace-MS paper's Algorithm 1
    //
    curA = (1.0 + sqrt(4.0 * prevA * prevA + 1.0)) * 0.5;

    // coeff is (a_k - 1) / ( a_(k+1) ) in paper.
    float coeff = (prevA - 1.0) / curA;

    // Back-Tracking loop
    int numBackTrak = 0;
    for (numBackTrak = 0; numBackTrak < npVars_.maxBackTrack; numBackTrak++) {
      // fill in nextCoordinates with given stepLength_
      for (auto& nb : nbVec_) {
        nb->nesterovUpdateCoordinates(coeff);
      }

      nbc_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

      int numDiverge = 0;
      for (auto& nb : nbVec_) {
        updateNextGradient(nb);
        numDiverge += nb->isDiverged();
      }

      // NaN or inf is detected in WireLength/Density Coef
      if (numDiverge > 0 || isDiverged_) {
        isDiverged_ = true;
        divergeMsg_ = "RePlAce diverged at wire/density gradient Sum.";
        divergeCode_ = 306;
        break;
      }

      int stepLengthLimitOK = 0;
      numDiverge = 0;
      for (auto& nb : nbVec_) {
        stepLengthLimitOK += nb->nesterovUpdateStepLength();
        numDiverge += nb->isDiverged();
      }

      if (numDiverge > 0) {
        isDiverged_ = true;
        divergeMsg_ = "RePlAce diverged at newStepLength.";
        divergeCode_ = 305;
        break;
      }

      if (stepLengthLimitOK != nbVec_.size()) {
        break;
      }
    }

    debugPrint(log_, GPL, "np", 1, "NumBackTrak: {}", numBackTrak + 1);

    // Adjust Phi dynamically for larger designs
    for (auto& nb : nbVec_) {
      nb->nesterovAdjustPhi();
    }

    if (npVars_.maxBackTrack == numBackTrak) {
      debugPrint(log_,
                 GPL,
                 "np",
                 1,
                 "Backtracking limit reached so a small step will be taken");
    }

    if (isDiverged_) {
      break;
    }

    updateNextIter(iter);

    // For JPEG Saving
    // debug

    if (graphics_) {
      bool update
          = (iter == 0 || (iter + 1) % npVars_.debug_update_iterations == 0);
      if (update) {
        bool pause
            = (iter == 0 || (iter + 1) % npVars_.debug_pause_iterations == 0);
        graphics_->cellPlot(pause);
      }
    }

    // timing driven feature
    // if virtual do reweight on timing-critical nets,
    // otherwise keep all modifications by rsz.
    if (npVars_.timingDrivenMode
        && tb_->isTimingNetWeightOverflow(average_overflow_)) {
      // update db's instance location from current density coordinates
      updateDb();

      // Call resizer's estimateRC API to fill in PEX using placed locations,
      // Call sta's API to extract worst timing paths,
      // and update GNet's weights from worst timing paths.
      //
      // See timingBase.cpp in detail
      bool virtual_td_iter
          = (average_overflow_ > npVars_.keepResizeBelowOverflow);

      log_->info(
          GPL, 100, "Timing-driven iteration {}/{}, virtual: {}.", 
          ++npVars_.timingDrivenIterCounter, 
          tb_->getTimingNetWeightOverflowSize(),
          virtual_td_iter);

      log_->info(GPL, 101, "Iter: {}, overflow: {:.3f}, keep rsz at: {}",
          iter,
          average_overflow_,
          npVars_.keepResizeBelowOverflow);

      if (!virtual_td_iter)
        db_cbk_->addOwner(pbc_->db()->getChip()->getBlock());
      else
        db_cbk_->removeOwner();

      auto block = pbc_->db()->getChip()->getBlock();
      bool shouldTdProceed = tb_->updateGNetWeights(virtual_td_iter);

      db_cbk_->printCallCounts();
      db_cbk_->resetCallCounts();

      if (!virtual_td_iter) {
        for (auto& nesterov : nbVec_) {
          nesterov->updateGCellState(wireLengthCoefX_, wireLengthCoefY_);
          // order in routability:
          // 1. change areas
          // 2. set target density with delta area
          // 3. updateareas
          // 4. updateDensitySize

          nesterov->setTargetDensity(
              static_cast<float>(nbc_->getDeltaArea()
                                 + nesterov->nesterovInstsArea()
                                 + nesterov->totalFillerArea())
              / static_cast<float>(nesterov->whiteSpaceArea()));

          log_->info(GPL, 107, "Timing-driven: RSZ delta area:     {}",
                       block->dbuAreaToMicrons(nbc_->getDeltaArea()));
          log_->info(GPL, 108, "Timing-driven: new target density: {}",
                       nesterov->targetDensity());
          nbc_->resetDeltaArea();
          nesterov->updateAreas();
          nesterov->updateDensitySize();
        }
      }

      // problem occured
      // escape timing driven later
      if (!shouldTdProceed) {
        npVars_.timingDrivenMode = false;
      }
    }

    // diverge detection on
    // large max_phi_cof value + large design
    //
    // 1) happen overflow < 20%
    // 2) Hpwl is growing

    int numDiverge = 0;
    for (auto& nb : nbVec_) {
      numDiverge += nb->checkDivergence();
    }

    if (numDiverge > 0) {
      divergeMsg_ = "RePlAce divergence detected. ";
      divergeMsg_ += "Re-run with a smaller max_phi_cof value.";
      divergeCode_ = 307;
      isDiverged_ = true;

      // revert back to the original rb solutions
      // one more opportunity
      if (!isDivergeTriedRevert && rb_->numCall() >= 1) {
        // get back to the working rc size
        rb_->revertGCellSizeToMinRc();

        curA = snapshotA;
        wireLengthCoefX_ = snapshotWlCoefX;
        wireLengthCoefY_ = snapshotWlCoefY;

        nbc_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

        for (auto& nb : nbVec_) {
          nb->revertDivergence();
        }

        isDiverged_ = false;
        divergeCode_ = 0;
        divergeMsg_ = "";
        isDivergeTriedRevert = true;
        // turn off the RD forcely
        isRoutabilityNeed_ = false;
      } else {
        // no way to revert
        break;
      }
    }

    if (!isSnapshotSaved && npVars_.routabilityDrivenMode
        && 0.6 >= average_overflow_unscaled_) {
      snapshotWlCoefX = wireLengthCoefX_;
      snapshotWlCoefY = wireLengthCoefY_;
      snapshotA = curA;
      isSnapshotSaved = true;

      for (auto& nb : nbVec_) {
        nb->snapshot();
      }

      log_->report("[NesterovSolve] Snapshot saved at iter = {}", iter);
    }

    // check routability using GR
    if (npVars_.routabilityDrivenMode && isRoutabilityNeed_
        && npVars_.routabilityCheckOverflow >= average_overflow_unscaled_) {
      // recover the densityPenalty values
      // if further routability-driven is needed
      std::pair<bool, bool> result = rb_->routability();
      isRoutabilityNeed_ = result.first;
      bool isRevertInitNeeded = result.second;

      // if routability is needed
      if (isRoutabilityNeed_ || isRevertInitNeeded) {
        // cutFillerCoordinates();

        // revert back the current density penality
        curA = snapshotA;
        wireLengthCoefX_ = snapshotWlCoefX;
        wireLengthCoefY_ = snapshotWlCoefY;

        nbc_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

        for (auto& nb : nbVec_) {
          nb->revertDivergence();
          nb->resetMinSumOverflow();
        }
        log_->report("[NesterovSolve] Revert back to snapshot coordi");
      }
    }
    // check each for converge and if all are converged then stop
    int numConverge = 0;
    for (auto& nb : nbVec_) {
      numConverge += nb->checkConvergence();
    }

    if (numConverge == nbVec_.size()) {
      // log_->report("[NesterovSolve] Finished, all regions converged");
      break;
    }
  }
  // in all case including diverge,
  // db should be updated.
  updateDb();

  if (isDiverged_) {
    log_->error(GPL, divergeCode_, divergeMsg_);
  }

  if (graphics_) {
    graphics_->status("End placement");
    graphics_->cellPlot(true);
  }

  if (db_cbk_ != nullptr) {
    log_->report("NesterovPlace::doNesteroPlace() remove owner");
    db_cbk_->removeOwner();
    db_cbk_ = nullptr;
  }
  return iter;
}

void NesterovPlace::updateWireLengthCoef(float overflow)
{
  if (overflow > 1.0) {
    wireLengthCoefX_ = wireLengthCoefY_ = 0.1;
  } else if (overflow < 0.1) {
    wireLengthCoefX_ = wireLengthCoefY_ = 10.0;
  } else {
    wireLengthCoefX_ = wireLengthCoefY_
        = 1.0 / pow(10.0, (overflow - 0.1) * 20 / 9.0 - 1.0);
  }

  wireLengthCoefX_ *= baseWireLengthCoef_;
  wireLengthCoefY_ *= baseWireLengthCoef_;
  debugPrint(log_, GPL, "np", 1, "NewWireLengthCoef: {:g}", wireLengthCoefX_);
}

void NesterovPlace::updateNextIter(const int iter)
{
  total_sum_overflow_ = 0;
  total_sum_overflow_unscaled_ = 0;

  for (auto& nb : nbVec_) {
    nb->updateNextIter(iter);
    total_sum_overflow_ += nb->getSumOverflow();
    total_sum_overflow_unscaled_ += nb->getSumOverflowUnscaled();
  }

  average_overflow_ = total_sum_overflow_ / nbVec_.size();
  average_overflow_unscaled_ = total_sum_overflow_unscaled_ / nbVec_.size();

  // For coefficient, using average regions' overflow
  updateWireLengthCoef(average_overflow_);
}

void NesterovPlace::updateDb()
{
  nbc_->updateDbGCells();
}

nesterovDbCbk::nesterovDbCbk(NesterovPlace* nesterov_place)
    : nesterov_place_(nesterov_place)
{
}

// If placer base and placer base common also have their vector replaced,
// the gcell parameter is not necessary, and can be accessed with dbToNb()
void NesterovPlace::createGCell(odb::dbInst* db_inst)
{
  auto gcell_index = nbc_->createGCell(db_inst);
  for (auto& nesterov : nbVec_) {
    // TODO: properly manage regions, not every region (NB) should create a
    // gcell.
    nesterov->createGCell(db_inst, gcell_index, rb_.get());
  }
}

void NesterovPlace::destroyGCell(odb::dbInst* db_inst)
{
  for (auto& nesterov : nbVec_) {
    nesterov->destroyGCell(db_inst);
  }
}

void NesterovPlace::createGNet(odb::dbNet* db_net)
{
  // Only process SIGNAL or CLOCK nets, escape nets with VDD/VSS/reset nets
  odb::dbSigType netType = db_net->getSigType();
  if (!nbc_->isValidSigType(netType)) {
    log_->report("db_net:{} is not signal or clock: {}",
                 db_net->getName(),
                 db_net->getSigType().getString());
    return;
  }
  nbc_->createGNet(db_net, pbc_->skipIoMode());
}

void NesterovPlace::destroyGNet(odb::dbNet* db_net)
{
  nbc_->destroyGNet(db_net);
}

void NesterovPlace::createITerm(odb::dbITerm* iterm)
{
  if (!nbc_->isValidSigType(iterm->getSigType())) {
    // log_->report("iterm:{} is not signal or clock:
    // {}",iterm->getName('|'),iterm->getSigType().getString());
    return;
  }
  nbc_->createITerm(iterm);
}

void NesterovPlace::destroyITerm(odb::dbITerm* iterm)
{
  if (!nbc_->isValidSigType(iterm->getSigType())) {
    log_->report("iterm:{} is not signal or clock: {}",
                 iterm->getName('|'),
                 iterm->getSigType().getString());
    return;
  }
  nbc_->destroyITerm(iterm);
}

void NesterovPlace::resizeGCell(odb::dbInst* db_inst)
{
  // log_->report("start resizing cell: {}", db_inst->getName());
  nbc_->resizeGCell(db_inst);
  // log_->report("done resizing cell: {}", db_inst->getName());
}

void NesterovPlace::moveGCell(odb::dbInst* db_inst)
{
  // auto bbox = db_inst->getBBox();
  //  log_->report("db_inst in moveGCell:{}, BBOX:({},{}),
  //  ({},{})",db_inst->getName(),bbox->xMin(),bbox->yMin(),bbox->xMax(),bbox->yMax());
  nbc_->moveGCell(db_inst);
}

void nesterovDbCbk::inDbInstCreate(odb::dbInst* db_inst)
{
  ++inDbInstCreateCount;
  nesterov_place_->createGCell(db_inst);
}

// TODO: actually use the region to create new gcell. Find the region among
// nbVec?
void nesterovDbCbk::inDbInstCreate(odb::dbInst* db_inst, odb::dbRegion* region)
{
  ++inDbInstCreateCount;
}

void nesterovDbCbk::inDbInstSwapMasterAfter(odb::dbInst* db_inst)
{
  ++inDbInstSwapMasterAfterCount;
  // log_.report("Check: inDbInstSwapMasterAfter: {}",db_inst->getName());
  nesterov_place_->resizeGCell(db_inst);
}

void nesterovDbCbk::inDbInstDestroy(odb::dbInst* db_inst)
{
  ++inDbInstDestroyCount;
  // log_.report("Check: inDbInstDestroy: {}",db_inst->getName());
  nesterov_place_->destroyGCell(db_inst);
}

void nesterovDbCbk::inDbITermCreate(odb::dbITerm* iterm)
{
  ++inDbITermCreateCount;
  // log_.report("Check: inDbITermCreate, iterm create:{}",iterm->getName('|'));
  nesterov_place_->createITerm(iterm);
}

void nesterovDbCbk::inDbITermDestroy(odb::dbITerm* iterm)
{
  ++inDbITermDestroyCount;
  // log_.report("Check: inDbITermDestroy,iterm:{}",iterm->getName('|'));
  nesterov_place_->destroyITerm(iterm);
}

// void nesterovDbCbk::inDbITermPreDisconnect(odb::dbITerm* iterm) {
//     ++inDbITermPreDisconnectCount;
//     log_.report("Check: inDbITermPreDisconnect, iterm: {}",
//     iterm->getName('|'));
// }

void nesterovDbCbk::inDbITermPostDisconnect(odb::dbITerm* iterm,
                                            odb::dbNet* net)
{
  ++inDbITermPostDisconnectCount;
  // log_.report("Check: inDbITermPostDisconnect, net: {}, iterm: {}",
  // net->getName(), iterm->getName('|'));
  // nesterov_place_->disconnectIterm(iterm, net);
}

// void nesterovDbCbk::inDbITermPreConnect(odb::dbITerm* iterm, odb::dbNet* net)
// {
//     ++inDbITermPreConnectCount;
//     log_.report("Check: inDbITermPreConnect, net: {}, iterm: {}",
//     net->getName(), iterm->getName('|'));
// }

void nesterovDbCbk::inDbITermPostConnect(odb::dbITerm* iterm)
{
  ++inDbITermPostConnectCount;
  // log_.report("Check: inDbITermPostConnect iterm: {}", iterm->getName('|'));
  // nesterov_place_->connectIterm(iterm);
}

// void nesterovDbCbk::inDbPreMoveInst(odb::dbInst* db_inst) {
//     ++inDbPreMoveInstCount;
//     log_.report("Check: inDbPreMoveInst called");
//     // nesterov_place_->moveGCell(db_inst);
// }

void nesterovDbCbk::inDbPostMoveInst(odb::dbInst* db_inst)
{
  ++inDbPostMoveInstCount;
  // log_.report("Check: inDbPostMoveInst called, inst:{}", db_inst->getName());
  nesterov_place_->moveGCell(db_inst);
}

void nesterovDbCbk::inDbNetCreate(odb::dbNet* db_net)
{
  ++inDbNetCreateCount;
  // log_.report("Check: inDbNetCreate, net:{}", db_net->getName());
  nesterov_place_->createGNet(db_net);
}

void nesterovDbCbk::inDbNetDestroy(odb::dbNet* db_net)
{
  ++inDbNetDestroyCount;
  // log_.report("Check: inDbNetDestroy, net:{}", db_net->getName());
  // nesterov_place_->destroyGNet(db_net);
}

void nesterovDbCbk::inDbNetPreMerge(odb::dbNet* net1, odb::dbNet* net2)
{
  ++inDbNetPreMergeCount;
  // log_.report("Check: inDbNetPreMerge, net1:{}, net2:{}", net1->getName(),
  // net2->getName());
}

void nesterovDbCbk::inDbBTermCreate(odb::dbBTerm* bterm)
{
  ++inDbBTermCreateCount;
  // log_.report("Check: inDbBTermCreate called");
}

void nesterovDbCbk::inDbBTermDestroy(odb::dbBTerm* bterm)
{
  ++inDbBTermDestroyCount;
  // log_.report("Check: inDbBTermDestroy called");
}

void nesterovDbCbk::inDbBTermPreConnect(odb::dbBTerm* bterm, odb::dbNet* net)
{
  ++inDbBTermPreConnectCount;
  // log_.report("Check: inDbBTermPreConnect called");
}

void nesterovDbCbk::inDbBTermPostConnect(odb::dbBTerm* bterm)
{
  ++inDbBTermPostConnectCount;
  // log_.report("Check: inDbBTermPostConnect called");
}

void nesterovDbCbk::inDbBTermPreDisconnect(odb::dbBTerm* bterm)
{
  ++inDbBTermPreDisconnectCount;
  // log_.report("Check: inDbBTermPreDisconnect called");
}

void nesterovDbCbk::inDbBTermPostDisConnect(odb::dbBTerm* bterm,
                                            odb::dbNet* net)
{
  ++inDbBTermPostDisConnectCount;
  // log_.report("Check: inDbBTermPostDisConnect called");
}

void nesterovDbCbk::inDbBTermSetIoType(odb::dbBTerm* bterm,
                                       const odb::dbIoType& ioType)
{
  ++inDbBTermSetIoTypeCount;
  // log_.report("Check: inDbBTermSetIoType called");
}

void nesterovDbCbk::inDbBPinCreate(odb::dbBPin* bpin)
{
  ++inDbBPinCreateCount;
  // log_.report("Check: inDbBPinCreate called");
}

void nesterovDbCbk::inDbBPinDestroy(odb::dbBPin* bpin)
{
  ++inDbBPinDestroyCount;
  // log_.report("Check: inDbBPinDestroy called");
}

void nesterovDbCbk::inDbBlockageCreate(odb::dbBlockage* blockage)
{
  ++inDbBlockageCreateCount;
  // log_.report("Check: inDbBlockageCreate called");
}

void nesterovDbCbk::inDbObstructionCreate(odb::dbObstruction* obstruction)
{
  ++inDbObstructionCreateCount;
  // log_.report("Check: inDbObstructionCreate called");
}

void nesterovDbCbk::inDbObstructionDestroy(odb::dbObstruction* obstruction)
{
  ++inDbObstructionDestroyCount;
  // log_.report("Check: inDbObstructionDestroy called");
}

void nesterovDbCbk::inDbRegionCreate(odb::dbRegion* region)
{
  ++inDbRegionCreateCount;
  // log_.report("Check: inDbRegionCreate called");
}

void nesterovDbCbk::inDbRegionAddBox(odb::dbRegion* region, odb::dbBox* box)
{
  ++inDbRegionAddBoxCount;
  // log_.report("Check: inDbRegionAddBox called");
}

void nesterovDbCbk::inDbRegionDestroy(odb::dbRegion* region)
{
  ++inDbRegionDestroyCount;
  // log_.report("Check: inDbRegionDestroy called");
}

void nesterovDbCbk::inDbRowCreate(odb::dbRow* row)
{
  ++inDbRowCreateCount;
  // log_.report("Check: inDbRowCreate called");
}

void nesterovDbCbk::inDbRowDestroy(odb::dbRow* row)
{
  ++inDbRowDestroyCount;
  // log_.report("Check: inDbRowDestroy called");
}

void nesterovDbCbk::printCallCounts()
{
  log_.report("Call Counts:");
  log_.report("inDbInstCreate: " + std::to_string(inDbInstCreateCount));
  log_.report("inDbInstDestroy: " + std::to_string(inDbInstDestroyCount));
  log_.report("inDbInstSwapMasterAfter: "
              + std::to_string(inDbInstSwapMasterAfterCount));
  log_.report("inDbITermCreate: " + std::to_string(inDbITermCreateCount));
  log_.report("inDbITermDestroy: " + std::to_string(inDbITermDestroyCount));
  log_.report("inDbITermPreDisconnect: "
              + std::to_string(inDbITermPreDisconnectCount));
  log_.report("inDbITermPostDisconnect: "
              + std::to_string(inDbITermPostDisconnectCount));
  log_.report("inDbITermPreConnect: "
              + std::to_string(inDbITermPreConnectCount));
  log_.report("inDbITermPostConnect: "
              + std::to_string(inDbITermPostConnectCount));
  log_.report("inDbPreMoveInst: " + std::to_string(inDbPreMoveInstCount));
  log_.report("inDbPostMoveInst: " + std::to_string(inDbPostMoveInstCount));
  log_.report("inDbNetCreate: " + std::to_string(inDbNetCreateCount));
  log_.report("inDbNetDestroy: " + std::to_string(inDbNetDestroyCount));
  log_.report("inDbNetPreMerge: " + std::to_string(inDbNetPreMergeCount));
  log_.report("inDbBTermCreate: " + std::to_string(inDbBTermCreateCount));
  log_.report("inDbBTermDestroy: " + std::to_string(inDbBTermDestroyCount));
  log_.report("inDbBTermPreConnect: "
              + std::to_string(inDbBTermPreConnectCount));
  log_.report("inDbBTermPostConnect: "
              + std::to_string(inDbBTermPostConnectCount));
  log_.report("inDbBTermPreDisconnect: "
              + std::to_string(inDbBTermPreDisconnectCount));
  log_.report("inDbBTermPostDisConnect: "
              + std::to_string(inDbBTermPostDisConnectCount));
  log_.report("inDbBTermSetIoType: " + std::to_string(inDbBTermSetIoTypeCount));
  log_.report("inDbBPinCreate: " + std::to_string(inDbBPinCreateCount));
  log_.report("inDbBPinDestroy: " + std::to_string(inDbBPinDestroyCount));
  log_.report("inDbBlockageCreate: " + std::to_string(inDbBlockageCreateCount));
  log_.report("inDbObstructionCreate: "
              + std::to_string(inDbObstructionCreateCount));
  log_.report("inDbObstructionDestroy: "
              + std::to_string(inDbObstructionDestroyCount));
  log_.report("inDbRegionCreate: " + std::to_string(inDbRegionCreateCount));
  log_.report("inDbRegionAddBox: " + std::to_string(inDbRegionAddBoxCount));
  log_.report("inDbRegionDestroy: " + std::to_string(inDbRegionDestroyCount));
  log_.report("inDbRowCreate: " + std::to_string(inDbRowCreateCount));
  log_.report("inDbRowDestroy: " + std::to_string(inDbRowDestroyCount));
}

void nesterovDbCbk::resetCallCounts()
{
  inDbInstCreateCount = 0;
  inDbInstDestroyCount = 0;
  inDbInstSwapMasterAfterCount = 0;
  inDbITermCreateCount = 0;
  inDbITermDestroyCount = 0;
  inDbITermPreDisconnectCount = 0;
  inDbITermPostDisconnectCount = 0;
  inDbITermPreConnectCount = 0;
  inDbITermPostConnectCount = 0;
  inDbPreMoveInstCount = 0;
  inDbPostMoveInstCount = 0;
  inDbNetCreateCount = 0;
  inDbNetDestroyCount = 0;
  inDbNetPreMergeCount = 0;
  inDbBTermCreateCount = 0;
  inDbBTermDestroyCount = 0;
  inDbBTermPreConnectCount = 0;
  inDbBTermPostConnectCount = 0;
  inDbBTermPreDisconnectCount = 0;
  inDbBTermPostDisConnectCount = 0;
  inDbBTermSetIoTypeCount = 0;
  inDbBPinCreateCount = 0;
  inDbBPinDestroyCount = 0;
  inDbBlockageCreateCount = 0;
  inDbObstructionCreateCount = 0;
  inDbObstructionDestroyCount = 0;
  inDbRegionCreateCount = 0;
  inDbRegionAddBoxCount = 0;
  inDbRegionDestroyCount = 0;
  inDbRowCreateCount = 0;
  inDbRowDestroyCount = 0;
}

}  // namespace gpl
