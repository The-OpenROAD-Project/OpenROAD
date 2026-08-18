#pragma once

#include <csignal>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "grt/GRoute.h"
#include "odb/PtrSetMap.h"
#include "odb/geom.h"

namespace odb {
class dbDatabase;
class dbNet;
class dbITerm;
class dbBTerm;
}  // namespace odb

namespace sta {
class dbSta;
class dbNetwork;
}  // namespace sta

namespace stt {
class SteinerTreeBuilder;
}  // namespace stt

namespace utl {
class Logger;
class ServiceRegistry;
}  // namespace utl

namespace grt {

class Design;
class GridGraph;
class GRNet;
class GRTreeNode;
class BoxT;

struct Constants
{
  double weight_wire_length = 0.5;
  double weight_via_number = 4.0;
  double weight_short_area = 500.0;

  int min_routing_layer = 1;

  double cost_logistic_slope = 1.0;

  // allowed stem length increase to trunk length ratio
  double max_detour_ratio = 0.25;
  int target_detour_count = 20;

  double via_multiplier = 2.0;

  double maze_logistic_slope = 0.5;

  // Min net length (routed-tree length, gcells) for res-aware; shorter nets
  // are skipped.
  int resistance_min_net_length = 3;

  // Scales the res-aware resistance cost to CUGR's wire-cost magnitude.
  double resistance_weight = 50.0;

  double pin_patch_threshold = 20.0;
  int pin_patch_padding = 1;
  double wire_patch_threshold = 2.0;
  double wire_patch_inflation_rate = 1.2;

  // Cost multiplier for wires that don't fit an edge, biasing vias to climb to
  // a free layer; 0 disables the gate.
  double congestion_gate_penalty = 4.0;

  bool write_heatmap = false;
};

class CUGR
{
 public:
  CUGR(odb::dbDatabase* db,
       utl::Logger* log,
       utl::ServiceRegistry* service_registry,
       stt::SteinerTreeBuilder* stt_builder,
       sta::dbSta* sta);
  ~CUGR();
  void init(int min_routing_layer,
            int max_routing_layer,
            const odb::PtrSet<odb::dbNet>& clock_nets);
  // Reset per-session netlist state (like FastRouteCore::clear); Tcl-applied
  // configuration survives, and init() rebuilds design_/grid_graph_.
  void clear();
  void route(bool incremental);
  void write(const std::string& guide_file);
  NetRouteMap getRoutes();
  GRoute getNetRoute(odb::dbNet* db_net);
  void updateDbCongestion();
  // CUGR-native congestion table (GRT-0130): fractional tracks, demand and
  // overflow split into wire vs via-stub shares (proportional attribution);
  // sub-min layers shown as all-zero rows. Gated on verbose_.
  void reportCongestion() const;
  void getITermsAccessPoints(
      odb::dbNet* net,
      odb::PtrMap<odb::dbITerm, odb::Point3D>& access_points);
  void getBTermsAccessPoints(
      odb::dbNet* net,
      odb::PtrMap<odb::dbBTerm, odb::Point3D>& access_points);
  void setCriticalNetsPercentage(float percentage)
  {
    critical_nets_percentage_ = percentage;
  }
  float getCriticalNetsPercentage() const { return critical_nets_percentage_; }
  void setResistanceAware(bool resistance_aware)
  {
    resistance_aware_ = resistance_aware;
  }
  void setResAwareNetsPercentage(float percentage)
  {
    res_aware_percentage_ = percentage;
  }
  void setCongestionIterations(int iterations)
  {
    congestion_iterations_ = iterations;
  }
  void setVerbose(bool verbose) { verbose_ = verbose; }
  void updateNet(odb::dbNet* net);
  void removeNet(odb::dbNet* net);
  // Transfer removed net tree ownership to preserved net without removing
  // its GridGraph usage. Called at inDbNetPostMerge time.
  void mergeNet(odb::dbNet* preserved_net,
                odb::dbNet* removed_net,
                const std::vector<GSegment>& connection);
  // True if the edge on (layer_index, tile_x, tile_y) has capacity left for
  // db_net's NDR demand on that layer (1.0 for non-NDR nets); the CUGR
  // analog of FastRouteCore::hasAvailableResources.
  bool hasAvailableResources(odb::dbNet* db_net,
                             int layer_index,
                             int tile_x,
                             int tile_y) const;
  // True if a complete jumper -- the wire on layer_index between the tiles
  // plus a two-layer via stack at each endpoint -- fits the headroom of
  // every edge it would charge, accumulating demands that share an edge.
  bool hasJumperResources(odb::dbNet* db_net,
                          int layer_index,
                          int init_tile_x,
                          int init_tile_y,
                          int final_tile_x,
                          int final_tile_y) const;
  // Adopts an externally restored routing (journal restore): rebuilds the
  // net's routing tree from the segments and swaps the grid-graph demand
  // without scheduling a reroute. Returns false if the net must be rerouted.
  bool restoreNetRoute(odb::dbNet* db_net, const GRoute& route);

  const std::vector<int>& getOriginalResources() const;
  void computeCongestionInformation();
  const std::vector<int>& getTotalCapacityPerLayer() const;
  const std::vector<int>& getTotalUsagePerLayer() const;
  const std::vector<int>& getTotalOverflowPerLayer() const;
  const std::vector<int>& getMaxHorizontalOverflows() const;
  const std::vector<int>& getMaxVerticalOverflows() const;

  int totalOverflow();
  void saveCongestion();

 private:
  // True if (layer_0, tile_x, tile_y) indexes an existing grid edge.
  bool isEdgeInGrid(int layer_0, int tile_x, int tile_y) const;
  // Refresh net slacks, re-mark the res-aware/critical set, and demote
  // non-critical nets so the next stage routes critical nets first.
  void updateCriticalNets(const std::vector<int>& net_indices);
  // Re-extract parasitics and refresh every net's slack from the routing.
  void updateNetSlacks(const std::vector<int>& net_indices);
  // Refresh the slack of the given nets from STA, without re-extracting
  // parasitics (incremental scope).
  void refreshNetSlacks(const std::vector<int>& net_indices);
  // Slack value at the critical_nets_percentage_ percentile of the nets.
  float criticalSlackThreshold() const;
  // Push nets with slack above the threshold to the back of the default
  // ordering by maxing their slack; res-aware nets are exempt.
  void demoteNonCriticalNets(float slack_th);
  float getNetSlack(odb::dbNet* net);
  void setInitialNetSlacks(const std::vector<int>& net_indices);
  // Builds a routing tree spanning the segments' gcells; nullptr if the
  // segments are malformed or disconnected.
  std::shared_ptr<GRTreeNode> buildTreeFromRoute(const GRoute& route) const;
  // Debug (set_debug_level GRT verify_demand 1): recompute grid-graph demand
  // from every committed tree and report drift from the tracked demand.
  void verifyDemandConsistency(const char* tag);

  // Per-layer NDR demand factors, sized getNumLayers(); 1.0 without a rule.
  // Matches FastRoute's computeUserLayerNdr shape: factor = max(1.0,
  // (ndr_width / 2 + ndr_spacing + default_width / 2) / default_pitch).
  std::vector<double> computeNdrCosts(odb::dbNet* db_net) const;

  std::vector<int> computeNdrWidths(odb::dbNet* db_net) const;
  // Refills net_indices with nets whose tree touches an edge where
  // demand > capacity * threshold; thresholds < 1.0 widen the rip-up
  // set to near-overflow edges for the iterative RRR loop.
  void updateCongestedNets(std::vector<int>& net_indices,
                           double threshold = 1.0);

  void patternRoute(std::vector<int>& net_indices);
  // Re-route the critical nets on real 3D-tree resistance, right after the
  // neutral first PatternRoute.
  void patternRouteResAware(std::vector<int>& net_indices);
  void patternRouteWithDetours(std::vector<int>& net_indices);
  void mazeRoute(std::vector<int>& net_indices);

  // Stage 5: rip-up-and-re-route loop that sharpens the logistic cost slope
  // each pass and widens the rip-up set to near-full edges, targeting
  // per-layer over-concentration; early-exits when overflow is already zero.
  void iterativeRRR(std::vector<int>& net_indices);
  // res_aware_order selects the multi-factor res-aware ordering; false uses
  // the default slack/bbox order.
  void sortNetIndices(std::vector<int>& net_indices,
                      bool res_aware_order) const;
  void getGuides(const GRNet* net,
                 std::vector<std::pair<int, grt::BoxT>>& guides);
  // Append net's routing tree to route as GRoute segments.
  void buildNetRoute(const GRNet* net, GRoute& route) const;
  void printStatistics() const;

  // Tile classification for debugCongestion2D.
  struct Congestion2D
  {
    double total_3d_overflow = 0.0;
    double total_2d_overflow = 0.0;
    int tiles_3d_only = 0;
    int tiles_2d = 0;
  };
  Congestion2D computeCongestion2D() const;

  // Debug (set_debug_level GRT rrr_2d 1): splits residual overflow into
  // 2D-aggregate (true planar congestion) vs spreadable (a same-direction
  // layer at the tile still has slack a better assignment could use).
  void debugCongestion2D() const;

  std::unique_ptr<Design> design_;
  std::unique_ptr<GridGraph> grid_graph_;
  std::vector<int> net_indices_;
  std::vector<std::unique_ptr<GRNet>> gr_nets_;
  std::unordered_map<odb::dbNet*, GRNet*> db_net_map_;
  // Nets merged into a survivor; removeNet() must NOT remove their GridGraph
  // usage.
  std::unordered_set<odb::dbNet*> merged_nets_;

  odb::dbDatabase* db_;
  utl::Logger* logger_;
  utl::ServiceRegistry* service_registry_;
  stt::SteinerTreeBuilder* stt_builder_;
  sta::dbSta* sta_;

  Constants constants_;

  int area_of_pin_patches_ = 0;
  int area_of_wire_patches_ = 0;

  float critical_nets_percentage_ = 10;
  int congestion_iterations_ = 5;
  bool verbose_ = true;

  // Suppresses the global parasitics re-estimate during incremental routing.
  bool incremental_routing_ = false;
  // Dirty-net set for the current incremental pass; scopes congestion checks.
  std::vector<int> incremental_candidates_;

  bool resistance_aware_ = false;
  // Per-run normalisers for getResAwareScore (default 1 => well-defined).
  float worst_slack_ = 1.0f;
  float worst_resistance_ = 1.0f;
  int worst_fanout_ = 1;
  int worst_net_length_ = 1;

  // Percent of eligible candidate nets marked res-aware; set via
  // -res_aware_nets_percentage (FastRoute default).
  float res_aware_percentage_ = 15.0f;

  // Select the res-aware net set (like FastRoute updateSlacks) and refresh the
  // worst_* normalisers; no-op unless resistance_aware_.
  void markResAwareNets(const std::vector<int>& net_indices);

  // FR-style ordering score (lower routes first): slack/resistance/fanout/
  // length blend, each normalised by the per-run worst.
  float getResAwareScore(const GRNet* net) const;

  std::vector<int> nets_to_route_;
};

}  // namespace grt
