#include "GridGraph.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "CUGR.h"
#include "Design.h"
#include "GRNet.h"
#include "GRTree.h"
#include "geo.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "robin_hood.h"
#include "utl/Logger.h"

namespace grt {

GridGraph::GridGraph(const Design* design,
                     const Constants& constants,
                     utl::Logger* logger)
    : logger_(logger),
      gridlines_(design->getGridlines()),
      lib_dbu_(design->getLibDBU()),
      m2_pitch_(design->getLayer(1).getPitch()),
      num_layers_(design->getNumLayers()),
      x_size_(gridlines_[0].size() - 1),
      y_size_(gridlines_[1].size() - 1),
      design_(design),
      constants_(constants)
{
  grid_centers_.resize(2);
  for (int dimension = 0; dimension <= 1; dimension++) {
    grid_centers_[dimension].resize(gridlines_[dimension].size() - 1);
    for (int gridIndex = 0; gridIndex < gridlines_[dimension].size() - 1;
         gridIndex++) {
      grid_centers_[dimension][gridIndex]
          = (gridlines_[dimension][gridIndex]
             + gridlines_[dimension][gridIndex + 1])
            / 2;
    }
  }

  layer_names_.resize(num_layers_);
  layer_directions_.resize(num_layers_);
  layer_min_lengths_.resize(num_layers_);
  for (int layer_index = 0; layer_index < num_layers_; layer_index++) {
    const auto& layer = design->getLayer(layer_index);
    layer_names_[layer_index] = layer.getName();
    layer_directions_[layer_index] = layer.getDirection();
    layer_min_lengths_[layer_index] = layer.getMinLength();
  }

  unit_length_wire_cost_ = design->getUnitLengthWireCost();
  unit_via_cost_ = design->getUnitViaCost();
  unit_length_short_costs_.resize(num_layers_);
  for (int layer_index = 0; layer_index < num_layers_; layer_index++) {
    unit_length_short_costs_[layer_index]
        = design->getUnitLengthShortCost(layer_index);
  }

  // Init grid graph edges
  std::vector<std::vector<int>> gridTracks(num_layers_);
  graph_edges_.assign(num_layers_,
                      std::vector<std::vector<GraphEdge>>(
                          x_size_, std::vector<GraphEdge>(y_size_)));
  for (int layer_index = 0; layer_index < num_layers_; layer_index++) {
    const MetalLayer& layer = design->getLayer(layer_index);
    const int direction = layer.getDirection();

    const int nGrids = gridlines_[1 - direction].size() - 1;
    gridTracks[layer_index].resize(nGrids);
    for (size_t gridIndex = 0; gridIndex < nGrids; gridIndex++) {
      IntervalT locRange(gridlines_[1 - direction][gridIndex],
                         gridlines_[1 - direction][gridIndex + 1]);
      auto trackRange = layer.rangeSearchTracks(locRange);
      if (trackRange.IsValid()) {
        gridTracks[layer_index][gridIndex] = trackRange.range() + 1;
        // exclude the track on the higher gridline
        if (gridIndex != nGrids - 1
            && layer.getTrackLocation(trackRange.high()) == locRange.high()) {
          gridTracks[layer_index][gridIndex]--;
        }
      } else {
        gridTracks[layer_index][gridIndex] = 0;
      }
    }

    // Initialize edges' capacity to the number of tracks
    if (direction == MetalLayer::V) {
      for (size_t x = 0; x < x_size_; x++) {
        const CapacityT nTracks = gridTracks[layer_index][x];
        for (size_t y = 0; y + 1 < y_size_; y++) {
          graph_edges_[layer_index][x][y].capacity = nTracks;
        }
      }
    } else {
      for (size_t y = 0; y < y_size_; y++) {
        const CapacityT nTracks = gridTracks[layer_index][y];
        for (size_t x = 0; x + 1 < x_size_; x++) {
          graph_edges_[layer_index][x][y].capacity = nTracks;
        }
      }
    }
  }

  // Deduct obstacles usage for layers EXCEPT Metal 1
  std::vector<std::vector<BoxT>> obstacles(num_layers_);
  design->getAllObstacles(obstacles, true);
  for (int layer_index = 1; layer_index < num_layers_; layer_index++) {
    const MetalLayer& layer = design->getLayer(layer_index);
    const int direction = layer.getDirection();
    const int nGrids = gridlines_[1 - direction].size() - 1;
    const int nEdges = gridlines_[direction].size() - 2;
    int minEdgeLength = std::numeric_limits<int>::max();
    for (int edge_index = 0; edge_index < nEdges; edge_index++) {
      minEdgeLength = std::min(minEdgeLength,
                               grid_centers_[direction][edge_index + 1]
                                   - grid_centers_[direction][edge_index]);
    }
    std::vector<std::vector<std::shared_ptr<std::pair<BoxT, IntervalT>>>>
        obstaclesInGrid(nGrids);  // obstacle indices sorted in track grids
    // Sort obstacles in track grids
    for (auto& obs : obstacles[layer_index]) {
      const int width = std::min(obs.x().range(), obs.y().range());
      const int spacing
          = layer.getParallelSpacing(
                width, std::min(minEdgeLength, obs[direction].range()))
            + layer.getWidth() / 2 - 1;
      PointT margin(0, 0);
      margin[1 - direction] = spacing;
      const BoxT obsBox(obs.lx() - margin.x(),
                        obs.ly() - margin.y(),
                        obs.hx() + margin.x(),
                        obs.hy() + margin.y());  // enlarged obstacle box
      const IntervalT trackRange
          = layer.rangeSearchTracks(obsBox[1 - direction]);
      const std::shared_ptr<std::pair<BoxT, IntervalT>> obstacle
          = std::make_shared<std::pair<BoxT, IntervalT>>(obsBox, trackRange);
      // Get grid range
      const IntervalT gridRange
          = rangeSearchRows(1 - direction, obsBox[1 - direction]);
      for (int gridIndex = gridRange.low(); gridIndex <= gridRange.high();
           gridIndex++) {
        obstaclesInGrid[gridIndex].push_back(obstacle);
      }
    }
    // Handle each track grid
    IntervalT gridTrackRange;
    for (int gridIndex = 0; gridIndex < nGrids; gridIndex++) {
      if (gridIndex == 0) {
        gridTrackRange.Set(0, gridTracks[layer_index][gridIndex] - 1);
      } else {
        gridTrackRange.SetLow(gridTrackRange.high() + 1);
        gridTrackRange.addToHigh(gridTracks[layer_index][gridIndex]);
      }
      if (!gridTrackRange.IsValid()) {
        continue;
      }
      if (obstaclesInGrid[gridIndex].empty()) {
        continue;
      }
      std::vector<std::vector<std::shared_ptr<std::pair<BoxT, IntervalT>>>>
          obstaclesAtEdge(nEdges);
      for (auto& obstacle : obstaclesInGrid[gridIndex]) {
        const IntervalT gridlineRange
            = rangeSearchGridlines(direction, obstacle->first[direction]);
        const IntervalT edgeRange(std::max(gridlineRange.low() - 2, 0),
                                  std::min(gridlineRange.high(), nEdges - 1));
        for (int edge_index = edgeRange.low(); edge_index <= edgeRange.high();
             edge_index++) {
          obstaclesAtEdge[edge_index].emplace_back(obstacle);
        }
      }
      for (int edge_index = 0; edge_index < nEdges; edge_index++) {
        if (obstaclesAtEdge[edge_index].empty()) {
          continue;
        }
        const int gridline = gridlines_[direction][edge_index + 1];
        const IntervalT edgeInterval(grid_centers_[direction][edge_index],
                                     grid_centers_[direction][edge_index + 1]);
        // Update cpacity
        std::vector<IntervalT> usable_intervals(gridTrackRange.range() + 1,
                                                edgeInterval);
        for (auto& obstacle : obstaclesAtEdge[edge_index]) {
          const IntervalT affectedTrackRange
              = gridTrackRange.IntersectWith(obstacle->second);
          if (!affectedTrackRange.IsValid()) {
            continue;
          }
          for (int trackIndex = affectedTrackRange.low();
               trackIndex <= affectedTrackRange.high();
               trackIndex++) {
            const int tIdx = trackIndex - gridTrackRange.low();
            if (obstacle->first[direction].low() <= gridline
                && obstacle->first[direction].high() >= gridline) {
              // Completely blocked
              usable_intervals[tIdx] = {gridline, gridline};
            } else if (obstacle->first[direction].high() < gridline) {
              usable_intervals[tIdx].SetLow(
                  std::max(usable_intervals[tIdx].low(),
                           obstacle->first[direction].high()));
            } else if (obstacle->first[direction].low() > gridline) {
              usable_intervals[tIdx].SetHigh(
                  std::min(usable_intervals[tIdx].high(),
                           obstacle->first[direction].low()));
            }
          }
        }
        CapacityT capacity = 0;
        for (IntervalT& usable_interval : usable_intervals) {
          capacity
              += (CapacityT) usable_interval.range() / edgeInterval.range();
        }
        if (direction == MetalLayer::V) {
          graph_edges_[layer_index][gridIndex][edge_index].capacity = capacity;
        } else {
          graph_edges_[layer_index][edge_index][gridIndex].capacity = capacity;
        }
      }
    }
  }

  // Apply user-defined capacity adjustment
  for (int layer_index = 0; layer_index < num_layers_; layer_index++) {
    const float adjustment = design->getLayer(layer_index).getAdjustment();
    if (adjustment != 0.0) {
      for (size_t x = 0; x < x_size_; x++) {
        for (size_t y = 0; y < y_size_; y++) {
          graph_edges_[layer_index][x][y].capacity *= (1.0 - adjustment);
        }
      }
    }
  }
}

IntervalT GridGraph::rangeSearchGridlines(const int dimension,
                                          const IntervalT& loc_interval) const
{
  IntervalT range;
  range.Set(lower_bound(gridlines_[dimension].begin(),
                        gridlines_[dimension].end(),
                        loc_interval.low())
                - gridlines_[dimension].begin(),
            lower_bound(gridlines_[dimension].begin(),
                        gridlines_[dimension].end(),
                        loc_interval.high())
                - gridlines_[dimension].begin());
  if (range.high() >= gridlines_[dimension].size()) {
    range.SetHigh(gridlines_[dimension].size() - 1);
  } else if (gridlines_[dimension][range.high()] > loc_interval.high()) {
    range.addToHigh(-1);
  }
  return range;
}

IntervalT GridGraph::rangeSearchRows(const int dimension,
                                     const IntervalT& loc_interval) const
{
  const auto& lineRange = rangeSearchGridlines(dimension, loc_interval);
  return {gridlines_[dimension][lineRange.low()] == loc_interval.low()
              ? lineRange.low()
              : std::max(lineRange.low() - 1, 0),
          gridlines_[dimension][lineRange.high()] == loc_interval.high()
              ? lineRange.high() - 1
              : std::min(lineRange.high(), getSize(dimension) - 1)};
}

BoxT GridGraph::getCellBox(PointT point) const
{
  return {getGridline(0, point.x()),
          getGridline(1, point.y()),
          getGridline(0, point.x() + 1),
          getGridline(1, point.y() + 1)};
}

BoxT GridGraph::rangeSearchCells(const BoxT& box) const
{
  return {rangeSearchRows(0, box[0]), rangeSearchRows(1, box[1])};
}

int GridGraph::getEdgeLength(int direction, int edge_index) const
{
  return grid_centers_[direction][edge_index + 1]
         - grid_centers_[direction][edge_index];
}

double GridGraph::logistic(const CapacityT& input, const double slope) const
{
  return 1.0 / (1.0 + exp(input * slope));
}

CostT GridGraph::getWireCost(const int layer_index,
                             const PointT lower,
                             const CapacityT demand) const
{
  const int direction = layer_directions_[layer_index];
  const int edgeLength = getEdgeLength(direction, lower[direction]);
  const int demandLength = demand * edgeLength;
  const auto& edge = graph_edges_[layer_index][lower.x()][lower.y()];
  CostT cost = demandLength * unit_length_wire_cost_;
  cost += demandLength * unit_length_short_costs_[layer_index]
          * (edge.capacity < 1.0 ? 1.0
                                 : logistic(edge.capacity - edge.demand,
                                            constants_.cost_logistic_slope));
  return cost;
}

CostT GridGraph::getWireCost(const int layer_index,
                             const PointT u,
                             const PointT v) const
{
  const int direction = layer_directions_[layer_index];
  assert(u[1 - direction] == v[1 - direction]);
  CostT cost = 0;
  if (direction == MetalLayer::H) {
    const auto [l, h] = std::minmax({u.x(), v.x()});
    for (int x = l; x < h; x++) {
      cost += getWireCost(layer_index, {x, u.y()});
    }
  } else {
    const auto [l, h] = std::minmax({u.y(), v.y()});
    for (int y = l; y < h; y++) {
      cost += getWireCost(layer_index, {u.x(), y});
    }
  }
  return cost;
}

CostT GridGraph::getViaCost(const int layer_index, const PointT loc) const
{
  assert(layer_index + 1 < num_layers_);
  CostT cost = unit_via_cost_;
  // Estimated wire cost to satisfy min-area
  for (int l = layer_index; l <= layer_index + 1; l++) {
    const int direction = layer_directions_[l];
    PointT lowerLoc = loc;
    lowerLoc[direction] -= 1;
    const int lowerEdgeLength
        = loc[direction] > 0 ? getEdgeLength(direction, lowerLoc[direction])
                             : 0;
    const int higherEdgeLength = loc[direction] < getSize(direction) - 1
                                     ? getEdgeLength(direction, loc[direction])
                                     : 0;
    const CapacityT demand = (CapacityT) layer_min_lengths_[l]
                             / (lowerEdgeLength + higherEdgeLength)
                             * constants_.via_multiplier;
    if (lowerEdgeLength > 0) {
      cost += getWireCost(l, lowerLoc, demand);
    }
    if (higherEdgeLength > 0) {
      cost += getWireCost(l, loc, demand);
    }
  }
  return cost;
}

std::vector<AccessPoint> GridGraph::translateAccessPointsToGrid(
    const std::vector<odb::dbAccessPoint*>& aps,
    const odb::Point& inst_location) const
{
  const int amount_per_x = design_->getDieRegion().hx() / x_size_;
  const int amount_per_y = design_->getDieRegion().hy() / y_size_;
  std::vector<AccessPoint> aps_on_grid;
  for (const auto& ap : aps) {
    odb::Point ap_position = ap->getPoint();
    odb::dbTechLayer* layer = ap->getLayer();

    // Transform AP position according to instance location and orientation.
    odb::dbTransform xform;
    xform.setOffset({inst_location.getX(), inst_location.getY()});
    xform.setOrient(odb::dbOrientType(odb::dbOrientType::R0));
    xform.apply(ap_position);

    const int ap_x = (ap_position.getX() / amount_per_x >= x_size_)
                         ? x_size_ - 1
                         : ap_position.getX() / amount_per_x;
    const int ap_y = ((ap_position.getY() / amount_per_y >= y_size_)
                          ? y_size_ - 1
                          : ap_position.getY() / amount_per_y);
    const PointT selected_point = PointT(ap_x, ap_y);
    const int num_layer = ((layer->getNumber() - 2) > (getNumLayers() - 1))
                              ? getNumLayers() - 1
                              : layer->getNumber() - 2;
    const IntervalT selected_layer = IntervalT(num_layer);
    aps_on_grid.emplace_back(selected_point, selected_layer);
  }

  return aps_on_grid;
}

AccessPoint GridGraph::selectAccessPoint(
    const std::vector<AccessPoint>& access_points) const
{
  AccessPoint best_ap;
  int votes = -1;

  for (const AccessPoint& ap : access_points) {
    int equals = std::count(access_points.begin(), access_points.end(), ap);
    if (equals > votes) {
      votes = equals;
      best_ap = ap;
    }
  }
  return best_ap;
}

bool GridGraph::findODBAccessPoints(
    GRNet* net,
    AccessPointSet& selected_access_points) const
{
  bool has_aps = false;
  std::vector<odb::dbAccessPoint*> access_points;
  odb::dbNet* db_net = net->getDbNet();

  for (odb::dbBTerm* bterm : db_net->getBTerms()) {
    for (const odb::dbBPin* bpin : bterm->getBPins()) {
      const std::vector<odb::dbAccessPoint*>& bpin_pas
          = bpin->getAccessPoints();
      access_points.insert(
          access_points.begin(), bpin_pas.begin(), bpin_pas.end());
    }
    std::vector<AccessPoint> aps_on_grid
        = translateAccessPointsToGrid(access_points, odb::Point(0, 0));
    if (!aps_on_grid.empty()) {
      AccessPoint selected_ap = selectAccessPoint(aps_on_grid);
      selected_access_points.emplace(selected_ap);
      net->addBTermAccessPoint(bterm, selected_ap);
      access_points.clear();
      has_aps = true;
    }
  }

  for (auto iterm : db_net->getITerms()) {
    const auto& pref_access_points = iterm->getPrefAccessPoints();
    if (!pref_access_points.empty()) {
      access_points.insert(access_points.end(),
                           pref_access_points.begin(),
                           pref_access_points.end());
    } else if (!iterm->getInst()->isCore()) {
      // For non-core cells, DRT does not assign preferred APs.
      // Use all APs to ensure the guides covering at least one AP.
      for (const auto& [pin, aps] : iterm->getAccessPoints()) {
        access_points.insert(access_points.end(), aps.begin(), aps.end());
      }
    }

    int x, y;
    iterm->getInst()->getLocation(x, y);
    std::vector<AccessPoint> aps_on_grid
        = translateAccessPointsToGrid(access_points, odb::Point(x, y));
    AccessPoint selected_ap = selectAccessPoint(aps_on_grid);
    if (!aps_on_grid.empty()) {
      selected_access_points.emplace(selected_ap);
      net->addITermAccessPoint(iterm, selected_ap);
      access_points.clear();
      has_aps = true;
    }
  }

  return has_aps;
}

AccessPointSet GridGraph::selectAccessPoints(GRNet* net) const
{
  AccessPointHash hasher(y_size_);
  AccessPointSet selected_access_points(0, hasher);
  // cell hash (2d) -> access point, fixed layer interval
  selected_access_points.reserve(net->getNumPins());
  const auto& boundingBox = net->getBoundingBox();
  const PointT netCenter(boundingBox.cx(), boundingBox.cy());
  // Skips calculations if DRT already created APs in ODB
  if (!findODBAccessPoints(net, selected_access_points)) {
    int pin_idx = 0;
    for (const std::vector<GRPoint>& accessPoints : net->getPinAccessPoints()) {
      std::pair<int, int> bestAccessDist = {0, std::numeric_limits<int>::max()};
      int bestIndex = -1;
      for (int index = 0; index < accessPoints.size(); index++) {
        const GRPoint& point = accessPoints[index];
        int accessibility = 0;
        if (point.getLayerIdx() >= constants_.min_routing_layer) {
          const int direction = getLayerDirection(point.getLayerIdx());
          accessibility
              += getEdge(point.getLayerIdx(), point.x(), point.y()).capacity
                 >= 1;
          if (point[direction] > 0) {
            auto lower = point;
            lower[direction] -= 1;
            accessibility
                += getEdge(lower.getLayerIdx(), lower.x(), lower.y()).capacity
                   >= 1;
          }
        } else {
          accessibility = 1;
        }
        const int distance
            = abs(netCenter.x() - point.x()) + abs(netCenter.y() - point.y());
        if (accessibility > bestAccessDist.first
            || (accessibility == bestAccessDist.first
                && distance < bestAccessDist.second)) {
          bestIndex = index;
          bestAccessDist = {accessibility, distance};
        }
      }
      if (bestAccessDist.first == 0) {
        logger_->warn(utl::GRT, 274, "pin is hard to access.");
      }

      if (bestIndex == -1) {
        logger_->error(utl::GRT,
                       283,
                       "No preferred access point found for pin on net {}.",
                       net->getName());
      }

      const PointT selectedPoint = accessPoints[bestIndex];
      const AccessPoint ap{.point = selectedPoint, .layers = {}};
      auto it = selected_access_points.emplace(ap).first;
      IntervalT& fixedLayerInterval = it->layers;
      for (const auto& point : accessPoints) {
        if (point.x() == selectedPoint.x() && point.y() == selectedPoint.y()) {
          fixedLayerInterval.Update(point.getLayerIdx());
        }
      }

      net->addPreferredAccessPoint(pin_idx, *it);
      pin_idx++;
    }
  }
  return selected_access_points;
}

void GridGraph::commit(const int layer_index,
                       const PointT lower,
                       const CapacityT demand)
{
  graph_edges_[layer_index][lower.x()][lower.y()].demand += demand;
}

void GridGraph::commitWire(const int layer_index,
                           const PointT lower,
                           const bool rip_up)
{
  const int direction = layer_directions_[layer_index];
  const int edgeLength = getEdgeLength(direction, lower[direction]);
  if (rip_up) {
    commit(layer_index, lower, -1);
    total_length_ -= edgeLength;
  } else {
    commit(layer_index, lower, 1);
    total_length_ += edgeLength;
  }
}

void GridGraph::commitVia(const int layer_index,
                          const PointT loc,
                          const bool rip_up)
{
  assert(layer_index + 1 < num_layers_);
  for (int l = layer_index; l <= layer_index + 1; l++) {
    const int direction = layer_directions_[l];
    PointT lowerLoc = loc;
    lowerLoc[direction] -= 1;
    const int lowerEdgeLength
        = loc[direction] > 0 ? getEdgeLength(direction, lowerLoc[direction])
                             : 0;
    const int higherEdgeLength = loc[direction] < getSize(direction) - 1
                                     ? getEdgeLength(direction, loc[direction])
                                     : 0;
    const CapacityT demand = (CapacityT) layer_min_lengths_[l]
                             / (lowerEdgeLength + higherEdgeLength)
                             * constants_.via_multiplier;
    if (lowerEdgeLength > 0) {
      commit(l, lowerLoc, (rip_up ? -demand : demand));
    }
    if (higherEdgeLength > 0) {
      commit(l, loc, (rip_up ? -demand : demand));
    }
  }
  if (rip_up) {
    total_num_vias_ -= 1;
  } else {
    total_num_vias_ += 1;
  }
}

void GridGraph::commitTree(const std::shared_ptr<GRTreeNode>& tree,
                           const bool rip_up)
{
  GRTreeNode::preorder(tree, [&](const std::shared_ptr<GRTreeNode>& node) {
    for (const auto& child : node->getChildren()) {
      if (node->getLayerIdx() == child->getLayerIdx()) {
        const int direction = layer_directions_[node->getLayerIdx()];
        if (direction == MetalLayer::H) {
          assert(node->y() == child->y());
          const auto [l, h] = std::minmax({node->x(), child->x()});
          for (int x = l; x < h; x++) {
            commitWire(node->getLayerIdx(), {x, node->y()}, rip_up);
          }
        } else {
          assert(node->x() == child->x());
          const auto [l, h] = std::minmax({node->y(), child->y()});
          for (int y = l; y < h; y++) {
            commitWire(node->getLayerIdx(), {node->x(), y}, rip_up);
          }
        }
      } else {
        const int maxLayerIndex
            = std::max(node->getLayerIdx(), child->getLayerIdx());
        for (int layerIdx = std::min(node->getLayerIdx(), child->getLayerIdx());
             layerIdx < maxLayerIndex;
             layerIdx++) {
          commitVia(layerIdx, {node->x(), node->y()}, rip_up);
        }
      }
    }
  });
}

int GridGraph::checkOverflow(const int layer_index,
                             const PointT u,
                             const PointT v) const
{
  int num = 0;
  const int direction = layer_directions_[layer_index];
  if (direction == MetalLayer::H) {
    assert(u.y() == v.y());
    const auto [l, h] = std::minmax({u.x(), v.x()});
    for (int x = l; x < h; x++) {
      if (checkOverflow(layer_index, x, u.y())) {
        num++;
      }
    }
  } else {
    assert(u.x() == v.x());
    const auto [l, h] = std::minmax({u.y(), v.y()});
    for (int y = l; y < h; y++) {
      if (checkOverflow(layer_index, u.x(), y)) {
        num++;
      }
    }
  }
  return num;
}

int GridGraph::checkOverflow(const std::shared_ptr<GRTreeNode>& tree) const
{
  if (!tree) {
    return 0;
  }
  int num = 0;
  GRTreeNode::preorder(tree, [&](const std::shared_ptr<GRTreeNode>& node) {
    for (auto& child : node->getChildren()) {
      // Only check wires
      if (node->getLayerIdx() == child->getLayerIdx()) {
        num += checkOverflow(
            node->getLayerIdx(), (PointT) *node, (PointT) *child);
      }
    }
  });
  return num;
}

std::string GridGraph::getPythonString(
    const std::shared_ptr<GRTreeNode>& routing_tree) const
{
  std::vector<std::tuple<PointT, PointT, bool>> edges;
  GRTreeNode::preorder(
      routing_tree, [&](const std::shared_ptr<GRTreeNode>& node) {
        for (auto& child : node->getChildren()) {
          if (node->getLayerIdx() == child->getLayerIdx()) {
            const int direction = getLayerDirection(node->getLayerIdx());
            const int r = (*node)[1 - direction];
            const int l = std::min((*node)[direction], (*child)[direction]);
            const int h = std::max((*node)[direction], (*child)[direction]);
            if (l == h) {
              continue;
            }
            PointT lpoint
                = (direction == MetalLayer::H ? PointT(l, r) : PointT(r, l));
            const PointT hpoint
                = (direction == MetalLayer::H ? PointT(h, r) : PointT(r, h));
            bool congested = false;
            for (int c = l; c < h; c++) {
              const PointT cpoint
                  = (direction == MetalLayer::H ? PointT(c, r) : PointT(r, c));
              if (checkOverflow(node->getLayerIdx(), cpoint.x(), cpoint.y())
                  != congested) {
                if (lpoint != cpoint) {
                  edges.emplace_back(lpoint, cpoint, congested);
                  lpoint = cpoint;
                }
                congested = !congested;
              }
            }
            if (lpoint != hpoint) {
              edges.emplace_back(lpoint, hpoint, congested);
            }
          }
        }
      });
  std::stringstream ss;
  ss << "[";
  for (int i = 0; i < edges.size(); i++) {
    const auto& edge = edges[i];
    ss << "[" << std::get<0>(edge) << ", " << std::get<1>(edge) << ", "
       << (std::get<2>(edge) ? 1 : 0) << "]";
    ss << (i < edges.size() - 1 ? ", " : "]");
  }
  return ss.str();
}

void GridGraph::extractBlockageView(GridGraphView<bool>& view) const
{
  view.assign(2,
              std::vector<std::vector<bool>>(x_size_,
                                             std::vector<bool>(y_size_, true)));
  for (int layer_index = constants_.min_routing_layer;
       layer_index < num_layers_;
       layer_index++) {
    const int direction = getLayerDirection(layer_index);
    for (int x = 0; x < x_size_; x++) {
      for (int y = 0; y < y_size_; y++) {
        if (getEdge(layer_index, x, y).capacity >= 1.0) {
          view[direction][x][y] = false;
        }
      }
    }
  }
}

void GridGraph::extractCongestionView(GridGraphView<bool>& view) const
{
  view.assign(2,
              std::vector<std::vector<bool>>(
                  x_size_, std::vector<bool>(y_size_, false)));
  for (int layer_index = constants_.min_routing_layer;
       layer_index < num_layers_;
       layer_index++) {
    const int direction = getLayerDirection(layer_index);
    for (int x = 0; x < x_size_; x++) {
      for (int y = 0; y < y_size_; y++) {
        if (checkOverflow(layer_index, x, y)) {
          view[direction][x][y] = true;
        }
      }
    }
  }
}

void GridGraph::extractWireCostView(GridGraphView<CostT>& view) const
{
  view.assign(
      2,
      std::vector<std::vector<CostT>>(
          x_size_,
          std::vector<CostT>(y_size_, std::numeric_limits<CostT>::max())));
  for (int direction = 0; direction < 2; direction++) {
    std::vector<int> layerIndices;
    CostT unitLengthShortCost = std::numeric_limits<CostT>::max();
    for (int layer_index = constants_.min_routing_layer;
         layer_index < getNumLayers();
         layer_index++) {
      if (getLayerDirection(layer_index) == direction) {
        layerIndices.emplace_back(layer_index);
        unitLengthShortCost = std::min(unitLengthShortCost,
                                       getUnitLengthShortCost(layer_index));
      }
    }
    for (int x = 0; x < x_size_; x++) {
      for (int y = 0; y < y_size_; y++) {
        const int edge_index = direction == MetalLayer::H ? x : y;
        if (edge_index >= getSize(direction) - 1) {
          continue;
        }
        CapacityT capacity = 0;
        CapacityT demand = 0;
        for (int layer_index : layerIndices) {
          const auto& edge = getEdge(layer_index, x, y);
          capacity += edge.capacity;
          demand += edge.demand;
        }
        const int length = getEdgeLength(direction, edge_index);
        view[direction][x][y]
            = length
              * (unit_length_wire_cost_
                 + unitLengthShortCost
                       * (capacity < 1.0
                              ? 1.0
                              : logistic(capacity - demand,
                                         constants_.maze_logistic_slope)));
      }
    }
  }
}

void GridGraph::updateWireCostView(
    GridGraphView<CostT>& view,
    const std::shared_ptr<GRTreeNode>& routing_tree) const
{
  std::vector<std::vector<int>> sameDirectionLayers(2);
  std::vector<CostT> unitLengthShortCost(2, std::numeric_limits<CostT>::max());
  for (int layer_index = constants_.min_routing_layer;
       layer_index < getNumLayers();
       layer_index++) {
    const int direction = getLayerDirection(layer_index);
    sameDirectionLayers[direction].emplace_back(layer_index);
    unitLengthShortCost[direction] = std::min(
        unitLengthShortCost[direction], getUnitLengthShortCost(layer_index));
  }
  auto update = [&](int direction, int x, int y) {
    const int edge_index = direction == MetalLayer::H ? x : y;
    if (edge_index >= getSize(direction) - 1) {
      return;
    }
    CapacityT capacity = 0;
    CapacityT demand = 0;
    for (int layer_index : sameDirectionLayers[direction]) {
      if (getLayerDirection(layer_index) != direction) {
        continue;
      }
      const auto& edge = getEdge(layer_index, x, y);
      capacity += edge.capacity;
      demand += edge.demand;
    }
    const int length = getEdgeLength(direction, edge_index);
    view[direction][x][y]
        = length
          * (unit_length_wire_cost_
             + unitLengthShortCost[direction]
                   * (capacity < 1.0
                          ? 1.0
                          : logistic(capacity - demand,
                                     constants_.maze_logistic_slope)));
  };
  GRTreeNode::preorder(
      routing_tree, [&](const std::shared_ptr<GRTreeNode>& node) {
        for (const auto& child : node->getChildren()) {
          if (node->getLayerIdx() == child->getLayerIdx()) {
            const int direction = getLayerDirection(node->getLayerIdx());
            if (direction == MetalLayer::H) {
              assert(node->y() == child->y());
              const int l = std::min(node->x(), child->x()),
                        h = std::max(node->x(), child->x());
              for (int x = l; x < h; x++) {
                update(direction, x, node->y());
              }
            } else {
              assert(node->x() == child->x());
              const int l = std::min(node->y(), child->y()),
                        h = std::max(node->y(), child->y());
              for (int y = l; y < h; y++) {
                update(direction, node->x(), y);
              }
            }
          } else {
            const int maxLayerIndex
                = std::max(node->getLayerIdx(), child->getLayerIdx());
            for (int layerIdx
                 = std::min(node->getLayerIdx(), child->getLayerIdx());
                 layerIdx < maxLayerIndex;
                 layerIdx++) {
              const int direction = getLayerDirection(layerIdx);
              update(direction, node->x(), node->y());
              if ((*node)[direction] > 0) {
                update(direction,
                       node->x() - 1 + direction,
                       node->y() - direction);
              }
            }
          }
        }
      });
}

void GridGraph::write(const std::string& heatmap_file) const
{
  logger_->report("writing heatmap to file...");
  std::stringstream ss;

  ss << num_layers_ << " " << x_size_ << " " << y_size_ << '\n';
  for (int layer_index = 0; layer_index < num_layers_; layer_index++) {
    ss << layer_names_[layer_index] << '\n';
    for (int y = 0; y < y_size_; y++) {
      for (int x = 0; x < x_size_; x++) {
        ss << (graph_edges_[layer_index][x][y].capacity
               - graph_edges_[layer_index][x][y].demand)
           << (x == x_size_ - 1 ? "" : " ");
      }
      ss << '\n';
    }
  }
  std::ofstream fout(heatmap_file);
  fout << ss.str();
  fout.close();
}

}  // namespace grt
