///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <map>
#include <set>
#include <unordered_map>
#include <vector>

namespace par {
class PartitionMgr;
}

namespace utl {
class Logger;
}

namespace sta {
class dbNetwork;
}

namespace odb {
class dbBlock;
class dbInst;
class dbBTerm;
class dbMaster;
class dbITerm;
class dbModule;
}  // namespace odb

namespace mpl2 {
class Metrics;
class Cluster;
class HardMacro;

struct DataFlow
{
  const int register_dist = 5;
  const float factor = 2.0;
  const float weight = 1;

  std::vector<std::pair<odb::dbITerm*, std::vector<std::set<odb::dbInst*>>>>
      macro_pin_to_regs;
  std::vector<std::pair<odb::dbBTerm*, std::vector<std::set<odb::dbInst*>>>>
      io_to_regs;
  std::vector<std::pair<odb::dbITerm*, std::vector<std::set<odb::dbInst*>>>>
      macro_pin_to_macros;
};

struct PhysicalHierarchyMaps
{
  std::map<int, Cluster*> id_to_cluster;
  std::unordered_map<odb::dbInst*, int> inst_to_cluster_id;
  std::unordered_map<odb::dbBTerm*, int> bterm_to_cluster_id;

  std::map<odb::dbInst*, HardMacro*> inst_to_hard;
  std::map<const odb::dbModule*, Metrics*> module_to_metrics;

  // Only for designs with IO Pads
  std::map<odb::dbBTerm*, odb::dbInst*> bterm_to_inst;
};

struct PhysicalHierarchy
{
  PhysicalHierarchy()
      : root(nullptr),
        base_max_macro(0),
        base_min_macro(0),
        base_max_std_cell(0),
        base_min_std_cell(0),
        max_level(0),
        bundled_ios_per_edge(0),
        large_net_threshold(0),
        min_net_count_for_connection(0),
        cluster_size_ratio(0.0f),
        cluster_size_tolerance(0.0f),
        virtual_weight(0.0f),
        halo_width(0.0f),
        halo_height(0.0f),
        macro_with_halo_area(0.0f),
        has_io_clusters(true),
        has_only_macros(false),
        has_std_cells(true),
        has_unfixed_macros(true)
  {
  }

  Cluster* root;
  PhysicalHierarchyMaps maps;

  int base_max_macro;
  int base_min_macro;
  int base_max_std_cell;
  int base_min_std_cell;

  int max_level;
  int bundled_ios_per_edge;
  int large_net_threshold;  // used to ignore global nets
  int min_net_count_for_connection;
  float cluster_size_ratio;
  float cluster_size_tolerance;
  float virtual_weight; // between std cell part and macro part

  float halo_width;
  float halo_height;
  float macro_with_halo_area;

  bool has_io_clusters;
  bool has_only_macros;
  bool has_std_cells;
  bool has_unfixed_macros;
};

class ClusteringEngine
{
 public:
  ClusteringEngine(odb::dbBlock* block,
                   sta::dbNetwork* network,
                   utl::Logger* logger,
                   par::PartitionMgr* triton_part);

  void run();
  void fetchDesignMetrics();

  void setDesignMetrics(Metrics* design_metrics);
  void setTree(PhysicalHierarchy* tree);

  // Methods to update the tree as the hierarchical
  // macro placement runs.
  void updateConnections();
  void updateDataFlow();
  void updateInstancesAssociation(Cluster* cluster);
  void updateInstancesAssociation(odb::dbModule* module,
                                  int cluster_id,
                                  bool include_macro);

  // Needed for macro placement
  void createClusterForEachMacro(const std::vector<HardMacro*>& hard_macros,
                                 std::vector<HardMacro>& sa_macros,
                                 std::vector<Cluster*>& macro_clusters,
                                 std::map<int, int>& cluster_to_macro,
                                 std::set<odb::dbMaster*>& masters);

 private:
  Metrics* computeMetrics(odb::dbModule* module);
  void reportLogicalHierarchyInformation(float core_area,
                                         float util,
                                         float core_util);
  void initTree();
  void setBaseThresholds();
  void createIOClusters();
  void mapIOPads();
  void treatEachMacroAsSingleCluster();
  void incorporateNewCluster(Cluster* cluster, Cluster* parent);
  void setClusterMetrics(Cluster* cluster);
  void multilevelAutocluster(Cluster* parent);
  void updateSizeThresholds();
  void breakCluster(Cluster* parent);
  void createFlatCluster(odb::dbModule* module, Cluster* parent);
  void addModuleInstsToCluster(Cluster* cluster, odb::dbModule* module);
  void createCluster(odb::dbModule* module, Cluster* parent);
  void createCluster(Cluster* parent);
  void updateSubTree(Cluster* parent);
  void breakLargeFlatCluster(Cluster* parent);
  void mergeClusters(std::vector<Cluster*>& candidate_clusters);
  void fetchMixedLeaves(Cluster* parent,
                        std::vector<std::vector<Cluster*>>& mixed_leaves);
  void breakMixedLeaves(const std::vector<std::vector<Cluster*>>& mixed_leaves);
  void breakMixedLeaf(Cluster* mixed_leaf);
  void mapMacroInCluster2HardMacro(Cluster* cluster);
  void getHardMacros(odb::dbModule* module,
                     std::vector<HardMacro*>& hard_macros);
  void createOneClusterForEachMacro(Cluster* parent,
                                    const std::vector<HardMacro*>& hard_macros,
                                    std::vector<Cluster*>& macro_clusters);
  void classifyMacrosBySize(const std::vector<HardMacro*>& hard_macros,
                            std::vector<int>& size_class);
  void classifyMacrosByConnSignature(
      const std::vector<Cluster*>& macro_clusters,
      std::vector<int>& signature_class);
  void classifyMacrosByInterconn(const std::vector<Cluster*>& macro_clusters,
                                 std::vector<int>& interconn_class);
  void groupSingleMacroClusters(const std::vector<Cluster*>& macro_clusters,
                                const std::vector<int>& size_class,
                                const std::vector<int>& signature_class,
                                std::vector<int>& interconn_class,
                                std::vector<int>& macro_class);
  void mergeMacroClustersWithinSameClass(Cluster* target, Cluster* source);
  void addStdCellClusterToSubTree(Cluster* parent,
                                  Cluster* mixed_leaf,
                                  std::vector<int>& virtual_conn_clusters);
  void replaceByStdCellCluster(Cluster* mixed_leaf,
                               std::vector<int>& virtual_conn_clusters);

  // Methods for data flow
  void createDataFlow();
  void dataFlowDFSIOPin(int parent,
                        int idx,
                        std::vector<std::set<odb::dbInst*>>& insts,
                        std::map<int, odb::dbBTerm*>& io_pin_vertex,
                        std::map<int, odb::dbInst*>& std_cell_vertex,
                        std::map<int, odb::dbITerm*>& macro_pin_vertex,
                        std::vector<bool>& stop_flag_vec,
                        std::vector<bool>& visited,
                        std::vector<std::vector<int>>& vertices,
                        std::vector<std::vector<int>>& hyperedges,
                        bool backward_search);
  void dataFlowDFSMacroPin(int parent,
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
                           bool backward_search);

  void printPhysicalHierarchyTree(Cluster* parent, int level);
  float computeMicronArea(odb::dbInst* inst);

  static bool isIgnoredMaster(odb::dbMaster* master);

  odb::dbBlock* block_;
  sta::dbNetwork* network_;
  utl::Logger* logger_;
  par::PartitionMgr* triton_part_;

  Metrics* design_metrics_;
  PhysicalHierarchy* tree_;

  int level_; // Current level
  int id_;    // Current "highest" id

  // Size limits of the current level
  int max_macro_;
  int min_macro_;
  int max_std_cell_;
  int min_std_cell_;

  DataFlow data_flow;

  const float size_tolerance_ = 0.1;
};

}  // namespace mpl2