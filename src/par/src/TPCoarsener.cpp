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

#include "TPCoarsener.h"

#include <set>

#include "TPEvaluator.h"
#include "TPHypergraph.h"
#include "Utilities.h"
#include "utl/Logger.h"

using utl::PAR;

namespace par {

// Implemention of TPcoarsener class

// The main function pf TPcoarsener class
// The input is a hypergraph
// The output is a sequence of coarser hypergraphs
// Notice that the input hypergraph is not const,
// because the hgraphs returned can be edited
// The timing cost of hgraph will be initialized if it has been not.
TP_coarse_graph_ptrs TPcoarsener::LazyFirstChoice(const HGraphPtr& hgraph) const
{
  const auto start_timestamp = std::chrono::high_resolution_clock::now();
  const bool timing_flag = hgraph->timing_flag_;
  std::vector<HGraphPtr> hierarchy;  // a sequence of coarser hypergraphs
  // start the FC multilevel coarsening by pushing the original hgraph to
  // hierarchy update the timing cost of hgraph if it has been not.
  if (hgraph->timing_flag_ == true && hgraph->path_timing_cost_.empty() == true
      && hgraph->hyperedge_timing_cost_.empty() == true) {
    evaluator_->InitializeTiming(hgraph);
  }
  hierarchy.push_back(hgraph);  // push original hgraph to hierarchy
  logger_->report("=========================================");
  logger_->report("[STATUS] Running FC multilevel coarsening");
  logger_->report("=========================================");
  if (timing_flag == true) {
    logger_->report(
        "[COARSEN] Level 0 :: num_vertices = {}, num_hyperedges = {}, "
        "num_timing_paths = {}",
        hierarchy.back()->num_vertices_,
        hierarchy.back()->num_hyperedges_,
        hierarchy.back()->num_timing_paths_);
  } else {
    logger_->report(
        "[COARSEN] Level 0 :: num_vertices = {}, num_hyperedges = {}",
        hierarchy.back()->num_vertices_,
        hierarchy.back()->num_hyperedges_);
  }

  for (int num_iter = 0; num_iter < max_coarsen_iters_; ++num_iter) {
    // do coarsening step
    auto hg = Aggregate(hierarchy.back());
    const int num_vertices_pre_iter = hierarchy.back()->num_vertices_;
    const int num_vertices_cur_iter = hg->num_vertices_;
    if (num_vertices_pre_iter == num_vertices_cur_iter) {
      break;  // the current hypergraph hg is the same as the hypergraph in
              // previous iteration
    }
    hierarchy.push_back(hg);
    if (timing_flag == true) {
      logger_->report(
          "[COARSEN] Level {} :: num_vertices = {}, num_hyperedges = {}, "
          "num_timing_paths = {}",
          num_iter + 1,
          hierarchy.back()->num_vertices_,
          hierarchy.back()->num_hyperedges_,
          hierarchy.back()->num_timing_paths_);
    } else {
      logger_->report(
          "[COARSEN] Level {} :: num_vertices = {}, num_hyperedges = {}",
          num_iter + 1,
          hierarchy.back()->num_vertices_,
          hierarchy.back()->num_hyperedges_);
    }
    // check the early-stop condition
    if (hierarchy.back()->num_vertices_ <= thr_coarsen_vertices_
        || hierarchy.back()->num_hyperedges_ <= thr_coarsen_hyperedges_
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
  logger_->info(PAR, 1, "Hierarchical coarsening time {} seconds", time_taken);
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
HGraphPtr TPcoarsener::GroupVertices(
    const HGraphPtr& hgraph,
    const std::vector<std::vector<int>>& group_attr) const
{
  std::vector<int>
      vertex_cluster_id_vec;          // map current vertex_id to cluster_id
  MATRIX<float> vertex_weights_c;     // cluster weight
  std::vector<int> community_attr_c;  // cluster community information
  std::vector<int> fixed_attr_c;      // cluster fixed attribute
  MATRIX<float> placement_attr_c;     // cluster placement attribute

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
  if (hgraph->timing_flag_ == true) {
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
HGraphPtr TPcoarsener::Aggregate(const HGraphPtr& hgraph) const
{
  std::vector<int> vertex_cluster_id_vec;
  MATRIX<float> vertex_weights_c;
  std::vector<int> community_attr_c;
  std::vector<int> fixed_attr_c;
  MATRIX<float> placement_attr_c;

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
  if (hgraph->timing_flag_ == true) {
    evaluator_->InitializeTiming(clustered_hgraph);
  }

  return clustered_hgraph;
}

// find the vertex matching scheme
// the inputs are the hgraph and the attributes of clusters
// the lazy update means that we do not change the hgraph itself,
// but during the matching process, we do dynamically update
// placement_attr_c. vertex_weights_c, fixed_attr_c and community_attr_c
void TPcoarsener::VertexMatching(
    const HGraphPtr& hgraph,
    std::vector<int>&
        vertex_cluster_id_vec,  // map current vertex_id to cluster_id
    // the remaining arguments are related to clusters
    MATRIX<float>& vertex_weights_c,
    std::vector<int>& community_attr_c,
    std::vector<int>& fixed_attr_c,
    MATRIX<float>& placement_attr_c) const
{
  // vertex_cluster_map_vec has the size of the number of vertices of hgraph
  vertex_cluster_id_vec.clear();
  vertex_cluster_id_vec.resize(hgraph->num_vertices_);
  std::fill(vertex_cluster_id_vec.begin(), vertex_cluster_id_vec.end(), -1);
  // reset the attributes of clusters
  vertex_weights_c.clear();  // cluster weight
  community_attr_c.clear();  // cluster community
  fixed_attr_c.clear();      // cluster fixed attribute
  placement_attr_c.clear();  // cluster location
  // check all the vertices to be clustered
  int cluster_id = 0;  // the id of cluster
  std::vector<int> unvisited;
  unvisited.reserve(hgraph->num_vertices_);
  // Ensure that fixed vertices in the hypergraph are not touched
  if (hgraph->fixed_vertex_flag_ == false) {
    // no fixed vertices
    unvisited.resize(hgraph->num_vertices_);
    std::iota(unvisited.begin(), unvisited.end(), 0);
  } else {
    for (int v = 0; v < hgraph->num_vertices_; ++v) {
      // mark fixed vertices as single-vertex clusters
      if (hgraph->fixed_attr_[v] > -1) {
        vertex_cluster_id_vec[v] = cluster_id++;
        vertex_weights_c.push_back(hgraph->vertex_weights_[v]);
        fixed_attr_c.push_back(hgraph->fixed_attr_[v]);
        if (hgraph->community_flag_ == true) {
          community_attr_c.push_back(hgraph->community_attr_[v]);
        }
        if (hgraph->placement_flag_ == true) {
          placement_attr_c.push_back(hgraph->placement_attr_[v]);
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
    const int first_valid_entry_v = hgraph->vptr_[v];
    const int first_invalid_entry_v = hgraph->vptr_[v + 1];
    for (int i = first_valid_entry_v; i < first_invalid_entry_v; ++i) {
      const int he = hgraph->vind_[i];  // hyperedge size
      const int first_valid_entry_he = hgraph->eptr_[he];
      const int first_invalid_entry_he = hgraph->eptr_[he + 1];
      const int he_size = first_invalid_entry_he - first_valid_entry_he;
      if (he_size <= 1 || he_size > thr_coarsen_hyperedge_size_skip_) {
        continue;
      }
      // get the normalized score
      const float he_score = evaluator_->GetNormEdgeScore(he, hgraph);
      // check the vertices in this hyperedge
      for (int j = first_valid_entry_he; j < first_invalid_entry_he; ++j) {
        const int nbr_v = hgraph->eind_[j];
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
        if ((hgraph->fixed_vertex_flag_ == true
             && hgraph->fixed_attr_[nbr_v] > -1)
            || (hgraph->community_flag_ == true
                && hgraph->community_attr_[v]
                       != hgraph->community_attr_[nbr_v])) {
          continue;
        }
        // check the vertex weight constraint
        const std::vector<float>& nbr_v_weight
            = vertex_cluster_id_vec[nbr_v] > -1
                  ? vertex_weights_c[vertex_cluster_id_vec[nbr_v]]
                  : hgraph->vertex_weights_[nbr_v];
        // This line needs to be updated
        if (hgraph->vertex_weights_[v] + nbr_v_weight > thr_cluster_weight_) {
          continue;  // cannot satisfy the vertex weight constraint
        }
        score_map[nbr_v] = he_score;
      }
    }  // finish traversing all the neighbors
    // if there is no neighbor, map current vertex as a single-vertex cluster
    if (score_map.empty()) {
      num_visited_vertices++;
      vertex_cluster_id_vec[v] = cluster_id++;
      vertex_weights_c.push_back(hgraph->vertex_weights_[v]);
      if (hgraph->placement_flag_ == true) {
        placement_attr_c.push_back(hgraph->placement_attr_[v]);
      }
      if (hgraph->community_flag_ == true) {
        community_attr_c.push_back(hgraph->community_attr_[v]);
      }
      if (hgraph->fixed_vertex_flag_ == true) {
        fixed_attr_c.push_back(hgraph->fixed_attr_[v]);
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
    if (hgraph->timing_flag_ == true && hgraph->num_timing_paths_ > 0) {
      const int first_valid_entry_pv = hgraph->pptr_v_[v];
      const int first_invalid_entry_pv = hgraph->pptr_v_[v + 1];
      for (int j = first_valid_entry_pv; j < first_invalid_entry_pv; ++j) {
        const int p = hgraph->pind_v_[j];  // path_id
        const int first_valid_entry_p = hgraph->vptr_p_[p];
        const int first_invalid_entry_p = hgraph->vptr_p_[p + 1];
        const float path_timing_score
            = evaluator_->GetPathTimingScore(p, hgraph);
        // traverse the current path
        for (int idx = first_valid_entry_p; idx < first_invalid_entry_p;
             idx++) {
          const int vertex_id = hgraph->vind_p_[idx];
          if (vertex_id != v) {
            continue;  // we need to find the neighbors of v, so continue here
          }
          std::vector<int> neighbors;
          if (idx > first_invalid_entry_p) {
            neighbors.push_back(hgraph->vind_p_[idx - 1]);  // left neighbor
          }
          if (idx < first_invalid_entry_p - 1) {
            neighbors.push_back(hgraph->vind_p_[idx + 1]);  // right neighbor
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
    if (hgraph->placement_flag_ == true) {
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

    // cluster best_vertex and v
    // Case 1 : best_vertex has been clustered with other vertices, add v to
    // that cluster Case 2 : best_vertex and v both are not clustered
    if (vertex_cluster_id_vec[best_vertex] > -1) {
      num_visited_vertices++;
      const int best_cluster_id = vertex_cluster_id_vec[best_vertex];
      vertex_cluster_id_vec[v] = best_cluster_id;
      // you cannot change the order here
      // update the placement location
      if (hgraph->placement_flag_ == true) {
        placement_attr_c[best_cluster_id]
            = evaluator_->GetAvgPlacementLoc(vertex_weights_c[best_cluster_id],
                                             hgraph->vertex_weights_[v],
                                             placement_attr_c[best_cluster_id],
                                             hgraph->placement_attr_[v]);
      }
      // update the weight of cluster
      vertex_weights_c[best_cluster_id]
          = vertex_weights_c[best_cluster_id] + hgraph->vertex_weights_[v];
    } else {
      num_visited_vertices += 2;
      vertex_cluster_id_vec[best_vertex] = cluster_id;
      vertex_cluster_id_vec[v] = cluster_id;
      cluster_id++;
      vertex_weights_c.push_back(hgraph->vertex_weights_[best_vertex]
                                 + hgraph->vertex_weights_[v]);
      if (hgraph->placement_flag_ == true) {
        placement_attr_c.push_back(
            evaluator_->GetAvgPlacementLoc(v, best_vertex, hgraph));
      }
      if (hgraph->community_flag_ == true) {
        community_attr_c.push_back(hgraph->community_attr_[v]);
      }
      if (hgraph->fixed_vertex_flag_ == true) {
        fixed_attr_c.push_back(hgraph->fixed_attr_[v]);
      }
    }
    const int remaining_vertices
        = hgraph->num_vertices_ + cluster_id - num_visited_vertices;
    // check the early-stop condition
    if (remaining_vertices <= num_early_stop_visited_vertices) {
      int num_visited_vertices_new = 0;
      for (auto flag_new : vertex_cluster_id_vec) {
        if (flag_new > -1) {
          num_visited_vertices_new++;
        }
      }
      v_iter++;
      while (v_iter != unvisited.end()) {
        const int cur_vertex = *v_iter;
        // increasr the pointer
        v_iter++;
        if (vertex_cluster_id_vec[cur_vertex] > -1) {
          continue;  // this vertex has been visited
        }
        vertex_cluster_id_vec[cur_vertex] = cluster_id++;
        vertex_weights_c.push_back(hgraph->vertex_weights_[cur_vertex]);
        if (hgraph->placement_flag_ == true) {
          placement_attr_c.push_back(hgraph->placement_attr_[cur_vertex]);
        }
        if (hgraph->community_flag_ == true) {
          community_attr_c.push_back(hgraph->community_attr_[cur_vertex]);
        }
        if (hgraph->fixed_vertex_flag_ == true) {
          fixed_attr_c.push_back(hgraph->fixed_attr_[cur_vertex]);
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
void TPcoarsener::ClusterBasedGroupInfo(
    const HGraphPtr& hgraph,
    const std::vector<std::vector<int>>& group_attr,
    std::vector<int>&
        vertex_cluster_id_vec,  // map current vertex_id to cluster_id
    // the remaining arguments are related to clusters
    MATRIX<float>& vertex_weights_c,
    std::vector<int>& community_attr_c,
    std::vector<int>& fixed_attr_c,
    MATRIX<float>& placement_attr_c) const
{
  // convert group_attr to vertex_cluster_id_vec
  if (group_attr.empty() == true && hgraph->fixed_attr_.empty() == true) {
    // no need to any group based on group_attr and hgraph->fixed_attr_
    vertex_cluster_id_vec.clear();
    vertex_cluster_id_vec.resize(hgraph->num_vertices_);
    std::iota(vertex_cluster_id_vec.begin(), vertex_cluster_id_vec.end(), 0);
    vertex_weights_c = hgraph->vertex_weights_;
    community_attr_c = hgraph->community_attr_;
    fixed_attr_c = hgraph->fixed_attr_;
    placement_attr_c = hgraph->placement_attr_;
    return;
  }

  // the remaining attributes
  // we need to convert the fixed vertices into clusters of vertices
  // in each block
  std::vector<std::vector<int>> temp_fixed_group;
  temp_fixed_group.resize(num_parts_);
  // check fixed vertices
  if (hgraph->fixed_attr_.empty() == false) {
    for (int v = 0; v < hgraph->num_vertices_; v++) {
      if (hgraph->fixed_attr_[v] > -1) {
        temp_fixed_group[hgraph->fixed_attr_[v]].push_back(v);
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
  vertex_cluster_id_vec.resize(hgraph->num_vertices_);
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
  for (int v = 0; v < hgraph->num_vertices_; v++) {
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
    std::vector<float> v_wt(hgraph->vertex_dimensions_, 0.0);
    vertex_weights_c.push_back(v_wt);
  }
  if (hgraph->community_flag_ == true) {
    community_attr_c.clear();
    community_attr_c.resize(num_clusters);
    std::fill(community_attr_c.begin(), community_attr_c.end(), -1);
  }
  if (hgraph->fixed_vertex_flag_ == true) {
    fixed_attr_c.clear();
    fixed_attr_c.resize(num_clusters);
    std::fill(fixed_attr_c.begin(), fixed_attr_c.end(), -1);
  }
  if (hgraph->placement_flag_ == true) {
    placement_attr_c.clear();
    for (int v = 0; v < num_clusters; v++) {
      std::vector<float> v_placement(hgraph->placement_dimensions_, 0.0);
      placement_attr_c.push_back(v_placement);
    }
  }

  // Update the attributes of clusters
  for (int v = 0; v < hgraph->num_vertices_; v++) {
    const int cluster_id = vertex_cluster_id_vec[v];
    if (hgraph->community_flag_ == true) {
      community_attr_c[cluster_id]
          = std::max(community_attr_c[cluster_id], hgraph->community_attr_[v]);
    }
    if (hgraph->fixed_vertex_flag_ == true) {
      fixed_attr_c[cluster_id]
          = std::max(fixed_attr_c[cluster_id], hgraph->fixed_attr_[v]);
    }
    if (hgraph->placement_flag_ == true) {
      placement_attr_c[cluster_id]
          = evaluator_->GetAvgPlacementLoc(vertex_weights_c[cluster_id],
                                           hgraph->vertex_weights_[v],
                                           placement_attr_c[cluster_id],
                                           hgraph->placement_attr_[v]);
    }
    vertex_weights_c[cluster_id]
        = vertex_weights_c[cluster_id] + hgraph->vertex_weights_[v];
  }
}

// order the vertices based on user-specified parameters
void TPcoarsener::OrderVertices(const HGraphPtr& hgraph,
                                std::vector<int>& vertices) const
{
  switch (vertex_order_choice_) {
    case CoarsenOrder::RANDOM:
      shuffle(
          vertices.begin(), vertices.end(), std::default_random_engine(seed_));
      return;

    case CoarsenOrder::DEFAULT:
      return;

    case CoarsenOrder::SIZE: {
      // sort the vertices based on vertex weight
      // calculate the weight for all the vertices
      std::vector<float> average_sizes(hgraph->num_vertices_, 0.0);
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
      std::vector<int> degrees(hgraph->num_vertices_, 0);
      for (const auto& v : vertices) {
        std::set<int> nbr_vertices;
        const int first_valid_entry_h = hgraph->vptr_[v];
        const int first_invalid_entry_h = hgraph->vptr_[v + 1];
        for (int j = first_valid_entry_h; j < first_invalid_entry_h; ++j) {
          const int he = hgraph->vind_[j];
          const int first_valid_entry_v = hgraph->eptr_[he];
          const int first_invalid_entry_k = hgraph->eptr_[he + 1];
          for (int k = first_valid_entry_v; k < first_invalid_entry_k; ++k) {
            const int nbr = hgraph->eind_[k];
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
HGraphPtr TPcoarsener::Contraction(
    const HGraphPtr& hgraph,
    const std::vector<int>&
        vertex_cluster_id_vec,  // map current vertex_id to cluster_id
    // the remaining arguments are related to clusters
    const MATRIX<float>& vertex_weights_c,
    const std::vector<int>& community_attr_c,
    const std::vector<int>& fixed_attr_c,
    const MATRIX<float>& placement_attr_c) const
{
  // Step 1:  identify the contracted hyperedges
  std::vector<int> hyperedge_cluster_id_vec;  // map the hyperedge to hyperedge
                                              // in clustered hypergraph
  hyperedge_cluster_id_vec.resize(hgraph->num_hyperedges_);
  // -1 means the hyperedge is fully within one cluster
  std::fill(
      hyperedge_cluster_id_vec.begin(), hyperedge_cluster_id_vec.end(), -1);
  MATRIX<int> hyperedges_c;  // represent each hyperedge as a set of clusters
  MATRIX<float> hyperedges_weights_c;  // each element represents the weight of
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
  for (int e = 0; e < hgraph->num_hyperedges_; e++) {
    const int first_valid_entry = hgraph->eptr_[e];
    const int first_invalid_entry = hgraph->eptr_[e + 1];
    const int he_size = first_invalid_entry - first_valid_entry;
    // if (he_size <= 1 || he_size > global_net_threshold_) {
    //  continue;  // ignore the single-vertex hyperedge and large hyperedge
    //}
    if (he_size <= 1 || he_size > thr_coarsen_hyperedge_size_skip_) {
      continue;  // ignore the single-vertex hyperedge and large hyperedge
    }
    // if (he_size <= 1) {
    //  continue; // ignore the single-vertex hyperedge and large hyperedge
    //}
    std::set<int> hyperedge_c;
    for (int j = first_valid_entry; j < first_invalid_entry; ++j) {
      hyperedge_c.insert(
          vertex_cluster_id_vec[hgraph->eind_[j]]);  // get cluster id
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
      hyperedges_weights_c.push_back(hgraph->hyperedge_weights_[e]);
      if (hgraph->timing_flag_ == true) {
        hyperedge_slack_c.push_back(
            hgraph->hyperedge_timing_attr_[e]);  // the slack of hyperedge
        hyperedge_arc_set_c.push_back(
            hgraph->hyperedge_arc_set_[e]);  // map the hyperedge to timing arcs
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
      hyperedges_weights_c.push_back(hgraph->hyperedge_weights_[e]);
      if (hgraph->timing_flag_ == true) {
        hyperedge_slack_c.push_back(
            hgraph->hyperedge_timing_attr_[e]);  // the slack of hyperedge
        hyperedge_arc_set_c.push_back(
            hgraph->hyperedge_arc_set_[e]);  // map the hyperedge to timing arcs
      }
    } else {
      // existed
      hyperedges_weights_c[parallel_hyperedge_c_id]
          = hyperedges_weights_c[parallel_hyperedge_c_id]
            + hgraph->hyperedge_weights_[e];
      hyperedge_cluster_id_vec[e] = parallel_hyperedge_c_id;
      if (hgraph->timing_flag_ == true) {
        hyperedge_slack_c[parallel_hyperedge_c_id]
            = std::min(hyperedge_slack_c[parallel_hyperedge_c_id],
                       hgraph->hyperedge_timing_attr_[e]);
        hyperedge_arc_set_c[parallel_hyperedge_c_id].insert(
            hgraph->hyperedge_arc_set_[e].begin(),
            hgraph->hyperedge_arc_set_[e].end());
      }
    }
  }

  // Step 2: identify all the timing paths
  std::vector<TimingPath> timing_paths_c;
  hash_map.clear();           // used to detect parallel timing path
  parallel_hash_map.clear();  // used to detect parallel timing path
  if (hgraph->timing_flag_ == true && hgraph->num_timing_paths_ > 0) {
    for (int p = 0; p < hgraph->num_timing_paths_; ++p) {
      // check vertex representation
      const int first_valid_entry_p = hgraph->vptr_p_[p];
      const int first_invalid_entry_p = hgraph->vptr_p_[p + 1];
      const int path_size_original
          = first_invalid_entry_p - first_valid_entry_p;
      if (path_size_original <= 1) {
        continue;  // ignore single-vertex path
      }
      std::vector<int> path_c;  // create path_c
      for (int idx = first_valid_entry_p; idx < first_invalid_entry_p; idx++) {
        const int cluster_id = vertex_cluster_id_vec[hgraph->vind_p_[idx]];
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
        for (int idx = hgraph->eptr_p_[p]; idx < hgraph->eptr_p_[p + 1];
             idx++) {
          const int hyperedge_c_id
              = hyperedge_cluster_id_vec[hgraph->eind_p_[idx]];
          // if hyperedge_c_id is -1, that means that hyperedge has been merged
          // during coarsening
          if ((hyperedge_c_id > -1)
              && (arcs_c.empty() == true || arcs_c.back() != hyperedge_c_id)) {
            arcs_c.push_back(hyperedge_c_id);
          }
        }
        hash_map[hash_value] = static_cast<int>(timing_paths_c.size());
        TimingPath timing_path_c(path_c, arcs_c, hgraph->path_timing_attr_[p]);
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
          for (int idx = hgraph->eptr_p_[p]; idx < hgraph->eptr_p_[p + 1];
               idx++) {
            const int hyperedge_c_id
                = hyperedge_cluster_id_vec[hgraph->eind_p_[idx]];
            if ((hyperedge_c_id > -1)
                && (arcs_c.empty() == true
                    || arcs_c.back() != hyperedge_c_id)) {
              arcs_c.push_back(hyperedge_c_id);
            }
          }
          parallel_hash_map[hash_value].push_back(
              static_cast<int>(timing_paths_c.size()));
          timing_paths_c.emplace_back(
              path_c, arcs_c, hgraph->path_timing_attr_[p]);
        } else {
          // existed
          timing_paths_c[parallel_timing_path_c_id].slack
              = std::min(timing_paths_c[parallel_timing_path_c_id].slack,
                         hgraph->path_timing_attr_[p]);
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
      = std::make_shared<TPHypergraph>(hgraph->vertex_dimensions_,
                                       hgraph->hyperedge_dimensions_,
                                       hgraph->placement_dimensions_,
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

  // fill ertex_c_attr which maps the vertex to its corresponding cluster
  // To simpify the implementation, the vertex_c_attr maps the original larger
  // hypergraph
  auto& vertex_c_attr = clustered_hgraph->vertex_c_attr_;
  vertex_c_attr.clear();
  vertex_c_attr.resize(clustered_hgraph->num_vertices_);
  for (int v = 0; v < hgraph->num_vertices_; v++) {
    vertex_c_attr[vertex_cluster_id_vec[v]].push_back(v);
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
