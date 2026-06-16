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
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
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
#include "est/ParasiticsService.h"
#include "geo.h"
#include "grt/GRoute.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "sta/MinMax.hh"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"
#include "utl/ServiceRegistry.h"

using utl::GRT;

namespace grt {

namespace {

// Per-3D-edge key used by saveCongestion() to attribute wires and via
// stubs to specific edges.
using EdgeKey = std::tuple<int, int, int>;  // (layer, x, y)
struct EdgeKeyHash
{
  size_t operator()(const EdgeKey& k) const
  {
    // boost::hash_combine pattern.
    size_t h = 0;
    h ^= std::hash<int>{}(std::get<0>(k)) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>{}(std::get<1>(k)) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>{}(std::get<2>(k)) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
  }
};

}  // namespace

CUGR::CUGR(odb::dbDatabase* db,
           utl::Logger* log,
           utl::ServiceRegistry* service_registry,
           stt::SteinerTreeBuilder* stt_builder,
           sta::dbSta* sta)
    : db_(db),
      logger_(log),
      service_registry_(service_registry),
      stt_builder_(stt_builder),
      sta_(sta)
{
}

CUGR::~CUGR() = default;

void CUGR::init(const int min_routing_layer,
                const int max_routing_layer,
                const odb::PtrSet<odb::dbNet>& clock_nets)
{
  constants_.min_routing_layer = min_routing_layer - 1;
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
    if (!base_net.isValid()) {
      gr_nets_.push_back(nullptr);
      net_indices_.push_back(index);
      index++;
      continue;
    }
    gr_nets_.push_back(std::make_unique<GRNet>(base_net, grid_graph_.get()));
    gr_nets_.back()->setNdrCosts(computeNdrCosts(base_net.getDbNet()));
    net_indices_.push_back(index);
    db_net_map_[base_net.getDbNet()] = gr_nets_.back().get();
    index++;
  }
}

float CUGR::calculatePartialSlack()
{
  std::vector<float> slacks;
  slacks.reserve(gr_nets_.size());
  if (auto* estimator = service_registry_->find<est::ParasiticsService>()) {
    estimator->estimateAllGlobalRouteParasitics();
  }
  for (const auto& net : gr_nets_) {
    if (net == nullptr) {
      continue;
    }
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
    if (gr_nets_[net_index] == nullptr) {
      continue;
    }
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
    if (net == nullptr) {
      continue;
    }
    float slack = getNetSlack(net->getDbNet());
    net->setSlack(slack);
  }
}

std::vector<double> CUGR::computeNdrCosts(odb::dbNet* db_net) const
{
  const int num_layers = grid_graph_->getNumLayers();
  std::vector<double> factors(std::max(num_layers, 0), 1.0);
  odb::dbTechNonDefaultRule* ndr = db_net->getNonDefaultRule();
  if (ndr == nullptr) {
    return factors;
  }
  std::vector<odb::dbTechLayerRule*> layer_rules;
  ndr->getLayerRules(layer_rules);
  for (odb::dbTechLayerRule* lr : layer_rules) {
    odb::dbTechLayer* tl = lr->getLayer();
    if (tl == nullptr || tl->getType() != odb::dbTechLayerType::ROUTING) {
      continue;
    }
    const int layer_idx = tl->getRoutingLevel() - 1;  // 0-based
    if (layer_idx < 0 || layer_idx >= num_layers) {
      continue;
    }
    const int default_width = tl->getWidth();
    const int default_pitch = tl->getPitch();
    if (default_pitch <= 0) {
      continue;
    }
    // Equivalent to (W/2 + S + D/2) / P; multiplied through by 2
    // to avoid integer truncation on odd-DBU widths.
    const double f = static_cast<double>(lr->getWidth() + 2 * lr->getSpacing()
                                         + default_width)
                     / static_cast<double>(2 * default_pitch);
    factors[layer_idx] = std::max(1.0, f);
  }
  return factors;
}

void CUGR::updateCongestedNets(std::vector<int>& net_indices,
                               const double threshold)
{
  net_indices.clear();
  for (const auto& net : gr_nets_) {
    if (net == nullptr) {
      continue;
    }
    if (!net->getRoutingTree()) {
      continue;
    }
    if (grid_graph_->checkCongestion(net->getRoutingTree(), threshold) > 0) {
      net_indices.push_back(net->getIndex());
    }
  }
  debugPrint(
      logger_, GRT, "rrr", 1, "Nets with congestion: {}.", net_indices.size());
}

void CUGR::patternRoute(std::vector<int>& net_indices)
{
  logger_->report("Stage 1: Pattern routing.");

  if (critical_nets_percentage_ != 0) {
    setInitialNetSlacks();
  }

  sortNetIndices(net_indices);
  for (const int net_index : net_indices) {
    if (gr_nets_[net_index] == nullptr) {
      continue;
    }
    if (gr_nets_[net_index]->getNumPins() < 2) {
      continue;
    }
    if (gr_nets_[net_index] == nullptr) {
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
    if (gr_nets_[net_index] == nullptr) {
      continue;
    }
    grid_graph_->addTreeUsage(gr_nets_[net_index]->getRoutingTree(),
                              gr_nets_[net_index]->getNdrCosts());
  }

  updateCongestedNets(net_indices);
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
    if (gr_nets_[net_index] == nullptr) {
      continue;
    }
    GRNet* net = gr_nets_[net_index].get();
    if (net->getNumPins() < 2) {
      continue;
    }
    grid_graph_->removeTreeUsage(net->getRoutingTree(), net->getNdrCosts());
    PatternRoute pattern_route(
        net, grid_graph_.get(), stt_builder_, constants_, logger_);
    pattern_route.constructSteinerTree();
    pattern_route.constructRoutingDAG();
    // KEY DIFFERENCE compared to stage 1 (patternRoute)
    pattern_route.constructDetours(congestion_view);
    pattern_route.run();
    grid_graph_->addTreeUsage(net->getRoutingTree(), net->getNdrCosts());
  }

  updateCongestedNets(net_indices);
}

void CUGR::mazeRoute(std::vector<int>& net_indices)
{
  if (net_indices.empty()) {
    return;
  }

  if (critical_nets_percentage_ != 0) {
    calculatePartialSlack();
  }

  for (const int net_index : net_indices) {
    if (gr_nets_[net_index] == nullptr) {
      continue;
    }
    grid_graph_->removeTreeUsage(gr_nets_[net_index]->getRoutingTree(),
                                 gr_nets_[net_index]->getNdrCosts());
  }
  GridGraphView<CostT> wire_cost_view;
  grid_graph_->extractWireCostView(wire_cost_view);
  sortNetIndices(net_indices);
  SparseGrid grid(10, 10, 0, 0);
  // Hoisted to reuse storage across NDR nets.
  GridGraphView<CostT> ndr_wire_cost_view;
  for (const int net_index : net_indices) {
    if (gr_nets_[net_index] == nullptr) {
      continue;
    }
    GRNet* net = gr_nets_[net_index].get();
    if (net->getNumPins() < 2) {
      continue;
    }
    // NDR nets need a per-net cost view; the shared one is NDR-blind.
    if (net->hasNdr()) {
      grid_graph_->extractWireCostView(ndr_wire_cost_view, net->getNdrCosts());
    }
    const GridGraphView<CostT>& view_for_net
        = net->hasNdr() ? ndr_wire_cost_view : wire_cost_view;
    MazeRoute maze_route(net, grid_graph_.get(), logger_);
    maze_route.constructSparsifiedGraph(view_for_net, grid);
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

    grid_graph_->addTreeUsage(net->getRoutingTree(), net->getNdrCosts());
    grid_graph_->updateWireCostView(wire_cost_view, net->getRoutingTree());
    grid.step();
  }

  updateCongestedNets(net_indices);
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
      if (net == nullptr) {
        continue;
      }
      net_indices.push_back(net->getIndex());
    }
  }

  patternRoute(net_indices);

  patternRouteWithDetours(net_indices);

  if (!net_indices.empty()) {
    logger_->report("Stage 3: Maze routing on sparsified graph.");
  }
  mazeRoute(net_indices);

  iterativeRRR(net_indices);

  printStatistics();
  debugCongestion2D();
  if (constants_.write_heatmap) {
    grid_graph_->write();
  }
}

void CUGR::debugCongestion2D() const
{
  if (!logger_->debugCheck(utl::GRT, "rrr_2d", 1)) {
    return;
  }

  const int x_size = grid_graph_->getXSize();
  const int y_size = grid_graph_->getYSize();
  const int num_layers = grid_graph_->getNumLayers();

  double total_3d_overflow = 0.0;
  double total_2d_overflow = 0.0;
  int tiles_3d_only = 0;
  int tiles_2d = 0;

  for (int direction = 0; direction < 2; ++direction) {
    std::vector<int> same_dir_layers;
    for (int l = constants_.min_routing_layer; l < num_layers; ++l) {
      if (grid_graph_->getLayerDirection(l) == direction) {
        same_dir_layers.push_back(l);
      }
    }
    if (same_dir_layers.empty()) {
      continue;
    }

    // For an H layer, an edge spans gcells (x, y)→(x+1, y), so the
    // valid edge index range is x < x_size - 1. Mirror for V.
    const int x_max = (direction == MetalLayer::H) ? x_size - 1 : x_size;
    const int y_max = (direction == MetalLayer::H) ? y_size : y_size - 1;

    for (int x = 0; x < x_max; ++x) {
      for (int y = 0; y < y_max; ++y) {
        double sum_cap = 0.0;
        double sum_dem = 0.0;
        double per_layer_overflow_sum = 0.0;
        for (int l : same_dir_layers) {
          const auto& edge = grid_graph_->getEdge(l, x, y);
          sum_cap += std::max(edge.capacity, 0.0);
          sum_dem += edge.demand;
          const double ovf = edge.demand - edge.capacity;
          if (ovf > 0.0) {
            per_layer_overflow_sum += ovf;
          }
        }
        const double tile_2d_overflow = std::max(0.0, sum_dem - sum_cap);
        total_3d_overflow += per_layer_overflow_sum;
        total_2d_overflow += tile_2d_overflow;
        if (tile_2d_overflow > 0.0) {
          ++tiles_2d;
        } else if (per_layer_overflow_sum > 0.0) {
          ++tiles_3d_only;
        }
      }
    }
  }

  const auto rnd = [](double v) { return static_cast<int>(std::round(v)); };
  const int spreadable = rnd(total_3d_overflow - total_2d_overflow);
  debugPrint(logger_, GRT, "rrr_2d", 1, "2D-aggregate congestion check:");
  debugPrint(logger_,
             GRT,
             "rrr_2d",
             1,
             "  3D overflow:               {} units",
             rnd(total_3d_overflow));
  debugPrint(logger_,
             GRT,
             "rrr_2d",
             1,
             "  2D-aggregate overflow:     {} units (unavoidable)",
             rnd(total_2d_overflow));
  debugPrint(
      logger_,
      GRT,
      "rrr_2d",
      1,
      "  Spreadable overflow:       {} units (could move to other layers)",
      spreadable);
  debugPrint(logger_,
             GRT,
             "rrr_2d",
             1,
             "  Tiles with 3D-only ovf:    {}",
             tiles_3d_only);
  debugPrint(logger_,
             GRT,
             "rrr_2d",
             1,
             "  Tiles with 2D ovf:         {} (true planar congestion)",
             tiles_2d);
}

void CUGR::iterativeRRR(std::vector<int>& net_indices)
{
  // Gate on the integer overflow metric (the one users see in
  // printStatistics and GRT-0096). Sub-1 fractional overflow rounds to 0
  // and cannot be driven lower by RRR, so don't waste iterations on it.
  if (totalOverflow() == 0) {
    return;
  }

  // Multiplier ramps up to saturate around slope=6 — beyond that the
  // logistic cost surface degenerates into a step function with no
  // gradient information, and the maze starts thrashing.
  constexpr double kMultiplierStep = 1.0;
  constexpr double kMultiplierCap = 6.0;
  constexpr double kCongestionThreshold = 0.9;
  // Iterations in the congested set before an NDR net is demoted.
  constexpr int kSoftNdrStreakThreshold = 2;

  std::unordered_map<int, int> ndr_congested_streak;
  int soft_ndr_demotions = 0;

  double multiplier = 1.0;
  for (int i = 1; i <= congestion_iterations_; ++i) {
    updateCongestedNets(net_indices, kCongestionThreshold);
    if (net_indices.empty()) {
      break;
    }

    // Soft-NDR escape valve.
    std::unordered_set<int> currently_congested(net_indices.begin(),
                                                net_indices.end());
    std::vector<std::string> demoted_now;
    for (const int net_index : net_indices) {
      if (gr_nets_[net_index] == nullptr) {
        continue;
      }
      GRNet* net = gr_nets_[net_index].get();
      if (!net->hasNdr()) {
        continue;
      }
      const int streak = ++ndr_congested_streak[net_index];
      if (streak >= kSoftNdrStreakThreshold) {
        grid_graph_->removeTreeUsage(net->getRoutingTree(), net->getNdrCosts());
        net->setSoftNdr();
        grid_graph_->addTreeUsage(net->getRoutingTree(), net->getNdrCosts());
        demoted_now.push_back(net->getName());
      }
    }
    // Reset streaks for nets that recovered (no longer congested).
    for (auto& [net_index, streak] : ndr_congested_streak) {
      if (!currently_congested.contains(net_index)) {
        streak = 0;
      }
    }
    if (!demoted_now.empty()) {
      soft_ndr_demotions += demoted_now.size();
      for (const std::string& name : demoted_now) {
        debugPrint(logger_,
                   GRT,
                   "softNDR",
                   1,
                   "Disabled NDR (to reduce congestion) for net: {}",
                   name);
      }
      logger_->warn(GRT,
                    305,
                    "Demoted {} NDR net(s) to default rule to reduce "
                    "congestion (use debug 'softNDR' for net list).",
                    demoted_now.size());
    }

    if (multiplier < kMultiplierCap) {
      multiplier += kMultiplierStep;
    }
    grid_graph_->setCostMultiplier(multiplier);
    logger_->info(
        GRT, 117, "Start extra iteration {}/{}", i, congestion_iterations_);
    mazeRoute(net_indices);
  }
  grid_graph_->setCostMultiplier(1.0);
  if (soft_ndr_demotions > 0) {
    logger_->info(GRT,
                  306,
                  "Iterative RRR soft-demoted {} NDR net(s) total.",
                  soft_ndr_demotions);
  }

  // Final summary: the last mazeRoute already printed "Nets with
  // congestion" via updateCongestedNets, so just warn (if anything remains)
  // using the same metric without re-printing the count.
  if (const int residual = totalOverflow(); residual > 0) {
    logger_->warn(GRT,
                  118,
                  "Iterative RRR finished with congestion remaining ({}).",
                  residual);
  }
}

void CUGR::write(const std::string& guide_file)
{
  area_of_pin_patches_ = 0;
  area_of_wire_patches_ = 0;
  std::stringstream ss;
  for (const auto& net : gr_nets_) {
    if (net == nullptr) {
      continue;
    }
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
    if (net == nullptr) {
      continue;
    }
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
    if (gr_nets_[lhs] == nullptr || gr_nets_[rhs] == nullptr) {
      return false;
    }
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
  for (const auto& net : gr_nets_) {
    if (net == nullptr) {
      continue;
    }
    GRTreeNode::preorder(
        net->getRoutingTree(), [&](const std::shared_ptr<GRTreeNode>& node) {
          for (const auto& child : node->getChildren()) {
            if (node->getLayerIdx() == child->getLayerIdx()) {
              const int direction
                  = grid_graph_->getLayerDirection(node->getLayerIdx());
              const int l = std::min((*node)[direction], (*child)[direction]);
              const int h = std::max((*node)[direction], (*child)[direction]);
              for (int c = l; c < h; c++) {
                wire_length += grid_graph_->getEdgeLength(direction, c);
              }
            } else {
              via_count += abs(node->getLayerIdx() - child->getLayerIdx());
            }
          }
        });
  }

  // Overflow is computed from edge.demand (which includes via-stub
  // demand). This is the same metric used by CUGR's checkCongestion,
  // updateCongestedNets, and extractCongestionView.
  CapacityT total_overflow = 0;
  CapacityT min_resource = std::numeric_limits<CapacityT>::max();
  GRPoint bottleneck(-1, -1, -1);
  for (int layer_index = constants_.min_routing_layer;
       layer_index < grid_graph_->getNumLayers();
       layer_index++) {
    const int direction = grid_graph_->getLayerDirection(layer_index);
    for (int x = 0; x < grid_graph_->getSize(0) - 1 + direction; x++) {
      for (int y = 0; y < grid_graph_->getSize(1) - direction; y++) {
        const auto& edge = grid_graph_->getEdge(layer_index, x, y);
        const CapacityT resource = edge.getResource();
        if (resource < min_resource) {
          min_resource = resource;
          bottleneck = {layer_index, x, y};
        }
        const CapacityT capacity = std::max(edge.capacity, 0.0);
        if (edge.demand > capacity) {
          total_overflow += edge.demand - capacity;
        }
      }
    }
  }

  logger_->report("Wire length:           {}",
                  wire_length / grid_graph_->getM2Pitch());
  logger_->report("Total via count:       {}", via_count);
  logger_->report("Total congestion:      {}", (int) total_overflow);
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
  // Skip sub-min layers: they hold no routing wire and their 0-capacity edges
  // would otherwise export as fully-blocked cells.
  for (int layer = constants_.min_routing_layer;
       layer < grid_graph_->getNumLayers();
       layer++) {
    odb::dbTechLayer* db_layer = db_tech->findRoutingLayer(layer + 1);
    if (db_layer == nullptr) {
      continue;
    }

    // Write per-cell capacity/usage matching FastRoute's semantics:
    //   capacity = original (pre-blockage, pre-adjustment) track count
    //   usage    = routing demand + lost capacity (blockage + adjustment)
    // Boundary cells without a corresponding edge replicate the previous
    // cell's value, again matching FastRoute's last_cell_cap fall-through.
    // Iterate along the layer's preferred direction in the inner loop so
    // the fall-through carries forward correctly for both H and V.
    const int direction = grid_graph_->getLayerDirection(layer);
    const bool is_h = direction == MetalLayer::H;
    const int outer_size = is_h ? y_size : x_size;
    const int inner_size = is_h ? x_size : y_size;
    for (int o = 0; o < outer_size; o++) {
      double last_cap = 0;
      double last_use = 0;
      for (int i = 0; i < inner_size; i++) {
        const int x = is_h ? i : o;
        const int y = is_h ? o : i;
        double cap, use;
        if (i == inner_size - 1) {
          cap = last_cap;
          use = last_use;
        } else {
          const GraphEdge& edge = grid_graph_->getEdge(layer, x, y);
          const double initial
              = grid_graph_->getInitialEdgeCapacity(layer, x, y);
          cap = initial;
          use = edge.demand + (initial - edge.capacity);
        }
        db_gcell->setCapacity(db_layer, x, y, cap);
        db_gcell->setUsage(db_layer, x, y, use);
        last_cap = cap;
        last_use = use;
      }
    }
  }
}

void CUGR::getITermsAccessPoints(
    odb::dbNet* net,
    odb::PtrMap<odb::dbITerm, odb::Point3D>& access_points)
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
    odb::PtrMap<odb::dbBTerm, odb::Point3D>& access_points)
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
      grid_graph_->removeTreeUsage(gr_net->getRoutingTree(),
                                   gr_net->getNdrCosts());
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
      grid_graph_->removeTreeUsage(gr_net->getRoutingTree(),
                                   gr_net->getNdrCosts());
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

void CUGR::removeNet(odb::dbNet* db_net)
{
  auto it = db_net_map_.find(db_net);
  if (it == db_net_map_.end()) {
    design_->removeNet(db_net);
    return;
  }

  GRNet* gr_net = it->second;
  if (gr_net->getRoutingTree()) {
    grid_graph_->removeTreeUsage(gr_net->getRoutingTree(),
                                 gr_net->getNdrCosts());
  }

  int index = gr_net->getIndex();
  gr_nets_[index] = nullptr;
  db_net_map_.erase(it);

  design_->removeNet(db_net);
}
const std::vector<int>& CUGR::getOriginalResources() const
{
  return grid_graph_->getOriginalResources();
}

void CUGR::computeCongestionInformation()
{
  grid_graph_->computeCongestionInformation();
}

const std::vector<int>& CUGR::getTotalCapacityPerLayer() const
{
  return grid_graph_->getTotalCapacityPerLayer();
}

const std::vector<int>& CUGR::getTotalUsagePerLayer() const
{
  return grid_graph_->getTotalUsagePerLayer();
}

const std::vector<int>& CUGR::getTotalOverflowPerLayer() const
{
  return grid_graph_->getTotalOverflowPerLayer();
}

const std::vector<int>& CUGR::getMaxHorizontalOverflows() const
{
  return grid_graph_->getMaxHorizontalOverflows();
}

const std::vector<int>& CUGR::getMaxVerticalOverflows() const
{
  return grid_graph_->getMaxVerticalOverflows();
}

int CUGR::totalOverflow()
{
  if (!grid_graph_) {
    return 0;
  }
  grid_graph_->computeCongestionInformation();
  int total = 0;
  for (int layer_overflow : grid_graph_->getTotalOverflowPerLayer()) {
    total += layer_overflow;
  }
  return total;
}

void CUGR::saveCongestion()
{
  if (!grid_graph_ || !design_) {
    return;
  }

  // Always start from a fresh "Global route" tree so that stale markers
  // from a previous routing pass don't linger in the GUI / .rpt when the
  // current pass has no overflow.
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::dbMarkerCategory* tool_category
      = odb::dbMarkerCategory::createOrReplace(block, "Global route");
  tool_category->setSource("GRT");

  if (totalOverflow() == 0) {
    return;
  }

  const int x_size = grid_graph_->getXSize();
  const int y_size = grid_graph_->getYSize();
  const int num_layers = grid_graph_->getNumLayers();

  // Walk every routed net's tree to gather per 3D edge (layer, x, y):
  //   - wire_count: number of same-layer wire segments crossing the edge
  //   - wire_nets:  the nets that own those wires
  //   - via_nets:   the nets that own vias whose stub demand commitVia()
  //                 attributes to this edge (the same neighbours commitVia
  //                 itself touches)
  std::unordered_map<EdgeKey, int, EdgeKeyHash> wire_count;
  std::unordered_map<EdgeKey, odb::PtrSet<odb::dbNet>, EdgeKeyHash> wire_nets;
  std::unordered_map<EdgeKey, odb::PtrSet<odb::dbNet>, EdgeKeyHash> via_nets;

  auto attribute_via = [&](int via_layer, int vx, int vy, odb::dbNet* db_net) {
    // commitVia(via_layer, loc) adds stub demand on edges adjacent to
    // (vx, vy) on layers via_layer and via_layer+1, in each layer's
    // preferred direction. Mirror that to know which nets touched
    // which edges.
    for (int l = via_layer; l <= via_layer + 1 && l < num_layers; l++) {
      const int dir = grid_graph_->getLayerDirection(l);
      if (dir == MetalLayer::H) {
        if (vx > 0) {
          via_nets[{l, vx - 1, vy}].insert(db_net);
        }
        if (vx + 1 < x_size) {
          via_nets[{l, vx, vy}].insert(db_net);
        }
      } else {
        if (vy > 0) {
          via_nets[{l, vx, vy - 1}].insert(db_net);
        }
        if (vy + 1 < y_size) {
          via_nets[{l, vx, vy}].insert(db_net);
        }
      }
    }
  };

  for (const auto& gr_net : gr_nets_) {
    if (gr_net == nullptr) {
      continue;
    }
    if (!gr_net->getRoutingTree()) {
      continue;
    }
    odb::dbNet* db_net = gr_net->getDbNet();
    GRTreeNode::preorder(
        gr_net->getRoutingTree(), [&](const std::shared_ptr<GRTreeNode>& node) {
          for (const auto& child : node->getChildren()) {
            if (node->getLayerIdx() == child->getLayerIdx()) {
              const int l = node->getLayerIdx();
              const int dir = grid_graph_->getLayerDirection(l);
              if (dir == MetalLayer::H) {
                const int y = node->y();
                const auto [lx, hx] = std::minmax({node->x(), child->x()});
                for (int x = lx; x < hx; x++) {
                  wire_count[{l, x, y}]++;
                  wire_nets[{l, x, y}].insert(db_net);
                }
              } else {
                const int x = node->x();
                const auto [ly, hy] = std::minmax({node->y(), child->y()});
                for (int y = ly; y < hy; y++) {
                  wire_count[{l, x, y}]++;
                  wire_nets[{l, x, y}].insert(db_net);
                }
              }
            } else {
              const int min_l
                  = std::min(node->getLayerIdx(), child->getLayerIdx());
              const int max_l
                  = std::max(node->getLayerIdx(), child->getLayerIdx());
              for (int via_l = min_l; via_l < max_l; via_l++) {
                attribute_via(via_l, node->x(), node->y(), db_net);
              }
            }
          }
        });
  }

  odb::dbMarkerCategory* h_subcat = nullptr;
  odb::dbMarkerCategory* v_subcat = nullptr;

  odb::dbTech* tech = db_->getTech();
  const int gridline_size = design_->getGridlineSize();
  auto cell_box = [&](int x, int y) -> odb::Rect {
    const int lx = grid_graph_->getGridline(0, x);
    const int ly = grid_graph_->getGridline(1, y);
    return {lx, ly, lx + gridline_size, ly + gridline_size};
  };

  // Emit one marker per congested 3D edge, tagged with its routing
  // layer. Comment carries capacity/usage/congestion and the source of
  // the congestion (wire segments, via stubs, or both). Skip sub-min
  // layers: their 0-capacity edges carry only pin-access via demand and
  // would otherwise emit false congestion markers.
  for (int l = constants_.min_routing_layer; l < num_layers; l++) {
    const int direction = grid_graph_->getLayerDirection(l);
    odb::dbTechLayer* db_layer = tech->findRoutingLayer(l + 1);
    const int x_max = (direction == MetalLayer::H) ? x_size - 1 : x_size;
    const int y_max = (direction == MetalLayer::H) ? y_size : y_size - 1;
    for (int x = 0; x < x_max; x++) {
      for (int y = 0; y < y_max; y++) {
        const GraphEdge& e = grid_graph_->getEdge(l, x, y);
        const double cap = std::max(e.capacity, 0.0);
        const double demand = e.demand;
        if (demand <= cap) {
          continue;
        }
        const int cap_int = static_cast<int>(std::round(cap));
        const int demand_int = static_cast<int>(std::round(demand));
        // Compute overflow on the unrounded values so a fractional
        // demand > capacity excess (common after the floor-to-1
        // adjustment when via-stub demand pushes an integer-capacity
        // edge slightly over) still produces a marker. Display value
        // is rounded up to the nearest integer so the comment never
        // shows "overflow:0" on a tile we just flagged.
        // TODO: update congestion report and ODB markers with double
        // for capacities and usages.
        const int overflow_int
            = std::max(1, static_cast<int>(std::ceil(demand - cap)));

        const auto wc_it = wire_count.find({l, x, y});
        const int wires = wc_it != wire_count.end() ? wc_it->second : 0;
        const bool wires_overflow = static_cast<double>(wires) > cap;
        const bool vias_contribute = (demand - wires) > 0;
        std::string kind = "vias";
        if (wires_overflow) {
          kind = vias_contribute ? "wires + vias" : "wires";
        }

        odb::dbMarkerCategory*& subcat
            = direction == MetalLayer::H ? h_subcat : v_subcat;
        if (subcat == nullptr) {
          subcat = odb::dbMarkerCategory::create(tool_category,
                                                 direction == MetalLayer::H
                                                     ? "Horizontal congestion"
                                                     : "Vertical congestion");
        }
        odb::dbMarker* marker = odb::dbMarker::create(subcat);
        if (marker == nullptr) {
          continue;
        }
        marker->addShape(cell_box(x, y));
        if (db_layer != nullptr) {
          marker->setTechLayer(db_layer);
        }
        marker->setComment("capacity:" + std::to_string(cap_int) + " usage:"
                           + std::to_string(demand_int) + " congestion:"
                           + std::to_string(overflow_int) + " (" + kind + ")");

        odb::PtrSet<odb::dbNet> sources;
        auto wn_it = wire_nets.find({l, x, y});
        if (wn_it != wire_nets.end()) {
          sources.insert(wn_it->second.begin(), wn_it->second.end());
        }
        auto vn_it = via_nets.find({l, x, y});
        if (vn_it != via_nets.end()) {
          sources.insert(vn_it->second.begin(), vn_it->second.end());
        }
        for (odb::dbNet* net : sources) {
          marker->addSource(net);
        }
      }
    }
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
  updateCongestedNets(overflow_nets);
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