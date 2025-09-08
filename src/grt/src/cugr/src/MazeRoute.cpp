#include "MazeRoute.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
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

namespace grt {

void SparseGraph::init(GridGraphView<CostT>& wire_cost_view, SparseGrid& grid)
{
  // 0. Create pseudo pins
  robin_hood::unordered_map<uint64_t, std::pair<PointT, IntervalT>>
      selectedAccessPoints;
  grid_graph_->selectAccessPoints(net_, selectedAccessPoints);
  pseudo_pins_.reserve(selectedAccessPoints.size());
  for (auto& selectedPoint : selectedAccessPoints) {
    pseudo_pins_.push_back(selectedPoint.second);
  }

  // 1. Collect additional routing grid lines
  std::vector<int> pxs;
  std::vector<int> pys;
  pxs.reserve(net_->getNumPins());
  pys.reserve(net_->getNumPins());
  for (const auto& pin : pseudo_pins_) {
    pxs.emplace_back(pin.first.x);
    pys.emplace_back(pin.first.y);
  }
  std::sort(pxs.begin(), pxs.end());
  std::sort(pys.begin(), pys.end());

  const int xSize = grid_graph_->getSize(0);
  const int ySize = grid_graph_->getSize(1);
  xs_.reserve(xSize / grid.interval.x + pxs.size());
  ys_.reserve(ySize / grid.interval.y + pys.size());
  int j = 0;
  for (int i = 0; true; i++) {
    int x = i * grid.interval.x + grid.offset.x;
    for (; j < pxs.size() && pxs[j] <= x; j++) {
      if ((!xs_.empty() && pxs[j] == xs_.back()) || pxs[j] == x) {
        continue;
      }
      xs_.emplace_back(pxs[j]);
    }
    if (x < xSize) {
      xs_.emplace_back(x);
    } else {
      break;
    }
  }
  j = 0;
  for (int i = 0; true; i++) {
    int y = i * grid.interval.y + grid.offset.y;
    for (; j < pys.size() && pys[j] <= y; j++) {
      if ((!ys_.empty() && pys[j] == ys_.back()) || pys[j] == y) {
        continue;
      }
      ys_.emplace_back(pys[j]);
    }
    if (y < ySize) {
      ys_.emplace_back(y);
    } else {
      break;
    }
  }

  // 2. Add vertices
  vertices_.reserve(2 * xs_.size() * ys_.size());
  for (unsigned direction = 0; direction < 2; direction++) {
    for (auto& y : ys_) {
      for (auto& x : xs_) {
        vertices_.emplace_back(direction, x, y);
      }
    }
  }

  // 3. Add same-layer connections
  edges_.resize(vertices_.size(), {-1, -1, -1});
  costs_.resize(vertices_.size(), {-1, -1, -1});
  auto addSameLayerEdge
      = [&](const unsigned direction, const int xi, const int yi) {
          const int u = getVertexIndex(direction, xi, yi);
          const int v = direction == MetalLayer::H ? u + 1 : u + xs_.size();
          PointT U(xs_[xi], ys_[yi]);
          PointT V(xs_[xi + 1 - direction], ys_[yi + direction]);

          edges_[u][0] = v;
          edges_[v][1] = u;
          costs_[u][0] = costs_[v][1] = wire_cost_view.sum(U, V);
        };

  for (unsigned direction = 0; direction < 2; direction++) {
    if (direction == MetalLayer::H) {
      for (int yi = 0; yi < ys_.size(); yi++) {
        for (int xi = 0; xi + 1 < xs_.size(); xi++) {
          addSameLayerEdge(direction, xi, yi);
        }
      }
    } else {
      for (int xi = 0; xi < xs_.size(); xi++) {
        for (int yi = 0; yi + 1 < ys_.size(); yi++) {
          addSameLayerEdge(direction, xi, yi);
        }
      }
    }
  }

  // 4. Add diff-layer connections
  auto addDiffLayerEdge = [&](const int xi, const int yi) {
    const int u = getVertexIndex(0, xi, yi);
    const int v = u + xs_.size() * ys_.size();

    edges_[u][2] = v;
    edges_[v][2] = u;
    costs_[u][2] = costs_[v][2] = grid_graph_->getUnitViaCost();
  };

  for (int xi = 0; xi < xs_.size(); xi++) {
    for (int yi = 0; yi < ys_.size(); yi++) {
      addDiffLayerEdge(xi, yi);
    }
  }

  // 5. Add pseudo pin locations
  robin_hood::unordered_map<int, int> xtoxi;
  robin_hood::unordered_map<int, int> ytoyi;
  for (int xi = 0; xi < xs_.size(); xi++) {
    xtoxi.emplace(xs_[xi], xi);
  }
  for (int yi = 0; yi < ys_.size(); yi++) {
    ytoyi.emplace(ys_[yi], yi);
  }

  pin_vertex_.resize(pseudo_pins_.size(), -1);
  for (int pinIndex = 0; pinIndex < pseudo_pins_.size(); pinIndex++) {
    const auto& pin = pseudo_pins_[pinIndex];
    const int xi = xtoxi[pin.first.x];
    const int yi = ytoyi[pin.first.y];
    const int u = getVertexIndex(0, xi, yi);
    vertex_pin_.emplace(u, pinIndex);
    pin_vertex_[pinIndex] = u;
    // Set the cost of the diff-layer connection at u to be 0
    costs_[u][2] = 0;
    costs_[u + xs_.size() * ys_.size()][2] = 0;
  }
}

void MazeRoute::run()
{
  std::vector<CostT> minCosts(graph_.getNumVertices(),
                              std::numeric_limits<CostT>::max());

  // lambda to compare solutions
  auto compareSolution = [&](const std::shared_ptr<Solution>& lhs,
                             const std::shared_ptr<Solution>& rhs) {
    return lhs->cost > rhs->cost;
  };

  std::priority_queue<std::shared_ptr<Solution>,
                      std::vector<std::shared_ptr<Solution>>,
                      decltype(compareSolution)>
      queue(compareSolution);

  // lambda to update solution
  auto updateSolution = [&](const std::shared_ptr<Solution>& solution) {
    queue.push(solution);
    if (solution->cost < minCosts[solution->vertex]) {
      minCosts[solution->vertex] = solution->cost;
    }
  };

  solutions_.reserve(net_->getNumPins());

  std::vector<bool> visited(net_->getNumPins(), false);
  const int startPinIndex = 0;
  visited[startPinIndex] = true;
  int numDetached = graph_.getNumPseudoPins() - 1;
  updateSolution(std::make_shared<Solution>(
      0, graph_.getPinVertex(startPinIndex), nullptr));

  while (numDetached > 0) {
    std::shared_ptr<Solution> foundSolution;
    int foundPinIndex = 0;
    while (!queue.empty()) {
      auto solution = queue.top();
      queue.pop();
      foundPinIndex = graph_.getVertexPin(solution->vertex);
      if (foundPinIndex != -1 && !visited[foundPinIndex]) {
        foundSolution = solution;
        break;
      }
      // Pruning
      if (solution->cost > minCosts[solution->vertex]) {
        continue;
      }
      for (int edgeIndex = 0; edgeIndex < 3; edgeIndex++) {
        int nextVertex = graph_.getNextVertex(solution->vertex, edgeIndex);
        if (nextVertex == -1
            || (solution->prev && nextVertex == solution->prev->vertex)) {
          continue;
        }
        CostT nextCost
            = solution->cost + graph_.getEdgeCost(solution->vertex, edgeIndex);
        if (nextCost < minCosts[nextVertex]) {
          updateSolution(
              std::make_shared<Solution>(nextCost, nextVertex, solution));
        }
      }
    }

    solutions_.emplace_back(foundSolution);
    visited[foundPinIndex] = true;
    numDetached -= 1;

    // Update the cost of the vertices_ on the path
    std::shared_ptr<Solution> temp = foundSolution;
    while (temp && temp->cost != 0) {
      updateSolution(std::make_shared<Solution>(0, temp->vertex, temp->prev));
      temp = temp->prev;
    }
  }

  if (numDetached != 0) {
    printf("Error: failed to connect all pins.");
  }
}

std::shared_ptr<SteinerTreeNode> MazeRoute::getSteinerTree() const
{
  std::shared_ptr<SteinerTreeNode> tree = nullptr;
  if (graph_.getNumPseudoPins() == 1) {
    const auto& pseudoPin = graph_.getPseudoPin(0);
    tree = std::make_shared<SteinerTreeNode>(pseudoPin.first, pseudoPin.second);
    return tree;
  }

  std::vector<bool> visited(net_->getNumPins(), false);
  robin_hood::unordered_map<int, std::shared_ptr<SteinerTreeNode>> created;
  for (auto& solution : solutions_) {
    std::shared_ptr<Solution> temp = solution;
    std::shared_ptr<SteinerTreeNode> lastNode = nullptr;
    while (temp) {
      auto it = created.find(temp->vertex);
      if (it == created.end()) {
        PointT point = graph_.getPoint(temp->vertex);
        auto node = std::make_shared<SteinerTreeNode>(point);
        created.emplace(temp->vertex, node);
        if (lastNode) {
          node->addChild(lastNode);
        }
        if (!temp->prev) {
          tree = node;
        }
        if (!lastNode || !temp->prev) {
          // Both the start and the end of the path should contain pins
          int pinIndex = graph_.getVertexPin(temp->vertex);
          assert(pinIndex != -1);
          node->setFixedLayers(graph_.getPseudoPin(pinIndex).second);
        }
        lastNode = node;
        temp = temp->prev;
      } else {
        if (lastNode) {
          it->second->addChild(lastNode);
        }
        break;
      }
    }
  }

  // Remove redundant tree nodes
  SteinerTreeNode::preorder(
      tree, [&](const std::shared_ptr<SteinerTreeNode>& node) {
        for (int childIndex = 0; childIndex < node->getNumChildren();
             childIndex++) {
          std::shared_ptr<SteinerTreeNode> child
              = node->getChildren()[childIndex];
          if (node->x == child->x && node->y == child->y) {
            for (auto& gradchild : child->getChildren()) {
              node->addChild(gradchild);
            }
            if (child->getFixedLayers().IsValid()) {
              if (node->getFixedLayers().IsValid()) {
                node->getFixedLayers().UnionWith(child->getFixedLayers());
              } else {
                node->setFixedLayers(child->getFixedLayers());
              }
            }
            node->removeChild(childIndex);
            childIndex -= 1;
          }
        }
      });

  // Remove intermediate tree nodes
  SteinerTreeNode::preorder(
      tree, [&](const std::shared_ptr<SteinerTreeNode>& node) {
        for (std::shared_ptr<SteinerTreeNode>& child : node->getChildren()) {
          unsigned direction
              = (node->y == child->y ? MetalLayer::H : MetalLayer::V);
          std::shared_ptr<SteinerTreeNode> temp = child;
          while (!temp->getFixedLayers().IsValid()
                 && temp->getNumChildren() == 1
                 && (*temp)[1 - direction]
                        == (*(temp->getChildren()[0]))[1 - direction]) {
            temp = temp->getChildren()[0];
          }
          child = temp;
        }
      });

  // Check duplicate tree nodes
  SteinerTreeNode::preorder(
      tree, [&](const std::shared_ptr<SteinerTreeNode>& node) {
        for (const auto& child : node->getChildren()) {
          if (node->x == child->x && node->y == child->y) {
            printf("Error: duplicate tree nodes encountered.");
          }
        }
      });
  return tree;
}

}  // namespace grt
