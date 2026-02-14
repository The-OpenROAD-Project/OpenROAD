// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace par {
class PartitionMgr;
}

namespace utl {
class Logger;
}

namespace sta {
class dbNetwork;
}

namespace mpl {
class MplObserver;

using InstToHardMap = std::map<odb::dbInst*, std::unique_ptr<HardMacro>>;
using ModuleToMetricsMap = std::map<odb::dbModule*, std::unique_ptr<Metrics>>;
using PathInsts = std::vector<std::set<odb::dbInst*>>;

struct PhysicalHierarchyMaps
{
  std::map<int, Cluster*> id_to_cluster;
  std::unordered_map<odb::dbInst*, int> inst_to_cluster_id;
  std::unordered_map<odb::dbBTerm*, int> bterm_to_cluster_id;

  InstToHardMap inst_to_hard;
  ModuleToMetricsMap module_to_metrics;
};

struct PhysicalHierarchy
{
  std::unique_ptr<Cluster> root;
  std::vector<odb::dbInst*> io_pads;
  PhysicalHierarchyMaps maps;

  BoundaryRegionList available_regions_for_unconstrained_pins;
  ClusterToBoundaryRegionMap io_cluster_to_constraint;

  HardMacro::Halo default_halo;
  int64_t macro_with_halo_area{0};

  // The constraint set by the user.
  odb::Rect global_fence;

  // The actual area used by MPL - computed using the dimensions
  // of the core versus the global fence set by the user.
  odb::Rect floorplan_shape;
  odb::Rect die_area;

  bool has_io_clusters{true};
  bool has_only_macros{false};
  bool has_std_cells{true};
  bool has_unfixed_macros{true};
  bool has_fixed_macros{false};

  int base_max_macro{0};
  int base_min_macro{0};
  int base_max_std_cell{0};
  int base_min_std_cell{0};

  int max_level{0};
  int large_net_threshold{0};  // used to ignore global nets
  float cluster_size_ratio{0.0f};
  float cluster_size_tolerance{0.0f};

  const int io_bundles_per_edge = 5;
};

struct IOBundleSpans
{
  int x{0};
  int y{0};
};

class ClusteringEngine
{
 public:
  ClusteringEngine(odb::dbBlock* block,
                   sta::dbNetwork* network,
                   utl::Logger* logger,
                   par::PartitionMgr* triton_part,
                   MplObserver* graphics);

  void run();

  void setTree(PhysicalHierarchy* tree);
  void setHalos(std::map<odb::dbInst*, HardMacro::Halo>& macro_to_halo);

  // Methods to update the tree as the hierarchical
  // macro placement runs.
  void rebuildConnections();
  void updateInstancesAssociation(Cluster* cluster);
  void updateInstancesAssociation(odb::dbModule* module,
                                  int cluster_id,
                                  bool include_macro);

  std::string generateMacroAndCoreDimensionsTable(const HardMacro* hard_macro,
                                                  const odb::Rect& core) const;
  void createTempMacroClusters(const std::vector<HardMacro*>& hard_macros,
                               std::vector<HardMacro>& sa_macros,
                               UniqueClusterVector& macro_clusters,
                               std::map<int, int>& cluster_to_macro,
                               std::set<odb::dbMaster*>& masters);
  void clearTempMacroClusterMapping(const UniqueClusterVector& macro_clusters);

  int getNumberOfIOs(Cluster* target) const;

  bool isIgnoredInst(odb::dbInst* inst);

 private:
  using UniqueClusterQueue = std::queue<std::unique_ptr<Cluster>>;

  struct Net
  {
    int driver_id{-1};
    std::vector<int> loads_ids;
  };

  void init();
  Metrics* computeModuleMetrics(odb::dbModule* module);
  std::vector<odb::dbInst*> getUnfixedMacros();
  void addIgnorableMacro(odb::dbInst* inst);
  void setDieArea();
  void setFloorplanShape();
  void createHardMacros();
  bool movableCellsFitInMacroPlacementArea() const;
  int64_t computeMacroWithHaloArea(
      const std::vector<odb::dbInst*>& unfixed_macros);
  std::vector<odb::dbInst*> getIOPads() const;
  void reportDesignData();
  void createRoot();
  void setBaseThresholds();
  void createIOClusters();
  bool designHasFixedIOPins() const;
  IOBundleSpans computeIOBundleSpans() const;
  void createIOBundles();
  void createIOBundles(Boundary boundary);
  void createIOBundle(Boundary boundary, int bundle_index);
  int findAssociatedBundledIOId(odb::dbBTerm* bterm) const;
  Cluster* findIOClusterWithSameConstraint(odb::dbBTerm* bterm) const;
  void createClusterOfUnplacedIOs(odb::dbBTerm* bterm);
  void createIOPadClusters();
  void createIOPadCluster(odb::dbInst* pad);
  void treatEachMacroAsSingleCluster();
  void incorporateNewCluster(std::unique_ptr<Cluster> cluster, Cluster* parent);
  void setClusterMetrics(Cluster* cluster);
  void multilevelAutocluster(Cluster* parent);
  void reportThresholds() const;
  void updateSizeThresholds();
  void breakCluster(Cluster* parent);
  void createFlatCluster(odb::dbModule* module, Cluster* parent);
  void addModuleLeafInstsToCluster(Cluster* cluster, odb::dbModule* module);
  void createCluster(odb::dbModule* module, Cluster* parent);
  void createCluster(Cluster* parent);
  void updateSubTree(Cluster* parent);
  bool isLargeFlatCluster(const Cluster* cluster) const;
  void breakLargeFlatCluster(Cluster* parent);
  bool partitionerSolutionIsFullyUnbalanced(const std::vector<int>& solution,
                                            int num_other_cluster_vertices);
  void mergeChildrenBelowThresholds(std::vector<Cluster*>& small_children);
  bool mergeHonorsMaxThresholds(const Cluster* a, const Cluster* b) const;
  bool sameConnectionSignature(Cluster* a, Cluster* b) const;
  bool strongConnection(Cluster* a,
                        Cluster* b,
                        const float* connection_weight = nullptr) const;
  Cluster* findSingleWellFormedConnectedCluster(
      Cluster* target_cluster,
      const std::vector<int>& small_clusters_id_list) const;
  std::vector<int> findNeighbors(Cluster* target_cluster,
                                 Cluster* ignored_cluster) const;
  bool attemptMerge(Cluster* receiver, Cluster* incomer);
  void fetchMixedLeaves(Cluster* parent,
                        std::vector<std::vector<Cluster*>>& mixed_leaves);
  void breakMixedLeaves(const std::vector<std::vector<Cluster*>>& mixed_leaves);
  void breakMixedLeaf(Cluster* mixed_leaf);
  void mapMacroInCluster2HardMacro(Cluster* cluster);
  void getHardMacros(odb::dbModule* module,
                     std::vector<HardMacro*>& hard_macros);
  void createOneClusterForEachMacro(Cluster* mixed_leaf_parent,
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
  void replaceByStdCellCluster(Cluster* mixed_leaf,
                               std::vector<int>& virtual_conn_clusters);

  void clearConnections();
  void buildNetListConnections();
  Net buildNet(odb::dbNet* db_net) const;
  void connectClusters(const Net& net);
  void connect(Cluster* a, Cluster* b, float connection_weight) const;

  void printPhysicalHierarchyTree(Cluster* parent, int level);
  int64_t computeArea(odb::dbInst* inst);

  bool isValidNet(odb::dbNet* net);

  odb::dbBlock* block_;
  sta::dbNetwork* network_;
  utl::Logger* logger_;
  par::PartitionMgr* triton_part_;
  MplObserver* graphics_;

  Metrics* design_metrics_{nullptr};
  PhysicalHierarchy* tree_{nullptr};

  // Keep this pointer to avoid searching for it when creating IO clusters.
  Cluster* cluster_of_unconstrained_io_pins_{nullptr};

  int level_{0};  // Current level
  int id_{0};     // Current "highest" id

  // Size limits of the current level
  int max_macro_{0};
  int min_macro_{0};
  int max_std_cell_{0};
  int min_std_cell_{0};

  const float minimum_connection_ratio_{0.08};

  int first_io_bundle_id_{-1};
  IOBundleSpans io_bundle_spans_;

  std::unordered_set<odb::dbInst*> ignorable_macros_;
  std::map<odb::dbInst*, HardMacro::Halo> macro_to_halo_;
};

}  // namespace mpl
