// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

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
