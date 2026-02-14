// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "Clustering.h"

#include <sys/timeb.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "lemon/list_graph.h"
#include "lemon/maps.h"
#include "lemon/network_simplex.h"
#include "utl/Logger.h"

namespace cts::CKMeans {

using lemon::INVALID;
using lemon::ListDigraph;
using lemon::NetworkSimplex;
using utl::CTS;

struct Sink
{
  Sink(const float x, const float y, const unsigned idx)
      : x(x), y(y), sink_idx(idx)
  {
  }

  // location
  const float x, y;
  int cluster_idx{-1};
  const unsigned sink_idx;  // index in sinks_
};

Clustering::Clustering(const std::vector<std::pair<float, float>>& sinks,
                       utl::Logger* logger)
{
  logger_ = logger;
  sinks_.reserve(sinks.size());
  for (size_t i = 0; i < sinks.size(); ++i) {
    sinks_.emplace_back(sinks[i].first, sinks[i].second, i);
  }
}

Clustering::Clustering(const std::vector<std::pair<float, float>>& sinks,
                       const float xBranch,
                       const float yBranch,
                       utl::Logger* logger)
    : Clustering(sinks, logger)
{
  branching_point_ = {xBranch, yBranch};
  srand(56);
}

Clustering::~Clustering() = default;

/*** Capacitated K means **************************************************/
void Clustering::iterKmeans(const unsigned iter,
                            const unsigned n,
                            const unsigned cap,
                            const unsigned max,
                            const unsigned power,
                            std::vector<std::pair<float, float>>& means)
{
  const unsigned midIdx = means.size() / 2 - 1;
  segment_length_ = std::abs(means[midIdx].first - means[midIdx + 1].first)
                    + std::abs(means[midIdx].second - means[midIdx + 1].second);

  std::vector<int> solution(sinks_.size());
  float max_silh = -1;
  auto tmp_means = means;
  for (unsigned i = 0; i < iter; ++i) {
    const float silh = Kmeans(n, cap, max, power, tmp_means);
    if (silh > max_silh) {
      max_silh = silh;
      for (size_t j = 0; j < sinks_.size(); ++j) {
        solution[j] = sinks_[j].cluster_idx;
      }
      means = tmp_means;
    }
  }

  for (size_t i = 0; i < sinks_.size(); ++i) {
    sinks_[i].cluster_idx = solution[i];
  }

  fixSegmentLengths(means);
}

void Clustering::fixSegmentLengths(std::vector<std::pair<float, float>>& means)
{
  if (!branching_point_) {
    return;
  }
  // First, fix the middle positions
  const unsigned midIdx = means.size() / 2 - 1;

  fixSegment(branching_point_.value(), segment_length_ / 2.0, means[midIdx]);
  fixSegment(
      branching_point_.value(), segment_length_ / 2.0, means[midIdx + 1]);

  // Fix lower branch
  for (unsigned i = midIdx; i > 0; --i) {
    fixSegment(means[i], segment_length_, means[i - 1]);
  }

  // Fix upper branch
  for (size_t i = midIdx + 1; i < means.size() - 1; ++i) {
    fixSegment(means[i], segment_length_, means[i + 1]);
  }
}

void Clustering::fixSegment(const std::pair<float, float>& fixedPoint,
                            const float targetDist,
                            std::pair<float, float>& movablePoint)
{
  const float actualDist = calcDist(fixedPoint, movablePoint);
  if (actualDist == 0) {
    return;
  }
  const float ratio = targetDist / actualDist;

  const float dx = (movablePoint.first - fixedPoint.first) * ratio;
  const float dy = (movablePoint.second - fixedPoint.second) * ratio;

  movablePoint.first = fixedPoint.first + dx;
  movablePoint.second = fixedPoint.second + dy;
}

float Clustering::Kmeans(const unsigned n,
                         const unsigned cap,
                         const unsigned max,
                         const unsigned power,
                         std::vector<std::pair<float, float>>& means)
{
  // initialize matching indexes for sinks
  for (auto& sink : sinks_) {
    sink.cluster_idx = -1;
  }

  std::vector<std::vector<Sink*>> clusters;
  bool stop = false;
  // Kmeans optimization
  unsigned iter = 1;
  while (!stop) {
    fixSegmentLengths(means);

    // sink to cluster matching based on min-cost flow
    minCostFlow(means, cap, 5200, power);

    // collect results
    clusters.clear();
    clusters.resize(n);
    for (Sink& sink : sinks_) {
      int position = 0;
      if (sink.cluster_idx >= 0 && sink.cluster_idx < n) {
        position = sink.cluster_idx;
      } else {
        // Added to check wrong assignment

        float minimumDist;
        int minimumDistClusterIndex = -1;

        // Initialize minimumDist and minimumDistClusterIndex with a cluster
        // with size < cap
        for (size_t j = 0; j < means.size(); ++j) {
          if (clusters[j].size() < cap) {
            minimumDist = calcDist(
                std::make_pair(means[j].first, means[j].second), &sink);
            minimumDistClusterIndex = j;
            break;
          }
        }

        if (minimumDistClusterIndex == -1) {
          // No cluster with size < cap
          minimumDistClusterIndex = 0;
        } else {
          // Nearest Cluster with size < cap
          for (size_t j = 0; j < means.size(); ++j) {
            if (clusters[j].size() < cap) {
              const float currentDist = calcDist(
                  std::make_pair(means[j].first, means[j].second), &sink);
              if (currentDist < minimumDist) {
                minimumDist = currentDist;
                minimumDistClusterIndex = j;
              }
            }
          }
        }

        position = minimumDistClusterIndex;
      }
      clusters[position].push_back(&sink);
    }

    float delta = 0;

    // use weighted center
    for (unsigned i = 0; i < n; ++i) {
      float sum_x = 0, sum_y = 0;
      for (const auto& cluster : clusters[i]) {
        sum_x += cluster->x;
        sum_y += cluster->y;
      }
      const float pre_x = means[i].first;
      const float pre_y = means[i].second;

      if (!clusters[i].empty()) {
        means[i] = std::make_pair(sum_x / clusters[i].size(),
                                  sum_y / clusters[i].size());
        delta += std::abs(pre_x - means[i].first)
                 + std::abs(pre_y - means[i].second);
      }
    }

    clusters_ = clusters;

    if (iter > max || delta < 0.5) {
      stop = true;
    }

    ++iter;
  }

  return calcSilh(means);
}

float Clustering::calcSilh(
    const std::vector<std::pair<float, float>>& means) const
{
  float sum_silh = 0;
  for (const Sink& sink : sinks_) {
    float in_d = 0, out_d = FLT_MAX;
    for (size_t j = 0; j < means.size(); ++j) {
      const float x = means[j].first;
      const float y = means[j].second;
      if (sink.cluster_idx == j) {
        // within the cluster
        in_d = calcDist({x, y}, &sink);
      } else {
        // outside of the cluster
        const float d = calcDist({x, y}, &sink);
        out_d = std::min(d, out_d);
      }
    }
    const float temp = std::max(out_d, in_d);
    if (temp == 0) {
      if (out_d == 0) {
        sum_silh += -1;
      }
      if (in_d == 0) {
        sum_silh += 1;
      }
    } else {
      sum_silh += (out_d - in_d) / temp;
    }
  }
  return sum_silh / sinks_.size();
}

/*** Min-Cost Flow ********************************************************/
void Clustering::minCostFlow(const std::vector<std::pair<float, float>>& means,
                             const unsigned cap,
                             const float dist,
                             const unsigned power)
{
  // Builds src -> [sink nodes] -> [cluster nodes] - > target
  ListDigraph graph;

  // source and target
  ListDigraph::Node src = graph.addNode();
  ListDigraph::Node target = graph.addNode();

  // collection of nodes in the flow
  std::vector<ListDigraph::Node> sink_nodes, cluster_nodes;

  // add nodes / edges to graph
  // nodes for sinks
  for (size_t i = 0; i < sinks_.size(); ++i) {
    sink_nodes.push_back(graph.addNode());
  }
  // nodes for clusters
  for (size_t i = 0; i < means.size(); ++i) {
    cluster_nodes.push_back(graph.addNode());
  }

  // collection of edges in the flow
  std::vector<ListDigraph::Arc> src_sink_edges, sink_cluster_edges,
      cluster_sink_edges;

  // edges between source and sinks
  for (auto& sink : sink_nodes) {
    src_sink_edges.push_back(graph.addArc(src, sink));
  }
  // edges between sinks and clusters
  std::vector<double> costs;
  for (size_t i = 0; i < sinks_.size(); ++i) {
    for (size_t j = 0; j < means.size(); ++j) {
      double d = calcDist(means[j], &sinks_[i]);
      if (d <= dist) {
        d = std::pow(d, power);
        if (d < std::numeric_limits<int>::max()) {
          ListDigraph::Arc e = graph.addArc(sink_nodes[i], cluster_nodes[j]);
          sink_cluster_edges.push_back(e);
          costs.push_back(d);
        }
      }
    }
  }
  // edges between clusters and target
  for (auto& cluster : cluster_nodes) {
    cluster_sink_edges.push_back(graph.addArc(cluster, target));
  }

  debugPrint(logger_,
             CTS,
             "clustering",
             1,
             "Graph has {} nodes and {} edges",
             countNodes(graph),
             countArcs(graph));

  // formulate min-cost flow
  ListDigraph::ArcMap<int> edge_cost(graph), edge_capacity(graph);
  for (auto& edge : src_sink_edges) {
    edge_capacity[edge] = 1;
  }
  for (size_t i = 0; i < sink_cluster_edges.size(); ++i) {
    edge_capacity[sink_cluster_edges[i]] = 1;
    edge_cost[sink_cluster_edges[i]] = costs[i];
  }

  const int remaining = sinks_.size() % means.size();

  for (size_t i = 0; i < cluster_sink_edges.size(); ++i) {
    if (i < remaining) {
      edge_capacity[cluster_sink_edges[i]] = cap + 1;
    } else {
      edge_capacity[cluster_sink_edges[i]] = cap;
    }
  }

  for (ListDigraph::ArcIt it(graph); it != INVALID; ++it) {
    debugPrint(logger_,
               CTS,
               "clustering",
               2,
               "{}-{}",
               graph.id(graph.source(it)),
               graph.id(graph.target(it)));
    debugPrint(logger_, CTS, "clustering", 2, " cost = ", edge_cost[it]);
    debugPrint(logger_, CTS, "clustering", 2, " cap = ", edge_capacity[it]);
  }

  NetworkSimplex<ListDigraph, int, int> flow(graph);
  flow.costMap(edge_cost);
  flow.upperMap(edge_capacity);
  flow.stSupply(src, target, means.size() * cap + remaining);
  flow.run();
  ListDigraph::ArcMap<int> solution(graph);
  flow.flowMap(solution);

  ListDigraph::NodeMap<std::pair<int, int>> node_map(graph);
  for (size_t i = 0; i < sink_nodes.size(); ++i) {
    node_map[sink_nodes[i]] = {i, -1};
  }

  for (size_t i = 0; i < cluster_nodes.size(); ++i) {
    node_map[cluster_nodes[i]] = {-1, i};
  }

  node_map[src] = {-2, -2};
  node_map[target] = {-2, -2};

  for (ListDigraph::ArcIt it(graph); it != INVALID; ++it) {
    if (solution[it] == 0) {
      continue;
    }
    if (node_map[graph.source(it)].second == -1
        && node_map[graph.target(it)].first == -1) {
      debugPrint(logger_,
                 CTS,
                 "clustering",
                 3,
                 "Flow from: sink_{}  to cluster_{} flow = {}",
                 node_map[graph.source(it)].first,
                 node_map[graph.target(it)].second,
                 solution[it]);

      sinks_[node_map[graph.source(it)].first].cluster_idx
          = node_map[graph.target(it)].second;
    }
  }
}

void Clustering::getClusters(
    std::vector<std::vector<unsigned>>& newClusters) const
{
  newClusters.clear();
  newClusters.resize(clusters_.size());
  for (size_t i = 0; i < clusters_.size(); ++i) {
    newClusters[i].resize(clusters_[i].size());
    for (unsigned j = 0; j < clusters_[i].size(); ++j) {
      newClusters[i][j] = clusters_[i][j]->sink_idx;
    }
  }
}

/* static */
float Clustering::calcDist(const std::pair<float, float>& loc, const Sink* sink)
{
  return calcDist(loc, {sink->x, sink->y});
}

/* static */
float Clustering::calcDist(const std::pair<float, float>& loc1,
                           const std::pair<float, float>& loc2)
{
  return std::abs(loc1.first - loc2.first)
         + std::abs(loc1.second - loc2.second);
}

}  // namespace cts::CKMeans
