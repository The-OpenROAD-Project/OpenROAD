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

#include <queue>

#include "object.h"

namespace par {
class PartitionMgr;
}

namespace utl {
class Logger;
}

namespace sta {
class dbNetwork;
}

namespace mpl2 {

using InstToHardMap = std::map<odb::dbInst*, std::unique_ptr<HardMacro>>;
using ModuleToMetricsMap = std::map<odb::dbModule*, std::unique_ptr<Metrics>>;

struct DataFlowHypergraph
{
  std::vector<std::vector<int>> vertices;
  std::vector<std::vector<int>> backward_vertices;
  std::vector<std::vector<int>> hyperedges;
};

// For data flow computation.
struct VerticesMaps
{
  std::map<int, odb::dbBTerm*> id_to_bterm;
  std::map<int, odb::dbInst*> id_to_std_cell;
  std::map<int, odb::dbITerm*> id_to_macro_pin;

  // Each index represents a vertex in which
  // the flow is interrupted.
  std::vector<bool> stoppers;
};

struct DataFlow
{
  bool is_empty{false};

  // Macro Pins --> Registers
  // Registers --> Macro Pins
  std::vector<std::pair<odb::dbITerm*, std::vector<std::set<odb::dbInst*>>>>
      macro_pins_and_regs;

  // IO --> Register
  // Register --> IO
  // IO --> Macro Pin
  // Macro Pin --> IO
  std::vector<std::pair<odb::dbBTerm*, std::vector<std::set<odb::dbInst*>>>>
      io_and_regs;

  // Macro Pin --> Macros
  // Macros --> Macro Pin
  std::vector<std::pair<odb::dbITerm*, std::vector<std::set<odb::dbInst*>>>>
      macro_pins_and_macros;
};

struct PhysicalHierarchyMaps
{
  std::map<int, Cluster*> id_to_cluster;
  std::unordered_map<odb::dbInst*, int> inst_to_cluster_id;
  std::unordered_map<odb::dbBTerm*, int> bterm_to_cluster_id;

  InstToHardMap inst_to_hard;
  ModuleToMetricsMap module_to_metrics;

  // Only for designs with IO Pads
  std::map<odb::dbBTerm*, odb::dbInst*> bterm_to_inst;
};

struct PhysicalHierarchy
{
  std::unique_ptr<Cluster> root;
  PhysicalHierarchyMaps maps;

  float halo_width{0.0f};
  float halo_height{0.0f};
  float macro_with_halo_area{0.0f};

  bool has_io_clusters{true};
  bool has_only_macros{false};
  bool has_std_cells{true};
  bool has_unfixed_macros{true};

  int base_max_macro{0};
  int base_min_macro{0};
  int base_max_std_cell{0};
  int base_min_std_cell{0};

  int max_level{0};
  int bundled_ios_per_edge{0};
  int large_net_threshold{0};  // used to ignore global nets
  int min_net_count_for_connection{0};
  float cluster_size_ratio{0.0f};
  float cluster_size_tolerance{0.0f};

  // Virtual connection weight between each macro cluster
  // and its corresponding standard-cell cluster to bias
  // the macro placer to place them together.
  const float virtual_weight = 10.0f;
};

class ClusteringEngine
{
 public:
  ClusteringEngine(odb::dbBlock* block,
                   sta::dbNetwork* network,
                   utl::Logger* logger,
                   par::PartitionMgr* triton_part);

  void run();

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

  void createTempMacroClusters(const std::vector<HardMacro*>& hard_macros,
                               std::vector<HardMacro>& sa_macros,
                               UniqueClusterVector& macro_clusters,
                               std::map<int, int>& cluster_to_macro,
                               std::set<odb::dbMaster*>& masters);
  void clearTempMacroClusterMapping(const UniqueClusterVector& macro_clusters);

 private:
  using UniqueClusterQueue = std::queue<std::unique_ptr<Cluster>>;

  void init();
  Metrics* computeModuleMetrics(odb::dbModule* module);
  std::vector<odb::dbInst*> getUnfixedMacros();
  float computeMacroWithHaloArea(
      const std::vector<odb::dbInst*>& unfixed_macros);
  void reportDesignData(float core_area);
  void createRoot();
  void setBaseThresholds();
  void createIOClusters();
  void mapIOPads();
  void treatEachMacroAsSingleCluster();
  void incorporateNewCluster(std::unique_ptr<Cluster> cluster, Cluster* parent);
  void setClusterMetrics(Cluster* cluster);
  void multilevelAutocluster(Cluster* parent);
  void updateSizeThresholds();
  void breakCluster(Cluster* parent);
  void createFlatCluster(odb::dbModule* module, Cluster* parent);
  void addModuleLeafInstsToCluster(Cluster* cluster, odb::dbModule* module);
  void createCluster(odb::dbModule* module, Cluster* parent);
  void createCluster(Cluster* parent);
  void updateSubTree(Cluster* parent);
  void breakLargeFlatCluster(Cluster* parent);
  bool partitionerSolutionIsFullyUnbalanced(const std::vector<int>& solution,
                                            int num_other_cluster_vertices);
  void mergeChildrenBelowThresholds(std::vector<Cluster*>& small_children);
  bool attemptMerge(Cluster* receiver, Cluster* incomer);
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
  void addStdCellClusterToSubTree(Cluster* parent,
                                  Cluster* mixed_leaf,
                                  std::vector<int>& virtual_conn_clusters);
  void replaceByStdCellCluster(Cluster* mixed_leaf,
                               std::vector<int>& virtual_conn_clusters);

  // Methods for data flow
  void createDataFlow();
  bool stdCellsHaveLiberty();
  VerticesMaps computeVertices();
  void computeIOVertices(VerticesMaps& vertices_maps);
  void computeStdCellVertices(VerticesMaps& vertices_maps);
  void computeMacroPinVertices(VerticesMaps& vertices_maps);
  DataFlowHypergraph computeHypergraph(int num_of_vertices);
  void dataFlowDFSIOPin(int parent,
                        int idx,
                        const VerticesMaps& vertices_maps,
                        const DataFlowHypergraph& hypergraph,
                        std::vector<std::set<odb::dbInst*>>& insts,
                        std::vector<bool>& visited,
                        bool backward_search);
  void dataFlowDFSMacroPin(int parent,
                           int idx,
                           const VerticesMaps& vertices_maps,
                           const DataFlowHypergraph& hypergraph,
                           std::vector<std::set<odb::dbInst*>>& std_cells,
                           std::vector<std::set<odb::dbInst*>>& macros,
                           std::vector<bool>& visited,
                           bool backward_search);
  std::set<int> computeSinks(const std::set<odb::dbInst*>& insts);
  float computeConnWeight(int hops);

  void printPhysicalHierarchyTree(Cluster* parent, int level);
  float computeMicronArea(odb::dbInst* inst);

  static bool isIgnoredMaster(odb::dbMaster* master);

  odb::dbBlock* block_;
  sta::dbNetwork* network_;
  utl::Logger* logger_;
  par::PartitionMgr* triton_part_;

  Metrics* design_metrics_{nullptr};
  PhysicalHierarchy* tree_{nullptr};

  int level_{0};  // Current level
  int id_{0};     // Current "highest" id

  // Size limits of the current level
  int max_macro_{0};
  int min_macro_{0};
  int max_std_cell_{0};
  int min_std_cell_{0};
  const float size_tolerance_ = 0.1;

  // Variables for data flow
  DataFlow data_connections_;

  // The register distance between two macros for
  // them to be considered connected when creating data flow.
  const int max_num_of_hops_ = 5;
};

}  // namespace mpl2
