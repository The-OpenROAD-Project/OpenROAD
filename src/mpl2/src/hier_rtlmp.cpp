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
#include <queue>
#include <thread>

// Partitioner note : currently we are still using MLPart to partition large
// flat clusters Later this will be replaced by our TritonPart
//#include "par/MLPart.h"

#include "SACoreHardMacro.h"
#include "SACoreSoftMacro.h"
#include "bus_synthesis.h"
#include "db_sta/dbNetwork.hh"
#include "graphics.h"
#include "object.h"
#include "odb/db.h"
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
                     utl::Logger* logger)
{
  network_ = network;
  db_ = db;
  sta_ = sta;
  logger_ = logger;
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

void HierRTLMP::setTopLevelClusterSize(int max_num_macro,
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

///////////////////////////////////////////////////////////////
// Top Level Interface function
void HierRTLMP::hierRTLMacroPlacer()
{
  //
  // Determine the size thresholds of parent clusters based on leaf cluster
  // thresholds and the coarsening factor
  //
  unsigned coarsening_factor = std::pow(coarsening_ratio_, max_num_level_ - 1);
  max_num_macro_base_ = max_num_macro_base_ * coarsening_factor;
  min_num_macro_base_ = min_num_macro_base_ * coarsening_factor;
  max_num_inst_base_ = max_num_inst_base_ * coarsening_factor;
  min_num_inst_base_ = min_num_inst_base_ * coarsening_factor;

  //
  // Get the database information
  //
  block_ = db_->getChip()->getBlock();
  dbu_ = db_->getTech()->getDbUnitsPerMicron();
  pitch_x_ = dbuToMicron(
      static_cast<float>(
          db_->getTech()->findRoutingLayer(snap_layer_)->getPitchX()),
      dbu_);
  pitch_y_ = dbuToMicron(
      static_cast<float>(
          db_->getTech()->findRoutingLayer(snap_layer_)->getPitchY()),
      dbu_);

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

  logger_->info(
      MPL,
      2023,
      "Floorplan Outline: ({}, {}) ({}, {}),  Core Outline: ({} {} {} {})",
      floorplan_lx_,
      floorplan_ly_,
      floorplan_ux_,
      floorplan_uy_,
      core_lx,
      core_ly,
      core_ux,
      core_uy);
  logger_->report(
      "num level: {}, max_macro: {}, min_macro: {}, max_inst:{}, min_inst:{}",
      max_num_level_,
      max_num_macro_base_,
      min_num_macro_base_,
      max_num_inst_base_,
      min_num_inst_base_);

  //
  // Compute metrics for dbModules
  // Associate all the hard macros with their HardMacro objects
  // and report the statistics
  //
  metric_ = computeMetric(block_->getTopModule());
  float core_area = (core_ux - core_lx) * (core_uy - core_ly);
  float util
      = (metric_->getStdCellArea() + metric_->getMacroArea()) / core_area;
  float core_util
      = metric_->getStdCellArea() / (core_area - metric_->getMacroArea());

  logger_->info(MPL,
                402,
                "Traversed logical hierarchy\n"
                "\tNumber of std cell instances : {}\n"
                "\tArea of std cell instances : {}\n"
                "\tNumber of macros : {}\n"
                "\tArea of macros : {}\n"
                "\tTotal area : {}\n"
                "\tDesign Utilization : {}\n"
                "\tCore Utilization: {}\n",
                metric_->getNumStdCell(),
                metric_->getStdCellArea(),
                metric_->getNumMacro(),
                metric_->getMacroArea(),
                metric_->getStdCellArea() + metric_->getMacroArea(),
                util,
                core_util);

  //
  // Initialize the physcial hierarchy tree
  // create root cluster
  //
  cluster_id_ = 0;
  // set the level of cluster to be 0
  root_cluster_ = new Cluster(cluster_id_, std::string("root"), logger_);
  // set the design metric as the metric for the root cluster
  root_cluster_->addDbModule(block_->getTopModule());
  root_cluster_->setMetric(*metric_);
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
  createDataFlow();

  logger_->report("Finish Calculate Data Flow detailed CR done");

  // Create physical hierarchy tree in a post-order DFS manner
  multiLevelCluster(root_cluster_);  // Recursive call for creating the tree
  printPhysicalHierarchyTree(root_cluster_, 0);

  //
  // Break leaf clusters into a standard-cell cluster and a hard-macro cluster
  // And merge macros into clusters based on connection signatures and
  // footprints We assume there is no direct connections between macros Based on
  // types of designs, we support two types of breaking up Suppose current
  // cluster is A Type 1:  Replace A by A1, A2, A3 Type 2:  Create a subtree for
  // A
  //          A  ->    A
  //               |  |   |
  //              A1  A2  A3
  //
  leafClusterStdCellHardMacroSep(root_cluster_);

  // PrintClusters();

  //
  // Print the created physical hierarchy tree
  // Thus user can debug the clustering results
  // and they can tune the options to get better
  // clustering results
  //
  printPhysicalHierarchyTree(root_cluster_, 0);

  // Map the macros in each cluster to their HardMacro objects
  for (auto& [cluster_id, cluster] : cluster_map_) {
    mapMacroInCluster2HardMacro(cluster);
  }

  // Place macros in a hierarchical mode based on the above
  // physcial hierarchical tree
  // Shape Engine Starts .................................................
  // Determine the shape and area for each cluster in a bottom-up manner
  // We first determine the macro tilings within each cluster
  // Then when we place clusters in each level, we can dynamic adjust the
  // utilization of std cells Step 1 : Determine the size of root cluster
  SoftMacro* root_soft_macro = new SoftMacro(root_cluster_);
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

  // Step 2:  Determine the macro tilings within each cluster in a bottom-up
  // manner (Post-Order DFS manner)
  calClusterMacroTilings(root_cluster_);
  // Cluster Placement Engine Starts ...........................................
  // create pin blockage for IO pins
  createPinBlockage();
  // The cluster placement is done in a top-down manner
  // (Preorder DFS)
  multiLevelMacroPlacement(root_cluster_);

  // Clear the memory to avoid memory leakage
  // release all the pointers
  // metric map
  for (auto& [module, metric] : logical_module_map_) {
    delete metric;
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
Metric* HierRTLMP::computeMetric(odb::dbModule* module)
{
  unsigned int num_std_cell = 0;
  float std_cell_area = 0.0;
  unsigned int num_macro = 0;
  float macro_area = 0.0;

  for (odb::dbInst* inst : module->getInsts()) {
    const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
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
    Metric* metric = computeMetric(inst->getMaster());
    num_std_cell += metric->getNumStdCell();
    std_cell_area += metric->getStdCellArea();
    num_macro += metric->getNumMacro();
    macro_area += metric->getMacroArea();
  }

  Metric* metric
      = new Metric(num_std_cell, num_macro, std_cell_area, macro_area);
  logical_module_map_[module] = metric;
  return metric;
}

// compute the metric for a cluster
// Here we do not include any Pads,  Covers or Marker
// number of standard cells
// number of macros
// area of standard cells
// area of macros
void HierRTLMP::setClusterMetric(Cluster* cluster)
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
  Metric metric(num_std_cell, num_macro, std_cell_area, macro_area);
  for (auto& module : cluster->getDbModules()) {
    metric.addMetric(*logical_module_map_[module]);
  }

  // for debug
  if (1) {
    logger_->report("[Debug][HierRTLMP::SetClusterMetric] {} ",
                    cluster->getName());
    logger_->report("num_macro : {} num_std_cell : {} ",
                    metric.getNumMacro(),
                    metric.getNumStdCell());
  }

  // end debug

  // update metric based on design type
  if (cluster->getClusterType() == HardMacroCluster) {
    cluster->setMetric(
        Metric(0, metric.getNumMacro(), 0.0, metric.getMacroArea()));
  } else if (cluster->getClusterType() == StdCellCluster) {
    cluster->setMetric(
        Metric(metric.getNumStdCell(), 0, metric.getStdCellArea(), 0.0));
  } else {
    cluster->setMetric(metric);
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
    if (lx == floorplan_lx_) {
      // The IO is on the left boundary
      cluster_id = cluster_id_base
                   + std::floor(((ly + uy) / 2.0 - floorplan_ly_) / y_base);
    } else if (uy == floorplan_uy_) {
      // The IO is on the top boundary
      cluster_id = cluster_id_base + num_bundled_IOs_
                   + std::floor(((lx + ux) / 2.0 - floorplan_lx_) / x_base);
    } else if (ux == floorplan_ux_) {
      // The IO is on the right boundary
      cluster_id = cluster_id_base + num_bundled_IOs_ * 2
                   + std::floor((floorplan_uy_ - (ly + uy) / 2.0) / y_base);
    } else if (ly == floorplan_ly_) {
      // The IO is on the bottom boundary
      cluster_id = cluster_id_base + num_bundled_IOs_ * 3
                   + std::floor((floorplan_ux_ - (lx + ux) / 2.0) / x_base);
    }

    // Check if the IO pins / Pads exist
    if (cluster_id == -1) {
      logger_->error(
          MPL, 2024, "Floorplan has not been initialized? Pin location error.");
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
      logger_->report("remove cluster : {}, id: {}",
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
  if (level_ >= max_num_level_) {  // limited by the user-specified parameter
    return;
  }
  level_++;
  // for debug
  if (1) {
    logger_->report(
        "[Debug][HierRTLMP::MultiLevelCluster]  {} {}:  num_macro {} "
        "num_stdcell {}",
        parent->getName(),
        level_,
        parent->getNumMacro(),
        parent->getNumStdCell());
  }
  // end debug

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

  if (parent->getNumMacro() > max_num_macro_
      || parent->getNumStdCell() > max_num_inst_) {
    breakCluster(parent);   // Break the parent cluster into children clusters
    updateSubTree(parent);  // update the subtree to the physical hierarchy tree
    for (auto& child : parent->getChildren()) {
      setInstProperty(child);
    }
    // print the basic information of the children cluster
    for (auto& child : parent->getChildren()) {
      child->printBasicInformation(logger_);
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
  logger_->report("[Debug][HierRTLMP::BreakCluster] cluster_name : {}",
                  parent->getName());
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
    // we will use the MLPart to partition this large flat cluster
    // in the follow-up UpdateSubTree function
    if (module->getChildren().size() == 0) {
      for (odb::dbInst* inst : module->getInsts()) {
        const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
        odb::dbMaster* master = inst->getMaster();
        // check if the instance is a Pad, Cover or empty block (such as marker)
        if (master->isPad() || master->isCover()
            || (master->isBlock() && liberty_cell == nullptr)) {
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
      setClusterMetric(cluster);
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
      odb::dbMaster* master = inst->getMaster();
      // check if the instance is a Pad, Cover or empty block (such as marker)
      if (master->isPad() || master->isCover()
          || (master->isBlock() && liberty_cell == nullptr)) {
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
      setClusterMetric(cluster);
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
      setClusterMetric(cluster);
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
      setClusterMetric(cluster);
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
  // for debug
  int merge_iter = 0;
  logger_->report("\n\n[Debug][HierRTLMP::MergeCluster]  Iter : {}",
                  merge_iter++);
  for (auto& cluster : candidate_clusters) {
    logger_->report("cluster  :  {}  num_std_cell: {} num_macro: {}",
                    cluster->getName(),
                    cluster->getNumStdCell(),
                    cluster->getNumMacro());
  }
  // end debug

  int num_candidate_clusters = candidate_clusters.size();
  while (true) {
    calculateConnection();  // update the connections between clusters
    // for debug
    // PrintConnection();
    // end debug

    std::vector<int> cluster_class(num_candidate_clusters, -1);  // merge flag
    std::vector<int> candidate_clusters_id;  // store cluster id
    for (auto& cluster : candidate_clusters) {
      candidate_clusters_id.push_back(cluster->getId());
    }
    // Firstly we perform Type 1 merge
    for (int i = 0; i < num_candidate_clusters; i++) {
      const int cluster_id = candidate_clusters[i]->getCloseCluster(
          candidate_clusters_id, signature_net_threshold_);
      logger_->report(
          "candidate_cluster : {}  -  {}",
          candidate_clusters[i]->getName(),
          (cluster_id != -1 ? cluster_map_[cluster_id]->getName() : "   "));
      // end debug
      if (cluster_id != -1 && !cluster_map_[cluster_id]->getIOClusterFlag()) {
        Cluster*& cluster = cluster_map_[cluster_id];
        bool delete_flag = false;
        if (cluster->mergeCluster(*candidate_clusters[i], delete_flag)) {
          if (delete_flag) {
            cluster_map_.erase(candidate_clusters[i]->getId());
            delete candidate_clusters[i];
          }
          setInstProperty(cluster);
          setClusterMetric(cluster);
          cluster_class[i] = cluster->getId();
        }
      }
    }

    // Then we perform Type 2 merge
    std::vector<Cluster*> new_candidate_clusters;
    for (int i = 0; i < num_candidate_clusters; i++) {
      if (cluster_class[i] == -1) {  // the cluster has not been merged
        new_candidate_clusters.push_back(candidate_clusters[i]);
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
              setClusterMetric(candidate_clusters[i]);
            }
          }
        }
      }
    }
    // If no more clusters have been merged, exit the merging loop
    if (num_candidate_clusters == new_candidate_clusters.size()) {
      break;
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
    num_candidate_clusters = candidate_clusters.size();

    // for debug
    logger_->report("\n\n[Debug][HierRTLMP::MergeCluster]  Iter: {}",
                    merge_iter++);
    for (auto& cluster : candidate_clusters) {
      logger_->report("cluster  :  {}", cluster->getName());
    }
    // end debug
    // merge small clusters
    if (candidate_clusters.size() == 0) {
      break;
    }
  }
  logger_->report("[Debug][HierRTLMP::MergeCluster]  Finish MergeCluster");
}

// Calculate Connections between clusters
void HierRTLMP::calculateConnection()
{
  // Initialize the connections of all clusters
  for (auto& [cluster_id, cluster] : cluster_map_) {
    cluster->initConnection();
  }

  // for debug
  // PrintClusters();
  // end debug

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
      odb::dbMaster* master = inst->getMaster();
      // check if the instance is a Pad, Cover or empty block (such as marker)
      // We ignore nets connecting Pads, Covers, or markers
      if (master->isPad() || master->isCover()
          || (master->isBlock() && liberty_cell == nullptr)) {
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
    // check the connected IO pins
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const int cluster_id
          = odb::dbIntProperty::find(bterm, "cluster_id")->getValue();
      if (bterm->getIoType() == odb::dbIoType::INPUT) {
        driver_id = cluster_id;
      } else {
        loads_id.push_back(cluster_id);
      }
    }
    // add the net to connections between clusters
    if (driver_id != -1 && loads_id.size() > 0
        && loads_id.size() < large_net_threshold_) {
      for (int i = 0; i < loads_id.size(); i++) {
        if (loads_id[i]
            != driver_id) {  // we model the connections as undirected edges
          cluster_map_[driver_id]->addConnection(loads_id[i], 1.0);
          cluster_map_[loads_id[i]]->addConnection(driver_id, 1.0);
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
  logger_->report("Finish Creating Vertices. Num of Vertices: {}",
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
      odb::dbMaster* master = inst->getMaster();
      // check if the instance is a Pad, Cover or empty block (such as marker)
      // We ignore nets connecting Pads, Covers, or markers
      if (master->isPad() || master->isCover()
          || (master->isBlock() && liberty_cell == nullptr)) {
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

  logger_->report("Finish hypergraph creation");

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
      const float weight = 1.0 / std::pow(dataflow_factor_, i);
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
      const float weight = 1.0 / std::pow(dataflow_factor_, i);
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
      const float weight = std::pow(dataflow_factor_, i);
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
  logger_->info(MPL, 2026, line);
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
  logger_->info(MPL, 2027, line);
}

// This function has two purposes:
// 1) remove all the internal clusters between parent and leaf clusters in its
// subtree 2) Call MLPart to partition large flat clusters (a cluster with no
// logical modules)
//    Later MLPart will be replaced by TritonPart
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

// Break large flat clusters with MLPart
// A flat cluster does not have a logical module
void HierRTLMP::breakLargeFlatCluster(Cluster* parent)
{
  // Check if the cluster is a large flat cluster
  if (parent->getDbModules().size() > 0
      || parent->getLeafStdCells().size() < max_num_inst_) {
    return;
  }

  return;
  std::map<int, int> cluster_vertex_id_map;
  std::map<odb::dbInst*, int> inst_vertex_id_map;
  const int parent_cluster_id = parent->getId();
  std::vector<odb::dbInst*> std_cells = parent->getLeafStdCells();
  std::vector<int> col_idx;  // edges represented by vertex indices
  std::vector<int> row_ptr;  // pointers for edges
  // vertices
  // other clusters behaves like fixed vertices
  // We do not consider vertices only between fixed vertices
  int num_vertices = cluster_map_.size() - 1;
  num_vertices += parent->getLeafMacros().size();
  num_vertices += std_cells.size();
  int vertex_id = 0;
  for (auto& [cluster_id, cluster] : cluster_map_) {
    cluster_vertex_id_map[cluster_id] = vertex_id++;
  }
  for (auto& macro : parent->getLeafMacros()) {
    inst_vertex_id_map[macro] = vertex_id++;
  }
  int num_fixed_vertices = vertex_id;  // we set these fixed vertices to part0
  for (auto& std_cell : std_cells) {
    inst_vertex_id_map[std_cell] = vertex_id++;
  }
  std::vector<int> part(num_vertices, -1);
  for (int i = 0; i < num_fixed_vertices; i++) {
    part[i] = 0;
  }

  // Traverse nets to create edges (col_idx, row_ptr)
  row_ptr.push_back(col_idx.size());
  for (odb::dbNet* net : block_->getNets()) {
    // ignore all the power net
    if (net->getSigType().isSupply()) {
      continue;
    }
    int driver_id = -1;         // vertex id of the driver instance
    std::vector<int> loads_id;  // vertex id of the sink instances
    bool pad_flag = false;
    // check the connected instances
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
      odb::dbMaster* master = inst->getMaster();
      // check if the instance is a Pad, Cover or empty block (such as marker)
      // if the nets connects to such pad, cover or empty block,
      // we should ignore such net
      if (master->isPad() || master->isCover()
          || (master->isBlock() && liberty_cell == nullptr)) {
        pad_flag = true;
        break;  // here CAN NOT be continue
      }
      const int cluster_id
          = odb::dbIntProperty::find(inst, "cluster_id")->getValue();
      int vertex_id = (cluster_id == parent_cluster_id)
                          ? cluster_vertex_id_map[cluster_id]
                          : inst_vertex_id_map[inst];
      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        driver_id = vertex_id;
      } else {
        loads_id.push_back(vertex_id);
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
        loads_id.push_back(cluster_vertex_id_map[cluster_id]);
      }
    }
    // add the net as a hyperedge
    if (driver_id != -1 && loads_id.size() > 0
        && loads_id.size() < large_net_threshold_) {
      col_idx.push_back(driver_id);
      col_idx.insert(col_idx.end(), loads_id.begin(), loads_id.end());
      row_ptr.push_back(col_idx.size());
    }
  }

  // we do not specify any weight for vertices or hyperedges
  std::vector<float> vertex_weight(part.size(), 1.0);
  std::vector<float> edge_weight(row_ptr.size() - 1, 1.0);

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
  for (int i = num_fixed_vertices; i < part.size(); i++) {
    if (part[i] == 0) {
      parent->addLeafStdCell(std_cells[i - num_fixed_vertices]);
    } else {
      cluster_part_1->addLeafStdCell(std_cells[i - num_fixed_vertices]);
    }
  }
  // update the property of parent cluster
  setInstProperty(parent);
  setClusterMetric(parent);
  // update the property of cluster_part_1
  setInstProperty(cluster_part_1);
  setClusterMetric(cluster_part_1);
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
    // for debug
    if (0) {
      logger_->report(
          "\n\n[Debug][HierRTLMP::LeafClusterStdCellHardMacroSep]  {}",
          cluster->getName());
    }
    // end debug
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
    // for debug
    if (0) {
      logger_->report("Finish mapping hard macros");
    }
    // end debug

    // create a cluster for each macro
    std::vector<Cluster*> macro_clusters;
    for (auto& hard_macro : hard_macros) {
      std::string cluster_name = hard_macro->getName();
      Cluster* macro_cluster = new Cluster(cluster_id_, cluster_name, logger_);
      macro_cluster->addLeafMacro(hard_macro->getInst());
      setInstProperty(macro_cluster);
      setClusterMetric(macro_cluster);
      cluster_map_[cluster_id_++] = macro_cluster;
      // modify the physical hierachy tree
      macro_cluster->setParent(parent_cluster);
      parent_cluster->addChild(macro_cluster);
      macro_clusters.push_back(macro_cluster);
    }

    // for debug
    if (0) {
      logger_->report("Finish mapping macros to clusters");
    }
    // end debug

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
    // std::vector<int> macro_size_class(hard_macros.size(), 1);
    // std::vector<int> macro_size_class(hard_macros.size(), 1);
    for (auto& id : macro_size_class) {
      logger_->report("{}   ", id);
    }
    logger_->report("\n");

    for (int i = 0; i < hard_macros.size(); i++) {
      macro_size_class[i]
          = (macro_size_class[i] == -1) ? i : macro_size_class[i];
    }

    for (auto& id : macro_size_class) {
      logger_->report("  {}", id);
    }
    logger_->report("\n");

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

    for (auto& id : macro_signature_class) {
      logger_->report(" {} ", id);
    }
    logger_->report("\n");

    // print the connnection signature
    for (auto& cluster : macro_clusters) {
      logger_->report("**************************************************");
      logger_->report("Macro Signature : {}", cluster->getName());

      for (auto& [cluster_id, weight] : cluster->getConnection()) {
        logger_->report(" {} {} ", cluster_map_[cluster_id]->getName(), weight);
      }
      logger_->report("\n");
    }

    // for debug
    if (1) {
      logger_->report("Finish  Macro Classification");
    }
    // end debug

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
            logger_->report("merge {}  {} ",
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

    for (auto& id : macro_class) {
      logger_->report(" {}", id);
    }
    logger_->report("\n");

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
      setClusterMetric(std_cell_cluster);
      cluster_map_[cluster_id_++] = std_cell_cluster;
      // modify the physical hierachy tree
      std_cell_cluster->setParent(parent_cluster);
      parent_cluster->addChild(std_cell_cluster);
      virtual_conn_clusters.push_back(std_cell_cluster->getId());
    } else {
      // If we need add a replacement
      cluster->clearLeafMacros();
      cluster->setClusterType(StdCellCluster);
      setClusterMetric(cluster);
      virtual_conn_clusters.push_back(cluster->getId());
      // In this case, we do not to modify the physical hierarchy tree
    }
    // Deal with macro clusters
    for (int i = 0; i < macro_class.size(); i++) {
      if (macro_class[i] != i) {
        continue;  // this macro cluster has been merged
      }
      macro_clusters[i]->setClusterType(HardMacroCluster);
      setClusterMetric(macro_clusters[i]);
      virtual_conn_clusters.push_back(cluster->getId());
    }

    // add virtual connections
    for (int i = 0; i < virtual_conn_clusters.size(); i++) {
      for (int j = i + 1; j < virtual_conn_clusters.size(); j++) {
        parent_cluster->addVirtualConnection(virtual_conn_clusters[i],
                                             virtual_conn_clusters[j]);
      }
    }
    /*
    // add virtual connections
    for (int i = 0; i < 1; i++) {
      for (int j = i + 1; j < virtual_conn_clusters.size(); j++) {
        parent_cluster->AddVirtualConnection(virtual_conn_clusters[i],
        virtual_conn_clusters[j]);
      }
    }
    */
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
    odb::dbMaster* master = inst->getMaster();
    // check if the instance is a pad or empty block (such as marker)
    if (master->isPad() || master->isCover()
        || (master->isBlock() && liberty_cell == nullptr)) {
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

// Print Physical Hierarchy tree
void HierRTLMP::printPhysicalHierarchyTree(Cluster* parent, int level)
{
  std::string line = "";
  for (int i = 0; i < level; i++) {
    line += "+----";
  }
  line += parent->getName();
  line += "  (" + std::to_string(parent->getId()) + ")  ";
  line += "  num_macro :  " + std::to_string(parent->getNumMacro());
  line += "  num_std_cell :  " + std::to_string(parent->getNumStdCell());
  line += "  macro_area :  " + std::to_string(parent->getMacroArea());
  line += "  std_cell_area : " + std::to_string(parent->getStdCellArea());
  logger_->info(MPL, 2025, line);
  int tot_num_macro_in_children = 0;
  for (auto& cluster : parent->getChildren()) {
    tot_num_macro_in_children += cluster->getNumMacro();
  }
  logger_->report("tot_num_macro_in_children = {}", tot_num_macro_in_children);

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
// Determine the macro tilings within each cluster in a bottom-up manner
// (Post-Order DFS manner)
void HierRTLMP::calClusterMacroTilings(Cluster* parent)
{
  // base case
  if (parent->getNumMacro() == 0) {
    return;
  }

  if (parent->getClusterType() == HardMacroCluster) {
    calHardMacroClusterShape(parent);
    return;
  }

  // recursively visit the children
  for (auto& cluster : parent->getChildren()) {
    if (cluster->getNumMacro() > 0) {
      calClusterMacroTilings(cluster);
    }
  }

  // calculate macro tiling for parent cluster based on the macro tilings
  // of its children
  std::vector<SoftMacro> macros;
  for (auto& cluster : parent->getChildren()) {
    if (cluster->getNumMacro() > 0) {
      SoftMacro macro = SoftMacro(cluster);
      macro.setShapes(cluster->getMacroTilings(), true);
      macros.push_back(macro);
    }
  }

  // if there is only one soft macro
  if (macros.size() == 1) {
    for (auto& cluster : parent->getChildren()) {
      if (cluster->getNumMacro() > 0) {
        parent->setMacroTilings(cluster->getMacroTilings());
        return;
      }
    }
  }
  // otherwise call simulated annealing to determine tilings
  // set the action probabilities
  // macro tilings
  std::set<std::pair<float, float>> macro_tilings;  // <width, height>
  const float action_sum = pos_swap_prob_ + neg_swap_prob_ + double_swap_prob_
                           + exchange_swap_prob_ + resize_prob_;
  const float pos_swap_prob = pos_swap_prob_ / action_sum;
  const float neg_swap_prob = neg_swap_prob_ / action_sum;
  const float double_swap_prob = double_swap_prob_ / action_sum;
  const float exchange_swap_prob = exchange_swap_prob_ / action_sum;
  const float resize_prob = resize_prob_ / action_sum;
  // get outline constraint
  const float outline_width = root_cluster_->getWidth();
  const float outline_height = root_cluster_->getHeight();
  // We vary the outline of cluster to generate differnt tilings
  std::vector<float> vary_factor_list{1.0};
  float vary_step = 1.0 / num_runs_;  // change the outline by at most halfly
  for (int i = 1; i < num_runs_; i++) {
    vary_factor_list.push_back(1.0 - i * vary_step);
  }
  const int num_perturb_per_step = (macros.size() > num_perturb_per_step_ / 10)
                                       ? macros.size()
                                       : num_perturb_per_step_ / 10;
  // const int num_perturb_per_step = macros.size() * 10;
  int run_thread = num_threads_;
  int remaining_runs = num_runs_;
  int run_id = 0;
  while (remaining_runs > 0) {
    std::vector<SACoreSoftMacro*> sa_vector;
    run_thread
        = (remaining_runs > num_threads_) ? num_threads_ : remaining_runs;
    if (graphics_) {
      run_thread = 1;
    }
    for (int i = 0; i < run_thread; i++) {
      const float width = outline_width * vary_factor_list[run_id++];
      const float height = outline_height;
      SACoreSoftMacro* sa = new SACoreSoftMacro(width,
                                                height,
                                                macros,
                                                1.0,
                                                1.0,
                                                0.0,
                                                0.0,
                                                0.0,
                                                0.0,
                                                0.0,
                                                0.0,  // penalty weight
                                                0.0,
                                                0.0,  // notch size
                                                pos_swap_prob,
                                                neg_swap_prob,
                                                double_swap_prob,
                                                exchange_swap_prob,
                                                resize_prob,
                                                init_prob_,
                                                max_num_step_,
                                                num_perturb_per_step,
                                                k_,
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
      if (sa->isValid()) {
        macro_tilings.insert(
            std::pair<float, float>(sa->getWidth(), sa->getHeight()));
      }
      // delete sa;  // avoid memory leakage
    }
    sa_vector.clear();
    remaining_runs -= run_thread;
  }

  run_thread = num_threads_;
  remaining_runs = num_runs_;
  run_id = 0;
  while (remaining_runs > 0) {
    std::vector<SACoreSoftMacro*> sa_vector;
    run_thread
        = (remaining_runs > num_threads_) ? num_threads_ : remaining_runs;
    if (graphics_) {
      run_thread = 1;
    }
    for (int i = 0; i < run_thread; i++) {
      const float width = outline_width;
      const float height = outline_height * vary_factor_list[run_id++];
      SACoreSoftMacro* sa = new SACoreSoftMacro(width,
                                                height,
                                                macros,
                                                1.0,
                                                1.0,
                                                0.0,
                                                0.0,
                                                0.0,
                                                0.0,
                                                0.0,
                                                0.0,  // penalty weight
                                                0.0,
                                                0.0,  // notch size
                                                pos_swap_prob,
                                                neg_swap_prob,
                                                double_swap_prob,
                                                exchange_swap_prob,
                                                resize_prob,
                                                init_prob_,
                                                max_num_step_,
                                                num_perturb_per_step,
                                                k_,
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
      if (sa->isValid()) {
        macro_tilings.insert(
            std::pair<float, float>(sa->getWidth(), sa->getHeight()));
      }
      // delete sa;  // avoid memory leakage
    }
    sa_vector.clear();
    remaining_runs -= run_thread;
  }

  std::vector<std::pair<float, float>> tilings(macro_tilings.begin(),
                                               macro_tilings.end());
  std::sort(tilings.begin(), tilings.end(), comparePairProduct);
  for (auto& shape : tilings) {
    logger_->report("width:  {} height: {}", shape.first, shape.second);
  }

  // we do not want very strange tilngs
  std::vector<std::pair<float, float>> new_tilings;
  for (auto& tiling : tilings) {
    if (tiling.second / tiling.first >= min_ar_
        && tiling.second / tiling.first <= 1.0 / min_ar_) {
      new_tilings.push_back(tiling);
    }
    logger_->report("tiling.second / tiling.first :  {} min_ar_: {}",
                    tiling.second / tiling.first,
                    min_ar_);
  }
  tilings = new_tilings;
  // update parent
  parent->setMacroTilings(tilings);
  if (tilings.size() == 0) {
    std::string line = "This is no valid tilings for cluster ";
    line += parent->getName();
    logger_->error(MPL, 2031, line);
  } else {
    std::string line = "The macro tiling for " + parent->getName() + "  ";
    for (auto& shape : tilings) {
      line += " < " + std::to_string(shape.first) + " , ";
      line += std::to_string(shape.second) + " >  ";
    }
    line += "\n";
    logger_->info(MPL, 2035, line);
  }
}

// Determine the macro tilings for each HardMacroCluster
// multi thread enabled
// random seed deterministic enabled
void HierRTLMP::calHardMacroClusterShape(Cluster* cluster)
{
  // Check if the cluster is a HardMacroCluster
  if (cluster->getClusterType() != HardMacroCluster) {
    return;
  }

  logger_->report("Enter CalHardMacroCluster : {}", cluster->getName());

  std::vector<HardMacro*> hard_macros = cluster->getHardMacros();
  // macro tilings
  std::set<std::pair<float, float>> macro_tilings;  // <width, height>
  // if the cluster only has one macro
  if (hard_macros.size() == 1) {
    float width = hard_macros[0]->getWidth();
    float height = hard_macros[0]->getHeight();
    logger_->report("width:  {} height: {}", width, height);

    std::vector<std::pair<float, float>> tilings;
    tilings.push_back(std::pair<float, float>(width, height));
    cluster->setMacroTilings(tilings);
    return;
  }
  // otherwise call simulated annealing to determine tilings
  // set the action probabilities
  const float action_sum = pos_swap_prob_ + neg_swap_prob_ + double_swap_prob_
                           + exchange_swap_prob_;
  const float pos_swap_prob = pos_swap_prob_ / action_sum;
  const float neg_swap_prob = neg_swap_prob_ / action_sum;
  const float double_swap_prob = double_swap_prob_ / action_sum;
  const float exchange_swap_prob = exchange_swap_prob_ / action_sum;
  // get outline constraint
  const float outline_width = root_cluster_->getWidth();
  const float outline_height = root_cluster_->getHeight();
  // We vary the outline of cluster to generate differnt tilings
  std::vector<float> vary_factor_list{1.0};
  float vary_step = 1.0 / num_runs_;  // change the outline by at most halfly
  for (int i = 1; i < num_runs_; i++) {
    vary_factor_list.push_back(1.0 - i * vary_step);
  }
  // update macros
  std::vector<HardMacro> macros;
  for (auto& macro : hard_macros) {
    macros.push_back(*macro);
  }
  const int num_perturb_per_step = (macros.size() > num_perturb_per_step_ / 10)
                                       ? macros.size()
                                       : num_perturb_per_step_ / 10;
  // const int num_perturb_per_step = macros.size() * 10; // increase the
  // num_perturb
  int run_thread = num_threads_;
  int remaining_runs = num_runs_;
  int run_id = 0;
  while (remaining_runs > 0) {
    std::vector<SACoreHardMacro*> sa_vector;
    run_thread
        = (remaining_runs > num_threads_) ? num_threads_ : remaining_runs;
    if (graphics_) {
      run_thread = 1;
    }
    for (int i = 0; i < run_thread; i++) {
      const float width = outline_width * vary_factor_list[run_id++];
      const float height = outline_height;
      SACoreHardMacro* sa = new SACoreHardMacro(width,
                                                height,
                                                macros,
                                                1.0,  // area_weight_
                                                1.0,
                                                0.0,
                                                0.0,
                                                0.0,  // penalty weight
                                                pos_swap_prob,
                                                neg_swap_prob,
                                                double_swap_prob,
                                                exchange_swap_prob,
                                                0.0,
                                                init_prob_,
                                                max_num_step_,
                                                num_perturb_per_step,
                                                k_,
                                                c_,
                                                random_seed_,
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
      if (sa->isValid()) {
        macro_tilings.insert(
            std::pair<float, float>(sa->getWidth(), sa->getHeight()));
      }
      // delete sa;  // avoid memory leakage
    }
    sa_vector.clear();
    remaining_runs -= run_thread;
  }

  run_thread = num_threads_;
  remaining_runs = num_runs_;
  run_id = 0;
  while (remaining_runs > 0) {
    std::vector<SACoreHardMacro*> sa_vector;
    run_thread
        = (remaining_runs > num_threads_) ? num_threads_ : remaining_runs;
    if (graphics_) {
      run_thread = 1;
    }
    for (int i = 0; i < run_thread; i++) {
      const float width = outline_width;
      const float height = outline_height * vary_factor_list[run_id++];
      SACoreHardMacro* sa = new SACoreHardMacro(width,
                                                height,
                                                macros,
                                                1.0,  // area_weight_
                                                1.0,
                                                0.0,
                                                0.0,
                                                0.0,  // penalty weight
                                                pos_swap_prob,
                                                neg_swap_prob,
                                                double_swap_prob,
                                                exchange_swap_prob,
                                                0.0,
                                                init_prob_,
                                                max_num_step_,
                                                num_perturb_per_step,
                                                k_,
                                                c_,
                                                random_seed_,
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
      if (sa->isValid()) {
        macro_tilings.insert(
            std::pair<float, float>(sa->getWidth(), sa->getHeight()));
      }
      // delete sa;  // avoid memory leakage
    }
    sa_vector.clear();
    remaining_runs -= run_thread;
  }

  std::vector<std::pair<float, float>> tilings(macro_tilings.begin(),
                                               macro_tilings.end());
  std::sort(tilings.begin(), tilings.end(), comparePairProduct);
  for (auto& shape : tilings) {
    logger_->report("width:  {}  height: {}", shape.first, shape.second);
  }

  // we do not want very strange tilngs
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
    std::string line = "This is no valid tilings for cluster ";
    line += cluster->getName();
    logger_->error(MPL, 2030, line);
  }
}

// Create pin blockage for Bundled IOs
void HierRTLMP::createPinBlockage()
{
  floorplan_lx_ = root_cluster_->getX();
  floorplan_ly_ = root_cluster_->getY();
  floorplan_ux_ = floorplan_lx_ + root_cluster_->getWidth();
  floorplan_uy_ = floorplan_ly_ + root_cluster_->getHeight();

  calculateConnection();
  // if the design has IO pads, we do not create pin blockage
  if (io_pad_map_.size() > 0) {
    return;
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
  // add the blockages into the blockages_
  const float v_blockage = (floorplan_uy_ - floorplan_ly_) * pin_access_th_;
  const float h_blockage = (floorplan_ux_ - floorplan_lx_) * pin_access_th_;
  float sum_range = 0.0;
  blockages_.push_back(Rect(floorplan_lx_,
                            pin_ranges[L].first,
                            floorplan_lx_ + h_blockage,
                            pin_ranges[L].second));
  blockages_.push_back(Rect(pin_ranges[T].first,
                            floorplan_uy_ - v_blockage,
                            pin_ranges[T].second,
                            floorplan_uy_));
  blockages_.push_back(Rect(floorplan_ux_ - h_blockage,
                            pin_ranges[R].first,
                            floorplan_ux_,
                            pin_ranges[R].second));
  blockages_.push_back(Rect(pin_ranges[B].first,
                            floorplan_ly_,
                            pin_ranges[B].second,
                            floorplan_ly_ + v_blockage));
  for (auto& [pin, range] : pin_ranges) {
    if (range.second > range.first) {
      sum_range += range.second - range.first;
    }
  }
  float net_sum = 0.0;
  for (auto& cluster : root_cluster_->getChildren()) {
    if (!cluster->getIOClusterFlag()) {
      continue;
    }
    for (auto& [cluster_id, num_net] : cluster->getConnection()) {
      net_sum += num_net;
    }
  }
  sum_range
      /= 2 * (floorplan_uy_ - floorplan_ly_ + floorplan_ux_ - floorplan_lx_);
  // update the pin_access_net_width_ratio_
  pin_access_net_width_ratio_ = sum_range / net_sum;
}

// Cluster Placement Engine Starts ...........................................
// The cluster placement is done in a top-down manner
// (Preorder DFS)

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

  // pin_access_th_ = pin_access_th_orig_ * parent->getStdCellArea() /
  //                 (parent->getStdCellArea() + parent->getMacroArea());
  // pin_access_th_ = pin_access_th_orig_
  //               * ( 1  - (parent->getStdCellArea() + parent->getMacroArea())
  //                 / (parent->getWidth() * parent->getHeight()));

  // set the instance property
  for (auto& cluster : parent->getChildren()) {
    logger_->report("Cluster: {}", cluster->getName());
    setInstProperty(parent);
  }
  // Place children clusters
  // map children cluster to soft macro
  for (auto& cluster : parent->getChildren()) {
    if (cluster->getIOClusterFlag()) {
      continue;
    }
    SoftMacro* macro = new SoftMacro(cluster);
    cluster->setSoftMacro(
        macro);  // we will detele the old one, no memory leakage
  }
  // detemine the settings for simulated annealing engine
  // Set outline information
  const float lx = parent->getX();
  const float ly = parent->getY();
  const float outline_width = parent->getWidth();
  const float outline_height = parent->getHeight();
  const float ux = lx + outline_width;
  const float uy = ly + outline_height;
  // For Debug
  if (1) {
    logger_->report(
        "[Debug][HierRTLMP::MultiLevelMacroPlacement] : {} {} {} {} {}",
        parent->getName(),
        lx,
        ly,
        outline_width,
        outline_height);
  }
  // finish Debug

  // Suppose the region, fence, guide has been mapped to cooresponding macros
  std::map<std::string, int> soft_macro_id_map;  // cluster_name, macro_id
  std::map<int, Rect> fences;
  std::map<int, Rect> guides;
  std::vector<SoftMacro> macros;
  std::vector<BundledNet> nets;
  std::vector<Rect> blockages;

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
      fences[macros.size() - 1]
          = fence;  // current macro id is macros.size() - 1
    }
    if (guide.isValid()) {
      guides[macros.size() - 1]
          = guide;  // current macro id is macros.size() - 1
    }
  }

  // update the connnection
  calculateConnection();
  logger_->report("finish calculate connection");
  updateDataFlow();
  logger_->report("finish updating dataflow");
  if (parent->getParent() != nullptr) {
    // the parent cluster is not the root cluster
    // First step model each pin access as the a dummy softmacro
    // In our simulated annealing engine, the dummary softmacro will
    // no effect on SA
    // We have four dummy SoftMacros based on our definition
    std::vector<PinAccess> pins = {L, T, R, B};
    for (auto& pin : pins) {
      soft_macro_id_map[toString(pin)] = macros.size();
      macros.push_back(SoftMacro(0.0, 0.0, toString(pin)));
    }
    // add the connections between pin access
    for (auto& [src_pin, pin_map] : parent->getBoundaryConnection()) {
      for (auto& [target_pin, old_weight] : pin_map) {
        float weight = old_weight;
        // if (macros[soft_macro_id_map[toString(src_pin)]].IsStdCellCluster()
        //  ||
        //    macros[soft_macro_id_map[toString(target_pin)]].IsStdCellCluster()
        //    )
        //  weight *= virtual_weight_;
        nets.push_back(BundledNet(soft_macro_id_map[toString(src_pin)],
                                  soft_macro_id_map[toString(target_pin)],
                                  weight));
      }
    }
  }
  // add the virtual connections
  for (const auto [cluster1, cluster2] : parent->getVirtualConnections()) {
    BundledNet net(soft_macro_id_map[cluster_map_[cluster1]->getName()],
                   soft_macro_id_map[cluster_map_[cluster2]->getName()],
                   virtual_weight_);
    net.src_cluster_id = cluster1;
    net.target_cluster_id = cluster2;
    nets.push_back(net);
  }

  for (auto inst : block_->getInsts()) {
    const int cluster_id
        = odb::dbIntProperty::find(inst, "cluster_id")->getValue();
    if (cluster_id == root_cluster_->getId()) {
      logger_->report("root  inst_name : {}", inst->getName());
    }
  }

  // convert the connections between clusters to SoftMacros
  for (auto& cluster : parent->getChildren()) {
    const int src_id = cluster->getId();
    const std::string src_name = cluster->getName();
    for (auto& [cluster_id, weight] : cluster->getConnection()) {
      logger_->report("********************************************");
      logger_->report(" {} {} {} ",
                      cluster->getName(),
                      cluster_map_[cluster_id]->getName(),
                      weight);
      logger_->report("\n");
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
  for (auto& net : nets) {
    logger_->report("net : {}  {}",
                    macros[net.terminals.first].getName(),
                    macros[net.terminals.second].getName());
  }

  if (parent->getParent() != nullptr) {
    // pin_access_th_ = 0.05;
    // update the size of each pin access macro
    // each pin access macro with have their fences
    // check the net connection

    std::map<int, float> net_map;
    net_map[soft_macro_id_map[toString(L)]] = 0.0;
    net_map[soft_macro_id_map[toString(T)]] = 0.0;
    net_map[soft_macro_id_map[toString(R)]] = 0.0;
    net_map[soft_macro_id_map[toString(B)]] = 0.0;
    for (auto& net : nets) {
      logger_->report(" {} {} ",
                      macros[net.terminals.first].getName(),
                      macros[net.terminals.second].getName());
      if (net_map.find(net.terminals.first) != net_map.end()) {
        net_map[net.terminals.first] += net.weight;
      }
      if (net_map.find(net.terminals.second) != net_map.end()) {
        net_map[net.terminals.second] += net.weight;
      }
    }

    for (auto [id, weight] : net_map) {
      logger_->report(" {} {} ", id, weight);
    }

    // float h_size = outline_width * pin_access_th_;
    // float v_size = outline_height * pin_access_th_;
    float std_cell_area = 0.0;
    float macro_area = 0.0;
    for (auto& cluster : parent->getChildren()) {
      std_cell_area += cluster->getStdCellArea();
      // macro_area += cluster->getMacroArea();
    }

    // std_cell_area = std_cell_area / (std_cell_area + macro_area) *
    // outline_width * outline_height;
    for (auto& shape : parent->getMacroTilings()) {
      if (shape.first <= outline_width && shape.second <= outline_height) {
        macro_area = shape.first * shape.second;
        break;
      }
    }

    float ratio = (1.0
                   - std::sqrt((macro_area + std_cell_area)
                               / (outline_width * outline_height)))
                  / 2.0;
    ratio = ratio / 2.0;
    const float h_size = outline_width * ratio;
    const float v_size = outline_height * ratio;

    const float l_size = net_map[soft_macro_id_map[toString(L)]] > 0
                             ? (outline_height - 2 * v_size) / 2.0
                             : 0.0;
    const float r_size = net_map[soft_macro_id_map[toString(R)]] > 0
                             ? (outline_height - 2 * v_size) / 2.0
                             : 0.0;
    const float b_size = net_map[soft_macro_id_map[toString(B)]] > 0
                             ? (outline_width - 2 * h_size) / 2.0
                             : 0.0;
    const float t_size = net_map[soft_macro_id_map[toString(T)]] > 0
                             ? (outline_width - 2 * h_size) / 2.0
                             : 0.0;

    logger_->report("pin_access_net_width_ratio_:  {}",
                    pin_access_net_width_ratio_);
    // update the size of pin access macro
    if (l_size > 0) {
      // l_size = v_size;
      macros[soft_macro_id_map[toString(L)]]
          = SoftMacro(h_size, l_size, toString(L));
      fences[soft_macro_id_map[toString(L)]]
          = Rect(0.0, 0.0, h_size, outline_height);
    }
    if (r_size > 0) {
      // r_size = v_size;
      macros[soft_macro_id_map[toString(R)]]
          = SoftMacro(h_size, r_size, toString(R));
      fences[soft_macro_id_map[toString(R)]]
          = Rect(outline_width - h_size, 0.0, outline_width, outline_height);
    }
    if (t_size > 0) {
      // t_size = h_size;
      macros[soft_macro_id_map[toString(T)]]
          = SoftMacro(t_size, v_size, toString(T));
      fences[soft_macro_id_map[toString(T)]]
          = Rect(0.0, outline_height - v_size, outline_width, outline_height);
    }
    if (b_size > 0) {
      // b_size = h_size;
      macros[soft_macro_id_map[toString(B)]]
          = SoftMacro(b_size, v_size, toString(B));
      fences[soft_macro_id_map[toString(B)]]
          = Rect(0.0, 0.0, outline_width, v_size);
    }
  }

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
  const float action_sum = pos_swap_prob_ + neg_swap_prob_ + double_swap_prob_
                           + exchange_swap_prob_ + resize_prob_;
  // set the penalty weight
  float weight_sum = outline_weight_ + wirelength_weight_ + guidance_weight_
                     + fence_weight_ + boundary_weight_ + notch_weight_;

  // We vary the target utilization to generate different tilings
  std::vector<float> target_utils{target_util_};
  std::vector<float> target_dead_spaces{target_dead_space_};
  for (int i = 1; i < num_target_util_; i++) {
    // if (target_util_ + i * target_util_step_ < 1.0)
    target_utils.push_back(target_util_ + i * target_util_step_);
  }
  for (int i = 1; i < num_target_dead_space_; i++) {
    if (target_dead_space_ + i * target_dead_space_step_ < 1.0) {
      target_dead_spaces.push_back(target_dead_space_
                                   + i * target_dead_space_step_);
    }
  }
  std::vector<float> target_util_list;  // target util has higher priority than
                                        // target_dead_space
  std::vector<float> target_dead_space_list;
  for (auto& target_util : target_utils) {
    for (auto& target_dead_space : target_dead_spaces) {
      target_util_list.push_back(target_util);
      target_dead_space_list.push_back(target_dead_space);
    }
  }

  float new_notch_weight = notch_weight_;
  for (auto& cluster : parent->getChildren()) {
    if (cluster->getClusterType() == HardMacroCluster) {
      new_notch_weight = weight_sum;
      break;
    }
  }

  const int num_perturb_per_step = (macros.size() > num_perturb_per_step_)
                                       ? macros.size()
                                       : num_perturb_per_step_;
  int run_thread = num_threads_;
  int remaining_runs = target_util_list.size();
  int run_id = 0;
  SACoreSoftMacro* best_sa = nullptr;
  float best_cost = std::numeric_limits<float>::max();
  logger_->report("Start Simulated Annealing Core");
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
      logger_->report("start run_id : {} ", run_id);
      const float target_util = target_util_list[run_id];
      const float target_dead_space = target_dead_space_list[run_id++];
      if (!shapeChildrenCluster(parent,
                                shaped_macros,
                                soft_macro_id_map,
                                target_util,
                                target_dead_space)) {
        continue;
      }
      logger_->report("finish run_id :  {}", run_id);
      weight_sum = 1.0;
      SACoreSoftMacro* sa
          = new SACoreSoftMacro(outline_width,
                                outline_height,
                                shaped_macros,
                                area_weight_,
                                outline_weight_ / weight_sum,
                                wirelength_weight_ / weight_sum,
                                guidance_weight_ / weight_sum,
                                fence_weight_ / weight_sum,
                                boundary_weight_ / weight_sum,
                                macro_blockage_weight_ / weight_sum,
                                new_notch_weight / weight_sum,
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
                                k_,
                                c_,
                                random_seed_,
                                graphics_.get(),
                                logger_);
      sa->setFences(fences);
      sa->setGuides(guides);
      sa->setNets(nets);
      sa->setBlockages(blockages);
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
      if (sa->isValid() && sa->getNormCost() < best_cost) {
        best_cost = sa->getNormCost();
        best_sa = sa;
      } else {
        // delete sa;  // avoid memory leakage
      }
    }
    sa_vector.clear();
    // add early stop mechanism
    if (best_sa != nullptr) {
      break;
    }
    remaining_runs -= run_thread;
  }
  logger_->report("Finish SA");

  if (best_sa == nullptr) {
    std::string line = "This is no valid tilings for cluster ";
    line += parent->getName();
    logger_->error(MPL, 2032, line);
  }
  // update the clusters and do bus planning
  std::vector<SoftMacro> shaped_macros;
  best_sa->fillDeadSpace();
  best_sa->alignMacroClusters();
  best_sa->getMacros(shaped_macros);
  logger_->report("Print Results");
  best_sa->printResults();

  file.open(file_name + ".fp.txt");
  for (auto& macro : shaped_macros) {
    file << macro.getName() << "   " << macro.getX() << "   " << macro.getY()
         << "   " << macro.getWidth() << "   " << macro.getHeight()
         << std::endl;
  }
  file.close();

  for (auto& macro : shaped_macros) {
    logger_->report(" {} {} {} {} {} ",
                    macro.getName(),
                    macro.getX(),
                    macro.getY(),
                    macro.getWidth(),
                    macro.getHeight());
    macro.printShape();
  }

  // just for test
  for (auto& cluster : parent->getChildren()) {
    *(cluster->getSoftMacro())
        = shaped_macros[soft_macro_id_map[cluster->getName()]];
    // For Debug
    if (1) {
      logger_->report("[Debug][HierRTLMP::MultiLevelMacroPlacement] Child : ");
      logger_->report("{} {} {} {} {}",
                      cluster->getName(),
                      cluster->getX(),
                      cluster->getY(),
                      cluster->getWidth(),
                      cluster->getHeight());
    }
  }

  // check if the parent cluster still need bus planning
  for (auto& child : parent->getChildren()) {
    if (child->getClusterType() == MixedCluster) {
      logger_->report("\n\n Call Bus Synthesis\n\n");
      // Call Path Synthesis to route buses
      callBusPlanning(shaped_macros, nets);
      break;
    }
  }

  // update the location of children cluster to their real location
  for (auto& cluster : parent->getChildren()) {
    cluster->setX(cluster->getX() + lx);
    cluster->setY(cluster->getY() + ly);
    logger_->report("lx :  {} ly: {}", lx, ly);
    // For Debug
    if (1) {
      logger_->report(
          "[Debug][HierRTLMP::MultiLevelMacroPlacement] Child : {} {} {}",
          cluster->getName(),
          cluster->getX(),
          cluster->getY());
    }
    // finish Debug
  }

  // for debug
  file.open("./" + report_directory_ + "/" + "pin_access.txt");
  for (auto& cluster : parent->getChildren()) {
    std::set<PinAccess> pin_access;
    for (auto& [cluster_id, pin_weight] : cluster->getPinAccessMap()) {
      pin_access.insert(pin_weight.first);
    }

    for (auto& [src, connection] : cluster->getBoundaryConnection()) {
      pin_access.insert(src);
      for (auto& [target, weight] : connection) {
        pin_access.insert(target);
      }
    }

    file << cluster->getName() << "   ";
    for (auto& pin : pin_access) {
      file << toString(pin) << "   ";
    }
    file << std::endl;

    logger_->report("pin access :  {}", cluster->getName());
    for (auto& pin : pin_access) {
      logger_->report(" {} ", toString(pin));
    }
    logger_->report("\n");
  }
  file.close();

  // Traverse the physical hierarchy tree in a DFS manner
  for (auto& cluster : parent->getChildren()) {
    if (cluster->getClusterType() == MixedCluster
        || cluster->getClusterType() == HardMacroCluster) {
      multiLevelMacroPlacement(cluster);
    }
  }

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
// Note that target_util is used for determining the size of MixedCluster
// the target_dead_space is used for determining the size of StdCellCluster
bool HierRTLMP::shapeChildrenCluster(
    Cluster* parent,
    std::vector<SoftMacro>& macros,
    std::map<std::string, int>& soft_macro_id_map,
    float target_util,
    float target_dead_space)
{
  logger_->report("Enter ShapeChildrenCluster");
  // check the valid values
  const float outline_width = parent->getWidth();
  const float outline_height = parent->getHeight();
  const float outline_area = outline_width * outline_height;
  logger_->report(
      "target_dead_space : {} target util {} outline width {} height {} macro "
      "area {} std cell area {}",
      target_dead_space,
      target_util,
      outline_width,
      outline_height,
      parent->getMacroArea(),
      parent->getStdCellArea());
  // (1) we need to calculate available space for std cells
  float macro_area_tot = 0.0;
  float std_cell_area_tot = 0.0;
  float std_cell_mixed_cluster = 0.0;
  // add the macro area for blockages, pin access and so on
  for (auto& macro : macros) {
    if (macro.getCluster() == nullptr) {
      macro_area_tot += macro.getArea();
    }
  }
  logger_->report("pin access area {} ", macro_area_tot);
  for (auto& cluster : parent->getChildren()) {
    if (cluster->getIOClusterFlag()) {
      continue;
    }
    if (cluster->getClusterType() == StdCellCluster) {
      std_cell_area_tot += cluster->getStdCellArea();
    } else if (cluster->getClusterType() == HardMacroCluster) {
      std::vector<std::pair<float, float>> shapes;
      for (auto& shape : cluster->getMacroTilings()) {
        if (shape.first < outline_width * (1 + conversion_tolerance_)
            && shape.second < outline_height * (1 + conversion_tolerance_)) {
          shapes.push_back(shape);
        }
      }
      if (shapes.size() == 0) {
        std::string line = "This is not enough space in cluster ";
        line += cluster->getName();
        line += "  num_tilings:  "
                + std::to_string(cluster->getMacroTilings().size());
        logger_->error(MPL, 2029, line);
      }
      macro_area_tot += shapes[0].first * shapes[0].second;
      cluster->setMacroTilings(shapes);
    } else {  // mixed cluster
      std_cell_area_tot
          += cluster->getStdCellArea() * std::pow(1 + 2 * pin_access_th_, 2);
      std_cell_mixed_cluster
          += cluster->getStdCellArea() * std::pow(1 + 2 * pin_access_th_, 2);
      std::vector<std::pair<float, float>> shapes;
      for (auto& shape : cluster->getMacroTilings()) {
        if (shape.first * (1 + 2 * pin_access_th_)
                < outline_width * (1 + conversion_tolerance_)
            && shape.second * (1 + 2 * pin_access_th_)
                   < outline_height * (1 + conversion_tolerance_)) {
          shapes.push_back(shape);
        }
      }
      if (shapes.size() == 0) {
        std::string line = "This is not enough space in cluster ";
        line += cluster->getName();
        line += "  num_tilings:  "
                + std::to_string(cluster->getMacroTilings().size());
        logger_->error(MPL, 2080, line);
      }
      macro_area_tot += shapes[0].first * shapes[0].second
                        * std::pow(1 + 2 * pin_access_th_, 2);
      cluster->setMacroTilings(shapes);
    }  // end for cluster type
  }
  // check how much available space to inflat for mixed cluster
  const float min_target_util
      = std_cell_mixed_cluster
        / (outline_area - std_cell_area_tot - macro_area_tot);
  if (target_util <= min_target_util) {
    target_util = min_target_util;
  }
  // else if (target_util >= 1.0) {
  //  target_util = 1.0;
  // }
  // check how many empty space left
  const float avail_space = outline_area - macro_area_tot
                            - (std_cell_area_tot - std_cell_mixed_cluster)
                            - std_cell_mixed_cluster / target_util;
  // make sure the target_dead_space in range
  if (target_dead_space <= 0.0) {
    target_dead_space = 0.01;
  } else if (target_dead_space >= 1.0) {
    target_dead_space = 0.99;
  }
  // shape clusters
  if (avail_space <= 0.0) {
    return false;  // no feasible solution
  }
  const float std_cell_inflat = avail_space * (1.0 - target_dead_space)
                                / (std_cell_area_tot - std_cell_mixed_cluster);
  logger_->report("std_cell_inflat : {}", std_cell_inflat);
  // set the shape for each macro
  for (auto& cluster : parent->getChildren()) {
    if (cluster->getIOClusterFlag()) {
      continue;
    }
    if (cluster->getClusterType() == StdCellCluster) {
      const float area = cluster->getArea() * (1.0 + std_cell_inflat);
      const float width = std::sqrt(area / min_ar_);
      std::vector<std::pair<float, float>> width_list;
      width_list.push_back(std::pair<float, float>(width, area / width));
      macros[soft_macro_id_map[cluster->getName()]].setShapes(width_list, area);
      logger_->report("[Debug][CalShape] std_cell_cluster : {}",
                      cluster->getName());
    } else if (cluster->getClusterType() == HardMacroCluster) {
      macros[soft_macro_id_map[cluster->getName()]].setShapes(
          cluster->getMacroTilings());
      logger_->report("[Debug][CalShape] hard_macro_cluster : {}",
                      cluster->getName());
      for (auto& shape : cluster->getMacroTilings()) {
        logger_->report(" ( {} , {} ) ", shape.first, shape.second);
      }
      logger_->report("\n");
    } else {  // Mixed cluster
      // float area = cluster->getStdCellArea() / target_util;
      const std::vector<std::pair<float, float>> tilings
          = cluster->getMacroTilings();
      std::vector<std::pair<float, float>> width_list;
      // area += tilings[0].first * tilings[0].second;
      // use the largest area
      float area = tilings[tilings.size() - 1].first
                   * tilings[tilings.size() - 1].second;
      area += cluster->getStdCellArea() / target_util;

      // leave the space for pin access
      area = area / std::pow(1 - 2 * pin_access_th_, 2);
      for (auto& shape : tilings) {
        if (shape.first * shape.second <= area) {
          width_list.push_back(std::pair<float, float>(
              shape.first / (1 - 2 * pin_access_th_),
              area / shape.second * (1 - 2 * pin_access_th_)));
          logger_->report(
              "shape.first {} shape.second {}", shape.first, shape.second);
        }
      }

      logger_->report(
          "[Debug][CalShape] name:  {} area: {}", cluster->getName(), area);
      logger_->report("width_list :  ");
      for (auto& width : width_list) {
        logger_->report(" [  {} {}  ] ", width.first, width.second);
      }
      macros[soft_macro_id_map[cluster->getName()]].setShapes(width_list, area);
    }
  }

  logger_->report("Finish shape function");
  return true;
}

// Call Path Synthesis to route buses
void HierRTLMP::callBusPlanning(std::vector<SoftMacro>& shaped_macros,
                                std::vector<BundledNet>& nets_old)
{
  std::vector<BundledNet> nets;
  for (auto& net : nets_old) {
    logger_->report("net.weight : {}", net.weight);
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
    logger_->error(MPL, 2029, "Fail !!! Cannot do bus planning !!!");
  }
}

// place macros within the HardMacroCluster
void HierRTLMP::hardMacroClusterMacroPlacement(Cluster* cluster)
{
  // Check if the cluster is a HardMacroCluster
  if (cluster->getClusterType() != HardMacroCluster) {
    return;
  }

  logger_->report("Enter the HardMacroClusterMacroPlacement :  {}",
                  cluster->getName());

  // get outline constraint
  const float lx = cluster->getX();
  const float ly = cluster->getY();
  const float outline_width = cluster->getWidth();
  const float outline_height = cluster->getHeight();
  const float ux = lx + outline_width;
  const float uy = ly + outline_height;
  std::vector<HardMacro> macros;
  std::vector<Cluster*> macro_clusters;
  std::map<int, int> cluster_id_macro_id_map;
  std::vector<HardMacro*> hard_macros = cluster->getHardMacros();
  num_hard_macros_cluster_ += hard_macros.size();
  std::map<int, Rect> fences;
  std::map<int, Rect> guides;
  // create a cluster for each macro
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
    if (temp_cluster->getIOClusterFlag()) {
      continue;
    }
    // model other cluster as a fixed macro with zero size
    cluster_id_macro_id_map[cluster_id] = macros.size();
    macros.push_back(HardMacro(
        std::pair<float, float>(
            temp_cluster->getX() + temp_cluster->getWidth() / 2.0 - lx,
            temp_cluster->getY() + temp_cluster->getHeight() / 2.0 - ly),
        temp_cluster->getName()));
    logger_->report(
        "terminal : lx = {} ly = {}",
        temp_cluster->getX() + temp_cluster->getWidth() / 2.0 - lx,
        temp_cluster->getY() + temp_cluster->getHeight() / 2.0 - ly);
  }
  // create bundled net
  std::vector<BundledNet> nets;
  for (auto macro_cluster : macro_clusters) {
    const int src_id = macro_cluster->getId();
    for (auto [cluster_id, weight] : macro_cluster->getConnection()) {
      if (cluster_map_[cluster_id]->getIOClusterFlag()) {
        continue;
      }
      BundledNet net(cluster_id_macro_id_map[src_id],
                     cluster_id_macro_id_map[cluster_id],
                     weight);
      net.src_cluster_id = src_id;
      net.target_cluster_id = cluster_id;
      nets.push_back(net);
    }  // end connection
  }    // end macro cluster
  // set global configuration
  // set the action probabilities
  const float action_sum = pos_swap_prob_ + neg_swap_prob_ + double_swap_prob_
                           + exchange_swap_prob_ + flip_prob_;
  // set the penalty weight
  const float weight_sum = area_weight_ + outline_weight_ + wirelength_weight_
                           + guidance_weight_ + fence_weight_;
  // We vary the outline of cluster to generate differnt tilings
  std::vector<float> vary_factor_list{1.0};
  float vary_step
      = 0.2 / num_runs_;  // change the outline by at most 10 percent
  for (int i = 1; i <= num_runs_ / 2 + 1; i++) {
    vary_factor_list.push_back(1.0 + i * vary_step);
    vary_factor_list.push_back(1.0 - i * vary_step);
  }
  const int num_perturb_per_step = (macros.size() > num_perturb_per_step_ / 10)
                                       ? macros.size()
                                       : num_perturb_per_step_ / 10;
  // const int num_perturb_per_step = macros.size();
  // num_threads_  = 1;
  int run_thread = num_threads_;
  int remaining_runs = num_runs_;
  int run_id = 0;
  SACoreHardMacro* best_sa = nullptr;
  float best_cost = std::numeric_limits<float>::max();
  while (remaining_runs > 0) {
    std::vector<SACoreHardMacro*> sa_vector;
    run_thread
        = (remaining_runs > num_threads_) ? num_threads_ : remaining_runs;
    if (graphics_) {
      run_thread = 1;
    }
    for (int i = 0; i < run_thread; i++) {
      const float width = outline_width * vary_factor_list[run_id++];
      const float height = outline_width * outline_height / width;
      SACoreHardMacro* sa
          = new SACoreHardMacro(width,
                                height,
                                macros,
                                area_weight_ / weight_sum,
                                outline_weight_ / weight_sum,
                                wirelength_weight_ / weight_sum,
                                guidance_weight_ / weight_sum,
                                fence_weight_ / weight_sum,
                                pos_swap_prob_ / action_sum,
                                neg_swap_prob_ / action_sum,
                                double_swap_prob_ / action_sum,
                                exchange_swap_prob_ / action_sum,
                                flip_prob_ / action_sum,
                                init_prob_,
                                max_num_step_,
                                num_perturb_per_step,
                                k_,
                                c_,
                                random_seed_,
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
      if (sa->isValid() && sa->getNormCost() < best_cost) {
        best_cost = sa->getNormCost();
        best_sa = sa;
      } else {
        // delete sa;  // avoid memory leakage
      }
    }
    sa_vector.clear();
    remaining_runs -= run_thread;
  }
  // update the hard macro
  if (best_sa == nullptr) {
    std::string line = "This is no valid tilings for cluster ";
    line += cluster->getName();
    logger_->error(MPL, 2033, line);
  } else {
    std::vector<HardMacro> best_macros;
    best_sa->getMacros(best_macros);
    best_sa->printResults();
    for (int i = 0; i < hard_macros.size(); i++) {
      *(hard_macros[i]) = best_macros[i];
    }
    // delete best_sa;
  }
  // update OpenDB
  for (auto& hard_macro : hard_macros) {
    num_updated_macros_++;
    hard_macro->setX(hard_macro->getX() + lx);
    hard_macro->setY(hard_macro->getY() + ly);
    hard_macro->updateDb(pitch_x_, pitch_y_);
  }
  setInstProperty(cluster);
}

void HierRTLMP::setDebug()
{
  int dbu = db_->getTech()->getDbUnitsPerMicron();
  graphics_ = std::make_unique<Graphics>(dbu, logger_);
}

}  // namespace mpl2
