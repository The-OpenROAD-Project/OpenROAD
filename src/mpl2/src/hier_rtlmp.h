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

#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Bfs.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"
#include "object.h"

namespace mpl {
// Hierarchial RTL-MP
// Support Multi-Level Clustering.
// Support designs with IO Pads.
// Support timing-driven macro placement (not enabled now, June, 2022)
// Identify RTL information based on logical hierarchy, connection signature and dataflow.
// Connection Signature is defined as the connection topology respect to other clusters.
// Dataflow is defined based on sequential graph. 
// Timing-driven macro placement is based on uniform delay model.
// We just odb::dbIntProperty::create() to cluster_id attribute for BTerms and dbInsts.
class HierRTLMP {
  public:
    HierRTLMP() {  }
    HierRTLMP(ord::dbNetwork* network,
              odb::dbDatabase* db,
              sta::dbSta* sta,
              utl::Logger* logger);
    // This function is the inferface for calling HierRTLMP
    // This function works as following:
    // 1) Traverse the logical hierarchy, get all the statistics of each logical module in logical_module_map_
    //    and associate each hard macro with its HardMacro object
    // 2) Create Bundled pins and treat each bundled pin as a cluster with no size
    //    The number of bundled IOs is num_bundled_IOs_ x 4  (four boundaries)
    // 3) 
    
    
    void HierRTLMacroPlacer();
    
    // We may enable a list of interface functions for different options
    // Hierarchical Macro Placement Related Options
    void SetGlobalFence(float fence_lx, float fence_ly,
                        float fence_ux, float fence_ux);
    void SetHaloWidth(float halo_width);


    // Hierarchical Clustering Related Options
    void SetNumBundledIOsPerBoundary(int num_bundled_ios);
    void SetTopLevelClusterSize(int max_num_macro, int min_num_macro,
                                int max_num_inst,  int min_num_inst);
    void SetClusterSizeTolerance(float tolerance);
    void SetMaxNumLevel(int max_num_level);
    void SetClusterSizeRatioPerLevel(float coarsening_ratio);
    void SetLargeNetThreshold(int large_net_threshold);
    void SetSignatureNetThreshold(int signature_net_threshold);


  private:
    ord::dbNetwork* network_ = nullptr;
    odb::dbDatabase* db_ = nullptr;
    odb::dbBlock* block_ = nullptr;
    sta::dbSta* sta_ = nullptr;
    utl::Logger* logger_ = nullptr;
 
    // technology-related variables
    float dbu_ = 0.0;
    
    // User can specify a global region for some designs
    int global_fence_lx_ = -1;
    int global_fence_ly_ = -1;
    int global_fence_ux_ = -1;
    int global_fence_uy_ = -1;

    // Parameters related to macro placement
    float halo_width_ = 0.0;

    // design-related variables
    // core area (in DBU)
    int floorplan_lx_ = -1;
    int floorplan_ly_ = -1;
    int floorplan_ux_ = -1;
    int floorplan_uy_ = -1;

    // statistics of the design
    Metric* metric_ = nullptr;
    // store the metric for each hierarchical logical module
    std::map<const odb::dbModule*, Metric*> logical_module_map_;
    Metric* ComputeMetric(odb::dbModule* module); // DFS-manner function to traverse the
                                                  // logical hierarchy
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
    int signature_net_threshold_ = 20.0;
    // We ignore global nets during clustering
    int large_net_threshold_ = 100; 

    // Physical hierarchy tree 
    int cluster_id_ = 0;
    Cluster* root_cluster_ = nullptr; // root cluster does not correspond to any logical
                                      // module, it's a special node in the physical hierachy tree
                                      // Our physical hierarchy tree is as following:
                                      //                  root_cluster_
                                      //            /    /     |       \    \
                                      //           L    T  top_design   B    T (L, T, B, T are clusters for bundled IOs)
                                      //                  /    |    \
                                      //                 A     B     C    (A, B, C are clusters for logical modules)
    std::map<int, Cluster*> cluster_map_;  // cluster_id, cluster

    // All the bundled IOs are children of root_cluster_
    // Bundled IO (Pads)
    // In the bundled IO clusters, we don't store the ios in their corresponding clusters
    // However, we store the instances in their corresponding clusters
    std::map<odb::dbBTerm*, odb::dbInst*> io_pad_map_; // map IO pins to Pads (for designs with IO pads)
    void MapIOPads(); // Map IOs to Pads (for designs with IO pads)
    void CreateBundledIOs(); // create bundled IOs as clusters (for designs with IO pins or Pads)

    // Create physical hierarchy tree in a post-order DFS manner
    void MultiLevelCluster(Cluster* parent);  // Recursive call for creating the tree
    void SetInstProperty(Cluster* cluster); // update the cluster_id property of insts in the cluster
    void SetInstProperty(odb::dbModule* module, int cluster_id); // update the cluster_id of inss in a logical module
    void BreakCluster(Cluster* parent);
    void UpdateSubTree(Cluster* parent);
    void MergeClusters(std::vector<Cluster*>& candidate_clusters);
    
    // Break large flat clusters with MLPart 
    // A flat cluster does not have a logical module
    void BreakLargeFlatCluster(Cluster* cluster); 
     
    void CalculateConnection();
    void PrintPhysicalHierarchyTree(Cluster* parent, int level = 0);

    // Metrics for dbModule and Cluster
    // store the metric for each hierarchical logical module
    // The ComputeMetric for cluster is based on
    // the result of ComputeMetric for module
    // So we need to compute the metrics for each
    // module first
    Metric ComputeMetric(Cluster* cluster);
    Metric ComputeMetric(std::vector<Cluster*>& clusters);
  };
}  // namespace mpl
