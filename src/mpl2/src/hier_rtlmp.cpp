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
{
  network_ = network;
  db_ = db;
  sta_ = sta;
  logger_ = logger;
  tritonpart_ = tritonpart;
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
  halo_width_ = halo_width;
}

void HierRTLMP::setHaloHeight(float halo_height)
{
  halo_height_ = halo_height;
}

// Options related to clustering
void HierRTLMP::setNumBundledIOsPerBoundary(int num_bundled_ios)
{
  num_bundled_IOs_ = num_bundled_ios;
}

void HierRTLMP::setClusterSize(int max_num_macro,
                               int min_num_macro,
                               int max_num_inst,
                               int min_num_inst)
{
  max_num_macro_base_ = max_num_macro;
  min_num_macro_base_ = min_num_macro;
  max_num_inst_base_ = max_num_inst;
  min_num_inst_base_ = min_num_inst;
}

void HierRTLMP::setClusterSizeTolerance(float tolerance)
{
  tolerance_ = tolerance;
}

void HierRTLMP::setMaxNumLevel(int max_num_level)
{
  max_num_level_ = max_num_level;
}

void HierRTLMP::setClusterSizeRatioPerLevel(float coarsening_ratio)
{
  coarsening_ratio_ = coarsening_ratio;
}

void HierRTLMP::setLargeNetThreshold(int large_net_threshold)
{
  large_net_threshold_ = large_net_threshold;
}

void HierRTLMP::setSignatureNetThreshold(int signature_net_threshold)
{
  signature_net_threshold_ = signature_net_threshold;
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

// Set defaults for min/max number of instances and macros if not set by user.
void HierRTLMP::setDefaultThresholds()
{
  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Setting default threholds...");

  std::string snap_layer_name;

  for (auto& macro : hard_macro_map_) {
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
  if (dynamic_congestion_weight_flag_ == true) {
    std::vector<std::string> layers;
    int tot_num_layer = 0;
    for (odb::dbTechLayer* layer : db_->getTech()->getLayers()) {
      if (layer->getType() == odb::dbTechLayerType::ROUTING) {
        layers.push_back(layer->getName());
        tot_num_layer++;
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
               "multilevel_autoclustering",
               1,
               "snap_layer : {}  congestion_weight : {}",
               snap_layer_,
               congestion_weight_);
  }

  if (max_num_macro_base_ <= 0 || min_num_macro_base_ <= 0
      || max_num_inst_base_ <= 0 || min_num_inst_base_ <= 0) {
    min_num_inst_base_
        = std::floor(metrics_->getNumStdCell()
                     / std::pow(coarsening_ratio_, max_num_level_));
    if (min_num_inst_base_ <= 1000) {
      min_num_inst_base_ = 1000;  // lower bound
    }
    max_num_inst_base_ = min_num_inst_base_ * coarsening_ratio_ / 2.0;
    min_num_macro_base_ = std::floor(
        metrics_->getNumMacro() / std::pow(coarsening_ratio_, max_num_level_));
    if (min_num_macro_base_ <= 0) {
      min_num_macro_base_ = 1;  // lowerbound
    }
    max_num_macro_base_ = min_num_macro_base_ * coarsening_ratio_ / 2.0;
    if (metrics_->getNumMacro() <= 150) {
      // If the number of macros is less than the threshold value, reset to
      // single level.
      max_num_level_ = 1;
      debugPrint(logger_,
                 MPL,
                 "multilevel_autoclustering",
                 1,
                 "Reset number of levels to 1");
    }
  }

  // for num_level = 1, we recommand to increase the macro_blockage_weight to
  // half of the outline_weight
  if (max_num_level_ == 1) {
    macro_blockage_weight_ = outline_weight_ / 2.0;
    debugPrint(logger_,
               MPL,
               "multilevel_autoclustering",
               1,
               "Reset macro_blockage_weight : {}",
               macro_blockage_weight_);
  }

  // Set sizes for root level based on coarsening_factor and the number of
  // physical hierarchy levels
  unsigned coarsening_factor = std::pow(coarsening_ratio_, max_num_level_ - 1);
  max_num_macro_base_ = max_num_macro_base_ * coarsening_factor;
  min_num_macro_base_ = min_num_macro_base_ * coarsening_factor;
  max_num_inst_base_ = max_num_inst_base_ * coarsening_factor;
  min_num_inst_base_ = min_num_inst_base_ * coarsening_factor;

  debugPrint(
      logger_,
      MPL,
      "multilevel_autoclustering",
      1,
      "num level: {}, max_macro: {}, min_macro: {}, max_inst:{}, min_inst:{}",
      max_num_level_,
      max_num_macro_base_,
      min_num_macro_base_,
      max_num_inst_base_,
      min_num_inst_base_);

  if (logger_->debugCheck(MPL, "multilevel_autoclustering", 1)) {
    logger_->report("\nPrint Default Parameters\n");
    logger_->report("area_weight_ = {}", area_weight_);
    logger_->report("outline_weight_ = {}", outline_weight_);
    logger_->report("wirelength_weight_ = {}", wirelength_weight_);
    logger_->report("guidance_weight_ = {}", guidance_weight_);
    logger_->report("fence_weight_ = {}", fence_weight_);
    logger_->report("boundary_weight_ = {}", boundary_weight_);
    logger_->report("notch_weight_ = {}", notch_weight_);
    logger_->report("macro_blockage_weight_ = {}", macro_blockage_weight_);
    logger_->report("halo_width_ = {}", halo_width_);
    logger_->report("halo_height_ = {}", halo_height_);
    logger_->report("bus_planning_on_ = {}\n", bus_planning_on_);
  }
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
  initMacroPlacer();

  if (!design_has_unfixed_macros_) {
    logger_->info(MPL, 17, "No unfixed macros. Skipping macro placement.");
    return;
  }

  runMultilevelAutoclustering();
  runCoarseShaping();

  if (graphics_) {
    graphics_->startFine();
  }

  if (bus_planning_on_) {
    runHierarchicalMacroPlacement(root_cluster_);
  } else {
    runHierarchicalMacroPlacementWithoutBusPlanning(root_cluster_);
  }

  if (graphics_) {
    graphics_->setMaxLevel(max_num_level_);
    graphics_->drawResult();
  }

  Pusher pusher(logger_, root_cluster_, block_, boundary_to_io_blockage_);
  pusher.pushMacrosToCoreBoundaries();

  updateMacrosOnDb();

  generateTemporaryStdCellsPlacement(root_cluster_);
  correctAllMacrosOrientation();

  commitMacroPlacementToDb();

  writeMacroPlacement(macro_placement_file_);
  clear();
}

////////////////////////////////////////////////////////////////////////
// Private functions
////////////////////////////////////////////////////////////////////////

void HierRTLMP::initMacroPlacer()
{
  block_ = db_->getChip()->getBlock();

  odb::Rect die = block_->getDieArea();
  odb::Rect core_box = block_->getCoreArea();

  float core_lx = block_->dbuToMicrons(core_box.xMin());
  float core_ly = block_->dbuToMicrons(core_box.yMin());
  float core_ux = block_->dbuToMicrons(core_box.xMax());
  float core_uy = block_->dbuToMicrons(core_box.yMax());

  logger_->report(
      "Floorplan Outline: ({}, {}) ({}, {}),  Core Outline: ({}, {}) ({}, {})",
      block_->dbuToMicrons(die.xMin()),
      block_->dbuToMicrons(die.yMin()),
      block_->dbuToMicrons(die.xMax()),
      block_->dbuToMicrons(die.yMax()),
      core_lx,
      core_ly,
      core_ux,
      core_uy);

  float core_area = (core_ux - core_lx) * (core_uy - core_ly);

  computeMetricsForModules(core_area);
}

void HierRTLMP::computeMetricsForModules(float core_area)
{
  metrics_ = computeMetrics(block_->getTopModule());

  float util
      = (metrics_->getStdCellArea() + metrics_->getMacroArea()) / core_area;
  float core_util
      = metrics_->getStdCellArea() / (core_area - metrics_->getMacroArea());

  // Check if placement is feasible in the core area when considering
  // the macro halos
  int unfixed_macros = 0;
  for (auto inst : block_->getInsts()) {
    auto master = inst->getMaster();
    if (master->isBlock()) {
      const auto width
          = block_->dbuToMicrons(master->getWidth()) + 2 * halo_width_;
      const auto height
          = block_->dbuToMicrons(master->getHeight()) + 2 * halo_width_;
      macro_with_halo_area_ += width * height;
      unfixed_macros += !inst->getPlacementStatus().isFixed();
    }
  }
  reportLogicalHierarchyInformation(core_area, util, core_util);

  if (unfixed_macros == 0) {
    design_has_unfixed_macros_ = false;
    return;
  }

  if (macro_with_halo_area_ + metrics_->getStdCellArea() > core_area) {
    logger_->error(MPL,
                   16,
                   "The instance area with halos {} exceeds the core area {}",
                   macro_with_halo_area_ + metrics_->getStdCellArea(),
                   core_area);
  }
}

void HierRTLMP::reportLogicalHierarchyInformation(float core_area,
                                                  float util,
                                                  float core_util)
{
  logger_->report(
      "Traversed logical hierarchy\n"
      "\tNumber of std cell instances: {}\n"
      "\tArea of std cell instances: {:.2f}\n"
      "\tNumber of macros: {}\n"
      "\tArea of macros: {:.2f}\n"
      "\tArea of macros with halos: {:.2f}\n"
      "\tArea of std cell instances + Area of macros: {:.2f}\n"
      "\tCore area: {:.2f}\n"
      "\tDesign Utilization: {:.2f}\n"
      "\tCore Utilization: {:.2f}\n"
      "\tManufacturing Grid: {}\n",
      metrics_->getNumStdCell(),
      metrics_->getStdCellArea(),
      metrics_->getNumMacro(),
      metrics_->getMacroArea(),
      macro_with_halo_area_,
      metrics_->getStdCellArea() + metrics_->getMacroArea(),
      core_area,
      util,
      core_util,
      block_->getTech()->getManufacturingGrid());
}

void HierRTLMP::initPhysicalHierarchy()
{
  setDefaultThresholds();

  cluster_id_ = 0;

  root_cluster_ = new Cluster(cluster_id_, std::string("root"), logger_);
  root_cluster_->addDbModule(block_->getTopModule());
  root_cluster_->setMetrics(*metrics_);

  cluster_map_[cluster_id_++] = root_cluster_;

  // Associate all instances to root
  for (odb::dbInst* inst : block_->getInsts()) {
    inst_to_cluster_[inst] = cluster_id_;
  }
}

// Transform the logical hierarchy into a physical hierarchy.
void HierRTLMP::runMultilevelAutoclustering()
{
  initPhysicalHierarchy();

  createIOClusters();
  createDataFlow();

  if (metrics_->getNumStdCell() == 0) {
    logger_->warn(MPL, 25, "Design has no standard cells!");

    treatEachMacroAsSingleCluster();
    resetSAParameters();

  } else {
    multilevelAutocluster(root_cluster_);

    std::vector<std::vector<Cluster*>> mixed_leaves;
    fetchMixedLeaves(root_cluster_, mixed_leaves);
    breakMixedLeaves(mixed_leaves);

    if (graphics_) {
      graphics_->finishedClustering(root_cluster_);
    }
  }

  if (logger_->debugCheck(MPL, "multilevel_autoclustering", 1)) {
    logger_->report("\nPrint Physical Hierarchy\n");
    printPhysicalHierarchyTree(root_cluster_, 0);
  }

  // Map the macros in each cluster to their HardMacro objects
  for (auto& [cluster_id, cluster] : cluster_map_) {
    mapMacroInCluster2HardMacro(cluster);
  }
}

/* static */
bool HierRTLMP::isIgnoredMaster(odb::dbMaster* master)
{
  // IO corners are sometimes marked as end caps
  return master->isPad() || master->isCover() || master->isEndCap();
}

void HierRTLMP::treatEachMacroAsSingleCluster()
{
  auto module = block_->getTopModule();
  for (odb::dbInst* inst : module->getInsts()) {
    odb::dbMaster* master = inst->getMaster();

    if (isIgnoredMaster(master)) {
      continue;
    }

    if (master->isBlock()) {
      std::string cluster_name = inst->getName();
      Cluster* cluster = new Cluster(cluster_id_, cluster_name, logger_);
      cluster->addLeafMacro(inst);
      incorporateNewClusterToTree(cluster, root_cluster_);
      cluster->setClusterType(HardMacroCluster);

      debugPrint(logger_,
                 MPL,
                 "multilevel_autoclustering",
                 1,
                 "model {} as a cluster.",
                 cluster_name);
    }

    if (!design_has_io_clusters_) {
      design_has_only_macros_ = true;
    }
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

  if (design_has_only_macros_) {
    logger_->warn(MPL, 27, "Design has only macros!");
    root_cluster_->setClusterType(HardMacroCluster);
    return;
  }

  if (graphics_) {
    graphics_->startCoarse();
  }

  calculateChildrenTilings(root_cluster_);

  setIOClustersBlockages();
  setPlacementBlockages();
}

void HierRTLMP::setRootShapes()
{
  SoftMacro* root_soft_macro = new SoftMacro(root_cluster_);

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
  root_cluster_->setSoftMacro(root_soft_macro);
}

// Traverse Logical Hierarchy
// Recursive function to collect the design metrics (number of std cells,
// area of std cells, number of macros and area of macros) in the logical
// hierarchy
Metrics* HierRTLMP::computeMetrics(odb::dbModule* module)
{
  unsigned int num_std_cell = 0;
  float std_cell_area = 0.0;
  unsigned int num_macro = 0;
  float macro_area = 0.0;

  for (odb::dbInst* inst : module->getInsts()) {
    odb::dbMaster* master = inst->getMaster();

    if (isIgnoredMaster(master)) {
      continue;
    }

    float inst_area = computeMicronArea(inst);

    if (master->isBlock()) {  // a macro
      num_macro += 1;
      macro_area += inst_area;

      // add hard macro to corresponding map
      HardMacro* macro = new HardMacro(inst, halo_width_, halo_height_);
      hard_macro_map_[inst] = macro;
    } else {
      num_std_cell += 1;
      std_cell_area += inst_area;
    }
  }

  // Be careful about the relationship between
  // odb::dbModule and odb::dbInst
  // odb::dbModule and odb::dbModInst
  // recursively traverse the hierarchical module instances
  for (odb::dbModInst* inst : module->getChildren()) {
    Metrics* metrics = computeMetrics(inst->getMaster());
    num_std_cell += metrics->getNumStdCell();
    std_cell_area += metrics->getStdCellArea();
    num_macro += metrics->getNumMacro();
    macro_area += metrics->getMacroArea();
  }

  Metrics* metrics
      = new Metrics(num_std_cell, num_macro, std_cell_area, macro_area);

  logical_module_map_[module] = metrics;

  return metrics;
}

float HierRTLMP::computeMicronArea(odb::dbInst* inst)
{
  const float width = static_cast<float>(
      block_->dbuToMicrons(inst->getBBox()->getBox().dx()));
  const float height = static_cast<float>(
      block_->dbuToMicrons(inst->getBBox()->getBox().dy()));

  return width * height;
}

void HierRTLMP::setClusterMetrics(Cluster* cluster)
{
  float std_cell_area = 0.0f;
  for (odb::dbInst* std_cell : cluster->getLeafStdCells()) {
    std_cell_area += computeMicronArea(std_cell);
  }

  float macro_area = 0.0f;
  for (odb::dbInst* macro : cluster->getLeafMacros()) {
    macro_area += computeMicronArea(macro);
  }

  const unsigned int num_std_cell = cluster->getLeafStdCells().size();
  const unsigned int num_macro = cluster->getLeafMacros().size();

  Metrics metrics(num_std_cell, num_macro, std_cell_area, macro_area);

  for (auto& module : cluster->getDbModules()) {
    metrics.addMetrics(*logical_module_map_[module]);
  }

  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Setting Cluster Metrics for {}: Num Macros: {} Num Std Cells: {}",
             cluster->getName(),
             metrics.getNumMacro(),
             metrics.getNumStdCell());

  if (cluster->getClusterType() == HardMacroCluster) {
    cluster->setMetrics(
        Metrics(0, metrics.getNumMacro(), 0.0, metrics.getMacroArea()));
  } else if (cluster->getClusterType() == StdCellCluster) {
    cluster->setMetrics(
        Metrics(metrics.getNumStdCell(), 0, metrics.getStdCellArea(), 0.0));
  } else {
    cluster->setMetrics(metrics);
  }
}

// Handle IOs: Map IOs to Pads for designs with IO pads
void HierRTLMP::mapIOPads()
{
  // Check if this design has IO pads
  bool is_pad_design = false;
  for (auto inst : block_->getInsts()) {
    if (inst->getMaster()->isPad()) {
      is_pad_design = true;
      break;
    }
  }

  if (!is_pad_design) {
    return;
  }

  for (odb::dbNet* net : block_->getNets()) {
    if (net->getBTerms().size() == 0) {
      continue;
    }

    //
    // If the design has IO pads, there is a net
    // connecting the IO pin and IO pad instance
    //
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      for (odb::dbITerm* iterm : net->getITerms()) {
        odb::dbInst* inst = iterm->getInst();
        io_pad_map_[bterm] = inst;
      }
    }
  }
}

// Model IO pins as bundled IO clusters under the root node.
// IO clusters are created in the following the order : L, T, R, B.
// We will have num_bundled_IOs_ x 4 clusters for bundled IOs.
// Bundled IOs are only created for pins in the region.
void HierRTLMP::createIOClusters()
{
  mapIOPads();

  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Creating bundledIO clusters...");

  const odb::Rect die = block_->getDieArea();

  // Get the floorplan information and get the range of bundled IO regions
  odb::Rect die_box = block_->getCoreArea();
  int core_lx = die_box.xMin();
  int core_ly = die_box.yMin();
  int core_ux = die_box.xMax();
  int core_uy = die_box.yMax();
  const int x_base = (die.xMax() - die.xMin()) / num_bundled_IOs_;
  const int y_base = (die.yMax() - die.yMin()) / num_bundled_IOs_;
  int cluster_id_base = cluster_id_;

  // Map all the BTerms / Pads to Bundled IOs (cluster)
  std::vector<std::string> prefix_vec;
  prefix_vec.emplace_back("L");
  prefix_vec.emplace_back("T");
  prefix_vec.emplace_back("R");
  prefix_vec.emplace_back("B");
  std::map<int, bool> cluster_io_map;
  for (int i = 0; i < 4;
       i++) {  // four boundaries (Left, Top, Right and Bottom in order)
    for (int j = 0; j < num_bundled_IOs_; j++) {
      std::string cluster_name = prefix_vec[i] + std::to_string(j);
      Cluster* cluster = new Cluster(cluster_id_, cluster_name, logger_);
      root_cluster_->addChild(cluster);
      cluster->setParent(root_cluster_);
      cluster_io_map[cluster_id_] = false;
      cluster_map_[cluster_id_++] = cluster;
      int x = 0.0;
      int y = 0.0;
      int width = 0;
      int height = 0;
      if (i == 0) {  // Left boundary
        x = die.xMin();
        y = die.yMin() + y_base * j;
        height = y_base;
      } else if (i == 1) {  // Top boundary
        x = die.xMin() + x_base * j;
        y = die.yMax();
        width = x_base;
      } else if (i == 2) {  // Right boundary
        x = die.xMax();
        y = die.yMax() - y_base * (j + 1);
        height = y_base;
      } else {  // Bottom boundary
        x = die.xMax() - x_base * (j + 1);
        y = die.yMin();
        width = x_base;
      }

      // set the cluster to a IO cluster
      cluster->setAsIOCluster(std::pair<float, float>(block_->dbuToMicrons(x),
                                                      block_->dbuToMicrons(y)),
                              block_->dbuToMicrons(width),
                              block_->dbuToMicrons(height));
    }
  }

  // Map all the BTerms to bundled IOs
  for (auto term : block_->getBTerms()) {
    int lx = std::numeric_limits<int>::max();
    int ly = std::numeric_limits<int>::max();
    int ux = 0;
    int uy = 0;
    // If the design has IO pads, these block terms
    // will not have block pins.
    // Otherwise, the design will have IO pins.
    for (const auto pin : term->getBPins()) {
      for (const auto box : pin->getBoxes()) {
        lx = std::min(lx, box->xMin());
        ly = std::min(ly, box->yMin());
        ux = std::max(ux, box->xMax());
        uy = std::max(uy, box->yMax());
      }
    }
    // remove power pins
    if (term->getSigType().isSupply()) {
      continue;
    }

    // If the term has a connected pad, get the bbox from the pad inst
    if (io_pad_map_.find(term) != io_pad_map_.end()) {
      lx = io_pad_map_[term]->getBBox()->xMin();
      ly = io_pad_map_[term]->getBBox()->yMin();
      ux = io_pad_map_[term]->getBBox()->xMax();
      uy = io_pad_map_[term]->getBBox()->yMax();
      if (lx <= core_lx) {
        lx = die.xMin();
      }
      if (ly <= core_ly) {
        ly = die.yMin();
      }
      if (ux >= core_ux) {
        ux = die.xMax();
      }
      if (uy >= core_uy) {
        uy = die.yMax();
      }
    }
    // calculate cluster id based on the location of IO Pins / Pads
    int cluster_id = -1;
    if (lx <= die.xMin()) {
      // The IO is on the left boundary
      cluster_id = cluster_id_base
                   + std::floor(((ly + uy) / 2.0 - die.yMin()) / y_base);
    } else if (uy >= die.yMax()) {
      // The IO is on the top boundary
      cluster_id = cluster_id_base + num_bundled_IOs_
                   + std::floor(((lx + ux) / 2.0 - die.xMin()) / x_base);
    } else if (ux >= die.xMax()) {
      // The IO is on the right boundary
      cluster_id = cluster_id_base + num_bundled_IOs_ * 2
                   + std::floor((die.yMax() - (ly + uy) / 2.0) / y_base);
    } else if (ly <= die.yMin()) {
      // The IO is on the bottom boundary
      cluster_id = cluster_id_base + num_bundled_IOs_ * 3
                   + std::floor((die.xMax() - (lx + ux) / 2.0) / x_base);
    }

    // Check if the IO pins / Pads exist
    if (cluster_id == -1) {
      logger_->error(
          MPL,
          2,
          "Floorplan has not been initialized? Pin location error for {}.",
          term->getName());
    } else {
      bterm_to_cluster_[term] = cluster_id;
    }

    cluster_io_map[cluster_id] = true;
  }

  // delete the IO clusters that do not have any pins assigned to them
  for (auto& [cluster_id, flag] : cluster_io_map) {
    if (!flag) {
      debugPrint(logger_,
                 MPL,
                 "multilevel_autoclustering",
                 1,
                 "Remove IO Cluster with no pins: {}, id: {}",
                 cluster_map_[cluster_id]->getName(),
                 cluster_id);
      cluster_map_[cluster_id]->getParent()->removeChild(
          cluster_map_[cluster_id]);
      delete cluster_map_[cluster_id];
      cluster_map_.erase(cluster_id);
    }
  }

  // At this point the cluster map has only the root (id = 0) and bundledIOs
  if (cluster_map_.size() == 1) {
    logger_->warn(MPL, 26, "Design has no IO pins!");
    design_has_io_clusters_ = false;
  }
}

// Create physical hierarchy tree in a post-order DFS manner
// Recursive call for creating the physical hierarchy tree
void HierRTLMP::multilevelAutocluster(Cluster* parent)
{
  bool force_split_root = false;
  if (level_ == 0) {
    const int leaf_max_std_cell
        = max_num_inst_base_ / std::pow(coarsening_ratio_, max_num_level_ - 1)
          * (1 + tolerance_);
    if (parent->getNumStdCell() < leaf_max_std_cell) {
      force_split_root = true;
      debugPrint(logger_,
                 MPL,
                 "multilevel_autoclustering",
                 1,
                 "Root number of std cells ({}) is below leaf cluster max "
                 "({}). Root will be force split.",
                 parent->getNumStdCell(),
                 leaf_max_std_cell);
    }
  }

  if (level_ >= max_num_level_) {
    return;
  }
  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Current cluster: {} - Level: {} - Macros: {} - Std Cells: {}",
             parent->getName(),
             level_,
             parent->getNumMacro(),
             parent->getNumStdCell());

  level_++;
  updateSizeThresholds();

  if (force_split_root || (parent->getNumStdCell() > max_num_inst_)) {
    breakCluster(parent);
    updateSubTree(parent);

    for (auto& child : parent->getChildren()) {
      updateInstancesAssociation(child);
    }

    for (auto& child : parent->getChildren()) {
      debugPrint(logger_,
                 MPL,
                 "multilevel_autoclustering",
                 1,
                 "\tChild Cluster: {}",
                 child->getName());
      multilevelAutocluster(child);
    }
  } else {
    multilevelAutocluster(parent);
  }

  updateInstancesAssociation(parent);
  level_--;
}

void HierRTLMP::updateSizeThresholds()
{
  const double coarse_factor = std::pow(coarsening_ratio_, level_ - 1);

  // a large coarsening_ratio_ helps the clustering process converge fast
  max_num_macro_ = max_num_macro_base_ / coarse_factor;
  min_num_macro_ = min_num_macro_base_ / coarse_factor;
  max_num_inst_ = max_num_inst_base_ / coarse_factor;
  min_num_inst_ = min_num_inst_base_ / coarse_factor;

  // We define the tolerance to improve the robustness of our hierarchical
  // clustering
  max_num_inst_ *= (1 + tolerance_);
  min_num_inst_ *= (1 - tolerance_);
  max_num_macro_ *= (1 + tolerance_);
  min_num_macro_ *= (1 - tolerance_);

  if (min_num_macro_ <= 0) {
    min_num_macro_ = 1;
    max_num_macro_ = min_num_macro_ * coarsening_ratio_ / 2.0;
  }

  if (min_num_inst_ <= 0) {
    min_num_inst_ = 100;
    max_num_inst_ = min_num_inst_ * coarsening_ratio_ / 2.0;
  }
}

void HierRTLMP::updateInstancesAssociation(Cluster* cluster)
{
  int cluster_id = cluster->getId();
  ClusterType cluster_type = cluster->getClusterType();
  if (cluster_type == HardMacroCluster || cluster_type == MixedCluster) {
    for (auto& inst : cluster->getLeafMacros()) {
      inst_to_cluster_[inst] = cluster_id;
    }
  }

  if (cluster_type == StdCellCluster || cluster_type == MixedCluster) {
    for (auto& inst : cluster->getLeafStdCells()) {
      inst_to_cluster_[inst] = cluster_id;
    }
  }

  // Note: macro clusters have no module.
  if (cluster_type == StdCellCluster) {
    for (auto& module : cluster->getDbModules()) {
      updateInstancesAssociation(module, cluster_id, false);
    }
  } else if (cluster_type == MixedCluster) {
    for (auto& module : cluster->getDbModules()) {
      updateInstancesAssociation(module, cluster_id, true);
    }
  }
}

// Unlike macros, std cells are always considered when when updating
// the inst -> cluster map with the data from a module.
void HierRTLMP::updateInstancesAssociation(odb::dbModule* module,
                                           int cluster_id,
                                           bool include_macro)
{
  if (include_macro) {
    for (odb::dbInst* inst : module->getInsts()) {
      inst_to_cluster_[inst] = cluster_id;
    }
  } else {  // only consider standard cells
    for (odb::dbInst* inst : module->getInsts()) {
      odb::dbMaster* master = inst->getMaster();

      if (isIgnoredMaster(master) || master->isBlock()) {
        continue;
      }

      inst_to_cluster_[inst] = cluster_id;
    }
  }
  for (odb::dbModInst* inst : module->getChildren()) {
    updateInstancesAssociation(inst->getMaster(), cluster_id, include_macro);
  }
}

// We expand the parent cluster into a subtree based on logical
// hierarchy in a DFS manner.  During the expansion process,
// we merge small clusters in the same logical hierarchy
void HierRTLMP::breakCluster(Cluster* parent)
{
  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Breaking Cluster: {}",
             parent->getName());

  if (parent->isEmpty()) {
    return;
  }

  if (parent->correspondsToLogicalModule()) {
    odb::dbModule* module = parent->getDbModules().front();
    // Flat module that will be partitioned with TritonPart when updating
    // the subtree later on.
    if (module->getChildren().size() == 0) {
      if (parent == root_cluster_) {
        createFlatCluster(module, parent);
      } else {
        addModuleInstsToCluster(parent, module);
        parent->clearDbModules();
        updateInstancesAssociation(parent);
      }
      return;
    }

    for (odb::dbModInst* child : module->getChildren()) {
      createCluster(child->getMaster(), parent);
    }
    createFlatCluster(module, parent);
  } else {
    // Parent is a cluster generated by merging small clusters:
    // It may have a few logical modules or many glue insts.
    for (auto& module : parent->getDbModules()) {
      createCluster(module, parent);
    }

    if (!parent->getLeafStdCells().empty()
        || !parent->getLeafMacros().empty()) {
      createCluster(parent);
    }
  }

  // Recursively break down non-flat large clusters with logical modules
  for (auto& child : parent->getChildren()) {
    if (!child->getDbModules().empty()) {
      if (child->getNumStdCell() > max_num_inst_
          || child->getNumMacro() > max_num_macro_) {
        breakCluster(child);
      }
    }
  }

  // Merge small clusters
  std::vector<Cluster*> candidate_clusters;
  for (auto& cluster : parent->getChildren()) {
    if (!cluster->isIOCluster() && cluster->getNumStdCell() < min_num_inst_
        && cluster->getNumMacro() < min_num_macro_) {
      candidate_clusters.push_back(cluster);
    }
  }

  mergeClusters(candidate_clusters);

  // Update the cluster_id
  // This is important to maintain the clustering results
  updateInstancesAssociation(parent);
}

// This cluster won't be associated with the module. It will only
// contain its macros and std cells as leaves.
void HierRTLMP::createFlatCluster(odb::dbModule* module, Cluster* parent)
{
  std::string cluster_name
      = std::string("(") + parent->getName() + ")_glue_logic";
  Cluster* cluster = new Cluster(cluster_id_, cluster_name, logger_);
  addModuleInstsToCluster(cluster, module);

  if (cluster->getLeafStdCells().empty() && cluster->getLeafMacros().empty()) {
    delete cluster;
    cluster = nullptr;
  } else {
    incorporateNewClusterToTree(cluster, parent);
  }
}

void HierRTLMP::createCluster(Cluster* parent)
{
  std::string cluster_name
      = std::string("(") + parent->getName() + ")_glue_logic";
  Cluster* cluster = new Cluster(cluster_id_, cluster_name, logger_);
  for (auto& inst : parent->getLeafStdCells()) {
    cluster->addLeafStdCell(inst);
  }
  for (auto& inst : parent->getLeafMacros()) {
    cluster->addLeafMacro(inst);
  }

  incorporateNewClusterToTree(cluster, parent);
}

void HierRTLMP::createCluster(odb::dbModule* module, Cluster* parent)
{
  std::string cluster_name = module->getHierarchicalName();
  Cluster* cluster = new Cluster(cluster_id_, cluster_name, logger_);
  cluster->addDbModule(module);
  incorporateNewClusterToTree(cluster, parent);
}

void HierRTLMP::addModuleInstsToCluster(Cluster* cluster, odb::dbModule* module)
{
  for (odb::dbInst* inst : module->getInsts()) {
    odb::dbMaster* master = inst->getMaster();
    if (isIgnoredMaster(master)) {
      continue;
    }
    cluster->addLeafInst(inst);
  }
}

void HierRTLMP::incorporateNewClusterToTree(Cluster* cluster, Cluster* parent)
{
  updateInstancesAssociation(cluster);
  setClusterMetrics(cluster);
  cluster_map_[cluster_id_++] = cluster;

  // modify physical hierarchy
  cluster->setParent(parent);
  parent->addChild(cluster);
}

// Merge small clusters with the same parent cluster
// Recursively merge clusters
// Here is an example process based on connection signature
// Iter1 :  A, B, C, D, E, F
// Iter2 :  A + C,  B + D,  E, F
// Iter3 :  A + C + F, B + D, E
// End if there is no same connection signature
// During the merging process, we support two types of merging
// Type 1: merging small clusters to their closely connected clusters
//         For example, if a small cluster A is closely connected to a
//         well-formed cluster B, (there are also other well-formed clusters
//         C, D), A is only connected to B and A has no connection with C, D
// Type 2: merging small clusters with the same connection signature
//         For example, if we merge small clusters A and B,  A and B will have
//         exactly the same connections relative to all other clusters (both
//         small clusters and well-formed clusters). In this case, if A and B
//         have the same connection signature, A and C have the same connection
//         signature, then B and C also have the same connection signature.
// Note in both types, we only merge clusters with the same parent cluster
void HierRTLMP::mergeClusters(std::vector<Cluster*>& candidate_clusters)
{
  if (candidate_clusters.empty()) {
    return;
  }

  int merge_iter = 0;
  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Merge Cluster Iter: {}",
             merge_iter++);
  for (auto& cluster : candidate_clusters) {
    debugPrint(logger_,
               MPL,
               "multilevel_autoclustering",
               1,
               "Cluster: {}, num std cell: {}, num macros: {}",
               cluster->getName(),
               cluster->getNumStdCell(),
               cluster->getNumMacro());
  }

  int num_candidate_clusters = candidate_clusters.size();
  while (true) {
    calculateConnection();  // update the connections between clusters

    std::vector<int> cluster_class(num_candidate_clusters, -1);  // merge flag
    std::vector<int> candidate_clusters_id;  // store cluster id
    candidate_clusters_id.reserve(candidate_clusters.size());
    for (auto& cluster : candidate_clusters) {
      candidate_clusters_id.push_back(cluster->getId());
    }
    // Firstly we perform Type 1 merge
    for (int i = 0; i < num_candidate_clusters; i++) {
      const int cluster_id = candidate_clusters[i]->getCloseCluster(
          candidate_clusters_id, signature_net_threshold_);
      debugPrint(
          logger_,
          MPL,
          "multilevel_autoclustering",
          1,
          "Candidate cluster: {} - {}",
          candidate_clusters[i]->getName(),
          (cluster_id != -1 ? cluster_map_[cluster_id]->getName() : "   "));
      if (cluster_id != -1 && !cluster_map_[cluster_id]->isIOCluster()) {
        Cluster*& cluster = cluster_map_[cluster_id];
        bool delete_flag = false;
        if (cluster->mergeCluster(*candidate_clusters[i], delete_flag)) {
          if (delete_flag) {
            cluster_map_.erase(candidate_clusters[i]->getId());
            delete candidate_clusters[i];
          }
          updateInstancesAssociation(cluster);
          setClusterMetrics(cluster);
          cluster_class[i] = cluster->getId();
        }
      }
    }

    // Then we perform Type 2 merge
    std::vector<Cluster*> new_candidate_clusters;
    for (int i = 0; i < num_candidate_clusters; i++) {
      if (cluster_class[i] == -1) {  // the cluster has not been merged
        // new_candidate_clusters.push_back(candidate_clusters[i]);
        for (int j = i + 1; j < num_candidate_clusters; j++) {
          if (cluster_class[j] != -1) {
            continue;
          }
          bool flag = candidate_clusters[i]->isSameConnSignature(
              *candidate_clusters[j], signature_net_threshold_);
          if (flag) {
            cluster_class[j] = i;
            bool delete_flag = false;
            if (candidate_clusters[i]->mergeCluster(*candidate_clusters[j],
                                                    delete_flag)) {
              if (delete_flag) {
                cluster_map_.erase(candidate_clusters[j]->getId());
                delete candidate_clusters[j];
              }
              updateInstancesAssociation(candidate_clusters[i]);
              setClusterMetrics(candidate_clusters[i]);
            }
          }
        }
      }
    }

    // Then we perform Type 3 merge:  merge all dust cluster
    const int dust_cluster_std_cell = 10;
    for (int i = 0; i < num_candidate_clusters; i++) {
      if (cluster_class[i] == -1) {  // the cluster has not been merged
        new_candidate_clusters.push_back(candidate_clusters[i]);
        if (candidate_clusters[i]->getNumStdCell() <= dust_cluster_std_cell
            && candidate_clusters[i]->getNumMacro() == 0) {
          for (int j = i + 1; j < num_candidate_clusters; j++) {
            if (cluster_class[j] != -1
                || candidate_clusters[j]->getNumMacro() > 0
                || candidate_clusters[j]->getNumStdCell()
                       > dust_cluster_std_cell) {
              continue;
            }
            cluster_class[j] = i;
            bool delete_flag = false;
            if (candidate_clusters[i]->mergeCluster(*candidate_clusters[j],
                                                    delete_flag)) {
              if (delete_flag) {
                cluster_map_.erase(candidate_clusters[j]->getId());
                delete candidate_clusters[j];
              }
              updateInstancesAssociation(candidate_clusters[i]);
              setClusterMetrics(candidate_clusters[i]);
            }
          }
        }
      }
    }

    // Update the candidate clusters
    // Some clusters have become well-formed clusters
    candidate_clusters.clear();
    for (auto& cluster : new_candidate_clusters) {
      if (cluster->getNumStdCell() < min_num_inst_
          && cluster->getNumMacro() < min_num_macro_) {
        candidate_clusters.push_back(cluster);
      }
    }

    // If no more clusters have been merged, exit the merging loop
    if (num_candidate_clusters == new_candidate_clusters.size()) {
      break;
    }

    num_candidate_clusters = candidate_clusters.size();

    debugPrint(logger_,
               MPL,
               "multilevel_autoclustering",
               1,
               "Merge Cluster Iter: {}",
               merge_iter++);
    for (auto& cluster : candidate_clusters) {
      debugPrint(logger_,
                 MPL,
                 "multilevel_autoclustering",
                 1,
                 "Cluster: {}",
                 cluster->getName());
    }
    // merge small clusters
    if (candidate_clusters.empty()) {
      break;
    }
  }
  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Finished merging clusters");
}

void HierRTLMP::calculateConnection()
{
  for (auto& [cluster_id, cluster] : cluster_map_) {
    cluster->initConnection();
  }

  for (odb::dbNet* net : block_->getNets()) {
    if (net->getSigType().isSupply()) {
      continue;
    }

    int driver_cluster_id = -1;
    std::vector<int> load_clusters_ids;
    bool net_has_pad_or_cover = false;

    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      odb::dbMaster* master = inst->getMaster();

      if (isIgnoredMaster(master)) {
        net_has_pad_or_cover = true;
        break;
      }

      const int cluster_id = inst_to_cluster_.at(inst);

      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        driver_cluster_id = cluster_id;
      } else {
        load_clusters_ids.push_back(cluster_id);
      }
    }

    if (net_has_pad_or_cover) {
      continue;
    }

    bool net_has_io_pin = false;

    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const int cluster_id = bterm_to_cluster_.at(bterm);
      net_has_io_pin = true;

      if (bterm->getIoType() == odb::dbIoType::INPUT) {
        driver_cluster_id = cluster_id;
      } else {
        load_clusters_ids.push_back(cluster_id);
      }
    }

    if (driver_cluster_id != -1 && !load_clusters_ids.empty()
        && load_clusters_ids.size() < large_net_threshold_) {
      const float weight = net_has_io_pin ? virtual_weight_ : 1.0;

      for (const int load_cluster_id : load_clusters_ids) {
        if (load_cluster_id != driver_cluster_id) { /* undirected connection */
          cluster_map_[driver_cluster_id]->addConnection(load_cluster_id,
                                                         weight);
          cluster_map_[load_cluster_id]->addConnection(driver_cluster_id,
                                                       weight);
        }
      }
    }
  }
}

// Dataflow is used to improve quality of macro placement.
// Here we model each std cell instance, IO pin and macro pin as vertices.
void HierRTLMP::createDataFlow()
{
  debugPrint(
      logger_, MPL, "multilevel_autoclustering", 1, "Creating dataflow...");
  if (max_num_ff_dist_ <= 0) {
    return;
  }
  // create vertex id property for std cell, IO pin and macro pin
  std::map<int, odb::dbBTerm*> io_pin_vertex;
  std::map<int, odb::dbInst*> std_cell_vertex;
  std::map<int, odb::dbITerm*> macro_pin_vertex;

  std::vector<bool> stop_flag_vec;
  // assign vertex_id property of each Bterm
  // All boundary terms are marked as sequential stopping pts
  for (odb::dbBTerm* term : block_->getBTerms()) {
    odb::dbIntProperty::create(term, "vertex_id", stop_flag_vec.size());
    io_pin_vertex[stop_flag_vec.size()] = term;
    stop_flag_vec.push_back(true);
  }

  // assign vertex_id property of each instance
  for (auto inst : block_->getInsts()) {
    odb::dbMaster* master = inst->getMaster();
    if (isIgnoredMaster(master) || master->isBlock()) {
      continue;
    }

    const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
    if (!liberty_cell) {
      continue;
    }

    // Mark registers
    odb::dbIntProperty::create(inst, "vertex_id", stop_flag_vec.size());
    std_cell_vertex[stop_flag_vec.size()] = inst;

    if (liberty_cell->hasSequentials()) {
      stop_flag_vec.push_back(true);
    } else {
      stop_flag_vec.push_back(false);
    }
  }
  // assign vertex_id property of each macro pin
  // all macro pins are flagged as sequential stopping pt
  for (auto& [macro, hard_macro] : hard_macro_map_) {
    for (odb::dbITerm* pin : macro->getITerms()) {
      if (pin->getSigType() != odb::dbSigType::SIGNAL) {
        continue;
      }
      odb::dbIntProperty::create(pin, "vertex_id", stop_flag_vec.size());
      macro_pin_vertex[stop_flag_vec.size()] = pin;
      stop_flag_vec.push_back(true);
    }
  }

  //
  // Num of vertices will be # of boundary pins + number of logical std cells +
  // number of macro pins)
  //
  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Number of vertices: {}",
             stop_flag_vec.size());

  // create hypergraphs
  std::vector<std::vector<int>> vertices(stop_flag_vec.size());
  std::vector<std::vector<int>> backward_vertices(stop_flag_vec.size());
  std::vector<std::vector<int>> hyperedges;  // dircted hypergraph
  // traverse the netlist
  for (odb::dbNet* net : block_->getNets()) {
    // ignore all the power net
    if (net->getSigType().isSupply()) {
      continue;
    }
    int driver_id = -1;      // driver vertex id
    std::set<int> loads_id;  // load vertex id
    bool ignore = false;
    // check the connected instances
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      odb::dbMaster* master = inst->getMaster();
      // We ignore nets connecting ignored masters
      if (isIgnoredMaster(master)) {
        ignore = true;
        break;
      }
      int vertex_id = -1;
      if (master->isBlock()) {
        vertex_id = odb::dbIntProperty::find(iterm, "vertex_id")->getValue();
      } else {
        vertex_id = odb::dbIntProperty::find(inst, "vertex_id")->getValue();
      }
      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        driver_id = vertex_id;
      } else {
        loads_id.insert(vertex_id);
      }
    }
    if (ignore) {
      continue;  // the nets with Pads should be ignored
    }

    // check the connected IO pins  of the net
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const int vertex_id
          = odb::dbIntProperty::find(bterm, "vertex_id")->getValue();
      if (bterm->getIoType() == odb::dbIoType::INPUT) {
        driver_id = vertex_id;
      } else {
        loads_id.insert(vertex_id);
      }
    }

    //
    // Skip high fanout nets or nets that do not have valid driver or loads
    //
    if (driver_id < 0 || loads_id.empty()
        || loads_id.size() > large_net_threshold_) {
      continue;
    }

    // Create the hyperedge
    std::vector<int> hyperedge{driver_id};
    for (auto& load : loads_id) {
      if (load != driver_id) {
        hyperedge.push_back(load);
      }
    }
    vertices[driver_id].push_back(hyperedges.size());
    for (int i = 1; i < hyperedge.size(); i++) {
      backward_vertices[hyperedge[i]].push_back(hyperedges.size());
    }
    hyperedges.push_back(hyperedge);
  }  // end net traversal

  debugPrint(
      logger_, MPL, "multilevel_autoclustering", 1, "Created hypergraph");

  // traverse hypergraph to build dataflow
  for (auto [src, src_pin] : io_pin_vertex) {
    int idx = 0;
    std::vector<bool> visited(vertices.size(), false);
    std::vector<std::set<odb::dbInst*>> insts(max_num_ff_dist_);
    dataFlowDFSIOPin(src,
                     idx,
                     insts,
                     io_pin_vertex,
                     std_cell_vertex,
                     macro_pin_vertex,
                     stop_flag_vec,
                     visited,
                     vertices,
                     hyperedges,
                     false);
    dataFlowDFSIOPin(src,
                     idx,
                     insts,
                     io_pin_vertex,
                     std_cell_vertex,
                     macro_pin_vertex,
                     stop_flag_vec,
                     visited,
                     backward_vertices,
                     hyperedges,
                     true);
    io_ffs_conn_map_.emplace_back(src_pin, insts);
  }

  for (auto [src, src_pin] : macro_pin_vertex) {
    int idx = 0;
    std::vector<bool> visited(vertices.size(), false);
    std::vector<std::set<odb::dbInst*>> std_cells(max_num_ff_dist_);
    std::vector<std::set<odb::dbInst*>> macros(max_num_ff_dist_);
    dataFlowDFSMacroPin(src,
                        idx,
                        std_cells,
                        macros,
                        io_pin_vertex,
                        std_cell_vertex,
                        macro_pin_vertex,
                        stop_flag_vec,
                        visited,
                        vertices,
                        hyperedges,
                        false);
    dataFlowDFSMacroPin(src,
                        idx,
                        std_cells,
                        macros,
                        io_pin_vertex,
                        std_cell_vertex,
                        macro_pin_vertex,
                        stop_flag_vec,
                        visited,
                        backward_vertices,
                        hyperedges,
                        true);
    macro_ffs_conn_map_.emplace_back(src_pin, std_cells);
    macro_macro_conn_map_.emplace_back(src_pin, macros);
  }
}

//
// Forward or Backward DFS search to find sequential paths from/to IO pins based
// on hop count to macro pins
//
void HierRTLMP::dataFlowDFSIOPin(int parent,
                                 int idx,
                                 std::vector<std::set<odb::dbInst*>>& insts,
                                 std::map<int, odb::dbBTerm*>& io_pin_vertex,
                                 std::map<int, odb::dbInst*>& std_cell_vertex,
                                 std::map<int, odb::dbITerm*>& macro_pin_vertex,
                                 std::vector<bool>& stop_flag_vec,
                                 std::vector<bool>& visited,
                                 std::vector<std::vector<int>>& vertices,
                                 std::vector<std::vector<int>>& hyperedges,
                                 bool backward_flag)
{
  visited[parent] = true;
  if (stop_flag_vec[parent]) {
    if (parent < io_pin_vertex.size()) {
      ;  // currently we do not consider IO pin to IO pin connnection
    } else if (parent < io_pin_vertex.size() + std_cell_vertex.size()) {
      insts[idx].insert(std_cell_vertex[parent]);
    } else {
      insts[idx].insert(macro_pin_vertex[parent]->getInst());
    }
    idx++;
  }

  if (idx >= max_num_ff_dist_) {
    return;
  }

  if (!backward_flag) {
    for (auto& hyperedge : vertices[parent]) {
      for (auto& vertex : hyperedges[hyperedge]) {
        // we do not consider pin to pin
        if (visited[vertex] || vertex < io_pin_vertex.size()) {
          continue;
        }
        dataFlowDFSIOPin(vertex,
                         idx,
                         insts,
                         io_pin_vertex,
                         std_cell_vertex,
                         macro_pin_vertex,
                         stop_flag_vec,
                         visited,
                         vertices,
                         hyperedges,
                         backward_flag);
      }
    }  // finish hyperedges
  } else {
    for (auto& hyperedge : vertices[parent]) {
      const int vertex = hyperedges[hyperedge][0];  // driver vertex
      // we do not consider pin to pin
      if (visited[vertex] || vertex < io_pin_vertex.size()) {
        continue;
      }
      dataFlowDFSIOPin(vertex,
                       idx,
                       insts,
                       io_pin_vertex,
                       std_cell_vertex,
                       macro_pin_vertex,
                       stop_flag_vec,
                       visited,
                       vertices,
                       hyperedges,
                       backward_flag);
    }  // finish hyperedges
  }    // finish current vertex
}

//
// Forward or Backward DFS search to find sequential paths between Macros based
// on hop count
//
void HierRTLMP::dataFlowDFSMacroPin(
    int parent,
    int idx,
    std::vector<std::set<odb::dbInst*>>& std_cells,
    std::vector<std::set<odb::dbInst*>>& macros,
    std::map<int, odb::dbBTerm*>& io_pin_vertex,
    std::map<int, odb::dbInst*>& std_cell_vertex,
    std::map<int, odb::dbITerm*>& macro_pin_vertex,
    std::vector<bool>& stop_flag_vec,
    std::vector<bool>& visited,
    std::vector<std::vector<int>>& vertices,
    std::vector<std::vector<int>>& hyperedges,
    bool backward_flag)
{
  visited[parent] = true;
  if (stop_flag_vec[parent]) {
    if (parent < io_pin_vertex.size()) {
      ;  // the connection between IO and macro pins have been considers
    } else if (parent < io_pin_vertex.size() + std_cell_vertex.size()) {
      std_cells[idx].insert(std_cell_vertex[parent]);
    } else {
      macros[idx].insert(macro_pin_vertex[parent]->getInst());
    }
    idx++;
  }

  if (idx >= max_num_ff_dist_) {
    return;
  }

  if (!backward_flag) {
    for (auto& hyperedge : vertices[parent]) {
      for (auto& vertex : hyperedges[hyperedge]) {
        // we do not consider pin to pin
        if (visited[vertex] || vertex < io_pin_vertex.size()) {
          continue;
        }
        dataFlowDFSMacroPin(vertex,
                            idx,
                            std_cells,
                            macros,
                            io_pin_vertex,
                            std_cell_vertex,
                            macro_pin_vertex,
                            stop_flag_vec,
                            visited,
                            vertices,
                            hyperedges,
                            backward_flag);
      }
    }  // finish hyperedges
  } else {
    for (auto& hyperedge : vertices[parent]) {
      const int vertex = hyperedges[hyperedge][0];
      // we do not consider pin to pin
      if (visited[vertex] || vertex < io_pin_vertex.size()) {
        continue;
      }
      dataFlowDFSMacroPin(vertex,
                          idx,
                          std_cells,
                          macros,
                          io_pin_vertex,
                          std_cell_vertex,
                          macro_pin_vertex,
                          stop_flag_vec,
                          visited,
                          vertices,
                          hyperedges,
                          backward_flag);
    }  // finish hyperedges
  }
}

void HierRTLMP::updateDataFlow()
{
  // bterm, macros or ffs
  for (const auto& [bterm, insts] : io_ffs_conn_map_) {
    if (bterm_to_cluster_.find(bterm) == bterm_to_cluster_.end()) {
      continue;
    }

    const int driver_id = bterm_to_cluster_.at(bterm);

    for (int i = 0; i < max_num_ff_dist_; i++) {
      const float weight = dataflow_weight_ / std::pow(dataflow_factor_, i);
      std::set<int> sink_clusters;

      for (auto& inst : insts[i]) {
        const int cluster_id = inst_to_cluster_.at(inst);
        sink_clusters.insert(cluster_id);
      }

      for (auto& sink : sink_clusters) {
        cluster_map_[driver_id]->addConnection(sink, weight);
        cluster_map_[sink]->addConnection(driver_id, weight);
      }
    }
  }

  // macros to ffs
  for (const auto& [iterm, insts] : macro_ffs_conn_map_) {
    const int driver_id = inst_to_cluster_.at(iterm->getInst());

    for (int i = 0; i < max_num_ff_dist_; i++) {
      const float weight = dataflow_weight_ / std::pow(dataflow_factor_, i);
      std::set<int> sink_clusters;

      for (auto& inst : insts[i]) {
        const int cluster_id = inst_to_cluster_.at(inst);
        sink_clusters.insert(cluster_id);
      }

      for (auto& sink : sink_clusters) {
        cluster_map_[driver_id]->addConnection(sink, weight);
        cluster_map_[sink]->addConnection(driver_id, weight);
      }
    }
  }

  // macros to macros
  for (const auto& [iterm, insts] : macro_macro_conn_map_) {
    const int driver_id = inst_to_cluster_.at(iterm->getInst());

    for (int i = 0; i < max_num_ff_dist_; i++) {
      const float weight = dataflow_weight_ / std::pow(dataflow_factor_, i);
      std::set<int> sink_clusters;

      for (auto& inst : insts[i]) {
        const int cluster_id = inst_to_cluster_.at(inst);
        sink_clusters.insert(cluster_id);
      }

      for (auto& sink : sink_clusters) {
        cluster_map_[driver_id]->addConnection(sink, weight);
      }
    }
  }
}

// Print Connnection For all the clusters
void HierRTLMP::printConnection()
{
  std::string line;
  line += "NUM_CLUSTERS  :   " + std::to_string(cluster_map_.size()) + "\n";
  for (auto& [cluster_id, cluster] : cluster_map_) {
    const std::map<int, float> connections = cluster->getConnection();
    if (connections.empty()) {
      continue;
    }
    line += "cluster " + cluster->getName() + " : \n";
    for (auto [target, num_nets] : connections) {
      line += "\t\t" + cluster_map_[target]->getName() + "  ";
      line += std::to_string(static_cast<int>(num_nets)) + "\n";
    }
  }
  logger_->report(line);
}

// Print All the clusters and their statics
void HierRTLMP::printClusters()
{
  std::string line;
  line += "NUM_CLUSTERS  :   " + std::to_string(cluster_map_.size()) + "\n";
  for (auto& [cluster_id, cluster] : cluster_map_) {
    line += cluster->getName() + "  ";
    line += std::to_string(cluster->getId()) + "\n";
  }
  logger_->report(line);
}

// This function has two purposes:
// 1) remove all the internal clusters between parent and leaf clusters in its
// subtree 2) Call TritonPart to partition large flat clusters (a cluster with
// no logical modules)
void HierRTLMP::updateSubTree(Cluster* parent)
{
  std::vector<Cluster*> children_clusters;
  std::vector<Cluster*> internal_clusters;
  std::queue<Cluster*> wavefront;
  for (auto child : parent->getChildren()) {
    wavefront.push(child);
  }

  while (!wavefront.empty()) {
    Cluster* cluster = wavefront.front();
    wavefront.pop();
    if (cluster->getChildren().empty()) {
      children_clusters.push_back(cluster);
    } else {
      internal_clusters.push_back(cluster);
      for (auto child : cluster->getChildren()) {
        wavefront.push(child);
      }
    }
  }

  // delete all the internal clusters
  for (auto& cluster : internal_clusters) {
    cluster_map_.erase(cluster->getId());
    delete cluster;
  }

  parent->removeChildren();
  parent->addChildren(children_clusters);
  for (auto& cluster : children_clusters) {
    cluster->setParent(parent);
    if (cluster->getNumStdCell() > max_num_inst_) {
      breakLargeFlatCluster(cluster);
    }
  }
}

// Break large flat clusters with TritonPart
// Binary coding method to differentiate partitions:
// cluster -> cluster_0, cluster_1
// cluster_0 -> cluster_0_0, cluster_0_1
// cluster_1 -> cluster_1_0, cluster_1_1 [...]
void HierRTLMP::breakLargeFlatCluster(Cluster* parent)
{
  // Check if the cluster is a large flat cluster
  if (!parent->getDbModules().empty()
      || parent->getLeafStdCells().size() < max_num_inst_) {
    return;
  }
  updateInstancesAssociation(parent);

  std::map<int, int> cluster_vertex_id_map;
  std::vector<float> vertex_weight;
  int vertex_id = 0;
  for (auto& [cluster_id, cluster] : cluster_map_) {
    cluster_vertex_id_map[cluster_id] = vertex_id++;
    vertex_weight.push_back(0.0f);
  }
  const int num_other_cluster_vertices = vertex_id;

  std::vector<odb::dbInst*> insts;
  std::map<odb::dbInst*, int> inst_vertex_id_map;
  for (auto& macro : parent->getLeafMacros()) {
    inst_vertex_id_map[macro] = vertex_id++;
    vertex_weight.push_back(computeMicronArea(macro));
    insts.push_back(macro);
  }
  for (auto& std_cell : parent->getLeafStdCells()) {
    inst_vertex_id_map[std_cell] = vertex_id++;
    vertex_weight.push_back(computeMicronArea(std_cell));
    insts.push_back(std_cell);
  }

  std::vector<std::vector<int>> hyperedges;
  for (odb::dbNet* net : block_->getNets()) {
    if (net->getSigType().isSupply()) {
      continue;
    }

    int driver_id = -1;
    std::set<int> loads_id;
    bool ignore = false;
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      odb::dbMaster* master = inst->getMaster();
      if (isIgnoredMaster(master)) {
        ignore = true;
        break;
      }

      const int cluster_id = inst_to_cluster_.at(inst);
      int vertex_id = (cluster_id != parent->getId())
                          ? cluster_vertex_id_map[cluster_id]
                          : inst_vertex_id_map[inst];
      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        driver_id = vertex_id;
      } else {
        loads_id.insert(vertex_id);
      }
    }

    if (ignore) {
      continue;
    }

    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const int cluster_id = bterm_to_cluster_.at(bterm);
      if (bterm->getIoType() == odb::dbIoType::INPUT) {
        driver_id = cluster_vertex_id_map[cluster_id];
      } else {
        loads_id.insert(cluster_vertex_id_map[cluster_id]);
      }
    }
    loads_id.insert(driver_id);
    if (driver_id != -1 && loads_id.size() > 1
        && loads_id.size() < large_net_threshold_) {
      std::vector<int> hyperedge;
      hyperedge.insert(hyperedge.end(), loads_id.begin(), loads_id.end());
      hyperedges.push_back(hyperedge);
    }
  }

  const int seed = 0;
  const float balance_constraint = 1.0;
  const int num_parts = 2;  // We use two-way partitioning here
  const int num_vertices = static_cast<int>(vertex_weight.size());
  std::vector<float> hyperedge_weights(hyperedges.size(), 1.0f);

  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Breaking flat cluster {} with TritonPart",
             parent->getName());

  std::vector<int> part
      = tritonpart_->PartitionKWaySimpleMode(num_parts,
                                             balance_constraint,
                                             seed,
                                             hyperedges,
                                             vertex_weight,
                                             hyperedge_weights);

  parent->clearLeafStdCells();
  parent->clearLeafMacros();

  const std::string cluster_name = parent->getName();
  parent->setName(cluster_name + std::string("_0"));
  Cluster* cluster_part_1
      = new Cluster(cluster_id_, cluster_name + std::string("_1"), logger_);

  for (int i = num_other_cluster_vertices; i < num_vertices; i++) {
    odb::dbInst* inst = insts[i - num_other_cluster_vertices];
    if (part[i] == 0) {
      parent->addLeafInst(inst);
    } else {
      cluster_part_1->addLeafInst(inst);
    }
  }

  updateInstancesAssociation(parent);
  setClusterMetrics(parent);
  incorporateNewClusterToTree(cluster_part_1, parent->getParent());

  // Recursive break the cluster
  // until the size of the cluster is less than max_num_inst_
  breakLargeFlatCluster(parent);
  breakLargeFlatCluster(cluster_part_1);
}

// Traverse the physical hierarchy tree in a DFS manner (post-order)
void HierRTLMP::fetchMixedLeaves(
    Cluster* parent,
    std::vector<std::vector<Cluster*>>& mixed_leaves)
{
  if (parent->getChildren().empty() || parent->getNumMacro() == 0) {
    return;
  }

  std::vector<Cluster*> sister_mixed_leaves;

  for (auto& child : parent->getChildren()) {
    updateInstancesAssociation(child);
    if (child->getNumMacro() > 0) {
      if (child->getChildren().empty()) {
        sister_mixed_leaves.push_back(child);
      } else {
        fetchMixedLeaves(child, mixed_leaves);
      }
    } else {
      child->setClusterType(StdCellCluster);
    }
  }

  // We push the leaves after finishing searching the children so
  // that each vector of clusters represents the children of one
  // parent.
  mixed_leaves.push_back(sister_mixed_leaves);
}

void HierRTLMP::breakMixedLeaves(
    const std::vector<std::vector<Cluster*>>& mixed_leaves)
{
  for (const std::vector<Cluster*>& sister_mixed_leaves : mixed_leaves) {
    if (!sister_mixed_leaves.empty()) {
      Cluster* parent = sister_mixed_leaves.front()->getParent();

      for (Cluster* mixed_leaf : sister_mixed_leaves) {
        breakMixedLeaf(mixed_leaf);
      }

      updateInstancesAssociation(parent);
    }
  }
}

// Break mixed leaf into standard-cell and hard-macro clusters.
// Merge macros based on connection signature and footprint.
// Based on types of designs, we support two types of breaking up:
//   1) Replace cluster A by A1, A2, A3
//   2) Create a subtree:
//      A  ->        A
//               |   |   |
//               A1  A2  A3
void HierRTLMP::breakMixedLeaf(Cluster* mixed_leaf)
{
  Cluster* parent = mixed_leaf;

  // Split by replacement if macro dominated.
  if (mixed_leaf->getNumStdCell() * macro_dominated_cluster_threshold_
      < mixed_leaf->getNumMacro()) {
    parent = mixed_leaf->getParent();
  }

  mapMacroInCluster2HardMacro(mixed_leaf);

  std::vector<HardMacro*> hard_macros = mixed_leaf->getHardMacros();
  std::vector<Cluster*> macro_clusters;

  createOneClusterForEachMacro(parent, hard_macros, macro_clusters);

  std::vector<int> size_class(hard_macros.size(), -1);
  classifyMacrosBySize(hard_macros, size_class);

  calculateConnection();

  std::vector<int> signature_class(hard_macros.size(), -1);
  classifyMacrosByConnSignature(macro_clusters, signature_class);

  std::vector<int> interconn_class(hard_macros.size(), -1);
  classifyMacrosByInterconn(macro_clusters, interconn_class);

  std::vector<int> macro_class(hard_macros.size(), -1);
  groupSingleMacroClusters(macro_clusters,
                           size_class,
                           signature_class,
                           interconn_class,
                           macro_class);

  mixed_leaf->clearHardMacros();

  // IMPORTANT: Restore the structure of physical hierarchical tree. Thus the
  // order of leaf clusters will not change the final macro grouping results.
  updateInstancesAssociation(mixed_leaf);

  // Never use SetInstProperty in the following lines for the reason above!
  std::vector<int> virtual_conn_clusters;

  // Deal with the std cells
  if (parent == mixed_leaf) {
    addStdCellClusterToSubTree(parent, mixed_leaf, virtual_conn_clusters);
  } else {
    replaceByStdCellCluster(mixed_leaf, virtual_conn_clusters);
  }

  // Deal with the macros
  for (int i = 0; i < macro_class.size(); i++) {
    if (macro_class[i] != i) {
      continue;  // this macro cluster has been merged
    }

    macro_clusters[i]->setClusterType(HardMacroCluster);

    if (interconn_class[i] != -1) {
      macro_clusters[i]->setAsArrayOfInterconnectedMacros();
    }

    setClusterMetrics(macro_clusters[i]);
    virtual_conn_clusters.push_back(mixed_leaf->getId());
  }

  // add virtual connections
  for (int i = 0; i < virtual_conn_clusters.size(); i++) {
    for (int j = i + 1; j < virtual_conn_clusters.size(); j++) {
      parent->addVirtualConnection(virtual_conn_clusters[i],
                                   virtual_conn_clusters[j]);
    }
  }
}

// Map all the macros into their HardMacro objects for all the clusters
void HierRTLMP::mapMacroInCluster2HardMacro(Cluster* cluster)
{
  if (cluster->getClusterType() == StdCellCluster) {
    return;
  }

  std::vector<HardMacro*> hard_macros;
  for (const auto& macro : cluster->getLeafMacros()) {
    hard_macros.push_back(hard_macro_map_[macro]);
  }
  for (const auto& module : cluster->getDbModules()) {
    getHardMacros(module, hard_macros);
  }
  cluster->specifyHardMacros(hard_macros);
}

// Get all the hard macros in a logical module
void HierRTLMP::getHardMacros(odb::dbModule* module,
                              std::vector<HardMacro*>& hard_macros)
{
  for (odb::dbInst* inst : module->getInsts()) {
    odb::dbMaster* master = inst->getMaster();

    if (isIgnoredMaster(master)) {
      continue;
    }

    if (master->isBlock()) {
      hard_macros.push_back(hard_macro_map_[inst]);
    }
  }

  for (odb::dbModInst* inst : module->getChildren()) {
    getHardMacros(inst->getMaster(), hard_macros);
  }
}

void HierRTLMP::createOneClusterForEachMacro(
    Cluster* parent,
    const std::vector<HardMacro*>& hard_macros,
    std::vector<Cluster*>& macro_clusters)
{
  for (auto& hard_macro : hard_macros) {
    std::string cluster_name = hard_macro->getName();
    Cluster* single_macro_cluster
        = new Cluster(cluster_id_, cluster_name, logger_);
    single_macro_cluster->addLeafMacro(hard_macro->getInst());
    incorporateNewClusterToTree(single_macro_cluster, parent);

    macro_clusters.push_back(single_macro_cluster);
  }
}

void HierRTLMP::classifyMacrosBySize(const std::vector<HardMacro*>& hard_macros,
                                     std::vector<int>& size_class)
{
  for (int i = 0; i < hard_macros.size(); i++) {
    if (size_class[i] == -1) {
      for (int j = i + 1; j < hard_macros.size(); j++) {
        if ((size_class[j] == -1) && ((*hard_macros[i]) == (*hard_macros[j]))) {
          size_class[j] = i;
        }
      }
    }
  }

  for (int i = 0; i < hard_macros.size(); i++) {
    size_class[i] = (size_class[i] == -1) ? i : size_class[i];
  }
}

void HierRTLMP::classifyMacrosByInterconn(
    const std::vector<Cluster*>& macro_clusters,
    std::vector<int>& interconn_class)
{
  for (int i = 0; i < macro_clusters.size(); i++) {
    if (interconn_class[i] == -1) {
      interconn_class[i] = i;
      for (int j = 0; j < macro_clusters.size(); j++) {
        if (macro_clusters[i]->hasMacroConnectionWith(
                *macro_clusters[j], signature_net_threshold_)) {
          if (interconn_class[j] != -1) {
            interconn_class[i] = interconn_class[j];
            break;
          }

          interconn_class[j] = i;
        }
      }
    }
  }
}

void HierRTLMP::classifyMacrosByConnSignature(
    const std::vector<Cluster*>& macro_clusters,
    std::vector<int>& signature_class)
{
  for (int i = 0; i < macro_clusters.size(); i++) {
    if (signature_class[i] == -1) {
      signature_class[i] = i;
      for (int j = i + 1; j < macro_clusters.size(); j++) {
        if (signature_class[j] != -1) {
          continue;
        }

        if (macro_clusters[i]->isSameConnSignature(*macro_clusters[j],
                                                   signature_net_threshold_)) {
          signature_class[j] = i;
        }
      }
    }
  }

  if (logger_->debugCheck(MPL, "multilevel_autoclustering", 2)) {
    logger_->report("\nPrint Connection Signature\n");
    for (auto& cluster : macro_clusters) {
      logger_->report("Macro Signature: {}", cluster->getName());
      for (auto& [cluster_id, weight] : cluster->getConnection()) {
        logger_->report(" {} {} ", cluster_map_[cluster_id]->getName(), weight);
      }
    }
  }
}

// We determine if the macros belong to the same class based on:
// 1. Size && and Interconnection (Directly connected macro clusters
//    should be grouped)
// 2. Size && Connection Signature (Macros with same connection
//    signature should be grouped)
void HierRTLMP::groupSingleMacroClusters(
    const std::vector<Cluster*>& macro_clusters,
    const std::vector<int>& size_class,
    const std::vector<int>& signature_class,
    std::vector<int>& interconn_class,
    std::vector<int>& macro_class)
{
  for (int i = 0; i < macro_clusters.size(); i++) {
    if (macro_class[i] != -1) {
      continue;
    }
    macro_class[i] = i;

    for (int j = i + 1; j < macro_clusters.size(); j++) {
      if (macro_class[j] != -1) {
        continue;
      }

      if (size_class[i] == size_class[j]) {
        if (interconn_class[i] == interconn_class[j]) {
          macro_class[j] = i;

          debugPrint(logger_,
                     MPL,
                     "multilevel_autoclustering",
                     1,
                     "Merging interconnected macro clusters {} and {}",
                     macro_clusters[j]->getName(),
                     macro_clusters[i]->getName());

          mergeMacroClustersWithinSameClass(macro_clusters[i],
                                            macro_clusters[j]);
        } else {
          // We need this so we can distinguish arrays of interconnected macros
          // from grouped macro clusters with same signature.
          interconn_class[i] = -1;

          if (signature_class[i] == signature_class[j]) {
            macro_class[j] = i;

            debugPrint(logger_,
                       MPL,
                       "multilevel_autoclustering",
                       1,
                       "Merging same signature clusters {} and {}.",
                       macro_clusters[j]->getName(),
                       macro_clusters[i]->getName());

            mergeMacroClustersWithinSameClass(macro_clusters[i],
                                              macro_clusters[j]);
          }
        }
      }
    }
  }
}

void HierRTLMP::mergeMacroClustersWithinSameClass(Cluster* target,
                                                  Cluster* source)
{
  bool delete_merged = false;
  target->mergeCluster(*source, delete_merged);

  if (delete_merged) {
    cluster_map_.erase(source->getId());
    delete source;
  }
}

void HierRTLMP::addStdCellClusterToSubTree(
    Cluster* parent,
    Cluster* mixed_leaf,
    std::vector<int>& virtual_conn_clusters)
{
  std::string std_cell_cluster_name = mixed_leaf->getName();
  Cluster* std_cell_cluster
      = new Cluster(cluster_id_, std_cell_cluster_name, logger_);

  std_cell_cluster->copyInstances(*mixed_leaf);
  std_cell_cluster->clearLeafMacros();
  std_cell_cluster->setClusterType(StdCellCluster);

  setClusterMetrics(std_cell_cluster);

  cluster_map_[cluster_id_++] = std_cell_cluster;

  // modify the physical hierachy tree
  std_cell_cluster->setParent(parent);
  parent->addChild(std_cell_cluster);
  virtual_conn_clusters.push_back(std_cell_cluster->getId());
}

// We don't modify the physical hierarchy when spliting by replacement
void HierRTLMP::replaceByStdCellCluster(Cluster* mixed_leaf,
                                        std::vector<int>& virtual_conn_clusters)
{
  mixed_leaf->clearLeafMacros();
  mixed_leaf->setClusterType(StdCellCluster);

  setClusterMetrics(mixed_leaf);

  virtual_conn_clusters.push_back(mixed_leaf->getId());
}

// Print Physical Hierarchy tree in a DFS manner
void HierRTLMP::printPhysicalHierarchyTree(Cluster* parent, int level)
{
  std::string line;
  for (int i = 0; i < level; i++) {
    line += "+---";
  }
  line += fmt::format(
      "{}  ({})  num_macro :  {}   num_std_cell :  {}"
      "  macro_area :  {}  std_cell_area : {}  cluster type: {} {}",
      parent->getName(),
      parent->getId(),
      parent->getNumMacro(),
      parent->getNumStdCell(),
      parent->getMacroArea(),
      parent->getStdCellArea(),
      parent->getIsLeafString(),
      parent->getClusterTypeString());
  logger_->report("{}", line);

  for (auto& cluster : parent->getChildren()) {
    printPhysicalHierarchyTree(cluster, level + 1);
  }
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
        calculateChildrenTilings(cluster);
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
      SoftMacro macro = SoftMacro(cluster);
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

  const Rect outline(
      0, 0, root_cluster_->getWidth(), root_cluster_->getHeight());

  const int num_perturb_per_step = (macros.size() > num_perturb_per_step_ / 10)
                                       ? macros.size()
                                       : num_perturb_per_step_ / 10;
  std::vector<SACoreSoftMacro*> sa_containers;
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
    std::vector<SACoreSoftMacro*> sa_vector;
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
      SACoreSoftMacro* sa
          = new SACoreSoftMacro(root_cluster_,
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
      sa_vector.push_back(sa);
    }
    if (sa_vector.size() == 1) {
      runSA<SACoreSoftMacro>(sa_vector[0]);
    } else {
      // multi threads
      std::vector<std::thread> threads;
      threads.reserve(sa_vector.size());
      for (auto& sa : sa_vector) {
        threads.emplace_back(runSA<SACoreSoftMacro>, sa);
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    // add macro tilings
    for (auto& sa : sa_vector) {
      sa_containers.push_back(sa);
      if (sa->isValid(outline)) {
        macro_tilings.insert(
            std::pair<float, float>(sa->getWidth(), sa->getHeight()));
      }
    }
    sa_vector.clear();
    remaining_runs -= run_thread;
  }
  // vary the outline height while keeping outline width fixed
  remaining_runs = num_runs_;
  run_id = 0;
  while (remaining_runs > 0) {
    std::vector<SACoreSoftMacro*> sa_vector;
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
      SACoreSoftMacro* sa
          = new SACoreSoftMacro(root_cluster_,
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
      sa_vector.push_back(sa);
    }
    if (sa_vector.size() == 1) {
      runSA<SACoreSoftMacro>(sa_vector[0]);
    } else {
      // multi threads
      std::vector<std::thread> threads;
      threads.reserve(sa_vector.size());
      for (auto& sa : sa_vector) {
        threads.emplace_back(runSA<SACoreSoftMacro>, sa);
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    // add macro tilings
    for (auto& sa : sa_vector) {
      sa_containers.push_back(sa);
      if (sa->isValid(outline)) {
        macro_tilings.insert(
            std::pair<float, float>(sa->getWidth(), sa->getHeight()));
      }
    }
    sa_vector.clear();
    remaining_runs -= run_thread;
  }
  // clean all the SA to avoid memory leakage
  sa_containers.clear();
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

  const Rect outline(
      0, 0, root_cluster_->getWidth(), root_cluster_->getHeight());

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

  std::vector<SACoreHardMacro*> sa_containers;
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
    std::vector<SACoreHardMacro*> sa_vector;
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
      SACoreHardMacro* sa
          = new SACoreHardMacro(new_outline,
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
      sa_vector.push_back(sa);
    }
    if (sa_vector.size() == 1) {
      runSA<SACoreHardMacro>(sa_vector[0]);
    } else {
      // multi threads
      std::vector<std::thread> threads;
      threads.reserve(sa_vector.size());
      for (auto& sa : sa_vector) {
        threads.emplace_back(runSA<SACoreHardMacro>, sa);
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    // add macro tilings
    for (auto& sa : sa_vector) {
      sa_containers.push_back(sa);
      if (sa->isValid(outline)) {
        macro_tilings.insert(
            std::pair<float, float>(sa->getWidth(), sa->getHeight()));
      }
    }
    sa_vector.clear();
    remaining_runs -= run_thread;
  }
  // change the outline height while keeping outline width fixed
  remaining_runs = num_runs_;
  run_id = 0;
  while (remaining_runs > 0) {
    std::vector<SACoreHardMacro*> sa_vector;
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
      SACoreHardMacro* sa
          = new SACoreHardMacro(new_outline,
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
      sa_vector.push_back(sa);
    }
    if (sa_vector.size() == 1) {
      runSA<SACoreHardMacro>(sa_vector[0]);
    } else {
      // multi threads
      std::vector<std::thread> threads;
      threads.reserve(sa_vector.size());
      for (auto& sa : sa_vector) {
        threads.emplace_back(runSA<SACoreHardMacro>, sa);
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    // add macro tilings
    for (auto& sa : sa_vector) {
      sa_containers.push_back(sa);
      if (sa->isValid(outline)) {
        macro_tilings.insert(
            std::pair<float, float>(sa->getWidth(), sa->getHeight()));
      }
    }
    sa_vector.clear();
    remaining_runs -= run_thread;
  }
  // clean the sa_container to avoid memory leakage
  sa_containers.clear();
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
  if (!io_pad_map_.empty()) {
    return;
  }

  IOSpans io_spans = computeIOSpans();
  const float depth = computeIOBlockagesDepth(io_spans);

  const Rect root(root_cluster_->getX(),
                  root_cluster_->getY(),
                  root_cluster_->getX() + root_cluster_->getWidth(),
                  root_cluster_->getY() + root_cluster_->getHeight());

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
  for (auto& cluster : root_cluster_->getChildren()) {
    if (cluster->getClusterType() == StdCellCluster) {
      std_cell_area += cluster->getArea();
    }
  }

  const float macro_dominance_factor
      = macro_with_halo_area_
        / (root_cluster_->getWidth() * root_cluster_->getHeight());
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
    updateInstancesAssociation(cluster);
  }
  // Place children clusters
  // map children cluster to soft macro
  for (auto& cluster : parent->getChildren()) {
    if (cluster->isIOCluster()) {  // ignore all the io clusters
      continue;
    }
    SoftMacro* macro = new SoftMacro(cluster);
    // no memory leakage, beacuse we set the soft macro, the old one
    // will be deleted
    cluster->setSoftMacro(macro);
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
      io_clusters.push_back(cluster);
      continue;
    }
    // for other clusters
    soft_macro_id_map[cluster->getName()] = macros.size();
    SoftMacro* soft_macro = new SoftMacro(cluster);
    updateInstancesAssociation(cluster);  // we need this step to calculate nets
    macros.push_back(*soft_macro);
    cluster->setSoftMacro(soft_macro);
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

  calculateConnection();
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
    BundledNet net(soft_macro_id_map[cluster_map_[cluster1]->getName()],
                   soft_macro_id_map[cluster_map_[cluster2]->getName()],
                   virtual_weight_);
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
                 cluster_map_[cluster_id]->getName(),
                 weight);
      const std::string name = cluster_map_[cluster_id]->getName();
      if (soft_macro_id_map.find(name) == soft_macro_id_map.end()) {
        float new_weight = weight;
        if (macros[soft_macro_id_map[src_name]].isStdCellCluster()) {
          new_weight *= virtual_weight_;
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
  for (auto& c : file_name) {
    if (c == '/') {
      c = '*';
    }
  }
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
  std::vector<SACoreSoftMacro*>
      sa_containers;  // store all the SA runs to avoid memory leakage
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
    std::vector<SACoreSoftMacro*> sa_vector;
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
      SACoreSoftMacro* sa
          = new SACoreSoftMacro(root_cluster_,
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
      sa_vector.push_back(sa);
    }
    if (sa_vector.size() == 1) {
      runSA<SACoreSoftMacro>(sa_vector[0]);
    } else {
      // multi threads
      std::vector<std::thread> threads;
      threads.reserve(sa_vector.size());
      for (auto& sa : sa_vector) {
        threads.emplace_back(runSA<SACoreSoftMacro>, sa);
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    remaining_runs -= run_thread;
    // add macro tilings
    for (auto& sa : sa_vector) {
      sa_containers.push_back(sa);
    }
    while (sa_containers.size() >= end_check) {
      while (begin_check < end_check) {
        auto& sa = sa_containers[begin_check];
        if (sa->isValid() && sa->getNormCost() < best_cost) {
          best_cost = sa->getNormCost();
          best_sa = sa;
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
    sa_containers.clear();
    best_cost = std::numeric_limits<float>::max();
    begin_check = 0;
    end_check = std::min(check_interval, remaining_runs);
    debugPrint(logger_,
               MPL,
               "hierarchical_macro_placement",
               1,
               "Start Simulated Annealing Core");
    while (remaining_runs > 0) {
      std::vector<SACoreSoftMacro*> sa_vector;
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
        SACoreSoftMacro* sa
            = new SACoreSoftMacro(root_cluster_,
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
        sa_vector.push_back(sa);
      }
      if (sa_vector.size() == 1) {
        runSA<SACoreSoftMacro>(sa_vector[0]);
      } else {
        // multi threads
        std::vector<std::thread> threads;
        threads.reserve(sa_vector.size());
        for (auto& sa : sa_vector) {
          threads.emplace_back(runSA<SACoreSoftMacro>, sa);
        }
        for (auto& th : threads) {
          th.join();
        }
      }
      remaining_runs -= run_thread;
      // add macro tilings
      for (auto& sa : sa_vector) {
        sa_containers.push_back(sa);
      }
      while (sa_containers.size() >= end_check) {
        while (begin_check < end_check) {
          auto& sa = sa_containers[begin_check];
          if (sa->isValid() && sa->getNormCost() < best_cost) {
            best_cost = sa->getNormCost();
            best_sa = sa;
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
      runHierarchicalMacroPlacement(cluster);
    }
  }

  sa_containers.clear();

  updateInstancesAssociation(parent);
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
    updateInstancesAssociation(cluster);
  }
  // Place children clusters
  // map children cluster to soft macro
  for (auto& cluster : parent->getChildren()) {
    if (cluster->isIOCluster()) {  // ignore all the io clusters
      continue;
    }
    SoftMacro* macro = new SoftMacro(cluster);
    // no memory leakage, beacuse we set the soft macro, the old one
    // will be deleted
    cluster->setSoftMacro(macro);
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
      io_clusters.push_back(cluster);
      continue;
    }
    // for other clusters
    soft_macro_id_map[cluster->getName()] = macros.size();
    SoftMacro* soft_macro = new SoftMacro(cluster);
    updateInstancesAssociation(cluster);  // we need this step to calculate nets
    macros.push_back(*soft_macro);
    cluster->setSoftMacro(soft_macro);
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
      for (auto cluster : frontwave->getParent()->getChildren()) {
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
  calculateConnection();
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Finished calculating connection");
  updateDataFlow();
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Finished updating dataflow");

  // add the virtual connections (the weight related to IOs and macros belong to
  // the same cluster)
  for (const auto& [cluster1, cluster2] : parent->getVirtualConnections()) {
    BundledNet net(soft_macro_id_map[cluster_map_[cluster1]->getName()],
                   soft_macro_id_map[cluster_map_[cluster2]->getName()],
                   virtual_weight_);
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
                 cluster_map_[cluster_id]->getName(),
                 weight);
      const std::string name = cluster_map_[cluster_id]->getName();
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
  for (auto& c : file_name) {
    if (c == '/') {
      c = '*';
    }
  }
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
  std::vector<SACoreSoftMacro*> sa_containers;
  // To give consistency across threads we check the solutions
  // at a fixed interval independent of how many threads we are using.
  const int check_interval = 10;
  int begin_check = 0;
  int end_check = std::min(check_interval, remaining_runs);
  float best_cost = std::numeric_limits<float>::max();
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Start Simulated Annealing Core");
  while (remaining_runs > 0) {
    std::vector<SACoreSoftMacro*> sa_vector;
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
      SACoreSoftMacro* sa
          = new SACoreSoftMacro(root_cluster_,
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
      sa_vector.push_back(sa);
    }
    if (sa_vector.size() == 1) {
      runSA<SACoreSoftMacro>(sa_vector[0]);
    } else {
      // multi threads
      std::vector<std::thread> threads;
      threads.reserve(sa_vector.size());
      for (auto& sa : sa_vector) {
        threads.emplace_back(runSA<SACoreSoftMacro>, sa);
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    remaining_runs -= run_thread;
    // add macro tilings
    for (auto& sa : sa_vector) {
      sa_containers.push_back(sa);
    }
    while (sa_containers.size() >= end_check) {
      while (begin_check < end_check) {
        auto& sa = sa_containers[begin_check];
        if (sa->isValid() && sa->getNormCost() < best_cost) {
          best_cost = sa->getNormCost();
          best_sa = sa;
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
      runHierarchicalMacroPlacementWithoutBusPlanning(cluster);
    }
  }

  sa_containers.clear();

  updateInstancesAssociation(parent);
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
    SoftMacro* macro = new SoftMacro(cluster);
    // no memory leakage, beacuse we set the soft macro, the old one
    // will be deleted
    cluster->setSoftMacro(macro);
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
      io_clusters.push_back(cluster);
      continue;
    }
    // for other clusters
    soft_macro_id_map[cluster->getName()] = macros.size();
    SoftMacro* soft_macro = new SoftMacro(cluster);
    updateInstancesAssociation(cluster);  // we need this step to calculate nets
    macros.push_back(*soft_macro);
    cluster->setSoftMacro(soft_macro);
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
      for (auto cluster : frontwave->getParent()->getChildren()) {
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
  calculateConnection();
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Finished calculating connection");
  updateDataFlow();
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "Finished updating dataflow");

  // add the virtual connections (the weight related to IOs and macros belong to
  // the same cluster)
  for (const auto& [cluster1, cluster2] : parent->getVirtualConnections()) {
    BundledNet net(soft_macro_id_map[cluster_map_[cluster1]->getName()],
                   soft_macro_id_map[cluster_map_[cluster2]->getName()],
                   virtual_weight_);
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
                 cluster_map_[cluster_id]->getName(),
                 weight);
      const std::string name = cluster_map_[cluster_id]->getName();
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
  for (auto& c : file_name) {
    if (c == '/') {
      c = '*';
    }
  }
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
  std::vector<SACoreSoftMacro*>
      sa_containers;  // store all the SA runs to avoid memory leakage
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
    std::vector<SACoreSoftMacro*> sa_vector;
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
      SACoreSoftMacro* sa
          = new SACoreSoftMacro(root_cluster_,
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
      sa->setNumberOfMacrosToPlace(macros_to_place);
      sa->setCentralizationAttemptOn(true);
      sa->setFences(fences);
      sa->setGuides(guides);
      sa->setNets(nets);
      sa->addBlockages(placement_blockages);
      sa->addBlockages(macro_blockages);
      sa_vector.push_back(sa);
    }
    if (sa_vector.size() == 1) {
      runSA<SACoreSoftMacro>(sa_vector[0]);
    } else {
      // multi threads
      std::vector<std::thread> threads;
      threads.reserve(sa_vector.size());
      for (auto& sa : sa_vector) {
        threads.emplace_back(runSA<SACoreSoftMacro>, sa);
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    remaining_runs -= run_thread;
    // add macro tilings
    for (auto& sa : sa_vector) {
      sa_containers.push_back(sa);
    }
    while (sa_containers.size() >= end_check) {
      while (begin_check < end_check) {
        auto& sa = sa_containers[begin_check];
        if (sa->isValid() && sa->getNormCost() < best_cost) {
          best_cost = sa->getNormCost();
          best_sa = sa;
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

  sa_containers.clear();
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

  std::vector<HardMacro*> hard_macros = cluster->getHardMacros();
  num_hard_macros_cluster_ += hard_macros.size();

  std::vector<HardMacro> sa_macros;
  std::vector<Cluster*> macro_clusters;  // needed to calculate connections
  std::map<int, int> cluster_to_macro;
  std::set<odb::dbMaster*> masters;
  createClusterForEachMacro(
      hard_macros, sa_macros, macro_clusters, cluster_to_macro, masters);

  const Rect outline(cluster->getX(),
                     cluster->getY(),
                     cluster->getX() + cluster->getWidth(),
                     cluster->getY() + cluster->getHeight());

  std::map<int, Rect> fences;
  std::map<int, Rect> guides;
  computeFencesAndGuides(hard_macros, outline, fences, guides);

  calculateConnection();

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
  std::vector<SACoreHardMacro*> sa_containers;  // store all the SA runs

  while (remaining_runs > 0) {
    std::vector<SACoreHardMacro*> sa_vector;
    const int run_thread
        = graphics_ ? 1 : std::min(remaining_runs, num_threads_);

    for (int i = 0; i < run_thread; i++) {
      if (graphics_) {
        graphics_->setOutline(micronsToDbu(outline));
      }

      SACoreHardMacro* sa
          = new SACoreHardMacro(outline,
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

      sa_vector.push_back(sa);

      run_id++;
    }
    if (sa_vector.size() == 1) {
      runSA<SACoreHardMacro>(sa_vector[0]);
    } else {
      std::vector<std::thread> threads;
      threads.reserve(sa_vector.size());

      for (auto& sa : sa_vector) {
        threads.emplace_back(runSA<SACoreHardMacro>, sa);
      }

      for (auto& th : threads) {
        th.join();
      }
    }

    for (auto& sa : sa_vector) {
      sa_containers.push_back(sa);

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
        best_sa = sa;
      }
    }

    sa_vector.clear();
    remaining_runs -= run_thread;
  }

  if (best_sa == nullptr) {
    for (auto& sa : sa_containers) {
      sa->printResults();
    }

    sa_containers.clear();
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
    num_updated_macros_++;
    hard_macro->setX(hard_macro->getX() + outline.xMin());
    hard_macro->setY(hard_macro->getY() + outline.yMin());
  }

  sa_containers.clear();

  updateInstancesAssociation(cluster);
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

void HierRTLMP::createClusterForEachMacro(
    const std::vector<HardMacro*>& hard_macros,
    std::vector<HardMacro>& sa_macros,
    std::vector<Cluster*>& macro_clusters,
    std::map<int, int>& cluster_to_macro,
    std::set<odb::dbMaster*>& masters)
{
  int macro_id = 0;
  std::string cluster_name;

  for (auto& hard_macro : hard_macros) {
    macro_id = sa_macros.size();
    cluster_name = hard_macro->getName();

    Cluster* macro_cluster = new Cluster(cluster_id_, cluster_name, logger_);
    macro_cluster->addLeafMacro(hard_macro->getInst());
    updateInstancesAssociation(macro_cluster);

    sa_macros.push_back(*hard_macro);
    macro_clusters.push_back(macro_cluster);
    cluster_to_macro[cluster_id_] = macro_id;
    masters.insert(hard_macro->getInst()->getMaster());

    cluster_map_[cluster_id_++] = macro_cluster;
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

void HierRTLMP::createFixedTerminals(
    const Rect& outline,
    const std::vector<Cluster*>& macro_clusters,
    std::map<int, int>& cluster_to_macro,
    std::vector<HardMacro>& sa_macros)
{
  std::set<int> clusters_ids;

  for (auto macro_cluster : macro_clusters) {
    for (auto [cluster_id, weight] : macro_cluster->getConnection()) {
      clusters_ids.insert(cluster_id);
    }
  }

  for (auto cluster_id : clusters_ids) {
    if (cluster_to_macro.find(cluster_id) != cluster_to_macro.end()) {
      continue;
    }
    auto& temp_cluster = cluster_map_[cluster_id];

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
    const std::vector<Cluster*>& macro_clusters,
    const std::map<int, int>& cluster_to_macro)
{
  std::vector<BundledNet> nets;

  for (auto macro_cluster : macro_clusters) {
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
      generateTemporaryStdCellsPlacement(child);
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
  for (const auto& [inst, hard_macro] : hard_macro_map_) {
    updateMacroOnDb(hard_macro);
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

  for (auto& [inst, hard_macro] : hard_macro_map_) {
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
  for (auto& [module, metrics] : logical_module_map_) {
    delete metrics;
  }
  logical_module_map_.clear();

  for (auto& [inst, hard_macro] : hard_macro_map_) {
    delete hard_macro;
  }
  hard_macro_map_.clear();

  for (auto& [cluster_id, cluster] : cluster_map_) {
    delete cluster;
  }
  cluster_map_.clear();

  if (graphics_) {
    graphics_->eraseDrawing();
  }
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             1,
             "number of macros in HardMacroCluster : {}",
             num_hard_macros_cluster_);
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
  for (Cluster* child : parent->getChildren()) {
    if (child->getClusterType() == HardMacroCluster) {
      macro_clusters.push_back(child);

      for (HardMacro* hard_macro : child->getHardMacros()) {
        hard_macros_.push_back(hard_macro);
      }

    } else if (child->getClusterType() == MixedCluster) {
      fetchMacroClusters(child, macro_clusters);
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

  for (Cluster* child : root_->getChildren()) {
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
