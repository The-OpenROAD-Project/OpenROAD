#pragma once
#include <cstddef>
#include <vector>

namespace utl {
class UnionFind
{
 public:
  explicit UnionFind(size_t n);
  size_t find(size_t x);
  void unite(size_t x, size_t y);

 private:
  std::vector<size_t> parent;
  std::vector<size_t> rank;
};
}  // namespace utl
