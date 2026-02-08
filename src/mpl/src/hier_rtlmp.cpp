// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "hier_rtlmp.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <regex>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "MplObserver.h"
#include "SACoreHardMacro.h"
#include "SACoreSoftMacro.h"
#include "SimulatedAnnealingCore.h"
#include "boost/polygon/polygon.hpp"
#include "clusterEngine.h"
#include "db_sta/dbNetwork.hh"
#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"
#include "odb/util.h"
#include "par/PartitionMgr.h"
#include "utl/Logger.h"

namespace mpl {

///////////////////////////////////////////////////////////
// Class HierRTLMP
using utl::MPL;

HierRTLMP::~HierRTLMP() = default;

// Constructors
HierRTLMP::HierRTLMP(sta::dbNetwork* network,
                     odb::dbDatabase* db,
                     utl::Logger* logger,
                     par::PartitionMgr* tritonpart)
    : network_(network),
      db_(db),
      logger_(logger),
      tritonpart_(tritonpart),
      tree_(std::make_unique<PhysicalHierarchy>())
{
}

///////////////////////////////////////////////////////////////////////////
// Interfaces for setting up options
// Options related to macro placement

void HierRTLMP::setAreaWeight(float weight)
{
  placement_core_weights_.area = weight;
}

void HierRTLMP::setOutlineWeight(float weight)
{
  placement_core_weights_.outline = weight;
}

void HierRTLMP::setWirelengthWeight(float weight)
{
  placement_core_weights_.wirelength = weight;
}

void HierRTLMP::setGuidanceWeight(float weight)
{
  placement_core_weights_.guidance = weight;
}

void HierRTLMP::setFenceWeight(float weight)
{
  placement_core_weights_.fence = weight;
}

void HierRTLMP::setBoundaryWeight(float weight)
{
  cluster_placement_weights_.boundary = weight;
}

void HierRTLMP::setNotchWeight(float weight)
{
  cluster_placement_weights_.notch = weight;
}

void HierRTLMP::setMacroBlockageWeight(float weight)
{
  cluster_placement_weights_.macro_blockage = weight;
}

void HierRTLMP::setGlobalFence(odb::Rect global_fence)
{
  if (global_fence.area() > 0) {
    tree_->global_fence = global_fence;
  } else {
    tree_->global_fence = block_->getCoreArea();
  }
}

void HierRTLMP::setDefaultHalo(int halo_width, int halo_height)
{
  tree_->default_halo = {.width = halo_width, .height = halo_height};
}

void HierRTLMP::setGuidanceRegions(
    const std::map<odb::dbInst*, odb::Rect>& guidance_regions)
{
  guides_ = guidance_regions;
}

void HierRTLMP::setMacroHalo(odb::dbInst* macro,
                             int halo_width,
                             int halo_height)
{
  macro_to_halo_[macro] = {.width = halo_width, .height = halo_height};
}

// Options related to clustering

void HierRTLMP::setClusterSize(int max_num_macro,
                               int min_num_macro,
                               int max_num_inst,
                               int min_num_inst)
{
  tree_->base_max_macro = max_num_macro;
  tree_->base_min_macro = min_num_macro;
  tree_->base_max_std_cell = max_num_inst;
  tree_->base_min_std_cell = min_num_inst;
}

void HierRTLMP::setClusterSizeTolerance(float tolerance)
{
  tree_->cluster_size_tolerance = tolerance;
}

void HierRTLMP::setMaxNumLevel(int max_num_level)
{
  tree_->max_level = max_num_level;
}

void HierRTLMP::setClusterSizeRatioPerLevel(float coarsening_ratio)
{
  tree_->cluster_size_ratio = coarsening_ratio;
}

void HierRTLMP::setLargeNetThreshold(int large_net_threshold)
{
  tree_->large_net_threshold = large_net_threshold;
}

void HierRTLMP::setTargetUtil(float target_util)
{
  target_utilization_ = target_util;
}

void HierRTLMP::setMinAR(float min_ar)
{
  min_ar_ = min_ar;
}

void HierRTLMP::setReportDirectory(const char* report_directory)
{
  report_directory_ = report_directory;
}

void HierRTLMP::setKeepClusteringData(bool keep_clustering_data)
{
  keep_clustering_data_ = keep_clustering_data;
}

// Top Level Function
// The flow of our MacroPlacer is divided into 6 stages.
// 1) Multilevel Autoclustering:
//      Transform logical hierarchy into physical hierarchy.
// 2) Coarse Shaping -> Bottom - Up:
//      Determine the rough shape function for each cluster.
// 3) Fine Shaping -> Top - Down:
//      Refine the possible shapes of each cluster based on the fixed
//      outline and location of its parent cluster.
//      *This is executed within the cluster placement method.
// 4) Hierarchical Macro Placement -> Top - Down
//      a) Placement of Clusters (one level at a time);
//      b) Placement of Macros (one macro cluster at a time).
// 5) Boundary Pushing
//      Push macro clusters to the boundaries of the design if they don't
//      overlap with either pin access blockages or other macros.
// 6) Orientation Improvement
//      Attempts macro flipping to improve WR.
void HierRTLMP::run()
{
  runMultilevelAutoclustering();
  if (skip_macro_placement_) {
    logger_->info(MPL, 13, "Skipping macro placement.");
    return;
  }

  if (keep_clustering_data_) {
    commitClusteringDataToDb();
  }

  if (!tree_->has_std_cells) {
    resetSAParameters();
  }

  std::unique_ptr<MplObserver> save_graphics;
  if (is_debug_only_final_result_) {
    save_graphics = std::move(graphics_);
  }

  runCoarseShaping();
  runHierarchicalMacroPlacement();

  if (save_graphics) {
    graphics_ = std::move(save_graphics);
    graphics_->setMaxLevel(tree_->max_level);
    graphics_->drawResult();
  }

  Pusher pusher(logger_, tree_->root.get(), block_, io_blockages_);
  pusher.pushMacrosToCoreBoundaries();

  updateMacrosOnDb();

  generateTemporaryStdCellsPlacement(tree_->root.get());
  correctAllMacrosOrientation();

  commitMacroPlacementToDb();
  writeMacroPlacement(macro_placement_file_);

  clear();

  computeWireLength();
}

void HierRTLMP::init()
{
  block_ = db_->getChip()->getBlock();
  clustering_engine_ = std::make_unique<ClusteringEngine>(
      block_, network_, logger_, tritonpart_, graphics_.get());

  // Set target structure
  clustering_engine_->setTree(tree_.get());
}

////////////////////////////////////////////////////////////////////////
// Private functions
////////////////////////////////////////////////////////////////////////

// Transform the logical hierarchy into a physical hierarchy.
void HierRTLMP::runMultilevelAutoclustering()
{
  clustering_engine_ = std::make_unique<ClusteringEngine>(
      block_, network_, logger_, tritonpart_, graphics_.get());

  // Set target structure
  clustering_engine_->setTree(tree_.get());
  clustering_engine_->setHalos(macro_to_halo_);
  clustering_engine_->run();

  if (!tree_->has_unfixed_macros) {
    skip_macro_placement_ = true;
    return;
  }

  if (graphics_) {
    graphics_->finishedClustering(tree_.get());
  }
}

void HierRTLMP::runHierarchicalMacroPlacement()
{
  if (graphics_) {
    graphics_->startFine();
  }

  adjustMacroBlockageWeight();
  tiny_cluster_max_number_of_std_cells_
      = computeTinyClusterMaxNumberOfStdCells();
  placeChildren(tree_->root.get());
}

int HierRTLMP::computeTinyClusterMaxNumberOfStdCells() const
{
  const float tiny_cluster_ratio = 0.001;
  const int max_number_of_std_cells
      = tiny_cluster_ratio * block_->getInsts().size();

  debugPrint(
      logger_,
      MPL,
      "fine_shaping",
      1,
      "Std cell clusters with less than {} cells will be considered tiny.",
      max_number_of_std_cells);

  return max_number_of_std_cells;
}

void HierRTLMP::resetSAParameters()
{
  pos_swap_prob_ = 0.2;
  neg_swap_prob_ = 0.2;
  double_swap_prob_ = 0.2;
  exchange_swap_prob_ = 0.2;
  resize_prob_ = 0.0;

  placement_core_weights_.fence = 0.0;

  cluster_placement_weights_.boundary = 0.0;
  cluster_placement_weights_.notch = 0.0;
  cluster_placement_weights_.macro_blockage = 0.0;
}

void HierRTLMP::runCoarseShaping()
{
  setRootShapes();

  if (tree_->has_only_macros) {
    logger_->warn(MPL, 27, "Design has only macros!");
    tree_->root->setClusterType(HardMacroCluster);
    return;
  }

  if (graphics_) {
    graphics_->startCoarse();
  }

  calculateChildrenTilings(tree_->root.get());

  searchAvailableRegionsForUnconstrainedPins();
  createPinAccessBlockages();
  setPlacementBlockages();
}

void HierRTLMP::setRootShapes()
{
  auto root_soft_macro = std::make_unique<SoftMacro>(tree_->root.get());

  const int64_t root_area = tree_->floorplan_shape.area();
  const int root_width = tree_->floorplan_shape.dx();
  const IntervalList root_width_intervals = {Interval(root_width, root_width)};

  root_soft_macro->setShapes(root_width_intervals, root_area);
  root_soft_macro->setWidth(root_width);  // This will set height automatically
  root_soft_macro->setX(tree_->floorplan_shape.xMin());
  root_soft_macro->setY(tree_->floorplan_shape.yMin());
  tree_->root->setSoftMacro(std::move(root_soft_macro));
}

// Determine the macro tilings within each cluster in a bottom-up manner.
// (Post-Order DFS manner)
// Coarse shaping:  In this step, we only consider the size of macros
// Ignore all the standard-cell clusters.
// At this stage, we assume the standard cell placer will automatically
// place standard cells in the empty space between macros.
void HierRTLMP::calculateChildrenTilings(Cluster* parent)
{
  // base case, no macros in current cluster
  if (parent->getNumMacro() == 0) {
    return;
  }

  debugPrint(logger_,
             MPL,
             "coarse_shaping",
             1,
             "Determine shapes for {}",
             parent->getName());

  // Current cluster is a hard macro cluster
  if (parent->getClusterType() == HardMacroCluster) {
    debugPrint(logger_,
               MPL,
               "coarse_shaping",
               1,
               "{} is a Macro cluster",
               parent->getName());
    calculateMacroTilings(parent);
    return;
  }

  if (!parent->getChildren().empty()) {
    debugPrint(logger_,
               MPL,
               "coarse_shaping",
               1,
               "Started visiting children of {}",
               parent->getName());

    // Recursively visit the children of Mixed Cluster
    for (auto& cluster : parent->getChildren()) {
      if (cluster->getNumMacro() > 0) {
        calculateChildrenTilings(cluster.get());
      }
    }

    debugPrint(logger_,
               MPL,
               "coarse_shaping",
               1,
               "Done visiting children of {}",
               parent->getName());
  }

  if (graphics_) {
    graphics_->setCurrentCluster(parent);
  }

  const odb::Rect outline = tree_->root->getBBox();

  std::vector<SoftMacro> macros;
  for (auto& cluster : parent->getChildren()) {
    if (cluster->isFixedMacro()) {
      macros.emplace_back(logger_, cluster->getHardMacros().front(), &outline);
      continue;
    }

    if (cluster->getNumMacro() > 0) {
      SoftMacro macro = SoftMacro(cluster.get());
      if (macro.isMacroCluster()) {
        macro.setShapes(cluster->getTilings(), true /* force */);
      } else { /* Mixed */
        const TilingList& tilings = cluster->getTilings();
        IntervalList width_intervals = computeWidthIntervals(tilings);
        // Note that we can use the area of any tiling.
        macro.setShapes(width_intervals, tilings.front().area());
      }

      macros.push_back(std::move(macro));
    }
  }
  // if there is only one soft macro
  // the parent cluster has the shape of the its child macro cluster
  if (macros.size() == 1) {
    for (auto& cluster : parent->getChildren()) {
      if (cluster->getNumMacro() > 0) {
        parent->setTilings(cluster->getTilings());
        return;
      }
    }
  }

  TilingSet tilings_set;
  // the probability of all actions should be summed to 1.0.
  const float action_sum = pos_swap_prob_ + neg_swap_prob_ + double_swap_prob_
                           + exchange_swap_prob_ + resize_prob_;

  const int num_perturb_per_step = (macros.size() > num_perturb_per_step_ / 10)
                                       ? macros.size()
                                       : num_perturb_per_step_ / 10;
  // we vary the outline of parent cluster to generate different tilings
  // we first vary the outline width while keeping outline height fixed
  // Then we vary the outline height while keeping outline width fixed
  // Vary the outline width
  std::vector<float> vary_factor_list{1.0};
  float vary_step = 1.0 / num_runs_;  // change the outline by based on num_runs
  for (int i = 1; i < num_runs_; i++) {
    vary_factor_list.push_back(1.0 - i * vary_step);
  }
  int remaining_runs = num_runs_;
  int run_id = 0;

  while (remaining_runs > 0) {
    SoftSAVector sa_batch;
    const int run_thread
        = graphics_ ? 1 : std::min(remaining_runs, num_threads_);
    for (int i = 0; i < run_thread; i++) {
      odb::Rect new_outline = outline;
      const int new_width = outline.dx() * vary_factor_list[run_id++];
      new_outline.set_xhi(new_outline.xMin() + new_width);

      if (graphics_) {
        graphics_->setOutline(new_outline);
      }
      std::unique_ptr<SACoreSoftMacro> sa
          = std::make_unique<SACoreSoftMacro>(tree_.get(),
                                              new_outline,
                                              macros,
                                              shaping_core_weights_,
                                              SASoftWeights(),
                                              0.0,  // no notch size
                                              0.0,  // no notch size
                                              pos_swap_prob_ / action_sum,
                                              neg_swap_prob_ / action_sum,
                                              double_swap_prob_ / action_sum,
                                              exchange_swap_prob_ / action_sum,
                                              resize_prob_ / action_sum,
                                              init_prob_,
                                              max_num_step_,
                                              num_perturb_per_step,
                                              random_seed_,
                                              graphics_.get(),
                                              logger_,
                                              block_);
      sa_batch.push_back(std::move(sa));
    }
    if (sa_batch.size() == 1) {
      runSA<SACoreSoftMacro>(sa_batch[0].get());
    } else {
      // multi threads
      std::vector<std::thread> threads;
      threads.reserve(sa_batch.size());
      for (auto& sa : sa_batch) {
        threads.emplace_back(runSA<SACoreSoftMacro>, sa.get());
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    // add macro tilings
    for (auto& sa : sa_batch) {
      if (sa->fitsIn(outline)) {
        tilings_set.insert({sa->getWidth(), sa->getHeight()});
      }
    }
    remaining_runs -= run_thread;
  }
  // vary the outline height while keeping outline width fixed
  remaining_runs = num_runs_;
  run_id = 0;
  while (remaining_runs > 0) {
    SoftSAVector sa_batch;
    const int run_thread
        = graphics_ ? 1 : std::min(remaining_runs, num_threads_);
    for (int i = 0; i < run_thread; i++) {
      odb::Rect new_outline = outline;
      const int new_height = outline.dy() * vary_factor_list[run_id++];
      new_outline.set_yhi(new_outline.yMin() + new_height);

      if (graphics_) {
        graphics_->setOutline(new_outline);
      }
      std::unique_ptr<SACoreSoftMacro> sa
          = std::make_unique<SACoreSoftMacro>(tree_.get(),
                                              new_outline,
                                              macros,
                                              shaping_core_weights_,
                                              SASoftWeights(),
                                              0.0,  // no notch size
                                              0.0,  // no notch size
                                              pos_swap_prob_ / action_sum,
                                              neg_swap_prob_ / action_sum,
                                              double_swap_prob_ / action_sum,
                                              exchange_swap_prob_ / action_sum,
                                              resize_prob_ / action_sum,
                                              init_prob_,
                                              max_num_step_,
                                              num_perturb_per_step,
                                              random_seed_,
                                              graphics_.get(),
                                              logger_,
                                              block_);
      sa_batch.push_back(std::move(sa));
    }
    if (sa_batch.size() == 1) {
      runSA<SACoreSoftMacro>(sa_batch.front().get());
    } else {
      // multi threads
      std::vector<std::thread> threads;
      threads.reserve(sa_batch.size());
      for (auto& sa : sa_batch) {
        threads.emplace_back(runSA<SACoreSoftMacro>, sa.get());
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    // add macro tilings
    for (auto& sa : sa_batch) {
      if (sa->fitsIn(outline)) {
        tilings_set.insert({sa->getWidth(), sa->getHeight()});
      }
    }
    remaining_runs -= run_thread;
  }

  if (tilings_set.empty()) {
    logger_->error(MPL,
                   3,
                   "There are no valid tilings for mixed cluster: {}",
                   parent->getName());
  }

  TilingList tilings_list(tilings_set.begin(), tilings_set.end());
  std::ranges::sort(tilings_list, isAreaSmaller);

  for (auto& tiling : tilings_list) {
    debugPrint(logger_,
               MPL,
               "coarse_shaping",
               2,
               "width: {}, height: {}, aspect_ratio: {}, min_ar: {}",
               tiling.width(),
               tiling.height(),
               tiling.aspectRatio(),
               min_ar_);
  }

  // we do not want very strange tilings if we have choices
  TilingList new_tilings_list;
  for (auto& tiling : tilings_list) {
    if (tiling.aspectRatio() >= min_ar_
        && tiling.aspectRatio() <= 1.0 / min_ar_) {
      new_tilings_list.push_back(tiling);
    }
  }

  if (!new_tilings_list.empty()) {
    tilings_list = std::move(new_tilings_list);
  }

  parent->setTilings(tilings_list);

  if (!tilings_list.empty()) {
    std::string line
        = "The macro tiling for mixed cluster " + parent->getName() + "  ";
    for (auto& tiling : tilings_list) {
      line += " < " + std::to_string(tiling.width()) + " , ";
      line += std::to_string(tiling.height()) + " >  ";
    }
    line += "\n";
    debugPrint(logger_, MPL, "coarse_shaping", 2, "{}", line);
  }
}

IntervalList HierRTLMP::computeWidthIntervals(const TilingList& tilings)
{
  IntervalList width_intervals;
  width_intervals.reserve(tilings.size());
  for (const Tiling& tiling : tilings) {
    width_intervals.emplace_back(tiling.width(), tiling.width());
  }

  std::ranges::sort(width_intervals, isMinWidthSmaller);

  return width_intervals;
}

void HierRTLMP::calculateMacroTilings(Cluster* cluster)
{
  if (cluster->isFixedMacro()) {
    return;
  }

  std::vector<HardMacro*> hard_macros = cluster->getHardMacros();
  int number_of_macros = hard_macros.size();
  int macro_width = hard_macros.front()->getWidth();
  int macro_height = hard_macros.front()->getHeight();

  TilingList tilings = generateTilingsForMacroCluster(
      macro_width, macro_height, number_of_macros);

  if (tilings.empty()) {
    tilings = generateTilingsForMacroCluster(
        macro_width, macro_height, number_of_macros + 1);
  }

  if (tilings.empty()) {
    logger_->error(MPL,
                   4,
                   "Unable to fit cluster {} within outline. Macro height: {}, "
                   "width: {}, number of macros: {}.",
                   cluster->getName(),
                   macro_width,
                   macro_height,
                   number_of_macros);
  }

  cluster->setTilings(tilings);

  if (logger_->debugCheck(MPL, "coarse_shaping", 2)) {
    std::string line = "Tiling for hard cluster " + cluster->getName()
                       + " with " + std::to_string(number_of_macros)
                       + " macros.\n";
    for (auto& tiling : tilings) {
      line += " < " + std::to_string(tiling.width()) + " , ";
      line += std::to_string(tiling.height()) + " >  ";
    }
    line += "\n";
    debugPrint(logger_, MPL, "coarse_shaping", 2, "{}", line);
  }
}

TilingList HierRTLMP::generateTilingsForMacroCluster(int macro_width,
                                                     int macro_height,
                                                     int number_of_macros)
{
  TilingList tight_packing_tilings;
  const odb::Rect outline = tree_->root->getBBox();

  int rows = 0;
  for (int cols = 1; cols <= number_of_macros; cols++) {
    if (number_of_macros % cols == 0) {
      rows = number_of_macros / cols;

      if (cols * macro_width <= outline.dx()
          && rows * macro_height <= outline.dy()) {
        tight_packing_tilings.emplace_back(cols * macro_width,
                                           rows * macro_height);
      }
    }
  }

  return tight_packing_tilings;
}

void HierRTLMP::searchAvailableRegionsForUnconstrainedPins()
{
  if (!treeHasUnconstrainedIOs()) {
    return;
  }

  const std::vector<odb::Rect>& blocked_regions_for_pins
      = block_->getBlockedRegionsForPins();

  BoundaryToRegionsMap boundary_to_blocked_regions
      = getBoundaryToBlockedRegionsMap(blocked_regions_for_pins);
  std::vector<odb::Rect> available_regions
      = computeAvailableRegions(boundary_to_blocked_regions);

  tree_->available_regions_for_unconstrained_pins.reserve(
      available_regions.size());
  for (const odb::Rect& region : available_regions) {
    tree_->available_regions_for_unconstrained_pins.emplace_back(
        rectToLine(block_, region, logger_), getBoundary(block_, region));
  }

  if (graphics_) {
    graphics_->setBlockedRegionsForPins(blocked_regions_for_pins);
    graphics_->setAvailableRegionsForUnconstrainedPins(
        tree_->available_regions_for_unconstrained_pins);
  }
}

// If there are no constraints at all, we give freedom to SA so it
// doesn't have to deal with pin access blockages across the entire
// extension of all edges of the die area.
void HierRTLMP::createPinAccessBlockages()
{
  if (!tree_->io_pads.empty()) {
    return;
  }

  const Metrics* top_module_metrics
      = tree_->maps.module_to_metrics.at(block_->getTopModule()).get();

  // Avoid creating blockages with zero depth.
  if (top_module_metrics->getStdCellArea() == 0) {
    return;
  }

  computePinAccessDepthLimits();

  createBlockagesForIOBundles();
  createBlockagesForAvailableRegions();
  createBlockagesForConstraintRegions();
}

void HierRTLMP::computePinAccessDepthLimits()
{
  const odb::Rect die = block_->getDieArea();

  constexpr float max_depth_proportion = 0.10;
  pin_access_depth_limits_.x.max = max_depth_proportion * die.dx();
  pin_access_depth_limits_.y.max = max_depth_proportion * die.dy();

  constexpr float min_depth_proportion = 0.04;
  const int proportional_min_width = min_depth_proportion * die.dx();
  const int proportional_min_height = min_depth_proportion * die.dy();

  const Tiling tiling = tree_->root->getTilings().front();
  // Required for designs that are too tight (i.e. MockArray)
  const int tiling_min_width = (die.dx() - tiling.width()) / 2;
  const int tiling_min_height = (die.dy() - tiling.height()) / 2;

  pin_access_depth_limits_.x.min
      = std::min(proportional_min_width, tiling_min_width);
  pin_access_depth_limits_.y.min
      = std::min(proportional_min_height, tiling_min_height);

  if (logger_->debugCheck(MPL, "coarse_shaping", 1)) {
    logger_->report("\n  Pin Access Depth (μm)  |  Min  |  Max");
    logger_->report("-----------------------------------------");
    logger_->report("             Horizontal  | {:>5.2f} | {:>6.2f}",
                    block_->dbuToMicrons(pin_access_depth_limits_.x.min),
                    block_->dbuToMicrons(pin_access_depth_limits_.x.max));
    logger_->report("               Vertical  | {:>5.2f} | {:>6.2f}\n",
                    block_->dbuToMicrons(pin_access_depth_limits_.y.min),
                    block_->dbuToMicrons(pin_access_depth_limits_.y.max));
  }
}

void HierRTLMP::createBlockagesForIOBundles()
{
  const std::vector<Cluster*> io_bundles = getIOBundles();
  if (io_bundles.empty()) {
    return;
  }

  int io_bundles_span = 0;
  for (Cluster* io_bundle : io_bundles) {
    // The shape of an IO bundle is just a line.
    io_bundles_span += io_bundle->getBBox().margin() / 2;
  }

  int total_fixed_ios = 0;
  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    if (bterm->getFirstPinPlacementStatus().isFixed()) {
      ++total_fixed_ios;
    }
  }

  const int base_depth = computePinAccessBaseDepth(io_bundles_span);

  for (Cluster* io_bundle : io_bundles) {
    const float number_of_ios
        = static_cast<float>(clustering_engine_->getNumberOfIOs(io_bundle));
    const float io_density_factor = 1.0 + (number_of_ios / total_fixed_ios);
    const int depth = base_depth * io_density_factor;
    const odb::Rect rect = io_bundle->getBBox();
    const odb::Line line = rectToLine(block_, rect, logger_);
    const BoundaryRegion region(line, getBoundary(block_, rect));
    createPinAccessBlockage(region, depth);
  }
}

std::vector<Cluster*> HierRTLMP::getIOBundles() const
{
  std::vector<Cluster*> io_bundles;
  for (const auto& child : tree_->root->getChildren()) {
    if (child->isIOBundle()) {
      io_bundles.push_back(child.get());
    }
  }
  return io_bundles;
}

bool HierRTLMP::treeHasUnconstrainedIOs() const
{
  std::vector<Cluster*> io_clusters = getClustersOfUnplacedIOPins();
  for (Cluster* io_cluster : io_clusters) {
    if (io_cluster->isClusterOfUnconstrainedIOPins()) {
      return true;
    }
  }
  return false;
}

bool HierRTLMP::treeHasConstrainedIOs() const
{
  std::vector<Cluster*> io_clusters = getClustersOfUnplacedIOPins();
  for (Cluster* io_cluster : io_clusters) {
    if (!io_cluster->isClusterOfUnconstrainedIOPins()) {
      return true;
    }
  }
  return false;
}

void HierRTLMP::createBlockagesForAvailableRegions()
{
  if (block_->getBlockedRegionsForPins().empty()) {
    return;
  }

  int io_span = 0;
  for (const BoundaryRegion& region :
       tree_->available_regions_for_unconstrained_pins) {
    io_span += std::sqrt(
        odb::Point::squaredDistance(region.line.pt0(), region.line.pt1()));
  }

  const int depth = computePinAccessBaseDepth(io_span);

  for (const BoundaryRegion region :
       tree_->available_regions_for_unconstrained_pins) {
    createPinAccessBlockage(region, depth);
  }
}

void HierRTLMP::createBlockagesForConstraintRegions()
{
  if (!treeHasConstrainedIOs()) {
    return;
  }

  int io_span = 0;
  std::vector<Cluster*> clusters_of_unplaced_ios
      = getClustersOfUnplacedIOPins();

  for (Cluster* cluster_of_unplaced_ios : clusters_of_unplaced_ios) {
    if (cluster_of_unplaced_ios->isClusterOfUnconstrainedIOPins()) {
      continue;
    }

    // Reminder: the region rect is always a line for a cluster
    // of constrained pins.
    const odb::Rect region = cluster_of_unplaced_ios->getBBox();
    io_span += region.margin() / 2;
  }

  const int base_depth = computePinAccessBaseDepth(io_span);
  int total_ios = 0;
  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    if (!bterm->getFirstPinPlacementStatus().isFixed()) {
      ++total_ios;
    }
  }

  for (Cluster* cluster_of_unplaced_ios : clusters_of_unplaced_ios) {
    if (cluster_of_unplaced_ios->isClusterOfUnconstrainedIOPins()) {
      continue;
    }

    const int cluster_number_of_ios
        = clustering_engine_->getNumberOfIOs(cluster_of_unplaced_ios);

    const float io_density_factor
        = 1.0 + (cluster_number_of_ios / static_cast<float>(total_ios));
    const int depth = base_depth * io_density_factor;

    const odb::Rect region_rect = cluster_of_unplaced_ios->getBBox();
    const odb::Line region_line = rectToLine(block_, region_rect, logger_);
    const BoundaryRegion region(region_line, getBoundary(block_, region_rect));
    createPinAccessBlockage(region, depth);
  }
}

BoundaryToRegionsMap HierRTLMP::getBoundaryToBlockedRegionsMap(
    const std::vector<odb::Rect>& blocked_regions_for_pins) const
{
  BoundaryToRegionsMap boundary_to_blocked_regions;
  std::queue<odb::Rect> blocked_regions;

  boundary_to_blocked_regions[Boundary::L] = blocked_regions;
  boundary_to_blocked_regions[Boundary::R] = blocked_regions;
  boundary_to_blocked_regions[Boundary::B] = blocked_regions;
  boundary_to_blocked_regions[Boundary::T] = blocked_regions;

  for (const odb::Rect& blocked_region : blocked_regions_for_pins) {
    Boundary boundary = getBoundary(block_, blocked_region);
    boundary_to_blocked_regions.at(boundary).push(blocked_region);

    debugPrint(logger_,
               MPL,
               "coarse_shaping",
               1,
               "Found blocked region {} in {} boundary.",
               blocked_region,
               toString(boundary));
  }

  return boundary_to_blocked_regions;
}

std::vector<odb::Rect> HierRTLMP::computeAvailableRegions(
    BoundaryToRegionsMap& boundary_to_blocked_regions) const
{
  std::vector<odb::Rect> available_regions;

  for (auto& [boundary, blocked_regions] : boundary_to_blocked_regions) {
    // The initial available region is the entire edge of the die area.
    std::vector<odb::Rect> boundary_available_regions = {getRect(boundary)};

    while (!blocked_regions.empty()) {
      odb::Rect blocked_region = blocked_regions.front();
      blocked_regions.pop();

      std::vector<odb::Rect> new_boundary_available_regions;
      for (const odb::Rect& boundary_available_region :
           boundary_available_regions) {
        if (!boundary_available_region.contains(blocked_region)) {
          new_boundary_available_regions.push_back(boundary_available_region);
          continue;
        }

        std::vector<odb::Rect> subtraction_result
            = subtractOverlapRegion(boundary_available_region, blocked_region);
        new_boundary_available_regions.insert(
            new_boundary_available_regions.end(),
            subtraction_result.begin(),
            subtraction_result.end());
      }

      boundary_available_regions = std::move(new_boundary_available_regions);
    }

    available_regions.insert(available_regions.end(),
                             boundary_available_regions.begin(),
                             boundary_available_regions.end());
  }

  return available_regions;
}

void HierRTLMP::createPinAccessBlockage(const BoundaryRegion& region,
                                        const int depth)
{
  int blockage_depth = depth;

  const Interval& limits = isVertical(region.boundary)
                               ? pin_access_depth_limits_.x
                               : pin_access_depth_limits_.y;

  if (blockage_depth > limits.max) {
    blockage_depth = limits.max;
  } else if (blockage_depth < limits.min) {
    blockage_depth = limits.min;
  }

  debugPrint(logger_,
             MPL,
             "coarse_shaping",
             1,
             "Creating pin access blockage in {} -> Region line = ({}) ({}) , "
             "Depth = {} um",
             toString(region.boundary),
             region.line.pt0(),
             region.line.pt1(),
             block_->dbuToMicrons(blockage_depth));

  odb::Rect blockage = lineToRect(region.line);
  switch (region.boundary) {
    case (Boundary::L): {
      blockage.set_xhi(blockage.xMin() + blockage_depth);
      break;
    }
    case (Boundary::R): {
      blockage.set_xlo(blockage.xMax() - blockage_depth);
      break;
    }
    case (Boundary::B): {
      blockage.set_yhi(blockage.yMin() + blockage_depth);
      break;
    }
    case (Boundary::T): {
      blockage.set_ylo(blockage.yMax() - blockage_depth);
      break;
    }
  }

  io_blockages_.push_back(blockage);
}

std::vector<Cluster*> HierRTLMP::getClustersOfUnplacedIOPins() const
{
  std::vector<Cluster*> clusters_of_unplaced_io_pins;
  for (const auto& child : tree_->root->getChildren()) {
    if (child->isClusterOfUnplacedIOPins()) {
      clusters_of_unplaced_io_pins.push_back(child.get());
    }
  }
  return clusters_of_unplaced_io_pins;
}

// The base depth of pin access blockages is computed based on:
// 1) Amount of std cell area in the design.
// 2) Extension of IO span.
// 3) Macro dominance quadratic factor.
int HierRTLMP::computePinAccessBaseDepth(const int io_span) const
{
  int64_t std_cell_area = 0;
  for (auto& cluster : tree_->root->getChildren()) {
    if (cluster->getClusterType() == StdCellCluster) {
      std_cell_area += cluster->getArea();
    }
  }

  if (std_cell_area == 0) {
    for (auto& cluster : tree_->root->getChildren()) {
      if (cluster->getClusterType() == MixedCluster) {
        std_cell_area += cluster->getArea();
      }
    }
  }

  int64_t root_area = (tree_->root->getWidth()
                       * static_cast<int64_t>(tree_->root->getHeight()));
  const double macro_dominance_factor
      = tree_->macro_with_halo_area / static_cast<double>(root_area);

  const int base_depth = std_cell_area / static_cast<double>(io_span)
                         * std::pow((1 - macro_dominance_factor), 2);

  debugPrint(logger_,
             MPL,
             "coarse_shaping",
             1,
             "Base pin access depth: {} μm",
             block_->dbuToMicrons(base_depth));

  return base_depth;
}

void HierRTLMP::setPlacementBlockages()
{
  for (odb::dbBlockage* blockage : block_->getBlockages()) {
    odb::Rect bbox = blockage->getBBox()->getBox();
    placement_blockages_.push_back(bbox);
  }
}

// Merge nets to reduce runtime
void HierRTLMP::mergeNets(BundledNetList& nets)
{
  std::vector<int> net_class(nets.size(), -1);
  for (int i = 0; i < nets.size(); i++) {
    if (net_class[i] == -1) {
      for (int j = i + 1; j < nets.size(); j++) {
        if (nets[i] == nets[j]) {
          net_class[j] = i;
        }
      }
    }
  }

  for (int i = 0; i < net_class.size(); i++) {
    if (net_class[i] > -1) {
      nets[net_class[i]].weight += nets[i].weight;
    }
  }

  BundledNetList merged_nets;
  for (int i = 0; i < net_class.size(); i++) {
    if (net_class[i] == -1) {
      merged_nets.push_back(nets[i]);
    }
  }

  nets = std::move(merged_nets);
}

void HierRTLMP::setMacroClustersShapes(
    std::vector<SoftMacro>& soft_macros) const
{
  for (SoftMacro& soft_macro : soft_macros) {
    if (soft_macro.isMacroCluster() && !soft_macro.isFixed()) {
      Cluster* cluster = soft_macro.getCluster();
      const TilingList& tilings = cluster->getTilings();
      soft_macro.setShapes(tilings);
    }
  }
}

// Recommendation from the original implementation:
// For single level, increase macro blockage weight to
// half of the outline weight.
void HierRTLMP::adjustMacroBlockageWeight()
{
  if (tree_->max_level == 1) {
    float new_macro_blockage_weight = placement_core_weights_.outline / 2.0;
    debugPrint(logger_,
               MPL,
               "hierarchical_macro_placement",
               1,
               "Tree max level is {}, Changing macro blockage weight from {} "
               "to {} (half of the outline weight)",
               tree_->max_level,
               cluster_placement_weights_.macro_blockage,
               new_macro_blockage_weight);
    cluster_placement_weights_.macro_blockage = new_macro_blockage_weight;
  }
}

void HierRTLMP::placeChildren(Cluster* parent)
{
  if (parent->getClusterType() == HardMacroCluster) {
    placeMacros(parent);
    return;
  }

  // Cover IO Clusters, Leaf Std Cells and Fixed Macros.
  if (parent->isLeaf()) {
    return;
  }

  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Placing children of cluster {}",
             parent->getName());

  for (auto& cluster : parent->getChildren()) {
    clustering_engine_->updateInstancesAssociation(cluster.get());
  }

  if (graphics_) {
    graphics_->setCurrentCluster(parent);
  }

  const odb::Rect outline = parent->getBBox();
  if (graphics_) {
    graphics_->setOutline(outline);
  }

  SoftMacroNameToIdMap soft_macro_id_map;
  std::map<int, odb::Rect> fences;
  std::map<int, odb::Rect> guides;
  std::vector<SoftMacro> macros;

  std::vector<odb::Rect> blockages = findBlockagesWithinOutline(outline);
  eliminateOverlaps(blockages);
  createSoftMacrosForBlockages(blockages, macros);

  // We store the io clusters to push them into the macros' vector
  // only after it is already populated with the clusters we're trying to
  // place. This will facilitate how we deal with fixed terminals in SA moves.
  std::vector<Cluster*> io_clusters;

  // Each cluster is modeled as Soft Macro
  // The fences or guides for each cluster is created by merging
  // the fences and guides for hard macros in each cluster
  for (auto& cluster : parent->getChildren()) {
    if (cluster->isIOCluster()) {
      io_clusters.push_back(cluster.get());
      continue;
    }

    soft_macro_id_map[cluster->getName()] = macros.size();

    if (cluster->isFixedMacro()) {
      macros.emplace_back(logger_, cluster->getHardMacros().front(), &outline);
      continue;
    }

    auto soft_macro = std::make_unique<SoftMacro>(cluster.get());
    // Needed for computing the nets.
    clustering_engine_->updateInstancesAssociation(cluster.get());
    macros.push_back(*soft_macro);
    cluster->setSoftMacro(std::move(soft_macro));

    // merge fences and guides for hard macros within cluster
    if (cluster->getClusterType() == StdCellCluster) {
      continue;
    }
    odb::Rect fence, guide;
    fence.mergeInit();
    guide.mergeInit();
    const std::vector<HardMacro*> hard_macros = cluster->getHardMacros();
    for (auto& hard_macro : hard_macros) {
      if (fences_.find(hard_macro->getName()) != fences_.end()) {
        fence.merge(fences_[hard_macro->getName()]);
      }
      auto itr = guides_.find(hard_macro->getInst());
      if (itr != guides_.end()) {
        guide.merge(itr->second);
      }
    }

    fence.intersection(outline, fence);
    guide.intersection(outline, guide);

    if (fence.area() > 0) {
      // current macro id is macros.size() - 1
      fence.moveDelta(-outline.xMin(), -outline.yMin());
      fences[macros.size() - 1] = fence;
    }
    if (guide.area() > 0) {
      // current macro id is macros.size() - 1
      guide.moveDelta(-outline.xMin(), -outline.yMin());
      guides[macros.size() - 1] = guide;
    }
  }

  if (graphics_) {
    graphics_->setGuides(guides);
    graphics_->setFences(fences);
  }

  const int number_of_sequence_pair_macros = static_cast<int>(macros.size());

  for (Cluster* io_cluster : io_clusters) {
    soft_macro_id_map[io_cluster->getName()] = macros.size();

    macros.emplace_back(odb::Point(io_cluster->getX() - outline.xMin(),
                                   io_cluster->getY() - outline.yMin()),
                        io_cluster->getName(),
                        io_cluster->getWidth(),
                        io_cluster->getHeight(),
                        io_cluster);
  }

  createFixedTerminals(parent, soft_macro_id_map, macros);

  clustering_engine_->rebuildConnections();

  BundledNetList nets = buildBundledNets(parent, soft_macro_id_map);
  mergeNets(nets);

  if (graphics_) {
    graphics_->setNets(nets);
  }

  std::string file_name_prefix
      = report_directory_ + "/"
        + std::regex_replace(
            parent->getName(), std::regex("[\":<>\\|\\*\\?/]"), "--");
  if (logger_->debugCheck(MPL, "hierarchical_macro_placement", 1)) {
    writeNetFile(file_name_prefix, macros, nets);
  }

  setMacroClustersShapes(macros);

  // The sum of probabilities should be 1.
  const float action_sum = pos_swap_prob_ + neg_swap_prob_ + double_swap_prob_
                           + exchange_swap_prob_ + resize_prob_;

  // The number of perturbations in each step should be larger than the
  // number of macros
  const int num_perturb_per_step
      = std::max(static_cast<int>(macros.size()), num_perturb_per_step_);

  const int total_number_of_runs = 10;
  std::vector<float> utilization_list
      = computeUtilizationList(total_number_of_runs);
  int remaining_runs = total_number_of_runs;
  int run_id = 0;

  std::unique_ptr<SACoreSoftMacro> best_sa;
  while (remaining_runs > 0) {
    SoftSAVector sa_batch;
    const int number_of_attempts
        = graphics_ ? 1 : std::min(remaining_runs, num_threads_);

    for (int i = 0; i < number_of_attempts; i++) {
      const float utilization = utilization_list[run_id++];
      if (!validUtilization(utilization, outline, macros)) {
        continue;
      }

      std::vector<SoftMacro> inflated_macros
          = applyUtilization(utilization, outline, macros);

      const bool single_array_single_std_cell_cluster
          = singleArraySingleStdCellCluster(macros);

      std::unique_ptr<SACoreSoftMacro> sa
          = std::make_unique<SACoreSoftMacro>(tree_.get(),
                                              outline,
                                              inflated_macros,
                                              placement_core_weights_,
                                              cluster_placement_weights_,
                                              notch_h_th_,
                                              notch_v_th_,
                                              pos_swap_prob_ / action_sum,
                                              neg_swap_prob_ / action_sum,
                                              double_swap_prob_ / action_sum,
                                              exchange_swap_prob_ / action_sum,
                                              resize_prob_ / action_sum,
                                              init_prob_,
                                              max_num_step_,
                                              num_perturb_per_step,
                                              random_seed_,
                                              graphics_.get(),
                                              logger_,
                                              block_);
      sa->setNumberOfSequencePairMacros(number_of_sequence_pair_macros);
      sa->enableEnhancements();
      sa->setFences(fences);
      sa->setGuides(guides);
      sa->setNets(nets);
      if (single_array_single_std_cell_cluster) {
        sa->forceCentralization();
      }
      sa_batch.push_back(std::move(sa));
    }

    if (graphics_ && !sa_batch.empty()) {
      runSA<SACoreSoftMacro>(sa_batch.front().get());
    } else {
      std::vector<std::thread> threads;
      threads.reserve(sa_batch.size());
      for (auto& sa : sa_batch) {
        threads.emplace_back(runSA<SACoreSoftMacro>, sa.get());
      }
      for (auto& th : threads) {
        th.join();
      }
    }

    const bool first_batch = remaining_runs == total_number_of_runs;
    remaining_runs -= number_of_attempts;

    for (int sa_index = 0; sa_index < sa_batch.size(); ++sa_index) {
      auto& sa = sa_batch[sa_index];

      if (sa->isValid()) {
        if (!first_batch || sa != sa_batch.front()) {
          const int utilization_index
              = (run_id - number_of_attempts) + sa_index;

          logger_->warn(MPL,
                        55,
                        "Couldn't find a solution for the specified "
                        "utilization. The utilization was adjusted to {}.",
                        utilization_list[utilization_index]);
        }

        best_sa = std::move(sa);
        break;
      }
    }

    if (best_sa) {
      break;
    }
  }

  if (!best_sa) {
    logger_->error(MPL,
                   40,
                   "Annealing engine failed to find a valid solution.\nCluster "
                   "Id: {}\n Cluster Name: {}",
                   parent->getId(),
                   parent->getName());
  }

  best_sa->fillDeadSpace();

  std::vector<SoftMacro> placed_macros = best_sa->getMacros();

  if (logger_->debugCheck(MPL, "hierarchical_macro_placement", 1)) {
    logger_->report("Cluster Placement Summary");
    printPlacementResult(parent, outline, best_sa.get());

    writeFloorplanFile(file_name_prefix, placed_macros);
    writeCostFile(file_name_prefix, best_sa.get());
  }

  updateChildrenShapesAndLocations(parent, placed_macros, soft_macro_id_map);
  updateChildrenRealLocation(parent, outline.xMin(), outline.yMin());

  for (auto& cluster : parent->getChildren()) {
    placeChildren(cluster.get());
  }

  clustering_engine_->updateInstancesAssociation(parent);
}

std::vector<float> HierRTLMP::computeUtilizationList(
    const float total_number_of_runs) const
{
  std::vector<float> utilization_list(total_number_of_runs);
  const float maximum_utilization = 1.0;
  const float exponential_ratio
      = std::pow(maximum_utilization / target_utilization_,
                 (1.0 / (total_number_of_runs - 1.0)));

  for (int i = 0; i < total_number_of_runs; i++) {
    utilization_list[i] = target_utilization_ * std::pow(exponential_ratio, i);

    debugPrint(logger_,
               MPL,
               "fine_shaping",
               1,
               "Utilization List[{}] = {}",
               i,
               utilization_list[i]);
  }

  return utilization_list;
}

// Find the area of blockages that are inside the outline.
std::vector<odb::Rect> HierRTLMP::findBlockagesWithinOutline(
    const odb::Rect& outline) const
{
  std::vector<odb::Rect> blockages_within_outline;

  for (auto& blockage : placement_blockages_) {
    getBlockageRegionWithinOutline(blockages_within_outline, blockage, outline);
  }

  for (auto& blockage : io_blockages_) {
    getBlockageRegionWithinOutline(blockages_within_outline, blockage, outline);
  }

  return blockages_within_outline;
}

void HierRTLMP::getBlockageRegionWithinOutline(
    std::vector<odb::Rect>& blockages_within_outline,
    const odb::Rect& blockage,
    const odb::Rect& outline) const
{
  const int b_lx = std::max(outline.xMin(), blockage.xMin());
  const int b_ly = std::max(outline.yMin(), blockage.yMin());
  const int b_ux = std::min(outline.xMax(), blockage.xMax());
  const int b_uy = std::min(outline.yMax(), blockage.yMax());

  if ((b_ux - b_lx > 0) && (b_uy - b_ly > 0)) {
    blockages_within_outline.emplace_back(b_lx - outline.xMin(),
                                          b_ly - outline.yMin(),
                                          b_ux - outline.xMin(),
                                          b_uy - outline.yMin());
  }
}

void HierRTLMP::eliminateOverlaps(std::vector<odb::Rect>& blockages) const
{
  namespace gtl = boost::polygon;
  using gtl::operators::operator+=;
  using PolygonSet = gtl::polygon_90_set_data<int>;

  PolygonSet polygons;
  for (const odb::Rect& blockage : blockages) {
    polygons += blockage;
  }

  blockages.clear();

  std::vector<odb::Rect> new_blockages;
  polygons.get_rectangles(new_blockages);

  for (const odb::Rect& new_blockage : new_blockages) {
    blockages.push_back(new_blockage);
  }
}

void HierRTLMP::createSoftMacrosForBlockages(
    const std::vector<odb::Rect>& blockages,
    std::vector<SoftMacro>& macros)
{
  for (int id = 0; id < blockages.size(); id++) {
    std::string name = fmt::format("blockage_{}", id);
    macros.emplace_back(blockages[id], name);
  }
}

// Create terminals for cluster placement (Soft) annealing.
void HierRTLMP::createFixedTerminals(
    Cluster* parent,
    std::map<std::string, int>& soft_macro_id_map,
    std::vector<SoftMacro>& soft_macros)
{
  if (!parent->getParent()) {
    return;
  }

  odb::Rect outline = parent->getBBox();
  std::queue<Cluster*> parents;
  parents.push(parent);

  while (!parents.empty()) {
    Cluster* frontwave = parents.front();
    parents.pop();

    Cluster* grandparent = frontwave->getParent();
    for (auto& cluster : grandparent->getChildren()) {
      if (cluster->getId() != frontwave->getId()) {
        const int id = static_cast<int>(soft_macros.size());
        soft_macro_id_map[cluster->getName()] = id;
        createFixedTerminal(cluster.get(), outline, soft_macros);
      }
    }

    if (frontwave->getParent()->getParent()) {
      parents.push(frontwave->getParent());
    }
  }
}

template <typename Macro>
void HierRTLMP::createFixedTerminal(Cluster* cluster,
                                    const odb::Rect& outline,
                                    std::vector<Macro>& macros)
{
  // A conventional fixed terminal is just a point without
  // the cluster data.
  odb::Point location = cluster->getCenter();
  int width = 0;
  int height = 0;
  Cluster* terminal_cluster = nullptr;

  if (cluster->isClusterOfUnplacedIOPins()) {
    // Clusters of unplaced IOs are not treated as conventional
    // fixed terminals. As they correspond to regions, we need
    // both their actual shape and their cluster data inside SA.
    location = {cluster->getX(), cluster->getY()};
    width = cluster->getWidth();
    height = cluster->getHeight();
    terminal_cluster = cluster;
  }

  location.addX(-outline.xMin());
  location.addY(-outline.yMin());

  macros.emplace_back(
      location, cluster->getName(), width, height, terminal_cluster);
}

bool HierRTLMP::validUtilization(
    const float utilization,
    const odb::Rect& outline,
    const std::vector<SoftMacro>& soft_macros) const
{
  int64_t blocked_area = 0;
  int64_t std_cell_cluster_area = 0;
  int64_t mixed_cluster_std_cell_area = 0;
  int64_t macro_cluster_area = 0;
  int64_t mixed_cluster_macro_area = 0;

  for (const SoftMacro& soft_macro : soft_macros) {
    Cluster* cluster = soft_macro.getCluster();

    if (!cluster) {
      blocked_area += soft_macro.getArea();  // Physical area.
      continue;
    }

    if (cluster->isIOCluster()) {
      continue;
    }

    if (cluster->isFixedMacro()) {
      // Here we need to use the area of the SoftMacro in order to cover the
      // cases in which the fixed macro is partially inside the outline.
      macro_cluster_area += soft_macro.getArea();
      continue;
    }

    switch (cluster->getClusterType()) {
      case StdCellCluster: {
        std_cell_cluster_area += cluster->getStdCellArea();
        break;
      }
      case HardMacroCluster: {
        const TilingList& tilings = cluster->getTilings();
        macro_cluster_area += tilings.front().area();
        break;
      }
      case MixedCluster: {
        const TilingList& tilings = cluster->getTilings();
        mixed_cluster_macro_area += tilings.front().area();
        mixed_cluster_std_cell_area += cluster->getStdCellArea();
      }
    }
  }

  const int64_t hard_area
      = blocked_area + macro_cluster_area + mixed_cluster_macro_area;
  const int64_t available_area = outline.area() - hard_area;

  const int64_t soft_area = std_cell_cluster_area + mixed_cluster_std_cell_area;
  const int64_t inflated_soft_area = soft_area / utilization;

  return inflated_soft_area < available_area;
}

std::vector<SoftMacro> HierRTLMP::applyUtilization(
    const float utilization,
    const odb::Rect& outline,
    const std::vector<SoftMacro>& original_soft_macros) const
{
  std::vector<SoftMacro> new_soft_macros = original_soft_macros;
  const bool single_array_single_std_cell_cluster
      = singleArraySingleStdCellCluster(original_soft_macros);

  for (SoftMacro& new_soft_macro : new_soft_macros) {
    Cluster* cluster = new_soft_macro.getCluster();

    if (!cluster || cluster->isIOCluster() || cluster->isFixedMacro()) {
      continue;
    }

    if (new_soft_macro.isStdCellCluster()) {
      int64_t area = cluster->getArea();
      int width = std::sqrt(area);
      int height = width;

      if (cluster->getNumStdCell() <= tiny_cluster_max_number_of_std_cells_
          || single_array_single_std_cell_cluster) {
        const int negligible_width = 1;
        width = negligible_width;
        height = width;
        area = width * static_cast<int64_t>(height);
      } else {
        area = cluster->getArea() / utilization;
        width = std::sqrt(area / min_ar_);
      }

      const int minimum_width = area / width;
      const int maximum_width = width;
      Interval width_interval(minimum_width, maximum_width);

      new_soft_macro.setShapes({width_interval}, area);
    }

    if (new_soft_macro.isMixedCluster()) {
      const TilingList& tilings = cluster->getTilings();
      int64_t macro_area = tilings.back().area();
      int64_t inflated_area
          = macro_area + (cluster->getStdCellArea() / utilization);

      IntervalList width_intervals;
      for (const Tiling& tiling : tilings) {
        const int minimum_width = tiling.width();
        const int maximum_width = inflated_area / tiling.height();
        width_intervals.emplace_back(minimum_width, maximum_width);
      }

      new_soft_macro.setShapes(width_intervals, inflated_area);
    }
  }

  if (logger_->debugCheck(MPL, "fine_shaping", 1)) {
    reportShapeCurves(new_soft_macros);
  }

  return new_soft_macros;
}

bool HierRTLMP::singleArraySingleStdCellCluster(
    const std::vector<SoftMacro>& soft_macros) const
{
  int number_of_macro_arrays = 0;
  int number_of_std_clusters = 0;

  for (const SoftMacro& soft_macro : soft_macros) {
    if (soft_macro.isMixedCluster()) {
      return false;
    }

    Cluster* cluster = soft_macro.getCluster();

    if (!cluster || cluster->isIOCluster()) {
      continue;
    }

    if (soft_macro.isMacroCluster()) {
      if (!cluster->isArrayOfInterconnectedMacros()) {
        return false;
      }

      ++number_of_macro_arrays;
    } else if (soft_macro.isStdCellCluster()) {
      ++number_of_std_clusters;
    }

    if (number_of_macro_arrays > 1 || number_of_std_clusters > 1) {
      return false;
    }
  }

  if (number_of_macro_arrays == 0 || number_of_std_clusters == 0) {
    return false;
  }

  return true;
}

void HierRTLMP::placeMacros(Cluster* cluster)
{
  if (cluster->isFixedMacro()) {
    return;
  }

  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Placing macros of macro cluster {}",
             cluster->getName());

  if (graphics_) {
    graphics_->setCurrentCluster(cluster);
  }

  UniqueClusterVector macro_clusters;  // needed to calculate connections
  std::vector<HardMacro*> hard_macros = cluster->getHardMacros();
  std::vector<HardMacro> sa_macros;
  std::map<int, int> cluster_to_macro;
  std::set<odb::dbMaster*> masters;
  clustering_engine_->createTempMacroClusters(
      hard_macros, sa_macros, macro_clusters, cluster_to_macro, masters);

  const odb::Rect outline = cluster->getBBox();

  std::map<int, odb::Rect> fences;
  std::map<int, odb::Rect> guides;
  computeFencesAndGuides(hard_macros, outline, fences, guides);
  if (graphics_) {
    graphics_->setGuides(guides);
    graphics_->setFences(fences);
  }

  clustering_engine_->rebuildConnections();

  createFixedTerminals(outline, macro_clusters, cluster_to_macro, sa_macros);
  BundledNetList nets = buildBundledNets(macro_clusters, cluster_to_macro);

  if (graphics_) {
    graphics_->setNets(nets);
  }

  // Use exchange more often when there are more instances of a common
  // master.
  float exchange_swap_prob
      = exchange_swap_prob_ * 5
        * (1 - (masters.size() / (float) hard_macros.size()));

  // set the action probabilities (summation to 1.0)
  const float action_sum = pos_swap_prob_ * 10 + neg_swap_prob_ * 10
                           + double_swap_prob_ + exchange_swap_prob;

  float pos_swap_prob = pos_swap_prob_ * 10 / action_sum;
  float neg_swap_prob = neg_swap_prob_ * 10 / action_sum;
  float double_swap_prob = double_swap_prob_ / action_sum;
  exchange_swap_prob = exchange_swap_prob / action_sum;

  const int number_of_sequence_pair_macros
      = static_cast<int>(hard_macros.size());
  const int minimum_perturbations_per_step = num_perturb_per_step_ / 10;
  const bool large_macro_cluster
      = number_of_sequence_pair_macros > minimum_perturbations_per_step;

  int perturbations_per_step = large_macro_cluster
                                   ? number_of_sequence_pair_macros
                                   : minimum_perturbations_per_step;

  SequencePair initial_seq_pair;
  bool invalid_states_allowed = true;
  if (cluster->isMacroArray()) {
    bool array_has_empty_space = false;
    initial_seq_pair = computeArraySequencePair(cluster, array_has_empty_space);

    if (array_has_empty_space) {
      invalid_states_allowed = false;
    } else {
      // We don't need to explore different shapes, so we just swap
      // macros until we find the best wire length.
      pos_swap_prob = 0.0f;
      neg_swap_prob = 0.0f;
      double_swap_prob = 0.0f;
      exchange_swap_prob = 1.0f;
    }

    // Large arrays need more steps to properly converge.
    if (large_macro_cluster) {
      perturbations_per_step = num_perturb_per_step_;
    }
  }

  int run_id = 0;
  int remaining_runs = num_runs_;
  float best_cost = std::numeric_limits<float>::max();

  SACoreHardMacro* best_sa = nullptr;
  HardSAVector sa_containers;  // The owner of the SACore objects.

  while (remaining_runs > 0) {
    HardSAVector sa_batch;
    const int run_thread
        = graphics_ ? 1 : std::min(remaining_runs, num_threads_);

    for (int i = 0; i < run_thread; i++) {
      if (graphics_) {
        graphics_->setOutline(outline);
      }

      SACoreWeights new_weights = placement_core_weights_;
      new_weights.outline *= (run_id + 1) * 10;
      new_weights.wirelength /= (run_id + 1);

      std::unique_ptr<SACoreHardMacro> sa
          = std::make_unique<SACoreHardMacro>(tree_.get(),
                                              outline,
                                              sa_macros,
                                              new_weights,
                                              pos_swap_prob,
                                              neg_swap_prob,
                                              double_swap_prob,
                                              exchange_swap_prob,
                                              init_prob_,
                                              max_num_step_,
                                              perturbations_per_step,
                                              random_seed_ + run_id,
                                              graphics_.get(),
                                              logger_,
                                              block_);
      sa->setNumberOfSequencePairMacros(number_of_sequence_pair_macros);
      sa->setNets(nets);
      sa->setFences(fences);
      sa->setGuides(guides);
      sa->setInitialSequencePair(initial_seq_pair);
      if (!invalid_states_allowed) {
        sa->disallowInvalidStates();
      }

      sa_batch.push_back(std::move(sa));

      run_id++;
    }
    if (sa_batch.size() == 1) {
      runSA<SACoreHardMacro>(sa_batch[0].get());
    } else {
      std::vector<std::thread> threads;
      threads.reserve(sa_batch.size());

      for (auto& sa : sa_batch) {
        threads.emplace_back(runSA<SACoreHardMacro>, sa.get());
      }

      for (auto& th : threads) {
        th.join();
      }
    }

    for (auto& sa : sa_batch) {
      // Reset weights so we can compare the final costs.
      sa->setWeights(placement_core_weights_);

      if (sa->isValid() && sa->getNormCost() < best_cost) {
        best_cost = sa->getNormCost();
        best_sa = sa.get();
      }

      sa_containers.push_back(std::move(sa));
    }

    remaining_runs -= run_thread;
  }

  if (best_sa == nullptr) {
    for (auto& sa : sa_containers) {
      sa->printResults();
    }
    logger_->error(MPL,
                   10,
                   "Macro placement failed for macro cluster: {}",
                   cluster->getName());
  } else {
    std::vector<HardMacro> best_macros = best_sa->getMacros();

    if (logger_->debugCheck(MPL, "hierarchical_macro_placement", 1)) {
      logger_->report("Macro Placement Summary");
      printPlacementResult(cluster, outline, best_sa);
    }

    for (int i = 0; i < hard_macros.size(); i++) {
      *(hard_macros[i]) = best_macros[i];
    }
  }

  for (auto& hard_macro : hard_macros) {
    hard_macro->setX(hard_macro->getX() + outline.xMin());
    hard_macro->setY(hard_macro->getY() + outline.yMin());
  }

  clustering_engine_->clearTempMacroClusterMapping(macro_clusters);
  clustering_engine_->updateInstancesAssociation(cluster);
}

// Suppose we have a 2x2 array such as:
//      +-----++-----+
//      |     ||     |
//      |  0  ||  2  |
//      |     ||     |
//      +-----++-----+
//      +-----++-----+
//      |     ||     |
//      |  1  ||  3  |
//      |     ||     |
//      +-----++-----+
//  We can represent it through a sequence pair in which:
// 1) The positive sequence is made by going each column top
//    to bottom starting from the upper left corner: 0 1 2 3
// 2) The negative sequence is made by going each column bottom
//    to top starting from the lower left corner: 1 0 3 2
//
// Obs: Doing this will still keep the IO clusters at the end
// of the sequence pair, which is needed for the way SA handles it.
SequencePair HierRTLMP::computeArraySequencePair(Cluster* cluster,
                                                 bool& array_has_empty_space)
{
  SequencePair sequence_pair;
  const std::vector<HardMacro*>& hard_macros = cluster->getHardMacros();
  const int number_of_macros = static_cast<int>(hard_macros.size());

  for (int id = 0; id < number_of_macros; ++id) {
    sequence_pair.pos_sequence.push_back(id);
  }

  const HardMacro* hard_macro = hard_macros.front();
  const int columns = std::round(cluster->getWidth() / hard_macro->getWidth());
  const int rows = std::round(cluster->getHeight() / hard_macro->getHeight());

  for (int i = 1; i <= columns; ++i) {
    for (int j = 1; j <= rows; j++) {
      const int macro_id = (rows * i) - j;
      if (macro_id < number_of_macros) {
        sequence_pair.neg_sequence.push_back(macro_id);
      } else {
        array_has_empty_space = true;
      }
    }
  }

  return sequence_pair;
}

void HierRTLMP::computeFencesAndGuides(
    const std::vector<HardMacro*>& hard_macros,
    const odb::Rect& outline,
    std::map<int, odb::Rect>& fences,
    std::map<int, odb::Rect>& guides)
{
  for (int i = 0; i < hard_macros.size(); ++i) {
    if (fences_.find(hard_macros[i]->getName()) != fences_.end()) {
      fences_[hard_macros[i]->getName()].intersection(outline, fences[i]);
      fences[i].moveDelta(-outline.xMin(), -outline.yMin());
    }
    auto itr = guides_.find(hard_macros[i]->getInst());
    if (itr != guides_.end()) {
      itr->second.intersection(outline, guides[i]);
      guides[i].moveDelta(-outline.xMin(), -outline.yMin());
    }
  }
}

// Create terminals for macro placement (Hard) annealing.
void HierRTLMP::createFixedTerminals(const odb::Rect& outline,
                                     const UniqueClusterVector& macro_clusters,
                                     std::map<int, int>& cluster_to_macro,
                                     std::vector<HardMacro>& sa_macros)
{
  std::set<int> clusters_ids;

  for (auto& macro_cluster : macro_clusters) {
    for (auto [cluster_id, weight] : macro_cluster->getConnectionsMap()) {
      clusters_ids.insert(cluster_id);
    }
  }

  for (auto cluster_id : clusters_ids) {
    if (cluster_to_macro.find(cluster_id) != cluster_to_macro.end()) {
      continue;
    }

    Cluster* temp_cluster = tree_->maps.id_to_cluster[cluster_id];
    const int terminal_id = static_cast<int>(sa_macros.size());

    cluster_to_macro[cluster_id] = terminal_id;
    createFixedTerminal(temp_cluster, outline, sa_macros);
  }
}

BundledNetList HierRTLMP::buildBundledNets(
    Cluster* parent,
    const SoftMacroNameToIdMap& soft_macro_id_map) const
{
  BundledNetList nets;
  const float virtual_connections_weight = 10.0f;

  for (const auto& [a_id, b_id] : parent->getVirtualConnections()) {
    Cluster* a = tree_->maps.id_to_cluster.at(a_id);
    Cluster* b = tree_->maps.id_to_cluster.at(b_id);
    const int macro_a_id = soft_macro_id_map.at(a->getName());
    const int macro_b_id = soft_macro_id_map.at(b->getName());

    nets.emplace_back(macro_a_id, macro_b_id, virtual_connections_weight);
  }

  for (const auto& child : parent->getChildren()) {
    const int source_macro_id = soft_macro_id_map.at(child->getName());
    const ConnectionsMap& connections_map = child->getConnectionsMap();

    for (const auto& [cluster_id, connection_weight] : connections_map) {
      Cluster* target_cluster = tree_->maps.id_to_cluster.at(cluster_id);
      const int target_macro_id
          = soft_macro_id_map.at(target_cluster->getName());

      // As connections are undirected and therefore exist in both directions,
      // this check prevents connections from being taken twice into account.
      if (child->getId() > target_cluster->getId()) {
        nets.emplace_back(source_macro_id, target_macro_id, connection_weight);
      }
    }
  }

  return nets;
}

BundledNetList HierRTLMP::buildBundledNets(
    const UniqueClusterVector& clusters,
    const ClusterToMacroMap& cluster_to_macro) const
{
  BundledNetList nets;

  for (const auto& cluster : clusters) {
    const int source_macro_id = cluster_to_macro.at(cluster->getId());
    const ConnectionsMap& connections_map = cluster->getConnectionsMap();

    for (const auto& [cluster_id, connection_weight] : connections_map) {
      const int target_macro_id = cluster_to_macro.at(cluster_id);
      nets.emplace_back(source_macro_id, target_macro_id, connection_weight);
    }
  }

  return nets;
}

// Update the location based on the parent's perspective.
void HierRTLMP::updateChildrenShapesAndLocations(
    Cluster* parent,
    const std::vector<SoftMacro>& shaped_macros,
    const std::map<std::string, int>& soft_macro_id_map)
{
  for (auto& child : parent->getChildren()) {
    // IO clusters are fixed and have no area,
    // so their shapes and locations should not be updated
    if (!child->isIOCluster()) {
      *(child->getSoftMacro())
          = shaped_macros[soft_macro_id_map.at(child->getName())];
    }
  }
}

// Move children to their real location in the die area. The offset is the
// parent's origin.
void HierRTLMP::updateChildrenRealLocation(Cluster* parent,
                                           float offset_x,
                                           float offset_y)
{
  for (auto& child : parent->getChildren()) {
    child->setX(child->getX() + offset_x);
    child->setY(child->getY() + offset_y);
  }
}

void HierRTLMP::setTemporaryStdCellLocation(Cluster* cluster,
                                            odb::dbInst* std_cell)
{
  const SoftMacro* soft_macro = cluster->getSoftMacro();

  if (!soft_macro) {
    return;
  }

  const int soft_macro_center_x_dbu = soft_macro->getPinX();
  const int soft_macro_center_y_dbu = soft_macro->getPinY();

  // Std cells are placed in the center of the cluster they belong to
  std_cell->setLocation(
      (soft_macro_center_x_dbu - std_cell->getBBox()->getDX() / 2),
      (soft_macro_center_y_dbu - std_cell->getBBox()->getDY() / 2));

  std_cell->setPlacementStatus(odb::dbPlacementStatus::PLACED);
}

void HierRTLMP::setModuleStdCellsLocation(Cluster* cluster,
                                          odb::dbModule* module)
{
  for (odb::dbInst* inst : module->getInsts()) {
    if (!inst->isCore()) {
      continue;
    }
    setTemporaryStdCellLocation(cluster, inst);
  }

  for (odb::dbModInst* mod_insts : module->getChildren()) {
    setModuleStdCellsLocation(cluster, mod_insts->getMaster());
  }
}

// Update the locations of std cells in odb using the locations that
// HierRTLMP estimates for the leaf standard clusters. This is needed
// for the orientation improvement step.
void HierRTLMP::generateTemporaryStdCellsPlacement(Cluster* cluster)
{
  if (cluster->isLeaf() && cluster->getNumStdCell() != 0) {
    for (odb::dbModule* module : cluster->getDbModules()) {
      setModuleStdCellsLocation(cluster, module);
    }

    for (odb::dbInst* leaf_std_cell : cluster->getLeafStdCells()) {
      setTemporaryStdCellLocation(cluster, leaf_std_cell);
    }
  } else {
    for (const auto& child : cluster->getChildren()) {
      generateTemporaryStdCellsPlacement(child.get());
    }
  }
}

float HierRTLMP::calculateRealMacroWirelength(odb::dbInst* macro)
{
  int64_t wirelength = 0;

  for (odb::dbITerm* macro_pin : macro->getITerms()) {
    if (macro_pin->getSigType() != odb::dbSigType::SIGNAL) {
      continue;
    }

    odb::dbNet* net = macro_pin->getNet();
    if (net != nullptr) {
      // Mimic dbNet::getTermBBox() behavior, but considering
      // the pin constraint region instead of its position.
      odb::Rect net_box;
      net_box.mergeInit();

      for (odb::dbITerm* iterm : net->getITerms()) {
        int x, y;
        if (iterm->getAvgXY(&x, &y)) {
          odb::Rect iterm_rect(x, y, x, y);
          net_box.merge(iterm_rect);
        }
      }

      for (odb::dbBTerm* bterm : net->getBTerms()) {
        if (bterm->getFirstPinPlacementStatus().isFixed()) {
          const odb::Rect pin_bbox = bterm->getBBox();
          const odb::Rect center_rect(pin_bbox.center(), pin_bbox.center());
          net_box.merge(center_rect);
        } else {
          const auto& constraint_region = bterm->getConstraintRegion();
          odb::Point nearest_point;
          if (constraint_region) {
            BoundaryRegion constraint(
                rectToLine(block_, *constraint_region, logger_),
                getBoundary(block_, *constraint_region));
            computeDistToNearestRegion(
                macro_pin->getBBox().center(), {constraint}, &nearest_point);
          } else {
            computeDistToNearestRegion(
                macro_pin->getBBox().center(),
                tree_->available_regions_for_unconstrained_pins,
                &nearest_point);
          }
          odb::Rect point_rect(nearest_point, nearest_point);
          net_box.merge(point_rect);
        }
      }

      wirelength += (net_box.dx() + net_box.dy());
    }
  }

  return wirelength;
}

void HierRTLMP::flipRealMacro(odb::dbInst* macro, const bool& is_vertical_flip)
{
  if (is_vertical_flip) {
    macro->setOrient(macro->getOrient().flipY());
  } else {
    macro->setOrient(macro->getOrient().flipX());
  }
}

void HierRTLMP::adjustRealMacroOrientation(const bool& is_vertical_flip)
{
  for (odb::dbInst* inst : block_->getInsts()) {
    if (!inst->isBlock() || inst->isFixed()) {
      continue;
    }

    const float original_wirelength = calculateRealMacroWirelength(inst);
    odb::Point macro_location = inst->getLocation();

    // Flipping is done by mirroring the macro about the "Y" or "X" axis,
    // so, after flipping, we must manually set the location (lower-left corner)
    // again to move the macro back to the the position choosen by mpl.
    flipRealMacro(inst, is_vertical_flip);
    inst->setLocation(macro_location.getX(), macro_location.getY());
    const float new_wirelength = calculateRealMacroWirelength(inst);

    debugPrint(logger_,
               MPL,
               "flipping",
               1,
               "Inst {} flip {} orig_WL {} new_WL {}",
               inst->getName(),
               is_vertical_flip ? "V" : "H",
               original_wirelength,
               new_wirelength);

    if (new_wirelength > original_wirelength) {
      flipRealMacro(inst, is_vertical_flip);
      inst->setLocation(macro_location.getX(), macro_location.getY());
    }
  }
}

void HierRTLMP::correctAllMacrosOrientation()
{
  // Apply vertical flip if necessary
  adjustRealMacroOrientation(true);

  // Apply horizontal flip if necessary
  adjustRealMacroOrientation(false);
}

void HierRTLMP::updateMacrosOnDb()
{
  for (const auto& [inst, hard_macro] : tree_->maps.inst_to_hard) {
    updateMacroOnDb(hard_macro.get());
  }
}

// We don't lock the macros here, because we'll attempt to improve
// orientation next.
void HierRTLMP::updateMacroOnDb(const HardMacro* hard_macro)
{
  odb::dbInst* inst = hard_macro->getInst();

  if (!inst || inst->isFixed()) {
    return;
  }

  const int x = hard_macro->getRealX();
  const int y = hard_macro->getRealY();

  // Orientation must be set before location so we don't end up flipping
  // and misplacing the macro.
  inst->setOrient(hard_macro->getOrientation());
  inst->setLocation(x, y);
  inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
}

void HierRTLMP::commitMacroPlacementToDb()
{
  Snapper snapper(logger_);

  for (auto& [inst, hard_macro] : tree_->maps.inst_to_hard) {
    if (!inst || inst->isFixed()) {
      continue;
    }

    snapper.setMacro(inst);
    snapper.snapMacro();

    inst->setPlacementStatus(odb::dbPlacementStatus::LOCKED);
  }
}

void HierRTLMP::commitClusteringDataToDb() const
{
  createGroupForCluster(tree_->root.get(), nullptr);
}

void HierRTLMP::createGroupForCluster(Cluster* cluster,
                                      odb::dbGroup* parent_group) const
{
  if (cluster->isIOCluster()) {
    return;
  }

  odb::dbGroup* cluster_group
      = parent_group != nullptr
            ? odb::dbGroup::create(parent_group, cluster->getName().c_str())
            : odb::dbGroup::create(block_, cluster->getName().c_str());

  cluster_group->setType(odb::dbGroupType::VISUAL_DEBUG);

  for (odb::dbInst* inst : cluster->getLeafStdCells()) {
    cluster_group->addInst(inst);
  }

  for (odb::dbInst* macro : cluster->getLeafMacros()) {
    cluster_group->addInst(macro);
  }

  for (odb::dbModule* module : cluster->getDbModules()) {
    for (odb::dbInst* inst : module->getLeafInsts()) {
      cluster_group->addInst(inst);
    }
  }

  for (const auto& child : cluster->getChildren()) {
    createGroupForCluster(child.get(), cluster_group);
  }
}

void HierRTLMP::setMacroPlacementFile(const std::string& file_name)
{
  macro_placement_file_ = file_name;
}

void HierRTLMP::writeMacroPlacement(const std::string& file_name)
{
  if (file_name.empty()) {
    return;
  }

  std::ofstream out(file_name);

  if (!out) {
    logger_->error(MPL, 11, "Cannot open file {}.", file_name);
  }

  out << odb::generateMacroPlacementString(block_);
}

void HierRTLMP::clear()
{
  tree_.reset();

  if (graphics_) {
    graphics_->eraseDrawing();
  }
}

void HierRTLMP::computeWireLength() const
{
  odb::WireLengthEvaluator wirelength_evaluator(block_);
  logger_->metric("macro_place__wirelength",
                  block_->dbuToMicrons(wirelength_evaluator.hpwl()));
}

void HierRTLMP::setDebug(std::unique_ptr<MplObserver>& graphics)
{
  graphics_ = std::move(graphics);
}

void HierRTLMP::setDebugShowBundledNets(bool show_bundled_nets)
{
  graphics_->setShowBundledNets(show_bundled_nets);
}

void HierRTLMP::setDebugShowClustersIds(bool show_clusters_ids)
{
  graphics_->setShowClustersIds(show_clusters_ids);
}

void HierRTLMP::setDebugSkipSteps(bool skip_steps)
{
  graphics_->setSkipSteps(skip_steps);
}

void HierRTLMP::setDebugOnlyFinalResult(bool only_final_result)
{
  graphics_->setOnlyFinalResult(only_final_result);
  is_debug_only_final_result_ = only_final_result;
}

void HierRTLMP::setDebugTargetClusterId(const int target_cluster_id)
{
  graphics_->setTargetClusterId(target_cluster_id);
}

// Example for a vertical region:
//  Base - Overlay = Result
//   |                  |
//   |                  | "b"
//   |                  |
//   |        |
//   |        |
//   |        |
//   |                  |
//   |                  | "a"
//   |                  |
std::vector<odb::Rect> HierRTLMP::subtractOverlapRegion(
    const odb::Rect& base,
    const odb::Rect& overlay) const
{
  Boundary base_boundary = getBoundary(block_, base);

  if (base_boundary != getBoundary(block_, overlay)) {
    logger_->critical(
        MPL, 46, "Attempting to subtract regions from different boundaries.");
  }

  std::vector<odb::Rect> result;
  odb::Rect a = base;
  odb::Rect b = base;

  if (isVertical(base_boundary)) {
    a.set_yhi(overlay.yMin());
    b.set_ylo(overlay.yMax());
  } else {
    a.set_xhi(overlay.xMin());
    b.set_xlo(overlay.xMax());
  }

  if (a.dx() != 0 || a.dy() != 0) {
    result.push_back(a);
  }

  if (b.dx() != 0 || b.dy() != 0) {
    result.push_back(b);
  }

  return result;
}

odb::Rect HierRTLMP::getRect(Boundary boundary) const
{
  odb::Rect boundary_rect = block_->getDieArea();

  switch (boundary) {
    case (Boundary::L): {
      boundary_rect.set_xhi(boundary_rect.xMin());
      break;
    }
    case (Boundary::R): {
      boundary_rect.set_xlo(boundary_rect.xMax());
      break;
    }
    case (Boundary::T): {
      boundary_rect.set_ylo(boundary_rect.yMax());
      break;
    }
    case (Boundary::B): {
      boundary_rect.set_yhi(boundary_rect.yMin());
      break;
    }
  }

  return boundary_rect;
}

void HierRTLMP::reportShapeCurves(
    const std::vector<SoftMacro>& soft_macros) const
{
  logger_->report("\nReporting Shape Curves\n");

  for (const SoftMacro& soft_macro : soft_macros) {
    soft_macro.reportShapeCurve(logger_);
  }
}

template <typename SACore>
void HierRTLMP::printPlacementResult(Cluster* parent,
                                     const odb::Rect& outline,
                                     SACore* sa_core)
{
  logger_->report("Id: {}", parent->getId());
  logger_->report("Outline: ({:^8.2f} {:^8.2f}) ({:^8.2f} {:^8.2f})",
                  block_->dbuToMicrons(outline.xMin()),
                  block_->dbuToMicrons(outline.yMin()),
                  block_->dbuToMicrons(outline.xMax()),
                  block_->dbuToMicrons(outline.yMax()));
  sa_core->printResults();
}

void HierRTLMP::writeNetFile(const std::string& file_name_prefix,
                             std::vector<SoftMacro>& macros,
                             BundledNetList& nets)
{
  std::ofstream file(file_name_prefix + ".net.txt");
  for (auto& net : nets) {
    file << macros[net.terminals.first].getName() << "   "
         << macros[net.terminals.second].getName() << "   " << net.weight
         << '\n';
  }
}

void HierRTLMP::writeFloorplanFile(const std::string& file_name_prefix,
                                   std::vector<SoftMacro>& macros)
{
  std::ofstream file(file_name_prefix + ".fp.txt");
  for (auto& macro : macros) {
    file << macro.getName() << "   " << macro.getX() << "   " << macro.getY()
         << "   " << macro.getWidth() << "   " << macro.getHeight() << '\n';
  }
}

template <typename SACore>
void HierRTLMP::writeCostFile(const std::string& file_name_prefix,
                              SACore* sa_core)
{
  sa_core->writeCostFile(file_name_prefix + ".cost.txt");
}

//////// Pusher ////////

Pusher::Pusher(utl::Logger* logger,
               Cluster* root,
               odb::dbBlock* block,
               const std::vector<odb::Rect>& io_blockages)
    : logger_(logger), root_(root), block_(block)
{
  core_ = block_->getCoreArea();
  setIOBlockages(io_blockages);
}

void Pusher::setIOBlockages(const std::vector<odb::Rect>& io_blockages)
{
  io_blockages_ = io_blockages;
}

void Pusher::fetchMacroClusters(Cluster* parent,
                                std::vector<Cluster*>& macro_clusters)
{
  for (auto& child : parent->getChildren()) {
    if (child->getClusterType() == HardMacroCluster) {
      macro_clusters.push_back(child.get());

      for (HardMacro* hard_macro : child->getHardMacros()) {
        hard_macros_.push_back(hard_macro);
      }

    } else if (child->getClusterType() == MixedCluster) {
      fetchMacroClusters(child.get(), macro_clusters);
    }
  }
}

void Pusher::pushMacrosToCoreBoundaries()
{
  // Case in which the design has nothing but macros.
  if (root_->getClusterType() == HardMacroCluster) {
    return;
  }

  if (designHasSingleCentralizedMacroArray()) {
    return;
  }

  std::vector<Cluster*> macro_clusters;
  fetchMacroClusters(root_, macro_clusters);

  for (Cluster* macro_cluster : macro_clusters) {
    if (macro_cluster->isFixedMacro()) {
      continue;
    }

    debugPrint(logger_,
               MPL,
               "boundary_push",
               1,
               "Macro Cluster {}",
               macro_cluster->getName());

    std::map<Boundary, int> boundaries_distance
        = getDistanceToCloseBoundaries(macro_cluster);

    if (logger_->debugCheck(MPL, "boundary_push", 1)) {
      logger_->report("Distance to Close Boundaries:");

      for (auto& [boundary, distance] : boundaries_distance) {
        logger_->report("{} {}", toString(boundary), distance);
      }
    }

    pushMacroClusterToCoreBoundaries(macro_cluster, boundaries_distance);
  }
}

bool Pusher::designHasSingleCentralizedMacroArray()
{
  int macro_cluster_count = 0;

  for (auto& child : root_->getChildren()) {
    switch (child->getClusterType()) {
      case MixedCluster:
        return false;
      case HardMacroCluster:
        ++macro_cluster_count;
        break;
      case StdCellCluster: {
        // Note: to check whether or not a std cell cluster is "tiny"
        // we use the area of its SoftMacro abstraction, because the
        // Cluster::getArea() will give us the actual std cell area
        // of the instances from that cluster.
        if (child->getSoftMacro()->getArea() != 0) {
          return false;
        }
      }
    }

    if (macro_cluster_count > 1) {
      return false;
    }
  }

  return true;
}

// We only group macros of the same size, so here we can use any HardMacro
// from the cluster to set the minimum distance from the respective
// boundary to trigger a push.
std::map<Boundary, int> Pusher::getDistanceToCloseBoundaries(
    Cluster* macro_cluster)
{
  std::map<Boundary, int> boundaries_distance;

  const odb::Rect cluster_box = macro_cluster->getBBox();

  HardMacro* hard_macro = macro_cluster->getHardMacros().front();

  Boundary hor_boundary_to_push;
  const int distance_to_left = std::abs(cluster_box.xMin() - core_.xMin());
  const int distance_to_right = std::abs(cluster_box.xMax() - core_.xMax());
  int smaller_hor_distance = 0;

  if (distance_to_left < distance_to_right) {
    hor_boundary_to_push = Boundary::L;
    smaller_hor_distance = distance_to_left;
  } else {
    hor_boundary_to_push = Boundary::R;
    smaller_hor_distance = distance_to_right;
  }

  const int hard_macro_width = hard_macro->getWidth();
  if (smaller_hor_distance < hard_macro_width) {
    boundaries_distance[hor_boundary_to_push] = smaller_hor_distance;
  }

  Boundary ver_boundary_to_push;
  const int distance_to_top = std::abs(cluster_box.yMax() - core_.yMax());
  const int distance_to_bottom = std::abs(cluster_box.yMin() - core_.yMin());
  int smaller_ver_distance = 0;

  if (distance_to_bottom < distance_to_top) {
    ver_boundary_to_push = Boundary::B;
    smaller_ver_distance = distance_to_bottom;
  } else {
    ver_boundary_to_push = Boundary::T;
    smaller_ver_distance = distance_to_top;
  }

  const int hard_macro_height = hard_macro->getHeight();
  if (smaller_ver_distance < hard_macro_height) {
    boundaries_distance[ver_boundary_to_push] = smaller_ver_distance;
  }

  return boundaries_distance;
}

void Pusher::pushMacroClusterToCoreBoundaries(
    Cluster* macro_cluster,
    const std::map<Boundary, int>& boundaries_distance)
{
  if (boundaries_distance.empty()) {
    return;
  }

  std::vector<HardMacro*> hard_macros = macro_cluster->getHardMacros();

  for (const auto& [boundary, distance] : boundaries_distance) {
    if (distance == 0) {
      continue;
    }

    for (HardMacro* hard_macro : hard_macros) {
      moveHardMacro(hard_macro, boundary, distance);
    }

    debugPrint(logger_,
               MPL,
               "boundary_push",
               1,
               "Moved {} in the direction of {}.",
               macro_cluster->getName(),
               toString(boundary));

    odb::Rect cluster_box = macro_cluster->getBBox();

    moveMacroClusterBox(cluster_box, boundary, distance);

    // Check based on the shape of the macro cluster to avoid iterating each
    // of its HardMacros.
    if (overlapsWithHardMacro(cluster_box, hard_macros)
        || overlapsWithIOBlockage(cluster_box)) {
      // Move back to original position.
      for (HardMacro* hard_macro : hard_macros) {
        moveHardMacro(hard_macro, boundary, (-distance));
      }
    }
  }
}

void Pusher::moveMacroClusterBox(odb::Rect& cluster_box,
                                 const Boundary boundary,
                                 const int distance)
{
  switch (boundary) {
    case (Boundary::L): {
      cluster_box.moveDelta(-distance, 0);
      break;
    }
    case (Boundary::R): {
      cluster_box.moveDelta(distance, 0);
      break;
    }
    case (Boundary::T): {
      cluster_box.moveDelta(0, distance);
      break;
    }
    case (Boundary::B): {
      cluster_box.moveDelta(0, -distance);
      break;
    }
  }
}

void Pusher::moveHardMacro(HardMacro* hard_macro,
                           const Boundary boundary,
                           const int distance)
{
  switch (boundary) {
    case (Boundary::L): {
      hard_macro->setX(hard_macro->getX() - distance);
      break;
    }
    case (Boundary::R): {
      hard_macro->setX(hard_macro->getX() + distance);
      break;
    }
    case (Boundary::T): {
      hard_macro->setY(hard_macro->getY() + distance);
      break;
    }
    case (Boundary::B): {
      hard_macro->setY(hard_macro->getY() - distance);
      break;
    }
  }
}

bool Pusher::overlapsWithHardMacro(
    const odb::Rect& cluster_box,
    const std::vector<HardMacro*>& cluster_hard_macros)
{
  for (const HardMacro* hard_macro : hard_macros_) {
    bool hard_macro_belongs_to_cluster = false;

    for (const HardMacro* cluster_hard_macro : cluster_hard_macros) {
      if (hard_macro == cluster_hard_macro) {
        hard_macro_belongs_to_cluster = true;
        break;
      }
    }

    if (hard_macro_belongs_to_cluster) {
      continue;
    }

    if (cluster_box.overlaps(hard_macro->getBBox())) {
      debugPrint(logger_,
                 MPL,
                 "boundary_push",
                 1,
                 "\tFound overlap with HardMacro {}. Push will be reverted.",
                 hard_macro->getName());
      return true;
    }
  }

  return false;
}

bool Pusher::overlapsWithIOBlockage(const odb::Rect& cluster_box) const
{
  for (const odb::Rect& io_blockage : io_blockages_) {
    if (cluster_box.overlaps(io_blockage)) {
      debugPrint(logger_,
                 MPL,
                 "boundary_push",
                 1,
                 "\tFound overlap with IO blockage {}. Push will be reverted.",
                 io_blockage);
      return true;
    }
  }

  return false;
}

//////// Snapper ////////

Snapper::Snapper(utl::Logger* logger) : logger_(logger), inst_(nullptr)
{
}

Snapper::Snapper(utl::Logger* logger, odb::dbInst* inst)
    : logger_(logger), inst_(inst)
{
}

void Snapper::snapMacro()
{
  snap(odb::dbTechLayerDir::VERTICAL);
  snap(odb::dbTechLayerDir::HORIZONTAL);
}

void Snapper::snap(const odb::dbTechLayerDir& target_direction)
{
  LayerDataList layers_data_list = computeLayerDataList(target_direction);

  int origin = target_direction == odb::dbTechLayerDir::VERTICAL
                   ? inst_->getOrigin().x()
                   : inst_->getOrigin().y();

  if (layers_data_list.empty()) {
    // There are no pins to align with the track-grid.
    alignWithManufacturingGrid(origin);
    setOrigin(origin, target_direction);
    return;
  }

  const std::vector<int>& lowest_grid_positions
      = layers_data_list[0].available_positions;
  odb::dbITerm* lowest_grid_pin = layers_data_list[0].pins[0];

  const int lowest_pin_center_pos
      = origin + getPinOffset(lowest_grid_pin, target_direction);

  auto closest_pos = std::ranges::lower_bound(lowest_grid_positions,

                                              lowest_pin_center_pos);

  int starting_position_index
      = std::distance(lowest_grid_positions.begin(), closest_pos);
  // If no position is found, use the last available
  if (starting_position_index == lowest_grid_positions.size()) {
    starting_position_index -= 1;
  }

  snapPinToPosition(lowest_grid_pin,
                    lowest_grid_positions[starting_position_index],
                    target_direction);

  attemptSnapToExtraPatterns(
      starting_position_index, layers_data_list, target_direction);
}

void Snapper::setOrigin(const int origin,
                        const odb::dbTechLayerDir& target_direction)
{
  if (target_direction == odb::dbTechLayerDir::VERTICAL) {
    inst_->setOrigin(origin, inst_->getOrigin().y());
  } else {
    inst_->setOrigin(inst_->getOrigin().x(), origin);
  }
}

Snapper::LayerDataList Snapper::computeLayerDataList(
    const odb::dbTechLayerDir& target_direction)
{
  TrackGridToPinListMap track_grid_to_pin_list;

  odb::dbBlock* block = inst_->getBlock();

  for (odb::dbITerm* iterm : inst_->getITerms()) {
    if (iterm->getSigType() != odb::dbSigType::SIGNAL) {
      continue;
    }

    for (odb::dbMPin* mpin : iterm->getMTerm()->getMPins()) {
      odb::dbTechLayer* layer = getPinLayer(mpin);

      if (layer->getDirection() != target_direction) {
        continue;
      }

      odb::dbTrackGrid* track_grid = block->findTrackGrid(layer);
      if (track_grid == nullptr) {
        logger_->error(
            MPL, 39, "No track-grid found for layer {}", layer->getName());
      }

      track_grid_to_pin_list[track_grid].push_back(iterm);
    }
  }

  auto compare_pin_center = [&](odb::dbITerm* pin1, odb::dbITerm* pin2) {
    return (target_direction == odb::dbTechLayerDir::VERTICAL
                ? pin1->getBBox().xCenter() < pin2->getBBox().xCenter()
                : pin1->getBBox().yCenter() < pin2->getBBox().yCenter());
  };

  LayerDataList layers_data;
  for (auto& [track_grid, pins] : track_grid_to_pin_list) {
    std::vector<int> positions;
    if (target_direction == odb::dbTechLayerDir::VERTICAL) {
      track_grid->getGridX(positions);
    } else {
      track_grid->getGridY(positions);
    }
    std::ranges::sort(pins, compare_pin_center);
    layers_data.push_back(LayerData{track_grid, std::move(positions), pins});
  }

  auto compare_layer_number = [](LayerData data1, LayerData data2) {
    return (data1.track_grid->getTechLayer()->getNumber()
            < data2.track_grid->getTechLayer()->getNumber());
  };
  std::ranges::sort(layers_data, compare_layer_number);

  return layers_data;
}

odb::dbTechLayer* Snapper::getPinLayer(odb::dbMPin* pin)
{
  return (*pin->getGeometry().begin())->getTechLayer();
}

int Snapper::getPinOffset(odb::dbITerm* pin,
                          const odb::dbTechLayerDir& direction)
{
  int pin_width = 0;
  if (direction == odb::dbTechLayerDir::VERTICAL) {
    pin_width = pin->getBBox().dx();
  } else {
    pin_width = pin->getBBox().dy();
  }

  int pin_to_origin = 0;
  odb::dbMTerm* mterm = pin->getMTerm();
  if (direction == odb::dbTechLayerDir::VERTICAL) {
    pin_to_origin = mterm->getBBox().xMin();
  } else {
    pin_to_origin = mterm->getBBox().yMin();
  }

  int pin_offset = pin_to_origin + (pin_width / 2);

  const odb::dbOrientType& orientation = inst_->getOrient();
  if (direction == odb::dbTechLayerDir::VERTICAL) {
    if (orientation == odb::dbOrientType::MY
        || orientation == odb::dbOrientType::R180) {
      pin_offset = -pin_offset;
    }
  } else {
    if (orientation == odb::dbOrientType::MX
        || orientation == odb::dbOrientType::R180) {
      pin_offset = -pin_offset;
    }
  }

  return pin_offset;
}

void Snapper::snapPinToPosition(odb::dbITerm* pin,
                                int position,
                                const odb::dbTechLayerDir& direction)
{
  int origin = position - getPinOffset(pin, direction);
  alignWithManufacturingGrid(origin);
  setOrigin(origin, direction);
}

void Snapper::getTrackGridPattern(odb::dbTrackGrid* track_grid,
                                  int pattern_idx,
                                  int& origin,
                                  int& step,
                                  const odb::dbTechLayerDir& target_direction)
{
  int count;
  if (target_direction == odb::dbTechLayerDir::VERTICAL) {
    track_grid->getGridPatternX(pattern_idx, origin, count, step);
  } else {
    track_grid->getGridPatternY(pattern_idx, origin, count, step);
  }
}

void Snapper::attemptSnapToExtraPatterns(
    const int start_index,
    const LayerDataList& layers_data_list,
    const odb::dbTechLayerDir& target_direction)
{
  const int total_attempts = 100;
  const int total_pins = std::accumulate(layers_data_list.begin(),
                                         layers_data_list.end(),
                                         0,
                                         [](int total, const LayerData& data) {
                                           return total + data.pins.size();
                                         });

  odb::dbITerm* snap_pin = layers_data_list[0].pins[0];
  const std::vector<int>& positions = layers_data_list[0].available_positions;

  int best_index = start_index;
  int best_snapped_pins = 0;

  for (int i = 0; i <= total_attempts; i++) {
    // Alternates steps from positive to negative incrementally
    int steps = (i % 2 == 1) ? (i + 1) / 2 : -(i / 2);

    int current_index = start_index + steps;

    if (current_index < 0
        || current_index >= layers_data_list[0].available_positions.size()) {
      continue;
    }
    snapPinToPosition(snap_pin, positions[current_index], target_direction);

    int snapped_pins = totalAlignedPins(layers_data_list, target_direction);

    if (snapped_pins > best_snapped_pins) {
      best_snapped_pins = snapped_pins;
      best_index = current_index;
      if (best_snapped_pins == total_pins) {
        break;
      }
    }
  }

  snapPinToPosition(snap_pin, positions[best_index], target_direction);

  if (best_snapped_pins != total_pins) {
    totalAlignedPins(layers_data_list, target_direction, true);

    logger_->warn(MPL,
                  2,
                  "Could not align all pins of the macro {} to the track-grid. "
                  "{} out of {} pins were aligned.",
                  inst_->getName(),
                  best_snapped_pins,
                  total_pins);
  }
}

int Snapper::totalAlignedPins(const LayerDataList& layers_data_list,
                              const odb::dbTechLayerDir& direction,
                              bool error_unaligned_right_way_on_grid)
{
  int pins_aligned = 0;

  for (auto& data : layers_data_list) {
    std::vector<int> pin_centers;
    pin_centers.reserve(data.pins.size());

    for (auto& pin : data.pins) {
      pin_centers.push_back(direction == odb::dbTechLayerDir::VERTICAL
                                ? pin->getBBox().xCenter()
                                : pin->getBBox().yCenter());
    }

    int i = 0, j = 0;
    while (i < pin_centers.size() && j < data.available_positions.size()) {
      if (pin_centers[i] == data.available_positions[j]) {
        pins_aligned++;
        i++;
      } else if (pin_centers[i] < data.available_positions[j]) {
        if (error_unaligned_right_way_on_grid
            && data.track_grid->getTechLayer()->isRightWayOnGridOnly()) {
          logger_->error(MPL,
                         5,
                         "Couldn't align pin {} from the RightWayOnGridOnly "
                         "layer {} with the track-grid.",
                         data.pins[i]->getName(),
                         data.track_grid->getTechLayer()->getName());
        }

        i++;
      } else {
        j++;
      }
    }
  }
  return pins_aligned;
}

void Snapper::alignWithManufacturingGrid(int& origin)
{
  const int manufacturing_grid
      = inst_->getDb()->getTech()->getManufacturingGrid();

  origin = std::round(origin / static_cast<double>(manufacturing_grid))
           * manufacturing_grid;
}

}  // namespace mpl
