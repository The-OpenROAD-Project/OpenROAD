#include "CUGR.h"

#include <chrono>

#include "Design.h"
#include "GRNet.h"
#include "GridGraph.h"
#include "MazeRoute.h"
#include "PatternRoute.h"
#include "odb/db.h"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace grt {

CUGR::CUGR(odb::dbDatabase* db,
           utl::Logger* log,
           stt::SteinerTreeBuilder* stt_builder)
    : db_(db), logger_(log), stt_builder_(stt_builder)
{
  design_ = new Design(db_, logger_);
  gridGraph = new GridGraph(design_);
  // Instantiate the global routing netlist
  const std::vector<CUGRNet>& baseNets = design_->getAllNets();
  nets.reserve(baseNets.size());
  for (const CUGRNet& baseNet : baseNets) {
    nets.push_back(new GRNet(baseNet, design_, gridGraph));
  }
}

void CUGR::route()
{
  // int n1 = 0, n2 = 0, n3 = 0;
  // double t1 = 0, t2 = 0, t3 = 0;

  // auto t = std::chrono::high_resolution_clock::now();

  // std::vector<int> netIndices;
  // netIndices.reserve(nets.size());
  // for (const auto& net : nets) {
  //   netIndices.push_back(net.getIndex());
  // }
  // // Stage 1: Pattern routing
  // n1 = netIndices.size();
  // PatternRoute::readFluteLUT();
  // logger_->report("stage 1: pattern routing");
  // sortNetIndices(netIndices);
  // for (const int netIndex : netIndices) {
  //   PatternRoute patternRoute(nets[netIndex], gridGraph);
  //   patternRoute.constructSteinerTree();
  //   patternRoute.constructRoutingDAG();
  //   patternRoute.run();
  //   gridGraph.commitTree(nets[netIndex].getRoutingTree());
  // }

  // netIndices.clear();
  // for (const auto& net : nets) {
  //   if (gridGraph.checkOverflow(net.getRoutingTree()) > 0) {
  //     netIndices.push_back(net.getIndex());
  //   }
  // }
  // logger_->report(netIndices.size()
  //                 << " / " << nets.size() << " nets have overflows.");

  // t1 =
  // std::chrono::duration<double>(std::chrono::high_resolution_clock::now()
  //                                    - t)
  //          .count();
  // t = std::chrono::high_resolution_clock::now();

  // // Stage 2: Pattern routing with possible detours
  // n2 = netIndices.size();
  // if (netIndices.size() > 0) {
  //   logger_->report("stage 2: pattern routing with possible detours");
  //   GridGraphView<bool>
  //       congestionView;  // (2d) direction -> x -> y -> has overflow?
  //   gridGraph.extractCongestionView(congestionView);
  //   // for (const int netIndex : netIndices) {
  //   //     GRNet& net = nets[netIndex];
  //   //     gridGraph.commitTree(net.getRoutingTree(), true);
  //   // }
  //   sortNetIndices(netIndices);
  //   for (const int netIndex : netIndices) {
  //     GRNet& net = nets[netIndex];
  //     gridGraph.commitTree(net.getRoutingTree(), true);
  //     PatternRoute patternRoute(net, gridGraph);
  //     patternRoute.constructSteinerTree();
  //     patternRoute.constructRoutingDAG();
  //     patternRoute.constructDetours(
  //         congestionView);  // KEY DIFFERENCE compared to stage 1
  //     patternRoute.run();
  //     gridGraph.commitTree(net.getRoutingTree());
  //   }

  //   netIndices.clear();
  //   for (const auto& net : nets) {
  //     if (gridGraph.checkOverflow(net.getRoutingTree()) > 0) {
  //       netIndices.push_back(net.getIndex());
  //     }
  //   }
  //   logger_->report(netIndices.size()
  //                   << " / " << nets.size() << " nets have overflows.");
  // }

  // t2 =
  // std::chrono::duration<double>(std::chrono::high_resolution_clock::now()
  //                                    - t)
  //          .count();
  // t = std::chrono::high_resolution_clock::now();

  // // Stage 3: maze routing on sparsified routing graph
  // n3 = netIndices.size();
  // if (netIndices.size() > 0) {
  //   logger_->report("stage 3: maze routing on sparsified routing graph");
  //   for (const int netIndex : netIndices) {
  //     GRNet& net = nets[netIndex];
  //     gridGraph.commitTree(net.getRoutingTree(), true);
  //   }
  //   GridGraphView<CostT> wireCostView;
  //   gridGraph.extractWireCostView(wireCostView);
  //   sortNetIndices(netIndices);
  //   SparseGrid grid(10, 10, 0, 0);
  //   for (const int netIndex : netIndices) {
  //     GRNet& net = nets[netIndex];
  //     // gridGraph.commitTree(net.getRoutingTree(), true);
  //     // gridGraph.updateWireCostView(wireCostView, net.getRoutingTree());
  //     MazeRoute mazeRoute(net, gridGraph);
  //     mazeRoute.constructSparsifiedGraph(wireCostView, grid);
  //     mazeRoute.run();
  //     std::shared_ptr<SteinerTreeNode> tree = mazeRoute.getSteinerTree();
  //     assert(tree != nullptr);

  //     PatternRoute patternRoute(net, gridGraph);
  //     patternRoute.setSteinerTree(tree);
  //     patternRoute.constructRoutingDAG();
  //     patternRoute.run();

  //     gridGraph.commitTree(net.getRoutingTree());
  //     gridGraph.updateWireCostView(wireCostView, net.getRoutingTree());
  //     grid.step();
  //   }
  //   netIndices.clear();
  //   for (const auto& net : nets) {
  //     if (gridGraph.checkOverflow(net.getRoutingTree()) > 0) {
  //       netIndices.push_back(net.getIndex());
  //     }
  //   }
  //   logger_->report(netIndices.size()
  //                   << " / " << nets.size() << " nets have overflows.");
  // }

  // t3 =
  // std::chrono::duration<double>(std::chrono::high_resolution_clock::now()
  //                                    - t)
  //          .count();
  // t = std::chrono::high_resolution_clock::now();

  // // std::cout << "iteration statistics "
  // //     << n1 << " " << std::setprecision(3) << std::fixed << t1 << " "
  // //     << n2 << " " << std::setprecision(3) << std::fixed << t2 << " "
  // //     << n3 << " " << std::setprecision(3) << std::fixed << t3);

  // printStatistics();
  // gridGraph.write();
}

void CUGR::sortNetIndices(std::vector<int>& netIndices) const
{
  // std::vector<int> halfParameters(nets.size());
  // for (int netIndex : netIndices) {
  //   auto& net = nets[netIndex];
  //   halfParameters[netIndex] = net.getBoundingBox().hp();
  // }
  // sort(netIndices.begin(), netIndices.end(), [&](int lhs, int rhs) {
  //   return halfParameters[lhs] < halfParameters[rhs];
  // });
}

void CUGR::getGuides(const GRNet* net,
                     std::vector<std::pair<int, BoxT<int>>>& guides)
{
  // auto& routingTree = net.getRoutingTree();
  // if (!routingTree) {
  //   return;
  // }
  // // 0. Basic guides
  // GRTreeNode::preorder(routingTree, [&](std::shared_ptr<GRTreeNode> node) {
  //   for (const auto& child : node->children) {
  //     if (node->layerIdx == child->layerIdx) {
  //       guides.emplace_back(node->layerIdx,
  //                           BoxT<int>(min(node->x, child->x),
  //                                     std::min(node->y, child->y),
  //                                     std::max(node->x, child->x),
  //                                     std::max(node->y, child->y)));
  //     } else {
  //       int maxLayerIndex = std::max(node->layerIdx, child->layerIdx);
  //       for (int layerIdx = std::min(node->layerIdx, child->layerIdx);
  //            layerIdx <= maxLayerIndex;
  //            layerIdx++) {
  //         guides.emplace_back(layerIdx, BoxT<int>(node->x, node->y));
  //       }
  //     }
  //   }
  // });

  // auto getSpareResource = [&](const GRPoint& point) {
  //   double resource = std::numeric_limits<double>::max();
  //   unsigned direction = gridGraph.getLayerDirection(point.layerIdx);
  //   if (point[direction] + 1 < gridGraph.getSize(direction)) {
  //     resource = std::min(
  //         resource,
  //         gridGraph.getEdge(point.layerIdx, point.x, point.y).getResource());
  //   }
  //   if (point[direction] > 0) {
  //     GRPoint lower = point;
  //     lower[direction] -= 1;
  //     resource = std::min(
  //         resource,
  //         gridGraph.getEdge(lower.layerIdx, point.x, point.y).getResource());
  //   }
  //   return resource;
  // };

  // // 1. Pin access patches
  // assert(min_routing_layer + 1 < gridGraph.getNumLayers());
  // for (auto& gpts : net.getPinAccessPoints()) {
  //   for (auto& gpt : gpts) {
  //     if (gpt.layerIdx < min_routing_layer) {
  //       int padding = 0;
  //       if (getSpareResource({min_routing_layer, gpt.x, gpt.y})
  //           < pin_patch_threshold) {
  //         padding = pin_patch_padding;
  //       }
  //       for (int layerIdx = gpt.layerIdx;
  //            layerIdx <= min_routing_layer + 1;
  //            layerIdx++) {
  //         guides.emplace_back(
  //             layerIdx,
  //             BoxT<int>(max(gpt.x - padding, 0),
  //                       std::max(gpt.y - padding, 0),
  //                       std::min(gpt.x + padding, (int) gridGraph.getSize(0)
  //                       - 1), std::min(gpt.y + padding, (int)
  //                       gridGraph.getSize(1) - 1)));
  //         areaOfPinPatches += (guides.back().second.x.range() + 1)
  //                             * (guides.back().second.y.range() + 1);
  //       }
  //     }
  //   }
  // }

  // // 2. Wire segment patches
  // GRTreeNode::preorder(routingTree, [&](std::shared_ptr<GRTreeNode> node) {
  //   for (const auto& child : node->children) {
  //     if (node->layerIdx == child->layerIdx) {
  //       double wire_patch_threshold = wire_patch_threshold;
  //       unsigned direction = gridGraph.getLayerDirection(node->layerIdx);
  //       int l = std::min((*node)[direction], (*child)[direction]);
  //       int h = std::max((*node)[direction], (*child)[direction]);
  //       int r = (*node)[1 - direction];
  //       for (int c = l; c <= h; c++) {
  //         bool patched = false;
  //         GRPoint point
  //             = (direction == MetalLayer::H ? GRPoint(node->layerIdx, c, r)
  //                                           : GRPoint(node->layerIdx, r, c));
  //         if (getSpareResource(point) < wire_patch_threshold) {
  //           for (int layerIndex = node->layerIdx - 1;
  //                layerIndex <= node->layerIdx + 1;
  //                layerIndex += 2) {
  //             if (layerIndex < min_routing_layer
  //                 || layerIndex >= gridGraph.getNumLayers()) {
  //               continue;
  //             }
  //             if (getSpareResource({layerIndex, point.x, point.y}) >= 1.0) {
  //               guides.emplace_back(layerIndex, BoxT<int>(point.x, point.y));
  //               areaOfWirePatches += 1;
  //               patched = true;
  //             }
  //           }
  //         }
  //         if (patched) {
  //           wire_patch_threshold = wire_patch_threshold;
  //         } else {
  //           wire_patch_threshold *= wire_patch_inflation_rate;
  //         }
  //       }
  //     }
  //   }
  // });
}

void CUGR::printStatistics() const
{
  // logger_->report("routing statistics");

  // // wire length and via count
  // uint64_t wireLength = 0;
  // int viaCount = 0;
  // std::vector<std::vector<std::vector<int>>> wireUsage;
  // wireUsage.assign(
  //     gridGraph.getNumLayers(),
  //     std::vector<std::vector<int>>(gridGraph.getSize(0),
  //                                   std::vector<int>(gridGraph.getSize(1),
  //                                   0)));
  // for (const auto& net : nets) {
  //   GRTreeNode::preorder(
  //       net.getRoutingTree(), [&](std::shared_ptr<GRTreeNode> node) {
  //         for (const auto& child : node->children) {
  //           if (node->layerIdx == child->layerIdx) {
  //             unsigned direction =
  //             gridGraph.getLayerDirection(node->layerIdx); int l =
  //             std::min((*node)[direction], (*child)[direction]); int h =
  //             std::max((*node)[direction], (*child)[direction]); int r =
  //             (*node)[1 - direction]; for (int c = l; c < h; c++) {
  //               wireLength += gridGraph.getEdgeLength(direction, c);
  //               int x = direction == MetalLayer::H ? c : r;
  //               int y = direction == MetalLayer::H ? r : c;
  //               wireUsage[node->layerIdx][x][y] += 1;
  //             }
  //           } else {
  //             viaCount += abs(node->layerIdx - child->layerIdx);
  //           }
  //         }
  //       });
  // }

  // // resource
  // CapacityT overflow = 0;

  // CapacityT minResource = std::numeric_limits<CapacityT>::max();
  // GRPoint bottleneck(-1, -1, -1);
  // for (int layerIndex = min_routing_layer;
  //      layerIndex < gridGraph.getNumLayers();
  //      layerIndex++) {
  //   unsigned direction = gridGraph.getLayerDirection(layerIndex);
  //   for (int x = 0; x < gridGraph.getSize(0) - 1 + direction; x++) {
  //     for (int y = 0; y < gridGraph.getSize(1) - direction; y++) {
  //       CapacityT resource = gridGraph.getEdge(layerIndex, x,
  //       y).getResource(); if (resource < minResource) {
  //         minResource = resource;
  //         bottleneck = {layerIndex, x, y};
  //       }
  //       CapacityT usage = wireUsage[layerIndex][x][y];
  //       CapacityT capacity
  //           = std::max(gridGraph.getEdge(layerIndex, x, y).capacity, 0.0);
  //       if (usage > 0.0 && usage > capacity) {
  //         overflow += usage - capacity;
  //       }
  //     }
  //   }
  // }

  // logger_->report("wire length (metric):  " << wireLength
  //                                                  / gridGraph.getM2Pitch());
  // logger_->report("total via count:       " << viaCount);
  // logger_->report("total wire overflow:   " << (int) overflow);

  // logger_->report("min resource: " << minResource);
  // logger_->report("bottleneck:   " << bottleneck);
}

void CUGR::write(std::string guide_file)
{
  // areaOfPinPatches = 0;
  // areaOfWirePatches = 0;
  // std::stringstream ss;
  // for (const GRNet& net : nets) {
  //   std::vector<std::pair<int, BoxT<int>>> guides;
  //   getGuides(net, guides);

  //   ss << net.getName());
  //   ss << "(" << std::endl;
  //   for (const auto& guide : guides) {
  //     ss << gridGraph.getGridline(0, guide.second.x.low) << " "
  //        << gridGraph.getGridline(1, guide.second.y.low) << " "
  //        << gridGraph.getGridline(0, guide.second.x.high + 1) << " "
  //        << gridGraph.getGridline(1, guide.second.y.high + 1) << " "
  //        << gridGraph.getLayerName(guide.first));
  //   }
  //   ss << ")" << std::endl;
  // }
  // logger_->report("total area of pin access patches: {}", areaOfPinPatches);
  // logger_->report("total area of wire segment patches: {}",
  // areaOfWirePatches);
}

}  // namespace grt
