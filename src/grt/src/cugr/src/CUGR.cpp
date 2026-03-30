#include "CUGR.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

#include "Design.h"
#include "GRNet.h"
#include "GRTree.h"
#include "GridGraph.h"
#include "Layers.h"
#include "MazeRoute.h"
#include "Netlist.h"
#include "PatternRoute.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "geo.h"
#include "grt/GRoute.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/MinMax.hh"
#include "stt/SteinerTreeBuilder.h"
#include "utl/CallBackHandler.h"
#include "utl/Logger.h"

using utl::GRT;

namespace grt {

CUGR::CUGR(odb::dbDatabase* db,
           utl::Logger* log,
           utl::CallBackHandler* callback_handler,
           stt::SteinerTreeBuilder* stt_builder,
           sta::dbSta* sta)
    : db_(db),
      logger_(log),
      callback_handler_(callback_handler),
      stt_builder_(stt_builder),
      sta_(sta)
{
}

CUGR::~CUGR() = default;

void CUGR::init(const int min_routing_layer,
                const int max_routing_layer,
                const std::set<odb::dbNet*>& clock_nets)
{
  design_ = std::make_unique<Design>(db_,
                                     logger_,
                                     constants_,
                                     min_routing_layer,
                                     max_routing_layer,
                                     clock_nets);
  grid_graph_ = std::make_unique<GridGraph>(design_.get(), constants_, logger_);
  // Instantiate the global routing netlist
  const std::vector<CUGRNet>& base_nets = design_->getAllNets();
  gr_nets_.reserve(base_nets.size());
  int index = 0;
  for (const CUGRNet& base_net : base_nets) {
    gr_nets_.push_back(std::make_unique<GRNet>(base_net, grid_graph_.get()));
    net_indices_.push_back(index);
    db_net_map_[base_net.getDbNet()] = gr_nets_.back().get();
    index++;
  }
}

float CUGR::calculatePartialSlack()
{
  std::vector<float> slacks;
  slacks.reserve(gr_nets_.size());
  callback_handler_->triggerOnEstimateParasiticsRequired();
  for (const auto& net : gr_nets_) {
    float slack = getNetSlack(net->getDbNet());
    slacks.push_back(slack);
    net->setSlack(slack);
  }

  std::ranges::stable_sort(slacks);

  // Find the slack threshold based on the percentage of critical nets
  // defined by the user
  const int threshold_index
      = std::ceil(slacks.size() * critical_nets_percentage_ / 100);
  const float slack_th
      = slacks.empty() ? 0.0f
                       : slacks[std::min(static_cast<size_t>(threshold_index),
                                         slacks.size() - 1)];

  // Set the non critical nets slack as the maximum float value, so they can be
  // ordered by the default sorting method.
  for (const int& net_index : net_indices_) {
    if (gr_nets_[net_index]->getSlack() > slack_th) {
      gr_nets_[net_index]->setSlack(
          std::ceil(std::numeric_limits<float>::max()));
    }
  }

  return slack_th;
}

float CUGR::getNetSlack(odb::dbNet* net)
{
  return sta_->slack(net, sta::MinMax::max());
}

void CUGR::setInitialNetSlacks()
{
  for (const auto& net : gr_nets_) {
    float slack = getNetSlack(net->getDbNet());
    net->setSlack(slack);
  }
}

void CUGR::updateOverflowNets(std::vector<int>& net_indices)
{
  net_indices.clear();
  for (const auto& net : gr_nets_) {
    if (net->getRoutingTree()
        && grid_graph_->checkOverflow(net->getRoutingTree()) > 0) {
      net_indices.push_back(net->getIndex());
    }
  }
  logger_->report("Nets with overflow: {}.", net_indices.size());
}

void CUGR::patternRoute(std::vector<int>& net_indices)
{
  logger_->report("Stage 1: Pattern routing.");

  if (critical_nets_percentage_ != 0) {
    setInitialNetSlacks();
  }

  sortNetIndices(net_indices);
  for (const int net_index : net_indices) {
    if (gr_nets_[net_index]->getNumPins() < 2) {
      continue;
    }
    PatternRoute pattern_route(gr_nets_[net_index].get(),
                               grid_graph_.get(),
                               stt_builder_,
                               constants_,
                               logger_);
    pattern_route.constructSteinerTree();
    pattern_route.constructRoutingDAG();
    pattern_route.run();
    grid_graph_->addTreeUsage(gr_nets_[net_index]->getRoutingTree());
  }

  updateOverflowNets(net_indices);
}

void CUGR::patternRouteWithDetours(std::vector<int>& net_indices)
{
  if (net_indices.empty()) {
    return;
  }
  logger_->report("Stage 2: Pattern routing with detours.");

  if (critical_nets_percentage_ != 0) {
    calculatePartialSlack();
  }

  // (2d) direction -> x -> y -> has overflow?
  GridGraphView<bool> congestion_view;
  grid_graph_->extractCongestionView(congestion_view);
  sortNetIndices(net_indices);
  for (const int net_index : net_indices) {
    GRNet* net = gr_nets_[net_index].get();
    if (net->getNumPins() < 2) {
      continue;
    }
    grid_graph_->removeTreeUsage(net->getRoutingTree());
    PatternRoute pattern_route(
        net, grid_graph_.get(), stt_builder_, constants_, logger_);
    pattern_route.constructSteinerTree();
    pattern_route.constructRoutingDAG();
    // KEY DIFFERENCE compared to stage 1 (patternRoute)
    pattern_route.constructDetours(congestion_view);
    pattern_route.run();
    grid_graph_->addTreeUsage(net->getRoutingTree());
  }

  updateOverflowNets(net_indices);
}

void CUGR::mazeRoute(std::vector<int>& net_indices)
{
  if (net_indices.empty()) {
    return;
  }
  logger_->report("Stage 3: Maze routing on sparsified graph.");

  if (critical_nets_percentage_ != 0) {
    calculatePartialSlack();
  }

  for (const int net_index : net_indices) {
    grid_graph_->removeTreeUsage(gr_nets_[net_index]->getRoutingTree());
  }
  GridGraphView<CostT> wire_cost_view;
  grid_graph_->extractWireCostView(wire_cost_view);
  sortNetIndices(net_indices);
  SparseGrid grid(10, 10, 0, 0);
  for (const int net_index : net_indices) {
    GRNet* net = gr_nets_[net_index].get();
    if (net->getNumPins() < 2) {
      continue;
    }
    MazeRoute maze_route(net, grid_graph_.get(), logger_);
    maze_route.constructSparsifiedGraph(wire_cost_view, grid);
    maze_route.run();
    std::shared_ptr<SteinerTreeNode> tree = maze_route.getSteinerTree();
    if (tree == nullptr) {
      logger_->error(GRT,
                     610,
                     "Failed to generate Steiner tree for net {}.",
                     net->getName());
    }

    PatternRoute pattern_route(
        net, grid_graph_.get(), stt_builder_, constants_, logger_);
    pattern_route.setSteinerTree(tree);
    pattern_route.constructRoutingDAG();
    pattern_route.run();

    grid_graph_->addTreeUsage(net->getRoutingTree());
    grid_graph_->updateWireCostView(wire_cost_view, net->getRoutingTree());
    grid.step();
  }

  updateOverflowNets(net_indices);
}

void CUGR::route()
{
  std::vector<int> net_indices;
  if (!nets_to_route_.empty()) {
    net_indices = nets_to_route_;
    nets_to_route_.clear();
  } else {
    net_indices.reserve(gr_nets_.size());
    for (const auto& net : gr_nets_) {
      net_indices.push_back(net->getIndex());
    }
  }

  patternRoute(net_indices);

  patternRouteWithDetours(net_indices);

  mazeRoute(net_indices);

  printStatistics();
  if (constants_.write_heatmap) {
    grid_graph_->write();
  }
}

void CUGR::write(const std::string& guide_file)
{
  area_of_pin_patches_ = 0;
  area_of_wire_patches_ = 0;
  std::stringstream ss;
  for (const auto& net : gr_nets_) {
    std::vector<std::pair<int, BoxT>> guides;
    getGuides(net.get(), guides);

    ss << net->getName() << '\n';
    ss << "(\n";
    for (const auto& guide : guides) {
      ss << grid_graph_->getGridline(0, guide.second.lx()) << " "
         << grid_graph_->getGridline(1, guide.second.ly()) << " "
         << grid_graph_->getGridline(0, guide.second.hx() + 1) << " "
         << grid_graph_->getGridline(1, guide.second.hy() + 1) << " "
         << grid_graph_->getLayerName(guide.first) << "\n";
    }
    ss << ")\n";
  }
  logger_->report("Total area of pin access patches: {}.",
                  area_of_pin_patches_);
  logger_->report("Total area of wire segment patches: {}.",
                  area_of_wire_patches_);
  std::ofstream fout(guide_file);
  fout << ss.str();
  fout.close();
}

NetRouteMap CUGR::getRoutes()
{
  NetRouteMap routes;
  for (const auto& net : gr_nets_) {
    if (net->getNumPins() < 2 || net->isLocal()) {
      continue;
    }
    odb::dbNet* db_net = net->getDbNet();
    GRoute& route = routes[db_net];

    const int half_gcell = design_->getGridlineSize() / 2;

    auto& routing_tree = net->getRoutingTree();
    if (!routing_tree) {
      continue;
    }
    GRTreeNode::preorder(
        routing_tree, [&](const std::shared_ptr<GRTreeNode>& node) {
          for (const auto& child : node->getChildren()) {
            if (node->getLayerIdx() == child->getLayerIdx()) {
              auto [min_x, max_x] = std::minmax({node->x(), child->x()});
              auto [min_y, max_y] = std::minmax({node->y(), child->y()});

              // convert to dbu
              min_x = grid_graph_->getGridline(0, min_x) + half_gcell;
              min_y = grid_graph_->getGridline(1, min_y) + half_gcell;
              max_x = grid_graph_->getGridline(0, max_x) + half_gcell;
              max_y = grid_graph_->getGridline(1, max_y) + half_gcell;

              route.emplace_back(min_x,
                                 min_y,
                                 node->getLayerIdx() + 1,
                                 max_x,
                                 max_y,
                                 child->getLayerIdx() + 1,
                                 false);
              route.back().setIs3DRoute(true);
            } else {
              const auto [bottom_layer, top_layer]
                  = std::minmax({node->getLayerIdx(), child->getLayerIdx()});
              for (int layer_idx = bottom_layer; layer_idx < top_layer;
                   layer_idx++) {
                const int x
                    = grid_graph_->getGridline(0, node->x()) + half_gcell;
                const int y
                    = grid_graph_->getGridline(1, node->y()) + half_gcell;

                route.emplace_back(
                    x, y, layer_idx + 1, x, y, layer_idx + 2, true);
                route.back().setIs3DRoute(true);
              }
            }
          }
        });
  }

  return routes;
}

void CUGR::sortNetIndices(std::vector<int>& net_indices) const
{
  std::ranges::stable_sort(net_indices, [&](int lhs, int rhs) {
    return std::make_tuple(gr_nets_[lhs]->getSlack(),
                           gr_nets_[lhs]->getBoundingBox().hp())
           < std::make_tuple(gr_nets_[rhs]->getSlack(),
                             gr_nets_[rhs]->getBoundingBox().hp());
  });
}

void CUGR::getGuides(const GRNet* net,
                     std::vector<std::pair<int, BoxT>>& guides)
{
  auto& routing_tree = net->getRoutingTree();
  if (!routing_tree) {
    return;
  }
  // 0. Basic guides
  GRTreeNode::preorder(
      routing_tree, [&](const std::shared_ptr<GRTreeNode>& node) {
        for (const auto& child : node->getChildren()) {
          if (node->getLayerIdx() == child->getLayerIdx()) {
            guides.emplace_back(node->getLayerIdx(),
                                BoxT(std::min(node->x(), child->x()),
                                     std::min(node->y(), child->y()),
                                     std::max(node->x(), child->x()),
                                     std::max(node->y(), child->y())));
          } else {
            const int max_layer_index
                = std::max(node->getLayerIdx(), child->getLayerIdx());
            for (int layer_idx
                 = std::min(node->getLayerIdx(), child->getLayerIdx());
                 layer_idx <= max_layer_index;
                 layer_idx++) {
              guides.emplace_back(layer_idx, BoxT(node->x(), node->y()));
            }
          }
        }
      });

  auto get_spare_resource = [&](const GRPoint& point) {
    double resource = std::numeric_limits<double>::max();
    const int direction = grid_graph_->getLayerDirection(point.getLayerIdx());
    if (point[direction] + 1 < grid_graph_->getSize(direction)) {
      resource = std::min(
          resource,
          grid_graph_->getEdge(point.getLayerIdx(), point.x(), point.y())
              .getResource());
    }
    if (point[direction] > 0) {
      GRPoint lower = point;
      lower[direction] -= 1;
      resource = std::min(
          resource,
          grid_graph_->getEdge(lower.getLayerIdx(), point.x(), point.y())
              .getResource());
    }
    return resource;
  };

  // 1. Pin access patches
  if (constants_.min_routing_layer + 1 >= grid_graph_->getNumLayers()) {
    logger_->error(GRT,
                   611,
                   "Min routing layer {} exceeds available layers.",
                   constants_.min_routing_layer);
  }
  for (auto& gpts : net->getPinAccessPoints()) {
    for (auto& gpt : gpts) {
      if (gpt.getLayerIdx() < constants_.min_routing_layer) {
        int padding = 0;
        if (get_spare_resource({constants_.min_routing_layer, gpt.x(), gpt.y()})
            < constants_.pin_patch_threshold) {
          padding = constants_.pin_patch_padding;
        }
        for (int layer_index = gpt.getLayerIdx();
             layer_index <= constants_.min_routing_layer + 1;
             layer_index++) {
          guides.emplace_back(
              layer_index,
              BoxT(std::max(gpt.x() - padding, 0),
                   std::max(gpt.y() - padding, 0),
                   std::min(gpt.x() + padding, grid_graph_->getSize(0) - 1),
                   std::min(gpt.y() + padding, grid_graph_->getSize(1) - 1)));
          area_of_pin_patches_ += (guides.back().second.x().range() + 1)
                                  * (guides.back().second.y().range() + 1);
        }
      }
    }
  }

  // 2. Wire segment patches
  GRTreeNode::preorder(
      routing_tree, [&](const std::shared_ptr<GRTreeNode>& node) {
        for (const auto& child : node->getChildren()) {
          if (node->getLayerIdx() == child->getLayerIdx()) {
            double wire_patch_threshold = constants_.wire_patch_threshold;
            const int direction
                = grid_graph_->getLayerDirection(node->getLayerIdx());
            const int l = std::min((*node)[direction], (*child)[direction]);
            const int h = std::max((*node)[direction], (*child)[direction]);
            const int r = (*node)[1 - direction];
            for (int c = l; c <= h; c++) {
              bool patched = false;
              const GRPoint point = (direction == MetalLayer::H
                                         ? GRPoint(node->getLayerIdx(), c, r)
                                         : GRPoint(node->getLayerIdx(), r, c));
              if (get_spare_resource(point) < wire_patch_threshold) {
                for (int layer_index = node->getLayerIdx() - 1;
                     layer_index <= node->getLayerIdx() + 1;
                     layer_index += 2) {
                  if (layer_index < constants_.min_routing_layer
                      || layer_index >= grid_graph_->getNumLayers()) {
                    continue;
                  }
                  if (get_spare_resource({layer_index, point.x(), point.y()})
                      >= 1.0) {
                    guides.emplace_back(layer_index,
                                        BoxT(point.x(), point.y()));
                    area_of_wire_patches_ += 1;
                    patched = true;
                  }
                }
              }
              if (patched) {
                wire_patch_threshold = constants_.wire_patch_threshold;
              } else {
                wire_patch_threshold *= constants_.wire_patch_inflation_rate;
              }
            }
          }
        }
      });
}

void CUGR::printStatistics() const
{
  logger_->report("Routing statistics");

  // wire length and via count
  uint64_t wire_length = 0;
  int via_count = 0;
  std::vector<std::vector<std::vector<int>>> wire_usage;
  wire_usage.assign(grid_graph_->getNumLayers(),
                    std::vector<std::vector<int>>(
                        grid_graph_->getSize(0),
                        std::vector<int>(grid_graph_->getSize(1), 0)));
  for (const auto& net : gr_nets_) {
    GRTreeNode::preorder(
        net->getRoutingTree(), [&](const std::shared_ptr<GRTreeNode>& node) {
          for (const auto& child : node->getChildren()) {
            if (node->getLayerIdx() == child->getLayerIdx()) {
              const int direction
                  = grid_graph_->getLayerDirection(node->getLayerIdx());
              const int l = std::min((*node)[direction], (*child)[direction]);
              const int h = std::max((*node)[direction], (*child)[direction]);
              const int r = (*node)[1 - direction];
              for (int c = l; c < h; c++) {
                wire_length += grid_graph_->getEdgeLength(direction, c);
                const int x = direction == MetalLayer::H ? c : r;
                const int y = direction == MetalLayer::H ? r : c;
                wire_usage[node->getLayerIdx()][x][y] += 1;
              }
            } else {
              via_count += abs(node->getLayerIdx() - child->getLayerIdx());
            }
          }
        });
  }

  // resource
  CapacityT overflow = 0;

  CapacityT min_resource = std::numeric_limits<CapacityT>::max();
  GRPoint bottleneck(-1, -1, -1);
  for (int layer_index = constants_.min_routing_layer;
       layer_index < grid_graph_->getNumLayers();
       layer_index++) {
    const int direction = grid_graph_->getLayerDirection(layer_index);
    for (int x = 0; x < grid_graph_->getSize(0) - 1 + direction; x++) {
      for (int y = 0; y < grid_graph_->getSize(1) - direction; y++) {
        const CapacityT resource
            = grid_graph_->getEdge(layer_index, x, y).getResource();
        if (resource < min_resource) {
          min_resource = resource;
          bottleneck = {layer_index, x, y};
        }
        const CapacityT usage = wire_usage[layer_index][x][y];
        const CapacityT capacity
            = std::max(grid_graph_->getEdge(layer_index, x, y).capacity, 0.0);
        if (usage > 0.0 && usage > capacity) {
          overflow += usage - capacity;
        }
      }
    }
  }

  logger_->report("Wire length:           {}",
                  wire_length / grid_graph_->getM2Pitch());
  logger_->report("Total via count:       {}", via_count);
  logger_->report("Total wire overflow:   {}", (int) overflow);
  logger_->report("Min resource:          {}", min_resource);
  logger_->report("Bottleneck:            {}", bottleneck);
}

void CUGR::updateDbCongestion()
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::dbGCellGrid* db_gcell = block->getGCellGrid();
  if (db_gcell == nullptr) {
    db_gcell = odb::dbGCellGrid::create(block);
  } else {
    db_gcell->resetGrid();
  }

  const int x_corner = design_->getDieRegion().lx();
  const int y_corner = design_->getDieRegion().ly();
  const int x_size = grid_graph_->getXSize();
  const int y_size = grid_graph_->getYSize();
  const int gridline_size = design_->getGridlineSize();
  db_gcell->addGridPatternX(x_corner, x_size, gridline_size);
  db_gcell->addGridPatternY(y_corner, y_size, gridline_size);

  odb::dbTech* db_tech = db_->getTech();
  for (int layer = 0; layer < grid_graph_->getNumLayers(); layer++) {
    odb::dbTechLayer* db_layer = db_tech->findRoutingLayer(layer + 1);
    if (db_layer == nullptr) {
      continue;
    }

    for (int y = 0; y < y_size; y++) {
      for (int x = 0; x < x_size; x++) {
        const GraphEdge& edge = grid_graph_->getEdge(layer, x, y);
        db_gcell->setCapacity(db_layer, x, y, edge.capacity);
        db_gcell->setUsage(db_layer, x, y, edge.demand);
      }
    }
  }
}

void CUGR::getITermsAccessPoints(
    odb::dbNet* net,
    std::map<odb::dbITerm*, odb::Point3D>& access_points)
{
  GRNet* gr_net = db_net_map_.at(net);
  for (const auto& [iterm, ap] : gr_net->getITermAccessPoints()) {
    const int x = grid_graph_->getGridline(0, ap.point.x());
    const int y = grid_graph_->getGridline(1, ap.point.y());
    access_points[iterm] = odb::Point3D(x, y, ap.layers.high() + 1);
  }
}

void CUGR::getBTermsAccessPoints(
    odb::dbNet* net,
    std::map<odb::dbBTerm*, odb::Point3D>& access_points)
{
  GRNet* gr_net = db_net_map_.at(net);
  for (const auto& [bterm, ap] : gr_net->getBTermAccessPoints()) {
    const int x = grid_graph_->getGridline(0, ap.point.x());
    const int y = grid_graph_->getGridline(1, ap.point.y());
    access_points[bterm] = odb::Point3D(x, y, ap.layers.high() + 1);
  }
}

void CUGR::addDirtyNet(odb::dbNet* net)
{
  auto it = db_net_map_.find(net);
  if (it != db_net_map_.end()) {
    GRNet* gr_net = it->second;
    if (gr_net->getRoutingTree()) {
      grid_graph_->removeTreeUsage(gr_net->getRoutingTree());
    }
    nets_to_route_.push_back(gr_net->getIndex());
  } else {
    logger_->warn(
        GRT, 600, "Net {} not found in CUGR net map.", net->getConstName());
  }
}

void CUGR::updateNet(odb::dbNet* db_net)
{
  auto it = db_net_map_.find(db_net);
  if (it != db_net_map_.end()) {
    GRNet* gr_net = it->second;
    if (gr_net->getRoutingTree()) {
      grid_graph_->removeTreeUsage(gr_net->getRoutingTree());
    }
    design_->updateNet(db_net);
    const int idx = gr_net->getIndex();
    const CUGRNet& base_net = design_->getAllNets()[idx];
    gr_nets_[idx] = std::make_unique<GRNet>(base_net, grid_graph_.get());
    db_net_map_[db_net] = gr_nets_[idx].get();
    nets_to_route_.push_back(idx);
  } else {
    design_->updateNet(db_net);
    const CUGRNet& base_net = design_->getAllNets().back();
    if (base_net.getNumPins() < 2) {
      return;
    }
    const int new_index = static_cast<int>(gr_nets_.size());
    gr_nets_.push_back(std::make_unique<GRNet>(base_net, grid_graph_.get()));
    net_indices_.push_back(new_index);
    db_net_map_[db_net] = gr_nets_.back().get();
    nets_to_route_.push_back(new_index);
  }
}

void CUGR::routeIncremental()
{
  if (nets_to_route_.empty()) {
    return;
  }

  std::vector<int> initial_nets = nets_to_route_;
  std::ranges::sort(initial_nets);
  auto [first, last] = std::ranges::unique(initial_nets);
  initial_nets.erase(first, last);

  route();

  std::vector<int> overflow_nets;
  updateOverflowNets(overflow_nets);
  std::vector<int> secondary_nets;
  std::ranges::set_difference(
      overflow_nets, initial_nets, std::back_inserter(secondary_nets));
  if (!secondary_nets.empty()) {
    for (int idx : secondary_nets) {
      addDirtyNet(gr_nets_[idx]->getDbNet());
    }
    route();
  }

  printStatistics();
}

}  // namespace grt
