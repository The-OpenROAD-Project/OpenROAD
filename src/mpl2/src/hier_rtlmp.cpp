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

#include "SACoreHardMacro.h"
#include "SACoreSoftMacro.h"
#include "bus_synthesis.h"
#include "db_sta/dbNetwork.hh"
#include "graphics.h"
#include "object.h"
#include "odb/db.h"
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

//
// Set defaults for min/max number of instances and macros if not set by user.
//
void HierRTLMP::setDefaultThresholds()
{
  std::string snap_layer_name;
  // calculate the pitch_x and pitch_y based on the pins of macros
  for (auto& macro : hard_macro_map_) {
    odb::dbMaster* master = macro.first->getMaster();
    for (odb::dbMTerm* mterm : master->getMTerms()) {
      if (mterm->getSigType() == odb::dbSigType::SIGNAL) {
        for (odb::dbMPin* mpin : mterm->getMPins()) {
          for (odb::dbBox* box : mpin->getGeometry()) {
            odb::dbTechLayer* layer = box->getTechLayer();
            snap_layer_name = layer->getName();
            pitch_x_
                = dbuToMicron(static_cast<float>(layer->getPitchX()), dbu_);
            pitch_y_
                = dbuToMicron(static_cast<float>(layer->getPitchY()), dbu_);
          }
        }
      }
    }
    break;  // we just need to calculate pitch_x and pitch_y once
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
    logger_->report("snap_layer : {}  congestion_weight : {}",
                    snap_layer_,
                    congestion_weight_);
  }

  if (max_num_macro_base_ <= 0 || min_num_macro_base_ <= 0
      || max_num_inst_base_ <= 0 || min_num_inst_base_ <= 0) {
    min_num_inst_base_
        = std::floor(metrics_->getNumStdCell()
                     / std::pow(coarsening_ratio_, max_num_level_));
    if (min_num_inst_base_ <= 1000)
      min_num_inst_base_ = 1000;  // lower bound
    max_num_inst_base_ = min_num_inst_base_ * coarsening_ratio_ / 2.0;
    min_num_macro_base_ = std::floor(
        metrics_->getNumMacro() / std::pow(coarsening_ratio_, max_num_level_));
    if (min_num_macro_base_ <= 0)
      min_num_macro_base_ = 1;  // lowerbound
    max_num_macro_base_ = min_num_macro_base_ * coarsening_ratio_ / 2.0;
    if (metrics_->getNumMacro() <= 150) {
      // If the number of macros is less than the threshold value, reset to
      // single level.
      max_num_level_ = 1;
      logger_->report("Reset number of levels to 1");
    }
  }

  //
  // Set sizes for root level based on coarsening_factor and the number of
  // physical hierarchy levels
  //
  unsigned coarsening_factor = std::pow(coarsening_ratio_, max_num_level_ - 1);
  max_num_macro_base_ = max_num_macro_base_ * coarsening_factor;
  min_num_macro_base_ = min_num_macro_base_ * coarsening_factor;
  max_num_inst_base_ = max_num_inst_base_ * coarsening_factor;
  min_num_inst_base_ = min_num_inst_base_ * coarsening_factor;

  logger_->report(
      "num level: {}, max_macro: {}, min_macro: {}, max_inst:{}, min_inst:{}",
      max_num_level_,
      max_num_macro_base_,
      min_num_macro_base_,
      max_num_inst_base_,
      min_num_inst_base_);
}

///////////////////////////////////////////////////////////////
// Top Level Interface function
//
void HierRTLMP::hierRTLMacroPlacer()
{
  //
  // Get the database information
  //
  block_ = db_->getChip()->getBlock();
  dbu_ = db_->getTech()->getDbUnitsPerMicron();
  // report the default parameters
  logger_->report("area_weight_ = {}", area_weight_);
  logger_->report("outline_weight_ = {}", outline_weight_);
  logger_->report("wirelength_weight_ = {}", wirelength_weight_);
  logger_->report("guidance_weight_ = {}", guidance_weight_);
  logger_->report("fence_weight_ = {}", fence_weight_);
  logger_->report("boundary_weight_ = {}", boundary_weight_);
  logger_->report("notch_weight_ = {}", notch_weight_);
  logger_->report("macro_blockage_weight_ = {}", macro_blockage_weight_);
  logger_->report("halo_width_ = {}", halo_width_);

  //
  // Get the floorplan information
  //
  odb::Rect die_box = block_->getDieArea();
  floorplan_lx_ = dbuToMicron(die_box.xMin(), dbu_);
  floorplan_ly_ = dbuToMicron(die_box.yMin(), dbu_);
  floorplan_ux_ = dbuToMicron(die_box.xMax(), dbu_);
  floorplan_uy_ = dbuToMicron(die_box.yMax(), dbu_);

  odb::Rect core_box = block_->getCoreArea();
  int core_lx = dbuToMicron(core_box.xMin(), dbu_);
  int core_ly = dbuToMicron(core_box.yMin(), dbu_);
  int core_ux = dbuToMicron(core_box.xMax(), dbu_);
  int core_uy = dbuToMicron(core_box.yMax(), dbu_);

  logger_->report(
      "Floorplan Outline: ({}, {}) ({}, {}),  Core Outline: ({}, {}) ({}, {})",
      floorplan_lx_,
      floorplan_ly_,
      floorplan_ux_,
      floorplan_uy_,
      core_lx,
      core_ly,
      core_ux,
      core_uy);

  //
  // Compute metrics for dbModules
  // Associate all the hard macros with their HardMacro objects
  // and report the statistics
  //
  metrics_ = computeMetrics(block_->getTopModule());
  float core_area = (core_ux - core_lx) * (core_uy - core_ly);
  float util
      = (metrics_->getStdCellArea() + metrics_->getMacroArea()) / core_area;
  float core_util
      = metrics_->getStdCellArea() / (core_area - metrics_->getMacroArea());

  logger_->report(
      "Traversed logical hierarchy\n"
      "\tNumber of std cell instances : {}\n"
      "\tArea of std cell instances : {:.2f}\n"
      "\tNumber of macros : {}\n"
      "\tArea of macros : {:.2f}\n"
      "\tTotal area : {:.2f}\n"
      "\tDesign Utilization : {:.2f}\n"
      "\tCore Utilization: {:.2f}\n",
      metrics_->getNumStdCell(),
      metrics_->getStdCellArea(),
      metrics_->getNumMacro(),
      metrics_->getMacroArea(),
      metrics_->getStdCellArea() + metrics_->getMacroArea(),
      util,
      core_util);

  setDefaultThresholds();

  //
  // Initialize the physcial hierarchy tree
  // create root cluster
  //
  cluster_id_ = 0;
  // set the level of cluster to be 0
  root_cluster_ = new Cluster(cluster_id_, std::string("root"), logger_);
  // set the design metrics as the metrics for the root cluster
  root_cluster_->addDbModule(block_->getTopModule());
  root_cluster_->setMetrics(*metrics_);
  cluster_map_[cluster_id_++] = root_cluster_;
  // assign cluster_id property of each instance
  for (auto inst : block_->getInsts()) {
    odb::dbIntProperty::create(inst, "cluster_id", cluster_id_);
  }

  //
  // model each bundled IO as a cluster under the root node
  // cluster_id:  0 to num_bundled_IOs x 4 are reserved for bundled IOs
  // following the sequence:  Left -> Top -> Right -> Bottom
  // starting at (floorplan_lx_, floorplan_ly_)
  // Map IOs to Pads if the design has pads
  //

  mapIOPads();
  createBundledIOs();

  // create data flow information
  logger_->report("\nCreate Data Flow");
  createDataFlow();

  // Create physical hierarchy tree in a post-order DFS manner
  logger_->report("\nPerform Clustering..");
  multiLevelCluster(root_cluster_);

  logger_->report("\nPrint Physical Hierachy Tree\n");
  printPhysicalHierarchyTree(root_cluster_, 0);

  //
  // Break mixed leaf clusters into a standard-cell cluster and hard-macro
  // clusters. Merge macros based on connection signatures and footprints. Based
  // on types of designs, we support two types of breaking up. Suppose current
  // cluster is A --
  // Type 1:  Replace A by A1, A2, A3
  // Type 2:  Create a subtree for A
  //          A  ->    A
  //               |  |   |
  //              A1  A2  A3
  //
  logger_->report("\nBreak mixed clusters into std cell and macro clusters.");
  leafClusterStdCellHardMacroSep(root_cluster_);

  logger_->report(
      "\nPrint Physical Hierachy Tree after splitting std cell and macros in "
      "leaf clusters\n");
  printPhysicalHierarchyTree(root_cluster_, 0);

  // Map the macros in each cluster to their HardMacro objects
  for (auto& [cluster_id, cluster] : cluster_map_) {
    mapMacroInCluster2HardMacro(cluster);
  }

  //
  // Place macros in a hierarchical mode based on the phhyscial hierarchical
  // tree
  //
  //  -- Shape Engine --
  // Calculate the shape and area of each cluster in a bottom-up approach.
  // Start by determining the macro tilings within each cluster.
  // When placing clusters at each level, we can adjust the utilization of
  // standard cells dynamically. Step 1: Establish the size of the root cluster.
  //
  SoftMacro* root_soft_macro = new SoftMacro(root_cluster_);

  // TODO: This adjustment should have been done earlier...
  //
  const float root_lx = std::max(
      dbuToMicron(block_->getCoreArea().xMin(), dbu_), global_fence_lx_);
  const float root_ly = std::max(
      dbuToMicron(block_->getCoreArea().yMin(), dbu_), global_fence_ly_);
  const float root_ux = std::min(
      dbuToMicron(block_->getCoreArea().xMax(), dbu_), global_fence_ux_);
  const float root_uy = std::min(
      dbuToMicron(block_->getCoreArea().yMax(), dbu_), global_fence_uy_);

  const float root_area = (root_ux - root_lx) * (root_uy - root_ly);
  const float root_width = root_ux - root_lx;
  const std::vector<std::pair<float, float>> root_width_list
      = {std::pair<float, float>(root_width, root_width)};

  root_soft_macro->setShapes(root_width_list, root_area);
  root_soft_macro->setWidth(root_width);  // This will set height automatically
  root_soft_macro->setX(root_lx);
  root_soft_macro->setY(root_ly);
  root_cluster_->setSoftMacro(root_soft_macro);

  //
  // Step 2: In a bottom-up approach (Post-Order DFS), determine the macro
  // tilings within each cluster.
  //
  logger_->report(
      "Determine shaping function for clusters -- Macro Tilings.\n");
  calClusterMacroTilings(root_cluster_);

  // create pin blockage for IO pins
  createPinBlockage();
  //
  // Perform macro placement in a top-down manner (pre-order DFS)
  //
  logger_->report("Perform Multilevel macro placement...");
  multiLevelMacroPlacement(root_cluster_);

  for (auto& [inst, hard_macro] : hard_macro_map_) {
    hard_macro->updateDb(pitch_x_, pitch_y_);
  }

  // Clear the memory to avoid memory leakage
  // release all the pointers
  // metrics map
  for (auto& [module, metrics] : logical_module_map_) {
    delete metrics;
  }
  logical_module_map_.clear();
  // hard macro map
  for (auto& [inst, hard_macro] : hard_macro_map_) {
    delete hard_macro;
  }
  hard_macro_map_.clear();
  // delete all clusters
  for (auto& [cluster_id, cluster] : cluster_map_) {
    delete cluster;
  }
  cluster_map_.clear();

  logger_->report("number of updated macros : {}", num_updated_macros_);
  logger_->report("number of macros in HardMacroCluster : {}",
                  num_hard_macros_cluster_);
}

////////////////////////////////////////////////////////////////////////
// Private functions
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// Hierarchical clustering related functions

//
// Traverse Logical Hierarchy
// Recursive function to collect the design metrics (number of std cells,
// area of std cells, number of macros and area of macros) in the logical
// hierarchy
//
Metrics* HierRTLMP::computeMetrics(odb::dbModule* module)
{
  unsigned int num_std_cell = 0;
  float std_cell_area = 0.0;
  unsigned int num_macro = 0;
  float macro_area = 0.0;

  for (odb::dbInst* inst : module->getInsts()) {
    const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
    if (liberty_cell == nullptr)
      continue;
    odb::dbMaster* master = inst->getMaster();
    // check if the instance is a pad or a cover macro
    if (master->isPad() || master->isCover()) {
      continue;
    }

    float inst_area = liberty_cell->area();
    if (master->isBlock()) {  // a macro
      num_macro += 1;
      macro_area += inst_area;
      // add hard macro to corresponding map
      HardMacro* macro = new HardMacro(inst, dbu_, halo_width_);
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

// compute the metrics for a cluster
// Here we do not include any Pads,  Covers or Marker
// number of standard cells
// number of macros
// area of standard cells
// area of macros
void HierRTLMP::setClusterMetrics(Cluster* cluster)
{
  unsigned int num_std_cell = 0;
  unsigned int num_macro = 0;
  float std_cell_area = 0.0;
  float macro_area = 0.0;
  num_std_cell += cluster->getLeafStdCells().size();
  num_macro += cluster->getLeafMacros().size();
  for (odb::dbInst* inst : cluster->getLeafStdCells()) {
    const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
    std_cell_area += liberty_cell->area();
  }
  for (odb::dbInst* inst : cluster->getLeafMacros()) {
    const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
    macro_area += liberty_cell->area();
  }
  Metrics metrics(num_std_cell, num_macro, std_cell_area, macro_area);
  for (auto& module : cluster->getDbModules()) {
    metrics.addMetrics(*logical_module_map_[module]);
  }

  debugPrint(logger_,
             MPL,
             "clustering",
             1,
             "Set Cluster Metrics for cluster: {}",
             cluster->getName());
  debugPrint(logger_,
             MPL,
             "clustering",
             1,
             "Num Macros: {} Num Std Cells: {}",
             metrics.getNumMacro(),
             metrics.getNumStdCell());

  // update metrics based on design type
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

//
// Handle IOs
// Map IOs to Pads for designs with IO pads
// If the design does not have IO pads,
// do nothing
//
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

//
// Create Bundled IOs in the following the order : L, T, R, B
// We will have num_bundled_IOs_ x 4 clusters for bundled IOs
// Bundled IOs are only created for pins in the region
//
void HierRTLMP::createBundledIOs()
{
  // convert from micron to dbu
  floorplan_lx_ = micronToDbu(floorplan_lx_, dbu_);
  floorplan_ly_ = micronToDbu(floorplan_ly_, dbu_);
  floorplan_ux_ = micronToDbu(floorplan_ux_, dbu_);
  floorplan_uy_ = micronToDbu(floorplan_uy_, dbu_);

  // Get the floorplan information and get the range of bundled IO regions
  odb::Rect die_box = block_->getCoreArea();
  int core_lx = die_box.xMin();
  int core_ly = die_box.yMin();
  int core_ux = die_box.xMax();
  int core_uy = die_box.yMax();
  const int x_base = (floorplan_ux_ - floorplan_lx_) / num_bundled_IOs_;
  const int y_base = (floorplan_uy_ - floorplan_ly_) / num_bundled_IOs_;
  int cluster_id_base = cluster_id_;

  // Map all the BTerms / Pads to Bundled IOs (cluster)
  std::vector<std::string> prefix_vec;
  prefix_vec.push_back(std::string("L"));
  prefix_vec.push_back(std::string("T"));
  prefix_vec.push_back(std::string("R"));
  prefix_vec.push_back(std::string("B"));
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
        x = floorplan_lx_;
        y = floorplan_ly_ + y_base * j;
        height = y_base;
      } else if (i == 1) {  // Top boundary
        x = floorplan_lx_ + x_base * j;
        y = floorplan_uy_;
        width = x_base;
      } else if (i == 2) {  // Right boundary
        x = floorplan_ux_;
        y = floorplan_uy_ - y_base * (j + 1);
        height = y_base;
      } else {  // Bottom boundary
        x = floorplan_ux_ - x_base * (j + 1);
        y = floorplan_ly_;
        width = x_base;
      }

      // set the cluster to a IO cluster
      cluster->setIOClusterFlag(
          std::pair<float, float>(dbuToMicron(x, dbu_), dbuToMicron(y, dbu_)),
          dbuToMicron(width, dbu_),
          dbuToMicron(height, dbu_));
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
        lx = floorplan_lx_;
      }
      if (ly <= core_ly) {
        ly = floorplan_ly_;
      }
      if (ux >= core_ux) {
        ux = floorplan_ux_;
      }
      if (uy >= core_uy) {
        uy = floorplan_uy_;
      }
    }
    // calculate cluster id based on the location of IO Pins / Pads
    int cluster_id = -1;
    if (lx <= floorplan_lx_) {
      // The IO is on the left boundary
      cluster_id = cluster_id_base
                   + std::floor(((ly + uy) / 2.0 - floorplan_ly_) / y_base);
    } else if (uy >= floorplan_uy_) {
      // The IO is on the top boundary
      cluster_id = cluster_id_base + num_bundled_IOs_
                   + std::floor(((lx + ux) / 2.0 - floorplan_lx_) / x_base);
    } else if (ux >= floorplan_ux_) {
      // The IO is on the right boundary
      cluster_id = cluster_id_base + num_bundled_IOs_ * 2
                   + std::floor((floorplan_uy_ - (ly + uy) / 2.0) / y_base);
    } else if (ly <= floorplan_ly_) {
      // The IO is on the bottom boundary
      cluster_id = cluster_id_base + num_bundled_IOs_ * 3
                   + std::floor((floorplan_ux_ - (lx + ux) / 2.0) / x_base);
    }

    // Check if the IO pins / Pads exist
    if (cluster_id == -1) {
      logger_->error(
          MPL,
          2,
          "Floorplan has not been initialized? Pin location error for {}.",
          term->getName());
    } else {
      odb::dbIntProperty::create(term, "cluster_id", cluster_id);
    }
    cluster_io_map[cluster_id] = true;
  }
  // convert from dbu to micron
  floorplan_lx_ = dbuToMicron(floorplan_lx_, dbu_);
  floorplan_ly_ = dbuToMicron(floorplan_ly_, dbu_);
  floorplan_ux_ = dbuToMicron(floorplan_ux_, dbu_);
  floorplan_uy_ = dbuToMicron(floorplan_uy_, dbu_);

  // delete the IO clusters that do not have any pins assigned to them
  for (auto& [cluster_id, flag] : cluster_io_map) {
    if (!flag) {
      debugPrint(logger_,
                 MPL,
                 "clustering",
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
}

// Create physical hierarchy tree in a post-order DFS manner
// Recursive call for creating the physical hierarchy tree
void HierRTLMP::multiLevelCluster(Cluster* parent)
{
  bool force_split = false;
  if (level_ == 0) {
    // check if root cluster is below the max size of a leaf cluster
    // Force create child clusters in this case
    const int leaf_cluster_size
        = max_num_inst_base_ / std::pow(coarsening_ratio_, max_num_level_ - 1);
    if (parent->getNumStdCell() < leaf_cluster_size)
      force_split = true;
    debugPrint(logger_,
               MPL,
               "clustering",
               1,
               "Force Split: root cluster size: {}, leaf cluster size: {}",
               parent->getNumStdCell(),
               leaf_cluster_size);
  }

  if (level_ >= max_num_level_) {
    return;
  }
  level_++;
  debugPrint(logger_,
             MPL,
             "clustering",
             1,
             "Parent: {}, level: {}, num macros: {}, num std cells: {}",
             parent->getName(),
             level_,
             parent->getNumMacro(),
             parent->getNumStdCell());

  // a large coarsening_ratio_ helps the clustering process converge fast
  max_num_macro_
      = max_num_macro_base_ / std::pow(coarsening_ratio_, level_ - 1);
  min_num_macro_
      = min_num_macro_base_ / std::pow(coarsening_ratio_, level_ - 1);
  max_num_inst_ = max_num_inst_base_ / std::pow(coarsening_ratio_, level_ - 1);
  min_num_inst_ = min_num_inst_base_ / std::pow(coarsening_ratio_, level_ - 1);
  // We define the tolerance to improve the robustness of our hierarchical
  // clustering
  max_num_inst_ = max_num_inst_ * (1 + tolerance_);
  min_num_inst_ = min_num_inst_ * (1 - tolerance_);
  max_num_macro_ = max_num_macro_ * (1 + tolerance_);
  min_num_macro_ = min_num_macro_ * (1 - tolerance_);
  if (min_num_macro_ <= 0) {
    min_num_macro_ = 1;
    // max_num_macro_ = min_num_macro_ * coarsening_ratio_ / 2.0;
    max_num_macro_ = min_num_macro_;
  }

  if (min_num_inst_ <= 0) {
    min_num_inst_ = 100;
    max_num_inst_ = min_num_inst_ * coarsening_ratio_ / 2.0;
  }

  if (force_split || (parent->getNumStdCell() > max_num_inst_)) {
    breakCluster(parent);   // Break the parent cluster into children clusters
    updateSubTree(parent);  // update the subtree to the physical hierarchy tree
    for (auto& child : parent->getChildren()) {
      setInstProperty(child);
    }
    // print the basic information of the children cluster
    for (auto& child : parent->getChildren()) {
      debugPrint(logger_,
                 MPL,
                 "clustering",
                 1,
                 "\tChild Cluster: {}",
                 child->getName());
      multiLevelCluster(child);
    }
  } else {
    multiLevelCluster(parent);
  }

  setInstProperty(parent);
  level_--;
}

// update the cluster_id property of insts in the cluster
// We have three types of clusters
// Note that HardMacroCluster will not have any logical modules
void HierRTLMP::setInstProperty(Cluster* cluster)
{
  int cluster_id = cluster->getId();
  ClusterType cluster_type = cluster->getClusterType();
  if (cluster_type == HardMacroCluster || cluster_type == MixedCluster) {
    for (auto& inst : cluster->getLeafMacros()) {
      odb::dbIntProperty::find(inst, "cluster_id")->setValue(cluster_id);
    }
  }

  if (cluster_type == StdCellCluster || cluster_type == MixedCluster) {
    for (auto& inst : cluster->getLeafStdCells()) {
      odb::dbIntProperty::find(inst, "cluster_id")->setValue(cluster_id);
    }
  }

  if (cluster_type == StdCellCluster) {
    for (auto& module : cluster->getDbModules()) {
      setInstProperty(module, cluster_id, false);
    }
  } else if (cluster_type == MixedCluster) {
    for (auto& module : cluster->getDbModules()) {
      setInstProperty(module, cluster_id, true);
    }
  }
}

// update the cluster_id property of insts in a logical module
// In any case, we will consider std cells in the logical modules
// If include_macro = true, we will consider the macros also.
// Otherwise, not.
void HierRTLMP::setInstProperty(odb::dbModule* module,
                                int cluster_id,
                                bool include_macro)
{
  if (include_macro) {
    for (odb::dbInst* inst : module->getInsts()) {
      odb::dbIntProperty::find(inst, "cluster_id")->setValue(cluster_id);
    }
  } else {  // only consider standard cells
    for (odb::dbInst* inst : module->getInsts()) {
      odb::dbMaster* master = inst->getMaster();
      // check if the instance is a Pad, Cover or empty block (such as marker)
      // or macro
      if (master->isPad() || master->isCover() || master->isBlock()) {
        continue;
      }
      odb::dbIntProperty::find(inst, "cluster_id")->setValue(cluster_id);
    }
  }
  for (odb::dbModInst* inst : module->getChildren()) {
    setInstProperty(inst->getMaster(), cluster_id, include_macro);
  }
}

// Break the parent cluster into children clusters
// We expand the parent cluster into a subtree based on logical
// hierarchy in a DFS manner.  During the expansion process,
// we merge small clusters in the same logical hierarchy
void HierRTLMP::breakCluster(Cluster* parent)
{
  debugPrint(
      logger_, MPL, "clustering", 1, "Dissolve Cluster: {}", parent->getName());
  //
  // Consider three different cases:
  // (a) parent is an empty cluster
  // (b) parent is a cluster corresponding to a logical module
  // (c) parent is a cluster generated by merging small clusters
  if ((parent->getLeafStdCells().size() == 0)
      && (parent->getLeafMacros().size() == 0)
      && (parent->getDbModules().size() == 0)) {
    // (a) parent is an empty cluster, do nothing
    // In the normal operation, this case should not happen
    return;
  } else if ((parent->getLeafStdCells().size() == 0)
             && (parent->getLeafMacros().size() == 0)
             && (parent->getDbModules().size() == 1)) {
    // (b) parent is a cluster corresponding to a logical module
    // For example, the root cluster_
    odb::dbModule* module = parent->getDbModules()[0];
    // Check the child logical modules
    // (b.1) if the logical module has no child logical module
    // this logical module is a leaf logical module
    // we will use the TritonPart to partition this large flat cluster
    // in the follow-up UpdateSubTree function
    if (module->getChildren().size() == 0) {
      for (odb::dbInst* inst : module->getInsts()) {
        const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
        if (liberty_cell == nullptr)
          continue;
        odb::dbMaster* master = inst->getMaster();
        // check if the instance is a Pad, Cover or empty block (such as marker)
        if (master->isPad() || master->isCover()) {
          continue;
        } else if (master->isBlock()) {
          parent->addLeafMacro(inst);
        } else {
          parent->addLeafStdCell(inst);
        }
      }
      parent->clearDbModules();  // remove module from the parent cluster
      setInstProperty(parent);
      return;
    }
    // (b.2) if the logical module has child logical modules,
    // we first model each child logical module as a cluster
    for (odb::dbModInst* child : module->getChildren()) {
      std::string cluster_name = child->getMaster()->getHierarchicalName();
      Cluster* cluster = new Cluster(cluster_id_, cluster_name, logger_);
      cluster->addDbModule(child->getMaster());
      setInstProperty(cluster);
      setClusterMetrics(cluster);
      cluster_map_[cluster_id_++] = cluster;
      // modify the physical hierarchy tree
      cluster->setParent(parent);
      parent->addChild(cluster);
    }
    // Check the glue logics
    std::string cluster_name
        = std::string("(") + parent->getName() + ")_glue_logic";
    Cluster* cluster = new Cluster(cluster_id_, cluster_name, logger_);
    for (odb::dbInst* inst : module->getInsts()) {
      const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
      if (liberty_cell == nullptr)
        continue;
      odb::dbMaster* master = inst->getMaster();
      // check if the instance is a Pad, Cover or empty block (such as marker)
      if (master->isPad() || master->isCover()) {
        continue;
      } else if (master->isBlock()) {
        cluster->addLeafMacro(inst);
      } else {
        cluster->addLeafStdCell(inst);
      }
    }
    // if the module has no meaningful glue instances
    if (cluster->getLeafStdCells().size() == 0
        && cluster->getLeafMacros().size() == 0) {
      delete cluster;
    } else {
      setInstProperty(cluster);
      setClusterMetrics(cluster);
      cluster_map_[cluster_id_++] = cluster;
      // modify the physical hierarchy tree
      cluster->setParent(parent);
      parent->addChild(cluster);
    }
  } else {
    // (c) parent is a cluster generated by merging small clusters
    // parent cluster has few logical modules or many glue insts
    for (auto& module : parent->getDbModules()) {
      std::string cluster_name = module->getHierarchicalName();
      Cluster* cluster = new Cluster(cluster_id_, cluster_name, logger_);
      cluster->addDbModule(module);
      setInstProperty(cluster);
      setClusterMetrics(cluster);
      cluster_map_[cluster_id_++] = cluster;
      // modify the physical hierachy tree
      cluster->setParent(parent);
      parent->addChild(cluster);
    }
    // Check glue logics
    if ((parent->getLeafStdCells().size() > 0)
        || (parent->getLeafMacros().size() > 0)) {
      std::string cluster_name
          = std::string("(") + parent->getName() + ")_glue_logic";
      Cluster* cluster = new Cluster(cluster_id_, cluster_name, logger_);
      for (auto& inst : parent->getLeafStdCells()) {
        cluster->addLeafStdCell(inst);
      }
      for (auto& inst : parent->getLeafMacros()) {
        cluster->addLeafMacro(inst);
      }
      setInstProperty(cluster);
      setClusterMetrics(cluster);
      cluster_map_[cluster_id_++] = cluster;
      // modify the physical hierachy tree
      cluster->setParent(parent);
      parent->addChild(cluster);
    }
  }

  // Recursively break down large clusters with logical modules
  // For large flat cluster, we will break it in the UpdateSubTree function
  for (auto& child : parent->getChildren()) {
    if (child->getDbModules().size() > 0) {
      if (child->getNumStdCell() > max_num_inst_
          || child->getNumMacro() > max_num_macro_) {
        breakCluster(child);
      }
    }
  }

  //
  // Merge small clusters
  std::vector<Cluster*> candidate_clusters;
  for (auto& cluster : parent->getChildren()) {
    if (!cluster->getIOClusterFlag() && cluster->getNumStdCell() < min_num_inst_
        && cluster->getNumMacro() < min_num_macro_) {
      candidate_clusters.push_back(cluster);
    }
  }

  mergeClusters(candidate_clusters);

  // Update the cluster_id
  // This is important to maintain the clustering results
  setInstProperty(parent);
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
  if (candidate_clusters.size() <= 0)
    return;

  int merge_iter = 0;
  debugPrint(
      logger_, MPL, "clustering", 1, "Merge Cluster Iter: {}", merge_iter++);
  for (auto& cluster : candidate_clusters) {
    debugPrint(logger_,
               MPL,
               "clustering",
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
          "clustering",
          1,
          "Candidate cluster: {} - {}",
          candidate_clusters[i]->getName(),
          (cluster_id != -1 ? cluster_map_[cluster_id]->getName() : "   "));
      if (cluster_id != -1 && !cluster_map_[cluster_id]->getIOClusterFlag()) {
        Cluster*& cluster = cluster_map_[cluster_id];
        bool delete_flag = false;
        if (cluster->mergeCluster(*candidate_clusters[i], delete_flag)) {
          if (delete_flag) {
            cluster_map_.erase(candidate_clusters[i]->getId());
            delete candidate_clusters[i];
          }
          setInstProperty(cluster);
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
              setInstProperty(candidate_clusters[i]);
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
                || candidate_clusters[i]->getNumStdCell()
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
              setInstProperty(candidate_clusters[i]);
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

    debugPrint(
        logger_, MPL, "clustering", 1, "Merge Cluster Iter: {}", merge_iter++);
    for (auto& cluster : candidate_clusters) {
      debugPrint(
          logger_, MPL, "clustering", 1, "Cluster: {}", cluster->getName());
    }
    // merge small clusters
    if (candidate_clusters.size() == 0) {
      break;
    }
  }
  debugPrint(logger_, MPL, "clustering", 1, "Finish merging clusters");
}

// Calculate Connections between clusters
void HierRTLMP::calculateConnection()
{
  // Initialize the connections of all clusters
  for (auto& [cluster_id, cluster] : cluster_map_) {
    cluster->initConnection();
  }

  // Traverse all nets through OpenDB
  for (odb::dbNet* net : block_->getNets()) {
    // ignore all the power net
    if (net->getSigType().isSupply()) {
      continue;
    }
    int driver_id = -1;         // cluster id of the driver instance
    std::vector<int> loads_id;  // cluster id of sink instances
    bool pad_flag = false;
    // check the connected instances
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
      if (liberty_cell == nullptr)
        continue;
      odb::dbMaster* master = inst->getMaster();
      // check if the instance is a Pad, Cover or empty block (such as marker)
      // We ignore nets connecting Pads, Covers, or markers
      if (master->isPad() || master->isCover()) {
        pad_flag = true;
        break;
      }
      const int cluster_id
          = odb::dbIntProperty::find(inst, "cluster_id")->getValue();
      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        driver_id = cluster_id;
      } else {
        loads_id.push_back(cluster_id);
      }
    }
    if (pad_flag) {
      continue;  // the nets with Pads should be ignored
    }
    bool io_flag = false;
    // check the connected IO pins
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const int cluster_id
          = odb::dbIntProperty::find(bterm, "cluster_id")->getValue();
      io_flag = true;
      if (bterm->getIoType() == odb::dbIoType::INPUT) {
        driver_id = cluster_id;
      } else {
        loads_id.push_back(cluster_id);
      }
    }
    // add the net to connections between clusters
    if (driver_id != -1 && loads_id.size() > 0
        && loads_id.size() < large_net_threshold_) {
      const float weight = io_flag == true ? virtual_weight_ : 1.0;
      for (int i = 0; i < loads_id.size(); i++) {
        if (loads_id[i]
            != driver_id) {  // we model the connections as undirected edges
          cluster_map_[driver_id]->addConnection(loads_id[i], weight);
          cluster_map_[loads_id[i]]->addConnection(driver_id, weight);
        }
      }  // end sinks
    }    // end adding current net
  }      // end net traversal
}

// Create Dataflow Information
// model each std cell instance, IO pin and macro pin as vertices
void HierRTLMP::createDataFlow()
{
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
    const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
    if (liberty_cell == nullptr)
      continue;
    odb::dbMaster* master = inst->getMaster();
    // check if the instance is a Pad, Cover or a block
    // We ignore nets connecting Pads, Covers
    // for blocks, we iterate over the block pins
    if (master->isPad() || master->isCover() || master->isBlock()) {
      continue;
    }

    // mark sequential instances
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
             "dataflow",
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
    bool pad_flag = false;
    // check the connected instances
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
      if (liberty_cell == nullptr)
        continue;
      odb::dbMaster* master = inst->getMaster();
      // check if the instance is a Pad, Cover or empty block (such as marker)
      // We ignore nets connecting Pads, Covers, or markers
      if (master->isPad() || master->isCover()) {
        pad_flag = true;
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
    if (pad_flag) {
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
    if (driver_id < 0 || loads_id.size() < 1
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

  debugPrint(logger_, MPL, "dataflow", 1, "Created hypergraph");

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
    io_ffs_conn_map_.push_back(
        std::pair<odb::dbBTerm*, std::vector<std::set<odb::dbInst*>>>(src_pin,
                                                                      insts));
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
    macro_ffs_conn_map_.push_back(
        std::pair<odb::dbITerm*, std::vector<std::set<odb::dbInst*>>>(
            src_pin, std_cells));
    macro_macro_conn_map_.push_back(
        std::pair<odb::dbITerm*, std::vector<std::set<odb::dbInst*>>>(src_pin,
                                                                      macros));
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
    const int driver_id
        = odb::dbIntProperty::find(bterm, "cluster_id")->getValue();
    for (int i = 0; i < max_num_ff_dist_; i++) {
      const float weight = dataflow_weight_ / std::pow(dataflow_factor_, i);
      std::set<int> sink_clusters;
      for (auto& inst : insts[i]) {
        const int cluster_id
            = odb::dbIntProperty::find(inst, "cluster_id")->getValue();
        sink_clusters.insert(cluster_id);
      }
      for (auto& sink : sink_clusters) {
        cluster_map_[driver_id]->addConnection(sink, weight);
        cluster_map_[sink]->addConnection(driver_id, weight);
      }  // add weight
    }    // number of ffs
  }

  // macros to ffs
  for (const auto& [iterm, insts] : macro_ffs_conn_map_) {
    const int driver_id
        = odb::dbIntProperty::find(iterm->getInst(), "cluster_id")->getValue();
    for (int i = 0; i < max_num_ff_dist_; i++) {
      const float weight = dataflow_weight_ / std::pow(dataflow_factor_, i);
      std::set<int> sink_clusters;
      for (auto& inst : insts[i]) {
        const int cluster_id
            = odb::dbIntProperty::find(inst, "cluster_id")->getValue();
        sink_clusters.insert(cluster_id);
      }
      for (auto& sink : sink_clusters) {
        cluster_map_[driver_id]->addConnection(sink, weight);
        cluster_map_[sink]->addConnection(driver_id, weight);
      }  // add weight
    }    // number of ffs
  }

  // macros to macros
  for (const auto& [iterm, insts] : macro_macro_conn_map_) {
    const int driver_id
        = odb::dbIntProperty::find(iterm->getInst(), "cluster_id")->getValue();
    for (int i = 0; i < max_num_ff_dist_; i++) {
      const float weight = dataflow_weight_ / std::pow(dataflow_factor_, i);
      std::set<int> sink_clusters;
      for (auto& inst : insts[i]) {
        const int cluster_id
            = odb::dbIntProperty::find(inst, "cluster_id")->getValue();
        sink_clusters.insert(cluster_id);
      }
      for (auto& sink : sink_clusters) {
        cluster_map_[driver_id]->addConnection(sink, weight);
      }  // add weight
    }    // number of ffs
  }
}

// Print Connnection For all the clusters
void HierRTLMP::printConnection()
{
  std::string line = "";
  line += "NUM_CLUSTERS  :   " + std::to_string(cluster_map_.size()) + "\n";
  for (auto& [cluster_id, cluster] : cluster_map_) {
    const std::map<int, float> connections = cluster->getConnection();
    if (connections.size() == 0) {
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
  std::string line = "";
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
    if (cluster->getChildren().size() == 0) {
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
// A flat cluster does not have a logical module
void HierRTLMP::breakLargeFlatCluster(Cluster* parent)
{
  // Check if the cluster is a large flat cluster
  if (parent->getDbModules().size() > 0
      || parent->getLeafStdCells().size() < max_num_inst_) {
    return;
  }
  // set the instance property
  setInstProperty(parent);
  std::map<int, int> cluster_vertex_id_map;
  std::map<odb::dbInst*, int> inst_vertex_id_map;
  const int parent_cluster_id = parent->getId();
  std::vector<odb::dbInst*> std_cells = parent->getLeafStdCells();
  std::vector<std::vector<int>> hyperedges;
  std::vector<float> vertex_weight;
  // vertices
  // other clusters behaves like fixed vertices
  // We do not consider vertices only between fixed vertices
  int vertex_id = 0;
  for (auto& [cluster_id, cluster] : cluster_map_) {
    cluster_vertex_id_map[cluster_id] = vertex_id++;
    vertex_weight.push_back(0.0f);
  }
  for (auto& macro : parent->getLeafMacros()) {
    inst_vertex_id_map[macro] = vertex_id++;
    const sta::LibertyCell* liberty_cell = network_->libertyCell(macro);
    vertex_weight.push_back(liberty_cell->area());
  }
  int num_fixed_vertices
      = vertex_id;  // we do not consider these vertices in later process
                    // They behaves like ''fixed vertices''
  for (auto& std_cell : std_cells) {
    inst_vertex_id_map[std_cell] = vertex_id++;
    const sta::LibertyCell* liberty_cell = network_->libertyCell(std_cell);
    vertex_weight.push_back(liberty_cell->area());
  }
  // Traverse nets to create hyperedges
  for (odb::dbNet* net : block_->getNets()) {
    // ignore all the power net
    if (net->getSigType().isSupply()) {
      continue;
    }
    int driver_id = -1;      // vertex id of the driver instance
    std::set<int> loads_id;  // vertex id of the sink instances
    bool pad_flag = false;
    // check the connected instances
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
      if (liberty_cell == nullptr)
        continue;
      odb::dbMaster* master = inst->getMaster();
      // check if the instance is a Pad, Cover or empty block (such as marker)
      // if the nets connects to such pad, cover or empty block,
      // we should ignore such net
      if (master->isPad() || master->isCover()) {
        pad_flag = true;
        break;  // here CAN NOT be continue
      }
      const int cluster_id
          = odb::dbIntProperty::find(inst, "cluster_id")->getValue();
      int vertex_id = (cluster_id != parent_cluster_id)
                          ? cluster_vertex_id_map[cluster_id]
                          : inst_vertex_id_map[inst];
      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        driver_id = vertex_id;
      } else {
        loads_id.insert(vertex_id);
      }
    }
    // ignore the nets with IO pads
    if (pad_flag) {
      continue;
    }
    // check the connected IO pins
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const int cluster_id
          = odb::dbIntProperty::find(bterm, "cluster_id")->getValue();
      if (bterm->getIoType() == odb::dbIoType::INPUT) {
        driver_id = cluster_vertex_id_map[cluster_id];
      } else {
        loads_id.insert(cluster_vertex_id_map[cluster_id]);
      }
    }
    loads_id.insert(driver_id);
    // add the net as a hyperedge
    if (driver_id != -1 && loads_id.size() > 1
        && loads_id.size() < large_net_threshold_) {
      std::vector<int> hyperedge;
      hyperedge.insert(hyperedge.end(), loads_id.begin(), loads_id.end());
      hyperedges.push_back(hyperedge);
    }
  }

  const int seed = 0;
  const float balance_constraint = 5.0;
  const int num_vertices = vertex_weight.size();
  const int num_hyperedges = hyperedges.size();

  std::vector<int> part = tritonpart_->TritonPart2Way(num_vertices,
                                                      num_hyperedges,
                                                      hyperedges,
                                                      vertex_weight,
                                                      balance_constraint,
                                                      seed);

  // create cluster based on partitioning solutions
  // Note that all the std cells are stored in the leaf_std_cells_ for a flat
  // cluster
  parent->clearLeafStdCells();
  // we follow binary coding method to differentiate different parts
  // of the cluster
  // cluster_name_0, cluster_name_1
  // cluster_name_0_0, cluster_name_0_1, cluster_name_1_0, cluster_name_1_1
  const std::string cluster_name = parent->getName();
  // set the parent cluster for part 0
  // update the name of parent cluster
  parent->setName(cluster_name + std::string("_0"));
  // create a new cluster for part 1
  Cluster* cluster_part_1
      = new Cluster(cluster_id_, cluster_name + std::string("_1"), logger_);
  // we do not need to touch the fixed vertices (they have been assigned before)
  for (int i = num_fixed_vertices; i < num_vertices; i++) {
    if (part[i] == 0) {
      parent->addLeafStdCell(std_cells[i - num_fixed_vertices]);
    } else {
      cluster_part_1->addLeafStdCell(std_cells[i - num_fixed_vertices]);
    }
  }
  // update the property of parent cluster
  setInstProperty(parent);
  setClusterMetrics(parent);
  // update the property of cluster_part_1
  setInstProperty(cluster_part_1);
  setClusterMetrics(cluster_part_1);
  cluster_map_[cluster_id_++] = cluster_part_1;
  cluster_part_1->setParent(parent->getParent());
  parent->getParent()->addChild(cluster_part_1);
  // Recursive break the cluster
  // until the size of the cluster is less than max_num_inst_
  breakLargeFlatCluster(parent);
  breakLargeFlatCluster(cluster_part_1);
}

// Traverse the physical hierarchy tree in a DFS manner
// Split macros and std cells in the leaf clusters
// In the normal operation, we call this function after
// creating the physical hierarchy tree
void HierRTLMP::leafClusterStdCellHardMacroSep(Cluster* root_cluster)
{
  if (root_cluster->getChildren().size() == 0
      || root_cluster->getNumMacro() == 0) {
    return;
  }

  // Traverse the physical hierarchy tree in a DFS manner (post-order)
  std::vector<Cluster*> leaf_clusters;
  for (auto& child : root_cluster->getChildren()) {
    setInstProperty(child);
    if (child->getNumMacro() > 0) {
      if (child->getChildren().size() == 0) {
        leaf_clusters.push_back(child);
      } else {
        leafClusterStdCellHardMacroSep(child);
      }
    } else {
      child->setClusterType(StdCellCluster);
    }
  }

  // after this step, the std cells and macros for leaf cluster has been
  // seperated. We need to update the metrics of the design if necessary
  // for each leaf clusters with macros, first group macros based on
  // connection signatures and macro sizes
  for (auto& cluster : leaf_clusters) {
    // based on the type of current cluster,
    Cluster* parent_cluster = cluster;  // std cell dominated cluster
    // add a subtree if the cluster is macro dominated cluster
    // based macro_dominated_cluster_threshold_ (defualt = 0.01)
    if (cluster->getNumStdCell() * macro_dominated_cluster_threshold_
        < cluster->getNumMacro()) {
      parent_cluster = cluster->getParent();  // replacement
    }

    // Map macros into the HardMacro objects
    // and get all the HardMacros
    mapMacroInCluster2HardMacro(cluster);
    std::vector<HardMacro*> hard_macros = cluster->getHardMacros();

    // create a cluster for each macro
    std::vector<Cluster*> macro_clusters;
    for (auto& hard_macro : hard_macros) {
      std::string cluster_name = hard_macro->getName();
      Cluster* macro_cluster = new Cluster(cluster_id_, cluster_name, logger_);
      macro_cluster->addLeafMacro(hard_macro->getInst());
      setInstProperty(macro_cluster);
      setClusterMetrics(macro_cluster);
      cluster_map_[cluster_id_++] = macro_cluster;
      // modify the physical hierachy tree
      macro_cluster->setParent(parent_cluster);
      parent_cluster->addChild(macro_cluster);
      macro_clusters.push_back(macro_cluster);
    }

    // classify macros based on size
    std::vector<int> macro_size_class(hard_macros.size(), -1);
    for (int i = 0; i < hard_macros.size(); i++) {
      if (macro_size_class[i] == -1) {
        for (int j = i + 1; j < hard_macros.size(); j++) {
          if ((macro_size_class[j] == -1)
              && ((*hard_macros[i]) == (*hard_macros[j]))) {
            macro_size_class[j] = i;
          }
        }
      }
    }

    for (int i = 0; i < hard_macros.size(); i++) {
      macro_size_class[i]
          = (macro_size_class[i] == -1) ? i : macro_size_class[i];
    }

    // classify macros based on connection signature
    calculateConnection();
    std::vector<int> macro_signature_class(hard_macros.size(), -1);
    for (int i = 0; i < hard_macros.size(); i++) {
      if (macro_signature_class[i] == -1) {
        macro_signature_class[i] = i;
        for (int j = i + 1; j < hard_macros.size(); j++) {
          if (macro_signature_class[j] != -1) {
            continue;
          }
          bool flag = macro_clusters[i]->isSameConnSignature(
              *macro_clusters[j], signature_net_threshold_);
          if (flag) {
            macro_signature_class[j] = i;
          }
        }
      }
    }

    // print the connnection signature
    for (auto& cluster : macro_clusters) {
      debugPrint(logger_,
                 MPL,
                 "clustering",
                 1,
                 "\nMacro Signature: {}",
                 cluster->getName());
      for (auto& [cluster_id, weight] : cluster->getConnection()) {
        debugPrint(logger_,
                   MPL,
                   "clustering",
                   1,
                   " {} {} ",
                   cluster_map_[cluster_id]->getName(),
                   weight);
      }
    }
    // macros with the same size and the same connection signature
    // belong to the same class
    std::vector<int> macro_class(hard_macros.size(), -1);
    for (int i = 0; i < hard_macros.size(); i++) {
      if (macro_class[i] == -1) {
        macro_class[i] = i;
        for (int j = i + 1; j < hard_macros.size(); j++) {
          if (macro_class[j] == -1 && macro_size_class[i] == macro_size_class[j]
              && macro_signature_class[i] == macro_signature_class[j]) {
            macro_class[j] = i;
            debugPrint(logger_,
                       MPL,
                       "clustering",
                       1,
                       "merge {} with {}",
                       macro_clusters[i]->getName(),
                       macro_clusters[j]->getName());
            bool delete_flag = false;
            macro_clusters[i]->mergeCluster(*macro_clusters[j], delete_flag);
            if (delete_flag) {
              // remove the merged macro cluster
              cluster_map_.erase(macro_clusters[j]->getId());
              delete macro_clusters[j];
            }
          }
        }
      }
    }

    // clear the hard macros in current leaf cluster
    cluster->clearHardMacros();
    // Restore the structure of physical hierarchical tree
    // Thus the order of leaf clusters will not change the final
    // macro grouping results (This is very important !!!
    // Don't touch the next line SetInstProperty command!!!)
    setInstProperty(cluster);

    std::vector<int> virtual_conn_clusters;
    // Never use SetInstProperty Command in following lines
    // for above reason!!!
    // based on different types of designs, we handle differently
    // whether a replacement or a subtree
    // Deal with the standard cell cluster
    if (parent_cluster == cluster) {
      // If we need a subtree , add standard cell cluster
      std::string std_cell_cluster_name = cluster->getName();
      Cluster* std_cell_cluster
          = new Cluster(cluster_id_, std_cell_cluster_name, logger_);
      std_cell_cluster->copyInstances(*cluster);
      std_cell_cluster->clearLeafMacros();
      std_cell_cluster->setClusterType(StdCellCluster);
      setClusterMetrics(std_cell_cluster);
      cluster_map_[cluster_id_++] = std_cell_cluster;
      // modify the physical hierachy tree
      std_cell_cluster->setParent(parent_cluster);
      parent_cluster->addChild(std_cell_cluster);
      virtual_conn_clusters.push_back(std_cell_cluster->getId());
    } else {
      // If we need add a replacement
      cluster->clearLeafMacros();
      cluster->setClusterType(StdCellCluster);
      setClusterMetrics(cluster);
      virtual_conn_clusters.push_back(cluster->getId());
      // In this case, we do not to modify the physical hierarchy tree
    }
    // Deal with macro clusters
    for (int i = 0; i < macro_class.size(); i++) {
      if (macro_class[i] != i) {
        continue;  // this macro cluster has been merged
      }
      macro_clusters[i]->setClusterType(HardMacroCluster);
      setClusterMetrics(macro_clusters[i]);
      virtual_conn_clusters.push_back(cluster->getId());
    }

    // add virtual connections
    for (int i = 0; i < virtual_conn_clusters.size(); i++) {
      for (int j = i + 1; j < virtual_conn_clusters.size(); j++) {
        parent_cluster->addVirtualConnection(virtual_conn_clusters[i],
                                             virtual_conn_clusters[j]);
      }
    }
  }
  // Set the inst property back
  setInstProperty(root_cluster);
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
    const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
    if (liberty_cell == nullptr)
      continue;
    odb::dbMaster* master = inst->getMaster();
    // check if the instance is a pad or empty block (such as marker)
    if (master->isPad() || master->isCover()) {
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

//
// Print Physical Hierarchy tree
//
void HierRTLMP::printPhysicalHierarchyTree(Cluster* parent, int level)
{
  std::string line;
  for (int i = 0; i < level; i++) {
    line += "+---";
  }
  line += fmt::format(
      "{}  ({})  num_macro :  {}   num_std_cell :  {}"
      "  macro_area :  {}  std_cell_area : {}",
      parent->getName(),
      parent->getId(),
      parent->getNumMacro(),
      parent->getNumStdCell(),
      parent->getMacroArea(),
      parent->getStdCellArea());
  logger_->report("{}\n", line);
  int tot_num_macro_in_children = 0;
  for (auto& cluster : parent->getChildren()) {
    tot_num_macro_in_children += cluster->getNumMacro();
  }

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

/////////////////////////////////////////////////////////////////////////////
// Macro Placement related functions
// Determine the macro tilings within each cluster in a bottom-up manner.
// (Post-Order DFS manner)
// Coarse coarsening:  In this step, we only consider the size of macros
// Ignore all the standard-cell clusters.
// At this stage, we assume the standard cell placer will automatically
// place standard cells in the empty space between macros.
//
void HierRTLMP::calClusterMacroTilings(Cluster* parent)
{
  // base case, no macros in current cluster
  if (parent->getNumMacro() == 0) {
    return;
  }
  // Current cluster is a hard macro cluster
  if (parent->getClusterType() == HardMacroCluster) {
    calHardMacroClusterShape(parent);
    return;
  }
  // Mixed size cluster
  // recursively visit the children
  for (auto& cluster : parent->getChildren()) {
    if (cluster->getNumMacro() > 0) {
      calClusterMacroTilings(cluster);
    }
  }
  // if the current cluster is the root cluster,
  // the shape is fixed, i.e., the fixed die.
  // Thus, we do not need to determine the shapes for it
  // if (parent->getParent() == nullptr)
  //  return;
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
  // call simulated annealing to determine tilings
  std::set<std::pair<float, float>> macro_tilings;  // <width, height>
  // the probability of all actions should be summed to 1.0.
  const float action_sum = pos_swap_prob_ + neg_swap_prob_ + double_swap_prob_
                           + exchange_swap_prob_ + resize_prob_;
  // get outline constraint
  // Here we use the floorplan size as the outline constraint
  const float outline_width = root_cluster_->getWidth();
  const float outline_height = root_cluster_->getHeight();
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
      const float width = outline_width * vary_factor_list[run_id++];
      const float height = outline_height;
      SACoreSoftMacro* sa = new SACoreSoftMacro(
          width,
          height,
          macros,
          1.0,
          1000.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,  // penalty weight
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
          k_,  // This will be replaced by min_temperature later
          c_,
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
      for (auto& sa : sa_vector) {
        threads.push_back(std::thread(runSA<SACoreSoftMacro>, sa));
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    // add macro tilings
    for (auto& sa : sa_vector) {
      sa_containers.push_back(sa);
      if (sa->isValid(outline_width, outline_height) == true) {
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
      const float width = outline_width;
      const float height = outline_height * vary_factor_list[run_id++];
      SACoreSoftMacro* sa = new SACoreSoftMacro(
          width,
          height,
          macros,
          1.0,
          1000.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,  // penalty weight
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
          k_,  // later this will be replaced by min_temperature
          c_,
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
      for (auto& sa : sa_vector) {
        threads.push_back(std::thread(runSA<SACoreSoftMacro>, sa));
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    // add macro tilings
    for (auto& sa : sa_vector) {
      sa_containers.push_back(sa);
      if (sa->isValid(outline_width, outline_height) == true) {
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
               "shaping",
               1,
               "width: {}, height: {}, aspect_ratio: {}, min_ar: {}",
               shape.first,
               shape.second,
               shape.second / shape.first,
               min_ar_);
  }
  // we do not want very strange tilngs if we have choices
  std::vector<std::pair<float, float>> new_tilings;
  for (auto& tiling : tilings) {
    if (tiling.second / tiling.first >= min_ar_
        && tiling.second / tiling.first <= 1.0 / min_ar_) {
      new_tilings.push_back(tiling);
    }
  }
  // if there are vaild tilings
  if (new_tilings.size() > 0)
    tilings = new_tilings;
  // update parent
  parent->setMacroTilings(tilings);
  if (tilings.size() == 0) {
    logger_->error(MPL,
                   3,
                   "There are no valid tilings for mixed cluster: {}",
                   parent->getName());
  } else {
    std::string line
        = "[CalClusterMacroTilings] The macro tiling for mixed cluster "
          + parent->getName() + "  ";
    for (auto& shape : tilings) {
      line += " < " + std::to_string(shape.first) + " , ";
      line += std::to_string(shape.second) + " >  ";
    }
    line += "\n";
    debugPrint(logger_, MPL, "shaping", 1, "{}", line);
  }
}

//
// Determine the macro tilings for each HardMacroCluster
// multi thread enabled
// random seed deterministic enabled
void HierRTLMP::calHardMacroClusterShape(Cluster* cluster)
{
  // Check if the cluster is a HardMacroCluster
  if (cluster->getClusterType() != HardMacroCluster) {
    return;
  }

  debugPrint(logger_,
             MPL,
             "shaping",
             1,
             "Calculate the possible shapes for hard macro cluster : {}",
             cluster->getName());
  std::vector<HardMacro*> hard_macros = cluster->getHardMacros();
  // macro tilings
  std::set<std::pair<float, float>> macro_tilings;  // <width, height>
  // if the cluster only has one macro
  if (hard_macros.size() == 1) {
    float width = hard_macros[0]->getWidth();
    float height = hard_macros[0]->getHeight();
    std::vector<std::pair<float, float>> tilings;
    tilings.push_back(std::pair<float, float>(width, height));
    cluster->setMacroTilings(tilings);
    return;
  }
  // otherwise call simulated annealing to determine tilings
  // set the action probabilities
  const float action_sum = pos_swap_prob_ + neg_swap_prob_ + double_swap_prob_
                           + exchange_swap_prob_;
  // get outline constraint
  // In this stage, we use the floorplan size as the outline
  const float outline_width = root_cluster_->getWidth();
  const float outline_height = root_cluster_->getHeight();
  // update macros
  std::vector<HardMacro> macros;
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
      const float width = outline_width * vary_factor_list[run_id++];
      const float height = outline_height;
      SACoreHardMacro* sa = new SACoreHardMacro(
          width,
          height,
          macros,
          1.0,     // area_weight_
          1000.0,  // boundary weight_
          0.0,
          0.0,
          0.0,  // penalty weight
          pos_swap_prob_ / action_sum,
          neg_swap_prob_ / action_sum,
          double_swap_prob_ / action_sum,
          exchange_swap_prob_ / action_sum,
          0.0,  // no flip
          init_prob_,
          max_num_step_,
          num_perturb_per_step,
          k_,  // later this will be replaced by min_temperature
          c_,
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
      for (auto& sa : sa_vector) {
        threads.push_back(std::thread(runSA<SACoreHardMacro>, sa));
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    // add macro tilings
    for (auto& sa : sa_vector) {
      sa_containers.push_back(sa);
      if (sa->isValid(outline_width, outline_height) == true) {
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
      const float width = outline_width;
      const float height = outline_height * vary_factor_list[run_id++];
      SACoreHardMacro* sa
          = new SACoreHardMacro(width,
                                height,
                                macros,
                                1.0,  // area_weight_
                                1000.0,
                                0.0,
                                0.0,
                                0.0,  // penalty weight
                                pos_swap_prob_ / action_sum,
                                neg_swap_prob_ / action_sum,
                                double_swap_prob_ / action_sum,
                                exchange_swap_prob_ / action_sum,
                                0.0,
                                init_prob_,
                                max_num_step_,
                                num_perturb_per_step,
                                k_,
                                c_,
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
      for (auto& sa : sa_vector) {
        threads.push_back(std::thread(runSA<SACoreHardMacro>, sa));
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    // add macro tilings
    for (auto& sa : sa_vector) {
      sa_containers.push_back(sa);
      if (sa->isValid(outline_width, outline_height) == true) {
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
               "shaping",
               1,
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
  tilings = new_tilings;
  // update parent
  cluster->setMacroTilings(tilings);
  if (tilings.size() == 0) {
    std::string line
        = "[CalHardMacroClusterShape] This is no valid tilings for "
          "hard macro cluster ";
    line += cluster->getName();
    logger_->error(MPL,
                   4,
                   "This no valid tilings for hard macro cluser: {}",
                   cluster->getName());
  }
}

//
// Create pin blockage for Bundled IOs
// Each pin blockage is a macro only blockage
//
void HierRTLMP::createPinBlockage()
{
  logger_->report("Creating the pin blockage for root cluster");
  floorplan_lx_ = root_cluster_->getX();
  floorplan_ly_ = root_cluster_->getY();
  floorplan_ux_ = floorplan_lx_ + root_cluster_->getWidth();
  floorplan_uy_ = floorplan_ly_ + root_cluster_->getHeight();
  // if the design has IO pads, we do not create pin blockage
  if (io_pad_map_.size() > 0) {
    return;
  }
  // Get the initial tilings
  const std::vector<std::pair<float, float>> tilings
      = root_cluster_->getMacroTilings();
  // When the program enter stage, the tilings cannot be empty
  const float ar = root_cluster_->getHeight() / root_cluster_->getWidth();
  float max_height = std::sqrt(tilings[0].first * tilings[0].second * ar);
  float max_width = max_height / ar;
  // convert to the limit to the depth of pin access
  max_width = ((floorplan_ux_ - floorplan_lx_) - max_width);
  max_height = ((floorplan_uy_ - floorplan_ly_) - max_height);
  // the area of standard-cell clusters
  float std_cell_area = 0.0;
  for (auto& cluster : root_cluster_->getChildren()) {
    if (cluster->getClusterType() == StdCellCluster) {
      std_cell_area += cluster->getArea();
    }
  }
  // Then we check the range of IO spans
  std::map<PinAccess, std::pair<float, float>> pin_ranges;
  pin_ranges[L] = std::pair<float, float>(floorplan_uy_, floorplan_ly_);
  pin_ranges[T] = std::pair<float, float>(floorplan_ux_, floorplan_lx_);
  pin_ranges[R] = std::pair<float, float>(floorplan_uy_, floorplan_ly_);
  pin_ranges[B] = std::pair<float, float>(floorplan_ux_, floorplan_lx_);
  int floorplan_lx = micronToDbu(floorplan_lx_, dbu_);
  // int floorplan_ly = micronToDbu(floorplan_ly_, dbu_);
  int floorplan_ux = micronToDbu(floorplan_ux_, dbu_);
  int floorplan_uy = micronToDbu(floorplan_uy_, dbu_);
  for (auto term : block_->getBTerms()) {
    int lx = std::numeric_limits<int>::max();
    int ly = std::numeric_limits<int>::max();
    int ux = 0;
    int uy = 0;
    // If the design with IO pads, these block terms
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

    // modify the pin ranges
    if (lx <= floorplan_lx) {
      pin_ranges[L].first
          = std::min(pin_ranges[L].first, dbuToMicron(ly, dbu_));
      pin_ranges[L].second
          = std::max(pin_ranges[L].second, dbuToMicron(uy, dbu_));
    } else if (uy >= floorplan_uy) {
      pin_ranges[T].first
          = std::min(pin_ranges[T].first, dbuToMicron(lx, dbu_));
      pin_ranges[T].second
          = std::max(pin_ranges[T].second, dbuToMicron(ux, dbu_));
    } else if (ux >= floorplan_ux) {
      pin_ranges[R].first
          = std::min(pin_ranges[R].first, dbuToMicron(ly, dbu_));
      pin_ranges[R].second
          = std::max(pin_ranges[R].second, dbuToMicron(uy, dbu_));
    } else {
      pin_ranges[B].first
          = std::min(pin_ranges[B].first, dbuToMicron(lx, dbu_));
      pin_ranges[B].second
          = std::max(pin_ranges[B].second, dbuToMicron(ux, dbu_));
    }
  }
  // calculate the depth based on area
  float sum_length = 0.0;
  int num_hor_access = 0;
  int num_ver_access = 0;
  for (auto& [pin_access, length] : pin_ranges) {
    if (length.second > length.first) {
      sum_length += std::abs(length.second - length.first);
      if (pin_access == R || pin_access == L) {
        num_hor_access++;
      } else {
        num_ver_access++;
      }
    }
  }
  max_width = num_hor_access > 0 ? max_width / num_hor_access : max_width;
  max_height = num_ver_access > 0 ? max_height / num_ver_access : max_height;
  const float depth = std_cell_area / sum_length;
  debugPrint(logger_, MPL, "macro_placement", 1, "Pin access for root cluster");
  // add the blockages into the macro_blockages_
  if (pin_ranges[L].second > pin_ranges[L].first) {
    macro_blockages_.push_back(Rect(floorplan_lx_,
                                    pin_ranges[L].first,
                                    floorplan_lx_ + std::min(max_width, depth),
                                    pin_ranges[L].second));
    debugPrint(logger_,
               MPL,
               "macro_placement",
               1,
               "Pin access for L : length : {}, depth :  {}",
               pin_ranges[L].second - pin_ranges[L].first,
               std::min(max_width, depth));
  }
  if (pin_ranges[T].second > pin_ranges[T].first) {
    macro_blockages_.push_back(Rect(pin_ranges[T].first,
                                    floorplan_uy_ - std::min(max_height, depth),
                                    pin_ranges[T].second,
                                    floorplan_uy_));
    debugPrint(logger_,
               MPL,
               "macro_placement",
               1,
               "Pin access for T : length : {}, depth : {}",
               pin_ranges[T].second - pin_ranges[T].first,
               std::min(max_height, depth));
  }
  if (pin_ranges[R].second > pin_ranges[R].first) {
    macro_blockages_.push_back(Rect(floorplan_ux_ - std::min(max_width, depth),
                                    pin_ranges[R].first,
                                    floorplan_ux_,
                                    pin_ranges[R].second));
    debugPrint(logger_,
               MPL,
               "macro_placement",
               1,
               "Pin access for R : length : {}, depth : {}",
               pin_ranges[R].second - pin_ranges[R].first,
               std::min(max_width, depth));
  }
  if (pin_ranges[B].second > pin_ranges[B].first) {
    macro_blockages_.push_back(
        Rect(pin_ranges[B].first,
             floorplan_ly_,
             pin_ranges[B].second,
             floorplan_ly_ + std::min(max_height, depth)));
    debugPrint(logger_,
               MPL,
               "macro_placement",
               1,
               "Pin access for B : length : {}, depth : {}",
               pin_ranges[B].second - pin_ranges[B].first,
               std::min(max_height, depth));
  }
}

//
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
//
void HierRTLMP::multiLevelMacroPlacement(Cluster* parent)
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
    hardMacroClusterMacroPlacement(parent);
    return;
  }

  bool boundary_weight_updated_flag = false;
  const float original_boundary_weight = boundary_weight_;
  if (update_boundary_weight_ == true) {
    // check if this is the last level
    boundary_weight_updated_flag = true;
    for (auto& cluster : parent->getChildren()) {
      if (cluster->getChildren().size() > 0) {
        boundary_weight_updated_flag = false;
        break;
      }
    }
    if (boundary_weight_updated_flag == true)
      boundary_weight_ = original_boundary_weight * 2.0;
  }

  logger_->report("boundary_weight_updated_flag = {},  boundary_weight_ = {}",
                  boundary_weight_updated_flag,
                  boundary_weight_);

  // set the instance property
  for (auto& cluster : parent->getChildren()) {
    setInstProperty(cluster);
  }
  // Place children clusters
  // map children cluster to soft macro
  for (auto& cluster : parent->getChildren()) {
    if (cluster->getIOClusterFlag())  // ignore all the io clusters
      continue;
    SoftMacro* macro = new SoftMacro(cluster);
    // no memory leakage, beacuse we set the soft macro, the old one
    // will be deleted
    cluster->setSoftMacro(macro);
  }
  // detemine the settings for simulated annealing engine
  // Set outline information
  const float lx = parent->getX();
  const float ly = parent->getY();
  const float outline_width = parent->getWidth();
  const float outline_height = parent->getHeight();
  const float ux = lx + outline_width;
  const float uy = ly + outline_height;
  logger_->report(
      "[MultiLevelMacroPlacement] Working on children of cluster: {}, Outline "
      "{}, {}  {}, {}",
      parent->getName(),
      lx,
      ly,
      outline_width,
      outline_height);

  // Suppose the region, fence, guide has been mapped to cooresponding macros
  // This step is done when we enter the Hier-RTLMP program
  std::map<std::string, int> soft_macro_id_map;  // cluster_name, macro_id
  std::map<int, Rect> fences;
  std::map<int, Rect> guides;
  std::vector<SoftMacro> macros;
  std::vector<BundledNet> nets;
  std::vector<Rect> blockages;        // general placement blockage
  std::vector<Rect> macro_blockages;  // placement blockage for macros
  //
  // Calculate the parts of blockages that have overlapped with current parent
  // cluster.  All the blockages are converted to hard macros with fences. Here
  // the blockage is the placement blockage. The placement blockage prevents
  // both macros and standard-cells to overlap the placement blockage
  //
  for (auto& blockage : blockages_) {
    // calculate the overlap between blockage and parent cluster
    const float b_lx = std::max(lx, blockage.xMin());
    const float b_ly = std::max(ly, blockage.yMin());
    const float b_ux = std::min(ux, blockage.xMax());
    const float b_uy = std::min(uy, blockage.yMax());
    if ((b_ux - b_lx > 0.0) && (b_uy - b_ly > 0.0)) {  // blockage exists
      blockages.push_back(Rect(b_lx - lx, b_ly - ly, b_ux - lx, b_uy - ly));
    }
  }
  // macro blockage
  for (auto& blockage : macro_blockages_) {
    // calculate the overlap between blockage and parent cluster
    const float b_lx = std::max(lx, blockage.xMin());
    const float b_ly = std::max(ly, blockage.yMin());
    const float b_ux = std::min(ux, blockage.xMax());
    const float b_uy = std::min(uy, blockage.yMax());
    if ((b_ux - b_lx > 0.0) && (b_uy - b_ly > 0.0)) {  // blockage exists
      macro_blockages.push_back(
          Rect(b_lx - lx, b_ly - ly, b_ux - lx, b_uy - ly));
    }
  }
  // Each cluster is modeled as Soft Macro
  // The fences or guides for each cluster is created by merging
  // the fences and guides for hard macros in each cluster
  for (auto& cluster : parent->getChildren()) {
    // for IO cluster
    if (cluster->getIOClusterFlag()) {
      soft_macro_id_map[cluster->getName()] = macros.size();
      macros.push_back(SoftMacro(
          std::pair<float, float>(cluster->getX() - lx, cluster->getY() - ly),
          cluster->getName(),
          cluster->getWidth(),
          cluster->getHeight(),
          cluster));
      continue;
    }
    // for other clusters
    soft_macro_id_map[cluster->getName()] = macros.size();
    SoftMacro* soft_macro = new SoftMacro(cluster);
    setInstProperty(cluster);  // we need this step to calculate nets
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
    fence.relocate(lx, ly, ux, uy);  // calculate the overlap with outline
    guide.relocate(lx, ly, ux, uy);  // calculate the overlap with outline
    if (fence.isValid()) {
      // current macro id is macros.size() - 1
      fences[macros.size() - 1] = fence;
    }
    if (guide.isValid()) {
      // current macro id is macros.size() - 1
      guides[macros.size() - 1] = guide;
    }
  }

  // update the connnection
  calculateConnection();
  debugPrint(logger_,
             MPL,
             "macro_placement",
             1,
             "[MultiLevelMacroPlacement] Finish calculating connection");
  debugPrint(logger_,
             MPL,
             "macro_placement",
             1,
             "[MultiLevelMacroPlacement] Finish updating dataflow");
  // Handle the pin access
  // Get the connections between pin accesses
  if (parent->getParent() != nullptr) {
    // the parent cluster is not the root cluster
    // First step model each pin access as the a dummy softmacro (width = 0.0,
    // height = 0.0) In our simulated annealing engine, the dummary softmacro
    // will no effect on SA We have four dummy SoftMacros based on our
    // definition
    std::vector<PinAccess> pins = {L, T, R, B};
    for (auto& pin : pins) {
      soft_macro_id_map[toString(pin)] = macros.size();
      macros.push_back(SoftMacro(0.0, 0.0, toString(pin)));
    }
    // add the connections between pin accesses, for example, L to R
    for (auto& [src_pin, pin_map] : parent->getBoundaryConnection()) {
      for (auto& [target_pin, weight] : pin_map) {
        nets.push_back(BundledNet(soft_macro_id_map[toString(src_pin)],
                                  soft_macro_id_map[toString(target_pin)],
                                  weight));
      }
    }
  }
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
                 "macro_placement",
                 1,
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

    // determine the width of each pin access
    logger_->report(
        "[MultiLevelMacroPlacement] number of tracks per micro in "
        "total :  {}",
        1.0 / pin_access_net_width_ratio_);
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
      if (tiling.first <= outline_width && tiling.second <= outline_height) {
        max_width = std::max(max_width, tiling.first);
        max_height = std::max(max_height, tiling.second);
      }
    }
    max_width = std::min(max_width, outline_width);
    max_height = std::min(max_height, outline_height);
    debugPrint(logger_,
               MPL,
               "macro_placement",
               1,
               "[MultiLevelMacroPlacement] Pin access calculation "
               "max_width: {}, max_height : {}",
               max_width,
               max_height);
    l_size = std::min(l_size, max_height);
    r_size = std::min(r_size, max_height);
    t_size = std::min(t_size, max_width);
    b_size = std::min(b_size, max_width);
    // determine the height of each pin access
    max_width = outline_width - max_width;
    max_height = outline_height - max_height;
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
          = Rect(0.0, 0.0, temp_width, outline_height);
      logger_->report("[MultiLevelMacroPlacement] L width : {}, height : {}",
                      l_size,
                      temp_width);
    }
    if (r_size > 0) {
      const float temp_width = std::min(max_width, depth);
      macros[soft_macro_id_map[toString(R)]]
          = SoftMacro(temp_width, r_size, toString(R));
      fences[soft_macro_id_map[toString(R)]] = Rect(
          outline_width - temp_width, 0.0, outline_width, outline_height);
      logger_->report("[MultiLevelMacroPlacement] R width : {}, height : {}",
                      r_size,
                      temp_width);
    }
    if (t_size > 0) {
      const float temp_height = std::min(max_height, depth);
      macros[soft_macro_id_map[toString(T)]]
          = SoftMacro(t_size, temp_height, toString(T));
      fences[soft_macro_id_map[toString(T)]] = Rect(
          0.0, outline_height - temp_height, outline_width, outline_height);
      logger_->report("[MultiLevelMacroPlacement] T width : {}, height : {}",
                      t_size,
                      temp_height);
    }
    if (b_size > 0) {
      const float temp_height = std::min(max_height, depth);
      macros[soft_macro_id_map[toString(B)]]
          = SoftMacro(b_size, temp_height, toString(B));
      fences[soft_macro_id_map[toString(B)]]
          = Rect(0.0, 0.0, outline_width, temp_height);
      logger_->report("[MultiLevelMacroPlacement] B width : {}, height : {}",
                      b_size,
                      temp_height);
    }
  }

  // Write the connections between macros
  std::ofstream file;
  std::string file_name = parent->getName();
  for (int i = 0; i < file_name.size(); i++) {
    if (file_name[i] == '/') {
      file_name[i] = '*';
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
  int run_thread = num_threads_;
  int remaining_runs = target_util_list.size();
  int run_id = 0;
  SACoreSoftMacro* best_sa = nullptr;
  std::vector<SACoreSoftMacro*>
      sa_containers;  // store all the SA runs to avoid memory leakage
  float best_cost = std::numeric_limits<float>::max();
  debugPrint(logger_,
             MPL,
             "macro_placement",
             1,
             "[MultiLevelMacroPlacement] Start Simulated Annealing Core");
  while (remaining_runs > 0) {
    std::vector<SACoreSoftMacro*> sa_vector;
    run_thread
        = (remaining_runs > num_threads_) ? num_threads_ : remaining_runs;
    if (graphics_) {
      run_thread = 1;
    }
    for (int i = 0; i < run_thread; i++) {
      std::vector<SoftMacro> shaped_macros = macros;  // copy for multithread
      // determine the shape for each macro
      debugPrint(logger_,
                 MPL,
                 "macro_plamcement",
                 1,
                 "Start Simulated Annealing (run_id = {})",
                 run_id);
      const float target_util = target_util_list[run_id];
      const float target_dead_space = target_dead_space_list[run_id++];
      if (!shapeChildrenCluster(parent,
                                shaped_macros,
                                soft_macro_id_map,
                                target_util,
                                target_dead_space)) {
        logger_->report(
            "[MultiLevelMacroPlacement] Cannot generate feasible "
            "shapes for children of cluster {}\n"
            "sa_id = {}, target_util = {}, target_dead_space = {}",
            parent->getName(),
            run_id,
            target_util,
            target_dead_space);
        continue;
      }
      debugPrint(logger_,
                 MPL,
                 "macro_placement",
                 1,
                 "[MultiLevelMacroPlacement] Finish generating shapes for "
                 "children of cluster {}\n"
                 "sa_id = {}, target_util = {}, target_dead_space = {}",
                 parent->getName(),
                 run_id,
                 target_util,
                 target_dead_space);
      // Note that all the probabilities are normalized to the summation of 1.0.
      // Note that the weight are not necessaries summarized to 1.0, i.e., not
      // normalized.
      SACoreSoftMacro* sa = new SACoreSoftMacro(
          outline_width,
          outline_height,
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
          k_,  // we will not use FastSA (k_, c_ will be replaced by
               // min_temperature later) [20221219]
          c_,
          random_seed_,
          graphics_.get(),
          logger_);
      sa->setFences(fences);
      sa->setGuides(guides);
      sa->setNets(nets);
      sa->setBlockages(blockages);
      sa->setBlockages(macro_blockages);
      sa_vector.push_back(sa);
    }
    if (sa_vector.size() == 1) {
      runSA<SACoreSoftMacro>(sa_vector[0]);
    } else {
      // multi threads
      std::vector<std::thread> threads;
      for (auto& sa : sa_vector) {
        threads.push_back(std::thread(runSA<SACoreSoftMacro>, sa));
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    // add macro tilings
    for (auto& sa : sa_vector) {
      sa_containers.push_back(sa);  // add SA to containers
      if (sa->isValid() && sa->getNormCost() < best_cost) {
        best_cost = sa->getNormCost();
        best_sa = sa;
      }
    }
    sa_vector.clear();
    // add early stop mechanism
    if (best_sa != nullptr) {
      break;
    }
    remaining_runs -= run_thread;
  }
  debugPrint(logger_,
             MPL,
             "macro_placement",
             1,
             "[MultiLevelMacroPlacement] Finish Simulated Annealing Core");
  if (best_sa == nullptr) {
    //
    // Error condition, print all the results
    //
    logger_->report(
        "[MultiLevelMacroPlacement] Simulated Annealing Summary for "
        "cluster {}",
        parent->getName());
    for (auto i = 0; i < sa_containers.size(); i++) {
      logger_->report(
          "[MultiLevelMacroPlacement] sa_id = {}, target_util = {}, "
          " target_dead_space = {}",
          i,
          target_util_list[i],
          target_dead_space_list[i]);
      sa_containers[i]->printResults();
    }
    logger_->error(MPL,
                   5,
                   "[MultiLevelMacroPlacement] Failed on cluster: {}",
                   parent->getName());
  }
  best_sa->alignMacroClusters();
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
      macro_blockages.push_back(Rect(0.0, l_ly, l_width, l_ly + l_height));
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
      macro_blockages.push_back(
          Rect(outline_width - r_width, r_ly, outline_width, r_ly + r_height));
      shaped_macros[soft_macro_id_map[toString(R)]]
          = SoftMacro(std::pair<float, float>(outline_width, r_ly),
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
      macro_blockages.push_back(Rect(
          t_lx, outline_height - t_height, t_lx + t_width, outline_height));
      shaped_macros[soft_macro_id_map[toString(T)]]
          = SoftMacro(std::pair<float, float>(t_lx, outline_height),
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
      macro_blockages.push_back(Rect(b_lx, 0.0, b_lx + b_width, b_height));
      shaped_macros[soft_macro_id_map[toString(B)]]
          = SoftMacro(std::pair<float, float>(b_lx, 0.0),
                      toString(B),
                      b_width,
                      0.0,
                      nullptr);
    }
    macros = shaped_macros;
    run_thread = num_threads_;
    remaining_runs = target_util_list.size();
    run_id = 0;
    best_sa = nullptr;
    sa_containers.clear();
    best_cost = std::numeric_limits<float>::max();
    logger_->report(
        "[MultiLevelMacroPlacement] Start Simulated Annealing Core");
    while (remaining_runs > 0) {
      std::vector<SACoreSoftMacro*> sa_vector;
      run_thread
          = (remaining_runs > num_threads_) ? num_threads_ : remaining_runs;
      if (graphics_) {
        run_thread = 1;
      }
      for (int i = 0; i < run_thread; i++) {
        std::vector<SoftMacro> shaped_macros = macros;  // copy for multithread
        // determine the shape for each macro
        debugPrint(logger_,
                   MPL,
                   "macro_placement",
                   1,
                   "[MultiLevelMacroPlacement] Start "
                   "Simulated Annealing (run_id = {})",
                   run_id);
        const float target_util = target_util_list[run_id];
        const float target_dead_space = target_dead_space_list[run_id++];
        if (!shapeChildrenCluster(parent,
                                  shaped_macros,
                                  soft_macro_id_map,
                                  target_util,
                                  target_dead_space)) {
          logger_->report(
              "[MultiLevelMacroPlacement] "
              "Cannot generate feasible shapes for children of cluster {}\n"
              "sa_id = {}, target_util = {}, target_dead_space = {}",
              parent->getName(),
              run_id,
              target_util,
              target_dead_space);
          continue;
        }
        debugPrint(logger_,
                   MPL,
                   "macro_placement",
                   1,
                   "[MultiLevelMacroPlacement] "
                   "Finish generating shapes for children of cluster: {}\n"
                   "sa_id = {}, target_util = {}, target_dead_space = {}",
                   parent->getName(),
                   run_id,
                   target_util,
                   target_dead_space);
        // Note that all the probabilities are normalized to the summation
        // of 1.0. Note that the weight are not necessaries summarized to 1.0,
        // i.e., not normalized.
        SACoreSoftMacro* sa = new SACoreSoftMacro(
            outline_width,
            outline_height,
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
            k_,  // we will not use FastSA (k_, c_ will be replaced by
                 // min_temperature later) [20221219]
            c_,
            random_seed_,
            graphics_.get(),
            logger_);
        sa->setFences(fences);
        sa->setGuides(guides);
        sa->setNets(nets);
        sa->setBlockages(blockages);
        sa->setBlockages(macro_blockages);
        sa_vector.push_back(sa);
      }
      if (sa_vector.size() == 1) {
        runSA<SACoreSoftMacro>(sa_vector[0]);
      } else {
        // multi threads
        std::vector<std::thread> threads;
        for (auto& sa : sa_vector) {
          threads.push_back(std::thread(runSA<SACoreSoftMacro>, sa));
        }
        for (auto& th : threads) {
          th.join();
        }
      }
      // add macro tilings
      for (auto& sa : sa_vector) {
        sa_containers.push_back(sa);  // add SA to containers
        if (sa->isValid() && sa->getNormCost() < best_cost) {
          best_cost = sa->getNormCost();
          best_sa = sa;
        }
      }
      sa_vector.clear();
      // add early stop mechanism
      if (best_sa != nullptr) {
        break;
      }
      remaining_runs -= run_thread;
    }
    debugPrint(logger_,
               MPL,
               "macro_placement",
               1,
               "[MultiLevelMacroPlacement] Finish Simulated Annealing Core");
    if (best_sa == nullptr) {
      // print all the results
      logger_->report("Simulated Annealing Summary for cluster {}",
                      parent->getName());
      for (auto i = 0; i < sa_containers.size(); i++) {
        logger_->report("sa_id = {}, target_util = {}, target_dead_space = {}",
                        i,
                        target_util_list[i],
                        target_dead_space_list[i]);
        sa_containers[i]->printResults();
      }
      logger_->error(MPL, 6, "SA failed on cluster: {}", parent->getName());
    }
    best_sa->alignMacroClusters();
    best_sa->fillDeadSpace();
    // update the clusters and do bus planning
    best_sa->getMacros(shaped_macros);
  }

  debugPrint(
      logger_,
      MPL,
      "macro_placement",
      1,
      "[MultiLevelMacroPlacement] Finish Simulated Annealing for cluster: {}",
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

  // update the shapes and locations for children clusters based on simulated
  // annealing results
  for (auto& cluster : parent->getChildren()) {
    *(cluster->getSoftMacro())
        = shaped_macros[soft_macro_id_map[cluster->getName()]];
  }
  // check if the parent cluster still need bus planning
  for (auto& child : parent->getChildren()) {
    if (child->getClusterType() == MixedCluster) {
      logger_->report("\nCall Bus Planning for cluster: {}\n",
                      parent->getName());
      // Call Path Synthesis to route buses
      callBusPlanning(shaped_macros, nets);
      break;
    }
  }
  // update the location of children cluster to their real location
  for (auto& cluster : parent->getChildren()) {
    cluster->setX(cluster->getX() + lx);
    cluster->setY(cluster->getY() + ly);
  }

  // if (parent->getParent() != nullptr)
  //  return;
  if (boundary_weight_updated_flag == true) {
    boundary_weight_ = original_boundary_weight;
  }

  // Traverse the physical hierarchy tree in a DFS manner
  for (auto& cluster : parent->getChildren()) {
    if (cluster->getClusterType() == MixedCluster
        || cluster->getClusterType() == HardMacroCluster) {
      multiLevelMacroPlacement(cluster);
    }
  }

  // align macros
  alignHardMacroGlobal(parent);
  // delete SA containers to avoid memory leakage
  sa_containers.clear();
  // done this branch and update the cluster_id property back
  setInstProperty(parent);
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
  nets = merged_nets;
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
bool HierRTLMP::shapeChildrenCluster(
    Cluster* parent,
    std::vector<SoftMacro>& macros,
    std::map<std::string, int>& soft_macro_id_map,
    float target_util,
    float target_dead_space)
{
  debugPrint(logger_,
             MPL,
             "macro_placement",
             1,
             "Starting ShapeChildrenCluster for cluster :  {}",
             parent->getName());
  // check the valid values
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
    if (cluster->getIOClusterFlag()) {
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
      if (shapes.size() == 0) {
        logger_->error(
            MPL,
            7,
            "Not enough space in cluster: {} for child hard macro cluster: {}",
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
      if (shapes.size() == 0) {
        std::string line
            = "[ShapeChildrenCluster] There is not enough space in cluster ";
        line += parent->getName() + " ";
        line += "for child mixed cluster: " + cluster->getName();
        logger_->error(
            MPL,
            8,
            "Not enough space in cluster: {} for child mixed cluster: {}",
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
    logger_->report(
        "This is no valid shaping solution for children clusters of cluster "
        "{}, "
        "avail_space = {}, min_target_util = {}",
        parent->getName(),
        avail_space,
        min_target_util);
    return false;
  }

  // set the shape for each macro
  for (auto& cluster : parent->getChildren()) {
    if (cluster->getIOClusterFlag()) {
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
      width_list.push_back(std::pair<float, float>(width, area / width));
      macros[soft_macro_id_map[cluster->getName()]].setShapes(width_list, area);
    } else if (cluster->getClusterType() == HardMacroCluster) {
      macros[soft_macro_id_map[cluster->getName()]].setShapes(
          cluster->getMacroTilings());
      debugPrint(logger_,
                 MPL,
                 "macro_placement",
                 1,
                 "  [ShapeChildrenCluster] hard_macro_cluster : {}",
                 cluster->getName());
      for (auto& shape : cluster->getMacroTilings()) {
        debugPrint(logger_,
                   MPL,
                   "macro_placement",
                   1,
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
          width_list.push_back(
              std::pair<float, float>(shape.first, area / shape.second));
        }
      }

      debugPrint(logger_,
                 MPL,
                 "macro_placement",
                 1,
                 "[ShapeChildrenCluster] name:  {} area: {}",
                 cluster->getName(),
                 area);
      debugPrint(logger_, MPL, "macro_placement", 1, "width_list :  ");
      for (auto& width : width_list) {
        debugPrint(logger_,
                   MPL,
                   "macro_placement",
                   1,
                   " [  {} {}  ] ",
                   width.first,
                   width.second);
      }
      macros[soft_macro_id_map[cluster->getName()]].setShapes(width_list, area);
    }
  }
  debugPrint(logger_,
             MPL,
             "macro_placement",
             1,
             "Finish ShapeChildrenCluster for cluster :  {}",
             parent->getName());
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
    logger_->error(MPL, 9, "Fail !!! Bus planning error !!!");
  }
}

// place macros within the HardMacroCluster
void HierRTLMP::hardMacroClusterMacroPlacement(Cluster* cluster)
{
  // Check if the cluster is a HardMacroCluster
  if (cluster->getClusterType() != HardMacroCluster) {
    return;
  }
  logger_->report(
      "\n[Hier-RTLMP::HardMacroClusterMacroPlacement] Place macros in cluster: "
      "{}",
      cluster->getName());
  // get outline constraint
  const float lx = cluster->getX();
  const float ly = cluster->getY();
  const float outline_width = cluster->getWidth();
  const float outline_height = cluster->getHeight();
  const float ux = lx + outline_width;
  const float uy = ly + outline_height;
  // the macros for Simulated Annealing Core
  std::vector<HardMacro> macros;
  // the clusters for each hard macro.
  // We need this to calculate the connections with other clusters
  std::vector<Cluster*> macro_clusters;
  std::map<int, int> cluster_id_macro_id_map;
  std::vector<HardMacro*> hard_macros = cluster->getHardMacros();
  // we define to verify that all the macros has been placed
  num_hard_macros_cluster_ += hard_macros.size();
  // calculate the fences and guides
  std::map<int, Rect> fences;
  std::map<int, Rect> guides;
  // create a cluster for each macro
  // and calculate the fences and guides
  for (auto& hard_macro : hard_macros) {
    int macro_id = macros.size();
    std::string cluster_name = hard_macro->getName();
    Cluster* macro_cluster = new Cluster(cluster_id_, cluster_name, logger_);
    macro_cluster->addLeafMacro(hard_macro->getInst());
    setInstProperty(macro_cluster);
    cluster_id_macro_id_map[cluster_id_] = macro_id;
    if (fences_.find(hard_macro->getName()) != fences_.end()) {
      fences[macro_id] = fences_[hard_macro->getName()];
      fences[macro_id].relocate(lx, ly, ux, uy);
    }
    if (guides_.find(hard_macro->getName()) != guides_.end()) {
      guides[macro_id] = guides_[hard_macro->getName()];
      guides[macro_id].relocate(lx, ly, ux, uy);
    }
    macros.push_back(*hard_macro);
    cluster_map_[cluster_id_++] = macro_cluster;
    macro_clusters.push_back(macro_cluster);
  }
  // calculate the connections with other clusters
  calculateConnection();
  std::set<int> cluster_id_set;
  for (auto macro_cluster : macro_clusters) {
    for (auto [cluster_id, weight] : macro_cluster->getConnection()) {
      cluster_id_set.insert(cluster_id);
    }
  }
  // create macros for other clusters
  for (auto cluster_id : cluster_id_set) {
    if (cluster_id_macro_id_map.find(cluster_id)
        != cluster_id_macro_id_map.end()) {
      continue;
    }
    auto& temp_cluster = cluster_map_[cluster_id];
    // model other cluster as a fixed macro with zero size
    cluster_id_macro_id_map[cluster_id] = macros.size();
    macros.push_back(HardMacro(
        std::pair<float, float>(
            temp_cluster->getX() + temp_cluster->getWidth() / 2.0 - lx,
            temp_cluster->getY() + temp_cluster->getHeight() / 2.0 - ly),
        temp_cluster->getName()));
  }
  // create bundled net
  std::vector<BundledNet> nets;
  for (auto macro_cluster : macro_clusters) {
    const int src_id = macro_cluster->getId();
    for (auto [cluster_id, weight] : macro_cluster->getConnection()) {
      BundledNet net(cluster_id_macro_id_map[src_id],
                     cluster_id_macro_id_map[cluster_id],
                     weight);
      net.src_cluster_id = src_id;
      net.target_cluster_id = cluster_id;
      nets.push_back(net);
    }  // end connection
  }    // end macro cluster
  // set global configuration
  // set the action probabilities (summation to 1.0)
  const float action_sum = pos_swap_prob_ * 10 + neg_swap_prob_ * 10
                           + double_swap_prob_ + exchange_swap_prob_
                           + flip_prob_;
  // We vary the outline of cluster to generate differnt tilings
  std::vector<float> vary_factor_list{1.0};
  float vary_step
      = 0.0 / num_runs_;  // change the outline by at most 10 percent
  for (int i = 1; i <= num_runs_ / 2 + 1; i++) {
    vary_factor_list.push_back(1.0 + i * vary_step);
    vary_factor_list.push_back(1.0 - i * vary_step);
  }
  const int num_perturb_per_step = (macros.size() > num_perturb_per_step_ / 10)
                                       ? macros.size()
                                       : num_perturb_per_step_ / 10;
  int run_thread = num_threads_;
  int remaining_runs = num_runs_;
  int run_id = 0;
  SACoreHardMacro* best_sa = nullptr;
  std::vector<SACoreHardMacro*> sa_containers;  // store all the SA runs
  float best_cost = std::numeric_limits<float>::max();
  while (remaining_runs > 0) {
    std::vector<SACoreHardMacro*> sa_vector;
    run_thread
        = (remaining_runs > num_threads_) ? num_threads_ : remaining_runs;
    if (graphics_) {
      run_thread = 1;
    }
    for (int i = 0; i < run_thread; i++) {
      // change the aspect ratio
      const float width = outline_width * vary_factor_list[run_id++];
      const float height = outline_width * outline_height / width;
      SACoreHardMacro* sa
          = new SACoreHardMacro(width,
                                height,
                                macros,
                                area_weight_,
                                outline_weight_ * (i + 1) * 10,
                                wirelength_weight_ / (i + 1),
                                guidance_weight_,
                                fence_weight_,
                                pos_swap_prob_ * 10 / action_sum,
                                neg_swap_prob_ * 10 / action_sum,
                                double_swap_prob_ / action_sum,
                                exchange_swap_prob_ / action_sum,
                                flip_prob_ / action_sum,
                                init_prob_,
                                max_num_step_,
                                num_perturb_per_step,
                                k_,  // later will be updated to min_temperature
                                c_,
                                random_seed_ + run_id,
                                graphics_.get(),
                                logger_);
      sa->setNets(nets);
      sa->setFences(fences);
      sa->setGuides(guides);
      sa_vector.push_back(sa);
    }
    if (sa_vector.size() == 1) {
      runSA<SACoreHardMacro>(sa_vector[0]);
    } else {
      // multi threads
      std::vector<std::thread> threads;
      for (auto& sa : sa_vector) {
        threads.push_back(std::thread(runSA<SACoreHardMacro>, sa));
      }
      for (auto& th : threads) {
        th.join();
      }
    }
    // add macro tilings
    for (auto& sa : sa_vector) {
      sa_containers.push_back(sa);  // add SA to containers
      if (sa->isValid(outline_width, outline_height)
          && sa->getNormCost() < best_cost) {
        best_cost = sa->getNormCost();
        best_sa = sa;
      }
    }
    sa_vector.clear();
    remaining_runs -= run_thread;
  }
  debugPrint(
      logger_,
      MPL,
      "macro_placement",
      1,
      "[HierRTLMP:HardMacroClusterMacroPlacement] Summary of macro placement "
      "for cluster {}",
      cluster->getName());
  // update the hard macro
  if (best_sa == nullptr) {
    for (auto& sa : sa_containers) {
      sa->printResults();
      // need
    }
    sa_containers.clear();
    logger_->error(
        MPL,
        10,
        "Cannot find valid macro placement for hard macro cluster: {}",
        cluster->getName());
  } else {
    std::vector<HardMacro> best_macros;
    best_sa->getMacros(best_macros);
    best_sa->printResults();
    for (int i = 0; i < hard_macros.size(); i++) {
      *(hard_macros[i]) = best_macros[i];
    }
  }
  // update OpenDB
  for (auto& hard_macro : hard_macros) {
    num_updated_macros_++;
    hard_macro->setX(hard_macro->getX() + lx);
    hard_macro->setY(hard_macro->getY() + ly);
    // hard_macro->updateDb(pitch_x_, pitch_y_);
  }
  // clean SA to avoid memory leakage
  sa_containers.clear();
  setInstProperty(cluster);
}

// Align all the macros globally to reduce the waste of standard cell space
void HierRTLMP::alignHardMacroGlobal(Cluster* parent)
{
  logger_->report("Align macros within the cluster {}", parent->getName());
  // get the floorplan information
  const odb::Rect core_box = block_->getCoreArea();
  int core_lx = core_box.xMin();
  int core_ly = core_box.yMin();
  int core_ux = core_box.xMax();
  int core_uy = core_box.yMax();
  // if the current parent cluster is not the root cluster
  if (parent->getParent() != nullptr) {
    core_lx = micronToDbu(parent->getX(), dbu_);
    core_ly = micronToDbu(parent->getY(), dbu_);
    core_ux = micronToDbu(parent->getX() + parent->getWidth(), dbu_);
    core_uy = micronToDbu(parent->getY() + parent->getHeight(), dbu_);
  }

  std::vector<HardMacro*> hard_macros = parent->getHardMacros();
  int boundary_v_th = std::numeric_limits<int>::max();
  int boundary_h_th = std::numeric_limits<int>::max();
  for (auto& macro_inst : hard_macros) {
    boundary_h_th = std::min(
        boundary_h_th, static_cast<int>(macro_inst->getRealWidthDBU() * 1.5));
    boundary_v_th = std::min(
        boundary_v_th, static_cast<int>(macro_inst->getRealHeightDBU() * 1.5));
  }
  // const int notch_v_th = std::min(micronToDbu(notch_v_th_, dbu_),
  // boundary_v_th); const int notch_h_th = std::min(micronToDbu(notch_h_th_,
  // dbu_), boundary_h_th);
  const int notch_v_th = boundary_v_th;
  const int notch_h_th = boundary_h_th;
  // const int notch_v_th = micronToDbu(notch_v_th_, dbu_) + boundary_v_th;
  // const int notch_h_th = micronToDbu(notch_h_th_, dbu_) + boundary_h_th;
  logger_->report("boundary_h_th : {}, boundary_v_th : {}",
                  dbuToMicron(boundary_h_th, dbu_),
                  dbuToMicron(boundary_v_th, dbu_));
  logger_->report("notch_h_th : {}, notch_v_th : {}",
                  dbuToMicron(notch_h_th, dbu_),
                  dbuToMicron(notch_v_th, dbu_));
  // define lamda function for check if the move is allowed
  auto isValidMove = [&](size_t macro_id) {
    // check if the macro can fit into the core area
    const int macro_lx = hard_macros[macro_id]->getRealXDBU();
    const int macro_ly = hard_macros[macro_id]->getRealYDBU();
    const int macro_ux = hard_macros[macro_id]->getRealUXDBU();
    const int macro_uy = hard_macros[macro_id]->getRealUYDBU();
    if (macro_lx < core_lx || macro_ly < core_ly || macro_ux > core_ux
        || macro_uy > core_uy)
      return false;
    // check if there is some overlap with other macros
    for (auto i = 0; i < hard_macros.size(); i++) {
      if (i == macro_id)
        continue;
      const int lx = hard_macros[i]->getRealXDBU();
      const int ly = hard_macros[i]->getRealYDBU();
      const int ux = hard_macros[i]->getRealUXDBU();
      const int uy = hard_macros[i]->getRealUYDBU();
      if (macro_lx >= ux || macro_ly >= uy || macro_ux <= lx || macro_uy <= ly)
        continue;
      else
        return false;  // there is some overlap with others
    }
    return true;  // this move is valid
  };
  // define lamda function for move a hard macro horizontally and vertically
  auto moveHor = [&](size_t macro_id, int x) {
    const int x_old = hard_macros[macro_id]->getXDBU();
    hard_macros[macro_id]->setXDBU(x);
    if (isValidMove(macro_id) == false) {
      hard_macros[macro_id]->setXDBU(x_old);
      return false;
    }
    return true;
  };

  auto moveVer = [&](size_t macro_id, int y) {
    const int y_old = hard_macros[macro_id]->getYDBU();
    hard_macros[macro_id]->setYDBU(y);
    if (isValidMove(macro_id) == false) {
      hard_macros[macro_id]->setYDBU(y_old);
      return false;
    }
    return true;
  };

  // Align macros with the corresponding boundaries
  // follow the order of left, top, right, bottom
  // left boundary
  for (auto j = 0; j < hard_macros.size(); j++) {
    if (std::abs(hard_macros[j]->getXDBU() - core_lx) < boundary_h_th) {
      moveHor(j, core_lx);
    }
  }
  // top boundary
  for (auto j = 0; j < hard_macros.size(); j++) {
    if (std::abs(hard_macros[j]->getUYDBU() - core_uy) < boundary_v_th) {
      moveVer(j, core_uy - hard_macros[j]->getHeightDBU());
    }
  }
  // right boundary
  for (auto j = 0; j < hard_macros.size(); j++) {
    if (std::abs(hard_macros[j]->getUXDBU() - core_ux) < boundary_h_th) {
      moveHor(j, core_ux - hard_macros[j]->getWidthDBU());
    }
  }
  // bottom boundary
  for (auto j = 0; j < hard_macros.size(); j++) {
    if (std::abs(hard_macros[j]->getUYDBU() - core_ly) < boundary_v_th) {
      moveVer(j, core_ly);
    }
  }

  // Comparator function to sort pairs according to second value
  auto LessOrEqualX = [&](std::pair<size_t, std::pair<int, int>>& a,
                          std::pair<size_t, std::pair<int, int>>& b) {
    if (a.second.first < b.second.first)
      return true;
    else if (a.second.first == b.second.first)
      return a.second.second < b.second.second;
    else
      return false;
  };

  auto LargeOrEqualX = [&](std::pair<size_t, std::pair<int, int>>& a,
                           std::pair<size_t, std::pair<int, int>>& b) {
    if (a.second.first > b.second.first)
      return true;
    else if (a.second.first == b.second.first)
      return a.second.second > b.second.second;
    else
      return false;
  };

  auto LessOrEqualY = [&](std::pair<size_t, std::pair<int, int>>& a,
                          std::pair<size_t, std::pair<int, int>>& b) {
    if (a.second.second < b.second.second)
      return true;
    else if (a.second.second == b.second.second)
      return a.second.first > b.second.first;
    else
      return false;
  };

  auto LargeOrEqualY = [&](std::pair<size_t, std::pair<int, int>>& a,
                           std::pair<size_t, std::pair<int, int>>& b) {
    if (a.second.second > b.second.second)
      return true;
    else if (a.second.second == b.second.second)
      return a.second.first < b.second.first;
    else
      return false;
  };

  std::queue<size_t> macro_queue;
  std::vector<size_t> macro_list;
  std::vector<bool> flags(hard_macros.size(), false);
  // align to the left
  std::vector<std::pair<size_t, std::pair<int, int>>> macro_lx_map;
  for (size_t j = 0; j < hard_macros.size(); j++)
    macro_lx_map.push_back(std::pair<size_t, std::pair<int, int>>(
        j,
        std::pair<int, int>(hard_macros[j]->getXDBU(),
                            hard_macros[j]->getYDBU())));
  std::sort(macro_lx_map.begin(), macro_lx_map.end(), LessOrEqualX);
  for (auto& pair : macro_lx_map) {
    if (pair.second.first <= core_lx + boundary_h_th) {
      flags[pair.first] = true;      // fix this
      macro_queue.push(pair.first);  // use this as an anchor
    } else if (hard_macros[pair.first]->getUXDBU() >= core_ux - boundary_h_th) {
      flags[pair.first] = true;  // fix this
    } else if (hard_macros[pair.first]->getUXDBU() <= core_ux / 2) {
      macro_list.push_back(pair.first);
    }
  }
  while (!macro_queue.empty()) {
    const size_t macro_id = macro_queue.front();
    macro_queue.pop();
    const int lx = hard_macros[macro_id]->getXDBU();
    const int ly = hard_macros[macro_id]->getYDBU();
    const int ux = hard_macros[macro_id]->getUXDBU();
    const int uy = hard_macros[macro_id]->getUYDBU();
    for (auto j : macro_list) {
      if (flags[j] == true)
        continue;
      const int lx_b = hard_macros[j]->getXDBU();
      const int ly_b = hard_macros[j]->getYDBU();
      const int ux_b = hard_macros[j]->getUXDBU();
      const int uy_b = hard_macros[j]->getUYDBU();
      // check if adjacent
      const bool y_flag = std::abs(ly - ly_b) < notch_v_th
                          || std::abs(ly - uy_b) < notch_v_th
                          || std::abs(uy - ly_b) < notch_v_th
                          || std::abs(uy - uy_b) < notch_v_th;
      if (y_flag == false)
        continue;
      // try to move horizontally
      if (lx_b >= lx && lx_b <= lx + notch_h_th && lx_b < ux)
        flags[j] = moveHor(j, lx);
      else if (ux_b >= lx && ux_b <= ux && ux_b >= ux - notch_h_th)
        flags[j] = moveHor(j, ux - hard_macros[j]->getWidthDBU());
      else if (lx_b >= ux && lx_b <= ux + notch_h_th)
        flags[j] = moveHor(j, ux);
      // check if moved correctly
      if (flags[j] == true) {
        macro_queue.push(j);
      }
    }
  }

  // align to the top
  macro_list.clear();
  std::fill(flags.begin(), flags.end(), false);
  std::vector<std::pair<size_t, std::pair<int, int>>> macro_uy_map;
  for (size_t j = 0; j < hard_macros.size(); j++)
    macro_uy_map.push_back(std::pair<size_t, std::pair<int, int>>(
        j,
        std::pair<int, int>(hard_macros[j]->getUXDBU(),
                            hard_macros[j]->getUYDBU())));
  std::sort(macro_uy_map.begin(), macro_uy_map.end(), LargeOrEqualY);
  for (auto& pair : macro_uy_map) {
    if (hard_macros[pair.first]->getYDBU() <= core_ly + boundary_v_th) {
      flags[pair.first] = true;  // fix this
    } else if (hard_macros[pair.first]->getUYDBU() >= core_uy - boundary_v_th) {
      flags[pair.first] = true;      // fix this
      macro_queue.push(pair.first);  // use this as an anchor
    } else if (hard_macros[pair.first]->getYDBU() >= core_uy / 2) {
      macro_list.push_back(pair.first);
    }
  }
  while (!macro_queue.empty()) {
    const size_t macro_id = macro_queue.front();
    macro_queue.pop();
    const int lx = hard_macros[macro_id]->getXDBU();
    const int ly = hard_macros[macro_id]->getYDBU();
    const int ux = hard_macros[macro_id]->getUXDBU();
    const int uy = hard_macros[macro_id]->getUYDBU();
    for (auto j : macro_list) {
      if (flags[j] == true)
        continue;
      const int lx_b = hard_macros[j]->getXDBU();
      const int ly_b = hard_macros[j]->getYDBU();
      const int ux_b = hard_macros[j]->getUXDBU();
      const int uy_b = hard_macros[j]->getUYDBU();
      // check if adjacent
      const bool x_flag = std::abs(lx - lx_b) < notch_h_th
                          || std::abs(lx - ux_b) < notch_h_th
                          || std::abs(ux - lx_b) < notch_h_th
                          || std::abs(ux - ux_b) < notch_h_th;
      if (x_flag == false)
        continue;
      // try to move vertically
      if (uy_b < uy && uy_b >= uy - notch_v_th && uy_b > ly)
        flags[j] = moveVer(j, uy - hard_macros[j]->getHeightDBU());
      else if (ly_b >= ly && ly_b <= uy && ly_b <= ly + notch_v_th)
        flags[j] = moveVer(j, ly);
      else if (uy_b <= ly && uy_b >= ly - notch_v_th)
        flags[j] = moveVer(j, ly - hard_macros[j]->getHeightDBU());
      // check if moved correctly
      if (flags[j] == true) {
        macro_queue.push(j);
      }
    }
  }

  // align to the right
  macro_list.clear();
  std::fill(flags.begin(), flags.end(), false);
  std::vector<std::pair<size_t, std::pair<int, int>>> macro_ux_map;
  for (size_t j = 0; j < hard_macros.size(); j++)
    macro_ux_map.push_back(std::pair<size_t, std::pair<int, int>>(
        j,
        std::pair<int, int>(hard_macros[j]->getUXDBU(),
                            hard_macros[j]->getUYDBU())));
  std::sort(macro_ux_map.begin(), macro_ux_map.end(), LargeOrEqualX);
  for (auto& pair : macro_ux_map) {
    if (hard_macros[pair.first]->getXDBU() <= core_lx + boundary_h_th) {
      flags[pair.first] = true;  // fix this
    } else if (hard_macros[pair.first]->getUXDBU() >= core_ux - boundary_h_th) {
      flags[pair.first] = true;      // fix this
      macro_queue.push(pair.first);  // use this as an anchor
    } else if (hard_macros[pair.first]->getUXDBU() >= core_ux / 2) {
      macro_list.push_back(pair.first);
    }
  }
  while (!macro_queue.empty()) {
    const size_t macro_id = macro_queue.front();
    macro_queue.pop();
    const int lx = hard_macros[macro_id]->getXDBU();
    const int ly = hard_macros[macro_id]->getYDBU();
    const int ux = hard_macros[macro_id]->getUXDBU();
    const int uy = hard_macros[macro_id]->getUYDBU();
    for (auto j : macro_list) {
      if (flags[j] == true)
        continue;
      const int lx_b = hard_macros[j]->getXDBU();
      const int ly_b = hard_macros[j]->getYDBU();
      const int ux_b = hard_macros[j]->getUXDBU();
      const int uy_b = hard_macros[j]->getUYDBU();
      // check if adjacent
      const bool y_flag = std::abs(ly - ly_b) < notch_v_th
                          || std::abs(ly - uy_b) < notch_v_th
                          || std::abs(uy - ly_b) < notch_v_th
                          || std::abs(uy - uy_b) < notch_v_th;
      if (y_flag == false)
        continue;
      // try to move horizontally
      if (ux_b < ux && ux_b >= ux - notch_h_th && ux_b > lx)
        flags[j] = moveHor(j, ux - hard_macros[j]->getWidthDBU());
      else if (lx_b >= lx && lx_b <= ux && lx_b <= lx + notch_h_th)
        flags[j] = moveHor(j, lx);
      else if (ux_b <= lx && ux_b >= lx - notch_h_th)
        flags[j] = moveHor(j, lx - hard_macros[j]->getWidthDBU());
      // check if moved correctly
      if (flags[j] == true) {
        macro_queue.push(j);
      }
    }
  }
  // align to the bottom
  macro_list.clear();
  std::fill(flags.begin(), flags.end(), false);
  std::vector<std::pair<size_t, std::pair<int, int>>> macro_ly_map;
  for (size_t j = 0; j < hard_macros.size(); j++)
    macro_ly_map.push_back(std::pair<size_t, std::pair<int, int>>(
        j,
        std::pair<int, int>(hard_macros[j]->getXDBU(),
                            hard_macros[j]->getYDBU())));
  std::sort(macro_ly_map.begin(), macro_ly_map.end(), LessOrEqualY);
  for (auto& pair : macro_ly_map) {
    if (hard_macros[pair.first]->getYDBU() <= core_ly + boundary_v_th) {
      flags[pair.first] = true;      // fix this
      macro_queue.push(pair.first);  // use this as an anchor
    } else if (hard_macros[pair.first]->getUYDBU() >= core_uy - boundary_v_th) {
      flags[pair.first] = true;  // fix this
    } else if (hard_macros[pair.first]->getUYDBU() <= core_uy / 2) {
      macro_list.push_back(pair.first);
    }
  }
  while (!macro_queue.empty()) {
    const size_t macro_id = macro_queue.front();
    macro_queue.pop();
    const int lx = hard_macros[macro_id]->getXDBU();
    const int ly = hard_macros[macro_id]->getYDBU();
    const int ux = hard_macros[macro_id]->getUXDBU();
    const int uy = hard_macros[macro_id]->getUYDBU();
    for (auto j : macro_list) {
      if (flags[j] == true)
        continue;
      const int lx_b = hard_macros[j]->getXDBU();
      const int ly_b = hard_macros[j]->getYDBU();
      const int ux_b = hard_macros[j]->getUXDBU();
      const int uy_b = hard_macros[j]->getUYDBU();
      // check if adjacent
      const bool x_flag = std::abs(lx - lx_b) < notch_h_th
                          || std::abs(lx - ux_b) < notch_h_th
                          || std::abs(ux - lx_b) < notch_h_th
                          || std::abs(uy - ux_b) < notch_h_th;
      if (x_flag == false)
        continue;
      // try to move vertically
      if (ly_b >= ly && ly_b < ly + notch_v_th && ly_b < uy)
        flags[j] = moveVer(j, ly);
      else if (uy_b >= ly && uy_b <= uy && uy_b >= uy - notch_v_th)
        flags[j] = moveVer(j, uy - hard_macros[j]->getHeightDBU());
      else if (ly_b >= uy && ly_b <= uy + notch_v_th)
        flags[j] = moveVer(j, uy);
      // check if moved correctly
      if (flags[j] == true) {
        macro_queue.push(j);
      }
    }
  }
}

// force-directed placement to generate guides for macros
// Attractive force and Repulsive force should be normalied separately
// Because their values can vary a lot.
void HierRTLMP::FDPlacement(std::vector<Rect>& blocks,
                            const std::vector<BundledNet>& nets,
                            float outline_width,
                            float outline_height,
                            std::string file_name)
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
          || blocks[sink].getWidth() < 1.0 || blocks[sink].getHeight() < 1.0)
        k = k * io_factor;
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
        if (blocks[src].ux <= blocks[target].lx)
          break;
        if (blocks[src].ly >= blocks[target].uy
            || blocks[src].uy <= blocks[target].ly)
          continue;
        // ignore the overlap between clusters and IO ports
        if (blocks[src].getWidth() < 1.0 || blocks[src].getHeight() < 1.0
            || blocks[target].getWidth() < 1.0
            || blocks[target].getHeight() < 1.0)
          continue;
        if (blocks[src].fixed_flag == true || blocks[target].fixed_flag == true)
          continue;
        // apply the force from src to target
        if (src > target)
          std::swap(src, target);
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
        for (auto& block : blocks)
          block.resetForce();
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
    for (auto j = 0; j < num_steps[i]; j++)
      MoveBlock(attract_factor,
                repulsive_factor,
                max_size / (1 + std::floor(j / 100)));
  }

  /*
  std::string net_file = file_name + ".net.txt";
  std::string block_file = file_name + ".block.txt";
  std::ofstream file;
  file.open(block_file);
  for (auto& rect : blocks)
    file << rect.lx << " " << rect.ly << "  " << rect.ux << "  " << rect.uy <<
  std::endl; file.close(); file.open(net_file); for (auto& net : nets) file <<
  net.terminals.first << "  " << net.terminals.second << "  " << net.weight <<
  std::endl; file.close();
  */
}

void HierRTLMP::setDebug()
{
  int dbu = db_->getTech()->getDbUnitsPerMicron();
  graphics_ = std::make_unique<Graphics>(dbu, logger_);
}

}  // namespace mpl2
