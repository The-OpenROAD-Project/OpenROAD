#pragma once
#include <vector>
namespace utl {
class UnionFind
{
 public:
  explicit UnionFind(int n);
  int find(int x);
  void unite(int x, int y);

 private:
  std::vector<int> parent;
  std::vector<int> rank;
};
}  // namespace utl
