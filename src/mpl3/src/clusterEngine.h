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

namespace mpl3 {

using InstToHardMap = std::map<odb::dbInst*, std::unique_ptr<HardMacro>>;
using ModuleToMetricsMap = std::map<Cluster*, std::unique_ptr<Metrics>>;

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

class LogicalHierarchy
{
 private:
  std::unique_ptr<Cluster> root;
  std::unordered_map<odb::dbInst*, Cluster*> inst_to_logical_module;
  utl::Logger* logger_{nullptr};
  int id_{0};
 public:
  LogicalHierarchy(utl::Logger* logger) : logger_(logger) {}
  
  void buildLogicalHierarchyDFS(odb::dbModule* top_module, Cluster* root) {
    for (odb::dbModInst* child_mod_inst : top_module->getChildren()) {
      odb::dbModule* child_module = child_mod_inst->getMaster();
      std::unique_ptr<Cluster> cluster = std::make_unique<Cluster>(++id_, child_module->getHierarchicalName(), logger_);
      cluster->setParent(root);
      buildLogicalHierarchyDFS(child_module, cluster.get());
      root->addChild(std::move(cluster));
    }
    for (auto inst : top_module->getInsts()) {
      root->addLeafInst(inst);
    }
    updateInstsCorresponse(root);
  };

  Cluster* getModule(odb::dbInst* inst)
  {
    auto it = inst_to_logical_module.find(inst);
    if (it != inst_to_logical_module.end()) {
      return it->second;
    }
    return nullptr;
  }
  Cluster* getRoot() { return root.get(); }
  size_t getRecursiveClusterStdCellsSize(Cluster* cluster)
  {
    size_t size = cluster->getNumLeafStdCells();
    for (auto& child : cluster->getChildren()) {
      size += getRecursiveClusterStdCellsSize(child.get());
    }
    return size;
  }
  size_t getRecursiveClusterMacrosSize(Cluster* cluster)
  {
    size_t size = cluster->getNumLeafMacros();
    for (auto& child : cluster->getChildren()) {
      size += getRecursiveClusterMacrosSize(child.get());
    }
    return size;
  }
  void setRoot(std::unique_ptr<Cluster> root) { this->root = std::move(root); }
  void updateInstsCorresponse(Cluster* cluster)
  {
    for (auto inst : cluster->getLeafStdCells()) {
      inst_to_logical_module[inst] = cluster;
    }
    for (auto inst : cluster->getLeafMacros()) {
      inst_to_logical_module[inst] = cluster;
    }
  }

  mapModule2InstVec findModulesForCluster(std::vector<odb::dbInst*>& cluster)
  {
    mapModule2InstVec map_module2inst_vec;
    for (auto inst : cluster) {
      Cluster* module = getModule(inst);
      auto it = map_module2inst_vec.find(module);
      if (it == map_module2inst_vec.end()) {
        map_module2inst_vec[module] = {inst};
      } else {
        it->second.push_back(inst);
      }
    }
    return map_module2inst_vec;
  }

  Cluster* findLCA(Cluster* module1, Cluster* module2)
  {
    std::set<Cluster*> ancestors;
    while (module1) {
      ancestors.insert(module1);
      module1 = module1->getParent();
    }
    while (module2) {
      if (ancestors.find(module2) != ancestors.end()) {
        return module2;
      }
      module2 = module2->getParent();
    }
    return module2;
  }

  Cluster* findLCAForModules(std::set<Cluster*>& modules)
  {
    if (modules.empty()) {
      return nullptr;
    }
    auto it = modules.begin();
    Cluster* lca = *it;
    ++it;
    for (; it != modules.end(); ++it) {
      lca = findLCA(lca, *it);
    }
    return lca;
  }

  Cluster* createChildModuleForModule(Cluster* parent_module,
                                      std::vector<odb::dbInst*>& sub_cluster)
  {
    std::string module_name
        = fmt::format("{}_{}", parent_module->getName(), ++id_);
    std::unique_ptr<Cluster> new_module
        = std::make_unique<Cluster>(id_, module_name, logger_);
    for (auto inst : sub_cluster) {
      // remove the instances from the parent module
      parent_module->removeLeafInst(inst);
      new_module->addLeafInst(inst);
    }
    Cluster* new_module_ptr = new_module.get();
    updateInstsCorresponse(new_module_ptr);
    new_module_ptr->setParent(parent_module);
    parent_module->addChild(std::move(new_module));
    return new_module_ptr;
  }

  void mergeModulesAsChild(Cluster* parent_module,
                               std::set<Cluster*>& modules)
  {
    for (auto candidate : modules) {
      // merge child module and leaf instances
      if (candidate != parent_module) {
        std::unique_ptr<Cluster> module = candidate->getParent()->releaseChild(candidate);
        std::vector<Cluster*> children;
        for (auto& child_module : module->getChildren()) {
          children.push_back(child_module.get());
        }
        for (auto child_module : children) {
          Cluster* child_cur_parent = child_module->getParent();
          child_module->setParent(parent_module);
          parent_module->addChild(std::move(child_cur_parent->releaseChild(child_module)));
        }
        for (auto inst : module->getLeafStdCells()) {
          parent_module->addLeafInst(inst);
        }
        for (auto inst : module->getLeafMacros()) {
          parent_module->addLeafInst(inst);
        }
        module->clearLeafStdCells();
        module->clearLeafMacros();
      }
    }
    updateInstsCorresponse(parent_module);
  }

  void printLogicalHierarchyTree(Cluster* parent, int level)
  {
    std::string line;
    for (int i = 0; i < level; i++) {
      line += "+---";
    }
    line += fmt::format(
        "{}  ({})  num_macro :  {}   num_std_cell :  {}"
        " cluster type: {} {}",
        parent->getName(),
        parent->getId(),
        getRecursiveClusterMacrosSize(parent),
        getRecursiveClusterStdCellsSize(parent),
        parent->getIsLeafString(),
        parent->getClusterTypeString());
    logger_->report("{}", line);

    for (auto& child : parent->getChildren()) {
      printLogicalHierarchyTree(child.get(), level + 1);
    }
  }
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
  void setIteration(int iteration){iteration_ = iteration;}
  // Methods to update the tree as the hierarchical
  // macro placement runs.
  void updateConnections();
  void updateDataFlow();
  void updateInstancesAssociation(Cluster* cluster);
  void updateInstancesAssociation(Cluster* module,
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
  Metrics* computeModuleMetrics(Cluster* module);
  std::vector<odb::dbInst*> getUnfixedMacros();
  float computeMacroWithHaloArea(
      const std::vector<odb::dbInst*>& unfixed_macros);
  void reportDesignData(float core_area);
  void buildLogicalHierarchy();
  Cluster* refineLogicalHierarchy();
  void createRoot();
  void setBaseThresholds();
  void createIOClusters();
  void mapIOPads();
  void treatEachMacroAsSingleCluster();
  void createSubLeindenCluster(Cluster* parent, std::vector<int> &partition);
  void mergeLeidenClustering(Cluster* parent, std::vector<int> &partition, std::vector<std::vector<Cluster*>>& mixed_leaves);
  void incorporateNewCluster(std::unique_ptr<Cluster> cluster, Cluster* parent);
  void setClusterMetrics(Cluster* cluster);
  void multilevelAutocluster(Cluster* parent);
  void updateSizeThresholds();
  void breakCluster(Cluster* parent);
  void createFlatCluster(Cluster* module, Cluster* parent);
  void addModuleLeafInstsToCluster(Cluster* cluster, Cluster* module);
  void createCluster(Cluster* module, Cluster* parent);
  void createCluster(Cluster* parent);
  void updateSubTree(Cluster* parent);
  void breakLargeFlatCluster(Cluster* parent);
  void mergeChildrenBelowThresholds(std::vector<Cluster*>& small_children);
  bool attemptMerge(Cluster* receiver, Cluster* incomer);
  void fetchMixedLeaves(Cluster* parent,
                        std::vector<std::vector<Cluster*>>& mixed_leaves);
  void breakMixedLeaves(const std::vector<std::vector<Cluster*>>& mixed_leaves);
  void breakMixedLeaf(Cluster* mixed_leaf);
  void mapMacroInCluster2HardMacro(Cluster* cluster);
  void getHardMacros(Cluster* module,
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
  Cluster* top_module_{nullptr};
  Metrics* design_metrics_{nullptr};
  PhysicalHierarchy* tree_{nullptr};
  std::unique_ptr<LogicalHierarchy> logical_tree_{nullptr};

  int level_{0};  // Current level
  int id_{0};     // Current "highest" id

  // Size limits of the current level
  int max_macro_{0};
  int min_macro_{0};
  int max_std_cell_{0};
  int min_std_cell_{0};
  const float size_tolerance_ = 0.1;
  int iteration_{10};
  // Variables for data flow
  DataFlow data_connections_;

  // The register distance between two macros for
  // them to be considered connected when creating data flow.
  const int max_num_of_hops_ = 5;
};

}  // namespace mpl3
