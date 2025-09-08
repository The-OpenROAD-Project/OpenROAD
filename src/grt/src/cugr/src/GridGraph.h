#pragma once
#include <algorithm>
#include <cassert>
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
#include "robin_hood.h"

namespace grt {

using CapacityT = double;
class GRNet;
template <typename Type>
class GridGraphView;

struct GraphEdge
{
  CapacityT capacity{0};
  CapacityT demand{0};

  CapacityT getResource() const { return capacity - demand; }
};

class GridGraph
{
 public:
  GridGraph(const Design* design, const Constants& constants);
  int getLibDBU() const { return lib_dbu_; }
  int getM2Pitch() const { return m2_pitch_; }
  unsigned getNumLayers() const { return num_layers_; }
  unsigned getSize(unsigned dimension) const
  {
    return (dimension ? y_size_ : x_size_);
  }
  std::string getLayerName(int layer_index) const
  {
    return layer_names_[layer_index];
  }
  unsigned getLayerDirection(int layer_index) const
  {
    return layer_directions_[layer_index];
  }

  uint64_t hashCell(const GRPoint& point) const
  {
    return ((uint64_t) point.getLayerIdx() * x_size_ + point.x) * y_size_
           + point.y;
  };
  uint64_t hashCell(const int x, const int y) const
  {
    return (uint64_t) x * y_size_ + y;
  }
  int getGridline(const unsigned dimension, const int index) const
  {
    return gridlines_[dimension][index];
  }
  BoxT getCellBox(PointT point) const;
  BoxT rangeSearchCells(const BoxT& box) const;
  GraphEdge getEdge(const int layer_index, const int x, const int y) const
  {
    return graph_edges_[layer_index][x][y];
  }

  // Costs
  int getEdgeLength(unsigned direction, unsigned edge_index) const;
  CostT getWireCost(int layer_index, PointT u, PointT v) const;
  CostT getViaCost(int layer_index, PointT loc) const;
  CostT getUnitViaCost() const { return unit_via_cost_; }

  // Misc
  void selectAccessPoints(
      GRNet* net,
      robin_hood::unordered_map<uint64_t, std::pair<PointT, IntervalT>>&
          selected_access_points) const;

  // Methods for updating demands
  void commitTree(const std::shared_ptr<GRTreeNode>& tree,
                  bool reverse = false);

  // Checks
  bool checkOverflow(int layer_index, int x, int y) const
  {
    return getEdge(layer_index, x, y).getResource() < 0.0;
  }
  int checkOverflow(int layer_index,
                    PointT u,
                    PointT v) const;  // Check wire overflow
  int checkOverflow(const std::shared_ptr<GRTreeNode>& tree)
      const;  // Check routing tree overflow (Only wires are checked)
  std::string getPythonString(
      const std::shared_ptr<GRTreeNode>& routing_tree) const;

  // 2D maps
  void extractBlockageView(GridGraphView<bool>& view) const;
  void extractCongestionView(
      GridGraphView<bool>& view) const;  // 2D overflow look-up table
  void extractWireCostView(GridGraphView<CostT>& view) const;
  void updateWireCostView(
      GridGraphView<CostT>& view,
      const std::shared_ptr<GRTreeNode>& routing_tree) const;

  // For visualization
  void write(const std::string& heatmap_file = "heatmap.txt") const;

 private:
  const int lib_dbu_;
  int m2_pitch_;

  unsigned num_layers_;
  unsigned x_size_;
  unsigned y_size_;
  std::vector<std::vector<int>> gridlines_;
  std::vector<std::vector<int>> grid_centers_;
  std::vector<std::string> layer_names_;
  std::vector<unsigned> layer_directions_;
  std::vector<int> layer_min_lengths_;

  // Unit costs
  CostT unit_length_wire_cost_;
  CostT unit_via_cost_;
  std::vector<CostT> unit_length_short_costs_;

  int total_length_ = 0;
  int total_num_vias_ = 0;
  std::vector<std::vector<std::vector<GraphEdge>>> graph_edges_;
  // gridEdges[l][x][y] stores the edge {(l, x, y), (l, x+1, y)} or {(l, x, y),
  // (l, x, y+1)} depending on the routing direction of the layer
  Constants constants_;

  IntervalT rangeSearchGridlines(unsigned dimension,
                                 const IntervalT& loc_interval) const;
  // Find the gridlines_ within [locInterval.low, locInterval.high]
  IntervalT rangeSearchRows(unsigned dimension,
                            const IntervalT& loc_interval) const;
  // Find the rows/columns overlapping with [locInterval.low, locInterval.high]

  // Utility functions for cost calculation
  CostT getUnitLengthWireCost() const { return unit_length_wire_cost_; }
  // CostT getUnitViaCost() const { return unit_via_cost_; }
  CostT getUnitLengthShortCost(int layer_index) const
  {
    return unit_length_short_costs_[layer_index];
  }

  double logistic(const CapacityT& input, double slope) const;
  CostT getWireCost(int layer_index,
                    PointT lower,
                    CapacityT demand = 1.0) const;

  // Methods for updating demands
  void commit(int layer_index, PointT lower, CapacityT demand);
  void commitWire(int layer_index, PointT lower, bool reverse = false);
  void commitVia(int layer_index, PointT loc, bool reverse = false);
};

template <typename Type>
class GridGraphView : public std::vector<std::vector<std::vector<Type>>>
{
 public:
  bool check(const PointT& u, const PointT& v) const
  {
    assert(u.x == v.x || u.y == v.y);
    if (u.y == v.y) {
      int l = std::min(u.x, v.x), h = std::max(u.x, v.x);
      for (int x = l; x < h; x++) {
        if ((*this)[MetalLayer::H][x][u.y]) {
          return true;
        }
      }
    } else {
      int l = std::min(u.y, v.y), h = std::max(u.y, v.y);
      for (int y = l; y < h; y++) {
        if ((*this)[MetalLayer::V][u.x][y]) {
          return true;
        }
      }
    }
    return false;
  }

  Type sum(const PointT& u, const PointT& v) const
  {
    assert(u.x == v.x || u.y == v.y);
    Type res = 0;
    if (u.y == v.y) {
      int l = std::min(u.x, v.x), h = std::max(u.x, v.x);
      for (int x = l; x < h; x++) {
        res += (*this)[MetalLayer::H][x][u.y];
      }
    } else {
      int l = std::min(u.y, v.y), h = std::max(u.y, v.y);
      for (int y = l; y < h; y++) {
        res += (*this)[MetalLayer::V][u.x][y];
      }
    }
    return res;
  }
};

}  // namespace grt
