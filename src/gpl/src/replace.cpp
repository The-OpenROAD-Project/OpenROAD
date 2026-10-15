// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "gpl/Replace.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include "AbstractGraphics.h"
#include "clockBase.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "graphicsNone.h"
#include "initialPlace.h"
#include "ioPinBase.h"
#include "mbff.h"
#include "nesterovBase.h"
#include "nesterovPlace.h"
#include "odb/db.h"
#include "odb/util.h"
#include "placerBase.h"
#include "ppl/IOPlacer.h"
#include "routeBase.h"
#include "rsz/Resizer.hh"
#include "sta/Graph.hh"
#include "sta/Path.hh"
#include "sta/StaMain.hh"
#include "sta/StaState.hh"
#include "timingBase.h"
#include "utl/Logger.h"
#include "utl/Validate.h"
#include "utl/timer.h"

namespace gpl {

using utl::GPL;

Replace::Replace(odb::dbDatabase* odb,
                 sta::dbSta* sta,
                 rsz::Resizer* resizer,
                 grt::GlobalRouter* router,
                 ppl::IOPlacer* pin_placer,
                 utl::Logger* logger)
    : db_(odb),
      sta_(sta),
      rs_(resizer),
      fr_(router),
      pin_placer_(pin_placer),
      log_(logger)
{
  graphics_ = std::make_unique<GraphicsNone>();

  // Register "orig_name" report_path field: original pin name before
  // multi-bit clustering. Reads "orig_name" property off the path
  // vertex's iterm. Registered at tool init so paths can be reported
  // even when the design is loaded from an already-clustered .odb
  // without re-running cluster_flops.
  if (sta_->findReportPathField(kOrigNameProp) == nullptr) {
    sta::dbNetwork* network = sta_->getDbNetwork();
    sta_->makeReportPathField(
        kOrigNameProp,
        kOrigNameProp,
        "Orig Name",
        36,
        true,
        nullptr,
        [network](const sta::Path* path,
                  const sta::StaState* sta) -> std::string {
          if (path == nullptr) {
            return {};
          }
          const sta::Pin* pin = path->vertex(sta)->pin();
          // staToDb requires all three out-params; only iterm is used.
          odb::dbITerm* iterm = nullptr;
          odb::dbBTerm* bterm = nullptr;
          odb::dbModITerm* moditerm = nullptr;
          network->staToDb(pin, iterm, bterm, moditerm);
          if (iterm == nullptr) {
            return {};
          }
          odb::dbStringProperty* prop
              = odb::dbStringProperty::find(iterm, kOrigNameProp);
          return prop ? prop->getValue() : std::string{};
        });
  }
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
  cb_.reset();
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

// Runs before PlacerBaseCommon is built: -place_ios makes an unplaced port a
// normal state there, so an unsupported design has to be rejected first.
void Replace::checkPlaceIosSupported(const PlaceOptions& options)
{
  if (!options.place_ios) {
    return;
  }
  if (db_->getChip()->getBlock()->getDieAreaPolygon().getPoints().size() > 5) {
    log_->error(GPL,
                173,
                "Concurrent IO placement does not support non-rectangular die. "
                "Please drop -place_ios or use a rectangular die.");
  }
  const ppl::PinPlacementSettings settings = pin_placer_->getSettings();
  if (settings.hor_layers.empty() || settings.ver_layers.empty()) {
    log_->error(GPL,
                174,
                "-place_ios: pin layers are not set. Set both "
                "-io_pin_hor_layers and -io_pin_ver_layers with "
                "set_place_config.");
  }
}

void Replace::doIncrementalPlace(const int threads, const PlaceOptions& options)
{
  utl::validate(options, log_);
  checkHasCoreRows();
  log_->info(GPL, 83, "Execute incremental mode global placement.");
  int placed_cnt = 0;
  int unplaced_cnt = 0;
  bool is_pbc_new = (pbc_ == nullptr);

  if (is_pbc_new) {
    pbc_ = std::make_shared<PlacerBaseCommon>(db_, options, log_);
  }

  // Count placed vs unplaced, and conditionally lock them down
  auto block = db_->getChip()->getBlock();
  for (auto inst : block->getInsts()) {
    auto status = inst->getPlacementStatus();
    if (status == odb::dbPlacementStatus::PLACED) {
      if (is_pbc_new) {
        pbc_->dbToPb(inst)->lock();
      }
      ++placed_cnt;
    } else if (!status.isPlaced()) {
      ++unplaced_cnt;
    }
  }

  if (is_pbc_new) {
    pbVec_.push_back(std::make_shared<PlacerBase>(db_, pbc_, log_, true));

    for (auto pd : db_->getChip()->getBlock()->getRegions()) {
      for (auto group : pd->getGroups()) {
        pbVec_.push_back(
            std::make_shared<PlacerBase>(db_, pbc_, log_, true, group));
      }
    }

    total_placeable_insts_ = 0;
    for (const auto& pb : pbVec_) {
      total_placeable_insts_ += pb->placeInsts().size();
    }
  }

  if (total_placeable_insts_ == 0) {
    // initNesterovPlace() would hit this same condition and refuse to build
    // np_, so bail out here instead of leaving every later np_ use guarded.
    log_->warn(GPL, 139, "No placeable instances - skipping placement.");
    return;
  }

  log_->info(GPL, 154, "Identified {} placed instances", placed_cnt);
  log_->info(GPL, 155, "Identified {} not placed instances", unplaced_cnt);

  if (unplaced_cnt == 0) {
    // Everything was already placed so we do the old incremental mode
    // which just skips initial placement and runs nesterov.
    log_->info(GPL,
               156,
               "Identified all instances as placed. Skipping incremental mode");
    return;
  }

  // Phase 1: place the unplaced (new) objects with everything else locked,
  // capped at 600 iterations so it can't run away if it fails to converge.
  PlaceOptions locked_options = options;
  locked_options.nesterov.max_iter = 600;

  doInitialPlace(threads, locked_options);

  // Build NesterovBase now (instead of lazily in doNesterovPlace() below) so
  // fillers can be redistributed before phase 1 sees its first iteration.
  if (initNesterovPlace(locked_options, threads, true)) {
    for (auto& nb : nbVec_) {
      nb->redistributeFillerCells();
    }
  }

  // Phase 1 is allowed to diverge and hand off to phase 2 instead of
  // aborting the whole incremental run, so doNesterovPlace() must not throw
  // (and log an ERROR) on a divergence it is expected to recover from; it
  // reports one via divergedLastRun() instead. Any other error (a resizer or
  // timing-driven failure, say) still throws and propagates normally -
  // nothing here is set up to recover from those.
  np_->setAllowDivergenceRecovery(true);
  int iter = doNesterovPlace(threads, locked_options);
  const bool phase1_diverged = np_->divergedLastRun();
  if (phase1_diverged) {
    log_->warn(GPL,
               195,
               "Phase 1 of incremental placement diverged before reaching "
               "overflow {:.3f}; continuing to phase 2 anyway.",
               locked_options.nesterov.target_overflow);
    np_->clearDivergence();
  }
  // Phase 2 has no further fallback, so a divergence there must fail loudly.
  np_->setAllowDivergenceRecovery(false);

  // Finish the overflow resolution from the locked placement
  log_->info(GPL, 133, "Unlocking all instances");
  for (auto& pb : pbVec_) {
    pb->unlockAll();
  }

  // Phase 1 may have run out of its iteration budget short of the real
  // target even without diverging, so check the actual overflow reached
  // rather than trusting the target alone.
  const bool phase1_missed_target
      = !phase1_diverged
        && np_->getAverageOverflow() > options.nesterov.target_overflow;

  if (phase1_diverged || phase1_missed_target) {
    // Enable phase 2's density-penalty controller to ramp the penalty up in
    // place whenever overflow regresses, instead of backing off.
    np_->enableIncrementalDensityPenaltyGuard();
    doNesterovPlace(threads, options, iter + 1);
  }
}

void Replace::doPlace(const int threads, const PlaceOptions& options)
{
  utl::Timer timer;
  doInitialPlace(threads, options);
  doNesterovPlace(threads, options);
  log_->info(GPL, 500, "Runtime: {:.2f}s", timer.elapsed());
}

void Replace::doInitialPlace(const int threads, const PlaceOptions& options)
{
  utl::validate(options, log_);
  checkHasCoreRows();
  checkPlaceIosSupported(options);
  if (pbc_ == nullptr) {
    pbc_ = std::make_shared<PlacerBaseCommon>(db_, options, log_);

    pbVec_.push_back(std::make_shared<PlacerBase>(db_, pbc_, log_, true));

    for (auto pd : db_->getChip()->getBlock()->getRegions()) {
      for (auto group : pd->getGroups()) {
        pbVec_.push_back(
            std::make_shared<PlacerBase>(db_, pbc_, log_, true, group));
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
                      const int num_paths,
                      const float clock_power_weight)
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
  pntset.Run(max_sz, alpha, beta, clock_power_weight);
}

void Replace::initIoPinPlace()
{
  // IO pins are only created for top-level placement, not for subregions.
  NesterovBase* nb = getTopLevelNB();
  if (nb->getGroup() == nullptr) {
    nb->setIoPins(std::make_unique<IoPinBase>(
        pin_placer_->buildSlotGrid(), nb->getPb().get(), nbc_.get(), log_));
  }
}

bool Replace::initNesterovPlace(const PlaceOptions& options,
                                const int threads,
                                bool check_density)
{
  if (!pbc_) {
    pbc_ = std::make_shared<PlacerBaseCommon>(db_, options, log_);

    pbVec_.push_back(
        std::make_shared<PlacerBase>(db_, pbc_, log_, check_density));

    for (auto pd : db_->getChip()->getBlock()->getRegions()) {
      for (auto group : pd->getGroups()) {
        pbVec_.push_back(std::make_shared<PlacerBase>(
            db_, pbc_, log_, check_density, group));
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
    if (options.place_ios) {
      initIoPinPlace();
    }
  }

  if (!rb_) {
    const RouteBaseVars rbVars(options);
    rb_ = std::make_shared<RouteBase>(rbVars, db_, fr_, nbc_, nbVec_, log_);
  }

  if (!tb_) {
    tb_ = std::make_shared<TimingBase>(nbc_, fr_, rs_, log_);
    tb_->setTimingNetWeightOverflows(options.timing.reweight_overflows);
    tb_->setTimingNetWeightMax(options.timing.net_weight_max);
    tb_->setTimingNetsPercentage(options.timing.nets_percentage);
    tb_->setRepairTiming(options.timing.repair_timing);
    tb_->setRepairTnsEndPercent(options.timing.repair_tns_end_percent);
    tb_->setRepairDesignOptions(options.timing.repair_design);
  }

  if (!cb_ && options.virtual_cts.enabled) {
    cb_ = std::make_shared<ClockBase>(sta_, db_, log_);
    cb_->setMaxSkewFraction(options.virtual_cts.max_skew_fraction);
  }

  if (!np_) {
    // The controller's proportional band is set by referenceHpwl, so it has to
    // be on the design's own scale. Resolve the default from the HPWL the
    // initial placement produced.
    NesterovPlaceVars npVars(options, nbc_->getHpwl());

    npVars.debug = gui_debug_;
    npVars.debug_pause_iterations = gui_debug_pause_iterations_;
    npVars.debug_update_iterations = gui_debug_update_iterations_;
    npVars.debug_draw_bins = gui_debug_draw_bins_;
    npVars.debug_inst = gui_debug_inst_;
    npVars.debug_pin = gui_debug_pin_;
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
                                          cb_,
                                          graphics_->MakeNew(log_),
                                          log_);
  }
  // Ensure these get set even if np_ already exists.
  np_->setTargetOverflow(options.nesterov.target_overflow);
  np_->setMaxIters(options.nesterov.max_iter);
  return true;
}

int Replace::doNesterovPlace(const int threads,
                             const PlaceOptions& options,
                             const int start_iter)
{
  utl::validate(options, log_);
  checkHasCoreRows();
  checkPlaceIosSupported(options);

  if (options.place_ios) {
    log_->info(GPL, 168, "Concurrent IO placement enabled.");
  }

  if (!initNesterovPlace(options, threads, true)) {
    return 0;
  }

  log_->info(GPL, 84, "---- Execute Nesterov Global Placement.");
  if (options.timing.enabled) {
    rs_->resizeSlackPreamble();
  }

  auto start = std::chrono::high_resolution_clock::now();

  int return_do_nesterov = np_->doNesterovPlace(start_iter);

  // Legalize the solved pins, moving each as little as possible.
  if (options.place_ios) {
    pin_placer_->runHungarianMatching(true);
  }

  reportHpwlMetric();

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  debugPrint(log_,
             GPL,
             "runtime",
             1,
             "NP->doNesterovPlace() runtime: {} seconds ",
             elapsed.count());

  if (options.routing_congestion) {
    fr_->globalRoute(options.routability.global_route);
  }
  return return_do_nesterov;
}

// Same evaluator dpl reports route__wirelength__estimated with, so the
// global and detailed placement numbers are directly comparable.
void Replace::reportHpwlMetric()
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  const int64_t hpwl = odb::WireLengthEvaluator(block).hpwl();
  log_->info(GPL, 1018, "Final HPWL (um): {:.2f}", block->dbuToMicrons(hpwl));
  log_->metric("route__wirelength__estimated", block->dbuToMicrons(hpwl));
}

NesterovBase* Replace::getTopLevelNB() const
{
  if (nbVec_.empty()) {
    log_->error(GPL, 104, "Top-level NesterovBase is not initialized.");
  }
  return nbVec_[0].get();
}

float Replace::getUniformTargetDensity(const PlaceOptions& options,
                                       const int threads)
{
  utl::validate(options, log_);
  log_->info(GPL, 22, "Initialize gpl and calculate uniform density.");
  log_->redirectStringBegin();

  PlaceOptions options_no_io = options;
  options_no_io.skipIo();  // in case bterms are not placed

  float density = 1.0f;
  if (initNesterovPlace(options_no_io, threads, false)) {
    density = getTopLevelNB()->getUniformTargetDensity();
  }

  log_->redirectStringEnd();  // discard output
  return density;
}

void Replace::setDebug(const int pause_iterations,
                       const int update_iterations,
                       const bool draw_bins,
                       const bool initial,
                       odb::dbInst* inst,
                       odb::dbBTerm* pin,
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
  gui_debug_pin_ = pin;
  gui_debug_start_iter_ = start_iter;
  gui_debug_rudy_start_ = start_rudy;
  gui_debug_rudy_stride_ = rudy_stride;
  gui_debug_generate_images_ = generate_images;
  gui_debug_images_path_ = images_path;
}

float Replace::estimateTargetDensity(const PlaceOptions& options,
                                     const int threads)
{
  utl::validate(options, log_);
  log_->info(GPL, 98, "Initialize gpl and estimate target density.");
  log_->redirectStringBegin();

  PlaceOptions options_no_io = options;
  options_no_io.skipIo();  // in case bterms are not placed

  float density = 1.0f;
  bool initialized = initNesterovPlace(options_no_io, threads, false);
  log_->redirectStringEnd();  // discard output

  if (initialized) {
    density = getTopLevelNB()->estimateTargetDensity(
        options_no_io.nesterov.target_overflow);
  }

  return density;
}

const utl::Schema<InitialPlaceOptions>& InitialPlaceOptions::schema()
{
  using Options = InitialPlaceOptions;
  static const utl::Schema<Options> schema
      = utl::Schema<Options>(GPL, 440)
            .field("max_iter",
                   &Options::max_iter,
                   410,
                   "Iterations of the quadratic (BiCGSTAB) initial placement; "
                   "0 skips it. Initial placement improves HPWL by ~5% on "
                   "large designs.",
                   utl::NonNegative{})
            .field("min_diff_length",
                   &Options::min_diff_length,
                   432,
                   "Minimum length change that counts as progress.",
                   utl::Positive{})
            .field("max_solver_iter",
                   &Options::max_solver_iter,
                   433,
                   "Iterations of the linear solver per placement iteration.",
                   utl::Positive{})
            .field("max_fanout",
                   &Options::max_fanout,
                   411,
                   "Nets with more pins are ignored.",
                   utl::Positive{})
            .field("net_weight_scale",
                   &Options::net_weight_scale,
                   434,
                   "Scale of the net weights.",
                   utl::Positive{})
            .field("force_center",
                   &Options::force_center,
                   "Start from the core (or region) center even when the "
                   "instances already have locations.")
            .field("perturbation_seed",
                   &Options::perturbation_seed,
                   437,
                   "Seed of the random perturbation of the initial locations.",
                   utl::NonNegative{})
            .field("perturbation_dist",
                   &Options::perturbation_dist,
                   "Distance of the random perturbation in DBU; -1 for the "
                   "internal default.");
  return schema;
}

const utl::Schema<NesterovOptions>& NesterovOptions::schema()
{
  using Options = NesterovOptions;
  static const utl::Schema<Options> schema
      = utl::Schema<Options>(GPL, 441)
            .field("max_iter",
                   &Options::max_iter,
                   431,
                   "Maximum Nesterov iterations.",
                   utl::Positive{})
            .field("target_overflow",
                   &Options::target_overflow,
                   406,
                   "Overflow at which the placement stops.",
                   utl::Range{0.0, 1.0})
            .field("init_density_penalty",
                   &Options::init_density_penalty,
                   402,
                   "Initial density penalty.",
                   utl::Positive{})
            .field("init_wirelength_coef",
                   &Options::init_wirelength_coef,
                   403,
                   "Initial wirelength coefficient.",
                   utl::Positive{})
            .field("min_phi_coef",
                   &Options::min_phi_coef,
                   404,
                   "Lower bound of the step scaling (pcof_min); 0.95-1.05 "
                   "works well.",
                   utl::Positive{})
            .field("max_phi_coef",
                   &Options::max_phi_coef,
                   405,
                   "Upper bound of the step scaling (pcof_max); 1.00-1.20 "
                   "works well.",
                   utl::Positive{})
            .field("reference_hpwl",
                   &Options::reference_hpwl,
                   409,
                   "Wirelength change the density penalty controller "
                   "measures against, in DBU; 0 derives it from the design.",
                   utl::NonNegative{})
            .field("revert_if_diverge",
                   &Options::revert_if_diverge,
                   "Keep snapshots and revert to the last one when the "
                   "placement diverges.");
  return schema;
}

const utl::Schema<TimingOptions>& TimingOptions::schema()
{
  using Options = TimingOptions;
  static const utl::Schema<Options> schema
      = utl::Schema<Options>(GPL, 442)
            .field("enabled",
                   &Options::enabled,
                   "Reweight the nets with the worst slack, found by a "
                   "virtual repair_design.")
            .field("reweight_overflows",
                   &Options::reweight_overflows,
                   422,
                   "Overflows at which the slacks are recomputed and the "
                   "nets reweighted; fewer values cost less runtime.",
                   utl::Positive{})
            .field("net_weight_max",
                   &Options::net_weight_max,
                   423,
                   "Weight of the net with the worst slack; the weight "
                   "falls to 1 at nets_percentage.",
                   utl::Positive{})
            .field("nets_percentage",
                   &Options::nets_percentage,
                   424,
                   "Share of the nets, from the worst slack, that are "
                   "reweighted.",
                   utl::Percent{})
            .field("keep_resize_below_overflow",
                   &Options::keep_resize_below_overflow,
                   425,
                   "Below this overflow the resizer's changes are kept "
                   "(non-virtual repair_design).",
                   utl::Range{0.0, 1.0})
            .field("repair_timing",
                   &Options::repair_timing,
                   "Also run a conservative repair_timing when finding the "
                   "slacks.")
            .field("repair_tns_end_percent",
                   &Options::repair_tns_end_percent,
                   438,
                   "TNS end percent of that repair_timing.",
                   utl::Percent{})
            .group("repair_design",
                   &Options::repair_design,
                   "The repair_design run that finds the slacks.");
  return schema;
}

const utl::Schema<VirtualCtsOptions>& VirtualCtsOptions::schema()
{
  using Options = VirtualCtsOptions;
  static const utl::Schema<Options> schema
      = utl::Schema<Options>(GPL, 443)
            .field("enabled",
                   &Options::enabled,
                   "Model the clock insertion delay per sink with a "
                   "lightweight clock tree before each timing-driven "
                   "iteration.")
            .field("max_skew_fraction",
                   &Options::max_skew_fraction,
                   165,
                   "Maximum clock insertion delay as a fraction of the clock "
                   "period.",
                   utl::Range{0.0, 1.0});
  return schema;
}

const utl::Schema<RoutabilityOptions>& RoutabilityOptions::schema()
{
  using Options = RoutabilityOptions;
  static const utl::Schema<Options> schema
      = utl::Schema<Options>(GPL, 444)
            .field("enabled",
                   &Options::enabled,
                   "Inflate the cells in congested tiles until the target "
                   "RC metric is met.")
            .field("use_rudy",
                   &Options::use_rudy,
                   "Estimate congestion with RUDY; otherwise run the global "
                   "router (global_route below).")
            .field("check_overflow",
                   &Options::check_overflow,
                   414,
                   "Overflow below which congestion is checked.",
                   utl::Range{0.0, 1.0})
            .field("snapshot_overflow",
                   &Options::snapshot_overflow,
                   413,
                   "Overflow at which the placement is saved for a revert.",
                   utl::Range{0.0, 1.0})
            .field("max_density",
                   &Options::max_density,
                   415,
                   "Maximum target density inflation may raise it to.",
                   utl::Range{0.0, 1.0})
            .field("target_rc_metric",
                   &Options::target_rc_metric,
                   412,
                   "Routing congestion metric to reach; raise it when "
                   "routability iterations take too long.",
                   utl::Positive{})
            .field("inflation_ratio_coef",
                   &Options::inflation_ratio_coef,
                   416,
                   "Coefficient of the inflation ratio.",
                   utl::Positive{})
            .field("max_inflation_ratio",
                   &Options::max_inflation_ratio,
                   417,
                   "Maximum inflation of one cell.",
                   utl::Positive{})
            .field("min_congestion_for_inflation",
                   &Options::min_congestion_for_inflation,
                   436,
                   "Minimum tile congestion for its cells to be inflated.",
                   utl::NonNegative{})
            .field("max_inflation_total",
                   &Options::max_inflation_total,
                   426,
                   "Ceiling on the area inflation may add, as a fraction of "
                   "the movable area.",
                   utl::Positive{})
            .field("net_weight_max",
                   &Options::net_weight_max,
                   427,
                   "Wirelength weight of the nets that put the most wire "
                   "into congested tiles; 1 disables it.",
                   utl::Positive{})
            .field("congested_nets_percentage",
                   &Options::congested_nets_percentage,
                   428,
                   "Share of those nets that is weighted.",
                   utl::Percent{})
            .field("rc_coefficients",
                   &Options::rc_coefficients,
                   418,
                   "Coefficients {k1 k2 k3 k4} of the RC metric.",
                   utl::NonNegative{})
            .group("global_route",
                   &Options::global_route,
                   "The global route run when not using RUDY, and for the "
                   "congestion heatmap.");
  return schema;
}

const utl::Schema<PlaceOptions>& PlaceOptions::schema()
{
  using Options = PlaceOptions;
  static const utl::Schema<Options> schema
      = utl::Schema<Options>(GPL, 430)
            .field("density",
                   &Options::density,
                   401,
                   "Target density; 0.7 is 70%.",
                   utl::Range{0.0, 1.0})
            .field("uniform_density",
                   &Options::uniform_density,
                   "Use the minimum feasible target density instead of "
                   "density.")
            .field("bin_grid_count_x",
                   &Options::bin_grid_count_x,
                   400,
                   "Bin grid columns; 0 lets the placer choose. Powers of "
                   "two from 64 work best.",
                   utl::NonNegative{})
            .field("bin_grid_count_y",
                   &Options::bin_grid_count_y,
                   439,
                   "Bin grid rows; 0 lets the placer choose.",
                   utl::NonNegative{})
            .field("pad_left",
                   &Options::pad_left,
                   407,
                   "Left padding in sites.",
                   utl::NonNegative{})
            .field("pad_right",
                   &Options::pad_right,
                   408,
                   "Right padding in sites.",
                   utl::NonNegative{})
            .field("skip_io",
                   &Options::skip_io,
                   "Ignore the IO pins when computing wirelength.")
            .field("place_ios",
                   &Options::place_ios,
                   "Place the movable IO pins concurrently with the cells, "
                   "along the die perimeter. Not supported on a rectilinear "
                   "die.")
            .field("pin_density_adjust",
                   &Options::pin_density_adjust,
                   "Grow the area of pin-dense instances.")
            .field("routing_congestion",
                   &Options::routing_congestion,
                   "Run the global router after placement, for the routing "
                   "congestion heatmap.")
            .group("initial", &Options::initial, "Initial placement.")
            .group("nesterov", &Options::nesterov, "Nesterov placement.")
            .group("timing", &Options::timing, "Timing-driven placement.")
            .group("virtual_cts",
                   &Options::virtual_cts,
                   "Virtual clock tree for timing-driven placement.")
            .group("routability",
                   &Options::routability,
                   "Routability-driven placement.")
            .invariant(
                169,
                "place_ios cannot be used with skip_io placement.",
                [](const Options& o) { return !(o.place_ios && o.skip_io); });
  return schema;
}

void PlaceOptions::skipIo()
{
  skip_io = true;
  initial.max_iter = 0;
  timing.enabled = false;
  routability.enabled = false;
}

}  // namespace gpl
