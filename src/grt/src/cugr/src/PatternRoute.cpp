#include "PatternRoute.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <functional>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "Design.h"
#include "GRTree.h"
#include "GridGraph.h"
#include "Layers.h"
#include "geo.h"
#include "robin_hood.h"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace grt {

void SteinerTreeNode::preorder(
    const std::shared_ptr<SteinerTreeNode>& node,
    const std::function<void(std::shared_ptr<SteinerTreeNode>)>& visit)
{
  visit(node);
  for (const auto& child : node->getChildren()) {
    preorder(child, visit);
  }
}

std::string SteinerTreeNode::getPythonString(
    const std::shared_ptr<SteinerTreeNode>& node)
{
  std::vector<std::pair<PointT, PointT>> edges;
  preorder(node, [&](const std::shared_ptr<SteinerTreeNode>& node) {
    for (const auto& child : node->getChildren()) {
      edges.emplace_back(*node, *child);
    }
  });
  std::stringstream ss;
  ss << "[";
  for (int i = 0; i < edges.size(); i++) {
    auto& edge = edges[i];
    ss << "[" << edge.first << ", " << edge.second << "]";
    ss << (i < edges.size() - 1 ? ", " : "]");
  }
  return ss.str();
}

std::string PatternRoutingNode::getPythonString(
    std::shared_ptr<PatternRoutingNode> routing_dag_)
{
  std::vector<std::pair<PointT, PointT>> edges;
  std::function<void(std::shared_ptr<PatternRoutingNode>)> get_edges
      = [&](const std::shared_ptr<PatternRoutingNode>& node) {
          for (auto& child_paths : node->getPaths()) {
            for (auto& path : child_paths) {
              edges.emplace_back(*node, *path);
              get_edges(path);
            }
          }
        };
  get_edges(std::move(routing_dag_));
  std::stringstream ss;
  ss << "[";
  for (int i = 0; i < edges.size(); i++) {
    auto& edge = edges[i];
    ss << "[" << edge.first << ", " << edge.second << "]";
    ss << (i < edges.size() - 1 ? ", " : "]");
  }
  return ss.str();
}

void PatternRoute::constructSteinerTree()
{
  auto selected_access_points = grid_graph_->selectAccessPoints(net_);

  const int degree = selected_access_points.size();
  if (degree == 1) {
    const auto& access_point = *selected_access_points.begin();
    steiner_tree_ = std::make_shared<SteinerTreeNode>(access_point.point,
                                                      access_point.layers);
    return;
  }

  // Sort access points by (x, y) before passing to FLUTE for deterministic
  // output when equal-coordinate points exist across different STL builds.
  std::vector<std::pair<int, int>> sorted_points;
  sorted_points.reserve(selected_access_points.size());
  for (auto& access_point : selected_access_points) {
    sorted_points.emplace_back(access_point.point.x(), access_point.point.y());
  }
  std::ranges::stable_sort(sorted_points);

  std::vector<int> xs;
  std::vector<int> ys;
  xs.reserve(sorted_points.size());
  ys.reserve(sorted_points.size());
  for (auto& [x, y] : sorted_points) {
    xs.push_back(x);
    ys.push_back(y);
  }

  stt::Tree flute_tree = stt_builder_->flute(xs, ys, flute_accuracy_);
  const int num_branches = degree + degree - 2;
  std::vector<PointT> steiner_points;
  steiner_points.reserve(num_branches);
  std::vector<std::vector<int>> adjacent_list(num_branches);

  for (int branch_index = 0; branch_index < num_branches; branch_index++) {
    const stt::Branch& branch = flute_tree.branch[branch_index];
    steiner_points.emplace_back(branch.x, branch.y);
    if (branch_index == branch.n) {
      continue;
    }
    adjacent_list[branch_index].push_back(branch.n);
    adjacent_list[branch.n].push_back(branch_index);
  }

  std::function<void(std::shared_ptr<SteinerTreeNode>&, int, int)>
      construct_tree = [&](std::shared_ptr<SteinerTreeNode>& parent,
                           int prev_index,
                           int cur_index) {
        std::shared_ptr<SteinerTreeNode> current
            = std::make_shared<SteinerTreeNode>(steiner_points[cur_index]);
        if (parent != nullptr && parent->x() == current->x()
            && parent->y() == current->y()) {
          for (int next_index : adjacent_list[cur_index]) {
            if (next_index == prev_index) {
              continue;
            }
            construct_tree(parent, cur_index, next_index);
          }
          return;
        }
        // Build subtree
        for (int next_index : adjacent_list[cur_index]) {
          if (next_index == prev_index) {
            continue;
          }
          construct_tree(current, cur_index, next_index);
        }
        // Set fixed layer interval
        const AccessPoint current_pt{.point = {current->x(), current->y()},
                                     .layers = {}};
        if (auto it = selected_access_points.find(current_pt);
            it != selected_access_points.end()) {
          current->setFixedLayers(it->layers);
        }
        // Connect current to parent
        if (parent == nullptr) {
          parent = std::move(current);
        } else {
          parent->addChild(current);
        }
      };
  // Pick a root having degree 1
  int root = 0;
  std::function<bool(int)> has_degree1 = [&](int index) {
    if (adjacent_list[index].size() == 1) {
      int next_index = adjacent_list[index][0];
      if (steiner_points[index] == steiner_points[next_index]) {
        return has_degree1(next_index);
      }
      return true;
    }

    return false;
  };
  for (int i = 0; i < steiner_points.size(); i++) {
    if (has_degree1(i)) {
      root = i;
      break;
    }
  }
  construct_tree(steiner_tree_, -1, root);
}

void PatternRoute::constructRoutingDAG()
{
  std::function<void(std::shared_ptr<PatternRoutingNode>&,
                     std::shared_ptr<SteinerTreeNode>&)>
      construct_dag = [&](std::shared_ptr<PatternRoutingNode>& dst_node,
                          std::shared_ptr<SteinerTreeNode>& steiner) {
        std::shared_ptr<PatternRoutingNode> current
            = std::make_shared<PatternRoutingNode>(
                *steiner, steiner->getFixedLayers(), num_dag_nodes_++);
        for (auto steiner_child : steiner->getChildren()) {
          construct_dag(current, steiner_child);
        }
        if (dst_node == nullptr) {
          dst_node = std::move(current);
        } else {
          dst_node->addChild(current);
          constructPaths(dst_node, current);
        }
      };
  construct_dag(routing_dag_, steiner_tree_);
}

void PatternRoute::constructPaths(std::shared_ptr<PatternRoutingNode>& start,
                                  std::shared_ptr<PatternRoutingNode>& end,
                                  int child_index)
{
  if (child_index == -1) {
    child_index = start->getPaths().size();
    start->getPaths().emplace_back();
  }
  std::vector<std::shared_ptr<PatternRoutingNode>>& child_paths
      = start->getPaths()[child_index];
  if (start->x() == end->x() || start->y() == end->y()) {
    child_paths.push_back(end);
  } else {
    for (int path_index = 0; path_index <= 1;
         path_index++) {  // two paths of different L-shape
      PointT mid_point = path_index ? PointT(start->x(), end->y())
                                    : PointT(end->x(), start->y());
      std::shared_ptr<PatternRoutingNode> mid
          = std::make_shared<PatternRoutingNode>(
              mid_point, num_dag_nodes_++, true);
      mid->getPaths() = {{end}};
      child_paths.push_back(std::move(mid));
    }
  }
}

void PatternRoute::constructDetours(GridGraphView<bool>& congestion_view)
{
  struct ScaffoldNode
  {
    std::shared_ptr<PatternRoutingNode> node;
    std::vector<std::shared_ptr<ScaffoldNode>> children;
    ScaffoldNode(std::shared_ptr<PatternRoutingNode> n) : node(std::move(n)) {}
  };

  std::vector<std::vector<std::shared_ptr<ScaffoldNode>>> scaffolds(2);
  std::vector<std::vector<std::shared_ptr<ScaffoldNode>>> scaffold_nodes(
      2,
      std::vector<std::shared_ptr<ScaffoldNode>>(
          num_dag_nodes_,
          nullptr));  // direction -> num_dag_nodes_ -> scaffold node
  std::vector<bool> visited(num_dag_nodes_, false);

  std::function<void(std::shared_ptr<PatternRoutingNode>)> build_scaffolds =
      [&](const std::shared_ptr<PatternRoutingNode>& node) {
        if (visited[node->getIndex()]) {
          return;
        }
        visited[node->getIndex()] = true;

        if (node->isOptional()) {
          assert(node->getPaths().size() == 1 && node->getPaths()[0].size() == 1
                 && !node->getPaths()[0][0]->isOptional());
          auto& path = node->getPaths()[0][0];
          build_scaffolds(path);
          int direction
              = (node->y() == path->y() ? MetalLayer::H : MetalLayer::V);
          if (!scaffold_nodes[direction][path->getIndex()]
              && congestion_view.check(*node, *path)) {
            scaffold_nodes[direction][path->getIndex()]
                = std::make_shared<ScaffoldNode>(path);
          }
        } else {
          for (auto& child_paths : node->getPaths()) {
            for (auto& path : child_paths) {
              build_scaffolds(path);
              int direction
                  = (node->y() == path->y() ? MetalLayer::H : MetalLayer::V);
              if (path->isOptional()) {
                if (!scaffold_nodes[direction][node->getIndex()]
                    && congestion_view.check(*node, *path)) {
                  scaffold_nodes[direction][node->getIndex()]
                      = std::make_shared<ScaffoldNode>(node);
                }
              } else {
                if (congestion_view.check(*node, *path)) {
                  if (!scaffold_nodes[direction][node->getIndex()]) {
                    scaffold_nodes[direction][node->getIndex()]
                        = std::make_shared<ScaffoldNode>(node);
                  }
                  if (!scaffold_nodes[direction][path->getIndex()]) {
                    scaffold_nodes[direction][node->getIndex()]
                        ->children.emplace_back(
                            std::make_shared<ScaffoldNode>(path));
                  } else {
                    scaffold_nodes[direction][node->getIndex()]
                        ->children.emplace_back(
                            scaffold_nodes[direction][path->getIndex()]);
                    scaffold_nodes[direction][path->getIndex()] = nullptr;
                  }
                }
              }
            }
            for (auto& child : node->getChildren()) {
              for (int direction = 0; direction < 2; direction++) {
                if (scaffold_nodes[direction][child->getIndex()]) {
                  scaffolds[direction].emplace_back(
                      std::make_shared<ScaffoldNode>(node));
                  scaffolds[direction].back()->children.emplace_back(
                      scaffold_nodes[direction][child->getIndex()]);
                  scaffold_nodes[direction][child->getIndex()] = nullptr;
                }
              }
            }
          }
        }
      };

  build_scaffolds(routing_dag_);
  for (int direction = 0; direction < 2; direction++) {
    if (scaffold_nodes[direction][routing_dag_->getIndex()]) {
      scaffolds[direction].emplace_back(
          std::make_shared<ScaffoldNode>(nullptr));
      scaffolds[direction].back()->children.emplace_back(
          scaffold_nodes[direction][routing_dag_->getIndex()]);
    }
  }

  std::function<void(const std::shared_ptr<ScaffoldNode>&,
                     IntervalT&,
                     std::vector<int>&,
                     int,
                     bool)>
      get_trunk_and_stems
      = [&](const std::shared_ptr<ScaffoldNode>& scaffold_node,
            IntervalT& trunk,
            std::vector<int>& stems,
            int direction,
            bool starting) {
          if (starting) {
            if (scaffold_node->node) {
              stems.emplace_back((*scaffold_node->node)[1 - direction]);
              trunk.update((*scaffold_node->node)[direction]);
            }
            for (auto& scaffold_child : scaffold_node->children) {
              get_trunk_and_stems(
                  scaffold_child, trunk, stems, direction, false);
            }
          } else {
            trunk.update((*scaffold_node->node)[direction]);
            if (scaffold_node->node->getFixedLayers().isValid()) {
              stems.emplace_back((*scaffold_node->node)[1 - direction]);
            }
            for (const auto& tree_child : scaffold_node->node->getChildren()) {
              bool scaffolded = false;
              for (auto& scaffold_child : scaffold_node->children) {
                if (tree_child == scaffold_child->node) {
                  get_trunk_and_stems(
                      scaffold_child, trunk, stems, direction, false);
                  scaffolded = true;
                  break;
                }
              }
              if (!scaffolded) {
                stems.emplace_back((*tree_child)[1 - direction]);
                trunk.update((*tree_child)[direction]);
              }
            }
          }
        };

  auto get_total_stem_length
      = [&](const std::vector<int>& stems, const int pos) {
          int length = 0;
          for (int stem : stems) {
            length += abs(stem - pos);
          }
          return length;
        };

  std::function<std::shared_ptr<PatternRoutingNode>(
      std::shared_ptr<ScaffoldNode>, int, int)>
      build_detour = [&](const std::shared_ptr<ScaffoldNode>& scaffold_node,
                         int direction,
                         int shift_amount) {
        std::shared_ptr<PatternRoutingNode> tree_node = scaffold_node->node;
        if (tree_node->getFixedLayers().isValid()) {
          std::shared_ptr<PatternRoutingNode> dup_tree_node
              = std::make_shared<PatternRoutingNode>(
                  (PointT) *tree_node,
                  tree_node->getFixedLayers(),
                  num_dag_nodes_++);
          std::shared_ptr<PatternRoutingNode> shifted_tree_node
              = std::make_shared<PatternRoutingNode>((PointT) *tree_node,
                                                     num_dag_nodes_++);
          (*shifted_tree_node)[1 - direction] += shift_amount;
          constructPaths(shifted_tree_node, dup_tree_node);
          for (auto& tree_child : tree_node->getChildren()) {
            bool built = false;
            for (auto& scaffold_child : scaffold_node->children) {
              if (tree_child == scaffold_child->node) {
                auto shifted_child_tree_node
                    = build_detour(scaffold_child, direction, shift_amount);
                constructPaths(shifted_tree_node, shifted_child_tree_node);
                built = true;
                break;
              }
            }
            if (!built) {
              constructPaths(shifted_tree_node, tree_child);
            }
          }
          return shifted_tree_node;
        }
        std::shared_ptr<PatternRoutingNode> shifted_tree_node
            = std::make_shared<PatternRoutingNode>((PointT) *tree_node,
                                                   num_dag_nodes_++);
        (*shifted_tree_node)[1 - direction] += shift_amount;
        for (auto& tree_child : tree_node->getChildren()) {
          bool built = false;
          for (auto& scaffold_child : scaffold_node->children) {
            if (tree_child == scaffold_child->node) {
              auto shifted_child_tree_node
                  = build_detour(scaffold_child, direction, shift_amount);
              constructPaths(shifted_tree_node, shifted_child_tree_node);
              built = true;
              break;
            }
          }
          if (!built) {
            constructPaths(shifted_tree_node, tree_child);
          }
        }
        return shifted_tree_node;
      };

  for (int direction = 0; direction < 2; direction++) {
    for (const std::shared_ptr<ScaffoldNode>& scaffold : scaffolds[direction]) {
      assert(scaffold->children.size() == 1);

      IntervalT trunk;
      std::vector<int> stems;
      get_trunk_and_stems(scaffold, trunk, stems, direction, true);
      std::ranges::stable_sort(stems);
      int trunk_pos = (*scaffold->children[0]->node)[1 - direction];
      int original_length = get_total_stem_length(stems, trunk_pos);
      IntervalT shift_interval(trunk_pos);
      int max_length_increase = trunk.range() * constants_.max_detour_ratio;
      while (shift_interval.low() - 1 >= 0
             && get_total_stem_length(stems, shift_interval.low() - 1)
                        - original_length
                    <= max_length_increase) {
        shift_interval.addToLow(-1);
      }
      while (shift_interval.high() + 1 < grid_graph_->getSize(1 - direction)
             && get_total_stem_length(stems, shift_interval.high() - 1)
                        - original_length
                    <= max_length_increase) {
        shift_interval.addToHigh(1);
      }
      int step = 1;
      while ((trunk_pos - shift_interval.low()) / (step + 1)
                 + (shift_interval.high() - trunk_pos) / (step + 1)
             >= constants_.target_detour_count) {
        step++;
      }

      shift_interval.set(
          trunk_pos - (trunk_pos - shift_interval.low()) / step * step,
          trunk_pos + (shift_interval.high() - trunk_pos) / step * step);
      for (int pos = shift_interval.low(); pos <= shift_interval.high();
           pos += step) {
        int shift_amount = (pos - trunk_pos);
        if (shift_amount == 0) {
          continue;
        }
        if (scaffold->node) {
          auto& scaffold_child = scaffold->children[0];
          if ((*scaffold_child->node)[1 - direction] + shift_amount < 0
              || (*scaffold_child->node)[1 - direction] + shift_amount
                     >= grid_graph_->getSize(1 - direction)) {
            continue;
          }
          for (int child_index = 0;
               child_index < scaffold->node->getNumChildren();
               child_index++) {
            auto& tree_child = scaffold->node->getChildren()[child_index];
            if (tree_child == scaffold_child->node) {
              std::shared_ptr<PatternRoutingNode> shifted_child
                  = build_detour(scaffold_child, direction, shift_amount);
              constructPaths(scaffold->node, shifted_child, child_index);
            }
          }
        } else {
          std::shared_ptr<ScaffoldNode> scaffold_node = scaffold->children[0];
          auto tree_node = scaffold_node->node;
          if (tree_node->getNumChildren() == 1) {
            if ((*tree_node)[1 - direction] + shift_amount < 0
                || (*tree_node)[1 - direction] + shift_amount
                       >= grid_graph_->getSize(1 - direction)) {
              continue;
            }
            std::shared_ptr<PatternRoutingNode> shifted_tree_node
                = std::make_shared<PatternRoutingNode>((PointT) *tree_node,
                                                       num_dag_nodes_++);
            (*shifted_tree_node)[1 - direction] += shift_amount;
            constructPaths(tree_node, shifted_tree_node, 0);
            for (auto& tree_child : tree_node->getChildren()) {
              bool built = false;
              for (auto& scaffold_child : scaffold_node->children) {
                if (tree_child == scaffold_child->node) {
                  auto shifted_child_tree_node
                      = build_detour(scaffold_child, direction, shift_amount);
                  constructPaths(shifted_tree_node, shifted_child_tree_node);
                  built = true;
                  break;
                }
              }
              if (!built) {
                constructPaths(shifted_tree_node, tree_child);
              }
            }

          } else {
            logger_->warn(
                utl::GRT, 277, "The root doesn't have exactly one child.");
          }
        }
      }
    }
  }
}

void PatternRoute::run()
{
  calculateRoutingCosts(routing_dag_);
  net_->setRoutingTree(getRoutingTree(routing_dag_));
}

void PatternRoute::calculateRoutingCosts(
    std::shared_ptr<PatternRoutingNode>& node)
{
  if (!node->getCosts().empty()) {
    return;
  }
  std::vector<std::vector<std::pair<CostT, int>>>
      child_costs;  // child_index -> layer_index -> (cost, path_index)
  // Calculate child costs
  if (!node->getPaths().empty()) {
    child_costs.resize(node->getPaths().size());
  }
  for (int child_index = 0; child_index < node->getPaths().size();
       child_index++) {
    auto& child_paths = node->getPaths()[child_index];
    auto& costs = child_costs[child_index];
    costs.assign(grid_graph_->getNumLayers(),
                 {std::numeric_limits<CostT>::max(), -1});
    for (int path_index = 0; path_index < child_paths.size(); path_index++) {
      std::shared_ptr<PatternRoutingNode>& path = child_paths[path_index];
      calculateRoutingCosts(path);
      int direction = node->x() == path->x() ? MetalLayer::V : MetalLayer::H;
      assert((*node)[1 - direction] == (*path)[1 - direction]);
      for (int layer_index = constants_.min_routing_layer;
           layer_index < grid_graph_->getNumLayers();
           layer_index++) {
        if (grid_graph_->getLayerDirection(layer_index) != direction) {
          continue;
        }
        CostT cost
            = net_->isInsideLayerRange(layer_index)
                  ? path->getCosts()[layer_index]
                        + grid_graph_->getWireCost(layer_index, *node, *path)
                  : std::numeric_limits<CostT>::max();
        if (cost < costs[layer_index].first) {
          costs[layer_index] = std::make_pair(cost, path_index);
        }
      }
    }
  }

  node->getCosts().assign(grid_graph_->getNumLayers(),
                          std::numeric_limits<CostT>::max());
  node->getBestPaths().resize(grid_graph_->getNumLayers());
  if (!node->getPaths().empty()) {
    for (int layer_index = 1; layer_index < grid_graph_->getNumLayers();
         layer_index++) {
      node->getBestPaths()[layer_index].assign(node->getPaths().size(),
                                               {-1, -1});
    }
  }
  // Calculate the partial sum of the via costs
  std::vector<CostT> via_costs(grid_graph_->getNumLayers());
  via_costs[0] = 0;
  for (int layer_index = 1; layer_index < grid_graph_->getNumLayers();
       layer_index++) {
    via_costs[layer_index] = via_costs[layer_index - 1]
                             + grid_graph_->getViaCost(layer_index - 1, *node);
  }
  IntervalT fixed_layers(node->getFixedLayers());
  fixed_layers.set(
      std::min(fixed_layers.low(), grid_graph_->getNumLayers() - 1),
      std::max(fixed_layers.high(), constants_.min_routing_layer));

  for (int low_layer_index = 0; low_layer_index <= fixed_layers.low();
       low_layer_index++) {
    std::vector<CostT> min_child_costs;
    std::vector<std::pair<int, int>> best_paths;
    if (!node->getPaths().empty()) {
      min_child_costs.assign(node->getPaths().size(),
                             std::numeric_limits<CostT>::max());
      best_paths.assign(node->getPaths().size(), {-1, -1});
    }
    for (int layer_index = low_layer_index;
         layer_index < grid_graph_->getNumLayers();
         layer_index++) {
      for (int child_index = 0; child_index < node->getPaths().size();
           child_index++) {
        if (child_costs[child_index][layer_index].first
            < min_child_costs[child_index]) {
          min_child_costs[child_index]
              = child_costs[child_index][layer_index].first;
          best_paths[child_index] = std::make_pair(
              child_costs[child_index][layer_index].second, layer_index);
        }
      }
      if (layer_index >= fixed_layers.high()) {
        CostT cost = via_costs[layer_index] - via_costs[low_layer_index];
        for (CostT child_cost : min_child_costs) {
          cost += child_cost;
        }
        if (cost < node->getCosts()[layer_index]) {
          node->getCosts()[layer_index] = cost;
          node->getBestPaths()[layer_index] = best_paths;
        }
      }
    }
    for (int layer_index = grid_graph_->getNumLayers() - 2;
         layer_index >= low_layer_index;
         layer_index--) {
      if (node->getCosts()[layer_index + 1] < node->getCosts()[layer_index]) {
        node->getCosts()[layer_index] = node->getCosts()[layer_index + 1];
        node->getBestPaths()[layer_index]
            = node->getBestPaths()[layer_index + 1];
      }
    }
  }
}

std::shared_ptr<GRTreeNode> PatternRoute::getRoutingTree(
    std::shared_ptr<PatternRoutingNode>& node,
    int parent_layer_index)
{
  if (parent_layer_index == -1) {
    CostT min_cost = std::numeric_limits<CostT>::max();
    for (int layer_index = 0; layer_index < grid_graph_->getNumLayers();
         layer_index++) {
      if (routing_dag_->getCosts()[layer_index] < min_cost) {
        min_cost = routing_dag_->getCosts()[layer_index];
        parent_layer_index = layer_index;
      }
    }
  }
  assert(parent_layer_index >= 0);
  if (parent_layer_index < 0) {
    logger_->error(utl::GRT,
                   286,
                   "Failed to determine parent layer index on net {}.",
                   net_->getName());
  }
  std::shared_ptr<GRTreeNode> routing_node
      = std::make_shared<GRTreeNode>(parent_layer_index, node->x(), node->y());
  std::shared_ptr<GRTreeNode> lowest_routing_node = routing_node;
  std::shared_ptr<GRTreeNode> highest_routing_node = routing_node;
  if (!node->getPaths().empty()) {
    int path_index, layer_index;
    std::vector<std::vector<std::shared_ptr<PatternRoutingNode>>>
        paths_on_layer(grid_graph_->getNumLayers());
    for (int child_index = 0; child_index < node->getPaths().size();
         child_index++) {
      std::tie(path_index, layer_index)
          = node->getBestPaths()[parent_layer_index][child_index];
      paths_on_layer[layer_index].push_back(
          node->getPaths()[child_index][path_index]);
    }
    if (!paths_on_layer[parent_layer_index].empty()) {
      for (auto& path : paths_on_layer[parent_layer_index]) {
        routing_node->addChild(getRoutingTree(path, parent_layer_index));
      }
    }
    for (int layer_index = parent_layer_index - 1; layer_index >= 0;
         layer_index--) {
      if (!paths_on_layer[layer_index].empty()) {
        lowest_routing_node->addChild(
            std::make_shared<GRTreeNode>(layer_index, node->x(), node->y()));
        lowest_routing_node = lowest_routing_node->getChildren().back();
        for (auto& path : paths_on_layer[layer_index]) {
          lowest_routing_node->addChild(getRoutingTree(path, layer_index));
        }
      }
    }
    for (int layer_index = parent_layer_index + 1;
         layer_index < grid_graph_->getNumLayers();
         layer_index++) {
      if (!paths_on_layer[layer_index].empty()) {
        highest_routing_node->addChild(
            std::make_shared<GRTreeNode>(layer_index, node->x(), node->y()));
        highest_routing_node = highest_routing_node->getChildren().back();
        for (auto& path : paths_on_layer[layer_index]) {
          highest_routing_node->addChild(getRoutingTree(path, layer_index));
        }
      }
    }
  }
  if (lowest_routing_node->getLayerIdx() > node->getFixedLayers().low()) {
    lowest_routing_node->addChild(std::make_shared<GRTreeNode>(
        node->getFixedLayers().low(), node->x(), node->y()));
  }
  if (highest_routing_node->getLayerIdx() < node->getFixedLayers().high()) {
    highest_routing_node->addChild(std::make_shared<GRTreeNode>(
        node->getFixedLayers().high(), node->x(), node->y()));
  }
  return routing_node;
}

}  // namespace grt
