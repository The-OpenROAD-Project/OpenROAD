///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////
#include "hier_rtlmp.h"

#include <fstream>
#include <iostream>
#include <queue>
#include <thread>
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

  Pusher pusher(logger_, tree_->root.get(), block_, boundary_to_io_blockage_);
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
      block_, network_, logger_, tritonpart_);

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

  setPinAccessBlockages();
  setPlacementBlockages();
}

void HierRTLMP::setRootShapes()
{
  auto root_soft_macro = std::make_unique<SoftMacro>(tree_->root.get());

  const float root_area = tree_->floorplan_shape.getArea();
  const float root_width = tree_->floorplan_shape.getWidth();
  const std::vector<std::pair<float, float>> root_width_list
      = {std::pair<float, float>(root_width, root_width)};

  root_soft_macro->setShapes(root_width_list, root_area);
  root_soft_macro->setWidth(root_width);  // This will set height automatically
  root_soft_macro->setX(tree_->floorplan_shape.xMin());
  root_soft_macro->setY(tree_->floorplan_shape.yMin());
  tree_->root->setSoftMacro(std::move(root_soft_macro));
}

// Compare two intervals according to the product
static bool comparePairProduct(const std::pair<float, float>& p1,
                               const std::pair<float, float>& p2)
{
  return p1.first * p1.second < p2.first * p2.second;
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
      macro.setShapes(cluster->getMacroTilings(), true);  // force_flag = true
      macros.push_back(macro);
    }
  }
  // if there is only one soft macro
  // the parent cluster has the shape of the its child macro cluster
  if (macros.size() == 1) {
    for (auto& cluster : parent->getChildren()) {
      if (cluster->getNumMacro() > 0) {
        parent->setMacroTilings(cluster->getMacroTilings());
        return;
      }
    }
  }

  debugPrint(
      logger_, MPL, "coarse_shaping", 1, "Running SA to calculate tiling...");

  // call simulated annealing to determine tilings
  std::set<std::pair<float, float>> macro_tilings;  // <width, height>
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
                                              logger_);
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
        macro_tilings.insert(
            std::pair<float, float>(sa->getWidth(), sa->getHeight()));
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
                                              logger_);
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
        macro_tilings.insert(
            std::pair<float, float>(sa->getWidth(), sa->getHeight()));
      }
    }
    remaining_runs -= run_thread;
  }
  std::vector<std::pair<float, float>> tilings(macro_tilings.begin(),
                                               macro_tilings.end());
  std::sort(tilings.begin(), tilings.end(), comparePairProduct);
  for (auto& shape : tilings) {
    debugPrint(logger_,
               MPL,
               "coarse_shaping",
               2,
               "width: {}, height: {}, aspect_ratio: {}, min_ar: {}",
               shape.first,
               shape.second,
               shape.second / shape.first,
               min_ar_);
  }
  // we do not want very strange tilings if we have choices
  std::vector<std::pair<float, float>> new_tilings;
  for (auto& tiling : tilings) {
    if (tiling.second / tiling.first >= min_ar_
        && tiling.second / tiling.first <= 1.0 / min_ar_) {
      new_tilings.push_back(tiling);
    }
  }
  // if there are valid tilings
  if (!new_tilings.empty()) {
    tilings = std::move(new_tilings);
  }
  // update parent
  parent->setMacroTilings(tilings);
  if (tilings.empty()) {
    logger_->error(MPL,
                   3,
                   "There are no valid tilings for mixed cluster: {}",
                   parent->getName());
  } else {
    std::string line
        = "The macro tiling for mixed cluster " + parent->getName() + "  ";
    for (auto& shape : tilings) {
      line += " < " + std::to_string(shape.first) + " , ";
      line += std::to_string(shape.second) + " >  ";
    }
    line += "\n";
    debugPrint(logger_, MPL, "coarse_shaping", 2, "{}", line);
  }
}

void HierRTLMP::calculateMacroTilings(Cluster* cluster)
{
  if (cluster->getClusterType() != HardMacroCluster) {
    return;
  }

  std::vector<HardMacro*> hard_macros = cluster->getHardMacros();
  std::set<std::pair<float, float>> macro_tilings;  // <width, height>

  if (hard_macros.size() == 1) {
    float width = hard_macros[0]->getWidth();
    float height = hard_macros[0]->getHeight();

    std::vector<std::pair<float, float>> tilings;

    tilings.emplace_back(width, height);
    cluster->setMacroTilings(tilings);

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
                                              logger_);
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
        macro_tilings.insert(
            std::pair<float, float>(sa->getWidth(), sa->getHeight()));
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
                                              logger_);
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
        macro_tilings.insert(
            std::pair<float, float>(sa->getWidth(), sa->getHeight()));
      }
    }
    remaining_runs -= run_thread;
  }

  // sort the tilings based on area
  std::vector<std::pair<float, float>> tilings(macro_tilings.begin(),
                                               macro_tilings.end());
  std::sort(tilings.begin(), tilings.end(), comparePairProduct);
  for (auto& shape : tilings) {
    debugPrint(logger_,
               MPL,
               "coarse_shaping",
               2,
               "width: {}, height: {}",
               shape.first,
               shape.second);
  }
  // we only keep the minimum area tiling since all the macros has the same size
  // later this can be relaxed.  But this may cause problems because the
  // minimizing the wirelength may leave holes near the boundary
  std::vector<std::pair<float, float>> new_tilings;
  for (auto& tiling : tilings) {
    if (tiling.first * tiling.second <= tilings[0].first * tilings[0].second) {
      new_tilings.push_back(tiling);
    }
  }
  tilings = std::move(new_tilings);
  // update parent
  cluster->setMacroTilings(tilings);
  if (tilings.empty()) {
    logger_->error(MPL,
                   4,
                   "No valid tilings for hard macro cluster: {}",
                   cluster->getName());
  }

  std::string line = "Tiling for hard cluster " + cluster->getName() + "  ";
  for (auto& shape : tilings) {
    line += " < " + std::to_string(shape.first) + " , ";
    line += std::to_string(shape.second) + " >  ";
  }
  line += "\n";
  debugPrint(logger_, MPL, "coarse_shaping", 2, "{}", line);
}

// Used only for arrays of interconnected macros.
void HierRTLMP::setTightPackingTilings(Cluster* macro_array)
{
  std::vector<std::pair<float, float>> tight_packing_tilings;

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

  macro_array->setMacroTilings(tight_packing_tilings);
}

void HierRTLMP::setPinAccessBlockages()
{
  if (!tree_->maps.pad_to_bterm.empty()) {
    return;
  }

  const Metrics* top_module_metrics
      = tree_->maps.module_to_metrics.at(block_->getTopModule()).get();

  // Avoid creating blockages with zero depth.
  if (top_module_metrics->getStdCellArea() == 0.0) {
    return;
  }

  std::vector<Cluster*> clusters_of_unplaced_io_pins
      = getClustersOfUnplacedIOPins();
  const Rect die = dbuToMicrons(block_->getDieArea());

  const float depth
      = computePinAccessBlockagesDepth(clusters_of_unplaced_io_pins, die);

  for (Cluster* cluster_of_unplaced_io_pins : clusters_of_unplaced_io_pins) {
    Boundary constraint_boundary
        = cluster_of_unplaced_io_pins->getConstraintBoundary();
    if (constraint_boundary != NONE) {
      createPinAccessBlockage(constraint_boundary, depth, die);
    }
  }

  if (boundary_to_io_blockage_.empty()) {
    // If there are no constraints at all, give freedom to SA so it
    // doesn't have to deal with pin access blockages in all boundaries.
    // This will help SA not relying on extreme utilizations to
    // converge for designs such as sky130hd/uW.
    if (tree_->blocked_boundaries.empty()) {
      return;
    }

    // There are only -exclude constraints, so we create pin access
    // blockages based on the boundaries that are not blocked.
    if (tree_->blocked_boundaries.find(L) == tree_->blocked_boundaries.end()) {
      createPinAccessBlockage(L, depth, die);
    }

    if (tree_->blocked_boundaries.find(R) == tree_->blocked_boundaries.end()) {
      createPinAccessBlockage(R, depth, die);
    }

    if (tree_->blocked_boundaries.find(B) == tree_->blocked_boundaries.end()) {
      createPinAccessBlockage(B, depth, die);
    }

    if (tree_->blocked_boundaries.find(T) == tree_->blocked_boundaries.end()) {
      createPinAccessBlockage(T, depth, die);
    }
  }
}

void HierRTLMP::createPinAccessBlockage(Boundary constraint_boundary,
                                        const float depth,
                                        const Rect& die)
{
  Rect blockage = die;
  if (constraint_boundary == L) {
    blockage.setXMax(blockage.xMin() + depth);
  } else if (constraint_boundary == T) {
    blockage.setYMin(blockage.yMax() - depth);
  } else if (constraint_boundary == R) {
    blockage.setXMin(blockage.xMax() - depth);
  } else {  // Bottom
    blockage.setYMax(blockage.yMin() + depth);
  }

  boundary_to_io_blockage_[constraint_boundary] = blockage;
  macro_blockages_.push_back(blockage);
}

std::vector<Cluster*> HierRTLMP::getClustersOfUnplacedIOPins()
{
  std::vector<Cluster*> clusters_of_unplaced_io_pins;

  for (const auto& child : tree_->root->getChildren()) {
    if (child->isClusterOfUnplacedIOPins()) {
      clusters_of_unplaced_io_pins.push_back(child.get());
    }
  }

  return clusters_of_unplaced_io_pins;
}

// The depth of pin access blockages is computed based on:
// 1) Amount of std cell area in the design.
// 2) Extension of the IO clusters across the design's boundaries.
float HierRTLMP::computePinAccessBlockagesDepth(
    const std::vector<Cluster*>& io_clusters,
    const Rect& die)
{
  float io_clusters_extension = 0.0;

  for (Cluster* io_cluster : io_clusters) {
    if (io_cluster->getConstraintBoundary() == NONE) {
      const Rect die = io_cluster->getBBox();
      io_clusters_extension = die.getPerimeter();
      break;
    }

    Boundary constraint_boundary = io_cluster->getConstraintBoundary();

    if (constraint_boundary == L || constraint_boundary == R) {
      io_clusters_extension += die.getWidth();
    } else {  // Bottom or Top
      io_clusters_extension += die.getHeight();
    }
  }

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
  const float depth = (std_cell_area / io_clusters_extension)
                      * std::pow((1 - macro_dominance_factor), 2);

  debugPrint(logger_,
             MPL,
             "coarse_shaping",
             1,
             "Pin access blockages depth = {}",
             depth);

  return depth;
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
                                              logger_);
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
                                              logger_);
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
    auto frontwave = parents.front();
    parents.pop();

    Cluster* grandparent = frontwave->getParent();
    for (auto& cluster : grandparent->getChildren()) {
      if (cluster->getId() != frontwave->getId()) {
        soft_macro_id_map[cluster->getName()]
            = static_cast<int>(soft_macros.size());

        const float center_x = cluster->getX() + cluster->getWidth() / 2.0;
        const float center_y = cluster->getY() + cluster->getHeight() / 2.0;
        Point location = {center_x - outline.xMin(), center_y - outline.yMin()};

        // The information of whether or not a cluster is a group of
        // unplaced IO pins is needed inside the SA Core, so if a fixed
        // terminal corresponds to a cluster of unplaced IO pins it needs
        // to contain that cluster data.
        Cluster* fixed_terminal_cluster
            = cluster->isClusterOfUnplacedIOPins() ? cluster.get() : nullptr;

        // Note that a fixed terminal is just a point.
        soft_macros.emplace_back(location,
                                 cluster->getName(),
                                 0.0f /* width */,
                                 0.0f /* height */,
                                 fixed_terminal_cluster);
      }
    }

    if (frontwave->getParent()->getParent() != nullptr) {
      parents.push(frontwave->getParent());
    }
  }
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
      std::vector<std::pair<float, float>> shapes;
      for (auto& shape : cluster->getMacroTilings()) {
        if (shape.first < outline_width * (1 + conversion_tolerance_)
            && shape.second < outline_height * (1 + conversion_tolerance_)) {
          shapes.push_back(shape);
        }
      }
      if (shapes.empty()) {
        logger_->error(MPL,
                       7,
                       "Not enough space in cluster: {} for "
                       "child hard macro cluster: {}",
                       parent->getName(),
                       cluster->getName());
      }
      macro_cluster_area += shapes[0].first * shapes[0].second;
      cluster->setMacroTilings(shapes);
    } else {  // mixed cluster
      std_cell_mixed_cluster_area += cluster->getStdCellArea();
      std::vector<std::pair<float, float>> shapes;
      for (auto& shape : cluster->getMacroTilings()) {
        if (shape.first < outline_width * (1 + conversion_tolerance_)
            && shape.second < outline_height * (1 + conversion_tolerance_)) {
          shapes.push_back(shape);
        }
      }
      if (shapes.empty()) {
        logger_->error(MPL,
                       8,
                       "Not enough space in cluster: {} for "
                       "child mixed cluster: {}",
                       parent->getName(),
                       cluster->getName());
      }
      macro_mixed_cluster_area += shapes[0].first * shapes[0].second;
      cluster->setMacroTilings(shapes);
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
      std::vector<std::pair<float, float>> width_list;
      width_list.emplace_back(width, area / width);
      macros[soft_macro_id_map[cluster->getName()]].setShapes(width_list, area);
    } else if (cluster->getClusterType() == HardMacroCluster) {
      macros[soft_macro_id_map[cluster->getName()]].setShapes(
          cluster->getMacroTilings());
      debugPrint(logger_,
                 MPL,
                 "fine_shaping",
                 2,
                 "hard_macro_cluster : {}",
                 cluster->getName());
      for (auto& shape : cluster->getMacroTilings()) {
        debugPrint(logger_,
                   MPL,
                   "fine_shaping",
                   2,
                   "    ( {} , {} ) ",
                   shape.first,
                   shape.second);
      }
    } else {  // Mixed cluster
      const std::vector<std::pair<float, float>> tilings
          = cluster->getMacroTilings();
      std::vector<std::pair<float, float>> width_list;
      // use the largest area
      float area = tilings[tilings.size() - 1].first
                   * tilings[tilings.size() - 1].second;
      area += cluster->getStdCellArea() / target_util;
      for (auto& shape : tilings) {
        if (shape.first * shape.second <= area) {
          width_list.emplace_back(shape.first, area / shape.second);
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
      for (auto& width : width_list) {
        debugPrint(logger_,
                   MPL,
                   "fine_shaping",
                   2,
                   " [  {} {}  ] ",
                   width.first,
                   width.second);
      }
      macros[soft_macro_id_map[cluster->getName()]].setShapes(width_list, area);
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
                                              logger_);
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
    auto& temp_cluster = tree_->maps.id_to_cluster[cluster_id];

    // model other cluster as a fixed macro with zero size
    cluster_to_macro[cluster_id] = sa_macros.size();
    sa_macros.emplace_back(
        std::pair<float, float>(
            temp_cluster->getX() + temp_cluster->getWidth() / 2.0
                - outline.xMin(),
            temp_cluster->getY() + temp_cluster->getHeight() / 2.0
                - outline.yMin()),
        temp_cluster->getName(),
        // The information of whether or not a cluster is a group of
        // unplaced IO pins is needed inside the SA Core, so if a fixed
        // terminal corresponds to a cluster of unplaced IO pins it needs
        // to contain that cluster data.
        temp_cluster->isClusterOfUnplacedIOPins() ? temp_cluster : nullptr);
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

// force-directed placement to generate guides for macros
// Attractive force and Repulsive force should be normalied separately
// Because their values can vary a lot.
void HierRTLMP::FDPlacement(std::vector<Rect>& blocks,
                            const std::vector<BundledNet>& nets,
                            float outline_width,
                            float outline_height,
                            const std::string& file_name)
{
  // following the ideas of Circuit Training
  logger_->report(
      "*****************************************************************");
  logger_->report("Start force-directed placement");
  const float ar = outline_height / outline_width;
  const std::vector<int> num_steps{1000, 1000, 1000, 1000};
  const std::vector<float> attract_factors{1.0, 100.0, 1.0, 1.0};
  const std::vector<float> repel_factors{1.0, 1.0, 100.0, 10.0};
  const float io_factor = 1000.0;
  const float max_size = std::max(outline_width, outline_height);
  std::mt19937 rand_gen(random_seed_);
  std::uniform_real_distribution<float> distribution(0.0, 1.0);

  auto calcAttractiveForce = [&](float attract_factor) {
    for (auto& net : nets) {
      const int& src = net.terminals.first;
      const int& sink = net.terminals.second;
      // float k = net.weight * attract_factor;
      float k = net.weight;
      if (blocks[src].fixed_flag == true || blocks[sink].fixed_flag == true
          || blocks[src].getWidth() < 1.0 || blocks[src].getHeight() < 1.0
          || blocks[sink].getWidth() < 1.0 || blocks[sink].getHeight() < 1.0) {
        k = k * io_factor;
      }
      const float x_dist
          = (blocks[src].getX() - blocks[sink].getX()) / max_size;
      const float y_dist
          = (blocks[src].getY() - blocks[sink].getY()) / max_size;
      const float dist = std::sqrt(x_dist * x_dist + y_dist * y_dist);
      const float f_x = k * x_dist * dist;
      const float f_y = k * y_dist * dist;
      blocks[src].addAttractiveForce(-1.0 * f_x, -1.0 * f_y);
      blocks[sink].addAttractiveForce(f_x, f_y);
    }
  };

  auto calcRepulsiveForce = [&](float repulsive_factor) {
    std::vector<int> macros(blocks.size());
    std::iota(macros.begin(), macros.end(), 0);
    std::sort(macros.begin(), macros.end(), [&](int src, int target) {
      return blocks[src].lx < blocks[target].lx;
    });
    // traverse all the macros
    auto iter = macros.begin();
    while (iter != macros.end()) {
      int src = *iter;
      for (auto iter_loop = ++iter; iter_loop != macros.end(); iter_loop++) {
        int target = *iter_loop;
        if (blocks[src].ux <= blocks[target].lx) {
          break;
        }
        if (blocks[src].ly >= blocks[target].uy
            || blocks[src].uy <= blocks[target].ly) {
          continue;
        }
        // ignore the overlap between clusters and IO ports
        if (blocks[src].getWidth() < 1.0 || blocks[src].getHeight() < 1.0
            || blocks[target].getWidth() < 1.0
            || blocks[target].getHeight() < 1.0) {
          continue;
        }
        if (blocks[src].fixed_flag == true
            || blocks[target].fixed_flag == true) {
          continue;
        }
        // apply the force from src to target
        if (src > target) {
          std::swap(src, target);
        }
        // check the overlap
        const float x_min_dist
            = (blocks[src].getWidth() + blocks[target].getWidth()) / 2.0;
        const float y_min_dist
            = (blocks[src].getHeight() + blocks[target].getHeight()) / 2.0;
        const float x_overlap
            = std::abs(blocks[src].getX() - blocks[target].getX()) - x_min_dist;
        const float y_overlap
            = std::abs(blocks[src].getY() - blocks[target].getY()) - y_min_dist;
        if (x_overlap <= 0.0 && y_overlap <= 0.0) {
          float x_dist
              = (blocks[src].getX() - blocks[target].getX()) / x_min_dist;
          float y_dist
              = (blocks[src].getY() - blocks[target].getY()) / y_min_dist;
          float dist = std::sqrt(x_dist * x_dist + y_dist * y_dist);
          const float min_dist = 0.01;
          if (dist <= min_dist) {
            x_dist = std::sqrt(min_dist);
            y_dist = std::sqrt(min_dist);
            dist = min_dist;
          }
          // const float f_x = repulsive_factor * x_dist / (dist * dist);
          // const float f_y = repulsive_factor * y_dist / (dist * dist);
          const float f_x = x_dist / (dist * dist);
          const float f_y = y_dist / (dist * dist);
          blocks[src].addRepulsiveForce(f_x, f_y);
          blocks[target].addRepulsiveForce(-1.0 * f_x, -1.0 * f_y);
        }
      }
    }
  };

  auto MoveBlock =
      [&](float attract_factor, float repulsive_factor, float max_move_dist) {
        for (auto& block : blocks) {
          block.resetForce();
        }
        if (attract_factor > 0) {
          calcAttractiveForce(attract_factor);
        }
        if (repulsive_factor > 0) {
          calcRepulsiveForce(repulsive_factor);
        }
        // normalization
        float max_f_a = 0.0;
        float max_f_r = 0.0;
        float max_f = 0.0;
        for (auto& block : blocks) {
          max_f_a = std::max(
              max_f_a,
              std::sqrt(block.f_x_a * block.f_x_a + block.f_y_a * block.f_y_a));
          max_f_r = std::max(
              max_f_r,
              std::sqrt(block.f_x_r * block.f_x_r + block.f_y_r * block.f_y_r));
        }
        max_f_a = std::max(max_f_a, 1.0f);
        max_f_r = std::max(max_f_r, 1.0f);
        // Move node
        // The move will be cancelled if the block will be pushed out of the
        // boundary
        for (auto& block : blocks) {
          const float f_x = attract_factor * block.f_x_a / max_f_a
                            + repulsive_factor * block.f_x_r / max_f_r;
          const float f_y = attract_factor * block.f_y_a / max_f_a
                            + repulsive_factor * block.f_y_r / max_f_r;
          block.setForce(f_x, f_y);
          max_f = std::max(
              max_f, std::sqrt(block.f_x * block.f_x + block.f_y * block.f_y));
        }
        max_f = std::max(max_f, 1.0f);
        for (auto& block : blocks) {
          const float x_dist
              = block.f_x / max_f * max_move_dist
                + (distribution(rand_gen) - 0.5) * 0.1 * max_move_dist;
          const float y_dist
              = block.f_y / max_f * max_move_dist
                + (distribution(rand_gen) - 0.5) * 0.1 * max_move_dist;
          block.move(x_dist, y_dist, 0.0f, 0.0f, outline_width, outline_height);
        }
      };

  // initialize all the macros
  for (auto& block : blocks) {
    block.makeSquare(ar);
    block.setLoc(outline_width * distribution(rand_gen),
                 outline_height * distribution(rand_gen),
                 0.0f,
                 0.0f,
                 outline_width,
                 outline_height);
  }

  // Iteratively place the blocks
  for (auto i = 0; i < num_steps.size(); i++) {
    // const float max_move_dist = max_size / num_steps[i];
    const float attract_factor = attract_factors[i];
    const float repulsive_factor = repel_factors[i];
    for (auto j = 0; j < num_steps[i]; j++) {
      MoveBlock(attract_factor,
                repulsive_factor,
                max_size / (1 + std::floor(j / 100)));
    }
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
        auto constraint_region = bterm->getConstraintRegion();
        if (constraint_region) {
          int x = constraint_region->xCenter();
          int y = constraint_region->yCenter();
          odb::Rect region_rect(x, y, x, y);
          net_box.merge(region_rect);
        } else {
          odb::Point macro_pin_location(macro_pin->getBBox().xCenter(),
                                        macro_pin->getBBox().yCenter());
          Boundary closest_boundary = getClosestBoundary(
              macro_pin_location, tree_->unblocked_boundaries);

          // As we classify the blocked/unblocked state of the boundary based on
          // the extension of the -exclude constraint, it's possible to have
          // all boundaries blocked for IOs even though there are small
          // unblocked spaces in those boundaries. For this situation, we just
          // skip IOs without constraint regions.
          if (closest_boundary == NONE) {
            continue;
          }

          odb::Point closest_point
              = getClosestBoundaryPoint(macro_pin_location, closest_boundary);
          odb::Rect closest_point_rect(closest_point, closest_point);
          net_box.merge(closest_point_rect);
        }
      }

      wirelength += block_->dbuToMicrons(net_box.dx() + net_box.dy());
    }
  }

  return wirelength;
}

// Search the given boundaries list for the closest boundary to the point.
Boundary HierRTLMP::getClosestBoundary(const odb::Point& from,
                                       const std::set<Boundary>& boundaries)
{
  Boundary closest_boundary = NONE;
  int shortest_distance = std::numeric_limits<int>::max();

  for (const Boundary boundary : boundaries) {
    const int dist_to_boundary = getDistanceToBoundary(from, boundary);
    if (dist_to_boundary < shortest_distance) {
      shortest_distance = dist_to_boundary;
      closest_boundary = boundary;
    }
  }

  return closest_boundary;
}

int HierRTLMP::getDistanceToBoundary(const odb::Point& from,
                                     const Boundary boundary)
{
  int distance = 0;

  if (boundary == L) {
    distance = from.x() - block_->getDieArea().xMin();
  } else if (boundary == R) {
    distance = from.x() - block_->getDieArea().xMax();
  } else if (boundary == B) {
    distance = from.y() - block_->getDieArea().yMin();
  } else if (boundary == T) {
    distance = from.y() - block_->getDieArea().yMax();
  }

  return std::abs(distance);
}

odb::Point HierRTLMP::getClosestBoundaryPoint(const odb::Point& from,
                                              const Boundary boundary)
{
  odb::Point closest_boundary_point;
  const odb::Rect& die = block_->getDieArea();

  if (boundary == L) {
    closest_boundary_point.setX(die.xMin());
    closest_boundary_point.setY(from.y());
  } else if (boundary == R) {
    closest_boundary_point.setX(die.xMax());
    closest_boundary_point.setY(from.y());
  } else if (boundary == B) {
    closest_boundary_point.setX(from.x());
    closest_boundary_point.setY(die.yMin());
  } else {  // Top
    closest_boundary_point.setX(from.x());
    closest_boundary_point.setY(die.yMax());
  }

  return closest_boundary_point;
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
               const std::map<Boundary, Rect>& boundary_to_io_blockage)
    : logger_(logger), root_(root), block_(block)
{
  core_ = block_->getCoreArea();
  setIOBlockages(boundary_to_io_blockage);
}

void Pusher::setIOBlockages(
    const std::map<Boundary, Rect>& boundary_to_io_blockage)
{
  for (const auto& [boundary, box] : boundary_to_io_blockage) {
    boundary_to_io_blockage_[boundary]
        = odb::Rect(block_->micronsToDbu(box.getX()),
                    block_->micronsToDbu(box.getY()),
                    block_->micronsToDbu(box.getX() + box.getWidth()),
                    block_->micronsToDbu(box.getY() + box.getHeight()));
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
    hor_boundary_to_push = L;
    smaller_hor_distance = distance_to_left;
  } else {
    hor_boundary_to_push = R;
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
    ver_boundary_to_push = B;
    smaller_ver_distance = distance_to_bottom;
  } else {
    ver_boundary_to_push = T;
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
        || overlapsWithIOBlockage(cluster_box, boundary)) {
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
    case NONE:
      return;
    case L: {
      cluster_box.moveDelta(-distance, 0);
      break;
    }
    case R: {
      cluster_box.moveDelta(distance, 0);
      break;
    }
    case T: {
      cluster_box.moveDelta(0, distance);
      break;
    }
    case B: {
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
    case NONE:
      return;
    case L: {
      hard_macro->setXDBU(hard_macro->getXDBU() - distance);
      break;
    }
    case R: {
      hard_macro->setXDBU(hard_macro->getXDBU() + distance);
      break;
    }
    case T: {
      hard_macro->setYDBU(hard_macro->getYDBU() + distance);
      break;
    }
    case B: {
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

bool Pusher::overlapsWithIOBlockage(const odb::Rect& cluster_box,
                                    const Boundary boundary)
{
  if (boundary_to_io_blockage_.find(boundary)
      == boundary_to_io_blockage_.end()) {
    return false;
  }

  const odb::Rect box = boundary_to_io_blockage_.at(boundary);

  if (cluster_box.xMin() < box.xMax() && cluster_box.yMin() < box.yMax()
      && cluster_box.xMax() > box.xMin() && cluster_box.yMax() > box.yMin()) {
    debugPrint(logger_,
               MPL,
               "boundary_push",
               1,
               "\tFound overlap with IO blockage {}. Push will be reverted.",
               box);
    return true;
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
  SameDirectionLayersData layers_data
      = computeSameDirectionLayersData(target_direction);

  int origin = target_direction == odb::dbTechLayerDir::VERTICAL
                   ? inst_->getOrigin().x()
                   : inst_->getOrigin().y();

  if (!layers_data.snap_layer) {
    // There are no pins to align with the track-grid.
    alignWithManufacturingGrid(origin);
    setOrigin(origin, target_direction);
    return;
  }

  odb::dbITerm* snap_pin = layers_data.layer_to_pin.at(layers_data.snap_layer);
  const LayerParameters& snap_layer_params
      = layers_data.layer_to_params.at(layers_data.snap_layer);

  if (!pinsAreAlignedWithTrackGrid(
          snap_pin, snap_layer_params, target_direction)) {
    // The idea here is to first align the origin of the macro with
    // the track-grid taking into account that the grid has a certain
    // offset with regards to (0,0) and, then, compensate the offset
    // of the pins themselves so that the lines of the grid cross
    // their center.
    origin = std::round(origin / static_cast<double>(snap_layer_params.pitch))
                 * snap_layer_params.pitch
             + snap_layer_params.offset - snap_layer_params.pin_offset;

    alignWithManufacturingGrid(origin);
    setOrigin(origin, target_direction);
  }

  attemptSnapToExtraLayers(
      origin, layers_data, snap_layer_params, target_direction);
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

SameDirectionLayersData Snapper::computeSameDirectionLayersData(
    const odb::dbTechLayerDir& target_direction)
{
  SameDirectionLayersData data;

  for (odb::dbITerm* iterm : inst_->getITerms()) {
    if (iterm->getSigType() != odb::dbSigType::SIGNAL) {
      continue;
    }

    for (odb::dbMPin* mpin : iterm->getMTerm()->getMPins()) {
      for (odb::dbBox* box : mpin->getGeometry()) {
        odb::dbTechLayer* layer = box->getTechLayer();
        if (layer->getDirection() == target_direction) {
          if (data.layer_to_pin.find(layer) != data.layer_to_pin.end()) {
            continue;
          }

          if (data.layer_to_pin.empty()) {
            data.snap_layer = layer;
          }

          data.layer_to_pin[layer] = iterm;
          data.layer_to_params[layer]
              = computeLayerParameters(layer, iterm, target_direction);
        }
      }
    }
  }

  return data;
}

LayerParameters Snapper::computeLayerParameters(
    odb::dbTechLayer* layer,
    odb::dbITerm* pin,
    const odb::dbTechLayerDir& target_direction)
{
  LayerParameters params;

  odb::dbBlock* block = inst_->getBlock();
  odb::dbTrackGrid* track_grid = block->findTrackGrid(layer);

  if (track_grid) {
    getTrackGrid(track_grid, params.offset, params.pitch, target_direction);
  } else {
    logger_->error(
        MPL, 39, "No track-grid found for layer {}", layer->getName());
  }

  params.pin_width = getPinWidth(pin, target_direction);
  params.lower_left_to_first_pin
      = getPinToLowerLeftDistance(pin, target_direction);

  // The distance between the pins and the lower-left corner
  // of the master of a macro instance may not be a multiple
  // of the track-grid, in these cases, we need to compensate
  // a small offset.
  int mterm_offset = 0;
  if (params.pitch != 0) {
    mterm_offset = params.lower_left_to_first_pin
                   - std::floor(params.lower_left_to_first_pin / params.pitch)
                         * params.pitch;
  }
  params.pin_offset = params.pin_width / 2 + mterm_offset;

  const odb::dbOrientType& orientation = inst_->getOrient();
  if (target_direction == odb::dbTechLayerDir::VERTICAL) {
    if (orientation == odb::dbOrientType::MY
        || orientation == odb::dbOrientType::R180) {
      params.pin_offset = -params.pin_offset;
    }
  } else if (orientation == odb::dbOrientType::MX
             || orientation == odb::dbOrientType::R180) {
    params.pin_offset = -params.pin_offset;
  }
  return params;
}

int Snapper::getPinWidth(odb::dbITerm* pin,
                         const odb::dbTechLayerDir& target_direction)
{
  int pin_width = 0;
  if (target_direction == odb::dbTechLayerDir::VERTICAL) {
    pin_width = pin->getBBox().dx();
  } else {
    pin_width = pin->getBBox().dy();
  }
  return pin_width;
}

void Snapper::getTrackGrid(odb::dbTrackGrid* track_grid,
                           int& origin,
                           int& step,
                           const odb::dbTechLayerDir& target_direction)
{
  // TODO: handle multiple patterns
  int count;
  if (target_direction == odb::dbTechLayerDir::VERTICAL) {
    track_grid->getGridPatternX(0, origin, count, step);
  } else {
    track_grid->getGridPatternY(0, origin, count, step);
  }
}

int Snapper::getPinToLowerLeftDistance(
    odb::dbITerm* pin,
    const odb::dbTechLayerDir& target_direction)
{
  int pin_to_origin = 0;

  odb::dbMTerm* mterm = pin->getMTerm();
  if (target_direction == odb::dbTechLayerDir::VERTICAL) {
    pin_to_origin = mterm->getBBox().xMin();
  } else {
    pin_to_origin = mterm->getBBox().yMin();
  }

  return pin_to_origin;
}

void Snapper::attemptSnapToExtraLayers(
    const int origin,
    const SameDirectionLayersData& layers_data,
    const LayerParameters& snap_layer_params,
    const odb::dbTechLayerDir& target_direction)
{
  // Calculate LCM from the layers pitches to define the search
  // range
  int lcm = 1;
  for (const auto& [layer, params] : layers_data.layer_to_params) {
    lcm = std::lcm(lcm, params.pitch);
  }
  const int pitch = snap_layer_params.pitch;
  const int total_attempts = lcm / snap_layer_params.pitch;

  const int total_layers = static_cast<int>(layers_data.layer_to_pin.size());

  int best_origin = origin;
  int best_snapped_layers = 1;

  for (int i = 0; i <= total_attempts; i++) {
    // Alternates steps from positive to negative incrementally
    int steps = (i % 2 == 1) ? (i + 1) / 2 : -(i / 2);

    setOrigin(origin + (pitch * steps), target_direction);
    int snapped_layers = 0;
    for (const auto& [layer, pin] : layers_data.layer_to_pin) {
      if (pinsAreAlignedWithTrackGrid(
              pin, layers_data.layer_to_params.at(layer), target_direction)) {
        ++snapped_layers;
      }
    }

    if (snapped_layers > best_snapped_layers) {
      best_snapped_layers = snapped_layers;
      best_origin = origin + (pitch * steps);

      // Stop search if all layers are snapped
      if (total_layers == best_snapped_layers) {
        break;
      }
    }
  }

  setOrigin(best_origin, target_direction);
}

bool Snapper::pinsAreAlignedWithTrackGrid(
    odb::dbITerm* pin,
    const LayerParameters& layer_params,
    const odb::dbTechLayerDir& target_direction)
{
  int pin_center = target_direction == odb::dbTechLayerDir::VERTICAL
                       ? pin->getBBox().xCenter()
                       : pin->getBBox().yCenter();
  return (pin_center - layer_params.offset) % layer_params.pitch == 0;
}

void Snapper::alignWithManufacturingGrid(int& origin)
{
  const int manufacturing_grid
      = inst_->getDb()->getTech()->getManufacturingGrid();

  origin = std::round(origin / static_cast<double>(manufacturing_grid))
           * manufacturing_grid;
}

}  // namespace mpl
