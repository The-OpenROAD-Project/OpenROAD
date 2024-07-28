#include "KDTree.hpp"

#include <algorithm>
#include <cmath>

KDTree::KDTree(const std::vector<std::pair<int, int>>& points)
{
  std::vector<std::pair<int, std::pair<int, int>>> pointsWithIds;
  for (int i = 0; i < points.size(); i++) {
    pointsWithIds.push_back(std::make_pair(i, points[i]));
  }
  root = buildTree(pointsWithIds, 0);
}

std::vector<int> KDTree::radiusSearch(const std::pair<int, int>& target,
                                      int radius) const
{
  std::vector<int> result;
  radiusSearchHelper(root, target, radius, 0, result);
  return result;
}

KDTreeNode* KDTree::buildTree(
    const std::vector<std::pair<int, std::pair<int, int>>>& points,
    int depth)
{
  if (points.empty())
    return nullptr;

  auto sorted_points = points;
  size_t axis = depth % 2;
  std::sort(sorted_points.begin(),
            sorted_points.end(),
            [axis](const std::pair<int, std::pair<int, int>>& a,
                   const std::pair<int, std::pair<int, int>>& b) {
              return axis == 0 ? a.second.first < b.second.first
                               : a.second.second < b.second.second;
            });

  size_t median_index = sorted_points.size() / 2;
  KDTreeNode* node = new KDTreeNode(sorted_points[median_index].first,
                                    sorted_points[median_index].second);

  std::vector<std::pair<int, std::pair<int, int>>> left_points(
      sorted_points.begin(), sorted_points.begin() + median_index);
  std::vector<std::pair<int, std::pair<int, int>>> right_points(
      sorted_points.begin() + median_index + 1, sorted_points.end());

  node->left = buildTree(left_points, depth + 1);
  node->right = buildTree(right_points, depth + 1);

  return node;
}

void KDTree::radiusSearchHelper(KDTreeNode* node,
                                const std::pair<int, int>& target,
                                int radius,
                                int depth,
                                std::vector<int>& result) const
{
  if (!node)
    return;

  int distance = std::sqrt(std::pow(node->point.first - target.first, 2)
                           + std::pow(node->point.second - target.second, 2));
  if (distance <= radius) {
    result.push_back(node->id);
  }

  size_t axis = depth % 2;
  int diff = (axis == 0 ? target.first - node->point.first
                        : target.second - node->point.second);

  if (diff < 0) {
    radiusSearchHelper(node->left, target, radius, depth + 1, result);
    if (std::abs(diff) <= radius) {
      radiusSearchHelper(node->right, target, radius, depth + 1, result);
    }
  } else {
    radiusSearchHelper(node->right, target, radius, depth + 1, result);
    if (std::abs(diff) <= radius) {
      radiusSearchHelper(node->left, target, radius, depth + 1, result);
    }
  }
}
