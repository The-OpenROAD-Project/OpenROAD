// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "neighbors.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <numeric>
#include <tuple>
#include <vector>

#include "odb/geom.h"

namespace pdr {

using odb::Point;
using std::vector;

// This is based on the double monotone chains algorithm described in
// "Provably Optimal Planar Pareto Nearest Neighbor Search with Double
// Monotone Chains" (Guo et al, DATE'26).  As in the paper, the nodes are
// swept in increasing x and the left neighbors of each node form a
// monotone staircase above and below it that is walked one neighbor at
// a time.
//
// The paper assumes distinct coordinates.  Shared x or y values are
// common here (pins on rows and sites) so this differs from it:
//  - Coincident nodes are merged and linked to the first node at
//    their location.
//  - Nodes are swept a column (equal x) at a time.  Nodes in a column
//    are linked to their adjacent column mates and bound the rows in
//    which their left neighbors can lie.
//  - Only the rightmost swept node in each row is a candidate.  A
//    segment tree over the rows of these nodes finds each staircase
//    step in O(log n) instead of following the paper's chain pointers.
//    With ties, a column of k nodes can force O(k) chain pointer
//    updates per inserted node.

namespace {

// Max segment tree over y rows holding the x of the rightmost pin swept
// so far in each row.
class RowMaxTree
{
 public:
  explicit RowMaxTree(const int num_rows)
  {
    while (size_ < num_rows) {
      size_ *= 2;
    }
    max_x_.assign(2 * static_cast<std::size_t>(size_),
                  std::numeric_limits<int>::min());
  }

  int x(const int row) const { return max_x_[size_ + row]; }

  // Row x values only increase as the sweep moves right.
  void set(const int row, const int x)
  {
    std::size_t node = size_ + row;
    max_x_[node] = x;
    for (node /= 2; node > 0; node /= 2) {
      max_x_[node] = std::max(max_x_[2 * node], max_x_[2 * node + 1]);
    }
  }

  // Lowest row in [lo, hi) whose x is > min_x, or -1.
  int first(const int lo, const int hi, const int min_x) const
  {
    if (lo >= hi) {
      return -1;
    }
    return first(1, 0, size_, lo, hi, min_x);
  }

  // Highest row in [lo, hi) whose x is > min_x, or -1.
  int last(const int lo, const int hi, const int min_x) const
  {
    if (lo >= hi) {
      return -1;
    }
    return last(1, 0, size_, lo, hi, min_x);
  }

 private:
  int first(const int node,
            const int node_lo,
            const int node_hi,
            const int lo,
            const int hi,
            const int min_x) const
  {
    if (node_hi <= lo || hi <= node_lo || max_x_[node] <= min_x) {
      return -1;
    }
    if (node_hi - node_lo == 1) {
      return node_lo;
    }
    const int mid = (node_lo + node_hi) / 2;
    const int row = first(2 * node, node_lo, mid, lo, hi, min_x);
    return row >= 0 ? row : first(2 * node + 1, mid, node_hi, lo, hi, min_x);
  }

  int last(const int node,
           const int node_lo,
           const int node_hi,
           const int lo,
           const int hi,
           const int min_x) const
  {
    if (node_hi <= lo || hi <= node_lo || max_x_[node] <= min_x) {
      return -1;
    }
    if (node_hi - node_lo == 1) {
      return node_lo;
    }
    const int mid = (node_lo + node_hi) / 2;
    const int row = last(2 * node + 1, mid, node_hi, lo, hi, min_x);
    return row >= 0 ? row : last(2 * node, node_lo, mid, lo, hi, min_x);
  }

  int size_ = 1;
  vector<int> max_x_;
};

}  // namespace

vector<Neighbors> getNearestNeighbors(const vector<Point>& pts)
{
  const int pt_count = pts.size();
  vector<Neighbors> neighbors(pt_count);
  auto link = [&neighbors](const int a, const int b) {
    neighbors[a].push_back(b);
    neighbors[b].push_back(a);
  };

  // Sweep order: columns left to right, each column bottom to top.
  // Coincident points are adjacent with the lowest index first.
  vector<int> sorted(pt_count);
  std::iota(sorted.begin(), sorted.end(), 0);
  std::sort(sorted.begin(), sorted.end(), [&pts](const int i, const int j) {
    return std::make_tuple(pts[i].getX(), pts[i].getY(), i)
           < std::make_tuple(pts[j].getX(), pts[j].getY(), j);
  });

  // A coincident point only needs a zero length edge to the first point
  // at its location, which stands in for it in the sweep.
  vector<int> unique_pts;
  unique_pts.reserve(pt_count);
  for (const int pt : sorted) {
    if (!unique_pts.empty() && pts[unique_pts.back()] == pts[pt]) {
      link(unique_pts.back(), pt);
    } else {
      unique_pts.push_back(pt);
    }
  }

  vector<int> row_ys;
  row_ys.reserve(unique_pts.size());
  for (const int pt : unique_pts) {
    row_ys.push_back(pts[pt].getY());
  }
  std::sort(row_ys.begin(), row_ys.end());
  row_ys.erase(std::unique(row_ys.begin(), row_ys.end()), row_ys.end());
  const int num_rows = row_ys.size();
  auto row_of = [&row_ys](const int y) {
    return std::lower_bound(row_ys.begin(), row_ys.end(), y) - row_ys.begin();
  };

  // Only the rightmost swept point in a row can neighbor a later point
  // as any other point in that row lies on the bbox boundary.
  RowMaxTree swept(num_rows);
  vector<int> row_pt(num_rows, -1);

  const int unique_count = unique_pts.size();
  vector<int> col_rows;
  for (int col_begin = 0; col_begin < unique_count;) {
    const int col_x = pts[unique_pts[col_begin]].getX();
    int col_end = col_begin;
    col_rows.clear();
    while (col_end < unique_count && pts[unique_pts[col_end]].getX() == col_x) {
      col_rows.push_back(row_of(pts[unique_pts[col_end]].getY()));
      ++col_end;
    }

    for (int k = col_begin; k < col_end; ++k) {
      const int pt = unique_pts[k];
      const int row = col_rows[k - col_begin];
      // Points in the same column bound the rows that can hold a neighbor
      const bool has_below = k > col_begin;
      const bool has_above = k + 1 < col_end;
      const int lo_row = has_below ? col_rows[k - col_begin - 1] + 1 : 0;
      const int hi_row = has_above ? col_rows[k - col_begin + 1] : num_rows;
      if (has_above) {
        link(pt, unique_pts[k + 1]);
      }

      // Upper left staircase, starting with the point in the same row.
      // Each step must be strictly closer in x than the last or the last
      // lies in its bbox.
      int min_x = std::numeric_limits<int>::min();
      for (int r = swept.first(row, hi_row, min_x); r >= 0;
           r = swept.first(r + 1, hi_row, min_x)) {
        link(pt, row_pt[r]);
        min_x = swept.x(r);
      }

      // Lower left staircase.  A point in the same row bounds it too.
      min_x = swept.x(row);
      for (int r = swept.last(lo_row, row, min_x); r >= 0;
           r = swept.last(lo_row, r, min_x)) {
        link(pt, row_pt[r]);
        min_x = swept.x(r);
      }
    }

    for (int k = col_begin; k < col_end; ++k) {
      const int row = col_rows[k - col_begin];
      swept.set(row, col_x);
      row_pt[row] = unique_pts[k];
    }
    col_begin = col_end;
  }

  // Make the search order independent of the sweep order
  for (Neighbors& pt_neighbors : neighbors) {
    std::sort(pt_neighbors.begin(), pt_neighbors.end());
  }

  return neighbors;
}

}  // namespace pdr
