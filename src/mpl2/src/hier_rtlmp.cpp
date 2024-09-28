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

#include "Mpl2Observer.h"
#include "SACoreHardMacro.h"
#include "SACoreSoftMacro.h"
#include "bus_synthesis.h"
#include "db_sta/dbNetwork.hh"
#include "object.h"
#include "odb/db.h"
#include "odb/util.h"
#include "par/PartitionMgr.h"
#include "sta/Liberty.hh"
#include "utl/Logger.h"

namespace mpl2 {

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
                     sta::dbSta* sta,
                     utl::Logger* logger,
                     par::PartitionMgr* tritonpart)
    : network_(network),
      db_(db),
      sta_(sta),
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
  area_weight_ = weight;
}

void HierRTLMP::setOutlineWeight(float weight)
{
  outline_weight_ = weight;
}

void HierRTLMP::setWirelengthWeight(float weight)
{
  wirelength_weight_ = weight;
}

void HierRTLMP::setGuidanceWeight(float weight)
{
  guidance_weight_ = weight;
}

void HierRTLMP::setFenceWeight(float weight)
{
  fence_weight_ = weight;
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
  global_fence_lx_ = fence_lx;
  global_fence_ly_ = fence_ly;
  global_fence_ux_ = fence_ux;
  global_fence_uy_ = fence_uy;
}

void HierRTLMP::setHaloWidth(float halo_width)
{
  tree_->halo_width = halo_width;
}

void HierRTLMP::setHaloHeight(float halo_height)
{
  tree_->halo_height = halo_height;
}

// Options related to clustering
void HierRTLMP::setNumBundledIOsPerBoundary(int num_bundled_ios)
{
  tree_->bundled_ios_per_edge = num_bundled_ios;
}

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

void HierRTLMP::setSnapLayer(int snap_layer)
{
  snap_layer_ = snap_layer;
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
//      *This is executed within hierarchical placement method.
// 4) Hierarchical Macro Placement -> Top - Down
//      a) Placement of Clusters (one level at a time);
//      b) Placement of Macros (one macro cluster at a time).
// 5) Boundary Pushing
//      Push macro clusters to the boundaries of the design if they don't
//      overlap with either bundled IOs' blockages or other macros.
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

  runCoarseShaping();
  runHierarchicalMacroPlacement();

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

  // Set target structures
  clustering_engine_->setDesignMetrics(metrics_);
  clustering_engine_->setTree(tree_.get());

  clustering_engine_->run();

  if (!tree_->has_unfixed_macros) {
    skip_macro_placement_ = true;
    return;
  }

  if (graphics_) {
    graphics_->finishedClustering(tree_->root.get());
  }
}

void HierRTLMP::runHierarchicalMacroPlacement()
{
  if (graphics_) {
    graphics_->startFine();
  }

  adjustMacroBlockageWeight();
  if (logger_->debugCheck(MPL, "hierarchical_macro_placement", 1)) {
    reportSAWeights();
  }

  if (bus_planning_on_) {
    adjustCongestionWeight();
    runHierarchicalMacroPlacement(tree_->root.get());
  } else {
    runHierarchicalMacroPlacementWithoutBusPlanning(tree_->root.get());
  }

  if (graphics_) {
    graphics_->setMaxLevel(tree_->max_level);
    graphics_->drawResult();
  }
}

void HierRTLMP::resetSAParameters()
{
  pos_swap_prob_ = 0.2;
  neg_swap_prob_ = 0.2;
  double_swap_prob_ = 0.2;
  exchange_swap_prob_ = 0.2;
  flip_prob_ = 0.2;
  resize_prob_ = 0.0;
  guidance_weight_ = 0.0;
  fence_weight_ = 0.0;
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

  setIOClustersBlockages();
  setPlacementBlockages();
}

void HierRTLMP::setRootShapes()
{
  auto root_soft_macro = std::make_unique<SoftMacro>(tree_->root.get());

  const float core_lx
      = static_cast<float>(block_->dbuToMicrons(block_->getCoreArea().xMin()));
  const float root_lx = std::max(core_lx, global_fence_lx_);

  const float core_ly
      = static_cast<float>(block_->dbuToMicrons(block_->getCoreArea().yMin()));
  const float root_ly = std::max(core_ly, global_fence_ly_);

  const float core_ux
      = static_cast<float>(block_->dbuToMicrons(block_->getCoreArea().xMax()));
  const float root_ux = std::min(core_ux, global_fence_ux_);

  const float core_uy
      = static_cast<float>(block_->dbuToMicrons(block_->getCoreArea().yMax()));
  const float root_uy = std::min(core_uy, global_fence_uy_);

  const float root_area = (root_ux - root_lx) * (root_uy - root_ly);
  const float root_width = root_ux - root_lx;
  const std::vector<std::pair<float, float>> root_width_list
      = {std::pair<float, float>(root_width, root_width)};

  root_soft_macro->setShapes(root_width_list, root_area);
  root_soft_macro->setWidth(root_width);  // This will set height automatically
  root_soft_macro->setX(root_lx);
  root_soft_macro->setY(root_ly);
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
          = std::make_unique<SACoreSoftMacro>(tree_->root.get(),
                                              new_outline,
                                              macros,
                                              1.0,     // area weight
                                              1000.0,  // outline weight
                                              0.0,     // wirelength weight
                                              0.0,     // guidance weight
                                              0.0,     // fence weight
                                              0.0,     // boundary weight
                                              0.0,     // macro blockage
                                              0.0,     // notch weight
                                              0.0,     // no notch size
                                              0.0,     // no notch size
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
          = std::make_unique<SACoreSoftMacro>(tree_->root.get(),
                                              new_outline,
                                              macros,
                                              1.0,     // area weight
                                              1000.0,  // outline weight
                                              0.0,     // wirelength weight
                                              0.0,     // guidance weight
                                              0.0,     // fence weight
                                              0.0,     // boundary weight
                                              0.0,     // macro blockage
                                              0.0,     // notch weight
                                              0.0,     // no notch size
                                              0.0,     // no notch size
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
          = std::make_unique<SACoreHardMacro>(new_outline,
                                              macros,
                                              1.0,     // area_weight
                                              1000.0,  // outline weight
                                              0.0,     // wirelength weight
                                              0.0,     // guidance
                                              0.0,     // fence weight
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
          = std::make_unique<SACoreHardMacro>(new_outline,
                                              macros,
                                              1.0,     // area_weight
                                              1000.0,  // outline weight
                                              0.0,     // wirelength weight
                                              0.0,     // guidance
                                              0.0,     // fence weight
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

void HierRTLMP::setIOClustersBlockages()
{
  if (!tree_->maps.bterm_to_inst.empty()) {
    return;
  }

  IOSpans io_spans = computeIOSpans();
  const float depth = computeIOBlockagesDepth(io_spans);

  const Rect root(tree_->root->getX(),
                  tree_->root->getY(),
                  tree_->root->getX() + tree_->root->getWidth(),
                  tree_->root->getY() + tree_->root->getHeight());

  // Note that the range can be larger than the respective core dimension.
  // As SA only sees what is inside its current outline, this is not a problem.
  if (io_spans[L].second > io_spans[L].first) {
    const Rect left_io_blockage(root.xMin(),
                                io_spans[L].first,
                                root.xMin() + depth,
                                io_spans[L].second);

    boundary_to_io_blockage_[L] = left_io_blockage;
    macro_blockages_.push_back(left_io_blockage);
  }

  if (io_spans[T].second > io_spans[T].first) {
    const Rect top_io_blockage(io_spans[T].first,
                               root.yMax() - depth,
                               io_spans[T].second,
                               root.yMax());

    boundary_to_io_blockage_[T] = top_io_blockage;
    macro_blockages_.push_back(top_io_blockage);
  }

  if (io_spans[R].second > io_spans[R].first) {
    const Rect right_io_blockage(root.xMax() - depth,
                                 io_spans[R].first,
                                 root.xMax(),
                                 io_spans[R].second);

    boundary_to_io_blockage_[R] = right_io_blockage;
    macro_blockages_.push_back(right_io_blockage);
  }

  if (io_spans[B].second > io_spans[B].first) {
    const Rect bottom_io_blockage(io_spans[B].first,
                                  root.yMin(),
                                  io_spans[B].second,
                                  root.yMin() + depth);

    boundary_to_io_blockage_[B] = bottom_io_blockage;
    macro_blockages_.push_back(bottom_io_blockage);
  }
}

// Determine the range of IOs in each boundary of the die.
HierRTLMP::IOSpans HierRTLMP::computeIOSpans()
{
  IOSpans io_spans;

  odb::Rect die = block_->getDieArea();

  // Initialize spans based on the dimensions of the die area.
  io_spans[L]
      = {block_->dbuToMicrons(die.yMax()), block_->dbuToMicrons(die.yMin())};
  io_spans[T]
      = {block_->dbuToMicrons(die.xMax()), block_->dbuToMicrons(die.xMin())};
  io_spans[R] = io_spans[L];
  io_spans[B] = io_spans[T];

  for (auto term : block_->getBTerms()) {
    if (term->getSigType().isSupply()) {
      continue;
    }

    int lx = std::numeric_limits<int>::max();
    int ly = std::numeric_limits<int>::max();
    int ux = 0;
    int uy = 0;

    for (const auto pin : term->getBPins()) {
      for (const auto box : pin->getBoxes()) {
        lx = std::min(lx, box->xMin());
        ly = std::min(ly, box->yMin());
        ux = std::max(ux, box->xMax());
        uy = std::max(uy, box->yMax());
      }
    }

    // Modify ranges based on the position of the IO pins.
    if (lx <= die.xMin()) {
      io_spans[L].first = std::min(
          io_spans[L].first, static_cast<float>(block_->dbuToMicrons(ly)));
      io_spans[L].second = std::max(
          io_spans[L].second, static_cast<float>(block_->dbuToMicrons(uy)));
    } else if (uy >= die.yMax()) {
      io_spans[T].first = std::min(
          io_spans[T].first, static_cast<float>(block_->dbuToMicrons(lx)));
      io_spans[T].second = std::max(
          io_spans[T].second, static_cast<float>(block_->dbuToMicrons(ux)));
    } else if (ux >= die.xMax()) {
      io_spans[R].first = std::min(
          io_spans[R].first, static_cast<float>(block_->dbuToMicrons(ly)));
      io_spans[R].second = std::max(
          io_spans[R].second, static_cast<float>(block_->dbuToMicrons(uy)));
    } else {
      io_spans[B].first = std::min(
          io_spans[B].first, static_cast<float>(block_->dbuToMicrons(lx)));
      io_spans[B].second = std::max(
          io_spans[B].second, static_cast<float>(block_->dbuToMicrons(ux)));
    }
  }

  return io_spans;
}

// The depth of IO clusters' blockages is generated based on:
// 1) How many vertical or horizontal boundaries have signal IO pins.
// 2) The total length of the io spans in all used boundaries.
float HierRTLMP::computeIOBlockagesDepth(const IOSpans& io_spans)
{
  float sum_length = 0.0;
  int num_hor_access = 0;
  int num_ver_access = 0;

  for (auto& [pin_access, length] : io_spans) {
    if (length.second > length.first) {
      sum_length += std::abs(length.second - length.first);

      if (pin_access == R || pin_access == L) {
        num_hor_access++;
      } else {
        num_ver_access++;
      }
    }
  }

  float std_cell_area = 0.0;
  for (auto& cluster : tree_->root->getChildren()) {
    if (cluster->getClusterType() == StdCellCluster) {
      std_cell_area += cluster->getArea();
    }
  }

  const float macro_dominance_factor
      = tree_->macro_with_halo_area
        / (tree_->root->getWidth() * tree_->root->getHeight());
  const float depth = (std_cell_area / sum_length)
                      * std::pow((1 - macro_dominance_factor), 2);

  debugPrint(logger_,
             MPL,
             "coarse_shaping",
             1,
             "Bundled IO clusters blokaged depth = {}",
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

void HierRTLMP::adjustCongestionWeight()
{
  std::string snap_layer_name;

  for (auto& macro : tree_->maps.inst_to_hard) {
    odb::dbMaster* master = macro.first->getMaster();
    for (odb::dbMTerm* mterm : master->getMTerms()) {
      if (mterm->getSigType() == odb::dbSigType::SIGNAL) {
        for (odb::dbMPin* mpin : mterm->getMPins()) {
          for (odb::dbBox* box : mpin->getGeometry()) {
            odb::dbTechLayer* layer = box->getTechLayer();
            snap_layer_name = layer->getName();
          }
        }
      }
    }

    break;
  }

  // update weight
  if (dynamic_congestion_weight_flag_) {
    std::vector<std::string> layers;
    for (odb::dbTechLayer* layer : db_->getTech()->getLayers()) {
      if (layer->getType() == odb::dbTechLayerType::ROUTING) {
        layers.push_back(layer->getName());
      }
    }
    snap_layer_ = 0;
    for (int i = 0; i < layers.size(); i++) {
      if (layers[i] == snap_layer_name) {
        snap_layer_ = i + 1;
        break;
      }
    }
    if (snap_layer_ <= 0) {
      congestion_weight_ = 0.0;
    } else {
      congestion_weight_ = 1.0 * snap_layer_ / layers.size();
    }
    debugPrint(logger_,
               MPL,
               "bus_planning",
               1,
               "Adjusting congestion weight to {} - Snap layer is {}",
               congestion_weight_,
               snap_layer_);
  }
}

// Cluster Placement Engine Starts ...........................................
// The cluster placement is done in a top-down manner
// (Preorder DFS)
// The magic happens at how we determine the size of pin access
// If the size of pin access is too large, SA cannot generate the valid macro
// placement If the size of pin access is too small, the effect of bus synthesis
// can be ignored. Here our trick is to determine the size of pin access and
// standard-cell clusters together. We assume the region occupied by pin access
// will be filled by standard cells. More specifically, we first determine the
// width of pin access based on number of connections passing the pin access,
// then we calculate the height of the pin access based on the available area,
// standard-cell area and macro area. Note that, here we allow the utilization
// of standard-cell clusters larger than 1.0.  If there is no standard cells,
// the area of pin access is 0. Another important trick is that we call SA two
// times. The first time is to determine the location of pin access. The second
// time is to determine the location of children clusters. We assume the
// summation of pin access size is equal to the area of standard-cell clusters
void HierRTLMP::runHierarchicalMacroPlacement(Cluster* parent)
{
  // base case
  // If the parent cluster has no macros (parent cluster is a StdCellCluster or
  // IOCluster) We do not need to determine the positions and shapes of its
  // children clusters
  if (parent->getNumMacro() == 0) {
    return;
  }
  // If the parent is a HardMacroCluster
  if (parent->getClusterType() == HardMacroCluster) {
    placeMacros(parent);
    return;
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
  const Rect outline(parent->getX(),
                     parent->getY(),
                     parent->getX() + parent->getWidth(),
                     parent->getY() + parent->getHeight());

  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Working on children of cluster: {}, Outline "
             "{}, {}  {}, {}",
             parent->getName(),
             outline.xMin(),
             outline.yMin(),
             outline.getWidth(),
             outline.getHeight());

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

  // We store the bundled io clusters to push them into the macros' vector
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
    Rect fence(-1.0, -1.0, -1.0, -1.0);
    Rect guide(-1.0, -1.0, -1.0, -1.0);
    const std::vector<HardMacro*> hard_macros = cluster->getHardMacros();
    for (auto& hard_macro : hard_macros) {
      if (fences_.find(hard_macro->getName()) != fences_.end()) {
        fence.merge(fences_[hard_macro->getName()]);
      }
      if (guides_.find(hard_macro->getName()) != guides_.end()) {
        guide.merge(guides_[hard_macro->getName()]);
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

  clustering_engine_->updateConnections();
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Finished calculating connection");

  int number_of_pin_access = 0;
  // Handle the pin access
  // Get the connections between pin accesses
  if (parent->getParent() != nullptr) {
    // the parent cluster is not the root cluster
    // First step model each pin access as the a dummy softmacro (width = 0.0,
    // height = 0.0) In our simulated annealing engine, the dummary softmacro
    // will no effect on SA We have four dummy SoftMacros based on our
    // definition
    std::vector<Boundary> pins = {L, T, R, B};
    for (auto& pin : pins) {
      soft_macro_id_map[toString(pin)] = macros.size();
      macros.emplace_back(0.0, 0.0, toString(pin));

      ++number_of_pin_access;
    }
    // add the connections between pin accesses, for example, L to R
    for (auto& [src_pin, pin_map] : parent->getBoundaryConnection()) {
      for (auto& [target_pin, weight] : pin_map) {
        nets.emplace_back(soft_macro_id_map[toString(src_pin)],
                          soft_macro_id_map[toString(target_pin)],
                          weight);
      }
    }
  }

  int num_of_macros_to_place = static_cast<int>(macros.size());

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
                 2,
                 " Cluster connection: {} {} {} ",
                 cluster->getName(),
                 tree_->maps.id_to_cluster[cluster_id]->getName(),
                 weight);
      const std::string name = tree_->maps.id_to_cluster[cluster_id]->getName();
      if (soft_macro_id_map.find(name) == soft_macro_id_map.end()) {
        float new_weight = weight;
        if (macros[soft_macro_id_map[src_name]].isStdCellCluster()) {
          new_weight *= tree_->virtual_weight;
        }
        // if the cluster_id is out of the parent cluster
        BundledNet net(
            soft_macro_id_map[src_name],
            soft_macro_id_map[toString(parent->getPinAccess(cluster_id).first)],
            new_weight);
        net.src_cluster_id = src_id;
        net.target_cluster_id = cluster_id;
        nets.push_back(net);
      } else if (src_id > cluster_id) {
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

  if (parent->getParent() != nullptr) {
    // update the size of each pin access macro
    // each pin access macro with have their fences
    // check the net connection, i.e., how many nets crossing the boundaries
    std::map<int, float> net_map;
    net_map[soft_macro_id_map[toString(L)]] = 0.0;
    net_map[soft_macro_id_map[toString(T)]] = 0.0;
    net_map[soft_macro_id_map[toString(R)]] = 0.0;
    net_map[soft_macro_id_map[toString(B)]] = 0.0;
    for (auto& net : nets) {
      if (net_map.find(net.terminals.first) != net_map.end()) {
        net_map[net.terminals.first] += net.weight;
      }
      if (net_map.find(net.terminals.second) != net_map.end()) {
        net_map[net.terminals.second] += net.weight;
      }
    }

    debugPrint(logger_,
               MPL,
               "hierarchical_macro_placement",
               1,
               "Total tracks per micron:  {}",
               1.0 / pin_access_net_width_ratio_);

    // determine the width of each pin access
    float l_size
        = net_map[soft_macro_id_map[toString(L)]] * pin_access_net_width_ratio_;
    float r_size
        = net_map[soft_macro_id_map[toString(R)]] * pin_access_net_width_ratio_;
    float b_size
        = net_map[soft_macro_id_map[toString(B)]] * pin_access_net_width_ratio_;
    float t_size
        = net_map[soft_macro_id_map[toString(T)]] * pin_access_net_width_ratio_;
    const std::vector<std::pair<float, float>> tilings
        = parent->getMacroTilings();
    // When the program enter stage, the tilings cannot be empty
    // const float ar = outline_height / outline_width;
    // float max_height = std::sqrt(tilings[0].first * tilings[0].second * ar);
    // float max_width = max_height / ar;
    // max_height = std::max(max_height, tilings[0].first);
    // max_width = std::max(max_width, tilings[0].second);
    float max_height = 0.0;
    float max_width = 0.0;
    for (auto& tiling : tilings) {
      if (tiling.first <= outline.getWidth()
          && tiling.second <= outline.getHeight()) {
        max_width = std::max(max_width, tiling.first);
        max_height = std::max(max_height, tiling.second);
      }
    }
    max_width = std::min(max_width, outline.getWidth());
    max_height = std::min(max_height, outline.getHeight());
    debugPrint(logger_,
               MPL,
               "hierarchical_macro_placement",
               1,
               "Pin access calculation "
               "max_width: {}, max_height : {}",
               max_width,
               max_height);
    l_size = std::min(l_size, max_height);
    r_size = std::min(r_size, max_height);
    t_size = std::min(t_size, max_width);
    b_size = std::min(b_size, max_width);
    // determine the height of each pin access
    max_width = outline.getWidth() - max_width;
    max_height = outline.getHeight() - max_height;
    // the area of standard-cell clusters
    float std_cell_area = 0.0;
    for (auto& cluster : parent->getChildren()) {
      if (cluster->getClusterType() == StdCellCluster) {
        std_cell_area += cluster->getArea();
      }
    }
    // calculate the depth based on area
    float sum_length = 0.0;
    int num_hor_access = 0;
    int num_ver_access = 0;
    if (l_size > 0.0) {
      num_hor_access += 1;
      sum_length += l_size;
    }
    if (r_size > 0.0) {
      num_hor_access += 1;
      sum_length += r_size;
    }
    if (t_size > 0.0) {
      num_ver_access += 1;
      sum_length += t_size;
    }
    if (b_size > 0.0) {
      num_ver_access += 1;
      sum_length += b_size;
    }
    max_width = num_hor_access > 0 ? max_width / num_hor_access : max_width;
    max_height = num_ver_access > 0 ? max_height / num_ver_access : max_height;
    const float depth = std_cell_area / sum_length;
    // update the size of pin access macro
    if (l_size > 0) {
      const float temp_width = std::min(max_width, depth);
      macros[soft_macro_id_map[toString(L)]]
          = SoftMacro(temp_width, l_size, toString(L));
      fences[soft_macro_id_map[toString(L)]]
          = Rect(0.0, 0.0, temp_width, outline.getWidth());

      debugPrint(logger_,
                 MPL,
                 "hierarchical_macro_placement",
                 1,
                 "Left - width : {}, height : {}",
                 l_size,
                 temp_width);
    }
    if (r_size > 0) {
      const float temp_width = std::min(max_width, depth);
      macros[soft_macro_id_map[toString(R)]]
          = SoftMacro(temp_width, r_size, toString(R));
      fences[soft_macro_id_map[toString(R)]]
          = Rect(outline.getWidth() - temp_width,
                 0.0,
                 outline.getWidth(),
                 outline.getHeight());

      debugPrint(logger_,
                 MPL,
                 "hierarchical_macro_placement",
                 1,
                 "Right - width : {}, height : {}",
                 r_size,
                 temp_width);
    }
    if (t_size > 0) {
      const float temp_height = std::min(max_height, depth);
      macros[soft_macro_id_map[toString(T)]]
          = SoftMacro(t_size, temp_height, toString(T));
      fences[soft_macro_id_map[toString(T)]]
          = Rect(0.0,
                 outline.getHeight() - temp_height,
                 outline.getWidth(),
                 outline.getHeight());

      debugPrint(logger_,
                 MPL,
                 "hierarchical_macro_placement",
                 1,
                 "Top - width : {}, height : {}",
                 t_size,
                 temp_height);
    }
    if (b_size > 0) {
      const float temp_height = std::min(max_height, depth);
      macros[soft_macro_id_map[toString(B)]]
          = SoftMacro(b_size, temp_height, toString(B));
      fences[soft_macro_id_map[toString(B)]]
          = Rect(0.0, 0.0, outline.getWidth(), temp_height);

      debugPrint(logger_,
                 MPL,
                 "hierarchical_macro_placement",
                 1,
                 "Bottom - width : {}, height : {}",
                 b_size,
                 temp_height);
    }
  }

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

  // Write the connections between macros
  std::ofstream file;
  std::string file_name = parent->getName();
  file_name = make_filename(file_name);

  file_name = report_directory_ + "/" + file_name;
  file.open(file_name + "net.txt");
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
  SoftSAVector sa_containers;  // The owner of SACores objects.
  float best_cost = std::numeric_limits<float>::max();
  // To give consistency across threads we check the solutions
  // at a fixed interval independent of how many threads we are using.
  const int check_interval = 10;
  int begin_check = 0;
  int end_check = std::min(check_interval, remaining_runs);
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Start Simulated Annealing Core");
  while (remaining_runs > 0) {
    SoftSAVector sa_batch;
    const int run_thread
        = graphics_ ? 1 : std::min(remaining_runs, num_threads_);
    for (int i = 0; i < run_thread; i++) {
      debugPrint(logger_,
                 MPL,
                 "hierarchical_macro_placement",
                 1,
                 "Start Simulated Annealing (run_id = {})",
                 run_id);

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
                 "Finished adjusting shapes for children of cluster {}",
                 parent->getName());
      // Note that all the probabilities are normalized to the summation of 1.0.
      // Note that the weight are not necessaries summarized to 1.0, i.e., not
      // normalized.
      std::unique_ptr<SACoreSoftMacro> sa
          = std::make_unique<SACoreSoftMacro>(tree_->root.get(),
                                              outline,
                                              shaped_macros,
                                              area_weight_,
                                              outline_weight_,
                                              wirelength_weight_,
                                              guidance_weight_,
                                              fence_weight_,
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
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Finished Simulated Annealing Core");

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

    logger_->error(MPL, 5, "Failed on cluster {}", parent->getName());
  }

  if (best_sa->centralizationWasReverted()) {
    best_sa->alignMacroClusters();
  }
  best_sa->fillDeadSpace();

  // update the clusters and do bus planning
  std::vector<SoftMacro> shaped_macros;
  best_sa->getMacros(shaped_macros);
  file.open(file_name + ".fp.txt.temp");
  for (auto& macro : shaped_macros) {
    file << macro.getName() << "   " << macro.getX() << "   " << macro.getY()
         << "   " << macro.getWidth() << "   " << macro.getHeight()
         << std::endl;
  }
  file.close();

  if (parent->getParent() != nullptr) {
    // ***********************************************************************
    // Now convert the area occupied by pin access macros to hard macro
    // blockages. Note at this stage, the size of each pin access macro is 0.0.
    // But you cannot remove the pin access macros. You still need these pin
    // access macros to maintain the connections
    // ***********************************************************************
    if (shaped_macros[soft_macro_id_map[toString(L)]].getWidth() > 0.0) {
      const float l_ly = shaped_macros[soft_macro_id_map[toString(L)]].getY();
      const float l_width
          = shaped_macros[soft_macro_id_map[toString(L)]].getWidth();
      const float l_height
          = shaped_macros[soft_macro_id_map[toString(L)]].getHeight();
      macro_blockages.emplace_back(0.0, l_ly, l_width, l_ly + l_height);
      shaped_macros[soft_macro_id_map[toString(L)]]
          = SoftMacro(std::pair<float, float>(0.0, l_ly),
                      toString(L),
                      0.0,
                      l_height,
                      nullptr);
    }
    if (shaped_macros[soft_macro_id_map[toString(R)]].getWidth() > 0.0) {
      const float r_ly = shaped_macros[soft_macro_id_map[toString(R)]].getY();
      const float r_width
          = shaped_macros[soft_macro_id_map[toString(R)]].getWidth();
      const float r_height
          = shaped_macros[soft_macro_id_map[toString(R)]].getHeight();
      macro_blockages.emplace_back(outline.getWidth() - r_width,
                                   r_ly,
                                   outline.getWidth(),
                                   r_ly + r_height);
      shaped_macros[soft_macro_id_map[toString(R)]]
          = SoftMacro(std::pair<float, float>(outline.getWidth(), r_ly),
                      toString(R),
                      0.0,
                      r_height,
                      nullptr);
    }
    if (shaped_macros[soft_macro_id_map[toString(T)]].getWidth() > 0.0) {
      const float t_lx = shaped_macros[soft_macro_id_map[toString(T)]].getX();
      const float t_width
          = shaped_macros[soft_macro_id_map[toString(T)]].getWidth();
      const float t_height
          = shaped_macros[soft_macro_id_map[toString(T)]].getHeight();
      macro_blockages.emplace_back(t_lx,
                                   outline.getHeight() - t_height,
                                   t_lx + t_width,
                                   outline.getHeight());
      shaped_macros[soft_macro_id_map[toString(T)]]
          = SoftMacro(std::pair<float, float>(t_lx, outline.getHeight()),
                      toString(T),
                      t_width,
                      0.0,
                      nullptr);
    }
    if (shaped_macros[soft_macro_id_map[toString(B)]].getWidth() > 0.0) {
      const float b_lx = shaped_macros[soft_macro_id_map[toString(B)]].getX();
      const float b_width
          = shaped_macros[soft_macro_id_map[toString(B)]].getWidth();
      const float b_height
          = shaped_macros[soft_macro_id_map[toString(B)]].getHeight();
      macro_blockages.emplace_back(b_lx, 0.0, b_lx + b_width, b_height);
      shaped_macros[soft_macro_id_map[toString(B)]]
          = SoftMacro(std::pair<float, float>(b_lx, 0.0),
                      toString(B),
                      b_width,
                      0.0,
                      nullptr);
    }

    // Exclude the pin access macros from the sequence pair now that
    // they were converted to macro blockages.
    num_of_macros_to_place -= number_of_pin_access;

    macros = shaped_macros;
    remaining_runs = target_util_list.size();
    run_id = 0;
    best_sa = nullptr;
    sa_containers.clear();  // Destroy SACores used for pin access.
    best_cost = std::numeric_limits<float>::max();
    begin_check = 0;
    end_check = std::min(check_interval, remaining_runs);

    int outline_weight, boundary_weight;
    setWeightsForConvergence(
        outline_weight, boundary_weight, nets, num_of_macros_to_place);

    debugPrint(logger_,
               MPL,
               "hierarchical_macro_placement",
               1,
               "Start Simulated Annealing Core");
    while (remaining_runs > 0) {
      SoftSAVector sa_batch;
      const int run_thread
          = graphics_ ? 1 : std::min(remaining_runs, num_threads_);
      for (int i = 0; i < run_thread; i++) {
        debugPrint(logger_,
                   MPL,
                   "hierarchical_macro_placement",
                   1,
                   "Start Simulated Annealing (run_id = {})",
                   run_id);

        std::vector<SoftMacro> shaped_macros = macros;  // copy for multithread

        const float target_util = target_util_list[run_id];
        const float target_dead_space = target_dead_space_list[run_id++];

        debugPrint(logger_,
                   MPL,
                   "fine_shaping",
                   1,
                   "Starting adjusting shapes for children of {}. target_util "
                   "= {}, target_dead_space = {}",
                   parent->getName(),
                   target_util,
                   target_dead_space);

        if (!runFineShaping(parent,
                            shaped_macros,
                            soft_macro_id_map,
                            target_util,
                            target_dead_space)) {
          debugPrint(
              logger_,
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
                   "Finished adjusting shapes for children of cluster {}",
                   parent->getName());
        // Note that all the probabilities are normalized to the summation
        // of 1.0. Note that the weight are not necessaries summarized to 1.0,
        // i.e., not normalized.
        std::unique_ptr<SACoreSoftMacro> sa = std::make_unique<SACoreSoftMacro>(
            tree_->root.get(),
            outline,
            shaped_macros,
            area_weight_,
            outline_weight,
            wirelength_weight_,
            guidance_weight_,
            fence_weight_,
            boundary_weight,
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
    debugPrint(logger_,
               MPL,
               "hierarchical_macro_placement",
               1,
               "Finished Simulated Annealing Core");
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

      logger_->error(MPL, 6, "Failed on cluster {}", parent->getName());
    }

    if (best_sa->centralizationWasReverted()) {
      best_sa->alignMacroClusters();
    }
    best_sa->fillDeadSpace();

    // update the clusters and do bus planning
    best_sa->getMacros(shaped_macros);
  }

  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Finished Simulated Annealing for cluster: {}\n",
             parent->getName());
  best_sa->printResults();
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

  // check if the parent cluster still need bus planning
  for (auto& child : parent->getChildren()) {
    if (child->getClusterType() == MixedCluster) {
      debugPrint(logger_,
                 MPL,
                 "bus_planning",
                 1,
                 "Calling bus planning for cluster {}",
                 parent->getName());
      callBusPlanning(shaped_macros, nets);
      break;
    }
  }

  updateChildrenRealLocation(parent, outline.xMin(), outline.yMin());

  // Continue cluster placement on children
  for (auto& cluster : parent->getChildren()) {
    if (cluster->getClusterType() == MixedCluster
        || cluster->getClusterType() == HardMacroCluster) {
      runHierarchicalMacroPlacement(cluster.get());
    }
  }

  sa_containers.clear();

  clustering_engine_->updateInstancesAssociation(parent);
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
    float new_macro_blockage_weight = outline_weight_ / 2.0;
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

void HierRTLMP::setWeightsForConvergence(int& outline_weight,
                                         int& boundary_weight,
                                         const std::vector<BundledNet>& nets,
                                         const int number_of_placeable_macros)
{
  outline_weight = outline_weight_;
  boundary_weight = boundary_weight_;

  if (nets.size() < number_of_placeable_macros / 2) {
    // If a design has too few connections, there's a possibily that, for
    // some tight outlines, the outline penalty will struggle to win the
    // fight against the boundary penalty. This can happen specially for
    // large hierarchical designs that were reduced by deltaDebug.
    outline_weight *= 2;
    boundary_weight /= 2;

    debugPrint(logger_,
               MPL,
               "hierarchical_macro_placement",
               1,
               "Number of bundled nets is below half of the number of "
               "placeable macros, adapting "
               "weights. Outline {} -> {}, Boundary {} -> {}.",
               outline_weight_,
               outline_weight,
               boundary_weight_,
               boundary_weight);
  }
}

void HierRTLMP::reportSAWeights()
{
  logger_->report("\nSimmulated Annealing Weights:\n");
  logger_->report("Area = {}", area_weight_);
  logger_->report("Outline = {}", outline_weight_);
  logger_->report("WL = {}", wirelength_weight_);
  logger_->report("Guidance = {}", guidance_weight_);
  logger_->report("Fence = {}", fence_weight_);
  logger_->report("Boundary = {}", boundary_weight_);
  logger_->report("Notch = {}", notch_weight_);
  logger_->report("Macro Blockage = {}\n", macro_blockage_weight_);
}

// Multilevel macro placement without bus planning
void HierRTLMP::runHierarchicalMacroPlacementWithoutBusPlanning(Cluster* parent)
{
  // base case
  // If the parent cluster has no macros (parent cluster is a StdCellCluster or
  // IOCluster) We do not need to determine the positions and shapes of its
  // children clusters
  if (parent->getNumMacro() == 0) {
    return;
  }
  // If the parent is a HardMacroCluster
  if (parent->getClusterType() == HardMacroCluster) {
    placeMacros(parent);
    return;
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
  const Rect outline(parent->getX(),
                     parent->getY(),
                     parent->getX() + parent->getWidth(),
                     parent->getY() + parent->getHeight());

  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Working on children of cluster: {}, Outline "
             "{}, {}  {}, {}",
             parent->getName(),
             outline.xMin(),
             outline.yMin(),
             outline.getWidth(),
             outline.getHeight());

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

  // We store the bundled io clusters to push them into the macros' vector
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
    Rect fence(-1.0, -1.0, -1.0, -1.0);
    Rect guide(-1.0, -1.0, -1.0, -1.0);
    const std::vector<HardMacro*> hard_macros = cluster->getHardMacros();
    for (auto& hard_macro : hard_macros) {
      if (fences_.find(hard_macro->getName()) != fences_.end()) {
        fence.merge(fences_[hard_macro->getName()]);
      }
      if (guides_.find(hard_macro->getName()) != guides_.end()) {
        guide.merge(guides_[hard_macro->getName()]);
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

  // model other clusters as fixed terminals
  if (parent->getParent() != nullptr) {
    std::queue<Cluster*> parents;
    parents.push(parent);
    while (parents.empty() == false) {
      auto frontwave = parents.front();
      parents.pop();
      for (auto& cluster : frontwave->getParent()->getChildren()) {
        if (cluster->getId() != frontwave->getId()) {
          // model this as a fixed softmacro
          soft_macro_id_map[cluster->getName()] = macros.size();
          macros.emplace_back(
              std::pair<float, float>(
                  cluster->getX() + cluster->getWidth() / 2.0 - outline.xMin(),
                  cluster->getY() + cluster->getHeight() / 2.0
                      - outline.yMin()),
              cluster->getName(),
              0.0,
              0.0,
              nullptr);
          debugPrint(
              logger_,
              MPL,
              "hierarchical_macro_placement",
              1,
              "fixed cluster : {}, lx = {}, ly = {}, width = {}, height = {}",
              cluster->getName(),
              cluster->getX(),
              cluster->getY(),
              cluster->getWidth(),
              cluster->getHeight());
        }
      }
      if (frontwave->getParent()->getParent() != nullptr) {
        parents.push(frontwave->getParent());
      }
    }
  }

  // update the connnection
  clustering_engine_->updateConnections();
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Finished calculating connection");
  clustering_engine_->updateDataFlow();
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Finished updating dataflow");

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
                 2,
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
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Finished creating bundled connections");
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

  int outline_weight, boundary_weight;
  setWeightsForConvergence(
      outline_weight, boundary_weight, nets, num_of_macros_to_place);

  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Start Simulated Annealing Core");
  while (remaining_runs > 0) {
    SoftSAVector sa_batch;
    const int run_thread
        = graphics_ ? 1 : std::min(remaining_runs, num_threads_);
    for (int i = 0; i < run_thread; i++) {
      debugPrint(logger_,
                 MPL,
                 "hierarchical_macro_placement",
                 1,
                 "Start Simulated Annealing (run_id = {})",
                 run_id);

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
          = std::make_unique<SACoreSoftMacro>(tree_->root.get(),
                                              outline,
                                              shaped_macros,
                                              area_weight_,
                                              outline_weight,
                                              wirelength_weight_,
                                              guidance_weight_,
                                              fence_weight_,
                                              boundary_weight,
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
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Finished Simulated Annealing Core");
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

    runEnhancedHierarchicalMacroPlacement(parent);
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
    debugPrint(logger_,
               MPL,
               "hierarchical_macro_placement",
               1,
               "Finished Simulated Annealing for cluster {}",
               parent->getName());
    best_sa->printResults();
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

  // Continue cluster placement on children
  for (auto& cluster : parent->getChildren()) {
    if (cluster->getClusterType() == MixedCluster
        || cluster->getClusterType() == HardMacroCluster) {
      runHierarchicalMacroPlacementWithoutBusPlanning(cluster.get());
    }
  }

  clustering_engine_->updateInstancesAssociation(parent);
}

// This function is used in cases with very high density, in which it may
// be very hard to generate a valid tiling for the clusters.
// Here, we may want to try setting the area of all standard-cell clusters to 0.
// This should be only be used in mixed clusters.
void HierRTLMP::runEnhancedHierarchicalMacroPlacement(Cluster* parent)
{
  // base case
  // If the parent cluster has no macros (parent cluster is a StdCellCluster or
  // IOCluster) We do not need to determine the positions and shapes of its
  // children clusters
  if (parent->getNumMacro() == 0) {
    return;
  }
  // If the parent is the not the mixed cluster
  if (parent->getClusterType() != MixedCluster) {
    return;
  }
  // If the parent has children of mixed cluster
  for (auto& cluster : parent->getChildren()) {
    if (cluster->getClusterType() == MixedCluster) {
      return;
    }
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

  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Working on children of cluster: {}, Outline "
             "{}, {}  {}, {}",
             parent->getName(),
             outline.xMin(),
             outline.yMin(),
             outline.getWidth(),
             outline.getHeight());

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

  // We store the bundled io clusters to push them into the macros' vector
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
    Rect fence(-1.0, -1.0, -1.0, -1.0);
    Rect guide(-1.0, -1.0, -1.0, -1.0);
    const std::vector<HardMacro*> hard_macros = cluster->getHardMacros();
    for (auto& hard_macro : hard_macros) {
      if (fences_.find(hard_macro->getName()) != fences_.end()) {
        fence.merge(fences_[hard_macro->getName()]);
      }
      if (guides_.find(hard_macro->getName()) != guides_.end()) {
        guide.merge(guides_[hard_macro->getName()]);
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

  // model other clusters as fixed terminals
  if (parent->getParent() != nullptr) {
    std::queue<Cluster*> parents;
    parents.push(parent);
    while (parents.empty() == false) {
      auto frontwave = parents.front();
      parents.pop();
      for (auto& cluster : frontwave->getParent()->getChildren()) {
        if (cluster->getId() != frontwave->getId()) {
          // model this as a fixed softmacro
          soft_macro_id_map[cluster->getName()] = macros.size();
          macros.emplace_back(
              std::pair<float, float>(
                  cluster->getX() + cluster->getWidth() / 2.0 - outline.xMin(),
                  cluster->getY() + cluster->getHeight() / 2.0
                      - outline.yMin()),
              cluster->getName(),
              0.0,
              0.0,
              nullptr);
          debugPrint(
              logger_,
              MPL,
              "hierarchical_macro_placement",
              1,
              "fixed cluster : {}, lx = {}, ly = {}, width = {}, height = {}",
              cluster->getName(),
              cluster->getX(),
              cluster->getY(),
              cluster->getWidth(),
              cluster->getHeight());
        }
      }
      if (frontwave->getParent()->getParent() != nullptr) {
        parents.push(frontwave->getParent());
      }
    }
  }

  // update the connnection
  clustering_engine_->updateConnections();
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Finished calculating connection");
  clustering_engine_->updateDataFlow();
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Finished updating dataflow");

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
                 2,
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
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Finished creating bundled connections");
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

  int outline_weight, boundary_weight;
  setWeightsForConvergence(
      outline_weight, boundary_weight, nets, macros_to_place);

  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Start Simulated Annealing Core");
  while (remaining_runs > 0) {
    SoftSAVector sa_batch;
    const int run_thread
        = graphics_ ? 1 : std::min(remaining_runs, num_threads_);
    for (int i = 0; i < run_thread; i++) {
      std::vector<SoftMacro> shaped_macros = macros;  // copy for multithread
      // determine the shape for each macro
      debugPrint(logger_,
                 MPL,
                 "hierarchical_macro_placement",
                 1,
                 "Start Simulated Annealing (run_id = {})",
                 run_id);
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
          = std::make_unique<SACoreSoftMacro>(tree_->root.get(),
                                              outline,
                                              shaped_macros,
                                              area_weight_,
                                              outline_weight,
                                              wirelength_weight_,
                                              guidance_weight_,
                                              fence_weight_,
                                              boundary_weight,
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
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Finished Simulated Annealing Core");
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

  // update the clusters and do bus planning
  std::vector<SoftMacro> shaped_macros;
  best_sa->getMacros(shaped_macros);
  file.open(file_name + ".fp.txt.temp");
  for (auto& macro : shaped_macros) {
    file << macro.getName() << "   " << macro.getX() << "   " << macro.getY()
         << "   " << macro.getWidth() << "   " << macro.getHeight()
         << std::endl;
  }
  file.close();

  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Finish Simulated Annealing for cluster {}",
             parent->getName());

  best_sa->printResults();
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
      continue;  // IO clusters have no area
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
      continue;  // the area of IO cluster is 0.0
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

// Call Path Synthesis to route buses
void HierRTLMP::callBusPlanning(std::vector<SoftMacro>& shaped_macros,
                                std::vector<BundledNet>& nets_old)
{
  std::vector<BundledNet> nets;
  for (auto& net : nets_old) {
    debugPrint(logger_, MPL, "bus_planning", 1, "net weight: {}", net.weight);
    if (net.weight > bus_net_threshold_) {
      nets.push_back(net);
    }
  }

  std::vector<int> soft_macro_vertex_id;
  std::vector<Edge> edge_list;
  std::vector<Vertex> vertex_list;
  if (!calNetPaths(shaped_macros,
                   soft_macro_vertex_id,
                   edge_list,
                   vertex_list,
                   nets,
                   congestion_weight_,
                   logger_)) {
    logger_->error(MPL, 9, "Bus planning has failed!");
  }
}

void HierRTLMP::placeMacros(Cluster* cluster)
{
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Place macros in cluster: {}",
             cluster->getName());

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

      std::unique_ptr<SACoreHardMacro> sa = std::make_unique<SACoreHardMacro>(
          outline,
          sa_macros,
          area_weight_,
          outline_weight_ * (run_id + 1) * 10,
          wirelength_weight_ / (run_id + 1),
          guidance_weight_,
          fence_weight_,
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
      SACoreWeights weights;
      weights.area = area_weight_;
      weights.outline = outline_weight_;
      weights.wirelength = wirelength_weight_;
      weights.guidance = guidance_weight_;
      weights.fence = fence_weight_;

      // Reset weights so we can compare the final costs.
      sa->setWeights(weights);
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
    best_sa->printResults();

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
    if (guides_.find(hard_macros[i]->getName()) != guides_.end()) {
      guides[i] = guides_[hard_macros[i]->getName()];
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
        temp_cluster->getName());
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

  for (odb::dbITerm* iterm : macro->getITerms()) {
    if (iterm->getSigType() != odb::dbSigType::SIGNAL) {
      continue;
    }

    odb::dbNet* net = iterm->getNet();
    if (net != nullptr) {
      const odb::Rect bbox = net->getTermBBox();
      wirelength += block_->dbuToMicrons(bbox.dx() + bbox.dy());
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
    if (!inst->isBlock()) {
      continue;
    }

    const float original_wirelength = calculateRealMacroWirelength(inst);
    odb::Point macro_location = inst->getLocation();

    // Flipping is done by mirroring the macro about the "Y" or "X" axis,
    // so, after flipping, we must manually set the location (lower-left corner)
    // again to move the macro back to the the position choosen by mpl2.
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
  Snapper snapper;

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

void HierRTLMP::setBusPlanningOn(bool bus_planning_on)
{
  bus_planning_on_ = bus_planning_on;
}

void HierRTLMP::setDebug(std::unique_ptr<Mpl2Observer>& graphics)
{
  graphics_ = std::move(graphics);
}

void HierRTLMP::setDebugShowBundledNets(bool show_bundled_nets)
{
  graphics_->setShowBundledNets(show_bundled_nets);
}

void HierRTLMP::setDebugSkipSteps(bool skip_steps)
{
  graphics_->setSkipSteps(skip_steps);
}

void HierRTLMP::setDebugOnlyFinalResult(bool only_final_result)
{
  graphics_->setOnlyFinalResult(only_final_result);
}

odb::Rect HierRTLMP::micronsToDbu(const Rect& micron_rect)
{
  return odb::Rect(block_->micronsToDbu(micron_rect.xMin()),
                   block_->micronsToDbu(micron_rect.yMin()),
                   block_->micronsToDbu(micron_rect.xMax()),
                   block_->micronsToDbu(micron_rect.yMax()));
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
      debugPrint(logger_,
                 MPL,
                 "boundary_push",
                 1,
                 "Overlap found when moving {} to {}. Push reverted.",
                 macro_cluster->getName(),
                 toString(boundary));

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
    return true;
  }

  return false;
}

//////// Snapper ////////

Snapper::Snapper() : inst_(nullptr)
{
}

Snapper::Snapper(odb::dbInst* inst) : inst_(inst)
{
}

void Snapper::snapMacro()
{
  const odb::Point snap_origin = computeSnapOrigin();
  inst_->setOrigin(snap_origin.x(), snap_origin.y());
}

odb::Point Snapper::computeSnapOrigin()
{
  SnapParameters x, y;

  std::set<odb::dbTechLayer*> snap_layers;

  odb::dbMaster* master = inst_->getMaster();
  for (odb::dbMTerm* mterm : master->getMTerms()) {
    if (mterm->getSigType() != odb::dbSigType::SIGNAL) {
      continue;
    }

    for (odb::dbMPin* mpin : mterm->getMPins()) {
      for (odb::dbBox* box : mpin->getGeometry()) {
        odb::dbTechLayer* layer = box->getTechLayer();

        if (snap_layers.find(layer) != snap_layers.end()) {
          continue;
        }

        snap_layers.insert(layer);

        if (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
          y = computeSnapParameters(layer, box, false);
        } else {
          x = computeSnapParameters(layer, box, true);
        }
      }
    }
  }

  // The distance between the pins and the lower-left corner of the master of
  // a macro instance may not be a multiple of the track-grid, in these cases,
  // we need to compensate a small offset.
  const int mterm_offset_x
      = x.pitch != 0
            ? x.lower_left_to_first_pin
                  - std::floor(x.lower_left_to_first_pin / x.pitch) * x.pitch
            : 0;
  const int mterm_offset_y
      = y.pitch != 0
            ? y.lower_left_to_first_pin
                  - std::floor(y.lower_left_to_first_pin / y.pitch) * y.pitch
            : 0;

  int pin_offset_x = x.pin_width / 2 + mterm_offset_x;
  int pin_offset_y = y.pin_width / 2 + mterm_offset_y;

  odb::dbOrientType orientation = inst_->getOrient();

  if (orientation == odb::dbOrientType::MX) {
    pin_offset_y = -pin_offset_y;
  } else if (orientation == odb::dbOrientType::MY) {
    pin_offset_x = -pin_offset_x;
  } else if (orientation == odb::dbOrientType::R180) {
    pin_offset_x = -pin_offset_x;
    pin_offset_y = -pin_offset_y;
  }

  // This may NOT be the lower-left corner.
  int origin_x = inst_->getOrigin().x();
  int origin_y = inst_->getOrigin().y();

  // Compute trackgrid alignment only if there are pins in the grid's direction.
  // Note that we first align the macro origin with the track grid and then, we
  // compensate the necessary offset.
  if (x.pin_width != 0) {
    origin_x = std::round(origin_x / x.pitch) * x.pitch + x.offset;
    origin_x -= pin_offset_x;
  }
  if (y.pin_width != 0) {
    origin_y = std::round(origin_y / y.pitch) * y.pitch + y.offset;
    origin_y -= pin_offset_y;
  }

  const int manufacturing_grid
      = inst_->getDb()->getTech()->getManufacturingGrid();

  origin_x = std::round(origin_x / manufacturing_grid) * manufacturing_grid;
  origin_y = std::round(origin_y / manufacturing_grid) * manufacturing_grid;

  const odb::Point snap_origin(origin_x, origin_y);

  return snap_origin;
}

SnapParameters Snapper::computeSnapParameters(odb::dbTechLayer* layer,
                                              odb::dbBox* box,
                                              const bool vertical_layer)
{
  SnapParameters params;

  odb::dbBlock* block = inst_->getBlock();
  odb::dbTrackGrid* track_grid = block->findTrackGrid(layer);

  if (track_grid) {
    std::vector<int> coordinate_grid;
    getTrackGrid(track_grid, coordinate_grid, vertical_layer);

    params.offset = coordinate_grid[0];
    params.pitch = coordinate_grid[1] - coordinate_grid[0];
  } else {
    params.pitch = getPitch(layer, vertical_layer);
    params.offset = getOffset(layer, vertical_layer);
  }

  params.pin_width = getPinWidth(box, vertical_layer);
  params.lower_left_to_first_pin
      = getPinToLowerLeftDistance(box, vertical_layer);

  return params;
}

int Snapper::getPitch(odb::dbTechLayer* layer, const bool vertical_layer)
{
  int pitch = 0;
  if (vertical_layer) {
    pitch = layer->getPitchX();
  } else {
    pitch = layer->getPitchY();
  }
  return pitch;
}

int Snapper::getOffset(odb::dbTechLayer* layer, const bool vertical_layer)
{
  int offset = 0;
  if (vertical_layer) {
    offset = layer->getOffsetX();
  } else {
    offset = layer->getOffsetY();
  }
  return offset;
}

int Snapper::getPinWidth(odb::dbBox* box, const bool vertical_layer)
{
  int pin_width = 0;
  if (vertical_layer) {
    pin_width = box->getDX();
  } else {
    pin_width = box->getDY();
  }
  return pin_width;
}

void Snapper::getTrackGrid(odb::dbTrackGrid* track_grid,
                           std::vector<int>& coordinate_grid,
                           const bool vertical_layer)
{
  if (vertical_layer) {
    track_grid->getGridX(coordinate_grid);
  } else {
    track_grid->getGridY(coordinate_grid);
  }
}

int Snapper::getPinToLowerLeftDistance(odb::dbBox* box, bool vertical_layer)
{
  int pin_to_origin = 0;
  if (vertical_layer) {
    pin_to_origin = box->getBox().xMin();
  } else {
    pin_to_origin = box->getBox().yMin();
  }
  return pin_to_origin;
}

}  // namespace mpl2
