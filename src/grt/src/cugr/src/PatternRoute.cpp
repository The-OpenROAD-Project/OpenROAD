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
  std::function<void(std::shared_ptr<PatternRoutingNode>)> getEdges
      = [&](const std::shared_ptr<PatternRoutingNode>& node) {
          for (auto& childPaths : node->getPaths()) {
            for (auto& path : childPaths) {
              edges.emplace_back(*node, *path);
              getEdges(path);
            }
          }
        };
  getEdges(std::move(routing_dag_));
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
  auto selectedAccessPoints = grid_graph_->selectAccessPoints(net_);

  const int degree = selectedAccessPoints.size();
  if (degree == 1) {
    const auto& accessPoint = *selectedAccessPoints.begin();
    steiner_tree_ = std::make_shared<SteinerTreeNode>(accessPoint.point,
                                                      accessPoint.layers);
    return;
  }

  std::vector<int> xs;
  std::vector<int> ys;
  for (auto& accessPoint : selectedAccessPoints) {
    xs.push_back(accessPoint.point.x());
    ys.push_back(accessPoint.point.y());
  }

  stt::Tree flutetree = stt_builder_->flute(xs, ys, flute_accuracy_);
  const int numBranches = degree + degree - 2;
  std::vector<PointT> steinerPoints;
  steinerPoints.reserve(numBranches);
  std::vector<std::vector<int>> adjacentList(numBranches);

  for (int branchIndex = 0; branchIndex < numBranches; branchIndex++) {
    const stt::Branch& branch = flutetree.branch[branchIndex];
    steinerPoints.emplace_back(branch.x, branch.y);
    if (branchIndex == branch.n) {
      continue;
    }
    adjacentList[branchIndex].push_back(branch.n);
    adjacentList[branch.n].push_back(branchIndex);
  }

  std::function<void(std::shared_ptr<SteinerTreeNode>&, int, int)> constructTree
      = [&](std::shared_ptr<SteinerTreeNode>& parent,
            int prevIndex,
            int curIndex) {
          std::shared_ptr<SteinerTreeNode> current
              = std::make_shared<SteinerTreeNode>(steinerPoints[curIndex]);
          if (parent != nullptr && parent->x() == current->x()
              && parent->y() == current->y()) {
            for (int nextIndex : adjacentList[curIndex]) {
              if (nextIndex == prevIndex) {
                continue;
              }
              constructTree(parent, curIndex, nextIndex);
            }
            return;
          }
          // Build subtree
          for (int nextIndex : adjacentList[curIndex]) {
            if (nextIndex == prevIndex) {
              continue;
            }
            constructTree(current, curIndex, nextIndex);
          }
          // Set fixed layer interval
          const AccessPoint current_pt{{current->x(), current->y()}, {}};
          if (auto it = selectedAccessPoints.find(current_pt);
              it != selectedAccessPoints.end()) {
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
  std::function<bool(int)> hasDegree1 = [&](int index) {
    if (adjacentList[index].size() == 1) {
      int nextIndex = adjacentList[index][0];
      if (steinerPoints[index] == steinerPoints[nextIndex]) {
        return hasDegree1(nextIndex);
      }
      return true;
    }

    return false;
  };
  for (int i = 0; i < steinerPoints.size(); i++) {
    if (hasDegree1(i)) {
      root = i;
      break;
    }
  }
  constructTree(steiner_tree_, -1, root);
}

void PatternRoute::constructRoutingDAG()
{
  std::function<void(std::shared_ptr<PatternRoutingNode>&,
                     std::shared_ptr<SteinerTreeNode>&)>
      constructDag = [&](std::shared_ptr<PatternRoutingNode>& dstNode,
                         std::shared_ptr<SteinerTreeNode>& steiner) {
        std::shared_ptr<PatternRoutingNode> current
            = std::make_shared<PatternRoutingNode>(
                *steiner, steiner->getFixedLayers(), num_dag_nodes_++);
        for (auto steinerChild : steiner->getChildren()) {
          constructDag(current, steinerChild);
        }
        if (dstNode == nullptr) {
          dstNode = std::move(current);
        } else {
          dstNode->addChild(current);
          constructPaths(dstNode, current);
        }
      };
  constructDag(routing_dag_, steiner_tree_);
}

void PatternRoute::constructPaths(std::shared_ptr<PatternRoutingNode>& start,
                                  std::shared_ptr<PatternRoutingNode>& end,
                                  int child_index)
{
  if (child_index == -1) {
    child_index = start->getPaths().size();
    start->getPaths().emplace_back();
  }
  std::vector<std::shared_ptr<PatternRoutingNode>>& childPaths
      = start->getPaths()[child_index];
  if (start->x() == end->x() || start->y() == end->y()) {
    childPaths.push_back(end);
  } else {
    for (int pathIndex = 0; pathIndex <= 1;
         pathIndex++) {  // two paths of different L-shape
      PointT midPoint = pathIndex ? PointT(start->x(), end->y())
                                  : PointT(end->x(), start->y());
      std::shared_ptr<PatternRoutingNode> mid
          = std::make_shared<PatternRoutingNode>(
              midPoint, num_dag_nodes_++, true);
      mid->getPaths() = {{end}};
      childPaths.push_back(std::move(mid));
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
  std::vector<std::vector<std::shared_ptr<ScaffoldNode>>> scaffoldNodes(
      2,
      std::vector<std::shared_ptr<ScaffoldNode>>(
          num_dag_nodes_,
          nullptr));  // direction -> num_dag_nodes_ -> scaffold node
  std::vector<bool> visited(num_dag_nodes_, false);

  std::function<void(std::shared_ptr<PatternRoutingNode>)> buildScaffolds =
      [&](const std::shared_ptr<PatternRoutingNode>& node) {
        if (visited[node->getIndex()]) {
          return;
        }
        visited[node->getIndex()] = true;

        if (node->isOptional()) {
          assert(node->getPaths().size() == 1 && node->getPaths()[0].size() == 1
                 && !node->getPaths()[0][0]->isOptional());
          auto& path = node->getPaths()[0][0];
          buildScaffolds(path);
          int direction
              = (node->y() == path->y() ? MetalLayer::H : MetalLayer::V);
          if (!scaffoldNodes[direction][path->getIndex()]
              && congestion_view.check(*node, *path)) {
            scaffoldNodes[direction][path->getIndex()]
                = std::make_shared<ScaffoldNode>(path);
          }
        } else {
          for (auto& childPaths : node->getPaths()) {
            for (auto& path : childPaths) {
              buildScaffolds(path);
              int direction
                  = (node->y() == path->y() ? MetalLayer::H : MetalLayer::V);
              if (path->isOptional()) {
                if (!scaffoldNodes[direction][node->getIndex()]
                    && congestion_view.check(*node, *path)) {
                  scaffoldNodes[direction][node->getIndex()]
                      = std::make_shared<ScaffoldNode>(node);
                }
              } else {
                if (congestion_view.check(*node, *path)) {
                  if (!scaffoldNodes[direction][node->getIndex()]) {
                    scaffoldNodes[direction][node->getIndex()]
                        = std::make_shared<ScaffoldNode>(node);
                  }
                  if (!scaffoldNodes[direction][path->getIndex()]) {
                    scaffoldNodes[direction][node->getIndex()]
                        ->children.emplace_back(
                            std::make_shared<ScaffoldNode>(path));
                  } else {
                    scaffoldNodes[direction][node->getIndex()]
                        ->children.emplace_back(
                            scaffoldNodes[direction][path->getIndex()]);
                    scaffoldNodes[direction][path->getIndex()] = nullptr;
                  }
                }
              }
            }
            for (auto& child : node->getChildren()) {
              for (int direction = 0; direction < 2; direction++) {
                if (scaffoldNodes[direction][child->getIndex()]) {
                  scaffolds[direction].emplace_back(
                      std::make_shared<ScaffoldNode>(node));
                  scaffolds[direction].back()->children.emplace_back(
                      scaffoldNodes[direction][child->getIndex()]);
                  scaffoldNodes[direction][child->getIndex()] = nullptr;
                }
              }
            }
          }
        }
      };

  buildScaffolds(routing_dag_);
  for (int direction = 0; direction < 2; direction++) {
    if (scaffoldNodes[direction][routing_dag_->getIndex()]) {
      scaffolds[direction].emplace_back(
          std::make_shared<ScaffoldNode>(nullptr));
      scaffolds[direction].back()->children.emplace_back(
          scaffoldNodes[direction][routing_dag_->getIndex()]);
    }
  }

  std::function<void(const std::shared_ptr<ScaffoldNode>&,
                     IntervalT&,
                     std::vector<int>&,
                     int,
                     bool)>
      getTrunkAndStems = [&](const std::shared_ptr<ScaffoldNode>& scaffoldNode,
                             IntervalT& trunk,
                             std::vector<int>& stems,
                             int direction,
                             bool starting) {
        if (starting) {
          if (scaffoldNode->node) {
            stems.emplace_back((*scaffoldNode->node)[1 - direction]);
            trunk.Update((*scaffoldNode->node)[direction]);
          }
          for (auto& scaffoldChild : scaffoldNode->children) {
            getTrunkAndStems(scaffoldChild, trunk, stems, direction, false);
          }
        } else {
          trunk.Update((*scaffoldNode->node)[direction]);
          if (scaffoldNode->node->getFixedLayers().IsValid()) {
            stems.emplace_back((*scaffoldNode->node)[1 - direction]);
          }
          for (const auto& treeChild : scaffoldNode->node->getChildren()) {
            bool scaffolded = false;
            for (auto& scaffoldChild : scaffoldNode->children) {
              if (treeChild == scaffoldChild->node) {
                getTrunkAndStems(scaffoldChild, trunk, stems, direction, false);
                scaffolded = true;
                break;
              }
            }
            if (!scaffolded) {
              stems.emplace_back((*treeChild)[1 - direction]);
              trunk.Update((*treeChild)[direction]);
            }
          }
        }
      };

  auto getTotalStemLength = [&](const std::vector<int>& stems, const int pos) {
    int length = 0;
    for (int stem : stems) {
      length += abs(stem - pos);
    }
    return length;
  };

  std::function<std::shared_ptr<PatternRoutingNode>(
      std::shared_ptr<ScaffoldNode>, int, int)>
      buildDetour = [&](const std::shared_ptr<ScaffoldNode>& scaffoldNode,
                        int direction,
                        int shiftAmount) {
        std::shared_ptr<PatternRoutingNode> treeNode = scaffoldNode->node;
        if (treeNode->getFixedLayers().IsValid()) {
          std::shared_ptr<PatternRoutingNode> dupTreeNode
              = std::make_shared<PatternRoutingNode>((PointT) *treeNode,
                                                     treeNode->getFixedLayers(),
                                                     num_dag_nodes_++);
          std::shared_ptr<PatternRoutingNode> shiftedTreeNode
              = std::make_shared<PatternRoutingNode>((PointT) *treeNode,
                                                     num_dag_nodes_++);
          (*shiftedTreeNode)[1 - direction] += shiftAmount;
          constructPaths(shiftedTreeNode, dupTreeNode);
          for (auto& treeChild : treeNode->getChildren()) {
            bool built = false;
            for (auto& scaffoldChild : scaffoldNode->children) {
              if (treeChild == scaffoldChild->node) {
                auto shiftedChildTreeNode
                    = buildDetour(scaffoldChild, direction, shiftAmount);
                constructPaths(shiftedTreeNode, shiftedChildTreeNode);
                built = true;
                break;
              }
            }
            if (!built) {
              constructPaths(shiftedTreeNode, treeChild);
            }
          }
          return shiftedTreeNode;
        }
        std::shared_ptr<PatternRoutingNode> shiftedTreeNode
            = std::make_shared<PatternRoutingNode>((PointT) *treeNode,
                                                   num_dag_nodes_++);
        (*shiftedTreeNode)[1 - direction] += shiftAmount;
        for (auto& treeChild : treeNode->getChildren()) {
          bool built = false;
          for (auto& scaffoldChild : scaffoldNode->children) {
            if (treeChild == scaffoldChild->node) {
              auto shiftedChildTreeNode
                  = buildDetour(scaffoldChild, direction, shiftAmount);
              constructPaths(shiftedTreeNode, shiftedChildTreeNode);
              built = true;
              break;
            }
          }
          if (!built) {
            constructPaths(shiftedTreeNode, treeChild);
          }
        }
        return shiftedTreeNode;
      };

  for (int direction = 0; direction < 2; direction++) {
    for (const std::shared_ptr<ScaffoldNode>& scaffold : scaffolds[direction]) {
      assert(scaffold->children.size() == 1);

      IntervalT trunk;
      std::vector<int> stems;
      getTrunkAndStems(scaffold, trunk, stems, direction, true);
      std::ranges::stable_sort(stems);
      int trunkPos = (*scaffold->children[0]->node)[1 - direction];
      int originalLength = getTotalStemLength(stems, trunkPos);
      IntervalT shiftInterval(trunkPos);
      int maxLengthIncrease = trunk.range() * constants_.max_detour_ratio;
      while (shiftInterval.low() - 1 >= 0
             && getTotalStemLength(stems, shiftInterval.low() - 1)
                        - originalLength
                    <= maxLengthIncrease) {
        shiftInterval.addToLow(-1);
      }
      while (shiftInterval.high() + 1 < grid_graph_->getSize(1 - direction)
             && getTotalStemLength(stems, shiftInterval.high() - 1)
                        - originalLength
                    <= maxLengthIncrease) {
        shiftInterval.addToHigh(1);
      }
      int step = 1;
      while ((trunkPos - shiftInterval.low()) / (step + 1)
                 + (shiftInterval.high() - trunkPos) / (step + 1)
             >= constants_.target_detour_count) {
        step++;
      }

      shiftInterval.Set(
          trunkPos - (trunkPos - shiftInterval.low()) / step * step,
          trunkPos + (shiftInterval.high() - trunkPos) / step * step);
      for (int pos = shiftInterval.low(); pos <= shiftInterval.high();
           pos += step) {
        int shiftAmount = (pos - trunkPos);
        if (shiftAmount == 0) {
          continue;
        }
        if (scaffold->node) {
          auto& scaffoldChild = scaffold->children[0];
          if ((*scaffoldChild->node)[1 - direction] + shiftAmount < 0
              || (*scaffoldChild->node)[1 - direction] + shiftAmount
                     >= grid_graph_->getSize(1 - direction)) {
            continue;
          }
          for (int child_index = 0;
               child_index < scaffold->node->getNumChildren();
               child_index++) {
            auto& treeChild = scaffold->node->getChildren()[child_index];
            if (treeChild == scaffoldChild->node) {
              std::shared_ptr<PatternRoutingNode> shiftedChild
                  = buildDetour(scaffoldChild, direction, shiftAmount);
              constructPaths(scaffold->node, shiftedChild, child_index);
            }
          }
        } else {
          std::shared_ptr<ScaffoldNode> scaffoldNode = scaffold->children[0];
          auto treeNode = scaffoldNode->node;
          if (treeNode->getNumChildren() == 1) {
            if ((*treeNode)[1 - direction] + shiftAmount < 0
                || (*treeNode)[1 - direction] + shiftAmount
                       >= grid_graph_->getSize(1 - direction)) {
              continue;
            }
            std::shared_ptr<PatternRoutingNode> shiftedTreeNode
                = std::make_shared<PatternRoutingNode>((PointT) *treeNode,
                                                       num_dag_nodes_++);
            (*shiftedTreeNode)[1 - direction] += shiftAmount;
            constructPaths(treeNode, shiftedTreeNode, 0);
            for (auto& treeChild : treeNode->getChildren()) {
              bool built = false;
              for (auto& scaffoldChild : scaffoldNode->children) {
                if (treeChild == scaffoldChild->node) {
                  auto shiftedChildTreeNode
                      = buildDetour(scaffoldChild, direction, shiftAmount);
                  constructPaths(shiftedTreeNode, shiftedChildTreeNode);
                  built = true;
                  break;
                }
              }
              if (!built) {
                constructPaths(shiftedTreeNode, treeChild);
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
      childCosts;  // child_index -> layerIndex -> (cost, pathIndex)
  // Calculate child costs
  if (!node->getPaths().empty()) {
    childCosts.resize(node->getPaths().size());
  }
  for (int child_index = 0; child_index < node->getPaths().size();
       child_index++) {
    auto& childPaths = node->getPaths()[child_index];
    auto& costs = childCosts[child_index];
    costs.assign(grid_graph_->getNumLayers(),
                 {std::numeric_limits<CostT>::max(), -1});
    for (int pathIndex = 0; pathIndex < childPaths.size(); pathIndex++) {
      std::shared_ptr<PatternRoutingNode>& path = childPaths[pathIndex];
      calculateRoutingCosts(path);
      int direction = node->x() == path->x() ? MetalLayer::V : MetalLayer::H;
      assert((*node)[1 - direction] == (*path)[1 - direction]);
      for (int layerIndex = constants_.min_routing_layer;
           layerIndex < grid_graph_->getNumLayers();
           layerIndex++) {
        if (grid_graph_->getLayerDirection(layerIndex) != direction) {
          continue;
        }
        CostT cost
            = net_->isInsideLayerRange(layerIndex)
                  ? path->getCosts()[layerIndex]
                        + grid_graph_->getWireCost(layerIndex, *node, *path)
                  : std::numeric_limits<CostT>::max();
        if (cost < costs[layerIndex].first) {
          costs[layerIndex] = std::make_pair(cost, pathIndex);
        }
      }
    }
  }

  node->getCosts().assign(grid_graph_->getNumLayers(),
                          std::numeric_limits<CostT>::max());
  node->getBestPaths().resize(grid_graph_->getNumLayers());
  if (!node->getPaths().empty()) {
    for (int layerIndex = 1; layerIndex < grid_graph_->getNumLayers();
         layerIndex++) {
      node->getBestPaths()[layerIndex].assign(node->getPaths().size(),
                                              {-1, -1});
    }
  }
  // Calculate the partial sum of the via costs
  std::vector<CostT> viaCosts(grid_graph_->getNumLayers());
  viaCosts[0] = 0;
  for (int layerIndex = 1; layerIndex < grid_graph_->getNumLayers();
       layerIndex++) {
    viaCosts[layerIndex] = viaCosts[layerIndex - 1]
                           + grid_graph_->getViaCost(layerIndex - 1, *node);
  }
  IntervalT fixedLayers(node->getFixedLayers());
  fixedLayers.Set(std::min(fixedLayers.low(), grid_graph_->getNumLayers() - 1),
                  std::max(fixedLayers.high(), constants_.min_routing_layer));

  for (int lowLayerIndex = 0; lowLayerIndex <= fixedLayers.low();
       lowLayerIndex++) {
    std::vector<CostT> minChildCosts;
    std::vector<std::pair<int, int>> bestPaths;
    if (!node->getPaths().empty()) {
      minChildCosts.assign(node->getPaths().size(),
                           std::numeric_limits<CostT>::max());
      bestPaths.assign(node->getPaths().size(), {-1, -1});
    }
    for (int layerIndex = lowLayerIndex;
         layerIndex < grid_graph_->getNumLayers();
         layerIndex++) {
      for (int child_index = 0; child_index < node->getPaths().size();
           child_index++) {
        if (childCosts[child_index][layerIndex].first
            < minChildCosts[child_index]) {
          minChildCosts[child_index]
              = childCosts[child_index][layerIndex].first;
          bestPaths[child_index] = std::make_pair(
              childCosts[child_index][layerIndex].second, layerIndex);
        }
      }
      if (layerIndex >= fixedLayers.high()) {
        CostT cost = viaCosts[layerIndex] - viaCosts[lowLayerIndex];
        for (CostT childCost : minChildCosts) {
          cost += childCost;
        }
        if (cost < node->getCosts()[layerIndex]) {
          node->getCosts()[layerIndex] = cost;
          node->getBestPaths()[layerIndex] = bestPaths;
        }
      }
    }
    for (int layerIndex = grid_graph_->getNumLayers() - 2;
         layerIndex >= lowLayerIndex;
         layerIndex--) {
      if (node->getCosts()[layerIndex + 1] < node->getCosts()[layerIndex]) {
        node->getCosts()[layerIndex] = node->getCosts()[layerIndex + 1];
        node->getBestPaths()[layerIndex] = node->getBestPaths()[layerIndex + 1];
      }
    }
  }
}

std::shared_ptr<GRTreeNode> PatternRoute::getRoutingTree(
    std::shared_ptr<PatternRoutingNode>& node,
    int parentLayerIndex)
{
  if (parentLayerIndex == -1) {
    CostT minCost = std::numeric_limits<CostT>::max();
    for (int layerIndex = 0; layerIndex < grid_graph_->getNumLayers();
         layerIndex++) {
      if (routing_dag_->getCosts()[layerIndex] < minCost) {
        minCost = routing_dag_->getCosts()[layerIndex];
        parentLayerIndex = layerIndex;
      }
    }
  }
  assert(parentLayerIndex >= 0);
  if (parentLayerIndex < 0) {
    logger_->error(utl::GRT,
                   286,
                   "Failed to determine parent layer index on net {}.",
                   net_->getName());
  }
  std::shared_ptr<GRTreeNode> routingNode
      = std::make_shared<GRTreeNode>(parentLayerIndex, node->x(), node->y());
  std::shared_ptr<GRTreeNode> lowestRoutingNode = routingNode;
  std::shared_ptr<GRTreeNode> highestRoutingNode = routingNode;
  if (!node->getPaths().empty()) {
    int pathIndex, layerIndex;
    std::vector<std::vector<std::shared_ptr<PatternRoutingNode>>> pathsOnLayer(
        grid_graph_->getNumLayers());
    for (int child_index = 0; child_index < node->getPaths().size();
         child_index++) {
      std::tie(pathIndex, layerIndex)
          = node->getBestPaths()[parentLayerIndex][child_index];
      pathsOnLayer[layerIndex].push_back(
          node->getPaths()[child_index][pathIndex]);
    }
    if (!pathsOnLayer[parentLayerIndex].empty()) {
      for (auto& path : pathsOnLayer[parentLayerIndex]) {
        routingNode->addChild(getRoutingTree(path, parentLayerIndex));
      }
    }
    for (int layerIndex = parentLayerIndex - 1; layerIndex >= 0; layerIndex--) {
      if (!pathsOnLayer[layerIndex].empty()) {
        lowestRoutingNode->addChild(
            std::make_shared<GRTreeNode>(layerIndex, node->x(), node->y()));
        lowestRoutingNode = lowestRoutingNode->getChildren().back();
        for (auto& path : pathsOnLayer[layerIndex]) {
          lowestRoutingNode->addChild(getRoutingTree(path, layerIndex));
        }
      }
    }
    for (int layerIndex = parentLayerIndex + 1;
         layerIndex < grid_graph_->getNumLayers();
         layerIndex++) {
      if (!pathsOnLayer[layerIndex].empty()) {
        highestRoutingNode->addChild(
            std::make_shared<GRTreeNode>(layerIndex, node->x(), node->y()));
        highestRoutingNode = highestRoutingNode->getChildren().back();
        for (auto& path : pathsOnLayer[layerIndex]) {
          highestRoutingNode->addChild(getRoutingTree(path, layerIndex));
        }
      }
    }
  }
  if (lowestRoutingNode->getLayerIdx() > node->getFixedLayers().low()) {
    lowestRoutingNode->addChild(std::make_shared<GRTreeNode>(
        node->getFixedLayers().low(), node->x(), node->y()));
  }
  if (highestRoutingNode->getLayerIdx() < node->getFixedLayers().high()) {
    highestRoutingNode->addChild(std::make_shared<GRTreeNode>(
        node->getFixedLayers().high(), node->x(), node->y()));
  }
  return routingNode;
}

}  // namespace grt
