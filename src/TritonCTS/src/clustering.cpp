/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "clustering.h"
#include <lemon/list_graph.h>
#include <lemon/maps.h>
#include <lemon/network_simplex.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/timeb.h>
#include <time.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <stack>
#include <string>
#include <vector>

#include "utility/Logger.h"

#define CPLEX_CLUSTERING

namespace CKMeans {
using namespace lemon;
using utl::CTS;

clustering::clustering(const std::vector<std::pair<float, float>>& sinks,
                       float xBranch,
                       float yBranch,
                       Logger* logger)
{
  for (long int i = 0; i < sinks.size(); ++i) {
    flops.push_back(flop(sinks[i].first, sinks[i].second, i));
    flops.back().dists.resize(1);
    flops.back().match_idx.resize(1);
    flops.back().silhs.resize(1);
  }
  srand(56);

  branchingPoint = {xBranch, yBranch};
  _logger = logger;
}

clustering::~clustering()
{
}

/*** Capacitated K means **************************************************/
void clustering::iterKmeans(unsigned ITER,
                            unsigned N,
                            unsigned CAP,
                            unsigned IDX,
                            std::vector<std::pair<float, float>>& _means,
                            unsigned MAX,
                            unsigned power)
{
  std::vector<std::pair<float, float>> sol;
  sol.resize(flops.size());

  unsigned midIdx = _means.size() / 2 - 1;
  segmentLength = std::abs(_means[midIdx].first - _means[midIdx + 1].first)
                  + std::abs(_means[midIdx].second - _means[midIdx + 1].second);

  float max_silh = -1;
  for (long int i = 0; i < ITER; ++i) {
    std::vector<std::pair<float, float>> means = _means;
    float silh = Kmeans(N, CAP, IDX, means, MAX, power);
    if (silh > max_silh) {
      max_silh = silh;
      for (long int j = 0; j < flops.size(); ++j)
        sol[j] = flops[j].match_idx[IDX];
      _means.resize(means.size());
      for (long int j = 0; j < means.size(); ++j)
        _means[j] = means[j];
    }
  }

  for (long int i = 0; i < flops.size(); ++i)
    flops[i].match_idx[IDX] = sol[i];

  // print clustering solution
  if (TEST_LAYOUT == 1) {
    std::ofstream outFile;
    for (long int i = 0; i < _means.size(); ++i) {
      outFile << "TRAY " << i << " " << _means[i].first << " "
              << _means[i].second << std::endl;
    }
    for (long int i = 0; i < flops.size(); ++i) {
      flop* f = &flops[i];
    }
    outFile.close();
  }

  fixSegmentLengths(_means);
}

void clustering::fixSegmentLengths(std::vector<std::pair<float, float>>& _means)
{
  // First, fix the middle positions
  unsigned midIdx = _means.size() / 2 - 1;

  const bool overwriteBranchingPoint = false;

  if (overwriteBranchingPoint) {
    float minX = std::min(_means[midIdx].first, _means[midIdx + 1].first);
    float minY = std::min(_means[midIdx].second, _means[midIdx + 1].second);
    float maxX = std::max(_means[midIdx].first, _means[midIdx + 1].first);
    float maxY = std::max(_means[midIdx].second, _means[midIdx + 1].second);
    branchingPoint
        = std::make_pair(minX + (maxX - minX) / 2.0, minY + (maxY - minY) / 2.0);
  }

  fixSegment(branchingPoint, _means[midIdx], segmentLength / 2.0);
  fixSegment(branchingPoint, _means[midIdx + 1], segmentLength / 2.0);

  // Fix lower branch
  for (long int i = midIdx; i > 0; --i) {
    fixSegment(_means[i], _means[i - 1], segmentLength);
  }

  // Fix upper branch
  for (long int i = midIdx + 1; i < _means.size() - 1; ++i) {
    fixSegment(_means[i], _means[i + 1], segmentLength);
  }
}

void clustering::fixSegment(const std::pair<float, float>& fixedPoint,
                            std::pair<float, float>& movablePoint,
                            float targetDist)
{
  float actualDist = calcDist(fixedPoint, movablePoint);
  float ratio = targetDist / actualDist;

  float dx = (fixedPoint.first - movablePoint.first) * ratio;
  float dy = (fixedPoint.second - movablePoint.second) * ratio;

  movablePoint.first = fixedPoint.first - dx;
  movablePoint.second = fixedPoint.second - dy;
}

float clustering::Kmeans(unsigned N,
                         unsigned CAP,
                         unsigned IDX,
                         std::vector<std::pair<float, float>>& means,
                         unsigned MAX,
                         unsigned power)
{
  std::vector<std::vector<flop*>> clusters;

  for (long int i = 0; i < flops.size(); ++i) {
    flops[i].dists[IDX] = 0;
    for (long int j = 0; j < N; ++j) {
      flops[i].dists[IDX]
          += calcDist(std::make_pair(means[j].first, means[j].second), &flops[i]);
    }
  }

  // initialize matching indexes for flops
  for (long int i = 0; i < flops.size(); ++i) {
    flop* f = &flops[i];
    f->match_idx[IDX] = std::make_pair(-1, -1);
  }

  bool stop = false;
  // Kmeans optimization
  unsigned iter = 1;
  while (!stop) {
    fixSegmentLengths(means);

    // flop to slot matching based on min-cost flow
    if (iter == 1)
      minCostFlow(means, CAP, IDX, 5200, power);
    else if (iter == 2)
      minCostFlow(means, CAP, IDX, 5200, power);
    else if (iter > 2)
      minCostFlow(means, CAP, IDX, 5200, power);

    // collect results
    clusters.clear();
    clusters.resize(N);
    for (long int i = 0; i < flops.size(); ++i) {
      flop* f = &flops[i];
      int position = 0;
      if (f->match_idx[IDX].first >= 0 && f->match_idx[IDX].first < N) {
        position = f->match_idx[IDX].first;
      }
      else{

        // Added to check wrong assignment

        float minimumDist;
        int minimumDistClusterIndex = -1;

        // Initialize minimumDist and minimumDistClusterIndex with a cluster with size < CAP
        for (long int j = 0; j < means.size(); ++j) {
          if(clusters[j].size() < CAP){
            minimumDist = calcDist(std::make_pair(means[j].first, means[j].second), f);
            minimumDistClusterIndex = j;
            break;
          }
        }

        if (minimumDistClusterIndex == -1) {
          // No cluster with size < CAP
          minimumDistClusterIndex = 0;
        }
        else {
          // Nearest Cluster with size < CAP
          for (long int j = 0; j < means.size(); ++j) {
            if(clusters[j].size() < CAP) {
              float currentDist = calcDist(std::make_pair(means[j].first, means[j].second), f);
              if(currentDist < minimumDist){
                minimumDist = currentDist;
                minimumDistClusterIndex = j;
              }
            }
          }
        }

        position = minimumDistClusterIndex;
      
      }
      clusters[position].push_back(f);
    }

    float delta = 0;

    // use weighted center
    for (long int i = 0; i < N; ++i) {
      float sum_x = 0, sum_y = 0;
      for (long int j = 0; j < clusters[i].size(); ++j) {
        sum_x += clusters[i][j]->x;
        sum_y += clusters[i][j]->y;
      }
      float pre_x = means[i].first;
      float pre_y = means[i].second;

      if (clusters[i].size() > 0) {
        means[i]
            = std::make_pair(sum_x / clusters[i].size(), sum_y / clusters[i].size());
        delta += abs(pre_x - means[i].first) + abs(pre_y - means[i].second);
      }
    }

    this->clusters = clusters;

    if (TEST_LAYOUT == 1 || TEST_ITER == 1) {
      float silh = calcSilh(means, CAP, IDX);
    }

    if (iter > MAX || delta < 0.5)
      stop = true;

    ++iter;
  }

  float silh = calcSilh(means, CAP, IDX);

  return silh;
}

float clustering::calcSilh(const std::vector<std::pair<float, float>>& means,
                           unsigned CAP,
                           unsigned IDX)
{
  float sum_silh = 0;
  for (long int i = 0; i < flops.size(); ++i) {
    flop* f = &flops[i];
    float in_d = 0, out_d = FLT_MAX;
    for (long int j = 0; j < means.size(); ++j) {
      float _x = means[j].first;
      float _y = means[j].second;
      if (f->match_idx[IDX].first == j) {
        // within the cluster
        unsigned k = f->match_idx[IDX].second;
        in_d = calcDist(std::make_pair(_x, _y), f);
      } else {
        // outside of the cluster
        float d = calcDist(std::make_pair(_x, _y), f);
        if (d < out_d) {
          out_d = d;
        }
      }
    }
    float temp = std::max(out_d, in_d);
    if (temp == 0) {
      if (out_d == 0)
        f->silhs[IDX] = -1;
      if (in_d == 0)
        f->silhs[IDX] = 1;
    } else {
      f->silhs[IDX] = (out_d - in_d) / temp;
    }
    sum_silh += f->silhs[IDX];
  }
  return sum_silh / flops.size();
}

/*** Min-Cost Flow ********************************************************/
void clustering::minCostFlow(const std::vector<std::pair<float, float>>& means,
                             unsigned CAP,
                             unsigned IDX,
                             float DIST,
                             unsigned power)
{
  int remaining = flops.size() % means.size();

  ListDigraph g;
  // collection of nodes in the flow
  std::vector<ListDigraph::Node> f_nodes, c_nodes;
  // collection of edges in the flow
  std::vector<ListDigraph::Arc> i_edges, d_edges, /*g_edges,*/ o_edges;

  // source and sink
  ListDigraph::Node s = g.addNode();
  ListDigraph::Node t = g.addNode();

  // add nodes / edges to graph
  // nodes for flops
  for (long int i = 0; i < flops.size(); ++i) {
    ListDigraph::Node v = g.addNode();
    f_nodes.push_back(v);
  }
  // nodes for clusters
  for (long int i = 0; i < means.size(); ++i) {
    ListDigraph::Node v = g.addNode();
    c_nodes.push_back(v);
  }
  // edges between source and flops
  for (long int i = 0; i < flops.size(); ++i) {
    ListDigraph::Arc e = g.addArc(s, f_nodes[i]);
    i_edges.push_back(e);
  }
  // edges between flops and slots
  std::vector<float> costs;
  for (long int i = 0; i < flops.size(); ++i) {
    for (long int j = 0; j < means.size(); ++j) {
      float _x = means[j].first;
      float _y = means[j].second;

      float d = calcDist(std::make_pair(_x, _y), &flops[i]);
      if (d <= DIST && std::pow(d, power) < std::numeric_limits<int>::max()) {
        d = std::pow(d, power);
        ListDigraph::Arc e = g.addArc(f_nodes[i], c_nodes[j]);
        d_edges.push_back(e);
        costs.push_back(d);
      }
    }
  }
  // edges between clusters and sink
  for (long int i = 0; i < means.size(); ++i) {
    ListDigraph::Arc e = g.addArc(c_nodes[i], t);
    o_edges.push_back(e);
  }

  debugPrint(_logger, CTS, "tritoncts", 1, 
          "Graph has {} nodes and {} edges", countNodes(g), countArcs(g)); 

  // formulate min-cost flow
  NetworkSimplex<ListDigraph, int, int> flow(g);
  ListDigraph::ArcMap<int> f_cost(g), f_cap(g), f_sol(g);
  ListDigraph::NodeMap<std::pair<int, std::pair<int, int>>> node_map(g);
  for (long int i = 0; i < i_edges.size(); ++i) {
    f_cap[i_edges[i]] = 1;
  }
  for (long int i = 0; i < d_edges.size(); ++i) {
    f_cap[d_edges[i]] = 1;
    f_cost[d_edges[i]] = costs[i];
  }

  for (long int i = 0; i < o_edges.size(); ++i) {
    if (i < remaining) {
      f_cap[o_edges[i]] = CAP + 1;
    } else {
      f_cap[o_edges[i]] = CAP;
    }
  }

  for (long int i = 0; i < f_nodes.size(); ++i)
    node_map[f_nodes[i]] = std::make_pair(i, std::make_pair(-1, -1));

  for (long int i = 0; i < c_nodes.size(); ++i)
    node_map[c_nodes[i]] = std::make_pair(-1, std::make_pair(i, 0));

  node_map[s] = std::make_pair(-2, std::make_pair(-2, -2));
  node_map[t] = std::make_pair(-2, std::make_pair(-2, -2));

  for (ListDigraph::ArcIt it(g); it != INVALID; ++it) {
    debugPrint(_logger, CTS, "tritoncts", 2, 
        "{}-{}", g.id(g.source(it)), g.id(g.target(it))); 
    debugPrint(_logger, CTS, "tritoncts", 2, 
        " cost = ", f_cost[it]); 
    debugPrint(_logger, CTS, "tritoncts", 2, 
        " cap = ", f_cap[it]); 
  }

  flow.costMap(f_cost);
  flow.upperMap(f_cap);
  flow.stSupply(s, t, means.size() * CAP + remaining);
  flow.run();
  flow.flowMap(f_sol);

  for (ListDigraph::ArcIt it(g); it != INVALID; ++it) {
    if (f_sol[it] != 0) {
      if (node_map[g.source(it)].second.first == -1
          && node_map[g.target(it)].first == -1) {
        debugPrint(_logger, CTS, "tritoncts", 3,"Flow from: flop_",
                   node_map[g.source(it)].first);
        debugPrint(_logger, CTS, "tritoncts", 3," to cluster_",
                   node_map[g.target(it)].second.first);
        debugPrint(_logger, CTS, "tritoncts", 3," slot_",
                   node_map[g.target(it)].second.second);
        debugPrint(_logger, CTS, "tritoncts", 3," flow = ",
                   f_sol[it]);
        
        flops[node_map[g.source(it)].first].match_idx[IDX]
            = node_map[g.target(it)].second;
      }
    }
  }
}

void clustering::plotClusters(
    const std::vector<std::vector<flop*>>& clusters,
    const std::vector<std::pair<float, float>>& means,
    const std::vector<std::pair<float, float>>& pre_means,
    int iter) const
{
  std::ofstream fout;
  std::stringstream sol_ss;
  sol_ss << plotFile << "__" << iter << ".py";
  fout.open(sol_ss.str());
  fout << "#! /home/kshan/anaconda2/bin/python" << std::endl << std::endl;
  fout << "import numpy as np" << std::endl;
  fout << "import matplotlib.pyplot as plt" << std::endl;
  fout << "import matplotlib.path as mpath" << std::endl;
  fout << "import matplotlib.lines as mlines" << std::endl;
  fout << "import matplotlib.patches as mpatches" << std::endl;
  fout << "from matplotlib.collections import PatchCollection" << std::endl;

  fout << std::endl;
  fout << "fig, ax = plt.subplots()" << std::endl;
  fout << "patches = []" << std::endl;

  std::array<char, 6> colors = {'b', 'r', 'g', 'm', 'c', 'y'};
  for (size_t i = 0; i < clusters.size(); i++) {
    _logger->report("clusters ", i);
    for (size_t j = 0; j < clusters[i].size(); j++) {
      fout << "plt.scatter(" << clusters[i][j]->x << ", " << clusters[i][j]->y
           << ", color=\'" << colors[i % colors.size()] << "\')\n";

      if (clusters[i][j]->x < means[i].first) {
        fout << "plt.plot([" << clusters[i][j]->x << ", " << means[i].first
             << "], [" << clusters[i][j]->y << ", " << means[i].second
             << "], color = '" << colors[i % colors.size()] << "')\n";
      } else {
        fout << "plt.plot([" << means[i].first << ", " << clusters[i][j]->x
             << "], [" << means[i].second << ", " << clusters[i][j]->y
             << "], color = '" << colors[i % colors.size()] << "')\n";
      }
    }
  }

  for (size_t i = 0; i < means.size(); i++) {
    fout << "plt.scatter(" << means[i].first << ", " << means[i].second
         << ", color=\'k\')\n";
  }

  for (size_t i = 0; i < pre_means.size(); i++) {
    fout << "plt.scatter(" << pre_means[i].first << ", " << pre_means[i].second
         << ", color=\'g\')\n";
  }

  fout << "plt.scatter(" << branchingPoint.first << ", "
       << branchingPoint.second << ", color=\'y\')\n";
  fout << "plt.show()";
  fout.close();
};

void clustering::getClusters(std::vector<std::vector<unsigned>>& newClusters)
{
  newClusters.clear();
  newClusters.resize(this->clusters.size());
  for (long int i = 0; i < clusters.size(); ++i) {
    newClusters[i].resize(clusters[i].size());
    for (unsigned j = 0; j < clusters[i].size(); ++j) {
      newClusters[i][j] = clusters[i][j]->sinkIdx;
    }
  }
}

}  // namespace CKMeans
