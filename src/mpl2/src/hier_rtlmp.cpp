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


#include <sys/stat.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <queue>
#include <string>
#include <tuple>
#include <vector>
#include <thread>


// Partitioner note : currently we are still using MLPart to partition large flat clusters
// Later this will be replaced by our TritonPart
//#include "par/MLPart.h"
#include "odb/db.h"
#include "sta/ArcDelayCalc.hh"
#include "sta/Bfs.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/ExceptionPath.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/PathAnalysisPt.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/PatternMatch.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchPred.hh"
#include "sta/Sequential.hh"
#include "sta/Sta.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"
#include "object.h"
#include "SimulatedAnnealingCore.h"
#include "SACoreHardMacro.h"
#include "SACoreSoftMacro.h"

namespace mpl {

///////////////////////////////////////////////////////////
// Class HierRTLMP
using utl::MPL;


// Constructors
HierRTLMP::HierRTLMP(ord::dbNetwork* network,  
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
void HierRTLMP::SetGlobalFence(float fence_lx, float fence_ly,
                               float fence_ux, float fence_uy) 
{
  global_fence_lx_ = fence_lx;
  global_fence_ly_ = fence_ly;
  global_fence_ux_ = fence_ux;
  global_fence_uy_ = fence_uy;
}

void HierRTLMP::SetHaloWidth(float halo_width)
{
  halo_width_ = halo_width;
}


// Options related to clustering
void HierRTLMP::SetNumBundledIOsPerBoundary(int num_bundled_ios)
{
  num_bundled_IOs_ = num_bundled_ios;  
}

void HierRTLMP::SetTopLevelClusterSize(int max_num_macro, 
                                       int min_num_macro,
                                       int max_num_inst,  
                                       int min_num_inst)
{
  max_num_macro_base_ = max_num_macro;
  min_num_macro_base_ = min_num_macro;
  max_num_inst_base_  = max_num_inst;
  min_num_inst_base_  = min_num_inst;
}

void HierRTLMP::SetClusterSizeTolerance(float tolerance) 
{
  tolerance_ = tolerance;   
}
     
void HierRTLMP::SetMaxNumLevel(int max_num_level) 
{
  max_num_level_ = max_num_level;  
}
     
void HierRTLMP::SetClusterSizeRatioPerLevel(float coarsening_ratio) 
{
  coarsening_ratio_ = coarsening_ratio;  
}
     
void HierRTLMP::SetLargeNetThreshold(int large_net_threshold) 
{
  large_net_threshold_ = large_net_threshold;  
}
    
void HierRTLMP::SetSignatureNetThreshold(int signature_net_threshold) 
{
  signature_net_threshold_ = signature_net_threshold;
}


///////////////////////////////////////////////////////////////
// Top Level Interface function
void HierRTLMP::HierRTLMacroPlacer() 
{
  // Get the database information
  block_ = db_->getChip()->getBlock();
  dbu_ = db_->getTech()->getDbUnitsPerMicron();

  // Get the floorplan information
  odb::Rect die_box = block_->getDieArea();
  floorplan_lx_ = Dbu2Micro(die_box.xMin(), dbu_);
  floorplan_ly_ = Dbu2Micro(die_box.yMin(), dbu_);
  floorplan_ux_ = Dbu2Micro(die_box.xMax(), dbu_);
  floorplan_uy_ = Dbu2Micro(die_box.yMax(), dbu_);
  logger_->info(MPL,
                2023,
                "Basic Floorplan Information\n"
                "Floorplan_lx : {}\n"
                "Floorplan_lx : {}\n"
                "Floorplan_lx : {}\n"
                "Floorplan_lx : {}",
                floorplan_lx_,
                floorplan_ly_,
                floorplan_ux_,
                floorplan_uy_);

  // Compute metrics for dbModules
  // Associate all the hard macros with their HardMacro objects
  // and report the statistics
  metric_ = ComputeMetric(block_->getTopModule());
  float util = metric_->GetStdCellArea() + metric_->GetMacroArea();
  util /= floorplan_uy_ - floorplan_ly_;
  util /= floorplan_ux_ - floorplan_lx_;
  logger_->info(MPL,
                402,
                "Traversed logical hierarchy\n"
                "\tNumber of std cell instances : {}\n"
                "\tArea of std cell instances : {}\n"
                "\tNumber of macros : {}\n"
                "\tArea of macros : {}\n"
                "\tTotal area : {}\n"
                "\tUtilization : {}\n",
                metric_->GetNumStdCell(),
                metric_->GetStdCellArea(),
                metric_->GetNumMacro(),
                metric_->GetMacroArea(),
                metric_->GetStdCellArea() + metric_->GetMacroArea(),
                util);

  // Initialize the physcial hierarchy tree
  // create root cluster
  cluster_id_ = 0;
  // set the level of cluster to be 0
  root_cluster_ = new Cluster(cluster_id_, std::string("root"));
  // set the design metric as the metric for the root cluster
  root_cluster_->AddDbModule(block_->getTopModule());
  root_cluster_->SetMetric(*metric_);
  cluster_map_[cluster_id_++] = root_cluster_;
  // assign cluster_id property of each instance
  for (auto inst : block_->getInsts())
     odb::dbIntProperty::create(inst, "cluster_id", cluster_id_);
 
  // model each bundled IO as a cluster under the root node
  // cluster_id:  0 to num_bundled_IOs x 4 are reserved for bundled IOs
  // following the sequence:  Left -> Top -> Right -> Bottom
  // starting at (floorplan_lx_, floorplan_ly_)
  // Map IOs to Pads if the design has pads
  MapIOPads();
  CreateBundledIOs();

  // Create physical hierarchy tree in a post-order DFS manner
  MultiLevelCluster(root_cluster_);  // Recursive call for creating the tree 

  // Break leaf clusters into a standard-cell cluster and a hard-macro cluster
  // And merge macros into clusters based on connection signatures and footprints
  // We assume there is no direct connections between macros
  // Based on types of designs, we support two types of breaking up 
  // Suppose current cluster is A
  // Type 1:  Replace A by A1, A2, A3
  // Type 2:  Create a subtree for A
  //          A  ->   A 
  //               /  |  \ 
  //              A1  A2  A3
  LeafClusterStdCellHardMacroSep(root_cluster_);

  // Print the created physical hierarchy tree
  // Thus user can debug the clustering results
  // and they can tune the options to get better
  // clustering results
  PrintPhysicalHierarchyTree(root_cluster_, 0);

  // Map the macros in each cluster to their HardMacro objects
  for (auto& [cluster_id, cluster] : cluster_map_)
    MapMacroInCluster2HardMacro(cluster);

  // Place macros in a hierarchical mode based on the above
  // physcial hierarchical tree
  // The macro placement is done in a DFS manner (pre-order)
  // First set the SoftMacro object for root_cluster_
  SoftMacro* root_soft_macro = new SoftMacro(root_cluster_);
  float root_lx = std::max(floorplan_lx_, global_fence_lx_);
  float root_ly = std::max(floorplan_ly_, global_fence_ly_);
  float root_ux = std::min(floorplan_ux_, global_fence_ux_);
  float root_uy = std::min(floorplan_uy_, global_fence_uy_);
  float root_area = (root_ux - root_lx) * (root_uy - root_ly);
  float root_ar = (root_uy - root_lx) / (root_ux - root_lx);
  std::vector<std::pair<float, float> > root_ars { 
                 std::pair<float, float>(root_ar, root_ar) };
  root_soft_macro->SetArea(root_area);
  root_soft_macro->SetAspectRatios(root_ars);
  root_soft_macro->SetWidth(root_ux - root_lx); // This will set height automatically
  root_soft_macro->SetX(root_lx);
  root_soft_macro->SetY(root_ly);
  root_cluster_->SetSoftMacro(root_soft_macro);
  MultiLevelMacroPlacement(root_cluster_);
    
  // Clear the memory to avoid memory leakage
  // release all the pointers
  // metric map
  for (auto& [module, metric] : logical_module_map_)
    delete metric;
  logical_module_map_.clear();
  // hard macro map
  for (auto& [inst, hard_macro] : hard_macro_map_)
    delete hard_macro;
  hard_macro_map_.clear();
  // delete all clusters
  for (auto& [cluster_id, cluster] : cluster_map_)
    delete cluster;
  cluster_map_.clear();
}


////////////////////////////////////////////////////////////////////////
// Private functions
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
// Hierarchical clustering related functions

// Traverse Logical Hierarchy
// Recursive function to collect the design metrics (number of std cells, 
// area of std cells, number of macros and area of macros) in the logical hierarchy
Metric* HierRTLMP::ComputeMetric(odb::dbModule* module) 
{
  unsigned int num_std_cell = 0;
  float std_cell_area = 0.0;
  unsigned int num_macro = 0;
  float macro_area = 0.0;
 
  for (odb::dbInst* inst : module->getInsts()) {
    const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
    odb::dbMaster* master = inst->getMaster();
    // check if the instance is a pad, a cover or 
    // empty block (such as marker)
    if (master->isPad() || master->isCover() || 
        (master->isBlock() && liberty_cell == nullptr))
      continue; 

    float inst_area = liberty_cell->area();
    if (master->isBlock()) { // a macro
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
  for (odb::dbModInst* inst : module->getChildren()) {
    Metric* metric = ComputeMetric(inst->getMaster());
    num_std_cell  += metric->GetNumStdCell();
    std_cell_area += metric->GetStdCellArea();
    num_macro     += metric->GetNumMacro();
    macro_area    += metric->GetMacroArea();
  }

  Metric* metric = new Metric(num_std_cell, num_macro, std_cell_area, macro_area);
  logical_module_map_[module] = metric;
  return metric;
}

// compute the metric for a cluster
// Here we do not include any Pads,  Covers or Marker
// number of standard cells
// number of macros
// area of standard cells
// area of macros
void HierRTLMP::SetClusterMetric(Cluster* cluster) 
{ 
  unsigned int num_std_cell = 0;
  unsigned int num_macro    = 0;
  float std_cell_area       = 0.0;
  float macro_area          = 0.0;
  num_std_cell += cluster->GetLeafStdCells().size();
  num_macro    += cluster->GetLeafMacros().size();
  for (odb::dbInst* inst : cluster->GetLeafStdCells())  {
    const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
    std_cell_area += liberty_cell->area();
  }
  for (odb::dbInst* inst : cluster->GetLeafMacros())  {
    const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
    macro_area += liberty_cell->area();
  }
  Metric metric(num_std_cell, num_macro, std_cell_area, macro_area);
  for(auto& module : cluster->GetDbModules())
    metric.AddMetric(*logical_module_map_[module]);
  // update metric based on design type
  if (cluster->GetClusterType() == HardMacroCluster) 
    cluster->SetMetric(Metric(0, metric.GetNumMacro(),
                          0.0, metric.GetMacroArea()));
  else if (cluster->GetClusterType() == StdCellCluster)
    cluster->SetMetric(Metric(metric.GetNumStdCell(), 0,
                          metric.GetStdCellArea(), 0.0));
  else
    cluster->SetMetric(metric);
}

// Handle IOS
// Map IOs to Pads for designs with IO pads
// If the design does not have IO pads, 
// do nothing
void HierRTLMP::MapIOPads() 
{
  // Check if this design has IO pads
  bool is_pad_design = false;
  for (auto inst : block_->getInsts()) {
    if (inst->getMaster()->isPad() == true) {
      is_pad_design = true;
      break;
    }
  }
  if (is_pad_design == false)
    return;
    
  for (odb::dbNet* net : block_->getNets()) {
    if (net->getBTerms().size() == 0)
      continue;

    // If the design has IO pads, there is a net only
    // connecting the IO pin and IO pad instance
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      for (odb::dbITerm* iterm : net->getITerms()) {
        odb::dbInst* inst = iterm->getInst();
        io_pad_map_[bterm] = inst;
      } 
    }
  }
}

// Create Bunded IOs following the order : L, T, R, B
// We will have num_bundled_IOs_ x 4 clusters for bundled IOs
void HierRTLMP::CreateBundledIOs() 
{
  // Get the floorplan information
  odb::Rect die_box = block_->getCoreArea();
  int core_lx = die_box.xMin();
  int core_ly = die_box.yMin();
  int core_ux = die_box.xMax();
  int core_uy = die_box.yMax();
  const int x_base = (floorplan_ux_ - floorplan_lx_) / num_bundled_IOs_;
  const int y_base = (floorplan_uy_ - floorplan_ly_) / num_bundled_IOs_;
  
  // Map all the BTerms / Pads to Bundled IOs (cluster)
  std::vector<std::string> prefix_vec;
  prefix_vec.push_back(std::string("L"));
  prefix_vec.push_back(std::string("T"));
  prefix_vec.push_back(std::string("R"));
  prefix_vec.push_back(std::string("B"));
  for (int i = 0; i < 4; i++) { // four boundaries (Left, Top, Right and Bottom in order)
    for (int j = 0; j < num_bundled_IOs_; j++) {
      std::string cluster_name = prefix_vec[i] + std::to_string(j);
      Cluster* cluster = new Cluster(cluster_id_, cluster_name);
      root_cluster_->AddChild(cluster);
      cluster->SetParent(root_cluster_);
      cluster_map_[cluster_id_++] = cluster;
      int x = 0.0;
      int y = 0.0;
      if (i == 0) {
        x = floorplan_lx_;
        y = floorplan_ly_ + y_base * (j + 0.5);
      } else if (i == 1) {
        x = floorplan_lx_ + x_base * (j + 0.5);
        y = floorplan_uy_;
      } else if (i == 2) {
        x = floorplan_ux_;
        y = floorplan_uy_ - y_base * (j + 0.5);
      } else {
        x = floorplan_ux_ - x_base * (j + 0.5);
        y = floorplan_ly_;
      }
      // set the cluster to a IO cluster
      cluster->SetIOClusterFlag(std::pair<float, float>(
                Dbu2Micro(x, dbu_), Dbu2Micro(y, dbu_)));
    }  
  }

  // Map all the BTerms to bundled IOs
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
    // If the design with Pads
    if (io_pad_map_.find(term) != io_pad_map_.end()) {
      lx = io_pad_map_[term]->getBBox()->xMin();
      ly = io_pad_map_[term]->getBBox()->yMin();
      ux = io_pad_map_[term]->getBBox()->xMax();
      uy = io_pad_map_[term]->getBBox()->yMax();
      if (lx <= core_lx)
        lx = floorplan_lx_;
      if (ly <= core_ly)
        ly = floorplan_ly_;
      if (ux >= core_ux)
        ux = floorplan_ux_;
      if (uy >= core_uy)
        uy = floorplan_uy_;
    }
    // calculate cluster id based on the location of IO Pins / Pads
    int cluster_id = -1;
    if (lx == floorplan_lx_) {
      // The IO is on the left boundary
      int cluster_id_base = 0; // based on the sequence we defined
      cluster_id = cluster_id_base + std::floor(((ly + uy) / 2.0 - floorplan_ly_) / y_base);
    } else if (uy == floorplan_uy_) {
      // The IO is on the top boundary
      int cluster_id_base = num_bundled_IOs_;
      cluster_id = cluster_id_base + std::floor(((lx + ux) / 2.0 - floorplan_lx_) / x_base);
    } else if (ux == floorplan_ux_) {
      // The IO is on the right boundary
      int cluster_id_base = num_bundled_IOs_ * 2; // based on the sequence we defined
      cluster_id = cluster_id_base + std::floor((floorplan_uy_ - (ly + uy) / 2.0) / y_base);
    } else if (ly == floorplan_ly_) {
      // The IO is on the bottom boundary
      int cluster_id_base = num_bundled_IOs_ * 3; // based on the sequence we defined
      cluster_id = cluster_id_base + std::floor((floorplan_ux_ - (lx + ux) / 2.0) / x_base);
    }
    // Check if the IO pins / Pads exist
    if (cluster_id == -1)
      logger_->error(
        MPL, 2024, "Floorplan has not been initialized? Pin location error.");
    else 
      odb::dbIntProperty::create(term, "cluster_id", cluster_id);
  }
}


// Create physical hierarchy tree in a post-order DFS manner
// Recursive call for creating the physical hierarchy tree
void HierRTLMP::MultiLevelCluster(Cluster* parent) 
{
  if (level_ >= max_num_level_) // limited by the user-specified parameter
    return;  
  level_++;
  // a large coarsening_ratio_ helps the clustering process converge fast
  max_num_macro_ = max_num_macro_base_ / std::pow(coarsening_ratio_, level_ - 1);
  min_num_macro_ = min_num_macro_base_ / std::pow(coarsening_ratio_, level_ - 1);
  max_num_inst_  = max_num_inst_base_  / std::pow(coarsening_ratio_, level_ - 1);
  min_num_inst_  = min_num_inst_base_  / std::pow(coarsening_ratio_, level_ - 1);
  // We define the tolerance to improve the robustness of our hierarchical clustering
  max_num_inst_  = max_num_inst_  * (1 + tolerance_);
  min_num_inst_  = min_num_inst_  * (1 - tolerance_);
  max_num_macro_ = max_num_macro_ * (1 + tolerance_);
  min_num_macro_ = min_num_macro_ * (1 - tolerance_);

  if (parent->GetNumMacro() > max_num_macro_ ||
      parent->GetNumStdCell() > max_num_inst_) {
    BreakCluster(parent);  // Break the parent cluster into children clusters
    UpdateSubTree(parent); // update the subtree to the physical hierarchy tree
    for (auto& child : parent->GetChildren())
      SetInstProperty(child);
    // print the basic information of the children cluster
    for (auto& child : parent->GetChildren()) {
      child->PrintBasicInformation(logger_);
      MultiLevelCluster(child);
    }
  } else {
    MultiLevelCluster(parent);  
  }
 
  SetInstProperty(parent);
  level_--;
}

// update the cluster_id property of insts in the cluster
// We have three types of clusters
// Note that HardMacroCluster will not have any logical modules
void HierRTLMP::SetInstProperty(Cluster* cluster) 
{
  int cluster_id = cluster->GetId();
  ClusterType cluster_type = cluster->GetClusterType();
  if (cluster_type == HardMacroCluster ||
      cluster_type == MixedCluster) 
    for (auto& inst : cluster->GetLeafMacros())
      odb::dbIntProperty::find(inst, "cluster_id")->setValue(cluster_id);

  if (cluster_type == StdCellCluster ||
      cluster_type == MixedCluster) 
    for (auto& inst : cluster->GetLeafStdCells())
      odb::dbIntProperty::find(inst, "cluster_id")->setValue(cluster_id);
    
  if (cluster_type == StdCellCluster) 
    for (auto& module : cluster->GetDbModules())
      SetInstProperty(module, cluster_id, false);
  else if (cluster_type == MixedCluster) 
    for (auto& module : cluster->GetDbModules())
      SetInstProperty(module, cluster_id, true);
}


// update the cluster_id property of insts in a logical module
void HierRTLMP::SetInstProperty(odb::dbModule* module, int cluster_id, 
                                bool include_macro) 
{
  if (include_macro == true) {
    for (odb::dbInst* inst : module->getInsts())
      odb::dbIntProperty::find(inst, "cluster_id")->setValue(cluster_id);
  } else { // only consider standard cells
    for (odb::dbInst* inst : module->getInsts()) {
      const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
      odb::dbMaster* master = inst->getMaster();
      // check if the instance is a Pad, Cover or empty block (such as marker)
      // or macro
      if (master->isPad() || master->isCover() || master->isBlock())
        continue;
      odb::dbIntProperty::find(inst, "cluster_id")->setValue(cluster_id);
    }
  }
  for (odb::dbModInst* inst : module->getChildren())
    SetInstProperty(inst->getMaster(), cluster_id, include_macro);
}


// Break the parent cluster into children clusters
// We expand the parent cluster into a subtree based on logical
// hierarchy in a DFS manner.  During the expansion process,
// we merge small clusters in the same logical hierarchy
void HierRTLMP::BreakCluster(Cluster* parent)
{
  // Consider three different cases:
  // (a) parent is an empty cluster
  // (b) parent is a cluster corresponding to a logical module
  // (c) parent is a cluster generated by merging small clusters
  if ((parent->GetLeafStdCells().size() == 0) &&
      (parent->GetLeafMacros().size()   == 0) &&
      (parent->GetDbModules().size()    == 0)) {
    // (a) parent is an empty cluster, do nothing 
    // In the normal operation, this case should not happen
    return;
  } else if ((parent->GetLeafStdCells().size() == 0) &&
             (parent->GetLeafMacros().size()   == 0) &&
             (parent->GetDbModules().size()    == 1)) {
    // (b) parent is a cluster corresponding to a logical module
    // For example, the root cluster_
    odb::dbModule* module = parent->GetDbModules()[0];
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
        if (master->isPad() || master->isCover() || 
           (master->isBlock() && liberty_cell == nullptr))
          continue;
        else if (master->isBlock())
          parent->AddLeafMacro(inst);
        else
          parent->AddLeafStdCell(inst); 
      } 
      parent->ClearDbModules();  // remove module from the parent cluster
      SetInstProperty(parent);
      return; 
    } 
    // (b.2) if the logical module has child logical modules,
    // we first model each child logical module as a cluster
    for (odb::dbModInst* child : module->getChildren()) {
      std::string cluster_name = child->getMaster()->getHierarchicalName();
      Cluster* cluster = new Cluster(cluster_id_, cluster_name);
      cluster->AddDbModule(child->getMaster());
      SetInstProperty(cluster);
      SetClusterMetric(cluster);
      cluster_map_[cluster_id_++] = cluster;
      // modify the physical hierarchy tree
      cluster->SetParent(parent);
      parent->AddChild(cluster);
    }    
    // Check the glue logics
    std::string cluster_name = std::string("(") + parent->GetName() + ")_glue_logic";
    Cluster* cluster = new Cluster(cluster_id_, cluster_name);
    for (odb::dbInst* inst : module->getInsts()) {
      const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
      odb::dbMaster* master = inst->getMaster();
      // check if the instance is a Pad, Cover or empty block (such as marker)
      if (master->isPad() || master->isCover() || 
         (master->isBlock() && liberty_cell == nullptr))
        continue;
      else if (master->isBlock())
        cluster->AddLeafMacro(inst);
      else
        cluster->AddLeafStdCell(inst); 
    }
    // if the module has no meaningful glue instances
    if (cluster->GetLeafStdCells().size() == 0 && 
        cluster->GetLeafMacros().size()   == 0 ) {
      delete cluster;
    } else {
      SetInstProperty(cluster);
      SetClusterMetric(cluster);
      cluster_map_[cluster_id_++] = cluster;
      // modify the physical hierarchy tree
      cluster->SetParent(parent);
      parent->AddChild(cluster);
    }
  } else {
    // (c) parent is a cluster generated by merging small clusters
    // parent cluster has few logical modules or many glue insts
    for (auto& module : parent->GetDbModules()) {
      std::string cluster_name = module->getHierarchicalName();
      Cluster* cluster = new Cluster(cluster_id_, cluster_name);
      cluster->AddDbModule(module);
      SetInstProperty(cluster);
      SetClusterMetric(cluster);
      cluster_map_[cluster_id_++] = cluster;
      // modify the physical hierachy tree   
      cluster->SetParent(parent);
      parent->AddChild(cluster);
    }
    // Check glue logics
    if ((parent->GetLeafStdCells().size() > 0) ||
        (parent->GetLeafMacros().size()   > 0)) {
      std::string cluster_name = std::string("(") + parent->GetName() + ")_glue_logic";
      Cluster* cluster = new Cluster(cluster_id_, cluster_name);
      for (auto& inst : parent->GetLeafStdCells()) 
        cluster->AddLeafStdCell(inst);
      for (auto& inst : parent->GetLeafMacros())
        cluster->AddLeafMacro(inst);
      SetInstProperty(cluster);
      SetClusterMetric(cluster);
      cluster_map_[cluster_id_++] = cluster;
      // modify the physical hierachy tree   
      cluster->SetParent(parent);
      parent->AddChild(cluster);
    }
  }
   
  // Merge small clusters
  std::vector<Cluster*> candidate_clusters;
  for (auto& cluster : parent->GetChildren()) 
    if (cluster->GetNumStdCell() < min_num_inst_ &&
        cluster->GetNumMacro()   < min_num_macro_)
      candidate_clusters.push_back(cluster);
  MergeClusters(candidate_clusters);
 
  // Recursively break down large clusters with logical modules
  // For large flat cluster, we will break it in the UpdateSubTree function
  for (auto& child : parent->GetChildren()) 
    if (child->GetDbModules().size() > 0) 
      if (child->GetNumStdCell() > max_num_inst_ ||
          child->GetNumMacro()   > max_num_macro_)
        BreakCluster(child);
  
  // Update the cluster_id 
  // This is important to maintain the clustering results
  SetInstProperty(parent);
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
//         For example, if we merge small clusters A and B,  A and B will have exactly
//         the same connections relative to all other clusters (both small clusters and
//         well-formed clusters). In this case, if A and B have the same connection 
//         signature, A and C have the same connection signature, then B and C also
//         have the same connection signature.
// Note in both types, we only merge clusters with the same parent cluster
void HierRTLMP::MergeClusters(std::vector<Cluster*>& candidate_clusters) {
  int num_candidate_clusters = candidate_clusters.size();
  while (true) {
    CalculateConnection(); // update the connections between clusters
    std::vector<int> cluster_class(num_candidate_clusters, -1); // merge flag
    // Firstly we perform Type 1 merge
    for (int i = 0; i < num_candidate_clusters; i++) {
      Cluster* cluster = candidate_clusters[i]->GetCloseCluster(
                               candidate_clusters, large_net_threshold_);
      if (cluster != nullptr) {
        cluster->MergeCluster(*candidate_clusters[i]);
        cluster_map_.erase(candidate_clusters[i]->GetId());
        delete candidate_clusters[i];
        SetInstProperty(cluster);
        SetClusterMetric(cluster);
        cluster_class[i] = cluster->GetId();
      }
    }
    // Then we perform Type 2 merge
    std::vector<Cluster*> new_candidate_clusters;
    for (int i = 0; i < num_candidate_clusters; i++) {
      if (cluster_class[i] == -1) { // the cluster has not been merged
        new_candidate_clusters.push_back(candidate_clusters[i]);
        for (int j = i + 1; j < num_candidate_clusters; j++) {
          if (cluster_class[j] != -1)
            continue;
          bool flag = candidate_clusters[i]->IsSameConnSignature(
                      *candidate_clusters[j], large_net_threshold_);
          if (flag == true) {
            cluster_class[j] = i;
            candidate_clusters[i]->MergeCluster(*candidate_clusters[j]);
            cluster_map_.erase(candidate_clusters[j]->GetId());
            delete candidate_clusters[j];
            SetInstProperty(candidate_clusters[i]);
            SetClusterMetric(candidate_clusters[i]);
          }
        }
      } 
    }
    
    // If no more clusters have been merged, exit the merging loop
    if (num_candidate_clusters == new_candidate_clusters.size())
      break; 
    // Update the candidate clusters
    // Some clusters have become well-formed clusters
    candidate_clusters.clear();
    for (auto& cluster : new_candidate_clusters) 
      if (cluster->GetNumStdCell() < min_num_inst_ &&
          cluster->GetNumMacro()   < min_num_macro_)
        candidate_clusters.push_back(cluster);
    num_candidate_clusters = candidate_clusters.size();
  }
}


// Calculate Connections between clusters
void HierRTLMP::CalculateConnection() {
  // Initialize the connections of all clusters
  for (auto & [cluster_id, cluster] : cluster_map_)
    cluster->InitConnection();
  
  // Traverse all nets through OpenDB
  for (odb::dbNet* net : block_->getNets()) {
    // ignore all the power net
    if (net->getSigType().isSupply())
      continue;
    int driver_id = -1; // cluster id of the driver instance
    std::vector<int> loads_id; // cluster id of sink instances
    bool pad_flag = false;
    // check the connected instances
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
      odb::dbMaster* master = inst->getMaster();
      // check if the instance is a Pad, Cover or empty block (such as marker)
      // We ignore nets connecting Pads, Covers, or markers
      if (master->isPad() || master->isCover() || 
         (master->isBlock() && liberty_cell == nullptr)) {
        pad_flag = true;
        break;
      }
      const int cluster_id = odb::dbIntProperty::find(inst, "cluster_id")->getValue();
      if (iterm->getIoType() == odb::dbIoType::OUTPUT)
        driver_id = cluster_id;
      else
        loads_id.push_back(cluster_id);
    }
    if (pad_flag == true)
      continue; // the nets with Pads should be ignored  
    // check the connected IO pins
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const int cluster_id = odb::dbIntProperty::find(bterm, "cluster_id")->getValue();
      if (bterm->getIoType() == odb::dbIoType::INPUT)
        driver_id = cluster_id;
      else
        loads_id.push_back(cluster_id);
    }
    // add the net to connections between clusters
    if (driver_id != -1 && loads_id.size() > 0 && 
        loads_id.size() < large_net_threshold_) {
      for (int i = 0; i < loads_id.size(); i++) {
        if (loads_id[i] != driver_id) { // we model the connections as undirected edges
          cluster_map_[driver_id]->AddConnection(loads_id[i], 1.0);
          cluster_map_[loads_id[i]]->AddConnection(driver_id, 1.0);
        }
      } // end sinks
    } // end adding current net
  } // end net traversal
}


// This function has two purposes:
// 1) remove all the internal clusters between parent and leaf clusters in its subtree
// 2) Call MLPart to partition large flat clusters (a cluster with no logical modules)
//    Later MLPart will be replaced by TritonPart
void HierRTLMP::UpdateSubTree(Cluster* parent) 
{
  std::vector<Cluster*> children_clusters;
  std::vector<Cluster*> internal_clusters;
  std::queue<Cluster*> wavefront;
  for (auto child : parent->GetChildren()) 
    wavefront.push(child);
  
  while (wavefront.empty() == false) {
    Cluster* cluster = wavefront.front();
    wavefront.pop();
    if (cluster->GetChildren().size() == 0) {
      children_clusters.push_back(cluster);  
    } else {
      internal_clusters.push_back(cluster);
      for (auto child : cluster->GetChildren()) 
        wavefront.push(child);
    }
  }
  
  // delete all the internal clusters
  for (auto& cluster : internal_clusters) {
    cluster_map_.erase(cluster->GetId());
    delete cluster;
  }
 
  parent->RemoveChildren();
  parent->AddChildren(children_clusters);
  for (auto& cluster : children_clusters) {
    cluster->SetParent(parent);
    if (cluster->GetNumStdCell() > max_num_inst_)
      BreakLargeFlatCluster(cluster);
  }
}

// Break large flat clusters with MLPart 
// A flat cluster does not have a logical module
void HierRTLMP::BreakLargeFlatCluster(Cluster* parent)
{
  // Check if the cluster is a large flat cluster
  if (parent->GetDbModules().size() > 0 ||
      parent->GetLeafStdCells().size() < max_num_inst_)
    return;

  return;
  std::map<int, int> cluster_vertex_id_map;
  std::map<odb::dbInst*, int> inst_vertex_id_map;
  const int parent_cluster_id = parent->GetId();
  std::vector<odb::dbInst*> std_cells = parent->GetLeafStdCells();
  std::vector<int> col_idx;  // edges represented by vertex indices
  std::vector<int> row_ptr;  // pointers for edges
  // vertices
  // other clusters behaves like fixed vertices
  // We do not consider vertices only between fixed vertices
  int num_vertices = cluster_map_.size() - 1;
  num_vertices += parent->GetLeafMacros().size();
  num_vertices += std_cells.size();
  int vertex_id = 0;
  for (auto& [cluster_id, cluster] : cluster_map_)
    cluster_vertex_id_map[cluster_id] = vertex_id++;
  for (auto& macro : parent->GetLeafMacros())
    inst_vertex_id_map[macro] = vertex_id++;
  int num_fixed_vertices = vertex_id; // we set these fixed vertices to part0
  for (auto& std_cell : std_cells)
    inst_vertex_id_map[std_cell] = vertex_id++;
  std::vector<int> part(num_vertices, -1);
  for (int i = 0; i < num_fixed_vertices; i++)
    part[i] = 0;
  
  // Traverse nets to create edges (col_idx, row_ptr)
  row_ptr.push_back(col_idx.size());
  for (odb::dbNet* net : block_->getNets()) {
    // ignore all the power net
    if (net->getSigType().isSupply())
      continue;
    int driver_id = -1; // vertex id of the driver instance
    std::vector<int> loads_id; // vertex id of the sink instances
    bool pad_flag = false;
    // check the connected instances
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
      odb::dbMaster* master = inst->getMaster();
      // check if the instance is a Pad, Cover or empty block (such as marker)
      // if the nets connects to such pad, cover or empty block,
      // we should ignore such net
      if (master->isPad() || master->isCover() || 
         (master->isBlock() && liberty_cell == nullptr)) {
        pad_flag = true;
        break; // here CAN NOT be continue
      }
      const int cluster_id = odb::dbIntProperty::find(inst, "cluster_id")->getValue();
      int vertex_id = (cluster_id == parent_cluster_id) ?
                      cluster_vertex_id_map[cluster_id] :
                      inst_vertex_id_map[inst];
      if (iterm->getIoType() == odb::dbIoType::OUTPUT)
        driver_id = vertex_id;
      else
        loads_id.push_back(vertex_id);
    }
    // ignore the nets with IO pads
    if (pad_flag == true)
      continue;
    // check the connected IO pins
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const int cluster_id = odb::dbIntProperty::find(bterm, "cluster_id")->getValue();
      if (bterm->getIoType() == odb::dbIoType::INPUT)
        driver_id = cluster_vertex_id_map[cluster_id];
      else
        loads_id.push_back(cluster_vertex_id_map[cluster_id]);
    }
    // add the net as a hyperedge
    if (driver_id != -1 && loads_id.size() > 0 &&
            loads_id.size() < large_net_threshold_) {
       col_idx.push_back(driver_id);
       col_idx.insert(col_idx.end(), loads_id.begin(), loads_id.end());
       row_ptr.push_back(col_idx.size());
    }
  }   

  // we do not specify any weight for vertices or hyperedges
  std::vector<float> vertex_weight(part.size(), 1.0);
  std::vector<float> edge_weight(row_ptr.size() - 1, 1.0);
  // MLPart only support 2-way partition
  const int npart = 2;
  double balance_array[2] = {0.5, 0.5};
  double tolerance = 0.05;
  unsigned int seed = 0; // for fixed results
  // interface function of MLPart
  /*
  UMpack_mlpart(num_vertices,
                row_ptr.size() - 1, // num_hyperedges
                vertex_weight.data(),
                row_ptr.data(),
                col_idx.data(),
                edge_weight.data(),
                npart,
                balance_array,
                tolerance,
                part.data(),
                1,  // Starts Per Run #TODO: add a tcl command
                1,  // Number of Runs
                0,  // Debug Level
                seed);
  */
  // create cluster based on partitioning solutions
  // Note that all the std cells are stored in the leaf_std_cells_ for a flat cluster
  parent->ClearLeafStdCells();
  // we follow binary coding method to differentiate different parts
  // of the cluster
  // cluster_name_0, cluster_name_1
  // cluster_name_0_0, cluster_name_0_1, cluster_name_1_0, cluster_name_1_1
  const std::string cluster_name = parent->GetName();
  // set the parent cluster for part 0
  // update the name of parent cluster
  parent->SetName(cluster_name + std::string("_0"));
  // create a new cluster for part 1
  Cluster* cluster_part_1 = new Cluster(cluster_id_, cluster_name + std::string("_1"));
  // we do not need to touch the fixed vertices (they have been assigned before)
  for (int i = num_fixed_vertices; i < part.size(); i++) 
    if (part[i] == 0)
      parent->AddLeafStdCell(std_cells[i - num_fixed_vertices]);
    else 
      cluster_part_1->AddLeafStdCell(std_cells[i - num_fixed_vertices]);
  // update the property of parent cluster
  SetInstProperty(parent);
  SetClusterMetric(parent);
  // update the property of cluster_part_1
  SetInstProperty(cluster_part_1);
  SetClusterMetric(cluster_part_1);
  cluster_map_[cluster_id_++] = cluster_part_1;
  cluster_part_1->SetParent(parent->GetParent());
  parent->GetParent()->AddChild(cluster_part_1);

  // Recursive break the cluster
  // until the size of the cluster is less than max_num_inst_
  BreakLargeFlatCluster(parent);
  BreakLargeFlatCluster(cluster_part_1);
}


// Traverse the physical hierarchy tree in a DFS manner
// Split macros and std cells in the leaf clusters
// In the normal operation, we call this function after
// creating the physical hierarchy tree
void HierRTLMP::LeafClusterStdCellHardMacroSep(Cluster* root_cluster)
{
  if (root_cluster->GetChildren().size() == 0  ||
      root_cluster->GetNumMacro() == 0)
    return;
    
  // Traverse the physical hierarchy tree in a DFS manner (post-order)
  std::vector<Cluster*> leaf_clusters;
  for (auto& child : root_cluster->GetChildren()) {
    SetInstProperty(child);
    if (child->GetNumMacro() > 0) {
      if (child->GetChildren().size() == 0)
        leaf_clusters.push_back(child);
      else
        LeafClusterStdCellHardMacroSep(child);
    }
  }

  // after this step, the std cells and macros for leaf cluster has been
  // seperated. We need to update the metrics of the design if necessary
  // for each leaf clusters with macros, first group macros based on 
  // connection signatures and macro sizes
  for (auto& cluster : leaf_clusters) {
    // based on the type of current cluster,
    Cluster* parent_cluster = cluster;  // std cell dominated cluster
                                // add a subtree
    // if the cluster is macro dominated cluster
    // based macro_dominated_cluster_threshold_ (defualt = 0.01)
    if (cluster->GetNumStdCell() * macro_dominated_cluster_threshold_
        < cluster->GetNumMacro())
      parent_cluster = cluster->GetParent(); // replacement

    // Map macros into the HardMacro obects
    // and get all the HardMacros
    MapMacroInCluster2HardMacro(cluster);
    std::vector<HardMacro*> hard_macros = cluster->GetHardMacros();
    // create a cluster for each macro
    std::vector<Cluster*> macro_clusters;
    for (auto& hard_macro : hard_macros) {
      std::string cluster_name = hard_macro->GetName();
      Cluster* macro_cluster = new Cluster(cluster_id_, cluster_name);
      macro_cluster->AddLeafMacro(hard_macro->GetInst());
      SetInstProperty(macro_cluster);
      SetClusterMetric(macro_cluster);
      cluster_map_[cluster_id_++] = macro_cluster;
      // modify the physical hierachy tree   
      macro_cluster->SetParent(parent_cluster);
      parent_cluster->AddChild(macro_cluster);
      macro_clusters.push_back(macro_cluster);
    }
    
    // classify macros based on size
    std::vector<int> macro_size_class(hard_macros.size(), -1); 
    for (int i = 0; i < hard_macros.size(); i++) 
      if (macro_size_class[i] == -1) 
        for (int j = i + 1; j < hard_macros.size(); j++)
          if (macro_size_class[j] == -1 && hard_macros[i] == hard_macros[j])
            macro_size_class[j] = i;
    
    for (int i = 0; i < hard_macros.size(); i++)
      macro_size_class[i] = (macro_size_class[i] == -1) ? i : macro_size_class[i];

    // classify macros based on connection signature
    std::vector<int> macro_signature_class(hard_macros.size(), -1);
    for (int i = 0; i < hard_macros.size(); i++) {
      if (macro_signature_class[i] == -1) { 
        macro_signature_class[i] = i;
        for (int j = i + 1; i < hard_macros.size(); i++) {
          if (macro_signature_class[j] != -1)
            continue;
          bool flag = macro_clusters[i]->IsSameConnSignature(
                          *macro_clusters[j], large_net_threshold_);
          if (flag == true) 
            macro_signature_class[j] = i;
        }
      }
    }

    // macros with the same size and the same connection signature
    // belong to the same class
    std::vector<int> macro_class(hard_macros.size(), -1);
    for (int i = 0; i < hard_macros.size(); i++) {
      if (macro_class[i] == -1) {
        macro_class[i] = i;
        for (int j = i + 1; i < hard_macros.size(); i++) { 
          if (macro_class[j] == -1 &&
              macro_size_class[i] == macro_size_class[j] &&
              macro_signature_class[i] == macro_signature_class[j]) {
            macro_class[j] = i;
            macro_clusters[i]->MergeCluster(*macro_clusters[j]);
            // remove the merged macro cluster
            cluster_map_.erase(macro_clusters[j]->GetId());
            delete macro_clusters[j];
          }
        }
      } 
    }
    
    // clear the hard macros in current leaf cluster
    cluster->ClearHardMacros(); 
    // Restore the structure of physical hierarchical tree
    // Thus the order of leaf clusters will not change the final
    // macro grouping results (This is very important !!!
    // Don't touch the next line SetInstProperty command!!!)
    SetInstProperty(cluster);

    // Never use SetInstProperty Command in following lines 
    // for above reason!!!
    // based on different types of designs, we handle differently
    // whether a replacement or a subtree
    // Deal with the standard cell cluster
    if (parent_cluster == cluster) {
      // If we need a subtree , add standard cell cluster
      std::string std_cell_cluster_name = cluster->GetName();
      Cluster* std_cell_cluster = new Cluster(cluster_id_, 
                                      std_cell_cluster_name);
      std_cell_cluster->CopyInstances(*cluster);
      std_cell_cluster->ClearLeafMacros();
      std_cell_cluster->SetClusterType(StdCellCluster);
      SetClusterMetric(std_cell_cluster);
      cluster_map_[cluster_id_++] = std_cell_cluster;
      // modify the physical hierachy tree   
      std_cell_cluster->SetParent(parent_cluster);
      parent_cluster->AddChild(std_cell_cluster);
    } else {
      // If we need add a replacement
      cluster->ClearLeafMacros();
      cluster->SetClusterType(StdCellCluster);
      SetClusterMetric(cluster);
      // In this case, we do not to modify the physical hierarchy tree
    }
    // Deal with macro clusters
    for (int i = 0; i < macro_class.size(); i++) {
      if (macro_class[i] != i)
        continue;  // this macro cluster has been merged
      macro_clusters[i]->SetClusterType(HardMacroCluster);
      SetClusterMetric(macro_clusters[i]);
    }
  }

  // Set the inst property back
  SetInstProperty(root_cluster);
}


// Map all the macros into their HardMacro objects for all the clusters
void HierRTLMP::MapMacroInCluster2HardMacro(Cluster* cluster) 
{
  if (cluster->GetClusterType() == StdCellCluster)
    return;

  std::vector<HardMacro*> hard_macros;
  for (const auto& macro : cluster->GetLeafMacros())
    hard_macros.push_back(hard_macro_map_[macro]);
  for (const auto& module : cluster->GetDbModules())
    GetHardMacros(module, hard_macros);
  cluster->SpecifyHardMacros(hard_macros);
}

// Get all the hard macros in a logical module
void HierRTLMP::GetHardMacros(odb::dbModule* module, 
               std::vector<HardMacro*>& hard_macros)
{
  for (odb::dbInst* inst : module->getInsts()) {
    const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
    odb::dbMaster* master = inst->getMaster();
    // check if the instance is a pad or empty block (such as marker)
    if (master->isPad() || master->isCover() || 
       (master->isBlock() && liberty_cell == nullptr))
      continue;
    if (master->isBlock()) 
      hard_macros.push_back(hard_macro_map_[inst]);
  }
   
  for (odb::dbModInst* inst : module->getChildren()) 
    GetHardMacros(inst->getMaster(), hard_macros);
}


// Print Physical Hierarchy tree
void HierRTLMP::PrintPhysicalHierarchyTree(Cluster* parent, int level) 
{
  std::string line = "";
  for (int i = 0; i < level; i++)
    line += "+----";
  line += parent->GetName();
  logger_->info(MPL, 2025, line);
  for (auto& cluster : parent->GetChildren())
    PrintPhysicalHierarchyTree(cluster, level + 1);
}


/////////////////////////////////////////////////////////////////////////////
// Macro Placement related functions
// Place macros in a hierarchical mode based on the above
// physcial hierarchical tree 
// The macro placement is done in a DFS manner (PreOrder)
void HierRTLMP::MultiLevelMacroPlacement(Cluster* parent)
{
  // If the parent cluster has no macros (parent cluster is a StdCellCluster)
  // We do not need to determine the positions and shapes of its children clusters
  if (parent->GetHardMacros().size() == 0)
    return;
  
  // update the cluster_id of instances in the cluster
  SetInstProperty(parent);
  
  // Handle children clusters
  // Step1 : determine the shape (area, possible aspect ratios) of each cluster
  for (auto& cluster : parent->GetChildren())
    CalClusterShape(cluster);  // determine the shape of a cluster 
  // Step2 : determine positions and implementation of each children cluster
  PlaceChildrenClusters(parent);
  // Step3 : check if we need path synthesis to determine the routing paths
  // of buses for some MixedCluster childrens
  bool path_synthesis_flag = false;
  for (auto& cluster : parent->GetChildren()) {
    if (cluster->GetClusterType() == MixedCluster) {
      path_synthesis_flag = true;
      break;
    }
  }  
  if (path_synthesis_flag == true) 
    CallPathSynthesis(parent); // route buses within the parent cluster
  // Step4 : Determine the orientation and position of each hard macro
  // in each HardMacroCluster
  for (auto& cluster : parent->GetChildren())
    if (cluster->GetClusterType() == HardMacroCluster)
      PlaceHardMacros(cluster);

  // Traverse the physical hierarchy tree in a DFS manner
  for (auto& cluster : parent->GetChildren())
    if (cluster->GetClusterType() == MixedCluster)
      MultiLevelMacroPlacement(cluster);

  // done this branch and update the cluster_id property back
  SetInstProperty(parent);
}


// Determine the macro tilings within each cluster with macros
// multi thread enabled
// random seed deterministic enabled
void HierRTLMP::CalMacroTilings(Cluster* cluster) 
{
  // Check if the cluster is an IO cluster or StdCellCluster
  if (cluster->GetIOClusterFlag() == true ||
      cluster->GetClusterType() == StdCellCluster)
    return;
  
  std::vector<HardMacro*> hard_macros = cluster->GetHardMacros();
  // macro tilings
  std::set<std::pair<float, float> > macro_tilings; // <width, height>
  // if the cluster only has one macro
  if (hard_macros.size() == 1) {
    float width = hard_macros[0]->GetWidth();
    float height = hard_macros[0]->GetHeight();
    std::vector<std::pair<float, float> > tilings;
    tilings.push_back(std::pair<float, float>(width, height));
    cluster->SetMacroTilings(tilings);
    return;
  }

  // set the action probabilities
  const float action_sum  = pos_swap_prob_ + neg_swap_prob_ + double_swap_prob_
                            + exchange_swap_prob_;
  const float pos_swap_prob      = pos_swap_prob_ / action_sum;
  const float neg_swap_prob      = neg_swap_prob_ / action_sum;
  const float double_swap_prob   = double_swap_prob_ / action_sum;
  const float exchange_swap_prob = exchange_swap_prob_ / action_sum;
  // get outline constraint
  const float outline_width = cluster->GetWidth();
  const float outline_height = cluster->GetHeight();
  // We vary the outline of cluster to generate differnt tilings
  std::vector<float> vary_factor_list { 1.0 };
  float vary_step = 1.0 / num_runs_;  // change the outline by at most halfly
  for (int i = 1; i <= num_runs_ / 2 + 1; i++) {
    vary_factor_list.push_back(1.0 + i * vary_step);
    vary_factor_list.push_back(1.0 - i * vary_step);
  }
  // update macros
  std::vector<HardMacro> macros;
  for (auto& macro : hard_macros)
    macros.push_back(*macro);
  const int num_perturb_per_step = (macros.size() > num_perturb_per_step_) ?
                                      macros.size() : num_perturb_per_step_;
  int run_thread = num_threads_;
  int remaining_runs = num_runs_;
  int run_id = 0;
  while (remaining_runs > 0) {
    std::vector<SACoreHardMacro*> sa_vector;
    run_thread = (remaining_runs > num_threads_) ? num_threads_ : remaining_runs;
    for (int i = 0; i < run_thread; i++) {
      const float width = outline_width * vary_factor_list[run_id++];
      const float height = outline_width * outline_height / width;
      SACoreHardMacro* sa = new SACoreHardMacro(width, height, macros,
                                                1.0, 0.0, 0.0, 0.0,  // penalty weight
                                                pos_swap_prob, neg_swap_prob,
                                                double_swap_prob, exchange_swap_prob, 0.0,
                                                init_prob_, max_num_step_,
                                                num_perturb_per_step,
                                                k_, c_, random_seed_);
      sa_vector.push_back(sa);
    }
    // multi threads 
    std::vector<std::thread> threads;
    for (auto& sa : sa_vector)
      threads.push_back(std::thread(RunSA<SACoreHardMacro>, sa));
    for (auto& th : threads)
      th.join();
    // add macro tilings
    for (auto& sa : sa_vector) {
      macro_tilings.insert(std::pair<float, float>(sa->GetWidth(), 
                                                   sa->GetHeight()));
      delete sa; // avoid memory leakage
    }
    sa_vector.clear();
    remaining_runs -= run_thread;
  }
  // sort tilings based on area and aspect ratios
  // remove duplicate tilings
  std::vector<std::pair<float, float> > unique_tilings;
  for (auto& tiling : macro_tilings)
    if (tiling.first  <= outline_width && 
        tiling.second <= outline_height)
      unique_tilings.push_back(tiling);

  std::sort(unique_tilings.begin(), unique_tilings.end(), SortShape);
  cluster->SetMacroTilings(unique_tilings);
}

// Determine the shape of each child cluster
void HierRTLMP::CalClusterShape(Cluster* parent) 
{
  return;
}


// Determine positions and implementation of each children cluster
void HierRTLMP::PlaceChildrenClusters(Cluster* parent)
{
  // Get physical information
  const float lx = parent->GetX();
  const float ly = parent->GetY();
  const float ux = lx + parent->GetWidth();
  const float uy = ly + parent->GetHeight();
  const float outline_width = ux - lx;
  const float outline_height = uy - ly;
  // Suppose the region, fence, guide has been mapped to cooresponding macros
  // Eech blockage will be modeled as SoftMacro with fence
  std::map<std::string, int> soft_macro_id_map;
  std::map<int, Rect> fences;
  std::map<int, Rect> guides;
  // Eech blockage will be modeled as SoftMacro with fence
  std::vector<SoftMacro> macros;
  int blockage_id = 1;
  for (auto& blockage : blockages_) {
    // calculate the overlap
    const float b_lx = std::max(lx, blockage.xMin());
    const float b_ly = std::max(ly, blockage.yMin());
    const float b_ux = std::min(ux, blockage.xMax());
    const float b_uy = std::min(uy, blockage.yMax());
    if ((b_ux - b_lx > 0.0) && (b_uy - b_ly > 0.0)) {
      std::string name = std::string("blockage_") + std::to_string(blockage_id++);
      macros.push_back(SoftMacro(b_ux - b_lx, b_uy - b_ly, name));
      soft_macro_id_map[name] = macros.size() - 1;
      fences[macros.size() - 1] = Rect(b_lx - lx, b_ly - ly, b_ux - lx, b_uy - ly);
    }
  }

  // Model each pin access as a movable macro with fence
  std::set<PinAccess> pin_access_set;
  for (auto& [cluster_id, pin_access] : parent->GetPinAccessMap())
    pin_access_set.insert(pin_access);
  for (auto& pin_access : pin_access_set) {
    std::string name = "L";
    if (pin_access == L) {
      name = "L";
      macros.push_back(SoftMacro(outline_height * pin_access_th_,
                                 outline_width  * pin_access_th_,
                                 name));
      fences[macros.size() - 1] = Rect(0.0,  0.0, outline_width * pin_access_th_, 
                                       outline_height);
    } else if (pin_access == T) {
      name = "T";
      macros.push_back(SoftMacro(outline_width * pin_access_th_,
                                 outline_height  * pin_access_th_,
                                 name));
      fences[macros.size() - 1] = Rect(0.0, outline_height - outline_height  * pin_access_th_,
                                       outline_width, outline_height);
    } else if (pin_access == R) {
      name = "R";
      macros.push_back(SoftMacro(outline_height * pin_access_th_,
                                 outline_width  * pin_access_th_,
                                 name));
      fences[macros.size() - 1] = Rect(outline_width - outline_width * pin_access_th_, 0.0,
                                       outline_width, outline_height);
    } else {
      name = "B";
      macros.push_back(SoftMacro(outline_width * pin_access_th_,
                                 outline_height  * pin_access_th_,
                                 name));
      fences[macros.size() - 1] = Rect(0.0, outline_height  * pin_access_th_,
                                       outline_width, outline_height  * pin_access_th_);
    }
    soft_macro_id_map[name] = macros.size() - 1;
  }

  // handle fences and guides for hard macros
  for (auto& cluster : parent->GetChildren()) {
    if (cluster->GetIOClusterFlag() == true) {
      macros.push_back(*cluster->GetSoftMacro());
      soft_macro_id_map[cluster->GetName()] = macros.size() - 1;
      continue;
    }
    SoftMacro* soft_macro = new SoftMacro(cluster);
    SetInstProperty(cluster);
    macros.push_back(*soft_macro);
    cluster->SetSoftMacro(soft_macro);
    soft_macro_id_map[cluster->GetName()] = macros.size() - 1;
    // merge fences and guides for hard macros within cluster
    if (cluster->GetClusterType() == StdCellCluster)
      continue;
    Rect fence(-1.0, -1.0, -1.0, -1.0);
    Rect guide(-1.0, -1.0, -1.0, -1.0);
    const std::vector<HardMacro*> hard_macros = cluster->GetHardMacros();
    for (auto& hard_macro : hard_macros) {
      if (fences_.find(hard_macro->GetName()) != fences_.end())
        fence.Merge(fences_[hard_macro->GetName()]);
      if (guides_.find(hard_macro->GetName()) != guides_.end())
        guide.Merge(guides_[hard_macro->GetName()]);
    }
    fence.Relocate(lx, ly, ux, uy);
    guide.Relocate(lx, ly, ux, uy);
    if (fence.IsValid())
      fences[macros.size() - 1] = fence;
    if (guide.IsValid())
      guides[macros.size() - 1] = guide;
  }

  // Determine the locations for each cluster
  // set the action probabilities
  const float action_sum  = pos_swap_prob_ + neg_swap_prob_ + double_swap_prob_
                            + exchange_swap_prob_ + resize_prob_;
  // set the penalty weight
  const float weight_sum   =  outline_weight_ + wirelength_weight_
                              + guidance_weight_   + fence_weight_    
                              + boundary_weight_   + notch_weight_;
  // We vary the target utilization to generate different layouts
  std::vector<float> target_util_list { target_util_ };
  float util_step = (1.0 - target_util_) * 2 / num_runs_;  // change util by at most halfly
  for (int i = 1; i <= num_runs_ / 2 + 1; i++) {
    target_util_list.push_back(1.0 + i * util_step);
    target_util_list.push_back(1.0 - i * util_step);
  }

  // Processing nets
  // update the connections
  std::vector<BundledNet> nets;
  CalculateConnection();
  for (auto& cluster : parent->GetChildren()) {
    const int src_id = cluster->GetId();
    const std::string src_name = cluster->GetName();
    for (auto& [cluster_id, weight] : cluster->GetConnection()) {
      if (src_id >= cluster_id)
        continue;
      const std::string name = cluster_map_[cluster_id]->GetName();
      if (soft_macro_id_map.find(name) == soft_macro_id_map.end()) {
        // connection with pin access
        const PinAccess pin_access = parent->GetPinAccess(cluster_id);
        nets.push_back(BundledNet(soft_macro_id_map[src_name],
                       soft_macro_id_map[to_string(pin_access)], weight));
      } else { // net between children clusters
        nets.push_back(BundledNet(soft_macro_id_map[src_name],
                                  soft_macro_id_map[name], weight));
      }
    }
  }
  MergeNets(nets);


  // Place each clusters in multithreads
  std::vector<SACoreSoftMacro*> candidates;
  int run_thread = num_threads_;
  int remaining_runs = num_runs_;
  int run_id = 0;
  while (remaining_runs > 0) {
    run_thread = (remaining_runs > num_threads_) ? num_threads_ : remaining_runs;
    std::vector<SACoreSoftMacro*> sa_vector;
    for (int i = 0; i < run_thread; i++) {
      // determine the shapes of each SoftMacro
      std::vector<SoftMacro> macros;
      
      const int num_perturb_per_step = (macros.size() > num_perturb_per_step_) ?
                                          macros.size() : num_perturb_per_step_;
      // create SA core
      SACoreSoftMacro* sa = new SACoreSoftMacro(outline_width, outline_height, macros,
                                outline_weight_ / weight_sum, 
                                wirelength_weight_ / weight_sum,
                                guidance_weight_ / weight_sum,
                                fence_weight_ / weight_sum,
                                boundary_weight_ / weight_sum,
                                notch_weight_ / weight_sum,
                                notch_h_th_, notch_v_th_,
                                pos_swap_prob_ / action_sum,
                                neg_swap_prob_ / action_sum,
                                double_swap_prob_ / action_sum,
                                exchange_swap_prob_ / action_sum,
                                resize_prob_ / action_sum,
                                init_prob_, max_num_step_,
                                num_perturb_per_step,
                                k_, c_, random_seed_);
      sa->SetNets(nets);
      sa->SetFences(fences);
      sa->SetGuides(guides);
      sa_vector.push_back(sa);
    }
    // multi threads 
    std::vector<std::thread> threads;
    for (auto& sa : sa_vector)
      threads.push_back(std::thread(RunSA<SACoreSoftMacro>, sa));
    for (auto& th : threads)
      th.join();
    for (auto& sa : sa_vector) 
      if (sa->IsValid())
        candidates.push_back(sa);
      else
        delete sa;
    sa_vector.clear();
    remaining_runs -= run_thread;
  }
   
  // get

}



// Merge nets to reduce runtime
void HierRTLMP::MergeNets(std::vector<BundledNet>& nets)
{
  std::vector<int> net_class(nets.size(), -1);
  for (int i = 0; i < nets.size(); i++) {
    if (net_class[i] == -1) {
      for (int j = i + 1; j < nets.size(); j++) 
        if (nets[i] == nets[j])
          net_class[j] = i;
    }
  }

  for (int i = 0; i < net_class.size(); i++) 
    if (net_class[i] > 0)
      nets[net_class[i]].weight += nets[i].weight;

  std::vector<BundledNet> merged_nets;
  for (int i = 0; i < net_class.size(); i++)
    if (net_class[i] == -1)
      merged_nets.push_back(nets[i]);
  nets.clear();
  nets = merged_nets;
}


// route buses within the parent cluster
void HierRTLMP::CallPathSynthesis(Cluster* parent)
{
  if (path_syn_flag_ == false)
    return;
}

// Determine the orientation and position of each hard macro
// in each HardMacroCluster
void HierRTLMP::PlaceHardMacros(Cluster* parent)
{
  if (parent->GetClusterType() != HardMacroCluster)
    return;
}


}  // namespace mpl
