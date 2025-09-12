#include "GRTree.h"

#include <functional>
#include <iostream>
#include <memory>
#include <sstream>

#include "utl/Logger.h"

namespace grt {

void GRTreeNode::preorder(
    const std::shared_ptr<GRTreeNode>& node,
    const std::function<void(const std::shared_ptr<GRTreeNode>&)>& visit)
{
  visit(node);
  for (auto& child : node->children_) {
    preorder(child, visit);
  }
}

void GRTreeNode::print(const std::shared_ptr<GRTreeNode>& node,
                       utl::Logger* logger)
{
  preorder(node, [logger](const std::shared_ptr<GRTreeNode>& node) {
    std::stringstream ss;
    ss << *node << (!node->children_.empty() ? " -> " : "");
    for (auto& child : node->children_) {
      ss << *child << (child == node->children_.back() ? "" : ", ");
    }
    logger->report("{}", ss.str());
  });
}

}  // namespace grt
