#pragma once

#include <vector>

struct KDTreeNode
{
  int id;
  std::pair<int, int> point;
  KDTreeNode* left;
  KDTreeNode* right;

  KDTreeNode(const int& idIn, const std::pair<int, int>& pt)
      : id(idIn), point(pt), left(nullptr), right(nullptr)
  {
  }
};

class KDTree
{
 public:
  KDTree(const std::vector<std::pair<int, int>>& points);

  ~KDTree() { destroyTree(root); }

  std::vector<int> radiusSearch(const std::pair<int, int>& target,
                                int radius) const;

 private:
  KDTreeNode* root;

  KDTreeNode* buildTree(
      const std::vector<std::pair<int, std::pair<int, int>>>& points,
      int depth);

  void radiusSearchHelper(KDTreeNode* node,
                          const std::pair<int, int>& target,
                          int radius,
                          int depth,
                          std::vector<int>& result) const;

  void destroyTree(KDTreeNode* node)
  {
    if (!node)
      return;
    destroyTree(node->left);
    destroyTree(node->right);
    delete node;
  }
};
