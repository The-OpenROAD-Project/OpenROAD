#include "GRTree.h"

namespace grt {

void GRTreeNode::preorder(
    const std::shared_ptr<GRTreeNode>& node,
    std::function<void(const std::shared_ptr<GRTreeNode>&)> visit)
{
  visit(node);
  for (auto& child : node->children) {
    preorder(child, visit);
  }
}

void GRTreeNode::print(std::shared_ptr<GRTreeNode> node)
{
  preorder(std::move(node), [](const std::shared_ptr<GRTreeNode>& node) {
    std::cout << *node << (!node->children.empty() ? " -> " : "");
    for (auto& child : node->children) {
      std::cout << *child << (child == node->children.back() ? "" : ", ");
    }
    std::cout << std::endl;
  });
}

}  // namespace grt