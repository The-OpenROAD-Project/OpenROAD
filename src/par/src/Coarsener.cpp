///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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
// High-level description
// This file includes the implementation for first-choice based coarsening
// Coarsening is an operation class:  It takes a hypergraph hgraph and returns
// a sequence of coarser hypergraphs. (Top level function: LazyFirstChoice)
///////////////////////////////////////////////////////////////////////////////

#include "Coarsener.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <set>
#include <utility>
#include <vector>

#include "Evaluator.h"
#include "Hypergraph.h"
#include "Utilities.h"
#include "utl/Logger.h"
using utl::PAR;

namespace par {

Coarsener::Coarsener(const int num_parts,
                     const int thr_coarsen_hyperedge_size_skip,
                     const int thr_coarsen_vertices,
                     const int thr_coarsen_hyperedges,
                     const float coarsening_ratio,
                     const int max_coarsen_iters,
                     const float adj_diff_ratio,
                     const std::vector<float>& thr_cluster_weight,
                     const int random_seed,
                     const CoarsenOrder vertex_order_choice,
                     EvaluatorPtr evaluator,
                     utl::Logger* logger)
    : num_parts_(num_parts),
      thr_coarsen_hyperedge_size_skip_(thr_coarsen_hyperedge_size_skip),
      thr_coarsen_vertices_(thr_coarsen_vertices),
      thr_coarsen_hyperedges_(thr_coarsen_hyperedges),
      coarsening_ratio_(coarsening_ratio),
      max_coarsen_iters_(max_coarsen_iters),
      adj_diff_ratio_(adj_diff_ratio),
      thr_cluster_weight_(thr_cluster_weight),
      random_seed_(random_seed),
      vertex_order_choice_(vertex_order_choice)
{
  evaluator_ = std::move(evaluator);
  logger_ = logger;
}

// The main function pf Coarsener class
// The input is a hypergraph
// The output is a sequence of coarser hypergraphs
// Notice that the input hypergraph is not const,
// because the hgraphs returned can be edited
// The timing cost of hgraph will be initialized if it has been not.
CoarseGraphPtrs Coarsener::LazyFirstChoice(const HGraphPtr& hgraph) const
{
  const auto start_timestamp = std::chrono::high_resolution_clock::now();
  const bool timing_flag = hgraph->HasTiming();
  std::vector<HGraphPtr> hierarchy;  // a sequence of coarser hypergraphs
  // start the FC multilevel coarsening by pushing the original hgraph to
  // hierarchy update the timing cost of hgraph if it has been not.
  if (hgraph->HasTiming() && hgraph->GetTimingPathCostSize() == 0
      && !hgraph->HasHyperedgeTimingCost()) {
    evaluator_->InitializeTiming(hgraph);
  }
  hierarchy.push_back(hgraph);  // push original hgraph to hierarchy
  debugPrint(
      logger_, PAR, "coarsening", 1, "Running FC Multilevel Coarsening...");
  if (timing_flag == true) {
    debugPrint(logger_,
               PAR,
               "coarsening",
               1,
               "Level 0 :: num_vertices = {}, num_hyperedges = {}, "
               "num_timing_paths = {}",
               hierarchy.back()->GetNumVertices(),
               hierarchy.back()->GetNumHyperedges(),
               hierarchy.back()->GetNumTimingPaths());
  } else {
    debugPrint(logger_,
               PAR,
               "coarsening",
               1,
               "Level 0 :: num_vertices = {}, num_hyperedges = {}",
               hierarchy.back()->GetNumVertices(),
               hierarchy.back()->GetNumHyperedges());
  }

  for (int num_iter = 0; num_iter < max_coarsen_iters_; ++num_iter) {
    // do coarsening step
    auto hg = Aggregate(hierarchy.back());
    const int num_vertices_pre_iter = hierarchy.back()->GetNumVertices();
    const int num_vertices_cur_iter = hg->GetNumVertices();
    if (num_vertices_pre_iter == num_vertices_cur_iter) {
      break;  // the current hypergraph hg is the same as the hypergraph in
              // previous iteration
    }
    hierarchy.push_back(hg);
    if (timing_flag == true) {
      debugPrint(logger_,
                 PAR,
                 "coarsening",
                 1,
                 "Level {} :: num_vertices = {}, num_hyperedges = {}, "
                 "num_timing_paths = {}",
                 num_iter + 1,
                 hierarchy.back()->GetNumVertices(),
                 hierarchy.back()->GetNumHyperedges(),
                 hierarchy.back()->GetNumTimingPaths());
    } else {
      debugPrint(logger_,
                 PAR,
                 "coarsening",
                 1,
                 "Level {} :: num_vertices = {}, num_hyperedges = {}",
                 num_iter + 1,
                 hierarchy.back()->GetNumVertices(),
                 hierarchy.back()->GetNumHyperedges());
    }
    // check the early-stop condition
    if (hierarchy.back()->GetNumVertices() <= thr_coarsen_vertices_
        || hierarchy.back()->GetNumHyperedges() <= thr_coarsen_hyperedges_
        || (num_vertices_pre_iter - num_vertices_cur_iter)
               <= num_vertices_pre_iter * adj_diff_ratio_) {
      break;
    }
  }
  auto end_timestamp = std::chrono::high_resolution_clock::now();
  double time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(
                          end_timestamp - start_timestamp)
                          .count()
                      * 1e-9;
  debugPrint(logger_,
             PAR,
             "coarsening",
             1,
             "Hierarchical coarsening time {} seconds",
             time_taken);
  return hierarchy;
}

// create a coarser hypergraph based on specified grouping information
// for each vertex.
// each vertex has been map its group
// (1) remove single-vertex hyperedge
// (2) remove lager hyperedge
// (3) detect parallel hyperedges
// (4) handle group information
// (5) group fixed vertices based on each block
// group vertices based on group_attr and hgraph->fixed_attr_
HGraphPtr Coarsener::GroupVertices(
    const HGraphPtr& hgraph,
    const std::vector<std::vector<int>>& group_attr) const
{
  std::vector<int>
      vertex_cluster_id_vec;          // map current vertex_id to cluster_id
  Matrix<float> vertex_weights_c;     // cluster weight
  std::vector<int> community_attr_c;  // cluster community information
  std::vector<int> fixed_attr_c;      // cluster fixed attribute
  Matrix<float> placement_attr_c;     // cluster placement attribute

  // Cluster based group information
  ClusterBasedGroupInfo(hgraph,
                        group_attr,
                        vertex_cluster_id_vec,
                        vertex_weights_c,
                        community_attr_c,
                        fixed_attr_c,
                        placement_attr_c);

  // coarsen the input hypergraph based on vertex matching map
  auto clustered_hgraph = Contraction(hgraph,
                                      vertex_cluster_id_vec,
                                      vertex_weights_c,
                                      community_attr_c,
                                      fixed_attr_c,
                                      placement_attr_c);

  // update the timing cost of the clusterd_hgraph
  // hgraph will be updated here
  // For timing-driven flow,
  // we need to convert the slack information to related weight
  // Basically we will transform the path_timing_attr_ to path_timing_cost_,
  // and transform hyperedge_timing_attr_ to hyperedge_timing_cost_.
  // Then overlay the path weighgts onto corresponding weights
  if (hgraph->HasTiming()) {
    evaluator_->InitializeTiming(clustered_hgraph);
  }

  return clustered_hgraph;
}

// --------------------------------------------------------------------------
// Private functions
// ---------------------------------------------------------------------------

// Single-level Coarsening
// The input is a hypergraph
// The output is a coarser hypergraph
HGraphPtr Coarsener::Aggregate(const HGraphPtr& hgraph) const
{
  std::vector<int> vertex_cluster_id_vec;
  Matrix<float> vertex_weights_c;
  std::vector<int> community_attr_c;
  std::vector<int> fixed_attr_c;
  Matrix<float> placement_attr_c;

  // find the vertex matching scheme
  VertexMatching(hgraph,
                 vertex_cluster_id_vec,
                 vertex_weights_c,
                 community_attr_c,
                 fixed_attr_c,
                 placement_attr_c);

  // coarsen the input hypergraph based on vertex matching map
  auto clustered_hgraph = Contraction(hgraph,
                                      vertex_cluster_id_vec,
                                      vertex_weights_c,
                                      community_attr_c,
                                      fixed_attr_c,
                                      placement_attr_c);

  // update the timing cost of the clusterd_hgraph
  // hgraph will be updated here
  // For timing-driven flow,
  // we need to convert the slack information to related weight
  // Basically we will transform the path_timing_attr_ to path_timing_cost_,
  // and transform hyperedge_timing_attr_ to hyperedge_timing_cost_.
  // Then overlay the path weighgts onto corresponding weights
  if (hgraph->HasTiming()) {
    evaluator_->InitializeTiming(clustered_hgraph);
  }

  return clustered_hgraph;
}

// find the vertex matching scheme
// the inputs are the hgraph and the attributes of clusters
// the lazy update means that we do not change the hgraph itself,
// but during the matching process, we do dynamically update
// placement_attr_c. vertex_weights_c, fixed_attr_c and community_attr_c
void Coarsener::VertexMatching(
    const HGraphPtr& hgraph,
    std::vector<int>&
        vertex_cluster_id_vec,  // map current vertex_id to cluster_id
    // the remaining arguments are related to clusters
    Matrix<float>& vertex_weights_c,
    std::vector<int>& community_attr_c,
    std::vector<int>& fixed_attr_c,
    Matrix<float>& placement_attr_c) const
{
  // vertex_cluster_map_vec has the size of the number of vertices of hgraph
  vertex_cluster_id_vec.clear();
  vertex_cluster_id_vec.resize(hgraph->GetNumVertices());
  std::fill(vertex_cluster_id_vec.begin(), vertex_cluster_id_vec.end(), -1);
  // reset the attributes of clusters
  vertex_weights_c.clear();  // cluster weight
  community_attr_c.clear();  // cluster community
  fixed_attr_c.clear();      // cluster fixed attribute
  placement_attr_c.clear();  // cluster location
  // check all the vertices to be clustered
  int cluster_id = 0;  // the id of cluster
  std::vector<int> unvisited;
  unvisited.reserve(hgraph->GetNumVertices());
  // Ensure that fixed vertices in the hypergraph are not touched
  if (!hgraph->HasFixedVertices()) {
    // no fixed vertices
    unvisited.resize(hgraph->GetNumVertices());
    std::iota(unvisited.begin(), unvisited.end(), 0);
  } else {
    for (int v = 0; v < hgraph->GetNumVertices(); ++v) {
      // mark fixed vertices as single-vertex clusters
      if (hgraph->GetFixedAttr(v) > -1) {
        vertex_cluster_id_vec[v] = cluster_id++;
        vertex_weights_c.push_back(hgraph->GetVertexWeights(v));
        fixed_attr_c.push_back(hgraph->GetFixedAttr(v));
        if (hgraph->HasCommunity()) {
          community_attr_c.push_back(hgraph->GetCommunity(v));
        }
        if (hgraph->HasPlacement()) {
          placement_attr_c.push_back(hgraph->GetPlacement(v));
        }
      } else {
        unvisited.push_back(v);  // this vertex is not fixed
      }
    }
  }
  // shuffle the remaining vertices based on user-specified options
  OrderVertices(hgraph, unvisited);
  // calculate the best vertex to cluster for current vertex
  // if the number of visited vertices is larger than
  // num_early_stop_visited_vertices, then stop the coarsening process
  const int num_early_stop_visited_vertices
      = static_cast<int>(unvisited.size()) / coarsening_ratio_;
  int num_visited_vertices = 0;
  for (auto v_iter = unvisited.begin(); v_iter != unvisited.end(); v_iter++) {
    const int v = *v_iter;  // here we should use iterator to enable early-stop
                            // mechanism
    if (vertex_cluster_id_vec[v] > -1) {
      continue;  // this vertex has been mapped
    }

    // initialize the score for neighbors
    std::map<int, float> score_map;
    // traverse all its neighbors
    for (const int he : hgraph->Edges(v)) {
      const auto edge_range = hgraph->Vertices(he);
      const int he_size = edge_range.size();
      if (he_size <= 1 || he_size > thr_coarsen_hyperedge_size_skip_) {
        continue;
      }
      // get the normalized score
      const float he_score = evaluator_->GetNormEdgeScore(he, hgraph);
      // check the vertices in this hyperedge
      for (const int nbr_v : edge_range) {
        if (nbr_v == v) {
          continue;  // ignore the vertex v itself
        }
        // if the nbr_v has been identified
        if (score_map.find(nbr_v) != score_map.end()) {
          score_map[nbr_v] += he_score;
          continue;
        }
        // if the nbr_v is a new neighbor
        //
        // check if the merging conditions are satisfied
        // we do not allow the weight of cluster exceed the weight threshold
        // we do not allow the merging of non-fixed vertices with fixed-vertices
        // we do not allow the merging between vertices in different communities
        if ((hgraph->HasFixedVertices() && hgraph->GetFixedAttr(nbr_v) > -1)
            || (hgraph->HasCommunity()
                && hgraph->GetCommunity(v) != hgraph->GetCommunity(nbr_v))) {
          continue;
        }
        // check the vertex weight constraint
        const std::vector<float>& nbr_v_weight
            = vertex_cluster_id_vec[nbr_v] > -1
                  ? vertex_weights_c[vertex_cluster_id_vec[nbr_v]]
                  : hgraph->GetVertexWeights(nbr_v);
        // This line needs to be updated
        if (hgraph->GetVertexWeights(v) + nbr_v_weight > thr_cluster_weight_) {
          continue;  // cannot satisfy the vertex weight constraint
        }
        score_map[nbr_v] = he_score;
      }
    }  // finish traversing all the neighbors

    // if there is no neighbor, map current vertex as a single-vertex cluster
    if (score_map.empty()) {
      num_visited_vertices++;
      vertex_cluster_id_vec[v] = cluster_id++;
      vertex_weights_c.push_back(hgraph->GetVertexWeights(v));
      if (hgraph->HasPlacement()) {
        placement_attr_c.push_back(hgraph->GetPlacement(v));
      }
      if (hgraph->HasCommunity()) {
        community_attr_c.push_back(hgraph->GetCommunity(v));
      }
      if (hgraph->HasFixedVertices()) {
        fixed_attr_c.push_back(hgraph->GetFixedAttr(v));
      }
      continue;
    }
    // update the score based on critical timing paths
    // Here we do not need to traverse the entire paths
    // we just need to check the neighbors of the path
    // because if there is a path, the most important neighbors
    // must have been counter when traversing hyperedges before
    // We just consider the direct neighbors of the vertex
    // i.e., left neighbor and right neighbor
    // TODO: 20230409:
    // Exploration that if we can further improve the results by considering
    // more neighbors on timing-critical paths
    // No idea yet.
    if (hgraph->HasTiming() && hgraph->GetNumTimingPaths() > 0) {
      for (const int p : hgraph->TimingPathsThrough(v)) {
        const float path_timing_score
            = evaluator_->GetPathTimingScore(p, hgraph);
        // traverse the current path
        auto path_range = hgraph->PathVertices(p);
        for (auto iter = path_range.begin(); iter != path_range.end(); ++iter) {
          const int vertex_id = *iter;
          if (vertex_id != v) {
            continue;  // we need to find the neighbors of v, so continue here
          }
          std::vector<int> neighbors;
          if (iter != path_range.begin()) {
            neighbors.push_back(*(iter - 1));  // left neighbor
          }
          if (iter + 1 != path_range.end()) {
            neighbors.push_back(*(iter + 1));  // right neighbor
          }
          // add the score.
          // If the neighbor not found by connectivity, which means the balance
          // constraint cannot be statisfied
          for (const auto& nbr_v : neighbors) {
            if (score_map.find(nbr_v) != score_map.end()) {
              score_map[nbr_v] += path_timing_score;
            }
          }
        }  // finish traversing current paths
      }    // finish current nbr_v
    }
    // update the score based on physical location information
    if (hgraph->HasPlacement()) {
      for (auto& [u, score] : score_map) {  // the score will be updated
        score += evaluator_->GetPlacementScore(v, u, hgraph);
      }
    }
    // find the best neighbor vertex
    float best_score = -std::numeric_limits<float>::max();
    int best_vertex = -1;
    for (const auto& [u, score] : score_map) {
      if (score > best_score) {
        best_vertex = u;
        best_score = score;
      } else if (score == best_score && vertex_cluster_id_vec[u] == -1) {
        best_vertex = u;
      }
    }

    if (best_vertex == -1) {
      num_visited_vertices += 1;
      vertex_cluster_id_vec[v] = cluster_id;
      cluster_id++;
      vertex_weights_c.push_back(hgraph->GetVertexWeights(v));
      if (hgraph->HasPlacement()) {
        placement_attr_c.push_back(hgraph->GetPlacement(v));
      }
      if (hgraph->HasCommunity()) {
        community_attr_c.push_back(hgraph->GetCommunity(v));
      }
      if (hgraph->HasFixedVertices()) {
        fixed_attr_c.push_back(hgraph->GetFixedAttr(v));
      }
      continue;
    }

    // cluster best_vertex and v
    // Case 1 : best_vertex has been clustered with other vertices, add v to
    // that cluster Case 2 : best_vertex and v both are not clustered
    if (vertex_cluster_id_vec[best_vertex] > -1) {
      num_visited_vertices++;
      const int best_cluster_id = vertex_cluster_id_vec[best_vertex];
      vertex_cluster_id_vec[v] = best_cluster_id;
      // you cannot change the order here
      // update the placement location
      if (hgraph->HasPlacement()) {
        placement_attr_c[best_cluster_id]
            = evaluator_->GetAvgPlacementLoc(vertex_weights_c[best_cluster_id],
                                             hgraph->GetVertexWeights(v),
                                             placement_attr_c[best_cluster_id],
                                             hgraph->GetPlacement(v));
      }
      // update the weight of cluster
      vertex_weights_c[best_cluster_id]
          = vertex_weights_c[best_cluster_id] + hgraph->GetVertexWeights(v);
    } else {
      num_visited_vertices += 2;
      vertex_cluster_id_vec[best_vertex] = cluster_id;
      vertex_cluster_id_vec[v] = cluster_id;
      cluster_id++;
      vertex_weights_c.push_back(hgraph->GetVertexWeights(best_vertex)
                                 + hgraph->GetVertexWeights(v));
      if (hgraph->HasPlacement()) {
        placement_attr_c.push_back(
            evaluator_->GetAvgPlacementLoc(v, best_vertex, hgraph));
      }
      if (hgraph->HasCommunity()) {
        community_attr_c.push_back(hgraph->GetCommunity(v));
      }
      if (hgraph->HasFixedVertices()) {
        fixed_attr_c.push_back(hgraph->GetFixedAttr(v));
      }
    }
    const int remaining_vertices
        = hgraph->GetNumVertices() + cluster_id - num_visited_vertices;
    // check the early-stop condition
    if (remaining_vertices <= num_early_stop_visited_vertices) {
      v_iter++;
      while (v_iter != unvisited.end()) {
        const int cur_vertex = *v_iter;
        // increasr the pointer
        v_iter++;
        if (vertex_cluster_id_vec[cur_vertex] > -1) {
          continue;  // this vertex has been visited
        }
        vertex_cluster_id_vec[cur_vertex] = cluster_id++;
        vertex_weights_c.push_back(hgraph->GetVertexWeights(cur_vertex));
        if (hgraph->HasPlacement()) {
          placement_attr_c.push_back(hgraph->GetPlacement(cur_vertex));
        }
        if (hgraph->HasCommunity()) {
          community_attr_c.push_back(hgraph->GetCommunity(cur_vertex));
        }
        if (hgraph->HasFixedVertices()) {
          fixed_attr_c.push_back(hgraph->GetFixedAttr(cur_vertex));
        }
      }
      return;  // exit the coarsening process
    }          // early exit
  }
}

// handle group information
// group fixed vertices based on each block
// group vertices based on group_attr and hgraph->fixed_attr_
// the community id of a group vertices is the maximum of the community id of
// all vertices the fixed block id of a group vertices is the maximum of the
// fixed block id of all vertices Two group will be merged if two groups share
// the same vertex All the fixed vertices in one block will be identified as the
// same group
void Coarsener::ClusterBasedGroupInfo(
    const HGraphPtr& hgraph,
    const std::vector<std::vector<int>>& group_attr,
    std::vector<int>&
        vertex_cluster_id_vec,  // map current vertex_id to cluster_id
    // the remaining arguments are related to clusters
    Matrix<float>& vertex_weights_c,
    std::vector<int>& community_attr_c,
    std::vector<int>& fixed_attr_c,
    Matrix<float>& placement_attr_c) const
{
  // convert group_attr to vertex_cluster_id_vec
  if (group_attr.empty() == true && hgraph->GetFixedAttrSize() == 0) {
    // no need to any group based on group_attr and hgraph->fixed_attr_
    vertex_cluster_id_vec.clear();
    vertex_cluster_id_vec.resize(hgraph->GetNumVertices());
    std::iota(vertex_cluster_id_vec.begin(), vertex_cluster_id_vec.end(), 0);
    vertex_weights_c = hgraph->GetVertexWeights();
    hgraph->CopyCommunity(community_attr_c);
    hgraph->CopyFixedAttr(fixed_attr_c);
    hgraph->CopyPlacement(placement_attr_c);
    return;
  }

  // the remaining attributes
  // we need to convert the fixed vertices into clusters of vertices
  // in each block
  std::vector<std::vector<int>> temp_fixed_group;
  temp_fixed_group.resize(num_parts_);
  // check fixed vertices
  if (hgraph->GetFixedAttrSize() > 0) {
    for (int v = 0; v < hgraph->GetNumVertices(); v++) {
      if (hgraph->GetFixedAttr(v) > -1) {
        temp_fixed_group[hgraph->GetFixedAttr(v)].push_back(v);
      }
    }
  }

  std::vector<std::vector<int>> fixed_group;
  for (auto& group : temp_fixed_group) {
    if (group.empty() == false) {
      fixed_group.push_back(group);
    }
  }

  // concatenate fixed group and group_attr
  fixed_group.insert(fixed_group.end(), group_attr.begin(), group_attr.end());
  // We need to merge groups if two groups share at least one common vertex
  // first initialize the vertex_cluster_id_vec
  vertex_cluster_id_vec.clear();
  vertex_cluster_id_vec.resize(hgraph->GetNumVertices());
  std::fill(vertex_cluster_id_vec.begin(), vertex_cluster_id_vec.end(), -1);
  int cluster_id = 0;  // start cluster id
  for (const auto& group : fixed_group) {
    // compute the cluster id of current group
    // to check if we need to merge the group
    int cur_cluster_id = -1;
    for (const auto& v : group) {
      cur_cluster_id = std::max(cur_cluster_id, vertex_cluster_id_vec[v]);
    }
    if (cur_cluster_id == -1) {
      cur_cluster_id = cluster_id++;
    }
    for (const auto& v : group) {
      vertex_cluster_id_vec[v] = cur_cluster_id;
    }
  }
  // map remaining vertex into a single-vertex cluster
  for (int v = 0; v < hgraph->GetNumVertices(); v++) {
    if (vertex_cluster_id_vec[v] == -1) {
      vertex_cluster_id_vec[v] = cluster_id++;
    }
  }
  const int num_clusters = cluster_id;
  // update attributes
  vertex_weights_c.clear();
  community_attr_c.clear();
  fixed_attr_c.clear();
  placement_attr_c.clear();
  // update vertex weights
  vertex_weights_c.clear();
  for (int v = 0; v < num_clusters; v++) {
    std::vector<float> v_wt(hgraph->GetVertexDimensions(), 0.0);
    vertex_weights_c.push_back(v_wt);
  }
  if (hgraph->HasCommunity()) {
    community_attr_c.clear();
    community_attr_c.resize(num_clusters);
    std::fill(community_attr_c.begin(), community_attr_c.end(), -1);
  }
  if (hgraph->HasFixedVertices()) {
    fixed_attr_c.clear();
    fixed_attr_c.resize(num_clusters);
    std::fill(fixed_attr_c.begin(), fixed_attr_c.end(), -1);
  }
  if (hgraph->HasPlacement()) {
    placement_attr_c.clear();
    for (int v = 0; v < num_clusters; v++) {
      std::vector<float> v_placement(hgraph->GetPlacementDimensions(), 0.0);
      placement_attr_c.push_back(v_placement);
    }
  }

  // Update the attributes of clusters
  for (int v = 0; v < hgraph->GetNumVertices(); v++) {
    const int cluster_id = vertex_cluster_id_vec[v];
    if (hgraph->HasCommunity()) {
      community_attr_c[cluster_id]
          = std::max(community_attr_c[cluster_id], hgraph->GetCommunity(v));
    }
    if (hgraph->HasFixedVertices()) {
      fixed_attr_c[cluster_id]
          = std::max(fixed_attr_c[cluster_id], hgraph->GetFixedAttr(v));
    }
    if (hgraph->HasPlacement()) {
      placement_attr_c[cluster_id]
          = evaluator_->GetAvgPlacementLoc(vertex_weights_c[cluster_id],
                                           hgraph->GetVertexWeights(v),
                                           placement_attr_c[cluster_id],
                                           hgraph->GetPlacement(v));
    }
    vertex_weights_c[cluster_id]
        = vertex_weights_c[cluster_id] + hgraph->GetVertexWeights(v);
  }
}

// order the vertices based on user-specified parameters
void Coarsener::OrderVertices(const HGraphPtr& hgraph,
                              std::vector<int>& vertices) const
{
  switch (vertex_order_choice_) {
    case CoarsenOrder::RANDOM:
      shuffle(vertices.begin(),
              vertices.end(),
              std::default_random_engine(random_seed_));
      return;

    case CoarsenOrder::DEFAULT:
      return;

    case CoarsenOrder::SIZE: {
      // sort the vertices based on vertex weight
      // calculate the weight for all the vertices
      std::vector<float> average_sizes(hgraph->GetNumVertices(), 0.0);
      for (const auto& v : vertices) {
        average_sizes[v] = evaluator_->GetVertexWeightNorm(v, hgraph);
      }
      // define the sort function
      auto lambda_sort_size = [&](int& x, int& y) -> bool {
        return average_sizes[x] < average_sizes[y];
      };
      std::sort(vertices.begin(), vertices.end(), lambda_sort_size);
    }
      return;

    case CoarsenOrder::DEGREE: {
      // sort the vertices based on degree of each vertex in non-decreasing
      // order i.e., number of neighboring vertices
      std::vector<int> degrees(hgraph->GetNumVertices(), 0);
      for (const auto& v : vertices) {
        std::set<int> nbr_vertices;
        for (const int he : hgraph->Edges(v)) {
          for (const int nbr : hgraph->Vertices(he)) {
            if (nbr != v) {
              nbr_vertices.insert(nbr);
            }
          }
        }
        degrees[v] = static_cast<int>(nbr_vertices.size());
      }
      auto lambda_sort_degree
          = [&](int& x, int& y) -> bool { return degrees[x] > degrees[y]; };
      std::sort(vertices.begin(), vertices.end(), lambda_sort_degree);
    }
      return;

    default:
      return;  // use the default order of vertices
  }
}

//  create the contracted hypergraph based on the vertex matching in
//  vertex_cluster_id_vec
HGraphPtr Coarsener::Contraction(
    const HGraphPtr& hgraph,
    const std::vector<int>&
        vertex_cluster_id_vec,  // map current vertex_id to cluster_id
    // the remaining arguments are related to clusters
    const Matrix<float>& vertex_weights_c,
    const std::vector<int>& community_attr_c,
    const std::vector<int>& fixed_attr_c,
    const Matrix<float>& placement_attr_c) const
{
  // Step 1:  identify the contracted hyperedges
  std::vector<int> hyperedge_cluster_id_vec;  // map the hyperedge to hyperedge
                                              // in clustered hypergraph
  hyperedge_cluster_id_vec.resize(hgraph->GetNumHyperedges());
  // -1 means the hyperedge is fully within one cluster
  std::fill(
      hyperedge_cluster_id_vec.begin(), hyperedge_cluster_id_vec.end(), -1);
  Matrix<int> hyperedges_c;  // represent each hyperedge as a set of clusters
  Matrix<float> hyperedges_weights_c;  // each element represents the weight of
                                       // the clustered hyperedge
  std::vector<float> hyperedge_slack_c;  // the slack for clustered hyperedge.
  std::vector<std::set<int>>
      hyperedge_arc_set_c;  // map current hyperedge into arcs in timing graph.
                            // We need this for propagation
  std::map<size_t, int>
      hash_map;  // store the hash value of each contracted hyperedge
  std::map<size_t, std::vector<int>>
      parallel_hash_map;  // store the hyperedges_c with the same hash_value
                          // (candidate)
  for (int e = 0; e < hgraph->GetNumHyperedges(); e++) {
    const auto range = hgraph->Vertices(e);
    const int he_size = range.size();
    if (he_size <= 1 || he_size > thr_coarsen_hyperedge_size_skip_) {
      continue;  // ignore the single-vertex hyperedge and large hyperedge
    }
    std::set<int> hyperedge_c;
    for (const int vertex_id : range) {
      hyperedge_c.insert(vertex_cluster_id_vec[vertex_id]);  // get cluster id
    }
    if (hyperedge_c.size() <= 1) {
      continue;  // ignore the single-vertex hyperedge
    }
    size_t hash_value = std::inner_product(hyperedge_c.begin(),
                                           hyperedge_c.end(),
                                           hyperedge_c.begin(),
                                           static_cast<size_t>(0));
    // check if the hash value has been used
    // for detecting parallel hyperedge
    // hyperedge_slack_c[e] = min_slack(hyperedge_arc_set_c[e])
    if (hash_map.find(hash_value) == hash_map.end()) {
      const int hyperedge_c_id = static_cast<int>(hyperedges_c.size());
      hyperedge_cluster_id_vec[e] = hyperedge_c_id;
      hash_map[hash_value] = hyperedge_c_id;
      hyperedges_c.push_back(
          std::vector<int>(hyperedge_c.begin(), hyperedge_c.end()));
      hyperedges_weights_c.push_back(hgraph->GetHyperedgeWeights(e));
      if (hgraph->HasTiming()) {
        hyperedge_slack_c.push_back(
            hgraph->GetHyperedgeTimingAttr(e));  // the slack of hyperedge
        hyperedge_arc_set_c.push_back(
            hgraph->GetHyperedgeArcSet(e));  // map the hyperedge to timing arcs
      }
      continue;
    }

    // there may be parallel hyperedges
    const int hash_hyperedge_c_id
        = hash_map[hash_value];  // the hyperedge_c has been found
    std::vector<int> hyperedge_vec(hyperedge_c.begin(), hyperedge_c.end());
    // check the representative hyperedge_c
    int parallel_hyperedge_c_id
        = -1;  // the hyperedge_c_id of parallel hyperedge
    // find the parallel_hyperedge_c_id
    if (hyperedge_vec == hyperedges_c[hash_hyperedge_c_id]) {
      // check the representative hyperedge_c
      parallel_hyperedge_c_id = hash_hyperedge_c_id;
    } else {
      // check the parallel hyperedge_c_id
      for (const auto& candidate_id : parallel_hash_map[hash_value]) {
        if (hyperedge_vec == hyperedges_c[candidate_id]) {
          parallel_hyperedge_c_id = candidate_id;
          break;  // found the same hyperedge_c
        }
      }
    }
    // check if the hyperedge has been existed
    if (parallel_hyperedge_c_id == -1) {
      // not existed
      const int hyperedge_c_id = static_cast<int>(hyperedges_c.size());
      hyperedge_cluster_id_vec[e] = hyperedge_c_id;
      parallel_hash_map[hash_value].push_back(hyperedge_c_id);
      hyperedges_c.push_back(hyperedge_vec);
      hyperedges_weights_c.push_back(hgraph->GetHyperedgeWeights(e));
      if (hgraph->HasTiming()) {
        hyperedge_slack_c.push_back(
            hgraph->GetHyperedgeTimingAttr(e));  // the slack of hyperedge
        hyperedge_arc_set_c.push_back(
            hgraph->GetHyperedgeArcSet(e));  // map the hyperedge to timing arcs
      }
    } else {
      // existed
      hyperedges_weights_c[parallel_hyperedge_c_id]
          = hyperedges_weights_c[parallel_hyperedge_c_id]
            + hgraph->GetHyperedgeWeights(e);
      hyperedge_cluster_id_vec[e] = parallel_hyperedge_c_id;
      if (hgraph->HasTiming()) {
        hyperedge_slack_c[parallel_hyperedge_c_id]
            = std::min(hyperedge_slack_c[parallel_hyperedge_c_id],
                       hgraph->GetHyperedgeTimingAttr(e));
        hyperedge_arc_set_c[parallel_hyperedge_c_id].insert(
            hgraph->GetHyperedgeArcSet(e).begin(),
            hgraph->GetHyperedgeArcSet(e).end());
      }
    }
  }

  // Step 2: identify all the timing paths
  std::vector<TimingPath> timing_paths_c;
  hash_map.clear();           // used to detect parallel timing path
  parallel_hash_map.clear();  // used to detect parallel timing path
  if (hgraph->HasTiming() && hgraph->GetNumTimingPaths() > 0) {
    for (int p = 0; p < hgraph->GetNumTimingPaths(); ++p) {
      // check vertex representation
      auto path_range = hgraph->PathVertices(p);
      const int path_size_original = path_range.size();

      if (path_size_original <= 1) {
        continue;  // ignore single-vertex path
      }
      std::vector<int> path_c;  // create path_c
      for (const int vertex_id : path_range) {
        const int cluster_id = vertex_cluster_id_vec[vertex_id];
        if (path_c.empty() == true || path_c.back() != cluster_id) {
          path_c.push_back(cluster_id);
        }
      }
      if (path_c.size() <= 1) {
        continue;  // ignore single-vertex path
      }
      const size_t hash_value = std::inner_product(
          path_c.begin(), path_c.end(), path_c.begin(), static_cast<size_t>(0));
      if (hash_map.find(hash_value) == hash_map.end()) {
        // the path does not exists
        // check hyperedge representation
        std::vector<int> arcs_c;
        for (const int edge : hgraph->PathEdges(p)) {
          const int hyperedge_c_id = hyperedge_cluster_id_vec[edge];
          // if hyperedge_c_id is -1, that means that hyperedge has been merged
          // during coarsening
          if ((hyperedge_c_id > -1)
              && (arcs_c.empty() == true || arcs_c.back() != hyperedge_c_id)) {
            arcs_c.push_back(hyperedge_c_id);
          }
        }
        hash_map[hash_value] = static_cast<int>(timing_paths_c.size());
        TimingPath timing_path_c(path_c, arcs_c, hgraph->PathTimingSlack(p));
        timing_paths_c.push_back(timing_path_c);
      } else {
        const int timing_path_c_id = hash_map[hash_value];
        int parallel_timing_path_c_id
            = -1;  // the timing_path_id of parallel timing path
        if (path_c == timing_paths_c[timing_path_c_id].path) {
          // check the representative timing path
          parallel_timing_path_c_id = timing_path_c_id;
        } else {
          // check the parallel timing paths
          for (const auto& candidate_id : parallel_hash_map[hash_value]) {
            if (path_c == timing_paths_c[candidate_id].path) {
              parallel_timing_path_c_id = candidate_id;
              break;  // found the same timing path
            }
          }
        }
        // check if the timing path has been existed
        if (parallel_timing_path_c_id == -1) {
          // not existed
          std::vector<int> arcs_c;
          for (const int edge : hgraph->PathEdges(p)) {
            const int hyperedge_c_id = hyperedge_cluster_id_vec[edge];
            if ((hyperedge_c_id > -1)
                && (arcs_c.empty() == true
                    || arcs_c.back() != hyperedge_c_id)) {
              arcs_c.push_back(hyperedge_c_id);
            }
          }
          parallel_hash_map[hash_value].push_back(
              static_cast<int>(timing_paths_c.size()));
          timing_paths_c.emplace_back(
              path_c, arcs_c, hgraph->PathTimingSlack(p));
        } else {
          // existed
          timing_paths_c[parallel_timing_path_c_id].slack
              = std::min(timing_paths_c[parallel_timing_path_c_id].slack,
                         hgraph->PathTimingSlack(p));
        }
      }
    }
  }

  // create vertex_type for clustered hypergraph
  // Since we allow the merge between different types of vertices
  // so there is no meaning of vertex_type for clusterd hypergraph
  // so we use empty vector here
  std::vector<VertexType> vertex_types_c;

  // Step 3: create the contracted hypergraph
  auto clustered_hgraph
      = std::make_shared<Hypergraph>(hgraph->GetVertexDimensions(),
                                     hgraph->GetHyperedgeDimensions(),
                                     hgraph->GetPlacementDimensions(),
                                     hyperedges_c,
                                     vertex_weights_c,
                                     hyperedges_weights_c,
                                     // vertex attributes
                                     fixed_attr_c,
                                     community_attr_c,
                                     placement_attr_c,
                                     vertex_types_c,
                                     // timing information
                                     hyperedge_slack_c,
                                     hyperedge_arc_set_c,
                                     timing_paths_c,
                                     logger_);

  // fill vertex_c_attr which maps the vertex to its corresponding cluster
  // To simpify the implementation, the vertex_c_attr maps the original larger
  // hypergraph
  clustered_hgraph->ResetVertexCAttr();
  for (int v = 0; v < hgraph->GetNumVertices(); v++) {
    clustered_hgraph->AddVertexCAttr(vertex_cluster_id_vec[v], v);
  }

  return clustered_hgraph;
}

// Utility functions
std::string ToString(const CoarsenOrder order)
{
  switch (order) {
    case CoarsenOrder::RANDOM:
      return std::string("RANDOM");

    case CoarsenOrder::DEGREE:
      return std::string("DEGREE");

    case CoarsenOrder::SIZE:
      return std::string("SIZE");

    case CoarsenOrder::DEFAULT:
      return std::string("DEFAULT");

    default:
      return std::string("DEFAULT");
  }
};

}  // namespace par
