// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "hier_rtlmp.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "MplObserver.h"
#include "SACoreHardMacro.h"
#include "SACoreSoftMacro.h"
#include "db_sta/dbNetwork.hh"
#include "object.h"
#include "odb/db.h"
#include "odb/util.h"
#include "par/PartitionMgr.h"
#include "sta/Liberty.hh"
#include "utl/Logger.h"

namespace mpl {

using std::string;

static string make_filename(const string& file_name)
{
  return std::regex_replace(file_name, std::regex("[\":<>\\|\\*\\?/]"), "--");
}

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
  boundary_weight_ = weight;
}

void HierRTLMP::setNotchWeight(float weight)
{
  notch_weight_ = weight;
}

void HierRTLMP::setMacroBlockageWeight(float weight)
{
  macro_blockage_weight_ = weight;
}

void HierRTLMP::setGlobalFence(float fence_lx,
                               float fence_ly,
                               float fence_ux,
                               float fence_uy)
{
  tree_->global_fence = Rect(fence_lx, fence_ly, fence_ux, fence_uy);
}

void HierRTLMP::setHaloWidth(float halo_width)
{
  tree_->halo_width = halo_width;
}

void HierRTLMP::setHaloHeight(float halo_height)
{
  tree_->halo_height = halo_height;
}

void HierRTLMP::setGuidanceRegions(
    const std::map<odb::dbInst*, Rect>& guidance_regions)
{
  guides_ = guidance_regions;
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

void HierRTLMP::setSignatureNetThreshold(int signature_net_threshold)
{
  tree_->min_net_count_for_connection = signature_net_threshold;
}

void HierRTLMP::setPinAccessThreshold(float pin_access_th)
{
  pin_access_th_ = pin_access_th;
  pin_access_th_orig_ = pin_access_th;
}

void HierRTLMP::setTargetUtil(float target_util)
{
  target_util_ = target_util;
}

void HierRTLMP::setTargetDeadSpace(float target_dead_space)
{
  target_dead_space_ = target_dead_space;
}

void HierRTLMP::setMinAR(float min_ar)
{
  min_ar_ = min_ar;
}

void HierRTLMP::setReportDirectory(const char* report_directory)
{
  report_directory_ = report_directory;
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
}

void HierRTLMP::init()
{
  block_ = db_->getChip()->getBlock();
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
  placeChildren(tree_->root.get());
}

void HierRTLMP::resetSAParameters()
{
  pos_swap_prob_ = 0.2;
  neg_swap_prob_ = 0.2;
  double_swap_prob_ = 0.2;
  exchange_swap_prob_ = 0.2;
  flip_prob_ = 0.2;
  resize_prob_ = 0.0;

  placement_core_weights_.fence = 0.0;

  boundary_weight_ = 0.0;
  notch_weight_ = 0.0;
  macro_blockage_weight_ = 0.0;
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

  const float root_area = tree_->floorplan_shape.getArea();
  const float root_width = tree_->floorplan_shape.getWidth();
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

  // if the current cluster is the root cluster,
  // the shape is fixed, i.e., the fixed die.
  // Thus, we do not need to determine the shapes for it
  // calculate macro tiling for parent cluster based on
  // the macro tilings of its children
  std::vector<SoftMacro> macros;
  for (auto& cluster : parent->getChildren()) {
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

      macros.push_back(macro);
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

  const Rect outline(0, 0, tree_->root->getWidth(), tree_->root->getHeight());

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
      const Rect new_outline(0,
                             0,
                             outline.getWidth() * vary_factor_list[run_id++],
                             outline.getHeight());
      if (graphics_) {
        graphics_->setOutline(micronsToDbu(new_outline));
      }
      std::unique_ptr<SACoreSoftMacro> sa
          = std::make_unique<SACoreSoftMacro>(tree_.get(),
                                              new_outline,
                                              macros,
                                              shaping_core_weights_,
                                              0.0,  // boundary weight
                                              0.0,  // macro blockage
                                              0.0,  // notch weight
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
      if (sa->isValid(outline)) {
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
      const Rect new_outline(0,
                             0,
                             outline.getWidth(),
                             outline.getHeight() * vary_factor_list[run_id++]);
      if (graphics_) {
        graphics_->setOutline(micronsToDbu(new_outline));
      }
      std::unique_ptr<SACoreSoftMacro> sa
          = std::make_unique<SACoreSoftMacro>(tree_.get(),
                                              new_outline,
                                              macros,
                                              shaping_core_weights_,
                                              0.0,  // boundary weight
                                              0.0,  // macro blockage
                                              0.0,  // notch weight
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
      if (sa->isValid(outline)) {
        tilings_set.insert({sa->getWidth(), sa->getHeight()});
      }
    }
    remaining_runs -= run_thread;
  }

  TilingList tilings_list(tilings_set.begin(), tilings_set.end());
  std::sort(tilings_list.begin(), tilings_list.end(), isAreaSmaller);

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

  if (tilings_list.empty()) {
    logger_->error(MPL,
                   3,
                   "There are no valid tilings for mixed cluster: {}",
                   parent->getName());
  } else {
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

  std::sort(width_intervals.begin(), width_intervals.end(), isMinWidthSmaller);

  return width_intervals;
}

void HierRTLMP::calculateMacroTilings(Cluster* cluster)
{
  if (cluster->getClusterType() != HardMacroCluster) {
    return;
  }

  std::vector<HardMacro*> hard_macros = cluster->getHardMacros();
  TilingSet tilings_set;

  if (hard_macros.size() == 1) {
    float width = hard_macros[0]->getWidth();
    float height = hard_macros[0]->getHeight();

    TilingList tilings;
    tilings.emplace_back(width, height);
    cluster->setTilings(tilings);

    debugPrint(logger_,
               MPL,
               "coarse_shaping",
               1,
               "{} has only one macro, set tiling according to macro with halo",
               cluster->getName());
    return;
  }

  if (cluster->isArrayOfInterconnectedMacros()) {
    setTightPackingTilings(cluster);
    return;
  }

  if (graphics_) {
    graphics_->setCurrentCluster(cluster);
  }

  // otherwise call simulated annealing to determine tilings
  // set the action probabilities
  const float action_sum = pos_swap_prob_ + neg_swap_prob_ + double_swap_prob_
                           + exchange_swap_prob_;

  const Rect outline(0, 0, tree_->root->getWidth(), tree_->root->getHeight());

  // update macros
  std::vector<HardMacro> macros;
  macros.reserve(hard_macros.size());
  for (auto& macro : hard_macros) {
    macros.push_back(*macro);
  }
  int num_perturb_per_step = (macros.size() > num_perturb_per_step_ / 10)
                                 ? macros.size()
                                 : num_perturb_per_step_ / 10;
  if (cluster->getParent() == nullptr) {
    num_perturb_per_step = (macros.size() > num_perturb_per_step_ / 5)
                               ? macros.size()
                               : num_perturb_per_step_ / 5;
  }

  // To generate different macro tilings, we vary the outline constraints
  // we first vary the outline width while keeping outline_height fixed
  // Then we vary the outline height while keeping outline_width fixed
  // We vary the outline of cluster to generate different tilings
  std::vector<float> vary_factor_list{1.0};
  float vary_step = 1.0 / num_runs_;  // change the outline by at most halfly
  for (int i = 1; i < num_runs_; i++) {
    vary_factor_list.push_back(1.0 - i * vary_step);
  }
  int remaining_runs = num_runs_;
  int run_id = 0;
  while (remaining_runs > 0) {
    HardSAVector sa_batch;
    const int run_thread
        = graphics_ ? 1 : std::min(remaining_runs, num_threads_);
    for (int i = 0; i < run_thread; i++) {
      const Rect new_outline(0,
                             0,
                             outline.getWidth() * vary_factor_list[run_id++],
                             outline.getHeight());
      if (graphics_) {
        graphics_->setOutline(micronsToDbu(new_outline));
      }
      std::unique_ptr<SACoreHardMacro> sa
          = std::make_unique<SACoreHardMacro>(tree_.get(),
                                              new_outline,
                                              macros,
                                              shaping_core_weights_,
                                              pos_swap_prob_ / action_sum,
                                              neg_swap_prob_ / action_sum,
                                              double_swap_prob_ / action_sum,
                                              exchange_swap_prob_ / action_sum,
                                              0.0,  // no flip
                                              init_prob_,
                                              max_num_step_,
                                              num_perturb_per_step,
                                              random_seed_ + run_id,
                                              graphics_.get(),
                                              logger_,
                                              block_);
      sa_batch.push_back(std::move(sa));
    }
    if (sa_batch.size() == 1) {
      runSA<SACoreHardMacro>(sa_batch[0].get());
    } else {
      // multi threads
      std::vector<std::thread> threads;
      threads.reserve(sa_batch.size());
      for (auto& sa : sa_batch) {
        threads.emplace_back(runSA<SACoreHardMacro>, sa.get());
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    // add macro tilings
    for (auto& sa : sa_batch) {
      if (sa->isValid(outline)) {
        tilings_set.insert({sa->getWidth(), sa->getHeight()});
      }
    }
    remaining_runs -= run_thread;
  }
  // change the outline height while keeping outline width fixed
  remaining_runs = num_runs_;
  run_id = 0;
  while (remaining_runs > 0) {
    HardSAVector sa_batch;
    const int run_thread
        = graphics_ ? 1 : std::min(remaining_runs, num_threads_);
    for (int i = 0; i < run_thread; i++) {
      const Rect new_outline(0,
                             0,
                             outline.getWidth(),
                             outline.getHeight() * vary_factor_list[run_id++]);
      if (graphics_) {
        graphics_->setOutline(micronsToDbu(new_outline));
      }
      std::unique_ptr<SACoreHardMacro> sa
          = std::make_unique<SACoreHardMacro>(tree_.get(),
                                              new_outline,
                                              macros,
                                              shaping_core_weights_,
                                              pos_swap_prob_ / action_sum,
                                              neg_swap_prob_ / action_sum,
                                              double_swap_prob_ / action_sum,
                                              exchange_swap_prob_ / action_sum,
                                              0.0,
                                              init_prob_,
                                              max_num_step_,
                                              num_perturb_per_step,
                                              random_seed_ + run_id,
                                              graphics_.get(),
                                              logger_,
                                              block_);
      sa_batch.push_back(std::move(sa));
    }
    if (sa_batch.size() == 1) {
      runSA<SACoreHardMacro>(sa_batch[0].get());
    } else {
      // multi threads
      std::vector<std::thread> threads;
      threads.reserve(sa_batch.size());
      for (auto& sa : sa_batch) {
        threads.emplace_back(runSA<SACoreHardMacro>, sa.get());
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    // add macro tilings
    for (auto& sa : sa_batch) {
      if (sa->isValid(outline)) {
        tilings_set.insert({sa->getWidth(), sa->getHeight()});
      }
    }
    remaining_runs -= run_thread;
  }

  TilingList tilings_list(tilings_set.begin(), tilings_set.end());
  std::sort(tilings_list.begin(), tilings_list.end(), isAreaSmaller);

  for (auto& tiling : tilings_list) {
    debugPrint(logger_,
               MPL,
               "coarse_shaping",
               2,
               "width: {}, height: {}",
               tiling.width(),
               tiling.height());
  }

  // we only keep the minimum area tiling since all the macros has the same size
  // later this can be relaxed.  But this may cause problems because the
  // minimizing the wirelength may leave holes near the boundary
  TilingList new_tilings_list;
  float first_tiling_area = tilings_list.front().area();
  for (auto& tiling : tilings_list) {
    if (tiling.area() <= first_tiling_area) {
      new_tilings_list.push_back(tiling);
    }
  }
  tilings_list = std::move(new_tilings_list);
  cluster->setTilings(tilings_list);

  if (tilings_list.empty()) {
    logger_->error(MPL,
                   4,
                   "No valid tilings for hard macro cluster: {}",
                   cluster->getName());
  }

  std::string line = "Tiling for hard cluster " + cluster->getName() + "  ";
  for (auto& tiling : tilings_list) {
    line += " < " + std::to_string(tiling.width()) + " , ";
    line += std::to_string(tiling.height()) + " >  ";
  }
  line += "\n";
  debugPrint(logger_, MPL, "coarse_shaping", 2, "{}", line);
}

// Used only for arrays of interconnected macros.
void HierRTLMP::setTightPackingTilings(Cluster* macro_array)
{
  TilingList tight_packing_tilings;

  int divider = 1;
  int columns = 0, rows = 0;

  while (divider <= macro_array->getNumMacro()) {
    if (macro_array->getNumMacro() % divider == 0) {
      columns = macro_array->getNumMacro() / divider;
      rows = divider;

      // We don't consider tilings for right angle rotation orientations,
      // because they're not allowed in our macro placer.
      tight_packing_tilings.emplace_back(
          columns * macro_array->getHardMacros().front()->getWidth(),
          rows * macro_array->getHardMacros().front()->getHeight());
    }

    ++divider;
  }

  macro_array->setTilings(tight_packing_tilings);
}

void HierRTLMP::searchAvailableRegionsForUnconstrainedPins()
{
  if (treeHasConstrainedIOs()) {
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

void HierRTLMP::createPinAccessBlockages()
{
  if (!tree_->maps.pad_to_bterm.empty()) {
    return;
  }

  if (!treeHasConstrainedIOs() && block_->getBlockedRegionsForPins().empty()) {
    // If there are no constraints at all, we give freedom to SA so it
    // doesn't have to deal with pin access blockages across the entire
    // extension of all edges of the die area. This should help SA not
    // relying on extreme utilizations to converge for designs such as
    // sky130hd/uW.
    return;
  }

  const Metrics* top_module_metrics
      = tree_->maps.module_to_metrics.at(block_->getTopModule()).get();

  // Avoid creating blockages with zero depth.
  if (top_module_metrics->getStdCellArea() == 0.0) {
    return;
  }

  computePinAccessDepthLimits();

  if (!tree_->available_regions_for_unconstrained_pins.empty()) {
    createBlockagesForAvailableRegions();
  } else {
    createBlockagesForConstraintRegions();
  }
}

void HierRTLMP::computePinAccessDepthLimits()
{
  const Rect die = dbuToMicrons(block_->getDieArea());
  constexpr float max_depth_proportion = 0.20;

  pin_access_depth_limits_.horizontal = max_depth_proportion * die.getWidth();
  pin_access_depth_limits_.vertical = max_depth_proportion * die.getHeight();
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
  double io_span = 0.0;
  for (const BoundaryRegion& region :
       tree_->available_regions_for_unconstrained_pins) {
    io_span += std::sqrt(
        odb::Point::squaredDistance(region.line.pt0(), region.line.pt1()));
  }

  const float depth = computePinAccessBaseDepth(block_->dbuToMicrons(io_span));

  for (const BoundaryRegion region :
       tree_->available_regions_for_unconstrained_pins) {
    createPinAccessBlockage(region, depth);
  }
}

void HierRTLMP::createBlockagesForConstraintRegions()
{
  float io_span = 0.0f;
  std::vector<Cluster*> clusters_of_unplaced_ios
      = getClustersOfUnplacedIOPins();

  for (Cluster* cluster_of_unplaced_ios : clusters_of_unplaced_ios) {
    if (cluster_of_unplaced_ios->isClusterOfUnconstrainedIOPins()) {
      continue;
    }

    // Reminder: the region rect is always a line for a cluster
    // of constrained pins.
    const Rect region = cluster_of_unplaced_ios->getBBox();
    io_span += region.getPerimeter() / 2;
  }

  const float base_depth = computePinAccessBaseDepth(io_span);
  const int total_ios = static_cast<int>(block_->getBTerms().size());

  for (Cluster* cluster_of_unplaced_ios : clusters_of_unplaced_ios) {
    if (cluster_of_unplaced_ios->isClusterOfUnconstrainedIOPins()) {
      continue;
    }

    const float cluster_number_of_ios = static_cast<float>(
        clustering_engine_->getNumberOfIOs(cluster_of_unplaced_ios));

    const float io_density_factor = cluster_number_of_ios / total_ios;
    const float depth = base_depth * io_density_factor;

    const odb::Rect region_rect
        = micronsToDbu(cluster_of_unplaced_ios->getBBox());
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

      boundary_available_regions = new_boundary_available_regions;
    }

    available_regions.insert(available_regions.end(),
                             boundary_available_regions.begin(),
                             boundary_available_regions.end());
  }

  return available_regions;
}

void HierRTLMP::createPinAccessBlockage(const BoundaryRegion& region,
                                        const float depth)
{
  float blockage_depth;
  if (isVertical(region.boundary)) {
    blockage_depth = depth > pin_access_depth_limits_.horizontal
                         ? pin_access_depth_limits_.horizontal
                         : depth;
  } else {
    blockage_depth = depth > pin_access_depth_limits_.vertical
                         ? pin_access_depth_limits_.vertical
                         : depth;
  }

  debugPrint(logger_,
             MPL,
             "coarse_shaping",
             1,
             "Creating pin access blockage in {} -> Region line = ({}) ({}) , "
             "Depth = {}",
             toString(region.boundary),
             region.line.pt0(),
             region.line.pt1(),
             blockage_depth);

  Rect blockage = dbuToMicrons(lineToRect(region.line));
  switch (region.boundary) {
    case (Boundary::L): {
      blockage.setXMax(blockage.xMin() + blockage_depth);
      break;
    }
    case (Boundary::R): {
      blockage.setXMin(blockage.xMax() - blockage_depth);
      break;
    }
    case (Boundary::B): {
      blockage.setYMax(blockage.yMin() + blockage_depth);
      break;
    }
    case (Boundary::T): {
      blockage.setYMin(blockage.yMax() - blockage_depth);
      break;
    }
  }

  macro_blockages_.push_back(blockage);
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
float HierRTLMP::computePinAccessBaseDepth(const double io_span) const
{
  float std_cell_area = 0.0;
  for (auto& cluster : tree_->root->getChildren()) {
    if (cluster->getClusterType() == StdCellCluster) {
      std_cell_area += cluster->getArea();
    }
  }

  if (std_cell_area == 0.0) {
    for (auto& cluster : tree_->root->getChildren()) {
      if (cluster->getClusterType() == MixedCluster) {
        std_cell_area += cluster->getArea();
      }
    }
  }

  const float macro_dominance_factor
      = tree_->macro_with_halo_area
        / (tree_->root->getWidth() * tree_->root->getHeight());
  const float base_depth
      = (std_cell_area / io_span) * std::pow((1 - macro_dominance_factor), 2);

  debugPrint(logger_,
             MPL,
             "coarse_shaping",
             1,
             "Base pin access depth: {} μm",
             base_depth);

  return base_depth;
}

void HierRTLMP::setPlacementBlockages()
{
  for (odb::dbBlockage* blockage : block_->getBlockages()) {
    odb::Rect bbox = blockage->getBBox()->getBox();

    Rect bbox_micron(block_->dbuToMicrons(bbox.xMin()),
                     block_->dbuToMicrons(bbox.yMin()),
                     block_->dbuToMicrons(bbox.xMax()),
                     block_->dbuToMicrons(bbox.yMax()));

    placement_blockages_.push_back(bbox_micron);
  }
}

// Merge nets to reduce runtime
void HierRTLMP::mergeNets(std::vector<BundledNet>& nets)
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

  std::vector<BundledNet> merged_nets;
  for (int i = 0; i < net_class.size(); i++) {
    if (net_class[i] == -1) {
      merged_nets.push_back(nets[i]);
    }
  }
  nets.clear();
  nets = std::move(merged_nets);

  if (graphics_) {
    graphics_->setBundledNets(nets);
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
               macro_blockage_weight_,
               new_macro_blockage_weight);
    macro_blockage_weight_ = new_macro_blockage_weight;
  }
}

void HierRTLMP::placeChildren(Cluster* parent)
{
  if (parent->getClusterType() == HardMacroCluster) {
    placeMacros(parent);
    return;
  }

  if (parent->isLeaf()) {  // Cover IO Clusters && Leaf Std Cells
    return;
  }

  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Placing children of cluster {}",
             parent->getName());

  if (graphics_) {
    graphics_->setCurrentCluster(parent);
  }

  for (auto& cluster : parent->getChildren()) {
    clustering_engine_->updateInstancesAssociation(cluster.get());
  }
  // Place children clusters
  // map children cluster to soft macro
  for (auto& cluster : parent->getChildren()) {
    if (cluster->isIOCluster()) {  // ignore all the io clusters
      continue;
    }
    auto soft_macro = std::make_unique<SoftMacro>(cluster.get());
    // no memory leakage, beacuse we set the soft macro, the old one
    // will be deleted
    cluster->setSoftMacro(std::move(soft_macro));
  }

  // The simulated annealing outline is determined by the parent's shape
  const Rect outline = parent->getBBox();

  // Suppose the region, fence, guide has been mapped to cooresponding macros
  // This step is done when we enter the Hier-RTLMP program
  std::map<std::string, int> soft_macro_id_map;  // cluster_name, macro_id
  std::map<int, Rect> fences;
  std::map<int, Rect> guides;
  std::vector<SoftMacro> macros;
  std::vector<BundledNet> nets;

  std::vector<Rect> placement_blockages;
  std::vector<Rect> macro_blockages;

  findOverlappingBlockages(macro_blockages, placement_blockages, outline);

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
    // for other clusters
    soft_macro_id_map[cluster->getName()] = macros.size();
    auto soft_macro = std::make_unique<SoftMacro>(cluster.get());
    clustering_engine_->updateInstancesAssociation(
        cluster.get());  // we need this step to calculate nets
    macros.push_back(*soft_macro);
    cluster->setSoftMacro(std::move(soft_macro));
    // merge fences and guides for hard macros within cluster
    if (cluster->getClusterType() == StdCellCluster) {
      continue;
    }
    Rect fence, guide;
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

    // Calculate overlap with outline
    fence.relocate(
        outline.xMin(), outline.yMin(), outline.xMax(), outline.yMax());
    guide.relocate(
        outline.xMin(), outline.yMin(), outline.xMax(), outline.yMax());

    if (fence.isValid()) {
      // current macro id is macros.size() - 1
      fences[macros.size() - 1] = fence;
    }
    if (guide.isValid()) {
      // current macro id is macros.size() - 1
      guides[macros.size() - 1] = guide;
    }
  }

  if (graphics_) {
    graphics_->setGuides(guides);
    graphics_->setFences(fences);
  }

  const int num_of_macros_to_place = static_cast<int>(macros.size());

  for (Cluster* io_cluster : io_clusters) {
    soft_macro_id_map[io_cluster->getName()] = macros.size();

    macros.emplace_back(
        std::pair<float, float>(io_cluster->getX() - outline.xMin(),
                                io_cluster->getY() - outline.yMin()),
        io_cluster->getName(),
        io_cluster->getWidth(),
        io_cluster->getHeight(),
        io_cluster);
  }

  createFixedTerminals(parent, soft_macro_id_map, macros);

  clustering_engine_->updateConnections();
  clustering_engine_->updateDataFlow();

  // add the virtual connections (the weight related to IOs and macros belong to
  // the same cluster)
  for (const auto& [cluster1, cluster2] : parent->getVirtualConnections()) {
    BundledNet net(
        soft_macro_id_map[tree_->maps.id_to_cluster[cluster1]->getName()],
        soft_macro_id_map[tree_->maps.id_to_cluster[cluster2]->getName()],
        tree_->virtual_weight);
    net.src_cluster_id = cluster1;
    net.target_cluster_id = cluster2;
    nets.push_back(net);
  }

  // convert the connections between clusters to SoftMacros
  for (auto& cluster : parent->getChildren()) {
    const int src_id = cluster->getId();
    const std::string src_name = cluster->getName();
    for (auto& [cluster_id, weight] : cluster->getConnection()) {
      debugPrint(logger_,
                 MPL,
                 "hierarchical_macro_placement",
                 3,
                 " Cluster connection: {} {} {} ",
                 cluster->getName(),
                 tree_->maps.id_to_cluster[cluster_id]->getName(),
                 weight);
      const std::string name = tree_->maps.id_to_cluster[cluster_id]->getName();
      if (src_id > cluster_id) {
        BundledNet net(
            soft_macro_id_map[src_name], soft_macro_id_map[name], weight);
        net.src_cluster_id = src_id;
        net.target_cluster_id = cluster_id;
        nets.push_back(net);
      }
    }
  }

  // merge nets to reduce runtime
  mergeNets(nets);

  // Write the connections between macros
  std::ofstream file;
  std::string file_name = parent->getName();
  file_name = make_filename(file_name);

  file_name = report_directory_ + "/" + file_name;
  file.open(file_name + ".net.txt");
  for (auto& net : nets) {
    file << macros[net.terminals.first].getName() << "   "
         << macros[net.terminals.second].getName() << "   " << net.weight
         << std::endl;
  }
  file.close();

  // Call Simulated Annealing Engine to place children
  // set the action probabilities
  // the summation of probabilities should be one.
  const float action_sum = pos_swap_prob_ + neg_swap_prob_ + double_swap_prob_
                           + exchange_swap_prob_ + resize_prob_;
  // In our implementation, target_util and target_dead_space are different
  // target_util is used to determine the utilization for MixedCluster
  // target_dead_space is used to determine the utilization for
  // StandardCellCluster We vary the target utilization to generate different
  // tilings
  std::vector<float> target_utils{target_util_};
  std::vector<float> target_dead_spaces{target_dead_space_};
  // In our implementation, the utilization can be larger than 1.
  for (int i = 1; i < num_target_util_; i++) {
    target_utils.push_back(target_util_ + i * target_util_step_);
  }
  // In our implementation, the target_dead_space should be less than 1.0.
  // The larger the target dead space, the higher the utilization.
  for (int i = 1; i < num_target_dead_space_; i++) {
    if (target_dead_space_ + i * target_dead_space_step_ < 1.0) {
      target_dead_spaces.push_back(target_dead_space_
                                   + i * target_dead_space_step_);
    }
  }
  // Since target_util and target_dead_space are independent variables
  // the combination should be (target_util, target_dead_space_list)
  // target util has higher priority than target_dead_space
  std::vector<float> target_util_list;
  std::vector<float> target_dead_space_list;
  for (auto& target_util : target_utils) {
    for (auto& target_dead_space : target_dead_spaces) {
      target_util_list.push_back(target_util);
      target_dead_space_list.push_back(target_dead_space);
    }
  }
  // The number of perturbations in each step should be larger than the
  // number of macros
  const int num_perturb_per_step = (macros.size() > num_perturb_per_step_)
                                       ? macros.size()
                                       : num_perturb_per_step_;
  int remaining_runs = target_util_list.size();
  int run_id = 0;
  SACoreSoftMacro* best_sa = nullptr;
  SoftSAVector sa_containers;  // The owner of SACore objects
  // To give consistency across threads we check the solutions
  // at a fixed interval independent of how many threads we are using.
  const int check_interval = 10;
  int begin_check = 0;
  int end_check = std::min(check_interval, remaining_runs);
  float best_cost = std::numeric_limits<float>::max();

  while (remaining_runs > 0) {
    SoftSAVector sa_batch;
    const int run_thread
        = graphics_ ? 1 : std::min(remaining_runs, num_threads_);
    for (int i = 0; i < run_thread; i++) {
      std::vector<SoftMacro> shaped_macros = macros;  // copy for multithread

      const float target_util = target_util_list[run_id];
      const float target_dead_space = target_dead_space_list[run_id++];

      debugPrint(logger_,
                 MPL,
                 "fine_shaping",
                 1,
                 "Starting adjusting shapes for children of {}. target_util = "
                 "{}, target_dead_space = {}",
                 parent->getName(),
                 target_util,
                 target_dead_space);

      if (!runFineShaping(parent,
                          shaped_macros,
                          soft_macro_id_map,
                          target_util,
                          target_dead_space)) {
        debugPrint(logger_,
                   MPL,
                   "fine_shaping",
                   1,
                   "Cannot generate feasible shapes for children of {}, sa_id: "
                   "{}, target_util: {}, target_dead_space: {}",
                   parent->getName(),
                   run_id,
                   target_util,
                   target_dead_space);
        continue;
      }
      debugPrint(logger_,
                 MPL,
                 "fine_shaping",
                 1,
                 "Finished generating shapes for children of cluster {}",
                 parent->getName());
      // Note that all the probabilities are normalized to the summation of 1.0.
      // Note that the weight are not necessaries summarized to 1.0, i.e., not
      // normalized.
      std::unique_ptr<SACoreSoftMacro> sa
          = std::make_unique<SACoreSoftMacro>(tree_.get(),
                                              outline,
                                              shaped_macros,
                                              placement_core_weights_,
                                              boundary_weight_,
                                              macro_blockage_weight_,
                                              notch_weight_,
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
      sa->setNumberOfMacrosToPlace(num_of_macros_to_place);
      sa->setCentralizationAttemptOn(true);
      sa->setFences(fences);
      sa->setGuides(guides);
      sa->setNets(nets);
      sa->addBlockages(placement_blockages);
      sa->addBlockages(macro_blockages);
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
    remaining_runs -= run_thread;
    // add macro tilings
    for (auto& sa : sa_batch) {
      sa_containers.push_back(std::move(sa));
    }
    while (sa_containers.size() >= end_check) {
      while (begin_check < end_check) {
        auto& sa = sa_containers[begin_check];
        if (sa->isValid() && sa->getNormCost() < best_cost) {
          best_cost = sa->getNormCost();
          best_sa = sa.get();
        }
        ++begin_check;
      }
      // add early stop mechanism
      if (best_sa || remaining_runs == 0) {
        break;
      }
      end_check = begin_check + std::min(check_interval, remaining_runs);
    }
    if (best_sa) {
      break;
    }
  }

  if (best_sa == nullptr) {
    placeChildrenUsingMinimumTargetUtil(parent);
  } else {
    if (best_sa->centralizationWasReverted()) {
      best_sa->alignMacroClusters();
    }
    best_sa->fillDeadSpace();

    std::vector<SoftMacro> shaped_macros;
    best_sa->getMacros(shaped_macros);
    file.open(file_name + ".fp.txt.temp");
    for (auto& macro : shaped_macros) {
      file << macro.getName() << "   " << macro.getX() << "   " << macro.getY()
           << "   " << macro.getWidth() << "   " << macro.getHeight()
           << std::endl;
    }
    file.close();

    if (logger_->debugCheck(MPL, "hierarchical_macro_placement", 1)) {
      logger_->report("Cluster Placement Summary");
      printPlacementResult(parent, outline, best_sa);
    }

    // write the cost function. This can be used to tune the temperature
    // schedule and cost weight
    best_sa->writeCostFile(file_name + ".cost.txt");
    // write the floorplan information
    file.open(file_name + ".fp.txt");
    for (auto& macro : shaped_macros) {
      file << macro.getName() << "   " << macro.getX() << "   " << macro.getY()
           << "   " << macro.getWidth() << "   " << macro.getHeight()
           << std::endl;
    }
    file.close();

    updateChildrenShapesAndLocations(parent, shaped_macros, soft_macro_id_map);

    updateChildrenRealLocation(parent, outline.xMin(), outline.yMin());
  }

  for (auto& cluster : parent->getChildren()) {
    placeChildren(cluster.get());
  }

  clustering_engine_->updateInstancesAssociation(parent);
}

// This function is used in cases with very high density, in which it may
// be very hard to generate a valid tiling for the clusters.
// Here, we may want to try setting the area of all standard-cell clusters to 0.
// This should be only be used in mixed clusters.
void HierRTLMP::placeChildrenUsingMinimumTargetUtil(Cluster* parent)
{
  if (parent->getClusterType() != MixedCluster) {
    return;
  }

  // We only run this enhanced cluster placement version if there are no
  // further levels ahead in the current branch of the physical hierarchy.
  for (auto& cluster : parent->getChildren()) {
    if (cluster->getClusterType() == MixedCluster) {
      return;
    }
  }

  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Conventional cluster placement failed. Attempting with minimum "
             "target utilization.");

  if (graphics_) {
    graphics_->setCurrentCluster(parent);
  }

  // Place children clusters
  // map children cluster to soft macro
  for (auto& cluster : parent->getChildren()) {
    if (cluster->isIOCluster()) {  // ignore all the io clusters
      continue;
    }
    auto soft_macro = std::make_unique<SoftMacro>(cluster.get());
    // no memory leakage, beacuse we set the soft macro, the old one
    // will be deleted
    cluster->setSoftMacro(std::move(soft_macro));
  }

  // The simulated annealing outline is determined by the parent's shape
  const Rect outline(parent->getX(),
                     parent->getY(),
                     parent->getX() + parent->getWidth(),
                     parent->getY() + parent->getHeight());

  // Suppose the region, fence, guide has been mapped to cooresponding macros
  // This step is done when we enter the Hier-RTLMP program
  std::map<std::string, int> soft_macro_id_map;  // cluster_name, macro_id
  std::map<int, Rect> fences;
  std::map<int, Rect> guides;
  std::vector<SoftMacro> macros;
  std::vector<BundledNet> nets;

  std::vector<Rect> placement_blockages;
  std::vector<Rect> macro_blockages;

  findOverlappingBlockages(macro_blockages, placement_blockages, outline);

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
    // for other clusters
    soft_macro_id_map[cluster->getName()] = macros.size();
    auto soft_macro = std::make_unique<SoftMacro>(cluster.get());
    clustering_engine_->updateInstancesAssociation(
        cluster.get());  // we need this step to calculate nets
    macros.push_back(*soft_macro);
    cluster->setSoftMacro(std::move(soft_macro));
    // merge fences and guides for hard macros within cluster
    if (cluster->getClusterType() == StdCellCluster) {
      continue;
    }
    Rect fence, guide;
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

    // Calculate overlap with outline
    fence.relocate(
        outline.xMin(), outline.yMin(), outline.xMax(), outline.yMax());
    guide.relocate(
        outline.xMin(), outline.yMin(), outline.xMax(), outline.yMax());
    if (fence.isValid()) {
      // current macro id is macros.size() - 1
      fences[macros.size() - 1] = fence;
    }
    if (guide.isValid()) {
      // current macro id is macros.size() - 1
      guides[macros.size() - 1] = guide;
    }
  }

  if (graphics_) {
    graphics_->setGuides(guides);
    graphics_->setFences(fences);
  }

  const int macros_to_place = static_cast<int>(macros.size());

  for (Cluster* io_cluster : io_clusters) {
    soft_macro_id_map[io_cluster->getName()] = macros.size();

    macros.emplace_back(
        std::pair<float, float>(io_cluster->getX() - outline.xMin(),
                                io_cluster->getY() - outline.yMin()),
        io_cluster->getName(),
        io_cluster->getWidth(),
        io_cluster->getHeight(),
        io_cluster);
  }

  createFixedTerminals(parent, soft_macro_id_map, macros);

  clustering_engine_->updateConnections();
  clustering_engine_->updateDataFlow();

  // add the virtual connections (the weight related to IOs and macros belong to
  // the same cluster)
  for (const auto& [cluster1, cluster2] : parent->getVirtualConnections()) {
    BundledNet net(
        soft_macro_id_map[tree_->maps.id_to_cluster[cluster1]->getName()],
        soft_macro_id_map[tree_->maps.id_to_cluster[cluster2]->getName()],
        tree_->virtual_weight);
    net.src_cluster_id = cluster1;
    net.target_cluster_id = cluster2;
    nets.push_back(net);
  }

  // convert the connections between clusters to SoftMacros
  for (auto& cluster : parent->getChildren()) {
    const int src_id = cluster->getId();
    const std::string src_name = cluster->getName();
    for (auto& [cluster_id, weight] : cluster->getConnection()) {
      debugPrint(logger_,
                 MPL,
                 "hierarchical_macro_placement",
                 3,
                 " Cluster connection: {} {} {} ",
                 cluster->getName(),
                 tree_->maps.id_to_cluster[cluster_id]->getName(),
                 weight);
      const std::string name = tree_->maps.id_to_cluster[cluster_id]->getName();
      if (src_id > cluster_id) {
        BundledNet net(
            soft_macro_id_map[src_name], soft_macro_id_map[name], weight);
        net.src_cluster_id = src_id;
        net.target_cluster_id = cluster_id;
        nets.push_back(net);
      }
    }
  }

  // merge nets to reduce runtime
  mergeNets(nets);

  // Write the connections between macros
  std::ofstream file;
  std::string file_name = parent->getName();
  file_name = make_filename(file_name);

  file_name = report_directory_ + "/" + file_name;
  file.open(file_name + ".net.txt");
  for (auto& net : nets) {
    file << macros[net.terminals.first].getName() << "   "
         << macros[net.terminals.second].getName() << "   " << net.weight
         << std::endl;
  }
  file.close();

  // Call Simulated Annealing Engine to place children
  // set the action probabilities
  // the summation of probabilities should be one.
  const float action_sum = pos_swap_prob_ + neg_swap_prob_ + double_swap_prob_
                           + exchange_swap_prob_ + resize_prob_;
  // In our implementation, target_util and target_dead_space are different
  // target_util is used to determine the utilization for MixedCluster
  // target_dead_space is used to determine the utilization for
  // StandardCellCluster We vary the target utilization to generate different
  // tilings
  std::vector<float> target_utils{1e6};
  std::vector<float> target_dead_spaces{0.99999};
  // Since target_util and target_dead_space are independent variables
  // the combination should be (target_util, target_dead_space_list)
  // target util has higher priority than target_dead_space
  std::vector<float> target_util_list;
  std::vector<float> target_dead_space_list;
  for (auto& target_util : target_utils) {
    for (auto& target_dead_space : target_dead_spaces) {
      target_util_list.push_back(target_util);
      target_dead_space_list.push_back(target_dead_space);
    }
  }
  // The number of perturbations in each step should be larger than the
  // number of macros
  const int num_perturb_per_step = (macros.size() > num_perturb_per_step_)
                                       ? macros.size()
                                       : num_perturb_per_step_;
  int remaining_runs = target_util_list.size();
  int run_id = 0;
  SACoreSoftMacro* best_sa = nullptr;
  SoftSAVector sa_containers;  // The owner of SACore objects
  float best_cost = std::numeric_limits<float>::max();
  // To give consistency across threads we check the solutions
  // at a fixed interval independent of how many threads we are using.
  const int check_interval = 10;
  int begin_check = 0;
  int end_check = std::min(check_interval, remaining_runs);

  while (remaining_runs > 0) {
    SoftSAVector sa_batch;
    const int run_thread
        = graphics_ ? 1 : std::min(remaining_runs, num_threads_);
    for (int i = 0; i < run_thread; i++) {
      std::vector<SoftMacro> shaped_macros = macros;  // copy for multithread

      const float target_util = target_util_list[run_id];
      const float target_dead_space = target_dead_space_list[run_id++];
      debugPrint(logger_,
                 MPL,
                 "fine_shaping",
                 1,
                 "Starting adjusting shapes for children of {}. target_util = "
                 "{}, target_dead_space = {}",
                 parent->getName(),
                 target_util,
                 target_dead_space);
      if (!runFineShaping(parent,
                          shaped_macros,
                          soft_macro_id_map,
                          target_util,
                          target_dead_space)) {
        debugPrint(logger_,
                   MPL,
                   "fine_shaping",
                   1,
                   "Cannot generate feasible shapes for children of {}, sa_id: "
                   "{}, target_util: {}, target_dead_space: {}",
                   parent->getName(),
                   run_id,
                   target_util,
                   target_dead_space);
        continue;
      }
      debugPrint(logger_,
                 MPL,
                 "fine_shaping",
                 1,
                 "Finished generating shapes for children of cluster {}",
                 parent->getName());
      // Note that all the probabilities are normalized to the summation of 1.0.
      // Note that the weight are not necessaries summarized to 1.0, i.e., not
      // normalized.
      std::unique_ptr<SACoreSoftMacro> sa
          = std::make_unique<SACoreSoftMacro>(tree_.get(),
                                              outline,
                                              shaped_macros,
                                              placement_core_weights_,
                                              boundary_weight_,
                                              macro_blockage_weight_,
                                              notch_weight_,
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
      sa->setNumberOfMacrosToPlace(macros_to_place);
      sa->setCentralizationAttemptOn(true);
      sa->setFences(fences);
      sa->setGuides(guides);
      sa->setNets(nets);
      sa->addBlockages(placement_blockages);
      sa->addBlockages(macro_blockages);
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
    remaining_runs -= run_thread;
    // add macro tilings
    for (auto& sa : sa_batch) {
      sa_containers.push_back(std::move(sa));
    }
    while (sa_containers.size() >= end_check) {
      while (begin_check < end_check) {
        auto& sa = sa_containers[begin_check];
        if (sa->isValid() && sa->getNormCost() < best_cost) {
          best_cost = sa->getNormCost();
          best_sa = sa.get();
        }
        ++begin_check;
      }
      // add early stop mechanism
      if (best_sa || remaining_runs == 0) {
        break;
      }
      end_check = begin_check + std::min(check_interval, remaining_runs);
    }
    if (best_sa) {
      break;
    }
  }

  if (best_sa == nullptr) {
    debugPrint(logger_,
               MPL,
               "hierarchical_macro_placement",
               1,
               "SA Summary for cluster {}",
               parent->getName());

    for (auto i = 0; i < sa_containers.size(); i++) {
      debugPrint(logger_,
                 MPL,
                 "hierarchical_macro_placement",
                 1,
                 "sa_id: {}, target_util: {}, target_dead_space: {}",
                 i,
                 target_util_list[i],
                 target_dead_space_list[i]);

      sa_containers[i]->printResults();
    }
    logger_->error(MPL, 40, "Failed on cluster {}", parent->getName());
  }

  if (best_sa->centralizationWasReverted()) {
    best_sa->alignMacroClusters();
  }
  best_sa->fillDeadSpace();

  std::vector<SoftMacro> shaped_macros;
  best_sa->getMacros(shaped_macros);
  file.open(file_name + ".fp.txt.temp");
  for (auto& macro : shaped_macros) {
    file << macro.getName() << "   " << macro.getX() << "   " << macro.getY()
         << "   " << macro.getWidth() << "   " << macro.getHeight()
         << std::endl;
  }
  file.close();

  if (logger_->debugCheck(MPL, "hierarchical_macro_placement", 1)) {
    logger_->report("Cluster Placement Summary (Minimum Target Utilization)");
    printPlacementResult(parent, outline, best_sa);
  }

  // write the cost function. This can be used to tune the temperature schedule
  // and cost weight
  best_sa->writeCostFile(file_name + ".cost.txt");
  // write the floorplan information
  file.open(file_name + ".fp.txt");
  for (auto& macro : shaped_macros) {
    file << macro.getName() << "   " << macro.getX() << "   " << macro.getY()
         << "   " << macro.getWidth() << "   " << macro.getHeight()
         << std::endl;
  }
  file.close();

  updateChildrenShapesAndLocations(parent, shaped_macros, soft_macro_id_map);

  updateChildrenRealLocation(parent, outline.xMin(), outline.yMin());
}

// Verify the blockages' areas that have overlapped with current parent
// cluster. All the blockages will be converted to hard macros with fences.
void HierRTLMP::findOverlappingBlockages(std::vector<Rect>& macro_blockages,
                                         std::vector<Rect>& placement_blockages,
                                         const Rect& outline)
{
  for (auto& blockage : placement_blockages_) {
    computeBlockageOverlap(placement_blockages, blockage, outline);
  }

  for (auto& blockage : macro_blockages_) {
    computeBlockageOverlap(macro_blockages, blockage, outline);
  }

  if (graphics_) {
    graphics_->setOutline(micronsToDbu(outline));
    graphics_->setMacroBlockages(macro_blockages);
    graphics_->setPlacementBlockages(placement_blockages);
  }
}

void HierRTLMP::computeBlockageOverlap(std::vector<Rect>& overlapping_blockages,
                                       const Rect& blockage,
                                       const Rect& outline)
{
  const float b_lx = std::max(outline.xMin(), blockage.xMin());
  const float b_ly = std::max(outline.yMin(), blockage.yMin());
  const float b_ux = std::min(outline.xMax(), blockage.xMax());
  const float b_uy = std::min(outline.yMax(), blockage.yMax());

  if ((b_ux - b_lx > 0.0) && (b_uy - b_ly > 0.0)) {
    overlapping_blockages.emplace_back(b_lx - outline.xMin(),
                                       b_ly - outline.yMin(),
                                       b_ux - outline.xMin(),
                                       b_uy - outline.yMin());
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

  Rect outline = parent->getBBox();
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
                                    const Rect& outline,
                                    std::vector<Macro>& macros)
{
  // A conventional fixed terminal is just a point without
  // the cluster data.
  Point location = cluster->getCenter();
  float width = 0.0f;
  float height = 0.0f;
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

  location.first -= outline.xMin();
  location.second -= outline.yMin();

  macros.emplace_back(
      location, cluster->getName(), width, height, terminal_cluster);
}

// Determine the shape of each cluster based on target utilization
// and target dead space.  In constrast to all previous works, we
// use two parameters: target utilization, target_dead_space.
// This is the trick part.  During our experiements, we found that keeping
// the same utilization of standard-cell clusters and mixed cluster will make
// SA very difficult to find a feasible solution.  With different utilization,
// SA can more easily find the solution. In our method, the target_utilization
// is used to determine the bloating ratio for mixed cluster, the
// target_dead_space is used to determine the bloating ratio for standard-cell
// cluster. The target utilization is based on tiling results we calculated
// before. The tiling results (which only consider the contribution of hard
// macros) will give us very close starting point.
bool HierRTLMP::runFineShaping(Cluster* parent,
                               std::vector<SoftMacro>& macros,
                               std::map<std::string, int>& soft_macro_id_map,
                               float target_util,
                               float target_dead_space)
{
  const float outline_width = parent->getWidth();
  const float outline_height = parent->getHeight();
  float pin_access_area = 0.0;
  float std_cell_cluster_area = 0.0;
  float std_cell_mixed_cluster_area = 0.0;
  float macro_cluster_area = 0.0;
  float macro_mixed_cluster_area = 0.0;
  // add the macro area for blockages, pin access and so on
  for (auto& macro : macros) {
    if (macro.getCluster() == nullptr) {
      pin_access_area += macro.getArea();  // get the physical-only area
    }
  }

  for (auto& cluster : parent->getChildren()) {
    if (cluster->isIOCluster()) {
      continue;
    }
    if (cluster->getClusterType() == StdCellCluster) {
      std_cell_cluster_area += cluster->getStdCellArea();
    } else if (cluster->getClusterType() == HardMacroCluster) {
      TilingList valid_tilings;
      for (auto& tiling : cluster->getTilings()) {
        if (tiling.width() < outline_width * (1 + conversion_tolerance_)
            && tiling.height() < outline_height * (1 + conversion_tolerance_)) {
          valid_tilings.push_back(tiling);
        }
      }

      if (valid_tilings.empty()) {
        logger_->error(MPL,
                       7,
                       "Not enough space in cluster: {} for "
                       "child hard macro cluster: {}",
                       parent->getName(),
                       cluster->getName());
      }

      macro_cluster_area += valid_tilings.front().area();
      cluster->setTilings(valid_tilings);
    } else {  // mixed cluster
      std_cell_mixed_cluster_area += cluster->getStdCellArea();
      TilingList valid_tilings;
      for (auto& tiling : cluster->getTilings()) {
        if (tiling.width() < outline_width * (1 + conversion_tolerance_)
            && tiling.height() < outline_height * (1 + conversion_tolerance_)) {
          valid_tilings.push_back(tiling);
        }
      }

      if (valid_tilings.empty()) {
        logger_->error(MPL,
                       8,
                       "Not enough space in cluster: {} for "
                       "child mixed cluster: {}",
                       parent->getName(),
                       cluster->getName());
      }

      macro_mixed_cluster_area += valid_tilings.front().area();
      cluster->setTilings(valid_tilings);
    }  // end for cluster type
  }

  // check how much available space to inflate for mixed cluster
  const float min_target_util
      = std_cell_mixed_cluster_area
        / (outline_width * outline_height - pin_access_area - macro_cluster_area
           - macro_mixed_cluster_area);
  if (target_util <= min_target_util) {
    target_util = min_target_util;  // target utilization for standard cells in
                                    // mixed cluster
  }
  // calculate the std_cell_util
  const float avail_space
      = outline_width * outline_height
        - (pin_access_area + macro_cluster_area + macro_mixed_cluster_area
           + std_cell_mixed_cluster_area / target_util);
  const float std_cell_util
      = std_cell_cluster_area / (avail_space * (1 - target_dead_space));
  // shape clusters
  if ((std_cell_cluster_area > 0.0 && avail_space < 0.0)
      || (std_cell_mixed_cluster_area > 0.0 && min_target_util <= 0.0)) {
    debugPrint(logger_,
               MPL,
               "fine_shaping",
               1,
               "No valid solution for children of {}"
               "avail_space = {}, min_target_util = {}",
               parent->getName(),
               avail_space,
               min_target_util);

    return false;
  }

  // set the shape for each macro
  for (auto& cluster : parent->getChildren()) {
    if (cluster->isIOCluster()) {
      continue;
    }
    if (cluster->getClusterType() == StdCellCluster) {
      float area = cluster->getArea();
      float width = std::sqrt(area);
      float height = width;
      const float dust_threshold
          = 1.0 / macros.size();  // check if the cluster is the dust cluster
      const int dust_std_cell = 100;
      if ((width / outline_width <= dust_threshold
           && height / outline_height <= dust_threshold)
          || cluster->getNumStdCell() <= dust_std_cell) {
        width = 1e-3;
        height = 1e-3;
        area = width * height;
      } else {
        area = cluster->getArea() / std_cell_util;
        width = std::sqrt(area / min_ar_);
      }
      IntervalList width_intervals
          = {Interval(area / width /* min */, width /* max */)};
      macros[soft_macro_id_map[cluster->getName()]].setShapes(width_intervals,
                                                              area);
    } else if (cluster->getClusterType() == HardMacroCluster) {
      macros[soft_macro_id_map[cluster->getName()]].setShapes(
          cluster->getTilings());
      debugPrint(logger_,
                 MPL,
                 "fine_shaping",
                 2,
                 "hard_macro_cluster : {}",
                 cluster->getName());
      for (auto& tiling : cluster->getTilings()) {
        debugPrint(logger_,
                   MPL,
                   "fine_shaping",
                   2,
                   "    ( {} , {} ) ",
                   tiling.width(),
                   tiling.height());
      }
    } else {  // Mixed cluster
      const TilingList& tilings = cluster->getTilings();
      IntervalList width_intervals;
      float area = tilings.back().area();
      area += cluster->getStdCellArea() / target_util;
      for (auto& tiling : tilings) {
        if (tiling.area() <= area) {
          width_intervals.emplace_back(tiling.width(), area / tiling.height());
        }
      }

      debugPrint(logger_,
                 MPL,
                 "fine_shaping",
                 2,
                 "name:  {} area: {}",
                 cluster->getName(),
                 area);

      debugPrint(logger_, MPL, "fine_shaping", 2, "width_list :  ");
      for (auto& width_interval : width_intervals) {
        debugPrint(logger_,
                   MPL,
                   "fine_shaping",
                   2,
                   " [  {} {}  ] ",
                   width_interval.min,
                   width_interval.max);
      }
      macros[soft_macro_id_map[cluster->getName()]].setShapes(width_intervals,
                                                              area);
    }
  }

  return true;
}

void HierRTLMP::placeMacros(Cluster* cluster)
{
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

  const Rect outline(cluster->getX(),
                     cluster->getY(),
                     cluster->getX() + cluster->getWidth(),
                     cluster->getY() + cluster->getHeight());

  std::map<int, Rect> fences;
  std::map<int, Rect> guides;
  computeFencesAndGuides(hard_macros, outline, fences, guides);
  if (graphics_) {
    graphics_->setGuides(guides);
    graphics_->setFences(fences);
  }

  clustering_engine_->updateConnections();

  createFixedTerminals(outline, macro_clusters, cluster_to_macro, sa_macros);

  std::vector<BundledNet> nets
      = computeBundledNets(macro_clusters, cluster_to_macro);

  if (graphics_) {
    graphics_->setBundledNets(nets);
  }

  // Use exchange more often when there are more instances of a common
  // master.
  float exchange_swap_prob
      = exchange_swap_prob_ * 5
        * (1 - (masters.size() / (float) hard_macros.size()));

  // set the action probabilities (summation to 1.0)
  const float action_sum = pos_swap_prob_ * 10 + neg_swap_prob_ * 10
                           + double_swap_prob_ + exchange_swap_prob
                           + flip_prob_;

  float pos_swap_prob = pos_swap_prob_ * 10 / action_sum;
  float neg_swap_prob = neg_swap_prob_ * 10 / action_sum;
  float double_swap_prob = double_swap_prob_ / action_sum;
  exchange_swap_prob = exchange_swap_prob / action_sum;
  float flip_prob = flip_prob_ / action_sum;

  const int macros_to_place = static_cast<int>(hard_macros.size());

  int num_perturb_per_step = (macros_to_place > num_perturb_per_step_ / 10)
                                 ? macros_to_place
                                 : num_perturb_per_step_ / 10;

  SequencePair initial_seq_pair;
  if (cluster->isArrayOfInterconnectedMacros()) {
    setArrayTilingSequencePair(cluster, macros_to_place, initial_seq_pair);

    pos_swap_prob = 0.0f;
    neg_swap_prob = 0.0f;
    double_swap_prob = 0.0f;
    exchange_swap_prob = 0.95;
    flip_prob = 0.05;

    // Large arrays need more steps to properly converge.
    if (num_perturb_per_step > macros_to_place) {
      num_perturb_per_step *= 2;
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
        graphics_->setOutline(micronsToDbu(outline));
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
                                              flip_prob,
                                              init_prob_,
                                              max_num_step_,
                                              num_perturb_per_step,
                                              random_seed_ + run_id,
                                              graphics_.get(),
                                              logger_,
                                              block_);
      sa->setNumberOfMacrosToPlace(macros_to_place);
      sa->setNets(nets);
      sa->setFences(fences);
      sa->setGuides(guides);
      sa->setInitialSequencePair(initial_seq_pair);

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

      if (sa->isValid(outline) && sa->getNormCost() < best_cost) {
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
    std::vector<HardMacro> best_macros;
    best_sa->getMacros(best_macros);

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
void HierRTLMP::setArrayTilingSequencePair(Cluster* cluster,
                                           const int macros_to_place,
                                           SequencePair& initial_seq_pair)
{
  // Set positive sequence
  for (int i = 0; i < macros_to_place; ++i) {
    initial_seq_pair.pos_sequence.push_back(i);
  }

  // Set negative sequence
  const int columns
      = cluster->getWidth() / cluster->getHardMacros().front()->getWidth();
  const int rows
      = cluster->getHeight() / cluster->getHardMacros().front()->getHeight();

  for (int i = 1; i <= columns; ++i) {
    for (int j = 1; j <= rows; j++) {
      initial_seq_pair.neg_sequence.push_back(rows * i - j);
    }
  }
}

void HierRTLMP::computeFencesAndGuides(
    const std::vector<HardMacro*>& hard_macros,
    const Rect& outline,
    std::map<int, Rect>& fences,
    std::map<int, Rect>& guides)
{
  for (int i = 0; i < hard_macros.size(); ++i) {
    if (fences_.find(hard_macros[i]->getName()) != fences_.end()) {
      fences[i] = fences_[hard_macros[i]->getName()];
      fences[i].relocate(
          outline.xMin(), outline.yMin(), outline.xMax(), outline.yMax());
    }
    auto itr = guides_.find(hard_macros[i]->getInst());
    if (itr != guides_.end()) {
      guides[i] = itr->second;
      guides[i].relocate(
          outline.xMin(), outline.yMin(), outline.xMax(), outline.yMax());
    }
  }
}

// Create terminals for macro placement (Hard) annealing.
void HierRTLMP::createFixedTerminals(const Rect& outline,
                                     const UniqueClusterVector& macro_clusters,
                                     std::map<int, int>& cluster_to_macro,
                                     std::vector<HardMacro>& sa_macros)
{
  std::set<int> clusters_ids;

  for (auto& macro_cluster : macro_clusters) {
    for (auto [cluster_id, weight] : macro_cluster->getConnection()) {
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

std::vector<BundledNet> HierRTLMP::computeBundledNets(
    const UniqueClusterVector& macro_clusters,
    const std::map<int, int>& cluster_to_macro)
{
  std::vector<BundledNet> nets;

  for (auto& macro_cluster : macro_clusters) {
    const int src_id = macro_cluster->getId();

    for (auto [cluster_id, weight] : macro_cluster->getConnection()) {
      BundledNet net(
          cluster_to_macro.at(src_id), cluster_to_macro.at(cluster_id), weight);

      net.src_cluster_id = src_id;
      net.target_cluster_id = cluster_id;
      nets.push_back(net);
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

  const int soft_macro_center_x_dbu
      = block_->micronsToDbu(soft_macro->getPinX());
  const int soft_macro_center_y_dbu
      = block_->micronsToDbu(soft_macro->getPinY());

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
  float wirelength = 0.0f;

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

      wirelength += block_->dbuToMicrons(net_box.dx() + net_box.dy());
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
    if (!inst->isBlock() || ClusteringEngine::isIgnoredInst(inst)) {
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

  if (!inst) {
    return;
  }

  const int x = block_->micronsToDbu(hard_macro->getRealX());
  const int y = block_->micronsToDbu(hard_macro->getRealY());

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
    if (!inst) {
      continue;
    }

    snapper.setMacro(inst);
    snapper.snapMacro();

    inst->setPlacementStatus(odb::dbPlacementStatus::LOCKED);
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

odb::Rect HierRTLMP::micronsToDbu(const Rect& micron_rect)
{
  return odb::Rect(block_->micronsToDbu(micron_rect.xMin()),
                   block_->micronsToDbu(micron_rect.yMin()),
                   block_->micronsToDbu(micron_rect.xMax()),
                   block_->micronsToDbu(micron_rect.yMax()));
}

Rect HierRTLMP::dbuToMicrons(const odb::Rect& dbu_rect)
{
  return Rect(block_->dbuToMicrons(dbu_rect.xMin()),
              block_->dbuToMicrons(dbu_rect.yMin()),
              block_->dbuToMicrons(dbu_rect.xMax()),
              block_->dbuToMicrons(dbu_rect.yMax()));
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

bool HierRTLMP::isVertical(Boundary boundary) const
{
  return boundary == Boundary::L || boundary == Boundary::R;
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

template <typename SACore>
void HierRTLMP::printPlacementResult(Cluster* parent,
                                     const Rect& outline,
                                     SACore* sa_core)
{
  logger_->report("Id: {}", parent->getId());
  logger_->report("Outline: ({:^8.2f} {:^8.2f}) ({:^8.2f} {:^8.2f})",
                  outline.xMin(),
                  outline.yMin(),
                  outline.xMax(),
                  outline.yMax());
  sa_core->printResults();
}

//////// Pusher ////////

Pusher::Pusher(utl::Logger* logger,
               Cluster* root,
               odb::dbBlock* block,
               const std::vector<Rect>& io_blockages)
    : logger_(logger), root_(root), block_(block)
{
  core_ = block_->getCoreArea();
  setIOBlockages(io_blockages);
}

void Pusher::setIOBlockages(const std::vector<Rect>& io_blockages)
{
  for (const Rect& blockage : io_blockages) {
    io_blockages_.emplace_back(block_->micronsToDbu(blockage.xMin()),
                               block_->micronsToDbu(blockage.yMin()),
                               block_->micronsToDbu(blockage.xMax()),
                               block_->micronsToDbu(blockage.yMax()));
  }
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

  const odb::Rect cluster_box(
      block_->micronsToDbu(macro_cluster->getX()),
      block_->micronsToDbu(macro_cluster->getY()),
      block_->micronsToDbu(macro_cluster->getX() + macro_cluster->getWidth()),
      block_->micronsToDbu(macro_cluster->getY() + macro_cluster->getHeight()));

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

  const int hard_macro_width = hard_macro->getWidthDBU();
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

  const int hard_macro_height = hard_macro->getHeightDBU();
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

    odb::Rect cluster_box(
        block_->micronsToDbu(macro_cluster->getX()),
        block_->micronsToDbu(macro_cluster->getY()),
        block_->micronsToDbu(macro_cluster->getX() + macro_cluster->getWidth()),
        block_->micronsToDbu(macro_cluster->getY()
                             + macro_cluster->getHeight()));

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
      hard_macro->setXDBU(hard_macro->getXDBU() - distance);
      break;
    }
    case (Boundary::R): {
      hard_macro->setXDBU(hard_macro->getXDBU() + distance);
      break;
    }
    case (Boundary::T): {
      hard_macro->setYDBU(hard_macro->getYDBU() + distance);
      break;
    }
    case (Boundary::B): {
      hard_macro->setYDBU(hard_macro->getYDBU() - distance);
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

    if (cluster_box.xMin() < hard_macro->getUXDBU()
        && cluster_box.yMin() < hard_macro->getUYDBU()
        && cluster_box.xMax() > hard_macro->getXDBU()
        && cluster_box.yMax() > hard_macro->getYDBU()) {
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

  auto closest_pos = std::lower_bound(lowest_grid_positions.begin(),
                                      lowest_grid_positions.end(),
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
    std::sort(pins.begin(), pins.end(), compare_pin_center);
    layers_data.push_back(LayerData{track_grid, std::move(positions), pins});
  }

  auto compare_layer_number = [](LayerData data1, LayerData data2) {
    return (data1.track_grid->getTechLayer()->getNumber()
            < data2.track_grid->getTechLayer()->getNumber());
  };
  std::sort(layers_data.begin(), layers_data.end(), compare_layer_number);

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

  if (best_snapped_pins != total_pins) {
    reportUnalignedPins(layers_data_list, target_direction);
  }

  snapPinToPosition(snap_pin, positions[best_index], target_direction);
}

int Snapper::totalAlignedPins(const LayerDataList& layers_data_list,
                              const odb::dbTechLayerDir& direction,
                              bool report_unaligned_pins)
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
        if (report_unaligned_pins) {
          if (data.track_grid->getTechLayer()->isRightWayOnGridOnly()) {
            logger_->error(MPL,
                           5,
                           "Couldn't align pin {} from the RightWayOnGridOnly "
                           "layer {} with the track-grid.",
                           data.pins[i]->getName(),
                           data.track_grid->getTechLayer()->getName());
          } else {
            logger_->warn(
                MPL,
                2,
                "Couldn't align pin {} from layer {} to the track-grid.",
                data.pins[i]->getName(),
                data.track_grid->getTechLayer()->getName());
          }
        }

        i++;
      } else {
        j++;
      }
    }
  }
  return pins_aligned;
}

void Snapper::reportUnalignedPins(const LayerDataList& layers_data_list,
                                  const odb::dbTechLayerDir& direction)
{
  totalAlignedPins(layers_data_list, direction, true);
}

void Snapper::alignWithManufacturingGrid(int& origin)
{
  const int manufacturing_grid
      = inst_->getDb()->getTech()->getManufacturingGrid();

  origin = std::round(origin / static_cast<double>(manufacturing_grid))
           * manufacturing_grid;
}

}  // namespace mpl
