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

#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "SACoreHardMacro.h"
#include "SACoreSoftMacro.h"
#include "SimulatedAnnealingCore.h"
#include "bus_synthesis.h"
#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbSta.hh"
#include "object.h"
#include "odb/db.h"
#include "sta/Bfs.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"

namespace mpl {
// Hierarchial RTL-MP
// Support Multi-Level Clustering.
// Support designs with IO Pads.
// Support timing-driven macro placement (not enabled now, June, 2022)
// Identify RTL information based on logical hierarchy, connection signature and
// dataflow. Connection Signature is defined as the connection topology respect
// to other clusters. Dataflow is defined based on sequential graph.
// Timing-driven macro placement is based on uniform delay model.
// We just odb::dbIntProperty::create() to cluster_id attribute for BTerms and
// dbInsts.
class HierRTLMP
{
 public:
  HierRTLMP() {}
  HierRTLMP(ord::dbNetwork* network,
            odb::dbDatabase* db,
            sta::dbSta* sta,
            utl::Logger* logger);

  // Top Level Interface Function
  // This function is the inferface for calling HierRTLMP
  // This function works as following:
  // 1) Traverse the logical hierarchy, get all the statistics of each logical
  // module
  //    in logical_module_map_ and associate each hard macro with its HardMacro
  //    object
  // 2) Create Bundled pins and treat each bundled pin as a cluster with no size
  //    The number of bundled IOs is num_bundled_IOs_ x 4  (four boundaries)
  // 3) Create physical hierarchy tree in a DFS manner (Postorder)
  // 4) Place clusters and macros in a BFS manner (Preorder)
  void HierRTLMacroPlacer();

  // Interfaces functions for setting options
  // Hierarchical Macro Placement Related Options
  void SetGlobalFence(float fence_lx,
                      float fence_ly,
                      float fence_ux,
                      float fence_uy);
  void SetHaloWidth(float halo_width);
  // Hierarchical Clustering Related Options
  void SetNumBundledIOsPerBoundary(int num_bundled_ios);
  void SetTopLevelClusterSize(int max_num_macro,
                              int min_num_macro,
                              int max_num_inst,
                              int min_num_inst);
  void SetClusterSizeTolerance(float tolerance);
  void SetMaxNumLevel(int max_num_level);
  void SetClusterSizeRatioPerLevel(float coarsening_ratio);
  void SetLargeNetThreshold(int large_net_threshold);
  void SetSignatureNetThreshold(int signature_net_threshold);
  void SetAreaWeight(float area_weight);
  void SetOutlineWeight(float outline_weight);
  void SetWirelengthWeight(float wirelength_weight);
  void SetGuidanceWeight(float guidance_weight);
  void SetFenceWeight(float fence_weight);
  void SetBoundaryWeight(float boundary_weight);
  void SetNotchWeight(float notch_weight);
  void SetPinAccessThreshold(float pin_access_th);
  void SetTargetUtil(float target_util);
  void SetTargetDeadSpace(float target_dead_space);
  void SetMinAR(float min_ar);
  void SetSnapLayer(int snap_layer);
  void SetReportDirectory(const char* report_directory);

 private:
  ord::dbNetwork* network_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;

  // technology-related variables
  float dbu_ = 0.0;

  // enable bus synthesis or not
  // this feature is just for test
  bool path_syn_flag_ = false;

  int num_updated_macros_ = 0;
  int num_hard_macros_cluster_ = 0;

  // Parameters related to macro placement
  const char* report_directory_;
  // User can specify a global region for some designs
  float global_fence_lx_ = std::numeric_limits<float>::max();
  float global_fence_ly_ = std::numeric_limits<float>::max();
  float global_fence_ux_ = 0.0;
  float global_fence_uy_ = 0.0;

  float halo_width_ = 0.0;

  int num_runs_ = 10;     // number of runs for SA
  int num_threads_ = 10;  // number of threads
  int random_seed_ = 0;   // random seed for deterministic

  float target_dead_space_ = 0.2;        // dead space for the cluster
  float target_util_ = 0.25;             // target utilization of the design
  float target_dead_space_step_ = 0.05;  // step for dead space
  float target_util_step_ = 0.1;         // step for utilization
  float num_target_util_ = 10;
  float num_target_dead_space_ = 20;

  float min_ar_ = 0.3;  // the aspect ratio range for StdCellCluster (min_ar_, 1
                        // / min_ar_)

  float pin_access_th_ = 0.1;  // each pin access is modeled as a SoftMacro
  float pin_access_th_orig_ = 0.1;
  float pin_access_net_width_ratio_
      = 0.1;  // define the ratio of number of connections
              // related to IOs to the range of these IO spans
  float notch_v_th_ = 100.0;
  float notch_h_th_ = 100.0;

  int snap_layer_ = 4;
  float pitch_x_ = 0.0;
  float pitch_y_ = 0.0;

  // SA related parameters
  // weight for different penalty
  float area_weight_ = 0.1;
  float outline_weight_ = 1.0;
  float wirelength_weight_ = 1.0;
  float guidance_weight_ = 10.0;
  float fence_weight_ = 10.0;
  float boundary_weight_ = 5.0;
  float notch_weight_ = 1.0;
  float macro_blockage_weight_ = 5.0;

  // gudiances, fences, constraints
  std::map<std::string, Rect> fences_;  // macro_name, fence
  std::map<std::string, Rect> guides_;  // macro_name, guide
  std::vector<Rect> blockages_;         // blockages

  // Fast SA hyperparameter
  float init_prob_ = 0.9;
  int max_num_step_ = 5000;
  int num_perturb_per_step_ = 3000;
  // if step < k_, T = init_T_ / (c_ * step_);
  // else T = init_T_ / step
  int k_ = 5000000;
  int c_ = 1000.0;

  // the virtual weight between std cell part and corresponding macro part
  // to force them stay together
  float virtual_weight_ = 2000.0;

  // probability of each action
  float pos_swap_prob_ = 0.3;
  float neg_swap_prob_ = 0.3;
  float double_swap_prob_ = 0.2;
  float exchange_swap_prob_ = 0.0;
  float flip_prob_ = 0.2;
  float resize_prob_ = 0.2;

  // design-related variables
  // core area (in float)
  float floorplan_lx_ = 0.0;
  float floorplan_ly_ = 0.0;
  float floorplan_ux_ = 0.0;
  float floorplan_uy_ = 0.0;

  // dataflow parameters and store the dataflow
  int max_num_ff_dist_ = 3;  // maximum number of FF distances between
  float dataflow_factor_ = 2.0;
  std::vector<std::pair<odb::dbITerm*, std::vector<std::set<odb::dbInst*>>>>
      macro_ffs_conn_map_;
  std::vector<std::pair<odb::dbBTerm*, std::vector<std::set<odb::dbInst*>>>>
      io_ffs_conn_map_;
  std::vector<std::pair<odb::dbITerm*, std::vector<std::set<odb::dbInst*>>>>
      macro_macro_conn_map_;
  void CalDataFlow();
  void UpdateDataFlow();
  void DataFlowDFSIOPin(int parent,
                        int idx,
                        std::vector<std::set<odb::dbInst*>>& insts,
                        std::map<int, odb::dbBTerm*>& io_pin_vertex,
                        std::map<int, odb::dbInst*>& std_cell_vertex,
                        std::map<int, odb::dbITerm*>& macro_pin_vertex,
                        std::vector<bool>& stop_flag_vec,
                        std::vector<bool>& visited,
                        std::vector<std::vector<int>>& vertices,
                        std::vector<std::vector<int>>& hyperedges,
                        bool backward_flag);
  void DataFlowDFSMacroPin(int parent,
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
                           bool backward_flag);

  // statistics of the design
  // Here when we calculate macro area, we do not include halo_width
  Metric* metric_ = nullptr;
  // store the metric for each hierarchical logical module
  std::map<const odb::dbModule*, Metric*> logical_module_map_;
  // DFS-manner function to traverse the logical hierarchy
  Metric* ComputeMetric(odb::dbModule* module);
  // Calculate metric for cluster based its type
  void SetClusterMetric(Cluster* cluster);
  // associate each Macro to the HardMacro object
  std::map<odb::dbInst*, HardMacro*> hard_macro_map_;

  // user-specified variables
  int num_bundled_IOs_ = 3;  // number of bundled IOs on each boundary
  int max_num_macro_base_ = 0;
  int min_num_macro_base_ = 0;
  int max_num_inst_base_ = 0;
  int min_num_inst_base_ = 0;

  // we define the tolerance for num_insts to improve the
  // robustness of the clustering engine
  float tolerance_ = 0.1;
  int max_num_macro_ = 0;
  int min_num_macro_ = 0;
  int max_num_inst_ = 0;
  int min_num_inst_ = 0;

  // Multilevel support
  int max_num_level_ = 2;
  int level_ = 0;
  float coarsening_ratio_ = 5.0;

  // connection signature
  // minimum number of connections between two clusters
  // for them to be identified as connected
  int signature_net_threshold_ = 20;
  // We ignore global nets during clustering
  int large_net_threshold_ = 100;
  // we only consider bus when we do bus planning
  int bus_net_threshold_ = 32;
  // the weight used for balance timing and congestion
  float congestion_weight_ = 0.5;

  // Determine if the cluster is macro dominated
  // if num_std_cell * macro_dominated_cluster_threshold_ < num_macro
  // then the cluster is macro-dominated cluster
  float macro_dominated_cluster_threshold_ = 0.01;

  // since we convert from the database unit to the micrometer
  // during calculation, we may loss some accuracy.
  float conversion_tolerance_ = 0.01;

  // Physical hierarchy tree
  int cluster_id_ = 0;
  // root cluster does not correspond to top design
  // it's a special node in the physical hierachy tree
  // Our physical hierarchy tree is as following:
  //             root_cluster_ (top design)
  //            *    *    *    *    *   * 
  //           L    T     M1   M2    B    T  (L, T, B, T are clusters for
  //           bundled IOs)
  //                    *   * 
  //                   M3    M4     (M1, M2, M3 and M4 are clusters for logical
  //                   modules
  Cluster* root_cluster_ = nullptr;      // cluster_id = 0 for root cluster
  std::map<int, Cluster*> cluster_map_;  // cluster_id, cluster

  // All the bundled IOs are children of root_cluster_
  // Bundled IO (Pads)
  // In the bundled IO clusters, we don't store the ios in their corresponding
  // clusters However, we store the instances in their corresponding clusters
  // map IO pins to Pads (for designs with IO pads)
  std::map<odb::dbBTerm*, odb::dbInst*> io_pad_map_;
  // Map IOs to Pads (for designs with IO pads)
  void MapIOPads();
  // create bundled IOs as clusters (for designs with IO pins or Pads)
  void CreateBundledIOs();

  // update the cluster_id property of insts in the cluster
  // Create physical hierarchy tree in a post-order DFS manner
  void MultiLevelCluster(Cluster* parent);
  void SetInstProperty(Cluster* cluster);
  void SetInstProperty(odb::dbModule* module,
                       int cluster_id,
                       bool include_macro);
  void BreakCluster(Cluster* parent);
  void MergeClusters(std::vector<Cluster*>& candidate_clusters);
  void CalculateConnection();
  void PrintConnection();
  void PrintClusters();
  void UpdateSubTree(Cluster* parent);
  // Break large flat clusters with MLPart
  // A flat cluster does not have a logical module
  void BreakLargeFlatCluster(Cluster* cluster);

  // Traverse the physical hierarchy tree in a DFS manner
  // Split macros and std cells in the leaf clusters
  // In the normal operation, we call this function after
  // creating the physical hierarchy tree
  void LeafClusterStdCellHardMacroSep(Cluster* root_cluster);
  // Map all the macros into their HardMacro objects for all the clusters
  void MapMacroInCluster2HardMacro(Cluster* cluster);
  // Get all the hard macros in a logical module
  void GetHardMacros(odb::dbModule* module,
                     std::vector<HardMacro*>& hard_macros);

  // Print the physical hierachical tree in a DFS manner
  void PrintPhysicalHierarchyTree(Cluster* parent, int level);
  // Determine the macro tilings within each cluster in a bottom-up manner
  // (Post-Order DFS manner)
  void CalClusterMacroTilings(Cluster* root_cluster);
  // calculate the pin blockage for IO pins
  void CreatePinBlockage();
  // Determine the macro tilings for each HardMacroCluster
  // multi thread enabled
  // random seed deterministic enabled
  void CalHardMacroClusterShape(Cluster* cluster);
  // The cluster placement is done in a top-down manner
  // (Preorder DFS)
  void MultiLevelMacroPlacement(Cluster* parent);
  // place macros within the HardMacroCluster
  void HardMacroClusterMacroPlacement(Cluster* parent);
  // Merge nets to reduce runtime
  void MergeNets(std::vector<BundledNet>& nets);
  // determine the shape for children cluster
  bool ShapeChildrenCluster(Cluster* parent,
                            std::vector<SoftMacro>& macros,
                            std::map<std::string, int>& soft_macro_id_map,
                            float target_util,
                            float target_dead_space);
  // Call Path Synthesis to route buses
  void CallBusPlanning(std::vector<SoftMacro>& shaped_macros,
                       std::vector<BundledNet>& nets_old);

  /*
  // Place macros in a hierarchical mode based on the above
  // physcial hierarchical tree
  // The macro placement is done in a DFS manner (PreOrder)
  void MultiLevelMacroPlacement(Cluster* cluster);
  // Determine the shape (area, possible aspect ratios) of each cluster
  void CalClusterShape(Cluster* cluster);
  // Calculate possible macro tilings for HardMacroCluster
  void CalMacroTilings(Cluster* cluster);
  // Determine positions and implementation of each children cluster
  void PlaceChildrenClusters(Cluster* parent);
  // Merge nets to reduce runtime
  void MergeNets(std::vector<BundledNet>& nets);
  // Route buses within the parent cluster
  void CallPathSynthesis(Cluster* parent);
  // Determine the orientation and position of each hard macro
  // in each HardMacroCluster
  void PlaceHardMacros(Cluster* cluster);
  */
};
}  // namespace mpl
