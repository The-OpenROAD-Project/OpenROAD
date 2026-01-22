// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "DataType.h"
#include "boost/multi_array.hpp"
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
    uint16_t cap;    // max layer capacity
    double cap_ndr;  // capacity available for NDR
  };

  struct NDRCongestion
  {
    int net_id;          // NDR net id
    uint16_t num_edges;  // number of congested edges

    NDRCongestion(int net_id, uint16_t num_edges)
        : net_id(net_id), num_edges(num_edges)
    {
    }
  };

  struct NDRCongestionComparator
  {
    bool operator()(const NDRCongestion& a, const NDRCongestion& b) const
    {
      return a.num_edges > b.num_edges;
    }
  };

  void init(int x_grid, int y_grid, int num_layers, utl::Logger* logger);
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
  void addEstUsageToUsage();
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
  void saveResources(int x, int y, bool is_horizontal);
  bool computeSuggestedAdjustment(int x,
                                  int y,
                                  bool is_horizontal,
                                  int& adjustment);

  void updateEstUsageH(const Interval& xi, int y, FrNet* net, double usage);
  void updateEstUsageH(int x, int y, FrNet* net, double usage);
  void updateEstUsageV(int x, const Interval& yi, FrNet* net, double usage);
  void updateEstUsageV(int x, int y, FrNet* net, double usage);
  void updateUsageH(const Interval& xi, int y, FrNet* net, int usage);
  void updateUsageH(int x, int y, FrNet* net, int usage);
  void updateUsageV(int x, const Interval& yi, FrNet* net, int usage);
  void updateUsageV(int x, int y, FrNet* net, int usage);
  void initCap3D();
  void updateCap3D(int x,
                   int y,
                   int layer,
                   EdgeDirection direction,
                   double cap);

  void clearNDRnets();
  std::vector<NDRCongestion> getCongestedNDRnets() { return congested_ndrs_; };
  void clearCongestedNDRnets() { congested_ndrs_.clear(); };
  void addCongestedNDRnet(int net_id, uint16_t num_edges);
  void sortCongestedNDRnets();
  int getOneCongestedNDRnet();
  std::vector<int> getMultipleCongestedNDRnet();

 private:
  int x_grid_ = 0;
  int y_grid_ = 0;
  int num_layers_ = 0;
  std::set<std::string> congestion_nets_;

  void updateCongList(const std::string& net_name, double edge_cost);
  void printAllElements();
  double getCostNDRAware(FrNet* net,
                         int x,
                         int y,
                         double edge_cost,
                         EdgeDirection direction);
  void updateNDRCapLayer(int x,
                         int y,
                         FrNet* net,
                         EdgeDirection dir,
                         double edge_cost);
  bool hasNDRCapacity(FrNet* net, int x, int y, EdgeDirection direction);
  void printNDRCap(int x, int y);
  void printEdgeCapPerLayer();
  void initNDRnets();

  void foreachEdge(const std::function<void(Edge&)>& func);

  multi_array<Edge, 2> v_edges_;    // The way it is indexed is (X, Y)
  multi_array<Edge, 2> h_edges_;    // The way it is indexed is (X, Y)
  multi_array<Cap3D, 3> v_cap_3D_;  // The way it is indexed is (Layer, X, Y)
  multi_array<Cap3D, 3> h_cap_3D_;  // The way it is indexed is (Layer, X, Y)
  multi_array<std::set<FrNet*>, 2>
      v_ndr_nets_;  // The way it is indexed is (X, Y)
  multi_array<std::set<FrNet*>, 2>
      h_ndr_nets_;  // The way it is indexed is (X, Y)
  std::vector<NDRCongestion> congested_ndrs_;

  utl::Logger* logger_ = nullptr;

  std::set<std::pair<int, int>> h_used_ggrid_;
  std::set<std::pair<int, int>> v_used_ggrid_;
};

}  // namespace grt
