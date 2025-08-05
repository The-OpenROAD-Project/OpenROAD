#pragma once
#include "Design.h"
#include "GridGraph.h"
#include "global.h"

class GRNet
{
 public:
  GRNet(const Net& baseNet, const Design& design, const GridGraph& gridGraph);

  int getIndex() const { return index; }
  std::string getName() const { return name; }
  int getNumPins() const { return pinAccessPoints.size(); }
  const std::vector<std::vector<GRPoint>>& getPinAccessPoints() const
  {
    return pinAccessPoints;
  }
  const utils::BoxT<int>& getBoundingBox() const { return boundingBox; }
  const std::shared_ptr<GRTreeNode>& getRoutingTree() const
  {
    return routingTree;
  }
  // void getGuides(std::vector<std::pair<int, utils::BoxT<int>>>& guides)
  // const;

  void setRoutingTree(std::shared_ptr<GRTreeNode> tree) { routingTree = tree; }
  void clearRoutingTree() { routingTree = nullptr; }

 private:
  int index;
  std::string name;
  std::vector<std::vector<GRPoint>> pinAccessPoints;
  utils::BoxT<int> boundingBox;
  std::shared_ptr<GRTreeNode> routingTree;
};