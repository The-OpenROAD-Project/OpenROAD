// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

// Debug controls: npinit, updateGrad, np, updateNextIter

#include "nesterovPlace.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

#include "AbstractGraphics.h"
#include "nesterovBase.h"
#include "odb/db.h"
#include "placerBase.h"
#include "routeBase.h"
#include "timingBase.h"
#include "utl/Logger.h"

namespace gpl {
using utl::GPL;

NesterovPlace::NesterovPlace(const NesterovPlaceVars& npVars,
                             const std::shared_ptr<PlacerBaseCommon>& pbc,
                             const std::shared_ptr<NesterovBaseCommon>& nbc,
                             std::vector<std::shared_ptr<PlacerBase>>& pbVec,
                             std::vector<std::shared_ptr<NesterovBase>>& nbVec,
                             std::shared_ptr<RouteBase> rb,
                             std::shared_ptr<TimingBase> tb,
                             std::unique_ptr<gpl::AbstractGraphics> graphics,
                             utl::Logger* log)
    : npVars_(npVars)
{
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

  if (graphics) {
    graphics_ = std::move(graphics);
    graphics_->setDebugOn(npVars.debug);
    graphics_->debugForNesterovPlace(this,
                                     pbc_,
                                     nbc_,
                                     rb_,
                                     pbVec_,
                                     nbVec_,
                                     npVars_.debug_draw_bins,
                                     npVars_.debug_inst);
  }

  init();
}

NesterovPlace::~NesterovPlace()
{
  reset();
}

void NesterovPlace::npUpdatePrevGradient(
    const std::shared_ptr<NesterovBase>& nb)
{
  nb->nbUpdatePrevGradient(wireLengthCoefX_, wireLengthCoefY_);
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
    npUpdatePrevGradient(nb);
    return;
  }

  checkInvalidValues(wireLengthGradSum, densityGradSum);
}

void NesterovPlace::npUpdateCurGradient(const std::shared_ptr<NesterovBase>& nb)
{
  nb->nbUpdateCurGradient(wireLengthCoefX_, wireLengthCoefY_);
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
    npUpdateCurGradient(nb);
    return;
  }

  checkInvalidValues(wireLengthGradSum, densityGradSum);
}

void NesterovPlace::npUpdateNextGradient(
    const std::shared_ptr<NesterovBase>& nb)
{
  nb->nbUpdateNextGradient(wireLengthCoefX_, wireLengthCoefY_);

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
    npUpdateNextGradient(nb);
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
    npUpdateCurGradient(nb);

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
    npUpdatePrevGradient(nb);
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

void NesterovPlace::updateIterGraphics(
    int iter,
    const std::string& reports_dir,
    const std::string& routability_driven_dir,
    int routability_driven_revert_count,
    int timing_driven_count,
    bool& final_routability_image_saved)
{
  if (!graphics_ || !graphics_->enabled()) {
    return;
  }

  // For JPEG Saving
  updateDb();

  // Calculate RUDY every stride iteration, depending on debug_rudy_start
  if (npVars_.routability_driven_mode && npVars_.debug
      && npVars_.debug_rudy_start > 0 && iter >= npVars_.debug_rudy_start
      && npVars_.debug_rudy_stride > 0
      && (iter - npVars_.debug_rudy_start) % npVars_.debug_rudy_stride == 0) {
    rb_->calculateRudyTiles();
    rb_->updateRudyAverage(/*verbose=*/false);
  }

  graphics_->addIter(iter, average_overflow_unscaled_);

  if (!npVars_.debug) {
    return;
  }
  int debug_start_iter = npVars_.debug_start_iter;
  if (debug_start_iter == 0 || iter + 1 >= debug_start_iter) {
    bool update
        = (iter == 0 || (iter + 1) % npVars_.debug_update_iterations == 0);
    if (update) {
      bool pause
          = (iter == 0 || (iter + 1) % npVars_.debug_pause_iterations == 0);
      graphics_->cellPlot(pause);
    }
  }

  if (npVars_.debug_generate_images && iter == 0) {
    std::string gif_path = fmt::format("{}/placement.gif", reports_dir);
    placement_gif_key_ = graphics_->gifStart(gif_path);
  }

  if (npVars_.debug_generate_images && iter % 10 == 0) {
    odb::Rect region;
    int width_px = 500;
    odb::Rect bbox = pbc_->db()->getChip()->getBlock()->getBBox()->getBox();
    int max_dim = std::max(bbox.dx(), bbox.dy());
    double dbu_per_pixel = static_cast<double>(max_dim) / 1000.0;
    int delay = 20;
    std::string label = fmt::format("Iter {} |R: {} |T: {}",
                                    iter,
                                    routability_driven_revert_count,
                                    timing_driven_count);
    std::string label_name = fmt::format("frame_label_{}", iter);

    graphics_->addFrameLabel(bbox, label, label_name);
    graphics_->gifAddFrame(
        placement_gif_key_, region, width_px, dbu_per_pixel, delay);
    graphics_->deleteLabel(label_name);
  }

  // Save image once if routability not needed and below routability overflow
  if (npVars_.routability_driven_mode && !is_routability_need_
      && average_overflow_unscaled_ <= npVars_.routability_end_overflow
      && !final_routability_image_saved) {
    if (npVars_.debug_generate_images) {
      const std::string label = fmt::format("Iter {} |R: {} |T: {}",
                                            iter,
                                            routability_driven_revert_count,
                                            timing_driven_count);

      graphics_->saveLabeledImage(
          fmt::format("{}/1_routability_final_{:05d}.png",
                      routability_driven_dir,
                      iter),
          label,
          /* select_buffers = */ false);

      graphics_->saveLabeledImage(
          fmt::format("{}/1_density_routability_final_{:05d}.png",
                      routability_driven_dir,
                      iter),
          label,
          false,
          "Heat Maps/Placement Density");

      graphics_->saveLabeledImage(
          fmt::format("{}/1_rudy_routability_final_{:05d}.png",
                      routability_driven_dir,
                      iter),
          label,
          false,
          "Heat Maps/Estimated Congestion (RUDY)");

      graphics_->saveLabeledImage(
          fmt::format("{}/1_routability_final_{:05d}.png",
                      routability_driven_dir,
                      iter),
          fmt::format("Iter {} |R: {} |T: {} final route",
                      iter,
                      routability_driven_revert_count,
                      timing_driven_count),
          false);
    }
    final_routability_image_saved = true;
  }
}

void NesterovPlace::runTimingDriven(int iter,
                                    const std::string& timing_driven_dir,
                                    int routability_driven_revert_count,
                                    int& timing_driven_count,
                                    int64_t& td_accumulated_delta_area,
                                    bool is_routability_gpl_iter)
{
  // timing driven feature
  // if virtual, do reweight on timing-critical nets,
  // otherwise keep all modifications by rsz.
  if (npVars_.timingDrivenMode
      && tb_->isTimingNetWeightOverflow(average_overflow_unscaled_)
      && (!is_routability_gpl_iter || !npVars_.routability_driven_mode)) {
    // update db's instance location from current density coordinates
    updateDb();

    if (graphics_ && graphics_->enabled()) {
      graphics_->addTimingDrivenIter(iter);

      if (npVars_.debug_generate_images) {
        graphics_->saveLabeledImage(
            fmt::format("{}/timing_{:05d}_0.png", timing_driven_dir, iter),
            fmt::format("Iter {} |R: {} |T: {} before TD",
                        iter,
                        routability_driven_revert_count,
                        timing_driven_count),
            /* select_buffers = */ false);
      }
    }

    // Call resizer's estimateRC API to fill in PEX using placed locations,
    // Call sta's API to extract worst timing paths,
    // and update GNet's weights from worst timing paths.
    //
    // See timingBase.cpp in detail
    bool virtual_td_iter
        = (average_overflow_unscaled_ > npVars_.keepResizeBelowOverflow);

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
               average_overflow_unscaled_,
               npVars_.keepResizeBelowOverflow,
               nbc_->getHpwl());

    if (!virtual_td_iter) {
      db_cbk_->addOwner(pbc_->db()->getChip()->getBlock());
    } else {
      db_cbk_->removeOwner();
    }

    auto block = pbc_->db()->getChip()->getBlock();
    int nb_gcells_before_td = 0;
    int nb_gcells_after_td = 0;
    int nbc_total_gcells_before_td = nbc_->getGCells().size();

    for (auto& nb : nbVec_) {
      nb_gcells_before_td += nb->getGCells().size();
    }

    bool shouldTdProceed = tb_->executeTimingDriven(virtual_td_iter);
    // TODO remove fillers for TD iterations
    // for (auto& nesterov : nbVec_) {
    //   nesterov->cutFillerCells(nbc_->getDeltaArea());
    // }

    nbVec_[0]->setTrueReprintIterHeader();
    ++timing_driven_count;

    const int nbc_total_gcells_delta
        = nbc_->getNewGcellsCount() - nbc_->getDeletedGcellsCount();
    td_accumulated_delta_area += nbc_->getDeltaArea();
    for (auto& nb : nbVec_) {
      nb_gcells_after_td += nb->getGCells().size();
    }
    const int nb_total_gcells_delta = nb_gcells_after_td - nb_gcells_before_td;
    if (nb_total_gcells_delta != nbc_total_gcells_delta) {
      log_->warn(GPL,
                 92,
                 "Mismatch in #cells between central object and all regions. "
                 "NesterovBaseCommon: {}, Summing all regions: {}",
                 nbc_total_gcells_delta,
                 nb_total_gcells_delta);
    }

    if (graphics_ && graphics_->enabled() && npVars_.debug_generate_images) {
      updateDb();
      bool select_buffers = !virtual_td_iter;
      graphics_->saveLabeledImage(
          fmt::format("{}/timing_{:05d}_1.png", timing_driven_dir, iter),
          fmt::format("Iter {} |R: {} |T: {} after TD",
                      iter,
                      routability_driven_revert_count,
                      timing_driven_count),
          select_buffers);
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
                               + nesterov->getNesterovInstsArea()
                               + nesterov->getTotalFillerArea())
            / static_cast<float>(nesterov->getWhiteSpaceArea()));

        nesterov->setMovableArea(nesterov->getTargetDensity()
                                 * nesterov->getWhiteSpaceArea());
        float rsz_delta_area_microns
            = block->dbuAreaToMicrons(nbc_->getDeltaArea());
        float rsz_delta_area_percentage
            = (nbc_->getDeltaArea()
               / static_cast<float>(nesterov->getNesterovInstsArea()))
              * 100.0f;
        log_->info(
            GPL,
            107,
            "Timing-driven: repair_design delta area: {:.3f} um^2 ({:+.2f}%)",
            rsz_delta_area_microns,
            rsz_delta_area_percentage);

        float delta_gcells_percentage = 0.0f;
        if (nbc_total_gcells_before_td > 0) {
          delta_gcells_percentage
              = ((nbc_total_gcells_delta)
                 / static_cast<float>(nbc_total_gcells_before_td))
                * 100.0f;
        }
        log_->info(
            GPL,
            108,
            "Timing-driven: repair_design, gpl delta gcells: {} ({:+.2f}%)",
            (nbc_total_gcells_delta),
            delta_gcells_percentage);
        log_->info(
            GPL,
            109,
            "Timing-driven: repair_design, gcells created: {}, deleted: {}",
            nbc_->getNewGcellsCount(),
            nbc_->getDeletedGcellsCount());

        log_->info(GPL,
                   110,
                   "Timing-driven: new target density: {}",
                   nesterov->getTargetDensity());
        nbc_->resetDeltaArea();
        nbc_->resetNewGcellsCount();
        nesterov->updateAreas();
        nesterov->updateDensitySize();
        nesterov->checkConsistency();
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
}

bool NesterovPlace::isDiverged(float& diverge_snapshot_WlCoefX,
                               float& diverge_snapshot_WlCoefY,
                               bool& is_diverge_snapshot_saved)
{
  // diverge detection on
  // large max_phi_cof value + large design
  //
  // 1) happen overflow < 20%
  // 2) Hpwl is growing

  num_region_diverged_ = 0;
  for (auto& nb : nbVec_) {
    num_region_diverged_ += nb->checkDivergence();
  }

  if (!npVars_.disableRevertIfDiverge && num_region_diverged_ == 0
      && (!npVars_.routability_driven_mode || !is_routability_need_)) {
    if (is_min_hpwl_) {
      diverge_snapshot_WlCoefX = wireLengthCoefX_;
      diverge_snapshot_WlCoefY = wireLengthCoefY_;
      for (auto& nb : nbVec_) {
        nb->saveSnapshot();
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
    // if (!isDivergeTriedRevert && rb_->getRevertCount() >= 1) {
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
                 998,
                 "Divergence detected, reverting to snapshot with min hpwl.");
      log_->warn(GPL,
                 999,
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
    } else {
      divergeMsg_
          = "RePlAce divergence detected: "
            "Current overflow is low and increasing relative to minimum,"
            "and the HPWL has significantly worsened. "
            "Consider re-running with a smaller max_phi_cof value.";
      divergeCode_ = 307;
    }
    return true;
  }
  return false;
}

void NesterovPlace::routabilitySnapshot(
    int iter,
    float curA,
    const std::string& routability_driven_dir,
    int routability_driven_revert_count,
    int timing_driven_count,
    bool& is_routability_snapshot_saved,
    float& route_snapshot_WlCoefX,
    float& route_snapshot_WlCoefY,
    float& route_snapshotA)
{
  if (!is_routability_snapshot_saved && npVars_.routability_driven_mode
      && npVars_.routability_snapshot_overflow >= average_overflow_unscaled_) {
    route_snapshot_WlCoefX = wireLengthCoefX_;
    route_snapshot_WlCoefY = wireLengthCoefY_;
    route_snapshotA = curA;
    is_routability_snapshot_saved = true;

    for (auto& nb : nbVec_) {
      nb->saveSnapshot();
    }

    log_->info(GPL, 38, "Routability snapshot saved at iter = {}", iter + 1);
    odb::dbBlock* block = pbc_->db()->getChip()->getBlock();
    const int64_t hpwl = nbc_->getHpwl();
    log_->report("{:9d} | {:8.4f} | {:13.6e} | {:8} | {:9} | {:>5}",
                 iter,
                 average_overflow_unscaled_,
                 block->dbuToMicrons(hpwl),
                 " ",
                 " ",
                 " ");
    if (graphics_ && graphics_->enabled()) {
      graphics_->addRoutabilitySnapshot(iter);
    }

    // Save image of routability snapshot
    if (graphics_ && graphics_->enabled() && npVars_.debug_generate_images) {
      graphics_->saveLabeledImage(
          fmt::format("{}/0_routability_snapshot_{:05d}.png",
                      routability_driven_dir,
                      iter),
          fmt::format("Iter {} |R: {} |T: {} save snapshot",
                      iter,
                      routability_driven_revert_count,
                      timing_driven_count),
          /* select_buffers = */ false);
    }
  }
}

void NesterovPlace::runRoutability(int iter,
                                   int timing_driven_count,
                                   const std::string& routability_driven_dir,
                                   const float route_snapshotA,
                                   const float route_snapshot_WlCoefX,
                                   const float route_snapshot_WlCoefY,
                                   int& routability_driven_revert_count,
                                   float& curA)
{
  // check routability using RUDY or GR
  if (npVars_.routability_driven_mode && is_routability_need_
      && average_overflow_unscaled_ <= npVars_.routability_end_overflow) {
    nbVec_[0]->setTrueReprintIterHeader();
    ++routability_driven_revert_count;

    if (graphics_ && graphics_->enabled() && npVars_.debug_generate_images) {
      updateDb();
      std::string label = fmt::format("Iter {} |R: {} |T: {}",
                                      iter,
                                      routability_driven_revert_count,
                                      timing_driven_count);

      graphics_->saveLabeledImage(
          fmt::format("{}/density_routability_{:05d}.png",
                      routability_driven_dir,
                      iter),
          label,
          /* select_buffers = */ false,
          "Heat Maps/Placement Density");

      graphics_->saveLabeledImage(
          fmt::format(
              "{}/rudy_routability_{:05d}.png", routability_driven_dir, iter),
          label,
          /* select_buffers = */ false,
          "Heat Maps/Estimated Congestion (RUDY)");

      odb::Rect region;
      int width_px = 500;
      odb::Rect bbox = pbc_->db()->getChip()->getBlock()->getBBox()->getBox();
      int max_dim = std::max(bbox.dx(), bbox.dy());
      double dbu_per_pixel = static_cast<double>(max_dim) / 1000.0;
      int delay = 20;
      std::string label_name = fmt::format("frame_label_routability_{}", iter);

      if (routability_gif_key_ == -1) {
        log_->report("start routability gif at iter {}", iter);
        std::string gif_path
            = fmt::format("{}/routability.gif", routability_driven_dir);
        gif_path = fmt::format("{}/routability.gif", routability_driven_dir);
        routability_gif_key_ = graphics_->gifStart(gif_path);
      }

      graphics_->addFrameLabel(bbox, label, label_name);

      graphics_->setDisplayControl("Heat Maps/Estimated Congestion (RUDY)",
                                   true);
      graphics_->gifAddFrame(
          routability_gif_key_, region, width_px, dbu_per_pixel, delay);
      graphics_->setDisplayControl("Heat Maps/Estimated Congestion (RUDY)",
                                   false);

      graphics_->deleteLabel(label_name);
    }

    // recover the densityPenalty values
    // if further routability-driven is needed
    std::pair<bool, bool> result
        = rb_->routability(routability_driven_revert_count);
    is_routability_need_ = result.first;
    bool isRevertInitNeeded = result.second;

    if (graphics_ && graphics_->enabled()) {
      graphics_->addRoutabilityIter(iter, isRevertInitNeeded);
    }

    // if routability is needed
    if (is_routability_need_ || isRevertInitNeeded) {
      // revert back the current density penality
      curA = route_snapshotA;
      wireLengthCoefX_ = route_snapshot_WlCoefX;
      wireLengthCoefY_ = route_snapshot_WlCoefY;

      nbc_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

      for (auto& nb : nbVec_) {
        nb->revertToSnapshot();
        nb->resetMinSumOverflow();
      }
    }

    if (is_routability_need_ && isRevertInitNeeded) {
      log_->info(GPL,
                 87,
                 "Routability end iteration: increase inflation and revert "
                 "back to snapshot.");
    }

    if (!is_routability_need_ && isRevertInitNeeded) {
      log_->info(GPL,
                 89,
                 "Routability finished. Reverting to minimal observed "
                 "routing congestion, could not reach target.");
    }

    if (!is_routability_need_ && !isRevertInitNeeded) {
      log_->info(GPL,
                 90,
                 "Routability finished. Target routing congestion achieved "
                 "succesfully.");
    }

    if (!is_routability_need_) {
      if (graphics_ && graphics_->enabled() && npVars_.debug_generate_images
          && routability_gif_key_ != -1) {
        graphics_->gifEnd(routability_gif_key_);
        routability_gif_key_ = -1;
      }
    }
  }
}

bool NesterovPlace::isConverged(int gpl_iter_count,
                                int routability_gpl_iter_count)
{
  // check each for converge and if all are converged then stop
  int num_region_converge = 0;
  for (auto& nb : nbVec_) {
    num_region_converge += nb->checkConvergence(
        gpl_iter_count, routability_gpl_iter_count, rb_.get());
  }

  if (num_region_converge == nbVec_.size()) {
    if (graphics_ && graphics_->enabled() && npVars_.debug_generate_images) {
      graphics_->gifEnd(placement_gif_key_);
    }
    return true;
  }
  return false;
}

std::string NesterovPlace::getReportsDir() const
{
  if (npVars_.debug_images_path == "REPORTS_DIR") {
    const char* reports_dir_env = std::getenv("REPORTS_DIR");
    return reports_dir_env ? reports_dir_env : "reports";
  }
  return npVars_.debug_images_path;
}

void NesterovPlace::cleanReportsDirs(
    const std::string& timing_driven_dir,
    const std::string& routability_driven_dir) const
{
  namespace fs = std::filesystem;
  auto clean_directory
      = [](const fs::path& dir, const std::string& exclude = "") {
          if (!fs::exists(dir)) {
            fs::create_directories(dir);
            return;
          }
          for (const auto& entry : fs::directory_iterator(dir)) {
            if (exclude.empty() || entry.path().filename() != exclude) {
              fs::remove_all(entry.path());
            }
          }
        };

  if (graphics_ && graphics_->enabled() && npVars_.debug_generate_images) {
    clean_directory(timing_driven_dir);
    clean_directory(routability_driven_dir);
  }
}

void NesterovPlace::doBackTracking(const float coeff)
{
  // Back-Tracking loop
  int numBackTrak = 0;
  for (numBackTrak = 0; numBackTrak < NesterovPlaceVars::maxBackTrack;
       numBackTrak++) {
    // fill in nextCoordinates with given stepLength_
    for (auto& nb : nbVec_) {
      nb->nesterovUpdateCoordinates(coeff);
    }

    nbc_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

    num_region_diverged_ = 0;
    for (auto& nb : nbVec_) {
      npUpdateNextGradient(nb);
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
      divergeMsg_
          = "RePlAce diverged during gradient descent calculation, resulting "
            "in an invalid step length (Inf or NaN). This is often caused by "
            "numerical instability or high placement density. Consider "
            "reducing placement density to potentially resolve the issue.";
      divergeCode_ = 305;
      break;
    }

    if (stepLengthLimitOK != nbVec_.size()) {
      break;
    }
  }

  debugPrint(log_, GPL, "np", 1, "NumBackTrak: {}", numBackTrak + 1);
  if (NesterovPlaceVars::maxBackTrack == numBackTrak) {
    debugPrint(log_,
               GPL,
               "np",
               1,
               "Backtracking limit reached so a small step will be taken");
  }
}

void NesterovPlace::reportResults(int nesterov_iter,
                                  int64_t original_area,
                                  int64_t td_accumulated_delta_area)
{
  auto block = pbc_->db()->getChip()->getBlock();

  if (graphics_ && graphics_->enabled()) {
    // Final plot point
    graphics_->addIter(nesterov_iter, average_overflow_unscaled_);

    if (npVars_.debug_generate_images) {
      updateDb();
      std::string label
          = fmt::format("Final Iter {} |R: ? |T: ?", nesterov_iter);

      graphics_->saveLabeledImage(
          fmt::format(
              "{}/final_nesterov_{:05d}.png", getReportsDir(), nesterov_iter),
          label,
          /* select_buffers = */ false);
    }
  }

  if (nesterov_iter >= npVars_.maxNesterovIter) {
    log_->warn(GPL,
               1010,
               "GPL reached the maximum number of iterations for nesterov {}. "
               "Placement may have failed to converge.",
               npVars_.maxNesterovIter);
  }

  if (npVars_.routability_driven_mode || npVars_.timingDrivenMode) {
    log_->info(GPL,
               1011,
               "Original area (um^2): {:.2f}",
               block->dbuAreaToMicrons(original_area));
  }

  if (npVars_.routability_driven_mode) {
    const int64_t routability_inflation_area = rb_->getTotalInflation();
    const float routability_diff
        = 100.0 * routability_inflation_area / original_area;
    log_->info(GPL,
               1012,
               "Total routability artificial inflation: {:.2f} ({:+.2f}%)",
               block->dbuAreaToMicrons(routability_inflation_area),
               routability_diff);
  }

  if (npVars_.timingDrivenMode) {
    const float td_diff = 100.0 * td_accumulated_delta_area / original_area;
    log_->info(GPL,
               1013,
               "Total timing-driven delta area: {:.2f} ({:+.2f}%)",
               block->dbuAreaToMicrons(td_accumulated_delta_area),
               td_diff);
  }

  int64_t new_area = 0;
  for (auto& nb : nbVec_) {
    new_area += nb->getNesterovInstsArea();
  }
  const float placement_diff
      = 100.0 * (new_area - original_area) / original_area;
  log_->info(GPL,
             1014,
             "Final placement area: {:.2f} ({:+.2f}%)",
             block->dbuAreaToMicrons(new_area),
             placement_diff);
}

int NesterovPlace::doNesterovPlace(int start_iter)
{
  // if replace diverged in init() function, Nesterov must be skipped.
  if (num_region_diverged_ > 0) {
    log_->error(GPL, divergeCode_, divergeMsg_);
  }

  // routability snapshot info
  bool is_routability_snapshot_saved = false;
  float route_snapshotA = 0;
  float route_snapshot_WlCoefX = 0;
  float route_snapshot_WlCoefY = 0;

  // divergence snapshot info
  bool is_diverge_snapshot_saved = false;
  float diverge_snapshot_WlCoefX = 0;
  float diverge_snapshot_WlCoefY = 0;

  // backTracking variable.
  float curA = 1.0;

  int routability_driven_revert_count = 0;
  int routability_gpl_iter_count_ = 0;
  int timing_driven_count = 0;
  bool final_routability_image_saved = false;
  int64_t original_area = 0;
  int64_t td_accumulated_delta_area = 0;

  if (graphics_ && graphics_->enabled() && npVars_.debug
      && npVars_.debug_start_iter == start_iter) {
    graphics_->cellPlot(true);
  }

  for (auto& nb : nbVec_) {
    nb->setIter(start_iter);
    nb->setMaxPhiCoefChanged(false);
    nb->resetMinSumOverflow();
    original_area += nb->getNesterovInstsArea();
  }

  const std::string reports_dir = getReportsDir();
  const std::string timing_driven_dir = reports_dir + "/gpl_timing_driven";
  const std::string routability_driven_dir
      = reports_dir + "/gpl_routability_driven";

  cleanReportsDirs(timing_driven_dir, routability_driven_dir);
  if (graphics_ && graphics_->enabled() && npVars_.debug_generate_images) {
    updateDb();
    std::string label = fmt::format("init_nesterov");

    graphics_->saveLabeledImage(
        fmt::format("{}/init_nesterov.png", getReportsDir()),
        label,
        /* select_buffers = */ false);
  }

  // Core Nesterov Loop
  int nesterov_iter = start_iter;
  for (; nesterov_iter < npVars_.maxNesterovIter; nesterov_iter++) {
    const float prevA = curA;

    // here, prevA is a_(k), curA is a_(k+1)
    // See, the ePlace-MS paper's Algorithm 1
    curA = (1.0 + sqrt(4.0 * prevA * prevA + 1.0)) * 0.5;

    // coeff is (a_k - 1) / ( a_(k+1) ) in paper.
    const float coeff = (prevA - 1.0) / curA;

    doBackTracking(coeff);

    // Adjust Phi dynamically for larger designs
    for (auto& nb : nbVec_) {
      nb->nesterovAdjustPhi();
    }

    if (num_region_diverged_ > 0) {
      break;
    }

    updateNextIter(nesterov_iter);

    updateIterGraphics(nesterov_iter,
                       reports_dir,
                       routability_driven_dir,
                       routability_driven_revert_count,
                       timing_driven_count,
                       final_routability_image_saved);

    bool is_routability_gpl_iter
        = is_routability_snapshot_saved
          && average_overflow_unscaled_ > npVars_.routability_end_overflow;
    if (is_routability_gpl_iter) {
      ++routability_gpl_iter_count_;
      ++npVars_.maxNesterovIter;
    }

    runTimingDriven(nesterov_iter,
                    timing_driven_dir,
                    routability_driven_revert_count,
                    timing_driven_count,
                    td_accumulated_delta_area,
                    is_routability_gpl_iter);

    if (isDiverged(diverge_snapshot_WlCoefX,
                   diverge_snapshot_WlCoefY,
                   is_diverge_snapshot_saved)) {
      break;
    }

    routabilitySnapshot(nesterov_iter,
                        curA,
                        routability_driven_dir,
                        routability_driven_revert_count,
                        timing_driven_count,
                        is_routability_snapshot_saved,
                        route_snapshot_WlCoefX,
                        route_snapshot_WlCoefY,
                        route_snapshotA);

    runRoutability(nesterov_iter,
                   timing_driven_count,
                   routability_driven_dir,
                   route_snapshotA,
                   route_snapshot_WlCoefX,
                   route_snapshot_WlCoefY,
                   routability_driven_revert_count,
                   curA);

    if (isConverged(nesterov_iter, routability_gpl_iter_count_)) {
      break;
    }
  }

  reportResults(nesterov_iter, original_area, td_accumulated_delta_area);

  // In all case, including divergence, the db should be updated.
  updateDb();

  if (num_region_diverged_ > 0) {
    log_->error(GPL, divergeCode_, divergeMsg_);
  }

  if (graphics_ && graphics_->enabled() && npVars_.debug) {
    graphics_->status("End placement");
    graphics_->cellPlot(true);

    if (npVars_.debug_generate_images) {
      const std::string label = fmt::format("Iter {} |R: {} |T: {}",
                                            nesterov_iter,
                                            routability_driven_revert_count,
                                            timing_driven_count);

      graphics_->saveLabeledImage(fmt::format("{}/2_final_placement_{:05d}.png",
                                              routability_driven_dir,
                                              nesterov_iter),
                                  label,
                                  /* select_buffers = */ false);
    }
  }

  if (db_cbk_ != nullptr) {
    db_cbk_->removeOwner();
  }
  return nesterov_iter;
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

void NesterovPlace::updateNextIter(int iter)
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

void NesterovPlace::createCbkGCell(odb::dbInst* db_inst)
{
  auto gcell_index = nbc_->createCbkGCell(db_inst);
  // Always create gcell on top-level
  nbVec_[0]->createCbkGCell(db_inst, gcell_index);
  // TODO: create new gcell in its proper region
  // for (auto& nesterov : nbVec_) {
  //   nesterov->createCbkGCell(db_inst, gcell_index);
  // }
}

void NesterovPlace::destroyCbkGCell(odb::dbInst* db_inst)
{
  for (auto& nesterov : nbVec_) {
    nesterov->destroyCbkGCell(db_inst);
  }
}

void NesterovPlace::createGNet(odb::dbNet* db_net)
{
  odb::dbSigType netType = db_net->getSigType();
  if (!isValidSigType(netType)) {
    return;
  }
  nbc_->createCbkGNet(db_net, pbc_->isSkipIoMode());
}

void NesterovPlace::destroyCbkGNet(odb::dbNet* db_net)
{
  nbc_->destroyCbkGNet(db_net);
}

void NesterovPlace::createCbkITerm(odb::dbITerm* iterm)
{
  if (!isValidSigType(iterm->getSigType())) {
    return;
  }
  nbc_->createCbkITerm(iterm);
}

void NesterovPlace::destroyCbkITerm(odb::dbITerm* iterm)
{
  if (!isValidSigType(iterm->getSigType())) {
    return;
  }
  nbc_->destroyCbkITerm(iterm);
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
  nesterov_place_->createCbkGCell(db_inst);
}

// TODO: use the region to create new gcell.
void nesterovDbCbk::inDbInstCreate(odb::dbInst* db_inst, odb::dbRegion* region)
{
}

void nesterovDbCbk::inDbInstDestroy(odb::dbInst* db_inst)
{
  nesterov_place_->destroyCbkGCell(db_inst);
}

void nesterovDbCbk::inDbITermCreate(odb::dbITerm* iterm)
{
  nesterov_place_->createCbkITerm(iterm);
}

void nesterovDbCbk::inDbITermDestroy(odb::dbITerm* iterm)
{
  nesterov_place_->destroyCbkITerm(iterm);
}

void nesterovDbCbk::inDbNetCreate(odb::dbNet* db_net)
{
  nesterov_place_->createGNet(db_net);
}

void nesterovDbCbk::inDbNetDestroy(odb::dbNet* db_net)
{
  nesterov_place_->destroyCbkGNet(db_net);
}

}  // namespace gpl
