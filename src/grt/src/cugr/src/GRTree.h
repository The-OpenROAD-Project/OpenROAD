#pragma once
#include "global.h"

namespace grt {

class GRPoint : public PointT<int>
{
 public:
  // int x
  // int y
  int layerIdx;
  GRPoint(int l, int _x, int _y) : layerIdx(l), PointT<int>(_x, _y) {}
  friend inline std::ostream& operator<<(std::ostream& os, const GRPoint& pt)
  {
    os << "(" << pt.layerIdx << ", " << pt.x << ", " << pt.y << ")";
    return os;
  }
};

class GRTreeNode : public GRPoint
{
 public:
  std::vector<std::shared_ptr<GRTreeNode>> children;

  GRTreeNode(int l, int _x, int _y) : GRPoint(l, _x, _y) {}
  GRTreeNode(const GRPoint& point) : GRPoint(point) {}
  // ~GRTreeNode() { for (auto& child : children) child->~GRTreeNode(); }
  static void preorder(std::shared_ptr<GRTreeNode> node,
                       std::function<void(std::shared_ptr<GRTreeNode>)> visit);
  static void print(std::shared_ptr<GRTreeNode> node);
};

}  // namespace grt
