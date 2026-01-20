// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include "utl/unionFind.h"

#include <cstddef>
#include <numeric>

namespace utl {
UnionFind::UnionFind(size_t n) : parent(n), rank(n, 0)
{
  std::iota(parent.begin(), parent.end(), 0);
}

size_t UnionFind::find(size_t x)
{
  if (parent[x] != x) {
    parent[x] = find(parent[x]);
  }
  return parent[x];
}

void UnionFind::unite(size_t x, size_t y)
{
  const size_t px = find(x);
  const size_t py = find(y);
  if (px == py) {
    return;
  }
  if (rank[px] < rank[py]) {
    parent[px] = py;
  } else if (rank[px] > rank[py]) {
    parent[py] = px;
  } else {
    parent[py] = px;
    rank[px]++;
  }
}
}  // namespace utl
