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

  double pin_patch_threshold = 20.0;
  int pin_patch_padding = 1;
  double wire_patch_threshold = 2.0;
  double wire_patch_inflation_rate = 1.2;

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
  void route();
  void write(const std::string& guide_file);
  NetRouteMap getRoutes();
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
  void setCongestionIterations(int iterations)
  {
    congestion_iterations_ = iterations;
  }
  void addDirtyNet(odb::dbNet* net);
  void updateNet(odb::dbNet* net);
  void removeNet(odb::dbNet* net);
  void routeIncremental();

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
  float calculatePartialSlack();
  float getNetSlack(odb::dbNet* net);
  void setInitialNetSlacks();

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
  void patternRouteWithDetours(std::vector<int>& net_indices);
  void mazeRoute(std::vector<int>& net_indices);

  /**
   * @brief Stage 4 — iterative rip-up and re-route.
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
   * is already zero, so designs that finished stage 3 clean pay no
   * cost. Emits `GRT-0117` per iteration and `GRT-0118` if overflow
   * remains when the loop ends.
   *
   * See `src/grt/doc/01-iterative-rrr.md` for the cost-model audit
   * and the rationale for the chosen defaults.
   *
   * @param net_indices Reused scratch buffer (cleared on entry by
   *                    `updateCongestedNets`).
   */
  void iterativeRRR(std::vector<int>& net_indices);
  void sortNetIndices(std::vector<int>& net_indices) const;
  void getGuides(const GRNet* net,
                 std::vector<std::pair<int, grt::BoxT>>& guides);
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

  float critical_nets_percentage_ = 0;
  int congestion_iterations_ = 5;

  std::vector<int> nets_to_route_;
};

}  // namespace grt
