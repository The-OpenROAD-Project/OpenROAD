#include "utl/unionFind.h"

#include <vector>

namespace utl {
UnionFind::UnionFind(int n) : parent(n), rank(n, 0)
{
  for (int i = 0; i < n; i++) {
    parent[i] = i;
  }
}

int UnionFind::find(int x)
{
  if (parent[x] != x) {
    parent[x] = find(parent[x]);
  }
  return parent[x];
}

void UnionFind::unite(int x, int y)
{
  int px = find(x);
  int py = find(y);
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
