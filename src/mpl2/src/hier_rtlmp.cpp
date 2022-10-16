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
#include "bus_synthesis.h"

namespace mpl {

using std::string;

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

void HierRTLMP::SetAreaWeight(float weight)
{
  area_weight_ = weight;
}

void HierRTLMP::SetOutlineWeight(float weight)
{
  outline_weight_ = weight;
}

void HierRTLMP::SetWirelengthWeight(float weight)
{
  wirelength_weight_ = weight;
}

void HierRTLMP::SetGuidanceWeight(float weight)
{
  guidance_weight_ = weight;
}

void HierRTLMP::SetFenceWeight(float weight)
{
  fence_weight_ = weight;
}

void HierRTLMP::SetBoundaryWeight(float weight)
{
  boundary_weight_ = weight;
}

void HierRTLMP::SetNotchWeight(float weight)
{
  notch_weight_ = weight;
}


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

void HierRTLMP::SetPinAccessThreshold(float pin_access_th)
{
  pin_access_th_ = pin_access_th;
  pin_access_th_orig_ = pin_access_th;
}

void HierRTLMP::SetTargetUtil(float target_util)
{
  target_util_ = target_util;
}


void HierRTLMP::SetTargetDeadSpace(float target_dead_space)
{
  target_dead_space_ = target_dead_space;
}

void HierRTLMP::SetMinAR(float min_ar)
{
  min_ar_ = min_ar;
}

void HierRTLMP::SetSnapLayer(int snap_layer)
{
  snap_layer_ = snap_layer;
}
void HierRTLMP::SetReportDirectory(const char* report_directory)
{
  report_directory_ = report_directory;
}


///////////////////////////////////////////////////////////////
// Top Level Interface function
void HierRTLMP::HierRTLMacroPlacer() 
{
  // user just need to specify the lowest level size of cluster
  max_num_macro_base_ = max_num_macro_base_ * std::pow(coarsening_ratio_, max_num_level_ - 1);
  min_num_macro_base_ = min_num_macro_base_ * std::pow(coarsening_ratio_, max_num_level_ - 1);
  max_num_inst_base_  = max_num_inst_base_  * std::pow(coarsening_ratio_, max_num_level_ - 1);
  min_num_inst_base_  = min_num_inst_base_  * std::pow(coarsening_ratio_, max_num_level_ - 1);
 
  // Get the database information
  block_ = db_->getChip()->getBlock();
  dbu_ = db_->getTech()->getDbUnitsPerMicron();
  pitch_x_  = Dbu2Micro(static_cast<float>(
        db_->getTech()->findRoutingLayer(snap_layer_)->getPitchX()), dbu_);
  pitch_y_  = Dbu2Micro(static_cast<float>(
        db_->getTech()->findRoutingLayer(snap_layer_)->getPitchY()), dbu_);

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

  // create data flow information
  CalDataFlow();

  std::cout << "Finish Calculate Data Flow" << std::endl;


  // Create physical hierarchy tree in a post-order DFS manner
  MultiLevelCluster(root_cluster_);  // Recursive call for creating the tree 
  PrintPhysicalHierarchyTree(root_cluster_, 0);

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

  //PrintClusters();

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
  // Shape Engine Starts .................................................
  // Determine the shape and area for each cluster in a bottom-up manner
  // We first determine the macro tilings within each cluster
  // Then when we place clusters in each level, we can dynamic adjust the utilization
  // of std cells
  // Step 1 : Determine the size of root cluster
  SoftMacro* root_soft_macro = new SoftMacro(root_cluster_);
  const float root_lx = std::max(Dbu2Micro(block_->getCoreArea().xMin(), dbu_), global_fence_lx_);
  const float root_ly = std::max(Dbu2Micro(block_->getCoreArea().yMin(), dbu_), global_fence_ly_);
  const float root_ux = std::min(Dbu2Micro(block_->getCoreArea().xMax(), dbu_), global_fence_ux_);
  const float root_uy = std::min(Dbu2Micro(block_->getCoreArea().yMax(), dbu_), global_fence_uy_);
  const float root_area = (root_ux - root_lx) * (root_uy - root_ly);
  const float root_width = root_ux - root_lx;
  const std::vector<std::pair<float, float> > root_width_list = 
                { std::pair<float, float>(root_width, root_width) };
  root_soft_macro->SetShapes(root_width_list, root_area);
  root_soft_macro->SetWidth(root_width); // This will set height automatically
  root_soft_macro->SetX(root_lx);
  root_soft_macro->SetY(root_ly);
  root_cluster_->SetSoftMacro(root_soft_macro);
  
  // Step 2:  Determine the macro tilings within each cluster in a bottom-up manner
  // (Post-Order DFS manner)
  CalClusterMacroTilings(root_cluster_);
  // Cluster Placement Engine Starts ...........................................
  // create pin blockage for IO pins
  CreatePinBlockage();
  // The cluster placement is done in a top-down manner
  // (Preorder DFS)
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

  std::cout << "number of updated macros : " << num_updated_macros_ << std::endl;
  std::cout << "number of macros in HardMacroCluster : " << num_hard_macros_cluster_ << std::endl;
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

  // for debug
  if (1) {
    std::cout << "[Debug][HierRTLMP::SetClusterMetric] ";
    std::cout << cluster->GetName() << "  ";
    std::cout << "num_macro : " << metric.GetNumMacro() << "  ";
    std::cout << "num_std_cell : " << metric.GetNumStdCell() << "  ";
    std::cout << std::endl;
  }

  // end debug


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
  // convert from micro to dbu
  floorplan_lx_ = Micro2Dbu(floorplan_lx_, dbu_);
  floorplan_ly_ = Micro2Dbu(floorplan_ly_, dbu_);
  floorplan_ux_ = Micro2Dbu(floorplan_ux_, dbu_);
  floorplan_uy_ = Micro2Dbu(floorplan_uy_, dbu_); 
  // Get the floorplan information
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
  for (int i = 0; i < 4; i++) { // four boundaries (Left, Top, Right and Bottom in order)
    for (int j = 0; j < num_bundled_IOs_; j++) {
      std::string cluster_name = prefix_vec[i] + std::to_string(j);
      Cluster* cluster = new Cluster(cluster_id_, cluster_name);
      root_cluster_->AddChild(cluster);
      cluster->SetParent(root_cluster_);
      cluster_io_map[cluster_id_] = false;
      cluster_map_[cluster_id_++] = cluster;
      int x = 0.0;
      int y = 0.0;
      int width = 0;
      int height = 0;
      if (i == 0) {
        x = floorplan_lx_;
        y = floorplan_ly_ + y_base * j;
        height = y_base;
      } else if (i == 1) {
        x = floorplan_lx_ + x_base * j;
        y = floorplan_uy_;
        width = x_base;
      } else if (i == 2) {
        x = floorplan_ux_;
        y = floorplan_uy_ - y_base * (j + 1);
        height = y_base;
      } else {
        x = floorplan_ux_ - x_base * (j + 1);
        y = floorplan_ly_;
        width = x_base;
      }
      // set the cluster to a IO cluster
      cluster->SetIOClusterFlag(std::pair<float, float>(
                Dbu2Micro(x, dbu_), Dbu2Micro(y, dbu_)), 
                Dbu2Micro(width, dbu_), Dbu2Micro(height, dbu_));
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
 
    //std::cout << "*****************************************************" << std::endl;
    //std::cout << "lx :  " << lx << "    " << floorplan_lx_ << "   "
    //          << "ly :  " << ly << "    " << floorplan_ly_ << "   "
    //          << "ux :  " << ux << "    " << floorplan_ux_ << "   "
    //          << "uy :  " << uy << "    " << floorplan_uy_ << "   "
    //          << std::endl;
    
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
      cluster_id = cluster_id_base + std::floor(((ly + uy) / 2.0 - floorplan_ly_) / y_base);
    } else if (uy == floorplan_uy_) {
      // The IO is on the top boundary
      cluster_id = cluster_id_base + num_bundled_IOs_ + 
                   std::floor(((lx + ux) / 2.0 - floorplan_lx_) / x_base);
    } else if (ux == floorplan_ux_) {
      // The IO is on the right boundary
      cluster_id = cluster_id_base + num_bundled_IOs_ * 2 + 
                   std::floor((floorplan_uy_ - (ly + uy) / 2.0) / y_base);
    } else if (ly == floorplan_ly_) {
      // The IO is on the bottom boundary
      cluster_id = cluster_id_base + num_bundled_IOs_ * 3 + 
                   std::floor((floorplan_ux_ - (lx + ux) / 2.0) / x_base);
    }
    
    std::cout << "cluster_name :  " << cluster_map_[cluster_id]->GetName()
              << "\n\n";

    // Check if the IO pins / Pads exist
    if (cluster_id == -1)
      logger_->error(
        MPL, 2024, "Floorplan has not been initialized? Pin location error.");
    else 
      odb::dbIntProperty::create(term, "cluster_id", cluster_id);
    cluster_io_map[cluster_id] = true;
  }
  // convert from dbu to micro
  floorplan_lx_ = Dbu2Micro(floorplan_lx_, dbu_);
  floorplan_ly_ = Dbu2Micro(floorplan_ly_, dbu_);
  floorplan_ux_ = Dbu2Micro(floorplan_ux_, dbu_);
  floorplan_uy_ = Dbu2Micro(floorplan_uy_, dbu_);
  
  for (auto& [cluster_id, flag] : cluster_io_map) {
    if (flag == false) {
        std::cout  << "remove cluster : " << cluster_map_[cluster_id]->GetName() << std::endl;
      cluster_map_[cluster_id]->GetParent()->RemoveChild(cluster_map_[cluster_id]);
      delete cluster_map_[cluster_id];
      cluster_map_.erase(cluster_id);
      std::cout << "cluster_id : " << cluster_id << std::endl;
    }
  }

}


// Create physical hierarchy tree in a post-order DFS manner
// Recursive call for creating the physical hierarchy tree
void HierRTLMP::MultiLevelCluster(Cluster* parent) 
{
  if (level_ >= max_num_level_) // limited by the user-specified parameter
    return;  
  level_++;
  // for debug
  if (1) {
    std::cout << "[Debug][HierRTLMP::MultiLevelCluster]   ";
    std::cout << parent->GetName() << "  " << level_ << "   ";
    std::cout << "num_macro : " << parent->GetNumMacro() << "   ";
    std::cout << "num_std_cell :  " << parent->GetNumStdCell() << "  ";
    std::cout << std::endl;
  }
  // end debug


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
  //std::cout << "[Debug][HierRTLMP::SetInstProperty]  ";
  //std::cout << "cluster_name : " << cluster->GetName() << std::endl;
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
// In any case, we will consider std cells in the logical modules
// If include_macro = true, we will consider the macros also.
// Otherwise, not.
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
  std::cout << "[Debug][HierRTLMP::BreakCluster] cluster_name : " << parent->GetName() << std::endl;
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

  // Recursively break down large clusters with logical modules
  // For large flat cluster, we will break it in the UpdateSubTree function
  for (auto& child : parent->GetChildren()) 
    if (child->GetDbModules().size() > 0) 
      if (child->GetNumStdCell() > max_num_inst_ ||
          child->GetNumMacro()   > max_num_macro_)
        BreakCluster(child);
 
  // Merge small clusters
  std::vector<Cluster*> candidate_clusters;
  for (auto& cluster : parent->GetChildren()) 
    if (cluster->GetIOClusterFlag() == false     && 
        cluster->GetNumStdCell() < min_num_inst_ &&
        cluster->GetNumMacro()   < min_num_macro_)
      candidate_clusters.push_back(cluster);
  MergeClusters(candidate_clusters);

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
void HierRTLMP::MergeClusters(std::vector<Cluster*>& candidate_clusters) 
{
  // for debug
  int merge_iter = 0;
  std::cout << "\n\n";
  std::cout << "[Debug][HierRTLMP::MergeCluster]  ";
  std::cout << "Iter :  " << merge_iter++ << std::endl;
  for (auto& cluster : candidate_clusters) {
    std::cout << "cluster  :  " <<  cluster->GetName() << "   ";
    std::cout << "num_std_cell  :  " << cluster->GetNumStdCell() << "  ";
    std::cout << "num_macro : " << cluster->GetNumMacro() << std::endl;
  }
  std::cout << std::endl;
  // end debug

  int num_candidate_clusters = candidate_clusters.size();
  while (true) {
    CalculateConnection(); // update the connections between clusters
    // for debug
    // PrintConnection();
    // end debug

    std::vector<int> cluster_class(num_candidate_clusters, -1); // merge flag
    std::vector<int> candidate_clusters_id; // store cluster id
    for (auto& cluster : candidate_clusters)
      candidate_clusters_id.push_back(cluster->GetId());
    // Firstly we perform Type 1 merge
    for (int i = 0; i < num_candidate_clusters; i++) {
      int cluster_id = candidate_clusters[i]->GetCloseCluster(
                       candidate_clusters_id, signature_net_threshold_);
      // for debug
      //std::cout << "cluster_id : " << cluster_id << std::endl;
      //for (auto& [cluster_id, cluster] : cluster_map_)
      //  std::cout << cluster->GetName() << "  " << cluster_id << std::endl;
      std::cout << "candidate_cluster : " << candidate_clusters[i]->GetName() << "   "
                << " - " << (cluster_id != -1 ? cluster_map_[cluster_id]->GetName() : "   ") << std::endl;
      // end debug
      if (cluster_id != -1 && cluster_map_[cluster_id]->GetIOClusterFlag() == false) {
        Cluster*& cluster = cluster_map_[cluster_id];
        //for debug
        /*
        std::cout << "\n\n[Debug][HierRTLMP::MergeCluster]  ";
        std::cout << "merge " << cluster->GetName() << "  ";
        std::cout << "\t\t" + candidate_clusters[i]->GetName() << "  \n";
        */
        //end debug
        bool delete_flag = false;
        if (cluster->MergeCluster(*candidate_clusters[i], delete_flag) == true) {
          if (delete_flag == true) {
            cluster_map_.erase(candidate_clusters[i]->GetId());
            delete candidate_clusters[i];
          }
          SetInstProperty(cluster);
          SetClusterMetric(cluster);
          cluster_class[i] = cluster->GetId();
        } 
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
                      *candidate_clusters[j], signature_net_threshold_);
          if (flag == true) {
            cluster_class[j] = i;
            bool delete_flag = false;
            if (candidate_clusters[i]->MergeCluster(
                    *candidate_clusters[j], delete_flag) == true) {
              if (delete_flag == true) {
                cluster_map_.erase(candidate_clusters[j]->GetId());
                delete candidate_clusters[j];
              }
              SetInstProperty(candidate_clusters[i]);
              SetClusterMetric(candidate_clusters[i]);
            }
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

    // for debug
    std::cout << "\n\n";
    std::cout << "[Debug][HierRTLMP::MergeCluster]  ";
    std::cout << "Iter :  " << merge_iter++ << std::endl;
    for (auto& cluster : candidate_clusters)
      std::cout << "cluster  :  " <<  cluster->GetName() << std::endl;
    std::cout << std::endl;
    // end debug
    // merge small clusters
    if (candidate_clusters.size() == 0)
      break;
    
    /*
    int remain_num_std_cells = 0;
    int remain_num_macros = 0;
    for (auto& cluster : candidate_clusters) {
      remain_num_std_cells += cluster->GetNumStdCell();
      remain_num_macros += cluster->GetNumMacro();
    }

    if (remain_num_std_cells < min_num_inst_ && remain_num_macros < min_num_macro_) {
      for (int j = 1; j < candidate_clusters.size(); j++) {    
        bool delete_flag = false;
        if (candidate_clusters[0]->MergeCluster(
            *candidate_clusters[j], delete_flag) == true) {
          if (delete_flag == true) {
            cluster_map_.erase(candidate_clusters[j]->GetId());
            delete candidate_clusters[j];
          }
          SetInstProperty(candidate_clusters[0]);
          SetClusterMetric(candidate_clusters[0]);
        }
      }
      break;
    } 
    */
  }
  // for debug
  std::cout << "\n\n";
  std::cout << "[Debug][HierRTLMP::MergeCluster]  Finish MergeCluster\n";
  // end debug
}


// Calculate Connections between clusters
void HierRTLMP::CalculateConnection() {
  // Initialize the connections of all clusters
  for (auto & [cluster_id, cluster] : cluster_map_)
    cluster->InitConnection();

  // for debug
  // PrintClusters();
  // end debug

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

// Create Dataflow Information
// model each std cell instance, IO pin and macro pin as vertices
void HierRTLMP::CalDataFlow()
{
  if (max_num_ff_dist_ <= 0)
    return;
  // create vertex id property for std cell, IO pin and macro pin 
  std::map<int, odb::dbBTerm*> io_pin_vertex;
  std::map<int, odb::dbInst*>  std_cell_vertex;
  std::map<int, odb::dbITerm*> macro_pin_vertex;

  std::vector<bool> stop_flag_vec;
  // assign vertex_id property of each Bterm
  for (odb::dbBTerm* term : block_->getBTerms()) {
    odb::dbIntProperty::create(term, "vertex_id", stop_flag_vec.size());
    io_pin_vertex[stop_flag_vec.size()] = term;
    stop_flag_vec.push_back(true);
  }
  // assign vertex_id property of each instance
  for (auto inst : block_->getInsts()) {
    const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
    odb::dbMaster* master = inst->getMaster();
    // check if the instance is a Pad, Cover or empty block (such as marker)
    // We ignore nets connecting Pads, Covers, or markers
    if (master->isPad() || master->isCover() || master->isBlock())
      continue;
    odb::dbIntProperty::create(inst, "vertex_id", stop_flag_vec.size());
    std_cell_vertex[stop_flag_vec.size()] = inst;
    if (liberty_cell->hasSequentials())
      stop_flag_vec.push_back(true);
    else
      stop_flag_vec.push_back(false);
  }
  // assign vertex_id property of each macro pin
  for (auto& [macro, hard_macro] : hard_macro_map_) {
    for (odb::dbITerm* pin : macro->getITerms()) {
      if (pin->getSigType() != odb::dbSigType::SIGNAL)
        continue;
      odb::dbIntProperty::create(pin, "vertex_id", stop_flag_vec.size());
      macro_pin_vertex[stop_flag_vec.size()] = pin;
      stop_flag_vec.push_back(true);
    }
  }

  std::cout << "Finish Creating Vertices" << std::endl;

  // create hypergraphs
  std::vector<std::vector<int> > vertices(stop_flag_vec.size());
  std::vector<std::vector<int> > backward_vertices(stop_flag_vec.size());
  std::vector<std::vector<int> > hyperedges; // dircted hypergraph
  // traverse the netlist
  for (odb::dbNet* net : block_->getNets()) {
    // ignore all the power net
    if (net->getSigType().isSupply())
      continue;
    int driver_id = -1; // driver vertex id
    std::set<int> loads_id; // loas vertex id
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
      int vertex_id = -1;
      if (master->isBlock())
        vertex_id = odb::dbIntProperty::find(iterm, "vertex_id")->getValue();
      else
        vertex_id = odb::dbIntProperty::find(inst, "vertex_id")->getValue();
      if (iterm->getIoType() == odb::dbIoType::OUTPUT)
        driver_id = vertex_id;
      else
        loads_id.insert(vertex_id);
    }
    if (pad_flag == true)
      continue; // the nets with Pads should be ignored  
    // check the connected IO pins
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const int vertex_id = odb::dbIntProperty::find(bterm, "vertex_id")->getValue();
      if (bterm->getIoType() == odb::dbIoType::INPUT)
        driver_id = vertex_id;
      else
        loads_id.insert(vertex_id);
    }    
    if (driver_id < 0 || loads_id.size() < 1 || loads_id.size() > large_net_threshold_)
      continue;
    std::vector<int> hyperedge { driver_id };
    for (auto& load : loads_id)
      if (load != driver_id)
        hyperedge.push_back(load);
    //for (auto& vertex_id : hyperedge)
    //  vertices[vertex_id].push_back(hyperedges.size());
    vertices[driver_id].push_back(hyperedges.size());
    for (int i = 1; i < hyperedge.size(); i++)
      backward_vertices[hyperedge[i]].push_back(hyperedges.size());
    hyperedges.push_back(hyperedge);
  } // end net traversal 
   
  std::cout << "Finish hypergraph creation" << std::endl;
 
  
  // traverse hypergraph to build dataflow
  for (auto [src, src_pin] : io_pin_vertex) {
    int idx = 0;
    std::vector<bool> visited(vertices.size(), false);
    std::vector<std::set<odb::dbInst*> > insts(max_num_ff_dist_);
    DataFlowDFSIOPin(src, idx, insts, io_pin_vertex, std_cell_vertex, macro_pin_vertex, 
                     stop_flag_vec, visited,  vertices, hyperedges, false);
    DataFlowDFSIOPin(src, idx, insts, io_pin_vertex, std_cell_vertex, macro_pin_vertex, 
                     stop_flag_vec, visited,  backward_vertices, hyperedges, true);
    io_ffs_conn_map_.push_back(
      std::pair<odb::dbBTerm*, std::vector<std::set<odb::dbInst*> > >(src_pin, insts));
  }

  for (auto [src, src_pin] : macro_pin_vertex) {
    int idx = 0;
    std::vector<bool> visited(vertices.size(), false);
    std::vector<std::set<odb::dbInst*> > std_cells(max_num_ff_dist_);
    std::vector<std::set<odb::dbInst*> > macros(max_num_ff_dist_);
    DataFlowDFSMacroPin(src, idx, std_cells, macros, 
                     io_pin_vertex, std_cell_vertex, macro_pin_vertex, 
                     stop_flag_vec, visited, vertices, hyperedges, false);
    DataFlowDFSMacroPin(src, idx, std_cells, macros, 
                     io_pin_vertex, std_cell_vertex, macro_pin_vertex, 
                     stop_flag_vec, visited, backward_vertices, hyperedges, true);
    macro_ffs_conn_map_.push_back(
      std::pair<odb::dbITerm*, std::vector<std::set<odb::dbInst*> > >(src_pin, std_cells));
    macro_macro_conn_map_.push_back(
      std::pair<odb::dbITerm*, std::vector<std::set<odb::dbInst*> > >(src_pin, macros));
  }
}

void HierRTLMP::DataFlowDFSIOPin(int parent, int idx, 
           std::vector<std::set<odb::dbInst*> >& insts,
           std::map<int, odb::dbBTerm*>& io_pin_vertex,
           std::map<int, odb::dbInst*>& std_cell_vertex,
           std::map<int, odb::dbITerm*>& macro_pin_vertex,
           std::vector<bool>& stop_flag_vec,
           std::vector<bool>& visited,
           std::vector<std::vector<int> >& vertices,
           std::vector<std::vector<int> >& hyperedges,
           bool backward_flag) 
{
  visited[parent] = true;
  if (stop_flag_vec[parent] == true) {
    if (parent < io_pin_vertex.size()) {
      ; // currently we do not consider IO pin to IO pin connnection
    } else if (parent < io_pin_vertex.size() + std_cell_vertex.size()) {
      insts[idx].insert(std_cell_vertex[parent]);
    } else {
      insts[idx].insert(macro_pin_vertex[parent]->getInst());
    }
    idx++;  
  }

  if (idx >= max_num_ff_dist_)
    return;

  if (backward_flag == false) {
    for (auto& hyperedge : vertices[parent]) {
      for (auto& vertex : hyperedges[hyperedge]) {
        // we do not consider pin to pin
        if (visited[vertex] == true || vertex < io_pin_vertex.size())
          continue;
        DataFlowDFSIOPin(vertex, idx, insts, io_pin_vertex, std_cell_vertex, 
                macro_pin_vertex, stop_flag_vec, visited, vertices, 
                hyperedges, backward_flag);
      }
    } // finish hyperedges
  } else {
    for (auto& hyperedge : vertices[parent]) {
      const int vertex = hyperedges[hyperedge][0]; // driver vertex
      // we do not consider pin to pin
      if (visited[vertex] == true || vertex < io_pin_vertex.size())
        continue;
      DataFlowDFSIOPin(vertex, idx, insts, io_pin_vertex, std_cell_vertex, 
         macro_pin_vertex, stop_flag_vec, visited, vertices, 
         hyperedges, backward_flag);
    } // finish hyperedges
  } // finish current vertex
}


void HierRTLMP::DataFlowDFSMacroPin(int parent, int idx, 
           std::vector<std::set<odb::dbInst*> >& std_cells,
           std::vector<std::set<odb::dbInst*> >& macros,
           std::map<int, odb::dbBTerm*>& io_pin_vertex,
           std::map<int, odb::dbInst*>& std_cell_vertex,
           std::map<int, odb::dbITerm*>& macro_pin_vertex,
           std::vector<bool>& stop_flag_vec,
           std::vector<bool>& visited, 
           std::vector<std::vector<int> >& vertices,
           std::vector<std::vector<int> >& hyperedges,
           bool backward_flag) 
{
  visited[parent] = true;
  if (stop_flag_vec[parent] == true) {
    if (parent < io_pin_vertex.size()) {
      ; // the connection between IO and macro pins have been considers
    } else if (parent < io_pin_vertex.size() + std_cell_vertex.size()) {
      std_cells[idx].insert(std_cell_vertex[parent]);
    } else {
      macros[idx].insert(macro_pin_vertex[parent]->getInst());
    }
    idx++;  
  }

  if (idx >= max_num_ff_dist_)
    return;

  if (backward_flag == false) {
    for (auto& hyperedge : vertices[parent]) {
      for (auto& vertex : hyperedges[hyperedge]) {
        // we do not consider pin to pin
        if (visited[vertex] == true || vertex < io_pin_vertex.size())
          continue;
        DataFlowDFSMacroPin(vertex, idx, std_cells, macros,
           io_pin_vertex, std_cell_vertex, macro_pin_vertex, 
           stop_flag_vec, visited, vertices, hyperedges, backward_flag);
      }
    } // finish hyperedges
  } else {
    for (auto& hyperedge : vertices[parent]) {
      const int vertex = hyperedges[hyperedge][0];
      // we do not consider pin to pin
      if (visited[vertex] == true || vertex < io_pin_vertex.size())
        continue;
      DataFlowDFSMacroPin(vertex, idx, std_cells, macros,
          io_pin_vertex, std_cell_vertex, macro_pin_vertex, 
          stop_flag_vec, visited, vertices, hyperedges, backward_flag);
    } // finish hyperedges
  }
}

void HierRTLMP::UpdateDataFlow()
{
  // bterm, macros or ffs
  for (auto bterm_pair : io_ffs_conn_map_) {
    const int driver_id = odb::dbIntProperty::find(bterm_pair.first, 
                               "cluster_id")->getValue();
    for (int i = 0; i < max_num_ff_dist_; i++) {
      const float weight = 1.0 /  std::pow(dataflow_factor_, i);
      std::set<int> sink_clusters;
      for (auto& inst : bterm_pair.second[i]) {
        const int cluster_id = odb::dbIntProperty::find(inst, "cluster_id")->getValue();
        sink_clusters.insert(cluster_id);
      }
      for (auto& sink : sink_clusters) {
        cluster_map_[driver_id]->AddConnection(sink, weight);
        cluster_map_[sink]->AddConnection(driver_id, weight);  
      } // add weight
    } // number of ffs
  }
  
  // macros to ffs
  for (auto iterm_pair : macro_ffs_conn_map_) {
    const int driver_id = odb::dbIntProperty::find(iterm_pair.first->getInst(), 
                               "cluster_id")->getValue();
    for (int i = 0; i < max_num_ff_dist_; i++) {
      const float weight = 1.0 / std::pow(dataflow_factor_, i);
      std::set<int> sink_clusters;
      for (auto& inst : iterm_pair.second[i]) {
        const int cluster_id = odb::dbIntProperty::find(inst, "cluster_id")->getValue();
        sink_clusters.insert(cluster_id);
      }
      for (auto& sink : sink_clusters) {
        cluster_map_[driver_id]->AddConnection(sink, weight);
        cluster_map_[sink]->AddConnection(driver_id, weight);  
      } // add weight
    } // number of ffs
  }

  // macros to macros
  for (auto iterm_pair : macro_macro_conn_map_) {
    const int driver_id = odb::dbIntProperty::find(iterm_pair.first->getInst(), 
                               "cluster_id")->getValue();
    for (int i = 0; i < max_num_ff_dist_; i++) {
      const float weight = std::pow(dataflow_factor_, i);
      std::set<int> sink_clusters;
      for (auto& inst : iterm_pair.second[i]) {
        const int cluster_id = odb::dbIntProperty::find(inst, "cluster_id")->getValue();
        sink_clusters.insert(cluster_id);
      }
      for (auto& sink : sink_clusters) {
        cluster_map_[driver_id]->AddConnection(sink, weight);
      } // add weight
    } // number of ffs
  }
}


// Print Connnection For all the clusters
void HierRTLMP::PrintConnection() 
{
  std::string line = "";
  line += "NUM_CLUSTERS  :   " + std::to_string(cluster_map_.size()) + "\n";
  for (auto & [cluster_id, cluster] : cluster_map_) {
    const std::map<int, float> connections = cluster->GetConnection();
    if (connections.size() == 0)
      continue;
    line += "cluster " + cluster->GetName() + " : \n";
    for (auto [target, num_nets] : connections) {
      line += "\t\t" + cluster_map_[target]->GetName() + "  ";
      line += std::to_string(static_cast<int>(num_nets)) + "\n";
    }
  }
  logger_->info(MPL, 2026, line);
}

// Print All the clusters and their statics
void HierRTLMP::PrintClusters()
{
  std::string line = "";
  line += "NUM_CLUSTERS  :   " + std::to_string(cluster_map_.size()) + "\n";
  for (auto & [cluster_id, cluster] : cluster_map_) {
    line += cluster->GetName() + "  ";
    line += std::to_string(cluster->GetId()) + "\n";    
  }
  logger_->info(MPL, 2027, line);
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
    } else {
      child->SetClusterType(StdCellCluster);
    }
  }

  // after this step, the std cells and macros for leaf cluster has been
  // seperated. We need to update the metrics of the design if necessary
  // for each leaf clusters with macros, first group macros based on 
  // connection signatures and macro sizes
  for (auto& cluster : leaf_clusters) {
    // for debug
    if (0) {
      std::cout << "\n\n[Debug][HierRTLMP::LeafClusterStdCellHardMacroSep]  ";
      std::cout << cluster->GetName() << std::endl;
    }
    // end debug
    // based on the type of current cluster,
    Cluster* parent_cluster = cluster;  // std cell dominated cluster
    // add a subtree if the cluster is macro dominated cluster
    // based macro_dominated_cluster_threshold_ (defualt = 0.01)
    if (cluster->GetNumStdCell() * macro_dominated_cluster_threshold_
        < cluster->GetNumMacro())
      parent_cluster = cluster->GetParent(); // replacement

    // Map macros into the HardMacro objects
    // and get all the HardMacros
    MapMacroInCluster2HardMacro(cluster);
    std::vector<HardMacro*> hard_macros = cluster->GetHardMacros();
    // for debug
    if (0) {
      std::cout << "Finish mapping hard macros" << std::endl;   
    }
    // end debug

    
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
    
    // for debug
    if (0) {
      std::cout << "Finish mapping macros to clusters" << std::endl;   
    }
    // end debug

    // classify macros based on size
    std::vector<int> macro_size_class(hard_macros.size(), -1); 
    for (int i = 0; i < hard_macros.size(); i++) {
      if (macro_size_class[i] == -1) {
        for (int j = i + 1; j < hard_macros.size(); j++) {
          if ((macro_size_class[j] == -1) && ((*hard_macros[i]) == (*hard_macros[j])))
            macro_size_class[j] = i;
        }
      }
    }
    //std::vector<int> macro_size_class(hard_macros.size(), 1); 
    //std::vector<int> macro_size_class(hard_macros.size(), 1); 
    for (auto& id : macro_size_class)
      std::cout << id << "   ";
    std::cout << std::endl;
   
    for (int i = 0; i < hard_macros.size(); i++)
      macro_size_class[i] = (macro_size_class[i] == -1) ? i : macro_size_class[i];

    for (auto& id : macro_size_class)
      std::cout << id << "   ";
    std::cout << std::endl;


    // for debug
    if (0) {
      std::cout << "Finish classifying based on size" << std::endl;   
    }
    // end debug


    // classify macros based on connection signature
    CalculateConnection();
    std::vector<int> macro_signature_class(hard_macros.size(), -1);
    for (int i = 0; i < hard_macros.size(); i++) {
      if (macro_signature_class[i] == -1) { 
        macro_signature_class[i] = i;
        for (int j = i + 1; j < hard_macros.size(); j++) {
          if (macro_signature_class[j] != -1)
            continue;
          bool flag = macro_clusters[i]->IsSameConnSignature(
                          *macro_clusters[j], signature_net_threshold_);
          if (flag == true) 
            macro_signature_class[j] = i;
        }
      }
    }
    
    for (auto& id : macro_signature_class)
      std::cout << id << "   ";
    std::cout << std::endl;

    // print the connnection signature
    for (auto& cluster : macro_clusters) {
      std::cout << "**************************************************" << std::endl;
      std::cout << "Macro Signature : " << cluster->GetName() << std::endl;;
      for (auto& [cluster_id, weight] : cluster->GetConnection())
        std::cout << cluster_map_[cluster_id]->GetName() << "  " << weight << std::endl;
      std::cout << std::endl;
    }

    // for debug
    if (1) {
      std::cout << "Finish  Macro Classification" << std::endl;   
    }
    // end debug

    // macros with the same size and the same connection signature
    // belong to the same class
    std::vector<int> macro_class(hard_macros.size(), -1);
    for (int i = 0; i < hard_macros.size(); i++) {
      if (macro_class[i] == -1) {
        macro_class[i] = i;
        for (int j = i + 1; j < hard_macros.size(); j++) { 
          if (macro_class[j] == -1 &&
              macro_size_class[i] == macro_size_class[j] &&
              macro_signature_class[i] == macro_signature_class[j]) {
            macro_class[j] = i;
            std::cout << "merge " << macro_clusters[i]->GetName() << "   "
                      << macro_clusters[j]->GetName() << std::endl;
            bool delete_flag = false;
            macro_clusters[i]->MergeCluster(*macro_clusters[j], delete_flag);
            if (delete_flag == true) {
              // remove the merged macro cluster
              cluster_map_.erase(macro_clusters[j]->GetId());
              delete macro_clusters[j];
            }
          }
        }
      } 
    }
    
    for (auto& id : macro_class)
      std::cout << id << "   ";
    std::cout << std::endl;


    // for debug
    if (0) {
      std::cout << "Finish Macro Grouping" << std::endl;   
    }
    // end debug

    // clear the hard macros in current leaf cluster
    cluster->ClearHardMacros(); 
    // Restore the structure of physical hierarchical tree
    // Thus the order of leaf clusters will not change the final
    // macro grouping results (This is very important !!!
    // Don't touch the next line SetInstProperty command!!!)
    SetInstProperty(cluster);

    std::vector<int> virtual_conn_clusters;
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
      virtual_conn_clusters.push_back(std_cell_cluster->GetId());
    } else {
      // If we need add a replacement
      cluster->ClearLeafMacros();
      cluster->SetClusterType(StdCellCluster);
      SetClusterMetric(cluster);
      virtual_conn_clusters.push_back(cluster->GetId());
      // In this case, we do not to modify the physical hierarchy tree
    }
    // Deal with macro clusters
    for (int i = 0; i < macro_class.size(); i++) {
      if (macro_class[i] != i)
        continue;  // this macro cluster has been merged
      macro_clusters[i]->SetClusterType(HardMacroCluster);
      SetClusterMetric(macro_clusters[i]);
      virtual_conn_clusters.push_back(cluster->GetId());
    }
    

    // add virtual connections
    for (int i = 0; i < virtual_conn_clusters.size(); i++) 
      for (int j = i + 1; j < virtual_conn_clusters.size(); j++)
        parent_cluster->AddVirtualConnection(virtual_conn_clusters[i],
        virtual_conn_clusters[j]);
    /*
    // add virtual connections
    for (int i = 0; i < 1; i++) 
      for (int j = i + 1; j < virtual_conn_clusters.size(); j++)
        parent_cluster->AddVirtualConnection(virtual_conn_clusters[i],
        virtual_conn_clusters[j]);
    */
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
  line += "  (" + std::to_string(parent->GetId()) + ")  ";
  line += "  num_macro :  " + std::to_string(parent->GetNumMacro());
  line += "  num_std_cell :  " + std::to_string(parent->GetNumStdCell());
  line += "  macro_area :  " + std::to_string(parent->GetMacroArea());
  line += "  std_cell_area : " + std::to_string(parent->GetStdCellArea());
  logger_->info(MPL, 2025, line);
  int tot_num_macro_in_children = 0;
  for (auto& cluster : parent->GetChildren())
     tot_num_macro_in_children += cluster->GetNumMacro();
  std::cout << "tot_num_macro_in_children = "
             << tot_num_macro_in_children
             << "\n\n" << std::endl;

  for (auto& cluster : parent->GetChildren())
    PrintPhysicalHierarchyTree(cluster, level + 1);
}


/////////////////////////////////////////////////////////////////////////////
// Macro Placement related functions
// Determine the macro tilings within each cluster in a bottom-up manner
// (Post-Order DFS manner)
void HierRTLMP::CalClusterMacroTilings(Cluster* parent)
{
  // base case
  if (parent->GetNumMacro() == 0)
    return;
  
  if (parent->GetClusterType() == HardMacroCluster) {
    CalHardMacroClusterShape(parent);
    return;
  }
     
  // recursively visit the children
  for (auto& cluster : parent->GetChildren()) {
    if (cluster->GetNumMacro() > 0)
      CalClusterMacroTilings(cluster);
  }

  // calculate macro tiling for parent cluster based on the macro tilings 
  // of its children
  std::vector<SoftMacro> macros;
  for (auto& cluster : parent->GetChildren()) {
    if (cluster->GetNumMacro() > 0) {
      SoftMacro macro = SoftMacro(cluster);
      macro.SetShapes(cluster->GetMacroTilings(), true);
      macros.push_back(macro);
    }
  }
  
  // if there is only one soft macro
  if (macros.size() == 1) {
    for (auto& cluster : parent->GetChildren()) {
      if (cluster->GetNumMacro() > 0) {
        parent->SetMacroTilings(cluster->GetMacroTilings()); 
        return;      
      }
    }
  }
  // otherwise call simulated annealing to determine tilings
  // set the action probabilities
  // macro tilings
  std::set<std::pair<float, float> > macro_tilings; // <width, height> 
  const float action_sum  = pos_swap_prob_ + neg_swap_prob_ 
                            + double_swap_prob_ + exchange_swap_prob_
                            + resize_prob_;
  const float pos_swap_prob      = pos_swap_prob_ / action_sum;
  const float neg_swap_prob      = neg_swap_prob_ / action_sum;
  const float double_swap_prob   = double_swap_prob_ / action_sum;
  const float exchange_swap_prob = exchange_swap_prob_ / action_sum;
  const float resize_prob        = resize_prob_ / action_sum;
  // get outline constraint
  const float outline_width = root_cluster_->GetWidth();
  const float outline_height = root_cluster_->GetHeight();
  // We vary the outline of cluster to generate differnt tilings
  std::vector<float> vary_factor_list { 1.0 };
  float vary_step = 1.0 / num_runs_;  // change the outline by at most halfly
  for (int i = 1; i < num_runs_; i++) {
    vary_factor_list.push_back(1.0 - i * vary_step);
  }
  const int num_perturb_per_step = (macros.size() > num_perturb_per_step_ / 10 ) ?
                                    macros.size() : num_perturb_per_step_ / 10;
  //const int num_perturb_per_step = macros.size() * 10;
  int run_thread = num_threads_;
  int remaining_runs = num_runs_;
  int run_id = 0;
  while (remaining_runs > 0) {
    std::vector<SACoreSoftMacro*> sa_vector;
    run_thread = (remaining_runs > num_threads_) ? num_threads_ : remaining_runs;
    for (int i = 0; i < run_thread; i++) {
      const float width = outline_width * vary_factor_list[run_id++];
      const float height = outline_height;
      SACoreSoftMacro* sa = new SACoreSoftMacro(width, height, macros,
                                                1.0, 
                                                1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, // penalty weight
                                                0.0, 0.0, // notch size
                                                pos_swap_prob, neg_swap_prob,
                                                double_swap_prob, exchange_swap_prob, 
                                                resize_prob,
                                                init_prob_, max_num_step_,
                                                num_perturb_per_step,
                                                k_, c_, random_seed_);
      sa_vector.push_back(sa);
    }
    // multi threads 
    std::vector<std::thread> threads;
    for (auto& sa : sa_vector)
      threads.push_back(std::thread(RunSA<SACoreSoftMacro>, sa));
    for (auto& th : threads)
      th.join();
    // add macro tilings
    for (auto& sa : sa_vector) {
      if (sa->IsValid())
        macro_tilings.insert(std::pair<float, float>(sa->GetWidth(), sa->GetHeight()));
      delete sa; // avoid memory leakage
    }
    sa_vector.clear();
    remaining_runs -= run_thread;
  }

  run_thread = num_threads_;
  remaining_runs = num_runs_;
  run_id = 0;
  while (remaining_runs > 0) {
    std::vector<SACoreSoftMacro*> sa_vector;
    run_thread = (remaining_runs > num_threads_) ? num_threads_ : remaining_runs;
    for (int i = 0; i < run_thread; i++) {
      const float width = outline_width;
      const float height = outline_height * vary_factor_list[run_id++];
      SACoreSoftMacro* sa = new SACoreSoftMacro(width, height, macros,
                                                1.0, 
                                                1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, // penalty weight
                                                0.0, 0.0, // notch size
                                                pos_swap_prob, neg_swap_prob,
                                                double_swap_prob, exchange_swap_prob, 
                                                resize_prob,
                                                init_prob_, max_num_step_,
                                                num_perturb_per_step,
                                                k_, c_, random_seed_);
      sa_vector.push_back(sa);
    }
    // multi threads 
    std::vector<std::thread> threads;
    for (auto& sa : sa_vector)
      threads.push_back(std::thread(RunSA<SACoreSoftMacro>, sa));
    for (auto& th : threads)
      th.join();
    // add macro tilings
    for (auto& sa : sa_vector) {
      if (sa->IsValid())
        macro_tilings.insert(std::pair<float, float>(sa->GetWidth(), sa->GetHeight()));
      delete sa; // avoid memory leakage
    }
    sa_vector.clear();
    remaining_runs -= run_thread;
  }

  std::vector<std::pair<float, float> > tilings(macro_tilings.begin(), macro_tilings.end()); 
  std::sort(tilings.begin(), tilings.end(), ComparePairProduct);
  for (auto& shape : tilings)
    std::cout << "width:  " << shape.first << "   height:  " << shape.second << std::endl;
  std::cout << std::endl;
  // we do not want very strange tilngs
  std::vector<std::pair<float, float> > new_tilings;
  for (auto& tiling : tilings) {
    if (tiling.second / tiling.first >= min_ar_ &&
        tiling.second / tiling.first <= 1.0 / min_ar_)
      new_tilings.push_back(tiling);
    std::cout << "tiling.second / tiling.first :  " 
              << tiling.second / tiling.first
              << " min_ar_ :   "
              << min_ar_ << std::endl;
  }
  tilings = new_tilings;
  // update parent
  parent->SetMacroTilings(tilings);
  if (tilings.size() == 0) {
    std::string line = "This is no valid tilings for cluster ";
    line += parent->GetName();
    logger_->error(MPL, 2031, line);      
  } else {
    std::string line = "The macro tiling for " + parent->GetName() + "  ";
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
void HierRTLMP::CalHardMacroClusterShape(Cluster* cluster) 
{
  // Check if the cluster is a HardMacroCluster
  if (cluster->GetClusterType() != HardMacroCluster)
    return;
  
  std::cout << "Enter CalHardMacroCluster : " << cluster->GetName() << std::endl;

  std::vector<HardMacro*> hard_macros = cluster->GetHardMacros();
  // macro tilings
  std::set<std::pair<float, float> > macro_tilings; // <width, height>
  // if the cluster only has one macro
  if (hard_macros.size() == 1) {
    float width = hard_macros[0]->GetWidth();
    float height = hard_macros[0]->GetHeight();
    std::cout << "width:  " << width << " height:  " << height << std::endl;
    std::cout << std::endl;
    std::vector<std::pair<float, float> > tilings;
    tilings.push_back(std::pair<float, float>(width, height));
    cluster->SetMacroTilings(tilings);
    return;
  }
  // otherwise call simulated annealing to determine tilings
  // set the action probabilities
  const float action_sum  = pos_swap_prob_ + neg_swap_prob_ 
                            + double_swap_prob_
                            + exchange_swap_prob_;
  const float pos_swap_prob      = pos_swap_prob_ / action_sum;
  const float neg_swap_prob      = neg_swap_prob_ / action_sum;
  const float double_swap_prob   = double_swap_prob_ / action_sum;
  const float exchange_swap_prob = exchange_swap_prob_ / action_sum;
  // get outline constraint
  const float outline_width = root_cluster_->GetWidth();
  const float outline_height = root_cluster_->GetHeight();
  // We vary the outline of cluster to generate differnt tilings
  std::vector<float> vary_factor_list { 1.0 };
  float vary_step = 1.0 / num_runs_;  // change the outline by at most halfly
  for (int i = 1; i < num_runs_ ; i++) {
    vary_factor_list.push_back(1.0 - i * vary_step);
  }
  // update macros
  std::vector<HardMacro> macros;
  for (auto& macro : hard_macros)
    macros.push_back(*macro);
  const int num_perturb_per_step = (macros.size() > num_perturb_per_step_ / 10) ?
                                    macros.size() : num_perturb_per_step_ / 10;
  //const int num_perturb_per_step = macros.size() * 10; // increase the num_perturb
  int run_thread = num_threads_;
  int remaining_runs = num_runs_;
  int run_id = 0;
  while (remaining_runs > 0) {
    std::vector<SACoreHardMacro*> sa_vector;
    run_thread = (remaining_runs > num_threads_) ? num_threads_ : remaining_runs;
    for (int i = 0; i < run_thread; i++) {
      const float width = outline_width * vary_factor_list[run_id++];
      const float height = outline_height;
      SACoreHardMacro* sa = new SACoreHardMacro(width, height, macros,
                                                1.0, // area_weight_ 
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
      if (sa->IsValid())
        macro_tilings.insert(std::pair<float, float>(sa->GetWidth(), sa->GetHeight()));
      delete sa; // avoid memory leakage
    }
    sa_vector.clear();
    remaining_runs -= run_thread;
  }


  run_thread = num_threads_;
  remaining_runs = num_runs_;
  run_id = 0;
  while (remaining_runs > 0) {
    std::vector<SACoreHardMacro*> sa_vector;
    run_thread = (remaining_runs > num_threads_) ? num_threads_ : remaining_runs;
    for (int i = 0; i < run_thread; i++) {
      const float width = outline_width;
      const float height = outline_height * vary_factor_list[run_id++];
      SACoreHardMacro* sa = new SACoreHardMacro(width, height, macros,
                                                1.0, // area_weight_ 
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
      if (sa->IsValid())
        macro_tilings.insert(std::pair<float, float>(sa->GetWidth(), sa->GetHeight()));
      delete sa; // avoid memory leakage
    }
    sa_vector.clear();
    remaining_runs -= run_thread;
  }


  std::vector<std::pair<float, float> > tilings(macro_tilings.begin(), macro_tilings.end());  
  std::sort(tilings.begin(), tilings.end(), ComparePairProduct);
  for (auto& shape : tilings)
    std::cout << "width:  " << shape.first << "   height:  " << shape.second << std::endl;
  std::cout << std::endl;
  // we do not want very strange tilngs
  std::vector<std::pair<float, float> > new_tilings;
  for (auto& tiling : tilings) {
    if (tiling.first * tiling.second <= tilings[0].first * tilings[0].second)
      new_tilings.push_back(tiling);
    /*
    if (tiling.second / tiling.first >= min_ar_ &&
        tiling.second / tiling.first <= 1.0 / min_ar_ &&
        tiling.first * tiling.second <= tilings[0].first * tilings[0].second)
      new_tilings.push_back(tiling);
    std::cout << "tiling.second / tiling.first :  " 
              << tiling.second / tiling.first
              << " min_ar_ :   "
              << min_ar_ << std::endl;
    */
  }
  tilings = new_tilings;
  // update parent
 
  cluster->SetMacroTilings(tilings);
  if (tilings.size() == 0) {
    std::string line = "This is no valid tilings for cluster ";
    line += cluster->GetName();
    logger_->error(MPL, 2030, line);      
  }
}


// Create pin blockage for Bundled IOs
void HierRTLMP::CreatePinBlockage()
{
  //pin_access_th_ = pin_access_th_orig_ 
  //               * ( 1  - (root_cluster_->GetStdCellArea() + root_cluster_->GetMacroArea())
  //                 / (root_cluster_->GetWidth() * root_cluster_->GetHeight()));  
  //  pin_access_th_ = pin_access_th_orig_ * root_cluster_->GetStdCellArea() / 
  //                 (root_cluster_->GetStdCellArea() + root_cluster_->GetMacroArea());
  floorplan_lx_ = root_cluster_->GetX();
  floorplan_ly_ = root_cluster_->GetY();
  floorplan_ux_ = floorplan_lx_ + root_cluster_->GetWidth();
  floorplan_uy_ = floorplan_ly_ + root_cluster_->GetHeight();
  
  CalculateConnection();
  // if the design has IO pads, we do not create pin blockage
  if (io_pad_map_.size() > 0)
    return;
  // Then we check the range of IO spans
  std::map<PinAccess, std::pair<float, float> > pin_ranges;
  pin_ranges[L] = std::pair<float, float>(floorplan_uy_, floorplan_ly_);
  pin_ranges[T] = std::pair<float, float>(floorplan_ux_, floorplan_lx_);
  pin_ranges[R] = std::pair<float, float>(floorplan_uy_, floorplan_ly_);
  pin_ranges[B] = std::pair<float, float>(floorplan_ux_, floorplan_lx_);
  int floorplan_lx = Micro2Dbu(floorplan_lx_, dbu_);
  int floorplan_ly = Micro2Dbu(floorplan_ly_, dbu_);
  int floorplan_ux = Micro2Dbu(floorplan_ux_, dbu_);
  int floorplan_uy = Micro2Dbu(floorplan_uy_, dbu_);
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


    std::cout << "Aug0815 :  " << Dbu2Micro(lx, dbu_) << "  "
              << Dbu2Micro(ly, dbu_) << "  "
              << Dbu2Micro(ux, dbu_) << "  "
              << Dbu2Micro(uy, dbu_) << "  "
              << std::endl;
              

    // modify the pin ranges
    if (lx <= floorplan_lx) {
      pin_ranges[L].first = std::min(pin_ranges[L].first,   Dbu2Micro(ly, dbu_));
      pin_ranges[L].second = std::max(pin_ranges[L].second, Dbu2Micro(uy, dbu_));
    } else if (uy >= floorplan_uy) {
      pin_ranges[T].first = std::min(pin_ranges[T].first,   Dbu2Micro(lx, dbu_));
      pin_ranges[T].second = std::max(pin_ranges[T].second, Dbu2Micro(ux, dbu_));
    } else if (ux >= floorplan_ux) {
      pin_ranges[R].first = std::min(pin_ranges[R].first,   Dbu2Micro(ly, dbu_));
      pin_ranges[R].second = std::max(pin_ranges[R].second, Dbu2Micro(uy, dbu_));
    } else {
      pin_ranges[B].first = std::min(pin_ranges[B].first,   Dbu2Micro(lx, dbu_));
      pin_ranges[B].second = std::max(pin_ranges[B].second, Dbu2Micro(ux, dbu_));
    }
  }
  // add the blockages into the blockages_
  const float v_blockage = (floorplan_uy_ - floorplan_ly_) * pin_access_th_;
  const float h_blockage = (floorplan_ux_ - floorplan_lx_) * pin_access_th_;
  float sum_range = 0.0;
  blockages_.push_back(Rect(floorplan_lx_, pin_ranges[L].first, 
                            floorplan_lx_ + h_blockage, pin_ranges[L].second));
  blockages_.push_back(Rect(pin_ranges[T].first, floorplan_uy_ - v_blockage,
                            pin_ranges[T].second, floorplan_uy_));
  blockages_.push_back(Rect(floorplan_ux_ - h_blockage, pin_ranges[R].first, 
                            floorplan_ux_, pin_ranges[R].second));
  blockages_.push_back(Rect(pin_ranges[B].first, floorplan_ly_,
                            pin_ranges[B].second, floorplan_ly_ + v_blockage));
  for (auto& [pin, range] : pin_ranges)
    if (range.second > range.first)
      sum_range += range.second - range.first;
  float net_sum = 0.0;
  for (auto& cluster : root_cluster_->GetChildren()) {
    if (cluster->GetIOClusterFlag() == false)
      continue;
    for (auto& [cluster_id, num_net] : cluster->GetConnection())
      net_sum += num_net;
  }
  sum_range /= 2 * (floorplan_uy_ - floorplan_ly_ + floorplan_ux_ - floorplan_lx_);
  // update the pin_access_net_width_ratio_
  pin_access_net_width_ratio_ = sum_range / net_sum;
}




// Cluster Placement Engine Starts ...........................................
// The cluster placement is done in a top-down manner
// (Preorder DFS)
void HierRTLMP::MultiLevelMacroPlacement(Cluster* parent)
{
  // base case
  // If the parent cluster has no macros (parent cluster is a StdCellCluster or IOCluster)
  // We do not need to determine the positions and shapes of its children clusters
  if (parent->GetNumMacro() == 0)
    return;
  // If the parent is a HardMacroCluster
  if (parent->GetClusterType() == HardMacroCluster) {
    HardMacroClusterMacroPlacement(parent);
    return;
  }

  
  //pin_access_th_ = pin_access_th_orig_ * parent->GetStdCellArea() /
  //                 (parent->GetStdCellArea() + parent->GetMacroArea());
  //pin_access_th_ = pin_access_th_orig_ 
  //               * ( 1  - (parent->GetStdCellArea() + parent->GetMacroArea())
  //                 / (parent->GetWidth() * parent->GetHeight()));


  // set the instance property
  for (auto& cluster : parent->GetChildren())
    SetInstProperty(parent);

  // Place children clusters
  // map children cluster to soft macro
  for (auto& cluster : parent->GetChildren()) {
    if (cluster->GetIOClusterFlag() == true)
      continue;
    SoftMacro* macro = new SoftMacro(cluster); 
    cluster->SetSoftMacro(macro); // we will detele the old one, no memory leakage
  }
  // detemine the settings for simulated annealing engine
  // Set outline information
  const float lx = parent->GetX();
  const float ly = parent->GetY();
  const float outline_width  = parent->GetWidth();
  const float outline_height = parent->GetHeight();
  const float ux = lx + outline_width;
  const float uy = ly + outline_height;
  // For Debug
  if (1) {
    std::cout << "\n\n" << std::endl;
    std::cout << "[Debug][HierRTLMP::MultiLevelMacroPlacement] : ";
    std::cout << parent->GetName() << "  " << lx  << "  " << ly << "  ";
    std::cout << outline_width << "   " << outline_height << std::endl;
  }
  // finish Debug
  
  
  // Suppose the region, fence, guide has been mapped to cooresponding macros
  std::map<std::string, int> soft_macro_id_map; // cluster_name, macro_id
  std::map<int, Rect> fences;
  std::map<int, Rect> guides;
  std::vector<SoftMacro> macros;
  std::vector<BundledNet> nets;
  std::vector<Rect> blockages;
  // create macros
  // Eech blockage will be modeled as fixed SoftMacro
  /*
  for (auto& blockage : blockages_) {
    //std::cout << "lx:  " << lx << "  xMin : " << blockage.xMin() << std::endl;
    //std::cout << "ly:  " << ly << "  yMin : " << blockage.yMin() << std::endl;
    //std::cout << "ux:  " << ux << "  xMax : " << blockage.xMax() << std::endl;
    //std::cout << "uy:  " << uy << "  yMax : " << blockage.yMax() << std::endl;
    
    // calculate the overlap between blockage and parent cluster
    const float b_lx = std::max(lx, blockage.xMin());
    const float b_ly = std::max(ly, blockage.yMin());
    const float b_ux = std::min(ux, blockage.xMax());
    const float b_uy = std::min(uy, blockage.yMax());
    if ((b_ux - b_lx > 0.0) && (b_uy - b_ly > 0.0)) { // blockage exists
      std::string name = std::string("blockage_") + std::to_string(macros.size());
      soft_macro_id_map[name] = macros.size();
      //macros.push_back(SoftMacro(b_ux - b_lx, b_uy - b_ly, name, b_lx - lx, b_ly -ly));
      macros.push_back(SoftMacro(b_ux - b_lx, b_uy - b_ly, name));
      macros[macros.size() - 1].SetReference(b_lx - lx, b_ly - ly);
      fences[macros.size() - 1] = Rect(b_lx - lx, b_ly - ly, b_ux - lx, b_uy - ly);
    }
  }
  */
  for (auto& blockage : blockages_) {
    // calculate the overlap between blockage and parent cluster
    const float b_lx = std::max(lx, blockage.xMin());
    const float b_ly = std::max(ly, blockage.yMin());
    const float b_ux = std::min(ux, blockage.xMax());
    const float b_uy = std::min(uy, blockage.yMax());
    if ((b_ux - b_lx > 0.0) && (b_uy - b_ly > 0.0)) { // blockage exists
      blockages.push_back(Rect(b_lx - lx, b_ly - ly, b_ux - lx, b_uy - ly));
    }
  }

  // Each cluster is modeled as Soft Macro
  // The fences or guides for each cluster is created by merging
  // the fences and guides for hard macros in each cluster
  for (auto& cluster : parent->GetChildren()) {
    // for IO cluster
    if (cluster->GetIOClusterFlag() == true) {
      soft_macro_id_map[cluster->GetName()] = macros.size();
      macros.push_back(SoftMacro(std::pair<float, float>(cluster->GetX() - lx, 
                                 cluster->GetY() - ly), cluster->GetName(),
                                 cluster->GetWidth(), cluster->GetHeight(), cluster));
      continue;
    }
    // for other clusters
    soft_macro_id_map[cluster->GetName()] = macros.size();
    SoftMacro* soft_macro = new SoftMacro(cluster);
    SetInstProperty(cluster); // we need this step to calculate nets
    macros.push_back(*soft_macro);
    cluster->SetSoftMacro(soft_macro);
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
    fence.Relocate(lx, ly, ux, uy); // calculate the overlap with outline
    guide.Relocate(lx, ly, ux, uy); // calculate the overlap with outline
    if (fence.IsValid())
      fences[macros.size() - 1] = fence; // current macro id is macros.size() - 1
    if (guide.IsValid())
      guides[macros.size() - 1] = guide; // current macro id is macros.size() - 1
  }
  
  // update the connnection
  CalculateConnection();
  std::cout << "finish calculate connection" << std::endl;
  UpdateDataFlow();
  std::cout << "finish updating dataflow" << std::endl;
  if (parent->GetParent() != nullptr) { 
    // the parent cluster is not the root cluster
    // First step model each pin access as the a dummy softmacro
    // In our simulated annealing engine, the dummary softmacro will 
    // no effect on SA
    // We have four dummy SoftMacros based on our definition
    std::vector<PinAccess> pins = {L, T, R, B};
    for (auto& pin : pins) {
      soft_macro_id_map[to_string(pin)] = macros.size();
      macros.push_back(SoftMacro(0.0, 0.0, to_string(pin)));
    }
    // add the connections between pin access
    for (auto& [src_pin, pin_map] : parent->GetBoundaryConnection()) {
      for (auto& [target_pin, old_weight] : pin_map) {
        float weight = old_weight;
        //if (macros[soft_macro_id_map[to_string(src_pin)]].IsStdCellCluster() == true ||
        //    macros[soft_macro_id_map[to_string(target_pin)]].IsStdCellCluster() == true)
        //  weight *= virtual_weight_;
        nets.push_back(BundledNet(soft_macro_id_map[to_string(src_pin)],
                                  soft_macro_id_map[to_string(target_pin)], weight));
      }
    }
  } 
  // add the virtual connections
  for (auto conn : parent->GetVirtualConnections())
  {
    BundledNet net(soft_macro_id_map[cluster_map_[conn.first]->GetName()], 
                   soft_macro_id_map[cluster_map_[conn.second]->GetName()],
                   virtual_weight_);
    net.src_cluster_id = conn.first;
    net.target_cluster_id = conn.second;
    nets.push_back(net);
  }
 
  for (auto inst : block_->getInsts()) {
    const int cluster_id = odb::dbIntProperty::find(inst, "cluster_id")->getValue();
    if (cluster_id == root_cluster_->GetId())
      std::cout << "root  inst_name : " << inst->getName() << std::endl;
  }

  // convert the connections between clusters to SoftMacros
  for (auto& cluster : parent->GetChildren()) {
    const int src_id = cluster->GetId();
    const std::string src_name = cluster->GetName();
    for (auto& [cluster_id, weight] : cluster->GetConnection()) {
      std::cout << "********************************************" << std::endl;
      std::cout << cluster->GetName() << "   " 
                << cluster_map_[cluster_id]->GetName() << "   "
                << weight << std::endl;
      std::cout << std::endl;
      const std::string name = cluster_map_[cluster_id]->GetName();
      if (soft_macro_id_map.find(name) == soft_macro_id_map.end()) {
        float new_weight = weight;
        if (macros[soft_macro_id_map[src_name]].IsStdCellCluster() == true)
          new_weight *= virtual_weight_;
        // if the cluster_id is out of the parent cluster
        BundledNet net(soft_macro_id_map[src_name], 
                       soft_macro_id_map[to_string(parent->GetPinAccess(cluster_id).first)],
                       new_weight);
        net.src_cluster_id = src_id;
        net.target_cluster_id = cluster_id;
        nets.push_back(net);
      } else if (src_id > cluster_id) {
        BundledNet net(soft_macro_id_map[src_name], soft_macro_id_map[name],
                       weight);
        net.src_cluster_id = src_id;
        net.target_cluster_id = cluster_id;
        nets.push_back(net);
      }
      //  nets.push_back(BundledNet(soft_macro_id_map[src_name],
      //    soft_macro_id_map[to_string(parent->GetPinAccess(cluster_id))], weight));
      //else
      //  nets.push_back(BundledNet(soft_macro_id_map[src_name],
      //                            soft_macro_id_map[name], weight));
      //if (src_id >= cluster_id) // the connection graph is undirected
      //  continue;
      //const std::string name = cluster_map_[cluster_id]->GetName();
      // if the connection with pin access
      //if (soft_macro_id_map.find(name) == soft_macro_id_map.end())
      //  nets.push_back(BundledNet(soft_macro_id_map[src_name],
      //    soft_macro_id_map[to_string(parent->GetPinAccess(cluster_id))], weight));
      //else
      //  nets.push_back(BundledNet(soft_macro_id_map[src_name],
      //                            soft_macro_id_map[name], weight));
      //if (soft_macro_id_map.find(name) != soft_macro_id_map.end())
      //  nets.push_back(BundledNet(soft_macro_id_map[src_name],
      //                            soft_macro_id_map[name], weight));
    }
    //for (auto& [target_pin, weight] : cluster->GetPinAccessMap())
    //  nets.push_back(BundledNet(soft_macro_id_map[src_name],
    //                            soft_macro_id_map[to_string(target_pin)], weight));
  }
  // merge nets to reduce runtime
  MergeNets(nets);
  std::cout << "Aug11_test" << std::endl;
  for (auto& net : nets) {
    std::cout << "net : " << macros[net.terminals.first].GetName()
              << "   " << macros[net.terminals.second].GetName()
              << std::endl;
  }
  
  if (parent->GetParent() != nullptr) {
    //pin_access_th_ = 0.05;
    // update the size of each pin access macro
    // each pin access macro with have their fences
    // check the net connection
          
    std::map<int, float> net_map;
    net_map[soft_macro_id_map[to_string(L)]] = 0.0;
    net_map[soft_macro_id_map[to_string(T)]] = 0.0;
    net_map[soft_macro_id_map[to_string(R)]] = 0.0;
    net_map[soft_macro_id_map[to_string(B)]] = 0.0;
    for (auto& net : nets) {
      std::cout << macros[net.terminals.first].GetName() << "   "
                << macros[net.terminals.second].GetName() << std::endl;
      if (net_map.find(net.terminals.first) != net_map.end())
        net_map[net.terminals.first] += net.weight;
      if (net_map.find(net.terminals.second) != net_map.end())
        net_map[net.terminals.second] += net.weight;
    }
    
    for (auto [id, weight] : net_map)
      std::cout << id << "   " << weight << std::endl;

    //float h_size = outline_width * pin_access_th_;
    //float v_size = outline_height * pin_access_th_;
    float std_cell_area = 0.0;
    float macro_area = 0.0;
    for (auto& cluster : parent->GetChildren()) {
      std_cell_area += cluster->GetStdCellArea();
      //macro_area += cluster->GetMacroArea();
    }

    //std_cell_area = std_cell_area / (std_cell_area + macro_area) * outline_width * outline_height;
    for (auto& shape : parent->GetMacroTilings()) {
      if (shape.first <= outline_width && shape.second <= outline_height) {
        macro_area = shape.first * shape.second;
        break;
      }
    }
    
    float ratio = (1.0 - std::sqrt((macro_area + std_cell_area) / (outline_width * outline_height))) / 2.0;
    ratio  = ratio / 2.0;
    float h_size = outline_width * ratio;
    float v_size = outline_height * ratio;
    
    //std_cell_area = outline_width * outline_height - macro_area - std_cell_area;
    //if (std_cell_area <= 0.0)
    //   std_cell_area = 0.0;
    //std_cell_area = outline_width * outline_height - macro_area - std_cell_area;
    //float h_size = std_cell_area / 4.0 / outline_height;
    //float v_size = std_cell_area / 4.0 / outline_width;
    //if (parent->GetParent()->GetParent() == nullptr) {
    //  h_size *= 2;
    //  v_size *= 2;
    //}

    float net_to_width = 1.0 / 12.0 * 4;
    //float L_size = std::min(outline_height - 2 * v_size, 
    //               net_to_width * net_map[soft_macro_id_map[to_string(L)]]);
    //float R_size = std::min(outline_height - 2 * v_size, 
    //               net_to_width * net_map[soft_macro_id_map[to_string(R)]]);
    //float B_size = std::min(outline_width - 2 * h_size,
    //               net_to_width * net_map[soft_macro_id_map[to_string(B)]]); 
    //float T_size = std::min(outline_width - 2 * h_size, 
    //               net_to_width * net_map[soft_macro_id_map[to_string(T)]]);
    
    float L_size =  net_map[soft_macro_id_map[to_string(L)]] > 0 ?  (outline_height - 2 * v_size) / 2.0 : 0.0;
    float R_size =  net_map[soft_macro_id_map[to_string(R)]] > 0 ?  (outline_height - 2 * v_size) / 2.0 : 0.0;
    float B_size =  net_map[soft_macro_id_map[to_string(B)]] > 0 ?  (outline_width - 2 * h_size) / 2.0 : 0.0;
    float T_size =  net_map[soft_macro_id_map[to_string(T)]] > 0 ?  (outline_width - 2 * h_size) / 2.0 : 0.0;

    /*
    const float ratio = 1.0;
    float L_size = std::min(outline_height * ratio, 
                   outline_height * net_map[soft_macro_id_map[to_string(L)]] 
                   * pin_access_net_width_ratio_);
    float R_size = std::min(outline_height * ratio, 
                   outline_height * net_map[soft_macro_id_map[to_string(R)]] 
                   * pin_access_net_width_ratio_);
    float B_size = std::min(outline_width * ratio, 
                   outline_width * net_map[soft_macro_id_map[to_string(B)]] 
                   * pin_access_net_width_ratio_);
    float T_size = std::min(outline_width * ratio, 
                   outline_width * net_map[soft_macro_id_map[to_string(T)]] 
                   * pin_access_net_width_ratio_);
    */
    std::cout << "pin_access_net_width_ratio_:  " << pin_access_net_width_ratio_ << std::endl;
    // update the size of pin access macro
    if (L_size > 0) {
      //L_size = v_size;
      macros[soft_macro_id_map[to_string(L)]] = SoftMacro(h_size, L_size, to_string(L));
      fences[soft_macro_id_map[to_string(L)]] = Rect(0.0, 0.0, h_size, outline_height);
    }
    if (R_size > 0) {
      //R_size = v_size;
      macros[soft_macro_id_map[to_string(R)]] = SoftMacro(h_size, R_size, to_string(R));
      fences[soft_macro_id_map[to_string(R)]] = Rect(outline_width - h_size, 0.0, 
                                                     outline_width, outline_height);
    }
    if (T_size > 0) {
      //T_size = h_size;
      macros[soft_macro_id_map[to_string(T)]] = SoftMacro(T_size, v_size, to_string(T));
      fences[soft_macro_id_map[to_string(T)]] = Rect(0.0, outline_height - v_size, 
                                                     outline_width, outline_height);
    }
    if (B_size > 0) {
      //B_size = h_size;
      macros[soft_macro_id_map[to_string(B)]] = SoftMacro(B_size, v_size, to_string(B));
      fences[soft_macro_id_map[to_string(B)]] = Rect(0.0, 0.0, outline_width, v_size);
    }
  }
  
  std::ofstream file;
  std::string file_name = parent->GetName();
  for (int i = 0; i < file_name.size(); i++)
    if (file_name[i] == '/')
      file_name[i] = '*';
      
  file_name = string(report_directory_) + string("/") + file_name;
  file.open(file_name + "net.txt");
  for (auto& net : nets)
    file << macros[net.terminals.first].GetName() << "   "
         << macros[net.terminals.second].GetName() << "   "
         << net.weight << std::endl;
  file.close();


  // Call Simulated Annealing Engine to place children
  // set the action probabilities
  const float action_sum  = pos_swap_prob_ + neg_swap_prob_ + double_swap_prob_
                            + exchange_swap_prob_ + resize_prob_;
  // set the penalty weight
  float weight_sum  = outline_weight_ + wirelength_weight_
                            + guidance_weight_   + fence_weight_    
                            + boundary_weight_   + notch_weight_;

  // We vary the target utilization to generate different tilings
  std::vector<float> target_utils { target_util_  };
  std::vector<float> target_dead_spaces { target_dead_space_ };
  for (int i = 1; i < num_target_util_; i++)
    //if (target_util_ + i * target_util_step_ < 1.0)
      target_utils.push_back(target_util_ + i * target_util_step_);
  for (int i = 1; i < num_target_dead_space_; i++)
    if (target_dead_space_ + i * target_dead_space_step_ < 1.0)
      target_dead_spaces.push_back(target_dead_space_ + i * target_dead_space_step_);
  std::vector<float> target_util_list; // target util has higher priority than target_dead_space
  std::vector<float> target_dead_space_list;
  for (auto& target_util : target_utils) {
    for (auto& target_dead_space : target_dead_spaces) {
      target_util_list.push_back(target_util);
      target_dead_space_list.push_back(target_dead_space);
    }
  }



  float new_notch_weight = notch_weight_;
  for (auto& cluster : parent->GetChildren()) {
    if (cluster->GetClusterType() == HardMacroCluster) {
      new_notch_weight = weight_sum;
      break;
    }
  }



  const int num_perturb_per_step = (macros.size() > num_perturb_per_step_) ?
                                    macros.size() : num_perturb_per_step_;
  int run_thread = num_threads_;
  int remaining_runs = target_util_list.size();
  int run_id = 0;
  SACoreSoftMacro* best_sa = nullptr;
  float best_cost = std::numeric_limits<float>::max();
  std::cout << "Start Simulated Annealing Core" << std::endl;
  while (remaining_runs > 0) {
    std::vector<SACoreSoftMacro*> sa_vector;
    run_thread = (remaining_runs > num_threads_) ? num_threads_ : remaining_runs;
    for (int i = 0; i < run_thread; i++) {
      std::vector<SoftMacro> shaped_macros = macros; // copy for multithread
      // determine the shape for each macro
      std::cout << "start run_id :  " << run_id << std::endl;
      const float target_util = target_util_list[run_id];
      const float target_dead_space = target_dead_space_list[run_id++];
      if (ShapeChildrenCluster(parent, shaped_macros, soft_macro_id_map, 
          target_util, target_dead_space) == false)
        continue;
      std::cout << "finish run_id :  " << run_id << std::endl;
      weight_sum = 1.0;
      SACoreSoftMacro* sa = new SACoreSoftMacro(outline_width, outline_height, 
                                                shaped_macros,
                                                area_weight_, 
                                                outline_weight_ / weight_sum,
                                                wirelength_weight_ / weight_sum,
                                                guidance_weight_ / weight_sum,
                                                fence_weight_ / weight_sum,
                                                boundary_weight_ / weight_sum,
                                                macro_blockage_weight_ / weight_sum, 
                                                new_notch_weight / weight_sum,
                                                notch_h_th_, notch_v_th_,
                                                pos_swap_prob_ / action_sum,
                                                neg_swap_prob_ / action_sum,
                                                double_swap_prob_ / action_sum,
                                                exchange_swap_prob_ / action_sum,
                                                resize_prob_ / action_sum,
                                                init_prob_, max_num_step_,
                                                num_perturb_per_step,
                                                k_, c_, random_seed_);
      sa->SetFences(fences);
      sa->SetGuides(guides);
      sa->SetNets(nets);
      sa->SetBlockages(blockages);
      sa_vector.push_back(sa);
    }
    // multi threads 
    std::vector<std::thread> threads;
    for (auto& sa : sa_vector)
      threads.push_back(std::thread(RunSA<SACoreSoftMacro>, sa));
    for (auto& th : threads)
      th.join();
    // add macro tilings
    for (auto& sa : sa_vector) {
      if (sa->IsValid() && sa->GetNormCost() < best_cost) {
        best_cost = sa->GetNormCost();
        best_sa = sa;
      } else {
        delete sa; // avoid memory leakage
      }
    }
    sa_vector.clear();
    // add early stop mechanism
    if (best_sa != nullptr)
      break;
    remaining_runs -= run_thread;
  }
  std::cout << "Finish SA" << std::endl;

  if (best_sa == nullptr) {
    std::string line = "This is no valid tilings for cluster ";
    line += parent->GetName();
    logger_->error(MPL, 2032, line);      
  } 
  // update the clusters and do bus planning
  std::vector<SoftMacro> shaped_macros;
  best_sa->FillDeadSpace();
  best_sa->AlignMacroClusters();
  best_sa->GetMacros(shaped_macros);
  std::cout << "\n\n\n";
  best_sa->PrintResults();
  std::cout << "\n\n";

  // For debug
  //std::ofstream file;
  //file_name = string("./") + string(report_directory_) + string("/") + file_name;
  logger_->report(" file_name (cur) = {}", file_name);

  file.open(file_name + ".fp.txt");
  for (auto& macro : shaped_macros)
    file << macro.GetName() << "   "
         << macro.GetX()    << "   "
         << macro.GetY()    << "   "
         << macro.GetWidth() << "   "
         << macro.GetHeight() << std::endl;
  file.close();

  for (auto& macro : shaped_macros) {
    std::cout << macro.GetName() << "   "
         << macro.GetX()    << "   "
         << macro.GetY()    << "   "
         << macro.GetWidth() << "   "
         << macro.GetHeight() << std::endl;
    macro.PrintShape();
  }
  
  // just for test
  for (auto& cluster : parent->GetChildren()) {
    *(cluster->GetSoftMacro()) = shaped_macros[soft_macro_id_map[cluster->GetName()]]; 
    // For Debug
    if (1) {
      std::cout << "\n\n" << std::endl;
      std::cout << "[Debug][HierRTLMP::MultiLevelMacroPlacement] Child : ";
      std::cout << cluster->GetName() << "  ";
      std::cout << cluster->GetX() << "  ";
      std::cout << cluster->GetY() << "  ";
      std::cout << std::endl;
      std::cout << cluster->GetWidth() << "  ";
      std::cout << cluster->GetHeight() << "  ";
      std::cout << std::endl;
    }
  }
 
  // check if the parent cluster still need bus planning
  for (auto& child : parent->GetChildren()) {
    if (child->GetClusterType() == MixedCluster) {
      std::cout << "\n\n Call Bus Synthesis\n\n" << std::endl; 
      // Call Path Synthesis to route buses
      CallBusPlanning(shaped_macros, nets);
      break;
    }
  }

  //std::cout << "\n\n Call Bus Synthesis\n\n" << std::endl; 
  // Call Path Synthesis to route buses
  //CallBusPlanning(shaped_macros, nets);
  // remove sa
  delete best_sa;
  
  // update the location of children cluster to their real location
  for (auto& cluster : parent->GetChildren()) {
    cluster->SetX(cluster->GetX() + lx);
    cluster->SetY(cluster->GetY() + ly);
    std::cout << "lx :  " << lx << "  ly :  " << ly << std::endl;
    // For Debug
    if (1) {
      std::cout << "\n\n" << std::endl;
      std::cout << "[Debug][HierRTLMP::MultiLevelMacroPlacement] Child : ";
      std::cout << cluster->GetName() << "  ";
      std::cout << cluster->GetX() << "  ";
      std::cout << cluster->GetY() << "  ";
      std::cout << std::endl;
    }
    // finish Debug
  }
  
    
  // for debug
  file.open(string("./") + string(report_directory_) + string("/") + "pin_access.txt");
  for (auto& cluster : parent->GetChildren()) {
    std::set<PinAccess> pin_access;
    for (auto& [cluster_id, pin_weight] : cluster->GetPinAccessMap())
      pin_access.insert(pin_weight.first);
    
    for (auto& [src, connection] : cluster->GetBoundaryConnection()) {
      pin_access.insert(src);
      for (auto& [target, weight] : connection)
        pin_access.insert(target);
    }

    file << cluster->GetName() << "   ";
    for (auto& pin : pin_access)
      file << to_string(pin)   << "   ";
    file << std::endl;

    std::cout << "pin access :  " << cluster->GetName() << "   ";
    for (auto& pin : pin_access)
      std::cout << to_string(pin) << "  ";
    std::cout << std::endl;
  }
  file.close();



  // Traverse the physical hierarchy tree in a DFS manner
  for (auto& cluster : parent->GetChildren())
    if (cluster->GetClusterType() == MixedCluster ||
        cluster->GetClusterType() == HardMacroCluster)
      MultiLevelMacroPlacement(cluster);

  // done this branch and update the cluster_id property back
  SetInstProperty(parent);
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
    if (net_class[i] > -1)
      nets[net_class[i]].weight += nets[i].weight;

  std::vector<BundledNet> merged_nets;
  for (int i = 0; i < net_class.size(); i++)
    if (net_class[i] == -1)
      merged_nets.push_back(nets[i]);
  nets.clear();
  nets = merged_nets;
}




/*
// Determine the shape of each cluster based on target utilization
// Note that target_util is used for determining the size of MixedCluster
// the target_dead_space is used for determining the size of StdCellCluster
bool HierRTLMP::ShapeChildrenCluster(Cluster* parent, 
                      std::vector<SoftMacro>& macros,
        std::map<std::string, int>& soft_macro_id_map, 
           float target_util, float target_dead_space)
{
  std::cout << "Enter ShapeChildrenCluster" << std::endl;
  // check the valid values
  if (target_util > 1.0 || target_util <= 0.0)
    target_util = 1.0;
  if (target_dead_space > 1.0 || target_dead_space < 0.0)
    target_dead_space = 1.0;
  const float outline_width = parent->GetWidth();
  const float outline_height = parent->GetHeight();
  std::cout << "target_dead_space : " << target_dead_space << std::endl;
  std::cout << "target utilization : " << target_util << std::endl;
  std::cout << "outline_width : " << outline_width << "  ";
  std::cout << "outline_height : " << outline_height << std::endl;
  std::cout << "macro_area : " << parent->GetMacroArea() << "  ";
  std::cout << "std cell area : " << parent->GetStdCellArea() << std::endl;
  // (1) we need to calculate available space for std cells
  float macro_area_tot = 0.0;
  float std_cell_area_tot = 0.0;
  // add the macro area for blockages, pin access and so on
  for (auto& macro : macros) {
    if (macro.GetCluster() == nullptr)
      macro_area_tot += macro.GetArea();
  }
  std::cout << "pin access area " << macro_area_tot << std::endl;
  // add the macro area in each cluster
  for (auto& cluster : parent->GetChildren()) {
    if (cluster->GetIOClusterFlag() == true) 
      continue;
    if (cluster->GetClusterType() == HardMacroCluster ||
        cluster->GetClusterType() == MixedCluster) {
      std::cout << "start this process" << std::endl;
      std::vector<std::pair<float, float> > shapes;
      for (auto& shape : cluster->GetMacroTilings()) {
        if (shape.first < outline_width * (1 + conversion_tolerance_) && 
            shape.second < outline_height * (1 + conversion_tolerance_)) {
          shapes.push_back(shape);
          std::cout << "Enter shape" << std::endl;
          std::cout << "shapes.size() : " << shapes.size() << std::endl;
          std::cout << "width : " << shape.first << "    "
                  << "height :  " << shape.second << "   "
                  << "outline_width : " << outline_width << "   "
                  << "outline_height :  " << outline_height << "   "
                  << std::endl;
        }
        std::cout << "conversion tolerance : " << conversion_tolerance_ << std::endl;
        std::cout << "width : " << shape.first << "    "
                  << "height :  " << shape.second << "   "
                  << "outline_width : " << outline_width << "   "
                  << "outline_height :  " << outline_height << "   "
                  << std::endl;
      }
      std::cout << "shapes.size()  :  " << shapes.size() << std::endl;
      if (shapes.size() == 0) {
        std::cout << "shapes.size() == 0 : " << (shapes.size() == 0) << std::endl;
        std::string line = "This is not enough space in cluster ";
        line += cluster->GetName();
        line += "  num_tilings:  " + std::to_string(cluster->GetMacroTilings().size()); 
        logger_->error(MPL, 2029, line);      
      } else {
        if (cluster->GetClusterType() == HardMacroCluster)
          macro_area_tot += shapes[0].first * shapes[0].second;
        else if (cluster->GetClusterType() == MixedCluster)
          macro_area_tot += shapes[0].first * shapes[0].second
                            * (1 + 2 * pin_access_th_)
                            * (1 + 2 * pin_access_th_);
        cluster->SetMacroTilings(shapes);
      }
      macro_area_tot += cluster->GetStdCellArea() / target_util 
                        * (1 + 2 * pin_access_th_)
                        * (1 + 2 * pin_access_th_);
    } else if (cluster->GetClusterType() == StdCellCluster) {
      std_cell_area_tot += cluster->GetStdCellArea();  
    }
  } 
  
  std::cout << "total macro area : " << macro_area_tot << std::endl;
  std::cout << "std_cell_area_tot : " << std_cell_area_tot << std::endl;
  const float avail_space = parent->GetWidth() * parent->GetHeight() - macro_area_tot
                            - std_cell_area_tot;
  if (avail_space <= 0.0)
    return false;   // no feasible solution
  const float std_cell_inflat = avail_space * (1.0 - target_dead_space) / std_cell_area_tot;
  std::cout << "std_cell_inflat : " << std_cell_inflat << std::endl;
  // set the shape for each macro
  for (auto& cluster : parent->GetChildren()) {
    if (cluster->GetIOClusterFlag() == true)
      continue;
    if (cluster->GetClusterType() == StdCellCluster) {
      const float area = cluster->GetArea() * (1.0 + std_cell_inflat);
      const float width = std::sqrt(area / min_ar_);
      std::vector<std::pair<float, float> > width_list;
      width_list.push_back(std::pair<float, float>(width, area / width));
      macros[soft_macro_id_map[cluster->GetName()]].SetShapes(width_list, area);
      std::cout << "[Debug][CalShape] std_cell_cluster : " << cluster->GetName() 
                << std::endl;
    } else if (cluster->GetClusterType() == HardMacroCluster) {
      macros[soft_macro_id_map[cluster->GetName()]].SetShapes(cluster->GetMacroTilings());
      std::cout << "[Debug][CalShape] hard_macro_cluster : " << cluster->GetName() 
                << std::endl;
    } else { // Mixed cluster
      float area = cluster->GetStdCellArea() / target_util;
      const std::vector<std::pair<float, float> > tilings = cluster->GetMacroTilings();
      std::vector<std::pair<float, float> > width_list;
      //area += tilings[0].first * (1 + pin_access_th_)
      //       * tilings[0].second * (1 + pin_access_th_);
      //for (auto& shape : tilings) {
      //  if (shape.first * shape.second <= area)
      //    width_list.push_back(std::pair<float, float>(
      //                  shape.first * (1 + pin_access_th_), 
      //                  area / shape.second / (1 + pin_access_th_)));
      //}
      area += tilings[0].first * tilings[0].second;
      area *= (1 + 2 * pin_access_th_);
      area *= (1 + 2 * pin_access_th_);
      for (auto& shape : tilings) {
        if (shape.first * shape.second <= area)
          width_list.push_back(std::pair<float, float>(
                        shape.first * (1 + 2 * pin_access_th_), 
                        area / shape.second / (1 + 2* pin_access_th_)));
      }
      //width_list.push_back(std::pair<float, float>(shape.first, area / shape.second));
      std::cout << "[Debug][CalShape] name:  " << cluster->GetName() << "  ";
      std::cout << "area:  " << area << "  ";
      std::cout << "width_list:  " << width_list[0].first << "   ";
      std::cout << width_list[0].second << std::endl;
      macros[soft_macro_id_map[cluster->GetName()]].SetShapes(width_list, area);
    }
  }

  for (auto& macro : macros)
    std::cout << "after shaping :  " << macro.GetName() << "   "
              << macro.GetWidth()    << "    "
              << macro.GetHeight()   << std::endl;
  
  
  std::cout << "Finish shape function" << std::endl;
  return true;
}
*/

// Determine the shape of each cluster based on target utilization
// Note that target_util is used for determining the size of MixedCluster
// the target_dead_space is used for determining the size of StdCellCluster
bool HierRTLMP::ShapeChildrenCluster(Cluster* parent, 
                      std::vector<SoftMacro>& macros,
        std::map<std::string, int>& soft_macro_id_map, 
           float target_util, float target_dead_space)
{
  std::cout << "Enter ShapeChildrenCluster" << std::endl;
  // check the valid values
  const float outline_width = parent->GetWidth();
  const float outline_height = parent->GetHeight();
  const float outline_area = outline_width * outline_height;
  std::cout << "target_dead_space : " << target_dead_space << std::endl;
  std::cout << "target utilization : " << target_util << std::endl;
  std::cout << "outline_width : " << outline_width << "  ";
  std::cout << "outline_height : " << outline_height << std::endl;
  std::cout << "macro_area : " << parent->GetMacroArea() << "  ";
  std::cout << "std cell area : " << parent->GetStdCellArea() << std::endl;
  // (1) we need to calculate available space for std cells
  float macro_area_tot = 0.0;
  float std_cell_area_tot = 0.0;
  float std_cell_mixed_cluster = 0.0;
  // add the macro area for blockages, pin access and so on
  for (auto& macro : macros) {
    if (macro.GetCluster() == nullptr)
      macro_area_tot += macro.GetArea();
  }
  std::cout << "pin access area " << macro_area_tot << std::endl;
  for (auto& cluster : parent->GetChildren()) {
    if (cluster->GetIOClusterFlag() == true) 
      continue;
    if (cluster->GetClusterType() == StdCellCluster) {
      std_cell_area_tot += cluster->GetStdCellArea();  
    } else if (cluster->GetClusterType() == HardMacroCluster) {
      std::vector<std::pair<float, float> > shapes;
      for (auto& shape : cluster->GetMacroTilings()) {
        if (shape.first < outline_width * (1 + conversion_tolerance_) && 
            shape.second < outline_height * (1 + conversion_tolerance_)) {
          shapes.push_back(shape);
        }
      }
      if (shapes.size() == 0) {
        std::string line = "This is not enough space in cluster ";
        line += cluster->GetName();
        line += "  num_tilings:  " + std::to_string(cluster->GetMacroTilings().size()); 
        logger_->error(MPL, 2029, line);      
      }
      macro_area_tot += shapes[0].first * shapes[0].second;
      cluster->SetMacroTilings(shapes);
    } else { // mixed cluster
      std_cell_area_tot += cluster->GetStdCellArea() * std::pow(1 + 2 * pin_access_th_, 2);  
      std_cell_mixed_cluster += cluster->GetStdCellArea() * std::pow(1 + 2 * pin_access_th_, 2);
      std::vector<std::pair<float, float> > shapes;
      for (auto& shape : cluster->GetMacroTilings()) {
        if (shape.first * (1 + 2 * pin_access_th_) < outline_width * (1 + conversion_tolerance_) && 
            shape.second * (1 + 2 * pin_access_th_) < outline_height * (1 + conversion_tolerance_)) {
          shapes.push_back(shape);
        }
      }
      if (shapes.size() == 0) {
        std::string line = "This is not enough space in cluster ";
        line += cluster->GetName();
        line += "  num_tilings:  " + std::to_string(cluster->GetMacroTilings().size()); 
        logger_->error(MPL, 2080, line);      
      }
      macro_area_tot += shapes[0].first * shapes[0].second 
                     * std::pow(1 + 2 * pin_access_th_, 2);
      cluster->SetMacroTilings(shapes);
    } // end for cluster type
  }
  // check how much available space to inflat for mixed cluster
  const float min_target_util = std_cell_mixed_cluster / 
                                (outline_area - std_cell_area_tot - macro_area_tot);
  if (target_util <= min_target_util)
    target_util = min_target_util;
  //else if (target_util >= 1.0)
  //  target_util = 1.0;
  // check how many empty space left
  const float avail_space = outline_area - macro_area_tot 
                          - (std_cell_area_tot - std_cell_mixed_cluster)
                          - std_cell_mixed_cluster / target_util;
  // make sure the target_dead_space in range
  if (target_dead_space <= 0.0)
    target_dead_space = 0.01;
  else if (target_dead_space >= 1.0)
    target_dead_space = 0.99;
  // shape clusters
  if (avail_space <= 0.0)
    return false;   // no feasible solution
  const float std_cell_inflat = avail_space * (1.0 - target_dead_space) / 
                            (std_cell_area_tot - std_cell_mixed_cluster);
  std::cout << "std_cell_inflat : " << std_cell_inflat << std::endl;
  // set the shape for each macro
  for (auto& cluster : parent->GetChildren()) {
    if (cluster->GetIOClusterFlag() == true)
      continue;
    if (cluster->GetClusterType() == StdCellCluster) {
      const float area = cluster->GetArea() * (1.0 + std_cell_inflat);
      const float width = std::sqrt(area / min_ar_);
      std::vector<std::pair<float, float> > width_list;
      width_list.push_back(std::pair<float, float>(width, area / width));
      macros[soft_macro_id_map[cluster->GetName()]].SetShapes(width_list, area);
      std::cout << "[Debug][CalShape] std_cell_cluster : " << cluster->GetName() 
                << std::endl;
    } else if (cluster->GetClusterType() == HardMacroCluster) {
      macros[soft_macro_id_map[cluster->GetName()]].SetShapes(cluster->GetMacroTilings());
      std::cout << "[Debug][CalShape] hard_macro_cluster : " << cluster->GetName() 
                << std::endl;
      for (auto& shape : cluster->GetMacroTilings())
        std::cout << " ( " << shape.first << " , " << shape.second << " ) " << "   ";
      std::cout << std::endl;
    } else { // Mixed cluster
      //float area = cluster->GetStdCellArea() / target_util;
      const std::vector<std::pair<float, float> > tilings = cluster->GetMacroTilings();
      std::vector<std::pair<float, float> > width_list;
      //area += tilings[0].first * tilings[0].second;
      // use the largest area
      float area = tilings[tilings.size() - 1].first * tilings[tilings.size() - 1].second;
      area += cluster->GetStdCellArea() / target_util;
      //float empty_space = area - cluster->GetMacroArea();
      //if (cluster->GetStdCellArea() / target_util - empty_space > 0.0)
      //  area += cluster->GetStdCellArea() / target_util - empty_space;
      
      // leave the space for pin access
      area = area / std::pow(1 - 2 * pin_access_th_, 2);
      for (auto& shape : tilings) {
        if (shape.first * shape.second <= area) {
          width_list.push_back(std::pair<float, float>(
                        shape.first / (1 - 2 * pin_access_th_), 
                        area / shape.second * (1 - 2 * pin_access_th_)));
          std::cout << "shape.first " << shape.first << "  "
                    << "shape.second  " << shape.second << "  "
                    << std::endl; 
        } 
      }
   
      /*
      for (auto& shape : tilings) {
        if (shape.first * shape.second <= area) {
          width_list.push_back(std::pair<float, float>(
                        shape.first, 
                        area / shape.second));
          std::cout << "shape.first " << shape.first << "  "
                    << "shape.second  " << shape.second << "  "
                    << std::endl; 
      }
      }
      */
      std::cout << "[Debug][CalShape] name:  " << cluster->GetName() << "  ";
      std::cout << "area:  " << area << "  ";
      std::cout << "width_list :  " << "   ";
      for (auto& width : width_list)
        std::cout << " [  " << width.first << "  " << width.second << "  ]  ";
      std::cout << std::endl;
      //std::cout << "width_list:  " << width_list[0].first << "   ";
      //std::cout << width_list[0].second << std::endl;
      macros[soft_macro_id_map[cluster->GetName()]].SetShapes(width_list, area);
    }
  }
  
  std::cout << "Finish shape function" << std::endl;
  return true;
}


// Call Path Synthesis to route buses
void HierRTLMP::CallBusPlanning(std::vector<SoftMacro>&  shaped_macros, 
     std::vector<BundledNet>& nets_old)
{
  std::vector<BundledNet> nets;
  for (auto& net : nets_old) {
    std::cout << "net.weight : " << net.weight << std::endl;
    if (net.weight > bus_net_threshold_)
      nets.push_back(net);
  }

  std::vector<int> soft_macro_vertex_id;
  std::vector<Edge> edge_list;
  std::vector<Vertex> vertex_list;
  if (CalNetPaths(shaped_macros, soft_macro_vertex_id, edge_list, vertex_list, 
      nets, congestion_weight_) == false)
    logger_->error(MPL, 2029, "Fail !!! Cannot do bus planning !!!");
}
  
// place macros within the HardMacroCluster
void HierRTLMP::HardMacroClusterMacroPlacement(Cluster* cluster)
{
  // Check if the cluster is a HardMacroCluster
  if (cluster->GetClusterType() != HardMacroCluster)
    return;
  
  std::cout << "Enter the HardMacroClusterMacroPlacement :  " 
            << cluster->GetName()
            << std::endl;
  
  // get outline constraint
  const float lx = cluster->GetX();
  const float ly = cluster->GetY();
  const float outline_width = cluster->GetWidth();
  const float outline_height = cluster->GetHeight();
  const float ux = lx + outline_width;
  const float uy = ly + outline_height;
  std::vector<HardMacro> macros;
  std::vector<Cluster*> macro_clusters;
  std::map<int, int> cluster_id_macro_id_map;
  std::vector<HardMacro*> hard_macros = cluster->GetHardMacros();
  num_hard_macros_cluster_ += hard_macros.size();
  std::map<int, Rect> fences;
  std::map<int, Rect> guides;
  // create a cluster for each macro
  for (auto& hard_macro : hard_macros) {
    int macro_id = macros.size();
    std::string cluster_name = hard_macro->GetName();
    Cluster* macro_cluster = new Cluster(cluster_id_, cluster_name);
    macro_cluster->AddLeafMacro(hard_macro->GetInst());
    SetInstProperty(macro_cluster);
    cluster_id_macro_id_map[cluster_id_] = macro_id;
    if (fences_.find(hard_macro->GetName()) != fences_.end()) {
      fences[macro_id] = fences_[hard_macro->GetName()];
      fences[macro_id].Relocate(lx, ly, ux, uy); 
    }
    if (guides_.find(hard_macro->GetName()) != guides_.end()) {
      guides[macro_id] = guides_[hard_macro->GetName()];
      guides[macro_id].Relocate(lx, ly, ux, uy); 
    }
    macros.push_back(*hard_macro);
    cluster_map_[cluster_id_++] = macro_cluster;
    macro_clusters.push_back(macro_cluster);
  }
  CalculateConnection();
  std::set<int> cluster_id_set;
  for (auto macro_cluster : macro_clusters) 
    for (auto [cluster_id, weight] : macro_cluster->GetConnection())
      cluster_id_set.insert(cluster_id);
  // create macros for other clusters
  for (auto cluster_id : cluster_id_set) {
    if (cluster_id_macro_id_map.find(cluster_id) !=
        cluster_id_macro_id_map.end())
      continue;
    auto& temp_cluster = cluster_map_[cluster_id];
    if (temp_cluster->GetIOClusterFlag() == true)
      continue;
    // model other cluster as a fixed macro with zero size
    cluster_id_macro_id_map[cluster_id] = macros.size();
    macros.push_back(HardMacro(std::pair<float, float>(
            temp_cluster->GetX() + temp_cluster->GetWidth() / 2.0 - lx,
            temp_cluster->GetY() + temp_cluster->GetHeight() / 2.0 - ly),
            temp_cluster->GetName()));
    std::cout << "terminal : lx = " << temp_cluster->GetX() + temp_cluster->GetWidth() / 2.0 - lx
              << "  ly = " << temp_cluster->GetY() + temp_cluster->GetHeight() / 2.0 - ly
              << std::endl;
  }
  // create bundled net
  std::vector<BundledNet> nets;
  for (auto macro_cluster : macro_clusters) {
    const int src_id = macro_cluster->GetId();
    for (auto [cluster_id, weight] : macro_cluster->GetConnection()) {
      if (cluster_map_[cluster_id]->GetIOClusterFlag() == true)
        continue;
      BundledNet net(cluster_id_macro_id_map[src_id],
                     cluster_id_macro_id_map[cluster_id], weight);
      net.src_cluster_id = src_id;
      net.target_cluster_id = cluster_id;
      nets.push_back(net);
    } // end connection
  } // end macro cluster
  // set global configuration
  // set the action probabilities
  const float action_sum  = pos_swap_prob_ + neg_swap_prob_ 
                          + double_swap_prob_  + exchange_swap_prob_
                          + flip_prob_;
  // set the penalty weight
  const float weight_sum  = area_weight_ + outline_weight_  
                          + wirelength_weight_  
                          + guidance_weight_ + fence_weight_;
  // We vary the outline of cluster to generate differnt tilings
  std::vector<float> vary_factor_list { 1.0 };
  float vary_step = 0.2 / num_runs_;  // change the outline by at most 10 percent
  for (int i = 1; i <= num_runs_ / 2 + 1; i++) {
    vary_factor_list.push_back(1.0 + i * vary_step);
    vary_factor_list.push_back(1.0 - i * vary_step);
  }
  const int num_perturb_per_step = (macros.size() > num_perturb_per_step_ / 10) ?
                                    macros.size() : num_perturb_per_step_ / 10;
  //const int num_perturb_per_step = macros.size();
  //num_threads_  = 1; 
  int run_thread = num_threads_;
  int remaining_runs = num_runs_;
  int run_id = 0;
  SACoreHardMacro* best_sa = nullptr;
  float best_cost = std::numeric_limits<float>::max();
  while (remaining_runs > 0) {
    std::vector<SACoreHardMacro*> sa_vector;
    run_thread = (remaining_runs > num_threads_) ? num_threads_ : remaining_runs;
    for (int i = 0; i < run_thread; i++) {
      const float width = outline_width * vary_factor_list[run_id++];
      const float height = outline_width * outline_height / width;
      SACoreHardMacro* sa = new SACoreHardMacro(width, height, macros,
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
      threads.push_back(std::thread(RunSA<SACoreHardMacro>, sa));
    for (auto& th : threads)
      th.join();
    // add macro tilings
    for (auto& sa : sa_vector) {
      if (sa->IsValid() && sa->GetNormCost() < best_cost) {
        best_cost = sa->GetNormCost();
        best_sa = sa;
      } else {
        delete sa; // avoid memory leakage
      }
    }
    sa_vector.clear();
    remaining_runs -= run_thread;
  }
  // update the hard macro
  if (best_sa == nullptr) {
    std::string line = "This is no valid tilings for cluster ";
    line += cluster->GetName();
    logger_->error(MPL, 2033, line);      
  } else {
    std::vector<HardMacro> best_macros;
    best_sa->GetMacros(best_macros);
    best_sa->PrintResults();
    for (int i = 0; i < hard_macros.size(); i++)
      *(hard_macros[i]) = best_macros[i];
    delete best_sa;
  }
  // update OpenDB
  for (auto& hard_macro : hard_macros) {
    num_updated_macros_++;
    hard_macro->SetX(hard_macro->GetX() + lx);
    hard_macro->SetY(hard_macro->GetY() + ly);
    hard_macro->UpdateDb(pitch_x_, pitch_y_);
  }
  SetInstProperty(cluster);
}

}  // namespace mpl
