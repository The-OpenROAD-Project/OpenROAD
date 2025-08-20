// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "FastRoute.h"

namespace grt {

void Graph2D::init(const int x_grid,
                   const int y_grid,
                   const int h_capacity,
                   const int v_capacity)
{
  h_edges_.resize(boost::extents[x_grid - 1][y_grid]);
  v_edges_.resize(boost::extents[x_grid][y_grid - 1]);

  for (int x = 0; x < x_grid - 1; x++) {
    for (int y = 0; y < y_grid; y++) {
      // Edge initialization
      h_edges_[x][y].cap = h_capacity;
      h_edges_[x][y].usage = 0;
      h_edges_[x][y].est_usage = 0;
      h_edges_[x][y].red = 0;
      h_edges_[x][y].last_usage = 0;
    }
  }
  for (int x = 0; x < x_grid; x++) {
    for (int y = 0; y < y_grid - 1; y++) {
      // Edge initialization
      v_edges_[x][y].cap = v_capacity;
      v_edges_[x][y].usage = 0;
      v_edges_[x][y].est_usage = 0;
      v_edges_[x][y].red = 0;
      v_edges_[x][y].last_usage = 0;
    }
  }
}

void Graph2D::InitEstUsage()
{
  foreachEdge([](Edge& edge) { edge.est_usage = 0; });
}

void Graph2D::InitLastUsage(const int upType)
{
  foreachEdge([](Edge& edge) { edge.last_usage = 0; });

  if (upType == 1) {
    foreachEdge([](Edge& edge) { edge.congCNT = 0; });
  } else if (upType == 2) {
    foreachEdge([](Edge& edge) { edge.last_usage = edge.last_usage * 0.2; });
  }
}

void Graph2D::clear()
{
  h_edges_.resize(boost::extents[0][0]);
  v_edges_.resize(boost::extents[0][0]);
}

void Graph2D::clearUsed()
{
  v_used_ggrid_.clear();
  h_used_ggrid_.clear();
}

bool Graph2D::hasEdges() const
{
  return !h_edges_.empty() && !v_edges_.empty();
}

uint16_t Graph2D::getUsageH(const int x, const int y) const
{
  return h_edges_[x][y].usage;
}

uint16_t Graph2D::getUsageV(const int x, const int y) const
{
  return v_edges_[x][y].usage;
}

int16_t Graph2D::getLastUsageH(int x, int y) const
{
  return h_edges_[x][y].last_usage;
}

int16_t Graph2D::getLastUsageV(int x, int y) const
{
  return v_edges_[x][y].last_usage;
}

double Graph2D::getEstUsageH(const int x, const int y) const
{
  return h_edges_[x][y].est_usage;
}

double Graph2D::getEstUsageV(const int x, const int y) const
{
  return v_edges_[x][y].est_usage;
}

uint16_t Graph2D::getUsageRedH(const int x, const int y) const
{
  return h_edges_[x][y].usage_red();
}

uint16_t Graph2D::getUsageRedV(const int x, const int y) const
{
  return v_edges_[x][y].usage_red();
}

double Graph2D::getEstUsageRedH(const int x, const int y) const
{
  return h_edges_[x][y].est_usage_red();
}

double Graph2D::getEstUsageRedV(const int x, const int y) const
{
  return v_edges_[x][y].est_usage_red();
}

int Graph2D::getOverflowH(const int x, const int y) const
{
  const auto& edge = h_edges_[x][y];
  return edge.usage - edge.cap;
}

int Graph2D::getOverflowV(const int x, const int y) const
{
  const auto& edge = v_edges_[x][y];
  return edge.usage - edge.cap;
}

uint16_t Graph2D::getCapH(int x, int y) const
{
  return h_edges_[x][y].cap;
}

uint16_t Graph2D::getCapV(int x, int y) const
{
  return v_edges_[x][y].cap;
}

const std::set<std::pair<int, int>>& Graph2D::getUsedGridsH() const
{
  return h_used_ggrid_;
}

const std::set<std::pair<int, int>>& Graph2D::getUsedGridsV() const
{
  return v_used_ggrid_;
}

void Graph2D::addCapH(const int x, const int y, const int cap)
{
  h_edges_[x][y].cap += cap;
}

void Graph2D::addCapV(const int x, const int y, const int cap)
{
  v_edges_[x][y].cap += cap;
}

void Graph2D::addEstUsageH(const Interval& xi, const int y, const double usage)
{
  for (int x = xi.lo; x < xi.hi; x++) {
    h_edges_[x][y].est_usage += usage;
    if (usage > 0) {
      h_used_ggrid_.insert({x, y});
    }
  }
}

void Graph2D::addEstUsageH(const int x, const int y, const double usage)
{
  h_edges_[x][y].est_usage += usage;
  if (usage > 0) {
    h_used_ggrid_.insert({x, y});
  }
}

void Graph2D::addEstUsageToUsage()
{
  foreachEdge([](Edge& edge) { edge.usage += edge.est_usage; });
}

void Graph2D::addEstUsageV(const int x, const Interval& yi, const double usage)
{
  for (int y = yi.lo; y < yi.hi; y++) {
    v_edges_[x][y].est_usage += usage;
    if (usage > 0) {
      v_used_ggrid_.insert({x, y});
    }
  }
}

void Graph2D::addEstUsageV(const int x, const int y, const double usage)
{
  v_edges_[x][y].est_usage += usage;
  if (usage > 0) {
    v_used_ggrid_.insert({x, y});
  }
}

void Graph2D::addRedH(const int x, const int y, const int red)
{
  auto& val = h_edges_[x][y].red;
  val = std::max(val + red, 0);
}

void Graph2D::addRedV(const int x, const int y, const int red)
{
  auto& val = v_edges_[x][y].red;
  val = std::max(val + red, 0);
}

void Graph2D::addUsageH(const Interval& xi, const int y, const int used)
{
  for (int x = xi.lo; x < xi.hi; x++) {
    h_edges_[x][y].usage += used;
    if (used > 0) {
      h_used_ggrid_.insert({x, y});
    }
  }
}

void Graph2D::addUsageV(const int x, const Interval& yi, const int used)
{
  for (int y = yi.lo; y < yi.hi; y++) {
    v_edges_[x][y].usage += used;
    if (used > 0) {
      v_used_ggrid_.insert({x, y});
    }
  }
}

void Graph2D::addUsageH(const int x, const int y, const int used)
{
  h_edges_[x][y].usage += used;
  if (used > 0) {
    h_used_ggrid_.insert({x, y});
  }
}

void Graph2D::addUsageV(const int x, const int y, const int used)
{
  v_edges_[x][y].usage += used;
  if (used > 0) {
    v_used_ggrid_.insert({x, y});
  }
}

/*
 * num_iteration : the total number of iterations for maze route to run
 * round : the number of maze route stages runned
 */

void Graph2D::updateCongestionHistory(const int up_type,
                                      const int ahth,
                                      bool stop_decreasing,
                                      int& max_adj)
{
  int maxlimit = 0;

  if (up_type == 2) {
    stop_decreasing = max_adj < ahth;
  }

  auto updateEdges = [&](const auto& grid, auto& edges) {
    for (const auto& [x, y] : grid) {
      const int overflow = edges[x][y].usage - edges[x][y].cap;
      if (overflow > 0) {
        edges[x][y].congCNT++;
        edges[x][y].last_usage += overflow;
      } else if (!stop_decreasing) {
        if (up_type != 1) {
          edges[x][y].congCNT = std::max<int>(0, edges[x][y].congCNT - 1);
        }
        if (up_type != 3) {
          edges[x][y].last_usage *= 0.9;
        } else {
          edges[x][y].last_usage
              = std::max<int>(edges[x][y].last_usage + overflow, 0);
        }
      }
      maxlimit = std::max<int>(maxlimit, edges[x][y].last_usage);
    }
  };

  updateEdges(h_used_ggrid_, h_edges_);
  updateEdges(v_used_ggrid_, v_edges_);

  max_adj = maxlimit;
}

void Graph2D::str_accu(const int rnd)
{
  foreachEdge([rnd](Edge& edge) {
    const int overflow = edge.usage - edge.cap;
    if (overflow > 0 || edge.congCNT > rnd) {
      edge.last_usage += edge.congCNT * overflow / 2;
    }
  });
}

void Graph2D::foreachEdge(const std::function<void(Edge&)>& func)
{
  auto inner = [&](auto& edges) {
    Edge* edges_data = edges.data();

    const size_t num_edges = edges.num_elements();

    for (size_t i = 0; i < num_edges; ++i) {
      func(edges_data[i]);
    }
  };
  inner(h_edges_);
  inner(v_edges_);
}

void Graph2D::saveResources(const int x, const int y, bool is_horizontal)
{
  if (is_horizontal) {
    h_edges_[x][y].real_cap = h_edges_[x][y].cap;
  } else {
    v_edges_[x][y].real_cap = v_edges_[x][y].cap;
  }
}

int Graph2D::getSuggestAdjustment(const int x, const int y, bool is_horizontal)
{
  float real_capacity, usage;
  if (is_horizontal) {
    real_capacity = h_edges_[x][y].real_cap;
    usage = h_edges_[x][y].usage;
  } else {
    real_capacity = v_edges_[x][y].real_cap;
    usage = v_edges_[x][y].usage;
  }
  if (real_capacity >= usage) {
    const int suggest_adj = (1.0 - (usage / real_capacity)) * 100;
    return suggest_adj;
  }
  return -1;
}

}  // namespace grt
