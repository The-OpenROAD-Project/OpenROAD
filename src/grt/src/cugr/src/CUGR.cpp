#include "CUGR.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
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
#include "geo.h"
#include "grt/GRoute.h"
#include "odb/db.h"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace grt {

CUGR::CUGR(odb::dbDatabase* db,
           utl::Logger* log,
           stt::SteinerTreeBuilder* stt_builder)
    : db_(db), logger_(log), stt_builder_(stt_builder)
{
}

CUGR::~CUGR() = default;

void CUGR::init(const int min_routing_layer, const int max_routing_layer)
{
  design_ = std::make_unique<Design>(
      db_, logger_, constants_, min_routing_layer, max_routing_layer);
  grid_graph_ = std::make_unique<GridGraph>(design_.get(), constants_);
  // Instantiate the global routing netlist
  const std::vector<CUGRNet>& baseNets = design_->getAllNets();
  gr_nets_.reserve(baseNets.size());
  for (const CUGRNet& baseNet : baseNets) {
    gr_nets_.push_back(std::make_unique<GRNet>(baseNet, grid_graph_.get()));
  }
}

void CUGR::route()
{
  std::vector<int> netIndices;
  netIndices.reserve(gr_nets_.size());
  for (const auto& net : gr_nets_) {
    netIndices.push_back(net->getIndex());
  }
  // Stage 1: Pattern routing
  logger_->report("stage 1: pattern routing");
  sortNetIndices(netIndices);
  for (const int netIndex : netIndices) {
    PatternRoute patternRoute(
        gr_nets_[netIndex].get(), grid_graph_.get(), stt_builder_, constants_);
    patternRoute.constructSteinerTree();
    patternRoute.constructRoutingDAG();
    patternRoute.run();
    grid_graph_->commitTree(gr_nets_[netIndex]->getRoutingTree());
  }

  netIndices.clear();
  for (const auto& net : gr_nets_) {
    if (grid_graph_->checkOverflow(net->getRoutingTree()) > 0) {
      netIndices.push_back(net->getIndex());
    }
  }
  std::cout << netIndices.size() << " / " << gr_nets_.size()
            << " gr_nets_ have overflows.\n";

  // Stage 2: Pattern routing with possible detours
  if (!netIndices.empty()) {
    logger_->report("stage 2: pattern routing with possible detours");
    GridGraphView<bool>
        congestionView;  // (2d) direction -> x -> y -> has overflow?
    grid_graph_->extractCongestionView(congestionView);
    // for (const int netIndex : netIndices) {
    //     GRNet& net = gr_nets_[netIndex];
    //     grid_graph_->commitTree(net->getRoutingTree(), true);
    // }
    sortNetIndices(netIndices);
    for (const int netIndex : netIndices) {
      GRNet* net = gr_nets_[netIndex].get();
      grid_graph_->commitTree(net->getRoutingTree(), true);
      PatternRoute patternRoute(
          net, grid_graph_.get(), stt_builder_, constants_);
      patternRoute.constructSteinerTree();
      patternRoute.constructRoutingDAG();
      patternRoute.constructDetours(
          congestionView);  // KEY DIFFERENCE compared to stage 1
      patternRoute.run();
      grid_graph_->commitTree(net->getRoutingTree());
    }

    netIndices.clear();
    for (const auto& net : gr_nets_) {
      if (grid_graph_->checkOverflow(net->getRoutingTree()) > 0) {
        netIndices.push_back(net->getIndex());
      }
    }
    std::cout << netIndices.size() << " / " << gr_nets_.size()
              << " gr_nets_ have overflows.\n";
  }

  // Stage 3: maze routing on sparsified routing graph
  if (!netIndices.empty()) {
    std::cout << "stage 3: maze routing on sparsified routing graph\n";
    for (const int netIndex : netIndices) {
      grid_graph_->commitTree(gr_nets_[netIndex]->getRoutingTree(), true);
    }
    GridGraphView<CostT> wireCostView;
    grid_graph_->extractWireCostView(wireCostView);
    sortNetIndices(netIndices);
    SparseGrid grid(10, 10, 0, 0);
    for (const int netIndex : netIndices) {
      GRNet* net = gr_nets_[netIndex].get();
      // grid_graph_->commitTree(net->getRoutingTree(), true);
      // grid_graph_->updateWireCostView(wireCostView, net->getRoutingTree());
      MazeRoute mazeRoute(net, grid_graph_.get());
      mazeRoute.constructSparsifiedGraph(wireCostView, grid);
      mazeRoute.run();
      std::shared_ptr<SteinerTreeNode> tree = mazeRoute.getSteinerTree();
      assert(tree != nullptr);

      PatternRoute patternRoute(
          net, grid_graph_.get(), stt_builder_, constants_);
      patternRoute.setSteinerTree(tree);
      patternRoute.constructRoutingDAG();
      patternRoute.run();

      grid_graph_->commitTree(net->getRoutingTree());
      grid_graph_->updateWireCostView(wireCostView, net->getRoutingTree());
      grid.step();
    }
    netIndices.clear();
    for (const auto& net : gr_nets_) {
      if (grid_graph_->checkOverflow(net->getRoutingTree()) > 0) {
        netIndices.push_back(net->getIndex());
      }
    }
    std::cout << netIndices.size() << " / " << gr_nets_.size()
              << " gr_nets_ have overflows.\n";
  }

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
      ss << grid_graph_->getGridline(0, guide.second.x.low) << " "
         << grid_graph_->getGridline(1, guide.second.y.low) << " "
         << grid_graph_->getGridline(0, guide.second.x.high + 1) << " "
         << grid_graph_->getGridline(1, guide.second.y.high + 1) << " "
         << grid_graph_->getLayerName(guide.first) << "\n";
    }
    ss << ")\n";
  }
  logger_->report("total area of pin access patches: {}", area_of_pin_patches_);
  logger_->report("total area of wire segment patches: {}",
                  area_of_wire_patches_);
  std::ofstream fout(guide_file);
  fout << ss.str();
  fout.close();
}

NetRouteMap CUGR::getRoutes()
{
  NetRouteMap routes;
  for (const auto& net : gr_nets_) {
    if (net->getNumPins() < 2) {
      continue;
    }
    odb::dbNet* db_net = net->getDbNet();
    GRoute& route = routes[db_net];

    const int half_gcell_size = design_->getGridlineSize() / 2;

    auto& routing_tree = net->getRoutingTree();
    if (routing_tree) {
      GRTreeNode::preorder(
          routing_tree, [&](const std::shared_ptr<GRTreeNode>& node) {
            for (const auto& child : node->children) {
              if (node->getLayerIdx() == child->getLayerIdx()) {
                auto [min_x, max_x] = std::minmax(node->x, child->x);
                auto [min_y, max_y] = std::minmax(node->y, child->y);
                GSegment segment(
                    grid_graph_->getGridline(0, min_x) + half_gcell_size,
                    grid_graph_->getGridline(1, min_y) + half_gcell_size,
                    node->getLayerIdx() + 1,
                    grid_graph_->getGridline(0, max_x) + half_gcell_size,
                    grid_graph_->getGridline(1, max_y) + half_gcell_size,
                    child->getLayerIdx() + 1,
                    false);
                route.push_back(segment);
              } else {
                const int bottom_layer
                    = std::min(node->getLayerIdx(), child->getLayerIdx());
                const int top_layer
                    = std::max(node->getLayerIdx(), child->getLayerIdx());
                for (int layer_idx = bottom_layer; layer_idx < top_layer;
                     layer_idx++) {
                  GSegment segment(
                      grid_graph_->getGridline(0, node->x) + half_gcell_size,
                      grid_graph_->getGridline(1, node->y) + half_gcell_size,
                      layer_idx + 1,
                      grid_graph_->getGridline(0, node->x) + half_gcell_size,
                      grid_graph_->getGridline(1, node->y) + half_gcell_size,
                      layer_idx + 2,
                      true);
                  route.push_back(segment);
                }
              }
            }
          });
    }
  }

  return routes;
}

void CUGR::sortNetIndices(std::vector<int>& netIndices) const
{
  std::vector<int> halfParameters(gr_nets_.size());
  for (int netIndex : netIndices) {
    auto& net = gr_nets_[netIndex];
    halfParameters[netIndex] = net->getBoundingBox().hp();
  }
  sort(netIndices.begin(), netIndices.end(), [&](int lhs, int rhs) {
    return halfParameters[lhs] < halfParameters[rhs];
  });
}

void CUGR::getGuides(const GRNet* net,
                     std::vector<std::pair<int, BoxT>>& guides)
{
  auto& routingTree = net->getRoutingTree();
  if (!routingTree) {
    return;
  }
  // 0. Basic guides
  GRTreeNode::preorder(
      routingTree, [&](const std::shared_ptr<GRTreeNode>& node) {
        for (const auto& child : node->children) {
          if (node->getLayerIdx() == child->getLayerIdx()) {
            guides.emplace_back(node->getLayerIdx(),
                                BoxT(std::min(node->x, child->x),
                                     std::min(node->y, child->y),
                                     std::max(node->x, child->x),
                                     std::max(node->y, child->y)));
          } else {
            int maxLayerIndex
                = std::max(node->getLayerIdx(), child->getLayerIdx());
            for (int layerIdx
                 = std::min(node->getLayerIdx(), child->getLayerIdx());
                 layerIdx <= maxLayerIndex;
                 layerIdx++) {
              guides.emplace_back(layerIdx, BoxT(node->x, node->y));
            }
          }
        }
      });

  auto getSpareResource = [&](const GRPoint& point) {
    double resource = std::numeric_limits<double>::max();
    unsigned direction = grid_graph_->getLayerDirection(point.getLayerIdx());
    if (point[direction] + 1 < grid_graph_->getSize(direction)) {
      resource
          = std::min(resource,
                     grid_graph_->getEdge(point.getLayerIdx(), point.x, point.y)
                         .getResource());
    }
    if (point[direction] > 0) {
      GRPoint lower = point;
      lower[direction] -= 1;
      resource
          = std::min(resource,
                     grid_graph_->getEdge(lower.getLayerIdx(), point.x, point.y)
                         .getResource());
    }
    return resource;
  };

  // 1. Pin access patches
  assert(constants_.min_routing_layer + 1 < grid_graph_->getNumLayers());
  for (auto& gpts : net->getPinAccessPoints()) {
    for (auto& gpt : gpts) {
      if (gpt.getLayerIdx() < constants_.min_routing_layer) {
        int padding = 0;
        if (getSpareResource({constants_.min_routing_layer, gpt.x, gpt.y})
            < constants_.pin_patch_threshold) {
          padding = constants_.pin_patch_padding;
        }
        for (int layerIdx = gpt.getLayerIdx();
             layerIdx <= constants_.min_routing_layer + 1;
             layerIdx++) {
          guides.emplace_back(
              layerIdx,
              BoxT(std::max(gpt.x - padding, 0),
                   std::max(gpt.y - padding, 0),
                   std::min(gpt.x + padding, (int) grid_graph_->getSize(0) - 1),
                   std::min(gpt.y + padding,
                            (int) grid_graph_->getSize(1) - 1)));
          area_of_pin_patches_ += (guides.back().second.x.range() + 1)
                                  * (guides.back().second.y.range() + 1);
        }
      }
    }
  }

  // 2. Wire segment patches
  GRTreeNode::preorder(
      routingTree, [&](const std::shared_ptr<GRTreeNode>& node) {
        for (const auto& child : node->children) {
          if (node->getLayerIdx() == child->getLayerIdx()) {
            double wire_patch_threshold = constants_.wire_patch_threshold;
            unsigned direction
                = grid_graph_->getLayerDirection(node->getLayerIdx());
            int l = std::min((*node)[direction], (*child)[direction]);
            int h = std::max((*node)[direction], (*child)[direction]);
            int r = (*node)[1 - direction];
            for (int c = l; c <= h; c++) {
              bool patched = false;
              GRPoint point = (direction == MetalLayer::H
                                   ? GRPoint(node->getLayerIdx(), c, r)
                                   : GRPoint(node->getLayerIdx(), r, c));
              if (getSpareResource(point) < wire_patch_threshold) {
                for (int layerIndex = node->getLayerIdx() - 1;
                     layerIndex <= node->getLayerIdx() + 1;
                     layerIndex += 2) {
                  if (layerIndex < constants_.min_routing_layer
                      || layerIndex >= grid_graph_->getNumLayers()) {
                    continue;
                  }
                  if (getSpareResource({layerIndex, point.x, point.y}) >= 1.0) {
                    guides.emplace_back(layerIndex, BoxT(point.x, point.y));
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
  logger_->report("routing statistics");

  // wire length and via count
  uint64_t wireLength = 0;
  int viaCount = 0;
  std::vector<std::vector<std::vector<int>>> wireUsage;
  wireUsage.assign(grid_graph_->getNumLayers(),
                   std::vector<std::vector<int>>(
                       grid_graph_->getSize(0),
                       std::vector<int>(grid_graph_->getSize(1), 0)));
  for (const auto& net : gr_nets_) {
    GRTreeNode::preorder(
        net->getRoutingTree(), [&](const std::shared_ptr<GRTreeNode>& node) {
          for (const auto& child : node->children) {
            if (node->getLayerIdx() == child->getLayerIdx()) {
              unsigned direction
                  = grid_graph_->getLayerDirection(node->getLayerIdx());
              int l = std::min((*node)[direction], (*child)[direction]);
              int h = std::max((*node)[direction], (*child)[direction]);
              int r = (*node)[1 - direction];
              for (int c = l; c < h; c++) {
                wireLength += grid_graph_->getEdgeLength(direction, c);
                int x = direction == MetalLayer::H ? c : r;
                int y = direction == MetalLayer::H ? r : c;
                wireUsage[node->getLayerIdx()][x][y] += 1;
              }
            } else {
              viaCount += abs(node->getLayerIdx() - child->getLayerIdx());
            }
          }
        });
  }

  // resource
  CapacityT overflow = 0;

  CapacityT minResource = std::numeric_limits<CapacityT>::max();
  GRPoint bottleneck(-1, -1, -1);
  for (int layerIndex = constants_.min_routing_layer;
       layerIndex < grid_graph_->getNumLayers();
       layerIndex++) {
    unsigned direction = grid_graph_->getLayerDirection(layerIndex);
    for (int x = 0; x < grid_graph_->getSize(0) - 1 + direction; x++) {
      for (int y = 0; y < grid_graph_->getSize(1) - direction; y++) {
        CapacityT resource
            = grid_graph_->getEdge(layerIndex, x, y).getResource();
        if (resource < minResource) {
          minResource = resource;
          bottleneck = {layerIndex, x, y};
        }
        CapacityT usage = wireUsage[layerIndex][x][y];
        CapacityT capacity
            = std::max(grid_graph_->getEdge(layerIndex, x, y).capacity, 0.0);
        if (usage > 0.0 && usage > capacity) {
          overflow += usage - capacity;
        }
      }
    }
  }

  std::cout << "wire length (metric):  "
            << wireLength / grid_graph_->getM2Pitch() << "\n";
  std::cout << "total via count:       " << viaCount << "\n";
  std::cout << "total wire overflow:   " << (int) overflow << "\n";

  std::cout << "min resource: " << minResource << "\n";
  std::cout << "bottleneck:   " << bottleneck << "\n";
}

}  // namespace grt
