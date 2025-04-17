// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

// Debug controls: npinit, updateGrad, np, updateNextIter

#include "nesterovPlace.h"

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

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
                                           npVars.debug_inst,
                                           npVars.debug_start_iter);
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
  float wireLengthGradSum = nb->getWireLengthGradSum();
  float densityGradSum = nb->getDensityGradSum();

  if (wireLengthGradSum == 0
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

  checkInvalidValues(wireLengthGradSum, densityGradSum);
}

void NesterovPlace::updateCurGradient(const std::shared_ptr<NesterovBase>& nb)
{
  nb->updateCurGradient(wireLengthCoefX_, wireLengthCoefY_);
  float wireLengthGradSum = nb->getWireLengthGradSum();
  float densityGradSum = nb->getDensityGradSum();

  if (wireLengthGradSum == 0
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

  checkInvalidValues(wireLengthGradSum, densityGradSum);
}

void NesterovPlace::updateNextGradient(const std::shared_ptr<NesterovBase>& nb)
{
  nb->updateNextGradient(wireLengthCoefX_, wireLengthCoefY_);

  float wireLengthGradSum = nb->getWireLengthGradSum();
  float densityGradSum = nb->getDensityGradSum();

  if (wireLengthGradSum == 0
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
  checkInvalidValues(wireLengthGradSum, densityGradSum);
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

  std::shared_ptr<utl::PrometheusRegistry> registry = log_->getRegistry();
  auto& hpwl_gauge_family
      = utl::BuildGauge()
            .Name("ord_hpwl")
            .Help("The half perimeter wire length of the block")
            .Register(*registry);
  auto& hpwl_gauge = hpwl_gauge_family.Add({});
  hpwl_gauge_ = &hpwl_gauge;

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
  num_region_diverged_ = 0;
  is_routability_need_ = true;

  divergeMsg_ = "";
  divergeCode_ = 0;

  recursionCntWlCoef_ = 0;
  recursionCntInitSLPCoef_ = 0;
}

int NesterovPlace::doNesterovPlace(int start_iter)
{
  // if replace diverged in init() function,
  // replace must be skipped.
  if (num_region_diverged_ > 0) {
    log_->error(GPL, divergeCode_, divergeMsg_);
    return 0;
  }

  if (graphics_ && npVars_.debug_start_iter == 0) {
    graphics_->cellPlot(true);
  }

  // routability snapshot info
  bool is_routability_snapshot_saved = false;
  float route_snapshotA = 0;
  float route_snapshot_WlCoefX = 0, route_snapshot_WlCoefY = 0;
  // bool isDivergeTriedRevert = false;

  // divergence snapshot info
  bool is_diverge_snapshot_saved = false;
  float diverge_snapshot_WlCoefX = 0, diverge_snapshot_WlCoefY = 0;

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

      num_region_diverged_ = 0;
      for (auto& nb : nbVec_) {
        updateNextGradient(nb);
        num_region_diverged_ += nb->isDiverged();
      }

      // NaN or inf is detected in WireLength/Density Coef
      if (num_region_diverged_ > 0) {
        divergeMsg_ = "RePlAce diverged at wire/density gradient Sum.";
        divergeCode_ = 306;
        break;
      }

      int stepLengthLimitOK = 0;
      num_region_diverged_ = 0;
      for (auto& nb : nbVec_) {
        stepLengthLimitOK += nb->nesterovUpdateStepLength();
        num_region_diverged_ += nb->isDiverged();
      }

      if (num_region_diverged_ > 0) {
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

    if (num_region_diverged_ > 0) {
      break;
    }

    updateNextIter(iter);

    // For JPEG Saving
    // debug
    if (npVars_.debug && npVars_.debug_update_db_every_iteration) {
      updateDb();
    }
    const int debug_start_iter = npVars_.debug_start_iter;
    if (graphics_ && (debug_start_iter == 0 || iter + 1 >= debug_start_iter)) {
      bool update
          = (iter == 0 || (iter + 1) % npVars_.debug_update_iterations == 0);
      if (update) {
        bool pause
            = (iter == 0 || (iter + 1) % npVars_.debug_pause_iterations == 0);
        graphics_->cellPlot(pause);
      }
    }

    // timing driven feature
    // if virtual, do reweight on timing-critical nets,
    // otherwise keep all modifications by rsz.
    const bool is_before_routability
        = average_overflow_ > routability_save_snapshot_;
    const bool is_after_routability
        = (average_overflow_ < npVars_.routability_end_overflow
           && !is_routability_need_);
    if (npVars_.timingDrivenMode
        && tb_->isTimingNetWeightOverflow(average_overflow_) &&
        // do not execute timing-driven if routability is under execution
        (is_before_routability || is_after_routability
         || !npVars_.routability_driven_mode)) {
      // update db's instance location from current density coordinates
      updateDb();

      // Call resizer's estimateRC API to fill in PEX using placed locations,
      // Call sta's API to extract worst timing paths,
      // and update GNet's weights from worst timing paths.
      //
      // See timingBase.cpp in detail
      bool virtual_td_iter
          = (average_overflow_ > npVars_.keepResizeBelowOverflow);

      log_->info(GPL,
                 100,
                 "Timing-driven iteration {}/{}, virtual: {}.",
                 ++npVars_.timingDrivenIterCounter,
                 tb_->getTimingNetWeightOverflowSize(),
                 virtual_td_iter);

      log_->info(GPL,
                 101,
                 "   Iter: {}, overflow: {:.3f}, keep resizer changes at: {}, "
                 "HPWL: {}",
                 iter + 1,
                 average_overflow_,
                 npVars_.keepResizeBelowOverflow,
                 nbc_->getHpwl());

      if (!virtual_td_iter) {
        db_cbk_->addOwner(pbc_->db()->getChip()->getBlock());
      } else {
        db_cbk_->removeOwner();
      }

      auto block = pbc_->db()->getChip()->getBlock();
      int nb_total_gcells_delta = 0;
      int nb_gcells_before_td = 0;
      int nb_gcells_after_td = 0;
      int nbc_total_gcells_before_td = nbc_->getNewGcellsCount();

      for (auto& nb : nbVec_) {
        nb_gcells_before_td += nb->gCells().size();
      }

      bool shouldTdProceed = tb_->executeTimingDriven(virtual_td_iter);
      nbVec_[0]->setTrueReprintIterHeader();

      for (auto& nb : nbVec_) {
        nb_gcells_after_td += nb->gCells().size();
      }

      nb_total_gcells_delta = nb_gcells_after_td - nb_gcells_before_td;
      if (nb_total_gcells_delta != nbc_->getNewGcellsCount()) {
        log_->warn(GPL,
                   92,
                   "Mismatch in #cells between central object and all regions. "
                   "NesterovBaseCommon: {}, Summing all regions: {}",
                   nbc_->getNewGcellsCount(),
                   nb_total_gcells_delta);
      }
      if (!virtual_td_iter) {
        for (auto& nesterov : nbVec_) {
          nesterov->updateGCellState(wireLengthCoefX_, wireLengthCoefY_);
          // updates order in routability:
          // 1. change areas
          // 2. set target density with delta area
          // 3. updateareas
          // 4. updateDensitySize

          nesterov->setTargetDensity(
              static_cast<float>(nbc_->getDeltaArea()
                                 + nesterov->nesterovInstsArea()
                                 + nesterov->totalFillerArea())
              / static_cast<float>(nesterov->whiteSpaceArea()));

          float rsz_delta_area_microns
              = block->dbuAreaToMicrons(nbc_->getDeltaArea());
          float rsz_delta_area_percentage
              = (nbc_->getDeltaArea()
                 / static_cast<float>(nesterov->nesterovInstsArea()))
                * 100.0f;
          log_->info(
              GPL,
              107,
              "Timing-driven: repair_design delta area: {:.3f} um^2 ({:+.2f}%)",
              rsz_delta_area_microns,
              rsz_delta_area_percentage);

          float new_gcells_percentage = 0.0f;
          if (nbc_total_gcells_before_td > 0) {
            new_gcells_percentage
                = (nbc_->getNewGcellsCount()
                   / static_cast<float>(nbc_total_gcells_before_td))
                  * 100.0f;
          }
          log_->info(
              GPL,
              108,
              "Timing-driven: repair_design, gpl cells created: {} ({:+.2f}%)",
              nbc_->getNewGcellsCount(),
              new_gcells_percentage);

          if (tb_->repairDesignBufferCount() != nbc_->getNewGcellsCount()) {
            log_->warn(GPL,
                       93,
                       "Buffer insertion count by rsz ({}) and cells created "
                       "by gpl ({}) do not match.",
                       tb_->repairDesignBufferCount(),
                       nbc_->getNewGcellsCount());
          }
          log_->info(GPL,
                     109,
                     "Timing-driven: inserted buffers as reported by "
                     "repair_design: {}",
                     tb_->repairDesignBufferCount());
          log_->info(GPL,
                     110,
                     "Timing-driven: new target density: {}",
                     nesterov->targetDensity());
          nbc_->resetDeltaArea();
          nbc_->resetNewGcellsCount();
          nesterov->updateAreas();
          nesterov->updateDensitySize();
        }

        // update snapshot after non-virtual TD
        int64_t hpwl = nbc_->getHpwl();
        if (average_overflow_unscaled_ <= 0.25) {
          min_hpwl_ = hpwl;
          diverge_snapshot_average_overflow_unscaled_
              = average_overflow_unscaled_;
          diverge_snapshot_iter_ = iter + 1;
          is_min_hpwl_ = true;
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

    num_region_diverged_ = 0;
    for (auto& nb : nbVec_) {
      num_region_diverged_ += nb->checkDivergence();
    }

    if (!npVars_.disableRevertIfDiverge && num_region_diverged_ == 0) {
      if (is_min_hpwl_) {
        diverge_snapshot_WlCoefX = wireLengthCoefX_;
        diverge_snapshot_WlCoefY = wireLengthCoefY_;
        for (auto& nb : nbVec_) {
          nb->snapshot();
        }
        is_diverge_snapshot_saved = true;
      }
    }

    if (num_region_diverged_ > 0) {
      log_->report("Divergence occured in {} regions.", num_region_diverged_);

      // TODO: this divergence treatment uses the non-deterministic aspect of
      // routability inflation to try one more time if a divergence is detected.
      // This feature lost its consistency since we allow for non-virtual timing
      // driven iterations. Meaning we would go back to a snapshot without newly
      // added instances. A way to maintain this feature is to store two
      // snapshots one for routability revert if diverge and try again, and
      // another for simply revert if diverge and finish without hitting 0.10
      // overflow.
      // // revert back to the original rb solutions
      // // one more opportunity
      // if (!isDivergeTriedRevert && rb_->numCall() >= 1) {
      //   // get back to the working rc size
      //   rb_->revertGCellSizeToMinRc();
      //   curA = route_snapshotA;
      //   wireLengthCoefX_ = route_snapshot_WlCoefX;
      //   wireLengthCoefY_ = route_snapshot_WlCoefY;
      //   nbc_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);
      //   for (auto& nb : nbVec_) {
      //     nb->revertToSnapshot();
      //   }

      //   isDiverged_ = false;
      //   divergeCode_ = 0;
      //   divergeMsg_ = "";
      //   isDivergeTriedRevert = true;
      //   // turn off the RD forcely
      //   is_routability_need_ = false;
      // } else
      if (!npVars_.disableRevertIfDiverge && is_diverge_snapshot_saved) {
        // In case diverged and not in routability mode, finish with min hpwl
        // stored since overflow below 0.25
        log_->warn(GPL,
                   90,
                   "Divergence detected, reverting to snapshot with min hpwl.");
        log_->warn(GPL,
                   91,
                   "Revert to iter: {:4d} overflow: {:.3f} HPWL: {}",
                   diverge_snapshot_iter_,
                   diverge_snapshot_average_overflow_unscaled_,
                   min_hpwl_);
        wireLengthCoefX_ = diverge_snapshot_WlCoefX;
        wireLengthCoefY_ = diverge_snapshot_WlCoefY;
        nbc_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);
        for (auto& nb : nbVec_) {
          nb->revertToSnapshot();
        }
        num_region_diverged_ = 0;
        break;
      } else {
        divergeMsg_
            = "RePlAce divergence detected: "
              "Current overflow is low and increasing relative to minimum,"
              "and the HPWL has significantly worsened. "
              "Consider re-running with a smaller max_phi_cof value.";
        divergeCode_ = 307;
        break;
      }
    }

    if (!is_routability_snapshot_saved && npVars_.routability_driven_mode
        && routability_save_snapshot_ >= average_overflow_unscaled_) {
      route_snapshot_WlCoefX = wireLengthCoefX_;
      route_snapshot_WlCoefY = wireLengthCoefY_;
      route_snapshotA = curA;
      is_routability_snapshot_saved = true;

      for (auto& nb : nbVec_) {
        nb->snapshot();
      }

      log_->info(GPL, 88, "Routability snapshot saved at iter = {}", iter);
    }

    // check routability using RUDY or GR
    if (npVars_.routability_driven_mode && is_routability_need_
        && npVars_.routability_end_overflow >= average_overflow_unscaled_) {
      nbVec_[0]->setTrueReprintIterHeader();
      // recover the densityPenalty values
      // if further routability-driven is needed
      std::pair<bool, bool> result = rb_->routability();
      is_routability_need_ = result.first;
      bool isRevertInitNeeded = result.second;

      // if routability is needed
      if (is_routability_need_ || isRevertInitNeeded) {
        // cutFillerCoordinates();

        // revert back the current density penality
        curA = route_snapshotA;
        wireLengthCoefX_ = route_snapshot_WlCoefX;
        wireLengthCoefY_ = route_snapshot_WlCoefY;

        nbc_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

        for (auto& nb : nbVec_) {
          nb->revertToSnapshot();
          nb->resetMinSumOverflow();
        }
        log_->info(
            GPL, 89, "Routability end iteration: revert back to snapshot");
      }
    }

    // check each for converge and if all are converged then stop
    int numConverge = 0;
    for (auto& nb : nbVec_) {
      numConverge += nb->checkConvergence();
    }

    if (numConverge == nbVec_.size()) {
      break;
    }
  }
  // in all case including diverge,
  // db should be updated.
  updateDb();

  if (num_region_diverged_ > 0) {
    log_->error(GPL, divergeCode_, divergeMsg_);
  }

  if (graphics_) {
    graphics_->status("End placement");
    graphics_->cellPlot(true);
  }

  if (db_cbk_ != nullptr) {
    db_cbk_->removeOwner();
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

  // Update divergence snapshot
  if (!npVars_.disableRevertIfDiverge) {
    int64_t hpwl = nbc_->getHpwl();
    hpwl_gauge_->Set(hpwl);
    if (hpwl < min_hpwl_ && average_overflow_unscaled_ <= 0.25) {
      min_hpwl_ = hpwl;
      diverge_snapshot_average_overflow_unscaled_ = average_overflow_unscaled_;
      diverge_snapshot_iter_ = iter + 1;
      is_min_hpwl_ = true;
    } else {
      is_min_hpwl_ = false;
    }
  }
}

void NesterovPlace::updateDb()
{
  nbc_->updateDbGCells();
}

// divergence detection on
// Wirelength / density gradient calculation
void NesterovPlace::checkInvalidValues(float wireLengthGradSum,
                                       float densityGradSum)
{
  if (std::isnan(wireLengthGradSum) || std::isnan(densityGradSum)
      || std::isinf(wireLengthGradSum) || std::isinf(densityGradSum)) {
    divergeMsg_
        = "RePlAce diverged at wire/density gradient Sum. An internal value is "
          "NaN or Inf.";
    divergeCode_ = 306;
    num_region_diverged_ = 1;
    return;
  }
}

nesterovDbCbk::nesterovDbCbk(NesterovPlace* nesterov_place)
    : nesterov_place_(nesterov_place)
{
}

void NesterovPlace::createGCell(odb::dbInst* db_inst)
{
  auto gcell_index = nbc_->createGCell(db_inst);
  for (auto& nesterov : nbVec_) {
    // TODO: manage regions, not every NB should create a
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
  odb::dbSigType netType = db_net->getSigType();
  if (!isValidSigType(netType)) {
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
  if (!isValidSigType(iterm->getSigType())) {
    return;
  }
  nbc_->createITerm(iterm);
}

void NesterovPlace::destroyITerm(odb::dbITerm* iterm)
{
  if (!isValidSigType(iterm->getSigType())) {
    return;
  }
  nbc_->destroyITerm(iterm);
}

void NesterovPlace::resizeGCell(odb::dbInst* db_inst)
{
  nbc_->resizeGCell(db_inst);
}

void NesterovPlace::moveGCell(odb::dbInst* db_inst)
{
  nbc_->moveGCell(db_inst);
}

void nesterovDbCbk::inDbInstSwapMasterAfter(odb::dbInst* db_inst)
{
  nesterov_place_->resizeGCell(db_inst);
}

void nesterovDbCbk::inDbPostMoveInst(odb::dbInst* db_inst)
{
  nesterov_place_->moveGCell(db_inst);
}

void nesterovDbCbk::inDbInstCreate(odb::dbInst* db_inst)
{
  nesterov_place_->createGCell(db_inst);
}

// TODO: use the region to create new gcell.
void nesterovDbCbk::inDbInstCreate(odb::dbInst* db_inst, odb::dbRegion* region)
{
}

void nesterovDbCbk::inDbInstDestroy(odb::dbInst* db_inst)
{
  nesterov_place_->destroyGCell(db_inst);
}

void nesterovDbCbk::inDbITermCreate(odb::dbITerm* iterm)
{
  nesterov_place_->createITerm(iterm);
}

void nesterovDbCbk::inDbITermDestroy(odb::dbITerm* iterm)
{
  nesterov_place_->destroyITerm(iterm);
}

void nesterovDbCbk::inDbNetCreate(odb::dbNet* db_net)
{
  nesterov_place_->createGNet(db_net);
}

void nesterovDbCbk::inDbNetDestroy(odb::dbNet* db_net)
{
}

}  // namespace gpl
