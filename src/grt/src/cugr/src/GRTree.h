#pragma once
#include <functional>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

#include "geo.h"
#include "utl/Logger.h"

namespace grt {

class GRPoint : public PointT
{
 public:
  GRPoint(int l, int _x, int _y) : PointT(_x, _y), layer_idx_(l) {}

  int getLayerIdx() const { return layer_idx_; }

  friend std::ostream& operator<<(std::ostream& os, const GRPoint& pt)
  {
    os << "(" << pt.layer_idx_ << ", " << pt.x() << ", " << pt.y() << ")";
    return os;
  }

 private:
  int layer_idx_;
};

class GRTreeNode : public GRPoint
{
 public:
  GRTreeNode(int l, int _x, int _y) : GRPoint(l, _x, _y) {}
  GRTreeNode(const GRPoint& point) : GRPoint(point) {}

  const std::vector<std::shared_ptr<GRTreeNode>>& getChildren() const
  {
    return children_;
  }

  void addChild(std::shared_ptr<GRTreeNode> child)
  {
    children_.push_back(std::move(child));
  }

  static void preorder(
      const std::shared_ptr<GRTreeNode>& node,
      const std::function<void(const std::shared_ptr<GRTreeNode>&)>& visit);
  static void print(const std::shared_ptr<GRTreeNode>& node,
                    utl::Logger* logger);

 private:
  std::vector<std::shared_ptr<GRTreeNode>> children_;
};

}  // namespace grt
