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
#include "utl/Logger.h"

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

  struct Cap3D
  {
    uint16_t cap; // max layer capacity
    double cap_ndr; // capacity available for NDR
  };

  void init(int x_grid, int y_grid, int h_capacity, int v_capacity, int num_layers, utl::Logger* logger);
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

  
  void updateEstUsageH(const Interval& xi, const int y, FrNet* net, const double edge_cost);
  void updateEstUsageH(const int x, const int y, FrNet* net, const double edge_cost);
  void updateEstUsageV(const int x, const Interval& yi, FrNet* net, const double edge_cost);
  void updateEstUsageV(const int x, const int y, FrNet* net, const double edge_cost);
  void updateUsageH(const Interval& xi, int y, FrNet* net, const int edge_cost);
  void updateUsageH(int x, int y, FrNet* net, const int edge_cost);
  void updateUsageV(int x, const Interval& yi, FrNet* net, const int edge_cost);
  void updateUsageV(int x, int y, FrNet* net, const int edge_cost);
  bool hasNDRCapacity(FrNet* net, int x, int y, EdgeDirection direction);
  bool hasNDRCapacity(FrNet* net, int x, int y, EdgeDirection direction, double edge_cost);
  void printNDRCap(const int x, const int y);
  void printEdgeCapPerLayer();
  void initCap3D();
  void updateCap3D(int x, int y, int layer, EdgeDirection direction, const double cap);

  
 private:
  int x_grid_;
  int y_grid_;
  int num_layers_;

  double getCostNDRAware(FrNet* net, int x, int y, const double edgeCost, EdgeDirection direction);
  int getCostNDRAware(FrNet* net, int x, int y, const int edgeCost, EdgeDirection direction);
  void updateNDRCapLayer(const int x, const int y, FrNet* net, EdgeDirection dir, const double edge_cost);
  void fixFractionEdgeUsage(int minLayer, int maxLayer, int x, int y, double edge_cost, EdgeDirection dir);

  void foreachEdge(const std::function<void(Edge&)>& func);

  multi_array<Edge, 2> v_edges_;  // The way it is indexed is (X, Y)
  multi_array<Edge, 2> h_edges_;  // The way it is indexed is (X, Y)
  multi_array<uint16_t, 3> v_cap_3D_;  // The way it is indexed is (Layer, X, Y)
  multi_array<uint16_t, 3> h_cap_3D_;  // The way it is indexed is (Layer, X, Y)
  multi_array<Cap3D, 3> _v_cap_3D_;  // The way it is indexed is (Layer, X, Y)
  multi_array<Cap3D, 3> _h_cap_3D_;  // The way it is indexed is (Layer, X, Y)

  utl::Logger* logger_;

  std::set<std::pair<int, int>> h_used_ggrid_;
  std::set<std::pair<int, int>> v_used_ggrid_;
};

}  // namespace grt
