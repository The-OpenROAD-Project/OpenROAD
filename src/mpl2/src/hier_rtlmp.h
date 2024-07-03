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

#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "Mpl2Observer.h"

namespace odb {
class dbBTerm;
class dbBlock;
class dbDatabase;
class dbITerm;
class dbInst;
class dbMaster;
class dbModule;
}  // namespace odb

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

namespace mpl2 {
struct BundledNet;
class Cluster;
class HardMacro;
class Metrics;
struct Rect;
class SoftMacro;
class Snapper;

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
            sta::dbSta* sta,
            utl::Logger* logger,
            par::PartitionMgr* tritonpart);
  ~HierRTLMP();

  void run();

  // Interfaces functions for setting options
  // Hierarchical Macro Placement Related Options
  void setGlobalFence(float fence_lx,
                      float fence_ly,
                      float fence_ux,
                      float fence_uy);
  void setHaloWidth(float halo_width);
  void setHaloHeight(float halo_height);

  // Hierarchical Clustering Related Options
  void setNumBundledIOsPerBoundary(int num_bundled_ios);
  void setClusterSize(int max_num_macro,
                      int min_num_macro,
                      int max_num_inst,
                      int min_num_inst);
  void setClusterSizeTolerance(float tolerance);
  void setMaxNumLevel(int max_num_level);
  void setClusterSizeRatioPerLevel(float coarsening_ratio);
  void setLargeNetThreshold(int large_net_threshold);
  void setSignatureNetThreshold(int signature_net_threshold);
  void setAreaWeight(float area_weight);
  void setOutlineWeight(float outline_weight);
  void setWirelengthWeight(float wirelength_weight);
  void setGuidanceWeight(float guidance_weight);
  void setFenceWeight(float fence_weight);
  void setBoundaryWeight(float boundary_weight);
  void setNotchWeight(float notch_weight);
  void setMacroBlockageWeight(float macro_blockage_weight);
  void setPinAccessThreshold(float pin_access_th);
  void setTargetUtil(float target_util);
  void setTargetDeadSpace(float target_dead_space);
  void setMinAR(float min_ar);
  void setSnapLayer(int snap_layer);
  void setReportDirectory(const char* report_directory);
  void setDebug(std::unique_ptr<Mpl2Observer>& graphics);
  void setDebugShowBundledNets(bool show_bundled_nets);
  void setDebugSkipSteps(bool skip_steps);
  void setDebugOnlyFinalResult(bool only_final_result);
  void setBusPlanningOn(bool bus_planning_on);

  void setNumThreads(int threads) { num_threads_ = threads; }
  void setMacroPlacementFile(const std::string& file_name);
  void writeMacroPlacement(const std::string& file_name);

 private:
  using IOSpans = std::map<Boundary, std::pair<float, float>>;

  // General Hier-RTLMP flow functions
  void initMacroPlacer();
  void computeMetricsForModules(float core_area);
  void reportLogicalHierarchyInformation(float core_area,
                                         float util,
                                         float core_util);
  void updateDataFlow();
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
                        bool backward_flag);
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
                           bool backward_flag);
  Metrics* computeMetrics(odb::dbModule* module);
  float computeMicronArea(odb::dbInst* inst);
  void setClusterMetrics(Cluster* cluster);
  void calculateConnection();
  void getHardMacros(odb::dbModule* module,
                     std::vector<HardMacro*>& hard_macros);
  void updateMacrosOnDb();
  void updateMacroOnDb(const HardMacro* hard_macro);
  void commitMacroPlacementToDb();
  void clear();

  void printConnection();
  void printClusters();
  void FDPlacement(std::vector<Rect>& blocks,
                   const std::vector<BundledNet>& nets,
                   float outline_width,
                   float outline_height,
                   const std::string& file_name);

  // Multilevel Autoclustering
  void runMultilevelAutoclustering();
  void initPhysicalHierarchy();
  void setDefaultThresholds();
  void createIOClusters();
  void mapIOPads();
  void createDataFlow();
  void treatEachMacroAsSingleCluster();
  void resetSAParameters();
  void multilevelAutocluster(Cluster* parent);
  void updateSizeThresholds();
  void printPhysicalHierarchyTree(Cluster* parent, int level);
  void updateInstancesAssociation(Cluster* cluster);
  void updateInstancesAssociation(odb::dbModule* module,
                                  int cluster_id,
                                  bool include_macro);
  void breakCluster(Cluster* parent);
  void createFlatCluster(odb::dbModule* module, Cluster* parent);
  void createCluster(Cluster* parent);
  void createCluster(odb::dbModule* module, Cluster* parent);
  void addModuleInstsToCluster(Cluster* cluster, odb::dbModule* module);
  void incorporateNewClusterToTree(Cluster* cluster, Cluster* parent);
  void mergeClusters(std::vector<Cluster*>& candidate_clusters);
  void updateSubTree(Cluster* parent);
  void breakLargeFlatCluster(Cluster* parent);

  void fetchMixedLeaves(Cluster* parent,
                        std::vector<std::vector<Cluster*>>& mixed_leaves);
  void breakMixedLeaves(const std::vector<std::vector<Cluster*>>& mixed_leaves);

  void breakMixedLeaf(Cluster* mixed_leaf);
  void mapMacroInCluster2HardMacro(Cluster* cluster);
  void createOneClusterForEachMacro(Cluster* parent,
                                    const std::vector<HardMacro*>& hard_macros,
                                    std::vector<Cluster*>& macro_clusters);
  void classifyMacrosBySize(const std::vector<HardMacro*>& hard_macros,
                            std::vector<int>& size_class);
  void classifyMacrosByInterconn(const std::vector<Cluster*>& macro_clusters,
                                 std::vector<int>& interconn_class);
  void classifyMacrosByConnSignature(
      const std::vector<Cluster*>& macro_clusters,
      std::vector<int>& signature_class);
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

  // Coarse Shaping
  void runCoarseShaping();
  void setRootShapes();
  void calculateChildrenTilings(Cluster* parent);
  void calculateMacroTilings(Cluster* cluster);
  void setTightPackingTilings(Cluster* macro_array);
  void setIOClustersBlockages();
  IOSpans computeIOSpans();
  float computeIOBlockagesDepth(const IOSpans& io_spans);
  void setPlacementBlockages();

  // Fine Shaping
  bool runFineShaping(Cluster* parent,
                      std::vector<SoftMacro>& macros,
                      std::map<std::string, int>& soft_macro_id_map,
                      float target_util,
                      float target_dead_space);

  // Hierarchical Macro Placement 1st stage: Cluster Placement
  void runHierarchicalMacroPlacement(Cluster* parent);
  void runHierarchicalMacroPlacementWithoutBusPlanning(Cluster* parent);
  void runEnhancedHierarchicalMacroPlacement(Cluster* parent);

  void findOverlappingBlockages(std::vector<Rect>& blockages,
                                std::vector<Rect>& placement_blockages,
                                const Rect& outline);
  void computeBlockageOverlap(std::vector<Rect>& overlapping_blockages,
                              const Rect& blockage,
                              const Rect& outline);
  void updateChildrenShapesAndLocations(
      Cluster* parent,
      const std::vector<SoftMacro>& shaped_macros,
      const std::map<std::string, int>& soft_macro_id_map);
  void updateChildrenRealLocation(Cluster* parent,
                                  float offset_x,
                                  float offset_y);
  void mergeNets(std::vector<BundledNet>& nets);

  // Hierarchical Macro Placement 2nd stage: Macro Placement
  void placeMacros(Cluster* cluster);
  void createClusterForEachMacro(const std::vector<HardMacro*>& hard_macros,
                                 std::vector<HardMacro>& sa_macros,
                                 std::vector<Cluster*>& macro_clusters,
                                 std::map<int, int>& cluster_to_macro,
                                 std::set<odb::dbMaster*>& masters);
  void computeFencesAndGuides(const std::vector<HardMacro*>& hard_macros,
                              const Rect& outline,
                              std::map<int, Rect>& fences,
                              std::map<int, Rect>& guides);
  void createFixedTerminals(const Rect& outline,
                            const std::vector<Cluster*>& macro_clusters,
                            std::map<int, int>& cluster_to_macro,
                            std::vector<HardMacro>& sa_macros);
  std::vector<BundledNet> computeBundledNets(
      const std::vector<Cluster*>& macro_clusters,
      const std::map<int, int>& cluster_to_macro);
  void setArrayTilingSequencePair(Cluster* cluster,
                                  int macros_to_place,
                                  SequencePair& initial_seq_pair);

  // Orientation Improvement
  void generateTemporaryStdCellsPlacement(Cluster* cluster);
  void setModuleStdCellsLocation(Cluster* cluster, odb::dbModule* module);
  void setTemporaryStdCellLocation(Cluster* cluster, odb::dbInst* std_cell);

  void correctAllMacrosOrientation();
  float calculateRealMacroWirelength(odb::dbInst* macro);
  void adjustRealMacroOrientation(const bool& is_vertical_flip);
  void flipRealMacro(odb::dbInst* macro, const bool& is_vertical_flip);

  // Bus Planning
  void callBusPlanning(std::vector<SoftMacro>& shaped_macros,
                       std::vector<BundledNet>& nets_old);

  static bool isIgnoredMaster(odb::dbMaster* master);

  // Aux for conversion
  odb::Rect micronsToDbu(const Rect& micron_rect);

  sta::dbNetwork* network_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;
  par::PartitionMgr* tritonpart_ = nullptr;

  // flag variables
  const bool dynamic_congestion_weight_flag_ = false;
  // Our experiments show that for most testcases, turn off bus planning
  // can generate better results.

  // We recommend that you turn on this flag for technology nodes with very
  // limited routing layers such as SkyWater130.  But for NanGate45,
  // ASASP7, you should turn off this option.
  bool bus_planning_on_ = false;

  int num_updated_macros_ = 0;
  int num_hard_macros_cluster_ = 0;

  // Parameters related to macro placement
  std::string report_directory_;
  std::string macro_placement_file_;

  // User can specify a global region for some designs
  float global_fence_lx_ = std::numeric_limits<float>::max();
  float global_fence_ly_ = std::numeric_limits<float>::max();
  float global_fence_ux_ = 0.0;
  float global_fence_uy_ = 0.0;

  float halo_width_ = 0.0;
  float halo_height_ = 0.0;

  const int num_runs_ = 10;    // number of runs for SA
  int num_threads_ = 10;       // number of threads
  const int random_seed_ = 0;  // random seed for deterministic

  float target_dead_space_ = 0.2;  // dead space for the cluster
  float target_util_ = 0.25;       // target utilization of the design
  const float target_dead_space_step_ = 0.05;  // step for dead space
  const float target_util_step_ = 0.1;         // step for utilization
  const float num_target_util_ = 10;
  const float num_target_dead_space_ = 20;

  float min_ar_ = 0.3;  // the aspect ratio range for StdCellCluster (min_ar_, 1
                        // / min_ar_)

  float pin_access_th_ = 0.1;  // each pin access is modeled as a SoftMacro
  float pin_access_th_orig_ = 0.1;
  float pin_access_net_width_ratio_
      = 0.1;  // define the ratio of number of connections
              // related to IOs to the range of these IO spans
  float notch_v_th_ = 10.0;
  float notch_h_th_ = 10.0;

  int snap_layer_ = 4;

  // SA related parameters
  // weight for different penalty
  float area_weight_ = 0.1;
  float outline_weight_ = 1.0;
  float wirelength_weight_ = 1.0;
  float guidance_weight_ = 10.0;
  float fence_weight_ = 10.0;
  float boundary_weight_ = 5.0;
  float notch_weight_ = 1.0;
  float macro_blockage_weight_ = 1.0;

  // guidances, fences, constraints
  std::map<std::string, Rect> fences_;  // macro_name, fence
  std::map<std::string, Rect> guides_;  // macro_name, guide
  std::vector<Rect> placement_blockages_;
  std::vector<Rect> macro_blockages_;
  std::map<Boundary, Rect> boundary_to_io_blockage_;

  // Fast SA hyperparameter
  float init_prob_ = 0.9;
  const int max_num_step_ = 2000;
  const int num_perturb_per_step_ = 500;

  // the virtual weight between std cell part and corresponding macro part
  // to force them stay together
  float virtual_weight_ = 10.0;

  // probability of each action
  float pos_swap_prob_ = 0.2;
  float neg_swap_prob_ = 0.2;
  float double_swap_prob_ = 0.2;
  float exchange_swap_prob_ = 0.2;
  float flip_prob_ = 0.4;
  float resize_prob_ = 0.4;

  // design-related variables
  float macro_with_halo_area_ = 0.0;

  // dataflow parameters and store the dataflow
  int max_num_ff_dist_ = 5;  // maximum number of FF distances between
  float dataflow_factor_ = 2.0;
  float dataflow_weight_ = 1;
  std::vector<std::pair<odb::dbITerm*, std::vector<std::set<odb::dbInst*>>>>
      macro_ffs_conn_map_;
  std::vector<std::pair<odb::dbBTerm*, std::vector<std::set<odb::dbInst*>>>>
      io_ffs_conn_map_;
  std::vector<std::pair<odb::dbITerm*, std::vector<std::set<odb::dbInst*>>>>
      macro_macro_conn_map_;

  // statistics of the design
  Metrics* metrics_ = nullptr;
  std::map<const odb::dbModule*, Metrics*> logical_module_map_;
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

  // minimum number of connections between two clusters
  // for them to be identified as connected
  int signature_net_threshold_ = 20;
  int large_net_threshold_ = 100;     // ignore global nets when clustering
  const int bus_net_threshold_ = 32;  // only for bus planning
  float congestion_weight_ = 0.5;     // for balance timing and congestion

  const float macro_dominated_cluster_threshold_ = 0.01;

  // since we convert from the database unit to the micrometer
  // during calculation, we may loss some accuracy.
  const float conversion_tolerance_ = 0.01;

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
  std::unordered_map<odb::dbInst*, int> inst_to_cluster_;    // inst, id
  std::unordered_map<odb::dbBTerm*, int> bterm_to_cluster_;  // io pin, id

  // All the bundled IOs are children of root_cluster_
  // Bundled IO (Pads)
  // In the bundled IO clusters, we don't store the ios in their corresponding
  // clusters However, we store the instances in their corresponding clusters
  // map IO pins to Pads (for designs with IO pads)
  std::map<odb::dbBTerm*, odb::dbInst*> io_pad_map_;
  bool design_has_io_clusters_ = true;
  bool design_has_only_macros_ = false;
  bool design_has_unfixed_macros_ = true;

  std::unique_ptr<Mpl2Observer> graphics_;
};

class Pusher
{
 public:
  Pusher(utl::Logger* logger,
         Cluster* root,
         odb::dbBlock* block,
         const std::map<Boundary, Rect>& boundary_to_io_blockage);

  void pushMacrosToCoreBoundaries();

 private:
  void setIOBlockages(const std::map<Boundary, Rect>& boundary_to_io_blockage);
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
  bool overlapsWithIOBlockage(const odb::Rect& cluster_box, Boundary boundary);

  utl::Logger* logger_;

  Cluster* root_;
  odb::dbBlock* block_;
  odb::Rect core_;

  std::map<Boundary, odb::Rect> boundary_to_io_blockage_;
  std::vector<HardMacro*> hard_macros_;
};

// The parameters necessary to compute one coordinate of the new
// origin for aligning the macros' pins to the track-grid
struct SnapParameters
{
  int offset = 0;
  int pitch = 0;
  int pin_width = 0;
  int lower_left_to_first_pin = 0;
};

class Snapper
{
 public:
  Snapper();
  Snapper(odb::dbInst* inst);

  void setMacro(odb::dbInst* inst) { inst_ = inst; }
  void snapMacro();

 private:
  odb::Point computeSnapOrigin();
  SnapParameters computeSnapParameters(odb::dbTechLayer* layer,
                                       odb::dbBox* box,
                                       bool vertical_layer);
  void getTrackGrid(odb::dbTrackGrid* track_grid,
                    std::vector<int>& coordinate_grid,
                    bool vertical_layer);
  int getPitch(odb::dbTechLayer* layer, bool vertical_layer);
  int getOffset(odb::dbTechLayer* layer, bool vertical_layer);
  int getPinWidth(odb::dbBox* box, bool vertical_layer);
  int getPinToLowerLeftDistance(odb::dbBox* box, bool vertical_layer);

  odb::dbInst* inst_;
};

}  // namespace mpl2
