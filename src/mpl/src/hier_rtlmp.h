// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "MplObserver.h"
#include "clusterEngine.h"
#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "shapes.h"

namespace sta {
class dbNetwork;
class dbSta;
}  // namespace sta

namespace utl {
class Logger;
}

namespace par {
class PartitionMgr;
}

namespace mpl {
struct BundledNet;
class Cluster;
class HardMacro;
class SoftMacro;
class Snapper;
class SACoreSoftMacro;
class SACoreHardMacro;

using BoundaryToRegionsMap = std::map<Boundary, std::queue<odb::Rect>>;
using SoftMacroNameToIdMap = std::map<std::string, int>;
using ClusterToMacroMap = std::map<int, int>;  // cluster_id -> macro_id

// The parameters necessary to compute one coordinate of the new
// origin for aligning the macros' pins to the track-grid
struct PatternParameters
{
  odb::dbITerm* iterm;
  int offset = 0;
  int pitch = 0;
  int pin_offset = 0;
};

// Hierarchical RTL-MP
// Support Multi-Level Clustering.
// Support designs with IO Pads.
// Support timing-driven macro placement (not enabled now, June, 2022)
// Identify RTL information based on logical hierarchy, connection signature and
// dataflow. Connection Signature is defined as the connection topology respect
// to other clusters. Dataflow is defined based on sequential graph.
// Timing-driven macro placement is based on uniform delay model.
class HierRTLMP
{
 public:
  HierRTLMP(sta::dbNetwork* network,
            odb::dbDatabase* db,
            utl::Logger* logger,
            par::PartitionMgr* tritonpart);
  ~HierRTLMP();

  void init();
  void run();

  // Interfaces functions for setting options
  // Hierarchical Macro Placement Related Options
  void setGlobalFence(odb::Rect global_fence);
  void setDefaultHalo(int halo_width, int halo_height);
  void setGuidanceRegions(
      const std::map<odb::dbInst*, odb::Rect>& guidance_regions);
  void setMacroHalo(odb::dbInst* macro, int halo_width, int halo_height);

  // Clustering Related Options
  void setClusterSize(int max_num_macro,
                      int min_num_macro,
                      int max_num_inst,
                      int min_num_inst);
  void setClusterSizeTolerance(float tolerance);
  void setMaxNumLevel(int max_num_level);
  void setClusterSizeRatioPerLevel(float coarsening_ratio);
  void setLargeNetThreshold(int large_net_threshold);
  void setAreaWeight(float area_weight);
  void setOutlineWeight(float outline_weight);
  void setWirelengthWeight(float wirelength_weight);
  void setGuidanceWeight(float guidance_weight);
  void setFenceWeight(float fence_weight);
  void setBoundaryWeight(float boundary_weight);
  void setNotchWeight(float notch_weight);
  void setMacroBlockageWeight(float macro_blockage_weight);
  void setTargetUtil(float target_util);
  void setMinAR(float min_ar);
  void setReportDirectory(const char* report_directory);
  void setKeepClusteringData(bool keep_clustering_data);

  void setDebug(std::unique_ptr<MplObserver>& graphics);
  void setDebugShowBundledNets(bool show_bundled_nets);
  void setDebugShowClustersIds(bool show_clusters_ids);
  void setDebugSkipSteps(bool skip_steps);
  void setDebugOnlyFinalResult(bool only_final_result);
  void setDebugTargetClusterId(int target_cluster_id);

  void setNumThreads(int threads) { num_threads_ = threads; }
  void setMacroPlacementFile(const std::string& file_name);
  void writeMacroPlacement(const std::string& file_name);

 private:
  struct PinAccessDepthLimits
  {
    Interval x;
    Interval y;
  };

  using SoftSAVector = std::vector<std::unique_ptr<SACoreSoftMacro>>;
  using HardSAVector = std::vector<std::unique_ptr<SACoreHardMacro>>;

  void runMultilevelAutoclustering();
  void runHierarchicalMacroPlacement();

  void resetSAParameters();

  void updateMacrosOnDb();
  void updateMacroOnDb(const HardMacro* hard_macro);
  void commitMacroPlacementToDb();
  void commitClusteringDataToDb() const;
  void createGroupForCluster(Cluster* cluster,
                             odb::dbGroup* parent_group) const;
  void clear();
  void computeWireLength() const;

  // Coarse Shaping
  void runCoarseShaping();
  void setRootShapes();
  void calculateChildrenTilings(Cluster* parent);
  void calculateMacroTilings(Cluster* cluster);
  IntervalList computeWidthIntervals(const TilingList& tilings);
  TilingList generateTilingsForMacroCluster(int macro_width,
                                            int macro_height,
                                            int number_of_macros);
  void searchAvailableRegionsForUnconstrainedPins();
  BoundaryToRegionsMap getBoundaryToBlockedRegionsMap(
      const std::vector<odb::Rect>& blocked_regions_for_pins) const;
  std::vector<odb::Rect> computeAvailableRegions(
      BoundaryToRegionsMap& boundary_to_blocked_regions) const;
  void createPinAccessBlockages();
  void computePinAccessDepthLimits();
  bool treeHasConstrainedIOs() const;
  bool treeHasUnconstrainedIOs() const;
  std::vector<Cluster*> getClustersOfUnplacedIOPins() const;
  std::vector<Cluster*> getIOBundles() const;
  void createPinAccessBlockage(const BoundaryRegion& region, int depth);
  int computePinAccessBaseDepth(int io_span) const;
  void createBlockagesForIOBundles();
  void createBlockagesForAvailableRegions();
  void createBlockagesForConstraintRegions();
  void setPlacementBlockages();

  // Fine Shaping
  int computeTinyClusterMaxNumberOfStdCells() const;
  bool singleArraySingleStdCellCluster(
      const std::vector<SoftMacro>& soft_macros) const;
  void setMacroClustersShapes(std::vector<SoftMacro>& soft_macros) const;
  std::vector<float> computeUtilizationList(float total_number_of_runs) const;
  bool validUtilization(float utilization,
                        const odb::Rect& outline,
                        const std::vector<SoftMacro>& soft_macros) const;
  std::vector<SoftMacro> applyUtilization(
      float utilization,
      const odb::Rect& outline,
      const std::vector<SoftMacro>& original_soft_macros) const;

  // Hierarchical Macro Placement 1st stage: Cluster Placement
  void adjustMacroBlockageWeight();
  void placeChildren(Cluster* parent);

  std::vector<odb::Rect> findBlockagesWithinOutline(
      const odb::Rect& outline) const;
  void getBlockageRegionWithinOutline(
      std::vector<odb::Rect>& blockages_within_outline,
      const odb::Rect& blockage,
      const odb::Rect& outline) const;
  void eliminateOverlaps(std::vector<odb::Rect>& blockages) const;
  void createSoftMacrosForBlockages(const std::vector<odb::Rect>& blockages,
                                    std::vector<SoftMacro>& macros);
  void createFixedTerminals(Cluster* parent,
                            std::map<std::string, int>& soft_macro_id_map,
                            std::vector<SoftMacro>& soft_macros);
  void updateChildrenShapesAndLocations(
      Cluster* parent,
      const std::vector<SoftMacro>& shaped_macros,
      const std::map<std::string, int>& soft_macro_id_map);
  void updateChildrenRealLocation(Cluster* parent,
                                  float offset_x,
                                  float offset_y);
  void mergeNets(BundledNetList& nets);

  // Hierarchical Macro Placement 2nd stage: Macro Placement
  void placeMacros(Cluster* cluster);
  void computeFencesAndGuides(const std::vector<HardMacro*>& hard_macros,
                              const odb::Rect& outline,
                              std::map<int, odb::Rect>& fences,
                              std::map<int, odb::Rect>& guides);
  void createFixedTerminals(const odb::Rect& outline,
                            const UniqueClusterVector& macro_clusters,
                            std::map<int, int>& cluster_to_macro,
                            std::vector<HardMacro>& sa_macros);
  // For cluster placement.
  BundledNetList buildBundledNets(
      Cluster* parent,
      const SoftMacroNameToIdMap& soft_macro_id_map) const;
  // For macro placement.
  BundledNetList buildBundledNets(
      const UniqueClusterVector& clusters,
      const ClusterToMacroMap& cluster_to_macro) const;
  SequencePair computeArraySequencePair(Cluster* cluster,
                                        bool& array_has_empty_space);

  // Orientation Improvement
  void generateTemporaryStdCellsPlacement(Cluster* cluster);
  void setModuleStdCellsLocation(Cluster* cluster, odb::dbModule* module);
  void setTemporaryStdCellLocation(Cluster* cluster, odb::dbInst* std_cell);

  void correctAllMacrosOrientation();
  float calculateRealMacroWirelength(odb::dbInst* macro);
  void adjustRealMacroOrientation(const bool& is_vertical_flip);
  void flipRealMacro(odb::dbInst* macro, const bool& is_vertical_flip);

  template <typename Macro>
  void createFixedTerminal(Cluster* cluster,
                           const odb::Rect& outline,
                           std::vector<Macro>& macros);

  odb::Rect getRect(Boundary boundary) const;

  std::vector<odb::Rect> subtractOverlapRegion(const odb::Rect& base,
                                               const odb::Rect& overlay) const;

  // For debugging
  void reportShapeCurves(const std::vector<SoftMacro>& soft_macros) const;
  template <typename SACore>
  void printPlacementResult(Cluster* parent,
                            const odb::Rect& outline,
                            SACore* sa_core);
  void writeNetFile(const std::string& file_name_prefix,
                    std::vector<SoftMacro>& macros,
                    BundledNetList& nets);
  void writeFloorplanFile(const std::string& file_name_prefix,
                          std::vector<SoftMacro>& macros);
  template <typename SACore>
  void writeCostFile(const std::string& file_name_prefix, SACore* sa_core);

  sta::dbNetwork* network_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  utl::Logger* logger_ = nullptr;
  par::PartitionMgr* tritonpart_ = nullptr;
  std::unique_ptr<PhysicalHierarchy> tree_;

  std::unique_ptr<ClusteringEngine> clustering_engine_;

  // Parameters related to macro placement
  std::string report_directory_;
  std::string macro_placement_file_;

  const int num_runs_ = 10;    // number of runs for SA
  int num_threads_ = 10;       // number of threads
  const int random_seed_ = 0;  // random seed for deterministic

  float target_utilization_{0.0};
  float min_ar_ = 0.3;  // the aspect ratio range for StdCellCluster (min_ar_, 1
                        // / min_ar_)

  float notch_v_th_ = 10.0;
  float notch_h_th_ = 10.0;

  SACoreWeights placement_core_weights_;  // For cluster and macro placement.
  SASoftWeights cluster_placement_weights_;

  // For generation of the coarse shape (tiling) of clusters with macros.
  const SACoreWeights shaping_core_weights_{1.0f /* area */,
                                            1000.0f /* outline */,
                                            0.0f /* wirelength */,
                                            0.0f /* guidance */,
                                            0.0f /* fence */};

  std::map<std::string, odb::Rect> fences_;   // macro_name, fence
  std::map<odb::dbInst*, odb::Rect> guides_;  // Macro -> Guidance Region
  std::map<odb::dbInst*, HardMacro::Halo> macro_to_halo_;
  std::vector<odb::Rect> placement_blockages_;
  std::vector<odb::Rect> io_blockages_;

  PinAccessDepthLimits pin_access_depth_limits_;
  float tiny_cluster_max_number_of_std_cells_{0};

  // Fast SA hyperparameter
  float init_prob_ = 0.9;
  const int max_num_step_ = 2000;
  const int num_perturb_per_step_ = 500;

  // probability of each action
  float pos_swap_prob_ = 0.2;
  float neg_swap_prob_ = 0.2;
  float double_swap_prob_ = 0.2;
  float exchange_swap_prob_ = 0.2;
  float resize_prob_ = 0.4;

  bool skip_macro_placement_ = false;
  bool keep_clustering_data_{false};

  std::unique_ptr<MplObserver> graphics_;
  bool is_debug_only_final_result_{false};
};

class Pusher
{
 public:
  Pusher(utl::Logger* logger,
         Cluster* root,
         odb::dbBlock* block,
         const std::vector<odb::Rect>& io_blockages);

  void pushMacrosToCoreBoundaries();

 private:
  void setIOBlockages(const std::vector<odb::Rect>& io_blockages);
  bool designHasSingleCentralizedMacroArray();
  void pushMacroClusterToCoreBoundaries(
      Cluster* macro_cluster,
      const std::map<Boundary, int>& boundaries_distance);
  void fetchMacroClusters(Cluster* parent,
                          std::vector<Cluster*>& macro_clusters);
  std::map<Boundary, int> getDistanceToCloseBoundaries(Cluster* macro_cluster);
  void moveHardMacro(HardMacro* hard_macro, Boundary boundary, int distance);
  void moveMacroClusterBox(odb::Rect& cluster_box,
                           Boundary boundary,
                           int distance);
  bool overlapsWithHardMacro(
      const odb::Rect& cluster_box,
      const std::vector<HardMacro*>& cluster_hard_macros);
  bool overlapsWithIOBlockage(const odb::Rect& cluster_box) const;

  utl::Logger* logger_;

  Cluster* root_;
  odb::dbBlock* block_;
  odb::Rect core_;

  std::vector<odb::Rect> io_blockages_;
  std::vector<HardMacro*> hard_macros_;
};

class Snapper
{
 public:
  Snapper(utl::Logger* logger);
  Snapper(utl::Logger* logger, odb::dbInst* inst);

  void setMacro(odb::dbInst* inst) { inst_ = inst; }
  void snapMacro();

 private:
  struct LayerData
  {
    odb::dbTrackGrid* track_grid;
    std::vector<int> available_positions;
    // ordered by pin centers
    std::vector<odb::dbITerm*> pins;
  };
  // ordered by TrackGrid layer number
  using LayerDataList = std::vector<LayerData>;
  using TrackGridToPinListMap
      = std::map<odb::dbTrackGrid*, std::vector<odb::dbITerm*>>;

  void snap(const odb::dbTechLayerDir& target_direction);
  void alignWithManufacturingGrid(int& origin);
  void setOrigin(int origin, const odb::dbTechLayerDir& target_direction);
  int totalAlignedPins(const LayerDataList& layers_data_list,
                       const odb::dbTechLayerDir& direction,
                       bool error_unaligned_right_way_on_grid = false);

  LayerDataList computeLayerDataList(
      const odb::dbTechLayerDir& target_direction);
  odb::dbTechLayer* getPinLayer(odb::dbMPin* pin);
  void getTrackGridPattern(odb::dbTrackGrid* track_grid,
                           int pattern_idx,
                           int& origin,
                           int& step,
                           const odb::dbTechLayerDir& target_direction);
  int getPinOffset(odb::dbITerm* pin, const odb::dbTechLayerDir& direction);
  void snapPinToPosition(odb::dbITerm* pin,
                         int position,
                         const odb::dbTechLayerDir& direction);
  void attemptSnapToExtraPatterns(int start_index,
                                  const LayerDataList& layers_data_list,
                                  const odb::dbTechLayerDir& target_direction);

  utl::Logger* logger_;
  odb::dbInst* inst_;
};

}  // namespace mpl
