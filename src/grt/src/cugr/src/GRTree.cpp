#include "GRTree.h"

#include <functional>
#include <iostream>
#include <memory>

namespace grt {

void GRTreeNode::preorder(
    const std::shared_ptr<GRTreeNode>& node,
    const std::function<void(const std::shared_ptr<GRTreeNode>&)>& visit)
{
  visit(node);
  for (auto& child : node->children) {
    preorder(child, visit);
  }
}

void GRTreeNode::print(const std::shared_ptr<GRTreeNode>& node)
{
  preorder(node, [](const std::shared_ptr<GRTreeNode>& node) {
    std::cout << *node << (!node->children.empty() ? " -> " : "");
    for (auto& child : node->children) {
      std::cout << *child << (child == node->children.back() ? "" : ", ");
    }
    std::cout << '\n';
  });
}

}  // namespace grt
