#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "CUGR.h"
#include "Design.h"
#include "GRTree.h"
#include "Layers.h"
#include "geo.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "robin_hood.h"

namespace grt {

using CapacityT = double;
class GRNet;
template <typename Type>
class GridGraphView;

struct AccessPoint
{
  PointT point;
  IntervalT layers;
  bool operator==(const AccessPoint& ap) const
  {
    return point == ap.point && layers == ap.layers;
  }
};

// Only hash and compare on the point, not the layers
class AccessPointHash
{
 public:
  AccessPointHash(int y_size) : y_size_(y_size) {}

  std::size_t operator()(const AccessPoint& ap) const
  {
    return robin_hood::hash_int(ap.point.x() * y_size_ + ap.point.y());
  }

 private:
  const uint64_t y_size_;
};

struct AccessPointEqual
{
  bool operator()(const AccessPoint& lhs, const AccessPoint& rhs) const
  {
    return lhs.point == rhs.point;
  }
};

using AccessPointSet
    = robin_hood::unordered_set<AccessPoint, AccessPointHash, AccessPointEqual>;

struct GraphEdge
{
  CapacityT getResource() const { return capacity - demand; }

  CapacityT capacity{0};
  CapacityT demand{0};
};

class GridGraph
{
 public:
  GridGraph(const Design* design,
            const Constants& constants,
            utl::Logger* logger);
  int getLibDBU() const { return lib_dbu_; }
  int getM2Pitch() const { return m2_pitch_; }
  int getNumLayers() const { return num_layers_; }
  int getSize(int dimension) const { return (dimension ? y_size_ : x_size_); }
  int getXSize() const { return x_size_; }
  int getYSize() const { return y_size_; }
  std::string getLayerName(int layer_index) const
  {
    return layer_names_[layer_index];
  }
  int getLayerDirection(int layer_index) const
  {
    return layer_directions_[layer_index];
  }

  uint64_t hashCell(const GRPoint& point) const
  {
    return ((uint64_t) point.getLayerIdx() * x_size_ + point.x()) * y_size_
           + point.y();
  };
  int getGridline(const int dimension, const int index) const
  {
    return gridlines_[dimension][index];
  }
  BoxT getCellBox(PointT point) const;
  BoxT rangeSearchCells(const BoxT& box) const;
  GraphEdge getEdge(const int layer_index, const int x, const int y) const
  {
    return graph_edges_[layer_index][x][y];
  }

  CapacityT getInitialEdgeCapacity(int layer_index, int x, int y) const
  {
    const int direction = layer_directions_[layer_index];
    const int perp = (direction == MetalLayer::H) ? y : x;
    return grid_tracks_[layer_index][perp];
  }

  const std::vector<int>& getOriginalResources() const
  {
    return original_resources_per_layer_;
  }
  void computeCongestionInformation();
  const std::vector<int>& getTotalCapacityPerLayer() const
  {
    return cap_per_layer_;
  }
  const std::vector<int>& getTotalUsagePerLayer() const
  {
    return usage_per_layer_;
  }
  const std::vector<int>& getTotalOverflowPerLayer() const
  {
    return overflow_per_layer_;
  }
  const std::vector<int>& getMaxHorizontalOverflows() const
  {
    return max_h_overflow_;
  }
  const std::vector<int>& getMaxVerticalOverflows() const
  {
    return max_v_overflow_;
  }

  int getEdgeLength(int direction, int edge_index) const;

  /**
   * @brief Returns the cost of placing a wire segment from u to v.
   *
   * `net_factor` scales the per-net demand seen by the logistic
   * penalty so NDR nets pay proportionally to their wider footprint.
   * The caller is responsible for looking up the layer-specific
   * factor (`GRNet::getNdrCost(layer_index)`).
   *
   * @param layer_index 0-based routing layer.
   * @param u           Segment start in gcell coordinates.
   * @param v           Segment end in gcell coordinates.
   * @param net_factor  Per-net, per-layer demand multiplier.
   *                    Default 1.0 = no NDR.
   *
   * @returns Wire cost in the same units as the maze's cost view.
   */
  CostT getWireCost(int layer_index,
                    PointT u,
                    PointT v,
                    double net_factor = 1.0) const;

  /**
   * @brief Returns the cost of a via between `layer_index` and
   *        `layer_index + 1` at `loc`.
   *
   * The unit via cost is constant; only the wire-patch demand each
   * via implies is scaled by the per-layer NDR factor. A via that
   * lands on M3-M5 may therefore use a different factor on its M3
   * patches than on its M4 patches.
   *
   * @param layer_index Lower-layer index of the via (0-based).
   * @param loc         Via location in gcell coordinates.
   * @param net_costs   Per-layer NDR cost vector. Empty (default) =
   *                    no NDR; out-of-range layers fall back to 1.0.
   *
   * @returns Total via cost (unit + wire patches).
   */
  CostT getViaCost(int layer_index,
                   PointT loc,
                   const std::vector<double>& net_costs = {}) const;

  CostT getUnitViaCost() const { return unit_via_cost_; }

  /**
   * @brief Sets the multiplier applied to the logistic-cost slopes.
   *
   * Scales `constants_.cost_logistic_slope` and
   * `constants_.maze_logistic_slope` wherever they are read by
   * `getWireCost`, `extractWireCostView` and `updateWireCostView`. Used
   * by CUGR's iterative RRR loop to sharpen the per-edge cost gradient
   * between iterations. The default value 1.0 leaves the cost surface
   * unchanged.
   *
   * @param m New multiplier value. Must be > 0.
   */
  void setCostMultiplier(double m) { cost_multiplier_ = m; }
  double getCostMultiplier() const { return cost_multiplier_; }

  /**
   * @brief Counts congested edges traversed by a routing tree.
   *
   * Walks the tree's wire segments and returns how many edges satisfy
   * `demand > capacity * threshold`. At threshold == 1.0 this collapses
   * to strict overflow (`demand > capacity`); at threshold < 1.0 it
   * widens to include near-overflow ("congested but not yet
   * overflowing") edges. Edges sitting exactly at capacity are
   * intentionally *not* flagged — being full is not the same as being
   * congested. Used by the RRR loop to extend the rip-up set beyond
   * strictly-overflowed nets.
   *
   * @param tree      Routing tree to walk.
   * @param threshold Per-edge utilization cutoff in [0.0, 1.0].
   *
   * @returns Number of tree edges meeting the predicate (>= 0).
   */
  int checkCongestion(const std::shared_ptr<GRTreeNode>& tree,
                      double threshold) const;

  // Misc
  AccessPointSet selectAccessPoints(GRNet* net) const;

  // Methods for updating demands - Public API.

  /**
   * @brief Adds the demand of a routing tree to every edge and via
   *        it covers.
   *
   * Each segment's demand is scaled by the layer's NDR cost from
   * `net_costs` (1.0 on layers without a rule, or when the vector
   * is empty).
   *
   * @param tree      Routing tree to commit.
   * @param net_costs Per-layer NDR cost vector. Empty = no NDR.
   */
  void addTreeUsage(const std::shared_ptr<GRTreeNode>& tree,
                    const std::vector<double>& net_costs = {});

  /**
   * @brief Removes the demand previously added for a routing tree.
   *
   * The caller must supply the same `net_costs` that were used when
   * the tree's demand was added so it is now subtracted exactly.
   *
   * @param tree      Routing tree to rip up.
   * @param net_costs Per-layer NDR cost vector. Empty = no NDR.
   */
  void removeTreeUsage(const std::shared_ptr<GRTreeNode>& tree,
                       const std::vector<double>& net_costs = {});

  // Checks
  bool checkOverflow(int layer_index, int x, int y) const
  {
    return getEdge(layer_index, x, y).getResource() < 0.0;
  }
  std::string getPythonString(
      const std::shared_ptr<GRTreeNode>& routing_tree) const;

  // 2D maps
  void extractBlockageView(GridGraphView<bool>& view) const;
  void extractCongestionView(
      GridGraphView<bool>& view) const;  // 2D overflow look-up table
  void extractWireCostView(GridGraphView<CostT>& view) const;

  /**
   * @brief Builds a 2D wire-cost view tailored to one NDR net.
   *
   * For NDR nets, restricts each edge on the best single-layer headroom
   * `max_l (capacity_l - demand_l)` and subtracts `(net_factor - 1)`.
   * This prevents a 2D-vs-3D mismatch in which summed capacity hides
   * per-layer granularity: an NDR wire with `net_factor` tracks
   * worth of footprint can't span 3D layers, so an edge with
   * `net_factor` free tracks spread across `net_factor` layers must
   * not be reported as routable.`net_factor == 1` (empty `net_costs`)
   * reproduces the default summed view exactly.
   *
   * @param view       Output cost map (2 * x_size * y_size).
   * @param net_costs  Per-layer NDR factor; empty / all-1.0 = no NDR.
   */
  void extractWireCostView(GridGraphView<CostT>& view,
                           const std::vector<double>& net_costs) const;
  void updateWireCostView(
      GridGraphView<CostT>& view,
      const std::shared_ptr<GRTreeNode>& routing_tree) const;

  // For visualization
  void write(const std::string& heatmap_file = "heatmap.txt") const;

 private:
  IntervalT rangeSearchGridlines(int dimension,
                                 const IntervalT& loc_interval) const;
  // Find the gridlines_ within [locInterval.low, locInterval.high]
  IntervalT rangeSearchRows(int dimension, const IntervalT& loc_interval) const;
  // Find the rows/columns overlapping with [locInterval.low, locInterval.high]

  // Utility functions for cost calculation
  CostT getUnitLengthWireCost() const { return unit_length_wire_cost_; }
  // CostT getUnitViaCost() const { return unit_via_cost_; }
  CostT getUnitLengthShortCost(int layer_index) const
  {
    return unit_length_short_costs_[layer_index];
  }
  std::vector<AccessPoint> translateAccessPointsToGrid(
      const std::vector<odb::dbAccessPoint*>& ap,
      const odb::Point& inst_location) const;
  AccessPoint selectAccessPoint(
      const std::vector<AccessPoint>& access_points) const;
  bool findODBAccessPoints(GRNet* net,
                           AccessPointSet& selected_access_points) const;

  double logistic(const CapacityT& input, double slope) const;
  CostT getWireCost(int layer_index,
                    PointT lower,
                    CapacityT demand = 1.0,
                    double net_factor = 1.0) const;

  // Methods for updating demands - Internal Implementation.
  // Single-layer commits take a per-layer `net_factor` (a scalar);
  // multi-layer commits take a per-layer NDR cost vector so they
  // can look up each layer's factor as they walk the tree / via
  // stack. Empty vectors mean "no NDR" (1.0 everywhere).
  void commit(int layer_index,
              PointT lower,
              CapacityT demand,
              double net_factor = 1.0);
  void commitWire(int layer_index,
                  PointT lower,
                  bool rip_up = false,
                  double net_factor = 1.0);
  void commitVia(int layer_index,
                 PointT loc,
                 bool rip_up = false,
                 const std::vector<double>& net_costs = {});
  void commitTree(const std::shared_ptr<GRTreeNode>& tree,
                  bool rip_up = false,
                  const std::vector<double>& net_costs = {});

  utl::Logger* logger_;
  const std::vector<std::vector<int>> gridlines_;
  std::vector<std::vector<int>> grid_centers_;
  std::vector<std::string> layer_names_;
  std::vector<int> layer_directions_;
  std::vector<int> layer_min_lengths_;

  const int lib_dbu_;
  const int m2_pitch_;

  const int num_layers_;
  const int x_size_;
  const int y_size_;

  const Design* design_;

  // Unit costs
  CostT unit_length_wire_cost_;
  CostT unit_via_cost_;
  std::vector<CostT> unit_length_short_costs_;

  int total_length_ = 0;
  int total_num_vias_ = 0;
  // gridEdges[l][x][y] stores the edge {(l, x, y), (l, x+1, y)} or {(l, x, y),
  // (l, x, y+1)} depending on the routing direction of the layer
  std::vector<std::vector<std::vector<GraphEdge>>> graph_edges_;
  // Per-layer initial track count keyed by perpendicular index
  // (row for H layers, column for V layers). Used to recover the
  // pre-blockage / pre-adjustment capacity of each edge.
  std::vector<std::vector<int>> grid_tracks_;
  // Per-layer capacity sums captured before user-defined adjustments are
  // applied (i.e. only blockage reductions are accounted for). Used by
  // resource reporting so that the report can show the reduction from
  // user adjustments analogous to FastRoute's real_cap vs cap.
  std::vector<int> original_resources_per_layer_;
  // Per-layer caches populated by computeCongestionInformation(). Kept
  // valid by congestion_info_dirty_: any commit() that mutates demand
  // marks the caches stale, so the next computeCongestionInformation()
  // refreshes them; otherwise it returns immediately.
  std::vector<int> cap_per_layer_;
  std::vector<int> usage_per_layer_;
  std::vector<int> overflow_per_layer_;
  std::vector<int> max_h_overflow_;
  std::vector<int> max_v_overflow_;
  bool congestion_info_dirty_ = true;
  const Constants constants_;
  // RRR slope multiplier. 1.0 leaves the cost surface unchanged.
  double cost_multiplier_ = 1.0;
};

template <typename Type>
class GridGraphView : public std::vector<std::vector<std::vector<Type>>>
{
 public:
  bool check(const PointT& u, const PointT& v) const
  {
    static_assert(std::is_same_v<Type, bool>, "Template argument must be bool");
    assert(u.x() == v.x() || u.y() == v.y());
    if (u.y() == v.y()) {
      const auto [l, h] = std::minmax({u.x(), v.x()});
      for (int x = l; x < h; x++) {
        if ((*this)[MetalLayer::H][x][u.y()]) {
          return true;
        }
      }
    } else {
      const auto [l, h] = std::minmax({u.y(), v.y()});
      for (int y = l; y < h; y++) {
        if ((*this)[MetalLayer::V][u.x()][y]) {
          return true;
        }
      }
    }
    return false;
  }

  Type sum(const PointT& u, const PointT& v) const
  {
    static_assert(std::is_integral_v<Type> || std::is_floating_point_v<Type>,
                  "Template argument must be integral or floating point");
    assert(u.x() == v.x() || u.y() == v.y());
    Type res = 0;
    if (u.y() == v.y()) {
      const auto [l, h] = std::minmax({u.x(), v.x()});
      for (int x = l; x < h; x++) {
        res += (*this)[MetalLayer::H][x][u.y()];
      }
    } else {
      const auto [l, h] = std::minmax({u.y(), v.y()});
      for (int y = l; y < h; y++) {
        res += (*this)[MetalLayer::V][u.x()][y];
      }
    }
    return res;
  }
};

}  // namespace grt