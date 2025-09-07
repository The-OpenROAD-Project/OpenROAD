#pragma once
#include <functional>
#include <memory>
#include <ostream>
#include <vector>

#include "geo.h"

namespace grt {

class GRPoint : public PointT
{
 public:
  GRPoint(int l, int _x, int _y) : PointT(_x, _y), layer_idx_(l) {}
  friend std::ostream& operator<<(std::ostream& os, const GRPoint& pt)
  {
    os << "(" << pt.layer_idx_ << ", " << pt.x << ", " << pt.y << ")";
    return os;
  }
  int getLayerIdx() const { return layer_idx_; }

 private:
  int layer_idx_;
};

class GRTreeNode : public GRPoint
{
 public:
  std::vector<std::shared_ptr<GRTreeNode>> children;

  GRTreeNode(int l, int _x, int _y) : GRPoint(l, _x, _y) {}
  GRTreeNode(const GRPoint& point) : GRPoint(point) {}
  // ~GRTreeNode() { for (auto& child : children) child->~GRTreeNode(); }
  static void preorder(
      const std::shared_ptr<GRTreeNode>& node,
      const std::function<void(const std::shared_ptr<GRTreeNode>&)>& visit);
  static void print(const std::shared_ptr<GRTreeNode>& node);
};

}  // namespace grt
