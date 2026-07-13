#pragma once

#include <csignal>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
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
  void route(bool incremental = false);
  void write(const std::string& guide_file);
  NetRouteMap getRoutes();
  GRoute getNetRoute(odb::dbNet* db_net);
  void updateDbCongestion();
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
  void routeIncremental();
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

  /**
   * @brief Computes per-layer NDR demand / cost multipliers for a net.
   *
   * Per-layer formula matches FastRoute's
   * `GlobalRouter::computeUserLayerNdr` shape:
   *   `ndr_pitch  = ndr_width / 2 + ndr_spacing + default_width / 2`
   *   `factor[l]  = max(1.0, ndr_pitch / default_pitch)`
   *
   * @param db_net The net whose NDR (if any) we are reading.
   *
   * @returns Vector of length `grid_graph_->getNumLayers()`; entry
   *          `l` is the factor on layer `l`, or 1.0 on layers
   *          without an NDR rule (and for nets that carry no NDR
   *          at all).
   */
  std::vector<double> computeNdrCosts(odb::dbNet* db_net) const;

  std::vector<int> computeNdrWidths(odb::dbNet* db_net) const;
  /**
   * @brief Builds the rip-up set of nets touching a congested edge.
   *
   * Populates `net_indices` with the indices of every net whose
   * routing tree contains at least one edge whose
   * `demand > capacity * threshold`. At threshold == 1.0 the result is
   * the strict-overflow set (the default used by the pattern/maze
   * stages); at threshold < 1.0 it widens to include near-overflow
   * edges, which is how the iterative RRR loop catches the "many nets
   * piled onto one layer, only a few overflow" failure mode.
   *
   * @param net_indices Output: cleared and refilled with the selected
   *                    net indices.
   * @param threshold   Per-edge utilization cutoff in [0.0, 1.0]
   *                    (default 1.0 = strict overflow).
   */
  void updateCongestedNets(std::vector<int>& net_indices,
                           double threshold = 1.0);

  void patternRoute(std::vector<int>& net_indices);
  // Re-route the critical nets on real 3D-tree resistance, right after the
  // neutral first PatternRoute.
  void patternRouteResAware(std::vector<int>& net_indices);
  void patternRouteWithDetours(std::vector<int>& net_indices);
  void mazeRoute(std::vector<int>& net_indices);

  /**
   * @brief Stage 5 — iterative rip-up and re-route.
   *
   * Wraps the maze stage in a loop that sharpens the logistic cost
   * slope each pass (so `PatternRoute` and the maze cost surface
   * penalise full edges more aggressively) and widens the rip-up set
   * to nets sitting on near-full edges (not just strictly-overflowed
   * ones). Designed for the per-layer over-concentration failure mode
   * where many nets pile onto a single low layer while upper layers
   * stay idle.
   *
   * Early-exits when the integer overflow metric (`totalOverflow()`)
   * is already zero, so designs that finished stage 4 clean pay no
   * cost. Emits `GRT-0117` per iteration and, in full route only,
   * `GRT-0118` if overflow remains when the loop ends (incremental
   * defers to the session-end `GRT-0128`).
   *
   * @param net_indices Reused scratch buffer (cleared on entry by
   *                    `updateCongestedNets`).
   */
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

  /**
   * @brief Diagnoses whether residual overflow is spreadable.
   *
   * For each `(direction, x, y)` tile, sums capacity and demand across
   * all same-direction routing layers and classifies the tile:
   *
   *   - **2D-aggregate overflow** (`sum_demand > sum_capacity`): true
   *     planar congestion — no layer-assignment policy can avoid it.
   *   - **3D-only overflow** (per-layer overflow but the aggregate has
   *     slack): some same-direction layer at the same tile still has
   *     unused capacity. The router could in principle redistribute
   *     the demand there.
   *
   * Reports `3D overflow / 2D-aggregate / spreadable = 3D − 2D` (the
   * gap is the upper bound on how much overflow a perfect layer
   * assignment could clear). Gated on `debugPrint(GRT, "rrr_2d", 1)`,
   * so default builds pay only the gate check. Enable via Tcl with
   * `set_debug_level GRT rrr_2d 1`.
   */
  void debugCongestion2D() const;

  std::unique_ptr<Design> design_;
  std::unique_ptr<GridGraph> grid_graph_;
  std::vector<int> net_indices_;
  std::vector<std::unique_ptr<GRNet>> gr_nets_;
  std::unordered_map<odb::dbNet*, GRNet*> db_net_map_;

  odb::dbDatabase* db_;
  utl::Logger* logger_;
  utl::ServiceRegistry* service_registry_;
  stt::SteinerTreeBuilder* stt_builder_;
  sta::dbSta* sta_;
  NetRouteMap routes_;

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
