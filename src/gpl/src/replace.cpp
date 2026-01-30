// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "gpl/Replace.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include "AbstractGraphics.h"
#include "db_sta/dbSta.hh"
#include "graphicsNone.h"
#include "initialPlace.h"
#include "mbff.h"
#include "nesterovBase.h"
#include "nesterovPlace.h"
#include "odb/db.h"
#include "odb/util.h"
#include "placerBase.h"
#include "routeBase.h"
#include "rsz/Resizer.hh"
#include "sta/StaMain.hh"
#include "timingBase.h"
#include "utl/Logger.h"
#include "utl/validation.h"

namespace gpl {

using utl::GPL;

Replace::Replace(odb::dbDatabase* odb,
                 sta::dbSta* sta,
                 rsz::Resizer* resizer,
                 grt::GlobalRouter* router,
                 utl::Logger* logger)
    : db_(odb), sta_(sta), rs_(resizer), fr_(router), log_(logger)
{
  graphics_ = std::make_unique<GraphicsNone>();
}

Replace::~Replace() = default;

void Replace::setGraphicsInterface(const AbstractGraphics& graphics)
{
  graphics_ = graphics.MakeNew(log_);
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
}

void Replace::addPlacementCluster(const Cluster& cluster)
{
  clusters_.emplace_back(cluster);
}

void Replace::checkHasCoreRows()
{
  if (!dbHasCoreRows(db_)) {
    log_->error(
        GPL,
        130,
        "No rows defined in design. Use initialize_floorplan to add rows.");
  }
}

void Replace::doIncrementalPlace(const int threads, const PlaceOptions& options)
{
  checkHasCoreRows();
  log_->info(GPL, 6, "Execute incremental mode global placement.");
  if (pbc_ == nullptr) {
    pbc_ = std::make_shared<PlacerBaseCommon>(db_, options, log_);

    pbVec_.push_back(std::make_shared<PlacerBase>(db_, pbc_, log_));

    for (auto pd : db_->getChip()->getBlock()->getRegions()) {
      for (auto group : pd->getGroups()) {
        pbVec_.push_back(std::make_shared<PlacerBase>(db_, pbc_, log_, group));
      }
    }

    total_placeable_insts_ = 0;
    for (const auto& pb : pbVec_) {
      total_placeable_insts_ += pb->placeInsts().size();
    }
  }

  // Lock down already placed objects
  int placed_cnt = 0;
  int unplaced_cnt = 0;
  auto block = db_->getChip()->getBlock();
  for (auto inst : block->getInsts()) {
    auto status = inst->getPlacementStatus();
    if (status == odb::dbPlacementStatus::PLACED) {
      pbc_->dbToPb(inst)->lock();
      ++placed_cnt;
    } else if (!status.isPlaced()) {
      ++unplaced_cnt;
    }
  }

  log_->info(GPL, 154, "Identified {} placed instances", placed_cnt);
  log_->info(GPL, 155, "Identified {} not placed instances", unplaced_cnt);

  if (unplaced_cnt == 0) {
    // Everything was already placed so we do the old incremental mode
    // which just skips initial placement and runs nesterov.
    log_->info(GPL,
               156,
               "Identified all instances as placed. Unlocking all instances "
               "and running nesterov from scratch.");
    for (auto& pb : pbVec_) {
      pb->unlockAll();
    }

    doNesterovPlace(threads, options);
    return;
  }

  // Roughly place the unplaced objects (allow more overflow).
  // Limit iterations to prevent objects drifting too far or
  // non-convergence.
  PlaceOptions rough_options = options;
  rough_options.overflow = std::max(options.overflow, 0.2f);
  rough_options.nesterovPlaceMaxIter = 300;

  doInitialPlace(threads, rough_options);
  const int iter = doNesterovPlace(threads, rough_options);

  // Finish the overflow resolution from the rough placement
  log_->info(GPL, 133, "Unlocking all instances");
  for (auto& pb : pbVec_) {
    pb->unlockAll();
  }

  if (options.overflow < rough_options.overflow) {
    doNesterovPlace(threads, options, iter + 1);
  }
}

void Replace::doPlace(const int threads, const PlaceOptions& options)
{
  doInitialPlace(threads, options);
  doNesterovPlace(threads, options);
}

void Replace::doInitialPlace(const int threads, const PlaceOptions& options)
{
  checkHasCoreRows();
  if (pbc_ == nullptr) {
    pbc_ = std::make_shared<PlacerBaseCommon>(db_, options, log_);

    pbVec_.push_back(std::make_shared<PlacerBase>(db_, pbc_, log_));

    for (auto pd : db_->getChip()->getBlock()->getRegions()) {
      for (auto group : pd->getGroups()) {
        pbVec_.push_back(std::make_shared<PlacerBase>(db_, pbc_, log_, group));
      }
    }

    if (pbVec_.front()->placeInsts().empty()) {
      log_->warn(
          GPL,
          123,
          "No placeable instances in the top-level region. Removing it.");
      pbVec_.erase(pbVec_.begin());
    }

    total_placeable_insts_ = 0;
    for (const auto& pb : pbVec_) {
      total_placeable_insts_ += pb->placeInsts().size();
    }
  }

  const InitialPlaceVars ipVars(options, gui_debug_initial_);

  std::unique_ptr<InitialPlace> ip(
      new InitialPlace(ipVars, pbc_, pbVec_, graphics_->MakeNew(log_), log_));
  ip_ = std::move(ip);
  ip_->doBicgstabPlace(threads);
}

void Replace::runMBFF(const int max_sz,
                      const float alpha,
                      const float beta,
                      const int threads,
                      const int num_paths)
{
  MBFF pntset(db_,
              sta_,
              log_,
              rs_,
              threads,
              20,
              num_paths,
              gui_debug_,
              graphics_->MakeNew(log_));
  pntset.Run(max_sz, alpha, beta);
}

bool Replace::initNesterovPlace(const PlaceOptions& options, const int threads)
{
  if (!pbc_) {
    pbc_ = std::make_shared<PlacerBaseCommon>(db_, options, log_);

    pbVec_.push_back(std::make_shared<PlacerBase>(db_, pbc_, log_));

    for (auto pd : db_->getChip()->getBlock()->getRegions()) {
      for (auto group : pd->getGroups()) {
        pbVec_.push_back(std::make_shared<PlacerBase>(db_, pbc_, log_, group));
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
    const NesterovBaseVars nbVars(options);

    nbc_ = std::make_shared<NesterovBaseCommon>(
        nbVars, pbc_, log_, threads, clusters_);

    for (const auto& pb : pbVec_) {
      nbVec_.push_back(std::make_shared<NesterovBase>(nbVars, pb, nbc_, log_));
    }
  }

  if (!rb_) {
    const RouteBaseVars rbVars(options);
    rb_ = std::make_shared<RouteBase>(rbVars, db_, fr_, nbc_, nbVec_, log_);
  }

  if (!tb_) {
    tb_ = std::make_shared<TimingBase>(nbc_, fr_, rs_, log_);
    tb_->setTimingNetWeightOverflows(options.timingNetWeightOverflows);
    tb_->setTimingNetWeightMax(options.timingNetWeightMax);
  }

  if (!np_) {
    NesterovPlaceVars npVars(options);

    npVars.debug = gui_debug_;
    npVars.debug_pause_iterations = gui_debug_pause_iterations_;
    npVars.debug_update_iterations = gui_debug_update_iterations_;
    npVars.debug_draw_bins = gui_debug_draw_bins_;
    npVars.debug_inst = gui_debug_inst_;
    npVars.debug_start_iter = gui_debug_start_iter_;
    npVars.debug_rudy_start = gui_debug_rudy_start_;
    npVars.debug_rudy_stride = gui_debug_rudy_stride_;
    npVars.debug_generate_images = gui_debug_generate_images_;
    npVars.debug_images_path = gui_debug_images_path_;

    for (const auto& nb : nbVec_) {
      nb->setNpVars(&npVars);
    }

    np_ = std::make_unique<NesterovPlace>(npVars,
                                          pbc_,
                                          nbc_,
                                          pbVec_,
                                          nbVec_,
                                          rb_,
                                          tb_,
                                          graphics_->MakeNew(log_),
                                          log_);
  }
  // Ensure these get set even if np_ already exists.
  np_->setTargetOverflow(options.overflow);
  np_->setMaxIters(options.nesterovPlaceMaxIter);
  return true;
}

int Replace::doNesterovPlace(const int threads,
                             const PlaceOptions& options,
                             const int start_iter)
{
  checkHasCoreRows();
  if (!initNesterovPlace(options, threads)) {
    return 0;
  }

  log_->info(GPL, 7, "---- Execute Nesterov Global Placement.");
  if (options.timingDrivenMode) {
    rs_->resizeSlackPreamble();
  }

  auto start = std::chrono::high_resolution_clock::now();

  int return_do_nesterov = np_->doNesterovPlace(start_iter);

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  debugPrint(log_,
             GPL,
             "runtime",
             1,
             "NP->doNesterovPlace() runtime: {} seconds ",
             elapsed.count());

  if (options.enable_routing_congestion) {
    fr_->setAllowCongestion(true);
    fr_->setCongestionIterations(0);
    fr_->setCriticalNetsPercentage(0);
    fr_->globalRoute();
  }
  return return_do_nesterov;
}

float Replace::getUniformTargetDensity(const PlaceOptions& options,
                                       const int threads)
{
  log_->info(GPL, 22, "Initialize gpl and calculate uniform density.");
  log_->redirectStringBegin();

  PlaceOptions options_no_io = options;
  options_no_io.skipIo();  // in case bterms are not placed

  float density = 1.0f;
  if (initNesterovPlace(options_no_io, threads)) {
    density = nbVec_[0]->getUniformTargetDensity();
  }

  log_->redirectStringEnd();  // discard output
  return density;
}

void Replace::setDebug(const int pause_iterations,
                       const int update_iterations,
                       const bool draw_bins,
                       const bool initial,
                       odb::dbInst* inst,
                       const int start_iter,
                       const int start_rudy,
                       const int rudy_stride,
                       const bool generate_images,
                       const std::string& images_path)
{
  gui_debug_ = true;
  gui_debug_pause_iterations_ = pause_iterations;
  gui_debug_update_iterations_ = update_iterations;
  gui_debug_draw_bins_ = draw_bins;
  gui_debug_initial_ = initial;
  gui_debug_inst_ = inst;
  gui_debug_start_iter_ = start_iter;
  gui_debug_rudy_start_ = start_rudy;
  gui_debug_rudy_stride_ = rudy_stride;
  gui_debug_generate_images_ = generate_images;
  gui_debug_images_path_ = images_path;
}

void PlaceOptions::validate(utl::Logger* logger)
{
  utl::Validator val(logger, GPL);
  val.check_non_negative("initialPlaceMaxIter", initialPlaceMaxIter, 326);
  val.check_positive("initialPlaceMaxFanout", initialPlaceMaxFanout, 327);
  val.check_positive("initialPlaceMaxFanout", initialPlaceMaxFanout, 327);
  val.check_range("Target density", density, 0.0f, 1.0f, 328);
}

void PlaceOptions::skipIo()
{
  skipIoMode = true;
  initialPlaceMaxIter = 0;
  timingDrivenMode = false;
  routabilityDrivenMode = false;
}

}  // namespace gpl
