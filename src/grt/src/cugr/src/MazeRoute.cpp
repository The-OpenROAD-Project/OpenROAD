#include "MazeRoute.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <limits>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

#include "Design.h"
#include "GridGraph.h"
#include "Layers.h"
#include "PatternRoute.h"
#include "geo.h"
#include "robin_hood.h"
#include "utl/Logger.h"

namespace grt {

void SparseGraph::init(const GridGraphView<CostT>& wire_cost_view,
                       const SparseGrid& grid)
{
  // 0. Create pseudo pins
  const auto selected_access_points = grid_graph_->selectAccessPoints(net_);
  pseudo_pins_.reserve(selected_access_points.size());
  for (const auto& selected_point : selected_access_points) {
    pseudo_pins_.push_back(selected_point);
  }
  // Sort for deterministic vertex index assignment across hash set orderings.
  std::ranges::stable_sort(pseudo_pins_,
                           [](const AccessPoint& a, const AccessPoint& b) {
                             if (a.point.x() != b.point.x()) {
                               return a.point.x() < b.point.x();
                             }
                             return a.point.y() < b.point.y();
                           });

  // 1. Collect additional routing grid lines
  std::vector<int> pxs;
  std::vector<int> pys;
  pxs.reserve(net_->getNumPins());
  pys.reserve(net_->getNumPins());
  for (const auto& pin : pseudo_pins_) {
    pxs.emplace_back(pin.point.x());
    pys.emplace_back(pin.point.y());
  }
  std::ranges::stable_sort(pxs);
  std::ranges::stable_sort(pys);

  const int x_size = grid_graph_->getSize(0);
  const int y_size = grid_graph_->getSize(1);
  xs_.reserve(x_size / grid.interval.x() + pxs.size());
  ys_.reserve(y_size / grid.interval.y() + pys.size());
  for (int i = 0, j = 0; true; i++) {
    const int x = i * grid.interval.x() + grid.offset.x();
    for (; j < pxs.size() && pxs[j] <= x; j++) {
      if ((!xs_.empty() && pxs[j] == xs_.back()) || pxs[j] == x) {
        continue;
      }
      xs_.emplace_back(pxs[j]);
    }
    if (x < x_size) {
      xs_.emplace_back(x);
    } else {
      break;
    }
  }
  for (int i = 0, j = 0; true; i++) {
    const int y = i * grid.interval.y() + grid.offset.y();
    for (; j < pys.size() && pys[j] <= y; j++) {
      if ((!ys_.empty() && pys[j] == ys_.back()) || pys[j] == y) {
        continue;
      }
      ys_.emplace_back(pys[j]);
    }
    if (y < y_size) {
      ys_.emplace_back(y);
    } else {
      break;
    }
  }

  // 2. Add vertices
  vertices_.reserve(2 * xs_.size() * ys_.size());
  for (int direction = 0; direction < 2; direction++) {
    for (auto& y : ys_) {
      for (auto& x : xs_) {
        vertices_.emplace_back(direction, x, y);
      }
    }
  }

  // 3. Add same-layer connections
  edges_.resize(vertices_.size(), {-1, -1, -1});
  costs_.resize(vertices_.size(), {-1, -1, -1});
  auto add_same_layer_edge
      = [&](const int direction, const int xi, const int yi) {
          const int u = getVertexIndex(direction, xi, yi);
          const int v = direction == MetalLayer::H ? u + 1 : u + xs_.size();
          const PointT U(xs_[xi], ys_[yi]);
          const PointT V(xs_[xi + 1 - direction], ys_[yi + direction]);

          edges_[u][0] = v;
          edges_[v][1] = u;
          costs_[u][0] = costs_[v][1] = wire_cost_view.sum(U, V);
        };

  for (int direction = 0; direction < 2; direction++) {
    if (direction == MetalLayer::H) {
      for (int yi = 0; yi < ys_.size(); yi++) {
        for (int xi = 0; xi + 1 < xs_.size(); xi++) {
          add_same_layer_edge(direction, xi, yi);
        }
      }
    } else {
      for (int xi = 0; xi < xs_.size(); xi++) {
        for (int yi = 0; yi + 1 < ys_.size(); yi++) {
          add_same_layer_edge(direction, xi, yi);
        }
      }
    }
  }

  // 4. Add diff-layer connections
  auto add_diff_layer_edge = [&](const int xi, const int yi) {
    const int u = getVertexIndex(0, xi, yi);
    const int v = u + xs_.size() * ys_.size();

    edges_[u][2] = v;
    edges_[v][2] = u;
    costs_[u][2] = costs_[v][2] = grid_graph_->getUnitViaCost();
  };

  for (int xi = 0; xi < xs_.size(); xi++) {
    for (int yi = 0; yi < ys_.size(); yi++) {
      add_diff_layer_edge(xi, yi);
    }
  }

  // 5. Add pseudo pin locations
  robin_hood::unordered_map<int, int> x_to_xi;
  robin_hood::unordered_map<int, int> y_to_yi;
  for (int xi = 0; xi < xs_.size(); xi++) {
    x_to_xi.emplace(xs_[xi], xi);
  }
  for (int yi = 0; yi < ys_.size(); yi++) {
    y_to_yi.emplace(ys_[yi], yi);
  }

  pin_vertex_.resize(pseudo_pins_.size(), -1);
  for (int pin_index = 0; pin_index < pseudo_pins_.size(); pin_index++) {
    const auto& pin = pseudo_pins_[pin_index];
    const int xi = x_to_xi[pin.point.x()];
    const int yi = y_to_yi[pin.point.y()];
    const int u = getVertexIndex(0, xi, yi);
    vertex_pin_.emplace(u, pin_index);
    pin_vertex_[pin_index] = u;
    // Set the cost of the diff-layer connection at u to be 0
    costs_[u][2] = 0;
    costs_[u + xs_.size() * ys_.size()][2] = 0;
  }
}

void MazeRoute::run()
{
  std::vector<CostT> min_costs(graph_.getNumVertices(),
                               std::numeric_limits<CostT>::max());

  // lambda to compare solutions; vertex index is a tiebreaker to ensure
  // deterministic heap ordering across different STL implementations.
  auto compare_solution = [&](const std::shared_ptr<Solution>& lhs,
                              const std::shared_ptr<Solution>& rhs) {
    if (lhs->cost != rhs->cost) {
      return lhs->cost > rhs->cost;
    }
    return lhs->vertex > rhs->vertex;
  };

  std::priority_queue<std::shared_ptr<Solution>,
                      std::vector<std::shared_ptr<Solution>>,
                      decltype(compare_solution)>
      queue(compare_solution);

  // lambda to update solution
  auto update_solution = [&](const std::shared_ptr<Solution>& solution) {
    queue.push(solution);
    min_costs[solution->vertex]
        = std::min(solution->cost, min_costs[solution->vertex]);
  };

  solutions_.reserve(net_->getNumPins());

  std::vector<bool> visited(net_->getNumPins(), false);
  const int start_pin_index = 0;
  visited[start_pin_index] = true;
  int num_detached = graph_.getNumPseudoPins() - 1;
  update_solution(std::make_shared<Solution>(
      0, graph_.getPinVertex(start_pin_index), nullptr));

  while (num_detached > 0) {
    std::shared_ptr<Solution> found_solution;
    int found_pin_index = 0;
    while (!queue.empty()) {
      auto solution = queue.top();
      queue.pop();
      found_pin_index = graph_.getVertexPin(solution->vertex);
      if (found_pin_index != -1 && !visited[found_pin_index]) {
        found_solution = std::move(solution);
        break;
      }
      // Pruning
      if (solution->cost > min_costs[solution->vertex]) {
        continue;
      }
      for (int edge_index = 0; edge_index < 3; edge_index++) {
        const int next_vertex
            = graph_.getNextVertex(solution->vertex, edge_index);
        if (next_vertex == -1
            || (solution->prev && next_vertex == solution->prev->vertex)) {
          continue;
        }
        const CostT next_cost
            = solution->cost + graph_.getEdgeCost(solution->vertex, edge_index);
        if (next_cost < min_costs[next_vertex]) {
          update_solution(
              std::make_shared<Solution>(next_cost, next_vertex, solution));
        }
      }
    }

    solutions_.emplace_back(found_solution);
    if (found_pin_index == -1) {
      logger_->error(utl::GRT,
                     282,
                     "Failed to find connected pin on net {}.",
                     net_->getName());
    }
    visited[found_pin_index] = true;
    num_detached -= 1;

    // Update the cost of the vertices_ on the path
    std::shared_ptr<Solution> temp = std::move(found_solution);
    while (temp && temp->cost != 0) {
      update_solution(std::make_shared<Solution>(0, temp->vertex, temp->prev));
      temp = temp->prev;
    }
  }

  if (num_detached != 0) {
    logger_->error(utl::GRT, 275, "Failed to connect all pins.");
  }
}

std::shared_ptr<SteinerTreeNode> MazeRoute::getSteinerTree() const
{
  std::shared_ptr<SteinerTreeNode> tree = nullptr;
  if (graph_.getNumPseudoPins() == 1) {
    const auto& pseudo_pin = graph_.getPseudoPin(0);
    tree = std::make_shared<SteinerTreeNode>(pseudo_pin.point,
                                             pseudo_pin.layers);
    return tree;
  }

  std::vector<bool> visited(net_->getNumPins(), false);
  robin_hood::unordered_map<int, std::shared_ptr<SteinerTreeNode>> created;
  for (auto& solution : solutions_) {
    std::shared_ptr<Solution> temp = solution;
    std::shared_ptr<SteinerTreeNode> last_node = nullptr;
    while (temp) {
      auto it = created.find(temp->vertex);
      if (it == created.end()) {
        const PointT point = graph_.getPoint(temp->vertex);
        auto node = std::make_shared<SteinerTreeNode>(point);
        created.emplace(temp->vertex, node);
        if (last_node) {
          node->addChild(last_node);
        }
        if (!temp->prev) {
          tree = node;
        }
        if (!last_node || !temp->prev) {
          // Both the start and the end of the path should contain pins
          const int pin_index = graph_.getVertexPin(temp->vertex);
          if (pin_index == -1) {
            logger_->error(utl::GRT,
                           284,
                           "Pin index not found for vertex {} on net {}.",
                           temp->vertex,
                           net_->getName());
          }
          node->setFixedLayers(graph_.getPseudoPin(pin_index).layers);
        }
        last_node = std::move(node);
        temp = temp->prev;
      } else {
        if (last_node) {
          it->second->addChild(last_node);
        }
        break;
      }
    }
  }

  if (tree == nullptr) {
    logger_->error(utl::GRT,
                   285,
                   "Steiner tree construction failed for net {}.",
                   net_->getName());
  }

  // Remove redundant tree nodes
  SteinerTreeNode::preorder(
      tree, [&](const std::shared_ptr<SteinerTreeNode>& node) {
        for (int child_index = 0; child_index < node->getNumChildren();
             child_index++) {
          const std::shared_ptr<SteinerTreeNode> child
              = node->getChildren()[child_index];
          if (node->x() == child->x() && node->y() == child->y()) {
            for (const auto& gradchild : child->getChildren()) {
              node->addChild(gradchild);
            }
            if (child->getFixedLayers().isValid()) {
              if (node->getFixedLayers().isValid()) {
                node->getFixedLayers().unionWith(child->getFixedLayers());
              } else {
                node->setFixedLayers(child->getFixedLayers());
              }
            }
            node->removeChild(child_index);
            child_index -= 1;
          }
        }
      });

  // Remove intermediate tree nodes
  SteinerTreeNode::preorder(
      tree, [&](const std::shared_ptr<SteinerTreeNode>& node) {
        for (std::shared_ptr<SteinerTreeNode>& child : node->getChildren()) {
          const int direction
              = (node->y() == child->y() ? MetalLayer::H : MetalLayer::V);
          std::shared_ptr<SteinerTreeNode> temp = child;
          while (!temp->getFixedLayers().isValid()
                 && temp->getNumChildren() == 1
                 && (*temp)[1 - direction]
                        == (*(temp->getChildren()[0]))[1 - direction]) {
            temp = temp->getChildren()[0];
          }
          child = std::move(temp);
        }
      });

  // Check duplicate tree nodes
  SteinerTreeNode::preorder(
      tree, [&](const std::shared_ptr<SteinerTreeNode>& node) {
        for (const auto& child : node->getChildren()) {
          if (node->x() == child->x() && node->y() == child->y()) {
            logger_->error(utl::GRT, 276, "Duplicated tree nodes encountered.");
          }
        }
      });
  return tree;
}

}  // namespace grt
