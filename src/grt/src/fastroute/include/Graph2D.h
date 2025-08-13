// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <boost/multi_array.hpp>
#include <cstdint>
#include <functional>
#include <set>
#include <utility>
#include <vector>

#include "DataType.h"

namespace grt {

using boost::multi_array;

class Graph2D
{
 public:
  struct Interval
  {
    const int lo;
    const int hi;
  };

  void init(int x_grid, int y_grid, int h_capacity, int v_capacity);
  void InitEstUsage();
  void InitLastUsage(int upType);
  void clear();
  void clearUsed();
  bool hasEdges() const;

  uint16_t getUsageH(int x, int y) const;
  uint16_t getUsageV(int x, int y) const;
  int16_t getLastUsageH(int x, int y) const;
  int16_t getLastUsageV(int x, int y) const;
  double getEstUsageH(int x, int y) const;
  double getEstUsageV(int x, int y) const;
  uint16_t getUsageRedH(int x, int y) const;
  uint16_t getUsageRedV(int x, int y) const;
  double getEstUsageRedH(int x, int y) const;
  double getEstUsageRedV(int x, int y) const;
  int getOverflowH(int x, int y) const;
  int getOverflowV(int x, int y) const;
  uint16_t getCapH(int x, int y) const;
  uint16_t getCapV(int x, int y) const;

  const std::set<std::pair<int, int>>& getUsedGridsH() const;
  const std::set<std::pair<int, int>>& getUsedGridsV() const;

  void addCapH(int x, int y, int cap);
  void addCapV(int x, int y, int cap);
  void addEstUsageH(const Interval& xi, int y, double usage);
  void addEstUsageH(int x, int y, double usage);
  void addEstUsageToUsage();
  void addEstUsageV(int x, const Interval& yi, double usage);
  void addEstUsageV(int x, int y, double usage);
  void addRedH(int x, int y, int red);
  void addRedV(int x, int y, int red);
  void addUsageH(const Interval& xi, int y, int used);
  void addUsageH(int x, int y, int used);
  void addUsageV(int x, const Interval& yi, int used);
  void addUsageV(int x, int y, int used);

  void updateCongestionHistory(int up_type,
                               int ahth,
                               bool stop_decreasing,
                               int& max_adj);
  void str_accu(int rnd);

 private:
  void foreachEdge(const std::function<void(Edge&)>& func);

  multi_array<Edge, 2> v_edges_;  // The way it is indexed is (X, Y)
  multi_array<Edge, 2> h_edges_;  // The way it is indexed is (X, Y)

  std::set<std::pair<int, int>> h_used_ggrid_;
  std::set<std::pair<int, int>> v_used_ggrid_;
};

}  // namespace grt
